/*
 *			A light compression/decompression library for video formats
 *      The list of all possible formats supported may be obtained
 *			through the 'extensions' call.
 *			The easiest way to add codec is to use one from the libavcodec project...
 *
 *
 * Copyright (C) 2002-2004  Dmitry Borisov, Fedor Druzhinin
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Dmitry Borisov
*/

#include <Python.h>
#include <structmember.h>

#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
 
#ifndef BUILD_NUM
#define BUILD_NUM 1
#endif

#if defined(CONFIG_WIN32)
static inline int strcasecmp(const char* s1, const char* s2) { return stricmp(s1,s2); }
#endif

#define MODULE_NAME "pymedia.video.vcodec"

const int PYBUILD= BUILD_NUM;
const char* PYVERSION= "1";
const char* PYDOC=
"Video data decoding/encoding routins\nAllows to :\n"
"\t- Decode/encode strings of video information into/from either planar or packed formats( such as YUV420, RGB24, etc )\n"
"\t- Provides all formats with the same interface\n";

#define ENCODER_NAME "Encoder"
#define DECODER_NAME "Decoder"
#define VFRAME_NAME "VFrame"

#define GET_CODEC_ID_NAME "getCodecID"
#define FRAME_CONVERT_NAME "convert"
#define CONVERT_NAME "decode"
#define GET_PARAM_NAME "getParams" 
#define ENCODE_NAME "encode"
#define RESET_NAME "reset"

#define CONVERT_DOC \
	  FRAME_CONVERT_NAME"( format {, size } ) -> frame\n \
		Convert existing video frame into another format with all needed color conversions and scaling.\n"

#define GET_PARAM_DOC \
	GET_PARAM_NAME"() -> params\n\
		Parameters that represents the current state of codec\n"

#define ENCODE_DOC \
	ENCODE_NAME"( frame, [ scale ]-> encoded string\n\
	        Encode frame"

#define GET_CODEC_ID_DOC \
	GET_CODEC_ID_NAME"(name) - returns internal numerical codec id for its name"

#define FRAME_CONVERT_DOC \
	FRAME_CONVERT_NAME"( frame, format ) -> VFrame\nconverts existing VFrame into the new one with desired format"

#define RESET_DOC \
	RESET_NAME"() -> None\nReset current state of codec\n"

#define VFRAME_DOC \
	VFRAME_NAME"( format, size, data ) -> "VFRAME_NAME"\nCreate a new VFrame with the parameters passed.\n" \
					"The 'format' should be one from vcodec.formats and the 'size' should represent the actual size of the frame( w, h )\n" \
					"If you create RGB picture, make sure that 'data' contains 3 strings as below: ( RGB, None, None )"

// Frame attributes
#define FORMAT "format"
#define DATA "data"
#define SIZE "size"
#define FRAME_RATE_ATTR "rate"
#define FRAME_RATE_B_ATTR "rate_base"
#define ASPECT_RATIO "aspect_ratio"
#define BITRATE "bitrate"
#define RESYNC "resync"
#define PICT_TYPE "pict_type"
#define FRAME_NUMBER "frame_number"


//char* FOURCC= "fourcc";
char* TYPE= "type";
char* ID= "id";
char* WIDTH= "width";
char* HEIGHT= "height";
char* FRAME_RATE= "frame_rate";
char* FRAME_RATE_B= "frame_rate_base";
char* GOP_SIZE="gop_size";
char* MAX_B_FRAMES="max_b_frames";
char* DEINTERLACE="deinterlace";

PyDoc_STRVAR(decoder_doc,
DECODER_NAME"( codecParams ) -> "DECODER_NAME"\n\
\n\
Creates new video decoder object based on codecParams. \n\
	codecParams - dictionary with the following items parameters:\n\
		'id', 'fourcc', 'type', 'bitrate', 'width', 'height', \n\
		'format', 'data', 'size', 'rate',\n\
		'rate_base', 'frame_rate', 'frame_rate_base', 'gop_size'\n\
		'max_b_frames', 'deinterlace'\n\
The list of available codecs may be accessed through vcodec.codecs\n\
All possible formats can be obtained through vcodec.formats\n\
Methods of "DECODER_NAME" are:\n"
CONVERT_DOC
GET_PARAM_DOC );

PyDoc_STRVAR(encoder_doc,
ENCODER_NAME"( codecParams ) -> "ENCODER_NAME"\n\
\n\
Creates new video encoder object based codecParams. \n\
	codecParams - dictionary with the following items parameters:\n\
		'id', 'fourcc', 'type', 'bitrate', 'width', 'height', \n\
		'format', 'data', 'size', 'rate',\n\
		'rate_base', 'frame_rate', 'frame_rate_base', 'gop_size'\n\
		'max_b_frames', 'deinterlace'\n\
The list of available codecs may be accessed through vcodec.codecs\n\
All possible formats to the convert video frame to are available trough vcodec.formats\n\
Methods of "ENCODER_NAME" are:\n"
ENCODE_DOC);

#define RETURN_NONE return (Py_INCREF(Py_None), Py_None); 

PyObject *g_cErr;

#define VCODEC_DEINTERLACE_FL 0x02
#define VCODEC_POSTPROC_FL    0x04

// ---------------------------------------------------------------------------------
static PyTypeObject PyFormatsType;
static PyTypeObject VFrameType;

// ---------------------------------------------------------------------------------
typedef struct
{
	PyObject_HEAD
} PyFormatsObject;

// ---------------------------------------------------------------------------------
typedef struct
{
	PyObject_HEAD
	AVCodecContext *cCodec;
	AVFrame frame;
	int iVcodecFlags;
} PyCodecObject;

// ---------------------------------------------------------------------------------
typedef struct
{
	PyObject_HEAD
	PyObject* cData[ 3 ];
	int width;
	int height;
	float aspect_ratio;
	int frame_rate;
	int frame_rate_base;
	int pix_fmt;
	int bit_rate;
	int resync;
	int pict_type;
	int frame_number;
} PyVFrameObject;


/* -----------------------------------------------------------------------------------------------*/
typedef struct AVILIBError
{
	int iErrCode;
	const char* sErrDesc;
} AVILIBError;

/* -----------------------------------------------------------------------------------------------*/
AVILIBError g_AvilibErr[]= {
	{ AVILIB_ERROR, "Generic error. No further help available." },
	{ AVILIB_NO_HEADER, "There is no header in a file where it should be" },
	{ AVILIB_BAD_FORMAT, "The format of file is wrong." },
	{ AVILIB_BAD_HEADER, "The header of the file is corrupted." },
	{ AVILIB_SMALL_BUFFER, "Output buffer is too small. You may need to increase vocdec.c/MAX_OUTFRAME_SIZE and recompile pymedia." },
	{ AVILIB_ENCRYPTED, "The stream is encrypted and cannot be processed by codec." },
	{ 0, NULL }
};

// ---------------------------------------------------------------------------------
int SetStructVal( int* pVal, PyObject* cObj, char* sKey )
{
	PyObject* obj =PyDict_GetItemString(cObj,sKey);
	if (obj && PyInt_Check(obj))
	{
		*pVal= PyInt_AsLong(obj);
		return 1;
	}
	else
	{
		*pVal= 0;
		return 0;
	}
}
// ---------------------------------------------------------------------------------
void SetFlagVal(PyCodecObject* cCodec, PyObject* cObj, int iFlag, char* sKey)
{
	PyObject* obj =PyDict_GetItemString(cObj,sKey);
	if (obj && PyInt_Check(obj)) {
		if (PyInt_AsLong(obj))
			cCodec->iVcodecFlags |= iFlag;
		else
			cCodec->iVcodecFlags &= ~iFlag;
	}
}

// ---------------------------------------------------------------------------------
int SetAttribute_i( PyObject* cDict, char* sKey, int iVal )
{
	PyObject* cVal= PyInt_FromLong( iVal );
	if( !cVal )
		return 0;
	PyDict_SetItemString( cDict, sKey, cVal );
	Py_DECREF( cVal );
	return 1;
}

// ---------------------------------------------------------------------------------
int SetAttribute_s( PyObject* cDict, char* sKey, char* sVal )
{
	PyObject* cVal= PyString_FromString( sVal );
	if( !cVal )
		return 0;
	PyDict_SetItemString( cDict, sKey, cVal );
	Py_DECREF( cVal );
	return 1;
}

// ---------------------------------------------------------------------------------
int SetAttribute_o( PyObject* cDict, char* sKey, PyObject* cVal )
{
	if( cVal )
	{
		PyDict_SetItemString( cDict, sKey, cVal );
		Py_DECREF( cVal );
	}
	return 1;
}

// ---------------------------------------------------------------------------------
#define PARAM_CHECK(v,s) \
		if( !SetStructVal( &v, cObj, s ) || v== 0 ) \
		{ \
			PyErr_Format( g_cErr, "required parameter '%s' is missing. Consider passing correct initialization params", s ); \
			i= 0;\
		}

// ---------------------------------------------------------------------------------
int SetCodecParams( PyCodecObject* obj, PyObject* cObj )
{
		int i= 1;
		PARAM_CHECK( obj->cCodec->bit_rate, BITRATE )
		PARAM_CHECK( obj->cCodec->height, HEIGHT )
		PARAM_CHECK( obj->cCodec->width, WIDTH )
		PARAM_CHECK( obj->cCodec->frame_rate, FRAME_RATE )

		if( !SetStructVal( &obj->cCodec->frame_rate_base, cObj, FRAME_RATE_B ))
			obj->cCodec->frame_rate= 1;

		if( !SetStructVal( &obj->cCodec->gop_size,cObj,GOP_SIZE))
			obj->cCodec->gop_size= 12;

		SetStructVal( &obj->cCodec->max_b_frames,cObj,MAX_B_FRAMES);
		SetFlagVal ( obj,cObj,VCODEC_DEINTERLACE_FL,DEINTERLACE);
		return i;
}

// ----------------------------------------------------------------
static PyObject *
FormatsRepr(PyFormatsObject *cFormats);

// ----------------------------------------------------------------
static PyTypeObject FormatsType =
{
	PyObject_HEAD_INIT(NULL)
	0,
	"Formats",
	sizeof(PyFormatsType),
	0,
	0,				//tp_dealloc
	0,			  //tp_print
	0,				//tp_getattr
	0,			  //tp_setattr
	0,			  //tp_compare
	(reprfunc)FormatsRepr,			  //tp_repr
	0,			  //tp_as_number
	0,			  //tp_as_sequence
	0,				//tp_as_mapping
	0,					/* tp_hash */
	0,					/* tp_call */
	0,					/* tp_str */
	PyObject_GenericGetAttr,		/* tp_getattro */
	0,					/* tp_setattro */
	0,					/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /* tp_flags */
	"",					/* tp_doc */
	0,					/* tp_traverse */
	0,					/* tp_clear */
	0,					/* tp_richcompare */
	0,					/* tp_weaklistoffset */
	0,					/* tp_iter */
	0,					/* tp_iternext */
	0,				  /* tp_methods */
	0,					/* tp_members */
	0,					/* tp_getset */
	0,					/* tp_base */
	0,					/* tp_dict */
	0,					/* tp_descr_get */
	0,					/* tp_descr_set */
	0,					/* tp_dictoffset */
	0,					/* tp_init */
	PyType_GenericAlloc,			/* tp_alloc */
	0,	/* tp_new */
	PyObject_Del,				/* tp_free */
};

// ----------------------------------------------------------------
static PyObject *
FormatsRepr(PyFormatsObject *cFormats)
{
	char sBuf[ 1000 ];
	int i;
	PyObject* cTmp= PyDict_Keys( FormatsType.tp_dict );
	sBuf[ 0 ]= '\0';
	for( i= 0; i< PyObject_Length( cTmp ); i++ )
	{
		char *s= PyString_AsString( PyList_GetItem( cTmp, i ));
		if( strlen( s )> 2 && ( s[ 0 ]== s[ 1 ] ) && ( s[ 1 ]== '_' ) )
			continue;
		if( i )
			strcat( sBuf, ",");
		strcat( sBuf, s);
	}
	return PyString_FromString( sBuf );
}

// ---------------------------------------------------------------------------------
#define SAVE_FORMAT(name,obj) PyDict_SetItemString( (PyObject*)obj, #name, PyInt_FromLong( name ))

// ---------------------------------------------------------------------------------
static PyObject *Formats_New()
{
	PyObject* d;
	PyFormatsObject* cFormats= (PyFormatsObject*)PyObject_New( PyFormatsObject, &FormatsType );
	if( !cFormats )
		return NULL;

	d = FormatsType.tp_dict;
 	// Start assembling result into frame
  SAVE_FORMAT( PIX_FMT_YUV420P, d );
  SAVE_FORMAT( PIX_FMT_YUV422, d );
  SAVE_FORMAT( PIX_FMT_RGB24, d );
  SAVE_FORMAT( PIX_FMT_BGR24, d );
  SAVE_FORMAT( PIX_FMT_YUV422P, d );
  SAVE_FORMAT( PIX_FMT_YUV444P, d );
  SAVE_FORMAT( PIX_FMT_RGBA32, d );
  SAVE_FORMAT( PIX_FMT_YUV410P, d );
  SAVE_FORMAT( PIX_FMT_YUV411P, d );
  SAVE_FORMAT( PIX_FMT_RGB565, d );
  SAVE_FORMAT( PIX_FMT_RGB555, d );
  SAVE_FORMAT( PIX_FMT_GRAY8, d );
  SAVE_FORMAT( PIX_FMT_MONOWHITE, d );
  SAVE_FORMAT( PIX_FMT_MONOBLACK, d );
  SAVE_FORMAT( PIX_FMT_PAL8,  d );
  SAVE_FORMAT( PIX_FMT_YUVJ420P, d );
  SAVE_FORMAT( PIX_FMT_YUVJ422P, d );
  SAVE_FORMAT( PIX_FMT_YUVJ444P, d );
  SAVE_FORMAT( PIX_FMT_XVMC_MPEG2_MC, d );
  SAVE_FORMAT( PIX_FMT_XVMC_MPEG2_IDCT, d );
  SAVE_FORMAT( PIX_FMT_NB, d );
	return (PyObject*)cFormats;
}

// ----------------------------------------------------------------
static void
FrameClose( PyVFrameObject *frame )
{
	int i= 0;
	for( i=0; i< 3; i++ ) {
		if( frame->cData[ i ] )
		{
			Py_DECREF( frame->cData[ i ] );
		}
//		if( frame->iLineSize[ i ] )
//			Py_DECREF( frame->iLineSize[ i ] );
	}

	PyObject_Free( (PyObject*)frame );
}

// ----------------------------------------------------------------

static int frame_get_width(PyVFrameObject *frame)
{
	return frame->width;
}

// ----------------------------------------------------------------
static int frame_get_height(PyVFrameObject *frame)
{
	return frame->height;
}

// ----------------------------------------------------------------
static PyObject *
frame_get_size(PyVFrameObject *frame, void *closure)
{
	return Py_BuildValue( "(ii)", frame_get_width(frame),	frame_get_height(frame));
}
// ----------------------------------------------------------------
static PyObject *
frame_get_data(PyVFrameObject *frame, void *closure)
{
	// return None when frame still in resync
	if( !frame->cData[ 0 ] )
	{
		Py_INCREF( Py_None );
		return Py_None;
	}

	switch( frame->pix_fmt )
	{
		// Three planes
		case PIX_FMT_YUV420P:
		case PIX_FMT_YUV422P:
		case PIX_FMT_YUV444P:
		case PIX_FMT_YUV422:
		case PIX_FMT_YUV410P:
		case PIX_FMT_YUV411P:
			return Py_BuildValue( "OOO", frame->cData[ 0 ], frame->cData[ 1 ], frame->cData[ 2 ] );
		// One plane
		default:
			Py_INCREF( frame->cData[ 0 ] );
			return frame->cData[ 0 ];
	};
}

// -------------------------
int PyVFrame2AVFrame(PyVFrameObject* cFrame, AVFrame *frame) 
{
	int iPlanes= 3, i;
  switch( cFrame->pix_fmt )
	{
		case PIX_FMT_RGB24:
		case PIX_FMT_BGR24:
		case PIX_FMT_RGBA32:
		case PIX_FMT_RGB565:
		case PIX_FMT_RGB555:
		case PIX_FMT_GRAY8:
		case PIX_FMT_MONOWHITE:
		case PIX_FMT_MONOBLACK:
		case PIX_FMT_PAL8:
			// 1 plane
			iPlanes= 1;
			break;
	}

	for( i= 0; i< iPlanes; i++) 
	{
		if (!cFrame->cData[i]) 
		{
			PyErr_Format(g_cErr,	"Frame plane structure incomplete. Plane %d is not found. At least %d planes should exists.", i, iPlanes );
			return -1;
		}

		frame->data[i] = (uint8_t*)PyString_AsString(cFrame->cData[i] );
		if (!i)
			frame->linesize[i] =  PyString_GET_SIZE( cFrame->cData[0] ) / frame_get_height(cFrame); //cFrame->cDec->cCodec->width;
		else
			frame->linesize[i] = PyString_GET_SIZE( cFrame->cData[0] ) / (2*frame_get_height(cFrame)); // cFrame->cDec->cCodec->width/2;
//		printf ("linesize:%d\n",frame->linesize[i]);
	}
	return 0;
}

// ---------------------------------------------------------------------------------
static PyObject * Frame_Convert( PyVFrameObject* obj, PyObject *args)
{
	PyVFrameObject *cSrc= obj;
	int iFormat, iWidth, iHeight, iDepth= 1, iPlanes= 3, i;
	PyObject *acPlanes[ 3 ]= { NULL, NULL, NULL };
	PyVFrameObject* cRes;

	// Create new AVPicture in a new format
	AVPicture cSrcPict;
	AVPicture cDstPict;

	if (!PyArg_ParseTuple(args, "i|(ii)", &iFormat, &iWidth, &iHeight ))
		return NULL;

	// Well, not very safe, but works
	PyVFrame2AVFrame( obj, (AVFrame*)&cSrcPict );
	memset( &cDstPict.data[ 0 ], 0, sizeof( cDstPict.data ) );

	// Create new VFrame
	cRes= (PyVFrameObject*)PyObject_New( PyVFrameObject, &VFrameType );
	if( !cRes )
		return NULL;
	// Start assembling result into frame
	memset( &cRes->cData[ 0 ], 0, sizeof( cRes->cData ) );

	// Find format by its id
	switch( iFormat )
	{
		case PIX_FMT_YUV420P:
		case PIX_FMT_YUV422:
		case PIX_FMT_YUV422P:
		case PIX_FMT_YUV444P:
		case PIX_FMT_YUV410P:
		case PIX_FMT_YUV411P:
		case PIX_FMT_YUVJ420P:
		case PIX_FMT_YUVJ422P:
		case PIX_FMT_YUVJ444P:
			// 3 planes
			acPlanes[ 1 ]= PyString_FromStringAndSize( NULL, cSrc->width* cSrc->height/ 4 );
			acPlanes[ 2 ]= PyString_FromStringAndSize( NULL, cSrc->width* cSrc->height/ 4 );
			cDstPict.data[ 1 ]= PyString_AsString( acPlanes[ 1 ] );
			cDstPict.data[ 2 ]= PyString_AsString( acPlanes[ 2 ] );
			cDstPict.linesize[ 1 ]= cDstPict.linesize[ 2 ]= cSrc->width/ 2;
			break;
		case PIX_FMT_RGBA32:
			iDepth++;
		case PIX_FMT_RGB24:
		case PIX_FMT_BGR24:
			iDepth++;
		case PIX_FMT_RGB565:
		case PIX_FMT_RGB555:
			iDepth++;
		case PIX_FMT_GRAY8:
		case PIX_FMT_MONOWHITE:
		case PIX_FMT_MONOBLACK:
		case PIX_FMT_PAL8:
			iPlanes= 1;
			break;
		default:
			// Raise an error if the format is not supported
			PyErr_Format( g_cErr, "Video frame with format %d cannot be created", iFormat );
			return NULL;
	}
	// 1 plane
	acPlanes[ 0 ]= PyString_FromStringAndSize( NULL, cSrc->width* cSrc->height* iDepth );
	cDstPict.linesize[ 0 ]= cSrc->width* iDepth;
	cDstPict.data[ 0 ]= PyString_AsString( acPlanes[ 0 ] );

	// Convert images
	if( img_convert( &cDstPict, iFormat, &cSrcPict, cSrc->pix_fmt, cSrc->width, cSrc->height )== -1 )
	{
		PyErr_Format( g_cErr, "Video frame with format %d cannot be converted to %d", cSrc->pix_fmt, iFormat );
		if( acPlanes[ 0 ] ) Py_DECREF( acPlanes[ 0 ] );
		if( acPlanes[ 1 ] ) Py_DECREF( acPlanes[ 1 ] );
		if( acPlanes[ 2 ] ) Py_DECREF( acPlanes[ 2 ] );
		return NULL;
	}

	//Preprocess_Frame(obj, picture,&buffer_to_free);
	cRes->aspect_ratio= cSrc->aspect_ratio;
	cRes->frame_rate= cSrc->frame_rate;
	cRes->frame_rate_base= cSrc->frame_rate_base;
	cRes->height= cSrc->height;
	cRes->width= cSrc->width;
	cRes->pix_fmt= iFormat;
	cRes->pict_type= cSrc->pict_type;

	// Copy string(s)
	for( i= 0; i< iPlanes; i++ )
		cRes->cData[ i ]= acPlanes[ i ];

	return (PyObject*)cRes;
}

// ---------------------------------------------------------------------------------
// List of all methods for the vcodec.decoder
static PyMethodDef frame_methods[] =
{
	{
		"convert",
		(PyCFunction)Frame_Convert,
		METH_VARARGS,
		CONVERT_DOC
	},
	{ NULL, NULL },
};

// ----------------------------------------------------------------
static PyMemberDef frame_members[] = 
{
	{BITRATE, T_INT, offsetof(PyVFrameObject,bit_rate), 0, "Bitrate of the stream. May change from frame to frame."},
	{FORMAT, T_INT, offsetof(PyVFrameObject,pix_fmt), 0, "Frame format as integer. See list of formats in vcodec.codecs"},
	{FRAME_RATE_ATTR, T_INT, offsetof(PyVFrameObject,frame_rate), 0, "Frame rate in units relative to the frame_base"},
	{FRAME_RATE_B_ATTR, T_INT, offsetof(PyVFrameObject,frame_rate_base), 0, "Frame rate base"},
	{ASPECT_RATIO, T_FLOAT, offsetof(PyVFrameObject,aspect_ratio), 0, "Picture default aspect ratio."},
	{RESYNC, T_INT, offsetof(PyVFrameObject,resync), 0, "Flag is set if resync still in place"},
	{PICT_TYPE, T_INT, offsetof(PyVFrameObject,pict_type), 0, "Type of the frame( 1-I, 2-P, 3-B ) "},
	{FRAME_NUMBER, T_INT, offsetof(PyVFrameObject,frame_number), 0, "Frame number in a stream"},
	{NULL},
};
 

// ----------------------------------------------------------------
static PyGetSetDef frame_getsetlist[] =
{
	{DATA, (getter)frame_get_data, NULL, "Data of the frame as a tuple of strings. Numbers of strings depend on frame type( 1 for RGB, 3 for component )"},
	{SIZE, (getter)frame_get_size, NULL, "Size of the frame in pixels( w, h )"},
	{0}
};

// ---------------------------------------------------------------------------------
static PyObject* PyNewVFrame( PyTypeObject *type, PyObject *args, PyObject *kwds )
{
	PyVFrameObject* cFrame;
	int iWidth, iHeight, iPixFmt;
	PyObject *y, *u, *v;

	if (!PyArg_ParseTuple(args, "i(ii)(OOO):", &iPixFmt, &iWidth, &iHeight, &y, &u, &v ))
		return NULL;

	// Create new VFrame object
	cFrame= (PyVFrameObject*)PyObject_New( PyVFrameObject, &VFrameType );
	if( !cFrame )
		return NULL;

	// Zeroing out frame
	memset( &cFrame->cData[ 0 ], 0, sizeof( cFrame->cData ) );

	// Store
	cFrame->cData[ 0 ]= y;
	Py_INCREF( y );
	cFrame->cData[ 1 ]= u;
	Py_INCREF( u );
	cFrame->cData[ 2 ]= v;
	Py_INCREF( v );
	cFrame->width= iWidth;
	cFrame->height= iHeight;
	cFrame->pix_fmt= iPixFmt;
	return (PyObject*)cFrame;
}

// ----------------------------------------------------------------
static PyTypeObject VFrameType =
{
	PyObject_HEAD_INIT(NULL)
	0,
	MODULE_NAME"."VFRAME_NAME,
	sizeof(PyVFrameObject),
	0,
	(destructor)FrameClose,  //tp_dealloc
	0,			  //tp_print
	0,				//tp_getattr
	0,			  //tp_setattr
	0,			  //tp_compare
	0,			  //tp_repr
	0,			  //tp_as_number
	0,			  //tp_as_sequence
	0,				//tp_as_mapping
	0,					/* tp_hash */
	0,					/* tp_call */
	0,					/* tp_str */
	PyObject_GenericGetAttr,		/* tp_getattro */
	0,		/* tp_setattro */
	0,					/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /* tp_flags */
	VFRAME_DOC,				/* tp_doc */
	0,					/* tp_traverse */
	0,					/* tp_clear */
	0,					/* tp_richcompare */
	0,					/* tp_weaklistoffset */
	0,		/* tp_iter */
	0,		/* tp_iternext */
	frame_methods,				/* tp_methods */
	frame_members,			/* tp_members */
	frame_getsetlist,			/* tp_getset */
	0,					/* tp_base */
	0,					/* tp_dict */
	0,					/* tp_descr_get */
	0,					/* tp_descr_set */
	0,					/* tp_dictoffset */
	0,			/* tp_init */
	PyType_GenericAlloc,			/* tp_alloc */
	PyNewVFrame,				/* tp_new */
	PyObject_Del,                           /* tp_free */ };

// ---------------------------------------------------------------------------------
//Preprocess frame.
// Only does deinterlacing for now
static void Preprocess_Frame(PyCodecObject* cObj, AVPicture *picture, void **bufp)
{
	AVCodecContext *dec;
	AVPicture *picture2;
	AVPicture picture_tmp;
	uint8_t *buf = 0;

	dec = cObj->cCodec;
	/* deinterlace : must be done before any resize */
	if ((cObj->iVcodecFlags & VCODEC_DEINTERLACE_FL) ||
			(cObj->iVcodecFlags & VCODEC_POSTPROC_FL)) {
		int size;
		/* create temporary picture */
		size = avpicture_get_size(dec->pix_fmt, dec->width, dec->height);
		buf = (uint8_t*)av_malloc(size);
		if (!buf)
			return;

		picture2 = &picture_tmp;
		avpicture_fill(picture2, buf, dec->pix_fmt, dec->width, dec->height);

		if (cObj->iVcodecFlags & VCODEC_DEINTERLACE_FL) {
			if(avpicture_deinterlace(picture2,
						picture,
						dec->pix_fmt,
						dec->width,
						dec->height) < 0) {
				/* if error, do not deinterlace */
				av_free(buf);
				buf = NULL;
				picture2 = picture;
			}
		} else {
			if (img_convert(picture2, dec->pix_fmt,
					picture, dec->pix_fmt,
					dec->width,
					dec->height) < 0) {
				/* if error, do not copy */
				av_free(buf);
				buf = NULL;
				picture2 = picture;
			}
		}
	} else {
		picture2 = picture;
	}

	//frame_hook_process(picture2, dec->pix_fmt, dec->width, dec->height);

	if (picture != picture2)
		*picture = *picture2;
	*bufp = buf;
}


// ---------------------------------------------------------------------------------
static PyVFrameObject* Create_New_PyVFrame( int pix_fmt, int h, AVPicture* picture, int resync) 
{
	PyVFrameObject* cFrame= (PyVFrameObject*)PyObject_New( PyVFrameObject, &VFrameType );
	if( !cFrame )
		return NULL;

	//Preprocess_Frame(obj, picture,&buffer_to_free);

	// Start assembling result into frame
	memset( &cFrame->cData[ 0 ], 0, sizeof( cFrame->cData ) );

	// Store
	if( !resync && picture->data[0] )
		switch( pix_fmt )
		{
			// Three planes
			case PIX_FMT_YUV420P:
			case PIX_FMT_YUV422P:
			case PIX_FMT_YUV444P:
			case PIX_FMT_YUV422:
			case PIX_FMT_YUV410P:
			case PIX_FMT_YUV411P:
				// Create three planes and copy data into it./
				if(!(cFrame->cData[0] = PyString_FromStringAndSize(
							(const char*)picture->data[0],
								picture->linesize[ 0 ]* h )) ||
					 !(cFrame->cData[1] = PyString_FromStringAndSize(
					   		(const char*)picture->data[1],
								picture->linesize[1]* h/2 )) ||
					 !(cFrame->cData[2] = PyString_FromStringAndSize(
					   		(const char*)picture->data[2],
								picture->linesize[2]* h/2 )))
				{
					return NULL;
				}
				break;
			// One plane
			default:
				cFrame->cData[0] = PyString_FromStringAndSize(
						(const char*)picture->data[0],
						picture->linesize[0]* h );
				break;
		}

	return cFrame; 
}

// ---------------------------------------------------------------------------------
// New frame for libavcodec codecs
//
static PyObject* Frame_New_LAVC( PyCodecObject* obj ) 
{
	PyVFrameObject* cRes= (PyVFrameObject*)Create_New_PyVFrame(
		obj->cCodec->pix_fmt,obj->cCodec->height, (AVPicture*)&obj->frame, obj->cCodec->resync);
	if( !cRes )
		return (PyObject*)cRes;

	cRes->aspect_ratio= obj->cCodec->aspect_ratio;
	cRes->frame_rate= obj->cCodec->frame_rate;
	cRes->frame_rate_base= obj->cCodec->frame_rate_base;
	cRes->pix_fmt= obj->cCodec->pix_fmt;
	cRes->width= obj->cCodec->width;
	cRes->height= obj->cCodec->height;
	cRes->bit_rate= obj->cCodec->bit_rate;
	cRes->resync= obj->cCodec->resync;
	cRes->pict_type= obj->frame.pict_type;
	cRes->frame_number= obj->cCodec->frame_number;
	return (PyObject*)cRes;
}

// ---------------------------------------------------------------------------------
static PyObject *
Codec_GetParams( PyCodecObject* obj, PyObject *args)
{
	PyObject* cRes= PyDict_New();
	if( !cRes )
		return NULL;

	SetAttribute_i( cRes, ID, obj->cCodec->codec_id );
	SetAttribute_i( cRes, TYPE, obj->cCodec->codec_type );
	SetAttribute_i( cRes, BITRATE, obj->cCodec->bit_rate );
	SetAttribute_i( cRes, WIDTH, obj->cCodec->width );
	SetAttribute_i( cRes, HEIGHT, obj->cCodec->height );
	SetAttribute_i( cRes, FRAME_RATE, obj->cCodec->frame_rate );
	SetAttribute_i( cRes, FRAME_RATE_B, obj->cCodec->frame_rate_base );
	SetAttribute_i( cRes, GOP_SIZE, obj->cCodec->gop_size);
	SetAttribute_i( cRes, MAX_B_FRAMES,obj->cCodec->max_b_frames);
	SetAttribute_i( cRes, DEINTERLACE, (obj->iVcodecFlags & VCODEC_DEINTERLACE_FL)?1:0);
	return cRes;
}

// ---------------------------------------------------------------------------------
static PyObject * Codec_Reset( PyCodecObject* obj)
{
	if( obj->cCodec->codec->resync )
		obj->cCodec->codec->resync( obj->cCodec );

	RETURN_NONE
}

// ---------------------------------------------------------------------------------
static PyObject *
Codec_Decode( PyCodecObject* obj, PyObject *args)
{
	PyObject* cRes= NULL;
	uint8_t *sData=NULL,*sBuf=NULL,*sData_ptr=NULL;
	int iLen=0, out_size=0, i=0, len=0, hurry= 0; 

	if (!PyArg_ParseTuple(args, "s#|i:decode", &sBuf, &iLen, &hurry ))
		return NULL;
 
	//need to add padding to buffer for libavcodec
	sData=(uint8_t*)av_malloc(iLen+FF_INPUT_BUFFER_PADDING_SIZE);
	memset (sData+ iLen,0,FF_INPUT_BUFFER_PADDING_SIZE);
	memcpy(sData,sBuf,iLen);
	sData_ptr= sData;

	while( iLen> 0 )
	{
		out_size= 0;
		obj->cCodec->hurry_up= hurry;
		len= obj->cCodec->codec->decode( obj->cCodec, &obj->frame, &out_size, sData, iLen );
		if( len < 0 )
		{
			// Need to report out the error( it should be in the error list )
			while( g_AvilibErr[ i ].iErrCode )
				if( g_AvilibErr[ i ].iErrCode== len )
				{
					PyErr_SetString(g_cErr, g_AvilibErr[ i ].sErrDesc );
					av_free(sData_ptr);
					return NULL;
				}
				else 
					i++;

			PyErr_Format(g_cErr, "Unspecified error %d. Cannot find any help text for it.", len );
			av_free(sData_ptr);
			return NULL;
		}
		else
		{
			iLen-= len;
			sData+= len;
			if( !cRes && out_size> 0 )  
				cRes= Frame_New_LAVC( obj );
		}
	}
#ifdef HAVE_MMX
    emms();
#endif 
		av_free(sData_ptr);

	if( cRes )
		return cRes;

	if( out_size )
		// Raise an error if data was found but no frames created
		return NULL;

	Py_INCREF( Py_None );
	return Py_None;
}
 
// ---------------------------------------------------------------------------------

static PyObject* Codec_Encode( PyCodecObject* obj, PyObject *args)
{
//#ifndef WIN32
	PyVFrameObject* cFrame = NULL;
	PyObject* cRes = NULL;
	int iLen = 0;
	AVFrame picture;

#define ENCODE_OUTBUF_SIZE 100000

	char sOutbuf[ ENCODE_OUTBUF_SIZE ];
	if (!PyArg_ParseTuple(args, "O", &cFrame ))
		return NULL;

	if (!(obj->cCodec ||obj->cCodec->codec)) 
	{
		PyErr_SetString(g_cErr, "Encode error:codec not initialized" );
		return NULL;
	}

	//reset codec parameters if frame size is  smaller than codec frame size
	if (!obj->cCodec->width || ! obj->cCodec->height) 
	{
		PyErr_SetString(g_cErr, "Encode: zero frame size set in codec" );
		return NULL;
	}

	if ((obj->cCodec->width > frame_get_width(cFrame)) ||
	   (obj->cCodec->height > frame_get_height(cFrame)) )
	{
		PyErr_SetString(g_cErr, "Encode: cannot change resolution for frame. Use scaling first..." );
		return NULL;
	}

	/* check codec params */
	PyVFrame2AVFrame(cFrame, &picture );
	iLen = avcodec_encode_video(obj->cCodec, sOutbuf, ENCODE_OUTBUF_SIZE,	&picture);
	if (iLen > 0)
		cRes = PyString_FromStringAndSize((const char*)sOutbuf,iLen );
	else
		PyErr_Format(g_cErr, "Failed to encode frame( error code is %d )", iLen );

#ifdef HAVE_MMX
    emms();
#endif 
 	return cRes;
}

// ---------------------------------------------------------------------------------
// List of all methods for the vcodec.decoder
static PyMethodDef decoder_methods[] =
{
	{
		RESET_NAME,
		(PyCFunction)Codec_Reset,
		METH_NOARGS,
		RESET_DOC
	},
	{
		GET_PARAM_NAME,
		(PyCFunction)Codec_GetParams,
		METH_NOARGS,
		GET_PARAM_DOC
	},
	{
		CONVERT_NAME,
		(PyCFunction)Codec_Decode,
		METH_VARARGS,
		CONVERT_DOC
	},
	{ NULL, NULL },
};

// ----------------------------------------------------------------
static void
CodecClose( PyCodecObject *obj )
{
	// Close avicodec first !!!
	avcodec_close(obj->cCodec);
	av_free( obj->cCodec );
	PyObject_Free( (PyObject*)obj );
}

// ---------------------------------------------------------------------------------
static PyCodecObject *
Codec_New( PyObject* cObj, PyTypeObject *type, int bDecoder )
{
	AVCodec *p;
	int iId, iRes;
	PyCodecObject* codec= (PyCodecObject* )type->tp_alloc(type, 0);
	if( !codec )
		return NULL;

		// Get id first if possible
	if( PyDict_Check( cObj ) )
		iId= PyInt_AsLong( PyDict_GetItemString( cObj, ID ));
	else
	{
		PyErr_Format(g_cErr, "Codec(): First parameter should be dict (codec id and params)" );
		return NULL;
	}

	p = ( bDecoder ) ? 
		avcodec_find_decoder((enum CodecID)iId):
		avcodec_find_encoder((enum CodecID)iId);

	// cleanup decoder data
	//codec->cCodec= (AVCodecContext*)av_mallocz( sizeof( AVCodecContext ));
	if( !p )
	{
		PyErr_Format(g_cErr, "cannot find codec with id %d. Check the id in params you pass.", iId );
		return NULL;
	}

  codec->cCodec= avcodec_alloc_context();
	if( !codec->cCodec )
	{
		PyErr_NoMemory();
		return NULL;
	}

	codec->cCodec->codec= p;
	// Populate some values from the dictionary
	iRes= SetCodecParams( codec, cObj );
	if( !iRes && !bDecoder )
		// There is some problem initializing codec with valid data
		return NULL;
	
	PyErr_Clear();
	if(p->capabilities & CODEC_CAP_TRUNCATED)
		codec->cCodec->flags |= CODEC_FLAG_TRUNCATED;

	avcodec_open( codec->cCodec, p );
	memset( &codec->frame, 0, sizeof( codec->frame ) );
	return codec;
}

// ---------------------------------------------------------------------------------
static PyObject *
DecoderNew( PyTypeObject *type, PyObject *args, PyObject *kwds )
{
	PyObject* cObj;
	if (!PyArg_ParseTuple(args, "O:", &cObj))
		return NULL;

	// Create new  codec
	return (PyObject*)Codec_New( cObj, type, 1 );
}
// ---------------------------------------------------------------------------------
/* Type object for socket objects. */
static PyTypeObject decoder_type = {
	PyObject_HEAD_INIT(NULL)	/* Must fill in type value later */
	0,					/* ob_size */
	MODULE_NAME"."DECODER_NAME,			/* tp_name */
	sizeof(PyCodecObject),		/* tp_basicsize */
	0,					/* tp_itemsize */
	(destructor)CodecClose,		/* tp_dealloc */
	0,					/* tp_print */
	0,					/* tp_getattr */
	0,					/* tp_setattr */
	0,					/* tp_compare */
	0,					/* tp_repr */
	0,					/* tp_as_number */
	0,					/* tp_as_sequence */
	0,					/* tp_as_mapping */
	0,					/* tp_hash */
	0,					/* tp_call */
	0,					/* tp_str */
	PyObject_GenericGetAttr,		/* tp_getattro */
	0,					/* tp_setattro */
	0,					/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /* tp_flags */
	decoder_doc,		/* tp_doc */
	0,					/* tp_traverse */
	0,					/* tp_clear */
	0,					/* tp_richcompare */
	0,					/* tp_weaklistoffset */
	0,					/* tp_iter */
	0,					/* tp_iternext */
	decoder_methods,				/* tp_methods */
	0,					/* tp_members */
	0,					/* tp_getset */
	0,					/* tp_base */
	0,					/* tp_dict */
	0,					/* tp_descr_get */
	0,					/* tp_descr_set */
	0,					/* tp_dictoffset */
	0,					/* tp_init */
	PyType_GenericAlloc,			/* tp_alloc */
	DecoderNew,	/* tp_new */
	PyObject_Del,				/* tp_free */
};

// ---------------------------------------------------------------------------------
// List of all methods for the vcodec.decoder
static PyMethodDef encoder_methods[] =
{
	{
		ENCODE_NAME,
		(PyCFunction)Codec_Encode,
		METH_VARARGS,
		ENCODE_DOC
	},
	{
		GET_PARAM_NAME,
		(PyCFunction)Codec_GetParams,
		METH_NOARGS,
		GET_PARAM_DOC
	},
	{ NULL, NULL },
};

// ---------------------------------------------------------------------------------
static PyObject *
EncoderNew( PyTypeObject *type, PyObject *args, PyObject *kwds )
{
	PyObject* cObj;

	if (!PyArg_ParseTuple(args, "O:"ENCODER_NAME, &cObj))
		return NULL;

	// Create new  codec
	return (PyObject*)Codec_New( cObj, type, 0 );
}

// ---------------------------------------------------------------------------------
/* Type object for socket objects. */
static PyTypeObject encoder_type = {
	PyObject_HEAD_INIT(NULL)	/* Must fill in type value later */
	0,					/* ob_size */
	MODULE_NAME"."ENCODER_NAME,			/* tp_name */
	sizeof(PyCodecObject),		/* tp_basicsize */
	0,					/* tp_itemsize */
	(destructor)CodecClose,		/* tp_dealloc */
	0,					/* tp_print */
	0,					/* tp_getattr */
	0,					/* tp_setattr */
	0,					/* tp_compare */
	0,					/* tp_repr */
	0,					/* tp_as_number */
	0,					/* tp_as_sequence */
	0,					/* tp_as_mapping */
	0,					/* tp_hash */
	0,					/* tp_call */
	0,					/* tp_str */
	PyObject_GenericGetAttr,		/* tp_getattro */
	0,					/* tp_setattro */
	0,					/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /* tp_flags */
	encoder_doc,		/* tp_doc */
	0,					/* tp_traverse */
	0,					/* tp_clear */
	0,					/* tp_richcompare */
	0,					/* tp_weaklistoffset */
	0,					/* tp_iter */
	0,					/* tp_iternext */
	encoder_methods,				/* tp_methods */
	0,					/* tp_members */
	0,					/* tp_getset */
	0,					/* tp_base */
	0,					/* tp_dict */
	0,					/* tp_descr_get */
	0,					/* tp_descr_set */
	0,					/* tp_dictoffset */
	0,					/* tp_init */
	PyType_GenericAlloc,			/* tp_alloc */
	EncoderNew,	/* tp_new */
	PyObject_Del,				/* tp_free */
};

// ---------------------------------------------------------------------------------
static PyObject * GetCodecID( PyObject* obj, PyObject *args)
{
	AVCodec *p;
	PyObject* cRes=NULL;
	char *sName=NULL;

	if (!PyArg_ParseTuple(args, "s", &sName ))
		return NULL;

	p = avcodec_find_encoder_by_name(sName);
	if( !p )
		p = avcodec_find_decoder_by_name(sName);

	if (p)
	{
		cRes = Py_BuildValue("i",p->id);
		return cRes;
	}
	else
	{
		PyErr_Format( g_cErr, "%s: no such codec exists", sName );
		return NULL;
	}
}

// ---------------------------------------------------------------------------------
// List of all methods for the mp3decoder
static PyMethodDef pyvcodec_methods[] =
{
	{
		GET_CODEC_ID_NAME,
		(PyCFunction)GetCodecID,
		METH_VARARGS,
		GET_CODEC_ID_DOC
	},
	{ NULL, NULL },
};

// ---------------------------------------------------------------------------------
PyMODINIT_FUNC
initvcodec(void)
{
	PyObject *cModule, *d;

	Py_Initialize();
	cModule= Py_InitModule(MODULE_NAME, pyvcodec_methods);
	d = PyModule_GetDict( cModule );

  	/* register all the codecs (you can also register only the codec you wish to have smaller code */
	avcodec_init();
	register_avcodec(&h263_decoder);
	register_avcodec(&mpeg4_decoder);
	register_avcodec(&msmpeg4v1_decoder);
	register_avcodec(&msmpeg4v2_decoder);
	register_avcodec(&msmpeg4v3_decoder);
	register_avcodec(&wmv1_decoder);
	register_avcodec(&wmv2_decoder);
	register_avcodec(&h263i_decoder);
	register_avcodec(&mpeg2video_decoder);
	register_avcodec(&mpeg1video_decoder);
	register_avcodec(&h264_decoder);
	register_avcodec(&mdec_decoder);
	register_avcodec(&svq1_decoder);
	register_avcodec(&svq3_decoder);
	
	register_avcodec(&mpeg4_encoder);
	register_avcodec(&mpeg1video_encoder);
	register_avcodec(&mpeg2video_encoder);
	register_avcodec(&msmpeg4v3_encoder);
	register_avcodec(&msmpeg4v2_encoder);
	register_avcodec(&msmpeg4v1_encoder);

	decoder_type.ob_type = &PyType_Type;
	Py_INCREF((PyObject *)&decoder_type);
	if (PyModule_AddObject(cModule, DECODER_NAME, (PyObject *)&decoder_type) != 0)
		return;

	encoder_type.ob_type = &PyType_Type;
	Py_INCREF((PyObject *)&encoder_type);
	if (PyModule_AddObject(cModule, ENCODER_NAME, (PyObject *)&encoder_type) != 0)
		return;

	// Do make it visible from the module level
	VFrameType.ob_type = &PyType_Type;
	Py_INCREF((PyObject *)&VFrameType);
	if (PyModule_AddObject(cModule, VFRAME_NAME, (PyObject *)&VFrameType) != 0)
		return;

 	PyModule_AddStringConstant( cModule, "__doc__", (char*)PYDOC );
	PyModule_AddStringConstant( cModule, "version", (char*)PYVERSION );
	PyModule_AddIntConstant( cModule, "build", PYBUILD );
	g_cErr= PyErr_NewException(MODULE_NAME".VCodecError", NULL, NULL);
	if( g_cErr )
		PyModule_AddObject( cModule, "VCodecError", g_cErr );

	if (PyType_Ready(&FormatsType) < 0)
		return;
	Py_INCREF( &FormatsType );

	PyModule_AddObject( cModule, "formats", Formats_New() );
}

  
/*

#! /bin/env python
import sys, os, time
import pymedia.video.muxer as muxer
import pymedia.video.vcodec as vcodec

def dumpVideo( inFile, outFilePattern, fmt, dummy= 1 ):
	if not dummy:
		import pygame
	dm= muxer.Demuxer( inFile.split( '.' )[ -1 ] )
	i= 1
	f= open( inFile, 'rb' )
	s= f.read( 400000 )
	r= dm.parse( s )
	v= filter( lambda x: x[ 'type' ]== muxer.CODEC_TYPE_VIDEO, dm.streams )
	if len( v )== 0:
		raise 'There is no video stream in a file %s' % inFile
	
	v_id= v[ 0 ][ 'index' ]
	print 'Assume video stream at %d index( %d ): ' % ( v_id, dm.streams[ v_id ][ 'id' ] )
	c= vcodec.Decoder( dm.streams[ v_id ] )
	frames= 0
	while len( s )> 0:
		for fr in r:
			if fr[ 0 ]== v_id:
				d= c.decode( fr[ 1 ] )
				# Save file as RGB24
				if d:
					if d.data:
						print d.rate, d.rate_base, float(d.rate_base)/d.rate
						frames+= 1
						if dummy== 0:
							dd= d.convert( fmt )
							print len( dd.data[ 0 ] ), dd.size
							if not dummy:
								img= pygame.image.fromstring( dd.data, dd.size, "RGB" )
								pygame.image.save( img, outFilePattern % i )
					else:
						print 'Empty frame'
					
					i+= 1
		
		s= f.read( 400000 )
		r= dm.parse( s )
	
	f.close()
	return frames

frames= dumpVideo( 'c:\\movies\\Who.Framed.Roger.Rabbit\\Who.Framed.Roger.Rabbit.1988.XviD-CD2.avi', 'c:\\bors\\hmedia\\libs\\pymedia\\examples\\dump\\test_%d.bmp', 2 )

t= time.time()
try: t= time.time()- t
except: t= time.time()- t

print 'Decoded %d frames in %.2f secs( %.2f fps )' % ( frames, t, float( frames )/ t )

*/


