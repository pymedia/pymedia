/*
 *			Class to support direct audio playing through OSS
 *
 *		Code is selectively taken form the libao2.oss / MPlayer
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

#ifndef SOUND_UNIX_H
#define SOUND_UNIX_H

#include <sys/ioctl.h>
#include <sys/soundcard.h>
//#include <pthread.h>
#include <fcntl.h>
//#include <string.h>

#include "afmt.h"

#define MAX_DEVICES 4
#define READ_BUFFER_SIZE 4096

// -------------------------------------------------------------------
// Vars and constants
struct BUFFER_CHUNK
{
	unsigned char* sBuf;
	int iLen;
	int iRead;
};

// -------------------------------------------------------------------
static char *dsp=PATH_DEV_DSP;
char *oss_mixer_device = PATH_DEV_MIXER;
const int MAX_INT_BUFFERS= 20;
const int OPEN_FLAGS= (O_WRONLY|O_NONBLOCK);

// -------------------------------------------------------------------
int GetDevicesCount()
{
	int audio_fd, i= 0;
	char audiopath[1024];

	/* Figure out what our audio device is */
	audio_fd = open( dsp, OPEN_FLAGS, 0);
	if( audio_fd> 0 )
	{
		i++;
		close( audio_fd );
	}

	/* If the first open fails, look for other devices */
	while( i< MAX_DEVICES )
	{
		struct stat sb;

		/* Don't use errno ENOENT - it may not be thread-safe */
		sprintf(audiopath, "%s%d", dsp, i);
		if ( stat(audiopath, &sb) == 0 )
			i++;
	}
	return i;
}

// *****************************************************************************************************
// Input device enumerator
// *****************************************************************************************************
class InputDevices
{
private:
	int iDevs;
	char sName[ 512 ];
	char sErr[ 512 ];

	// ----------------------------------------------
	bool RefreshDevice( int i )
	{
		if( i>= (int)this->iDevs )
		{
			sprintf( this->sErr, "Device id ( %d ) is out of range ( 0-%d )", i, this->iDevs- 1 );
			return false;
		}

		// No way to return textual descriptions in the time being
		strcpy( this->sName, "Device unknown" );
		return true;
	}

public:
	// ----------------------------------------------
	InputDevices()
	{
		this->iDevs= GetDevicesCount();
	}
	// ----------------------------------------------
	int Count(){ return this->iDevs; }
	// ----------------------------------------------
	char* GetName( int i )
	{
		if( !this->RefreshDevice( i ) )
			return NULL;

		return this->sName;
	}
	// ----------------------------------------------
	char* GetMID( int i )
	{
		if( !this->RefreshDevice( i ) )
			return NULL;

		sprintf( this->sName, "%x", 0 );
		return this->sName;
	}
	// ----------------------------------------------
	char* GetPID( int i )
	{
		if( !this->RefreshDevice( i ) )
			return NULL;

		sprintf( this->sName, "%x", 0 );
		return this->sName;
	}
	// ----------------------------------------------
	int GetChannels( int i )
	{
		if( !this->RefreshDevice( i ) )
			return -1;

		return -1;
	}
	// ----------------------------------------------
	int GetFormats( int i )
	{
		if( !this->RefreshDevice( i ) )
			return -1;

		return 0;
	}
};

// No difference between input and output...
typedef InputDevices OutputDevices;

// *****************************************************************************************************
//	Sound stream main class
// *****************************************************************************************************
class OSoundStream
{
private:
	bool bStopFlag;
	// Buffers to be freed by the stop op
	int dev;
	int format;
	int channels;
	int rate;
	int bps;
	int iMute;
	int bufsize;
	long long bytesPlayed;
	int iErr;
	char sErr[ 512 ];

	// ---------------------------------------------------------------------------------------------------
	// close audio device
	void Uninit()
	{
		if( this->dev == -1 )
			return;

		ioctl( this->dev, SNDCTL_DSP_RESET, NULL);
		close( this->dev );
		this->dev = -1;
	}

	// ---------------------------------------------------------------------------------------------------
	int Init( int rate, int channels, int format )
	{
		this->dev= open( dsp, O_WRONLY );
		if( this->dev< 0 )
		{
			this->iErr= errno;
			sprintf( &this->sErr[ 0 ], "%s at %s", strerror( errno ), "OPEN");
			return -1;
		}

    if (fcntl(dev, F_SETFL, 0) == -1)
		{
			this->iErr= errno;
			sprintf( &this->sErr[ 0 ], "%s at %s", strerror( errno ), "SET_BlOCK");
			return -1;
		}

		this->format=format;
		if( ioctl( this->dev, SNDCTL_DSP_SETFMT, &this->format)<0 || this->format != format)
			if( format == AFMT_AC3)
				return 0;

		this->channels = channels;
		if( format != AFMT_AC3 )
		{
			// We only use SNDCTL_DSP_CHANNELS for >2 channels, in case some drivers don't have it
			if( this->channels > 2 )
				if( ioctl( this->dev, SNDCTL_DSP_CHANNELS, &this->channels) == -1 || this->channels != channels )
				{
					this->iErr= errno;
					sprintf( &this->sErr[ 0 ], "%s at %s", strerror( errno ), "SET_CHANNELS");
					return -1;
				}
				else;
			else
			{
				int c = this->channels-1;
				if (ioctl( this->dev, SNDCTL_DSP_STEREO, &c) == -1)
				{
					this->iErr= errno;
					sprintf( &this->sErr[ 0 ], "%s at %s", strerror( errno ), "SET_STEREO");
					return -1;
				}

				this->channels= c+1;
			}
		}

		// set rate
		this->rate= rate;
		this->bytesPlayed= 0;
		ioctl( this->dev, SNDCTL_DSP_SPEED, &this->rate);
		return 0;
	}

public:
	// ---------------------------------------------------------------------------------------------------
	OSoundStream( int rate, int channels, int format, int flags, int iDev )
	{
		this->sErr[ 0 ]= 0;
		this->iErr= 0;
		if( this->Init( rate, channels, format )== 0 )
			this->bufsize= this->GetSpace();
	}

	// ---------------------------------------------------------------------------------------------------
	~OSoundStream()
	{
		this->Stop();
	}

	// ---------------------------------------------------------------------------------------------------
	char* GetErrorString()	{ return( this->sErr );	}
	// ---------------------------------------------------------------------------------------------------
	int GetLastError()	{ return( this->iErr );	}
	// ---------------------------------------------------------------------------------------------------
	int GetDevice(){ return this->dev;	}
	// ---------------------------------------------------------------------------------------------------
	int GetAudioBufSize(){ return this->bufsize;	}
	// ---------------------------------------------------------------------------------------------------
	// return: how many bytes can be played without blocking
	int GetSpace()
	{
		audio_buf_info zz;
		if( ioctl( this->dev , SNDCTL_DSP_GETOSPACE, &zz)== -1 )
			return -1;

		return zz.fragments*zz.fragsize;
	}
	// ---------------------------------------------------------------------------------------------------
	int GetRate(){ return this->rate;	}
	// ---------------------------------------------------------------------------------------------------
	int GetChannels(){ return this->channels;	}
	// ---------------------------------------------------------------------------------------------------
	// return: delay in seconds between first and last sample in buffer
	float IsPlaying()
	{
		int r=0;
		if(ioctl( this->GetDevice(), SNDCTL_DSP_GETODELAY, &r)!=-1)
			 return (float)r;

		return -1;
	}
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
	int GetVolume()
	{
		int fd, devs, v= 0;
		if( (fd = open(oss_mixer_device, O_RDONLY)) < 0)
			return -1;

		ioctl( fd, SOUND_MIXER_READ_DEVMASK, &devs );
		if( devs & SOUND_MASK_PCM )
			ioctl(fd, SOUND_MIXER_READ_PCM, &v);
		close( fd );
		return v;
	}

	// ---------------------------------------------------------------------------------------------------
	int SetVolume(int iVolume )
	{
		int fd, devs;
		if( (fd = open(oss_mixer_device, O_RDONLY)) < 0)
			return -1;

		ioctl( fd, SOUND_MIXER_READ_DEVMASK, &devs );
		if( devs & SOUND_MASK_PCM )
			ioctl( fd, SOUND_MIXER_WRITE_PCM, &iVolume);

		close( fd );
		return 0;
	}

	// ---------------------------------------------------------------------------------------------------
	int Pause()
	{
		ioctl( this->dev, SNDCTL_DSP_POST, NULL);
		return 0;
	}

	// ---------------------------------------------------------------------------------------------------
	int Unpause()
	{
		this->Play( NULL, 0 );
		return 0;
	}

	// ---------------------------------------------------------------------------------------------------
	float GetLeft()
	{
		return (float)( this->bufsize- this->GetSpace()- this->IsPlaying() )/ ((double)( 2* this->channels* this->rate ));
	}

	// ---------------------------------------------------------------------------------------------------
	int Stop()
	{
		this->Uninit();
		return 0;
	}

	// ---------------------------------------------------------------------------------------------------
	float Play( unsigned char *buf, int iLen )
	{
		if( this->GetDevice()== -1 )
			this->Init( this->rate, this->channels, this->format );

		// See how many bytes playing right now and add them to the pos
		while( iLen )
		{
			int iAvail= this->GetSpace();
			if( iAvail== 0 )
			{
				usleep( 5 );
				continue;
			}
			int i= write( this->GetDevice(), buf, ( iAvail> iLen ) ? iLen: iAvail );
			if( i< 0 )
			{
				if( GetLastError()== EAGAIN )
					// Some disconnect between GetOSpace and write...
					continue;

				strcpy( &this->sErr[ 0 ], strerror( errno ));
				return i;
			}
			this->bytesPlayed+= i;
			buf+= i;
			iLen-= i;
		}
		return this->GetLeft();
	}

	// ---------------------------------------------------------------------------------------------------
	// Return position in s
	double GetPosition()
	{
		int r=0;
		if(ioctl( this->GetDevice(), SNDCTL_DSP_GETODELAY, &r)==-1)
			return -1;

		return ((double)( this->bytesPlayed- r ))/((double)( 2* this->channels* this->rate ));
	}

};


// *****************************************************************************************************
//	Input sound stream main class
// *****************************************************************************************************
class ISoundStream
{
private:
	bool bStopFlag;
	// Buffers to be freed by the stop op
	int dev;
	int format;
	int channels;
	int rate;
	int bufscount;
	int bufsize;
	int bufsused;
	long long bytes;
	char sErr[ 512 ];
	int iErr;

	// ---------------------------------------------------------------------------------------------------
	// close audio device
	void Uninit()
	{
		if( this->dev == -1 )
			return;

		ioctl( this->dev, SNDCTL_DSP_RESET, NULL);
		close( this->dev );
		this->dev = -1;
	}

	// ---------------------------------------------------------------------------------------------------
	int Init( int rate, int channels, int format )
	{
		this->dev= open( dsp, O_RDONLY );
		if( this->dev< 0 )
		{
			this->iErr= errno;
			sprintf( &this->sErr[ 0 ], "%s at %s", strerror( errno ), "OPEN");
			return -1;
		}

    /*if (fcntl(this->dev, F_SETFL, O_NONBLOCK) == -1)
		{
			this->iErr= errno;
			sprintf( &this->sErr[ 0 ], "%s at %s", strerror( errno ), "SET_BlOCK");
			return -1;
		}*/

		this->format=format;
		if( ioctl( this->dev, SNDCTL_DSP_SETFMT, &this->format)<0 || this->format != format)

		this->channels = channels;
		int c = this->channels-1;
		if (ioctl( this->dev, SNDCTL_DSP_STEREO, &c) == -1)
		{
			this->iErr= errno;
			sprintf( &this->sErr[ 0 ], "%s at %s", strerror( errno ), "SET_CHANNELS");
			return -1;
		}

		this->channels= c+1;

		// set rate
		this->rate= rate;
		this->bytes= this->bufsused= 0;
		ioctl( this->dev, SNDCTL_DSP_SPEED, &this->rate);
		return 0;
	}

	// ---------------------------------------------------------------------------------------------------
	float GetLeft()
	{
		return (float)( this->bufsize- this->GetSpace() )/ ((double)( 2* this->channels* this->rate ));
	}
	// ---------------------------------------------------------------------------------------------------
	// return: how many bytes can be grabbed without blocking
	int GetSpace()
	{
		audio_buf_info zz;
		if( ioctl( this->dev , SNDCTL_DSP_GETISPACE, &zz)== -1 )
			return -1;

		return zz.fragments*zz.fragsize;
	}
public:
	// ---------------------------------------------------------------------------------------------------
	ISoundStream( int rate, int channels, int format, int flags, int iDev )
	{
		this->iErr= 0;
		this->sErr[ 0 ]= 0;
		this->bufsize= 0;
		if( this->Init( rate, channels, format )== 0 )
			this->bufsize= READ_BUFFER_SIZE;
		this->Stop();
	}
	// ---------------------------------------------------------------------------------------------------
	~ISoundStream()
	{
		this->Stop();
	}
	// ---------------------------------------------------------------------------------------------------
	int GetLastError()	{ return( this->iErr );	}
	// ---------------------------------------------------------------------------------------------------
	char* GetErrorString()	{ return( this->sErr );	}
	// ---------------------------------------------------------------------------------------------------
	int GetDevice(){ return this->dev;	}
	// ---------------------------------------------------------------------------------------------------
	int GetAudioBufSize(){ return this->bufsize;	}
	// ---------------------------------------------------------------------------------------------------
	int GetRate(){ return this->rate;	}
	// ---------------------------------------------------------------------------------------------------
	int GetChannels(){ return this->channels;	}

	// ---------------------------------------------------------------------------------------------------
	int Stop()
	{
		this->Uninit();
		return 0;
	}

	// ---------------------------------------------------------------------------------------------------
	bool Start()
	{
		if( this->GetDevice()== -1 )
			this->Init( this->rate, this->channels, this->format );

		return true;
	}

	// ---------------------------------------------------------------------------------------------------
	// Return position in s
	double GetPosition()
	{
		return ((double)this->bytes )/((double)( 2* this->channels* this->rate ));
	}
	// ---------------------------------------------------------------------------------------------------
	// Get size of the data that already in the buffer( read at least half a buffer )
	int GetSize()
	{
		return READ_BUFFER_SIZE;
	}
	// ---------------------------------------------------------------------------------------------------
	// Return data from the buffer
	int GetData( char* pData, int iSize )
	{
		int i= read( this->GetDevice(), pData, iSize );
		if( i< 0 )
		{
			this->iErr= errno;
			sprintf( &this->sErr[ 0 ], "%s at %s", strerror( errno ), "READ");
			return i;
		}
		this->bytes+= i;
		return i;
	}

};
#endif


/*


	*/

