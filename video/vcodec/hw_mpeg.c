/*
 * MPEG1 codec / MPEG2 decoder
 * Copyright (c) 2004 Dmitry Borisov
 * 
 * Standalone hw MPEG frame parser for generic chipset
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

#include <stdlib.h>
#include "hw_mpeg.h"
#include "cle266/hw_dec.h"

 /**
 * MPEG1/2 default tables.
 */
const int16_t ff_mpeg1_default_intra_matrix[64] = {
	8, 16, 19, 22, 26, 27, 29, 34,
	16, 16, 22, 24, 27, 29, 34, 37,
	19, 22, 26, 27, 29, 34, 34, 38,
	22, 22, 26, 27, 29, 34, 37, 40,
	22, 26, 27, 29, 32, 35, 40, 48,
	26, 27, 29, 32, 35, 40, 48, 58,
	26, 27, 29, 34, 38, 46, 56, 69,
	27, 29, 35, 38, 46, 56, 69, 83
};

const int16_t ff_mpeg1_default_non_intra_matrix[64] = {
    16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16,
};

const uint8_t ff_zigzag_direct[64] = {
    0,   1,  8, 16,  9,  2,  3, 10,
    17, 24, 32, 25, 18, 11,  4,  5,
    12, 19, 26, 33, 40, 48, 41, 34,
    27, 20, 13,  6,  7, 14, 21, 28,
    35, 42, 49, 56, 57, 50, 43, 36,
    29, 22, 15, 23, 30, 37, 44, 51,
    58, 59, 52, 45, 38, 31, 39, 46,
    53, 60, 61, 54, 47, 55, 62, 63
};

const uint8_t ff_alternate_horizontal_scan[64] = {
    0,  1,   2,  3,  8,  9, 16, 17, 
    10, 11,  4,  5,  6,  7, 15, 14,
    13, 12, 19, 18, 24, 25, 32, 33, 
    26, 27, 20, 21, 22, 23, 28, 29,
    30, 31, 34, 35, 40, 41, 48, 49, 
    42, 43, 36, 37, 38, 39, 44, 45,
    46, 47, 50, 51, 56, 57, 58, 59, 
    52, 53, 54, 55, 60, 61, 62, 63,
};

const uint8_t ff_alternate_vertical_scan[64] = {
    0,  8,  16, 24,  1,  9,  2, 10, 
    17, 25, 32, 40, 48, 56, 57, 49,
    41, 33, 26, 18,  3, 11,  4, 12, 
    19, 27, 34, 42, 50, 58, 35, 43,
    51, 59, 20, 28,  5, 13,  6, 14, 
    21, 29, 36, 44, 52, 60, 37, 45,
    53, 61, 22, 30,  7, 15, 23, 31, 
    38, 46, 54, 62, 39, 47, 55, 63,
};
 
static const double mpeg2_aspect[16]={
    0,
    1.0,
    -3.0/4.0,
    -9.0/16.0,
    -1.0/2.21
};
 
/* ----------------------------------------------------------------------------- */
void ff_init_scantable(uint8_t *permutation, ScanTable *st, const uint8_t *src_scantable){
    int i;
    int end;

    st->scantable= src_scantable;

    for(i=0; i<64; i++){
        int j;
        j = src_scantable[i];
        st->permutated[i] = permutation[j];
#ifdef ARCH_POWERPC
        st->inverse[j] = i;
#endif
    }

    end=-1;
    for(i=0; i<64; i++){
        int j;
        j = st->permutated[i];
        if(j>end) end=j;
        st->raster_end[i]= end;
    }
}
 
/* ----------------------------------------------------------------------------- */
/**
 * finds the end of the current frame in the bitstream.
 * @return the position of the first byte of the next frame, or -1
 */
static int mpeg1_find_frame_end(MpegContext *s, uint8_t *buf, int buf_size)
{
  ParseContext *pc= &s->parse_context;
  int i;
  uint32_t state;
  
  state= pc->state;
  
  i=0;
  if(!pc->frame_start_found){
    for(i=0; i<buf_size; i++){
      state= (state<<8) | buf[i];
      if(state >= SLICE_MIN_START_CODE && state <= SLICE_MAX_START_CODE){
        i++;
        pc->frame_start_found=1;
        break;
      }
    }
  }
  
  if(pc->frame_start_found){
    for(; i<buf_size; i++){
      state= (state<<8) | buf[i];
      if((state&0xFFFFFF00) == 0x100){
        if(state < SLICE_MIN_START_CODE || state > SLICE_MAX_START_CODE){
          pc->frame_start_found=0;
          pc->state=-1; 
          return i-3;
        }
      }
    }
  }        
  pc->state= state;
  return END_NOT_FOUND;
}

/* ----------------------------------------------------------------------------- */
/**
 * combines the (truncated) bitstream to a complete frame
 * @returns -1 if no complete frame could be created
 */
int ff_combine_frame( MpegContext *s, int next, uint8_t **buf, int *buf_size)
{
    ParseContext *pc= &s->parse_context;

    /* copy overreaded byes from last frame into buffer */
    for(; pc->overread>0; pc->overread--){
        pc->buffer[pc->index++]= pc->buffer[pc->overread_index++];
    }

    pc->last_index= pc->index;

    /* copy into buffer end return */
    if(next == END_NOT_FOUND){
        pc->buffer= av_realloc(pc->buffer, (*buf_size) + pc->index + FF_INPUT_BUFFER_PADDING_SIZE);

        memcpy(&pc->buffer[pc->index], *buf, *buf_size);
        pc->index += *buf_size;
        return -1;
    }

    *buf_size= pc->overread_index= pc->index + next;

    /* append to buffer */
    if(pc->index){
        pc->buffer= av_realloc(pc->buffer, next + pc->index + FF_INPUT_BUFFER_PADDING_SIZE);

        memcpy(&pc->buffer[pc->index], *buf, next + FF_INPUT_BUFFER_PADDING_SIZE );
        pc->index = 0;
        *buf= pc->buffer;
    }

    /* store overread bytes */
    for(;next < 0; next++)
		{
        pc->state = (pc->state<<8) | pc->buffer[pc->last_index + next];
        pc->overread++;
    }
    return 0;
}
 
/* ----------------------------------------------------------------------------- */
/* return the 8 bit start code value and update the search
   state. Return -1 if no start code found */
static int find_start_code(uint8_t **pbuf_ptr, uint8_t *buf_end)
{
  uint8_t *buf_ptr;
  unsigned int state=0xFFFFFFFF, v= 0;
  int val;

  buf_ptr = *pbuf_ptr;
  while (buf_ptr < buf_end) 
	{
    v = *buf_ptr++;
    if (state == 0x000001) 
		{
        state = ((state << 8) | v) & 0xffffff;
        val = state;
        goto found;
    }
    state = ((state << 8) | v) & 0xffffff;
  }
  val = -1;
found:
  *pbuf_ptr = buf_ptr;
  return val;
}

/* ----------------------------------------------------------------------------- */
static int mpeg1_decode_sequence( MpegContext *s, uint8_t *buf, int buf_size )
{
    int i, m;
    init_get_bits(&s->gb, buf, buf_size*8);
 
    s->width = get_bits(&s->gb, 12);
    s->height = get_bits(&s->gb, 12);
    s->aspect_ratio_info= get_bits(&s->gb, 4);
    s->frame_rate_index = get_bits(&s->gb, 4);

    if (s->frame_rate_index == 0)
        return -1;

    s->bit_rate = get_bits(&s->gb, 18) * 400;

    if (get_bits1(&s->gb) == 0) /* marker */
        return -1;

    if ( s->width <= 0 || s->height <= 0 ||
        (s->width % 2) != 0 || (s->height % 2) != 0)
        return -1;
		
    skip_bits(&s->gb, 10); /* vbv_buffer_size */
    skip_bits(&s->gb, 1);

    /* get matrix */
		m= get_bits1(&s->gb);
    for(i=0;i<64;i++) 
		{
			int j= ( m ) ? s->intra_scantable.permutated[i]: s->idct_permutation[i];
      int v= ( m ) ? get_bits(&s->gb, 8): ff_mpeg1_default_intra_matrix[i];
      s->intra_matrix[j] = v;
      s->chroma_intra_matrix[j] = v;
		}

		m= get_bits1(&s->gb);
    for(i=0;i<64;i++) 
		{
			int j= ( m ) ? s->intra_scantable.permutated[i]: s->idct_permutation[i];
      int v= ( m ) ? get_bits(&s->gb, 8): ff_mpeg1_default_non_intra_matrix[i];
			s->inter_matrix[j] = v;
			s->chroma_inter_matrix[j] = v;
    }

    /* we set mpeg2 parameters so that it emulates mpeg1 */
    s->progressive_sequence = 1;
    s->progressive_frame = 1;
    s->picture_structure = PICT_FRAME;
    s->frame_pred_frame_dct = 1;
    return 0;
}

/* ----------------------------------------------------------------------------- */
static void mpeg_decode_sequence_extension(MpegContext *s)
{
  int horiz_size_ext, vert_size_ext;
  int bit_rate_ext, vbv_buf_ext;
  int frame_rate_ext_n, frame_rate_ext_d;
  int level, profile;
  float aspect;

  skip_bits(&s->gb, 1); /* profil and level esc*/
  profile= get_bits(&s->gb, 3);
  level= get_bits(&s->gb, 4);
  s->progressive_sequence = get_bits1(&s->gb); /* progressive_sequence */
  skip_bits(&s->gb, 2); /* chroma_format */
  horiz_size_ext = get_bits(&s->gb, 2);
  vert_size_ext = get_bits(&s->gb, 2);
  s->width |= (horiz_size_ext << 12);
  s->height |= (vert_size_ext << 12);
  bit_rate_ext = get_bits(&s->gb, 12);  /* XXX: handle it */
  s->bit_rate = ((s->bit_rate / 400) | (bit_rate_ext << 12)) * 400;
  skip_bits1(&s->gb); /* marker */
  vbv_buf_ext = get_bits(&s->gb, 8);

  s->low_delay = get_bits1(&s->gb);
  //if(s->flags & CODEC_FLAG_LOW_DELAY) s->low_delay=1;

  frame_rate_ext_n = get_bits(&s->gb, 2);
  frame_rate_ext_d = get_bits(&s->gb, 5);
  /*av_reduce(
      &s->avctx->frame_rate, 
      &s->avctx->frame_rate_base, 
      frame_rate_tab[s->frame_rate_index] * (frame_rate_ext_n+1),
      MPEG1_FRAME_RATE_BASE * (frame_rate_ext_d+1),
      1<<30);
	*/
  aspect= (float)mpeg2_aspect[s->aspect_ratio_info];
  if(aspect>0.0)      s->aspect_ratio= (float)s->width/(aspect*s->height);
  else if(aspect<0.0) s->aspect_ratio= (float)-1.0/aspect;
}

/* ----------------------------------------------------------------------------- */
static void mpeg_decode_quant_matrix_extension(MpegContext *s)
{
  int i, v, j;
  if (get_bits1(&s->gb)) {
		for(i=0;i<64;i++) {
			v = get_bits(&s->gb, 8);
			j= s->idct_permutation[ ff_zigzag_direct[i] ];
			s->intra_matrix[j] = v;
			s->chroma_intra_matrix[j] = v;
		}
  }
  if (get_bits1(&s->gb)) {
    for(i=0;i<64;i++) {
      v = get_bits(&s->gb, 8);
      j= s->idct_permutation[ ff_zigzag_direct[i] ];
      s->inter_matrix[j] = v;
      s->chroma_inter_matrix[j] = v;
    }
  }
  if (get_bits1(&s->gb)) {
    for(i=0;i<64;i++) {
      v = get_bits(&s->gb, 8);
      j= s->idct_permutation[ ff_zigzag_direct[i] ];
      s->chroma_intra_matrix[j] = v;
    }
  }
  if (get_bits1(&s->gb)) {
    for(i=0;i<64;i++) {
			v = get_bits(&s->gb, 8);
			j= s->idct_permutation[ ff_zigzag_direct[i] ];
			s->chroma_inter_matrix[j] = v;
    }
  }
}

/* ----------------------------------------------------------------------------- */
static void mpeg_decode_picture_coding_extension(MpegContext *s)
{
  s->full_pel[0] = s->full_pel[1] = 0;
  s->mpeg_f_code[0][0] = get_bits(&s->gb, 4);
  s->mpeg_f_code[0][1] = get_bits(&s->gb, 4);
  s->mpeg_f_code[1][0] = get_bits(&s->gb, 4);
  s->mpeg_f_code[1][1] = get_bits(&s->gb, 4);
  s->intra_dc_precision = get_bits(&s->gb, 2);
  s->picture_structure = get_bits(&s->gb, 2);
  s->top_field_first = get_bits1(&s->gb);
  s->frame_pred_frame_dct = get_bits1(&s->gb);
  s->concealment_motion_vectors = get_bits1(&s->gb);
  s->q_scale_type = get_bits1(&s->gb);
  s->intra_vlc_format = get_bits1(&s->gb);
  s->alternate_scan = get_bits1(&s->gb);
  s->repeat_first_field = get_bits1(&s->gb);
  s->chroma_420_type = get_bits1(&s->gb);
  s->progressive_frame = get_bits1(&s->gb);
  
  if(s->picture_structure == PICT_FRAME)
      s->first_field=0;
  else
	{
    s->first_field ^= 1;
    memset(s->mbskip_table, 0, s->mb_stride*s->mb_height);
  }
  
  if(s->alternate_scan)
	{
    ff_init_scantable(s->idct_permutation, &s->inter_scantable  , ff_alternate_vertical_scan);
    ff_init_scantable(s->idct_permutation, &s->intra_scantable  , ff_alternate_vertical_scan);
    ff_init_scantable(s->idct_permutation, &s->intra_h_scantable, ff_alternate_vertical_scan);
    ff_init_scantable(s->idct_permutation, &s->intra_v_scantable, ff_alternate_vertical_scan);
  }
	else
	{
    ff_init_scantable(s->idct_permutation, &s->inter_scantable  , ff_zigzag_direct);
    ff_init_scantable(s->idct_permutation, &s->intra_scantable  , ff_zigzag_direct);
    ff_init_scantable(s->idct_permutation, &s->intra_h_scantable, ff_alternate_horizontal_scan);
    ff_init_scantable(s->idct_permutation, &s->intra_v_scantable, ff_alternate_vertical_scan);
  }
}

/* ----------------------------------------------------------------------------- */
static void mpeg_decode_extension(MpegContext *s, uint8_t *buf, int buf_size)
{
  int ext_type;

  init_get_bits(&s->gb, buf, buf_size*8);
  
  ext_type = get_bits(&s->gb, 4);
  switch(ext_type) 
	{
    case 0x1:
      /* sequence ext */
      mpeg_decode_sequence_extension(s);
      break;
    case 0x3:
      /* quant matrix extension */
      mpeg_decode_quant_matrix_extension(s);
      break;
    case 0x8:
      /* picture extension */
      mpeg_decode_picture_coding_extension(s);
      break;
  }
}

/* ----------------------------------------------------------------------------- */
static void mpeg_decode_user_data(MpegContext *s, uint8_t *buf, int buf_size)
{
	// Do nothing for now
}
        
/* ----------------------------------------------------------------------------- */
/**
 * handles slice ends.
 * @return 1 if it seems to be the last slice of 
 */
static int slice_end(MpegContext *s)
{
    /* end of slice reached */
    if (/*s->mb_y<<field_pic == s->mb_height &&*/ !s->first_field) {
        /* end of image */
        return 1;
    } else {
        return 0;
    }
}
 
/* ----------------------------------------------------------------------------- */
static int mpeg1_decode_picture(MpegContext *s, uint8_t *buf, int buf_size)
{
    int ref, f_code;

    init_get_bits(&s->gb, buf, buf_size*8);

    ref = get_bits(&s->gb, 10); /* temporal ref */
    s->pict_type = get_bits(&s->gb, 3);
    skip_bits(&s->gb, 16);
    if (s->pict_type == P_TYPE || s->pict_type == B_TYPE) {
        s->full_pel[0] = get_bits1(&s->gb);
        f_code = get_bits(&s->gb, 3);
        if (f_code == 0)
            return -1;
        s->mpeg_f_code[0][0] = f_code;
        s->mpeg_f_code[0][1] = f_code;
    }
    if (s->pict_type == B_TYPE) {
        s->full_pel[1] = get_bits1(&s->gb);
        f_code = get_bits(&s->gb, 3);
        if (f_code == 0)
            return -1;
        s->mpeg_f_code[1][0] = f_code;
        s->mpeg_f_code[1][1] = f_code;
    }
    s->first_slice = 1;
    return 0;
}
 
/* ----------------------------------------------------------------------------- */
/* Decide what framebuffer be used for what purpose. */
static void select_fb(MpegContext *s)
{
  int i;
  switch (s->pict_type)
  {
    case I_TYPE:
    case P_TYPE:
      i = s->fb.bwref;
      s->fb.bwref = s->fb.fwref;
      s->fb.fwref = i;
      s->fb.dst = i;
      s->fb.display = s->fb.bwref;
      break;
    case B_TYPE:
			i = s->fb.idle;
			s->fb.idle = s->fb.b;
			s->fb.b = i;
			s->fb.dst = i;
			s->fb.display = i;
      break;
  }
}
 
/* ----------------------------------------------------------------------------- */
// Start frame by initializing the HW
static int start_frame( MpegContext *s )
{
	s->second_field = !s->second_field && (s->picture_structure != PICT_FRAME );
//printf( "begin pict 0\n" );

	select_fb( s );
  
	hwdec_wait( s );
	hwdec_set_fb( s, 0, s->fb.dst );
	hwdec_set_fb( s, 1, s->fb.bwref );
	hwdec_set_fb( s, 2, s->fb.fwref );
	hwdec_begin_picture( s ); 
	return 0;
}

/* ----------------------------------------------------------------------------- */
/* handle buffering and image synchronisation */
int mpeg_decode_frame(MpegContext *s, void *data, int *data_size, uint8_t *buf, int buf_size)
{
  uint8_t *buf_end, *buf_ptr;
  int start_code, input_size;
	ParseContext saved_parse;
 
  *data_size = 0;

//printf( "start frame\n" );
  /* special case for last picture */
  /*if (buf_size == 0 && s->low_delay==0 && s->next_picture_ptr) 
	{
    *picture= *(AVFrame*)s->next_picture_ptr;
    s->next_picture_ptr= NULL;

    *data_size = sizeof(AVFrame);
    return 0;
  }*/

  //if(s2->flags&CODEC_FLAG_TRUNCATED)
	// Always have truncated frames for mpeg2 ????
	
	memcpy( &saved_parse, &s->parse_context, sizeof( saved_parse ) );
	if( buf )
	{
    int next= mpeg1_find_frame_end(s, buf, buf_size);
    
    if( buf && ff_combine_frame(s, next, &buf, &buf_size) < 0 )
        return buf_size;
  }
	else
	{
		buf= s->parse_context.buffer;
		buf_size= s->parse_context.overread_index;
	}
  
  buf_ptr = buf;
  buf_end = buf + buf_size;

  *data_size = 1;
  for(;;) 
	{
    /* find start next code */
    start_code = find_start_code(&buf_ptr, buf_end);
    if (start_code < 0)
		{
//printf( "No start code found %d\n", slice_end(s) );
      if (!slice_end(s))
          *data_size = 0;
			else if( *data_size )
			{
				s->first_slice= 1;
				hwdec_wait(s); 
			}
			
      return FFMAX( 0, buf_ptr - buf - s->parse_context.last_index);
    }

    input_size = buf_end - buf_ptr;

    /* prepare data for next start code */
    switch(start_code) 
		{
			case SEQ_START_CODE:
//printf( "seq \n" );
					mpeg1_decode_sequence(s, buf_ptr, input_size);
					hwdec_reset(s);
					hwdec_wait(s); 
					break;
                
			case PICTURE_START_CODE:
//printf( "pics \n" );
					/* we have a complete image : we try to decompress it */
					mpeg1_decode_picture(s, buf_ptr, input_size);
					if( s->resync && s->pict_type==I_TYPE )
						s->resync= 0;

					break;
			case EXT_START_CODE:
//printf( "strt \n" );
					mpeg_decode_extension(s, buf_ptr, input_size);
					break;
			case USER_START_CODE:
//printf( "user \n" );
					mpeg_decode_user_data(s, buf_ptr, input_size);
					break;
			case GOP_START_CODE:
					s->first_field=0;
					break;
			default:
					if (start_code >= SLICE_MIN_START_CODE &&
							start_code <= SLICE_MAX_START_CODE) 
					{
						uint8_t *buf_ptr_tmp= buf_ptr;
						/* skip b frames if we dont have reference frames */
						if( (s->fb.bwref< 0 && s->pict_type==B_TYPE) ||
							/* Resync process is in progress */
								(s->resync && s->pict_type!=I_TYPE) )
							break;

						if( s->first_slice )
						{
//printf( "start frame \n" );
							start_frame( s );
//printf( "start frame done\n" );
							s->first_slice= 0;
						}

						// See if buffers were supplied, if not, bail out
						if( !s->fb.u[ 0 ] )
						{
							// Restore parser info( HACK )
							void *b= s->parse_context.buffer; 
							memcpy( &s->parse_context, &saved_parse, sizeof( saved_parse ) );
							s->parse_context.buffer= b;
							return ERROR_NEED_BUFFERS;
						}

						// Find the end of slice and have the whole slice to be sent to HW
				    start_code = find_start_code(&buf_ptr_tmp, buf_end);
						if( start_code>= 0 )
						{
							buf_ptr_tmp-= 4;
							input_size= buf_ptr_tmp- buf_ptr;
						}

//printf( "hw slice \n" );
//printf( "slice %x\n", start_code );
						hwdec_wait(s); 
//printf( "slice %d \n", *(int*)buf_ptr );
						hwdec_write_slice( s, buf_ptr- 4, input_size+ 4);
						buf_ptr= buf_ptr_tmp;
					}
					break;
			}
  }
}

/* ----------------------------------------------------------------------------- */
void mpeg_reset( MpegContext *s )
{
  s->resync= 1;
}

/* ----------------------------------------------------------------------------- */
// Set up parameters after parsing, returning values are:
// ERROR_NEED_DATA - not enough data for probe
// OK - have parsed and got the stream data
// ERROR_STREAM_UNKNOWN - cannot identify stream, posible not a valid stream
// MpegContext will contain the following members set:
//   - stride_num - number of surfaces needed for the codec
//   - width, height - size of each surface
//   - offset - if offsets should be supplied instead of real addresses
int mpeg_init( MpegContext *s )
{
	int i;
  s->fb.bwref = 0;
  s->fb.fwref = 1;
  s->fb.b = 2;
  s->fb.idle = 3;
	s->fb.u[ 0 ]= 0;
	s->second_field= 0;
	hwdec_open( s );

	for(i=0; i<64; i++)
			s->idct_permutation[i]= i;
	/*
  switch(idct_permutation_type)
	{
		case FF_NO_IDCT_PERM:
				for(i=0; i<64; i++)
						s->idct_permutation[i]= i;
				break;
		case FF_LIBMPEG2_IDCT_PERM:
				for(i=0; i<64; i++)
						s->idct_permutation[i]= (i & 0x38) | ((i & 6) >> 1) | ((i & 1) << 2);
				break;
		case FF_SIMPLE_IDCT_PERM:
				for(i=0; i<64; i++)
						s->idct_permutation[i]= simple_mmx_permutation[i];
				break;
		case FF_TRANSPOSE_IDCT_PERM:
				for(i=0; i<64; i++)
						s->idct_permutation[i]= ((i&7)<<3) | (i>>3);
				break;
  } 
	*/
  return 0;
}

/* ----------------------------------------------------------------------------- */
// Close all allocated buffers and reset HW if needed
int mpeg_close( MpegContext *s )
{
  return 0;
}

/* ----------------------------------------------------------------------------- */
// Try to see if codec id works for us or HW is compatible and available
int mpeg_probe()
{
	MpegContext s;
  return hwdec_open( &s );
}

/* ----------------------------------------------------------------------------- */
int mpeg_index( MpegContext *s )
{
	return s->fb.display;
}

/* ----------------------------------------------------------------------------- */
void mpeg_set_surface( MpegContext *s, int iSurf, int offs, int stride )
{
  int uo = stride * s->height;
  int vo = uo + (stride >> 1) * (s->height >> 1);
	s->fb.y[ iSurf ]= offs;
	s->fb.u[ iSurf ]= offs+ uo;
	s->fb.v[ iSurf ]= offs+ vo;
}

/* ----------------------------------------------------------------------------- */
void mpeg_set_fb_stride( MpegContext *s, int stride )
{
	hwdec_set_fb_stride(s, stride, stride >> 1 );
}
/* ----------------------------------------------------------------------------- */
/*void mpeg_blit_surface( MpegContext *s, uint32_t offs, uint32_t iSurf )
{
	hwdec_blit_surface( offs, s->fb.y[ iSurf ] );
}
*/