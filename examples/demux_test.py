#! /bin/env python
import sys
import pymedia.muxer as muxer


import glob

BufferSize=800000
audio=0
video=1
nodecode=0

def demuxVideo( inFile, ftype ):
  if nodecode==0:
    import pymedia.video.vcodec as vcodec
    import pygame
    YV12= pygame.YV12_OVERLAY
    pygame.init()
  r=""
  filmlen=0
  reqpos=0
  pos=0
  print '*****************************************************************************'
  print "%s" % inFile,
  print "file type %s" % ftype
  print '*****************************************************************************'
  try:
   dm= muxer.Demuxer(ftype)
  except:
   print "Unsupported fyle type:%s" % ftype
   return
  f= open( inFile, 'rb' )
  while not dm.hasHeader():
   s= f.read( BufferSize)	 
   try:
    r= dm.parse( s ,pos)
    pos=pos+BufferSize
   except muxer.SeekRequired, reqpos:
    print "seek to %d" % reqpos[0]
    if dm.hasHeader() and reqpos[0]!=0:
                  r=dm.ReadyData()
    f.seek( reqpos[0])
    pos=reqpos[0]
  info=dm.getInfo()
  print "Meta data:"
  print 'title: %s' % info['title']
  print 'author: %s' % info['artist']
  print 'album: %s' % info['album']
  print 'track: %s' % info['track']
  print 'year: %s' % info['year']
#  print 'copyright: %s' % info['copyright']
# print 'comment: %s' % info['comment']
  for st in dm.streams:
   print "Stream %d : " % st['index'] ,
   if st[ 'id' ]!= 2:
    st[ 'id' ]= st[ 'id' ]+ 1
   print "id %d" % st['id'],
   print "\tduration %d : " % st['length'] ,
   if st['type']== muxer.CODEC_TYPE_VIDEO : 
     print "\tvideo, width %d " % st['width'],
     print "height %d" % st['height'],
     print ", %d fps" % st['frame_rate']
     filmlen=st['length']

   if st['type']== muxer.CODEC_TYPE_AUDIO : 
    print "\taudio, bitrate %d" % st['bitrate']
  
  if video==1:
   v= filter( lambda x: x[ 'type' ]== muxer.CODEC_TYPE_VIDEO, dm.streams ) 
  if audio==1:
   v= filter( lambda x: x[ 'type' ]== muxer.CODEC_TYPE_AUDIO, dm.streams ) 
  if len( v )== 0:
    print 'There is no supported stream in a file %s' % inFile
    return
  v_id= v[ 0 ][ 'index' ]
  w= v[ 0 ][ 'width' ]
  h= v[ 0 ][ 'height']
  if video==1:
   print '\nAssume video stream at %d index: ' % v_id
  if audio==1:
   print '\nAssume audio stream at %d index: ' % v_id
#  print 'Press ENTER to continue...'
#  sys.stdin.read(1)
  stream=dm.streams[ v_id ]
  if video==1 and nodecode==0 :
   pygame.display.set_mode( (800,600), 0 )
   overlay= None
  try:
   if stream[ 'id' ]!= 2:
    stream[ 'id' ]= stream[ 'id' ]+ 1
   c= vcodec.Decoder(stream)
  except:
   print "decoder id [%d] failed",stream[ 'id' ]
  frcnt=0
  dlen=0
  
  while len( s )> 0:
    dlen=dlen+len(s)
    if nodecode==1:
      print "demuxed %d " % dlen 
    for fr in r:
      if fr[ 0 ]== v_id and nodecode==0:
        try:
         d= c.decode( fr[ 1 ] )  	
         if d and d.data:
          if video==1:
           if w==0: w=d.size[0]
           if h==0: h=d.size[1]	
           if not overlay:
             overlay= pygame.Overlay( YV12,(w+1,h) )
           overlay.display(d.data)
          frcnt=frcnt+1
          print "decoded %d\r" % frcnt,
        except:
         print "\ndecode error"
    s=f.read(BufferSize)
    try:
     r=None
     r= dm.parse( s  ,pos)
     pos=pos+BufferSize
    except muxer.SeekRequired,reqpos:
     f.seek( reqpos[0], 0 )
     pos=reqpos[0]
     r=dm.ReadyData()
     print "seek to" % pos
  print "\ndecoded %d from %d" % (frcnt,filmlen)
  dm=None
  if nodecode==0:
    pygame.quit()

if __name__== '__main__':
  if len( sys.argv )< 2:
    print "Pymedia Demuxer test Utility 1.0\n Copyright Alex Galkn 2005"
    print "Usage: %s <in_file> [buffer size in bytes] [nodecode]" % sys.argv[0]
    print "Example: %s *.mpeg 400000" % sys.argv[0]
  else:
    
    if len( sys.argv )>2 and int(sys.argv[ 2 ])>0 :
	BufferSize=int(sys.argv[ 2 ])
    if len( sys.argv )>3 and sys.argv[ 3 ]=='nodecode' :
	nodecode=1

    cycle=0
    while cycle<100:
      files=glob.glob(sys.argv[ 1 ])
      for curfile in files:		
       ftype=curfile.split( '.' )[ -1 ].lower()
       video=1
       if ftype=='wav' or ftype=='mp3'  or ftype=='wma' : 
            videodata=0
            audio=1
            video=0
       if ftype=='wmv': ftype='asf'
       if ftype=='wma': ftype='asf'
       if ftype=='mpg': ftype='mpeg'
       if ftype=='vob': ftype='mpeg'
       demuxVideo( curfile ,ftype)
      cycle=cycle+1
      print '\ncycle %d' % cycle




