/*
 * RAW encoder and decoder
 * Copyright (c) 2001 Fabrice Bellard.
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

// ---------------------------------------------------------------------------------
typedef struct
{
  char szTitle[30];
  char szArtist[30];
  char szAlbum[30];
  char szYear[4];
  char szCDDBCat[30];
  unsigned char genre;
} ID3INFO;


/* simple formats */
int raw_write_header(struct AVFormatContext *s)
{
    return 0;
}

int raw_write_packet(struct AVFormatContext *s,
                     int stream_index,
                     unsigned char *buf, int size, int force_pts)
{
    put_buffer(&s->pb, buf, size);
    put_flush_packet(&s->pb);
    return 0;
}

int raw_write_trailer(struct AVFormatContext *s)
{
    return 0;
}

/* raw input */
static int raw_read_header(AVFormatContext *s,
                           AVFormatParameters *ap)
{
    AVStream *st;
    int id;

    st = av_new_stream(s, 0);
    if (!st)
        return AVERROR_NOMEM;
    if (ap) {
        id = s->iformat->value;
        if (id == CODEC_ID_RAWVIDEO) {
            st->codec.codec_type = CODEC_TYPE_VIDEO;
        } else {
            st->codec.codec_type = CODEC_TYPE_AUDIO;
        }
        st->codec.codec_id = id;

        switch(st->codec.codec_type) {
        case CODEC_TYPE_AUDIO:
            st->codec.sample_rate = ap->sample_rate;
            st->codec.channels = ap->channels;
            break;
        case CODEC_TYPE_VIDEO:
            st->codec.frame_rate = ap->frame_rate;
            st->codec.width = ap->width;
            st->codec.height = ap->height;
            break;
        default:
            return -1;
        }
    } else {
        return -1;
    }
    return 0;
}

#define RAW_PACKET_SIZE 1024

int raw_read_packet(AVFormatContext *s,
                    AVPacket *pkt)
{
    int ret, size;
    AVStream *st = s->streams[0];

    size= RAW_PACKET_SIZE;

    if (av_new_packet(pkt, size) < 0)
        return -EIO;

    pkt->stream_index = 0;
    ret = get_buffer(&s->pb, pkt->data, size);
    /* note: we need to modify the packet size here to handle the last
       packet */
    pkt->size = ret;
    return ret;
}

int raw_read_close(AVFormatContext *s)
{
    return 0;
}

/* --------------------------------------------------------------------------------- */
void get_mp3_id3v2_tag( char* sDest, char* sFrameName, char* sBuf, int iLen )
{
	/* HACK. Locate desired frame in a buffer and get the data from there */
	int iPos= 0;
	while( iPos< iLen )
	{
		char* sTmp=	strstr( sBuf, sFrameName );
		if( sTmp )
		{
			/* Possibly found the tag. Just return the value...*/
			int iSize= ( sTmp[ 4 ] << 21 ) | ( sTmp[ 5 ] << 14 ) | ( sTmp[ 6 ] << 7 ) | sTmp[ 7 ];
			if( iSize> 0 )
				memcpy( sDest, sTmp+ 11, iSize- 1 );
			return;
		}

		iPos+= strlen( sBuf )+ 1;
		sBuf+= strlen( sBuf )+ 1;
	}
}

/* --------------------------------------------------------------------------------- */
/* mp3 head read */
static int mp3_read_header(AVFormatContext *s,
                           AVFormatParameters *ap)
{
  AVStream *st;
	char sTmp[ 1024 ];
  ByteIOContext *pb = &s->pb;

  st = av_new_stream(s, 0);
  if (!st)
      return AVERROR_NOMEM;

	if( s->has_header )
		return 0;

	/* See if id tags are present */
	if( get_mem_buffer_size( pb )< 10 )
		return AVILIB_NEED_DATA;

	get_str( pb, sTmp, 3 );
	if( !strncmp( sTmp, "ID3", 3 ))
	{
		/* Almost sure we have id3v2 tag in there, lets get the length and see if the whole tag fits into the buffer */
		int iLen1;
		get_str( pb, sTmp, 7 );
		iLen1= ( sTmp[ 3 ] << 21 ) | ( sTmp[ 4 ] << 14 ) | ( sTmp[ 5 ] << 7 ) | sTmp[ 6 ];
		if( iLen1 > get_mem_buffer_size( pb ) )
		{
			/* No enough data to get the correct tag info( hopefully it won't happen ever... ) */
			url_fseek( pb, -10, SEEK_CUR );
			return 0;
		}

		/* Copy only up to the buffer length or all if any */
		iLen1= ( iLen1 < sizeof( sTmp )- 1 ) ? iLen1: sizeof( sTmp )- 1;
		get_str( pb, sTmp, iLen1 );

		get_mp3_id3v2_tag( s->title, "TIT2", sTmp, iLen1 );
		get_mp3_id3v2_tag( s->author, "TPE1", sTmp, iLen1 );
		get_mp3_id3v2_tag( s->album, "TALB", sTmp, iLen1 );
		get_mp3_id3v2_tag( s->track, "TRCK", sTmp, iLen1 );
		get_mp3_id3v2_tag( s->year, "TYER", sTmp, iLen1 );
		s->has_header= 1;
	}
	else if( !strncmp( sTmp, "TAG", 3 ))
	{
		/* Almost sure we have id3 tag in here */
		ID3INFO* id3_info= (ID3INFO*)sTmp;
		if( get_mem_buffer_size( pb )< 125 )
			/* No enough data to get the correct tag info( hopefully it won't happen ever... ) */
			return 0;

		get_str( pb, sTmp, 125 );

		// Populate title/author/album/year/track etc
		strncpy( s->title, id3_info->szTitle, 30 );
		strncpy( s->author, id3_info->szArtist, 30 );
		strncpy( s->album, id3_info->szAlbum, 30 );
		strncpy( s->year, id3_info->szYear, 4 );
		s->has_header= 1;
	}
	else
		url_fseek( pb, -3, SEEK_CUR );

  st->codec.codec_type = CODEC_TYPE_AUDIO;
  st->codec.codec_id = CODEC_ID_MP2;
  /* the parameters will be extracted from the compressed bitstream */
  return 0;
}


#define SEQ_START_CODE		0x000001b3
#define GOP_START_CODE		0x000001b8
#define PICTURE_START_CODE	0x00000100

AVInputFormat mp3_iformat = {
    "mp3",
    "MPEG audio",
    0,
    NULL,
    mp3_read_header,
    raw_read_packet,
    raw_read_close,
		NULL,
		0,
    "mp2,mp3", /* XXX: use probe */
};

#ifdef CONFIG_FAAD

AVInputFormat ac3_iformat = {
    "ac3",
    "raw ac3",
    0,
    NULL,
    raw_read_header,
    raw_read_packet,
    raw_read_close,
		NULL,
		0,
    "ac3",
    CODEC_ID_AC3
};

#endif


#ifdef WORDS_BIGENDIAN
#define BE_DEF(s) s
#define LE_DEF(s) NULL
#else
#define BE_DEF(s) NULL
#define LE_DEF(s) s
#endif


int raw_init(void)
{
    av_register_input_format(&mp3_iformat);
#ifdef CONFIG_FAAD
    av_register_input_format(&ac3_iformat);
#endif
    return 0;
}
