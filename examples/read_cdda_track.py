#! /bin/env python

import pymedia.audio.cd as cd
import sys

def readTrack( track, offset, bytes ):
  cd.init()
  c= cd.CD(0)
  tr0= c.open( "%sTrack %02d" % ( c.getName(), track ) )
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
