#!/bin/env python

import pymedia.removable.cd as cd, pymedia.audio.sound as sound
import sys, time

CHUNK_SIZE= 2352* 100

def playTrack( track ):
  cd.init()
  if cd.getCount()== 0:
    print 'There is no cdrom found. Bailing out...'
    return 0
  
  c= cd.CD(0)
  props= c.getProperties()
  if props[ 'type' ]!= 'AudioCD':
    print 'Media in %s has type %s, not AudioCD. Cannot read audio data.' % ( c.getName(), props[ 'type' ] )
    return 0
  
  tr0= c.open( props[ 'titles' ][ track- 1 ] )
  snd= sound.Output( 44100, 2, sound.AFMT_S16_LE )
  s= ' '
  while len( s ):
    s= tr0.read( CHUNK_SIZE )
    snd.play( s )
  
  while snd.isPlaying():
    time.sleep( .01 )

# ----------------------------------------------------------------------------------
# Read track or its part from the Audio CD and save it a raw file containing the data
# The file will be PCM 44100 Hz 2 channels 16 unsigned
# http://pymedia.org/
if __name__== '__main__':
  if len( sys.argv )!= 2:
    print "Usage: play_cdda_track <track>"
  else:
    track= int( sys.argv[ 1 ] )
    s= playTrack( track )
    print 'Read of %d bytes from track %d completed successfully' % ( int( sys.argv[ 3 ] ), track )
 