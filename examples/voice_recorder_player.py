import time, sys
import pymedia.audio.sound as sound
import pymedia.audio.acodec as acodec
import pymedia.muxer as muxer

READ_CHUNK= 512

def voiceRecorderPlayer( secs, delay, interval, name ):
  # Turn on the microphone input
  open( name, 'wb' ).close()
  mixer= sound.Mixer()
  controls= mixer.getControls()
  micControls= filter( lambda x: \
      x[ 'connection' ]== 'Microphone' and \
      x[ 'destination' ]== 'Recording Control' and \
      x[ 'name' ].count( 'Volume' )> 0, controls )
  if len( micControls ):
    micControls[ 0 ][ 'control' ].setActive()
    print 'Setting microphone input as active'
  
  # Minimum set of parameters we need to create Encoder
  cparams= { 'id': acodec.getCodecID( 'mp3' ),
             'bitrate': 128000,
             'sample_rate': 44100,
             'channels': 2 } 
  ac= acodec.Encoder( cparams )
  dm= muxer.Demuxer( 'mp3' )
  dc= acodec.Decoder( cparams )
  sndIn= sound.Input( 44100, 2, sound.AFMT_S16_LE )
  sndOut= sound.Output( 44100, 2, sound.AFMT_S16_LE )
  sndIn.start()
  bytesWritten= 0
  bytesRead= 0
  t= time.time()
  paused= False
  
  # Loop until recorded position greater than the limit specified
  while sndIn.getPosition()<= secs:
    s= sndIn.getData()
    if s and len( s ):
      f= open( name, 'ab' )
      for fr in ac.encode( s ):
        # We definitely should use mux first, but for
        # simplicity reasons this way it'll work also
        f.write( fr )
        bytesWritten+= len( fr )
      
      f.close()
    else:
      # 1/10 of the second buffer for playback
      if sndOut.getLeft()< 0.1 and not paused:
        if bytesRead< bytesWritten+ READ_CHUNK:
          f= open( name, 'rb' )
          f.seek( bytesRead )
          s= f.read( READ_CHUNK )
          f.close()
          bytesRead+= len( s )
          frames= dm.parse( s )
          if frames:
            for fr in frames:
              r= dc.decode( fr[ 1 ] )
              if r and r.data:
                sndOut.play( r.data )
      else:
        time.sleep( .01 )
      
      # See if pause should be working
      i= int(( time.time()- t ) % ( interval+ delay ))
      paused= i> interval
  
  # Stop listening the incoming sound from the microphone or line in
  sndIn.stop()

# ----------------------------------------------------------------------------------
# Record stereo sound from the line in or microphone and save it as mp3 file
# Replay MP3 instantly and pause for certain amount (delay) every (interval) seconds
# Specify length and output file name
# http://pymedia.org/
if __name__ == "__main__":
  if len( sys.argv )!= 5:
    print 'Usage: voice_recorder_player <seconds_to_record> <seconds_to_pause> <seconds_to_play> <file_name>'
  else:
    voiceRecorderPlayer( int( sys.argv[ 1 ] ), int( sys.argv[ 2 ] ), int( sys.argv[ 3 ] ), sys.argv[ 4 ]  )
