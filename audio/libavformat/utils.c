/*
 * Various utilities for ffmpeg system
 * Copyright (c) 2000, 2001, 2002 Fabrice Bellard
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include "avformat.h"
#include <ctype.h>
#ifndef CONFIG_WIN32
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#else
#define strcasecmp _stricmp
#include <sys/types.h>
#include <sys/timeb.h>
#endif
#include <time.h>

AVInputFormat *first_iformat;
AVOutputFormat *first_oformat;

void av_register_input_format(AVInputFormat *format)
{
    AVInputFormat **p;
    p = &first_iformat;
    while (*p != NULL) p = &(*p)->next;
    *p = format;
    format->next = NULL;
}

void av_register_output_format(AVOutputFormat *format)
{
    AVOutputFormat **p;
    p = &first_oformat;
    while (*p != NULL) p = &(*p)->next;
    *p = format;
    format->next = NULL;
}

int match_ext(const char *filename, const char *extensions)
{
    const char *ext, *p;
    char ext1[32], *q;

    ext = strrchr(filename, '.');
    if (ext) {
        ext++;
        p = extensions;
        for(;;) {
            q = ext1;
            while (*p != '\0' && *p != ',')
                *q++ = *p++;
            *q = '\0';
            if (!strcasecmp(ext1, ext))
                return 1;
            if (*p == '\0')
                break;
            p++;
        }
    }
    return 0;
}

AVOutputFormat *guess_format(const char *short_name, const char *filename,
                             const char *mime_type)
{
    AVOutputFormat *fmt, *fmt_found;
    int score_max, score;

    /* find the proper file type */
    fmt_found = NULL;
    score_max = 0;
    fmt = first_oformat;
    while (fmt != NULL) {
        score = 0;
        if (fmt->name && short_name && !strcmp(fmt->name, short_name))
            score += 100;
        if (fmt->mime_type && mime_type && !strcmp(fmt->mime_type, mime_type))
            score += 10;
        if (filename && fmt->extensions &&
            match_ext(filename, fmt->extensions)) {
            score += 5;
        }
        if (score > score_max) {
            score_max = score;
            fmt_found = fmt;
        }
        fmt = fmt->next;
    }
    return fmt_found;
}

AVOutputFormat *guess_stream_format(const char *short_name, const char *filename,
                             const char *mime_type)
{
    AVOutputFormat *fmt = guess_format(short_name, filename, mime_type);

    if (fmt) {
        AVOutputFormat *stream_fmt;
        char stream_format_name[64];

        snprintf(stream_format_name, sizeof(stream_format_name), "%s_stream", fmt->name);
        stream_fmt = guess_format(stream_format_name, NULL, NULL);

        if (stream_fmt)
            fmt = stream_fmt;
    }

    return fmt;
}

AVInputFormat *guess_input_format(const char *filename)
{
    AVInputFormat *fmt;
    for(fmt = first_iformat; fmt != NULL; fmt = fmt->next) {
        if ( match_ext(filename, fmt->extensions))
            return fmt;
    }
    return NULL;
}

AVInputFormat *av_find_input_format(const char *short_name)
{
    AVInputFormat *fmt;
    for(fmt = first_iformat; fmt != NULL; fmt = fmt->next) {
        if (!strcmp(fmt->name, short_name))
            return fmt;
    }
    return NULL;
}

/* memory handling */

/**
 * Allocate the payload of a packet and intialized its fields to default values.
 *
 * @param pkt packet
 * @param size wanted payload size
 * @return 0 if OK. AVERROR_xxx otherwise.
 */
int av_new_packet(AVPacket *pkt, int size)
{
    int i;
		if( pkt->data && pkt->size< size )
				av_free_packet( pkt );

		if( !pkt->data )
			pkt->data = av_malloc(size + FF_INPUT_BUFFER_PADDING_SIZE);

    if (!pkt->data)
        return AVERROR_NOMEM;
    pkt->size = ( pkt->size> size ) ? pkt->size: size;
    /* sane state */
    pkt->pts = AV_NOPTS_VALUE;
    pkt->stream_index = 0;
    pkt->flags = 0;

    for(i=0; i<FF_INPUT_BUFFER_PADDING_SIZE; i++)
        pkt->data[size+i]= 0;

    return 0;
}

/**
 * Free a packet
 *
 * @param pkt packet to free
 */
void av_free_packet(AVPacket *pkt)
{
    av_freep(&pkt->data);
    /* fail safe */
    pkt->size = 0;
}

int filename_number_test(const char *filename)
{
    char buf[1024];
    return get_frame_filename(buf, sizeof(buf), filename, 1);
}

/* guess file format */
AVInputFormat *av_probe_input_format(AVProbeData *pd, int is_opened)
{
    AVInputFormat *fmt1, *fmt;
    int score, score_max;

    fmt = NULL;
    score_max = 0;
    for(fmt1 = first_iformat; fmt1 != NULL; fmt1 = fmt1->next) {
        if (!is_opened && !(fmt1->flags & AVFMT_NOFILE))
            continue;
        score = 0;
        if (fmt1->read_probe) {
            score = fmt1->read_probe(pd);
        } else if (fmt1->extensions) {
            if (match_ext(pd->filename, fmt1->extensions)) {
                score = 50;
            }
        }
        if (score > score_max) {
            score_max = score;
            fmt = fmt1;
        }
    }
    return fmt;
}

/************************************************************/
/* input media file */

#define PROBE_BUF_SIZE 2048

/**
 * Read a packet from a media file
 * @param s media file handle
 * @param pkt is filled
 * @return 0 if OK. AVERROR_xxx if error.
 */
int av_read_packet(AVFormatContext *s, AVPacket *pkt)
{
    AVPacketList *pktl;

    pktl = s->packet_buffer;
    if (pktl) {
        /* read packet from packet buffer, if there is data */
        *pkt = pktl->pkt;
        s->packet_buffer = pktl->next;
        av_free(pktl);
        return 0;
    } else {
        return s->iformat->read_packet(s, pkt);
    }
}

/* state for codec information */
#define CSTATE_NOTFOUND    0
#define CSTATE_DECODING    1
#define CSTATE_FOUND       2

static int has_codec_parameters(AVCodecContext *enc)
{
    int val;
    switch(enc->codec_type) {
    case CODEC_TYPE_AUDIO:
        val = enc->sample_rate;
        break;
    case CODEC_TYPE_VIDEO:
        val = enc->width;
        break;
    default:
        val = 1;
        break;
    }
    return (val != 0);
}

/**
 * Close a media file (but not its codecs)
 *
 * @param s media file handle
 */
void av_close_input_file(AVFormatContext *s)
{
    if (s->packet_buffer) {
        AVPacketList *p, *p1;
        p = s->packet_buffer;
        while (p != NULL) {
            p1 = p->next;
            av_free_packet(&p->pkt);
            av_free(p);
            p = p1;
        }
        s->packet_buffer = NULL;
    }
    if (s->iformat && s->iformat->read_close)
        s->iformat->read_close(s);

    av_freep(&s->priv_data);
    //av_free(s);
}

/**
 * Add a new stream to a media file. Can only be called in the
 * read_header function. If the flag AVFMT_NOHEADER is in the format
 * description, then new streams can be added in read_packet too.
 *
 *
 * @param s media file handle
 * @param id file format dependent stream id
 */
AVStream *av_new_stream(AVFormatContext *s, int id)
{
    AVStream *st;

    if (s->nb_streams >= MAX_STREAMS)
        return NULL;

    st = av_mallocz(sizeof(AVStream));
    if (!st)
        return NULL;
    avcodec_get_context_defaults(&st->codec);

    st->index = s->nb_streams;
    st->id = id;
    s->streams[s->nb_streams++] = st;
    return st;
}

/* Return in 'buf' the path with '%d' replaced by number. Also handles
   the '%0nd' format where 'n' is the total number of digits and
   '%%'. Return 0 if OK, and -1 if format error */
int get_frame_filename(char *buf, int buf_size,
                       const char *path, int number)
{
    const char *p;
    char *q, buf1[20];
    int nd, len, c, percentd_found;

    q = buf;
    p = path;
    percentd_found = 0;
    for(;;) {
        c = *p++;
        if (c == '\0')
            break;
        if (c == '%') {
            do {
                nd = 0;
                while (isdigit(*p)) {
                    nd = nd * 10 + *p++ - '0';
                }
                c = *p++;
                if (c == '*' && nd > 0) {
                    // The nd field is actually the modulus
                    number = number % nd;
                    c = *p++;
                    nd = 0;
                }
            } while (isdigit(c));

            switch(c) {
            case '%':
                goto addchar;
            case 'd':
                if (percentd_found)
                    goto fail;
                percentd_found = 1;
                snprintf(buf1, sizeof(buf1), "%0*d", nd, number);
                len = strlen(buf1);
                if ((q - buf + len) > buf_size - 1)
                    goto fail;
                memcpy(q, buf1, len);
                q += len;
                break;
            default:
                goto fail;
            }
        } else {
        addchar:
            if ((q - buf) < buf_size - 1)
                *q++ = c;
        }
    }
    if (!percentd_found)
        goto fail;
    *q = '\0';
    return 0;
 fail:
    *q = '\0';
    return -1;
}

/**
 *
 * Print on stdout a nice hexa dump of a buffer
 * @param buf buffer
 * @param size buffer size
 */
void av_hex_dump(UINT8 *buf, int size)
{
    int len, i, j, c;

    for(i=0;i<size;i+=16) {
        len = size - i;
        if (len > 16)
            len = 16;
        printf("%08x ", i);
        for(j=0;j<16;j++) {
            if (j < len)
                printf(" %02x", buf[i+j]);
            else
                printf("   ");
        }
        printf(" ");
        for(j=0;j<len;j++) {
            c = buf[i+j];
            if (c < ' ' || c > '~')
                c = '.';
            printf("%c", c);
        }
        printf("\n");
    }
}


/**
 * Set the pts for a given stream
 * @param s stream
 * @param pts_wrap_bits number of bits effectively used by the pts
 *        (used for wrap control, 33 is the value for MPEG)
 * @param pts_num numerator to convert to seconds (MPEG: 1)
 * @param pts_den denominator to convert to seconds (MPEG: 90000)
 */
void av_set_pts_info(AVFormatContext *s, int pts_wrap_bits,
                     int pts_num, int pts_den)
{
    s->pts_wrap_bits = pts_wrap_bits;
    s->pts_num = pts_num;
    s->pts_den = pts_den;
}

