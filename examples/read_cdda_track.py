#!/bin/env python

import pymedia.removable.cd as cd
import sys

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
  
  tr0= c.open( props[ 'titles' ][ track- 1 ][ 'name' ] )
  tr0.seek( offset, cd.SEEK_SET )
  return tr0.read( bytes )

# Test media module 
if __name__== '__main__':
  if len( sys.argv )!= 5:
    print "Usage: read_track <file_name> <track> <offset> <bytes>"
  else:
    track= int( sys.argv[ 2 ] )
    s= readTrack( track, int( sys.argv[ 3 ] ), int( sys.argv[ 4 ] ) )
    f= open( sys.argv[ 1 ], 'wb' )
    f.write( s )
    f.close()
    print 'Read of %d bytes from track %d completed successfully' % ( len( s ), track )
