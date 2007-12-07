#! /bin/env python

def dumpPCM( name ):
	import pymedia.audio.acodec as acodec
	import pymedia.muxer as muxer
	import time, wave, string, os
	name1= str.split( name, '.' )
	name2= string.join( name1[ : len( name1 )- 1 ] )
	# Open demuxer first
	dm= muxer.Demuxer( name1[ -1 ].lower() )
	dec= None
	f= open( name, 'rb' )
	snd= None
	s= " "
	while len( s ):
		s= f.read( 20000 )
		if len( s ):
			frames= dm.parse( s )
			for fr in frames:
				if dec== None:
					# Open decoder
					dec= acodec.Decoder( dm.streams[ 0 ] )
				r= dec.decode( fr[ 1 ] )
				if r and r.data:
					if snd== None:
						snd= open( name2+ '.pcm', 'wb' )

					snd.write( r.data )

# ----------------------------------------------------------------------------------
# Save compressed audio file into the PCM file suitable for writing on a regular Audio CD
# http://pymedia.org/
import sys
if len( sys.argv )!= 2:
	print "Usage: dump_pcm <filename>"
else:
	dumpPCM( sys.argv[ 1 ] )
