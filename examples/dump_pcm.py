#! /bin/env python

import pymedia.audio.acodec as acodec
import sys, string

def dumpPCM( fname ):
  s= fname.split( '.' )
  dec= acodec.Decoder( s[ -1 ].lower() )
  f= open( fname, 'rb' )
  fname1= string.join( s[ : len( s )-1 ], '.' )+ '.pcm'
  f1= open( fname1, 'wb' )
  s= f.read( 4096 )
  while len( s )> 0:
    res= dec.decode( s )
    f1.write( res.data )
    s= f.read( 4096 )

  f.close()
  f1.close()
  print 'Dump completed into %s ' % fname1

# Test media module 
if __name__== '__main__':
  if len( sys.argv )!= 2:
    print "Usage: dump_pcm <file_name>"
  else:
    dumpPCM( sys.argv[ 1 ] )
