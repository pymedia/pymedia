##    cache - Part of the pymedia package. Audio module cache support.
##        Primarily designated for caching files into the memory. It is critical to lower down
##        power consumption when using small devices. Every IO device consumes as much as 5-7W of power which
##        is converted to the pure heat and nothing else.
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

import Queue, os, string, threading, time, pycdda, traceback
from __main__ import *

MIN_CHUNKS_IN_CACHE= 3
CHUNK_SIZE= 500000

CHILDREN_KEY= 'children'
PARENT_KEY= 'parent'
NAME_KEY= 'name'
ROOT_KEY= 'root'
TITLE_KEY= 'title'
DEVICE_KEY= 'device'
CDDA_KEY= 'isCDDA'
TRACK_KEY= 'tracknum'
DIR_KEY= 'dir'

CDDA_EXT= 'cdda'
MISSING= '<missing> '
PROCESSED_KEY= 'processed'

PARENT_DIR= [
  { NAME_KEY: '..',
    TITLE_KEY: '[ .. ]',
    PROCESSED_KEY: 2,
    DIR_KEY: 1,
    PARENT_KEY: None,
    ROOT_KEY: 0,
    CHILDREN_KEY: None},]

# ****************************************************************************************************
class CacheException( Exception ):
  def __init__(self, value):
    self.value = value
  def __str__(self):
    return repr(self.value)

# ****************************************************************************************************
class CachedFile:
  """
    Original file object replacement to allow dynamic caching
  """
  # ---------------------------------------------------
  def __init__( self, file ):
    """
      Constructor which takes original file object and cache where the data will be stored
    """
    self.size= self.status= 0
    # data is a dictionary of { offset: data }
    self.data= {}
    self.file= file
    self.close()
  
  # ---------------------------------------------------
  def getFile( self ):
    """
      Get the physical file out of CachedFile
    """
    return self.file
  
  # ---------------------------------------------------
  def isClosed( self ):
    """
      Whether cached file is closed
    """
    return self.pos== -1
  
  # ---------------------------------------------------
  def getSize( self ):
    return self.size
    
  # ---------------------------------------------------
  def setSize( self, size ):
    self.size= size
    
  # ---------------------------------------------------
  def getCachedSize( self ):
    """
      Get the size of cached files
    """
    return reduce( lambda x,y: x+ len( self.data[ y ] ), self.data, 0 )
  
  # ---------------------------------------------------
  def setStatus( self, status ):
    """
      Set status for the physical read op
    """
    self.status= status
  
  # ---------------------------------------------------
  def getStatus( self ):
    """
      Get status for the physical read op
    """
    return self.status
    
  # ---------------------------------------------------
  def release( self, size= 0 ):
    """
      Release cached file or its part from the memory and close all handles when no other chunks are left
    """
    # Apply filter only for opened file which is currently being read
    #print 'Trying to release %d' % size
    offsets= self.data.keys()
    released= 0
    if size and self.isClosed()== 0:
      offsets= filter( lambda x: (( x+ len( self.data[ x ] ) )<= self.pos ), self.data )
      offsets.sort()
    
    for i in offsets:
      j= len( self.data[ i ] )
      released+= j
      cache.deallocate( j )
      del( self.data[ i ] )
      if size:
        size-= j
        if size== 0:
          break
    
    # Scan all data
    if self.getCachedSize()== 0:
      self.data= {}
      self.offset= 0
      
    #print 'Released %d out of %d' % ( released, size )
    return released
  
  # ---------------------------------------------------
  def open( self ):
    """
      Open cached file for reading either from cache or from the physical placement
    """
    self.pos= self.offset= 0
    if self.getSize()== -1:
      try:
        if self.file.has_key( CDDA_KEY ):
          f= self.file[ DEVICE_KEY ].open( self.file[ TRACK_KEY ]- 1 )
        else:
          f= open( cache.getPathName( self.file ), 'rb' )
      except:
        traceback.print_exc()
        self.file[ TITLE_KEY ]= MISSING+ self.file[ NAME_KEY ]
        return None
      
      # Get the file size
      f.seek( 0, 2 )
      self.setSize( f.tell() )
      f.close()
    
    return self
  
  # ---------------------------------------------------
  def close( self ):
    """
      Close cached file logically
    """
    if len( self.data ):
      if min( self.data )!= 0:
        # Free up the file if the file is partially read.
        #print 'File is closed'
        self.release()
    
    self.pos= self.offset= self.size= -1
  
  # ---------------------------------------------------
  def appendChunk( self, offset, chunk ):
    """
      Internal function to add a file chunk to the cache
    """
    offset1= offset+ len( chunk )
    # See if we have chunks intersected( it shouldn't be a case )
    s= filter(
      lambda x: ( x>= offset and x+ len( self.data[ x ] )< offset1 ),
      self.data )
    if len( s )> 0:
      raise CacheException( 'Cannot add the same buffer to cache twice for %s:%d' % \
        ( cache.getPathName( self.file ), offset ) )
    self.data[ offset ]= chunk
  
  # ---------------------------------------------------
  def getOffsets( self ):
    offsets= filter( lambda x: ( x+ len( self.data[ x ] )> self.pos ), self.data )
    offsets.sort()
    return offsets
  
  # ---------------------------------------------------
  def read( self, bytes ):
    """
      Read certain amount of bytes from cache. If not available, try to read from file.
    """
    global urgentEventQueue
    if self.pos== -1:
      raise CacheException( 'File %s cannot be read because its closed' % cache.getPathName( self.file ) )
    
    # Just a check for a real size
    if bytes+ self.pos> self.size:
      bytes= self.size- self.pos
    
    # Start getting data into the buffer
    res= []
    while bytes> 0:
      try:
        offsets= self.getOffsets()
      except RuntimeError:
        # Ignore error due to contention
        offsets= []
      
      if len( offsets )< MIN_CHUNKS_IN_CACHE and \
         self.getStatus()== 0 and \
         self.offset< self.size:
        self.setStatus( 1 )
        #print 'Try to load some chunks ', self.getCachedSize(), self.size, bytes, self.pos
        urgentEventQueue.put( ( 'EXECUTE', self.readFile, None ) )
      
      if len( offsets ):
        offs= self.pos- offsets[ 0 ]
        s= self.data[ offsets[ 0 ] ][ offs: offs+ bytes ]
        if len( s )<= bytes:
          del offsets[ 0 ]
        
        self.pos+= len( s )
        bytes-= len( s )
        res.append( s )
      elif self.getStatus()== 1:
        #print 'Waiting for read to complete '
        time.sleep( .1 )
      else:
        raise CacheException( 'File %s is corrupted.' % cache.getPathName( self.file ) )
    
    return string.join( res, '' )
  
  # ---------------------------------------------------
  def readFileChunks( self, f ):
    """
      Read file from external device
      Each file is read by even chunks with default CHUNK_SIZE size.
    """
    # Set status that data is about to be read
    s= ' '
    offset= self.offset
    while len( s ):
      try:
        # Read the whole file into the cache
        s= f.read( CHUNK_SIZE )
      except:
        traceback.print_exc()
        break
      
      if len( s )> 0:
        if cache.canAllocate( len( s ) )== 0 and cache.cleanUpLRU( len(s) )== 0:
          # No cache space available
          break
        
        cache.allocate( len( s ) )
        self.appendChunk( offset, s )
        offset+= len( s )
      
      # Exit immediately if file is closed( another file is requested )
      if self.isClosed():
        break
      
      # Let other processes run
      time.sleep( .01 )
    
    # Data can be taken from cache
    self.offset= offset
    self.setStatus( 0 )
  
  # ---------------------------------------------------
  def readFile( self, param ):
    """
      Background file read. Supports for 2 type of files: _raw_ and os file.
      When raw mode enabled it reads data directly from the device without using OSes file objects.
    """
    if self.file.has_key( CDDA_KEY ):
      f= self.file[ DEVICE_KEY ].open( self.file[ TRACK_KEY ]- 1 )
    else:
      f= open( cache.getPathName( self.file ), 'rb' )
    
    f.seek( self.offset, 0 )
    self.readFileChunks( f )
    f.close()

# **********************************************************************************************
# *
# **********************************************************************************************
class FileCache:
  """
    Local file cache, allows to store directory hierarchy information along with the
    file itself and its data
  """
  # ---------------------------------------------------
  def __init__( self ):
    """
      Cache constructor. Sets cache size to 0. Use setCacheSize() later.
    """
    self.cacheSize= 0
  
  # ---------------------------------------------------
  def setCacheSize( self, cacheSize ):
    """
      Set maximum allowed cache size allowed for files.
    """
    self.cacheSize= long( cacheSize )
  
  # ---------------------------------------------------
  def setRoot( self, rootDirs ):
    """
      Set root directories for the cache. The file would be cached when it belongs to the 
      root directory.
    """
    # Save roots for the files in a cache. No dirs allowed to be outside the defined roots.
    self.rootDirs= rootDirs
    # List of root objects
    self.roots= {}
    self.lruList= []
    # Total size occupied by cache
    self.totalSize= 0
    res= []
    # Get cdroms
    self.cdroms= [ pycdda.CD( x ) for x in range( pycdda.getCount() )]
    # Get cdrom names
    self.cdromNames= [ x.getName() for x in self.cdroms ]
    for rootDir in self.rootDirs:
      res.append( self.addFile( None, rootDir, 1 ))
    
    return res
  
  # ---------------------------------------------------
  def getDummyFile( self, filePath ):
    """
      Returns dummy file when file cannot be opened.
    """
    return { TITLE_KEY: translate( filePath ),
             NAME_KEY: translate( filePath ),
             DIR_KEY: 0,
             ROOT_KEY: None,
             PARENT_KEY: None,
             PROCESSED_KEY: 0 }
  
  # ---------------------------------------------------
  def getExtension( self, file ):
    """
      Return file extension
    """
    if file.has_key( CDDA_KEY ):
      return CDDA_EXT
    
    s= file[ NAME_KEY ].split( '.' )
    return s[ -1 ].lower()
  
  # ---------------------------------------------------
  def getFile( self, filePath, parent= None ):
    """
      Return the file from the internal list of files. If does not exists- return None.
    """
    filePath= translate( filePath )
    if parent== None:
      # No parent, find by name only
      if filePath in self.rootDirs:
        return self.roots[ filePath ]
      
      path, fileName= os.path.split( filePath )
      if fileName== '' and path not in self.rootDirs:
        raise CacheException( 'Wrong file name %s. Cannot find the root for it.' % filePath )
      
      parent= self.getFile( path )
    else:
      fileName= filePath
      
    fileName= translate( fileName )
    children= [ x[ NAME_KEY ] for x in parent[ CHILDREN_KEY ] ]
    if fileName in children:
      return parent[ CHILDREN_KEY ][ children.index( fileName ) ]
    
    raise CacheException( 'No file %s in folder %s' % ( str( filePath ), str( self.getPathName( parent )) ) )
  
  # ---------------------------------------------------
  # params is a ( file )
  def checkIsAudio( self, params ):
    """
      Background check for a file type. If it is cdda audio, just set the isAudio parameter.
    """
    file= params[ 0 ]
    if file[ 'device' ].isReady():
      # The device is ready, try to read files from it
      # file[ 'isAudio' ]= file[ 'device' ].isAudio()
      s= os.popen( 'dir /S /N "%s"' % self.getPathName( file ) ).readlines()
      if string.strip( s[ 0 ][ 19: ] )== 'is Audio CD':
        file[ CDDA_KEY ]= 1
  
  # ---------------------------------------------------
  def delFile( self, filePath, parent= None ):
    """
      Delete file from the cache. Free up all the resources it occupies.
    """
    filePath= translate( filePath )
    file= self.getFile( filePath, parent )
    if file:
      # Remove its children
      try:
        children= file[ CHILDREN_KEY ]
        for child in children:
          self.delFile( cache.getPathName( child ), file )
      except:
        # No children
        pass
    
    # Remove parent's reference
    parent= file[ PARENT_KEY ]
    if parent!= None:
      # Should have children
      children= parent[ CHILDREN_KEY ]
      for i in xrange( len( children )):
        if children[ i ][ NAME_KEY ]== file[ NAME_KEY ]:
          break
      
      # Remove cached object
      del children[ i ]
  
  # ---------------------------------------------------
  def addFile( self, parent, filePath, isDir, order= -1 ):
    """
      Add new file to the cache or return existing one.
    """
    global eventQueue
    filePath= translate( filePath )
    # Verify whether file already exists
    try:
      fileObj= self.getFile( filePath, parent )
      return fileObj
    except:
      pass
    
    # Add child to the parent if exists
    fileName= filePath
    if parent== None:
      # Process root
      parentPath, fileName= os.path.split( filePath )
      if filePath in self.rootDirs:
        file= { TITLE_KEY: fileName, NAME_KEY: fileName, DIR_KEY: isDir, ROOT_KEY: filePath, PARENT_KEY: None, PROCESSED_KEY: 0 }
        if filePath in self.cdromNames:
          print 'Removable ', filePath
          # Set properties to the new file in a cache
          file[ 'removable' ]= 1
          file[ 'device' ]= self.cdroms[ self.cdromNames.index( filePath ) ]
          eventQueue.put( ( 'EXECUTE', self.checkIsAudio, file ))
        
        self.roots[ filePath ]= file
        return file
      
      # Verify the file can be added and exists under the root dir
      if fileName== '':
        raise CacheException( 'The file or folder "%s" cannot be cached. It does not belong to the root tree. Check your file name.' % parentPath )
      
      # Get parent for the current file
      parent= self.addFile( None, parentPath, 1 )
    
    # Create new file
    file= { TITLE_KEY: fileName, NAME_KEY: fileName, DIR_KEY: isDir, PARENT_KEY: parent, PROCESSED_KEY: 0 }
    
    # Add child to the parent
    self.addChild( parent, file, order )
    return file
  
  # ---------------------------------------------------
  def addChild( self, parent, child, order= -1 ):
    """
      Add new child to the current file. If the file already have exctly the same child, just do nothing.
    """
    # Assume the parent is a dir, otherwise raise an exception
    try:
      childrenList= parent[ CHILDREN_KEY ]
    except:
      childrenList= []
      parent[ CHILDREN_KEY ]= childrenList
    
    if child[ NAME_KEY ] not in [ x[ NAME_KEY ] for x in filter( lambda x: x!= None, childrenList )]:
      # Verify the order. If set, use it
      if order== -1:
        childrenList.append( child )
      else:
        while len( childrenList )< order:
          childrenList.append( None )
        
        childrenList[ order- 1 ]= child
  
  # ---------------------------------------------------
  def getChildren( self, file ):
    """
      Return list of children for the file or None if not exists.
    """
    try: return file[ CHILDREN_KEY ]
    except: return None
  
  # ---------------------------------------------------
  def getPathName( self, file ):
    """
      Return full path name to a file
    """
    if file[ PARENT_KEY ]== None:
      if file[ ROOT_KEY ]: return file[ ROOT_KEY ]
      else: return file[ NAME_KEY ]
      
    return os.path.join( self.getPathName( file[ PARENT_KEY ] ), file[ NAME_KEY ] )
  
  # ---------------------------------------------------
  def setProperty( self, file, propName, propValue ):
    """
      Set file's property
    """
    file[ propName ]= propValue
  
  # ---------------------------------------------------
  def getProperty( self, file, property ):
    """
      Return specified property for a file
    """
    return file[ property ]
  
  # ---------------------------------------------------
  def open( self, file, noCache= 0 ):
    """
      Open file which exists in cache for reading.
      It may be opened as regular file or cached file.
    """
    # Just return a file object when no caching is needed
    if noCache== 1:
      return open( self.getPathName( file ), 'rb' )
    
    # Verify if we have cache already
    try: f= file[ 'object' ]
    except: f= CachedFile( file )
    
    f= f.open()
    # Place accessed file in the beginning of the LRU list
    if f:
      try: self.lruList.remove( f )
      except: pass
      self.lruList.append( f )
      file[ 'object' ]= f
    
    return f

  # ---------------------------------------------------
  def allocate( self, size ):
    """
    """
    self.totalSize+= size
  
  # ---------------------------------------------------
  def deallocate( self, size ):
    """
    """
    self.totalSize-= size
    
  # ---------------------------------------------------
  def canAllocate( self, size ):
    """
      Return whether cache can allocate needed amount of memory
    """
    return ( self.totalSize+ size ) < self.cacheSize
    
  # ---------------------------------------------------
  def releaseFileCache( self, file, size= 0 ):
    """
      Release cache file
    """
    removed= file.release( size )
    if file.getCachedSize()== 0:
      try:
        del( self.lruList[ self.lruList.index( file ) ] )
        #print 'File released...'
      except: print 'File not released...'; pass;
      del file.getFile()[ 'object' ]
    
    return removed
    
  # ---------------------------------------------------
  def cleanUpLRU( self, size= 0 ):
    """
      Cleanup LRU lists to free up some space if needed.
    """
    # See if total size exceeds the allowed size, remove some file from the cache
    #print self.totalSize, size, self.cacheSize
    while ( self.totalSize+ size )> self.cacheSize and len( self.lruList )> 0:
      if self.releaseFileCache( self.lruList[ 0 ], size )== 0:
        break
    
    return ( self.totalSize+ size )<= self.cacheSize

cache= FileCache()

# **********************************************************************************************
# test scripts
# **********************************************************************************************
if __name__ == "__main__":
  import traceback, time
  pycdda.init()
  urgentEventQueue= Queue.Queue()

  # -------------------------------------
  def translate( s ):
    return s
  
  # Start queue processor
  # -------------------------------------
  def bgExecutor( eventQueue ):
    # Prepare surface to work with
    print 'Executor started'
    while 1:
      cmd= eventQueue.get()
      
      if cmd[ 0 ]== 'STOP':
        break
      
      if cmd[ 0 ]== 'EXECUTE':
        cmd[ 1 ]( cmd[ 2: ] )
      
    print 'Executor stopped'
  
  # -------------------------------------
  def fileCacheTest():
    global cache
    ROOT= ( 'c:\\music', 'c:\\temp', 'd:\\' )
    cache.setRoot( ROOT )
    cache.setCacheSize( 20000000 )
    
    # This call should succeed and should create 4 entries in a cache
    file= 'c:\\music\\ftv2.mp2'
    print 'Add file %s to the empty hierarchy' % file, '->',
    try:
      f= cache.addFile( None, file, 0 )
      print f
      print 'succeeded'
    except:
      traceback.print_exc()
      print 'failed'
      
    # This call should fail
    file= 'c:\\Python22\\python.exe'
    print 'Add file %s to the wrong hierarchy should fail' % file, '->',
    try:
      cache.addFile( None, file, 0 )
      print 'failed'
    except:
      traceback.print_exc()
      print 'succeeded'
    
    # This call should succeed
    file= 'c:\\music'
    print 'Get root folder %s should succeed' % file, '->',
    try:
      print cache.getFile( file )
      print 'succeeded'
    except:
      traceback.print_exc()
      print 'failed'
    
    # This call should succeed
    file= 'c:\\music\\ftv2.mp2'
    print 'Get folder %s should succeed' % file, '->',
    try:
      f= cache.getFile( file )
      print f
      print 'succeeded'
    except:
      traceback.print_exc()
      print 'failed'
    
    # This call should succeed
    print 'Get name should succeed', '->',
    try:
      print cache.getPathName( f )
      print 'succeeded'
    except:
      traceback.print_exc()
      print 'failed'

    # This call should succeed and should create 4 entries in a cache
    file= 'd:\\'
    print 'Add root file %s to the empty hierarchy' % file, '->',
    try:
      f= cache.addFile( None, file, 1 )
      print f
      print 'succeeded'
    except:
      traceback.print_exc()
      print 'failed'
      
    # This call should succeed and should create 4 entries in a cache
    file= 'c:\\music'
    print 'Try to remove folder %s' % file, '->',
    try:
      cache.delFile( file )
      print 'succeeded'
    except:
      traceback.print_exc()
      print 'failed'
    
  try:
    thread= threading.Thread( target= bgExecutor, args= ( urgentEventQueue, ) )
    thread.start()
    
    fileCacheTest()
    f= cache.getFile( 'd:\\' )
    time.sleep( 0.5 )
    print 'Root removable ', f
    
    # Verify file cache for read
    fileName= 'c:\\music\\ftv2.mp2'
    print 'Try to read file %s from a cache ' % fileName
    file= cache.getFile( fileName )
    f= cache.open( file )
    # Get timing for the first read
    t1= time.time()
    totRead= 0
    s= ' '
    while len( s ):
      s= f.read( 200000 )
      totRead+= len( s )
    
    print 'Total read %d bytes out of %d in %f secs' % ( totRead, f.getSize(), time.time()- t1 )
      
    # Get timing for the second read
    t1= time.time()
    s= ' '
    totRead= 0
    f= cache.open( file )
    while len( s ):
      s= f.read( 200000 )
      totRead+= len( s )
    
    print 'Total cache read %d bytes out of %d in %f secs' % ( totRead, f.getSize(), time.time()- t1 )
  finally:
    urgentEventQueue.put( ( 'STOP', ) )

