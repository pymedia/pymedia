#ifdef CONFIG_VORBIS
/*
 * Ogg bitstream support
 * Mark Hills <mark@pogo.org.uk>
 *
 * Uses libogg, but requires libvorbisenc to construct correct headers
 * when containing Vorbis stream -- currently the only format supported
 */

#include <ogg/ogg.h>
#include <vorbis/vorbisenc.h>

#include "avformat.h"
#include <libavcodec/oggvorbis.h>

#define DECODER_BUFFER_SIZE 4096

typedef struct OggContext {
    /* output */
    ogg_stream_state os ;
    int header_handled ;
    ogg_int64_t base_packet_no ;
    ogg_int64_t base_granule_pos ;

    /* input */
    ogg_sync_state oy ;
} OggContext ;


#define TITLE_STR "title"
#define ARTIST_STR "artist"
#define ALBUM_STR "album"
#define TRACK_STR "tracknum"
#define YEAR_STR "year"
/* --------------------------------------------------------------------------------- */
void get_ogg_tag( char* sDest, char* sFrameName, char** sBufs, int iCount, int* piLens )
{
	int iLen= strlen( sFrameName );
	int i= 0;
	for( ; i< iCount; i++ )
		if( !strncmp( sBufs[ i ], sFrameName, iLen ) && sBufs[ i ][ iLen ]== '=' )
		{
			/* We found the frame, just write it to the buffer provided */
			strncpy( sDest, &sBufs[ i ][ iLen+ 1 ], piLens[ i ]- iLen- 1 );
			return;
		}
}

/* --------------------------------------------------------------------------------- */
static int next_packet(AVFormatContext *avfcontext, ogg_packet *op, int header )
{
  OggContext *context = avfcontext->priv_data ;
  ogg_page og ;
  char *buf;

  while(ogg_stream_packetout(&context->os, op) != 1)
	{
		/* while no pages are available, read in more data to the sync */
		while(ogg_sync_pageout(&context->oy, &og) != 1)
		{
				int buf_size= DECODER_BUFFER_SIZE;
				buf = ogg_sync_buffer(&context->oy, buf_size) ;
				if(get_buffer(&avfcontext->pb, buf, buf_size) <= 0)
					return AVILIB_NEED_DATA;
				ogg_sync_wrote(&context->oy, buf_size) ;
		}
		if( header )
			ogg_stream_init(&context->os, ogg_page_serialno(&og)) ;

		/* got a page. Feed it into the stream and get the packet */
		if(ogg_stream_pagein(&context->os, &og) != 0)
				return AVILIB_BAD_FORMAT;
  }

  return 0 ;
}

/* -----------------------------------------------------------------------------------------------*/
static int ogg_read_header(AVFormatContext *avfcontext, AVFormatParameters *ap)
{
    AVStream *ast ;
    OggContext *context= avfcontext->priv_data;
		ogg_packet op;
    ogg_page og ;
		char* buf;
		int ret;

    ogg_sync_init(&context->oy) ;
    //buf = ogg_sync_buffer(&context->oy, DECODER_BUFFER_SIZE) ;

    //if(get_buffer(&avfcontext->pb, buf, DECODER_BUFFER_SIZE) <= 0)
		//	return -EIO ;

    //ogg_sync_wrote(&context->oy, DECODER_BUFFER_SIZE) ;
    //ogg_sync_pageout(&context->oy, &og) ;

		{
			/* Process comments and stream data */
			vorbis_info vi;
			vorbis_comment vc;
			int header= 1, packets= 0;

			vorbis_info_init(&vi) ;
			vorbis_comment_init(&vc) ;

			ret= 0;
			while( ret== 0 && packets< 3 )
			{
				if( ( ret= next_packet( avfcontext, &op, header ))!= 0 )
					return ret;

				if( vorbis_synthesis_headerin(&vi, &vc, &op)!= 0 )
					return AVILIB_BAD_FORMAT;

				header= 0;
				packets++;
			}

			/* Convert comments into strings  */
			{
				char **s= vc.user_comments;
				if( vc.comment_lengths )
				{
					get_ogg_tag( avfcontext->title, "title", s, vc.comments, vc.comment_lengths );
					get_ogg_tag( avfcontext->author, "artist", s, vc.comments, vc.comment_lengths );
					get_ogg_tag( avfcontext->album, "album", s, vc.comments, vc.comment_lengths );
					get_ogg_tag( avfcontext->track, "tracknum", s, vc.comments, vc.comment_lengths );
					get_ogg_tag( avfcontext->year, "year", s, vc.comments, vc.comment_lengths );
					avfcontext->has_header= 1;
				}
			}

			/* Preserve stream params */
			ast = av_new_stream(avfcontext, 0) ;
			if(!ast)
				return AVERROR_NOMEM;

			vorbis_info_clear(&vi) ;
			vorbis_comment_clear(&vc) ;
			ogg_stream_clear(&context->os) ;
			ogg_sync_clear(&context->oy) ;
			/* Seek to the beginning of the buffer */
			avfcontext->pb.buf_ptr= avfcontext->pb.buffer;
		}

    /* currently only one vorbis stream supported */
    ogg_sync_init(&context->oy) ;
    buf = ogg_sync_buffer(&context->oy, DECODER_BUFFER_SIZE) ;
    if(get_buffer(&avfcontext->pb, buf, DECODER_BUFFER_SIZE) <= 0)
			return -EIO;

    /* Initialize stream for the correct libavcodec processing */
    ogg_sync_wrote(&context->oy, DECODER_BUFFER_SIZE) ;
    ogg_sync_pageout(&context->oy, &og) ;
    ogg_stream_init(&context->os, ogg_page_serialno(&og)) ;
    ogg_stream_pagein(&context->os, &og) ;

    ast->codec.codec_type = CODEC_TYPE_AUDIO ;
    ast->codec.codec_id = CODEC_ID_VORBIS ;

    return 0;
}


static int ogg_read_packet(AVFormatContext *avfcontext, AVPacket *pkt) {
    ogg_packet op ;
		int ret;

    ret= next_packet(avfcontext, &op, 0);
		if( ret )
			return ret;
    if(av_new_packet(pkt, sizeof(ogg_packet) + op.bytes) < 0)
			return AVILIB_ERROR;
    pkt->stream_index = 0 ;
    memcpy(pkt->data, &op, sizeof(ogg_packet)) ;
    memcpy(pkt->data + sizeof(ogg_packet), op.packet, op.bytes) ;

    return sizeof(ogg_packet) + op.bytes ;
}


static int ogg_read_close(AVFormatContext *avfcontext) {
	  int i;
    OggContext *context = avfcontext->priv_data ;

    ogg_stream_clear(&context->os) ;
    ogg_sync_clear(&context->oy) ;
    for(i=0;i<avfcontext->nb_streams;i++)
			av_free( avfcontext->streams[i] );
    return 0 ;
}


static AVInputFormat ogg_iformat = {
    "ogg",
    "Ogg Vorbis",
    sizeof(OggContext),
    NULL,
    ogg_read_header,
    ogg_read_packet,
    ogg_read_close,
		NULL,
		0,
    "ogg",
} ;


int ogg_init(void) {
    av_register_input_format(&ogg_iformat);
    return 0 ;
}

#endif CONFIG_VORBIS
