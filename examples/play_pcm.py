#! /bin/env python

import sys

def playPCM( fname ):
  import pymedia.audio.sound as sound
  import time
  snd1= sound.Output( 44100, 2, sound.AFMT_S16_LE )
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
  if len( sys.argv )!= 2:
    print "Usage: play_pcm <file>"
  else:
    playPCM( sys.argv[ 1 ] )

