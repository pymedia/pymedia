#! /bin/env python
import sys, time
import pymedia.video.muxer as muxer
import pymedia.video.vcodec as vcodec

def videoDecodeBenchmark( inFile ):
  dm= muxer.Demuxer( inFile.split( '.' )[ -1 ] )
  f= open( inFile, 'rb' )
  s= f.read( 400000 )
  r= dm.parse( s )
  v= filter( lambda x: x[ 'type' ]== muxer.CODEC_TYPE_VIDEO, dm.streams )
  if len( v )== 0:
    raise 'There is no video stream in a file %s' % inFile
  
  v_id= v[ 0 ][ 'index' ]
  print 'Assume video stream at %d index: ' % v_id
  t= time.time()
  
  c= vcodec.Decoder( dm.streams[ v_id ] )
  frames= 0
  while len( s )> 0:
    for fr in r:
      if fr[ 0 ]== v_id:
        d= c.decode( fr[ 1 ] )
        if d:
          frames+= 1
    
    s= f.read( 400000 )
    r= dm.parse( s )
    
    tt= time.time()- t
    print '%d frames in %d secs( %.02f fps )' % ( frames, tt, float(frames)/tt )

if __name__== '__main__':
  if len( sys.argv )!= 2:
    print "Usage: videobench <in_file>"
  else:
    videoDecodeBenchmark( sys.argv[ 1 ] )


