
import sys, thread, time, traceback, Queue, os

import pymedia.video.muxer as muxer
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

try:
    from pycar import menu
except:
    menu= Menu()

########################################################################3
# Simple video player 
class VPlayer:
    # ------------------------------------
    def aud( self, queue ):
        self.snd= None
        ac= None
        print 'VPlayer: Audio thread has started'
        try:
            while 1:
                d= queue.get()
                if type( d )== str:
                    if d== 'STOP':
                        break
                    if d== 'START':
                        params= queue.get()
                        ac= acodec.Decoder( params )
                        self.snd= None
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
                if self.snd== None:
                    print 'Sound: ', afr.sample_rate, afr.channels
                    try:
                        # Hardcode S16 ( 16 bit signed ) for now
                        self.snd= sound.Output( afr.sample_rate, afr.channels, 0x10 )
                    except:
                        traceback.print_exc()
                        self.err.append( sys.exc_info()[1] )
                        continue
                
                self.snd.play( afr.data )
        except:
            traceback.print_exc()
            
        print 'VPlayer: Audio thread has stopped'
    
    # ------------------------------------
    def vid( self, queue ):
        frRate= 0
        vc= None
        sndDelay= lastPts= pts= 0
        clock= 0
        delay= .0
        print 'VPlayer: Video thread has started'
        try:
            while 1:
                # Video stream processing
                d= queue.get()
                if type( d )== str:
                    if d== 'STOP':
                        break
                    if d== 'START':
                        self.overlay= None
                        params= queue.get()
                        try:
                            vc= vcodec.Decoder( params )
                            sndDelay= -.2
                        except:
                            traceback.print_exc()
                            self.err.append( sys.exc_info()[1] )
                            continue
                        
                        clock= time.time()
                    continue
                
                if self.playingFile== None or len( self.err )> 0:
                    continue
                
                if vc== None:
                    print 'Video codec is not set. Need to run START first.'
                    continue
                
                vfr= vc.decode( d[ 1 ] )
                if vfr:
                    #print d[3]
                    if d[ 3 ]> 0:
                        # Calc frRate
                        if pts== 0:
                            pts= d[3]
                        elif lastPts< d[3]:
                            r= float( d[ 3 ]- pts ) / 90000
                            frRate= r/ self.frameNum
                            lastPts= d[3]
                            sndDelay= -.2
                            #print 'PTS: ', d[3], self.frameNum, frRate
                    
                    if self.overlayLoc and self.overlay== None:
                        self.overlay= pygame.Overlay( YV12, vfr.size )
                        self.pictureSize= vfr.size
                        self.setOverlay( self.overlayLoc )
                    
                    if self.frameNum> 0:
                        # use audio clock
                        aPTS= self.getAPTS()
                        if aPTS== 0:
                            # use our internal clock instead
                            aPTS= time.time()- clock
                        
                        delay= ( frRate* self.frameNum )- aPTS+ sndDelay # mpeg2 +.3, avi -.2
                        #print frRate, len( d[1] ), self.frameNum, delay, aPTS
                        if delay> 0 and delay< 2:
                            time.sleep( delay )
                    else:
                        frRate= float(vfr.rate_base)/ float(vfr.rate)
                        self.frameNum= 0
                        
                    if self.overlay:
                        self.overlay.display( vfr.data )
                    
                    self.frameNum+= 1
        except:
            traceback.print_exc()
        
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
            self.playingFile= None
        # Clean up the queues
        while self.aq.qsize() or self.vq.qsize(): time.sleep( .001 )
        # Close the overlay
        self.setOverlay( None )
        self.paused= 0
    
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
        return self.snd.getPosition()
    
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
                        vq.put( d )
                    elif d[ 0 ]== aindex:
                        aq.put( d ) 
                
                s= f.read( 300000 )
                r= dm.parse( s )
                
            if f: f.close()
            # Close current file when error detected
            if len( self.err ) or len( s )== 0:
                self.stopPlayback( False )
                self.playingFile= None
            
        print 'Main video loop has closed.'
                
player= VPlayer()

if __name__ == "__main__":
  if len( sys.argv )< 2 or len( sys.argv )> 3:
      print 'Usage: vplayer <file_name> [ fullscreen ]'
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

"""

./ffmpeg -i /home/bors/Forrest.Gump\(\ DVDRip.DivX\ \).CD2.avi -vn -ar 48000 -ab 128 test.mp3

"""
