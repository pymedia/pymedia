##    parser - Part of the homedia package. Parses menus, areas and modules
##        Defines menus internal structure based on extrenal xml file
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
import string, pyRXP, pygame
from pymedia import *
from __main__ import *

currentModule= None
currentApp= None


# ****************************************************************************************************
class NodeElement( menu.ModuleHelper ):
  """
    Base class that gives some basic conversion functions to all elements  
  """
  # -----------------------------------------------------------------
  def __init__( self, node ):
    self.node= node
    self.attributes= self.getAttributes( self.node )
    self.params= {}
  
  # -----------------------------------------------------------------
  def init( self ):
    self.execute( 'onInit', None, None )
    return self
  
  # -----------------------------------------------------------------
  def load( self ):
    self.execute( 'onLoad', None, None )
    return self
  
  # -----------------------------------------------------------------
  def loadXMLFile( self, fileName ):
    s= open( fileName, 'rt' ).read()
    p= pyRXP.Parser()
    return p.parse( s )
    
  # -----------------------------------------------------------------
  def processNodeByName( self, node ):
    try:
      cls= eval( node[ 0 ]+ 'Element' )
    except:
      return None
    
    element= cls( node )
    element.parse()
    return element
  
  # -----------------------------------------------------------------
  def getAttributes( self, node ):
    atts= node[ 1 ]
    for att in atts:
      if type( atts[ att ] )== list:
        atts[ att ]= atts[ att ][ 0 ]
    
    return atts
  
  # -----------------------------------------------------------------
  def getParams( self, node ):
    # return leaves as list of dictionaries
    res= {}
    if node[ 0 ]== 'params':
      for i in xrange( len( node[ 2 ] ) ):
        child= node[ 2 ][ i ]
        if child[ 0 ]== 'param':
          atts= self.getAttributes( child )
          if atts.has_key( 'list' ) and atts[ 'list' ]== 'yes':
            vals= self.getLeavesPlain( child )
          elif atts.has_key( 'eval' ) and atts[ 'eval' ]== 'true':
            vals= eval( child[ 2 ][ 0 ] )
          else:
            vals= child[ 2 ][ 0 ]
          
          res[ atts[ 'id' ]]= vals
        
    return res
  
  # -----------------------------------------------------------------
  def getItems( self, node ):
    # return leaves as list of dictionaries
    res= []
    if node[ 0 ]== 'items':
      for i in xrange( len( node[ 2 ] ) ):
        child= node[ 2 ][ i ]
        if child[ 0 ]== 'item':
          atts= self.getAttributes( child )
          if atts.has_key( 'eval' ) and atts[ 'eval' ]== 'true':
            res+= eval( child[ 2 ][ 0 ] )
          else:
            res.append( atts )
        
    return res
  
  # -----------------------------------------------------------------
  def getLeaves( self, node ):
    # return leaves as list of dictionaries
    res= []
    for i in xrange( len( node.childNodes[ 2 ] ) ):
      child= node.childNodes[ 2 ][ i ]
      nodeEl= self.getAttributes( child )
      res+= nodeEl
    
    return res
  
  # -----------------------------------------------------------------
  def getLeavesPlain( self, node ):
    # return nodes as list of values
    exec( self.getModuleImport() )
    res= []
    for i in xrange( len( node[ 2 ] ) ):
      child= node[ 2 ][ i ]
      if child[ 0 ]== 'value':
        if child[ 1 ] and child[ 1 ].has_key( 'eval' ) and child[ 1 ][ 'eval' ]== 'true':
          res+= eval( child[ 2 ][ 0 ] )
        else:
          res.append( child[ 2 ][ 0 ] )
    
    return res
  
  # -----------------------------------------------------------------
  def parse( self ):
    # get areas, menus and modules
    self.attributes[ 'app' ]= currentApp
    if currentModule:
      self.attributes[ 'module' ]= currentModule
    if self.node[ 2 ]== None:
      return
    for i in xrange( len( self.node[ 2 ] ) ):
      curNode= self.node[ 2 ][ i ]
      if curNode[ 0 ]== 'params':
        self.params= self.getParams( curNode )
      elif curNode[ 0 ] in self.filter:
        if curNode[ 0 ]== 'items':
          items= self.getItems( curNode )
          self.params[ 'items' ]= items
        else:
          for j in xrange( len( curNode[ 2 ] ) ):
            obj= self.processNodeByName( curNode[ 2 ][ j ] )
            if obj:
              # Place node structure into the appropriate dictionary
              target= eval( curNode[ 0 ] )
              target[ obj.attributes[ 'id' ] ]= obj.load()

# ****************************************************************************************************
class appElement( NodeElement ):
  """
    Class that processes app node in config file  
  """
  # -----------------------------------------------------------------
  def __init__( self, node ):
    global currentApp
    NodeElement.__init__( self, node )
    currentApp= self
    print 'Application %s' % ( self.attributes[ 'id' ], )
    self.filter= ( 'areas', 'modules', 'menus', 'fonts' )
  
  # -----------------------------------------------------------------
  def getParam( self, name, default= None ):
    try: return self.attributes[ name ]
    except: return default

# ****************************************************************************************************
# Class that holds fonts
class fontElement( appElement ):
  # -----------------------------------------------------------------
  def __init__( self, node ):
    NodeElement.__init__( self, node )

# ****************************************************************************************************
# Class that creates area definition based on content of current node
class areaElement( NodeElement ):
  """
    Class that processes area node in the config file  
  """
  # -----------------------------------------------------------------
  def __init__( self, node ):
    NodeElement.__init__( self, node )
    self.filter= ()
  
  # -----------------------------------------------------------------
  def getCoords( self, str ):
    return map( lambda x: int( x ), string.split( str, ',' ) )
  
  # -----------------------------------------------------------------
  def load( self ):
    exec( self.getModuleImport() )
    attributes= self.attributes
    params= self.params
    rect= self.getCoords( attributes[ 'coords' ] )
    
    renderer= None
    if attributes[ 'renderer' ]!= '':
      renderer= eval( '%s( rect, attributes, params )' % attributes[ 'renderer' ] )
      
    s= '%s( rect, attributes, params, renderer )' % attributes[ 'displayer' ]
    area= eval( s )
    area.setActive( 0 )
    return area

# ****************************************************************************************************
# Class that creates area definition based on content of current node
class menuElement( NodeElement ):
  """
    Class that processes menu node in the config file  
  """
  # -----------------------------------------------------------------
  def __init__( self, node ):
    NodeElement.__init__( self, node )
    self.filter= ( 'items', )
    self.changed= 0
  
  # -----------------------------------------------------------------
  def items( self ):
    return filter( lambda x: not x.has_key( 'hidden' ) or x[ 'hidden' ]== 'no', self.params[ 'items' ] )
  
  # -----------------------------------------------------------------
  def setChanged( self, changed ):
    self.changed= changed
    
  # -----------------------------------------------------------------
  def hasChanged( self ):
    return self.changed
  
  # -----------------------------------------------------------------
  def processKey( self, itemPos, key ):
    if itemPos== -1:
      item= None
    else:
      item= self.items()[ itemPos ]
      
    if key!= pygame.K_RETURN:
      self.execute( 'onKeyPress', key, item )
    else:
      if item.has_key( 'onClick' ):
        self.attributes[ 'onClick' ]= item[ 'onClick' ]
        self.execute( 'onClick', key, item )
    
  
# ****************************************************************************************************
# Class that processes application node in config file
class moduleElement( NodeElement ):
  """
    Class that processes module node in the config file  
  """
  # -----------------------------------------------------------------
  def __init__( self, node ):
    """
      Complex constructor for the module element.
      Imports needed modules and puts them into the __main__ context.
    """
    global currentModule
    currentModule= self
    attributes= self.getAttributes( node )
    self.node= self.loadXMLFile( attributes[ 'file' ] )
    self.attributes= self.getAttributes( self.node )
    self.filter= ( 'areas', 'menus', 'fonts' )
    
    # Import actions
    self.importActions()
    print 'Module %s imported' % ( self.attributes[ 'id' ] )
  
  # -----------------------------------------------------------------
  def load( self ):
    """
      Load module into the memory.
      basically just executes onLoad action.
    """
    print 'Load module:', self.attributes[ 'id' ]
    self.execute( 'onLoad', None, None )
    return self
  
  # -----------------------------------------------------------------
  def unload( self ):
    """
      Unload module from the memory.
      Basically just executes onUnLoad action.
      onUnLoad action should clean up everything that were allocated and
      stop any playing devices
    """
    print 'Unload module:', self.attributes[ 'id' ]
    self.execute( 'onUnLoad', None, None )
  
# ****************************************************************************************************
# Class that directs root elements in config file to the right place
class ConfigElement( NodeElement ):
  """
    Class that processes root node of the config file
  """
  # -----------------------------------------------------------------
  def __init__( self ):
    currentModule= None
    self.node= None
  
  # -----------------------------------------------------------------
  def parse( self, node ):
    # take the list of applications and put them into the apps dictionary
    for i in xrange( len( node ) ):
      appNode= node[ i ]
      if appNode[ 0 ]== 'app':
        app= self.processNodeByName( appNode )
        apps[ app.attributes[ 'id' ] ]= app.load()
  
  # -----------------------------------------------------------------
  def parseFile( self, fileName ):
    """
      Main entry for the parsing.
      Start from the very first child( should be app element )
    """
    f= self.loadXMLFile( fileName )
    self.parse( f[ 2 ] )

# Test for the basic parser functionality
if __name__== '__main__':
  def hideAll( areas ):
    for s in areas:
      areas[ s ].setVisible( 0 )
  
  xmlStr= """
<config>
  <!-- Defines app name and default icon -->
  <app id="homedia" resolution="640,480" fullScreen="yes" caption="Home Media center" icon="" onLoad="self.init()" onInit="hideAll( areas ); areas[ 'rootmenu' ].init( menuDefs[ 'rootMenu' ] );">
    <!-- Define areas where root menus will reside -->
    <areas>
      <area id="rootmenu" 
            module="menu" 
            renderer="menu.MenuItem" 
            displayer="menu.ListDisplay"
            coords="210,120,210,200">
        <params>
          <param id="font">Arial.ttf</param>
          <param id="fontSize">20</param>
          <param id="scroll">yes</param>
        </params>
      </area>
    </areas>
    <!-- Define menus for the root of application -->		
    <menuDefs>
      <!-- Root app menu -->
      <menu id="rootMenu" onInit="hideAll( areas ); areas[ 'rootmenu' ].setVisible( 1 ); setActive( areas[ 'rootmenu' ] );">
        <items>
          <item caption="Music" onClick="areas[ 'menu' ].init( menuDefs[ 'rootDirectoryMenu' ] ); "/>
          <item caption="GPS" onClick=""/>
          <item caption="Settings" onClick=""/>
          <item caption="Exit" onClick="EXIT_FLAG= 1;"/>
        </items>
      </menu>
    </menuDefs>
    <!-- Defines modules and their properties -->
    <modules>
      <module id="music" import="pycdda,pympg,pysound" onLoad="pycdda.init(); pysound.stop();" onInit="hideAll( areas ); areas[ 'menu' ].init( menuDefs[ 'rootAudioMenu' ] ); areas[ 'menu' ].setVisible( 1 ); areas[ 'bgdisplay' ].setVisible( 1 );">
        <params>
          <param id="root" list="yes">
            <value>c:/bors/music</value>
            <value generated="yes">pycdda.cdroms();</value>	
          </param>
          <param id="iconFiles" list="yes">
            <value>JPG</value>
            <value>BMP</value>
            <value>GIF</value>
          </param>
        </params>
        <menuDefs>
          <!-- Root audio menu -->
          <menu id="rootAudioMenu" onInit="hideAll( areas ); areas[ 'audiomenu' ].setVisible( 1 ); areas[ 'audiobgdisplay' ].setVisible( 1 ); setActive( areas[ 'audiomenu' ] );">
            <items>
              <item caption="Directories" onClick="areas[ 'audiomenu' ].init( menuDefs[ 'rootDirectoryMenu' ] ); "/>
              <item caption="Playlists" onClick="areas[ 'audiomenu' ].init( menuDefs[ 'rootPlaylistMenu' ] );"/>
              <item caption="What's playing" onClick="areas[ 'audiomenu' ].init( menuDefs[ 'playingMenu' ] );"/>
              <item caption="Back" onClick="areas[ 'rootmenu' ].init( menuDefs[ 'rootMenu' ] );"/>
            </items>
          </menu>
          <!-- Root menu for showing playlists -->
          <menu id="rootPlaylistMenu" onInit="hideAll( areas ); areas[ 'audiomenu' ].setVisible( 1 ); areas[ 'audiobgdisplay' ].setVisible( 1 ); areas[ 'audioplaylistnames' ].setVisible( 1 ); setActive( areas[ 'audioplaylistnames' ] ); ">
            <items>
              <item caption="New" onClick="areas[ 'rootentertext' ].setText( areas[ 'audioplaylistnames' ].src.getNewFileName() ); areas[ 'rootentertext' ].setVisible( 1 ); setActive( areas[ 'rootentertext' ] );"/>
              <item caption="Delete" onClick="areas[ 'rootyesno' ].setText( 'Remove '+ areas[ 'playlistnames' ].src.getCurrentFileName()+ ' ?' ); areas[ 'rootyesno' ].setVisible( 1 ); setActive( areas[ 'rootyesno' ] );"/>
              <item caption="Back" onClick="areas[ 'audiomenu' ].init( menuDefs[ 'rootAudioMenu' ] );"/>
            </items>
          </menu>
          <!-- Root menu for showing directories -->
          <menu id="rootDirectoryMenu" onInit="hideAll( areas ); areas[ 'audiomenu' ].setVisible( 1 ); areas[ 'audiobgdisplay' ].setVisible( 1 ); areas[ 'audiofilelist' ].setVisible( 1 ); setActive( areas[ 'audiomenu' ] );">
            <items>
              <item caption="Play All" onClick="files=areas[ 'audiofilelist' ].src.getFiles(); for f in files: addToPlayList( f, 1, 0 ); areas[ 'audiofilelist' ].setChanged( 1 );"/>
              <item caption="Play Files" onClick="files=areas[ 'audiofilelist' ].src.getFiles(); for f in files: addToPlayList( f, 0, 0 ); areas[ 'audiofilelist' ].setChanged( 1 );"/>
              <item caption="Sort By Title" onClick="areas[ 'audiofilelist' ].src.sortByTitle();"/>
              <item caption="Randomize" onClick="areas[ 'audiofilelist' ].src.randomize();"/>
              <item caption="Back" onClick="areas[ 'audiomenu' ].init( menuDefs[ 'rootAudioMenu' ] );"/>
            </items>
          </menu>
          <!-- Root menu for showing what's playing right now -->
          <menu id="playingMenu" onInit="hideAll( areas ); areas[ 'audiomenu' ].setVisible( 1 ); areas[ 'audioplayerdisplay' ].setVisible( 1 ); areas[ 'audioplaylist' ].setVisible( 1 ); setActive( areas[ 'audiomenu' ] );">
            <items>
              <item caption="Find Playing" onClick="index=areas[ 'audioplaylist' ].control.getPlaying(); areas[ 'audioplaylist' ].control.setSelected( index ); setActive( areas[ 'audioplaylist' ] );"/>
              <item caption="Sort By Title" onClick="areas[ 'audioplaylist' ].control.sortByTitle(); "/>
              <item caption="Randomize" onClick="areas[ 'audioplaylist' ].src.randomize();"/>
              <item caption="Back" onClick="areas[ 'audiomenu' ].init( menuDefs[ 'rootAudioMenu' ] );"/>
            </items>
          </menu>
          <!-- Menu for the selected playlist -->
          <menu id="playlistMenu" onInit="hideAll( areas ); areas[ 'audiomenu' ].setVisible( 1 ); areas[ 'audioplayerdisplay' ].setVisible( 1 ); areas[ 'audioplaylist' ].setVisible( 1 ); setActive( areas[ 'audiomenu' ] );">
            <items>
              <item caption="Play" onClick="audiofile.player.setPlaylist( areas[ 'audioplaylist' ].src );"/>
              <item caption="Randomize" onClick="areas[ 'audioplaylist' ].src.randomize();"/>
              <item caption="Delete" onClick="areas[ 'audioplaylist' ].src.removeActive();"/>
              <item caption="Back" onClick="areas[ 'audiomenu' ].init( menuDefs[ 'rootPlaylistMenu' ] );"/>
            </items>
          </menu>
        </menuDefs>
        <areas>
          <!-- Audio menu properties -->
          <area id="audiomenu" 
                module="menu" 
                renderer="menu.MenuItem" 
                displayer="menu.ListDisplay" 
                coords="0,40,200,240">
            <params>
              <param id="font">Arial.ttf</param>
              <param id="fontSize">20</param>
              <param id="scroll">yes</param>
            </params>
          </area>
          <!-- List of audio playlists -->
          <area id="audioplaylistnames" 
                module="menu" 
                renderer="menu.PlainListItem" 
                displayer="menu.ListDisplay" 
                coords="0,40,200,240">
            <params>
              <param id="font">Arial.ttf</param>
              <param id="fontSize">30</param>
              <param id="scroll">yes</param>
            </params>
          </area>
        </areas>
      </module>
    </modules>
  </app>
</config>
"""
  # Import pygame
  import pygame
  pygame.init()
  
  # Create basic dictionaries
  apps= {}
  fonts= {}
  areas= {}
  modules= {}
  menus= {}
  
  # Pass dom model to the parser
  config= ConfigElement()
  config.parseFile( 'config.xml' )
  
  # Print result 
  print 'Apps', apps
  print 'Modules', modules
  print 'Areas', areas
  print 'Menus', menus
  print 'Fonts', fonts
  