##    aplayer - Part of the pymedia package. Major playlists functionality.
##        Includes global audio player for various types of audio files.
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

# length in seconds: $this->filesize / ($this->bitrate * 125); 

import threading, traceback, time, os, string, random, glob
import pympg, pysound, pygame, aaudio

from __main__ import *
from pymedia import menu

BUFFER_LENGTH= 20000
CDDA_BUFFER_LENGTH= 2000000
EXT= 'm3u'
LEFT_C= (0,0)

# ---------------------------------------------------
def getFileName( fullName ):
  s= string.split( fullName, '.' )
  return string.join( s[ : len( s )- 1 ], '.' )

# **********************************************************************************************
# *
# **********************************************************************************************
class AbstractFileList:
  # ---------------------------------------------------
  def __init__( self ):
    self.clear()
    self.sorted= 0
  
  # ---------------------------------------------------
  def addFile( self, file, pos= -1 ):
    """
    addFile( file, pos= -1 ) -> None
    Adds or inserts file into the playlist
    """
    fullName= aaudio.cache.getPathName( file )
    if not self.fileNames.has_key( fullName ):
      self.files.append( file )
      self.fileNames[ aaudio.cache.getPathName( file ) ]= len( self.files )- 1
  
  # ---------------------------------------------------
  def addFilePath( self, filePath, pos= -1 ):
    """
    addFile( file, pos= -1 ) -> None
    Adds or inserts file into the playlist
    """
    file= aaudio.cache.getFile( filePath )
    self.addFile( file, pos )
  
  # ---------------------------------------------------
  def clear( self ):
    """
    """
    self.files= []
    self.fileNames= {}
    self.setChanged( 1 )
  
  # ---------------------------------------------------
  def remove( self, filePos ):
    """
      remove( pos ) -> None
      Removes file from cache by its position
    """
    if len( self.files )> filePos:
      fullName= aaudio.cache.getPathName( self.files[ filePos ] )
      del( self.files[ filePos ] )
      del( self.fileNames[ fullName ] )
      self.setChanged( 1 )
  
  # ---------------------------------------------------
  def getFile( self, filePos ):
    """
    getFile( pos ) -> file | None
    Gets file from the playlist by its position
    """
    if len( self.files )> filePos:
      return self.files[ filePos ]
    
    return None
  
  # ---------------------------------------------------
  def getFileNum( self, file ):
    """
    getFile( pos ) -> file
    Returns file from cache by its position
    """
    fName= aaudio.cache.getPathName( file )
    if self.fileNames.has_key( fName ):
      return self.fileNames[ fName ]
    
    return -1
  
  # -----------------------------------------------------------------
  def sortByDate( self, ascending ):
    self.setChanged( 1 )
    return self.getList()
  
  # -----------------------------------------------------------------
  def randomize( self ):
    tmpList= []
    tmpNames= {}
    while len( self.files )> 0:
      j= int( random.random()* len( self.files ))
      tmpList.append( self.files[ j ] )
      tmpNames[ aaudio.cache.getPathName( self.files[ j ] ) ]= len( tmpList )- 1 
      del self.files[ j ]
    
    self.files= tmpList
    self.fileNames= tmpNames
    self.setChanged( 1 )
    return self.items()

  # -----------------------------------------------------------------
  def sortByName( self, isDummy= 0 ):
    # Get names
    if len( self.files )> 0:
      hasParent= 0
      if self.files[ 0 ][ 'name' ]== '..':
        del( self.files[ 0 ] )
        hasParent= 1
      
      names= map( lambda x: aaudio.cache.getPathName( x ), self.files )
      names.sort()
      if self.sorted== 0:
        self.sorted= 3
      else:
        self.sorted^= 2
      if self.sorted== 1:
        names.reverse()
        
      if isDummy:
        self.files= map( lambda x: aaudio.cache.getDummyFile( x ), names )
      else:
        self.files= map( lambda x: aaudio.cache.getFile( x ), names )
        if hasParent== 1:
          self.files= aaudio.PARENT_DIR+ self.files
      
      self.setChanged( 1 )
    
    return self.items()

  # -----------------------------------------------------------------
  def items( self ):
    return self.files

  # -----------------------------------------------------------------
  def getFilesCount( self ):
    return len( self.files )

  # ---------------------------------------------------
  def hasChanged( self ):
    return self.changed
  
  # ---------------------------------------------------
  def setChanged( self, changed ):
    self.changed= changed
  
# ****************************************************************************************************
class FileWrapper( AbstractFileList ):
  def __init__( self, items, filter ):
    AbstractFileList.__init__( self )
    self.changed= 1
    self.setItems( items, filter )
  
  def setItems( self, items, filter ):
    self.files= []
    filterLower= map( lambda x: string.lower( x ), filter )
    if items:
      for file in items:
        if file[ 'isDir' ] or file.has_key( 'isCDDA' ):
          self.files.append( file )
        else:
          fName= string.split( file[ 'name' ], '.' )
          if len( fName )> 1:
            ext= fName[ -1 ]
            if len( filter )== 0 or string.lower( ext ) in filterLower:
              self.files.append( file )
      
    self.setChanged( 1 )
  
  def init( self ):
    pass

# **********************************************************************************************
# *
# **********************************************************************************************
class PlayList( AbstractFileList ):
  """
    Class for storing playlist items( can store files only, no dirs so far )
  """
  # ---------------------------------------------------
  def __init__( self ):
    AbstractFileList.__init__( self )
    self.changed= 0
  
  # ---------------------------------------------------
  def save( self, fileName ):
    f= open( fileName, 'w' )
    for file in self.files:
      # If it is an audio file, just save Track %d
      if file.has_key( 'isCDDA' ):
        pass
        #f.write( 'CDDA:\t%s\t%d\n' % ( audiofile.cache.getPathName( file[ 'parent' ] ), file[ 'tracknum' ] ) )
      else:
        f.write( aaudio.cache.getPathName( file )+ '\n' )
    
    f.close()
    print 'Playlist saved to ', fileName
  
  # ---------------------------------------------------
  def load( self, fileName ):
    try:
      files= open( fileName, 'r' ).readlines()
    except:
      return 0
    
    for file in files:
      try:
        f= aaudio.cache.getFile( string.strip( file ))
      except:
        f= aaudio.cache.getDummyFile( string.strip( file ))
      
      self.addFile( f )
    
    return 1
  
  # ---------------------------------------------------
  def init( self ):
    self.changed= 1
  
# ****************************************************************************************************
class PlayListItem( menu.MenuItem ):
  # -----------------------------------------------------------------
  def __init__( self, rect, params ):
    menu.MenuItem.__init__( self, rect, params )
  
  # -----------------------------------------------------------------
  def processKey( self, itemPos, key ):
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
    pl= aaudio.player.getPlayList()
    if aaudio.playLists.getName( pl[ 0 ] )== s:
      s= '*'+ s
      
    # Find out whether it is a directory or file
    res= []
    # Select current item if needed
    if isFocused== 1:
      res.append( ( self.stripeIcon, LEFT_C ) )
    
    res.append( ( self.font[ 0 ].render( s, 1 , self.font[ 1 ] ), LEFT_C ) )
    return res

# **********************************************************************************************
# *
# **********************************************************************************************
class PlayLists:
  # ---------------------------------------------------
  def __init__( self ):
    pass
  
  # ---------------------------------------------------
  def save( self ):
    self.removeAllFiles()
    for list in self.lists:
      self.saveList( list[ 2 ] )
    
    self.savePositions()
  
  # ---------------------------------------------------
  def removeAllFiles( self ):
    map( lambda x: os.remove( x ), glob.glob( self.dir+ os.sep+ '*.'+ EXT ) )
  
  # ---------------------------------------------------
  def reload( self ):
    self.load( self.dir )
    
  # ---------------------------------------------------
  def load( self, dir ):
    self.dir= dir
    self.lists= \
      list( \
        map( lambda x: [ getFileName( x ), -1, PlayList() ], \
          filter( lambda x: string.split( x, '.' )[ -1 ]== EXT, os.listdir( dir ))))
    
    # Open playlists
    self.lists= filter( lambda x: x[ 2 ].load( os.path.join( self.dir, x[ 0 ]+ '.'+ EXT )), self.lists )
    
    # Load positions
    try:
      f= open( os.path.join( dir, 'positions.dat' ), 'rt').readlines()
      for line in f:
        name, pos= string.split( line, '\t' )
        try:
          index= self.lists.index( [ name, -1, None ] )
          self.lists[ index ][ 1 ]= int( pos )
        except:
          pass
    except:
      traceback.print_exc()
  
  # ---------------------------------------------------
  def init( self ):
    pass
  
  # ---------------------------------------------------
  def getIndexByList( self, list ):
    lists= map( lambda x: x[ 2 ], self.lists )
    try:
      index= lists.index( list )
      return index
    except:
      return None
  
  # ---------------------------------------------------
  def items( self ):
    return map( lambda x: { 'caption': x[ 0 ] }, self.lists )
  
  # ---------------------------------------------------
  def getPlayListPos( self, list ):
    index= self.getIndexByList( list )
    if index!= None:
      return self.lists[ index ][ 1 ]
    
  # ---------------------------------------------------
  def getDefault( self ):
    return self.getPlayList( 'Default' )
  
  # ---------------------------------------------------
  def getPlayList( self, name= None ):
    if name== None:
      # Assign the name automatically
      name= 'Playlist_%02d' % len( self.lists )
    try:
      names= map( lambda x: x[ 0 ], self.lists )
      index= names.index( name )
      if self.lists[ index ][ 2 ]!= None:
        return self.lists[ index ][ 2 ], self.lists[ index ][ 1 ]
    except:
      f= PlayList()
      self.lists.append( [ name, -1, f ] )
      return f, -1
    
    return self.lists[ index ][ 2 ], self.lists[ index ][ 1 ]
  
  # ---------------------------------------------------
  def saveList( self, list ):
    fileName= self.getName( list )
    if fileName:
      list.save( os.path.join( self.dir, fileName+ '.'+ EXT ))
  
  # ---------------------------------------------------
  def remove( self, list ):
    index= self.getIndexByList( list )
    if index:
      del self.lists[ index ]
  
  # ---------------------------------------------------
  def getName( self, list ):
    index= self.getIndexByList( list )
    if index!= None:
      return self.lists[ index ][ 0 ]
    
    return None
  
  # ---------------------------------------------------
  def setName( self, list, name ):
    index= self.getIndexByList( list )
    if index:
      self.lists[ index ][ 0 ]= name
  
  # ---------------------------------------------------
  def setPosition( self, listpos ):
    index= self.getIndexByList( listpos[ 0 ] )
    if index!= None:
      self.lists[ index ][ 1 ]= listpos[ 1 ]
      self.savePositions()
  
  # ---------------------------------------------------
  def savePositions( self ):
    # If found, then save it in the positions.dat file
    posData= map( lambda x: '%s\t%d' % ( x[ 0 ], x[ 1 ] ), self.lists )
    f= open( os.path.join( self.dir, 'positions.dat' ), 'w' )
    f.write( string.join( posData, '\n' ) )
    f.close()

# **********************************************************************************************
# *
# **********************************************************************************************
class Player:
  """
    Main player class, decodes mp3 stream and sends to the audio device
  """
  # --------------------------------------------------------
  def __init__( self ):
    self.thread= None
  
  # --------------------------------------------------------
  def start( self ):
    """ Start server """
    if self.thread:
      raise 'Cannot run another copy of player'
    
    self.startPosition= 0
    self.startOffset= 0
    self.stopFlag= 1
    self.exitFlag= 0
    self.fileChanged= 0
    self.currentFile= -1
    self.avgBitrate= -1
    self.iFreqRate= -1
    self.paused= 0
    
    self.thread= threading.Thread( target= self.readerLoop )
    self.thread.start()
  
  # --------------------------------------------------------
  def setPlayList( self, playList ):
    self.stopPlayback()
    self.playList= playList[ 0 ]
    self.currentFile= playList[ 1 ]
    
  # --------------------------------------------------------
  def getPlayList( self ):
    return self.playList, self.currentFile
  
  # --------------------------------------------------------
  def stop( self ):
    """ Stop server """
    self.exitFlag= 1
    self.stopPlayback()
    
  # --------------------------------------------------------
  def stopPlayback( self ):
    """ Stop playing all streams """
    res= pysound.isOpen()
    self.stopFlag= 1
    self.startPosition= 0
    self.startOffset= self.paused= 0
    pysound.stop()
    pysound.close()
    return res
    
  # --------------------------------------------------------
  def nextFile( self ):
    res= 0
    if self.currentFile+ 1< self.playList.getFilesCount():
      self.playFile( self.currentFile+ 1 )
      res= 1
    else:
      self.stopPlayback()
  
  # --------------------------------------------------------
  def prevFile( self ):
    res= 0
    if self.currentFile> 0:
      self.playFile( self.currentFile- 1 )
      res= 1
    else:
      self.stopPlayback()
    
    return res
  
  # --------------------------------------------------------
  def playFile( self, fileNum ):
    self.currentFile= fileNum
    self.fileChanged= 1
    self.stopFlag= 0
    self.paused= 0
    pysound.stop()
    return 1
  
  # --------------------------------------------------------
  def skipTime( self, secs ):
    # Calculate next position
    self.startOffset= secs
    self.fileChanged= 1
    self.stopFlag= 0
    pysound.stop()
  
  # --------------------------------------------------------
  def isPaused( self ):
    return self.paused
  
  # --------------------------------------------------------
  def pausePlayback( self ):
    """ Pause playing current stream """
    if pysound.isOpen():
      pysound.pause()
      self.paused= 1
    
    return self.paused
  
  # --------------------------------------------------------
  def unpausePlayback( self ):
    """ Resume playing current stream """
    if pysound.isOpen():
      pysound.unpause()
      self.paused= 0
      return 1
    
    return 0
    
  # --------------------------------------------------------
  def startPlayback( self ):
    """ Start playing the file """
    if self.currentFile== -1:
      self.currentFile= 0
    
    self.playFile( self.currentFile )
  
  # --------------------------------------------------------
  def getPosition( self ):
    if pysound.isOpen()== 0:
      return -1
    
    pos= pysound.getPosition()
    
    return ( pos- self.startPosition )/ self.iFreqRate
    
  # --------------------------------------------------------
  def getCurrentFileIndex( self ):
    if self.stopFlag:
      return -1
    
    return self.currentFile
    
  # --------------------------------------------------------
  def setCurrentFileIndex( self, index, stopPlayback= 1 ):
    if stopPlayback== 1:
      self.stopPlayback()
    self.currentFile= index
    
  # --------------------------------------------------------
  def isPlaying( self ):
    return self.stopFlag== 0
    
  # --------------------------------------------------------
  def readerLoop( self ):
    initFlag= 1
    reminder= ''
    readFileNum= 0
    f= None
    print 'Player started'
    while 1:
      # Handle stop and init
      if self.stopFlag== 1 or initFlag:
        # Close file if opened
        if f:
          f.close()
        
        # Remove the reference
        f= None
        readFileNum= -1
        unprocessedChunk= ''
        processedChunk= ''
        if initFlag:
          initFlag= 0
        else:
          if pysound.isOpen():
            pysound.close()
          if self.exitFlag:
            break
          
          time.sleep( 0.001 )
        
        continue
      
      # If no chunk is playing right now, get the next file to play
      chunk= pysound.getChunkNum()
      # Update currently playing file number
      if chunk== -1:
        # Set current file into the beginning
        if self.fileChanged== 1:
          initFlag= 1
          self.fileChanged= 0
          continue
        
        if readFileNum== -1:
          readFileNum= self.currentFile
        
        # Initialize list of chunks
        self.startOffset= 0
      
      if len( unprocessedChunk )== 0:
        # Get the next file to play
        playlistFile= self.playList.getFile( readFileNum )
        if playlistFile!= None:
          # Open file depending on its type
          aaudio.playLists.setPosition( self.getPlayList() )
          
          self.currentFile= readFileNum
          readFileNum+= 1
          
          f= aaudio.cache.open( playlistFile )
          if f== None:
            playlistFile= None
            continue
          elif not playlistFile.has_key( 'uncompressed' ):
            try:
              dec= pympg.Decoder( aaudio.cache.getExtension( playlistFile ) )
            except:
              # No appropriate extension filter
              traceback.print_exc()
              playlistFile= None
              continue
        
        # If no files have left and no data is playing, just stop
        if playlistFile== None:
          if pysound.getChunkNum()== -1:
            self.stopFlag= 1
          
          time.sleep( 0.001 )
          continue
      
      # Process data from the file into pcm
      if len( processedChunk )== 0:
        unprocessedChunk= ''
        if f.getFile().has_key( 'uncompressed' ):
          try:
            unprocessedChunk= f.read( CDDA_BUFFER_LENGTH )
          except:
            traceback.print_exc()
            playlistFile= None
          
          processedChunk= unprocessedChunk
          self.iFreqRate= 44100
        else:
          try:
            unprocessedChunk= f.read( BUFFER_LENGTH )
          except:
            traceback.print_exc()
            playlistFile= None
          
          if len( unprocessedChunk )> 0:
            kbPerSec, self.iFreqRate, sampleRate, channels, processedChunk= dec.convert2PCM( unprocessedChunk )
        
        if pysound.getRate()!= self.iFreqRate:
          pysound.stop()
          pysound.close()
        
        if pysound.isOpen()== 0:
          pysound.open( self.iFreqRate, 1 )
      else:
        try:
          # Commit chunk for playing
          tmpChunk= pysound.play( processedChunk )
          if chunk== -1:
            self.startPosition= pysound.getPosition()
            self.avgBitrate= 0
          processedChunk= ''
        except:
          # Too many chunks submitted already for playing
          time.sleep( 0.05 )
      
    pysound.stop()
    self.thread= None
    print 'Player stopped'
    

#if __name__ == "__main__":
"""
import player
pls= player.PlayList( ( "c:\\Bors\\music\\Leftfield\\Leftism\\11-21st Century Poem.mp3", "c:\\Bors\\music\\Leftfield\\Leftism\\04-Song Of Life.mp3" ) )
pls.addFilePath( "c:\\Bors\\music\\Kino\\The Best 82-86\\02-v_nashih_glazah_320_lame_cbr.mp3" )
pl= player.Player()
pl.start( pls )
pl.startPlayback()
pl.stopPlayback()


import pysound
pysound.open( 44100, 1 )
f= open( 'c:\\rr.pcm', 'rb' )
s= f.read( 400000 )
pysound.play( s )
s= f.read( 400000 )
pysound.play( s )
s= f.read( 400000 )
pysound.play( s )
s= f.read( 400000 )
pysound.play( s )
s= f.read( 400000 )
pysound.play( s )
s= f.read( 400000 )
pysound.play( s )







def cddb_check_sum(n):
    ret = 0
    while n > 0:
        ret = ret + (n % 10)
        n = n / 10
    return ret

def cddb_disc_id( device ):
    pygame.init()
    cd= pygame.cdrom.CD( device )
    cd.init()
    
    track_frames = []
    checksum = 0
    
    total_time= 0
    for i in xrange( cd.get_numtracks() ):
        start = cd.get_track_start( i )
        checksum += cddb_check_sum( int( start ) )
        total_time+= cd.get_track_length( i )
    
    discid = ((checksum % 0xff) << 24 | int( total_time ) << 8 | cd.get_numtracks() )
    return discid

import pycdda
pycdda.init()
cd= pycdda.CD( 0 )
tracksInfo= cd.getTOC()
import player
cddb= player.CDDB('c:\\bors\\cddb' )
cddb.getDiscInfo( tracksInfo )

import pygame
pygame.init()
cd= pygame.cdrom.CD( 0 )
cd.init()
tracksInfo= map( lambda x: ( int( cd.get_track_start( x )*75), int( cd.get_track_length( x )*75 ) ), range( cd.get_numtracks() ) )
import player
cddb= player.CDDB('c:\\bors\\cddb' )
cddb.getDiscInfo( tracksInfo )

"""
