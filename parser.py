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
import string, pyRXP
from __main__ import *
from pymedia import *

# ****************************************************************************************************
class NodeElement:
  """
    Base class that gives some basic conversion functions to all elements  
  """
  # -----------------------------------------------------------------
  def __init__( self, node ):
    self.node= node
    self.params= self.getAttributes( self.node )
  
  # -----------------------------------------------------------------
  def execute( self, action, key, item ):
    from __main__ import *
    try:
      s1= self.params[ action ]
    except:
      return
    
    if self.params.has_key( 'module' ) and self.params[ 'module' ]:
      modParams= self.params[ 'module' ].params
      # Get actions variable
      if s1[ 0 ]== '%':
        s= eval( modParams[ 'actions' ] )
        s1= s[ self.params[ 'id' ]+ '.'+ s1[ 1: ] ]
    
    exec( s1 )
    
  # -----------------------------------------------------------------
  def init( self ):
    self.execute( 'onInit', None, None )
  
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
    from __main__ import *
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
    # get areas, menuDefs and modules
    self.params[ 'module' ]= currentModule
    for i in xrange( len( self.node[ 2 ] ) ):
      curNode= self.node[ 2 ][ i ]
      if curNode[ 0 ]== 'params':
        params= self.getParams( curNode )
        self.params[ 'params' ]= params
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
              target[ obj.params[ 'id' ] ]= obj.load()

# ****************************************************************************************************
class appElement( NodeElement ):
  """
    Class that processes app node in config file  
  """
  # -----------------------------------------------------------------
  def __init__( self, node ):
    NodeElement.__init__( self, node )
    self.filter= ( 'areas', 'modules', 'menuDefs' )
  
  # -----------------------------------------------------------------
  def getParam( self, name ):
    return self.params[ name ]

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
    from __main__ import *
    params= self.params
    rect= self.getCoords( self.params[ 'coords' ] )
    
    renderer= None
    if self.params[ 'renderer' ]!= '':
      renderer= eval( '%s( rect, params )' % self.params[ 'renderer' ] )
      
    s= '%s( rect, params, renderer )' % self.params[ 'displayer' ]
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
        self.params[ 'onClick' ]= item[ 'onClick' ]
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
    params= self.getAttributes( node )
    self.node= self.loadXMLFile( params[ 'file' ] )
    self.params= self.getAttributes( self.node )
    self.filter= ( 'areas', 'menuDefs' )
    currentModule= self
    
    # import modules into the __main__ namespace
    import __main__
    s= self.params[ 'import' ]
    exec( 'import %s' % s )
    
    mods= string.split( s, ',' )
    modules= []
    for mod in mods:
      modules.append( string.split( mod, ' ' )[ -1 ] )
    
    for module in modules:
      __main__.__dict__[ module ]= eval( module )
      
    print 'Module %s modules %s' % ( self.params[ 'id' ], modules )
  
  # -----------------------------------------------------------------
  def execute( self, action ):
    """
      Execute action on the module level.
      Looks up into the actions parameter to get the file name where all
      actions are defined. If the action is not found, just ignore it.
    """
    from __main__ import *
    params= self.params[ 'params' ]
    # Get actions variable
    s= eval( self.params[ 'actions' ] )
    s1= self.params[ action ]
    if s1[ 0 ]== '%':
      s1= s[ self.params[ 'id' ]+ '.'+ s1[ 1: ] ]
    
    exec( s1 )
    
  # -----------------------------------------------------------------
  def load( self ):
    """
      Load module into the memory.
      basically just executes onLoad action.
    """
    print 'Load module:', self.params[ 'id' ]
    self.execute( 'onLoad' )
    return self
  
  # -----------------------------------------------------------------
  def unload( self ):
    """
      Unload module from the memory.
      Basically just executes onUnLoad action.
      onUnLoad action should clean up everything that were allocated and
      stop any playing devices
    """
    print 'Unload module:', self.params[ 'id' ]
    self.execute( 'onUnLoad' )
    params= self.params[ 'params' ]
  
# ****************************************************************************************************
# Class that directs root elements in config file to the right place
class ConfigElement( NodeElement ):
  """
    Class that processes root node of the config file
  """
  # -----------------------------------------------------------------
  def __init__( self ):
    global currentModule
    currentModule= None
    self.node= None
  
  # -----------------------------------------------------------------
  def parse( self, node ):
    # take the list of applications and put them into the apps dictionary
    for i in xrange( len( node ) ):
      appNode= node[ i ]
      if appNode[ 0 ]== 'app':
        app= self.processNodeByName( appNode )
        apps[ app.params[ 'id' ] ]= app.load()
  
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
  areas= {}
  modules= {}
  menuDefs= {}
  
  # Pass dom model to the parser
  config= ConfigElement()
  config.parseFile( 'config.xml' )
  
  # Print result 
  print 'Apps', apps
  print 'Modules', modules
  print 'Areas', areas
  print 'Menus', menuDefs
  