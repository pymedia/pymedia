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
#include <pthread.h>
#include <fcntl.h>
#include "Python.h"

#include "afmt.h"

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

// -------------------------------------------------------------------
int GetLastError(){ return errno; }
void SetLastError( int err ){ errno= err; }
void *WorkerThread( void *pvArg );

// *****************************************************************************************************
//	Sound stream main class
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
	int bps;
	int iMute;
	int bufsize;
	long long bytesPlayed;
	char* sErr;

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
			this->sErr= "Cannot open device. The sound device may not be available in a system.";
			return -1;
		}

    if (fcntl(dev, F_SETFL, 0) == -1)
		{
			this->sErr= "Cannot put device into blocking mode";
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
					this->sErr= "Cannot set channels for playing, Sound device may not suport it";
					return -1;
				}
				else;
			else
			{
				int c = this->channels-1;
				if (ioctl( this->dev, SNDCTL_DSP_STEREO, &c) == -1)
				{
					this->sErr= "Cannot set stereo mode for playing, Sound device may not support it";
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
	ISoundStream( int rate, int channels, int format, int flags )
	{
		SetLastError(0);
		this->sErr= "";
		if( this->Init( rate, channels, format )== 0 )
			this->bufsize= this->GetSpace();
	}

	// ---------------------------------------------------------------------------------------------------
	~ISoundStream()
	{
		this->Stop();
	}

	// ---------------------------------------------------------------------------------------------------
	char* GetErrorString()	{ return( this->sErr );	}
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
		return 0;
	}

	// ---------------------------------------------------------------------------------------------------
	int Stop()
	{
		this->Uninit();
		return 0;
	}

	// ---------------------------------------------------------------------------------------------------
	int Play( unsigned char *buf, int iLen )
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

				this->sErr= strerror( errno );
				return i;
			}
			this->bytesPlayed+= i;
			buf+= i;
			iLen-= i;
		}
		return 0;
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

#endif


/*


	*/

