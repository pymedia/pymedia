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
  def onAudioReady( self, afr ):
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

# ******************************************************
# 
# ******************************************************
def run( file ):
  pygame.init()
  pygame.display.set_mode( (800,600), 0 )
  callback= PlayerCallback()
  player= pymedia.Player( callback )
  player.start()
  player.startPlayback( file )
  stopped= False
  
  while player.isPlaying() and not stopped:
    time.sleep( PLAYER_DELAY )
    ev= pygame.event.get()
    for e in ev:
      if e.type== pygame.KEYDOWN:
        if e.key== pygame.K_ESCAPE: 
          stopped= True
        if e.key== pygame.K_LEFT:
          print 'seek left'
        if e.key== pygame.K_RIGHT:
          print 'seek right'
  
  player.stopPlayback()
  player.stop()

if __name__ == "__main__":
  # ----------------------------------------------------------------------------------
  # Play any compressed media file supported by pymedia
  # http://pymedia.org/
  if len( sys.argv )!= 2:
    print "Usage: player <filename>"
  else:
    run( sys.argv[ 1 ] )
