/*
 *			Python wrapper for the direct cdda extraction class.
 *			Also provides with some basic functionality to identify all
 *			CD/DVD ROM devices in a system plus some helpful information
 *			about them.
 *
 *						Copyright (C) 2002-2003  Dmitry Borisov
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


#include "Python.h"
#ifdef WIN32
#include "cdda_win.h"
#else
#include "cdda_unix.h"
#endif

#define INIT_DOC "init() -> \n\tinitialize cdrom subsystem\n"
#define QUIT_DOC "quit() -> \n\tquit cdrom subsystem\n"
#define GETCOUNT_DOC "getCount() -> ( count ) \n\treturn number of cdroms in a system\n"

#ifndef BUILD_NUM
#define BUILD_NUM 1
#endif

const int PYCDDABUILD= BUILD_NUM;
const char* PYCDDAVERSION= "2";
const char* PYDOC= 
"CD direct CD/DVD ROM access.\nProvides the following functionality:\n"
"- Identify all drives in a system\n"
"- Direct access in file based mode to these drives per track\n"
"The following methods available from the module directly:\n"
"\t"INIT_DOC
"\t"QUIT_DOC
"\t"GETCOUNT_DOC
"To access content of each individual drive in the system, use cd.CD( index )\n";

#define CD_ISAUDIO_DOC "isAudio() -> audio\n\tReturns whether cd-rom has audio data or not\n"
#define CD_ISREADY_DOC "isReady() -> ready\n\tReturns whether drive is ready or not\n"
#define CD_EJECT_DOC "eject() -> None\n\tEject tray\n"
#define CD_OPEN_DOC "open( track ) -> track\n\tReturn file like object mapped to given track.\n"\
										"\tTrack supports following methods:\n\t\tread( bytes ) -> data\n\t\tseek( offset, seek_pos ) -> None\n"\
										"\t\tclose() -> None\n\t\ttell() -> pos\n"
#define CD_GETNAME_DOC "getName() -> ( name )\n\tReturn the name of the drive\n"
#define CD_GETTOC_DOC "getTOC() -> ( ( frameStart, frameEnd )* )\n" \
											"\tReturn toc for the drive. If the drive has no audio, returns list of sessions( if any )\n"

#define CDDA_DOC "CD allow to open for direct reading particular CD-ROM based on its index in a system\n"\
								"Once opened, the following methods may be used to work with:\n"\
								"\t"CD_ISAUDIO_DOC \
								"\t"CD_ISREADY_DOC \
								"\t"CD_EJECT_DOC \
								"\t"CD_OPEN_DOC \
								"\t"CD_GETNAME_DOC \
								"\t"CD_GETTOC_DOC 

PyObject *g_cErr;
PyObject *g_d;

extern int GetDrivesList( char* sDrives[ 255 ][ 20 ] );

typedef struct 
{
	PyObject_HEAD
	CDDARead* cObject;
} PyCDDAObject;

typedef struct 
{
	PyObject_HEAD
	int iPos;
	int iStartFrame;
	int iEndFrame;
	CDDARead* cObject;
} PyTrackObject;


int g_iMaxCDROMIndex;
char g_sDrives[ 255 ][ 20 ];

/* ---------------------------------------------------------------------------------*/
static PyObject *
Track_Read( PyTrackObject *track, PyObject *args)
{
	PyObject *cRes;
	int iSize, iFrameFrom, iFrameTo, iStrLen, iOffs, i;
	void *pBuf= NULL;
	if (!PyArg_ParseTuple(args, "i:read", &iSize ))
		return NULL;

	iFrameFrom= track->iStartFrame+ ( track->iPos/ track->cObject->GetSectorSize() );
	iFrameTo= track->iStartFrame+ ( ( track->iPos+ iSize )/ track->cObject->GetSectorSize() )+ 1;
	if( iFrameTo> track->iEndFrame )
		iFrameTo= track->iEndFrame;

	/* Calculate the real length being read */
	iStrLen= ( iFrameTo- iFrameFrom )* track->cObject->GetSectorSize();
	iOffs= track->iPos- ( iFrameFrom- track->iStartFrame )* track->cObject->GetSectorSize();
	if( iOffs+ iSize> iStrLen )
		iSize= iStrLen- iOffs;
	if( !iSize )
		return PyString_FromStringAndSize( NULL, 0 );

	/* Create buffer to store the data of expected size */
	pBuf= malloc( iStrLen );
	if( !pBuf )
	{
		char s[ 255 ];
		sprintf( s, "Cannot allocate %d bytes of memory for cdda frames. Code %d .", iStrLen, GetLastError() );
		PyErr_SetString( g_cErr, s );
		return NULL; 
	}

	/* Read needed frames into the string */
  Py_BEGIN_ALLOW_THREADS
	i= track->cObject->ReadSectors( iFrameFrom, iFrameTo, (char*)pBuf, iStrLen );
  Py_END_ALLOW_THREADS
	if( i== -1 )
	{
		/* Raise an exception when the sampling rate is not supported by the module */
		char s[ 255 ];
		sprintf( s, "cdda read encountered error: Code %d .", GetLastError() );
		PyErr_SetString(g_cErr, s );
		free( pBuf );
		return NULL; 
	}

	cRes= PyString_FromStringAndSize( NULL, iSize );
	if( !cRes )
		return NULL;

	memcpy( PyString_AsString( cRes ), (char*)pBuf+ iOffs, iSize );
	track->iPos+= iSize;
	free( pBuf );
	return cRes;
}

/* ---------------------------------------------------------------------------------*/
static PyObject *
Track_Close( PyTrackObject *track, PyObject *args)
{
	if (!PyArg_ParseTuple(args, ":close"))
		return NULL;

	track->iPos= -1;
	Py_INCREF( Py_None );
	return Py_None;
}

/* ---------------------------------------------------------------------------------*/
static PyObject *
Track_Seek( PyTrackObject *track, PyObject *args)
{
	int iOffset, iOrigin, iSize, iTmp;
	if (!PyArg_ParseTuple(args, "ii:seek", &iOffset, &iOrigin ))
		return NULL;

	if( track->iPos< 0 )
	{
		PyErr_SetString(g_cErr, "Cannot seek through the closed file." );
		return NULL; 
	}

	iTmp= track->iPos;
	iSize= track->cObject->GetSectorSize()* ( track->iEndFrame- track->iStartFrame );
	switch( iOrigin )
	{
		case SEEK_CUR: 
			iTmp+= iOffset;
			break;
		case SEEK_END:
			iTmp= iSize+ iOffset;
			break;
		case SEEK_SET:
			iTmp= iOffset;
			break;
	};

	if( iTmp< 0 || track->iPos> iSize )
	{
		PyErr_SetString(g_cErr, "Cannot seek beyond the file bounds" );
		return NULL; 
	}

	track->iPos= iTmp;
	Py_INCREF( Py_None );
	return Py_None;
}

/* ---------------------------------------------------------------------------------*/
static PyObject *
Track_Tell( PyTrackObject *track, PyObject *args)
{
	if (!PyArg_ParseTuple(args, ":tell"))
		return NULL;

	if( track->iPos< 0 )
	{
		PyErr_SetString(g_cErr, "Cannot tell the position on a closed file." );
		return NULL; 
	}
	return PyLong_FromLong( track->iPos );
}

/* ---------------------------------------------------------------------------------*/
// File like object to support contiques read from a track
static PyMethodDef track_methods[] = 
{
	{ 
		"read", 
		(PyCFunction)Track_Read, 
		METH_VARARGS,
	  "read( size ) -> data\n"
		"reads data from the track in a file like fashion"
	},
	{ 
		"close", 
		(PyCFunction)Track_Close, 
		METH_VARARGS,
	  "close() -> None\n"
		"Close currently open file. If already closed, does nothing"
	},
	{ 
		"seek", 
		(PyCFunction)Track_Seek, 
		METH_VARARGS,
	  "seek( pos, origin ) -> None\n"
		"Seek track into the certain position"
	},
	{ 
		"tell", 
		(PyCFunction)Track_Tell, 
		METH_VARARGS,
	  "tell() -> pos\n"
		"Returns current position in a file"
	},
	{ NULL, NULL },
};


/* ----------------------------------------------------------------*/
static void
TrackFree( PyTrackObject *track )
{
	PyObject_Free( track ); 
}

/* ---------------------------------------------------------------- */
static PyObject *
TrackGetAttr( PyTrackObject *track, char *name)
{
	return Py_FindMethod( track_methods, (PyObject *)track, name);
}
 
/* ----------------------------------------------------------------*/
static PyTypeObject PyTrackType = 
{
	PyObject_HEAD_INIT(NULL)
	0,
	"Track",
	sizeof(PyTrackObject),
	0,
	(destructor)TrackFree,  /*tp_dealloc*/
	0,			  //tp_print
	(getattrfunc)TrackGetAttr, /*tp_getattr*/
	0,			  //tp_setattr
	0,			  //tp_compare
	0,			  //tp_repr
	0,			  //tp_as_number
	0,			  //tp_as_sequence
	0,				//tp_as_mapping
};
 

// ---------------------------------------------------------------------------------
static PyObject *
CD_Eject( PyCDDAObject *cdda, PyObject *args)
{
	if (!PyArg_ParseTuple(args, ":eject"))
		return NULL;

	if( cdda->cObject->Eject()== -1 )
	{
		// Raise an exception when eject was not successfull
		char s[ 255 ];
		sprintf( s, "cdda eject encountered error: Code %d .", GetLastError() );
		PyErr_SetString(g_cErr, s );
		return NULL; 
	}
	Py_INCREF( Py_None );
	return Py_None;
}

// ---------------------------------------------------------------------------------
static PyObject *
CD_IsReady( PyCDDAObject *cdda, PyObject *args)
{
	bool bReady;
	if (!PyArg_ParseTuple(args, ":isReady"))
		return NULL;

  Py_BEGIN_ALLOW_THREADS
	bReady= cdda->cObject->RefreshTOC( true )!= -1;
  Py_END_ALLOW_THREADS
	if( !bReady && GetLastError()== 31 )
		bReady= true;
	return PyLong_FromLong( bReady );
}
// ---------------------------------------------------------------------------------
static PyObject *
CD_IsAudio( PyCDDAObject *cdda, PyObject *args)
{
	bool bReady;
	if (!PyArg_ParseTuple(args, ":isAudio"))
		return NULL;

  Py_BEGIN_ALLOW_THREADS
	bReady= cdda->cObject->RefreshTOC( true )!= -1;
  Py_END_ALLOW_THREADS
	if( !bReady )
		if( GetLastError()== 31 )
			return PyLong_FromLong( false );
		else
		{
			char s[ 255 ];
			sprintf( s, "Removable disk is not ready: Error code %d .", GetLastError() );
			PyErr_SetString(g_cErr, s );
			return NULL; 
		}

	return PyLong_FromLong( true );
}
// ---------------------------------------------------------------------------------
static PyObject *
CD_GetName( PyCDDAObject *cdda, PyObject *args)
{
	if (!PyArg_ParseTuple(args, ":getName"))
		return NULL;

	return PyString_FromString( cdda->cObject->GetName() );
}

// ---------------------------------------------------------------------------------
static PyObject *
CD_Open( PyCDDAObject *cdda, PyObject *args)
{
	/* Some validations first */
	char *sPath, *sTrack;
	unsigned int iTrack, i;
	if (!PyArg_ParseTuple(args, "s:open", &sPath ))
		return NULL;

	// Get track number from the path( path is like 'Track 01' or '/dev/cdrom/Track 01' or 'd:\\Track 01' )
	sTrack= strstr( sPath, "Track " );
	// See if sPath not a relative path, then validate a path
	if( sTrack && sTrack!= sPath )
		if( strncmp( sPath, cdda->cObject->GetName(), strlen( cdda->cObject->GetName() )))
			sTrack= NULL;
		else if( sTrack- sPath!= (signed)strlen( cdda->cObject->GetName() ) )
			sTrack= NULL;

	if( sTrack && strlen( sTrack )!= 8 )
		sTrack= NULL;

	if( !sTrack )
	{
		char s[ 255 ];
		sprintf( s, "Path %s is not a valid cdda track name. Name should be like '<drive_path>/Track 01'. Cannot open.", sPath );
		PyErr_SetString(g_cErr, s );
		return NULL; 
	}

	// Refresh toc for correct sector numbers
  Py_BEGIN_ALLOW_THREADS
	i= cdda->cObject->RefreshTOC( true );
  Py_END_ALLOW_THREADS
	if( i== -1 )
	{
		// Raise an exception when the sampling rate is not supported by the module
		char s[ 255 ];
		sprintf( s, "cdda read toc encountered error: Code %d .", GetLastError() );
		PyErr_SetString(g_cErr, s );
		return NULL; 
	}

  iTrack= ( sTrack[ 6 ]- '0' )* 10+ sTrack[ 7 ]- '0';
	if( iTrack< 1 || iTrack> cdda->cObject->GetTracksCount() )
	{
		char s[ 255 ];
		sprintf( s, "Track number( %d ) is out of range( 1, %d ). Cannot open.", iTrack, cdda->cObject->GetTracksCount() );
		PyErr_SetString(g_cErr, s );
		return NULL; 
	}

	/* Create new file-like object based on a currently existsing track definitions */
	PyTrackObject* track= PyObject_New( PyTrackObject, &PyTrackType );	
	if( !track )
		return NULL;

	track->cObject= cdda->cObject;
	track->iPos= 0;
	track->iStartFrame= cdda->cObject->GetStartSector( iTrack- 1 );
	track->iEndFrame= cdda->cObject->GetStartSector( iTrack );
	return (PyObject*)track;
}

// ---------------------------------------------------------------------------------
static PyObject *
CD_GetTOC( PyCDDAObject *cdda, PyObject *args)
{
	int i;
	if (!PyArg_ParseTuple(args, ":getTOC" ))
		return NULL;

	// Refresh drive information
  Py_BEGIN_ALLOW_THREADS
	i= cdda->cObject->RefreshTOC( true );
  Py_END_ALLOW_THREADS
	if( i == -1 )
	{
		// Raise an exception when the sampling rate is not supported by the module
		char s[ 255 ];
		sprintf( s, "cdda read toc encountered error: Code %d .", GetLastError() );
		PyErr_SetString(g_cErr, s );
		return NULL; 
	}

	// Create tuple with certain number of entries
	PyObject* cRes= PyTuple_New( cdda->cObject->GetTracksCount() );
	for( i= 0; i< (int)cdda->cObject->GetTracksCount(); i++ )
	{
		// Create tuple for the 2 items( from and to )
		PyObject * cSubRes= PyTuple_New( 2 );
		int iStart= cdda->cObject->GetStartSector( i );
		PyTuple_SetItem( cSubRes, 0, PyLong_FromLong( iStart ) );
		PyTuple_SetItem( cSubRes, 1, PyLong_FromLong( cdda->cObject->GetStartSector( i+ 1 )- iStart ) );
		PyTuple_SetItem( cRes, i, cSubRes );
	}
	return cRes;
}

// ---------------------------------------------------------------------------------
// List of all methods for the mp3decoder
static PyMethodDef cdda_methods[] = 
{
	{ 
		"isAudio", 
		(PyCFunction)CD_IsAudio, 
		METH_VARARGS,
	  CD_ISAUDIO_DOC
	},
	{ 
		"isReady", 
		(PyCFunction)CD_IsReady, 
		METH_VARARGS,
	  CD_ISREADY_DOC
	},
	{ 
		"eject", 
		(PyCFunction)CD_Eject, 
		METH_VARARGS,
	  CD_EJECT_DOC
	},
	{ 
		"open", 
		(PyCFunction)CD_Open, 
		METH_VARARGS,
	  CD_OPEN_DOC
	},
	{ 
		"getName", 
		(PyCFunction)CD_GetName, 
		METH_VARARGS,
	  CD_GETNAME_DOC
	},
	{ 
		"getTOC", 
		(PyCFunction)CD_GetTOC, 
		METH_VARARGS,
	  CD_GETTOC_DOC
	},
	{ NULL, NULL },
};

// ----------------------------------------------------------------
static void
CDDAClose( PyCDDAObject *cdda )
{
	delete cdda->cObject;
	PyObject_Free( cdda ); 
}

// ---------------------------------------------------------------------------------
static PyObject *
CDNew( PyTypeObject *type, PyObject *args, PyObject *kwds )
{
	int iIndex;
	if (!PyArg_ParseTuple(args, "i:", &iIndex ))
		return NULL;

	// Whether init was run ?
	if( g_iMaxCDROMIndex== -1 )
	{
		PyErr_SetString(g_cErr, "cdda module was not initialized, Please initialize it first." );
		return NULL; 
	}

	if( iIndex< 0 || iIndex>= g_iMaxCDROMIndex )
	{
		// Raise an exception when the sampling rate is not supported by the module
		char s[ 255 ];
		sprintf( s, "No such cdrom %d in a system.", iIndex );
		PyErr_SetString(g_cErr, s );
		return NULL; 
	}

	char* sDriveName= g_sDrives[ iIndex ];
	PyCDDAObject* cdda= (PyCDDAObject*)type->tp_alloc(type, 0);
	if( !cdda )
		return NULL;

	cdda->cObject= new CDDARead( sDriveName );
	if( !cdda->cObject )
		return NULL;

	if( GetLastError()!= 0 )
	{
		// Raise an exception when cdrom cannot be opened
		char s[ 255 ];
		sprintf( s, "Cannot open CD drive. Error code is: %d", GetLastError() );
		PyErr_SetString(g_cErr, s );
		CDDAClose( cdda );
		return NULL; 
	}

	return (PyObject*)cdda;
} 

// ----------------------------------------------------------------
static PyTypeObject PyCDDAType = 
{
	PyObject_HEAD_INIT(NULL)
	0,
	"pymedia.cd.CD",
	sizeof(PyCDDAObject),
	0,
	(destructor)CDDAClose,  //tp_dealloc
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
	(char*)CDDA_DOC,		/* tp_doc */
	0,					/* tp_traverse */
	0,					/* tp_clear */
	0,					/* tp_richcompare */
	0,					/* tp_weaklistoffset */
	0,					/* tp_iter */
	0,					/* tp_iternext */
	cdda_methods,				/* tp_methods */
	0,					/* tp_members */
	0,					/* tp_getset */
	0,					/* tp_base */
	0,					/* tp_dict */
	0,					/* tp_descr_get */
	0,					/* tp_descr_set */
	0,					/* tp_dictoffset */
	0,					/* tp_init */
	PyType_GenericAlloc,			/* tp_alloc */
	CDNew,	/* tp_new */
	PyObject_Del,				/* tp_free */
};
 
// ---------------------------------------------------------------------------------
static PyObject *
CDROMCount( PyObject* obj)
{
	// Whether init was run ?
	if( g_iMaxCDROMIndex== -1 )
	{
		PyErr_SetString(g_cErr, "cdda module was not initialized, please initialize it first." );
		return NULL; 
	}

	return PyInt_FromLong( g_iMaxCDROMIndex );
}

// ---------------------------------------------------------------------------------
static PyObject *
Quit( PyObject* obj)
{
	g_iMaxCDROMIndex= -1;

	Py_INCREF( Py_None );
	return Py_None;
}

// ---------------------------------------------------------------------------------
// Fake for now. Need to improve.
static PyObject *
Init( PyObject* obj)
{
	/* Scan the system for CD-ROM drives */
	g_iMaxCDROMIndex= GetDrivesList( g_sDrives );

	Py_INCREF( Py_None );
	return Py_None;
}

// ---------------------------------------------------------------------------------
// List of all methods for the mp3decoder
static PyMethodDef pycdda_methods[] = 
{
	{ "init",	(PyCFunction)Init, 	METH_NOARGS, INIT_DOC },
	{ "quit", (PyCFunction)Quit,  METH_NOARGS, QUIT_DOC },
	{ "getCount", (PyCFunction)CDROMCount, METH_NOARGS, GETCOUNT_DOC	},
	{ NULL, NULL },
};

extern "C"
{ 
// ---------------------------------------------------------------------------------
#define INT_CONSTANT(name) PyModule_AddIntConstant( m, #name, name )

// ---------------------------------------------------------------------------------
DL_EXPORT(void)
initcd(void) 
{
	Py_Initialize();
	g_iMaxCDROMIndex= -1;
	PyObject *m = Py_InitModule("cd", pycdda_methods);
	PyModule_AddStringConstant( m, "__doc__", (char*)PYDOC );
	PyModule_AddStringConstant( m, "version", (char*)PYCDDAVERSION );
	PyModule_AddIntConstant( m, "build", PYCDDABUILD );

	INT_CONSTANT( SEEK_SET );
	INT_CONSTANT( SEEK_END );
	INT_CONSTANT( SEEK_CUR );
	g_cErr = PyErr_NewException("pymedia.cd.Error", NULL, NULL);
	if( g_cErr != NULL)
	  PyModule_AddObject(m, "error", g_cErr );

	PyCDDAType.ob_type = &PyType_Type;
	Py_INCREF((PyObject *)&PyCDDAType); 
	PyModule_AddObject(m, "CD", (PyObject *)&PyCDDAType);
}

};

/*
import cd
cd.init()
c= cd.CD(0)
toc= c.getTOC()
tr0= c.open( "d:\\Track 01" )
s= tr0.read( 400000 )

	*/
