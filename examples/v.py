
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
    tstsDelay=stsDelay=lastAPTS  = Dlinna=0
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
          # See if we have anything to play
        if len( s ) > 0 and self.seek== 0:
            
            
            if lastAPTS == 0 : lastAPTS = time.time()
            stsDelay =  float(tstsDelay - time.time()+lastAPTS)
            print 'APTS', self.getAPTS(), time.time()- lastAPTS,float(len(s)/(2*2)), float(len(s)/(2*2))/afr.sample_rate,tstsDelay,stsDelay
            if (stsDelay > 0) : time.sleep (stsDelay)
            self.snd.play( s )
            tstsDelay = tstsDelay+ float(len(s)/(2*2))/afr.sample_rate
    except:
      traceback.print_exc()
        
    print 'VPlayer: Audio thread has stopped'
  
  # ------------------------------------
  def vid( self, queue):
    tr =0
    frRate= 0
    vc= surfaces= None
    stsDelay= sndDelay= lastPts= pts= vPTS= vpPts= cPts = hurry= 0
    clock= 0
    delay= .0
    print 'VPlayer: Video thread has started'
    try:
      while 1:
        # Video stream processing
        d= queue.get()
        if type( d )== str:
          if d== 'SEEK':
            hurry = 1
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
          vfr= vc.decode( d[ 1 ] , d[ 3 ])
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
            vfr= vc.decode( d[ 1 ], d[ 3 ]  )
          else:
            raise      
        if vfr and vfr.data:
          if self.skipSound:
            self.frameNum= 0
            self.skipSound= 0
            vPTS= 0
#24000/1001 (23.976) - 3754
# 24 3750
# 25 3600
# 30000/1001 (29.97)   3003");
# 30
# 50
# 60000/1001 (59.94)
# 60
# 0.1 - Í
          cPts = 1    
          if vfr.rate_base == 1001:
              if vfr.rate == 30000:
                  cPts = 0.03336666666666
              if vfr.rate == 24000:
                  cPts = 0.04170833333333
              if vfr.rate == 60000:    
                  cPts = 0.01668333333333
          if vfr.rate_base == 1:       
              if vfr.rate == 24:
                  cPts = 0.04166666666666
              if vfr.rate == 25:
                  cPts = 0.04
              if vfr.rate == 30:
                  cPts = 0.03333333333333
              if vfr.rate == 50:
                  cPts = 0.02
              if vfr.rate == 60:
                  cPts = 0.01666666666666                        
          if self.vBitRate == 0: self.vBitRate= vfr.bitrate
          if hurry > 0: 
              if hurry == 1:
                stsDelay = time.time()
                hurry == 2
              if vfr.pict_type == 1:
                hurry = 0  
          if self.overlayLoc and self.overlay== None:
            self.overlay= pygame.Overlay( YV12, vfr.size )
            if vfr.aspect_ratio > .0:
              self.pictureSize= ( vfr.size[ 1 ]* vfr.aspect_ratio, vfr.size[ 1 ] )
            else:
              self.pictureSize= vfr.size
            self.setOverlay( self.overlayLoc )   
          stsDelay = cPts - stsDelay + time.time()   
          if self.frameNum> -1:           	    
              if  (stsDelay )> 0:
                 time.sleep( stsDelay  )
          stsDelay = time.time()  
          if self.overlay and vfr.data :
               self.overlay.set_data( vfr.data )
          if(vfr.frame_pts > 0): print 'VPTS', float(vfr.frame_pts) / 90000   
          if (hurry == 0):
               self.overlay.display()
          self.frameNum+= 1
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
      self.StartTime= 0
      self.Jump = 0
      self.QNumber = 0
  # ------------------------------------
  def start( self ):
      if self.ct:
          raise 'cannot run another copy of vplayer'
      self.TimeStart= 0
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
  
  def getSTS( self ):
      return self.StartTime
      
  def setSTS( self ):
    if self.StartTime == 0:
	self.StartTime = time.time()

  def getJump( self ): 
    return self.Jump
      
  def setJump( self , ttime):
    self.Jump = ttime

  def nextQNumber( self ):
      self.QNumber = self.QNumber + 1 
      
  def  getQNumber( self ):
      return self.QNumber
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
                        self.QNumber = self.QNumber + 1
                  
                  if self.seek:
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
/ffmpeg -i /home/bors/Forrest.Gump\(\ DVDRip.DivX\ \).CD2.avi -vn -ar =
48000 -ab 128 test.mp3
"""
