
import sys, os, string, threading, Queue, traceback, time
import pygame, pymedia

TRANSPARENT= ( 38, 109, 163 )

# -------------------------------------------------------------------------
def translate( s ):
  global codepages
  if s:
    for codepage in codepages:
      try: return unicode( string.strip( s ), codepage )
      except: pass
  
  return s

# ------------------------------------------------------------------------------------------------------
def bgExecutor( queue ):
  # Start executor which makes possible background work
  print 'Executor started'
  while 1:
    cmd= queue.get()
    
    if cmd[ 0 ]== 'STOP':
      break
    
    if cmd[ 0 ]== 'EXECUTE':
      try:
        cmd[ 1 ]( cmd[ 2: ] )
      except:
        traceback.print_exc()
    
  print 'Executor stopped'
 
# ------------------------------------------------------------------------------------------------------
def startBgExecutor():
  global eventQueue
  eventQueue= Queue.Queue()
  thread= threading.Thread( target= bgExecutor, args= ( eventQueue, ) )
  thread.start()
  return thread

# ------------------------------------------------------------------------------------------------------
def hideAll( areas ):
  for s in areas:
    areas[ s ].setVisible( 0 )
    areas[ s ].setActive( 0 )

# ------------------------------------------------------------------------------------------------------
def showAll( names, areas ):
  for name in names:
    areas[ name ].setVisible( 1 )

# ------------------------------------------------------------------------------------------------------
def setActive( area, altArea= None ):
  global activeArea
  
  if altArea:
    if len( area.getWrapper().items() )== 0:
      area= altArea
  
  if activeArea:
    activeArea.setActive( 0 )
  area.setVisible( 1 )
  activeArea= area
  activeArea.setActive( 1 )

# ------------------------------------------------------------------------------------------------------
def asList( s ):
  return map( lambda x: int( x ), string.split( s, ',' ) )

# ------------------------------------------------------------------------------------------------------
def drawArea( screen, name ):
  global areasCache, areas
  pos= areas[ name ].getPos()
  if areas[ name ].hasChanged() or not areasCache.has_key( name ):
    cacheScr= areas[ name ].render()
  else:
    cacheScr= areasCache[ name ]
    
  screen.blit( cacheScr, pos )
  areasCache[ name ]= cacheScr

# ------------------------------------------------------------------------------------------------------
def mainTest( fileName ):
  global codepages
  import parser
  pygame.init()
  
  startBgExecutor()
  try:
    global EXIT_FLAG
    config= parser.ConfigElement()
    config.parseFile( fileName )

    # Print result
    """print 'Apps', apps
    print 'Modules', modules
    print 'Areas', areas
    print 'Menus', menuDefs
    print 'Active area', activeArea"""
    
    app= apps[ 'homedia' ]
    codepages= app.params[ 'params' ][ 'codepages' ]
    codepages.append( 'utf-8' )
    resStr= app.getParam( 'resolution' )
    fullScreen= app.getParam( 'fullScreen' )
    if fullScreen== 'yes':
      fullScreen= pygame.FULLSCREEN
    else:
      fullScreen= 0
      
    screen = pygame.display.set_mode( asList( resStr ), fullScreen )
    pygame.display.set_caption( app.getParam( 'caption' ))
    
    # Cache for already rendered areas( don't think it will help performance, but still... )
    while EXIT_FLAG== 0:
      #use event.wait to keep from polling 100% cpu
      event = pygame.event.wait()
      # See if we can show area
      changed= map( lambda( x ):  areas[ x ].isVisible() and areas[ x ].hasChanged(), areas )
      if changed.count( 1 ):
        screen.fill( asList( app.getParam( 'bgcolor' )) )
        aArea= None
        for s in areas:
          if areas[ s ].isVisible():
            if areas[ s ].isActive():
              aArea= s
            else:
              drawArea( screen, s )
        
        if aArea:
          drawArea( screen, aArea )
        
        pygame.display.flip()
      
      if event.type is pygame.KEYDOWN:
        if event.key== pygame.K_PAUSE:
          try:
            #pymedia.pypower.suspendSystem( 1 )
            pass
          except:
            traceback.print_exc()
        else:
          try:
            activeArea.processKey( event.key )
          except:
            traceback.print_exc()
            break
  finally:
    eventQueue.put( ( 'STOP', ) )
    for s in modules:
      modules[ s ].unload()

# Test media module 
if __name__== '__main__':
  if len( sys.argv )!= 2:
    print "Usage: media <config_xml_file>"
  else:
     # Create basic dictionaries
    apps= {}
    areas= {}
    modules= {}
    menuDefs= {}
    activeArea= None
    codepages= ()
    eventQueue= None
    areasCache= {}
    EXIT_FLAG= 0
    mainTest( sys.argv[ 1 ] )

 