#ifndef _PYMUXER_H_
#define _PYMUXER_H_

#include <Python.h>
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"

extern PyObject *g_cErr;

extern PyTypeObject MuxerType ;

#define PM_ID  "id"
#define PM_TYPE  "type"
#define PM_BITRATE  "bitrate"
#define PM_WIDTH  "width"
#define PM_HEIGHT  "height"
#define PM_INDEX  "index"
#define PM_FRAME_RATE  "frame_rate"
#define PM_FRAME_RATE_B  "frame_rate_base"
#define PM_SAMPLE_RATE  "sample_rate"
#define PM_CHANNELS  "channels"
#define PM_GOP_SIZE "gop_size"
#define PM_MAX_B_FRAMES "max_b_frames"
#define PM_DURATION "length"
#define PM_EXTRA_DATA "extra_data"
#define PM_BLOCK_ALIGN "block_align"

#endif
