/*
 *			Extra light muxer/demuxer library for different file types
 *      such as avi, wmv, asf and so forth
 *      The list of all possible formats is available through 'formats' call
 *			The easiest way to add format is to use one from the libavformat project...
 *
 *
 *	Copyright (C) 2002-2003  Dmitry Borisov, Fedor Druzhinin
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

#include "muxer.h"

//#include "libavcodec/avcodec.h"
//#include "libavformat/avformat.h"

#ifndef BUILD_NUM
#define BUILD_NUM 1
#endif

const int PYBUILD= BUILD_NUM;
const char* PYVERSION= "2";
const char* PYDOC=
"Muxer allows to:\n"
"\t- Identify the format of individual streams inside compound streams such as asf, avi, mov etc\n"
"\t- Demux stream into the separate streams\n";

#define STREAMS "streams"
#define PARSE "parse"
#define RESET "reset"
#define HAS_HEADER "hasHeader"

#define PARSE_DOC \
	PARSE"( fragment ) -> streams\n \
	Parses stream and returns sub stream data in order it appear in the fragment"

#define RESET_DOC \
	RESET"() -> \n \
	Reset demultiplexer buffers"

#define STREAMS_DOC \
	STREAMS" -> streams\n \
	Returns list of streams within after the header is read in order they listed in a master stream.\n \
	It may contain the following attributes:\n \
		id	- internal stream identifier compatible with vcodec\n \
		fourcc - fourcc stream identifier if any \n\
		type - stream type{ video= 0 | audio= 1 | text= ?? }\n\
		bitrate - stream bitrate\n\
		width - picture width if any\n\
		height - picture height if any"

#define HAS_HEADER_DOC \
	HAS_HEADER"() -> { 1 | 0 }\n \
	Returns whether header presented or not in a stream. \n\
	You should not rely on 'streams' call if this function returns 0.\n\
	Some values may be used still, such as 'id' and 'type'\n\
	You should call parse() at least once before you can call this method.\n"

#define DEMUXER_DOC \
	"Demuxer( ext ) -> demuxer\n \
	Returns demuxer object based on extension passed. \nIt can demux stream into separate streams based on a \
stream type. Once demuxer is created the following methods are available:\n"\
"\t"HAS_HEADER_DOC \
"\t"PARSE_DOC \
"\t"RESET_DOC

PyObject *g_cErr;

// ---------------------------------------------------------------------------------
typedef struct
{
	PyObject_HEAD
	// Current strean context
	AVFormatContext ic;
	// Current packet buffer
	AVPacket pkt;
	// Whether header was found for the stream
	bool bHasHeader;
	// Whether we tried to get header from the stream
	bool bTriedHeader;
	PyObject* cBuffer;
} PyDemuxerObject;

/* -----------------------------------------------------------------------------------------------*/
typedef struct AVILIBError
{
	int iErrCode;
	const char* sErrDesc;
} AVILIBError;

/* -----------------------------------------------------------------------------------------------*/
AVILIBError g_AvilibErr[]= {
	{ AVILIB_NO_ERROR, "Generic error. No further help available." },
	{ AVILIB_NO_HEADER, "There is no header in a file where it should be" },
	{ AVILIB_BAD_FORMAT, "The format of file is wrong." },
	{ AVILIB_BAD_HEADER, "The header of the file is corrupted." },
	{ AVILIB_ENCRYPTED, "The stream is encrypted and cannot be processed by codec." },
	{ 0, NULL }
};

// ---------------------------------------------------------------------------------
void free_mem_buffer( ByteIOContext* stBuf )
{
	if( stBuf->buffer )
		av_free( stBuf->buffer );

	stBuf->buffer = NULL;
}
// ---------------------------------------------------------------------------------
int fill_mem_buffer( ByteIOContext* stBuf, unsigned char* s, int iSize )
{
	int i;
	void *bufTmp;
	if( !stBuf->buffer )
	{
		stBuf->buffer= (UINT8 *)av_malloc( iSize );
		if( !stBuf->buffer )
		{
			PyErr_Format(g_cErr, "Cannot allocate %d bytes of memory. Exiting...", iSize );
			return 0;
		}
		stBuf->buf_ptr= stBuf->buffer;
		stBuf->buf_end= stBuf->buffer+ iSize;
		stBuf->pos= 0;
		memcpy( stBuf->buf_ptr, s, iSize );
	}
	else
	{
		i= stBuf->buf_end- stBuf->buf_ptr;
		stBuf->pos+= stBuf->buf_ptr- stBuf->buffer;
		if( stBuf->buf_ptr- stBuf->buffer < iSize )
		{
			bufTmp= stBuf->buffer;
			stBuf->buffer= (UINT8 *)av_malloc( i+ iSize );
			if( !stBuf->buffer )
			{
				PyErr_Format(g_cErr, "Cannot allocate %d bytes of memory. Exiting...", iSize );
				return 0;
			}
			memcpy( stBuf->buffer, stBuf->buf_ptr, i );
			memcpy( stBuf->buffer+ i, s, iSize );
			stBuf->buf_end= stBuf->buffer+ i+ iSize;
			av_free( bufTmp );
		}
		else
		{
			memmove( stBuf->buffer, stBuf->buf_ptr, i );
			memcpy( stBuf->buffer+ i, s, iSize );
			stBuf->buf_end= stBuf->buffer+ i+ iSize;
		}
		stBuf->buf_ptr= stBuf->buffer;
	}
	return 1;
}

// ---------------------------------------------------------------------------------
bool SetAttribute( PyObject* cDict, char* sKey, int iVal )
{
	PyObject* cVal= PyInt_FromLong( iVal );
	if( !cVal )
		return false;
	PyDict_SetItemString( cDict, sKey, cVal );
	Py_DECREF( cVal );
	return true;
}

// ---------------------------------------------------------------------------------
bool SetAttribute( PyObject* cDict, char* sKey, char* sVal )
{
	PyObject* cVal= PyString_FromString( sVal );
	if( !cVal )
		return false;
	PyDict_SetItemString( cDict, sKey, cVal );
	Py_DECREF( cVal );
	return true;
}

// ---------------------------------------------------------------------------------
bool SetAttribute( PyObject* cDict, char* sKey, PyObject* cVal )
{
	PyDict_SetItemString( cDict, sKey, cVal );
	Py_DECREF( cVal );
	return true;
}

// ---------------------------------------------------------------------------------
extern "C"
{

// ---------------------------------------------------------------------------------
PyObject* GetStreams( PyDemuxerObject* obj )
{
	// Freeup previous formats data
	PyObject* cFormats= PyTuple_New( obj->ic.nb_streams );
	if( !cFormats )
		return NULL;

	for( int i= 0; i< obj->ic.nb_streams; i++ )
	{
		PyObject* cFormat= Py_None;
		AVCodecContext *cCodec= &obj->ic.streams[ i ]->codec;
 //printf( "Duration: %I64d\n",obj->ic.streams[ i ]->duration );

		if( cCodec->codec_id )
		{
			cFormat= PyDict_New();
			if( !cFormat )
				return NULL;

			SetAttribute( cFormat, PM_ID, cCodec->codec_id );
//			SetAttribute( cFormat, FOURCC, cCodec->fourcc );
			SetAttribute( cFormat, PM_TYPE, cCodec->codec_type );
			SetAttribute( cFormat, PM_BITRATE, cCodec->bit_rate );
			SetAttribute( cFormat, PM_WIDTH, cCodec->width );
			SetAttribute( cFormat, PM_HEIGHT, cCodec->height );
			SetAttribute( cFormat, PM_FRAME_RATE, cCodec->frame_rate );
			SetAttribute( cFormat, PM_FRAME_RATE_B, cCodec->frame_rate_base );
			SetAttribute( cFormat, PM_SAMPLE_RATE, cCodec->sample_rate );
			SetAttribute( cFormat, PM_CHANNELS, cCodec->channels );
			SetAttribute( cFormat, PM_DURATION, (int)( obj->ic.streams[ i ]->duration / AV_TIME_BASE ) );
			SetAttribute( cFormat, PM_INDEX, i );
		}

		PyTuple_SetItem( cFormats, i, cFormat );
	}
	return cFormats;
}

// ---------------------------------------------------------------------------------
void StartStreams( PyDemuxerObject* obj )
{
	if( obj->cBuffer )
		Py_DECREF( obj->cBuffer );

	obj->cBuffer= PyList_New( 0 );
}

// ---------------------------------------------------------------------------------
bool AppendStreamData( PyDemuxerObject* obj, AVPacket* cPkt )
{
	// Just a sanitary check
	if( cPkt->stream_index>= obj->ic.nb_streams || cPkt->stream_index< 0 )
		return true;

	{
		if( !obj->cBuffer )
			obj->cBuffer= PyList_New( 0 );
		PyObject* cRes= Py_BuildValue( "[is#iL]", cPkt->stream_index, (const char*)cPkt->data, cPkt->size,cPkt->size, cPkt->pts );
		PyList_Append( obj->cBuffer, cRes );
		Py_DECREF( cRes );
	}
	return true;
}

// ---------------------------------------------------------------------------------
static PyObject *
Demuxer_HasHeader( PyDemuxerObject* obj)
{
	return PyLong_FromLong( obj->bHasHeader );
}

// ---------------------------------------------------------------------------------
static PyObject *
Demuxer_Reset( PyDemuxerObject* obj)
{
	free_mem_buffer( &obj->ic.pb );
	Py_INCREF( Py_None );
	return Py_None;
}


// ---------------------------------------------------------------------------------
PyObject *
Demuxer_Parse( PyDemuxerObject* obj, PyObject *args)
{
	unsigned char* sData;
	int iLen, iRet= 0, i= 0;
	if (!PyArg_ParseTuple(args, "s#:parse", &sData, &iLen ))
		return NULL;

	// Get the header data first
	// Sync the buffers first
	i= obj->ic.nb_streams;
	fill_mem_buffer( &obj->ic.pb, sData, iLen );
	if( !obj->bTriedHeader )
	{
		AVFormatParameters params;
		memset( &params, 0, sizeof( params ) );
		iRet= obj->ic.iformat->read_header( &obj->ic, &params );
		if( iRet>= 0 )
		{
			// Set the flag that we've tried to read the header
			obj->bTriedHeader= true;
			if( !( obj->ic.iformat->flags & AVFMT_NOHEADER ) )
				obj->bHasHeader= true;
		}
	}

	// Create new list with possible formats
	StartStreams( obj );
	while( iRet>= 0 )
	{
		obj->pkt.size= 0;

		// more correct than obj->ic.iformat->read_packet( &obj->ic, &obj->pkt );
		iRet= av_read_packet(&obj->ic,&obj->pkt);
		// Parse single packet until all parsed
		if( iRet>= 0 )
		{
			if( obj->pkt.size> 0 )
				if( !AppendStreamData( obj, &obj->pkt ) )
				{
					PyErr_Format(g_cErr, "Cannot allocate memory ( %d bytes ) for codec parameters", obj->pkt.size );
					return NULL;
				}
		}
	}
	// In case of error or something
	if( iRet<= AVILIB_ERROR )
	{
		// Need to report out the error( it should be in a error list
		while( g_AvilibErr[ i ].iErrCode )
			if( g_AvilibErr[ i ].iErrCode== iRet )
			{
				PyErr_SetString(g_cErr, g_AvilibErr[ i ].sErrDesc );
				return NULL;
			}
			else
				i++;

		PyErr_Format(g_cErr, "Unspecified error %d. Cannot find any help text for it.", iRet );
		return NULL;
	}

	// Return the result
	Py_INCREF( obj->cBuffer );
	return obj->cBuffer;
}

// ---------------------------------------------------------------------------------
// List of all methods for the demuxer
static PyMethodDef demuxer_methods[] =
{
	{
		PARSE,
		(PyCFunction)Demuxer_Parse,
		METH_VARARGS,
		PARSE_DOC
	},
	{
		RESET,
		(PyCFunction)Demuxer_Reset,
		METH_NOARGS,
		RESET_DOC
	},
	{
		HAS_HEADER,
		(PyCFunction)Demuxer_HasHeader,
		METH_NOARGS,
		HAS_HEADER_DOC
	},
	{ NULL, NULL },
};

// ----------------------------------------------------------------
static void
DemuxerClose( PyDemuxerObject *obj )
{
	// Close avicodec first !!!
	if( obj->cBuffer )
		Py_DECREF( obj->cBuffer );

	av_close_input_file( &obj->ic );
	free_mem_buffer( &obj->ic.pb );

	PyObject_Free( (PyObject*)obj );
}

// ---------------------------------------------------------------------------------
PyObject *
DemuxerNew( PyTypeObject *type, PyObject *args, PyObject *kwds )
{
	char* s;
	AVInputFormat *fmt= NULL;
	if (!PyArg_ParseTuple(args, "s:", &s ))
		return NULL;

	// Have extension and match the codec first
	for( fmt= first_iformat; fmt != NULL; fmt = fmt->next)
		if ( (fmt->extensions) && (strstr( fmt->extensions, s )))
		{
			// Create new decoder
			PyDemuxerObject* demuxer= (PyDemuxerObject*)type->tp_alloc(type, 0);
			if( !demuxer )
				return NULL;

			// cleanup decoder data
			memset( &demuxer->ic, 0, sizeof(demuxer->ic));
			memset( &demuxer->pkt, 0, sizeof(demuxer->pkt));
			demuxer->cBuffer= NULL;
			demuxer->bHasHeader= demuxer->bTriedHeader= 0;
			demuxer->ic.iformat = fmt;

			// allocate private data
			if( fmt->priv_data_size )
			{
				demuxer->ic.priv_data = av_mallocz(fmt->priv_data_size);
				if (!demuxer->ic.priv_data)
				{
					PyErr_Format(g_cErr, "Cannot allocate memory ( %d bytes ) for codec parameters", fmt->priv_data_size );
					return NULL;
				}
			}
			else
				demuxer->ic.priv_data= NULL;

			return (PyObject*)demuxer;
		}


	PyErr_Format(g_cErr, "No registered demuxer for the '%s' extension", s );
	return NULL;
}

// ----------------------------------------------------------------
static PyGetSetDef demuxer_getsetlist[] =
{
	{STREAMS, (getter)GetStreams, NULL, STREAMS_DOC },
	{0},
};

// ----------------------------------------------------------------
PyTypeObject DemuxerType =
{
	PyObject_HEAD_INIT(NULL)
	0,
	"pymedia.video.muxer.Demuxer",
	sizeof(PyDemuxerObject),
	0,
	(destructor)DemuxerClose,  //tp_dealloc
	0,			  //tp_print
	0,		//tp_getattr
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
	0,					/* tp_setattro */
	0,					/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /* tp_flags */
	(char*)DEMUXER_DOC,		/* tp_doc */
	0,					/* tp_traverse */
	0,					/* tp_clear */
	0,					/* tp_richcompare */
	0,					/* tp_weaklistoffset */
	0,					/* tp_iter */
	0,					/* tp_iternext */
	demuxer_methods,				/* tp_methods */
	0,					/* tp_members */
	demuxer_getsetlist,					/* tp_getset */
	0,					/* tp_base */
	0,					/* tp_dict */
	0,					/* tp_descr_get */
	0,					/* tp_descr_set */
	0,					/* tp_dictoffset */
	0,					/* tp_init */
	PyType_GenericAlloc,			/* tp_alloc */
	DemuxerNew,	/* tp_new */
	PyObject_Del,				/* tp_free */
};

// ---------------------------------------------------------------------------------
// List of all methods for the mp3decoder
static PyMethodDef pymuxer_methods[] =
{
	{ NULL, NULL },
};

// ---------------------------------------------------------------------------------
#define INT_C(name) PyModule_AddIntConstant( m, #name, name )

// ---------------------------------------------------------------------------------
DL_EXPORT(void)
initmuxer(void)
{

	PyObject *m, *cExtensions;
	AVInputFormat *fmt;

	Py_Initialize();
	m= Py_InitModule("muxer", pymuxer_methods);

	// Formats
	avidec_init();
	mov_init();
	asf_init();
	mpegts_init();
	mpegps_init();

	cExtensions = PyList_New(0);
  for(fmt = first_iformat; fmt != NULL; fmt = fmt->next)
	{
		/* Split string by commas */
		PyObject *cStr;

		char *s1= NULL, *s= (char*)fmt->extensions;
		while( s && ( ( s1= strchr( s, ',' ) )!= NULL || s) )
		{
			if( s1 )
			{
				cStr= PyString_FromStringAndSize( s, s1- s );
				s= s1+ 1;
			}
			else
			{
				cStr= PyString_FromString( s );
				s= NULL;
			}

			s1= NULL;
			PyList_Append( cExtensions, cStr );
			Py_DECREF( cStr );
		}
	}

	PyModule_AddStringConstant( m, "__doc__", (char*)PYDOC );
	PyModule_AddStringConstant( m, "version", (char*)PYVERSION );
	PyModule_AddIntConstant( m, "build", PYBUILD );
	INT_C(CODEC_TYPE_AUDIO);
	INT_C(CODEC_TYPE_VIDEO);
	PyModule_AddObject( m, "extensions", cExtensions );

	g_cErr = PyErr_NewException("pymedia.video.muxer.MuxerError", NULL, NULL);
	if( g_cErr )
		PyModule_AddObject( m, "MuxerError", g_cErr );

	DemuxerType.ob_type = &PyType_Type;
	Py_INCREF((PyObject *)&DemuxerType);
	PyModule_AddObject(m, "Demuxer", (PyObject *)&DemuxerType);

	MuxerType.ob_type = &PyType_Type;
	Py_INCREF((PyObject *)&MuxerType);
	PyModule_AddObject(m, "Muxer", (PyObject *)&MuxerType);
}
};

/*

import pymedia.video.muxer as muxer
dm= muxer.Demuxer( 'avi' )
f= open( 'c:\\movies\\Lost.In.Translation\\Lost.In.Translation.CD2.avi', 'rb' )
#f= open( 'c:\movies\Our video.mpg', 'rb' )
s= f.read( 300000 )
r= dm.parse( s )
dm.streams


*/
