#ifndef AVFORMAT_H
#define AVFORMAT_H

#define LIBAVFORMAT_VERSION_INT 0x000406  
#define LIBAVFORMAT_VERSION     "0.4.6"
#define LIBAVFORMAT_BUILD       4602

#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <memory.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>

#include "mem.h"
#include "libavcodec/common.h"
#include "libavcodec/avcodec.h"
#include "avio.h"


/* packet functions */

#define AV_NOPTS_VALUE 0

typedef struct AVPacket {
    INT64 pts; /* presentation time stamp in stream units (set av_set_pts_info) */
    UINT8 *data;
    int size;
    int stream_index;
    int flags;
    int duration;       
#define PKT_FLAG_KEY   0x0001
} AVPacket; 

int av_new_packet(AVPacket *pkt, int size);
void av_free_packet(AVPacket *pkt);

/*************************************************/
/* fractional numbers for exact pts handling */

/* the exact value of the fractional number is: 'val + num / den'. num
   is assumed to be such as 0 <= num < den */
typedef struct AVFrac {
    INT64 val, num, den; 
} AVFrac;


/*************************************************/
/* input/output formats */

struct AVFormatContext;

/* this structure contains the data a format has to probe a file */
typedef struct AVProbeData {
    char *filename;
    unsigned char *buf;
    int buf_size;
} AVProbeData;

#define AVPROBE_SCORE_MAX 100

typedef struct AVFormatParameters {
    int frame_rate;
    int sample_rate;
    int channels;
    int width;
    int height;
    enum PixelFormat pix_fmt;
} AVFormatParameters;

#define AVFMT_NOFILE        0x0001 /* no file should be opened */
#define AVFMT_NEEDNUMBER    0x0002 /* needs '%d' in filename */ 
#define AVFMT_NOHEADER      0x0004 /* signal that no header is present
                                      (streams are added dynamically) */
#define AVFMT_SHOW_IDS      0x0008 /* show format stream IDs numbers */
#define AVFMT_RGB24         0x0010 /* force RGB24 output for ppm (hack
                                      - need better api) */
#define AVFMT_RAWPICTURE    0x0020 /* format wants AVPicture structure for
                                      raw picture data */

typedef struct AVOutputFormat {
    const char *name;
    const char *long_name;
    const char *mime_type;
    const char *extensions; /* comma separated extensions */
    /* size of private data so that it can be allocated in the wrapper */
    int priv_data_size;
    /* output support */
    enum CodecID audio_codec; /* default audio codec */
    enum CodecID video_codec; /* default video codec */
    int (*write_header)(struct AVFormatContext *);
    /* XXX: change prototype for 64 bit pts */
    int (*write_packet)(struct AVFormatContext *, 
                        int stream_index,
                        unsigned char *buf, int size, int force_pts);
    int (*write_trailer)(struct AVFormatContext *);
    /* can use flags: AVFMT_NOFILE, AVFMT_NEEDNUMBER */
    int flags;
    /* private fields */
    struct AVOutputFormat *next;
} AVOutputFormat;

typedef struct AVInputFormat {
    const char *name;
    const char *long_name;
    /* size of private data so that it can be allocated in the wrapper */
    int priv_data_size;
    /* tell if a given file has a chance of being parsing by this format */
    int (*read_probe)(AVProbeData *);
    /* read the format header and initialize the AVFormatContext
       structure. Return 0 if OK. 'ap' if non NULL contains
       additionnal paramters. Only used in raw format right
       now. 'av_new_stream' should be called to create new streams.  */
    int (*read_header)(struct AVFormatContext *,
                       AVFormatParameters *ap);
    /* read one packet and put it in 'pkt'. pts and flags are also
       set. 'av_new_stream' can be called only if the flag
       AVFMT_NOHEADER is used. */
    int (*read_packet)(struct AVFormatContext *, AVPacket *pkt);
    /* close the stream. The AVFormatContext and AVStreams are not
       freed by this function */
    int (*read_close)(struct AVFormatContext *);
    /* seek at or before a given pts (given in microsecond). The pts
       origin is defined by the stream */
    int (*read_seek)(struct AVFormatContext *, INT64 pts);
    /* can use flags: AVFMT_NOFILE, AVFMT_NEEDNUMBER, AVFMT_NOHEADER */
    int flags;
    /* if extensions are defined, then no probe is done. You should
       usually not use extension format guessing because it is not
       reliable enough */
    const char *extensions;
    /* general purpose read only value that the format can use */
    int value;
    /* private fields */
    struct AVInputFormat *next;
} AVInputFormat;

typedef struct AVStream {
    int index;    /* stream index in AVFormatContext */
    int id;       /* format specific stream id */
    AVCodecContext codec; /* codec context */
    int r_frame_rate;     /* real frame rate of the stream */
    uint64_t time_length; /* real length of the stream in miliseconds */
    void *priv_data;
    /* internal data used in av_find_stream_info() */
    int codec_info_state;     
    int codec_info_nb_repeat_frames;
    int codec_info_nb_real_frames;
    /* PTS generation when outputing stream */
    AVFrac pts;
    /* ffmpeg.c private use */
    int stream_copy; /* if TRUE, just copy stream */
    /* quality, as it has been removed from AVCodecContext and put in AVVideoFrame
     * MN:dunno if thats the right place, for it */
    float quality; 
} AVStream;

#define MAX_STREAMS 20

/* format I/O context */
typedef struct AVFormatContext {
    /* can only be iformat or oformat, not both at the same time */
    struct AVInputFormat *iformat;
    struct AVOutputFormat *oformat;
    void *priv_data;
    ByteIOContext pb;
    int nb_streams;
    AVStream *streams[MAX_STREAMS];
    char filename[1024]; /* input or output filename */
    /* stream info */
    char title[512];
    char author[512];
    char album[512];
    char track[6];
    char year[4];
    char copyright[512];
    char comment[512];
		int has_header;
    int flags; /* format specific flags */
    /* private data for pts handling (do not modify directly) */
    int pts_wrap_bits; /* number of bits in pts (used for wrapping control) */
    int pts_num, pts_den; /* value to convert to seconds */
    /* This buffer is only needed when packets were already buffered but
       not decoded, for example to get the codec parameters in mpeg
       streams */
   struct AVPacketList *packet_buffer;
} AVFormatContext;

typedef struct AVPacketList {
    AVPacket pkt;
    struct AVPacketList *next;
} AVPacketList;

extern AVInputFormat *first_iformat;
extern AVOutputFormat *first_oformat;

/* XXX: use automatic init with either ELF sections or C file parser */
/* modules */

/* asf.c */
int raw_init(void);

/* asf.c */
int asf_init(void);

/* ogg.c */
int ogg_init(void);

/* utils.c */
#define MKTAG(a,b,c,d) (a | (b << 8) | (c << 16) | (d << 24))
#define MKBETAG(a,b,c,d) (d | (c << 8) | (b << 16) | (a << 24))

void av_register_input_format(AVInputFormat *format);
void av_register_output_format(AVOutputFormat *format);
AVOutputFormat *guess_stream_format(const char *short_name, 
                                    const char *filename, const char *mime_type);
AVOutputFormat *guess_format(const char *short_name, 
                             const char *filename, const char *mime_type);

void av_hex_dump(UINT8 *buf, int size);


/* media file input */
AVInputFormat *guess_input_format(const char *filename);
AVInputFormat *av_find_input_format(const char *short_name);
AVInputFormat *av_probe_input_format(AVProbeData *pd, int is_opened);

#define AVERROR_UNKNOWN     (-1)  /* unknown error */
#define AVERROR_IO          (-2)  /* i/o error */
#define AVERROR_NUMEXPECTED (-3)  /* number syntax expected in filename */
#define AVERROR_INVALIDDATA (-4)  /* invalid data found */
#define AVERROR_NOMEM       (-5)  /* not enough memory */
#define AVERROR_NOFMT       (-6)  /* unknown format */

enum
{
	AVILIB_NO_ERROR= 0,
	AVILIB_NEED_DATA= -1,
	AVILIB_ERROR= -2,
	AVILIB_NO_HEADER= -3,
	AVILIB_BAD_FORMAT= -4,
	AVILIB_BAD_HEADER= -5,
	AVILIB_SMALL_BUFFER= -6
} AVILIB_ERR_CONST;

int av_find_stream_info(AVFormatContext *ic);
int av_read_packet(AVFormatContext *s, AVPacket *pkt);
void av_close_input_file(AVFormatContext *s);
AVStream *av_new_stream(AVFormatContext *s, int id);
void av_set_pts_info(AVFormatContext *s, int pts_wrap_bits,
                     int pts_num, int pts_den);

/* ffm specific for ffserver */
#define FFM_PACKET_SIZE 4096
offset_t ffm_read_write_index(int fd);
void ffm_write_write_index(int fd, offset_t pos);
void ffm_set_write_index(AVFormatContext *s, offset_t pos, offset_t file_size);

int get_frame_filename(char *buf, int buf_size,
                       const char *path, int number);
int filename_number_test(const char *filename);

#ifdef HAVE_AV_CONFIG_H
int strstart(const char *str, const char *val, const char **ptr);
int stristart(const char *str, const char *val, const char **ptr);
void pstrcpy(char *buf, int buf_size, const char *str);
char *pstrcat(char *buf, int buf_size, const char *s);

int match_ext(const char *filename, const char *extensions);

#endif /* HAVE_AV_CONFIG_H */

#endif /* AVFORMAT_H */
