#! /bin/env python
import sys
import pymedia.video.muxer as muxer
import pymedia.video.vcodec as vcodec

def demuxVideo( inFile, outFile ):
	dm= muxer.Demuxer( inFile.split( '.' )[ -1 ].lower() )
	f= open( inFile, 'rb' )
	fw= open( outFile, 'wb' )
	s= f.read( 400000 )
	r= dm.parse( s )
	v= filter( lambda x: x[ 'type' ]== muxer.CODEC_TYPE_VIDEO, dm.streams )
	if len( v )== 0:
		raise 'There is no video stream in a file %s' % inFile
	
	v_id= v[ 0 ][ 'index' ]
	print 'Assume video stream at %d index: ' % v_id
	while len( s )> 0:
		for fr in r:
			if fr[ 0 ]== v_id:
				fw.write( fr[ 1 ] )
		
		s= f.read( 400000 )
		r= dm.parse( s )

if __name__== '__main__':
  if len( sys.argv )!= 3:
    print "Usage: demux_video <in_file> <out_file>"
  else:
    demuxVideo( sys.argv[ 1 ], sys.argv[ 2 ] )


