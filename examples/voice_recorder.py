import time, sys
import pymedia.audio.sound as sound
import pymedia.audio.acodec as acodec

def voiceRecorder( secs, name ):
  f= open( name, 'wb' )
  snd= sound.Input( 22050, 1, sound.AFMT_S16_LE )
  cparams= { 'id': acodec.getCodecID( "mp2" ),
             'bitrate': 64000,
             'sample_rate': 22050,
             'channels': 1 } 
  ac= acodec.Encoder( cparams )
  snd.start()
  while snd.getPosition()< secs:
    s= snd.getData()
    if s:
      for fr in ac.encode( s ):
        f.write( fr )
    else:
      time.sleep( .01 )
  
  snd.stop()
  f.close()

# ----------------------------------------------------
if __name__ == "__main__":
  if len( sys.argv )!= 3:
    print 'Usage: voice_recorder <seconds> <file_name>'
  else:
    voiceRecorder( int( sys.argv[ 1 ] ), sys.argv[ 2 ] )
