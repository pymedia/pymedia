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

import cddb, copy, aplayer, cache, re, pygame
import pymedia, mpg

from __main__ import *
from pymedia import menu

VOLUME_DELTA= 0x7ff
MAX_VOLUME= 0xffff
LEFT_C=(0,0)
FRAME_SIZE= 4
HEADER_CHUNK_SIZE= 32000
DEFAULT_MAIN_AREA_POS= ( 200,10,260,160 )
RECURSIVE= 1
SINGLE_FILE= 2

TITLE_FONT= 'titleFont'
ALBUM_FONT= 'albumFont'
ARTIST_FONT= 'artistFont'
LENGTH_FONT= 'lengthFont'

ALBUM_KEY= 'album'
ARTIST_KEY= 'artist'
BITRATE_KEY= 'bitrate'
LENGTH_KEY= 'length'
FREQUENCY_KEY= 'frequency'
CHANNELS_KEY= 'channels'

ICON_EXT= ( 'JPG', 'BMP', 'GIF' )

# ------------------------------------------------------------------------------------------------------
def processUniKeys( key ):
  """
    Process default keys for the audio module
  """
  p= aplayer.player
  if key== pygame.K_PAUSE:
    if p.isPlaying():
      p.pausePlayBack()
    elif p.isPaused():
      p.unpausePlayBack()
  elif key== pygame.K_F2:
    if ( p.isPlaying() or p.isPaused() ) and p.isMute()== 0 and p.getVolume()> VOLUME_DELTA:
      p.setVolume( p.getVolume()- VOLUME_DELTA )
  elif key== pygame.K_F4:
    if ( p.isPlaying() or p.isPaused() ) and p.isMute()== 0 and p.getVolume()< MAX_VOLUME- VOLUME_DELTA:
      p.setVolume( p.getVolume()+ VOLUME_DELTA )
  elif key== pygame.K_F3:
    if p.isPlaying() or p.isPaused():
      p.setMute( p.isMute()== 0 )

# ------------------------------------------------------------------------------------------------------
def getDeviceLabel( deviceName, default ):
  """
    Return device's label. Supported only on NT clone oses.
  """
  if os.name in ( 'nt', 'dos' ): 
    volInfo= os.popen( u'dir %s' % deviceName ).readlines()
    i= volInfo[ 0 ].find( u' is ' )
    if i>= 0:
      return volInfo[ 0 ][ i+ 4: ].strip()
  
  return default

# ------------------------------------------------------------------------------------------------------
def addToPlayList( files, filter, flag ):
  """
    Add file/directory to the playlist. If the file already exists, then do nothing.
  """
  if files== None:
    return
  
  # Some speedup may happen here
  plAdd= aplayer.player.getPlayList()[ 0 ].addFile
  for file in files:
    if file[ cache.DIR_KEY ]:
      if flag & RECURSIVE:
        try: children= file[ cache.CHILDREN_KEY ]
        except: continue
        
        addToPlayList( children, filter, flag )
    else:
      res, index= plAdd( file )
      if aplayer.player.isPlaying()== 0 or ( flag & SINGLE_FILE and res== 0 ):
        aplayer.player.setCurrentFileIndex( index )
        aplayer.player.startPlayback()

# -----------------------------------------------------------------
def processWholeDir( file, extensions ):
  # Scan all disk info into the cache
  lastDir= None
  s= ( u'dir /S /N "%s"' % cache.cache.getPathName( file ) ).encode( 'cp1251' )
  lines= os.popen( s ).readlines()
  for line in lines:
    # Convert from DOS into unicode
    line= translate( line, [ 'cp866' ] )
    if line[ 25: 28 ]== u'DIR':
      # Process dir
      if line[ 39 ]!= '.':
        f= cache.cache.addFile( lastDir, string.strip( line[ 39: ] ), 1 )
        f[ cache.PROCESSED_KEY ]= 2
    elif line[ :9 ]== u'Directory':
      fName= line[ 13: ].strip()
      lastDir= cache.cache.getFile( fName )
    else:
      date= line[ :9 ]
      if len( date.split( '/' ) )== 3:
        # We have a file here
        fName= line[ 39: ].strip()
        if filter:
          fileType= fName.split( '.' )
          ext= fileType[ -1 ].lower()
          if ext not in extensions:
            continue
        
        f= cache.cache.addFile( lastDir, fName, 0 )

"""
Slow version of processWholeDir
# -----------------------------------------------------------------
def processWholeDir( file, extensions ):
  "
    Process directory and everything under it and create a cache structure
  "
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
      processWholeDir( f, extensions )
    else:
      # We have a file here
      if extensions:
        ext= string.split( line, '.' )[ -1 ]
        if string.lower( ext ) not in extensionsLower:
          continue
      
      f= cache.addFile( file, line, 0 )
"""

# ****************************************************************************************
# Simple popup which wait until 
class WaitPopup( menu.GenericChoiceDialog ):
  # -------------------------------------------------------------
  def processKey( self, key ):
    pass
  
  # -------------------------------------------------------------
  def hasChanged( self ):
    if self.returnParams[ 0 ][ cache.PROCESSED_KEY ]== 2:
      # just exit 
      self.exitDialog( 0 )
    
    return 0

# ****************************************************************************************************
class AudioFileHelper( menu.MenuHelper ):
  """
    Generic class to obtain information about the audio file. The following information can be gathered:
    - album   Name of the album for the track
    - artist  Name of the artist for the track
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
  def __init__( self, rect, attributes, params ):
    """
      Generic class constructor.
      Accepts parameters and rectangle where it should appear on a screen( relative ).
    """
    menu.MenuHelper.__init__( self, rect, attributes, params )
    # Load fonts and icons
    self.folderIcons= self.loadIcon( 'folderIcons', ( 'folder_0'+ menu.DEFAULT_EXT, 'folder_1'+ menu.DEFAULT_EXT ) )
    self.fileIcons= self.loadIcon( 'fileIcons', ( 'file_0'+ menu.DEFAULT_EXT, 'file_1'+ menu.DEFAULT_EXT ) )
    self.cddaIcon= self.loadIcon( 'cddaIcon', 'audio'+ menu.DEFAULT_EXT )
    self.emptyIcon= self.loadIcon( 'emptyIcon', 'empty'+ menu.DEFAULT_EXT )
    self.playingIcon= self.loadIcon( 'playingIcon', 'playing'+ menu.DEFAULT_EXT )
    self.playlistIcon= self.loadIcon( 'playlistIcon', 'playlist'+ menu.DEFAULT_EXT )
    self.extensions= mpg.extensions
    self.titleFont, self.albumFont, self.artistFont, self.lengthFont= \
      self.loadFont( ( TITLE_FONT, ALBUM_FONT, ARTIST_FONT, LENGTH_FONT ) )
  
  # -----------------------------------------------------------------
  def getFilter( self ):
    """
      Return audio files extensions that can be processed by this class.
    """
    return self.extensions
  
  # -----------------------------------------------------------------
  def processAudioDir( self, file, info ):
    """
      Take audio directory from cache and create a list of
      tracks based on information from the directory itself
    """
    # Scan all disk info we have so far
    toc= file[ cache.DEVICE_KEY ].getTOC()
    for track in info:
      if track[ :6 ]== 'TTITLE':
        s= info[ track ]
        trackNum= int( track[ 6: ] )+ 1
        f= cache.cache.addFile( file, s, 0, trackNum )
        f[ cache.PROCESSED_KEY ]= 2
        f[ ALBUM_KEY ]= file[ cache.TITLE_KEY ]
        f[ ARTIST_KEY ]= file[ ARTIST_KEY ]
        f[ cache.TITLE_KEY ]= s
        f[ cache.DEVICE_KEY ]= file[ cache.DEVICE_KEY ]
        f[ cache.TRACK_KEY ]= trackNum
        f[ cache.CDDA_KEY ]= 1
        f[ LENGTH_KEY ]= toc[ trackNum- 1 ][ 1 ]/ 75
        f[ BITRATE_KEY ]= 999
        f[ CHANNELS_KEY ]= 2
  
  # -----------------------------------------------------------------
  def processDir( self, param ):
    """
      Proces directory and everything under it. If the directory is an audio disk,
      it will call cddb to recognize if possible. 
    """
    file= param[ 0 ]
    text= None
    
    # Just in case 
    if file[ cache.PROCESSED_KEY ]!= 0:
      return
    
    # File is processing
    file[ cache.PROCESSED_KEY ]= 1
    try:
      self.setChanged( 1 )
      if file.has_key( cache.DEVICE_KEY ):
        # Process device directories
        device= file[ cache.DEVICE_KEY ]
        if device.isReady()== 0:
          print 'Device %s is not ready to read' % cache.cache.getPathName( file )
          file[ cache.TITLE_KEY ]= 'No disc( %s )' % cache.cache.getPathName( file )
          return
        else:
          print 'Device %s is about to be read' % cache.cache.getPathName( file )
          if file.has_key( cache.CDDA_KEY ):
            print 'File %s is an audio file...' % cache.cache.getPathName( file )
            # Got audio disk
            cddbobj= cddb.CDDB( self.attributes[ 'module' ].params[ 'cddb' ] )
            diskInfo= device.getTOC()
            info= cddbobj.getDiskInfo( diskInfo )
            text= info[ 'DTITLE' ].split( '/' )
            file[ cache.TITLE_KEY ]= text[ 1 ]
            file[ ARTIST_KEY ]= text[ 0 ]
            file[ ALBUM_KEY ]= None
            file[ LENGTH_KEY ]= info[ LENGTH_KEY ]
            self.processAudioDir( file, info )
            return
          else:
            # get volume info
            file[ cache.TITLE_KEY ]= getDeviceLabel( cache.cache.getPathName( file ), 'No volume' )
      
      # Just data disk
      #print 'Process whole dir ', file[ cache.NAME_KEY ]
      processWholeDir( file, self.extensions )
    finally:
      file[ cache.PROCESSED_KEY ]= 2
  
  # -----------------------------------------------------------------
  def getFileParams( self, f, file ):    
    dec= mpg.Decoder( cache.cache.getExtension( file ) )
    s= f.read( HEADER_CHUNK_SIZE )
    kbPerSec, iFreqRate, sampleRate, channels, processedChunk= dec.decode( s )
    if len( processedChunk )> 0 or dec.hasHeader()> 0:
      # Assign parameters to the file
      file[ FREQUENCY_KEY ]= iFreqRate
      file[ CHANNELS_KEY ]= channels
      file[ BITRATE_KEY ]= kbPerSec
      
      # Hardcoding for mp3
      if not dec.hasHeader() and cache.cache.getExtension( file )== 'mp3':
        f.seek( -128, 2 )
        dec= mpg.Decoder( 'mp3' )
        dec.decode( f.read( 128 ) )
      
    return dec.getInfo()
  
  # -----------------------------------------------------------------
  def processFile( self, param ):
    """
      Process regular audio file. Reads information from the file if needed and get the
      following attributes in addition to the generic ones:
      - frequency
      - channels
      - bitrate
    """
    file= param[ 0 ]
    f= None
    title= file[ cache.NAME_KEY ]
    album= artist= trackNum= None
    length= 0
    try:
      # Try to read some data to get extra info( read directly without caching )
      f= cache.cache.open( file, 1 )
      title= os.path.split( title )[ -1 ]
    except:
      traceback.print_exc()
    
    if f!= None:
      info= self.getFileParams( f, file )
      if file.has_key( BITRATE_KEY ):
        #length= float( info[ 'frames' ] ) * 0.02609
        f.seek( -1, 2 )
        length= f.tell()/ ( file[ BITRATE_KEY ]* 125 )
      
      f.close()
      
      if info[ cache.TITLE_KEY ]!= '':
        title= info[ cache.TITLE_KEY ]
        
      # Parse only when title is set
      if info.has_key( cache.TRACK_KEY ) and info[ cache.TRACK_KEY ]!= '':
        trackNum= info[ cache.TRACK_KEY ]
      else:
        # Try to extract track number from the file name( first two symbols )
        trackNum= file[ cache.NAME_KEY ][ :2 ]
      try:
        # Remove some redundant chars and split by space
        m= re.search( '\d*', str( trackNum ) )
        trackNum= int( m.group( 0 ) )
      except:
        trackNum= None
      
      album= info[ ALBUM_KEY ]
      artist= info[ ARTIST_KEY ]
      
    if file.has_key( LENGTH_KEY )== 0:
      file[ LENGTH_KEY ]= length
    
    file[ cache.TITLE_KEY ]= translate( title )
    file[ ALBUM_KEY ]= translate( album )
    file[ ARTIST_KEY ]= translate( artist )
    file[ cache.TRACK_KEY ]= trackNum
    file[ cache.PROCESSED_KEY ]= 2
    self.setChanged( 1 )

  # -----------------------------------------------------------------
  def getFileInfo( self, file ):
    # Find out whether it is a directory or file
    length= 0
    album= artist= bitRate= trackNum= None
    title= file[ cache.TITLE_KEY ]
    if file[ cache.PROCESSED_KEY ]== 0:
      if file[ cache.DIR_KEY ]== 0:
        eventQueue.put( ( 'EXECUTE', self.processFile, file ) )
      else:
        eventQueue.put( ( 'EXECUTE', self.processDir, file ) )
        if file.has_key( 'removable' ):
          title= u'Removable device( %s )' % cache.cache.getPathName( file )
    elif file[ cache.PROCESSED_KEY ]== 2:
      if file[ cache.DIR_KEY ]== 0 or file.has_key( cache.CDDA_KEY ):
        album, artist= \
          file[ ALBUM_KEY ], file[ ARTIST_KEY ]
        if file.has_key( LENGTH_KEY ):
          length= file[ LENGTH_KEY ]
        if file.has_key( BITRATE_KEY ):
          bitRate= file[ BITRATE_KEY ]
        if file.has_key( cache.TRACK_KEY ):
          trackNum= file[ cache.TRACK_KEY ]
    
    return title, album, artist, length, bitRate, trackNum
  
  # -----------------------------------------------------------------
  def getNameIcon( self, title, album, artist, track ):
    """
      Renders the file / directory informarmation as a text with predefing font and based on
      information it gets from the file attributes
    """
    if track:
      title= ( '%02d - ' % track )+ title
    
    titleIcon= menu.drawText( self.titleFont, title )
   
    if album:
      albumIcon= menu.drawText( self.albumFont, '-'+ album )
    if artist:
      artistIcon= menu.drawText( self.artistFont, artist )
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
  def __init__( self, rect, attributes, params, renderClass ):
    menu.GenericDisplay.__init__( self, rect, attributes, params, renderClass )
    self.currPos= -1
  
  # -----------------------------------------------------------------
  def render( self ):
    """
      Main method called by the main renderer
    """
    self.currPos= aplayer.player.getPosition()
    if self.currPos== -1:
      return []
    
    res= menu.drawText( self.font, '%02d:%02d' % ( int( self.currPos / 60 ), self.currPos % 60 ) )
    return [ ( res, self.rect[ :2 ] ) ]
  
  # -----------------------------------------------------------------
  def hasChanged( self ):
    """
      State tracker. If returns 1, then state has been changed.
    """
    return aplayer.player.getPosition()!= self.currPos

# ****************************************************************************************
class PlayerDisplay( menu.GenericDisplay, AudioFileHelper ):
  """
    Displays full featured player
  """
  # -----------------------------------------------------------------
  def __init__( self, rect, attributes, params, renderClass ):
    """
      Constructor whic accepts parameters, rectangle size and renderer
    """
    menu.GenericDisplay.__init__( self, rect, attributes, params, renderClass )
    # Load fonts and icons
    self.titleFont, self.albumFont, self.artistFont, self.lengthFont= \
      self.loadFont( ( TITLE_FONT, ALBUM_FONT, ARTIST_FONT, LENGTH_FONT ) )
    self.mainArea= self.getParamList( 'mainArea', DEFAULT_MAIN_AREA_POS )
    self.auxArea= self.getParamList( 'auxArea', ( 10,10,180,160 ) )
    self.itemSize= self.mainArea[ 2: ]
    self.faceIcon= self.loadIcon( 'face', 'int'+ menu.DEFAULT_EXT )
    self.loadControls( ( 'prev', 'play', 'pause', 'stop', 'next' ) )
    self.volumeIcon= self.loadIcon( 'volumeIcon', 'vol'+ menu.DEFAULT_EXT )
    self.currPos= -1
    self.currControl= -1

  # -----------------------------------------------------------------
  def loadControls( self, names ):
    """
      Load controls for the player. Controls are: prev, stop, play etc...
    """
    controls= {}
    index= 0
    for name in names:
      controlIcon= self.getParam( name+ 'Button', '' )
      controlPos= self.getParamList( name+ 'ButtonPos', LEFT_C )
      controlIndex= int( self.getParam( name+ 'ButtonIndex', index ) )
      index+= 1
      controls[ controlIndex ]= [ name, controlPos ]
      icons= map( lambda x: self.loadIcon( '', '%s_%d%s' % ( controlIcon, x, menu.DEFAULT_EXT ) ), range( 2 ))
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
    try:
      bitRateIcon= menu.drawText( self.lengthFont, '%3d kbps' % bitRate )
      return [( bitRateIcon, ( self.auxArea[ 0 ], self.auxArea[ 1 ]+ yOffset ) ) ]
    except:
      return []
  
  # -----------------------------------------------------------------
  def showVolume( self, yOffset ):
    """
      Show volume icons based on current volume level
    """
    volume= aplayer.player.getVolume()
    w,h= self.volumeIcon.get_width(), self.volumeIcon.get_height()
    xOffs= self.auxArea[ 2 ]- w
    width= float(w)* volume/ MAX_VOLUME
    return [( self.volumeIcon.subsurface( (0,0,width,h ) ), ( xOffs, self.auxArea[ 1 ]+ yOffset ) )]
  
  # -----------------------------------------------------------------
  def showProgress( self, currPos, length ):
    """
      Show progress for the currently playing track
    """
    try:
      x, y, x1, y1= ( int( currPos / 60 ), currPos % 60, int( length / 60 ), length % 60 )
      lengthIcon= menu.drawText( self.lengthFont, '%02d:%02d / %02d:%02d' % ( x, y, x1, y1 ) )
      return [( lengthIcon, self.auxArea[ :2 ] )]
    except:
      return []
  
  # -----------------------------------------------------------------
  def renderInfo( self ):
    """
      Render complete set of information to be shown on a player face.
    """
    yOffs= 20
    xOffs= 40
    i= aplayer.player.getCurrentFileIndex()
    self.currPos= aplayer.player.getPosition()
    if i== -1 or self.currPos== -1:
      # Nothing is playing
      res= []
      height, res1= self.getNameIcon( 'Nothing is playing', None, None, None )
    else:
      # Display file information( including the position, length and bitrate )
      playingFile= aplayer.player.getPlayList()[ 0 ].getFile( i )
      title, album, artist, length, bitRate, trackNum = self.getFileInfo( playingFile )
      
      currPos= self.currPos
      res= self.showProgress( currPos, length )
      yOffset= res[ 0 ][ 0 ].get_height()
      res+= self.showBitRate( yOffset, bitRate )
      res+= self.showVolume( yOffset )
      
      height, res1= self.getNameIcon( title, album, artist, trackNum )
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
      res= aplayer.player.prevFile()
    elif name== 'next':
      res= aplayer.player.nextFile()
    elif name== 'stop':
      res= aplayer.player.stopPlayback()
    elif name== 'play':
      res= aplayer.player.startPlayback()
    elif name== 'pause':
      if aplayer.player.isPaused():
        res= aplayer.player.unpausePlayback()
      else:
        res= aplayer.player.pausePlayback()
    
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
    return \
      aplayer.player.getPosition()!= self.currPos or \
      menu.MenuHelper.hasChanged( self )

# ****************************************************************************************************
class AudioFileFolderItem( AudioFileHelper ):
  """
    Folder display class.
    Displays all files and folders in a folder style way. All the details will be specified
    It is most common way of seeing your music...
  """
  # -----------------------------------------------------------------
  def __init__( self, rect, attributes, params ):
    """
      Constructor.
    """
    AudioFileHelper.__init__( self, rect, attributes, params )
    self.extFont= self.loadFont( 'extensionFont' )
    self.itemSize= ( rect[ 2 ], self.folderIcons[ 0 ].get_height()+ 5 )
    self.iconSize= self.folderIcons[ 0 ].get_size()
    # Identify max icon size
    self.maxIconSize= ( self.itemSize[ 1 ], self.itemSize[ 1 ] )
    self.itemsWrapper= None
    self.history= []
  
  # ------------------------------------------------------------------------------------------------------
  def setDirItems( self, params ):
    children= cache.cache.getChildren( params[ 0 ] )
    if children== None:
      children= []
    
    # Set parent directory for the fake item
    parent= cache.PARENT_DIR
    parent[ 0 ][ cache.PARENT_KEY ]= params[ 0 ]
    self.itemsWrapper.setItems( parent+ children, self.getFilter() )
  
  # ------------------------------------------------------------------------------------------------------
  def refresh( self ):
    """
      Refresh files specified in the area.
      It may refresh root(s) directory or anything below the root directory as well.
    """
    # Do we have something to refresh ?
    items= self.itemsWrapper.items()
    if len( items )== 0:
      return
    
    try:
      # If we are in the root, just refresh the root
      dirs= [ x[ cache.ROOT_KEY ] for x in items ]
      parent= None
    except:
      # Just process regular files
      parent= items[ 0 ][ cache.PARENT_KEY ]
      parent[ cache.PROCESSED_KEY ]= 0
      dirs= [ x[ cache.NAME_KEY ] for x in items[ 1: ] ]
      
    # Remove all items in the list
    for d in dirs:
      cache.cache.delFile( d, parent )
    
    # If we are in the root, just refresh the root
    if parent:
      # Set parent as non processed and submit for processing
      self.getFileInfo( parent )
      eventQueue.put( ( 'EXECUTE', self.setDirItems, parent ) )
    else:
      items= cache.cache.setRoot( dirs )
      self.itemsWrapper.setItems( items, self.getFilter() )
    
    return parent

  # -----------------------------------------------------------------
  def processFolderIcon( self, file ):
    """
      Render folder/directory icon.
      Load pictures from the directory if possible or just display the folder icon.
    """
    if file[ cache.DIR_KEY ]== 1:
      fileName= cache.cache.getPathName( file )
      self.changed= 1
      try:
        files= os.listdir( fileName )
      except:
        traceback.print_exc()
        return
        
      for f in files:
        f1= string.split( f, '.' )
        if len( f1 )> 1 and string.upper( f1[ -1 ] ) in ICON_EXT:
          # pygame cannot work with unicode pathes, just pass the file object instead
          icon= pygame.image.load( open( os.path.join( fileName, f ), 'rb' ) )
          # Resize the icon
          file[ 'icon' ].append( pygame.transform.scale( icon, self.iconSize ))
    else:
      # Process file extension
      extIcon= menu.drawText( self.extFont, cache.cache.getExtension( file ) )
      file[ 'icon' ].append( extIcon )
      
    self.setChanged( 1 )
    
  # -----------------------------------------------------------------
  def processIcon( self, params ):
    """
      Process icon for any type of file in a background.
      If it is an audio file, put some icons to indicate that.
    """
    file= params[ 0 ]
    if not file.has_key( 'removable' ):
      self.processFolderIcon( file )
      return
      
    if file.has_key( cache.CDDA_KEY ):
      file[ 'icon' ].append( self.cddaIcon )
    else:
      if file.has_key( cache.DEVICE_KEY ):
        if file[ cache.DEVICE_KEY ].isReady():
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
    try:
      icon= file[ 'icon' ]
    except:
      if file[ cache.DIR_KEY ]:
        icon= copy.copy( self.folderIcons )
      else:
        icon= copy.copy( self.fileIcons )
      
      file[ 'icon' ]= icon
      eventQueue.put( ( 'EXECUTE', self.processIcon, file ) )
    
    # Draw icons at the left side of the list
    resIcon= [ ( icon[ isFocused ], LEFT_C ) ]
    yOffset= 5
    if len( icon )> 2 and icon[ 2 ]:
      resIcon.append( ( icon[ 2 ], ( yOffset, 13 ) ) )
      yOffset+= icon[ 2 ].get_height()+ 3
    
    if file[ cache.DIR_KEY ]== 0 or file.has_key( cache.CDDA_KEY ):
      if file.has_key( LENGTH_KEY ):
        length= '%02d:%02d' % ( int( file[ LENGTH_KEY ]/60 ), int( file[ LENGTH_KEY ] % 60 ))
        lengthIcon= menu.drawText( self.lengthFont, length )
        resIcon.append( ( lengthIcon, ( ( icon[ isFocused ].get_width()- lengthIcon.get_width() )/2,yOffset) ) )
    
    return resIcon
  
  # -----------------------------------------------------------------
  def drawItem( self, pos, isFocused ):
    """
      Get the properties of the generic file.
      It may submit background request if needed.
    """
    file= self.itemsWrapper.items()[ pos ]
    title, album, artist, length, bitRate, tracknum = self.getFileInfo( file )
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
    height, text= self.getNameIcon( title, album, artist, tracknum )
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
    return aplayer.player.getPlayList()[ 0 ].getFileNum( file )!= -1
  
  # -----------------------------------------------------------------
  def isPlaying( self, file ):
    """
      Whether the file is currently playing
    """
    i= aplayer.player.getCurrentFileIndex()
    if i!= -1:
      playingFile= aplayer.player.getPlayList()[ 0 ].getFile( i )
      if cache.cache.getPathName( file )== cache.cache.getPathName( playingFile ):
        return 1
    
    return 0
  
  # -----------------------------------------------------------------
  def processKey( self, pos, key ):
    """
      Process generic keys from the parent control
    """
    activeItem= None
    item= self.itemsWrapper.items()[ pos ]
    
    if item[ cache.NAME_KEY ]== '..' and key== pygame.K_RETURN:
      # Assume we are at ..
      key= pygame.K_BACKSPACE
    
    if key== pygame.K_RETURN:
      if item[ cache.DIR_KEY ]:
        # Wait until we process directory( may take a while )
        if item[ cache.PROCESSED_KEY ]== 2:
          self.history.append( ( self.itemsWrapper.items(), pos, item[ cache.TITLE_KEY ] ) )
          self.setDirItems( [ item ] )
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
    s= item[ cache.TITLE_KEY ]
    pl= aplayer.player.getPlayList()
    if aplayer.playLists.getName( pl[ 0 ] )== s:
      s= '*'+ s
      
    # Find out whether it is a directory or file
    res= []
    # Select current item if needed
    if isFocused== 1:
      res.append( ( self.stripeIcon, LEFT_C ) )
    
    res.append( ( menu.drawText( self.font, s ), LEFT_C ) )
    return res

# ****************************************************************************************************
class PlaylistPlainItem( AudioFileFolderItem ):
  # -----------------------------------------------------------------
  def processKey( self, pos, key ):
    item= None
    if pos> -1:
      item= self.itemsWrapper.items()[ pos ]
    
    if key== pygame.K_RETURN:
      # Start playing selected file
      aplayer.player.setPlayList( ( self.itemsWrapper, pos ) ); 
      aplayer.player.startPlayback()
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
