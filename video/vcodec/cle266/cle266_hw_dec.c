/*** Hardware interface *****************************************************/

#if !defined( WIN32 )
#include <unistd.h>
#include <sys/io.h>
#include <sys/timeb.h>
#else
#include <conio.h>
#endif
#include "hw_dec.h"
 

/* ----------------------------------------------------------------------------- */
/* FIXME: Add 2ms timeout. If timed out -> reset decoder. */
uint32_t hwdec_wait(MpegContext *s)
{
#ifndef WIN32
    struct timeb time;
    unsigned short then;

    ftime(&time);
    then = time.millitm;
    while (hwdec_is_busy(s))
		{
      ftime(&time);
			if( time.millitm- then > 2 )
				break;
		}
    return hwdec_get_status(s);
#else
		return 0;
#endif

}

/* ----------------------------------------------------------------------------- */
int hwdec_open( MpegContext *s )
{
#if !defined( WIN32 )
  int fd;
  uint8_t* hwregs;
  unsigned long tmp;
  unsigned char *addr;

  if (ioperm(0, 0x400, 1) != 0)
      return 0;
  iopl(3);
  outb(0x2f, 0x3c4);

  tmp = inb(0x3c5);
  addr = (unsigned char*)(tmp<< 0x18);

  fd = open("/dev/mem",2);
  hwregs = (uint8_t*)mmap(0,0x1000,3,1,fd,(off_t)addr);
	close(fd);
  if ((int) hwregs == -1) 
		return 0;

  s->mmio= hwregs;
#else
	/*
  int fd;
  uint8_t* hwregs;
  unsigned long tmp;
  unsigned char *addr;

  _outp(0x2f, 0x3c4);

  tmp = _inp(0x3c5);
  addr = (unsigned char*)(tmp<< 0x18);

  //fd = open("/dev/mem",2);
  //hwregs = (uint8_t*)mmap(0,0x1000,3,1,fd,(off_t)addr);
	//close(fd);
  //if ((int) hwregs == -1) 
	//	return -1;

  s->mmio= hwregs;*/
	// Not supported on Windows for now
	return 0;
#endif
	return 1;
}

/* ----------------------------------------------------------------------------- */
int hwdec_is_busy(MpegContext *s)
{
    return (*MPEG_IO(0x54) & 0x27f) != 0x204;
}

/* ----------------------------------------------------------------------------- */
uint32_t hwdec_get_status(MpegContext *s)
{
//printf( "hw gs " );
    return *MPEG_IO(0x54);
}

/* ----------------------------------------------------------------------------- */
void hwdec_reset(MpegContext *s)
{
  int i,j;

//printf( "hw reset\n" );
  for (i = 0; i < 14; i++)
    *MPEG_IO(0x08) = 0;

  *MPEG_IO(0x98) = 0x400000;

  for (i = 0; i < 6; i++) 
	{
    *MPEG_IO(0x0c) = 0x43;
    for (j = 0x10; j < 0x20; j += 4)
      *MPEG_IO(j) = 0;
  }

  *MPEG_IO(0x0c) = 0xc3;
  for (j = 0x10; j < 0x20; j += 4)
    *MPEG_IO(j) = 0;
}

/* ----------------------------------------------------------------------------- */
void hwdec_set_fb(MpegContext *s, int i, int iFb )
{
//printf( "hw setfb %d %x %x %x\n", i, s->fb.y[ iFb ], s->fb.u[ iFb ], s->fb.v[ iFb ] );
  *MPEG_IO(0x20 + 12 * i) = s->fb.y[ iFb ] >> 3;
  *MPEG_IO(0x24 + 12 * i) = s->fb.u[ iFb ] >> 3;
  *MPEG_IO(0x28 + 12 * i) = s->fb.v[ iFb ] >> 3;
}

/* ----------------------------------------------------------------------------- */
void hwdec_set_fb_stride(MpegContext *s, uint32_t y_stride, uint32_t uv_stride)
{
//printf( "hw stride %d %d\n", y_stride, uv_stride );
  *MPEG_IO(0x50) = (y_stride >> 3) | ((uv_stride >> 3) << 16);
}

/* ----------------------------------------------------------------------------- */
/*void hwdec_blit_surface(MpegContext *s, uint32_t dst, uint32_t src_y, uint32_t src_u, uint32_t src_v )
{
	// Wait when HQv is available
  while ((VIDInD(HQV_CONTROL) & HQV_SW_FLIP) );
  VIDOutD(HQV_DST_STARTADDR1,dst); 
  VIDOutD(HQV_SRC_STARTADDR_Y, src_y);
  VIDOutD(HQV_SRC_STARTADDR_U, src_u);
  VIDOutD(HQV_SRC_STARTADDR_V, src_v);
	// Flip it !
  VIDOutD(HQV_CONTROL,( VIDInD(HQV_CONTROL)&~HQV_FLIP_ODD) |HQV_SW_FLIP|HQV_FLIP_STATUS);
}*/

/* ----------------------------------------------------------------------------- */
void hwdec_begin_picture( MpegContext *s )
{
    int j;
    int mb_width, mb_height;

    /* MPEG-2 spec, section 6.3.3. */
    mb_width = (s->width + 15) >> 4;
    if (s->progressive_sequence) {
        mb_height = (s->height + 15) >> 4;
    }
    else {
        mb_height = (s->height + 31) >> 5;
        if (s->picture_structure == PICT_FRAME) {
            mb_height <<= 1;
        }
    }

//printf( "hw bp %d %d %d %d %d\n", mb_width, mb_height, s->width, s->height, s->picture_structure );
    *MPEG_IO(0x00) =
        ((s->picture_structure & 3) << 2) |
        ((s->pict_type & 3) << 4) |
        ((s->alternate_scan) ? (1 << 6) : 0);

    //if (qmx->load_intra_quantiser_matrix) 
		{
        *MPEG_IO(0x5c) = 0;
        for (j = 0; j < 64; j += 4) {
/*printf( "%d ", s->intra_matrix[j] |
                (s->intra_matrix[j+1] << 8) |
                (s->intra_matrix[j+2] << 16) |
                (s->intra_matrix[j+3] << 24) );
            *MPEG_IO(0x60) =
                s->intra_matrix[j] |
                (s->intra_matrix[j+1] << 8) |
                (s->intra_matrix[j+2] << 16) |
                (s->intra_matrix[j+3] << 24);*/
        }
    }

//printf( "\ninter\n" );

    //if (qmx->load_non_intra_quantiser_matrix) 
		{
        *MPEG_IO(0x5c) = 1;
        for (j = 0; j < 64; j += 4) {
/*printf( "%d ", s->inter_matrix[j] |
                (s->inter_matrix[j+1] << 8) |
                (s->inter_matrix[j+2] << 16) |
                (s->inter_matrix[j+3] << 24) );
            *MPEG_IO(0x60) =
                s->inter_matrix[j] |
                (s->inter_matrix[j+1] << 8) |
                (s->inter_matrix[j+2] << 16) |
                (s->inter_matrix[j+3] << 24);*/
        }
    }

    *MPEG_IO(0x90) =
        ((mb_width * mb_height) & 0x3fff) |
        ((s->frame_pred_frame_dct) ? (1 << 14) : 0) |
        ((s->top_field_first) ? (1 << 15) : 0) |
        (1 << 16) |
        ((mb_width & 0xff) << 18);

    *MPEG_IO(0x94) =
        ((s->concealment_motion_vectors) ? 1 : 0) |
        ((s->q_scale_type) ? 2 : 0) |
        ((s->intra_dc_precision & 3) << 2) |
        (((1 + 0x100000 / mb_width) & 0xfffff) << 4) |
        ((s->intra_vlc_format) ? (1 << 24) : 0);

    *MPEG_IO(0x98) =
        (((s->mpeg_f_code[0][0]-1) & 0xf) << 0) |
        (((s->mpeg_f_code[0][1]-1) & 0xf) << 4) |
        (((s->mpeg_f_code[1][0]-1) & 0xf) << 8) |
        (((s->mpeg_f_code[1][1]-1) & 0xf) << 12) |
        ( s->second_field ? (1 << 20) : 0) | (0x0a6 << 16);

/*printf( ">>0x0 %x 0x90 %x 0x94 %x 0x98 %x\n", 
        ((s->picture_structure & 3) << 2) |
        ((s->pict_type & 3) << 4) |
        ((s->alternate_scan) ? (1 << 6) : 0),

        ((mb_width * mb_height) & 0x3fff) |
        ((s->frame_pred_frame_dct) ? (1 << 14) : 0) |
        ((s->top_field_first) ? (1 << 15) : 0) |
        (1 << 16) |
        ((mb_width & 0xff) << 18),

        ((s->concealment_motion_vectors) ? 1 : 0) |
        ((s->q_scale_type) ? 2 : 0) |
        ((s->intra_dc_precision & 3) << 2) |
        (((1 + 0x100000 / mb_width) & 0xfffff) << 4) |
        ((s->intra_vlc_format) ? (1 << 24) : 0),

        (((s->mpeg_f_code[0][0]-1) & 0xf) << 0) |
        (((s->mpeg_f_code[0][1]-1) & 0xf) << 4) |
        (((s->mpeg_f_code[1][0]-1) & 0xf) << 8) |
        (((s->mpeg_f_code[1][1]-1) & 0xf) << 12) |
        ( s->second_field ? (1 << 20) : 0) | (0x0a6 << 16) );*/
}

/* ----------------------------------------------------------------------------- */
void hwdec_write_slice(MpegContext *s, uint8_t* slice, int nbytes)
{
  int i, n, r;
  uint32_t* buf;

//printf( "hw ss %d \n", nbytes );
  n = nbytes >> 2;
  r = nbytes & 3;
  buf = (uint32_t*) slice;

  if (r) nbytes += 4 - r;

  nbytes += 8;

  *MPEG_IO(0x9c) = nbytes;
  for (i = 0; i < n; i++)
	{
//printf( "%d ", *buf );
    *MPEG_IO(0xa0) = *buf++;
	}

  if (r)
	{
//printf( "%d ", *buf & ((1 << (r << 3)) - 1) );
    *MPEG_IO(0xa0) = *buf & ((1 << (r << 3)) - 1);
	}

  *MPEG_IO(0xa0) = 0;
  *MPEG_IO(0xa0) = 0;
//printf( "\n" );
}
