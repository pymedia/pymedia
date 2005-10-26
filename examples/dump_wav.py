#! /bin/env python

def dumpWAV( name ):
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
						snd= wave.open( name2+ '.wav', 'wb' )
						snd.setparams( (r.channels, 2, r.sample_rate, 0, 'NONE','') )

					snd.writeframes( r.data )

# ----------------------------------------------------------------------------------
# Save compressed audio file into the WAV file suitable for writing on a regular Audio CD
# http://pymedia.org/
import sys
if len( sys.argv )!= 2:
	print "Usage: dump_wav <filename>"
else:
	dumpWAV( sys.argv[ 1 ] )
