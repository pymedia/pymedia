#!/bin/env python

import pymedia.removable.cd as cd
import sys, wave

CHUNK_SIZE= 2352* 10

def readTrack( track, offset, bytes ):
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
  tr0.seek( offset, cd.SEEK_SET )
  f= wave.open( props[ 'titles' ][ track- 1 ]+ '.wav', 'wb' )
  f.setparams( (2, 2, 44100, 0, 'NONE','') )
  s= ' '
  while len( s ) and bytes:
    if bytes> CHUNK_SIZE:
      s= tr0.read( CHUNK_SIZE )
    else:
      s= tr0.read( bytes )
    
    f.writeframes( s )
    bytes-= len( s )

# ----------------------------------------------------------------------------------
# Read track or its part from the Audio CD and save it a raw file containing the data
# The file will be PCM 44100 Hz 2 channels 16 unsigned
# http://pymedia.org/
if __name__== '__main__':
  if len( sys.argv )!= 4:
    print "Usage: read_track <track> <offset> <bytes>"
  else:
    track= int( sys.argv[ 1 ] )
    s= readTrack( track, int( sys.argv[ 2 ] ), int( sys.argv[ 3 ] ) )
    print 'Read of %d bytes from track %d completed successfully' % ( int( sys.argv[ 3 ] ), track )

