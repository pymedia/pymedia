/*
 *			Class to support direct audio playing
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

#ifndef SOUND_WIN32_H
#define SOUND_WIN32_H

#include <windows.h>
#include <mmsystem.h>
#include "afmt.h"

const int MAX_HEADERS= 20;
const int BUFFER_SIZE= 10000;

static void CALLBACK wave_callback(HWAVE hWave, UINT uMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2);
static void CALLBACK iwave_callback(HWAVE hWave, UINT uMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2);

// *****************************************************************************************************
// Input device enumerator
// *****************************************************************************************************
class InputDevices
{
private:
	UINT iDevs;
	char sName[ 512 ];
	char sErr[ 512 ];
	WAVEINCAPS pCaps;

	// ----------------------------------------------
	void FormatError()
	{
		LPVOID lpMsgBuf;
		FormatMessage(
				FORMAT_MESSAGE_ALLOCATE_BUFFER |
				FORMAT_MESSAGE_FROM_SYSTEM |
				FORMAT_MESSAGE_IGNORE_INSERTS,
				NULL,
				GetLastError(),
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
				(LPTSTR) &lpMsgBuf,
				0,
				NULL
		);
		strcpy( this->sErr, (char*)lpMsgBuf );
	}

	// ----------------------------------------------
	bool RefreshDevice( int i )
	{
		if( i>= (int)this->iDevs )
		{
			sprintf( this->sErr, "Device id ( %d ) is out of range ( 0-%d )", i, this->iDevs- 1 );
			return false;
		}

		if( waveInGetDevCaps( i, &this->pCaps, sizeof( this->pCaps ))!= MMSYSERR_NOERROR )
		{
			this->FormatError();
			return false;
		}
		return true;
	}

public:
	// ----------------------------------------------
	InputDevices()
	{
		this->iDevs= waveInGetNumDevs();
	}
	// ----------------------------------------------
	int Count(){ return this->iDevs; }
	// ----------------------------------------------
	char* GetName( int i )
	{
		if( !this->RefreshDevice( i ) )
			return NULL;

		strcpy( this->sName, this->pCaps.szPname );
		return this->sName;
	}
	// ----------------------------------------------
	char* GetMID( int i )
	{
		if( !this->RefreshDevice( i ) )
			return NULL;

		sprintf( this->sName, "%x", this->pCaps.wMid );
		return this->sName;
	}
	// ----------------------------------------------
	char* GetPID( int i )
	{
		if( !this->RefreshDevice( i ) )
			return NULL;

		sprintf( this->sName, "%x", this->pCaps.wPid );
		return this->sName;
	}
	// ----------------------------------------------
	int GetChannels( int i )
	{
		if( !this->RefreshDevice( i ) )
			return -1;

		return this->pCaps.wChannels;
	}
	// ----------------------------------------------
	int GetFormats( int i )
	{
		if( !this->RefreshDevice( i ) )
			return -1;

		return this->pCaps.dwFormats;
	}
};

// *****************************************************************************************************
// Output device enumerator
// *****************************************************************************************************
class OutputDevices
{
private:
	UINT iDevs;
	char sName[ 512 ];
	char sErr[ 512 ];
	WAVEOUTCAPS pCaps;

	// ----------------------------------------------
	void FormatError()
	{
		LPVOID lpMsgBuf;
		FormatMessage(
				FORMAT_MESSAGE_ALLOCATE_BUFFER |
				FORMAT_MESSAGE_FROM_SYSTEM |
				FORMAT_MESSAGE_IGNORE_INSERTS,
				NULL,
				GetLastError(),
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
				(LPTSTR) &lpMsgBuf,
				0,
				NULL
		);
		strcpy( this->sErr, (char*)lpMsgBuf );
	}

	// ----------------------------------------------
	bool RefreshDevice( int i )
	{
		if( i>= (int)this->iDevs )
		{
			sprintf( this->sErr, "Device id ( %d ) is out of range ( 0-%d )", i, this->iDevs- 1 );
			return false;
		}

		if( waveOutGetDevCaps( i, &this->pCaps, sizeof( this->pCaps ))!= MMSYSERR_NOERROR )
		{
			this->FormatError();
			return false;
		}
		return true;
	}

public:
	// ----------------------------------------------
	OutputDevices()
	{
		this->iDevs= waveOutGetNumDevs();
	}
	// ----------------------------------------------
	int Count(){ return this->iDevs; }
	// ----------------------------------------------
	char* GetName( int i )
	{
		if( !this->RefreshDevice( i ) )
			return NULL;

		strcpy( this->sName, this->pCaps.szPname );
		return this->sName;
	}
	// ----------------------------------------------
	char* GetMID( int i )
	{
		if( !this->RefreshDevice( i ) )
			return NULL;

		sprintf( this->sName, "%x", this->pCaps.wMid );
		return this->sName;
	}
	// ----------------------------------------------
	char* GetPID( int i )
	{
		if( !this->RefreshDevice( i ) )
			return NULL;

		sprintf( this->sName, "%x", this->pCaps.wPid );
		return this->sName;
	}
	// ----------------------------------------------
	int GetChannels( int i )
	{
		if( !this->RefreshDevice( i ) )
			return -1;

		return this->pCaps.wChannels;
	}
	// ----------------------------------------------
	int GetFormats( int i )
	{
		if( !this->RefreshDevice( i ) )
			return -1;

		return this->pCaps.dwFormats;
	}
};

// *****************************************************************************************************
// Mixer line
// *****************************************************************************************************
/*
class MixerLine
{
private:
	HMIXER mixer;
	MIXERLINE line;

public:
	// ----------------------------------------------
	MixerLine( HMIXER mixer, int iLine )
	{
		this->mixer= mixer;
		line.cbStruct= sizeof( MIXERLINE );
		line.dwDestination= j;

		if( mixerGetLineInfo( (HMIXEROBJ)mixer, &line, MIXER_GETLINEINFOF_DESTINATION  )== MMSYSERR_NOERROR )
		{
			// create enough structures for line control
			MIXERCONTROL *p= (MIXERCONTROL*)malloc( sizeof( MIXERCONTROL )* line.cControls );
			MIXERLINECONTROLS controls;
			controls.cbStruct= sizeof( MIXERLINECONTROLS );
			controls.dwLineID= line.dwLineID;
			controls.cControls= line.cControls;
			controls.cbmxctrl= sizeof( MIXERCONTROL );
			controls.pamxctrl= p;
			printf("Destination #%lu = %s\n", i, line.szName);
			int numSrc= line.cConnections;
			for( int k= 0; k< numSrc; k++ )
			{
				line.cbStruct= sizeof(MIXERLINE);
				line.dwDestination= j;
				line.dwSource= k;
				if( mixerGetLineInfo( (HMIXEROBJ)mixer, &line, MIXER_GETLINEINFOF_SOURCE )== MMSYSERR_NOERROR )
				{
	}

	// ----------------------------------------------
	GetControlsCount()
	{
	}

};

// *****************************************************************************************************
// Mixer devices enumerator
// *****************************************************************************************************
class MixerDevices
{
private:
	UINT iDevs;
	char sName[ 512 ];
	char sErr[ 512 ];
	WAVEOUTCAPS pCaps;

	// ----------------------------------------------
	void FormatError()
	{
		LPVOID lpMsgBuf;
		FormatMessage(
				FORMAT_MESSAGE_ALLOCATE_BUFFER |
				FORMAT_MESSAGE_FROM_SYSTEM |
				FORMAT_MESSAGE_IGNORE_INSERTS,
				NULL,
				GetLastError(),
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
				(LPTSTR) &lpMsgBuf,
				0,
				NULL
		);
		strcpy( this->sErr, (char*)lpMsgBuf );
	}

	// ----------------------------------------------
	bool RefreshDevice( int i )
	{
		HMIXEROBJ tmp_mixer;
		if( mixerOpen( &mixer, i, 0, NULL, MIXER_OBJECTF_MIXER )== MMSYSERR_NOERROR )
		{
			MIXERCAPS  mixcaps;
			if( mixerGetDevCaps((UINT)mixer, &mixcaps, sizeof(MIXERCAPS))== MMSYSERR_NOERROR )
			{
				for( int j= 0; j< mixcaps.cDestinations; j++ )
				{

					MIXERLINE line;
					line.cbStruct= sizeof( MIXERLINE );
					line.dwDestination= j;

					if( mixerGetLineInfo( (HMIXEROBJ)mixer, &line, MIXER_GETLINEINFOF_DESTINATION  )== MMSYSERR_NOERROR )
					{
						// create enough structures for line control
						MIXERCONTROL *p= (MIXERCONTROL*)malloc( sizeof( MIXERCONTROL )* line.cControls );
						MIXERLINECONTROLS controls;
						controls.cbStruct= sizeof( MIXERLINECONTROLS );
						controls.dwLineID= line.dwLineID;
						controls.cControls= line.cControls;
						controls.cbmxctrl= sizeof( MIXERCONTROL );
						controls.pamxctrl= p;
						printf("Destination #%lu = %s\n", i, line.szName);
						int numSrc= line.cConnections;
						for( int k= 0; k< numSrc; k++ )
						{
							line.cbStruct= sizeof(MIXERLINE);
							line.dwDestination= j;
							line.dwSource= k;
							if( mixerGetLineInfo( (HMIXEROBJ)mixer, &line, MIXER_GETLINEINFOF_SOURCE )== MMSYSERR_NOERROR )
							{
								printf("\tSource #%lu = %s, has %d controls\n", i, line.szName, line.cControls );
								// Get line controls if possible
									MIXERCONTROL       mixerControlArray[10];
									MIXERLINECONTROLS  mixerLineControls;
									mixerLineControls.cbStruct = sizeof(MIXERLINECONTROLS);
									mixerLineControls.cControls = line.cControls;
									mixerLineControls.dwLineID = line.dwLineID;
									mixerLineControls.pamxctrl = &mixerControlArray[0];
									mixerLineControls.cbmxctrl = sizeof(MIXERCONTROL);
									if( mixerGetLineControls((HMIXEROBJ)mixer, &mixerLineControls, MIXER_GETLINECONTROLSF_ALL)== MMSYSERR_NOERROR )
									{
										for( int l= 0; l< line.cControls; l++ )
										{
											printf( "\t\t%s=", mixerControlArray[l].szName );
											// Get the value for the control
											MIXERCONTROLDETAILS_UNSIGNED value[ 20 ];
											MIXERCONTROLDETAILS          mixerControlDetails;

											mixerControlDetails.cbStruct = sizeof(MIXERCONTROLDETAILS);
											mixerControlDetails.dwControlID = mixerControlArray[l].dwControlID;
											mixerControlDetails.cChannels = line.cChannels;
											mixerControlDetails.cMultipleItems = 0;
											mixerControlDetails.paDetails = &value;
											mixerControlDetails.cbDetails = sizeof(MIXERCONTROLDETAILS_UNSIGNED);
											if ( mixerGetControlDetails((HMIXEROBJ)mixer, &mixerControlDetails, MIXER_GETCONTROLDETAILSF_VALUE)== MMSYSERR_NOERROR )
												printf( "%d\n", value[ 0 ].dwValue );
											else
												printf( "undefined\n" );

										}
									}
							}
						}
					}
				}
			}
		}

	}

public:
	// ----------------------------------------------
	MixerDevices()
	{
		this->iDevs= waveOutGetNumDevs();
	}
	// ----------------------------------------------
	int Count(){ return this->iDevs; }
	// ----------------------------------------------
	char* GetName( int i )
	{
		if( !this->RefreshDevice( i ) )
			return NULL;

		strcpy( this->sName, this->pCaps.szPname );
		return this->sName;
	}
	// ----------------------------------------------
	char* GetMID( int i )
	{
		if( !this->RefreshDevice( i ) )
			return NULL;

		sprintf( this->sName, "%x", this->pCaps.wMid );
		return this->sName;
	}
	// ----------------------------------------------
	char* GetPID( int i )
	{
		if( !this->RefreshDevice( i ) )
			return NULL;

		sprintf( this->sName, "%x", this->pCaps.wPid );
		return this->sName;
	}
	// ----------------------------------------------
	int GetLines( int i )
	{
		if( !this->RefreshDevice( i ) )
			return -1;

		return this->pCaps.wChannels;
	}
	// ----------------------------------------------
	int GetControls( int i )
	{
		if( !this->RefreshDevice( i ) )
			return -1;

		return this->pCaps.dwFormats;
	}
};

*/

// *****************************************************************************************************
//	Sound stream main class
// *****************************************************************************************************
class OSoundStream
{
private:
	CRITICAL_SECTION cs;
	bool bStopFlag;
	// Buffers to be freed by the stop op
	WAVEHDR headers[ MAX_HEADERS ];
	HWAVEOUT dev;
	HANDLE hSem;
	char sBuf[ BUFFER_SIZE ];
	int format;
	int iProcessed;
	int iChannels;
	int iMute;
	int iRate;
	int iBuffers;
	int iErr;

	// ---------------------------------------------------------------------------------------------------
	// Function called by callback
	void free_buffer( WAVEHDR *wh )
	{
		waveOutUnprepareHeader(this->dev, wh, sizeof (WAVEHDR));
		//Deallocate the buffer memory
		if( wh->lpData )
			free( wh->lpData );

		wh->lpData= NULL;
	}

public:
	// ---------------------------------------------------------------------------------------------------
	OSoundStream( int rate, int channels, int format, int flags, int ii )
	{
		MMRESULT res;
		WAVEFORMATEX outFormatex;
		int i;

		memset( headers, 0, sizeof( WAVEHDR )* MAX_HEADERS );
		this->dev= NULL;

		this->iErr= 0;
		InitializeCriticalSection(&this->cs);

		this->hSem= CreateSemaphore( NULL, MAX_HEADERS- 1, MAX_HEADERS, NULL );
		if( !this->hSem )
		{
			this->iErr= ::GetLastError();
			return;
		}

		// No error is set, just do it manually
		if(rate == -1)
		{
			this->iErr= ::GetLastError();
		  return;
		}

		// Last error should be set already
		if(!waveOutGetNumDevs())
		{
			this->iErr= ::GetLastError();
			return;
		}

		this->iRate= rate;
		this->format= format;
		this->iBuffers= this->iProcessed= this->iMute= 0;
		this->bStopFlag= false;

		outFormatex.wFormatTag =
			( format!= AFMT_MPEG && format != AFMT_AC3 ) ? WAVE_FORMAT_PCM: -1;
		outFormatex.wBitsPerSample  =
			( format== AFMT_U8 || format== AFMT_S8 ) ? 8:
			( format== AFMT_S16_LE || format== AFMT_S16_BE || format== AFMT_U16_LE || format== AFMT_U16_BE  ) ? 16:
			( format== AFMT_AC3 ) ? 6: 0;
		this->iChannels= channels;
		outFormatex.nChannels       = channels;
		outFormatex.nSamplesPerSec  = rate;
		outFormatex.nAvgBytesPerSec = outFormatex.nSamplesPerSec * outFormatex.nChannels * outFormatex.wBitsPerSample/8;
		outFormatex.nBlockAlign     = outFormatex.nChannels * outFormatex.wBitsPerSample/8;
		res = waveOutOpen( &this->dev, 0, &outFormatex, (DWORD)wave_callback, (DWORD)this, CALLBACK_FUNCTION);
		if(res != MMSYSERR_NOERROR)
		{
			this->iErr= res;
		  return;
		}

		for( i= 0; i< MAX_HEADERS; i++ )
		{
			// Initialize buffers
			int res;
			LPWAVEHDR wh = &this->headers[ i ];
			void* p= malloc( BUFFER_SIZE );
			if( !p )
				return;
			wh->dwBufferLength = BUFFER_SIZE;
			wh->lpData = (char*)p;
			wh->dwFlags= 0;
			res = waveOutPrepareHeader( this->dev, wh, sizeof (WAVEHDR) );
			if(res)
			{
				this->iErr= ::GetLastError();
			  return;
		  }
		}
		/*
		switch(res)
		{
			case -1:
				sprintf( s, "Cannot initialize audio device" );
				break;
			case MMSYSERR_ALLOCATED:
				sprintf( s, "Device Is Already Open" );
				break;
			case MMSYSERR_BADDEVICEID:
				sprintf( s, "The Specified Device Is out of range");
				break;
			case MMSYSERR_NODRIVER:
				sprintf( s, "There is no audio driver in this system.");
			break;
			case MMSYSERR_NOMEM:
				sprintf( s, "Unable to allocate sound memory.");
			break;
			case WAVERR_BADFORMAT:
				sprintf( s, "This audio format is not supported.");
			break;
			case WAVERR_SYNC:
				sprintf( s, "The device is synchronous.");
			break;
			default:
				sprintf( s, "Unknown Media Error" );
			break;
		}
		*/

		waveOutReset( this->dev );
	}

	// ---------------------------------------------------------------------------------------------------
	~OSoundStream()
	{
		this->Stop();
		if( this->hSem )
			CloseHandle( this->hSem );

		if(this->dev)
		{
			 waveOutReset(this->dev);      //reset the device
			 waveOutClose(this->dev);      //close the device
			 this->dev=NULL;
		}
		for( int i= 0; i< MAX_HEADERS; i++ )
			this->free_buffer( &this->headers[ i ] );

		DeleteCriticalSection(&this->cs);
	}

	// ---------------------------------------------------------------------------------------------------
	int GetLastError(){ return ::GetLastError();	}
	// ---------------------------------------------------------------------------------------------------
	char* GetErrorString(){ return "";	}
	// ---------------------------------------------------------------------------------------------------
	int GetBuffersCount(){ return MAX_HEADERS;	}
	// ---------------------------------------------------------------------------------------------------
	int GetRate(){ return this->iRate;	}
	// ---------------------------------------------------------------------------------------------------
	int GetChannels(){ return this->iChannels;	}
	// ---------------------------------------------------------------------------------------------------
	int IsMute(){ return this->iMute;	}
	// ---------------------------------------------------------------------------------------------------
	void SetMute( bool bMute )
	{
		if( bMute )
		{
			this->iMute= this->GetVolume();
			this->SetVolume( 0 );
		}
		else
			if( this->IsMute() )
			{
				this->SetVolume( this->iMute );
				this->iMute= 0;
			}
	}
	// ---------------------------------------------------------------------------------------------------
	void CompleteBuffer( WAVEHDR *wh )
	{
		LONG p;
		ReleaseSemaphore( this->hSem, 1, &p );
 	  EnterCriticalSection( &this->cs );
		this->iBuffers--;
	  LeaveCriticalSection( &this->cs );
	}
	// ---------------------------------------------------------------------------------------------------
	int Pause()	{	 return ( waveOutPause( this->dev ) ) ? -1: 0; }

	// ---------------------------------------------------------------------------------------------------
	int GetVolume()
	{
		 DWORD dwVolume;
		 return ( waveOutGetVolume( this->dev, &dwVolume ) ) ? (-1): (int)dwVolume;
	}

	// ---------------------------------------------------------------------------------------------------
	int SetVolume(int iVolume ) { return (waveOutSetVolume( this->dev, iVolume )) ? (-1): 0; }

	// ---------------------------------------------------------------------------------------------------
	int Stop()
	{
		this->bStopFlag= true;
		if( this->dev )
			waveOutReset( this->dev );

		return 0;
	}

	// ---------------------------------------------------------------------------------------------------
	int Unpause() { return (waveOutRestart( this->dev )) ? (-1): 0; }

	// ---------------------------------------------------------------------------------------------------
	int IsPlaying() { return this->iBuffers!= 0; }

	// ---------------------------------------------------------------------------------------------------
	float Play(unsigned char *buf,int len)
	{
		double dPos= this->GetPosition();
		// Try to fit chunk in remaining buffers
		while( len> 0 )
		{
			WaitForSingleObject( this->hSem, INFINITE );
			int i= this->iProcessed % MAX_HEADERS;
			int l= ( len> BUFFER_SIZE ) ? BUFFER_SIZE: len;
			memcpy( this->headers[ i ].lpData, buf, l );
			this->headers[ i ].dwBufferLength = l;
			buf+= l;
			len-= l;
		  if( waveOutWrite( this->dev, &this->headers[ i ], sizeof (WAVEHDR) ) )
		  {
				this->iErr= ::GetLastError();
				return -1;
			}

			dPos+= l / ((double)( 2* this->iChannels* this->iRate ));

			EnterCriticalSection( &this->cs );
			this->iProcessed++;
			this->iBuffers++;
			LeaveCriticalSection( &this->cs );
		}
		return (float)( dPos- this->GetPosition());
	}

	// ---------------------------------------------------------------------------------------------------
	double GetPosition()
	{
		MMTIME stTime;
		stTime.wType= TIME_MS;
		waveOutGetPosition( this->dev, &stTime, sizeof( stTime ) );
		return ((double)stTime.u.ms)/ this->iRate;
	}
	// ---------------------------------------------------------------------------------------------------
	float GetLeft()
	{
		return ((float)this->iBuffers* BUFFER_SIZE )/ ((float)( 2* this->iChannels* this->iRate ));
	}

};


// *****************************************************************************************************
//	Input sound stream main class
// *****************************************************************************************************
class ISoundStream
{
private:
	CRITICAL_SECTION cs;
	bool bStopFlag;
	// Buffers to be freed by the stop op
	WAVEHDR headers[ MAX_HEADERS ];
	HWAVEIN dev;
	HANDLE hSem;
	int format;
	int iProcessed;
	int iChannels;
	int iRate;
	int iBuffers;
	int iErr;

	// ---------------------------------------------------------------------------------------------------
	// Function called by callback
	void free_buffer( WAVEHDR *wh )
	{
		waveInUnprepareHeader(this->dev, wh, sizeof (WAVEHDR));
		//Deallocate the buffer memory
		if( wh->lpData )
			free( wh->lpData );

		wh->lpData= NULL;
	}

public:
	// ---------------------------------------------------------------------------------------------------
	ISoundStream( int rate, int channels, int format, int flags, int iId )
	{
		MMRESULT res;
		WAVEFORMATEX inFormatex;
		int i;

		memset( headers, 0, sizeof( WAVEHDR )* MAX_HEADERS );
		this->dev= NULL;

		this->iErr= 0;
		InitializeCriticalSection(&this->cs);

		this->hSem= CreateSemaphore( NULL, MAX_HEADERS- 1, MAX_HEADERS, NULL );
		if( !this->hSem )
			return;

		// No error is set, just do it manually
		if(rate == -1)
		{
			this->iErr= ::GetLastError();
		  return;
		}

		// Last error should be set already
		if((int)waveInGetNumDevs()<= iId)
		{
			this->iErr= ::GetLastError();
			return;
		}

		this->iRate= rate;
		this->format= format;
		this->iBuffers= this->iProcessed= 0;
		this->bStopFlag= true;

		inFormatex.wFormatTag = WAVE_FORMAT_PCM;
		inFormatex.wBitsPerSample = ( format== AFMT_U8 || format== AFMT_S8 ) ? 8: 16;
		this->iChannels= channels;
		inFormatex.nChannels       = channels;
		inFormatex.nSamplesPerSec  = rate;
		inFormatex.nAvgBytesPerSec = inFormatex.nSamplesPerSec * inFormatex.nChannels * inFormatex.wBitsPerSample/8;
		inFormatex.nBlockAlign     = inFormatex.nChannels * inFormatex.wBitsPerSample/8;
		res = waveInOpen( &this->dev, iId, &inFormatex, (DWORD)iwave_callback, (DWORD)this, CALLBACK_FUNCTION);
		if(res != MMSYSERR_NOERROR)
		{
			this->iErr= res;
		  return;
		}

		for( i= 0; i< MAX_HEADERS; i++ )
		{
			// Initialize buffers
			int res;
			LPWAVEHDR wh = &this->headers[ i ];
			void* p= malloc( BUFFER_SIZE );
			if( !p )
				return;
			wh->dwBufferLength = BUFFER_SIZE;
			wh->lpData = (char*)p;
			wh->dwFlags= 0;
			res = waveInPrepareHeader( this->dev, wh, sizeof (WAVEHDR) );
			if(res)
			{
				this->iErr= ::GetLastError();
				return;
		  }
		}
		waveInReset( this->dev );
	}

	// ---------------------------------------------------------------------------------------------------
	~ISoundStream()
	{
		this->Stop();
		if( this->hSem )
			CloseHandle( this->hSem );

		if(this->dev)
		{
			 waveInReset(this->dev);      //reset the device
			 waveInClose(this->dev);      //close the device
			 this->dev=NULL;
		}
		for( int i= 0; i< MAX_HEADERS; i++ )
			this->free_buffer( &this->headers[ i ] );

		DeleteCriticalSection(&this->cs);
	}

	// ---------------------------------------------------------------------------------------------------
	int GetLastError(){ return this->iErr;	}
	// ---------------------------------------------------------------------------------------------------
	char* GetErrorString(){ return "";	}
	// ---------------------------------------------------------------------------------------------------
	int GetRate(){ return this->iRate;	}
	// ---------------------------------------------------------------------------------------------------
	int GetChannels(){ return this->iChannels;	}
	// ---------------------------------------------------------------------------------------------------
	void CompleteBuffer( WAVEHDR *wh )
	{
		// Submit buffer after it's completed
		if( wh )
		{
 			EnterCriticalSection( &this->cs );
			this->iBuffers++;
			this->iProcessed++;
			LeaveCriticalSection( &this->cs );
		}

		if( !this->bStopFlag )
		{
			if( WaitForSingleObject( this->hSem, 0 )== WAIT_TIMEOUT )
				// There is no buffers we can put data into, just bail out
				return;

			// Submit new buffer for playing if any
			int i= this->iProcessed % MAX_HEADERS;
			waveInAddBuffer( this->dev, &this->headers[ i ], sizeof(WAVEHDR) );
		}
	}
	// ---------------------------------------------------------------------------------------------------
	int Stop()
	{
		this->bStopFlag= true;
		if( this->dev )
			waveInReset( this->dev );

		return 0;
	}

	// ---------------------------------------------------------------------------------------------------
	bool Start()
	{
		if( this->bStopFlag )
		{
			this->bStopFlag= false;
			this->iBuffers= 0;

			if( waveInStart( this->dev )!= MMSYSERR_NOERROR )
			{
				this->iErr= ::GetLastError();
				return false;
			}

			// Submit buffer for grabbing
			this->CompleteBuffer( NULL );
		}
		return true;
	}

	// ---------------------------------------------------------------------------------------------------
	double GetPosition()
	{
		MMTIME stTime;
		stTime.wType= TIME_MS;
		waveInGetPosition( this->dev, &stTime, sizeof( stTime ) );
		return ((double)stTime.u.ms)/ this->iRate;
	}
	// ---------------------------------------------------------------------------------------------------
	// Get size of the data already in the buffers
	int GetSize()
	{
		return this->iBuffers* BUFFER_SIZE;
	}
	// ---------------------------------------------------------------------------------------------------
	// Return data from the buffers
	int GetData( char* pData, int iSize )
	{
		// Start looping through all buffers until we get the iSize bytes.
		// Note that we cannot get partial buffer, so iSize always mutliplies by BUFFER_SIZE
		int iBufs= 0;
		int i= ( this->iProcessed- this->iBuffers ) % MAX_HEADERS;
		while( iSize>= BUFFER_SIZE && this->iBuffers )
		{
			LONG p;
			memcpy( pData, this->headers[ i ].lpData, BUFFER_SIZE );
			i= ( i+ 1 ) % MAX_HEADERS;
			pData+= BUFFER_SIZE;
			iSize-= BUFFER_SIZE;
			iBufs++;

			ReleaseSemaphore( this->hSem, 1, &p );
 			EnterCriticalSection( &this->cs );
			this->iBuffers--;
			LeaveCriticalSection( &this->cs );
		}
		return iBufs* BUFFER_SIZE;
	}
};

// ---------------------------------------------------------------------------------------------------
// Callback for input
static void CALLBACK iwave_callback(HWAVE hWave, UINT uMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2)
{
	ISoundStream *cStream= (ISoundStream *)dwInstance;
	WAVEHDR *wh = (WAVEHDR *)dwParam1;
  if(uMsg == WIM_DATA )
		cStream->CompleteBuffer( wh );
}

// ---------------------------------------------------------------------------------------------------
// Callback itself
static void CALLBACK wave_callback(HWAVE hWave, UINT uMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2)
{
	 OSoundStream *cStream= (OSoundStream *)dwInstance;
	 WAVEHDR *wh = (WAVEHDR *)dwParam1;
   if(uMsg == WOM_DONE )
		cStream->CompleteBuffer( wh );
}

#endif
