/*
 * AVI encoder.
 * Copyright (c) 2000 Fabrice Bellard.
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
#include "avi.h"

/*
 * TODO: 
 *  - fill all fields if non streamed (nb_frames for example)
 */

typedef struct AVIIndex {
    unsigned char tag[4];
    unsigned int flags, pos, len;
    struct AVIIndex *next;
} AVIIndex;

typedef struct {
    offset_t movi_list, frames_hdr_all, frames_hdr_strm[MAX_STREAMS];
    int audio_strm_length[MAX_STREAMS];
    AVIIndex *first, *last;
} AVIContext;

offset_t start_tag(ByteIOContext *pb, const char *tag)
{
    put_tag(pb, tag);
    put_le32(pb, 0);
    return url_ftell(pb);
}

void end_tag(ByteIOContext *pb, offset_t start)
{
    offset_t pos;

    pos = url_ftell(pb);
    url_fseek(pb, start - 4, SEEK_SET);
    put_le32(pb, (UINT32)(pos - start));
    url_fseek(pb, pos, SEEK_SET);
}

/* Note: when encoding, the first matching tag is used, so order is
   important if multiple tags possible for a given codec. */
const CodecTag codec_bmp_tags[] = {
    { CODEC_ID_H263, MKTAG('H', '2', '6', '3') },
    { CODEC_ID_H263P, MKTAG('H', '2', '6', '3') },
    { CODEC_ID_H263I, MKTAG('I', '2', '6', '3') }, /* intel h263 */
    { CODEC_ID_MJPEG, MKTAG('M', 'J', 'P', 'G') },
    { CODEC_ID_MPEG4, MKTAG('D', 'I', 'V', 'X'), 1 },
    { CODEC_ID_MPEG4, MKTAG('d', 'i', 'v', 'x'), 1 },
    { CODEC_ID_MPEG4, MKTAG('D', 'X', '5', '0'), 1 },
    { CODEC_ID_MPEG4, MKTAG('X', 'V', 'I', 'D'), 1 },
    { CODEC_ID_MPEG4, MKTAG('x', 'v', 'i', 'd'), 1 },
    { CODEC_ID_MPEG4, MKTAG('m', 'p', '4', 's'), 1 },
    { CODEC_ID_MPEG4, MKTAG('M', 'P', '4', 'S') },
    { CODEC_ID_MPEG4, MKTAG('M', '4', 'S', '2') },
    { CODEC_ID_MPEG4, MKTAG('m', '4', 's', '2') },
    { CODEC_ID_MPEG4, MKTAG(0x04, 0, 0, 0) }, /* some broken avi use this */
    { CODEC_ID_MSMPEG4V3, MKTAG('D', 'I', 'V', '3'), 1 }, /* default signature when using MSMPEG4 */
    { CODEC_ID_MSMPEG4V3, MKTAG('d', 'i', 'v', '3'),  1 },
    { CODEC_ID_MSMPEG4V3, MKTAG('M', 'P', '4', '3') }, 
    { CODEC_ID_MSMPEG4V2, MKTAG('M', 'P', '4', '2') }, 
    { CODEC_ID_MSMPEG4V1, MKTAG('M', 'P', 'G', '4') }, 
    { CODEC_ID_WMV1, MKTAG('W', 'M', 'V', '1') }, 
    { CODEC_ID_DVVIDEO, MKTAG('d', 'v', 's', 'l') }, 
    { CODEC_ID_DVVIDEO, MKTAG('d', 'v', 's', 'd') }, 
    { CODEC_ID_DVVIDEO, MKTAG('D', 'V', 'S', 'D') }, 
    { CODEC_ID_DVVIDEO, MKTAG('d', 'v', 'h', 'd') }, 
    { CODEC_ID_MPEG1VIDEO, MKTAG('m', 'p', 'g', '1') }, 
    { CODEC_ID_MPEG1VIDEO, MKTAG('m', 'p', 'g', '2') }, 
    { CODEC_ID_MPEG1VIDEO, MKTAG('P', 'I', 'M', '1') }, 
    { CODEC_ID_MJPEG, MKTAG('M', 'J', 'P', 'G') },
    { CODEC_ID_HUFFYUV, MKTAG('H', 'F', 'Y', 'U') },
    { CODEC_ID_HUFFYUV, MKTAG('h', 'f', 'y', 'u') },
    { 0, 0 },
};

unsigned int codec_get_tag(const CodecTag *tags, int id)
{
    while (tags->id != 0) {
        if (tags->id == id)
            return tags->tag;
        tags++;
    }
    return 0;
}

static unsigned int codec_get_asf_tag(const CodecTag *tags, int id)
{
    while (tags->id != 0) {
        if (!tags->invalid_asf && tags->id == id)
            return tags->tag;
        tags++;
    }
    return 0;
}

int codec_get_id(const CodecTag *tags, unsigned int tag)
{
    while (tags->id != 0) {
        if (tags->tag == tag)
            return tags->id;
        tags++;
    }
    return 0;
}

unsigned int codec_get_bmp_tag(int id)
{
    return codec_get_tag(codec_bmp_tags, id);
}
