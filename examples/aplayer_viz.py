#! /bin/env python
import pygame

RES= ( 200, 200 )
SEEK_SEC= 5

def aplayer( name ):
	import pymedia.audio.acodec as acodec
	import pymedia.audio.sound as sound
	import time
	dec= acodec.Decoder( str.split( name, '.' )[ -1 ].lower() )
	f= open( name, 'rb' )
	br= None
	snd= None
	s= f.read( 50000 )
	while len( s ):
		r= dec.decode( s )
		if snd== None:
			print 'Opening sound with %d channels( %d bitrate )' % ( r.channels, r.bitrate )
			snd= sound.Output( r.sample_rate, r.channels, sound.AFMT_S16_LE )
			br= r.bitrate
		if r:
			snd.play( r.data )
		ev= pygame.event.get() 
		s= f.read( 512 )
		for e in ev:
			if e.type== pygame.KEYDOWN: 
				if e.key== pygame.K_RIGHT:
					# Seek forward
					f.seek( SEEK_SEC* br/ 8, 1 )
					dec.reset()
				if e.key== pygame.K_LEFT:
					# Seek forward
					if f.tell()> SEEK_SEC* br/ 8: 
						f.seek( -SEEK_SEC* br/ 8, 1 )
					dec.reset()
				
				if e.key== pygame.K_ESCAPE:
					s= ''
					break
	
	while snd.isPlaying():
	  time.sleep( .05 )

import sys
if len( sys.argv )!= 2:
	print "Usage: aplayer_viz <filename>"
else:
	pygame.init()
	pygame.display.set_mode( RES, 0 )
	aplayer( sys.argv[ 1 ] )
