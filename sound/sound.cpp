/*
 *			Python wrapper against sound stream handling
 *
 *					Copyright (C) 2002-2003  Dmitry Borisov
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

#if defined( WIN32 ) || defined( SYS_CYGWIN )
#include "audio_win.h"
#else
#include "audio_unix.h"
#endif

#include "resample.h"
#include "fft.h"

#ifndef BUILD_NUM
#define BUILD_NUM 1
#endif

#define MODULE_NAME "pymedia.audio.sound"

#define RETURN_NONE	Py_INCREF( Py_None ); return Py_None;

#define PYSNDBUILD BUILD_NUM
#define PYSNDVERSION "1"
#define PYMODULEDOC \
"Sound routines\nAllows to control sound device:\n\
\t- Place sound chunks into the sound queue\n\
\t- Get playback parameters( volume, position etc )\n\
\t- Control the playback( stop, pause, play )\n\
Here is the simple player for a pcm file\n\
( you may add your wav playback by parsing the header ):\n\
\timport pymedia.audio.sound as sound, time\n\
\tsnd1= sound.Output( 44100, 2, sound.AFMT_S16_LE )\n\
\tf= open( 'test.pcm', 'rb' )\n\
\ts= ' '\n\
\twhile len( s )> 0:\n\
\t\ts= f.read( 4000000 )\n\
\t\twhile 1:\n\
\t\t\tsnd1.play( s ) \n"

// RANGE: 32-576.  This is the # of samples of waveform data that you want.
//   Note that if it is less than 576, then VMS will do its best
//   to line up the waveforms from frame to frame for you, using
//   the extra samples as 'squish' space.
// Note: the more 'slush' samples you leave, the better the alignment
//   will be.  512 samples gives you ok alignment; 480 is better;
//   400 leaves room for fantastic alignment.
// Observe that if you specify a value here (say 400) and then only
//   render a sub-portion of that in some cases (say, 200 samples),
//   make sure you render the *middle* 200 samples (#100-300), because
//   the alignment happens *mostly at the center*.
#define NUM_WAVEFORM_SAMPLES         480

#define OUTPUT_NAME "Output"
#define INPUT_NAME "Input"
#define PLAY_NAME "play"
#define START_NAME "start"
#define CAN_PLAY_NAME "canPlay"
#define PAUSE_NAME "pause"
#define UNPAUSE_NAME "unpause"
#define STOP_NAME "stop"
#define POSITION_NAME "getPosition"
#define CHANNELS_NAME "getChannels"
#define ISMUTE_NAME "isMute"
#define SETMUTE_NAME "setMute"
#define GETVOLUME_NAME "getVolume"
#define GETRATE_NAME "getRate"
#define SETVOLUME_NAME "setVolume"
#define IS_PLAYING_NAME "isPlaying"
#define GET_LEFT_NAME "getLeft"
#define GET_SIZE_NAME "getSize"
#define GET_DATA_NAME "getData"
#define RESAMPLE_NAME "resample"
#define AS_FREQS_NAME "asFrequencies"
#define AS_BANDS_NAME "asBands"
#define GET_OUTPUT_DEVICES_NAME "getODevices"
#define GET_INPUT_DEVICES_NAME "getIDevices"

#define GET_OUTPUT_DEVICES_DOC GET_OUTPUT_DEVICES_NAME"()-> ( device_dict )\n"\
				"list of output devices presented as a dictionary.\nThe actual content may differ but these always available:\n"\
				"\tid - identifier of the device( >=0 ) can be provided when opening device for output( Output() )\n"\
				"\tname - device textual name as returned by OS\n"\
				"additional data may include OS/device specific information"

#define GET_INPUT_DEVICES_DOC GET_INPUT_DEVICES_NAME"()-> ( device_dict )\n"\
				"list of input devices presented as a dictionary.\nThe same as for output devices."

#define START_DOC START_NAME"() -> None\n\tStart grabbing sound data with parameters specified when opening the device\n"
#define I_STOP_DOC STOP_NAME"() -> None \n\tStops grabbing sound data\n"
#define PLAY_DOC PLAY_NAME"( fragment ) -> None\n\tPlay raw frangment using settings specified during output creation\n"
#define CAN_PLAY_DOC CAN_PLAY_NAME"() -> {0|1}\n\tWhether current sound can( ready ) play fragment or not\n"
#define PAUSE_DOC PAUSE_NAME"() -> None\n\tPauses currently playing fragment\n\tIf fragment is not playing, it has effect\n"
#define UNPAUSE_DOC UNPAUSE_NAME"() -> None \n\tUnpauses currently paused fragment\n\tIf fragment is not paused, it has no effect\n"
#define STOP_DOC STOP_NAME"() -> None \n\tStops currently played fragment\n"
#define POSITION_DOC POSITION_NAME"() -> int\n\tReturns position for the currently playing fragment.\n\tPosition is calculated from the last full stop. Pause does not considered as a stop\n"
#define CHANNELS_DOC CHANNELS_NAME"() -> {1..n}\n\tReturns number of channels that have been passed during the initialization\n"
#define ISMUTE_DOC ISMUTE_NAME"() -> ( { 1 | 0 } )\n\tWhether sound is mute\n"
#define SETMUTE_DOC ISMUTE_NAME"( { 1 | 0 } ) -> None\n\tSet or unset mute\n"
#define GETVOLUME_DOC GETVOLUME_NAME"() -> ( 0..0xffff )\n\tCurrent volume level\n"
#define GETRATE_DOC GETRATE_NAME"() -> rate\n\tCurrent frequency rate\n"
#define GET_SIZE_DOC GET_SIZE_NAME"() -> bufSize \n\tSize of the grabbing buffer. 0 if empty.\n"
#define SETVOLUME_DOC SETVOLUME_NAME"( volume ) -> None\n\tSet volume level\n"
#define GET_DATA_DOC GET_DATA_NAME"() -> data \n\treturns data from the sound device as string.\n"
#define IS_PLAYING_DOC IS_PLAYING_NAME"() -> {0|1}\n\tWhether sound is playing\n"
#define GET_LEFT_DOC GET_LEFT_NAME"() -> secs \n\tNumber of seconds left in a buffer to play\n"
#define RESAMPLE_DOC RESAMPLE_NAME"( data ) -> data\n\t\tstring with resampled audio samples\n"\
				"Resamples audio stream from->to certain format."
#define AS_FREQS_DOC AS_FREQS_NAME"( data ) -> [ ( freqs* ) ]\nList of frequency lists.\nFor every n( given as number of samples ) it returns \n"\
				"m frequencies. The result will contain list of lists of frequencies. \n"\
				"Number of lists will be len( data )/ n.\n"\
				"If there is no enough data to fullfill the n samples, it will be right padded with zeroes.\n"\
				"IMPORTANT: Please note that only mono signals are supported at this time."
#define AS_BANDS_DOC AS_BANDS_NAME"( bandsNum, data ) -> [ (( lowFreq, value )*) ]\nList of bands groupped by frequency.\n"\
				"All m frequencies will be splitted into bandNum bands accodring to number of octaves per band.\n" \
				"The result will be given as lists of tuples in which first two elements are frequency limits \n"\
				"to which value is computed.\n"\
				"It will be at least len( data ) / n elements in a list for every n samples in the input data.\n"\
				"If there is no enough data to fullfill the n samples, it will be right padded with zeroes.\n"\
				"IMPORTANT: Please note that only mono signals are supported at this time."

#define RESAMPLER_NAME "Resampler"

#define RESAMPLER_DOC \
	  "Resampler( (sample_rate_from, channels_from), (sample_rate_to, channels_to) ) -> Resampler\n" \
		"allows to convert audio streams between different sample rates and channels\n" \
	  "The following considerations are made for the mutltichannel audio streams: \n" \
		"\t2 channels -> (left,right)\n" \
		"\t3 channels -> (left,right,sub)\n" \
		"\t4 channels -> (left_front,center,right_front,sub)\n" \
		"\t5 channels -> (left_front,center,right_front,left_back,right_back)\n" \
		"\t6 channels -> (left_front,center,right_front,left_back,right_back,sub)\n" \
	  "The following methods available once you create Resampler instance:\n" \
	  "\t"RESAMPLE_NAME"()"

#define ANALYZER_NAME "SpectrAnalyzer"

#define ANALYZER_DOC \
	  ANALYZER_NAME"( samples_in, frequencies ) -> SpectAnalyzer\n" \
		"Allow to analyze audio stream for frequencies. It will convert mono audio samples \n" \
	  "into list of integers where every item represnts each frequency in 0..sample_rate/4 interval.\n" \
		"For instance if you want 10 frequencies for 44100 KHz, the frequencies will be:\n" \
		"~1KHz ~2KHz ~3KHz ~4KHz ~5KHz ~6KHz ~7KHz ~8KHz ~9KHz ~10Khz\n" \
		"Also note that bass takes frequencies: 200-800 Hz, treble: 1400-10Khz\n" \
	  "The following methods available for "ANALYZER_NAME":\n" \
	  "\t"AS_FREQS_NAME"()"\
	  "\n\t"AS_BANDS_NAME"()"

const char* OUTPUT_DOC=
		OUTPUT_NAME"( frequency, channels, format[ ,deviceId ] )-> OutputSound\n"
		"accepts raw strings of data and allows to control the playback through the following methods:\n"
		"\t"PLAY_NAME"()"
		"\n\t"PAUSE_NAME"()"
		"\n\t"UNPAUSE_NAME"()"
		"\n\t"STOP_NAME"()"
		"\n\t"POSITION_NAME"()"
		"\n\t"CHANNELS_NAME"()"
		"\n\t"ISMUTE_NAME"()"
		"\n\t"SETMUTE_NAME"()"
		"\n\t"GETVOLUME_NAME"()"
		"\n\t"GETRATE_NAME"()"
		"\n\t"SETVOLUME_NAME"()"
		"\n\t"GET_LEFT_NAME"()"
		"\n\t"IS_PLAYING_NAME"()";

const char* INPUT_DOC=
		INPUT_NAME"( frequency, channels, format[ ,deviceId ] )->	InputSound\n"
		"allows to grab sound with different settings of sample rate and channels. Following members are available:\n"
		"\t"START_NAME"()"
		"\n\t"STOP_NAME"()"
		"\n\t"POSITION_NAME"()"
		"\n\t"CHANNELS_NAME"()"
		"\n\t"GETRATE_NAME"()"
		"\n\t"GET_SIZE_NAME"()"
		"\n\t"GET_DATA_NAME"()";

const int MIN_SAMPLES= 32;
const int MAX_SAMPLES= 576;
const float LOW_FREQ= 200;
const float HIGH_FREQ= 11025;
const int MAX_BANDS= 25;

PyObject *g_cErr;

// ---------------------------------------------------------------------------------
typedef struct
{
	PyObject_HEAD

	ReSampleContext * resample_ctx;
} PyResamplerObject;

// ---------------------------------------------------------------------------------
typedef struct
{
	PyObject_HEAD

	FFT * analyzer;
	float* pSamples;
	float* pFreqs;
	int iChannels;
} PyAnalyzerObject;

// ---------------------------------------------------------------------------------
typedef struct
{
	PyObject_HEAD
	OSoundStream* cObj;
} PyOSoundObject;

// ---------------------------------------------------------------------------------
typedef struct
{
	PyObject_HEAD
	ISoundStream* cObj;
} PyISoundObject;


// ---------------------------------------------------------------------------------
static PyObject *
ISound_Start( PyISoundObject* obj )
{
	bool b;
  Py_BEGIN_ALLOW_THREADS
	b= obj->cObj->Start();
  Py_END_ALLOW_THREADS
  if( !b )
	{
 		PyErr_Format( g_cErr, "Cannot grab sound because of %d:%s", obj->cObj->GetLastError(), obj->cObj->GetErrorString() );
		return NULL;
	}
	// Return how many samples are queued
	RETURN_NONE
}

// ---------------------------------------------------------------------------------
static PyObject *
ISound_Stop( PyISoundObject* obj)
{
  obj->cObj->Stop();
	RETURN_NONE
}

// ---------------------------------------------------------------------------------
static PyObject *
ISound_GetPosition( PyISoundObject* obj)
{
  return PyFloat_FromDouble( obj->cObj->GetPosition() );
}

// ---------------------------------------------------------------------------------
static PyObject *
ISound_GetRate( PyISoundObject* obj)
{
	return PyInt_FromLong( obj->cObj->GetRate() );
}

// ---------------------------------------------------------------------------------
static PyObject *
ISound_GetChannels( PyISoundObject* obj)
{
	return PyInt_FromLong( obj->cObj->GetChannels() );
}

// ---------------------------------------------------------------------------------
static PyObject *
ISound_GetSize( PyISoundObject* obj)
{
  return PyInt_FromLong( obj->cObj->GetSize() );
}

// ---------------------------------------------------------------------------------
static PyObject *
ISound_GetData( PyISoundObject* obj)
{
	int iSize= obj->cObj->GetSize();
	if( iSize> 0 )
	{
		PyObject* cRes= PyString_FromStringAndSize( NULL, iSize );
		int i;
	  Py_BEGIN_ALLOW_THREADS
		i= obj->cObj->GetData( PyString_AsString( cRes ), iSize );
		Py_END_ALLOW_THREADS
		if( i< 0 )
		{
 			PyErr_Format( g_cErr, "Read buffer encountered error: %d, %s", obj->cObj->GetLastError(), obj->cObj->GetErrorString() );
			return NULL;
		}
		if( i!= iSize )
		{
 			PyErr_Format( g_cErr, "Read buffer size is not equal to the allocated size( %d vs %d )", i, iSize );
			return NULL;
		}
		return cRes;
	}
	RETURN_NONE
}

// ---------------------------------------------------------------------------------
// List of all methods for the mp3decoder
static PyMethodDef isound_methods[] =
{

	{ START_NAME, (PyCFunction)ISound_Start, METH_NOARGS, START_DOC },
	{ STOP_NAME, (PyCFunction)ISound_Stop, METH_NOARGS,	I_STOP_DOC },
	{ POSITION_NAME, (PyCFunction)ISound_GetPosition, METH_NOARGS,	POSITION_DOC	},
	{ CHANNELS_NAME, (PyCFunction)ISound_GetChannels, METH_NOARGS,	CHANNELS_DOC },
	{ GETRATE_NAME, (PyCFunction)ISound_GetRate, METH_NOARGS,	GETRATE_DOC	},
	{ GET_SIZE_NAME, (PyCFunction)ISound_GetSize, METH_NOARGS,	GET_SIZE_DOC	},
	{ GET_DATA_NAME, (PyCFunction)ISound_GetData, METH_NOARGS,	GET_DATA_DOC	},
	{ NULL, NULL },
};

// ----------------------------------------------------------------
static void
ISoundClose( PyISoundObject *sound )
{
	delete sound->cObj;
	PyObject_Free( sound );
}

// ----------------------------------------------------------------
static PyObject *
ISoundNew( PyTypeObject *type, PyObject *args, PyObject *kwds )
{
	int iFreqRate, iChannels, iFormat, iId= 0;
	if (!PyArg_ParseTuple(args, "iii|i:"INPUT_NAME, &iFreqRate, &iChannels, &iFormat, &iId ))
		return NULL;

	ISoundStream* cStream= new ISoundStream( iFreqRate, iChannels, iFormat, 0, iId );
	if( cStream->GetLastError()!= 0 )
	{
		// Raise an exception when the sampling rate is not supported by the module
		PyErr_Format( g_cErr, "Cannot create sound object. Error text is: %d, %s", cStream->GetLastError(), cStream->GetErrorString() );
		delete cStream;
		return NULL;
	}

	PyISoundObject* cSnd= (PyISoundObject*)type->tp_alloc(type, 0);
	if( !cSnd )
	{
		delete cStream;
		return NULL;
	}

	// return sound module
	cSnd->cObj= cStream;
	return (PyObject*)cSnd;
}

// ----------------------------------------------------------------
PyTypeObject PyISoundType =
{
	PyObject_HEAD_INIT(NULL)
	0,
	MODULE_NAME"."INPUT_NAME,
	sizeof(PyISoundObject),
	0,
	(destructor)ISoundClose,  //tp_dealloc
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
	(char*)INPUT_DOC,		/* tp_doc */
	0,					/* tp_traverse */
	0,					/* tp_clear */
	0,					/* tp_richcompare */
	0,					/* tp_weaklistoffset */
	0,					/* tp_iter */
	0,					/* tp_iternext */
	isound_methods,				/* tp_methods */
	0,					/* tp_members */
	0,					/* tp_getset */
	0,					/* tp_base */
	0,					/* tp_dict */
	0,					/* tp_descr_get */
	0,					/* tp_descr_set */
	0,					/* tp_dictoffset */
	0,					/* tp_init */
	PyType_GenericAlloc,			/* tp_alloc */
	ISoundNew,	/* tp_new */
	PyObject_Del,				/* tp_free */
};

// ---------------------------------------------------------------------------------
static PyObject *
Sound_Play( PyOSoundObject* obj, PyObject *args)
{
	unsigned char* sData;
	int iLen;
	float f;
	if (!PyArg_ParseTuple(args, "s#:play", &sData, &iLen ))
		return NULL;

  Py_BEGIN_ALLOW_THREADS
	f= obj->cObj->Play( sData, iLen );
  Py_END_ALLOW_THREADS
  if( f== -1 )
	{
 		PyErr_Format( g_cErr, "Cannot play sound because of %d:%s", obj->cObj->GetLastError(), obj->cObj->GetErrorString() );
		return NULL;
	}
	// Return how many samples are queued
	return PyFloat_FromDouble( f );
}

// ---------------------------------------------------------------------------------
static PyObject *
Sound_Pause( PyOSoundObject* obj)
{
  obj->cObj->Pause();
	RETURN_NONE
}

// ---------------------------------------------------------------------------------
static PyObject *
Sound_Stop( PyOSoundObject* obj)
{
  obj->cObj->Stop();
	RETURN_NONE
}

// ---------------------------------------------------------------------------------
static PyObject *
Sound_GetPosition( PyOSoundObject* obj)
{
  return PyFloat_FromDouble( obj->cObj->GetPosition() );
}

// ---------------------------------------------------------------------------------
static PyObject *
Sound_Unpause( PyOSoundObject* obj)
{
  obj->cObj->Unpause();
	RETURN_NONE
}

// ---------------------------------------------------------------------------------
static PyObject *
Sound_IsMute( PyOSoundObject* obj)
{
	return PyLong_FromLong( obj->cObj->IsMute() );
}

// ---------------------------------------------------------------------------------
static PyObject *
Sound_SetMute( PyOSoundObject* obj, PyObject *args)
{
	int bMute;
	if (!PyArg_ParseTuple(args, "i:isMute", &bMute ))
		return NULL;

	obj->cObj->SetMute( ( bMute!= 0 ) );
	RETURN_NONE
}

// ---------------------------------------------------------------------------------
static PyObject *
Sound_GetVolume( PyOSoundObject* obj)
{
	return PyLong_FromLong( obj->cObj->GetVolume() & 0xffff );
}

// ---------------------------------------------------------------------------------
static PyObject *
Sound_SetVolume( PyOSoundObject* obj, PyObject *args)
{
	int iVolume;
	if (!PyArg_ParseTuple(args, "i:setVolume", &iVolume ))
		return NULL;

	obj->cObj->SetVolume( iVolume | ( ((unsigned int)iVolume) << 16 ) );
	RETURN_NONE
}

// ---------------------------------------------------------------------------------
static PyObject *
Sound_GetRate( PyOSoundObject* obj)
{
	return PyInt_FromLong( obj->cObj->GetRate() );
}

// ---------------------------------------------------------------------------------
static PyObject *
Sound_GetChannels( PyOSoundObject* obj)
{
	return PyInt_FromLong( obj->cObj->GetChannels() );
}

// ---------------------------------------------------------------------------------
static PyObject *
Sound_IsPlaying( PyOSoundObject* obj)
{
	return PyInt_FromLong( (int)obj->cObj->IsPlaying() );
}

// ---------------------------------------------------------------------------------
static PyObject *
Sound_GetLeft( PyOSoundObject* obj)
{
  return PyFloat_FromDouble( obj->cObj->GetLeft() );
}

// ---------------------------------------------------------------------------------
// List of all methods for the mp3decoder
static PyMethodDef sound_methods[] =
{

	{ PLAY_NAME, (PyCFunction)Sound_Play, METH_VARARGS, PLAY_DOC },
	{ PAUSE_NAME, (PyCFunction)Sound_Pause, METH_NOARGS,	PAUSE_DOC	},
	{ UNPAUSE_NAME, (PyCFunction)Sound_Unpause, METH_NOARGS,	UNPAUSE_DOC	},
	{ STOP_NAME, (PyCFunction)Sound_Stop, METH_NOARGS,	STOP_DOC },
	{ POSITION_NAME, (PyCFunction)Sound_GetPosition, METH_NOARGS,	POSITION_DOC	},
	{ CHANNELS_NAME, (PyCFunction)Sound_GetChannels, METH_NOARGS,	CHANNELS_DOC },
	{ ISMUTE_NAME, (PyCFunction)Sound_IsMute, METH_NOARGS,	ISMUTE_DOC	},
	{ SETMUTE_NAME, (PyCFunction)Sound_SetMute, 	METH_VARARGS,	SETMUTE_DOC	},
	{ GETVOLUME_NAME, (PyCFunction)Sound_GetVolume, METH_NOARGS,	GETVOLUME_DOC	},
	{ GETRATE_NAME, (PyCFunction)Sound_GetRate, METH_NOARGS,	GETRATE_DOC	},
	{ SETVOLUME_NAME, (PyCFunction)Sound_SetVolume, METH_VARARGS,SETVOLUME_DOC	},
	{ IS_PLAYING_NAME, (PyCFunction)Sound_IsPlaying, METH_NOARGS,	IS_PLAYING_DOC	},
	{ GET_LEFT_NAME, (PyCFunction)Sound_GetLeft, METH_NOARGS,	GET_LEFT_DOC	},
	{ NULL, NULL },
};

// ----------------------------------------------------------------
static void
SoundClose( PyOSoundObject *sound )
{
	delete sound->cObj;
	PyObject_Free( sound );
}

// ----------------------------------------------------------------
static PyObject *
SoundNew( PyTypeObject *type, PyObject *args, PyObject *kwds )
{
	int iFreqRate, iChannels, iFormat, iId= 0;
	if (!PyArg_ParseTuple(args, "iii|i:"OUTPUT_NAME, &iFreqRate, &iChannels, &iFormat, &iId ))
		return NULL;

	OSoundStream* cStream= new OSoundStream( iFreqRate, iChannels, iFormat, 0, iId );
	if( cStream->GetLastError()!= 0 )
	{
		// Raise an exception when the sampling rate is not supported by the module
		PyErr_Format( g_cErr, "Cannot create sound object. Error text is: %d, %s", cStream->GetLastError(), cStream->GetErrorString() );
		delete cStream;
		return NULL;
	}

	PyOSoundObject* cSnd= (PyOSoundObject*)type->tp_alloc(type, 0);
	if( !cSnd )
	{
		delete cStream;
		return NULL;
	}

	// return sound module
	cSnd->cObj= cStream;
	return (PyObject*)cSnd;
}

// ----------------------------------------------------------------
PyTypeObject PySoundType =
{
	PyObject_HEAD_INIT(NULL)
	0,
	MODULE_NAME"."OUTPUT_NAME,
	sizeof(PyOSoundObject),
	0,
	(destructor)SoundClose,  //tp_dealloc
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
	(char*)OUTPUT_DOC,		/* tp_doc */
	0,					/* tp_traverse */
	0,					/* tp_clear */
	0,					/* tp_richcompare */
	0,					/* tp_weaklistoffset */
	0,					/* tp_iter */
	0,					/* tp_iternext */
	sound_methods,				/* tp_methods */
	0,					/* tp_members */
	0,					/* tp_getset */
	0,					/* tp_base */
	0,					/* tp_dict */
	0,					/* tp_descr_get */
	0,					/* tp_descr_set */
	0,					/* tp_dictoffset */
	0,					/* tp_init */
	PyType_GenericAlloc,			/* tp_alloc */
	SoundNew,	/* tp_new */
	PyObject_Del,				/* tp_free */
};


// ---------------------------------------------------------------------------------
static PyObject* Resampler_Resample( PyResamplerObject* obj, PyObject *args)
{
	char* sData = NULL;
	int iLen = 0, iSamples= 0;
	PyObject *cRes;
	if (!PyArg_ParseTuple(args, "s#:"RESAMPLE_NAME, &sData, &iLen ))
		return NULL;

	// Calc samples count
	iSamples= iLen/ ( sizeof( short )* obj->resample_ctx->input_channels );

	// Create output buffer
	cRes= PyString_FromStringAndSize( NULL, iSamples* sizeof( short )* obj->resample_ctx->output_channels );
	if( !cRes )
		return NULL;

	audio_resample( obj->resample_ctx, (short*)PyString_AsString( cRes ), (short*)sData, iSamples );
	return cRes;
}

// ---------------------------------------------------------------------------------
// List of all methods for the wmadecoder
static PyMethodDef resampler_methods[] =
{
	{
		RESAMPLE_NAME,
		(PyCFunction)Resampler_Resample,
		METH_VARARGS,
		RESAMPLE_DOC
	},
	{ NULL, NULL },
};

// ----------------------------------------------------------------
static void ResamplerClose( PyResamplerObject *cRes )
{
	if( cRes->resample_ctx )
		audio_resample_close( cRes->resample_ctx );

	PyObject_Free( (PyObject*)cRes );
}
// ---------------------------------------------------------------------------------
static PyObject *
ResamplerNew( PyTypeObject *type, PyObject *args, PyObject *kwds )
{
	ReSampleContext *cResCtx;
	PyResamplerObject* cRes;
	int iFromSRate, iFromChannels, iToSRate, iToChannels;
	if (!PyArg_ParseTuple(args, "(ii)(ii):Resampler", &iFromSRate, &iFromChannels, &iToSRate, &iToChannels ))
		return NULL;

	// try to init resampler
	cResCtx= audio_resample_init( iToChannels, iFromChannels, iToSRate, iFromSRate );
	if(!cResCtx)
	{
		PyErr_Format(g_cErr, "Can't create resampler, bad parameters combination" );
		return NULL;
	}

	cRes= (PyResamplerObject* )type->tp_alloc(type, 0);
	if( !cRes )
	{
		audio_resample_close( cResCtx );
		return NULL;
	}

	cRes->resample_ctx= cResCtx;
	return (PyObject*)cRes;
}

// ----------------------------------------------------------------
PyTypeObject ResamplerType =
{
	PyObject_HEAD_INIT(NULL)
	0,
	MODULE_NAME"."RESAMPLER_NAME,
	sizeof(PyResamplerObject),
	0,
	(destructor)ResamplerClose,  //tp_dealloc
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
	(char*)RESAMPLER_DOC,		/* tp_doc */
	0,					/* tp_traverse */
	0,					/* tp_clear */
	0,					/* tp_richcompare */
	0,					/* tp_weaklistoffset */
	0,					/* tp_iter */
	0,					/* tp_iternext */
	resampler_methods,				/* tp_methods */
	0,					/* tp_members */
	0,					/* tp_getset */
	0,					/* tp_base */
	0,					/* tp_dict */
	0,					/* tp_descr_get */
	0,					/* tp_descr_set */
	0,					/* tp_dictoffset */
	0,					/* tp_init */
	PyType_GenericAlloc,			/* tp_alloc */
	ResamplerNew,	/* tp_new */
	PyObject_Del,				/* tp_free */
};

// ---------------------------------------------------------------------------------
static PyObject* Analyzer_AsFrequencies( PyAnalyzerObject* obj, PyObject *args)
{
	short* sData = NULL;
	int iLen = 0, i, j;
	if (!PyArg_ParseTuple(args, "s#:"AS_FREQS_NAME, &sData, &iLen ))
		return NULL;

	// Start converting every int into float to feed to the analyzer
	iLen/= sizeof( short );
	PyObject* cRes= PyTuple_New( ( ( iLen- 1 )/ obj->analyzer->GetNumSamples() )+ 1 );
	if( !cRes )
		return NULL;

	for( i= 0; i< iLen; i+= obj->analyzer->GetNumSamples() )
	{
		for( j= i; j< i+ obj->analyzer->GetNumSamples() && j< iLen; j++ )
			obj->pSamples[ j- i ]= (float)sData[ j ];

		for( j= 0; j< obj->analyzer->GetNumFreq() / 2; j++ )
			obj->pFreqs[ j ]= 0;

		obj->analyzer->time_to_frequency_domain( obj->pSamples, obj->pFreqs );

		// Create list with frequency data
		PyObject* cRes1= PyTuple_New( obj->analyzer->GetNumFreq()/2 );
		for( int k= 0; k< obj->analyzer->GetNumFreq()/2; k++ )
		{
			PyObject* cTmp= PyFloat_FromDouble( obj->pFreqs[ k ] );
			PyTuple_SetItem( cRes1, k, cTmp );
		}
		PyTuple_SetItem( cRes, i/ obj->analyzer->GetNumSamples(), cRes1 );
	}

	return cRes;
}

// ---------------------------------------------------------------------------------
static PyObject* Analyzer_AsBands( PyAnalyzerObject* obj, PyObject *args)
{
	short* sData = NULL;
	int iBands, iLen = 0, i, j;
	if (!PyArg_ParseTuple(args, "is#:"AS_BANDS_NAME, &iBands, &sData, &iLen ))
		return NULL;

	// Sanity check
	if( iBands> MAX_BANDS )
	{
		PyErr_Format(g_cErr, "Number of bands specified( %d ) cannot exceed max possible "\
												 "bands( %d ). Use "AS_FREQS_NAME"() if you want support more bands.", iBands, MAX_BANDS );
		return NULL;
	}

	// Start converting every int into float to feed to the analyzer
	iLen/= sizeof( short );
	PyObject* cRes= PyTuple_New( ( ( iLen- 1 )/ obj->analyzer->GetNumSamples() )+ 1 );
	if( !cRes )
		return NULL;

	// Calc some static data
	float octaves= (float)( log( HIGH_FREQ/ LOW_FREQ )/ log( 2 ) );
  float octaves_per_band = octaves / iBands;
  float mult = powf( 2.0f, octaves_per_band ); // each band's highest freq.
	float fFreqDelta= ( HIGH_FREQ- LOW_FREQ )/ ( obj->analyzer->GetNumFreq() / 2 );
	for( i= 0; i< iLen; i+= obj->analyzer->GetNumSamples()* obj->iChannels )
	{
		for( j= i; j< i+ obj->analyzer->GetNumSamples() && j< iLen; j+= obj->iChannels )
			obj->pSamples[ ( j/ obj->iChannels )- i ]= (float)sData[ j ];

		for( j= 0; j< obj->analyzer->GetNumFreq() / 2; j++ )
			obj->pFreqs[ j ]= 0;

		obj->analyzer->time_to_frequency_domain( obj->pSamples, obj->pFreqs );

		PyObject* cRes1= PyTuple_New( iBands );
		for( j= 0; j< iBands; j++ )
		{
			int start = (int)(( obj->analyzer->GetNumFreq() / 2 )* LOW_FREQ* powf( mult, (float)j  )/ HIGH_FREQ );
			int end   = (int)(( obj->analyzer->GetNumFreq() / 2 ) * LOW_FREQ* powf( mult, (float)j+1 )/ HIGH_FREQ );
			if (start < 0)
				start = 0;
			if (end > ( obj->analyzer->GetNumFreq() / 2 ))
				end = ( obj->analyzer->GetNumFreq() / 2 );

			float fBandVal = 0;
			for (int k= start; k< end; k++ )
				fBandVal+= obj->pFreqs[ k ];

			PyObject* cRes2= PyTuple_New( 2 );
			PyTuple_SetItem( cRes2, 0, PyFloat_FromDouble( start* fFreqDelta ) );
			PyTuple_SetItem( cRes2, 1, PyFloat_FromDouble( fBandVal / (float)(end-start) ) );
			PyTuple_SetItem( cRes1, j, cRes2 );
		}
		PyTuple_SetItem( cRes, i/ obj->analyzer->GetNumSamples(), cRes1 );
	}
	return cRes;
}

// ---------------------------------------------------------------------------------
// List of all methods for the analyzer
static PyMethodDef analyzer_methods[] =
{
	{
		AS_FREQS_NAME,
		(PyCFunction)Analyzer_AsFrequencies,
		METH_VARARGS,
		AS_FREQS_DOC
	},
	{
		AS_BANDS_NAME,
		(PyCFunction)Analyzer_AsBands,
		METH_VARARGS,
		AS_BANDS_DOC
	},
	{ NULL, NULL },
};

// ----------------------------------------------------------------
static void AnalyzerClose( PyAnalyzerObject *cRes )
{
	if( cRes->analyzer )
		delete cRes->analyzer;

	cRes->analyzer= NULL;

	if( cRes->pFreqs )
		free( cRes->pFreqs );
	if( cRes->pSamples )
		free( cRes->pSamples );
	PyObject_Free( (PyObject*)cRes );
}
// ---------------------------------------------------------------------------------
static PyObject *
AnalyzerNew( PyTypeObject *type, PyObject *args, PyObject *kwds )
{
	PyAnalyzerObject* cRes;
	int iSamplesIn, iFreqsOut, iChannels= 1;
	if (!PyArg_ParseTuple(args, "iii:"ANALYZER_NAME, &iChannels, &iSamplesIn, &iFreqsOut ))
		return NULL;

		// Do some validation of parameters
	if( iSamplesIn< MIN_SAMPLES || iSamplesIn> MAX_SAMPLES )
	{
		PyErr_Format( g_cErr, "Number of samples should be > %d and < %d", MIN_SAMPLES, MAX_SAMPLES );
		return NULL;
	}

	int i= iFreqsOut;
	while( ( i & 1 )== 0 && i )
		i>>= 1;

	if( !i || i> 1 || iFreqsOut> 1024 )
	{
		PyErr_SetString( g_cErr, "Number of frequencies should be the power of 2( 2, 4, 8 etc ) but no more than 1024." );
		return NULL;
	}

	cRes= (PyAnalyzerObject* )type->tp_alloc(type, 0);
	if( !cRes )
		return NULL;

	cRes->analyzer= new FFT();
	cRes->analyzer->Init( iSamplesIn, iFreqsOut );
	cRes->pSamples= (float*)malloc( iSamplesIn* sizeof( float ) );
	cRes->iChannels= iChannels;
	if( !cRes->pSamples )
		return PyErr_NoMemory();

	cRes->pFreqs= (float*)malloc( iFreqsOut* sizeof( float ) );
	if( !cRes->pFreqs )
		return PyErr_NoMemory();

	return (PyObject*)cRes;
}

// ----------------------------------------------------------------
PyTypeObject AnalyzerType =
{
	PyObject_HEAD_INIT(NULL)
	0,
	MODULE_NAME"."ANALYZER_NAME,
	sizeof(PyAnalyzerObject),
	0,
	(destructor)AnalyzerClose,  //tp_dealloc
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
	(char*)ANALYZER_DOC,		/* tp_doc */
	0,					/* tp_traverse */
	0,					/* tp_clear */
	0,					/* tp_richcompare */
	0,					/* tp_weaklistoffset */
	0,					/* tp_iter */
	0,					/* tp_iternext */
	analyzer_methods,				/* tp_methods */
	0,					/* tp_members */
	0,					/* tp_getset */
	0,					/* tp_base */
	0,					/* tp_dict */
	0,					/* tp_descr_get */
	0,					/* tp_descr_set */
	0,					/* tp_dictoffset */
	0,					/* tp_init */
	PyType_GenericAlloc,			/* tp_alloc */
	AnalyzerNew,	/* tp_new */
	PyObject_Del,				/* tp_free */
};

// ---------------------------------------------------------------------------------
static PyObject* GetOutputDevices( PyObject* obj )
{
	// Query operating system for a list of devices
	OutputDevices cDevs;
	if( cDevs.Count()== 0 )
	{
		RETURN_NONE
	}

	// Create as many members as we have output devices
	PyObject* cRes= PyTuple_New( cDevs.Count() );
	if( !cRes )
		return NULL;

	for( int i= 0; i< cDevs.Count(); i++ )
	{
		PyObject* cDev= PyDict_New();
		PyObject* cTemp= PyInt_FromLong( i );
		PyDict_SetItemString( cDev, "id", cTemp );
		PyDict_SetItemString( cDev, "name", PyString_FromString( cDevs.GetName( i )) );
		PyDict_SetItemString( cDev, "manufId", PyString_FromString( cDevs.GetMID( i )) );
		PyDict_SetItemString( cDev, "productId", PyString_FromString( cDevs.GetPID( i )) );
		cTemp= PyInt_FromLong( cDevs.GetChannels( i ));
		PyDict_SetItemString( cDev, "channels", cTemp );
		cTemp= PyInt_FromLong( cDevs.GetFormats( i ));
		PyDict_SetItemString( cDev, "formats", cTemp );
		PyTuple_SetItem( cRes, i, cDev );
	}
	return cRes;
}

// ---------------------------------------------------------------------------------
static PyObject* GetInputDevices( PyObject* obj )
{
	// Query operating system for a list of devices
	InputDevices cDevs;
	if( cDevs.Count()== 0 )
	{
		RETURN_NONE
	}

	// Create as many members as we have output devices
	PyObject* cRes= PyTuple_New( cDevs.Count() );
	if( !cRes )
		return NULL;

	for( int i= 0; i< cDevs.Count(); i++ )
	{
		PyObject* cDev= PyDict_New();
		PyObject* cTemp= PyInt_FromLong( i );
		PyDict_SetItemString( cDev, "id", cTemp );
		PyDict_SetItemString( cDev, "name", PyString_FromString( cDevs.GetName( i )) );
		PyDict_SetItemString( cDev, "manufId", PyString_FromString( cDevs.GetMID( i )) );
		PyDict_SetItemString( cDev, "productId", PyString_FromString( cDevs.GetPID( i )) );
		cTemp= PyInt_FromLong( cDevs.GetChannels( i ));
		PyDict_SetItemString( cDev, "channels", cTemp );
		cTemp= PyInt_FromLong( cDevs.GetFormats( i ));
		PyDict_SetItemString( cDev, "formats", cTemp );
		PyTuple_SetItem( cRes, i, cDev );
	}
	return cRes;
}

// ---------------------------------------------------------------------------------
// List of all methods for the sound module
static PyMethodDef pysound_methods[] =
{
	{
		GET_OUTPUT_DEVICES_NAME,
		(PyCFunction)GetOutputDevices,
		METH_NOARGS,
		GET_OUTPUT_DEVICES_DOC
	},
	{
		GET_INPUT_DEVICES_NAME,
		(PyCFunction)GetInputDevices,
		METH_NOARGS,
		GET_INPUT_DEVICES_DOC
	},
	{ NULL, NULL },
};

extern "C"
{

#define _EXPORT_INT(mod, name) \
  if (PyModule_AddIntConstant(mod, #name, (long) (name)) == -1) return;

// ---------------------------------------------------------------------------------
DL_EXPORT(void)
initsound(void)
{
	Py_Initialize();
	PyObject *m= Py_InitModule(MODULE_NAME, pysound_methods);

	// Initialize tables
	PyModule_AddStringConstant(m, "__doc__", PYMODULEDOC );
	PyModule_AddStringConstant(m, "version", PYSNDVERSION );
	PyModule_AddIntConstant(m, "build", PYSNDBUILD );
  _EXPORT_INT(m, AFMT_MU_LAW);
  _EXPORT_INT(m, AFMT_A_LAW);
  _EXPORT_INT(m, AFMT_IMA_ADPCM);
  _EXPORT_INT(m, AFMT_U8);
  _EXPORT_INT(m, AFMT_S16_LE);
  _EXPORT_INT(m, AFMT_S16_BE);
  _EXPORT_INT(m, AFMT_S8);
  _EXPORT_INT(m, AFMT_U16_LE);
  _EXPORT_INT(m, AFMT_U16_BE);
  _EXPORT_INT(m, AFMT_MPEG);
  _EXPORT_INT(m, AFMT_AC3);
  _EXPORT_INT(m, AFMT_S16_NE);

	g_cErr = PyErr_NewException(MODULE_NAME".Error", NULL, NULL);
	if( g_cErr != NULL)
	  PyModule_AddObject(m, "error", g_cErr );

	PyISoundType.ob_type = &PyType_Type;
	Py_INCREF((PyObject *)&PyISoundType);
	PyModule_AddObject(m, INPUT_NAME, (PyObject *)&PyISoundType);
	PySoundType.ob_type = &PyType_Type;
	Py_INCREF((PyObject *)&PySoundType);
	PyModule_AddObject(m, OUTPUT_NAME, (PyObject *)&PySoundType);
	ResamplerType.ob_type = &PyType_Type;
	Py_INCREF((PyObject *)&ResamplerType);
	PyModule_AddObject(m, RESAMPLER_NAME, (PyObject *)&ResamplerType);
	AnalyzerType.ob_type = &PyType_Type;
	Py_INCREF((PyObject *)&AnalyzerType);
	PyModule_AddObject(m, ANALYZER_NAME, (PyObject *)&AnalyzerType);
}

};

/*
import pygame
pygame.init()
def playFile( sName, iRate ):
	pygame.mixer.init( iRate )
	ch= pygame.mixer.Channel( 0 )
	f= open( sName, 'rb' )
	snd= pygame.mixer.Sound( f )
	ch.play( snd )

def compareFiles( fName1, fName2 ):
	f1= open( fName1, 'rb' )
	f2= open( fName2, 'rb' )
	s1= f1.read( 2000 )
	s2= f2.read( 2000 )
	totalRead= len( s1 )
	while len( s1 )== len( s2 ) and len( s1 )> 0:
		for i in xrange( len( s1 ) ):
			if s1[ i ]!= s2[ i ]:
				print totalRead+ i, ' ',

		s1= f1.read( 2000 )
		s2= f2.read( 2000 )
		totalRead+= len( s1 )

import pysound
pysound.open( 44100, 1 )
f= open( 'test.pcm', 'rb' )
r= f.read(2000000 )
pysound.play( r )
r= f.read(2000000 )
pysound.play( r )
r= f.read(2000000 )
pysound.play( r )
r= f.read(2000000 )
pysound.play( r )
r= f.read(2000000 )
pysound.play( r )
r= f.read(2000000 )
pysound.play( r )
r= f.read(2000000 )
pysound.play( r )
r= f.read(2000000 )
pysound.play( r )
r= f.read(200000 )
pysound.play( r )
pysound.stop()

playFile( 'c:\\Bors\\hmedia\\pympg\\test_1_320.pcm', 44100 )


import pymedia.audio.sound as sound
snd1= sound.Output( 44100, 2, 0x10 )
f= open( 'c:\\bors\\hmedia\\libs\\pymedia\\examples\\test.pcm', 'rb' )
s= f.read( 400000 )
snd1.play( s )
print snd1.isPlaying()

# oss test
import ossaudiodev
snd= ossaudiodev.open( 'w' )
f= open( 'test.pcm', 'rb' )
s= f.read( 400000 )
snd.setparameters( 0x10, 2, 44100 )
snd.writeall( s )


import pymedia.audio
import pymedia.audio.sound as sound
r= sound.Resampler( (44100,6), (44100,2) )
r.resample( '123456781234123456781234' )

import math
import pymedia.audio
import pymedia.audio.sound as sound
SAMPLES= 500
BANDS= 3
a= sound.SpectrAnalyzer( SAMPLES, 256 )
r= sound.Resampler( (44100,2), (44100,1) )
snd= sound.Output( 44100, 2, 0x10 )
f= open( 'c:\\music\\roots.pcm', 'rb' )

s= f.read( SAMPLES* 2* 2 )
s1= r.resample( s )
res= a.asFrequencies( s1 )
res= a.asBands( 3, s1 )


import pymedia.audio.sound as sound
snd= sound.Input( 22050, 1, sound.AFMT_S16_LE )

	*/

