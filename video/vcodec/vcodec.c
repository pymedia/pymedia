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

#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
 
#ifndef BUILD_NUM
#define BUILD_NUM 1
#endif

#if defined(CONFIG_WIN32)
static inline int strcasecmp(const char* s1, const char* s2) { return stricmp(s1,s2); }
#endif

const int PYBUILD= BUILD_NUM;
const char* PYVERSION= "1";
const char* PYDOC=
"Video data decoding/encoding routins\nAllows to :\n"
"\t- Decode/encode strings of video information into/from either planar or packed formats( such as YUV420, RGB24, etc )\n"
"\t- Provides all formats with the same interface\n";

#define CONVERT_DOC \
	  "decode( frameStr ) -> frame\n \
		Convert video frame of compressed data into decompressed VFrame of the same formt as source.\n"

#define GET_PARAM_DOC \
	"getParams() -> params\n\
		Parameters that represents the current state of codec\n"

#define ENCODE_DOC \
	"encode( frame, [ scale ]-> encoded string\n\
	        Encode frame"

#define GET_CODEC_ID_DOC \
	"getCodecID(name) - returns internal numerical codec id for its name"

// Frame attributes
#define FORMAT "format"
#define DATA "data"
#define SIZE "size"
#define FRAME_RATE_ATTR "rate"
#define FRAME_RATE_B_ATTR "rate_base"

//char* FOURCC= "fourcc";
char* TYPE= "type";
char* ID= "id";
char* BITRATE= "bitrate";
char* WIDTH= "width";
char* HEIGHT= "height";
char* FRAME_RATE= "frame_rate";
char* FRAME_RATE_B= "frame_rate_base";
char* GOP_SIZE="gop_size";
char* MAX_B_FRAMES="max_b_frames";
char* DEINTERLACE="deinterlace";

PyDoc_STRVAR(decoder_doc,
"Decoder( codecParams ) -> Decoder\n\
\n\
Creates new video decoder object based on codecParams. \n\
	codecParams - dictionary with the following items parameters:\n\
		'id', 'fourcc', 'type', 'bitrate', 'width', 'height', \n\
		'format', 'data', 'size', 'rate',\n\
		'rate_base', 'frame_rate', 'frame_rate_base', 'gop_size'\n\
		'max_b_frames', 'deinterlace'\n\
The list of available codecs may be accessed through vcodec.codecs\n\
All possible formats to the convert video frame to are available trough vcodec.formats\n\
Here is the example of simple video player( plays 3Mb only )\n\
( assume you have video stream with index 0 ):\n\n\
\timport muxer, vcodec\n\
\tdm= muxer.Demuxer( 'avi' )\n\
\tf= open( 'test.avi', 'rb' )\n\
\ts= f.read( 3000000 )\n\
\tr= dm.parse( s )\n\
\tvc= vcodec.Decoder( dm.formats[ 0 ] )\n\
\tfor d in r:\n\
\t\t# d is a tuple ( stream_index stream_data )\n\
\t\tvfr= vc.decode( d[ 1 ] )\n\
\n\
Methods of Decoder are:\n"
CONVERT_DOC
GET_PARAM_DOC );

PyDoc_STRVAR(encoder_doc,
"Encoder( codecParams ) -> Encoder\n\
\n\
Creates new video encoder object based codecParams. \n\
	codecParams - dictionary with the following items parameters:\n\
		'id', 'fourcc', 'type', 'bitrate', 'width', 'height', \n\
		'format', 'data', 'size', 'rate',\n\
		'rate_base', 'frame_rate', 'frame_rate_base', 'gop_size'\n\
		'max_b_frames', 'deinterlace'\n\
The list of available codecs may be accessed through vcodec.codecs\n\
All possible formats to the convert video frame to are available trough vcodec.formats\n\
Methods of Encider are:\n"
ENCODE_DOC);

#define RETURN_NONE return (Py_INCREF(Py_None), Py_None); 

PyObject *g_cErr;

#define VCODEC_IS_ENCODER_FL  0x01
#define VCODEC_DEINTERLACE_FL 0x02
#define VCODEC_POSTPROC_FL    0x04

#define CODEC_TYPE_ENCODE "encode"
#define CODEC_TYPE_DECODE "decode"
// ---------------------------------------------------------------------------------
static PyTypeObject PyFormatsType;

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
	PyCodecObject *cDec;
	PyObject* cData[ 3 ];
	//PyObject* iLineSize[3];
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
	{ AVILIB_SMALL_BUFFER, "Output buffer is too small. You may need to increase MAX_OUTFRAME_SIZE." },
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

// ---------------------------------------------------------------------------------
static PyObject * Codec_GetID( PyObject* obj, PyObject *args)
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

	Py_DECREF( frame->cDec );
	PyObject_Free( (PyObject*)frame );
}

// ---------------------------------------------------------------------------------
// List of all methods for the vcodec.decoder
static PyMethodDef frame_methods[] =
{
/*	{
		"convert",
		(PyCFunction)Frame_AsFormat,
		METH_VARARGS,
		CONVERT_DOC
	},*/
	{ NULL, NULL },
};

// ----------------------------------------------------------------
static PyObject *
frame_get_format(PyVFrameObject *frame, void *closure)
{
	return PyInt_FromLong( frame->cDec->cCodec->pix_fmt );
}

// ----------------------------------------------------------------

static int frame_get_width(PyVFrameObject *frame)
{
	return frame->cDec->cCodec->width;
}

// ----------------------------------------------------------------
static int frame_get_height(PyVFrameObject *frame)
{
	return frame->cDec->cCodec->height;
}

// ----------------------------------------------------------------
static PyObject *
frame_get_size(PyVFrameObject *frame, void *closure)
{
	return Py_BuildValue( "(ii)", frame_get_width(frame),
			frame_get_height(frame));

}
// ----------------------------------------------------------------
static PyObject *
frame_get_frame_r(PyVFrameObject *frame, void *closure)
{
		return PyInt_FromLong( frame->cDec->cCodec->frame_rate );
}
// ----------------------------------------------------------------
static PyObject *
frame_get_frame_r_b(PyVFrameObject *frame, void *closure)
{
	return PyInt_FromLong( frame->cDec->cCodec->frame_rate_base );
}
// ----------------------------------------------------------------
static PyObject *
frame_get_data(PyVFrameObject *frame, void *closure)
{
	switch( frame->cDec->cCodec->pix_fmt )
	{
		// Three planes
		case PIX_FMT_YUV420P:
		case PIX_FMT_YUV422P:
		case PIX_FMT_YUV444P:
		case PIX_FMT_YUV422:
		case PIX_FMT_YUV410P:
		case PIX_FMT_YUV411P:
			return Py_BuildValue( "O,O,O", frame->cData[ 0 ], frame->cData[ 1 ], frame->cData[ 2 ] );
		// One plane
		default:
			Py_INCREF( frame->cData[ 0 ] );
			return frame->cData[ 0 ];
	};
}

// ----------------------------------------------------------------
static PyGetSetDef frame_getsetlist[] =
{
	{FORMAT, (getter)frame_get_format, NULL, "Frame format as integer. See list of formats in vcodec.codecs"},
	{SIZE, (getter)frame_get_size, NULL, "Size of the frame in pixels( w, h )"},
	{FRAME_RATE_ATTR, (getter)frame_get_frame_r, NULL, "Frame rate in units relative to the frame_base"},
	{FRAME_RATE_B_ATTR, (getter)frame_get_frame_r_b, NULL, "Frame rate base"},
	{DATA, (getter)frame_get_data, NULL, "Data of the frame as a tuple of strings. Numbers of strings depend on frame type."},
	{0},
};

// ----------------------------------------------------------------
static PyTypeObject VFrameType =
{
	PyObject_HEAD_INIT(NULL)
	0,
	"VFrame",
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
	"",				/* tp_doc */
	0,					/* tp_traverse */
	0,					/* tp_clear */
	0,					/* tp_richcompare */
	0,					/* tp_weaklistoffset */
	0,		/* tp_iter */
	0,		/* tp_iternext */
	frame_methods,				/* tp_methods */
	0,			/* tp_members */
	frame_getsetlist,			/* tp_getset */
	0,					/* tp_base */
	0,					/* tp_dict */
	0,					/* tp_descr_get */
	0,					/* tp_descr_set */
	0,					/* tp_dictoffset */
	0,			/* tp_init */
	PyType_GenericAlloc,			/* tp_alloc */
	0,				/* tp_new */
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
static PyVFrameObject* Create_New_PyVFrame(PyCodecObject* obj,AVPicture* picture) {

	int h= obj->cCodec->height;
	PyVFrameObject* cFrame= (PyVFrameObject*)PyObject_New( PyVFrameObject, &VFrameType );
	if( !cFrame )
		return NULL;

	//Preprocess_Frame(obj, picture,&buffer_to_free);

	// Start assembling result into frame
	memset( &cFrame->cData[ 0 ], 0, sizeof( cFrame->cData ) );
	cFrame->cDec= obj;
	Py_INCREF( obj );


	// Store
	switch( cFrame->cDec->cCodec->pix_fmt )
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
static PyObject* Frame_New_LAVC( PyCodecObject* obj) {

	return (PyObject*)Create_New_PyVFrame(obj,(AVPicture*)&obj->frame);
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
static PyObject *
Codec_Decode( PyCodecObject* obj, PyObject *args)
{
	PyObject* cRes= NULL;
	uint8_t *sData=NULL,*sBuf=NULL,*sData_ptr=NULL;
	int iLen=0, out_size=0, i=0, len=0;

	if (!PyArg_ParseTuple(args, "s#:decode", &sBuf, &iLen ))
		return NULL;
 
	//need to add padding to buffer for libavcodec
	sData=(uint8_t*)av_malloc(iLen+FF_INPUT_BUFFER_PADDING_SIZE);
	memset (sData+ iLen,0,FF_INPUT_BUFFER_PADDING_SIZE);
	memcpy(sData,sBuf,iLen);
	sData_ptr= sData;

	while( iLen> 0 )
	{
		out_size= 0;
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
		}

		if (out_size > 0)
			if( !cRes )
				cRes= Frame_New_LAVC( obj );
	}

	av_free(sData_ptr);

	if( cRes )
		return cRes;
	Py_INCREF( Py_None );
	return Py_None;
}

// -------------------------
AVFrame *PyVFrame2AVFrame(PyVFrameObject* cFrame) {

	AVFrame* frame = NULL;
	int i;
	frame = avcodec_alloc_frame();
	if (!frame) {
		PyErr_NoMemory();
		return NULL;
	}

	for (i=0;i<3;i++) {
		if (!cFrame->cData[i]) {
			PyErr_SetString(g_cErr,
					"Frame layer structure incomplete" );
			return NULL;
		}

		frame->data[i] = (uint8_t*)PyString_AsString(cFrame->cData[i] );
		if (!i)
			frame->linesize[i] =  PyString_GET_SIZE( cFrame->cData[0] ) / frame_get_height(cFrame); //cFrame->cDec->cCodec->width;
		else
			frame->linesize[i] = PyString_GET_SIZE( cFrame->cData[0] ) / (2*frame_get_height(cFrame)); // cFrame->cDec->cCodec->width/2;
//		printf ("linesize:%d\n",frame->linesize[i]);
	}

	return frame;
}

// ---------------------------------------------------------------------------------

static PyObject* Codec_Encode( PyCodecObject* obj, PyObject *args)
{
//#ifndef WIN32
	PyVFrameObject* cFrame = NULL;
	PyObject* cRes = NULL;
	int iLen = 0;
	AVFrame *picture;

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
	picture = PyVFrame2AVFrame(cFrame);
	if (!picture) 
		return NULL;

	iLen = avcodec_encode_video(obj->cCodec, sOutbuf, ENCODE_OUTBUF_SIZE,	picture);
	if (iLen > 0)
		cRes = PyString_FromStringAndSize((const char*)sOutbuf,iLen );
	else
		PyErr_Format(g_cErr, "Failed to encode frame( error code is %d )", iLen );

	av_free(picture);
	return cRes;
}

// ---------------------------------------------------------------------------------
// List of all methods for the vcodec.decoder
static PyMethodDef decoder_methods[] =
{
	{
		"getParams",
		(PyCFunction)Codec_GetParams,
		METH_NOARGS,
		GET_PARAM_DOC
	},
	{
		"decode",
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
	if (!PyArg_ParseTuple(args, "O:Decoder", &cObj))
		return NULL;

	// Create new  codec
	return (PyObject*)Codec_New( cObj, type, 1 );
}
// ---------------------------------------------------------------------------------
/* Type object for socket objects. */
static PyTypeObject decoder_type = {
	PyObject_HEAD_INIT(NULL)	/* Must fill in type value later */
	0,					/* ob_size */
	"pymedia.video.vcodec.Decoder",			/* tp_name */
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
		"encode",
		(PyCFunction)Codec_Encode,
		METH_VARARGS,
		ENCODE_DOC
	},
	{
		"getParams",
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

	if (!PyArg_ParseTuple(args, "O:Encoder", &cObj))
		return NULL;

	// Create new  codec
	return (PyObject*)Codec_New( cObj, type, 0 );
}

// ---------------------------------------------------------------------------------
/* Type object for socket objects. */
static PyTypeObject encoder_type = {
	PyObject_HEAD_INIT(NULL)	/* Must fill in type value later */
	0,					/* ob_size */
	"pymedia.video.vcodec.Encoder",			/* tp_name */
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
// List of all methods for the mp3decoder
static PyMethodDef pyvcodec_methods[] =
{
	{
		"getCodecID",
		(PyCFunction)Codec_GetID,
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
	cModule= Py_InitModule("vcodec", pyvcodec_methods);
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

	register_avcodec(&mpeg4_encoder);
	register_avcodec(&mpeg1video_encoder);
	register_avcodec(&mpeg2video_encoder);
	register_avcodec(&msmpeg4v3_encoder);
	register_avcodec(&msmpeg4v2_encoder);
	register_avcodec(&msmpeg4v1_encoder);

	decoder_type.ob_type = &PyType_Type;
	Py_INCREF((PyObject *)&decoder_type);
	if (PyModule_AddObject(cModule, "Decoder", (PyObject *)&decoder_type) != 0)
		return;

	encoder_type.ob_type = &PyType_Type;
	Py_INCREF((PyObject *)&encoder_type);
	if (PyModule_AddObject(cModule, "Encoder", (PyObject *)&encoder_type) != 0)
		return;

 	PyModule_AddStringConstant( cModule, "__doc__", (char*)PYDOC );
	PyModule_AddStringConstant( cModule, "version", (char*)PYVERSION );
	PyModule_AddIntConstant( cModule, "build", PYBUILD );
	g_cErr= PyErr_NewException("pymedia.video.vcodec.Error", NULL, NULL);
	if( g_cErr )
		PyModule_AddObject( cModule, "error", g_cErr );

	if (PyType_Ready(&FormatsType) < 0)
		return;
	Py_INCREF( &FormatsType );

	PyModule_AddObject( cModule, "formats", Formats_New() );
}

 
/*

import pymedia.video.muxer as muxer
import pymedia.video.vcodec as vcodec

def recode( inFile, outFile, outCodec ):
	dm= muxer.Demuxer( inFile.split( '.' )[ -1 ] )
	f= open( inFile, 'rb' )
	fw= open( outFile, 'wb' )
	s= f.read( 400000 )
	r= dm.parse( s )
	print dm.streams
	v= filter( lambda x: x[ 'type' ]== muxer.CODEC_TYPE_VIDEO, dm.streams )
	if len( v )== 0:
		raise 'There is no video stream in a file %s' % inFile
	
	v_id= v[ 0 ][ 'index' ]
	print 'Assume video stream at %d index: ' % v_id
	c= vcodec.Decoder( dm.streams[ v_id ] )
	e= None
	while len( s )> 0:
		for fr in r:
			if fr[ 0 ]== v_id: 
				print len( fr[ 1 ] )
				d= c.decode( fr[ 1 ] )
		
		s= f.read( 400000 )
		r= dm.parse( s )

recode( 'c:\\movies\\avseq01.dat', 'test.mpg', 'mpeg1video' )

*/


