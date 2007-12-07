#! /bin/env python

import sys

def playPCM( fname, sampleRate ):
  import pymedia.audio.sound as sound
  import time
  snd1= sound.Output( sampleRate, 2, sound.AFMT_S16_LE )
  f= open( fname, 'rb' )
  s= ' '
  while len( s ):
    s= f.read( 40960 )
    snd1.play( s )
    print 'Played %d bytes, pos %f ' % ( len( s ), snd1.getPosition() )
  
  while snd1.isPlaying():
    time.sleep( 0.05 )

# Test media module 
if __name__== '__main__':
  if not len( sys.argv ) in ( 2,3 ):
    print "Usage: play_pcm <file> [sample_rate]"
  else:
    sampleRate= 44100
    if len( sys.argv )== 3:
      sampleRate= int( sys.argv[ 2 ] )
    playPCM( sys.argv[ 1 ], sampleRate )

