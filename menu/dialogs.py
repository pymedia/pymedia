##    dialogs - Part of the pymedia package. Dialog drawing module.
##        Allows to define custom modal dialogs such as keyboard and
##        confirmation pages
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
import pygame
from __main__ import *
from generic import *

BORDER_OFFS= 6
Y_DELTA= 4
X_DELTA= 4
LEFT_C= (0,0)

# ****************************************************************************************
# Define generic choice dialog functions
class GenericChoiceDialog( GenericDisplay ):
  # -------------------------------------------------------------
  def __init__( self, rect, attributes, params, renderClass ):
    GenericDisplay.__init__( self, rect, attributes, params, renderClass )
    self.choiceFont= self.loadFont( 'choiceFont' )
    self.bgColor= self.getParamList( 'bgColor', ( 0,0,0 ) )
    self.buttonColors= ( self.getParamList( 'butColor0', ( 39,64,192 ) ),
                         self.getParamList( 'butColor1', ( 39,192,40 ) ))
    self.outlineColor= self.getParamList( 'outlineColor', ( 255,255,255 ) )
    self.choicesPerLine= self.rect[ 2 ]/ ( int( self.getParam( 'buttonWidth', 70 ))+ 5 )
    self.setChoices( self.getParam( 'choices', ( '<undefined>', ) ) )
    self.surface= self.renderFrame()
    self.buttonIcons= self.returnedArea= self.textIcons= self.captionIcons= None
    self.activeChoice= 0
    self.setChanged( 1 )
  
  # -------------------------------------------------------------
  def init( self, caption, text, returnArea, params ):
    self.setText( text )
    self.setCaption( caption )
    self.setReturnArea( returnArea )
    self.activeChoice= 0
    self.returnParams= params
  
  # -------------------------------------------------------------
  def drawButton( self, size, selected ):
    return drawRoundedBox( size, self.buttonColors[ selected ], 255, 1, self.outlineColor, 6 )
  
  # -------------------------------------------------------------
  def setReturnArea( self, area ):
    self.returnedArea= area
  
  # -------------------------------------------------------------
  def getParams( self ):
    return self.returnParams
      
  # -------------------------------------------------------------
  def getChoice( self ):
    if self.activeChoice== -1:
      return None
    
    return self.choices[ self.activeChoice ]
    
  # -------------------------------------------------------------
  def renderFrame( self ):
    return \
      ( drawRoundedBox( self.getSize(), self.bgColor, self.alpha, 1, self.outlineColor, 6 ),
      LEFT_C )
  
  # -------------------------------------------------------------
  def setChoices( self, choices ):
    self.choiceTexts= choices
    self.choices= map( lambda x:
      { 'caption': x,
        'width': int( self.getParam( 'buttonWidth', 70 )),
        'icon': drawText( self.choiceFont, x )
      }, self.choiceTexts )
    
    self.buttonIcons= self.renderButtons()
  
  # -------------------------------------------------------------
  def renderButtonsLine( self, choices, yPos ):
    width= reduce( lambda x,y: x+ y[ 'width' ], choices, 0 )
    # Align choices to the surface width
    height= self.choiceFont[ 0 ].get_linesize()
    xPos= X_DELTA
    res= []
    for choice in choices:
      if not choice.has_key( 'hidden' ) or choice[ 'hidden' ]!= 'yes':
        res.append( ( self.drawButton( ( choice[ 'width' ], height ), 0 ), ( xPos, yPos ) ) )
        # Align text into the button
        choice[ 'pos' ]= ( 
          xPos+ ( choice[ 'width' ]- choice[ 'icon' ].get_width() )/ 2, 
          yPos+ height- choice[ 'icon' ].get_height() 
        )
        choice[ 'buttonPos' ]= ( xPos, yPos )
      xPos+= X_DELTA+ choice[ 'width' ]
    
    return res
  
  # -------------------------------------------------------------
  def renderButtons( self ):
    yPos= i= 0
    iLines= ( len( self.choices )+ self.choicesPerLine- 1 )/ self.choicesPerLine
    res= []
    while i< len( self.choices ):
      yPos+= Y_DELTA
      res+= self.renderButtonsLine( self.choices[ i: i+ self.choicesPerLine ], yPos )
      yPos+= self.choiceFont[ 0 ].get_linesize()
      i+= self.choicesPerLine
    
    return res
  
  # -------------------------------------------------------------
  def setCaption( self, caption ):
    if caption:
      # Set caption with bgColor in it...
      c= drawText( self.font, caption )
      self.captionIcons= [ ( c, ( ( self.getWidth()- c.get_width() )/ 2, 0 )) ]
    else:
      self.captionIcons= None
  
  # -------------------------------------------------------------
  def setText( self, text ):
    if text:
      c= drawText( self.font, text )
      self.textIcons= [ ( c, ( ( self.getWidth()- c.get_width() )/ 2, 0 )) ]
    else:
      self.textIcons= None
    
    self.text= text
  
  # -------------------------------------------------------------
  def getText( self ):
    return self.text
  
  # -------------------------------------------------------------
  def exitDialog( self, item ):
    if item== None:
      self.activeChoice= -1
    
    areas[ self.returnedArea ].execute( 'onDialogReturn', None, self )
  
  # -------------------------------------------------------------
  def isHidden( self, activeChoice ):
    return \
      self.choices[ activeChoice ].has_key( 'hidden' ) and \
      self.choices[ activeChoice ][ 'hidden' ]== 'yes'
    
  # -------------------------------------------------------------
  def renderSelectedButton( self ):
    if self.activeChoice== -1:
      return None
    
    choice= self.choices[ self.activeChoice ]
    height= self.choiceFont[ 0 ].get_linesize()
    button= self.drawButton( ( choice[ 'width' ], height ), 1 )
    return ( button, choice[ 'buttonPos' ] )
  
  # -------------------------------------------------------------
  def render( self ):
    res= [ self.surface ]
    yPos= 0
    if self.captionIcons:
      yPos= self.getMaxPos( self.captionIcons )[ 1 ]
      res+= self.captionIcons
    if self.textIcons:
      res+= self.adjustPos( self.textIcons, (0,yPos) )
      yPos+= self.getMaxPos( self.textIcons )[ 1 ]
    # Draw choices as buttons
    if self.buttonIcons:
      maxX, maxY= self.getMaxPos( self.buttonIcons )
      x, y= alignSurface( ( self.getWidth(), self.getHeight()- yPos ), ( maxX, maxY ), CENTER, CENTER )
      r= [ ( z[ 'icon' ], z[ 'pos' ] ) for z in filter( lambda w: w.has_key( 'icon' ), self.choices ) ]
      res+= self.adjustPos( self.buttonIcons+ [ self.renderSelectedButton() ]+ r, (x,y+ yPos) )
      
    self.setChanged( 0 )
    return self.adjustPos( res, self.rect[ :2 ] )

# ****************************************************************************************
# Dialog area as the simple dialog 
class SimpleDialog( GenericChoiceDialog ):
  # -------------------------------------------------------------
  def processKey( self, key ):
    if key== pygame.K_LEFT:
      if self.activeChoice> 0:
        self.activeChoice-= 1
        self.setChanged( 1 )
    
    if key in ( pygame.K_RIGHT, pygame.K_TAB ):
      if self.activeChoice< len( self.choices )- 1:
        self.activeChoice+= 1
        self.setChanged( 1 )
    
    if key== pygame.K_ESCAPE:
      self.exitDialog( None )
    
    if key== pygame.K_RETURN:
      self.exitDialog( 0 )

# ****************************************************************************************
# Text enter dialog area as a modal dialog 
class TextEnterDialog( GenericChoiceDialog ):
  # -------------------------------------------------------------
  def __init__( self, rect, attributes, params, renderClass ):
    GenericChoiceDialog.__init__( self, rect, attributes, params, renderClass )
    # Generate choices
    self.cursorColor= self.getParamList( 'color', ( 0,0,0 ) )
    # Add additional choices for Ok and Cancel
    delta= self.choicesPerLine- ( len( self.choiceTexts ) % self.choicesPerLine )
    # Align choices to the left
    if delta< self.choicesPerLine:
      self.choices+= list( map( lambda x: { 'width': 0, 'hidden': 'yes' }, range( delta ) ) )
    # Add additional choices for the control
    ctrlChoices= ( 'Ok', 'Clear', 'Cancel' )
    self.choices+= map( lambda x: { 'caption': x, 'width': 100, 'icon': drawText( self.choiceFont, x ) }, ctrlChoices )
    self.buttonIcons= self.renderButtons()
    self.allowedSyms= self.getParam( 'allowedSyms', ( pygame.K_SPACE, pygame.K_z+ 1 ) )
    self.hasCursor= 0
  
  # -------------------------------------------------------------
  def setText( self, text ):
    if text:
      c= drawText( self.font, text )
      x= c.get_width()
      self.textIcons= self.drawEntryBox()+ [ ( c, ( BORDER_OFFS, 0 )) ]
    else:
      self.textIcons= self.drawEntryBox()
      x= 0
    
    if self.hasCursor:
      c= drawRoundedBox(
        ( 10, self.font[ 0 ].get_linesize()- BORDER_OFFS* 2 ),
        self.font[ 1 ] )
      self.textIcons.append( ( c, ( x+ BORDER_OFFS, BORDER_OFFS ) ))
      
    self.text= text
  
  # -------------------------------------------------------------
  def drawEntryBox( self ):
    i= BORDER_OFFS
    offs= self.font[ 0 ].get_linesize()
    # Draw black rectangle for entry space
    s= drawRoundedBox( ( self.rect[ 2 ]- i* 2, offs ), (0,0,1), 255, 2, self.cursorColor, 0 )
    return [ ( s, ( i, 2 ) ) ]
  
  # -------------------------------------------------------------
  def processKey( self, key ):
    if key== pygame.K_LEFT:
      if self.activeChoice> 0 and self.isHidden( self.activeChoice- 1 )== 0:
        self.activeChoice-= 1
        self.setChanged( 1 )
    
    if key in ( pygame.K_RIGHT, pygame.K_TAB ):
      if self.activeChoice< len( self.choices )- 1 and self.isHidden( self.activeChoice+ 1 )== 0:
        self.activeChoice+= 1
        self.setChanged( 1 )
    
    if key in range( self.allowedSyms[ 0 ], self.allowedSyms[ 1 ] ) and self.activeChoice== -1:
      self.setText( self.text+ chr( key ) )
      self.setChanged( 1 )
    
    if key== pygame.K_BACKSPACE and self.activeChoice== -1:
      self.setText( self.text[ : len( self.text )- 1 ] )
      self.setChanged( 1 )
      
    if key== pygame.K_DOWN:
      # See if we may move down
      if self.activeChoice== -1:
        self.hasCursor= 0
        self.setText( self.text )
        i= 0
      else:
        i= self.activeChoice+ self.choicesPerLine
      if i< len( self.choices )- 1 and self.isHidden( i )== 0:
        self.activeChoice= i
        self.setChanged( 1 )
    
    if key== pygame.K_UP:
      # See if we may move up
      i= self.activeChoice- self.choicesPerLine
      if i< 0:
        self.hasCursor= 1
        self.activeChoice= -1
        self.setText( self.text )
      
      if i>= 0 and self.isHidden( i )== 0:
        self.activeChoice= i
        
      self.setChanged( 1 )
    
    if key in ( pygame.K_RETURN, pygame.K_SPACE ):
      # See if this is the control buttons
      if self.activeChoice!= -1:
        choice= self.choices[ self.activeChoice ]
        if choice[ 'caption' ]== 'Clear':
          self.setText( '' )
          self.setChanged( 1 )
        elif choice[ 'caption' ]== 'Cancel':
          key= pygame.K_ESCAPE
        elif choice[ 'caption' ]== 'Ok':
          self.exitDialog( 0 )
        else:
          self.setText( self.text+ choice[ 'caption' ] )
          self.setChanged( 1 )
      elif key== pygame.K_RETURN:
        self.activeChoice= 1
        self.exitDialog( 0 )
    
    if key== pygame.K_ESCAPE:
      self.exitDialog( None )
    
