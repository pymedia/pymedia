
import Queue, os, string, threading, time, pycdda, traceback
from __main__ import *

CHUNK_SIZE= 500000
SECTORS_PER_READ= 40 
MISSING= '<missing> '

# **********************************************************************************************
# *
# **********************************************************************************************
class CachedFile:
  """ CachedFile substitute original file object to allow caching """
  # ---------------------------------------------------
  def __init__( self, cache, file ):
    self.size= 0
    self.data= []
    self.cache= cache
    self.file= file
    self.complete= 0
    self.close()
  
  # ---------------------------------------------------
  def read( self, bytes ):
    if self.pos== -1:
      raise 'File %s cannot be read because its closed' % self.cache.getPathName( self.file )
    
    res= []
    while bytes> 0:
      res1= ''
      if self.chunk< len( self.data ):
        res1= self.data[ self.chunk ][ self.offset: self.offset+ bytes ]
        bytes-= len( res1 )
        res.append( res1 )
        self.offset+= len( res1 )
        if self.offset>= len( self.data[ self.chunk ] ):
          self.chunk+= 1
          self.offset= 0
      
      if len( res1 )== 0 or self.chunk>= len( self.data ):
        if self.complete== 1:
          break
        else:
          # The file is being read by background process, wait until completes
          time.sleep( .001 )
    
    return string.join( res, '' )
  
  # ---------------------------------------------------
  def getFile( self ):
    return self.file
  
  # ---------------------------------------------------
  def release( self ):
    self.data= []
    del self.file[ 'object' ]
    self.close()
  
  # ---------------------------------------------------
  def open( self ):
    self.pos= self.offset= self.chunk= 0
  
  # ---------------------------------------------------
  def isClosed( self ):
    return self.pos== -1
  
  # ---------------------------------------------------
  def close( self ):
    self.pos= self.offset= -1
    self.chunk= 0
  
  # ---------------------------------------------------
  def getSize( self ):
    return self.size
  
  # ---------------------------------------------------
  def _appendChunk( self, chunk ):
    self.data.append( chunk )
    self.size+= len( chunk )
  
  # ---------------------------------------------------
  def _setComplete( self ):
    self.complete= 1

  # ---------------------------------------------------
  def _setIncomplete( self ):
    self.complete= -1
    self.release()

# **********************************************************************************************
# *
# **********************************************************************************************
class FileCache:
  """
    Local file cache, allows to store hierarchy information along with the
    file name and parent directory
  """
  # ---------------------------------------------------
  def __init__( self ):
    
    # Initialize cdda devices
    pycdda.init()
    self.cacheSize= 0
  
  # ---------------------------------------------------
  def setCacheSize( self, cacheSize ):
    self.cacheSize= long( cacheSize )
  
  # ---------------------------------------------------
  def setRoot( self, rootDirs ):
    # Save roots for the files in a cache. No dirs allowed to be outside the defined roots.
    self.rootDirs= rootDirs
    # List of root objects
    self.roots= {}
    self.lruList= []
    # Total size occupied by cache
    self.totalSize= 0
    res= []
    # Get cdroms
    self.cdroms= map( lambda x: pycdda.CD( x ), range( pycdda.getCount() ))
    # Get cdrom names
    self.cdromNames= map( lambda x: x.getName(), self.cdroms )
    for rootDir in rootDirs:
      res.append( self.addFile( None, rootDir, 1 ))
    
    return res
  
  # ---------------------------------------------------
  def getDummyFile( self, filePath ):
    return { 'name': filePath, 'isDir': 0, 'root': None, 'parent': None }
  
  # ---------------------------------------------------
  def getExtension( self, file ):
    s= str.split( file[ 'name' ], '.' )
    return string.lower( s[ -1 ] )
  
  # ---------------------------------------------------
  def getFile( self, filePath, parent= None ):
    if parent== None:
      # No parent, find by name only
      if filePath in self.rootDirs:
        return self.roots[ filePath ]
      
      path, fileName= os.path.split( filePath )
      if fileName== '' and path not in self.rootDirs:
        raise 'Wrong file name %s. Cannot find the root for it.' % filePath
      
      parent= self.getFile( path )
    else:
      fileName= filePath
      
    children= map( lambda x: x[ 'name' ], parent[ 'children' ] )
    if fileName in children:
      return parent[ 'children' ][ children.index( fileName ) ]
    
    raise 'No file %s in folder %s' % ( str( filePath ), str( self.getPathName( parent )) )
  
  # ---------------------------------------------------
  # params is a ( file )
  def checkIsAudio( self, params ):
    file= params[ 0 ]
    #file[ 'device' ].init()
    #tracks= map( lambda x: file[ 'device' ].get_track_audio( x ), range( file[ 'device' ].get_numtracks() ))
    #file[ 'device' ].quit()
    if file[ 'device' ].isReady():
      # The device is ready, try to read files from it
      s= os.popen( 'dir /S /N "%s"' % self.getPathName( file ) ).readlines()
      file[ 'isAudio' ]= ( string.strip( s[ 0 ][ 19: ] )== 'is Audio CD' )
    else:
      file[ 'isAudio' ]= 0
  
  # ---------------------------------------------------
  def delFile( self, filePath, parent= None ):
    global cache
    file= self.getFile( filePath, parent )
    if file:
      # Remove its children
      try:
        children= file[ 'children' ]
        for child in children:
          self.delFile( cache.getPathName( child ), file )
      except:
        # No children
        pass
    
    # Remove parent's reference
    parent= file[ 'parent' ]
    if parent!= None:
      # Should have children
      children= parent[ 'children' ]
      for i in xrange( len( children )):
        if children[ i ][ 'name' ]== file[ 'name' ]:
          break
      
      del children[ i ]
  
  # ---------------------------------------------------
  def addFile( self, parent, filePath, isDir, order= -1 ):
    global eventQueue
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
        file= { 'name': translate( fileName ), 'isDir': isDir, 'root': filePath, 'parent': None }
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
        raise 'The folder %s cannot be cached unless it is a root. Check your file name.' % filePath
      
      # Get parent for the current file
      parent= self.addFile( None, parentPath, 1 )
    
    # Create new file
    file= { 'name': fileName, 'isDir': isDir, 'parent': parent }
    
    # Add child to the parent
    self.addChild( parent, file, order )
    return file
  
  # ---------------------------------------------------
  def addChild( self, parent, child, order= -1 ):
    # Assume the parent is a dir, otherwise raise an exception
    try:
      childrenList= parent[ 'children' ]
    except:
      childrenList= []
      parent[ 'children' ]= childrenList
    
    if child[ 'name' ] not in map( lambda x: x[ 'name' ], filter( lambda x: x!= None, childrenList )):
      # Verify the order. If set, use it
      if order== -1:
        childrenList.append( child )
      else:
        while len( childrenList )< order:
          childrenList.append( None )
        
        childrenList[ order- 1 ]= child
  
  # ---------------------------------------------------
  def getChildren( self, file ):
    try:
      return file[ 'children' ]
    except:
      return None
  
  # ---------------------------------------------------
  def getPathName( self, file ):
    if file[ 'parent' ]== None:
      if file[ 'root' ]:
        return file[ 'root' ]
      else:
        return file[ 'name' ]
    
    return os.path.join( self.getPathName( file[ 'parent' ] ), file[ 'name' ] )
  
  # ---------------------------------------------------
  def setProperty( self, file, propName, propValue ):
    file[ propName ]= propValue
  
  # ---------------------------------------------------
  def getProperty( self, file, property ):
    return file[ property ]
  
  # ---------------------------------------------------
  def open( self, file, noCache= 0 ):
    # Just return a file object when no caching is needed
    global eventQueue
    if noCache== 1:
      return open( self.getPathName( file ), 'rb' )
    
    try:
      # Verify we have cache already
      f= file[ 'object' ]
      f.open()
    except:
      f= CachedFile( self, file )
      f1= None
      if not file.has_key( 'isCDDA' ):
        try:
          # Try to open the file first
          f1= open( self.getPathName( file ), 'rb' )
        except:
          traceback.print_exc()
          
          if file.has_key( 'processing' ):
            file[ 'title' ]= MISSING+ file[ 'name' ]
          
          return None
      
      # Submit read for the file
      file[ 'object' ]= f
      f.open()
      eventQueue.put( ( 'EXECUTE', self._readFile, file, f1 ) )
    
    return f

  # ---------------------------------------------------
  def cleanUpLRU( self ):
    # See if total size exceeds the allowed size, remove some file from the cache
    totalRemoved= 0
    while self.totalSize> self.cacheSize and len( self.lruList )> 0:
      if len( self.lruList )> 0:
        f1= self.lruList[ 0 ]
        self.totalSize-= f1.getSize()
        totalRemoved+= f1.getSize()
        f1.release()
        del self.lruList[ 0 ]
    
    return totalRemoved
  
  # ---------------------------------------------------
  def readOSFile( self, fileObj, f ):
    s= ' '
    while len( s ):
      try:
        # Read the whole file into the cache
        s= f.read( CHUNK_SIZE )
      except:
        traceback.print_exc()
        return 0
      
      if len( s )> 0:
        fileObj._appendChunk( s )
      
      # Exit immediately if file is closed
      if fileObj.isClosed():
        return 0 
      
      # Let other processes run
      time.sleep( .001 )
    
    return 1
  
  # ---------------------------------------------------
  def _readFile( self, param ):
    file, f= param
    bytesRead= 0
    fileObj= self.getProperty( file, 'object' )
    
    # Whether current file is an audio file
    if file.has_key( 'isCDDA' ):
      sectors= file[ 'sectors' ]
      iFrom= sectors[ 0 ]
      iTo= sectors[ 0 ]+ sectors[ 1 ]- 1
      for i in xrange( iFrom, iTo, SECTORS_PER_READ ):
        try:
          if i+ SECTORS_PER_READ> iTo:
            s= file[ 'device' ].read( i, iTo )
          else:
            s= file[ 'device' ].read( i, i+ SECTORS_PER_READ )
        except:
          traceback.print_exc()
          fileObj.release()
          return
        
        # Exit immediately if file is closed
        if fileObj.isClosed():
          fileObj.release()
          return
        
        fileObj._appendChunk( s )
        # Let other processes run
        time.sleep( .001 )
    else:
      # Process file
      try:
        if self.readOSFile( fileObj, f )== 0:
          fileObj.release()
          return
      finally:
        # Close physical file
        f.close()
    # 
    self.totalSize+= fileObj.getSize()
    self.cleanUpLRU()
    
    # Add to the lru list
    self.lruList.append( fileObj )
    fileObj._setComplete()
  
# **********************************************************************************************
# test scripts  
# **********************************************************************************************
if __name__ == "__main__":
  import traceback, time
  eventQueue= Queue.Queue()
  
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
    ROOT= ( 'c:\\bors', 'c:\\temp', 'd:\\' )
    cache= FileCache()
    cache.setRoot( ROOT )
    
    # This call should succeed and should create 4 entries in a cache
    file= 'c:\\bors\\music\\Neil Landstrumm\\Pro Audio\\Down On E.mp3'
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
    file= 'c:\\bors'
    print 'Get root folder %s should succeed' % file, '->',
    try:
      print cache.getFile( file )
      print 'succeeded'
    except:
      traceback.print_exc()
      print 'failed'
    
    # This call should succeed
    file= 'c:\\bors\\music\\Neil Landstrumm\\Pro Audio\\Down On E.mp3'
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
    file= 'c:\\bors\\music'
    print 'Try to remove folder %s' % file, '->',
    try:
      cache.delFile( file )
      print 'succeeded'
    except:
      traceback.print_exc()
      print 'failed'
    
    # This call should succeed
    file= 'c:\\bors\\music\\Neil Landstrumm\\Pro Audio\\Down On E.mp3'
    print 'Get folder %s should not succeed' % file, '->',
    try:
      f= cache.getFile( file )
      print f
      print 'failed'
    except:
      traceback.print_exc()
      print 'succeeded'
      
    file= 'c:\\bors\\music\\Neil Landstrumm\\Pro Audio\\Down On E.mp3'
    f= cache.addFile( None, file, 0 )
    
    return cache
  
  thread= threading.Thread( target= bgExecutor, args= ( eventQueue, ) )
  thread.start()
  
  cache= fileCacheTest()
  f= cache.getFile( 'd:\\' )
  time.sleep( 0.5 )
  print 'Root removable ', f
  
  # Verify file cache for read
  fileName= 'c:\\bors\\music\\Neil Landstrumm\\Pro Audio\\Down On E.mp3'
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
  eventQueue.put( ( 'STOP', ) )
  
