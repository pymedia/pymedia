/*
 * Buffered I/O for ffmpeg system
 * Copyright (c) 2000,2001 Fabrice Bellard
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
#include <stdarg.h>
#include "patch.h"

#define IO_BUFFER_SIZE 32768

int init_put_byte(ByteIOContext *s,
                  unsigned char *buffer,
                  int buffer_size,
                  int write_flag,
                  void *opaque,
                  int (*read_packet)(void *opaque, UINT8 *buf, int buf_size),
                  void (*write_packet)(void *opaque, UINT8 *buf, int buf_size),
                  int (*seek)(void *opaque, offset_t offset, int whence))
{
    s->buffer = buffer;
    s->buffer_size = buffer_size;
    s->buf_ptr = buffer;
    s->write_flag = write_flag;
    if (!s->write_flag)
        s->buf_end = buffer;
    else
        s->buf_end = buffer + buffer_size;
    s->opaque = opaque;
    s->write_packet = write_packet;
    s->read_packet = read_packet;
    s->seek = seek;
    s->pos = 0;
    s->must_flush = 0;
    s->eof_reached = 0;
    s->is_streamed = 0;
    s->max_packet_size = 0;
    return 0;
}


static void flush_buffer(ByteIOContext *s)
{
    if (s->buf_ptr > s->buffer) {
	    if (s->write_packet)
		    s->write_packet(s->opaque, s->buffer, s->buf_ptr - s->buffer);
	    s->pos += s->buf_ptr - s->buffer;
	    //FD: Saving flushed packets in memory buffer
	    //if (s->out_buf) 
			{
		    s->out_buf.buffer = av_realloc(s->out_buf.buffer,
				    s->out_buf.buf_size+
				    s->buf_ptr - s->buffer);
		    memcpy(s->out_buf.buffer + s->out_buf.buf_size,
				    s->buffer,
				    s->buf_ptr - s->buffer);
		    s->out_buf.buf_size += s->buf_ptr - s->buffer;
	    }
    }     
    s->buf_ptr = s->buffer;
}

void put_byte(ByteIOContext *s, int b)
{
    *(s->buf_ptr)++ = b;
    if (s->buf_ptr >= s->buf_end)
        flush_buffer(s);
}

void put_buffer(ByteIOContext *s, 
		const unsigned char *buf, 
		int size)
{
    int len;

    while (size > 0) {
        len = (s->buf_end - s->buf_ptr);
        if (len > size)
            len = size;
        memcpy(s->buf_ptr, buf, len);
        s->buf_ptr += len;

        if (s->buf_ptr >= s->buf_end)
            flush_buffer(s);

        buf += len;
        size -= len;
    }
}

void put_flush_packet(ByteIOContext *s)
{
    flush_buffer(s);
    s->must_flush = 0;
}

offset_t url_fseek(ByteIOContext *s, offset_t offset, int whence)
{
    offset_t offset1;

    if (whence != SEEK_CUR && whence != SEEK_SET)
        return -EINVAL;

    if (s->write_flag) {
        if (whence == SEEK_CUR) {
            offset1 = s->pos + (s->buf_ptr - s->buffer);
            if (offset == 0)
                return offset1;
            offset += offset1;
        }
        offset1 = offset - s->pos;
        if (!s->must_flush &&
            offset1 >= 0 && offset1 < (s->buf_end - s->buffer)) {
            /* can do the seek inside the buffer */
            s->buf_ptr = s->buffer + offset1;
        } else {
            if (!s->seek)
                return -EPIPE;
            flush_buffer(s);
            s->must_flush = 1;
            s->buf_ptr = s->buffer;
            s->seek(s->opaque, offset, SEEK_SET);
            s->pos = offset;
        }
    } else {
				offset1 = (s->buf_ptr - s->buffer);
        if (whence == SEEK_CUR)
				{
					// Can navigate only within the buffer
					if (offset == 0 )
							return s->pos+ offset1;

					if( -offset> offset1 || offset+ offset1> ( s->buf_end- s->buffer ) )
						return -1;

          offset += offset1;
				}
        else if (whence == SEEK_SET)
				{
					offset= offset- s->pos;
					if( offset> ( s->buf_end- s->buffer ) || offset< 0 )
						return -1;
				}

        /* can do the seek inside the buffer */
        s->buf_ptr = s->buffer + offset;
    }
    return s->pos+ offset;
}

offset_t url_fskip(ByteIOContext *s, offset_t offset)
{
    return url_fseek(s, offset, SEEK_CUR);
}

offset_t url_ftell(ByteIOContext *s)
{
    return url_fseek(s, 0, SEEK_CUR);
}

int url_feof(ByteIOContext *s)
{
    return s->eof_reached;
}

void put_le32(ByteIOContext *s, unsigned int val)
{
    put_byte(s, val);
    put_byte(s, val >> 8);
    put_byte(s, val >> 16);
    put_byte(s, val >> 24);
}

void put_be32(ByteIOContext *s, unsigned int val)
{
    put_byte(s, val >> 24);
    put_byte(s, val >> 16);
    put_byte(s, val >> 8);
    put_byte(s, val);
}

/* IEEE format is assumed */
void put_be64_double(ByteIOContext *s, double val)
{
    union {
        double d;
        UINT64 ull;
    } u;
    u.d = val;
    put_be64(s, u.ull);
}

void put_strz(ByteIOContext *s, const char *str)
{
    if (str)
        put_buffer(s, (const unsigned char *) str, strlen(str) + 1);
    else
        put_byte(s, 0);
}

void put_le64(ByteIOContext *s, UINT64 val)
{
    put_le32(s, (UINT32)(val & 0xffffffff));
    put_le32(s, (UINT32)(val >> 32));
}

void put_be64(ByteIOContext *s, UINT64 val)
{
    put_be32(s, (UINT32)(val >> 32));
    put_be32(s, (UINT32)(val & 0xffffffff));
}

void put_le16(ByteIOContext *s, unsigned int val)
{
    put_byte(s, val);
    put_byte(s, val >> 8);
}

void put_be16(ByteIOContext *s, unsigned int val)
{
    put_byte(s, val >> 8);
    put_byte(s, val);
}

void put_tag(ByteIOContext *s, const char *tag)
{
    while (*tag) {
        put_byte(s, *tag++);
    }
}

int get_mem_buffer_size( ByteIOContext* stBuf )
{
	return stBuf->buffer ? stBuf->buf_end- stBuf->buf_ptr: 0;
}

/* Input stream */

static void fill_buffer(ByteIOContext *s)
{
/*
    if (s->eof_reached)
        return;
    len = s->read_packet(s->opaque, s->buffer, s->buffer_size);
    if (len <= 0) {
        s->eof_reached = 1;
    } else {
        s->pos += len;
        s->buf_ptr = s->buffer;
        s->buf_end = s->buffer + len;
    }*/
}

/* NOTE: return 0 if EOF, so you cannot use it if EOF handling is
   necessary */
/* XXX: put an inline version */
int get_byte(ByteIOContext *s)
{
    if (s->buf_ptr < s->buf_end)
      return *s->buf_ptr++;
		else
			return 0;
}


int get_buffer(ByteIOContext *s, unsigned char *buf, int size)
{
    int len, size1;

    size1 = size;
    while (size > 0) {
        len = s->buf_end - s->buf_ptr;
        if (len > size)
            len = size;
				else if( len < size)
					return AVILIB_NEED_DATA;

        memcpy(buf, s->buf_ptr, len);
        buf += len;
        s->buf_ptr += len;
        size -= len;
    }
    return size1 - size;
}

unsigned int get_le16(ByteIOContext *s)
{
    unsigned int val;
    val = get_byte(s);
    val |= get_byte(s) << 8;
    return val;
}

unsigned int get_le32(ByteIOContext *s)
{
    unsigned int val;
    val = get_byte(s);
    val |= get_byte(s) << 8;
    val |= get_byte(s) << 16;
    val |= get_byte(s) << 24;
    return val;
}

UINT64 get_le64(ByteIOContext *s)
{
    UINT64 val;
    val = (UINT64)get_le32(s);
    val |= (UINT64)get_le32(s) << 32;
    return val;
}

unsigned int get_be16(ByteIOContext *s)
{
    unsigned int val;
    val = get_byte(s) << 8;
    val |= get_byte(s);
    return val;
}

unsigned int get_be32(ByteIOContext *s)
{
    unsigned int val;
    val = get_byte(s) << 24;
    val |= get_byte(s) << 16;
    val |= get_byte(s) << 8;
    val |= get_byte(s);
    return val;
}

double get_be64_double(ByteIOContext *s)
{
    union {
        double d;
        UINT64 ull;
    } u;

    u.ull = get_be64(s);
    return u.d;
}

char *get_strz(ByteIOContext *s, char *buf, int maxlen)
{
    int i = 0;
    char c;

    while ((c = get_byte(s))) {
        if (i < maxlen-1)
            buf[i++] = c;
    }

    buf[i] = 0; /* Ensure null terminated, but may be truncated */

    return buf;
}

char *get_str(ByteIOContext *s, char *buf, int maxlen)
{
    int i = 0;
		for( ; maxlen > 0; maxlen-- )
      buf[i++] = get_byte(s);

    return buf;
}

UINT64 get_be64(ByteIOContext *s)
{
    UINT64 val;
    val = (UINT64)get_be32(s) << 32;
    val |= (UINT64)get_be32(s);
    return val;
}

