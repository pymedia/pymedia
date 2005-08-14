#! /bin/env python
import sys, time
import pymedia.muxer as muxer
import pymedia.video.vcodec as vcodec

def demuxBench( inFile ):
	dm= muxer.Demuxer( inFile.split( '.' )[ -1 ].lower() )
	f= open( inFile, 'rb' )
	s= f.read( 400000 )
	i= len( s )
	t= 0
	r= dm.parse( s )
	while len( s )> 0:
		s= f.read( 400000 )
		tt= time.time()
		r= dm.parse( s )
		t+= time.time()- tt
		i+= len( s )
	
	print 'File %s ( %d bytes ) was demuxed in %2f secs' % ( inFile, i, t )

if __name__== '__main__':
  if len( sys.argv )!= 2:
    print "Usage: demux_bench <in_file>"
  else:
    demuxBench( sys.argv[ 1 ] )


