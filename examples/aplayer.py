#! /bin/env python

def aplayer( name ):
	import pymedia.audio.acodec as acodec
	import pymedia.audio.sound as sound
	import time
	dec= acodec.Decoder( str.split( name, '.' )[ -1 ].lower() )
	f= open( name, 'rb' )
	snd= None
	s= f.read( 50000 )
	while len( s ):
		r= dec.decode( s )
		if snd== None:
			print 'Opening sound with %d channels' % r.channels
			snd= sound.Output( r.sample_rate, r.channels, sound.AFMT_S16_LE )
		snd.play( r.data )
		s= f.read( 512 )
	
	while snd.isPlaying():
	  time.sleep( .05 )

import sys
if len( sys.argv )!= 2:
	print "Usage: aplayer <filename>"
else:
	aplayer( sys.argv[ 1 ] )
