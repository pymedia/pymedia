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

DEFAULT_FONT= 'Default'
DEFAULT_FONT_NAME= 'arialn'
DEFAULT_FONT_SIZE= '20'
DEFAULT_FONT_COLOR= '255, 255, 255'
STRIPE_DEFAULT_COLOR= ( 95,166,76)
DEFAULT_ALPHA= 255
DEFAULT_FONT_ALPHA= '255'
FONT_DIR= 'icons/'
LEFT_C= (0,0)
EMPTY_MESSAGE= 'This folder is empty'
NAV_ICONS= ( 'down_0.gif', 'up_0.gif' )
CENTER=0
LEFT=1
RIGHT=2

# -----------------------------------------------------------------
def drawText( font, text ):
  text1= font[ 0 ].render( text, 1, font[ 1 ] )
  if font[ 3 ]:
    text= font[ 0 ].render( text, 1, font[ 3 ] )
    text.blit( text1, font[ 4 ] )
    text1= text
  
  if font[ 2 ]!= 255:
    text1.set_alpha( font[ 2 ] )
  return text1

# -----------------------------------------------------------------
def alignSurface( parent, surf, x, y ):
  if x== CENTER:
    newX= ( parent[ 0 ]- surf[ 0 ] )/ 2
  if x== LEFT:
    newX= 0
  if x== RIGHT:
    newX= parent[ 0 ]- surf[ 0 ]
  if y== CENTER:
    newY= ( parent[ 1 ]- surf[ 1 ] )/ 2
  if y== LEFT:
    newY= 0
  if y== RIGHT:
    newY= parent[ 1 ]- surf[ 1 ]
  return ( newX, newY )

# -----------------------------------------------------------------
# Freevo rulez. We can't do any better, but we'll try
def drawCircle(s, color, pos, radius):
    """
    draws a circle to the surface s and fixes the borders
    pygame.draw.circle has a bug: there are some pixels where
    they don't belong. This function stores the values and
    restores them
    """
    def getAt( pos ):
      try: return s.get_at( pos )
      except: return None
    
    def setAt( pos, color ):
      if color:
        s.set_at( pos, color )
    
    x, y= pos
    p1 = [ getAt( ( z, y-radius-1 )) for z in xrange( x- 1, x+ 2 ) ]
    p2 = [ getAt( ( z, y+radius )) for z in xrange( x- 1, x+ 2 ) ]
    pygame.draw.circle(s, color, pos, radius)
    [ setAt( ( z, y-radius-1 ), p1[ z- x+ 1 ]) for z in xrange( x- 1, x+ 2 ) ]
    [ setAt( ( z, y+radius ), p2[ z- x+ 1 ]) for z in xrange( x- 1, x+ 2 ) ]

# -----------------------------------------------------------------
def drawRoundedBox( size, color, alpha, borderSize= 0, borderColor= None, radius= 0 ):
  surf= pygame.Surface( size, pygame.SRCALPHA )
  surf.fill( (0,0,0) )
  surf.set_colorkey( (0,0,0) )
  
  x,y= 0,0
  w,h= size
  if borderColor== None: borderColor= color
  while 1:
    if radius >= 1:
      drawCircle( surf, borderColor, ( x+ radius, y+ radius ), radius)
      drawCircle( surf, borderColor, ( x+ w- radius, y+ radius ), radius)
      drawCircle( surf, borderColor, ( x+ radius, y+ h-radius ), radius)
      drawCircle( surf, borderColor, ( x+ w-radius, y+ h-radius ), radius)
      pygame.draw.rect(surf, borderColor, (x+ radius, y, w-2*radius, h))
    pygame.draw.rect(surf, borderColor, (x, y+radius, w, h-2*radius))
    
    x += borderSize
    y += borderSize
    h -= 2* borderSize
    w -= 2* borderSize
    radius -= borderSize
    if borderSize== 0:
      break
    borderSize= 0
    borderColor= color
  
  if alpha!= 255:
    surf.set_alpha( alpha )
  
  return surf

# -----------------------------------------------------------------
def clipIcon( icons, size ):
  """
    Clip all icons in a list to a size passed
  """
  res= []
  for icon in icons:
    size1= [ size[ 0 ]- icon[ 1 ][ 0 ], size[ 1 ]- icon[ 1 ][ 1 ] ]
    if size1[ 0 ]> icon[ 0 ].get_width():
      size1[ 0 ]= icon[ 0 ].get_width()
    if size1[ 1 ]> icon[ 0 ].get_height():
      size1[ 1 ]= icon[ 0 ].get_height()
    res+= [ ( icon[ 0 ].subsurface( LEFT_C+ tuple(size1) ), icon[ 1 ] ) ]
  return res


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
    self.font= self.loadFont( 'font' )
    self.alpha= self.getParam( 'alpha', DEFAULT_ALPHA )
  
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
    font= fontDefs[ self.getParam( paramName, DEFAULT_FONT ) ]
    fontName= font.getParam( 'name', DEFAULT_FONT_NAME )
    size= font.getParam( 'size', DEFAULT_FONT_SIZE )
    alpha= int( font.getParam( 'alpha', DEFAULT_FONT_ALPHA ) )
    color= eval( font.getParam( 'color', DEFAULT_FONT_COLOR ) )
    shadow= eval( font.getParam( 'shadow', 'None' ) )
    shadowOffset= eval( font.getParam( 'offset', '-1,-1' ) )
    fontObj= pygame.font.Font( FONT_DIR+ fontName+ '.ttf', int( size ) )
    if font.getParam( 'bold', 'no' )== 'yes':
      fontObj.set_bold( 1 )
    
    return fontObj, color, alpha, shadow, shadowOffset
  
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
    self.itemSize= ( rect[ 2 ], self.font[ 0 ].get_height() )
    self.itemsWrapper= None
    
    # Scale default stripe
    self.stripeIcon= drawRoundedBox(
      size= self.itemSize,
      color= eval( self.getParam( 'stripeColor', STRIPE_DEFAULT_COLOR )), 
      alpha= 200,
      radius= 8,
      borderColor= (87,110,97),
      borderSize= 2)
  
  # -----------------------------------------------------------------
  def drawItem( self, itemPos, isFocused ):
    item= self.itemsWrapper.items()[ itemPos ]
    # Find out whether it is a directory or file
    text= drawText( self.font, item[ 'caption' ] )

    # Center text into rect
    if isFocused== 1:
      resItem= [( self.stripeIcon, LEFT_C )]
    else: resItem= []
    
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
    return \
      MenuHelper.hasChanged( self ) or \
      ( self.getRenderer() and self.getRenderer().hasChanged() )

# ****************************************************************************************
class BgTextDisplay( GenericDisplay ):
  """
    Pure text based displayer without any renderer. Displays the current state of menu.
  """
  # -----------------------------------------------------------------
  def __init__( self, rect, params, renderClass ):
    GenericDisplay.__init__( self, rect, params, renderClass )
    self.currPos= -1
    self.textIcon= None
  
  # -----------------------------------------------------------------
  def setText( self, text ):
    if len( text )> 0:
      self.textIcon= drawText( self.font, text )
      self.textIcon.set_alpha( self.alpha )
    
    self.setChanged( 1 )
  
  # -----------------------------------------------------------------
  def render( self ):
    """
      Main method called by the main renderer
    """
    self.setChanged( 0 )
    if self.textIcon:
      return [ ( self.textIcon, self.getPos() ) ]
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
    self.headerIcon= None
    self.navIcons= self.loadIcon( 'navIcons', NAV_ICONS )
    self.headerFont, self.scrollFont, self.messageFont= \
      map( lambda x: self.loadFont( x ), ( 'headerFont','scrollFont','messageFont' ))
  
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
  def setHeader( self, title ):
    i= self.getEffectiveWidth()
    size= self.headerFont[ 0 ].size( title )
    while size[ 0 ]> i:
      title= title[ 1: ]
      size= self.headerFont[ 0 ].size( title )
    
    hIcon= drawText( self.headerFont, title )
    j= hIcon.get_width()
    self.headerIcon= (( hIcon, ( ( i- j )/2 , 0 ) ),)
  
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
    ICON_OFFSET= 10
    if self.isActive()== 0:
      return []
    
    iWidth= self.navIcons[ 0 ].get_width()
    text= drawText( self.scrollFont, '%d of %d' % ( self.activeItem+ 1, self.getItemsCount() ) )
    yPos= self.rect[ 3 ]- text.get_height()
    res= [ ( text, ( self.rect[ 2 ]- ICON_OFFSET* 3- iWidth* 2- text.get_width(), yPos )) ]
    if self.startFrom!= 0:
      # Draw up arrow
      res.append( ( self.navIcons[ 0 ], ( self.rect[ 2 ]- ICON_OFFSET* 2- iWidth* 2, yPos )) )
    if self.getLastOnScreen()!= self.getItemsCount()- 1:
      # Draw down arrow
      res.append( ( self.navIcons[ 1 ], ( self.rect[ 2 ]- ICON_OFFSET- iWidth, yPos )) )
    
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
  def renderCustomMessage( self, message ):
    # For most cases it would be predefined.
    # If message should be custom, just override this function
    mess= drawText( self.messageFont, message )
    return [
      ( mess,
        alignSurface(
          parent= self.rect[ 2: ], surf= mess.get_size(), x= CENTER, y= CENTER )) ]
  
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
      res+= self.renderCustomMessage( EMPTY_MESSAGE )
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
  print 'This module cannot run idividually. It should be used in pymedia library only.'
