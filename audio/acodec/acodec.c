/*
 *			Extra light decompression library for audio files
 *      The list of all possible extensions supported may be obtained
 *			through the 'extensions' call.
 *			The easiest way to add codec is to use one from the libavcodec project...
 *
 *
 *		Copyright (C) 2002-2004  Dmitry Borisov, Fedor Druzhinin
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
 
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>

#ifndef BUILD_NUM
#define BUILD_NUM 1
#endif


#define MODULE_NAME "pymedia.audio.acodec"

const int PYBUILD= BUILD_NUM;
char* PYVERSION= "2";
char* PYDOC=
"Audio decoding module:\n"
"\t- Decode strings of sound information into raw PCM format\n"
"\t- Encodes raw PCM data into any available format( see acodec.formats )\n"
"\t- Parse and find additional information available in a header or special zones( tags ) in a compressed file\n"
"\t- Provides all formats with the same interface\n";

#define FRAMES "frames"
#define AUTHOR "artist"
#define TITLE "title"
#define YEAR "year"
#define ALBUM "album"
#define TRACK "track"
#define SAMPLE_RATE "sample_rate"
#define BITRATE "bitrate"
#define CHANNELS "channels"
#define SAMPLE_LENGTH "sample_length"
#define TYPE "type"
#define ID "id"
#define OLD_SAMPLE_RATE "old_sample_rate"
#define OLD_CHANNELS "old_channels"
#define DATA "data"

#define DECODE "decode"
#define ENCODE "encode"
#define HAS_HEADER "hasHeader"
#define GET_HEADER "getHeader"
#define GET_INFO "getInfo"
#define GET_CODEC_ID "getCodecId"
#define GET_PARAMS "getParams"

#define CONVERT_DOC \
	DECODE"( fragment ) -> audio_frame\n \
	Convert audio compressed data into pcm fragment. \n\
	Returns list of audio frame. Frame has the following members available:\n" \
	"\t"SAMPLE_RATE"\n" \
	"\t"BITRATE"\n" \
	"\t"CHANNELS"\n" \
	"\t"DATA"\n"

#define HAS_HEADER_DOC \
	HAS_HEADER"() -> { true | false }\n \
	Returns whether header presented or not\n"

#define GET_HEADER_DOC \
	GET_HEADER"() -> header_str\n Returns stream header if any\n"

#define GET_INFO_DOC \
	  GET_INFO"() -> info\n\
Return all information known by the codec as a dictionary. Predefined dictionary entries are:\n \
	frames - returns number of frames in a file\n \
	title - title for the song if exists\n \
	artist - author for the song if exists\n \
	album - song album if exists\n \
	track - track number if exists\n \
	year - year of album\n"

#define DECODER_DOC \
		"Decoder is a class which can decode compressed audio stream into raw uncomressed format( PCM )\n" \
		"suitable for playing through the sound module.\n" \
		"The following methods available:\n" \
		"\t"CONVERT_DOC \
		"\t"HAS_HEADER_DOC \
		"\t"GET_INFO_DOC

#define ENCODER_DOC \
	  "Encoder( codecParams ) -> Codec\n Decoder(default) or encoder that will not use any demuxer to work with the stream\n"\
	  "it will assume that the whole string coming for decode() will contain one frame only"\
	  "The following methods available once you create Codec instance:\n" \
	  "\t"ENCODE_DOC

#define GET_PARAM_DOC \
	  GET_PARAMS"() -> params\n\
	  Parameters that represents the current state of codec\n"

#define ENCODE_DOC \
	ENCODE"( samples ) -> ( frames ) list of encoded string\n\
	  Encodes audio frame(s). It accepts audio samples\n\
		as string, buffers them, and returns list of encoded frames. Note, that its\n\
		behaviour is different from vcodec - vcodec returns only one frame."

#define GET_CODEC_ID_DOC \
	GET_CODEC_ID"(name) - returns internal numerical codec id for its name"

#define FRAME_DOC \
	"frame is an object that stores data about audio frame in PCM format. Possible members are:\n" \
	"\t"SAMPLE_RATE"\n"\
	"\t"BITRATE"\n"\
	"\t"CHANNELS"\n"\
	"\t"DATA"\n"

#define RETURN_NONE return (Py_INCREF(Py_None), Py_None); 

PyObject *g_cErr;

#define ENCODE_OUTBUF_SIZE  40000
// ---------------------------------------------------------------------------------
typedef struct
{
	PyObject_HEAD
	AVFormatContext ic;
	AVCodecContext *cCodec;
	AVCodec *codec;
	AVPacket pkt;
	UINT8 * pBuf;
	int iBufLen;
	PyObject *cDict;
	int iTriedHeader;
	int iAcodecFlags;
	ByteIOContext stInBuf;
} PyACodecObject;

// ---------------------------------------------------------------------------------
typedef struct
{
	PyObject_HEAD

	PyObject* cData;
	int bit_rate;
	int sample_rate;
	int bits_per_sample;
	int channels;
} PyAFrameObject;

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
	{ AVILIB_SMALL_BUFFER, "Output buffer too small. You may need to increase MAX_OUTFRAME_SIZE." },
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
		memcpy( stBuf->buf_ptr, s, iSize );
	}
	else
	{
		i= stBuf->buf_end- stBuf->buf_ptr;
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
int SetStructVal( int* pVal, PyObject* cObj, char* sKey )
{
	PyObject* obj =PyDict_GetItemString(cObj,sKey);
	if (obj && PyInt_Check(obj))
		*pVal= PyInt_AsLong(obj);
	else
	  return 0;
	return 1;
}

// ---------------------------------------------------------------------------------
int SetAttribute( PyObject* cDict, char* sKey, int iVal )
{
	PyObject* cVal= PyInt_FromLong( iVal );
	if( !cVal )
		return 0;
	PyDict_SetItemString( cDict, sKey, cVal );
	Py_DECREF( cVal );
	return 1;
}

// ---------------------------------------------------------------------------------
int SetCodecParams( PyACodecObject* obj, PyObject* cObj )
{
	if( !SetStructVal( &obj->cCodec->bit_rate, cObj, BITRATE ))
		return 0;
	if( !SetStructVal( &obj->cCodec->channels, cObj, CHANNELS ))
	  return 0;
	if( !SetStructVal( &obj->cCodec->sample_rate, cObj, SAMPLE_RATE ))
	  return 0;
	if( !SetStructVal( (int*)&obj->cCodec->codec_id, cObj, ID ))
	  return 0;

	return 1;
}

// ---------------------------------------------------------------------------------
static PyObject * Codec_GetParams( PyACodecObject* obj, PyObject *args)
{
	PyObject* cRes= PyDict_New();
	if( !cRes )
		return NULL;
	SetAttribute( cRes, BITRATE, obj->cCodec->bit_rate );
	SetAttribute( cRes, CHANNELS, obj->cCodec->channels );
	SetAttribute( cRes, SAMPLE_RATE, obj->cCodec->sample_rate );
	SetAttribute( cRes, TYPE,obj->cCodec->codec_type);
	SetAttribute( cRes, ID,obj->cCodec->codec_id);
	return cRes;
}

// ---------------------------------------------------------------------------------
static PyObject * Codec_GetID( PyACodecObject* obj, PyObject *args)
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
		RETURN_NONE
}
// ---------------------------------------------------------------------------------
static PyObject *
Decoder_HasHeader( PyACodecObject* obj)
{
	return PyLong_FromLong( obj->ic.has_header );
}

// ---------------------------------------------------------------------------------
static PyObject *
Decoder_GetInfo( PyACodecObject* obj)
{
	/* Populate dictionary with the data if header already parsed */
	if( !obj->cDict && obj->iTriedHeader )
	{
		PyObject* cStr;
		obj->cDict= PyDict_New();
		if( !obj->cDict )
			return NULL;

		cStr= PyLong_FromLong( obj->cCodec->frame_number );
		PyDict_SetItemString( obj->cDict, FRAMES, cStr );
		Py_DECREF( cStr );
		cStr= PyString_FromString( obj->ic.author );
		PyDict_SetItemString( obj->cDict, AUTHOR, cStr );
		Py_DECREF( cStr );
		cStr= PyString_FromString( obj->ic.title );
		PyDict_SetItemString( obj->cDict, TITLE, cStr );
		Py_DECREF( cStr );
		cStr= PyString_FromString( obj->ic.year );
		PyDict_SetItemString( obj->cDict, YEAR, cStr );
		Py_DECREF( cStr );
		cStr= PyString_FromString( obj->ic.album );
		PyDict_SetItemString( obj->cDict, ALBUM, cStr );
		Py_DECREF( cStr );
		cStr= PyString_FromString( obj->ic.track );
		PyDict_SetItemString( obj->cDict, TRACK, cStr );
		Py_DECREF( cStr );
	}

	if( obj->cDict )
	{
		/* Return builtin dictionary */
		Py_INCREF( obj->cDict );
		return obj->cDict;
	}

	PyErr_SetString( g_cErr, "The header has not been read yet. Cannot get stream information." );
	return NULL;
}

// ----------------------------------------------------------------
static PyObject *
frame_get_data(PyAFrameObject *obj)
{
  Py_INCREF( obj->cData );
	return obj->cData;
}

// ----------------------------------------------------------------
static PyMemberDef frame_members[] = 
{
	{SAMPLE_RATE,	T_INT, offsetof(PyAFrameObject,sample_rate), 0, "frame sample rate."},
	{BITRATE,	T_INT, offsetof(PyAFrameObject,bit_rate), 0, "frame bitrate."},
	{SAMPLE_LENGTH,	T_INT, offsetof(PyAFrameObject,bits_per_sample), 0, "frame bits per sample."},
	{CHANNELS,	T_INT, offsetof(PyAFrameObject,channels), 0, "number of channels."},
	{NULL}	/* Sentinel */
};
 
// ----------------------------------------------------------------
static PyGetSetDef frame_getsetlist[] =
{
	{DATA, (getter)frame_get_data, NULL, "frame data as a string."},
	{0},
};

// ----------------------------------------------------------------
static void AFrameClose( PyAFrameObject *obj )
{
	// Close avcodec first !!!
	if( obj->cData )
	{
		Py_DECREF( obj->cData );
	}

	PyObject_Free( (PyObject*)obj );
}


// ----------------------------------------------------------------
PyTypeObject FrameType =
{
	PyObject_HEAD_INIT(NULL)
	0,
	MODULE_NAME".Frame",
	sizeof(PyAFrameObject),
	0,
	(destructor)AFrameClose,  //tp_dealloc
	0,			  //tp_print
	0, //tp_getattr
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
	(char*)FRAME_DOC,		/* tp_doc */
	0,					/* tp_traverse */
	0,					/* tp_clear */
	0,					/* tp_richcompare */
	0,					/* tp_weaklistoffset */
	0,					/* tp_iter */
	0,					/* tp_iternext */
	0,					/* tp_methods */
	frame_members,					/* tp_members */
	frame_getsetlist,					/* tp_getset */
	0,					/* tp_base */
	0,					/* tp_dict */
	0,					/* tp_descr_get */
	0,					/* tp_descr_set */
	0,					/* tp_dictoffset */
	0,					/* tp_init */
	PyType_GenericAlloc,			/* tp_alloc */
	0,				/* tp_new */
	PyObject_Del,				/* tp_free */
};


// ---------------------------------------------------------------------------------
static PyObject *
Decoder_Convert2PCM( PyACodecObject* obj, PyObject *args)
{
	unsigned char* sData;
	int iLen, iRet= 0, out_size, size, len, iLeft, i= 0;
	UINT8 *inbuf_ptr;
	if (!PyArg_ParseTuple(args, "s#:decode", &sData, &iLen ))
		return NULL;

	// Get the header data first
	// Sync the buffers first
	if( obj->ic.iformat )
		if( !fill_mem_buffer( &obj->ic.pb, sData, iLen ) )
			return NULL;

	if( !obj->iTriedHeader )
	{
		AVFormatParameters params;
		memset( &params, 0, sizeof( params ) );
		iRet= obj->ic.iformat->read_header( &obj->ic, &params );
		if( !iRet )
		{
			// Find codec
			obj->cCodec= &obj->ic.streams[ 0 ]->codec;
			obj->codec = avcodec_find_decoder(obj->cCodec->codec_id);
			if (!obj->codec)
			{
				PyErr_Format(g_cErr, "Codec with id %d cannot be found. libavformat is out of sync with libavcodec.", obj->cCodec->codec_id );
				return NULL;
			}
			if (avcodec_open( obj->cCodec, obj->codec) < 0)
			{
				PyErr_Format(g_cErr, "Could not open codec with id %d. Possibly codec is not setup correctly.", obj->cCodec->codec_id );
				return NULL;
			}
			// Set the flag that we've tried to read the header
			obj->iTriedHeader= 1;
		}
	}
	iLeft= obj->iBufLen;
	while( iRet>= 0 )
	{
		if( obj->ic.iformat )
			iRet= obj->ic.iformat->read_packet( &obj->ic, &obj->pkt );
		else
		{
			obj->pkt.data= sData;
			obj->pkt.size= iLen;
		}

		// Parse single packet until all parsed
		if( iRet>= 0 )
		{
			inbuf_ptr = obj->pkt.data;
			size= obj->pkt.size;
			while( size> 0 )
			{
				if( iLeft< AVCODEC_MAX_AUDIO_FRAME_SIZE )
				{
					// Realloc memory
					int iTmp= AVCODEC_MAX_AUDIO_FRAME_SIZE* 4;
					obj->iBufLen+= iTmp;
					obj->pBuf= (UINT8 *)realloc( obj->pBuf, obj->iBufLen );
					if( !obj->pBuf )
					{
						PyErr_SetString(g_cErr, "No memory for the conversion buffer" );
						return NULL;
					}
					iLeft+= iTmp;
				}
				// Decode frame
				out_size= 0;
				len= obj->codec->decode( obj->cCodec,
						obj->pBuf + obj->iBufLen - iLeft,
						&out_size,
						inbuf_ptr,
						size );
				if( len > 0 )
				{
					size-= len;
					inbuf_ptr+= len;
				}
				// Hmm. If we not doing break here it could be endless loop,
				// from the other hand we may lose some data...
				//else
				//	break;

				if (out_size > 0)
					iLeft-= out_size;
			}
		}
		// Exit loop when only one packet in the input
		if( !obj->ic.iformat )
			break;
	}

	if( !obj->ic.iformat )
		obj->pkt.data= NULL;

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
	{
		PyObject* cRes;
		PyAFrameObject *cFrame= (PyAFrameObject*)PyObject_New( PyAFrameObject, &FrameType );
		if( !cFrame )
			return NULL;

		cRes= (PyObject*)PyString_FromStringAndSize( (char*)obj->pBuf, obj->iBufLen- iLeft );
		if( !cRes )
			return NULL;

		cFrame->bit_rate= obj->cCodec->bit_rate;
		cFrame->sample_rate=	obj->cCodec->sample_rate;
		cFrame->bits_per_sample= obj->cCodec->bits_per_sample;
		cFrame->channels= obj->cCodec->channels;
		cFrame->cData= cRes;
		return (PyObject*)cFrame;
	}
}

// ---------------------------------------------------------------------------------
// List of all methods for the wmadecoder
static PyMethodDef decoder_methods[] =
{
	{
		DECODE,
		(PyCFunction)Decoder_Convert2PCM,
		METH_VARARGS,
		CONVERT_DOC
	},
	{
		HAS_HEADER,
		(PyCFunction)Decoder_HasHeader,
		METH_NOARGS,
		HAS_HEADER_DOC
	},
	{
		GET_INFO,
		(PyCFunction)Decoder_GetInfo,
		METH_NOARGS,
		GET_INFO_DOC
	},
	{
		GET_PARAMS,
		(PyCFunction)Codec_GetParams,
		METH_NOARGS,
		GET_PARAM_DOC
	},
	{ NULL, NULL },
};

// ---------------------------------------------------------------------------------
static int GetFrameSize(PyACodecObject* obj ) 
{

	int frame_size =0;

	/* ugly hack for PCM codecs (will be removed ASAP with new PCM
	 *        support to compute the input frame size in samples */
	if (obj->cCodec->frame_size <= 1) 
	{
		frame_size = ENCODE_OUTBUF_SIZE / obj->cCodec->channels;
		switch(obj->cCodec->codec_id) 
		{
			case CODEC_ID_PCM_S16LE:
			case CODEC_ID_PCM_S16BE:
			case CODEC_ID_PCM_U16LE:
			case CODEC_ID_PCM_U16BE:
				frame_size >>= 1;
				break;
			default:
				break;
		}
	} 
	else 
	{
		frame_size = obj->cCodec->frame_size;

	}
	return frame_size;
}

// ---------------------------------------------------------------------------------

static PyObject* ACodec_GetFrameSize( PyACodecObject* obj)
{
	return PyInt_FromLong( GetFrameSize(obj));
}

// ---------------------------------------------------------------------------------
static PyObject* ACodec_Encode( PyACodecObject* obj, PyObject *args)
{
	PyObject* cRes = NULL;
	uint8_t* sData = NULL;
	int iLen = 0;
	PyObject* cFrame = NULL;
	char sOutbuf[ ENCODE_OUTBUF_SIZE ];

	if (!PyArg_ParseTuple(args, "s#:encode", &sData,&iLen))
		return NULL;

	if (!(obj->cCodec || obj->cCodec->codec)) 
	{
		PyErr_SetString(g_cErr, "Encode error:codec not initialized" );
		return NULL;
	}
	if( !fill_mem_buffer( &obj->stInBuf, sData, iLen ) ) 
		return NULL;

	cRes = PyList_New(0);
	if (!cRes) 
		return NULL;

/*	printf ("frame size:%d, in buffer:%d\n",
			GetFrameSize(obj)*2*obj->cCodec->channels,
			obj->stInBuf.buf_end-obj->stInBuf.buf_ptr);
*/
	while (obj->stInBuf.buf_end-obj->stInBuf.buf_ptr >=
			GetFrameSize(obj)*2*obj->cCodec->channels)
	{
		iLen = avcodec_encode_audio(obj->cCodec,
				sOutbuf,
				ENCODE_OUTBUF_SIZE,
				(short*)obj->stInBuf.buf_ptr);

		obj->stInBuf.buf_ptr+=GetFrameSize(obj)*2*obj->cCodec->channels;
		if (iLen >ENCODE_OUTBUF_SIZE) 
		{
			fprintf (stderr, "Codec output size greated than "
			"%d bytes! Increase ENCODE_OUTBUF_SIZE in acodec/acodec.c"
			"and recompile the pymedia library\n",
			ENCODE_OUTBUF_SIZE);

			PyErr_SetString(g_cErr, "Encode error: internal buffer too small!");
			return NULL;
		}
		if (iLen > 0) 
		{
			cFrame = PyString_FromStringAndSize((const char*)sOutbuf, iLen );
			PyList_Append(cRes,cFrame);
			Py_DECREF( cFrame );
		}
	}

	return cRes;
}

// ---------------------------------------------------------------------------------
// List of all methods for the wmadecoder
static PyMethodDef encoder_methods[] =
{
	{
		ENCODE,
		(PyCFunction)ACodec_Encode,
		METH_VARARGS,
		ENCODE_DOC
	},
	{
		GET_PARAMS,
		(PyCFunction)Codec_GetParams,
		METH_NOARGS,
		GET_PARAM_DOC
	},
	{ NULL, NULL },
};

// ----------------------------------------------------------------
PyTypeObject EncoderType;
PyTypeObject DecoderType;

// ----------------------------------------------------------------
static void ACodecClose( PyACodecObject *acodec )
{
	// Close avcodec first !!!
	if( acodec->cCodec )
		avcodec_close(acodec->cCodec);
	av_close_input_file( &acodec->ic );

	free_mem_buffer( &acodec->ic.pb );
	if( acodec->pkt.data )
		av_free( acodec->pkt.data );

	if( acodec->pBuf )
		free( acodec->pBuf );

	if( acodec->cDict )
	{
		Py_DECREF( acodec->cDict );
	}
	//if (acodec->resample_ctx)
	//	audio_resample_close(acodec->resample_ctx);
	free_mem_buffer(&acodec->stInBuf);
	PyObject_Free( (PyObject*)acodec );
}

// ---------------------------------------------------------------------------------
static PyObject* CreateNewPyACodec(AVInputFormat *fmt, int bDecoder)
{
	// Create new decoder
	PyACodecObject* acodec= PyObject_New( PyACodecObject, ( bDecoder ) ? &DecoderType: &EncoderType );
	if( !acodec )
		return NULL;

	// cleanup decoder data
	memset( &acodec->ic, 0, sizeof(acodec->ic));
	memset( &acodec->pkt, 0, sizeof(acodec->pkt));
	acodec->cCodec= NULL;
	//acodec->codec= NULL;
	acodec->ic.iformat = fmt;
	acodec->pBuf= 0;
	acodec->iTriedHeader= 0;
	acodec->iBufLen= 0;
	acodec->cDict= NULL;
	memset( &acodec->stInBuf,0,sizeof(ByteIOContext));

	// allocate private data
	if( fmt && fmt->priv_data_size )
	{
		acodec->ic.priv_data = av_mallocz(fmt->priv_data_size);
		if (!acodec->ic.priv_data)
		{
			PyErr_SetString(g_cErr, "Cannot allocate memory for codec parameters" );
			return NULL;
		}
	}
	return (PyObject*)acodec;
}

// ---------------------------------------------------------------------------------
static PyObject *
CodecByParamsNew( PyObject* cParams, int bDecoder )
{
	// Find appropriate codec by its parameters passed as dictionary
	AVCodec *p;
	int iId;
	PyObject* cId;
	if( !PyDict_Check( cParams ) )
	{
		PyErr_Format(g_cErr, "Codec(): First parameter should be dict (codec id and params)" );
		return NULL;
	}

	cId= PyDict_GetItemString( cParams, ID );
	if( !cId )
	{
		PyErr_Format(g_cErr, "Codec(): Cannot find codec id in a parameters dictionary" );
		return NULL;
	}

	iId= PyInt_AsLong( cId );
	if( bDecoder )
		p = avcodec_find_decoder((enum CodecID)iId);
	else
		p = avcodec_find_encoder((enum CodecID)iId);

	if( p )
	{
		// Create new acodec
		PyACodecObject* acodec= (PyACodecObject*)CreateNewPyACodec(NULL, bDecoder );
		// cleanup acodec data
		memset( &acodec->stInBuf,0,sizeof(ByteIOContext));

		acodec->cCodec= (AVCodecContext*)av_mallocz(sizeof(AVCodecContext));
		if( !acodec->cCodec )
		{
			PyErr_NoMemory();
			return NULL;
		}

		//acodec->cCodec->codec= p;
		acodec->codec= p;
		avcodec_get_context_defaults( acodec->cCodec );

		// Populate some values from the dictionary
		// Verify that all parameters are set
		if( !SetCodecParams( acodec, cParams ))
			return NULL;

		acodec->cCodec->codec_type=	CODEC_TYPE_AUDIO;

		if(p->capabilities & CODEC_CAP_TRUNCATED)
				acodec->cCodec->flags |= CODEC_FLAG_TRUNCATED;

		//printf ("acodec->cCodec->codec:%p\n",acodec->cCodec->codec);
		if (avcodec_open( acodec->cCodec, p )<0)
		{
			PyErr_Format(g_cErr, "Encoder(): error opening codec.\nSome initialization parameters may have wrong values.");
			av_free(acodec->cCodec);
			return NULL;
		}
		acodec->iTriedHeader= 1;
		return (PyObject*)acodec;
	}
	PyErr_Format(g_cErr, "No registered codec with id= %d", iId );
	return NULL;
}

// ---------------------------------------------------------------------------------
static PyObject *
DecoderNew( PyTypeObject *type, PyObject *args, PyObject *kwds )
{
	PyObject* cObj;
	AVInputFormat *fmt= NULL;
	if (!PyArg_ParseTuple(args, "O:Decoder", &cObj ))
		return NULL;

	// Have extension and match the codec first
	if( PyString_Check( cObj ) )
	{
		char* s= PyString_AsString( cObj );
		for( fmt= first_iformat; fmt != NULL; fmt = fmt->next)
			if ( strstr( fmt->extensions, s ))
				return CreateNewPyACodec( fmt, 1 );
		{
			char s1[ 255 ];
			sprintf( s1, "No registered codec for the '%s' extension", s );
			PyErr_SetString(g_cErr, s1 );
		}
		return NULL;
	}
	else 
		return CodecByParamsNew( cObj, 1 );
}

// ----------------------------------------------------------------
PyTypeObject DecoderType =
{
	PyObject_HEAD_INIT(NULL)
	0,
	MODULE_NAME".Decoder",
	sizeof(PyACodecObject),
	0,
	(destructor)ACodecClose,  //tp_dealloc
	0,			  //tp_print
	0, //tp_getattr
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
	(char*)DECODER_DOC,		/* tp_doc */
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
static PyObject *
EncoderNew( PyTypeObject *type, PyObject *args, PyObject *kwds )
{
	PyObject* cObj;
	if (!PyArg_ParseTuple(args, "O:Encoder", &cObj ))
		return NULL;

	return CodecByParamsNew( cObj, 0 );
}
// ----------------------------------------------------------------
PyTypeObject EncoderType =
{
	PyObject_HEAD_INIT(NULL)
	0,
	MODULE_NAME".Encoder",
	sizeof(PyACodecObject),
	0,
	(destructor)ACodecClose,  //tp_dealloc
	0,			  //tp_print
	0, //tp_getattr
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
	(char*)ENCODER_DOC,		/* tp_doc */
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
static PyMethodDef pympg_methods[] =
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
DL_EXPORT(void)
initacodec(void)
{
	PyObject *m, *cExtensions;
	AVInputFormat *fmt;

	Py_Initialize();
	m= Py_InitModule(MODULE_NAME, pympg_methods);

  /* register all the codecs (you can also register only the codec you wish to have smaller code */
  register_avcodec(&wmav1_decoder);
  register_avcodec(&wmav2_decoder);
  register_avcodec(&mp2_decoder);
  register_avcodec(&mp3_decoder);
  register_avcodec(&ac3_decoder);

  register_avcodec(&ac3_encoder);
  register_avcodec(&mp2_encoder);

#ifdef CONFIG_VORBIS
  register_avcodec(&oggvorbis_decoder);
  register_avcodec(&oggvorbis_encoder);
  ogg_init();
#endif
#ifdef CONFIG_FAAD
	//register_avcodec(&mpeg4aac_decoder);
	//register_avcodec(&aac_decoder);
#endif
#if CONFIG_MP3LAME
	register_avcodec(&mp3lame_encoder);
#endif

  asf_init();
  raw_init();

	PyModule_AddStringConstant(m, "__doc__", PYDOC );
	PyModule_AddStringConstant(m, "version", PYVERSION );
	PyModule_AddIntConstant(m, "build", PYBUILD );

	g_cErr = PyErr_NewException(MODULE_NAME".Error", NULL, NULL);
	if( g_cErr != NULL)
	  PyModule_AddObject(m, "error", g_cErr );

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
	PyModule_AddObject(m, "extensions", cExtensions );
	DecoderType.ob_type = &PyType_Type;
	Py_INCREF((PyObject *)&DecoderType);
	PyModule_AddObject(m, "Decoder", (PyObject *)&DecoderType);
	EncoderType.ob_type = &PyType_Type;
	Py_INCREF((PyObject *)&EncoderType);
	PyModule_AddObject(m, "Encoder", (PyObject *)&EncoderType);
}

/*
def test( name ):
	import acodec, sound, time
	dec= acodec.Decoder( str.split( name, '.' )[ -1 ] )
	f= open( name, 'rb' )
	snd= None
	while 1:
		s= f.read( 20000 )
		if len( s ):
			r= dec.decode( s )
			if snd== None:
				print 'Opening sound with %d channels' % r[ 3 ]
				snd= sound.output( r[ 1 ], r[ 3 ], 0x10 )
			while 1:
				try:
					snd.play( r[ 4 ] )
					break
				except:
					time.sleep( .1 )
		else:
			break


# test codec with frame decoder
import pymedia.audio.acodec as acodec
parms= {'index': 1, 'type': 1, 'frame_rate_base': 1, 'height': 0, 'channels': 2, 'width': 0, 'sample_rate': 48000, 'frame_rate': 25, 'bitrate': 192000, 'id': 9}
dec= acodec.Decoder( parms )

# test codec with format decoder
import pymedia.audio.acodec as acodec
dec= acodec.Decoder( 'mp3' )
f= open( 'c:\\music\\Roots.mp3', 'rb' )
s= f.read( 16384 )
r= dec.decode( s )
print dec.hasHeader()
print dec.getInfo()
print r.sample_rate


	# test codec with encoder
def encode():
	import pymedia.audio.acodec as acodec
	codec= 'mp2'
	parms= {'channels': 2, 'sample_rate': 44100, 'bitrate': 128000, 'id': acodec.getCodecID(codec)}
	enc= acodec.Encoder( parms )
	f= open( 'c:\\music\\Roots.pcm', 'rb' )
	f1= open( 'c:\\music\\test_enc.'+ codec, 'wb' )
	s= f.read( 300000 )
	while len( s ):
		frames= enc.encode( s )
		#for fr in frames:
		#	f1.write( fr )
		s= f.read( 300000 )
	f.close()
	f1.close()

encode()

def aplayer( name ):
	import pymedia.audio.acodec as acodec
	import pymedia.audio.sound as sound
	import time
	dec= acodec.Decoder( str.split( name, '.' )[ -1 ].lower() )
	f= open( name, 'rb' )
	snd= None
	s= f.read( 50000 )
	while len( s ):
		r= dec.decode( s )
		if snd== None:
			print 'Opening sound with %d channels' % r.channels
			snd= sound.Output( r.sample_rate, r.channels, sound.AFMT_S16_LE )
		snd.play( r.data )
		s= f.read( 512 )
	
	while snd.isPlaying():
	  time.sleep( .05 )

aplayer( 'c:\\music\\Test\\test.wma' )

*/
