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
import pympg, pysound, pygame, aaudio, cache

from __main__ import *
from pymedia import menu

BUFFER_LENGTH= 20000
CDDA_BUFFER_LENGTH= 1000000
EXT= 'm3u'
LEFT_C= (0,0)

# ---------------------------------------------------
def getFileName( fullName ):
  """
    getFileName( fullName ) -> name
    
    Splits full file name and returns only the filename with no path.
  """
  s= string.split( fullName, '.' )
  return string.join( s[ : len( s )- 1 ], '.' )

# **********************************************************************************************
class AbstractFileList:
  """
    Generic class to provide list functionality for a set of files or links
  """
  # ---------------------------------------------------
  def __init__( self ):
    self.clear()
    self.sorted= 0
  
  # ---------------------------------------------------
  def addFile( self, file, pos= -1 ):
    """
      addFile( file, pos= -1 ) -> None
      
      Adds or inserts file into the list by the object
    """
    fullName= cache.cache.getPathName( file )
    if self.fileNames.has_key( fullName ):
      return 0, self.fileNames[ fullName ]
    
    self.files.append( file )
    self.fileNames[ cache.cache.getPathName( file ) ]= len( self.files )- 1
    return 1, len( self.fileNames )- 1
  
  # ---------------------------------------------------
  def addFilePath( self, filePath, pos= -1 ):
    """
      addFilePat ( file, pos= -1 ) -> None
      
      Adds or inserts file into the list by the full system name
    """
    file= cache.cache.getFile( filePath )
    self.addFile( file, pos )
  
  # ---------------------------------------------------
  def clear( self ):
    """
      clear() -> None
      
      Clear the list
    """
    self.files= []
    self.fileNames= {}
    self.setChanged( 1 )
  
  # ---------------------------------------------------
  def remove( self, filePos ):
    """
      remove( pos ) -> None
      
      Removes file from list by its position
    """
    if len( self.files )> filePos:
      fullName= cache.cache.getPathName( self.files[ filePos ] )
      del( self.files[ filePos ] )
      del( self.fileNames[ fullName ] )
      self.setChanged( 1 )
  
  # ---------------------------------------------------
  def getFile( self, filePos ):
    """
      getFile( pos ) -> file | None
      
      Gets file from the list by its position
    """
    if len( self.files )> filePos:
      return self.files[ filePos ]
    
    return None
  
  # ---------------------------------------------------
  def getFileNum( self, file ):
    """
      getFile( pos ) -> file
      
      Returns file from list by its position
    """
    fName= cache.cache.getPathName( file )
    if self.fileNames.has_key( fName ):
      return self.fileNames[ fName ]
    
    return -1
  
  # -----------------------------------------------------------------
  def sortByDate( self, ascending ):
    """
      sortByDate( acsending ) -> list
      
      Sorts the list by date. Not sure it can be done in generic way. Just a stub for now.
    """
    self.setChanged( 1 )
    return self.getList()
  
  # -----------------------------------------------------------------
  def randomize( self ):
    """
      randomize() -> list
      
      Randomize list of files
    """
    tmpList= []
    tmpNames= {}
    while len( self.files )> 0:
      j= int( random.random()* len( self.files ))
      tmpList.append( self.files[ j ] )
      tmpNames[ cache.cache.getPathName( self.files[ j ] ) ]= len( tmpList )- 1 
      del self.files[ j ]
    
    self.files= tmpList
    self.fileNames= tmpNames
    self.setChanged( 1 )
    return self.items()

  # -----------------------------------------------------------------
  def sortByName( self, isDummy= 0 ):
    """
      sortByName( isDummy= 0 ) -> list
      
      Sorts the list by name. Not sure it can be done in generic way.
      Just takes name attribute for now..
    """
    # Get names
    if len( self.files )> 0:
      hasParent= 0
      if self.files[ 0 ][ 'name' ]== '..':
        del( self.files[ 0 ] )
        hasParent= 1
      
      names= [ cache.cache.getPathName( x ) for x in self.files ]
      names.sort()
      if self.sorted== 0:
        self.sorted= 3
      else:
        self.sorted^= 2
      if self.sorted== 1:
        names.reverse()
        
      if isDummy:
        self.files= [ cache.cache.getDummyFile( x ) for x in names ]
      else:
        self.files= [ cache.cache.getFile( x ) for x in names ]
        if hasParent== 1:
          self.files= aaudio.PARENT_DIR+ self.files
      
      self.setChanged( 1 )
    
    return self.items()

  # -----------------------------------------------------------------
  def items( self ):
    """
      items() -> list
      
      Returns items in a list as a list
    """
    return self.files

  # -----------------------------------------------------------------
  def getFilesCount( self ):
    """
      getFilesCount() -> filesCount
      
      Return number of files in a list
    """
    return len( self.files )

  # ---------------------------------------------------
  def hasChanged( self ):
    """
      hasChanged() -> hasChanged_flag
      
      Whether or not list has changed
    """
    return self.changed
  
  # ---------------------------------------------------
  def setChanged( self, changed ):
    """
      setChanged( changed ) -> None
      
      Set whether or not list has changed
    """
    self.changed= changed
  
# ****************************************************************************************************
class FileWrapper( AbstractFileList ):
  """
    Specific file oriented implementation of a list of files for differnt
    controls such as menu.ListDisplay.
    Being used as wrapper for most of the lists.
  """
  # ---------------------------------------------------
  def __init__( self, items, filter ):
    """
      ctor( items, filter ) -> self
      
      Creates custom list with ability to apply filters
    """
    AbstractFileList.__init__( self )
    self.changed= 1
    self.setItems( items, filter )
  
  # ---------------------------------------------------
  def setItems( self, items, filter ):
    """
      setItems( items, filter ) -> None
        
      Set items into the list and filter them out using filter criteria
    """
    self.files= []
    filterLower= [ x.lower() for x in filter ]
    if items:
      for file in items:
        if file[ cache.DIR_KEY ] or file.has_key( cache.CDDA_KEY ):
          self.files.append( file )
        else:
          fName= string.split( file[ 'name' ], '.' )
          if len( fName )> 1:
            ext= fName[ -1 ]
            if len( filter )== 0 or string.lower( ext ) in filterLower:
              self.files.append( file )
      
    self.setChanged( 1 )
  
  # ---------------------------------------------------
  def init( self ):
    """
      init() -> None
      
      Just a stub for ListDisplay completeness
    """
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
    """
      ctor() -> PlayList
    """
    AbstractFileList.__init__( self )
    self.changed= 0
  
  # ---------------------------------------------------
  def save( self, fileName ):
    """
      save( fileName ) -> None
      
      Saves the playlist into the file as a regular m3u format( just a full filename )
    """
    f= open( fileName, 'w' )
    for file in self.files:
      # If it is an audio file, do not save it to the playlist
      if not file.has_key( cache.CDDA_KEY ):
        s= cache.cache.getPathName( file )+ '\n'
        f.write( s.encode( 'utf-8' ) )
    
    f.close()
    print 'Playlist saved to ', fileName
  
  # ---------------------------------------------------
  def load( self, fileName ):
    """
      load( fileName ) -> success
      
      Loads playlist from a simplified m3u file
    """
    try:
      files= open( fileName, 'r' ).readlines()
    except:
      return 0
    
    for file in files:
      file= unicode( file.strip(), 'utf-8' )
      try:
        f= cache.cache.getFile( file )
      except:
        f= cache.cache.getDummyFile( file )
      
      self.addFile( f )
    
    return 1
  
  # ---------------------------------------------------
  def init( self ):
    """
      Stub for ListDisplay
    """
    self.changed= 1
  
# **********************************************************************************************
# *
# **********************************************************************************************
class PlayLists:
  # ---------------------------------------------------
  def __init__( self ):
    self.default= 'Default'
  
  # ---------------------------------------------------
  def save( self ):
    self.removeAllFiles()
    for list in self.lists:
      self.saveList( list[ 2 ] )
    
    self.savePositions()
  
  # ---------------------------------------------------
  def removeAllFiles( self ):
    [ os.remove( x ) for x in glob.glob( self.dir+ os.sep+ '*.'+ EXT ) ]
  
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
      for line in f[ 1: ]:
        name, pos= string.split( line, '\t' )
        try:
          index= self.lists.index( [ name, -1, None ] )
          self.lists[ index ][ 1 ]= int( pos )
        except:
          pass
      # Set default playlist if any
      if len( f ):
        self.default= f[ 0 ].strip()
    except:
      traceback.print_exc()
  
  # ---------------------------------------------------
  def init( self ):
    pass
  
  # ---------------------------------------------------
  def getIndexByList( self, list ):
    lists= [ x[ 2 ] for x in self.lists ]
    try:
      index= lists.index( list )
      return index
    except:
      return None
  
  # ---------------------------------------------------
  def items( self ):
    return [ { 'caption': x[ 0 ] } for x in self.lists ]
  
  # ---------------------------------------------------
  def getPlayListPos( self, list ):
    index= self.getIndexByList( list )
    if index!= None:
      return self.lists[ index ][ 1 ]
    
  # ---------------------------------------------------
  def getDefault( self ):
    return self.getPlayList( self.default )
  
  # ---------------------------------------------------
  def getPlayList( self, name= None ):
    if name== None:
      # Assign the name automatically
      name= 'Playlist_%02d' % len( self.lists )
    try:
      names= [ x[ 0 ] for x in self.lists ]
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
    # Get the default playlist name
    pl= player.getPlayList()
    self.default= self.getName( pl[ 0 ] )
    # If found, then save it in the positions.dat file
    posData= [ '%s\t%d' % ( x[ 0 ], x[ 1 ] ) for x in self.lists ]
    f= open( os.path.join( self.dir, 'positions.dat' ), 'w' )
    f.write( self.default+ '\n' )
    f.write( string.join( posData, '\n' ) )
    f.close()

# **********************************************************************************************
# *
# **********************************************************************************************
class Player:
  """
    Main player class, decodes audio streams and sends to the audio device
  """
  # --------------------------------------------------------
  def __init__( self ):
    self.thread= None
  
  # --------------------------------------------------------
  def start( self ):
    """ Start server """
    if self.thread:
      raise 'Cannot run another copy of player'
    
    self.startOffset= 0
    self.stopFlag= 1
    self.exitFlag= 0
    self.fileChanged= 0
    self.currentFile= -1
    self.iFreqRate= -1
    self.channels= -1
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
    
    return ( pysound.getPosition()- self.startOffset )/ self.iFreqRate
    
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
    chunk= -1
    chunks= []
    newFile= 1
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
      chunk1= pysound.getChunkNum()
      # Update currently playing file number
      if chunk1== -1:
        # Set current file into the beginning
        if self.fileChanged== 1:
          initFlag= 1
          self.fileChanged= 0
          continue
        
        if readFileNum== -1:
          readFileNum= self.currentFile
        
        # Initialize list of chunks
        self.startOffset= 0
      elif chunk1!= chunk and len( chunks )> 0 and chunk1== chunks[ 0 ]:
        self.startOffset= pysound.getPosition()
        del chunks[ 0 ]
      
      chunk= chunk1
      
      if len( unprocessedChunk )== 0:
        # Get the next file to play
        playlistFile= self.playList.getFile( readFileNum )
        if playlistFile!= None:
          # Open file depending on its type
          playLists.setPosition( self.getPlayList() )
          
          self.currentFile= readFileNum
          readFileNum+= 1
          newFile= 1
          
          f= cache.cache.open( playlistFile )
          if f== None:
            playlistFile= None
            continue
          elif not playlistFile.has_key( cache.CDDA_KEY ):
            try:
              dec= pympg.Decoder( cache.cache.getExtension( playlistFile ) )
            except:
              # No appropriate extension filter
              traceback.print_exc()
              playlistFile= None
              continue
        
        # If no files have left and no data is playing, just stop
        if playlistFile== None:
          if pysound.getChunkNum()== -1:
            self.stopFlag= 1
          
          time.sleep( 0.1 )
          continue
      
      # Process data from the file into pcm
      if len( processedChunk )== 0:
        unprocessedChunk= ''
        if f.getFile().has_key( cache.CDDA_KEY ):
          try:
            unprocessedChunk= f.read( CDDA_BUFFER_LENGTH )
          except:
            traceback.print_exc()
            playlistFile= None
          
          processedChunk= unprocessedChunk
          self.iFreqRate= 44100
          self.channels= 2
        else:
          try:
            unprocessedChunk= f.read( BUFFER_LENGTH )
          except:
            traceback.print_exc()
            playlistFile= None
          
          if len( unprocessedChunk )> 0:
            kbPerSec, self.iFreqRate, sampleRate, self.channels, processedChunk= dec.convert2PCM( unprocessedChunk )
            f.getFile()[ 'bitrate' ]= kbPerSec
        
        if len( unprocessedChunk )== 0:
          f.close()
        
        if pysound.getRate()!= self.iFreqRate or pysound.getChannels()!= self.channels:
          pysound.stop()
          pysound.close()
        
        if pysound.isOpen()== 0:
          pysound.open( self.iFreqRate, self.channels )
      else:
        try:
          # Commit chunk for playing
          tmpChunk= pysound.play( processedChunk )
          processedChunk= ''
          if newFile== 1:
            chunks.append( tmpChunk )
          
          newFile= 0
        except:
          # Too many chunks submitted already for playing
          time.sleep( 0.05 )
      
    pysound.stop()
    self.thread= None
    print 'Player stopped'

playLists= PlayLists()
player= Player()

if __name__ == "__main__":
  print 'The aplayer module is a part of pymedia package.\nIt cannot be executed separately.'

"""
import player
pls= player.PlayList( ( "c:\\Bors\\music\\Leftfield\\Leftism\\11-21st Century Poem.mp3", "c:\\Bors\\music\\Leftfield\\Leftism\\04-Song Of Life.mp3" ) )
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
