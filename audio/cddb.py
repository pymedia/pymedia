import string, os

# **********************************************************************************************
# *
# **********************************************************************************************
class CDDB:
  """
    Local CDDB lookup and synchronization.
    No remote CDDB support yet.
  """
  # ---------------------------------------------------
  def __init__( self, path ):
    self.path= path
  
  # ---------------------------------------------------
  def getDiskId( self, tracksInfo ):
    """ returns disc id based on tracksInfo which is tuple of ( track_start, track_length )"""
    # check sum
    def sumDigits( val ):
      return int( reduce( lambda x, y: int( y )+ int( x ), str( val ) ) )
    
    # check sum 
    checkSum= reduce( lambda x, y: x+ y, map( lambda x: sumDigits( int( x[ 0 ]/75 )), tracksInfo ) )
    # total length
    totalTime= reduce( lambda x, y: x+ y, map( lambda x: x[ 1 ], tracksInfo ) )
    
    return ((checkSum % 0xff) << 24 | int( totalTime/75 ) << 8 | len( tracksInfo ) )
  
  # ---------------------------------------------------
  def getDiscInfo( self, tracksInfo ):
    # Find all files that may be suitable
    diskId= self.getDiskId( tracksInfo )
    hexId= '%x' % diskId
    fName= '%sto%s' % ( hexId[ :2 ], hexId[ :2 ] )
    res= { 'DTITLE': ' Unknown artist / Unknown title' }
    try:
      files= os.listdir( self.path )
    except:
      files= ()
    
    for file in files:
      dir= os.path.join( self.path, file )
      if os.path.isdir( dir ):
        # Go into the directory and try to find file with the fName name
        dirFiles= os.listdir( dir )
        if fName in dirFiles:
          # File was found, search for a id in a file
          f= open( os.path.join( dir, fName ))
          data= f.readlines()
          try:
            # Match track offsets to be sure we found the right file
            index= data.index( '#FILENAME=%s\n' % hexId )
            index+= data[ index: index+ 20 ].index( '# Track frame offsets:\n' )
            for i in xrange( len( tracksInfo ) ):
              s= '#\t%d\n' % ( int( tracksInfo[ i ][ 0 ] ) )
              if data[ index+ i+ 1 ]!= s:
                break
            
            # If tracks does not macth, continue searching
            if i+ 1< len( tracksInfo ):
              continue
            
            index+= i+ 8
            while index< len( data ):
              if data[ index ][ :9 ]== '#FILENAME':
                break
              
              # Save pair to the dictionary
              if data[ index ][ 0 ]!= '#':
                tmp= string.split( data[ index ], '=' )
                res[ tmp[ 0 ] ]= string.strip( tmp[ 1 ] )
              
              index+= 1
            
            return res
          except:
            pass
    
    # Populate unknown track names
    for i in xrange( len( tracksInfo ) ):
      res[ 'TTITLE%d' % i ]= '%02d - Unknown track' % i
    
    return res

