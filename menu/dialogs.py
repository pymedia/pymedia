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

# ****************************************************************************************
# Define generic choice dialog functions
class GenericChoiceDialog( GenericDisplay ):
  # -------------------------------------------------------------
  def __init__( self, rect, params, renderClass ):
    GenericDisplay.__init__( self, rect, params, renderClass )
    self.choiceFont= self.loadFont( 'choiceFont' )
    self.outlineChoice= 1
    self.font= self.loadFont( 'font' )
    self.color= self.getParamList( 'color', ( 255, 255, 255 ) )
    self.bgColor= self.getParamList( 'bgColor', ( 0,0,0 ) )
    self.buttonColors= ( self.getParamList( 'butColor0', ( 39,64,192 ) ),
                         self.getParamList( 'butColor1', ( 39,192,40 ) ))
    self.outlineColor= self.getParamList( 'outlineColor', ( 255,255,255 ) )
    self.returnedArea= self.textIcon= self.captionIcon= None
    self.setChoices( self.getParam( 'choices', ( '<undefined>', ) ) )
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
  def drawButton( self, size, color ):
    res= pygame.Surface( size )
    res.fill( color )
    return res
  
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
    res= pygame.Surface( self.rect[ 2: ] )
    res.fill( self.bgColor )
    pygame.draw.rect( res, self.color, ( BORDER_OFFS, BORDER_OFFS, res.get_width()- BORDER_OFFS*2, res.get_height()- BORDER_OFFS* 2 ), 1 )
    self.surface= [ ( res, (0,0) ) ]
  
  # -------------------------------------------------------------
  def setChoices( self, choices ):
    self.choiceTexts= choices
    self.choices= map( lambda x:
      { 'caption': x,
        'width': int( self.getParam( 'width', 70 )),
        'icon': self.choiceFont[ 0 ].render( x, 1, self.choiceFont[ 1 ] )
      }, self.choiceTexts )
    
    # Draw choice buttons
    self.renderFrame()
  
  # -------------------------------------------------------------
  def renderChoicesLine( self, choices, yPos ):
    width= reduce( lambda x,y: x+ y[ 'width' ], choices, 0 )
    # Align choices to the surface width
    xDelta= ( self.rect[ 2 ]- width )/ ( len( choices )+ 1 )
    height= self.choiceFont[ 0 ].get_linesize()
    xPos= xDelta
    for choice in choices:
      button= self.drawButton( ( choice[ 'width' ], height ), self.buttonColors[ 0 ] )
      self.surface[ 0 ][ 0 ].blit( button, ( xPos, yPos ) )
      # Align text into the button
      choice[ 'pos' ]= (
        xPos+ ( choice[ 'width' ]- choice[ 'icon' ].get_width() )/ 2,
        yPos+ height- choice[ 'icon' ].get_height()
      )
      choice[ 'buttonPos' ]= ( xPos, yPos )
      xPos+= xDelta+ button.get_width()
    
    return yPos+ height
  
  # -------------------------------------------------------------
  def renderChoices( self, yPos ):
    i= 0
    iLines= ( len( self.choices )+ self.choicesPerLine- 1 )/ self.choicesPerLine
    yDelta= ( self.rect[ 3 ]- yPos- self.choiceFont[ 0 ].get_linesize()* iLines )/ ( iLines+ 1 )
    while i< len( self.choices ):
      yPos+= yDelta
      lineIcon= self.renderChoicesLine( self.choices[ i: i+ self.choicesPerLine ], yPos )
      i+= self.choicesPerLine
  
  # -------------------------------------------------------------
  def setCaption( self, caption ):
    if caption:
      # Set caption with bgColor in it...
      self.captionIcon= self.font[ 0 ].render( caption, 1, self.font[ 1 ], self.bgColor )
  
  # -------------------------------------------------------------
  def setText( self, text ):
    self.textIcon= None
    self.text= text
    if text:
      self.textIcon= self.font[ 0 ].render( text, 1, self.font[ 1 ] )
  
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
    return self.choices[ activeChoice ].has_key( 'hidden' ) and self.choices[ activeChoice ][ 'hidden' ]== 'yes'
    
  # -------------------------------------------------------------
  def renderText( self, yPos ):
    # Draw text
    res= []
    if self.textIcon:
      xPos= ( self.rect[ 2 ]- self.textIcon.get_width() )/ 2
      res= [ ( self.textIcon, ( xPos, yPos- self.font[ 0 ].get_linesize()+ self.textIcon.get_height() ) ) ]
      yPos+= self.font[ 0 ].get_linesize()
    
    return yPos, res
    
  # -------------------------------------------------------------
  def renderCaption( self ):
    # Draw border( just a rectangle )
    yPos= BORDER_OFFS
    # Draw caption
    if self.captionIcon:
      xPos= ( self.rect[ 2 ]- self.captionIcon.get_width() )/ 2
      yPos+= self.font[ 0 ].get_linesize()
      res= [ ( self.captionIcon, ( xPos, self.font[ 0 ].get_linesize()- self.captionIcon.get_height() ) ) ]
    
    return yPos, res
  
  # -------------------------------------------------------------
  def renderSelected( self ):
    if self.activeChoice== -1:
      return []
    
    choice= self.choices[ self.activeChoice ]
    height= self.choiceFont[ 0 ].get_linesize()
    button= self.drawButton( ( choice[ 'width' ], height ), self.buttonColors[ 1 ] )
    return [ ( button, choice[ 'buttonPos' ] ) ]
  
  # -------------------------------------------------------------
  def render( self ):
    yPos, cap= self.renderCaption()
    yPos, text= self.renderText( yPos )
    # Draw choices as buttons
    choices= self.renderChoices( yPos )
    selected= self.renderSelected()
    self.setChanged( 0 )
    res= self.surface+ selected+ cap+ text+ map( lambda x: ( x[ 'icon' ], x[ 'pos' ] ), self.choices )
    return self.adjustPos( res, self.rect[ :2 ] )

# ****************************************************************************************
# Yes/No dialog area as the simple dialog 
class YesNoDialog( GenericChoiceDialog ):
  # -------------------------------------------------------------
  def __init__( self, rect, params, renderClass ):
    GenericChoiceDialog.__init__( self, rect, params, renderClass )
    self.choicesPerLine= len( self.choiceTexts )
    yPos= self.font[ 0 ].get_linesize()* 2
    self.renderChoices( yPos )
  
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
  def __init__( self, rect, params, renderClass ):
    GenericChoiceDialog.__init__( self, rect, params, renderClass )
    # Generate choices
    self.choicesPerLine= self.rect[ 2 ]/ ( int( self.getParam( 'width', 70 ))+ 5 )
    self.setChoices( self.getParam( 'choices', ( '<undefined>', ) ) )
    # Add additional choices for Ok and Cancel
    delta= self.choicesPerLine- ( len( self.choiceTexts ) % self.choicesPerLine )
    # Align choices to the left
    if delta< self.choicesPerLine:
      self.choices+= list( map( lambda x: { 'hidden': 'yes' }, range( delta ) ) )
    # Add additional choices for the control
    ctrlChoices= ( 'Ok', 'Clear', 'Cancel' )
    self.choices+= map( lambda x: { 'caption': x, 'width': 100 }, ctrlChoices )
    self.allowedSyms= self.getParam( 'allowedSyms', ( pygame.K_SPACE, pygame.K_z+ 1 ) )
  
  # -------------------------------------------------------------
  def renderText( self, yPos ):
    # Draw text
    offs= self.font[ 0 ].get_linesize()
    # Draw input border
    i= BORDER_OFFS+ 4
    res= [ ( pygame.Surface( ( self.rect[ 2 ]- i* 2, offs ) ), ( i, yPos+ 2 ) ) ]
    if self.textIcon:
      # Draw text
      res1= ( self.textIcon, ( i, yPos- self.textIcon.get_height()+ offs ) )
      # Draw cursor
      x= self.textIcon.get_width()
    else:
      x= 0
    
    pygame.draw.rect( res[ 0 ][ 0 ], self.color, ( i, yPos+ 2, self.rect[ 2 ], offs ), 2 )
    border= ( self.activeChoice== -1 )
    pygame.draw.rect( self.surface, self.font[ 1 ], ( x+ i, yPos+ 2, self.font[ 0 ].size(' ')[ 0 ], offs ), border )
    yPos+= offs
    return yPos
  
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
        self.activeChoice= -1
      
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
    
    if key== pygame.K_ESCAPE:
      self.exitDialog( None )
    
