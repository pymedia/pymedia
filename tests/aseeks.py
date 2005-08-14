#! /bin/env python

import sys, time, random

SEEK_INTERVAL= 1

def aplayer( name ):
  import pymedia.muxer as muxer, pymedia.audio.acodec as acodec, pymedia.audio.sound as sound
  import time
  dm= muxer.Demuxer( str.split( name, '.' )[ -1 ].lower() )
  f= open( name, 'rb' )
  f.seek( 0, 2 )
  size= f.tell()
  f.seek( 0, 0 )
  snd= resampler= dec= None
  s= f.read( 32000 )
  t= time.time()
  while len( s ):
    frames= dm.parse( s )
    if frames:
      for fr in frames:
        # Assume for now only audio streams
        if dec== None:
          print dm.getInfo(), dm.streams
          dec= acodec.Decoder( dm.streams[ fr[ 0 ] ] )
        
        r= dec.decode( fr[ 1 ] )
    if time.time()- t> SEEK_INTERVAL:
      newPos= random.randint( 0, size / 2 )
      print 'Seek to', newPos
      dec.reset()
      dm.reset()
      f.seek( newPos, 0 )
      t= time.time()
    
    s= f.read( 512 )

# ----------------------------------------------------------------------------------
# Play any compressed audio file with adjustable pitch
# http://pymedia.org/
if len( sys.argv )!= 2:
  print "Usage: aseeks <filename>"
else:
  aplayer( sys.argv[ 1 ] ) 