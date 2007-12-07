/*
 * MPEG2 transport stream (aka DVB) muxer
 * Copyright (c) 2003 Fabrice Bellard.
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */
#include "avformat.h"
#include "crc.h"
#include "mpegts.h"

/* write DVB SI sections */

/*********************************************/
/* mpegts section writer */

typedef struct MpegTSSection {
    int pid;
    int cc;
    void (*write_packet)(struct MpegTSSection *s, const uint8_t *packet);
    void *opaque;
} MpegTSSection;

/*********************************************/
/* mpegts writer */

#define DEFAULT_PMT_START_PID   0x1000
#define DEFAULT_START_PID       0x0100
#define DEFAULT_PROVIDER_NAME   "FFmpeg"
#define DEFAULT_SERVICE_NAME    "Service01"

/* default network id, transport stream and service identifiers */
#define DEFAULT_ONID            0x0001
#define DEFAULT_TSID            0x0001
#define DEFAULT_SID             0x0001

/* a PES packet header is generated every DEFAULT_PES_HEADER_FREQ packets */
#define DEFAULT_PES_HEADER_FREQ 2000
#define DEFAULT_PES_PAYLOAD_SIZE ((DEFAULT_PES_HEADER_FREQ - 1) * 184 + 170)

/* we retransmit the SI info at this rate */
#define SDT_RETRANS_TIME 500
#define PAT_RETRANS_TIME 100
#define PCR_RETRANS_TIME 20

typedef struct MpegTSWriteStream {
    struct MpegTSService *service;
    int pid; /* stream associated pid */
    int cc;
    int payload_index;
    int64_t payload_pts;
    int64_t payload_dts;
    uint8_t payload[DEFAULT_PES_PAYLOAD_SIZE];
    // Size of the payload (frame usually)
    int     payload_size;
    // Number of frames allowed to be in sequence without interleave
    int     interleave_count;
    int     max_interleave_count;
    int     dummy;
    int     packets_count;
    int64_t initial_pts;
} MpegTSWriteStream;

typedef struct MpegTSService {
    MpegTSSection pmt; /* MPEG2 pmt table context */
    int sid;           /* service ID */
    char *name;
    char *provider_name;
    int pcr_pid;
    int64_t pcr_packet_count;
    // How frequent frames should be having PCR
    int pcr_packet_freq;
    // How much PCR should be increased every packet 
    int pcr_packet_delta;
    // Number of frames written for the service
    int64_t frames;
} MpegTSService;

typedef struct MpegTSWrite {
    MpegTSSection pat; /* MPEG2 pat table */
    MpegTSSection sdt; /* MPEG2 sdt table context */
    MpegTSService **services;
    int sdt_packet_count;
    int sdt_packet_freq;
    int pat_packet_count;
    int pat_packet_freq;
    int nb_services;
    int onid;
    int tsid;
} MpegTSWrite;

/* NOTE: 4 bytes must be left at the end for the crc32 */
static void mpegts_write_section(MpegTSSection *s, uint8_t *buf, int len, MpegTSService* service)
{
    unsigned int crc;
    unsigned char packet[TS_PACKET_SIZE];
    const unsigned char *buf_ptr;
    unsigned char *q;
    int first, b, len1, left;

    crc = bswap_32(av_crc(av_crc04C11DB7, -1, buf, len - 4));
    buf[len - 4] = (crc >> 24) & 0xff;
    buf[len - 3] = (crc >> 16) & 0xff;
    buf[len - 2] = (crc >> 8) & 0xff;
    buf[len - 1] = (crc) & 0xff;

    /* send each packet */
    buf_ptr = buf;
    while (len > 0) {
        first = (buf == buf_ptr);
        q = packet;
        service->pcr_packet_count++;
        *q++ = 0x47;
        b = (s->pid >> 8);
        if (first)
            b |= 0x40;
        *q++ = b;
        *q++ = s->pid;
        s->cc = (s->cc + 1) & 0xf;
        *q++ = 0x10 | s->cc;
        if (first)
            *q++ = 0; /* 0 offset */
        len1 = TS_PACKET_SIZE - (q - packet);
        if (len1 > len)
            len1 = len;
        memcpy(q, buf_ptr, len1);
        q += len1;
        /* add known padding data */
        left = TS_PACKET_SIZE - (q - packet);
        if (left > 0)
            memset(q, 0xff, left);

        s->write_packet(s, packet);

        buf_ptr += len1;
        len -= len1;
    }
}

static inline void put16(uint8_t **q_ptr, int val)
{
    uint8_t *q;
    q = *q_ptr;
    *q++ = val >> 8;
    *q++ = val;
    *q_ptr = q;
}

static int mpegts_write_section1(MpegTSSection *s, int tid, int id,
                          int version, int sec_num, int last_sec_num,
                          uint8_t *buf, int len, MpegTSService* service)
{
    uint8_t section[1024], *q;
    unsigned int tot_len;

    tot_len = 3 + 5 + len + 4;
    /* check if not too big */
    if (tot_len > 1024)
        return -1;

    q = section;
    *q++ = tid;
    // private_indicator= 1
    put16(&q, ( tid== SDT_TID ? 0xf000: 0xb000 ) | (len + 5 + 4)); /* 5 byte header + 4 byte CRC */
    put16(&q, id);
    *q++ = 0xc1 | (version << 1); /* current_next_indicator = 1 */
    *q++ = sec_num;
    *q++ = last_sec_num;
    memcpy(q, buf, len);

    mpegts_write_section(s, section, tot_len, service);
    return 0;
}

static void mpegts_write_pat(AVFormatContext *s)
{
    MpegTSWrite *ts = s->priv_data;
    MpegTSService *service;
    uint8_t data[1012], *q;
    int i;

    q = data;
    for(i = 0; i < ts->nb_services; i++) {
        service = ts->services[i];
        put16(&q, service->sid);
        put16(&q, 0xe000 | service->pmt.pid);
    }
    mpegts_write_section1(&ts->pat, PAT_TID, ts->tsid, 0, 0, 0,
                          data, q - data, service);
}

static void mpegts_write_pmt(AVFormatContext *s, MpegTSService *service)
{
    //    MpegTSWrite *ts = s->priv_data;
    uint8_t data[1012], *q, *desc_length_ptr, *program_info_length_ptr;
    int val, stream_type, i;

    q = data;
    put16(&q, 0xe000 | service->pcr_pid);

    program_info_length_ptr = q;
    q += 2; /* patched after */

    /* put program info here */

    val = 0xf000 | (q - program_info_length_ptr - 2);
    program_info_length_ptr[0] = val >> 8;
    program_info_length_ptr[1] = val;

    for(i = 0; i < s->nb_streams; i++) {
        AVStream *st = s->streams[i];
        MpegTSWriteStream *ts_st = st->priv_data;
        switch(st->codec.codec_id) {
        case CODEC_ID_MPEG1VIDEO:
        case CODEC_ID_MPEG2VIDEO:
            stream_type = STREAM_TYPE_VIDEO_MPEG2;
            break;
        case CODEC_ID_MPEG4:
            stream_type = STREAM_TYPE_VIDEO_MPEG4;
            break;
        case CODEC_ID_H264:
            stream_type = STREAM_TYPE_VIDEO_H264;
            break;
        case CODEC_ID_MP2:
        case CODEC_ID_MP3:
            stream_type = STREAM_TYPE_AUDIO_MPEG1;
            break;
        case CODEC_ID_AAC:
            stream_type = STREAM_TYPE_AUDIO_AAC;
            break;
        case CODEC_ID_AC3:
            stream_type = STREAM_TYPE_AUDIO_AC3;
            break;
        default:
            stream_type = STREAM_TYPE_PRIVATE_DATA;
            break;
        }
        *q++ = stream_type;
        put16(&q, 0xe000 | ts_st->pid);
        desc_length_ptr = q;
        q += 2; /* patched after */

        /* write optional descriptors here */
        switch(st->codec.codec_type) {
        case CODEC_TYPE_AUDIO:
            if (strlen(st->language) == 3) {
                *q++ = 0x0a; /* ISO 639 language descriptor */
                *q++ = 4;
                *q++ = st->language[0];
                *q++ = st->language[1];
                *q++ = st->language[2];
                *q++ = 0; /* undefined type */
            }
            break;
        case CODEC_TYPE_SUBTITLE:
            {
                const char *language;
                language = st->language;
                if (strlen(language) != 3)
                    language = "eng";
                *q++ = 0x59;
                *q++ = 8;
                *q++ = language[0];
                *q++ = language[1];
                *q++ = language[2];
                *q++ = 0x10; /* normal subtitles (0x20 = if hearing pb) */
                put16(&q, 1); /* page id */
                put16(&q, 1); /* ancillary page id */
            }
            break;
        }

        val = 0xf000 | (q - desc_length_ptr - 2);
        desc_length_ptr[0] = val >> 8;
        desc_length_ptr[1] = val;
    }
    mpegts_write_section1(&service->pmt, PMT_TID, service->sid, 0, 0, 0,
                          data, q - data, service);
}

/* NOTE: str == NULL is accepted for an empty string */
static void putstr8(uint8_t **q_ptr, const char *str)
{
    uint8_t *q;
    int len;

    q = *q_ptr;
    if (!str)
        len = 0;
    else
        len = strlen(str);
    *q++ = len;
    memcpy(q, str, len);
    q += len;
    *q_ptr = q;
}

static void mpegts_write_sdt(AVFormatContext *s)
{
    MpegTSWrite *ts = s->priv_data;
    MpegTSService *service;
    uint8_t data[1012], *q, *desc_list_len_ptr, *desc_len_ptr;
    int i, running_status, free_ca_mode, val;

    q = data;
    put16(&q, ts->onid);
    *q++ = 0xff;
    for(i = 0; i < ts->nb_services; i++) {
        service = ts->services[i];
        put16(&q, service->sid);
        *q++ = 0xfc | 0x00; /* currently no EIT info */
        desc_list_len_ptr = q;
        q += 2;
        running_status = 4; /* running */
        free_ca_mode = 0;

        /* write only one descriptor for the service name and provider */
        *q++ = 0x48;
        desc_len_ptr = q;
        q++;
        *q++ = 0x01; /* digital television service */
        putstr8(&q, service->provider_name);
        putstr8(&q, service->name);
        desc_len_ptr[0] = q - desc_len_ptr - 1;

        /* fill descriptor length */
        val = (running_status << 13) | (free_ca_mode << 12) |
            (q - desc_list_len_ptr - 2);
        desc_list_len_ptr[0] = val >> 8;
        desc_list_len_ptr[1] = val;
    }
    mpegts_write_section1(&ts->sdt, SDT_TID, ts->tsid, 0, 0, 0,
                          data, q - data, service);
}

static MpegTSService *mpegts_add_service(MpegTSWrite *ts,
                                         int sid,
                                         const char *provider_name,
                                         const char *name)
{
    MpegTSService *service;

    service = av_mallocz(sizeof(MpegTSService));
    if (!service)
        return NULL;
    service->pmt.pid = DEFAULT_PMT_START_PID + ts->nb_services- 1; 
    service->sid = sid;
    service->provider_name = av_strdup(provider_name);
    service->name = av_strdup(name);
    service->pcr_pid = 0x1fff;
    dynarray_add(&ts->services, &ts->nb_services, service);
    return service;
}

static void section_write_packet(MpegTSSection *s, const uint8_t *packet)
{
    AVFormatContext *ctx = s->opaque;
    put_buffer(&ctx->pb, packet, TS_PACKET_SIZE);
}

static int mpegts_write_header(AVFormatContext *s)
{
    MpegTSWrite *ts = s->priv_data;
    MpegTSWriteStream *ts_st;
    MpegTSService *service;
    AVStream *st;
    int i, total_bit_rate, min_bit_rate= 9999999999;
    const char *service_name;

    ts->tsid = DEFAULT_TSID;
    ts->onid = DEFAULT_ONID;
    /* allocate a single DVB service */
    service_name = s->title;
    if (service_name[0] == '\0')
        service_name = DEFAULT_SERVICE_NAME;
    service = mpegts_add_service(ts, DEFAULT_SID,
                                 DEFAULT_PROVIDER_NAME, service_name);
    service->pmt.write_packet = section_write_packet;
    service->pmt.opaque = s;

    ts->pat.pid = PAT_PID;
    ts->pat.cc = 0;
    ts->pat.write_packet = section_write_packet;
    ts->pat.opaque = s;

    ts->sdt.pid = SDT_PID;
    ts->sdt.cc = 0;
    ts->sdt.write_packet = section_write_packet;
    ts->sdt.opaque = s;

    /* assign pids to each stream */
    total_bit_rate = 0;
    for(i = 0;i < s->nb_streams; i++) {
        st = s->streams[i];
        ts_st = av_mallocz(sizeof(MpegTSWriteStream));
        if (!ts_st)
            goto fail;
        st->priv_data = ts_st;
        ts_st->service = service;
        ts_st->pid = DEFAULT_START_PID + i;
        ts_st->payload_pts = AV_NOPTS_VALUE;
        ts_st->payload_dts = AV_NOPTS_VALUE;
        ts_st->initial_pts = AV_NOPTS_VALUE;
        /* update PCR pid by using the first video stream */
        if (st->codec.codec_type == CODEC_TYPE_VIDEO &&
            service->pcr_pid == 0x1fff)
            service->pcr_pid = ts_st->pid;
        total_bit_rate += st->codec.bit_rate;

        if( min_bit_rate> st->codec.bit_rate )
          min_bit_rate= st->codec.bit_rate;
    }

    // Calculate interleaving factor
    for(i = 0;i < s->nb_streams; i++) 
    {
      MpegTSWriteStream* ts_st = (MpegTSWriteStream*)s->streams[i]->priv_data;

      ts_st->max_interleave_count= 
      ts_st->interleave_count= 
        s->streams[i]->codec.bit_rate / min_bit_rate;
    }

    /* if no video stream, use the first stream as PCR */
    if (service->pcr_pid == 0x1fff && s->nb_streams > 0) {
        ts_st = s->streams[0]->priv_data;
        service->pcr_pid = ts_st->pid;
    }

    if (total_bit_rate <= 8 * 1024)
        total_bit_rate = 8 * 1024;
    s->bit_rate= ( total_bit_rate* 105 )/ 100;
    service->pcr_packet_delta= 27000000/ ( s->bit_rate / (TS_PACKET_SIZE* 8));
    service->pcr_packet_freq = (total_bit_rate * PCR_RETRANS_TIME) /
        (TS_PACKET_SIZE * 8 * 1000);
    ts->sdt_packet_freq = (total_bit_rate * SDT_RETRANS_TIME) /
        (TS_PACKET_SIZE * 8 * 1000);
    ts->pat_packet_freq = (total_bit_rate * PAT_RETRANS_TIME) /
        (TS_PACKET_SIZE * 8 * 1000);
#if 0
    printf("%d %d %d\n",
           total_bit_rate, ts->sdt_packet_freq, ts->pat_packet_freq);
#endif

    /* write info at the start of the file, so that it will be fast to
       find them */
    mpegts_write_sdt(s);
    mpegts_write_pat(s);
    for(i = 0; i < ts->nb_services; i++) {
        mpegts_write_pmt(s, ts->services[i]);
    }
    put_flush_packet(&s->pb);

    return 0;

 fail:
    for(i = 0;i < s->nb_streams; i++) {
        st = s->streams[i];
        av_free(st->priv_data);
    }
    return -1;
}

/* send SDT, PAT and PMT tables regulary */
static void retransmit_si_info(AVFormatContext *s)
{
    MpegTSWrite *ts = s->priv_data;
    int i;

    if (++ts->sdt_packet_count == ts->sdt_packet_freq) {
        ts->sdt_packet_count = 0;
        mpegts_write_sdt(s);
    }
    if (++ts->pat_packet_count == ts->pat_packet_freq) {
        ts->pat_packet_count = 0;
        mpegts_write_pat(s);
        for(i = 0; i < ts->nb_services; i++) {
            mpegts_write_pmt(s, ts->services[i]);
        }
    }
}

static void write_pts(uint8_t *q, int fourbits, int64_t pts)
{
    int val;

    val = fourbits << 4 | (((pts >> 30) & 0x07) << 1) | 1;
    *q++ = val;
    val = (((pts >> 15) & 0x7fff) << 1) | 1;
    *q++ = val >> 8;
    *q++ = val;
    val = (((pts) & 0x7fff) << 1) | 1;
    *q++ = val >> 8;
    *q++ = val;
}

/* NOTE: add dummy data to align bitrates */
static void mpegts_write_dummy(AVFormatContext *s, MpegTSWriteStream *ts_st)
{
  uint8_t buf[TS_PACKET_SIZE];
  uint8_t *q= buf;

  // Create stuffing packet to keep up with framerate
  ts_st->service->pcr_packet_count++;
  ts_st->packets_count++;
  *q++ = 0x47;
  *q++ = 0x1f;
  *q++ = 0xff;
  *q++ = 0x10;
  memset( q, 0, 184 );
  put_buffer(&s->pb, buf, TS_PACKET_SIZE);
}

/* Write atomic frame with PTS/PCR if needed */
static int mpegts_write_atomic_packet(
    AVFormatContext *s, 
    AVStream *st,
    MpegTSWriteStream *ts_st,
    int64_t pts, 
    int64_t dts, 
    int is_start, 
    int bForce )
{
    uint8_t buf[TS_PACKET_SIZE];
    uint8_t *q;
    int val, len, header_len, write_pcr, private_code, flags, stuff_int= 0, stuff_temp= 0;
    int afc_len, stuffing_len;
    int64_t pcr = -1; /* avoid warning */

    // No data to write. Do not create a packet
    if( ts_st->payload_size- ts_st->payload_index== 0 )
      return 0;

    retransmit_si_info(s);
    write_pcr = 0;
    if (ts_st->pid == ts_st->service->pcr_pid) 
    {
      // See if we need dummy frame to keep up with the bitrate
      if( pts!= AV_NOPTS_VALUE )
      {
        // Get number of packets to generate the known payload
        int64_t count;
        ts_st->service->frames++;
        if( ts_st->initial_pts== AV_NOPTS_VALUE )
          ts_st->initial_pts= pts;

        count= (( pts- ts_st->initial_pts )* 300 )/ ts_st->service->pcr_packet_delta;
        if( count> ts_st->service->pcr_packet_count )
          ts_st->dummy= count- ts_st->service->pcr_packet_count;
//printf( "%I64d %d %d\n", pts, ts_st->service->pcr_packet_delta, ts_st->dummy );
      }

      // BORS: Code modified to allow PCR writing
      if (ts_st->packets_count >= ts_st->service->pcr_packet_freq || is_start )
      {
        write_pcr = 1;
        pcr= ts_st->service->pcr_packet_delta* 
             ts_st->service->pcr_packet_count;
        ts_st->packets_count= 0;
      }
    }
    /* prepare packet header */
    q = buf;
    *q++ = 0x47;
    val = (ts_st->pid >> 8);
    if (is_start)
        val |= 0x40;
    *q++ = val;
    *q++ = ts_st->pid;
    *q++ = 0x10 | ts_st->cc | (write_pcr ? 0x20 : 0);
    ts_st->cc = (ts_st->cc + 1) & 0xf;
    if (write_pcr) {
        int64_t pcr_base;
        int64_t pcr_ext;

        pcr_base = (pcr / 300);
        pcr_ext = (pcr % 300);
        *q++ = 7; /* AFC length */
        *q++ = 0x10; /* flags: PCR present */
        *q++ = pcr_base >> 25;
        *q++ = pcr_base >> 17;
        *q++ = pcr_base >> 9;
        *q++ = pcr_base >> 1;
        *q++ = (((pcr_base & 1) << 7 ) & 0x80 ) | 0x7e | ((pcr_ext >> 8) & 0x01);
        *q++ = (pcr_ext) & 0xff;
    }
    if (is_start) {
        /* write PES header */
        *q++ = 0x00;
        *q++ = 0x00;
        *q++ = 0x01;
        private_code = 0;
        if (st->codec.codec_type == CODEC_TYPE_VIDEO) {
            *q++ = 0xe0;
        } else if (st->codec.codec_type == CODEC_TYPE_AUDIO &&
                   (st->codec.codec_id == CODEC_ID_MP2 ||
                    st->codec.codec_id == CODEC_ID_MP3)) {
            *q++ = 0xc0;
        } else {
            *q++ = 0xbd;
            if (st->codec.codec_type == CODEC_TYPE_SUBTITLE) {
                private_code = 0x20;
            }
        }
        header_len = 0;
        flags = 0;
        if (pts != AV_NOPTS_VALUE) {
            header_len += 5;
            flags |= 0x80;
        }
        if (dts != AV_NOPTS_VALUE) {
            header_len += 5;
            flags |= 0x40;
        }
        len = ts_st->payload_size- ts_st->payload_index+ header_len + 3;
        if (private_code != 0)
            len++;

        // BORS: Do not want to write the len for PCR stream
        if( ts_st->pid == ts_st->service->pcr_pid )
          len= 0;

        *q++ = len >> 8;
        *q++ = len;
        val = 0x80;
        /* data alignment indicator is required for subtitle data */
        if (st->codec.codec_type == CODEC_TYPE_SUBTITLE)
            val |= 0x04;
        *q++ = val;
        *q++ = flags;
        *q++ = header_len;
        if (pts != AV_NOPTS_VALUE) {
            write_pts(q, flags >> 6, pts);
            q += 5;
        }
        if (dts != AV_NOPTS_VALUE) {
            write_pts(q, 1, dts);
            q += 5;
        }
        if (private_code != 0)
            *q++ = private_code;
        is_start = 0;
    }
    /* header size */
    header_len = q - buf;
    /* data len */
    len = TS_PACKET_SIZE - header_len;
    if (len > ts_st->payload_size- ts_st->payload_index)
      len = ts_st->payload_size- ts_st->payload_index;

    stuffing_len = TS_PACKET_SIZE - header_len - len;

    if( !bForce && stuffing_len> 0 )
    {
      ts_st->cc = (ts_st->cc - 1) & 0xf;
      // No data and force flag is not set
      return 0;
    }

    // Increase number of packets in total
    ts_st->service->pcr_packet_count++;
    // Increase number of packets in for this particular stream to 
    ts_st->packets_count++;
    // Decrease interleave counter
    if( ts_st->interleave_count )
      ts_st->interleave_count--;

    if (stuffing_len > 0) {
        /* add stuffing with AFC */
        if (buf[3] & 0x20) {
            /* stuffing already present: increase its size */
            afc_len = buf[4] + 1;
            memmove(buf + 4 + afc_len + stuffing_len,
                    buf + 4 + afc_len,
                    header_len - (4 + afc_len));
            buf[4] += stuffing_len;
            memset(buf + 4 + afc_len, 0xff, stuffing_len);
        } else {
            /* add stuffing */
            memmove(buf + 4 + stuffing_len, buf + 4, header_len - 4);
            buf[3] |= 0x20;
            buf[4] = stuffing_len - 1;
            if (stuffing_len >= 2) {
                buf[5] = 0x00;
                memset(buf + 6, 0xff, stuffing_len - 2);
            }
        }
    }
    memcpy(buf + TS_PACKET_SIZE - len, ts_st->payload+ ts_st->payload_index, len);
    ts_st->payload_index+= len;
    put_buffer(&s->pb, buf, TS_PACKET_SIZE);
    put_flush_packet(&s->pb);
    return 1;
}

static int mpegts_write_packet(AVFormatContext *s, AVPacket *pkt)
{
    AVStream *st = s->streams[pkt->stream_index];
    int size= pkt->size, bNoData= -1;
    uint8_t *buf= pkt->data;
    MpegTSWriteStream *ts_st = st->priv_data;
    int i, j, len, max_payload_size;

    // BORS: Support for subtitle disabled for now
    //if (st->codec.codec_type == CODEC_TYPE_SUBTITLE) {
        /* for subtitle, a single PES packet must be generated */
    //    mpegts_write_atomic_packet(s, ts_st, buf, size, pkt->pts, AV_NOPTS_VALUE);
    //    return 0;
    //} 

    max_payload_size = DEFAULT_PES_PAYLOAD_SIZE;
 
    /* BORS: Code modified to have correct PTS generated */
    if( pkt->pts== AV_NOPTS_VALUE )
    {
      len = max_payload_size - ts_st->payload_index- ts_st->payload_size;
      if (len > size)
        len = size;
      memcpy(ts_st->payload + ts_st->payload_size, buf, len);
      ts_st->payload_size+= len;
      // Wait till the whole frame come in
      return 0;
    }

    // Scan all other streams to see if we have some interleaved data left
    while( bNoData== -1 )
      for( i= 0; i< s->nb_streams && bNoData== -1; i++ )
      {
        MpegTSWriteStream *ts_st_tmp = s->streams[ i ]->priv_data;
        int bNoDataTmp= -1;
        while( ts_st_tmp->interleave_count )
        {
          if( !mpegts_write_atomic_packet( 
            s, 
            s->streams[ i ], 
            ts_st_tmp, 
            ts_st_tmp->payload_pts, 
            ts_st_tmp->payload_dts, 
            ts_st_tmp->payload_index== 0, 
            // Force only if we came already
            i== pkt->stream_index ) )
          {
            // Only make no data signal when PCR stream
            if( ts_st_tmp->pid== ts_st_tmp->service->pcr_pid )
              bNoDataTmp= i;

            // It's time to reset the buffer
            if( ts_st->payload_size== ts_st->payload_index )
              ts_st->payload_size= ts_st->payload_index= 0;

            break;
          }
          else
            ts_st_tmp->payload_pts= ts_st_tmp->payload_dts= AV_NOPTS_VALUE;

          // Check if we need dummy packets to maintain bitrate
          j= 10;
          while( ts_st_tmp->dummy && j )
          {
  //printf( "dummy %d\n", ts_st_tmp->dummy );
            mpegts_write_dummy(s, ts_st_tmp);
            ts_st_tmp->dummy--;
            j--;
          }
        }

        // Restore interleave counter when data still exists
        if( bNoDataTmp== -1 )
          ts_st_tmp->interleave_count= ts_st_tmp->max_interleave_count;
        else
          bNoData= bNoDataTmp;
      }

    // Reset payload buffer
    while( ts_st->payload_size!= ts_st->payload_index )
      mpegts_write_atomic_packet( s, st, ts_st, AV_NOPTS_VALUE, AV_NOPTS_VALUE, ts_st->payload_index== 0, 1 );

    // Fill up the buffer wiuth the new frame start
    memcpy(ts_st->payload, buf, size);
    ts_st->payload_index= 0;
    ts_st->payload_size= size;
    ts_st->payload_pts= pkt->pts;
    ts_st->payload_dts= pkt->dts;
    return 0;
}

static int mpegts_write_end(AVFormatContext *s)
{
    MpegTSWrite *ts = s->priv_data;
    MpegTSWriteStream *ts_st;
    MpegTSService *service;
    AVStream *st;
    int i;

    /* flush current packets */
    for(i = 0; i < s->nb_streams; i++) {
        st = s->streams[i];
        ts_st = st->priv_data; 
        if (ts_st->payload_size- ts_st->payload_index > 0) {
            while( mpegts_write_atomic_packet(s, st, ts_st, AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0, 1 ));
        }
    }
    put_flush_packet(&s->pb);

    for(i = 0; i < ts->nb_services; i++) {
        service = ts->services[i];
        av_freep(&service->provider_name);
        av_freep(&service->name);
        av_free(service);
    }
    av_free(ts->services);

    return 0;
}

AVOutputFormat mpegts_muxer = {
    "mpegts",
    "MPEG2 transport stream format",
    "video/x-mpegts",
    "ts",
    sizeof(MpegTSWrite),
    CODEC_ID_MP2,
    CODEC_ID_MPEG2VIDEO,
    mpegts_write_header,
    mpegts_write_packet,
    mpegts_write_end,
};

int mpegtsenc_init(void)
{
    av_register_output_format(&mpegts_muxer);
    return 0;
}
