
/*
 *			Python wrapper for the direct dvd reader class.
 *			Has ability to retrieve full information about titles and chapters
 *
 *						Copyright (C) 2002-2004  Dmitry Borisov
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
#include "dvd.h"
#include "dvdlibs/dvdread/inttypes.h"

#define RETURN_NONE \
{ \
	Py_INCREF( Py_None ); \
	return Py_None; \
}

extern PyObject *g_cErr;

/* ---------------------------------------------------------------------------------*/
void TrackClose( DVD_TRACK_INFO* stTrack )
{
	if( stTrack->file )
		DVDCloseFile( stTrack->file );
	if( stTrack->ifo_file )
		ifoClose( stTrack->ifo_file );
	if( stTrack->dvd )
		DVDClose( stTrack->dvd );
	if( stTrack->sBuf )
		free( stTrack->sBuf );

	stTrack->file= NULL;
	stTrack->dvd= NULL;
	stTrack->ifo_file= NULL;
	stTrack->sBuf= NULL;
	stTrack->iVobuNo= -1;
}

/* ---------------------------------------------------------------------------------*/
bool TrackOpened( PyTrackObject *track, char* action )
{
	DVD_TRACK_INFO* stTrack= (DVD_TRACK_INFO*)track->pData;
	if( stTrack->iVobuNo== -1 )
		// Raise an exception when something wrong with dvd position
		PyErr_Format(g_cErr, "dvd track is closed. Cannot %s.", action );

	return ( stTrack->iVobuNo!= -1 );
}

/* ---------------------------------------------------------------------------------*/
float TitleLength( DVD_TRACK_INFO* stTrack )
{
	pgc_t* pgc= stTrack->ifo_file->vts_pgcit->pgci_srp[ 0 ].pgc;
	return (float)pgc->playback_time.hour* 3600+ 
				 pgc->playback_time.minute* 60+ 
				 pgc->playback_time.second;
}

/* ---------------------------------------------------------------------------------*/
int ReadVOBU( DVD_TRACK_INFO* stTrack, int iSector )
{
  unsigned char data[ DVD_VIDEO_LB_LEN ]; 
	dsi_t dsi_pack;
	int iCount;

  /**
   * Read NAV packet.
   */
  int len = DVDReadBlocks( stTrack->file, iSector, 1, data );
  if( len != 1 ) 
	{
		PyErr_Format( g_cErr, "Read failed for block %d\n", iSector );
		return -1;
  }

  /**
   * Parse the contained dsi packet.
   */
  navRead_DSI( &dsi_pack, &(data[ DSI_START_BYTE ]) );
  iCount= dsi_pack.dsi_gi.vobu_ea;
  iSector++;

	// Allocate memory if needed
	if( !stTrack->sBuf || stTrack->iLen< (int)( iCount* DVD_VIDEO_LB_LEN ) )
	{
		if( stTrack->sBuf )
			free( stTrack->sBuf );

		stTrack->sBuf= (unsigned char*)malloc( iCount* DVD_VIDEO_LB_LEN );
		if( !stTrack->sBuf )
		{
			PyErr_NoMemory();
			return -1;
		}
		stTrack->iLen= iCount* DVD_VIDEO_LB_LEN;
	}

  /**
   * Read in and output cursize packs.
   */
  len = DVDReadBlocks( stTrack->file, iSector, iCount, stTrack->sBuf );
  if( len != iCount ) 
	{
    PyErr_Format( g_cErr, "Read failed for %d blocks at %d\n", iCount, iSector );
    return -1;
  }

	return len;
}

/* ---------------------------------------------------------------------------------*/
INT64 GetTrackSize( DVD_TRACK_INFO* stTrack, int iVobuNo )
{
	INT64 iSize= 0;

	vobu_admap_t *vobu_admap= stTrack->ifo_file->vts_vobu_admap;
	int iVobuCount= (vobu_admap->last_byte + 1 - VOBU_ADMAP_SIZE)/4; 
  int entries = (stTrack->ifo_file->vts_c_adt->last_byte + 1 - C_ADT_SIZE)/sizeof(c_adt_t); 

	// Scan all vobus
	for( ; iVobuNo< stTrack->iVobuCount; iVobuNo++ )
	{
		int iVobuNo1= stTrack->aiVobu[ iVobuNo ];
		int iSects= ( ( iVobuNo1== iVobuCount- 1 ) ?
			stTrack->ifo_file->vts_c_adt->cell_adr_table[ entries- 1 ].last_sector:
			vobu_admap->vobu_start_sectors[ iVobuNo1+ 1 ] ) -
				vobu_admap->vobu_start_sectors[ iVobuNo1 ]- 1;
		if( iSects< 0 )
			return -1;

		iSize+= iSects* DVD_VIDEO_LB_LEN;
	}
	return iSize;
}

/* ---------------------------------------------------------------------------------*/
static PyObject *
DVDCD_Track_Read( PyTrackObject *track, PyObject *args)
{
	int iSize, iReqSize;
	if (!PyArg_ParseTuple(args, "i:read", &iReqSize ))
		return NULL;

	// Check if track is opened
	if( !TrackOpened( track, "read" ) )
		return NULL;

	// Find out sectors to read
	DVD_TRACK_INFO* stTrack= (DVD_TRACK_INFO*)track->pData;
	vobu_admap_t *vobu_admap= stTrack->ifo_file->vts_vobu_admap;
	int iVobuCount= (vobu_admap->last_byte + 1 - VOBU_ADMAP_SIZE)/4; 
	if( stTrack->iVobuNo>= stTrack->iVobuCount )
		return PyString_FromString( "" );

	// Allocate space for a final buffer
	iSize= iReqSize;
	PyObject* cRes= PyString_FromStringAndSize( NULL, iSize );
	if( !cRes )
		return NULL;

	char* sBuf= PyString_AsString( cRes );

	// Allocate buffers for intermediate read if needed
	while( iSize && stTrack->iVobuNo< iVobuCount )
	{
		int iVobuNo1= stTrack->aiVobu[ stTrack->iVobuNo ];
		int iStartSector= vobu_admap->vobu_start_sectors[ iVobuNo1 ];
		// Read into buffer
		int iLen;
	  Py_BEGIN_ALLOW_THREADS
		iLen= ReadVOBU( stTrack, iStartSector );
	  Py_END_ALLOW_THREADS
		if( iLen< 0 )
		{
			// Something went wrong( ReadVOBU has set up the error already )
			Py_DECREF( cRes );
			return NULL;
		}

		// Copy data from the recently read sectors into result string
		iLen*= DVD_VIDEO_LB_LEN;
		if( stTrack->iVobuPos< iLen )
		{
			if( iSize< ( iLen- stTrack->iVobuPos ) )
			{
				memcpy( sBuf, stTrack->sBuf+ stTrack->iVobuPos, iSize );
				stTrack->iVobuPos+= iSize;
				iSize= 0;
			}
			else
			{
				memcpy( sBuf, stTrack->sBuf+ stTrack->iVobuPos, iLen- stTrack->iVobuPos );
				iSize-= iLen- stTrack->iVobuPos;
				sBuf+= iLen- stTrack->iVobuPos;
				stTrack->iVobuPos= 0;
				stTrack->iVobuNo++;
			}
		}
		else
			break;
	}

	// Realloc string 
	if( !iSize )
		_PyString_Resize( &cRes, iReqSize- iSize );

	return cRes;
}

/* ---------------------------------------------------------------------------------*/
static PyObject *
DVDCD_Track_Close( PyTrackObject *track )
{
	if( !TrackOpened( track, "close" ) )
		return NULL;

	TrackClose( (DVD_TRACK_INFO*)track->pData );

	Py_INCREF( Py_None );
	return Py_None;
}

/* ---------------------------------------------------------------------------------*/
static PyObject *
DVDCD_Track_Seek( PyTrackObject *track, PyObject *args)
{
	// Assume SEEK_SET always in a time being
	INT64 iOffset;
	int iOrigin= SEEK_SET;
	if (!PyArg_ParseTuple(args, "L|i:seek", &iOffset, &iOrigin ))
		return NULL;

	// Calc offset in vobus and within
	DVD_TRACK_INFO* stTrack= (DVD_TRACK_INFO*)track->pData;
	vobu_admap_t *vobu_admap= stTrack->ifo_file->vts_vobu_admap;
	int iVobuCount= (vobu_admap->last_byte + 1 - VOBU_ADMAP_SIZE)/4; 
  int entries = (stTrack->ifo_file->vts_c_adt->last_byte + 1 - C_ADT_SIZE)/sizeof(c_adt_t); 

	// Start calc across all vobus( I know it is not always correct but... )
	stTrack->iVobuNo= stTrack->iVobuPos= 0;
	if( iOrigin== 2 )
	{
		INT64 iFSize= GetTrackSize( stTrack, 0 );
		iOffset= iFSize- iOffset;
	}
	// Start scanning vobus to find current position
	while( iOffset && stTrack->iVobuNo< stTrack->iVobuCount )
	{
		int iVobuNo1= stTrack->aiVobu[ stTrack->iVobuNo ];
		int iSects= ( ( iVobuNo1== iVobuCount- 1 ) ?
			stTrack->ifo_file->vts_c_adt->cell_adr_table[ entries- 1 ].last_sector:
			vobu_admap->vobu_start_sectors[ iVobuNo1+ 1 ] ) -
				vobu_admap->vobu_start_sectors[ iVobuNo1 ]- 1;
		if( iSects< 0 )
			return NULL;

		int iSize= iSects* DVD_VIDEO_LB_LEN;
		if( iSize<= iOffset )
		{
			iOffset-= iSize;
			stTrack->iVobuNo++;
		}
		else
		{
			stTrack->iVobuPos= (int)iOffset;
			iOffset= 0;
		}
	}

	// Check the last vobu by reading the nav header
	if( iOffset )
		if( iOrigin!= 2 )
		{
			PyErr_Format( g_cErr, "Trying to positioning beyond the file ( %d ).", (int)iOffset );
			stTrack->iVobuNo= stTrack->iVobuPos= 0;
			return NULL;
		}

	Py_INCREF( Py_None );
	return Py_None;
}

/* ---------------------------------------------------------------------------------*/
static PyObject *
DVDCD_Track_Tell( PyTrackObject *track )
{
	DVD_TRACK_INFO* stTrack= (DVD_TRACK_INFO*)track->pData;
	INT64 iFSize= GetTrackSize( stTrack, stTrack->iVobuNo );
	INT64 iFSize1= GetTrackSize( stTrack, 0 );
	return PyLong_FromLongLong( iFSize1- iFSize+ stTrack->iVobuPos );
}

/* ---------------------------------------------------------------------------------*/
// File like object to support contiques read from a track
static PyMethodDef dvdcd_track_methods[] = 
{
	{ 
		"read", 
		(PyCFunction)DVDCD_Track_Read, 
		METH_VARARGS,
	  "read( size ) -> data\n"
		"reads data from the track in a file like fashion"
	},
	{ 
		"close", 
		(PyCFunction)DVDCD_Track_Close, 
		METH_NOARGS,
	  "close() -> None\n"
		"Close currently open file. If already closed, does nothing"
	},
	{ 
		"seek", 
		(PyCFunction)DVDCD_Track_Seek, 
		METH_VARARGS,
	  "seek( pos, origin ) -> None\n"
		"Seek track into the certain position"
	},
	{ 
		"tell", 
		(PyCFunction)DVDCD_Track_Tell, 
		METH_NOARGS,
	  "tell() -> pos\n"
		"Returns current position in a file"
	},
	{ NULL, NULL },
};


/* ----------------------------------------------------------------*/
static void
DVDCD_TrackFree( PyTrackObject *track )
{
	TrackClose( (DVD_TRACK_INFO*)track->pData );

	if( track->pData )
		free( track->pData );
	PyObject_Free( track ); 
}

/* ---------------------------------------------------------------- */
static PyObject *
DVDCD_TrackGetAttr( PyTrackObject *track, char *name)
{
	return Py_FindMethod( dvdcd_track_methods, (PyObject *)track, name);
}

/* ----------------------------------------------------------------*/
PyTypeObject PyDVDCDTrackType = 
{
	PyObject_HEAD_INIT(NULL)
	0,
	"DVDTrack",
	sizeof(PyTrackObject),
	0,
	(destructor)DVDCD_TrackFree,  /*tp_dealloc*/
	0,			  //tp_print
	(getattrfunc)DVDCD_TrackGetAttr, /*tp_getattr*/
	0,			  //tp_setattr
	0,			  //tp_compare
	0,			  //tp_repr
	0,			  //tp_as_number
	0,			  //tp_as_sequence
	0,				//tp_as_mapping
};
