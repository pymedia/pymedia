#! /bin/env python

def dumpWAV( name ):
	import pymedia.audio.acodec as acodec
	import time, wave, string, os
	name1= str.split( name, '.' )
	name2= string.join( name1[ : len( name1 )- 1 ] )
	dec= acodec.Decoder( name1[ -1 ].lower() )
	f= open( name, 'rb' )
	snd= None
	s= " "
	while len( s ):
		s= f.read( 20000 )
		if len( s ):
			r= dec.decode( s )
			if snd== None:
				snd= wave.open( name2+ '.wav', 'wb' )
				snd.setparams( (r.channels, 2, r.sample_rate, 0, 'NONE','') )
			
			snd.writeframes( r.data )

import sys
if len( sys.argv )!= 2:
	print "Usage: dump_wav <filename>"
else:
	dumpWAV( sys.argv[ 1 ] )
