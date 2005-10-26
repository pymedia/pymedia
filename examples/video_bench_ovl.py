#! /bin/env python
import sys, time, os
import pymedia.muxer as muxer
import pymedia.video.vcodec as vcodec
import pymedia.audio.acodec as acodec
import pymedia.audio.sound as sound

if os.environ.has_key( 'PYCAR_DISPLAY' ) and os.environ[ 'PYCAR_DISPLAY' ]== 'directfb':
  import pydfb as pygame
  YV12= pygame.PF_YV12
else:
  import pygame
  YV12= pygame.YV12_OVERLAY

def videoDecodeBenchmark( inFile, opt ):
  pygame.init()
  pygame.display.set_mode( (800,600), 0 )
  ovl= None
  
  dm= muxer.Demuxer( inFile.split( '.' )[ -1 ] )
  f= open( inFile, 'rb' )
  s= f.read( 400000 )
  r= dm.parse( s )
  v= filter( lambda x: x[ 'type' ]== muxer.CODEC_TYPE_VIDEO, dm.streams )
  if len( v )== 0:
    raise 'There is no video stream in a file %s' % inFile
  
  v_id= v[ 0 ][ 'index' ]
  print 'Assume video stream at %d index: ' % v_id
  
  a= filter( lambda x: x[ 'type' ]== muxer.CODEC_TYPE_AUDIO, dm.streams )
  if len( a )== 0:
    print 'There is no audio stream in a file %s. Ignoring audio.' % inFile
    opt= 'noaudio'
  else:
    a_id= a[ 0 ][ 'index' ]
  
  t= time.time()
  
  vc= vcodec.Decoder( dm.streams[ v_id ] )
  print dm.streams[ v_id ]
  if opt!= 'noaudio':
    ac= acodec.Decoder( dm.streams[ a_id ] )
  resampler= None
  frames= 0
  q= []
  while len( s )> 0:
    for fr in r:
      if fr[ 0 ]== v_id:
        d= vc.decode( fr[ 1 ] )
        if d and d.data:
          frames+= 1
          #ff= open( 'c:\\test', 'wb' )
          #ff.write( d.data[ 0 ] )
          #ff.close()
          if not ovl:
            ovl= pygame.Overlay( YV12, d.size )
          q.append( d )
          if len( q )> 4:
            ovl.set_data( q[0].data )
            ovl.display()
            del( q[0] )
      elif opt!= 'noaudio' and fr[ 0 ]== a_id:
        d= ac.decode( fr[ 1 ] )
        if resampler== None:
          if d and d.channels> 2:
            resampler= sound.Resampler( (d.sample_rate,d.channels), (d.sample_rate,2) )
        else:
          data= resampler.resample( d.data )
    
    s= f.read( 400000 )
    r= dm.parse( s )
    
    tt= time.time()- t
    print '%d frames in %d secs( %.02f fps )' % ( frames, tt, float(frames)/tt )
    
    ev= pygame.event.get()
    for e in ev:
      if e.type== pygame.KEYDOWN and e.key== pygame.K_ESCAPE:
        s= ''
        break

if __name__== '__main__':
  if len( sys.argv )< 2 or len( sys.argv )> 3:
    print "Usage: video_bench <in_file> [ noaudio ]"
  else:
    s= ''
    if len( sys.argv )> 2:
      if sys.argv[ 2 ] not in ( 'noaudio' ):
        print "Option %s not recognized. Should be 'noaudio'. Ignored..." % sys.argv[ 2 ]
      else:
        s= sys.argv[ 2 ]
    
    videoDecodeBenchmark( sys.argv[ 1 ], s )


