/*
 *			Class to support direct cdda extraction from any valid device
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

#ifndef CDDA_READ_H
#define CDDA_READ_H

#include <windows.h>
#include <winioctl.h>  // From the Win32 SDK \Mstools\Include
#include "ntddcdrm.h"  // From the Windows NT DDK \Ddk\Src\Storage\Inc

const int SECTORS_PER_CHUNK= 40;

int GetDrivesList( char sDrives[ 255 ][ 20 ] )
{
	int iDrives= 0;
	char drive[ 40 ], i;
	for ( i='A'; i<='Z'; ++i ) 
	{
		sprintf(drive, "%c:\\", i);
		if ( GetDriveType(drive) == DRIVE_CDROM ) 
		{
			strcpy( &sDrives[ iDrives ][ 0 ], drive );
			iDrives++;
		}
	}
	return iDrives;
}

class CDDARead
{
private:
  char sDevName[ 255 ];				// Device name
  char sOrigName[ 255 ];				// Device name
	CDROM_TOC stTracks;	// MAXIMUM_NUMBER_TRACKS tracks should be enough for the audio disk. But who knows ?
	HANDLE  hCD;
	DISK_GEOMETRY dgCDROM;
public:
	// ---------------------------------------------------
	~CDDARead()
	{
		if( this->hCD )
			CloseHandle( this->hCD );
	}
	// ---------------------------------------------------
	CDDARead( char* sDevName )
	{
		// Open the file 
		SetLastError(0);
		sprintf( this->sDevName, "\\\\.\\%s", sDevName );
		if( this->sDevName[ strlen( this->sDevName )- 1 ]== '\\' )
			this->sDevName[ strlen( this->sDevName )- 1 ]= 0;

		strcpy( this->sOrigName, sDevName );
		this->hCD = CreateFile( 
			this->sDevName, 
			GENERIC_READ, 
			FILE_SHARE_READ,
      NULL, 
			OPEN_EXISTING, 
			FILE_ATTRIBUTE_NORMAL,
      NULL);
	}
	// ---------------------------------------------------
	inline unsigned int GetTracksCount()	{ return this->stTracks.LastTrack- this->stTracks.FirstTrack+ 1; }
	// ---------------------------------------------------
	inline unsigned int GetStartSector( int iTrackNum )	
	{ 
		unsigned char* pAddr= &this->stTracks.TrackData[ iTrackNum ].Address[ 0 ];
		return pAddr[1]*60*75+pAddr[2]*75+pAddr[3];
	}
	// ---------------------------------------------------
	inline char* GetName()	{ return &this->sOrigName[ 0 ]; }
	// ---------------------------------------------------
	inline unsigned int GetSectorSize()	{ return 2352;  } //this->dgCDROM.BytesPerSector; }
	// ---------------------------------------------------
	int Eject()
	{
		// Reread the dist to see if something has changed
		DWORD dwNotUsed;
		if( this->hCD )
			if( !DeviceIoControl (this->hCD, IOCTL_STORAGE_EJECT_MEDIA, NULL, 0, NULL, 0, &dwNotUsed, NULL))
				return -1;
		return 0;
	}
	// ---------------------------------------------------
	int RefreshTOC( bool bReadToc )
	{
		// Reread the dist to see if something has changed
		DWORD dwNotUsed;
		if( this->hCD )
		{
			if( !DeviceIoControl (this->hCD, IOCTL_CDROM_GET_DRIVE_GEOMETRY,
                           NULL, 0, &this->dgCDROM, sizeof(this->dgCDROM),
                           &dwNotUsed, NULL))
				return -1;

			// Reread toc to be sure we have recent info
			if( bReadToc )
			{
				// Read the TOC
				memset(&this->stTracks, 0x00,sizeof( this->stTracks )); 
				DWORD dwBytesRead;
				BOOL bRet= DeviceIoControl( 
					this->hCD, IOCTL_CDROM_READ_TOC, NULL, 0, &this->stTracks, sizeof( this->stTracks ), &dwBytesRead, NULL );

				return bRet ? 0: -1;
			}
			return 0;
		}
		return -1;
	}
	// ---------------------------------------------------
	int ReadSectors( int iSectorFrom, int iSectorTo, char* sBuf, unsigned int iLen )
	{
		// Validate we have enough buffer to read all the data
		if( iLen< ( iSectorTo- iSectorFrom )* this->GetSectorSize() )
			return -1;

		// Read all sectors through the internal buffer in chunks
		for( int i= iSectorFrom; i< iSectorTo; i+= SECTORS_PER_CHUNK )
		{
			int iTo= ( i+ SECTORS_PER_CHUNK > iSectorTo ) ? iSectorTo: i+ SECTORS_PER_CHUNK;
			DWORD dwBytesRead= 0;

			// Prepare RAW_READ structure
			RAW_READ_INFO stRawInfo;
			memset( &stRawInfo, 0, sizeof( stRawInfo ) );
			stRawInfo.DiskOffset.QuadPart= i * this->dgCDROM.BytesPerSector;
			stRawInfo.TrackMode= CDDA;
  		stRawInfo.SectorCount= iTo- i;

			// Read the data into the internal buffer
		  BOOL bRet= DeviceIoControl( 
				this->hCD, IOCTL_CDROM_RAW_READ, &stRawInfo, sizeof(stRawInfo), sBuf, iLen, &dwBytesRead, NULL );
			if( !bRet )
				return -1;

			// If we read less data than we should, raise an error
			if( dwBytesRead!= this->GetSectorSize()* stRawInfo.SectorCount )
				return -1;

			// Copy data into the buffer provided
			sBuf+= dwBytesRead;
			iLen-= dwBytesRead;
		}
		return 0;
	}

};

#endif CDDA_READ_H