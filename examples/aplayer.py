#! /bin/env python

def aplayer( name ):
	import pymedia.audio.acodec as acodec
	import pymedia.audio.sound as sound
	import time
	dec= acodec.Decoder( str.split( name, '.' )[ -1 ].lower() )
	f= open( name, 'rb' )
	snd= None
	while 1:
		s= f.read( 50000 )
		if len( s ):
			b= time.time()
			r= dec.decode( s )
			print 'D', time.time()- b
			if snd== None:
				print 'Opening sound with %d channels' % r.channels
				snd= sound.Output( r.sample_rate, r.channels, sound.AFMT_S16_LE )
			
			b= time.time()
			snd.play( r.data )
			print 'P', time.time()- b
		else:
			break
	
	while snd.isPlaying():
	  time.sleep( .05 )

import sys
if len( sys.argv )!= 2:
	print "Usage: aplayer <filename>"
else:
	aplayer( sys.argv[ 1 ] )
