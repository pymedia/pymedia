##    generic - Part of the pymedia package. Menu drawing utilities.
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

DEFAULT_FONT= 'arialn'
DEFAULT_FONT_SIZE= 20
DEFAULT_FONT_COLOR= ( 255, 255, 255 )
LEFT_C= (0,0)
EMPTY= 0
NAV_ICONS= ( 'down_0.gif', 'up_0.gif' )

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
    #font= pygame.font.SysFont( fontName, int( fontSize ) )
    font= pygame.font.Font( 'icons/'+ fontName+ '.ttf', int( fontSize ) )
    if self.getParam( paramName+ 'Bold', 'no' )== 'yes':
      font.set_bold( 1 )
    
    fontColor= self.getParamList( paramName+ 'Color', DEFAULT_FONT_COLOR )
    
    return font, fontColor
  
  # -----------------------------------------------------------------
  def loadIcon( self, paramName, default, alpha= -1 ):
    icons= self.getParam( paramName, default )
    icons1= None
    
    if type( icons )== str:
      icons1= pygame.image.load( 'icons/'+ icons )
      if alpha!= -1:
        icons1.set_alpha( alpha )
    elif type( icons )== list or type( icons )== tuple:
      icons1= map( lambda x: pygame.image.load( 'icons/'+ x ), icons )
      if alpha!= -1:
        map( lambda x: x.set_alpha( alpha ), icons1 )
    
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
    self.stripeIcon= pygame.Surface( self.itemSize )
    col= self.getParam( 'stripeColor', ( 95,166,76) )
    self.stripeIcon.fill( col )
    self.stripeIcon.set_alpha( 190 )
  
  # -----------------------------------------------------------------
  def drawItem( self, itemPos, isFocused ):
    item= self.itemsWrapper.items()[ itemPos ]
    # Find out whether it is a directory or file
    text1= self.font[ 0 ].render( item[ 'caption' ], 1 , self.font[ 1 ] )
    text= self.font[ 0 ].render( item[ 'caption' ], 1 , (0,0,0) )
    text.blit( text1, (-1,-1))
    
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

# ****************************************************************************************
class BgTextDisplay( GenericDisplay ):
  """
    Pure text based displayer without any renderer. Displays the current state of menu.
  """
  # -----------------------------------------------------------------
  def __init__( self, rect, params, renderClass ):
    GenericDisplay.__init__( self, rect, params, renderClass )
    self.font= self.loadFont( 'font' )
    self.alpha= self.getParam( 'alpha', 10 )
    self.currPos= -1
    self.textIcon= None
  
  # -----------------------------------------------------------------
  def setText( self, text ):
    if len( text )> 0:
      self.textIcon= self.font[ 0 ].render( text, 1, self.font[ 1 ] )
      #self.textIcon= self.font[ 0 ].render( text, 1, (0,0,0) )
      #self.textIcon.blit( textIcon1, ( -2,-2 ))
      self.textIcon.set_alpha( 100 )
    
    self.setChanged( 1 )
  
  # -----------------------------------------------------------------
  def render( self ):
    """
      Main method called by the main renderer
    """
    self.setChanged( 0 )
    if self.textIcon:
      return [ ( self.textIcon, self.rect[ :2 ] ) ]
    else:
      return []
  
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
    self.headerFont= self.loadFont( 'headerFont' )
    self.headerIcon= None
    self.scrollFont= self.loadFont( 'scrollFont' )
    self.messageFont= self.loadFont( 'messageFont' )
    self.navIcons= self.loadIcon( 'navIcons', NAV_ICONS )
  
  # -----------------------------------------------------------------
  def init( self, itemsWrapper, activeItem= 0 ):
    """
      Initiate process of displaying the data
    """
    itemsWrapper.init()
    self.renderClass.setItemsWrapper( itemsWrapper )
    self.itemsWrapper= itemsWrapper
    self.lastActiveItem= -1
    self.setItems( self.activeItem )
    self.moveTo( activeItem )
  
  # -----------------------------------------------------------------
  def getWrapper( self ):
    """
      Return wrapper for the items in a list.
    """
    return self.itemsWrapper
  
  # -----------------------------------------------------------------
  def setItems( self, itemFrom ):
    """
      Set starting item from which all items will be shown in an active area.
    """
    if itemFrom< 0:
      itemFrom= 0
    
    self.startFrom= itemFrom
  
  # -----------------------------------------------------------------
  def moveTo( self, item ):
    """
      Move the active item to the selected index. If it does not shown on a screen,
      scroll and show.
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
      Return number of items in list control. Takes itemWrapper items count.
    """
    return len( self.itemsWrapper.items() )
      
  # -----------------------------------------------------------------
  def getEffectiveHeight( self ):
    """
      Return effective window height excluding header and footer areas
    """
    i= 0
    if self.scrollFlag:
      i+= self.scrollFont[ 0 ].get_linesize()
      
    if self.headerIcon:
      i+= self.headerFont[ 0 ].get_linesize()
    
    return self.rect[ 3 ]- i
    
  # -----------------------------------------------------------------
  def getEffectiveWidth( self ):
    """
      Return effective width excluding any additional side information the list may have.
    """
    return self.rect[ 2 ]
  
  # -----------------------------------------------------------------
  def getItemsPerRow( self ):
    """
      Calculate number of items per row
    """
    return int( self.getEffectiveWidth() / self.getRenderer().getItemSize()[ 0 ] )
    
  # -----------------------------------------------------------------
  def getItemsPerColumn( self ):
    """
      Calculate number of items per column
    """
    return int( self.getEffectiveHeight() / self.getRenderer().getItemSize()[ 1 ] )
  
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
    """
      Return index for the last item visible on a screen
    """
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
      return active item as an index
    """
    return self.activeItem
    
  # -----------------------------------------------------------------
  def getItemByPos( self, pos ):
    """
      Return item's index by its position in an active area
    """
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
      
      Returns position in a parent area
    """
    return self.rect[ :2 ]
  
  # -----------------------------------------------------------------
  def setHeader( self, title ):
    i= self.getEffectiveWidth()
    size= self.headerFont[ 0 ].size( title )
    while size[ 0 ]> i:
      title= title[ 1: ]
      size= self.headerFont[ 0 ].size( title )
    
    self.headerIcon= self.headerFont[ 0 ].render( title, 1, self.headerFont[ 1 ] )
    j= self.headerIcon.get_width()
    self.headerIcon= (( self.headerIcon, ( ( i- j )/2 , 0 ) ),)
  
  # -----------------------------------------------------------------
  def renderItem( self, i, pos ):
    item= self.getRenderer().drawItem( i, self.activeItem== i and self.isActive() )
    return self.adjustPos( item, pos )
  
  # -----------------------------------------------------------------
  def renderLine( self, yPos, start ):
    """
      showLine( yPos, startIndex ) -> ( currentIndex, items )
      
      Shows line based either on top of current items or at the bottom.
      Returns whether it was successfull.
    """
    # Get drawer class
    renderClass= self.getRenderer()
    
    # Find out which item we should start from and how many of them would be displayed
    itemSize= renderClass.getItemSize()
    itemsPerRow= self.getItemsPerRow()
    
    # Start drawing items
    xPos= 0
    res= []
    for i in xrange( start, start+ itemsPerRow ):
      try:
        res+= self.renderItem( i, ( xPos, yPos ) )
        if itemSize[ 0 ]== 0:
          break
      
        xPos+= itemSize[ 0 ]
      except:
        traceback.print_exc()
        break
    
    return i- start+ 1, res
  
  # -----------------------------------------------------------------
  def renderHeader( self ):
    """
      renderHeader() -> yOffs, header
      
      Returns header if any.
    """
    if self.headerIcon:
      return self.headerIcon[ 0 ][ 0 ].get_height(), list( self.headerIcon )
    else:
      return 0, []
  
  # -----------------------------------------------------------------
  def renderStatusLine( self ):
    """
      showStatusLine() -> statusLineItems
      
      Returns status items that been rendered if any.
    """
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
  def renderCustomMessage( self, messageCode ):
    # For most cases it would be predefined.
    # If message should be custom, just override this function
    if messageCode== EMPTY:
      return self.renderMessage( 'Empty' )
    
  # -----------------------------------------------------------------
  def render( self ):
    # Start showing the menu if there was a change
    if self.visible== 0:
      return []
    
    itemSize= self.getRenderer().getItemSize()
    
    # Render only message when list is empty
    yPos, res= self.renderHeader()
    if self.getItemsCount()== 0:
      self.activeItem= -1
      res+= self.renderCustomMessage( EMPTY )
    else:
      # Show items on a screen
      iInitial= yPos
      i= self.startFrom
      while i< self.getItemsCount() and yPos- iInitial+ itemSize[ 1 ]<= self.getEffectiveHeight():
        j, resTmp= self.renderLine( yPos, i )
        i+= j
        res+= resTmp
        yPos+= itemSize[ 1 ]
      
      # If active item still not on a screen, just set the item
      if ( self.activeItem< self.startFrom or self.activeItem> self.getLastOnScreen() ) and self.activeItem!= -1:
        self.setItems( self.activeItem )
      
      # Show status at the bottom if needed
      if self.scrollFlag:
        res+= self.renderStatusLine()
    
    self.setChanged( 0 )
    return self.adjustPos( res, self.rect[ :2 ] )

# ***********************************************************************
# Run some tests against basic list functionality
if __name__== '__main__':
  print 'This module cannot run. It should be used in pymedia library only.'
