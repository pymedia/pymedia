/*
 *			A light hw decompression library for mpeg2
 *      It is only a wrapper for the underlying hw_codec.c
 *
 *
 * Copyright (C) 2002-2004  Dmitry Borisov
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

#include "hw_mpeg.h"
#include "libavcodec/avcodec.h"
 
#ifndef BUILD_NUM
#define BUILD_NUM 1
#endif

#if defined(CONFIG_WIN32)
static inline int strcasecmp(const char* s1, const char* s2) { return stricmp(s1,s2); }
#endif

#define HW_PLATFORM "cle266"

#define MODULE_NAME "pymedia.video.ext_codecs."HW_PLATFORM
#define EXT_MODULE_NAME "pymedia.video.ext_codecs"

const int PYBUILD= BUILD_NUM;
const char* PYVERSION= "1";
const char* PYDOC= "MPEG HW decoding support";

// Module funcs
#define ID_NAME "id"
#define PROBE_NAME "probe"

#define DECODER_NAME "Decoder"

// Decoder obj funcs
#define CONVERT_NAME "decode"
#define GET_PARAM_NAME "getParams"
#define RESET_NAME "reset"
#define SET_BUFFERS_NAME "setBuffers"

#define CONVERT_DOC \
	  CONVERT_NAME"( frameStr ) -> frame\n \
Convert video frame of compressed data into decompressed VFrame of the same format as source.\n\
It can raise exception in case if it needs buffers to be set. In that case you have to allocate \n\
buffers in VRAM and pass them to the codec"

#define GET_PARAM_DOC \
	GET_PARAM_NAME"() -> params\n\
Parameters that represents the current state of codec\n"

#define RESET_DOC \
	RESET_NAME"() -> None\nReset current state of codec\n"

#define PROBE_DOC \
	PROBE_NAME"() -> None\nCheck if HW decoder can run on the platform\n"

#define SET_BUFFERS_DOC \
	SET_BUFFERS_NAME"( stride, offs1, offs2, offs3, offs4 ) -> None\nSet the hw buffers for the hw decoder to work\n\
You should only do this if NeedBuffers exception is raised when you do decode()"

// Frame attributes
#define FORMAT "format"
#define DATA "data"
#define INDEX "index"
#define SIZE "size"
#define FRAME_RATE_ATTR "rate"
#define FRAME_RATE_B_ATTR "rate_base"
#define ASPECT_RATIO "aspect_ratio"
#define BITRATE "bitrate"
#define RESYNC "resync"
#define DISPLAY_NAME "display"

#define DISPLAY_DOC \
	DISPLAY_NAME"( surf ) -> None\nDisplaying HW surface on passed surface. \nThe passed surface should reside in HW and have \n" \
	" get_offset(), lock() and unlock() \n" \
	"functions defined. "DISPLAY_NAME"() will also perform all needed picture transformations \n defined for the codec in init params.\n"

//char* FOURCC= "fourcc";
char* TYPE= "type";
char* ID= "id";
char* WIDTH= "width";
char* HEIGHT= "height";
char* FRAME_RATE= "frame_rate";
char* FRAME_RATE_B= "frame_rate_base";
char* GOP_SIZE="gop_size";
char* MAX_B_FRAMES="max_b_frames";

#define RETURN_NONE return (Py_INCREF(Py_None), Py_None); 

PyObject *g_cErr;
PyObject *g_cNeedBuffersErr;

// ---------------------------------------------------------------------------------
typedef struct
{
	PyObject_HEAD
	MpegContext cCodec;
} PyCodecObject;

// ---------------------------------------------------------------------------------
typedef struct
{
	PyObject_HEAD
	int width;
	int height;
	float aspect_ratio;
	int frame_rate;
	int frame_rate_base;
	int bit_rate;
	int resync;
	int index;
} PyVFrameObject;

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

// ----------------------------------------------------------------
static void
FrameClose( PyVFrameObject *frame )
{
	PyObject_Free( (PyObject*)frame );
}

// ---------------------------------------------------------------------------------
/*
static PyObject *
Frame_Display( PyVFrameObject* obj, PyObject *args)
{
	PyObject* cSurf, *cRes, iOffs;
	if (!PyArg_ParseTuple(args, "O:"DISPLAY_NAME, &cSurf ))
		return NULL;
	
	// See if surface has lock(), unlock() and get_offset() funcs
	if( !PyObject_HasAttrString( cSurf, "lock" ) || 
			!PyObject_HasAttrString( cSurf, "unlock" ) || 
			!PyObject_HasAttrString( cSurf, "get_offset" ))
	{
		PyErr_SetString( g_cErr, "lock, unlock or get_offset functions are not defined for the surface" );
		return NULL;
	}

	cRes=  PyObject_CallMethod( cSurf, "lock", NULL );
	if( !cRes )
		return NULL;

	cRes=  PyObject_CallMethod( cSurf, "get_offset", NULL );
	if( !cRes )
	{
		PyObject_CallMethod( cSurf, "unlock", NULL );
		return NULL;
	}

	// Get offset and pass it to the HW blit function( should pass some flags for video engone, but not now.. )
	if( !PyArg_ParseTuple( cRes, "i", &iOffs ))
	{
		PyObject_CallMethod( cSurf, "unlock", NULL );
		return NULL;
	}

	mpeg_blit_surface( iOffs, obj->index );
	if( !PyObject_CallMethod( cSurf, "unlock", NULL ) )
		return NULL;

	RETURN_NONE
}
*/

// ---------------------------------------------------------------------------------
// List of all methods for the vcodec.decoder
static PyMethodDef frame_methods[] =
{
/*	{
		DISPLAY_NAME,
		(PyCFunction)Frame_Display,
		METH_VARARGS,
		DISPLAY_DOC
	},
*/
	{ NULL, NULL },
};

// ----------------------------------------------------------------
static PyObject *
frame_get_size(PyVFrameObject *frame, void *closure)
{
	return Py_BuildValue( "(ii)", frame->width,	frame->height);
}

// ----------------------------------------------------------------
static PyObject *
frame_get_data(PyVFrameObject *frame, void *closure)
{
	PyErr_SetString( g_cErr, DATA" property is not supported on HW codecs due to HW limitations" );
	return NULL;
}

// ----------------------------------------------------------------
static PyMemberDef frame_members[] = 
{
	{BITRATE, T_INT, offsetof(PyVFrameObject,bit_rate), 0, "Bitrate of the stream. May change from frame to frame."},
	{FRAME_RATE_ATTR, T_INT, offsetof(PyVFrameObject,frame_rate), 0, "Frame rate in units relative to the frame_base"},
	{FRAME_RATE_B_ATTR, T_INT, offsetof(PyVFrameObject,frame_rate_base), 0, "Frame rate base"},
	{ASPECT_RATIO, T_FLOAT, offsetof(PyVFrameObject,aspect_ratio), 0, "Picture default aspect ratio."},
	{RESYNC, T_INT, offsetof(PyVFrameObject,resync), 0, "Flag is set if resync still in place"},
	{INDEX, T_INT, offsetof(PyVFrameObject,index), 0, "Index of the surface where the frame is"},
	{NULL},
};
 

// ----------------------------------------------------------------
static PyGetSetDef frame_getsetlist[] =
{
	{SIZE, (getter)frame_get_size, NULL, "Size of the frame in pixels( w, h )"},
	{DATA, (getter)frame_get_data, NULL, "Get data is not supported for HW decoders"},
	{0}
};

// ----------------------------------------------------------------
static PyTypeObject VFrameType =
{
	PyObject_HEAD_INIT(NULL)
	0,
	MODULE_NAME".VFrame",
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
	frame_members,			/* tp_members */
	frame_getsetlist,			/* tp_getset */
	0,					/* tp_base */
	0,					/* tp_dict */
	0,					/* tp_descr_get */
	0,					/* tp_descr_set */
	0,					/* tp_dictoffset */
	0,			/* tp_init */
	PyType_GenericAlloc,			/* tp_alloc */
	0,				/* tp_new */
	PyObject_Del,                           /* tp_free */ 
};

// ---------------------------------------------------------------------------------
// New frame for libavcodec codecs
//
static PyObject* Frame_New_LAVC( PyCodecObject* obj ) 
{
	PyVFrameObject* cRes= (PyVFrameObject*)PyObject_New( PyVFrameObject, &VFrameType );
	if( !cRes )
		return NULL;

	cRes->aspect_ratio= obj->cCodec.aspect_ratio;
	cRes->frame_rate= obj->cCodec.frame_rate;
	cRes->frame_rate_base= obj->cCodec.frame_rate_base;
	cRes->width= obj->cCodec.width;
	cRes->height= obj->cCodec.height;
	cRes->bit_rate= obj->cCodec.bit_rate;
	cRes->resync= obj->cCodec.resync;
	cRes->index= mpeg_index( &obj->cCodec );
	return (PyObject*)cRes;
}

// ---------------------------------------------------------------------------------
static PyObject *
Codec_GetParams( PyCodecObject* obj, PyObject *args)
{
	PyObject* cRes= PyDict_New();
	if( !cRes )
		return NULL;

	SetAttribute_i( cRes, ID, CODEC_ID_MPEG2VIDEO );
	SetAttribute_i( cRes, BITRATE, obj->cCodec.bit_rate );
	SetAttribute_i( cRes, WIDTH, obj->cCodec.width );
	SetAttribute_i( cRes, HEIGHT, obj->cCodec.height );
	SetAttribute_i( cRes, FRAME_RATE, obj->cCodec.frame_rate );
	SetAttribute_i( cRes, FRAME_RATE_B, obj->cCodec.frame_rate_base );
	SetAttribute_i( cRes, GOP_SIZE, obj->cCodec.gop_size);
	SetAttribute_i( cRes, MAX_B_FRAMES,obj->cCodec.max_b_frames);
	return cRes;
}

// ---------------------------------------------------------------------------------
static PyObject * Codec_Reset( PyCodecObject* obj)
{
	if( !obj->cCodec.resync )
		mpeg_reset( &obj->cCodec );

	RETURN_NONE
}

// ---------------------------------------------------------------------------------
static PyObject *
Codec_SetBuffers( PyCodecObject* obj, PyObject *args)
{
	// Set buffers for hw decoding
	// The format is: ( base, stride, offs1, offs2, offs3, offs4 )
	int iOffs1, iOffs2, iOffs3, iOffs4, iStride;
	if (!PyArg_ParseTuple(args, "i(iiii):setBuffers", &iStride, &iOffs1, &iOffs2, &iOffs3, &iOffs4 ))
		return NULL;

	mpeg_set_surface( &obj->cCodec, 0, iOffs1, iStride );
	mpeg_set_surface( &obj->cCodec, 1, iOffs2, iStride );
	mpeg_set_surface( &obj->cCodec, 2, iOffs3, iStride );
	mpeg_set_surface( &obj->cCodec, 3, iOffs4, iStride );
  mpeg_set_fb_stride(&obj->cCodec, iStride );
 
	RETURN_NONE
}

// ---------------------------------------------------------------------------------
static PyObject *
Codec_Decode( PyCodecObject* obj, PyObject *args)
{
	PyObject* cRes= NULL;
	uint8_t *sData=NULL,*sBuf=NULL,*sData_ptr=NULL;
	int iLen=0, out_size=0, len=0, hurry= 0;

	if (!PyArg_ParseTuple(args, "s#|i:decode", &sBuf, &iLen, &hurry ))
		return NULL;
 
	//need to add padding to buffer for libavcodec
	if( iLen )
	{
		sData=(uint8_t*)av_malloc(iLen+FF_INPUT_BUFFER_PADDING_SIZE);
		memset (sData+ iLen,0,FF_INPUT_BUFFER_PADDING_SIZE);
		memcpy(sData,sBuf,iLen);
		sData_ptr= sData;
	}

	do
	{
		out_size= 0;
//printf( "Before decoding frame\n" );
	  //Py_BEGIN_ALLOW_THREADS
		len= mpeg_decode_frame( &obj->cCodec, NULL, &out_size, sData, iLen );
	  //Py_END_ALLOW_THREADS
//printf( "After frame is decoded %d\n", len );
		if( len == ERROR_NEED_BUFFERS )
		{
			PyErr_Format(g_cNeedBuffersErr, "Codec need at least %d buffers of HW memory for decoding", 4 );
			if( sData_ptr )
				av_free(sData_ptr);
			return NULL;
		}
		if( len < 0 )
		{
			PyErr_Format(g_cErr, "Unspecified error %d. Cannot find any help text for it.", len );
			if( sData_ptr )
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
	while( iLen> 0 );

	if( sData_ptr )
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
// List of all methods for the vcodec.decoder
static PyMethodDef decoder_methods[] =
{
	{
		SET_BUFFERS_NAME,
		(PyCFunction)Codec_SetBuffers,
		METH_VARARGS,
		SET_BUFFERS_DOC
	},
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
	mpeg_close(&obj->cCodec);
	PyObject_Free( (PyObject*)obj );
}

// ---------------------------------------------------------------------------------
static PyObject *
CodecNew( PyTypeObject *type, PyObject *args, PyObject *kwds )
{
	PyObject* cObj;
	PyCodecObject* codec;
	if (!PyArg_ParseTuple(args, "O:", &cObj))
		return NULL;

	// Create new  codec
	codec= (PyCodecObject* )type->tp_alloc(type, 0);
	if( !codec )
		return NULL;

	// Populate some values from the dictionary
	mpeg_init( &codec->cCodec );

	return (PyObject*)codec;
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
	0,		/* tp_doc */
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
	CodecNew,	/* tp_new */
	PyObject_Del,				/* tp_free */
};

// ----------------------------------------------------------------
static PyObject*
Probe( PyObject *obj )
{
	if( !mpeg_probe() )
	{
		PyErr_Format(g_cErr, HW_PLATFORM" HW codec is not supported on this platform" );
		return NULL;
	}
	RETURN_NONE
}

// ---------------------------------------------------------------------------------
static PyObject *
SavePGM( PyObject* obj, PyObject *args)
{
	char *s;
	void* pBuf;
	int w, h;
	if (!PyArg_ParseTuple(args, "siii:", &s, &pBuf, &w, &h ))
		return NULL;
	{
    FILE *f;
    f=fopen(s,"w");
    fprintf(f,"P5\n%d %d\n%d\n", w, h, 255);
    fwrite(pBuf,1, w* h,f);
    fclose(f); 
	}
	RETURN_NONE
}

// ---------------------------------------------------------------------------------
// List of all methods for the mp3decoder
static PyMethodDef pympeg_codec_methods[] =
{
	{
		PROBE_NAME,
		(PyCFunction)Probe,
		METH_NOARGS,
		PROBE_DOC
	},
	{
		"savePGM",
		(PyCFunction)SavePGM,
		METH_VARARGS,
		"do not use, for testing purposes only!!"
	},
	{ NULL, NULL },
};

// ---------------------------------------------------------------------------------
#define ENTRY_POINT( name ) \
PyMODINIT_FUNC \
init##name(void)\
{\
	PyObject *cModule, *d;\
\
	Py_Initialize();\
	cModule= Py_InitModule(MODULE_NAME, pympeg_codec_methods);\
	d = PyModule_GetDict( cModule );\
\
	decoder_type.ob_type = &PyType_Type;\
	Py_INCREF((PyObject *)&decoder_type);\
	if (PyModule_AddObject(cModule, DECODER_NAME, (PyObject *)&decoder_type) != 0)\
		return;\
\
	VFrameType.ob_type = &PyType_Type;\
	Py_INCREF((PyObject *)&VFrameType);\
\
	g_cErr= PyErr_NewException(MODULE_NAME".Error", NULL, NULL);\
	g_cNeedBuffersErr= PyErr_NewException(EXT_MODULE_NAME".NeedBufferException", NULL, NULL);\
\
 	PyModule_AddStringConstant( cModule, "name", HW_PLATFORM ); \
 	PyModule_AddIntConstant( cModule, "id", CODEC_ID_MPEG2VIDEO ); \
}

ENTRY_POINT( cle266 )

 
/*

#! /bin/env python
import sys, os, traceback
import pymedia
import pymedia.video.muxer as muxer
import pymedia.video.vcodec as vcodec

def dumpVideo( inFile ):
	#print 'Display is set!!!'
	ovl= None
	dm= muxer.Demuxer( inFile.split( '.' )[ -1 ].lower() )
	i= 1
	f= open( inFile, 'rb' )
	s= f.read( 400000 )
	r= dm.parse( s )
	#print dm.streams
	v= filter( lambda x: x[ 'type' ]== muxer.CODEC_TYPE_VIDEO, dm.streams )
	if len( v )== 0:
		raise 'There is no video stream in a file %s' % inFile
	
	v_id= v[ 0 ][ 'index' ]
	print 'Assume video stream at %d index( %d ): ' % ( v_id, dm.streams[ v_id ][ 'id' ] )
	c= pymedia.video.ext_codecs.Decoder( dm.streams[ v_id ] )
	#print 'Decoder created !!!'
	while len( s )> 0 and i< 20:
		for fr in r:
			if fr[ 0 ]== v_id:
				try:
					d= c.decode( fr[ 1 ] )
				except:
					cls= sys.exc_info()[ 0 ].__name__
					print cls
					if cls.index( 'NeedBuffer' )!= -1:
						params= c.getParams()
						print params
						size= params[ 'width' ], params[ 'height' ]
						c.setBuffers( 1, [ 1, 2, 3, 4 ] )
						d= c.decode(fr[ 1 ])
					else:
						raise
				
				# Save file as RGB24
				if d:
					print 'Frame', i, 'index', d.index, d.size
					i+= 1
		
		s= f.read( 400000 )
		r= dm.parse( s )

dumpVideo( 'D:\\VIDEO_TS\\VTS_01_1.VOB' )


#! /bin/env python
import sys, os, traceback, time
import pymedia
import pydfb
import pymedia.video.muxer as muxer
import pymedia.video.vcodec as vcodec

def playVideo( fileName ):
	pydfb.init()
	screen= pydfb.display.set_mode( (800,600), 0, 16 )
	ovl= None
	dm= muxer.Demuxer( fileName.split( '.' )[ -1 ].lower() )
	i= 1
	f= open( fileName, 'rb' )
	s= f.read( 400000 )
	r= dm.parse( s )
	#print dm.streams
	v= filter( lambda x: x[ 'type' ]== muxer.CODEC_TYPE_VIDEO, dm.streams )
	if len( v )== 0:
		raise 'There is no video stream in a file %s' % fileName
	
	v_id= v[ 0 ][ 'index' ]
	#print 'Assume video stream at %d index( %d ): ' % ( v_id, dm.streams[ v_id ][ 'id' ] )
	c= pymedia.video.ext_codecs.Decoder( dm.streams[ v_id ] )
	#print 'Decoder created !!!'
	t= time.time()
	while len( s )> 0 and i< 3000:
		for fr in r:
			if fr[ 0 ]== v_id:
				try:
					d= c.decode( fr[ 1 ] )
				except:
					cls= sys.exc_info()[ 0 ].__name__
					if cls.find( 'NeedBuffer' )!= -1:
						params= c.getParams()
						#print 'Before creating surfaces ', params
						size= params[ 'width' ], params[ 'height' ]
						b= [ pydfb.Surface( size, 0, pydfb.PF_YV12 ) for x in range( 4 ) ]
						st= [ x.lock() for x in b ]
						offs= [ x.get_offset() for x in b ]
						c.setBuffers( st[ 0 ][ 1 ], offs )
						d= c.decode(fr[ 1 ])
					else:
						raise
				
				# Show frame on the screen
				if d:
					print 'Frame', i, 'index', d.index, d.size
					i+= 1
					#pymedia.video.ext_codecs.cle266.savePGM( 'examples/dump/res_%03d.pgm' % i, st[ d.index ][ 0 ], st[ d.index ][ 1 ], d.size[ 1 ] )
					if not ovl:
						ovl= pydfb.Overlay( pydfb.PF_YV12, d.size, (0,0)+ d.size )
						ovls= ovl.surface()
					
					ovls.blit( b[ d.index ], (0,0) )
		
		s= f.read( 400000 )
		r= dm.parse( s )
	
	[ x.unlock() for x in b ]
	pydfb.quit()
	print '%.2f frames/sec' % ( float(i)/ ( time.time()- t ) )

playVideo( '/home/movies/Vts_10_11.vob' )

*/
