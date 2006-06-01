import sys, thread, time, traceback, Queue, os
import pymedia

PLAYER_DELAY= 0.03

try:
  # Try to use pydfb if possible
  import pydfb as pygame
  YV12= pygame.PF_YV12
except:
  # only pygame is available
  import pygame
  YV12= pygame.YV12_OVERLAY

# ******************************************************
# 
# ******************************************************
class PlayerCallback:
  # ------------------------------------
  def __init__( self ):
    self.overlay= None
  
  # ------------------------------------
  def removeOverlay( self ):
    # Remove overlay if any
    self.overlay= None
  
  # ------------------------------------
  def createOverlay( self, vfr ):
    # Create overlay if any
    self.overlay= pygame.Overlay( YV12, vfr.size )
    # Locate overlay on the screen
    res= pygame.display.get_surface().get_size()
    self.overlay.set_location( (0,0)+res )
  
  # ------------------------------------
  # Process incoming audio data ( usefull for visualization or sound post processing )
  # You can return audio stream as a string once you did something to it
  def onAudioReady( self, data, sampleRate, channels ):
    pass
  
  # ------------------------------------
  def onVideoReady( self, vfr ):
    if not self.overlay:
      self.createOverlay( vfr )
    
    # Display it
    try:
      # Part for a new pydfb and pygame syntax
      self.overlay.set_data( vfr.data )
      self.overlay.display()
    except:
      # old pygame syntax if any
      self.overlay.display( vfr.data )

  # ------------------------------------
  def onPlaybackEnd( self, player ):
    player.stopPlayback()

  # ------------------------------------
  # Called every 1 sec
  def onTimeChange( self, pos ):
    pass

# ******************************************************
# 
# ******************************************************
def run( file ):
  pygame.init()
  pygame.display.set_mode( (800,600), 0 )
  callback= PlayerCallback()
  player= pymedia.Player( callback )
  player.start()
  # Stat the path
  if os.path.isfile( file ):
    files= [ file ]
  else:
    files= [ os.path.join( file, x ) for x in os.listdir( file ) ]
  
  stopped= False
  i= 0
  while i< len( files ) and not stopped:
    f= files[ i ]
    print 'Playing %s...' % f
    player.startPlayback( open( f, 'rb' ), f.split( '.' )[ -1 ].lower() )
    i+= 1
    
    while player.isPlaying() and not stopped:
      time.sleep( PLAYER_DELAY )
      ev= pygame.event.get()
      br= False
      for e in ev:
        if e.type== pygame.KEYDOWN:
          if e.key== pygame.K_ESCAPE: 
            stopped= True
          if e.key== pygame.K_LEFT:
            print 'seek backward', player.getPosition(), player.getLength()
            player.seekTo( player.getPosition()- 20 )
          if e.key== pygame.K_RIGHT:
            print 'seek forward', player.getPosition(), player.getLength()
            player.seekTo( player.getPosition()+ 10 )
          if e.key== pygame.K_UP:
            br= True
            break
          if e.key== pygame.K_DOWN:
            if i> 1:
              br= True
              i-= 2
              break
      
      if br:
        break
  
  if len( player.getError() )> 0:
    print 'Error detected'
    print player.getError()[ 0 ][ 1 ]
  
  player.stopPlayback()
  player.stop()
  while player.isRunning():
    time.sleep( PLAYER_DELAY )

if __name__ == "__main__":
  # ----------------------------------------------------------------------------------
  # Play any compressed media file supported by pymedia
  # http://pymedia.org/
  if len( sys.argv )!= 2:
    print "Usage: player <filename1_or_path>"
  else:
    run( sys.argv[ 1 ] )
