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
#include "afmt.h"

const int MAX_HEADERS= 20;
const int BUFFER_SIZE= 10000;

static void CALLBACK wave_callback(HWAVE hWave, UINT uMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2);

// *****************************************************************************************************
//	Sound stream main class
// *****************************************************************************************************
class ISoundStream
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
	ISoundStream( int rate, int channels, int format, int flags )
	{
		MMRESULT res;
		WAVEFORMATEX outFormatex;
		int i;

		memset( headers, 0, sizeof( WAVEHDR )* MAX_HEADERS );
		this->dev= NULL;

		SetLastError( 0 );
		InitializeCriticalSection(&this->cs);

		this->hSem= CreateSemaphore( NULL, MAX_HEADERS- 1, MAX_HEADERS, NULL );
		if( !this->hSem )
			return;

		// No error is set, just do it manually
		if(rate == -1)
		{
			SetLastError( -1 );
		  return;
		}

		// Last error should be set already
		if(!waveOutGetNumDevs())
			return;

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
			SetLastError( res );
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
			 return;
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
	~ISoundStream()
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
 	  EnterCriticalSection( &this->cs );
		ReleaseSemaphore( this->hSem, 1, &p );
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
	int Play(unsigned char *buf,int len)
	{
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
				return -1;

			EnterCriticalSection( &this->cs );
			this->iProcessed++;
			this->iBuffers++;
			LeaveCriticalSection( &this->cs );
		}
		return ( this->iBuffers* BUFFER_SIZE );
	}

	// ---------------------------------------------------------------------------------------------------
	double GetPosition()
	{
		MMTIME stTime;
		stTime.wType= TIME_MS; 
		waveOutGetPosition( this->dev, &stTime, sizeof( stTime ) );
		return ((double)stTime.u.ms)/ this->iRate;
	}

};


// ---------------------------------------------------------------------------------------------------
// Callback itself
static void CALLBACK wave_callback(HWAVE hWave, UINT uMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2)
{
	 ISoundStream *cStream= (ISoundStream *)dwInstance;
	 WAVEHDR *wh = (WAVEHDR *)dwParam1;
   if(uMsg == WOM_DONE )
		cStream->CompleteBuffer( wh );
}

#endif
