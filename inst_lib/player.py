
import sys, thread, time, traceback, Queue, os

import pymedia.muxer as muxer
import pymedia.audio.acodec as acodec
import pymedia.video.vcodec as vcodec
import pymedia.audio.sound as sound

SEEK_IN_PROGRESS= -1
PAUSE_SLEEP= 0.003
VIDEO_FILE_CHUNK= 300000

########################################################################3
# Simple video player 
class Player:
  """
  Player is a class that plays all audio/video formats supported by pymedia
  It provides very simple interface and many features such as:
  - seeking
  - extracting metadata
  - multiple sound cards support
  - different sources for video rendering
  - error handling
  - volume operations
  
  Here is the simple way of calling Player:
    player= pymedia.Player()
    player.start()
    player.startPlayback( '<YOUR_MP3_FILE>' )
    while player.isPlaying():
      time.sleep( 0.01 )
  
  Player supports the following callback:
    class Callback:
      # called _before_ the audio frame is rendered
      # return your audio data after you process the audio frame
      # or return none for using the default sound processing
      def onAudioReady( self, afr ):
        pass
      
      # Called when the video frame is about to be rendered
      def onVideoReady( self, vfr ):
        pass
  
  Use the callback like below:
    c= Callback()
    player= pymedia.Player( c )
  """
  # ------------------------------------
  def __init__( self, callback= None ):
    """
    Create new Player instance. In case if you want to play video you have to supply the callback class instance
    which will get video data and will be able to display it properly ( onVideoReady() call ).
    For audio data the playback is done through the pymedia.sound.Output object.
    Before playing audio the onAudioReady( afr ) callback is called so you can modify audio data.
    Just return None if you do not wish to modify that data or return a sound data as string in case if you do.
    Remember that sampling frequency and number of channels should remain the same as in afr object.
    """
    self.frameNum= -1
    self.exitFlag= 1
    self.ct= None
    self.pictureSize= None
    self.paused= 0
    self.snd= None
    self.err= []
    self.aDelta= 0
    self.aBitRate= 0
    self.vBitRate= 0
    self.seek= 0
    self.vc= None
    self.ac= None
    self.ovlTime= 0
    self.playingFile= None
    self.endPos= None
    self.loops= 1
    self.volume= 0xffff
    self.startPos= 0
    self.initADelta= -1
    # Set length to 1 sec by default
    self.length= -1
    # By default the first audio card 
    self.audioCard= 0
    self.callback= callback
    self.metaData= {}
  
  # ------------------------------------
  def _resetAudio( self ):
    # No delta for audio so far
    self.snd= self.resampler= None
    self.aBitRate= self.aSampleRate= self.aChannels= self.aDelta= 0
    self.aDecodedFrames= []
    if self.ac:
      self.ac.reset()
  
  # ------------------------------------
  def _initAudio( self, params ):
    try:
      # Reset audio stream
      self._resetAudio()
      # Initializing audio codec
      self.ac= acodec.Decoder( params )
    except:
      self.err.append( sys.exc_info()[1] )
  
  # ------------------------------------
  def _resetVideo( self ):
    # Init all used vars first
    self.decodeTime= self.vBitRate= self.frameNum= \
    self.sndDelay= self.hurry= self.videoPTS= \
    self.lastPTS= self.frRate= self.vDelay= 0
    self.seek= 0
    self.pictureSize= None
    if self.initADelta!= -1:
      self.seekADelta= self.initADelta
    
    # Zeroing out decoded pics queue
    self.decodedFrames= []
    self.rawFrames= []
    if self.vc:
      self.vc.reset()
  
  # ------------------------------------
  def _initVideo( self, params ):
    # There is no overlay created yet
    try:
      # Set the initial sound delay to 0 for now
      # It defines initial offset from video in the beginning of the stream
      self.initADelta= -1
      self._resetVideo()
      self.seekADelta= 0
      # Setting up the HW video codec
      self.vc= pymedia.video.ext_codecs.Decoder( params ) 
    except:
      try:
        # Fall back to SW video codec
        self.vc= vcodec.Decoder( params )
      except:
        pass
  
  # ------------------------------------
  def _getStreamLength( self, format, dm, f, fr ):
    # Demux frames from the beginning and from the end and get the PTS diff
    if dm.hasHeader():
      # Return fixed value for now (10 secs)
      return
    
    startPTS= -1
    for d in fr:
      if d[ 3 ]> 0:
        startPTS= d[ 3 ] / 90
        break
      
    # Seek to the end and get the PTS from the end of the file
    if startPTS> 0:
      pos= f.tell()
      f.seek( 0, 0 )
      dm1= muxer.Demuxer( format )
      s= f.read( VIDEO_FILE_CHUNK )
      r= dm1.parse( s )
      endPTS= startPTS
      for d in fr:
        if d[ 3 ]> 0:
          endPTS= d[ 3 ] / 90
        
      f.seek( pos, 0 )
      self.length= endPTS- startPTS
  
  # ------------------------------------
  def _getVStreamParams( self, vindex, vparams, r ):
    # Decode one video frame and 1 audio frame to get stream data
    vDec= None
    for d in r:
      try:
        # Demux file into streams
        if d[ 0 ]== vindex:
          if not vDec:
            vDec= vcodec.Decoder( vparams )
          vfr= vDec.decode( d[ 1 ] )
          if vfr and vfr.data:
            if vfr.aspect_ratio> .0:
              self.pictureSize= ( int( vfr.size[ 1 ]* vfr.aspect_ratio ), vfr.size[ 1 ] )
            else:
              self.pictureSize= vfr.size
            
            break
      except:
        self.err.append( sys.exc_info()[1] )
        break
  
  # ------------------------------------
  def _processVideoFrame( self, d, forced= False ):
    # See if we should show video frame now
    if not forced:
      self.rawFrames.append( d )
    if len( self.decodedFrames )== 0:
      if self._decodeVideoFrame( forced )== -1:
        return
    
    # See if we have our frame inline with the sound
    self._processVideo( forced )

  # ------------------------------------
  def _decodeVideoFrame( self, forced= False ):
    # Decode the video frame
    if self.snd== None and self.seek!= SEEK_IN_PROGRESS and self.aindex!= -1 and not forced:
      return -1
    
    if len( self.decodedFrames )> vcodec.MAX_BUFFERS- 4:
      # Cannot decode video frame because of too many decoded frames already
      return 0
    
    while len( self.rawFrames ):
      d= self.rawFrames.pop( 0 )
      vfr= self.vc.decode( d[ 1 ] )
      if vfr:
        if self.seek== SEEK_IN_PROGRESS and not forced:
          if vfr.data:
            self.seek= 0
          else:
            # We decode video frame after seek, but no I frame so far
            return 0
        
        # If frame has data in it, put it in to the queue along with PTS
        self.decodedFrames.append( ( vfr, self.videoPTS ) )
        # Set up the video bitrate for the informational purpose
        if self.vBitRate== 0:
          self.vBitRate= vfr.bitrate
          
        # Handle the PTS
        rate= float( vfr.rate_base )/ vfr.rate
        if d[ 3 ]> 0 and self.lastPTS< d[3]:
          # Get the first lowest PTS( we do not have DTS :( )
          self.lastPTS= d[3]
          self.videoPTS= float( d[ 3 ] ) / 90000
          #print 'VPTS:', self.videoPTS, vfr.pict_type, self.getSndLeft()
        else:
          # We cannot accept PTS, just calculate it
          self.videoPTS+= rate
        
        return 0
    
    # No more raw frames to decode
    return -2
  
  # ------------------------------------
  def _processVideo( self, forced= False ):
    while 1:
      if len( self.decodedFrames )== 0:
        return
      
      vfr, videoPTS= self.decodedFrames[ 0 ]
      self.vDelay= videoPTS- self.seekADelta- self._getPTS()
      frRate= float( vfr.rate_base )/ vfr.rate
      res= self._decodeVideoFrame()
      #print '<<', res, self.vDelay, self.frameNum, videoPTS, self.getPTS(), len( self.decodedFrames ), len( self.rawFrames ), self.getSndLeft()
      #if res== -1 or ( self.snd and self.getSndLeft()< frRate ) or ( res== -2 and self.vDelay> 0 and self.getSndLeft()< self.vDelay and not forced ):
      if res== -1 or ( res== -2 and self.vDelay> 0 and self._getSndLeft()< self.vDelay and not forced ):
        return
      
      # If delay
      #print '!!', self.vDelay, self.frameNum, videoPTS, self.getPTS(), len( self.decodedFrames ), len( self.rawFrames ), self.getSndLeft(), self.seekADelta
      if self.vDelay> 0 and self._getSndLeft()> self.vDelay:
        time.sleep( self.vDelay / 6 )
        #print 'Delay', self.vDelay, self.getSndLeft()
        self.vDelay= 0
      
      if self.vDelay< frRate / 4 or forced:
        # Remove frame from the queue
        del( self.decodedFrames[ 0 ] )
        
        # Get delay for seeking
        if self.frameNum== 0 and self.initADelta== -1:
          self.initADelta= self._getSndLeft()
        
        # Increase number of frames
        self.frameNum+= 1
        
        # Skip frame if no data inside, but assume it was a valid frame though
        if vfr.data:
          if self.callback:
            self.callback.onVideoReady( vfr )
          
          self.vDelay= frRate
  
  # ------------------------------------
  def _processAudioFrame( self, d ):
    # Decode audio frame
    afr= self.ac.decode( d[ 1 ] )
    if afr:
      # See if we have to set up the sound
      if self.snd== None:
        self.aBitRate= afr.bitrate
        self.aSampleRate= afr.sample_rate
        self.aChannels= afr.channels
        try:
          # Hardcoded S16 ( 16 bit signed ) for now
          self.snd= sound.Output( afr.sample_rate, afr.channels, sound.AFMT_S16_LE, self.audioCard )
          self.snd.setVolume( self.volume )
          self.resampler= None
        except:
          try:
            # Create a resampler when no multichannel sound is available
            self.resampler= sound.Resampler( (afr.sample_rate,afr.channels), (afr.sample_rate,2) )
            # Fallback to 2 channels
            self.snd= sound.Output( afr.sample_rate, 2, sound.AFMT_S16_LE, self.audioCard )
            self.snd.setVolume( self.volume )
          except:
            self.err.append( sys.exc_info()[1] )
            raise
      
      # Handle the PTS accordingly
      snd= self.snd
      if d[ 3 ]> 0 and self.aDelta== 0 and snd:
        # set correction factor in case of PTS presence
        self.aDelta= ( float( d[ 3 ] ) / 90000 )- snd.getPosition()- snd.getLeft()
      
      # Play the raw data if we have it
      if len( afr.data )> 0:
        self.aDecodedFrames.append( afr )
        self._processAudio()
  
  # ------------------------------------
  def _processAudio( self ):
    snd= self.snd
    while len( self.aDecodedFrames ) and snd:
      # See if we can play sound chunk without clashing with the video frame
      if len( self.aDecodedFrames[ 0 ].data )> snd.getSpace() and self.vindex!= -1:
        break
      
      #print 'player SOUND LEFT:', self.snd.getLeft(), self.snd.getSpace(), self.isPlaying()
      if self.isPlaying():
        afr= self.aDecodedFrames.pop(0)
        s= afr.data
        if self.callback:
          afr1= self.callback.onAudioReady( afr )
          if afr1:
            s= afr1
        
        # See if we need to resample the audio data
        if self.resampler:
          s= self.resampler.resample( s )
        
        snd.play( s )
  
  # ------------------------------------
  def start( self ):
    """
    Start player object. It starts internal loop only, no physical playback is started.
    You have to call start only once for the player instance.
    If you wish to play multiple files just call startPlayback() subsequently.
    """
    if self.ct:
        raise 'cannot run another copy of vplayer'
    self.exitFlag= 0
    self.pictureSize= None
    self.ct= thread.start_new_thread( self._readerLoop, () )
  
  # ------------------------------------
  def stop( self ):
    """
    Stop player object. It stops internmal loop and any playing file.
    Once the internal loop is stopped the further playback is not possible until you issue start() again.
    """
    self.stopPlayback()
    if self.callback:
      self.callback.removeOverlay()
    # Turn the flag to exist the main thread
    self.exitFlag= 1
  
  # ------------------------------------
  def startPlayback( self, file, paused= False ):
    """
    Starts playback of the file passed as string or file like object.
    Player should already be started otherwise this call has no effect.
    If any file is already playing it will stop the playback and start playing the file you pass.
    'paused' parameter can specifiy if the playback should not start until unpausePlayback() is called.
    'paused' parameter is helpfull when you want to start your playback exactly at a certain time avoiding any delays
    caused by the file opening or codec initilizations.
    """
    self.stopPlayback()
    # Set the new file for playing
    self.paused= paused
    self.playingFile= file
  
  # ------------------------------------
  def stopPlayback( self ):
    """
    Stops file playback. If media file is not playing currently, it does nothing.
    If file was paused it will unpause it first.
    """
    self.playingFile= None
    self.unpausePlayback()
    if self.snd:
      self.snd.stop()
      self.snd= None
  
  # --------------------------------------------------------
  def pausePlayback( self ):
    """ Pause playing the current file """
    if self.isPlaying():
      self.paused= 1
      if self.snd:
        self.snd.pause()
  
  # --------------------------------------------------------
  def unpausePlayback( self ):
    """ Resume playing the current file """
    if self.isPlaying():
      if self.snd:
        self.snd.unpause()
    
    self.paused= 0
  
  # ------------------------------------
  def isPaused( self ):
    """ Returns whether playback is paused """
    return self.paused
  
  # ------------------------------------
  def seekTo( self, secs ):
    """
    Seeks the file playback position to a given number of seconds from the beginning.
    Seek may position not exactly as specified especially in video files.
    It will look for a first key frame and start playing from that position.
    In some files key frames could resides up to 10 seconds apart.
    """
    while self.seek:
      time.sleep( 0.01 )
    
    self.seek= secs
  
  # ------------------------------------
  def isPlaying( self ):
    """ Returns whether playback is active """
    return self.playingFile!= None
  
  # ------------------------------------
  def isActive( self ):
    """
    Returns whether Player object can do the playback.
    It will return False after you issue stop()
    """
    return self.exitFlag== 0
  
  # ------------------------------------
  def setLoops( self, loops ):
    """
    Set number of loops the player will play current file
    """
    self.loops= loops
  
  # ------------------------------------
  def setStartTime( self, timePosSec ):
    """
    Set start time for the media track to start playing.
    Whenever file is played it will start from the timePosSec position in the file.
    """
    self.startPos= timePosSec
  
  # ------------------------------------
  def setEndTime( self, timePos ):
    """
    Set end time for the media track.
    Whenever file is reached the endTime it will stop playing.
    """
    self.endPos= timePos
  
  # ------------------------------------
  def setVolume( self, volume ):
    """
    Set volume for the media track to play
    volume = [ 0..255 ]
    """
    self.volume= volume
  
  # ------------------------------------
  def getPictureSize( self ):
    """
    Whenever the file containing video is played, this function returns the actual picture size for the
    video part. It may be None if no video is found in the media file.
    Note: picture size may be unknown for up to 1 second after you call startPlayback() !
    """
    return self.pictureSize

  # ------------------------------------
  def getLength( self ):
    """
    Get length of the media file in seconds
    Note: length may be unknown for up to 1 second after you call startPlayback() !
    """
    return self.length

  # ------------------------------------
  def getMetaData( self ):
    """
    Get meta data from the media file as dictionary
    Note: metadata may be unknown for up to 1 second after you call startPlayback() !
    """
    return self.metaData

  # ------------------------------------
  def _hasQueue( self ):
    queue= 0
    if self.aindex!= -1:
      queue+= len( self.aDecodedFrames )
    if self.vindex!= -1:
      queue+= len( self.rawFrames )+ len( self.decodedFrames )
    
    return queue> 0
  
  # ------------------------------------
  def _getPTS( self ):
    if self.aindex== -1:
      if not self.aPTS:
        self.aPTS= time.time()
      
      return time.time()- self.aPTS
    
    if self.snd== None:
        return 0
    
    p= self.snd.getPosition()
    return p+ self.aDelta
  
  # ------------------------------------
  def _getSndLeft( self ):
    if self.snd== None:
      if self.aindex== -1:
        return 1
      else:
        return 0
    
    return self.snd.getLeft()
  
  # ------------------------------------
  def _readerLoop( self ):
    f= None
    try:
      while self.exitFlag== 0:
        if self.playingFile== None:
          time.sleep( 0.01 )
          continue
        
        self.frameNum= -1
        format= self.playingFile.split( '.' )[ -1 ].lower()
        # Initialize demuxer and read small portion of the file to have more info on the format
        dm= muxer.Demuxer( format )
        if type( self.playingFile )== str:
          try:
            f= open( self.playingFile, 'rb' )
          except:
            self.err.append( sys.exc_info()[1] )
            raise
        else:
          f= self.playingFile
        
        s= f.read( VIDEO_FILE_CHUNK )
        r= dm.parse( s )
        self.metaData= dm.getInfo()
        
        # This seek sets the seeking position already at the desired offset from the beginning
        if self.startPos:
          self.seekTo( self.startPos )
        
        # Setup video( only first matching stream will be used )
        self.err= []
        self.vindex= -1
        streams= filter( lambda x: x, dm.streams )
        for st in streams:
          if st and st[ 'type' ]== muxer.CODEC_TYPE_VIDEO:
            self._initVideo( st )
            self.vindex= list( streams ).index( st )
            break
        
        # Setup audio( only first matching stream will be used )
        self.aindex= -1
        self.aPTS= None
        for st in streams:
          if st and st[ 'type' ]== muxer.CODEC_TYPE_AUDIO:
            self._initAudio( st )
            self.aindex= list( streams ).index( st )
            break
        
        # Open current file for playing
        currentFile= self.playingFile
        if self.vindex>= 0:
          self._getVStreamParams( self.vindex, streams[ self.vindex ], r )
          self._getStreamLength( format, dm, f, r )
        
        # Play until no exit flag, not eof, no errs and file still the same
        while len(s) and len( self.err )== 0 and \
            self.exitFlag== 0 and self.playingFile and len( streams ) and \
            self.playingFile== currentFile:
          if self.isPaused():
            time.sleep( PAUSE_SLEEP )
            continue
          
          for d in r:
            if not self.playingFile:
              break
            
            # Seeking stuff
            if not self.seek in ( 0, SEEK_IN_PROGRESS ):
              f.seek( self.seek, 0 )
              self._resetAudio()
              self._resetVideo()
              self.rawFrames= []
              self.decodedFrames= []
              self.seek= SEEK_IN_PROGRESS
              break
            
            # See if we reached the end position of the video clip
            if self.endPos and self.getPTS()* 1000> self.endPos:
              # Seek at the end and close the reading loop instantly 
              f.seek( 0, 2 )
              break
            
            try:
              # Demux file into streams
              if d[ 0 ]== self.vindex:
                #print 'V', self.seek, self.aindex
                # Process video frame
                self._processVideoFrame( d )
                #print 'VV'
              elif d[ 0 ]== self.aindex and self.seek!= SEEK_IN_PROGRESS:
                #print 'A'
                # Decode and play audio frame
                self._processAudioFrame( d )
                #print 'AA'
            except:
              traceback.print_exc()
              raise
          
          # Read next encoded chunk and demux it
          s= f.read( 10000 )
          r= dm.parse( s )
        
        if f: f.close()
        # Close current file when error detected
        if len( self.err ):
          self.stopPlayback( False )
          
        # Wait until all frames are played
        while self._hasQueue()> 0 and self.isPlaying():
          print len( self.decodedFrames ), len( self.rawFrames ), len( self.aDecodedFrames )
          self._processVideoFrame( None, True )
          self._processAudio()
        
        self._resetAudio()
        self._resetVideo()
        
        self.loops-= 1
        if self.loops!= 0:
          continue
        
        self.playingFile= None
    except:
      traceback.print_exc()

