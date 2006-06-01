#!/bin/env python

import pymedia.removable.cd as cd
import sys, time

CHUNK_SIZE= 2352* 10

def readTracks():
  cd.init()
  if cd.getCount()== 0:
    print 'There is no cdrom found. Bailing out...'
    return 0
  
  c= cd.CD(0)
  props= c.getProperties()
  if props[ 'type' ]!= 'AudioCD':
    print 'Media in %s has type %s, not AudioCD. Cannot read audio data.' % ( c.getName(), props[ 'type' ] )
    return 0
  
  t= time.time()
  bytes= 0
  for i in xrange( len( props[ 'titles' ] ) ):
    print 'Reading %d track' % ( i+ 1 )
    tr= c.open( props[ 'titles' ][ i ] )
    s= ' '
    while len( s ):
      s= tr.read( CHUNK_SIZE )
      bytes+= len( s )
  
  print '%d bytes read in %.2f secs ( %d bytes/sec )' % ( bytes, time.time()- t, bytes / ( time.time()- t ) )
  

# ----------------------------------------------------------------------------------
# Read track or its part from the Audio CD and save it a raw file containing the data
# The file will be PCM 44100 Hz 2 channels 16 unsigned
# http://pymedia.org/
if __name__== '__main__':
  if len( sys.argv )!= 1:
    print "Usage: bench_cdda"
  else:
    readTracks()

