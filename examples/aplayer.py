#! /bin/env python

import sys

EMULATE=0

def aplayer( name, card, rate ):
  import pymedia.muxer as muxer, pymedia.audio.acodec as acodec, pymedia.audio.sound as sound
  import time
  dm= muxer.Demuxer( str.split( name, '.' )[ -1 ].lower() )
  snds= sound.getODevices()
  if card not in range( len( snds ) ):
    raise 'Cannot play sound to non existent device %d out of %d' % ( card+ 1, len( snds ) )
  f= open( name, 'rb' )
  snd= resampler= dec= None
  s= f.read( 32000 )
  t= 0
  while len( s ):
    frames= dm.parse( s )
    if frames:
      for fr in frames:
        # Assume for now only audio streams
        if dec== None:
          print dm.getInfo(), dm.streams
          dec= acodec.Decoder( dm.streams[ fr[ 0 ] ] )
        
        r= dec.decode( fr[ 1 ] )
        if r and r.data:
          if snd== None:
            print 'Opening sound with %d channels -> %s' % ( r.channels, snds[ card ][ 'name' ] )
            snd= sound.Output( r.sample_rate* rate, r.channels, sound.AFMT_S16_LE, card )
            if rate< 1 or rate> 1:
              resampler= sound.Resampler( (r.sample_rate,r.channels), (int(r.sample_rate/rate),r.channels) )
              print 'Sound resampling %d->%d' % ( r.sample_rate, r.sample_rate/rate )
          
          data= r.data
          if resampler:
            data= resampler.resample( data )
          if EMULATE:
            # Calc delay we should wait to emulate snd.play()
            d= len( data )/ float( r.sample_rate* r.channels* 2 )
            time.sleep( d )
            if int( t+d )!= int( t ):
              print 'playing: %d sec\r' % ( t+d ),
            t+= d
          else:
            snd.play( data )
    
    s= f.read( 512 )

  while snd.isPlaying():
    time.sleep( .05 )

# ----------------------------------------------------------------------------------
# Play any compressed audio file with adjustable pitch
# http://pymedia.org/
if len( sys.argv )< 2 or len( sys.argv )> 4:
  print "Usage: aplayer <filename> [ sound_card_index, rate( 0..1- slower, 1..4 faster ) ]"
else:
  i= 0
  r= 1
  if len( sys.argv )> 2 :
    i= int( sys.argv[ 2 ] )
  if len( sys.argv )> 3 :
    r= float( sys.argv[ 3 ] )
  aplayer( sys.argv[ 1 ], i, r )
