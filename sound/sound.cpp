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

#ifdef WIN32
#include "audio_win.h"
#else
#include "audio_unix.h"
#endif

#ifndef BUILD_NUM
#define BUILD_NUM 1
#endif

#define RETURN_NONE	Py_INCREF( Py_None ); return Py_None;

#define PYSNDBUILD BUILD_NUM
#define PYSNDVERSION "1"
#define PYDOC \
"Sound routines\nAllows to control sound device:\n\
\t- Place sound chunks into the sound queue\n\
\t- Get playback parameters( volume, position etc )\n\
\t- Control the playback( stop, pause, play )\n\
Here is the simple player for a pcm file\n\
( you may add your wav playback by parsing the header ):\n\
\timport sound, time\n\
\tsnd1= sound.output( 44100, 2, 0x00000010 )\n\
\tf= open( 'test.pcm', 'rb' )\n\
\ts= ' '\n\
\twhile len( s )> 0:\n\
\t\ts= f.read( 4000000 )\n\
\t\twhile 1:\n\
\t\t\ttry: snd1.play( s ) \n\
\t\t\texcept: time.sleep( .01 )\n"

#define PLAY_DOC "play( fragment ) -> None\n\tPlay raw frangment using settings specified during output creation\n"
#define CAN_PLAY_DOC "canPlay() -> {0|1}\n\tWhether current sound can( ready ) play fragment or not\n"
#define PAUSE_DOC "pause() -> None\n\tPauses currently playing fragment\n\tIf fragment is not playing, it has effect\n"
#define UNPAUSE_DOC "unpause() -> None \n\tUnpauses currently paused fragment\n\tIf fragment is not paused, it has no effect\n"
#define STOP_DOC "stop() -> None \n\tStops currently played fragment\n"
#define POSITION_DOC "getPosition() -> int\n\tReturns position for the currently playing fragment.\n\tPosition is calculated from the last full stop. Pause does not considered as a stop\n"
#define CHANNELS_DOC "getChannels() -> {1..n}\n\tReturns number of channels that have been passed during the initialization\n"
#define ISMUTE_DOC "isMute() -> ( { 1 | 0 } )\n\tWhether sound is mute\n"
#define SETMUTE_DOC "setMute( { 1 | 0 } ) -> None\n\tSet or unset mute\n"
#define GETVOLUME_DOC "getVolume() -> ( 0..0xffff )\n\tCurrent volume level\n"
#define GETRATE_DOC "getRate() -> ( rate )\n\tCurrent frequency rate\n"
#define SETVOLUME_DOC "setVolume( volume ) -> None\n\tSet volume level\n"
#define IS_PLAYING_DOC "isPlaying() -> {0|1}\n\tWhether sound is playing\n"

const char* OUTPUT_DOC=
"Sound object accepts raw strings of data and allows to \n\
control the playback through the following methods:\n"
"\t"PLAY_DOC
"\t"PAUSE_DOC
"\t"UNPAUSE_DOC
"\t"STOP_DOC
"\t"POSITION_DOC
"\t"CHANNELS_DOC
"\t"ISMUTE_DOC
"\t"SETMUTE_DOC
"\t"GETVOLUME_DOC
"\t"GETRATE_DOC
"\t"SETVOLUME_DOC
"\t"IS_PLAYING_DOC;

PyObject *g_cErr;

// ---------------------------------------------------------------------------------
typedef struct
{
	PyObject_HEAD
	ISoundStream* cObj;
} PyISoundObject;

// ---------------------------------------------------------------------------------
static PyObject *
Sound_Play( PyISoundObject* obj, PyObject *args)
{
	unsigned char* sData;
	int iLen, i;
	if (!PyArg_ParseTuple(args, "s#:play", &sData, &iLen ))
		return NULL;

  Py_BEGIN_ALLOW_THREADS
	i= obj->cObj->Play( sData, iLen );
  Py_END_ALLOW_THREADS
  if( i== -1 )
	{
 		PyErr_Format( g_cErr, "Cannot play sound because of %d:%s", GetLastError(), obj->cObj->GetErrorString() );
		return NULL;
	}
	// Return how many samples are queued
	return PyInt_FromLong( i/ ( obj->cObj->GetChannels()* 2 ) );
}

// ---------------------------------------------------------------------------------
static PyObject *
Sound_Pause( PyISoundObject* obj)
{
  obj->cObj->Pause();
	RETURN_NONE
}

// ---------------------------------------------------------------------------------
static PyObject *
Sound_Stop( PyISoundObject* obj)
{
  obj->cObj->Stop();
	RETURN_NONE
}

// ---------------------------------------------------------------------------------
static PyObject *
Sound_GetPosition( PyISoundObject* obj)
{
  return PyFloat_FromDouble( obj->cObj->GetPosition() );
}

// ---------------------------------------------------------------------------------
static PyObject *
Sound_Unpause( PyISoundObject* obj)
{
  obj->cObj->Unpause();
	RETURN_NONE
}

// ---------------------------------------------------------------------------------
static PyObject *
Sound_IsMute( PyISoundObject* obj)
{
	return PyLong_FromLong( obj->cObj->IsMute() );
}

// ---------------------------------------------------------------------------------
static PyObject *
Sound_SetMute( PyISoundObject* obj, PyObject *args)
{
	int bMute;
	if (!PyArg_ParseTuple(args, "i:isMute", &bMute ))
		return NULL;

	obj->cObj->SetMute( ( bMute!= 0 ) );
	RETURN_NONE
}

// ---------------------------------------------------------------------------------
static PyObject *
Sound_GetVolume( PyISoundObject* obj)
{
	return PyLong_FromLong( obj->cObj->GetVolume() & 0xffff );
}

// ---------------------------------------------------------------------------------
static PyObject *
Sound_SetVolume( PyISoundObject* obj, PyObject *args)
{
	int iVolume;
	if (!PyArg_ParseTuple(args, "i:setVolume", &iVolume ))
		return NULL;

	obj->cObj->SetVolume( iVolume | ( ((unsigned int)iVolume) << 16 ) );
	RETURN_NONE
}

// ---------------------------------------------------------------------------------
static PyObject *
Sound_GetRate( PyISoundObject* obj)
{
	return PyInt_FromLong( obj->cObj->GetRate() );
}

// ---------------------------------------------------------------------------------
static PyObject *
Sound_GetChannels( PyISoundObject* obj)
{
	return PyInt_FromLong( obj->cObj->GetChannels() );
}

// ---------------------------------------------------------------------------------
static PyObject *
Sound_IsPlaying( PyISoundObject* obj)
{
	return PyInt_FromLong( (int)obj->cObj->IsPlaying() );
}

// ---------------------------------------------------------------------------------
// List of all methods for the mp3decoder
static PyMethodDef sound_methods[] =
{
	{ "play", (PyCFunction)Sound_Play, METH_VARARGS, PLAY_DOC },
	{ "pause", (PyCFunction)Sound_Pause, METH_NOARGS,	PAUSE_DOC	},
	{ "unpause", (PyCFunction)Sound_Unpause, METH_NOARGS,	UNPAUSE_DOC	},
	{ "stop", (PyCFunction)Sound_Stop, METH_NOARGS,	STOP_DOC },
	{ "getPosition", (PyCFunction)Sound_GetPosition, METH_NOARGS,	POSITION_DOC	},
	{ "getChannels", (PyCFunction)Sound_GetChannels, METH_NOARGS,	CHANNELS_DOC },
	{ "isMute", (PyCFunction)Sound_IsMute, METH_NOARGS,	ISMUTE_DOC	},
	{ "setMute", (PyCFunction)Sound_SetMute, 	METH_VARARGS,	SETMUTE_DOC	},
	{ "getVolume", (PyCFunction)Sound_GetVolume, METH_NOARGS,	GETVOLUME_DOC	},
	{ "getRate", (PyCFunction)Sound_GetRate, METH_NOARGS,	GETRATE_DOC	},
	{ "setVolume", (PyCFunction)Sound_SetVolume, METH_VARARGS,SETVOLUME_DOC	},
	{ "isPlaying", (PyCFunction)Sound_IsPlaying, METH_NOARGS,	IS_PLAYING_DOC	},
	{ NULL, NULL },
};

// ----------------------------------------------------------------
static void
SoundClose( PyISoundObject *sound )
{
	delete sound->cObj;
	PyObject_Free( sound );
}

// ----------------------------------------------------------------
static PyObject *
SoundNew( PyTypeObject *type, PyObject *args, PyObject *kwds )
{
	int iFreqRate, iChannels, iFormat;
	if (!PyArg_ParseTuple(args, "iii:", &iFreqRate, &iChannels, &iFormat ))
		return NULL;

	ISoundStream* cStream= new ISoundStream( iFreqRate, iChannels, iFormat, 0 );
	if( GetLastError()!= 0 )
	{
		// Raise an exception when the sampling rate is not supported by the module
		PyErr_Format( g_cErr, "Cannot create sound object. Error code is: %d", GetLastError() );
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
PyTypeObject PySoundType =
{
	PyObject_HEAD_INIT(NULL)
	0,
	"pymedia.sound.Output",
	sizeof(PyISoundObject),
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
// List of all methods for the mp3decoder
static PyMethodDef pysound_methods[] =
{
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
	PyObject *m= Py_InitModule("sound", pysound_methods);

	// Initialize tables
	PyModule_AddStringConstant(m, "__doc__", PYDOC );
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

 	g_cErr = PyErr_NewException("pymedia.sound.Error", NULL, NULL);
	if( g_cErr != NULL)
	  PyModule_AddObject(m, "error", g_cErr );

	PySoundType.ob_type = &PyType_Type;
	Py_INCREF((PyObject *)&PySoundType);
	PyModule_AddObject(m, "Output", (PyObject *)&PySoundType);
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
snd1= sound.output( 44100, 2, 0x00000010 )
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


	*/

