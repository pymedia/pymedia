##    audio - Part of the pymedia package. Audio module supporting utilities.
##        Processes audio files and provides some rendering services
##    
##    Copyright (C) 2002-2003  Dmitry Borisov
##
##    This library is free software; you can redistribute it and/or
##    modify it under the terms of the GNU Library General Public
##    License as published by the Free Software Foundation; either
##    version 2 of the License, or (at your option) any later version.
##
##    This library is distributed in the hope that it will be useful,
##    but WITHOUT ANY WARRANTY; without even the implied warranty of
##    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
##    Library General Public License for more details.
##
##    You should have received a copy of the GNU Library General Public
##    License along with this library; if not, write to the Free
##    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
##
##    Dmitry Borisov

import cddb, copy, aplayer, cache, re
import pygame, pycdda, pysound, pympg

from __main__ import *
from pymedia import menu

VOLUME_DELTA= 0x7ff
LEFT_C=(0,0)
FRAME_SIZE= 4
HEADER_CHUNK_SIZE= 32000
DEFAULT_MAIN_AREA_POS= ( 200,10,260,160 )
RECURSIVE= 1
SINGLE_FILE= 2
PARENT_DIR= [
  { 'name': '..',
    'title': '< .. >',
    'isDir': 1,
    'parent': None,
    'root': 0,
    'removable': 0,
    'album': None,
    'author': None,
    'tracknum': None,
    'children': None},]

TITLE_FONT= 'titleFont'
ALBUM_FONT= 'albumFont'
ARTIST_FONT= 'artistFont'
LENGTH_FONT= 'lengthFont'

playLists= aplayer.PlayLists()
player= aplayer.Player()
cache= cache.FileCache()

# ------------------------------------------------------------------------------------------------------
def processUniKeys( key ):
  """
    Process default keys for the audio module
  """
  if key== pygame.K_PAUSE:
    if player.isPlaying():
      player.pausePlayBack()
    elif player.isPaused():
      player.unpausePlayBack()
  elif key== pygame.K_F2:
    if ( player.isPlaying() or player.isPaused() ) and pysound.isMute()== 0 and pysound.getVolume()> VOLUME_DELTA:
      pysound.setVolume( pysound.getVolume()- VOLUME_DELTA )
  elif key== pygame.K_F4:
    if ( player.isPlaying() or player.isPaused() ) and pysound.isMute()== 0 and pysound.getVolume()< 0xffff- VOLUME_DELTA:
      pysound.setVolume( pysound.getVolume()+ VOLUME_DELTA )
  elif key== pygame.K_F3:
    if player.isPlaying() or player.isPaused():
      pysound.setMute( pysound.isMute()== 0 )

# ------------------------------------------------------------------------------------------------------
def getDeviceLabel( deviceName, default ):
  """
    Return device's label. Supported only on NT clone oses.
  """
  if os.name in ( 'nt', 'dos' ): 
    volInfo= os.popen( 'dir %s' % deviceName ).readlines()
    i= string.find( volInfo[ 0 ], ' is ' )
    if i>= 0:
      return string.strip( volInfo[ 0 ][ i+ 4: ] )
  
  return default

# ------------------------------------------------------------------------------------------------------
def refreshFiles( area ):
  """
    Refresh files specified in the area.
    It may refresh root(s) directory or anything below the root directory as well.
  """
  # Do we have something to refresh ?
  global cache
  fileWrapper= area.getWrapper()
  items= fileWrapper.items()
  if len( items )== 0:
    return
  
  if items[ 0 ][ 'name' ]== '..':
    del( items[ 0 ] )
  
  try:
    # If we are in the root, just refresh the root
    dirs= map( lambda x: x[ 'root' ], items )
    parent= None
    isRoot= 1
  except:
    # Just process regular files
    dirs= map( lambda x: x[ 'name' ], items )
    parent= items[ 0 ][ 'parent' ]
    isRoot= 0
    
  # Remove all items in the list
  for d in dirs:
    cache.delFile( d, parent )
    
  # If we are in the root, just refresh the root
  filter= area.getRenderer().getFilter()
  if isRoot:
    items= cache.setRoot( dirs )
    fileWrapper.setItems( items, filter )
  else:
    processWholeDir( parent, filter )
    items= parent[ 'children' ]
    items= PARENT_DIR+ items
    
    fileWrapper.setItems( items, filter )
  
  fileWrapper.setChanged( 1 )

# ------------------------------------------------------------------------------------------------------
def addToPlayList( files, filter, flag ):
  """
    Add file/directory to the playlist. If the file already exists, then do nothing.
  """
  if files== None:
    return
  
  # Some speedup may happen here
  plAdd= player.getPlayList()[ 0 ].addFile
  for file in files:
    if file[ 'isDir' ]:
      if flag & RECURSIVE:
        try: children= file[ 'children' ]
        except: continue
        
        addToPlayList( children, filter, flag )
    else:
      res, index= plAdd( file )
      if player.isPlaying()== 0 or ( flag & SINGLE_FILE and res== 0 ):
        player.setCurrentFileIndex( index )
        player.startPlayback()

# -----------------------------------------------------------------
def processWholeDir( file, extensions ):
  """
    Process directory and everything under it and create a cache structure
  """
  global cache
  extensionsLower= map( lambda x: string.lower( x ), extensions )
  try:
    files= os.listdir( cache.getPathName( file ) )
  except:
    file[ 'processing' ]= 0
    traceback.print_exc()
  
  for line in files:
    if os.path.isdir( os.path.join( cache.getPathName( file ), line )):
      # Process dir
      f= cache.addFile( file, line, 1 )
      f[ 'album' ]= None
      f[ 'author' ]= None
      f[ 'tracknum' ]= None
      f[ 'processing' ]= 0
      processWholeDir( f, extensions )
    else:
      # We have a file here
      if extensions:
        ext= string.split( line, '.' )[ -1 ]
        if string.lower( ext ) not in extensionsLower:
          continue
      
      f= cache.addFile( file, line, 0 )

# ****************************************************************************************************
class AudioFileHelper( menu.MenuHelper ):
  """
    Generic class to obtain information about the audio file. The following information can be gathered:
    - album   Name of the album for the track
    - author  Name of the author for the track
    - title   Track title
    - device  Device on which track resides( if it is a removable device )
    - tracknum  Track number in album
    - uncompressed  Whether track is compressed
    - isCDDA  Whether track is in cdda format
    - length  Track's length
    - sectors  Sectors( from, to ) where the track is stored( cdda format )
  """
  # -----------------------------------------------------------------
  # Read settings from the file
  def __init__( self, rect, params ):
    """
      Generic class constructor.
      Accepts parameters and rectangle where it should appear on a screen( relative ).
    """
    menu.MenuHelper.__init__( self, rect, params )
    # Load fonts and icons
    self.folderIcons= self.loadIcon( 'folderIcons', ( 'folder_0.gif', 'folder_1.gif' ) )
    self.fileIcons= self.loadIcon( 'fileIcons', ( 'file_0.bmp', 'file_1.bmp' ) )
    self.cddaIcon= self.loadIcon( 'cddaIcon', 'audio.gif' )
    self.emptyIcon= self.loadIcon( 'emptyIcon', 'empty.gif' )
    self.playingIcon= self.loadIcon( 'playingIcon', 'playing.gif' )
    self.playlistIcon= self.loadIcon( 'playlistIcon', 'playlist.gif' )
    self.extensions= pympg.extensions
    self.titleFont, self.albumFont, self.artistFont, self.lengthFont= \
      map( lambda x: self.loadFont( x ), ( TITLE_FONT, ALBUM_FONT, ARTIST_FONT, LENGTH_FONT ) )
  
  # -----------------------------------------------------------------
  def getFilter( self ):
    """
      Return audio files extensions that can be processed by this class.
    """
    return self.extensions
  
  # -----------------------------------------------------------------
  def processAudioDir( self, file ):
    """
      Take audio directory from cache and create a list of
      tracks based on information from the directory itself
    """
    global cache
    # Scan all disk info we have so far
    info= file[ 'trackInfo' ]
    toc= file[ 'device' ].getTOC()
    for track in info:
      if track[ :6 ]== 'TTITLE':
        s= info[ track ]
        trackNum= int( track[ 6: ] )+ 1
        f= cache.addFile( file, s, 0, trackNum )
        f[ 'album' ]= file[ 'title' ]
        f[ 'author' ]= file[ 'author' ]
        f[ 'title' ]= s
        f[ 'device' ]= file[ 'device' ]
        f[ 'tracknum' ]= trackNum
        f[ 'uncompressed' ]= 1
        f[ 'isCDDA' ]= 1
        f[ 'length' ]= toc[ trackNum- 1 ][ 1 ]/ 75
        f[ 'sectors' ]= toc[ trackNum- 1 ]
        f[ 'bitRate' ]= 999
  
  # -----------------------------------------------------------------
  def processDir( self, param ):
    """
      Proces directory and everything under it. If the directory is an audio disk,
      it will call cddb to recognize if possible. 
    """
    global cache
    file= param[ 0 ]
    text= None
    
    if file[ 'processing' ]== 0:
      return
    
    self.setChanged( 1 )
    if file.has_key( 'device' ):
      # Process device directories
      device= file[ 'device' ]
      if device.isReady()== 0:
        print 'Device %s is not ready to read' % cache.getPathName( file )
        file[ 'title' ]= 'No disc( %s )' % cache.getPathName( file )
        file[ 'processing' ]= 0
        return
      else:
        print 'Device %s is about to be read' % cache.getPathName( file )
        if file.has_key( 'isAudio' ) and file[ 'isAudio' ]== 1:
          print 'File %s is an audio file...' % cache.getPathName( file )
          # Got audio disk
          cddbobj= cddb.CDDB( self.parentParams[ 'module' ].params[ 'params' ][ 'cddb' ] )
          diskInfo= device.getTOC()
          info= cddbobj.getDiscInfo( diskInfo )
          text= string.split( info[ 'DTITLE' ], '/' )
          file[ 'trackInfo' ]= info
          file[ 'title' ]= text[ 1 ]
          file[ 'author' ]= text[ 0 ]
          file[ 'length' ]= None
          self.processAudioDir( file )
          file[ 'processing' ]= 0
          return
        else:
          # get volume info
          file[ 'title' ]= getDeviceLabel( cache.getPathName( file ), 'No volume' )
    
    # Just data disk
    processWholeDir( file, self.extensions )
    file[ 'processing' ]= 0
  
  # -----------------------------------------------------------------
  def processFile( self, param ):
    """
      Process regular audio file. Reads information from the file if needed and get the
      following attributes in addition to the generic ones:
      - frequency
      - channels
      - bitrate
    """
    global cache
    file= param[ 0 ]
    f= None
    title= file[ 'name' ]
    album= artist= trackNum= None
    try:
      # Try to read some data to get extra info
      f= cache.open( file, 1 )
      title= os.path.split( title )[ -1 ]
    except:
      traceback.print_exc()
    
    if f!= None:
      dec= pympg.Decoder( cache.getExtension( file ) )
      s= f.read( HEADER_CHUNK_SIZE )
      kbPerSec, iFreqRate, sampleRate, channels, processedChunk= dec.convert2PCM( s )
      if len( processedChunk )> 0 or dec.hasHeader()> 0:
        # Assign parameters to the file
        file[ 'frequency' ]= iFreqRate
        file[ 'channels' ]= channels
        file[ 'bitrate' ]= kbPerSec
        
        # Hardcoding for mp3
        if not dec.hasHeader() and cache.getExtension( file )== 'mp3':
          f.seek( -128, 2 )
          dec= pympg.Decoder( 'mp3' )
          dec.convert2PCM( f.read( 128 ) )
        
      info= dec.getInfo()
      if kbPerSec== 0:
        file[ 'length' ]= float( info[ 'frames' ] ) * 0.02609
      else:
        f.seek( -1, 2 )
        file[ 'length' ]= f.tell()/ ( kbPerSec* 125 )
      
      f.close()
      
      if info[ 'title' ]!= '':
        title= info[ 'title' ]
        
      # Parse only when title is set
      if info.has_key( 'tracknum' ) and info[ 'tracknum' ]!= '':
        trackNum= info[ 'tracknum' ]
      else:
        # Try to extract track number from the file name( first two symbols )
        trackNum= file[ 'name' ][ :2 ]
        
      try:
        # Remove some redundant chars and split by space
        m= re.search( '\d*', trackNum )
        trackNum= int( m.group( 0 ) )
      except:
        trackNum= None
      
      album= info[ 'album' ]
      artist= info[ 'artist' ]
   
    file[ 'title' ]= translate( title )
    file[ 'album' ]= translate( album )
    file[ 'author' ]= translate( artist )
    file[ 'tracknum' ]= trackNum
    file[ 'processing' ]= 0
    self.setChanged( 1 )
  
  # -----------------------------------------------------------------
  def getNameIcon( self, title, album, artist, track ):
    """
      Renders the file / directory informarmation as a text with predefing font and based on
      information it gets from the file attributes
    """
    if track:
      title= ( '%02d - ' % track )+ title
    
    titleIcon= self.titleFont[ 0 ].render( title, 1, self.titleFont[ 1 ] )
   
    if album:
      albumIcon= self.albumFont[ 0 ].render( album, 1, self.albumFont[ 1 ] )
    if artist:
      artistIcon= self.artistFont[ 0 ].render( artist+ '-', 1, self.artistFont[ 1 ] )
      totalHeight= titleIcon.get_height()+ artistIcon.get_height()
    else:
      totalHeight= titleIcon.get_height()
   
    fileIcon= [ ( titleIcon, LEFT_C ) ]
    if artist:
      fileIcon.append( ( artistIcon, ( 0,titleIcon.get_height() )))
    if album:
      if artist:
        fileIcon.append( ( albumIcon, ( artistIcon.get_width(),titleIcon.get_height() )))
      else:
        fileIcon.append( ( albumIcon, ( 0,titleIcon.get_height() )))
    
    return totalHeight, fileIcon
  
# ****************************************************************************************
# Display position and some other music controls in the background in a small window
class BgPlayerDisplay( menu.GenericDisplay ):
  """
    The pure stateless displayer without any renderer. Displays the current playing
    time for a track.
  """
  # -----------------------------------------------------------------
  def __init__( self, rect, params, renderClass ):
    menu.GenericDisplay.__init__( self, rect, params, renderClass )
    self.currPos= -1
  
  # -----------------------------------------------------------------
  def render( self ):
    """
      Main method called by the main renderer
    """
    self.currPos= player.getPosition()
    if self.currPos!= -1:
      res= self.font[ 0 ].render( '%02d:%02d' % ( int( self.currPos / 60 ), self.currPos % 60 ), 1, self.font[ 1 ] )
      res.set_alpha( 150 )
      return [ ( res, self.rect[ :2 ] ) ]
    
    return []
  
  # -----------------------------------------------------------------
  def hasChanged( self ):
    """
      State tracker. If returns 1, then state has been changed.
    """
    global player
    return player.getPosition()!= self.currPos

# ****************************************************************************************
class PlayerDisplay( menu.GenericDisplay, AudioFileHelper ):
  """
    Displays full featured player
  """
  # -----------------------------------------------------------------
  def __init__( self, rect, params, renderClass ):
    """
      Constructor whic accepts parameters, rectangle size and renderer
    """
    menu.GenericDisplay.__init__( self, rect, params, renderClass )
    # Load fonts and icons
    self.titleFont, self.albumFont, self.artistFont, self.lengthFont= \
      map( lambda x: self.loadFont( x ), ( TITLE_FONT, ALBUM_FONT, ARTIST_FONT, LENGTH_FONT ) )
    self.mainArea= self.getParamList( 'mainArea', DEFAULT_MAIN_AREA_POS )
    self.auxArea= self.getParamList( 'auxArea', ( 10,10,180,160 ) )
    self.itemSize= self.mainArea[ 2: ]
    self.loadControls( ( 'prev', 'play', 'pause', 'stop', 'next' ) )
    self.loadVolumeIcons()
    self.currPos= -1
    self.currControl= -1

  # -----------------------------------------------------------------
  def loadVolumeIcons( self ):
    """
      Load volume icons
    """
    volumeSteps= self.getParam( 'volumeSteps', 10 )
    self.volumeIcons= map( lambda x: pygame.image.load( 'icons/vol_%d.gif' % x ), range( 1, volumeSteps+ 1 ))

  # -----------------------------------------------------------------
  def loadControls( self, names ):
    """
      Load controls for the player. Controls are: prev, stop, play etc...
    """
    faceIconName= self.getParam( 'face', '' )
    self.faceIcon= pygame.image.load( 'icons/%s' % faceIconName )

    controls= {}
    index= 0
    for name in names:
      controlIcon= self.getParam( name+ 'Button', '' )
      controlPos= self.getParamList( name+ 'ButtonPos', LEFT_C )
      controlIndex= int( self.getParam( name+ 'ButtonIndex', index ) )
      index+= 1
      controls[ controlIndex ]= [ name, controlPos ]
      icons= map( lambda x: pygame.image.load( 'icons/%s_%d.gif' % ( controlIcon, x ) ), range( 2 ))
      controls[ controlIndex ]+= icons
    
    # Sort controls by index
    indexes= controls.keys()
    indexes.sort()
    self.controls= map( lambda x: controls[ x ], indexes )
  
  # -----------------------------------------------------------------
  def showBitRate( self, yOffset, bitRate ):
    """
      Renders bitrate icon as text
    """
    bitRateIcon= self.lengthFont[ 0 ].render( '%3d kbps' % bitRate, 1, self.lengthFont[ 1 ] )
    return [( bitRateIcon, ( self.auxArea[ 0 ], self.auxArea[ 1 ]+ yOffset ) ) ]
  
  # -----------------------------------------------------------------
  def showVolume( self, yOffset ):
    """
      Show volume icons based on current volume level
    """
    volume= pysound.getVolume()
    step= ( int( volume )* len( self.volumeIcons ) )/ 65536
    xOffs= self.auxArea[ 2 ]- self.volumeIcons[ step ].get_width()
    return [( self.volumeIcons[ step ], ( xOffs, self.auxArea[ 1 ]+ yOffset ) )]
  
  # -----------------------------------------------------------------
  def showProgress( self, currPos, length ):
    """
      Show progress for the currently playing track
    """
    x, y, x1, y1= ( int( currPos / 60 ), currPos % 60, int( length / 60 ), length % 60 )
    lengthIcon= self.lengthFont[ 0 ].render( '%02d:%02d / %02d:%02d' % ( x, y, x1, y1 ), 1, self.lengthFont[ 1 ] )
    return [( lengthIcon, self.auxArea[ :2 ] )]
      
  # -----------------------------------------------------------------
  def renderInfo( self ):
    """
      Render complete set of information to be shown on a player face.
    """
    global player
    
    yOffs= 20
    xOffs= 40
    i= player.getCurrentFileIndex()
    self.currPos= player.getPosition()
    if i== -1:
      # Nothing is playing
      res= []
      height, res1= self.getNameIcon( 'Nothing is playing', None, None, None )
    else:
      # Display file information( including the position, length and bitrate )
      playingFile= player.getPlayList()[ 0 ].getFile( i )
      try:
        length, title, album, artist, bitRate= \
          playingFile[ 'length' ], playingFile[ 'title' ], playingFile[ 'album' ], playingFile[ 'author' ], playingFile[ 'bitrate' ]
      except:
        # The file is yet processed, submit it for processing
        length, title, album, artist, bitRate = 0, playingFile[ 'name' ], None, None, 0
        if playingFile[ 'isDir' ]== 0:
          eventQueue.put( ( 'EXECUTE', self.processFile, playingFile ) )
        else:
          eventQueue.put( ( 'EXECUTE', self.processDir, playingFile ) )
        
      if self.currPos== -1:
        currPos= length
      else:
        currPos= self.currPos
        
      res= self.showProgress( currPos, length )
      yOffset= res[ 0 ][ 0 ].get_height()
      res+= self.showBitRate( yOffset, bitRate )
      res+= self.showVolume( yOffset )
      
      track= None
      if playingFile.has_key( 'tracknum' ):
        track= playingFile[ 'tracknum' ]
      
      height, res1= self.getNameIcon( title, album, artist, track )
      res1= menu.clipIcon( res1, self.mainArea[ 2: ] )
    
    res+= self.adjustPos( res1, self.mainArea[ :2 ] )
    return res
  
  # -----------------------------------------------------------------
  def renderControls( self ):
    """
      Render control buttons
    """
    controls= [ ( self.faceIcon, LEFT_C ) ]
    for control in self.controls:
      controls.append( ( control[ ( self.activeItem== self.controls.index( control ) )+ 2 ], control[ 1 ] ) )
    
    return controls
    
  # -----------------------------------------------------------------
  def render( self ):
    """
      Main render call which gathers all controls and texts together
    """
    res= self.renderControls()
    res+= self.renderInfo()
    menu.MenuHelper.setChanged( self, 0 )
    return self.adjustPos( res, self.rect[ :2 ] )
  
  # -----------------------------------------------------------------
  def processControl( self, index ):
    """
      Process control actions
    """
    name= self.controls[ self.activeItem ][ 0 ]
    if name== 'prev':
      res= player.prevFile()
    elif name== 'next':
      res= player.nextFile()
    elif name== 'stop':
      res= player.stopPlayback()
    elif name== 'play':
      res= player.startPlayback()
    elif name== 'pause':
      if player.isPaused():
        res= player.unpausePlayback()
      else:
        res= player.pausePlayback()
    
    if res== 1:
      menu.MenuHelper.setChanged( self, res )
  
  # -----------------------------------------------------------------
  def processKey( self, key ):
    """
      Process player's control actions. It only has left, right, return
    """
    if key== pygame.K_LEFT:
      # Go to the previous control
      if self.activeItem!= 0:
        self.activeItem-= 1
        menu.MenuHelper.setChanged( self, 1 )
    elif key== pygame.K_RIGHT:
      # Go to the next control
      if self.activeItem!= len( self.controls )- 1:
        self.activeItem+= 1
        menu.MenuHelper.setChanged( self, 1 )
    elif key== pygame.K_RETURN:
      # Execute control
      self.processControl( self.activeItem )
    else:
      self.execute( 'onKeyPress', key, None )
  
  # -----------------------------------------------------------------
  def hasChanged( self ):
    """
      Have we had any changes in controls or player itself ?
    """
    return player.getPosition()!= self.currPos or menu.MenuHelper.hasChanged( self )

# ****************************************************************************************************
class AudioFileFolderItem( AudioFileHelper ):
  """
    Folder display class.
    Displays all files and folders in a folder style way. All the details will be specified
    It is most common way of seeing your music...
  """
  # -----------------------------------------------------------------
  def __init__( self, rect, params ):
    """
      Constructor.
    """
    AudioFileHelper.__init__( self, rect, params )
    self.extFont= self.loadFont( 'extensionFont' )
    self.iconSize= self.getParamList( 'iconSize', ( 72, 72 ) )
    self.itemSize= ( rect[ 2 ], 100 )
    # Identify max icon size
    self.maxIconSize= ( self.itemSize[ 1 ], self.itemSize[ 1 ] )
    self.itemsWrapper= None
    self.history= []
  
  # -----------------------------------------------------------------
  def processFolderIcon( self, file ):
    """
      Render folder/directory icon.
      Load pictures from the directory if possible or just display the folder icon.
    """
    if file[ 'isDir' ]== 1:
      fileName= cache.getPathName( file )
      self.changed= 1
      try:
        files= os.listdir( fileName )
      except:
        traceback.print_exc()
        return
        
      for f in files:
        f1= string.split( f, '.' )
        if len( f1 )> 1 and string.upper( f1[ -1 ] ) in( 'JPG', 'BMP', 'GIF' ):
          icon= pygame.image.load( os.path.join( fileName, f ) )
          # Resize the icon
          file[ 'icon' ].append( pygame.transform.scale( icon, self.iconSize ))
    else:
      # Process file extension
      extIcon= self.extFont[ 0 ].render( cache.getExtension( file ), 1, self.extFont[ 1 ] )
      extIcon.set_alpha( 200 )
      file[ 'icon' ].append( extIcon )
      
    self.setChanged( 1 )
    
  # -----------------------------------------------------------------
  def processIcon( self, params ):
    """
      Process icon for any type of file in a background.
      If it is an audio file, put some icons to indicate that.
    """
    global cache
    file= params[ 0 ]
    if not file.has_key( 'removable' ):
      self.processFolderIcon( file )
      return
      
    if file.has_key( 'isAudio' ) and file[ 'isAudio' ]== 1:
      file[ 'icon' ].append( self.cddaIcon )
    else:
      if file.has_key( 'device' ):
        if file[ 'device' ].isReady():
          self.processFolderIcon( file )
        else:
          file[ 'icon' ].append( self.emptyIcon )
    
    self.setChanged( 1 )
  
  # -----------------------------------------------------------------
  def drawIcon( self, file, isFocused ):
    """
      Draw icons for a generic file.
      Submit request for the additional information if needed.
    """
    # Clear out the icon place
    global eventQueue
    try:
      icon= file[ 'icon' ]
      ext= file[ 'ext' ]
    except:
      if file[ 'isDir' ]:
        ext= 'dir'
        icon= copy.copy( self.folderIcons )
      else:
        icon= copy.copy( self.fileIcons )
        ext= None
      
      file[ 'icon' ]= icon
      file[ 'ext' ]= ext
      eventQueue.put( ( 'EXECUTE', self.processIcon, file ) )
    
    # Draw icons at the left side of the list
    resIcon= [ ( icon[ isFocused ], LEFT_C ) ]
    yOffset= 5
    if len( icon )> 2 and icon[ 2 ]:
      resIcon.append( ( icon[ 2 ], ( yOffset, 13 ) ) )
      yOffset+= icon[ 2 ].get_height()+ 3
    
    if file[ 'isDir' ]== 0:
      if file.has_key( 'length' ):
        length= '%02d:%02d' % ( int( file[ 'length' ]/60 ), int( file[ 'length' ] % 60 ))
        lengthIcon= self.lengthFont[ 0 ].render( length, 1, self.lengthFont[ 1 ] )
        resIcon.append( ( lengthIcon, ( ( icon[ isFocused ].get_width()- lengthIcon.get_width() )/2,yOffset) ) )
      
      if file[ 'ext' ]:
        resIcon.append( ( file[ 'ext' ], ( 2, 2 ) ) )
    
    return resIcon
  
  # -----------------------------------------------------------------
  def drawItem( self, pos, isFocused ):
    """
      Get the properties of the generic file.
      It may submit background request if needed.
    """
    global eventQueue
    file= self.itemsWrapper.items()[ pos ]
    # Find out whether it is a directory or file
    try:
      title= file[ 'title' ]
    except:
      try:
        i= file[ 'removable' ]
        title= u'Removable device( %s )' % cache.getPathName( file )
      except:
        title= translate( file[ 'name' ] )
      
      file[ 'title' ]= title
      file[ 'author' ]= None
      file[ 'album' ]= None
      file[ 'tracknum' ]= None
      file[ 'processing' ]= 1
      if file[ 'isDir' ]== 0:
        eventQueue.put( ( 'EXECUTE', self.processFile, file ) )
      else:
        eventQueue.put( ( 'EXECUTE', self.processDir, file ) )
    
    # Draw icons at the left side of the item
    resIcon= self.drawIcon( file, isFocused )
    
    folderWidth= self.maxIconSize[ 0 ]+ FRAME_SIZE* 2
    width= self.getItemSize()[ 0 ]- folderWidth
    extraIcon= None
    if self.isPlaying( file ):
      # Add extra icon at the end of the text when playing
      extraIcon= self.playingIcon
    elif self.isSelected( file ):
      # Add extra icon at the end of the text when is in the playlist
      extraIcon= self.playlistIcon
    
    # Adjust extra icon if exists
    if extraIcon:
      width-= extraIcon.get_width()
      resIcon+= [ ( extraIcon, ( folderWidth+ width, ( self.getItemSize()[ 1 ]- extraIcon.get_height() )/2 ) ) ]
    
    # Center text into rect
    height, text= self.getNameIcon( file[ 'title' ], file[ 'album' ], file[ 'author' ], file[ 'tracknum' ] )
    text= menu.clipIcon( text, ( width, self.getItemSize()[ 1 ] ))
    resIcon+= self.adjustPos( text, ( folderWidth, ( self.getItemSize()[ 1 ]- height )/2 ) )
    return resIcon
  
  # -----------------------------------------------------------------
  def setItemsWrapper( self, itemsWrapper ):
    self.itemsWrapper= itemsWrapper
  
  # -----------------------------------------------------------------
  def hasChanged( self ):
    if self.itemsWrapper:
      return self.itemsWrapper.hasChanged()
  
  # -----------------------------------------------------------------
  def setChanged( self, changed ):
    if self.itemsWrapper:
      self.itemsWrapper.setChanged( changed )
  
  # -----------------------------------------------------------------
  def isSelected( self, file ):
    # Verify whether we are in the playlist
    return player.getPlayList()[ 0 ].getFileNum( file )!= -1
  
  # -----------------------------------------------------------------
  def isPlaying( self, file ):
    """
      Whether the file is currently playing
    """
    global cache
    i= player.getCurrentFileIndex()
    if i!= -1:
      playingFile= player.getPlayList()[ 0 ].getFile( i )
      if cache.getPathName( file )== cache.getPathName( playingFile ):
        return 1
    
    return 0
  
  # -----------------------------------------------------------------
  def processKey( self, pos, key ):
    """
      Process generic keys from the parent control
    """
    global cache
    activeItem= None
    item= self.itemsWrapper.items()[ pos ]
    
    if item[ 'name' ]== '..' and key== pygame.K_RETURN:
      # Assume we are at ..
      key= pygame.K_BACKSPACE
    
    if key== pygame.K_RETURN:
      if item[ 'isDir' ]:
        # Wait until we process directory( may take awhile )
        startTime= time.time()
        while item[ 'processing' ]== 1:
          time.sleep( 0.01 )
          if time.time()- startTime> 1:
            return
        
        self.history.append( ( self.itemsWrapper.items(), pos, item[ 'title' ] ) )
        children= cache.getChildren( item )
        if children== None:
          children= []
        
        self.itemsWrapper.setItems( PARENT_DIR+ children, self.getFilter() )
        activeItem= 0
      else:
        addToPlayList( ( item, ), None, SINGLE_FILE )
        # Set changed for the whole screen
        self.setChanged( 2 )
    elif key== pygame.K_BACKSPACE:
      if len( self.history ):
        self.itemsWrapper.setItems( self.history[ -1 ][ 0 ], self.getFilter() )
        activeItem= self.history[ -1 ][ 1 ]
        del( self.history[ -1 ] )
    
    self.execute( 'onKeyPress', key, item )
    return activeItem

# ****************************************************************************************************
class PlayListItem( menu.MenuItem ):
  """
    Regular playlist item renderer for a menu purposes
  """
  # -----------------------------------------------------------------
  def __init__( self, rect, params ):
    """
      ctor( rect, params ) -> PlayListItem
      
      Create basic renderer for a plain list of files in a playlist
    """
    menu.MenuItem.__init__( self, rect, params )
  
  # -----------------------------------------------------------------
  def processKey( self, itemPos, key ):
    """
      processKey( itemPos, key ) ->
    """
    item= self.itemsWrapper.items()[ itemPos ]
    paramName= 'onKeyPress'
    if key== pygame.K_RETURN:
      paramName= 'onEnter'
    
    self.execute( paramName, key, item )
  
  # -----------------------------------------------------------------
  def drawItem( self, itemPos, isFocused ):
    item= self.itemsWrapper.items()[ itemPos ]
    # See if current playlist item is a default. If it is then, write * at the end
    s= item[ 'caption' ]
    pl= player.getPlayList()
    if playLists.getName( pl[ 0 ] )== s:
      s= '*'+ s
      
    # Find out whether it is a directory or file
    res= []
    # Select current item if needed
    if isFocused== 1:
      res.append( ( self.stripeIcon, LEFT_C ) )
    
    res.append( ( self.font[ 0 ].render( s, 1 , self.font[ 1 ] ), LEFT_C ) )
    return res

# ****************************************************************************************************
class PlaylistPlainItem( AudioFileFolderItem ):
  # -----------------------------------------------------------------
  def __init__( self, rect, params ):
    AudioFileFolderItem.__init__( self, rect, params )
  
  # -----------------------------------------------------------------
  def processKey( self, pos, key ):
    global cache
    item= None
    if pos> -1:
      item= self.itemsWrapper.items()[ pos ]
    
    if key== pygame.K_RETURN:
      # Start playing selected file
      player.setPlayList( ( self.itemsWrapper, pos ) ); 
      player.startPlayback()
    else:
      self.execute( 'onKeyPress', key, pos )
      return
    
    self.setChanged( 1 )
  
  # -----------------------------------------------------------------
  def isSelected( self, file ):
    return 0

# ***********************************************************************
# Run some tests against basic list functionality
if __name__== '__main__':
  pl= BgPlayerDisplay( ( 100, 100, 200 , 200 ), {}, None )
  