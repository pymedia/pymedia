##    generic - Part of the homedia package. Menu drawing utilities.
##        Main drawing classes, supporting other modules in representing
##        information on the screen
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

import os, Queue, threading, string, traceback, time
import pygame
from __main__ import *

DEFAULT_FONT= 'arial'
DEFAULT_FONT_SIZE= 20
DEFAULT_FONT_COLOR= ( 255, 255, 255 )
LEFT_C= (0,0)

# ****************************************************************************************************
# Generic class to support menu items rendering
# Provides basic set of functions such as:
#   getItemSize() -> ( width, height )
#     etc
class MenuHelper:
  # -----------------------------------------------------------------
  # Constructor
  def __init__( self, rect, params ):
    self.attributes= params
    self.params= params[ 'params' ]
    self.parentParams= params
    self.changed= 1
  
  # -----------------------------------------------------------------
  # Return item size 
  def getItemSize( self ):
    return self.itemSize
  
  # -----------------------------------------------------------------
  def getRect( self ):
    return self.rect
  
  # -----------------------------------------------------------------
  def getPos( self ):
    return self.rect[ :2 ]
  
  # -----------------------------------------------------------------
  def getParam( self, name, default ):
    try:
      return self.params[ name ]
    except:
      return default
    
  # -----------------------------------------------------------------
  def getParamList( self, name, default ):
    try:
      return eval( '('+ self.params[ name ]+ ')' )
    except:
      return default
  
  # -----------------------------------------------------------------
  def setFocus( self, item, isFocused ):
    # Only icon item will change for the focused item
    self.drawItem( item, isFocused )
    
  # -----------------------------------------------------------------
  def adjustPos( self, items, offsetPos ):
    # Filter out only existing items
    items= filter( lambda x: x, items )
    # Adjust pos for all of the items in a list
    return map( lambda x: ( x[ 0 ], ( x[ 1 ][ 0 ]+ offsetPos[ 0 ], x[ 1 ][ 1 ]+ offsetPos[ 1 ] )), items )
  
  # -----------------------------------------------------------------
  def hasChanged( self ):
    return self.changed
  
  # -----------------------------------------------------------------
  def setChanged( self, changed ):
    self.changed= changed
  
  # -----------------------------------------------------------------
  def loadFont( self, paramName ):
    fontName= self.getParam( paramName, DEFAULT_FONT )
    fontSize= self.getParam( paramName+ 'Size', DEFAULT_FONT_SIZE )
    font= pygame.font.SysFont( fontName, int( fontSize ) )
    if self.getParam( paramName+ 'Bold', 'no' )== 'yes':
      font.set_bold( 1 )
    
    fontColor= self.getParamList( paramName+ 'Color', DEFAULT_FONT_COLOR )
    
    return font, fontColor
  
  # -----------------------------------------------------------------
  def loadIcon( self, paramName, default ):
    icons= self.getParam( paramName, default )
    icons1= None
    
    if type( icons )== str:
      icons1= pygame.image.load( 'icons/'+ icons )
    elif type( icons )== list or type( icons )== tuple:
      icons1= map( lambda x: pygame.image.load( 'icons/'+ x ), icons )
    
    return icons1

  # -----------------------------------------------------------------
  def execute( self, action, key, item ):
    from __main__ import *
    try:
      s1= self.attributes[ action ]
    except:
      return
    
    if self.attributes.has_key( 'module' ) and self.attributes[ 'module' ]:
      modParams= self.attributes[ 'module' ].params
      # Get actions variable
      if s1[ 0 ]== '%':
        s= eval( modParams[ 'actions' ] )
        s1= s[ self.attributes[ 'id' ]+ '.'+ s1[ 1: ] ]
    
    exec( s1 )
    

# ****************************************************************************************************
# Class to draw basic menu items
# Accepts the following params:
#   active - image name to draw below the active item
#   inactive - image name to draw below the inactive item
#   font - font name to draw menu items
#   fontSize - font size for the above
# 
# All params are optional
#
class MenuItem( MenuHelper ):
  # -----------------------------------------------------------------
  # Initialize menu renderer
  def __init__( self, rect, params ):
    # Initialize base class
    MenuHelper.__init__( self, rect, params )
    
    # Create font based on param passed
    self.font= self.loadFont( 'font' )
    self.itemSize= ( rect[ 2 ], self.font[ 0 ].get_height() )
    self.itemsWrapper= None
    
    # Scale default stripe
    self.stripeIcon= pygame.transform.scale( self.loadIcon( 'stripeIcons', ( 'stripe_1.bmp' ) ), self.itemSize )
  
  # -----------------------------------------------------------------
  def drawItem( self, itemPos, isFocused ):
    item= self.itemsWrapper.items()[ itemPos ]
    # Find out whether it is a directory or file
    text= self.font[ 0 ].render( item[ 'caption' ], 1 , self.font[ 1 ] )
    
    # Center text into rect
    resItem= []
    if isFocused== 1:
      resItem.append( ( self.stripeIcon, LEFT_C ) )
    
    resItem.append( ( text, ( 10, self.font[ 0 ].get_linesize()- self.font[ 0 ].get_height() ) ) )
    return resItem

  # -----------------------------------------------------------------
  def setItemsWrapper( self, itemsWrapper ):
    self.itemsWrapper= itemsWrapper
  
  # -----------------------------------------------------------------
  def processKey( self, itemPos, key ):
    self.itemsWrapper.processKey( itemPos, key )

# ****************************************************************************************************
# Generic base class for the display like components
class GenericDisplay( MenuHelper ):
  # -----------------------------------------------------------------
  # Constructor
  def __init__( self, rect, params, renderClass ):
    # Initialize base class
    MenuHelper.__init__( self, rect, params )
    
    self.rect= rect
    self.renderClass= renderClass
    self.visible= 0
    self.historyItems= []
    self.lastActiveItem= 0
    self.activeItem= -1
    self.active= 0
    self.setActive( 0 )
  
  # -----------------------------------------------------------------
  def setActive( self, isActive ):
    if isActive== 1 and self.active== 0:
      if self.lastActiveItem!= -1:
        self.activeItem= self.lastActiveItem
      if self.activeItem== -1:
        self.activeItem= 0
    elif isActive== 0 and self.active== 1:
      self.lastActiveItem= self.activeItem
      self.activeItem= -1
      
    self.active= isActive
    self.setChanged( 1 )
  
  # -----------------------------------------------------------------
  def setVisible( self, visible ):
    if self.visible!= visible:
      self.visible= visible
      self.setChanged( 1 )
      
      if visible== 0:
        self.setActive( 0 )
  
  # -----------------------------------------------------------------
  def isVisible( self ):
    return self.visible
  
  # -----------------------------------------------------------------
  def isActive( self ):
    return self.active
  
  # -----------------------------------------------------------------
  def getRenderer( self ):
    return self.renderClass
    
  # -----------------------------------------------------------------
  def hasChanged( self ):
    return MenuHelper.hasChanged( self ) or ( self.getRenderer() and self.getRenderer().hasChanged() )
  
# ****************************************************************************************************
# Plain version of list display. Accepts various renderers to draw the list
# 
class ListDisplay( GenericDisplay ):
  # -----------------------------------------------------------------
  def __init__( self, rect, params, renderClass ):
    """
      __init__( rect, renderClass, eventQueue ) ->
      Constructor for the class.
        renderClass - is a class which knows how to display certain type of files
                    control will display default icons and behaviors when None or not specified.
        eventQueue - allows to process some items in background for matter of reasons...
    """
    GenericDisplay.__init__( self, rect, params, renderClass )
    self.scrollFlag= self.getParam( 'scroll', 'no' )== 'yes'
    self.scrollFont= self.loadFont( 'scrollFont' )
    self.messageFont= self.loadFont( 'messageFont' )
    self.navIcons= self.loadIcon( 'navIcons', ( 'up_0.bmp', 'down_0.bmp' ) )
  
  # -----------------------------------------------------------------
  def init( self, itemsWrapper, activeItem= 0 ):
    """
    """
    # Initiate process of displaying the data
    itemsWrapper.init()
    self.renderClass.setItemsWrapper( itemsWrapper )
    self.itemsWrapper= itemsWrapper
    self.lastActiveItem= -1
    self.setItems( self.activeItem )
    self.moveTo( activeItem )
  
  # -----------------------------------------------------------------
  def getWrapper( self ):
    """
    """
    return self.itemsWrapper
  
  # -----------------------------------------------------------------
  def setItems( self, itemFrom ):
    """
    """
    if itemFrom< 0:
      itemFrom= 0
    
    self.startFrom= itemFrom
  
  # -----------------------------------------------------------------
  def moveTo( self, item ):
    """
    """
    if item< self.getItemsCount():
      if item> -1:
        if self.activeItem!= item:
          self.activeItem= item
          self.setChanged( 1 )
        
        if item> self.getLastOnScreen():
          self.setItems( item- self.getItemsPerColumn()* self.getItemsPerRow()+ 1 )
        elif item< self.startFrom:
          self.setItems( item )
      else:
        self.activeItem= item
    else:
      self.activeItem= 0
    
  # -----------------------------------------------------------------
  def getItemsCount( self ):
    """
    """
    return len( self.itemsWrapper.items() )
      
  # -----------------------------------------------------------------
  def getEffectiveHeight( self ):
    """
    """
    if self.scrollFlag:
      return self.rect[ 3 ]- self.scrollFont[ 0 ].get_height()
    else:
      return self.rect[ 3 ]
    
  # -----------------------------------------------------------------
  def getEffectiveWidth( self ):
    """
    """
    return self.rect[ 2 ]
  
  # -----------------------------------------------------------------
  def getItemsPerRow( self ):
    """
      Calculate number of items per row
    """
    return int( self.rect[ 2 ] / self.getRenderer().getItemSize()[ 0 ] )
    
  # -----------------------------------------------------------------
  def getItemsPerColumn( self ):
    """
      Calculate number of items per column
    """
    return int( self.rect[ 3 ] / self.getRenderer().getItemSize()[ 1 ] )
  
  # -----------------------------------------------------------------
  def getItem( self, index ):
    """
      Return item by index or None
    """
    try:
      return self.itemsWrapper.items()[ index ]
    except:
      return None
  
  # -----------------------------------------------------------------
  def getLastOnScreen( self ):
    if self.startFrom!= -1:
      i= self.startFrom+ self.getItemsPerColumn()*  self.getItemsPerRow()- 1
      if i> self.getItemsCount():
        i= self.getItemsCount()- 1
    else:
      i= -1
      
    return i
  
  # -----------------------------------------------------------------
  def getActiveItem( self ):
    """
        getActiveItem() -> ( index )
        return active item as an index
    """
    return self.activeItem
    
  # -----------------------------------------------------------------
  def getItemByPos( self, pos ):
    # Get position in a row and column
    if pos[ 0 ]< 0 or pos[ 1 ]< 0:
      return None
    
    col= pos[ 0 ]/ int( self.getEffectiveWidth() / self.getItemsPerRow() )
    row= pos[ 1 ]/ int( self.getEffectiveHeight() / self.getItemsPerColumn() )
    if col>= self.getItemsPerRow() or row>= self.getItemsPerColumn():
      if pos[ 0 ]< self.getEffectiveWidth() and pos[ 1 ]< self.getEffectiveHeight():
        return -1
      else:
        return None
    
    # Find out the first item on a screen
    offset= row* self.getItemsPerRow()+ col
    if offset>= len( self.items ):
      return -1
    return self.getItem( offset )
  
  # -----------------------------------------------------------------
  def getRect( self ):
    """
      getRect() -> ( posX, posY, sizeX, sizeY )
      returns grid settings for the current view
    """
    return self.rect
  
  # -----------------------------------------------------------------
  def getPos( self ):
    """
      getPos() -> ( posX, posY )
    """
    return self.rect[ :2 ]
  
  # -----------------------------------------------------------------
  def showItem( self, i, pos ):
    item= self.getRenderer().drawItem( i, self.activeItem== i and self.isActive() )
    return self.adjustPos( item, pos )
  
  # -----------------------------------------------------------------
  def showLine( self, yPos, start ):
    """
      _showLine( yPos ) -> ( success )
      Shows line based either on top of current items or at the bottom.
      Returns whether it was successfull.
    """
    # Get drawer class
    renderClass= self.getRenderer()
    
    # Find out which item we should start from and how many of them would be displayed
    itemSize= renderClass.getItemSize()
    itemsPerLine= self.getItemsPerRow()
    
    # Start drawing items
    xPos= 0
    res= []
    for i in xrange( start, start+ itemsPerLine ):
      try:
        res+= self.showItem( i, ( xPos, yPos ) )
        if itemSize[ 0 ]== 0:
          break
      
        xPos+= itemSize[ 0 ]
      except:
        traceback.print_exc()
        break
    
    return i- start+ 1, res
  
  # -----------------------------------------------------------------
  def showStatusLine( self ):
    if self.isActive()== 0:
      return []
    
    text= self.scrollFont[ 0 ].render( '%d of %d' % ( self.activeItem+ 1, self.getItemsCount() ), 1, self.scrollFont[ 1 ] )
    totalWidth= self.navIcons[ 0 ].get_width()* 2+ text.get_width()+ 30
    yPos= self.rect[ 3 ]- text.get_height()- 5
    res= [ ( text, ( self.rect[ 2 ]- totalWidth, yPos )) ]
    totalWidth-= text.get_width()+ 10
    if self.getLastOnScreen()!= self.getItemsCount()- 1:
      # Draw down arrow
      res.append( ( self.navIcons[ 0 ], ( self.rect[ 2 ]- totalWidth, yPos )) )
    
    totalWidth-= self.navIcons[ 0 ].get_width()+ 10
    
    if self.startFrom!= 0:
      # Draw up arrow
      res.append( ( self.navIcons[ 1 ], ( self.rect[ 2 ]- totalWidth, yPos )) )
    
    return res
  
  # -----------------------------------------------------------------
  def hasChanged( self ):
    if self.getRenderer():
      return self.getRenderer().hasChanged()
    
  # -----------------------------------------------------------------
  def setChanged( self, changed ):
    if self.getRenderer():
      self.getRenderer().setChanged( changed )
    
  # -----------------------------------------------------------------
  def processKey( self, key ):
    # Pass the key to the helper
    curr= self.getActiveItem()
    modifier= 1
    if key== pygame.K_PAGEDOWN:
      modifier= self.getItemsPerColumn()* self.getItemsPerRow()- 1
      key= pygame.K_DOWN
    elif key== pygame.K_PAGEUP:
      modifier= self.getItemsPerColumn()* self.getItemsPerRow()- 1
      key= pygame.K_UP
    
    if key== pygame.K_UP:
      while curr> 0 and modifier> 0:
        self.moveTo( curr- 1 )
        curr-= 1
        modifier-= 1
    elif key== pygame.K_DOWN:
      while curr< self.getItemsCount()- 1 and modifier> 0:
        self.moveTo( curr+ 1 )
        curr+= 1
        modifier-= 1
    elif key== pygame.K_HOME:
      self.moveTo( 0 )
    elif key== pygame.K_END:
      self.moveTo( self.getItemsCount()- 1 )
    else:
      activeItem= self.getRenderer().processKey( self.getActiveItem(), key )
      if activeItem!= None and self.itemsWrapper.hasChanged():
        self.init( self.itemsWrapper, activeItem )
  
  # -----------------------------------------------------------------
  def renderMessage( self, message ):
    text= self.messageFont[ 0 ].render( message, 1, self.messageFont[ 1 ] )
    # Center out the message
    x= ( self.rect[ 2 ]- text.get_width() )/ 2
    y= ( self.rect[ 3 ]- text.get_height() )/ 2
    return [ ( text, ( x,y ) ) ]
  
  # -----------------------------------------------------------------
  def render( self ):
    # Start showing the menu if there was a change
    if self.visible== 0:
      return []
    
    itemSize= self.getRenderer().getItemSize()
    
    # Render only message when list is empty
    if self.getItemsCount()== 0:
      self.activeItem= -1
      return self.adjustPos( self.renderMessage( 'Empty' ), self.rect[ :2 ] )
    
    # Show items on a screen
    yPos= 0
    i= self.startFrom
    res= []
    while i< self.getItemsCount() and yPos+ itemSize[ 1 ]< self.getEffectiveHeight():
      j, resTmp= self.showLine( yPos, i )
      i+= j
      res+= resTmp
      yPos+= itemSize[ 1 ]
    
    # If active item still not on a screen, just set the item
    if ( self.activeItem< self.startFrom or self.activeItem> self.getLastOnScreen() ) and self.activeItem!= -1:
      self.setItems( self.activeItem )
    
    # Show status at the bottom if needed
    if self.scrollFlag:
      res+= self.showStatusLine()
    
    self.setChanged( 0 )
    return self.adjustPos( res, self.rect[ :2 ] )

# ***********************************************************************
# Run some tests against basic list functionality
if __name__== '__main__':
  class MenuWrapper:
    def __init__( self, items ):
      self.menuItems= items
    def items( self ):
      return self.menuItems
    
  import pygame
  resolution = 640,480
  pygame.init()
  screen = pygame.display.set_mode(resolution)
  rect= ( 210,120,210,200 )
  params= { 'font': 'Arial.ttf',
            'fontSize': '20',
            'fontColor': '89,150,255',
            'scroll': 'no' }
  menuDef= (
    { 'caption': 'Music',
      'onClick': '' },
    { 'caption': 'GPS',
      'onClick': '' },
    { 'caption': 'Settings',
      'onClick': '' },
    { 'caption': 'Exit',
      'onClick': 'EXIT_FLAG=1' } )
    
  mItem= MenuItem( rect, params )
  listDisplay= ListDisplay( rect, params, mItem )
  listDisplay.init( MenuWrapper( menuDef ), 0 )
  
  EXIT_FLAG= 0
  listDisplay.setVisible( 1 )
  screen.blit( listDisplay.render(), rect[ :2 ] )
  pygame.display.flip()
  while EXIT_FLAG== 0:
    #use event.wait to keep from polling 100% cpu
    event = pygame.event.wait()
    if event.type is pygame.KEYDOWN:
      if event.key == pygame.K_ESCAPE:
        EXIT_FLAG= 1
