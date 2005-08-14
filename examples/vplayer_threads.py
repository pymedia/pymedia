
import sys, thread, time, traceback, Queue, os

import pymedia
import pymedia.muxer as muxer
import pymedia.audio.acodec as acodec
import pymedia.video.vcodec as vcodec
import pymedia.audio.sound as sound

if os.environ.has_key( 'PYCAR_DISPLAY' ) and os.environ[ 'PYCAR_DISPLAY' ]== 'directfb':
  import pydfb as pygame
  YV12= pygame.PF_YV12
else:
  import pygame
  YV12= pygame.YV12_OVERLAY

AQ_LIMIT= 10
SEEK_SEC= 40

########################################################################3
# Simple cache replacer for standalone testing
class Menu:
  NAME_KEY= 'name'
  class Cache:
    def open( self, f ):
      return open( f['name'], 'rb' )
    def getPathName( self, f ):
      return f[ 'name' ]
    def getExtension( self, f ):
      return f[ 'name' ].split( '.' )[ -1 ].lower()
  
  cache= Cache()

########################################################################3
# Simple video player 
class VPlayer:
  # ------------------------------------
  def aud( self, queue ):
    self.snd= None
    resampler= None
    ac= None
    print 'VPlayer: Audio thread has started'
    try:
      while 1:
        d= queue.get()
        if type( d )== str:
          if d== 'SEEK':
            if ac: ac.reset()
            #if self.snd:
            #  self.snd.stop()
          if d== 'STOP':
            break
          if d== 'START':
            params= queue.get()
            ac= acodec.Decoder( params )
            self.snd= None
            self.aDelta= 0
          continue
        
        # Skip queue items when not playing or some errors detected
        if self.playingFile== None or len( self.err )> 0:
          if self.snd:
            self.snd.stop()
            self.snd= None
          continue
        if ac== None:
          print 'Audio codec is not set. Need to run START first.'
          continue
        
        afr= ac.decode( d[ 1 ] )
        if self.snd== None and afr:
          self.aBitRate= afr.bitrate
          print 'Sound: ', afr.sample_rate, afr.channels, afr.bitrate 
          try:
            # Hardcoded S16 ( 16 bit signed ) for now
            self.snd= sound.Output( afr.sample_rate, afr.channels, 0x10 )
            resampler= None
          except:
            # Fallback to 2 channels
            try:
              resampler= sound.Resampler( (afr.sample_rate,afr.channels), (afr.sample_rate,2) )
              self.snd= sound.Output( afr.sample_rate, 2, 0x10 )
            except:
              self.err.append( sys.exc_info()[1] )
              continue
        
        if afr:
          s= afr.data
          if resampler:
            s= resampler.resample( s )
          
          if d[ 3 ]> 0:
            # set correction factor in case of PTS presence
            self.aDelta= ( float( d[ 3 ] ) / 90000 )- self.snd.getPosition()- self.snd.getLeft()
            #print 'AUDIO: ', d[3], self.aDelta, queue.qsize(), self.snd.getLeft()
      
          # See if we have anything to play
          if len( s )> 0 and self.seek== 0:
            self.snd.play( s )
    except:
      traceback.print_exc()
        
    print 'VPlayer: Audio thread has stopped'
  
  # ------------------------------------
  def vid( self, queue ):
    frRate= 0
    vc= surfaces= None
    sndDelay= lastPts= pts= vPTS= hurry= 0
    clock= 0
    delay= .0
    print 'VPlayer: Video thread has started'
    try:
      while 1:
        # Video stream processing
        d= queue.get()
        if type( d )== str:
          if d== 'SEEK':
            hurry= 0
            if vc: vc.reset()
            sndDelay= -.5
          if d== 'STOP':
            break
          if d== 'START':
            self.overlay= None
            params= queue.get()
            try:
              #print 'External codec params are:', params
              vc= pymedia.video.ext_codecs.Decoder( params ) 
              sndDelay= 0
            except:
              print 'Falling back to software decoder'
              try:
                vc= vcodec.Decoder( params )
                sndDelay= 0
              except:
                traceback.print_exc()
                self.err.append( sys.exc_info()[1] )
                continue
              
            clock= time.time()
          
          continue
        
        if self.playingFile== None or len( self.err )> 0 or self.seek:
          continue
        
        if vc== None:
          continue
        
        try:
          vfr= vc.decode( d[ 1 ] )
          #if hurry== 0:
          #  time.sleep( .035 )
        except:
          cls= sys.exc_info()[ 0 ].__name__
          if cls.find( 'NeedBuffer' )!= -1:
            params= vc.getParams()
            #print 'External codec need buffers', params
            size= params[ 'width' ], params[ 'height' ]
            surfaces= [ pygame.Surface( size, 0, YV12 ) for x in range( 4 ) ]
            st= [ x.lock() for x in surfaces ]
            offs= [ x.get_offset() for x in surfaces ]
            #print 'Surfaces allocated', offs
            vc.setBuffers( st[ 0 ][ 1 ], offs )
            vfr= vc.decode( d[ 1 ] )
          else:
            raise
        
        if vfr and vfr.data:
          if self.skipSound:
            self.frameNum= 0
            self.skipSound= 0
            vPTS= 0
          
          if self.vBitRate== 0: self.vBitRate= vfr.bitrate
          #print d[3]
          if d[ 3 ]> 0:
            # Calc frRate
            if lastPts< d[3] and self.frameNum:
              lastPts= d[3]
              vPTS= float( d[ 3 ] ) / 90000
              frRate= vPTS / self.frameNum
              if frRate> .04:
                frRate= .04
              
              sndDelay= -.5
              #print 'PTS: ', vPTS, self.getAPTS(), self.frameNum
          
          if self.overlayLoc and self.overlay== None:
            self.overlay= pygame.Overlay( YV12, vfr.size )
            if vfr.aspect_ratio> .0:
              self.pictureSize= ( vfr.size[ 1 ]* vfr.aspect_ratio, vfr.size[ 1 ] )
            else:
              self.pictureSize= vfr.size
            self.setOverlay( self.overlayLoc )
          
          if self.frameNum> 0:
            # use audio clock
            aPTS= self.getAPTS()
            if aPTS== 0:
              # use our internal clock instead
              aPTS= time.time()- clock
            
            delay= vPTS- aPTS+ sndDelay  # mpeg2 +.3, avi -.2
            print 'Delay', delay, self.frameNum, vPTS, aPTS, frRate
            if delay< 0:
              hurry= -delay/ frRate
            else:
              hurry= 0
            if delay> 0 and delay< 2:
              time.sleep( delay )
          else:
            frRate= float(vfr.rate_base)/ float(vfr.rate)
          
          if self.overlay and vfr.data:
            if surfaces:
              self.overlay.surface().blit( surfaces[ vfr.index ], (0,0) )
            else:
              self.overlay.set_data( vfr.data )
            
            self.overlay.display()
          
          self.frameNum+= 1
          vPTS+= frRate
    except:
      traceback.print_exc()
    
    if surfaces:
      [ x.unlock() for x in surfaces ]
    
    print 'VPlayer: Video thread has stopped'
  
  # ------------------------------------
  def __init__( self ):
      self.cq= None
      self.frameNum= -1
      self.exitFlag= 1
      self.showOverlay= 0
      self.ct= None
      self.pictureSize= None
      self.paused= 0
      self.snd= None
      self.aq= Queue.Queue( AQ_LIMIT )
      self.vq= Queue.Queue()
      self.stopPlayback()
      self.err= []
      self.aDelta= 0
      self.aBitRate= 0
      self.vBitRate= 0
      self.seek= 0
      self.skipSound= 0
  
  # ------------------------------------
  def start( self ):
      if self.ct:
          raise 'cannot run another copy of vplayer'
      
      self.exitFlag= 0
      vt= thread.start_new_thread( self.vid, ( self.vq, ))
      at= thread.start_new_thread( self.aud, ( self.aq, ))
      self.ct= thread.start_new_thread( self.readerLoop, (self.vq,self.aq) )
  
  # ------------------------------------
  def stop( self ):
      self.stopPlayback()
      self.exitFlag= 1
      self.aq.put( 'STOP' )
      self.vq.put( 'STOP' )
  
  # ------------------------------------
  def startPlayback( self, file ):
      self.stopPlayback()
      self.playingFile= file
  
  # ------------------------------------
  def stopPlayback( self, bForce= True ):
      if bForce:
          if self.snd:
              self.snd.stop()
          self.playingFile= None
      # Clean up the queues
      while self.aq.qsize() or self.vq.qsize(): time.sleep( .001 )
      # Close the overlay
      self.setOverlay( None )
      self.paused= 0
  
  # ------------------------------------
  def seekRelative( self, secs ):
      while self.seek:
        time.sleep( 0.01 )
      self.seek= secs
      self.snd.stop()
  
  # ------------------------------------
  def setPaused( self, paused ):
      if self.playingFile:
          self.paused= paused
  
  # ------------------------------------
  def isPaused( self ):
      return self.paused
  
  # --------------------------------------------------------
  def isMute( self ):
      if self.snd:
          return self.snd.isMute()
      return 0
  
  # --------------------------------------------------------
  def setMute( self, m ):
      if self.snd:
          self.snd.setMute( m )
  
  # --------------------------------------------------------
  def getVolume( self ):
      if self.snd:
          return self.snd.getVolume()
      return 0
    
  # --------------------------------------------------------
  def setVolume( self, vol ):
      if self.snd:
          self.snd.setVolume( vol )
    
  # --------------------------------------------------------
  def getErrors( self ):
      return self.err
  
  # ------------------------------------
  def setOverlay( self, loc ):
      self.overlayLoc= loc
      if loc== None:
          self.overlay= None
      elif self.overlay:
          # Calc the scaling factor
          sw,sh= self.overlayLoc[ 2: ]
          w,h= self.pictureSize
          x,y= self.overlayLoc[ :2 ]
          factor= min( float(sw)/float(w), float(sh)/float(h) )
          # Find appropriate x or y pos
          x= ( sw- factor* w ) / 2+ x
          y= ( sh- factor* h ) / 2+ y
          self.overlay.set_location( (int(x),int(y),int(float(w)*factor),int(float(h)*factor)) )
      
  # ------------------------------------
  def isPlaying( self ):
      return self.overlay!= None
  
  # ------------------------------------
  def getCurrPos( self ):
      return self.frameNum
  
  # ------------------------------------
  def getAPTS( self ):
      if self.snd== None:
          return 0
      return self.snd.getPosition()+ self.aDelta
  
  # ------------------------------------
  def getPlayingFile( self ):
      return self.playingFile
  
  # ------------------------------------
  def readerLoop( self, vq, aq ):
      """
      """
      f= None
      while self.exitFlag== 0:
          self.frameNum= -1
          if self.playingFile== None:
              time.sleep( 0.01 )
              continue
          
          format= menu.cache.getExtension( self.playingFile )
          dm= muxer.Demuxer( format )
          f= menu.cache.open( self.playingFile )
          s= f.read( 300000 )
          r= dm.parse( s )
          print dm.streams
          
          # Setup video( only first stream will be used )
          self.err= []
          for vindex in xrange( len( dm.streams )):
              if dm.streams[ vindex ][ 'type' ]== muxer.CODEC_TYPE_VIDEO:
                  vq.put( 'START' )
                  vq.put( dm.streams[ vindex ] )
                  break
          
          # Setup audio( only first stream will be used )
          for aindex in xrange( len( dm.streams )):
              if dm.streams[ aindex ][ 'type' ]== muxer.CODEC_TYPE_AUDIO:
                  aq.put( 'START' )
                  aq.put( dm.streams[ aindex ] )
                  break
          
          currentFile= menu.cache.getPathName( self.playingFile )
          
          # Play until no exit flag, not eof, no errs and file still the same
          while len(s) and len( self.err )== 0 and \
                self.exitFlag== 0 and self.playingFile and \
                menu.cache.getPathName( self.playingFile )== currentFile:
              for d in r:
                  if d[ 0 ]== vindex:
                      if self.skipSound:
                        while vq.qsize():
                          time.sleep( 0.001 )
                      
                      vq.put( d )
                  elif d[ 0 ]== aindex:
                      # See if skip sound still in place
                      if not self.skipSound:
                        aq.put( d ) 
                  
                  if self.seek:
                      self.skipSound= 1
                      aq.put( ('SEEK'))
                      vq.put( ('SEEK'))
                      while aq.qsize() or vq.qsize():
                          time.sleep( .01 )
                      f.seek( ( self.aBitRate+ self.vBitRate )/8* self.seek, 1 )
                      self.seek= 0
                      break
              
              s= f.read( 10000 )
              r= dm.parse( s )
              
          if f: f.close()
          # Close current file when error detected
          if len( self.err ) or len( s )== 0:
              self.stopPlayback( False )
              self.playingFile= None
          
      print 'Main video loop has closed.'
      
player= VPlayer()

if __name__ == "__main__":
  menu= Menu()
  if len( sys.argv )< 2 or len( sys.argv )> 3:
      print 'Usage: vplayer <file_name>'
  else:
      pygame.init()
      pygame.display.set_mode( (800,600), 0 )
      player.start()
      player.startPlayback( { 'name': sys.argv[ 1 ] } )
      player.setOverlay( (0,0,800,600) )
      while player.isPlaying()== 0:
          time.sleep( .05 )
      while player.isPlaying():
        e= pygame.event.wait()
        if e.type== pygame.KEYDOWN: 
            if e.key== pygame.K_ESCAPE: 
                player.stopPlayback()
                break
            if e.key== pygame.K_RIGHT: 
                player.seekRelative( SEEK_SEC )
            if e.key== pygame.K_LEFT: 
                player.seekRelative( -SEEK_SEC )
else:
  from pycar import menu

"""
./ffmpeg -i /home/bors/Forrest.Gump\(\ DVDRip.DivX\ \).CD2.avi -vn -ar 48000 -ab 128 test.mp3
"""
