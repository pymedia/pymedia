
actions={

# -----------------------------------------------------------------
# On music module load
'music.onLoad': """

audio.pycdda.init()
audio.pysound.stop()
items= audio.cache.setRoot( params[ 'root' ] )
audio.cache.setCacheSize( long( params[ 'cacheSizeM' ] )* 1000000 )
audio.playLists.load( params[ 'playlistDir' ] )
wrapper= audio.FileWrapper( items, () )
areas[ 'audiofilelist' ].init( wrapper, -1 )
pl= audio.playLists.getDefault()
audio.player.setPlayList( pl )
audio.player.start()
"""

# -----------------------------------------------------------------
# Music module init action
,'music.onInit': """

areas[ 'audiomenu' ].init( menuDefs[ 'rootAudioMenu' ], 0 )
"""

# -----------------------------------------------------------------
,'rootAudioMenu.onKeyPress':"""

audio.processUniKeys( key )
if key in( pygame.K_BACKSPACE, pygame.K_ESCAPE ):
	areas[ 'rootmenu' ].init( menuDefs[ 'rootMenu' ], 0 )
"""

# -----------------------------------------------------------------
,'rootAudioMenu.onInit':"""

hideAll( areas )
setActive( areas[ 'audiomenu' ] )
areas[ 'audiobgdisplay' ].setVisible( 1 )
"""

# -----------------------------------------------------------------
,'rootAudioMenu.whatsPlaying':"""

areas[ 'audiomenu' ].init( menuDefs[ 'playingMenu' ], 0 )
pl= audio.player.getPlayList()
areas[ 'playerlistcontent' ].init( pl[ 0 ], pl[ 1 ] )
if len( pl[ 0 ].items() )> 0:
	setActive( areas[ 'audioplayerdisplay' ] )
"""

# -----------------------------------------------------------------
,'rootPlaylistMenu.onKeyPress':"""

audio.processUniKeys( key )
if key in( pygame.K_ESCAPE,  ):
	areas[ 'rootmenu' ].init( menuDefs[ 'rootMenu' ], 0 )
if key in ( pygame.K_RIGHT, pygame.K_TAB ):
	setActive( areas[ 'audioplaylist' ], areas[ 'audiomenu' ] )
if key in( pygame.K_BACKSPACE, ):
	areas[ 'audiomenu' ].init( menuDefs[ 'rootAudioMenu' ], 0 )
"""

# -----------------------------------------------------------------
,'rootPlaylistMenu.onInit':"""

hideAll( areas )
showAll( ( 'audiomenu', 'audioplaylist', 'audiobgdisplay' ), areas )
areas[ 'audioplaylist' ].init( audio.playLists, -1 )
setActive( areas[ 'audioplaylist' ] )
"""

# -----------------------------------------------------------------
,'rootPlaylistMenu.new':"""

pl= audio.playLists.getPlayList()
audio.player.setPlayList( pl )
areas[ 'audioplaylist' ].init( audio.playLists, -1 )
areas[ 'audioplaylist' ].setChanged( 1 )
"""

# -----------------------------------------------------------------
,'rootPlaylistMenu.reload':"""

audio.playLists.reload()
pl= audio.playLists.getDefault()
audio.player.setPlayList( pl )
areas[ 'audioplaylist' ].setChanged( 1 )
"""

# -----------------------------------------------------------------
,'rootDirectoryMenu.onKeyPress':"""

audio.processUniKeys( key )
if key in( pygame.K_ESCAPE, ):
	areas[ 'rootmenu' ].init( menuDefs[ 'rootMenu' ], 0 )
if key in ( pygame.K_RIGHT, pygame.K_TAB ):
	setActive( areas[ 'audiofilelist' ], areas[ 'audiomenu' ] )
if key in( pygame.K_BACKSPACE, ):
	areas[ 'audiomenu' ].init( menuDefs[ 'rootAudioMenu' ], 0 )
"""

# -----------------------------------------------------------------
,'rootDirectoryMenu.onInit':"""

hideAll( areas )
showAll( ( 'audiomenu', 'audiofilelist', 'audiobgdisplay' ), areas )
setActive( areas[ 'audiomenu' ] )
"""

# -----------------------------------------------------------------
,'rootDirectoryMenu.playAll':"""

files= areas[ 'audiofilelist' ].getWrapper().items()
audio.addToPlayList( files, (), 1 )
areas[ 'audiofilelist' ].setChanged( 1 )
"""

# -----------------------------------------------------------------
,'rootDirectoryMenu.playFiles':"""

files=areas[ 'audiofilelist' ].getWrapper().items()
audio.addToPlayList( files, (), 0 )
areas[ 'audiofilelist' ].setChanged( 1 )
"""

# -----------------------------------------------------------------
,'rootDirectoryMenu.whatsPlaying':"""

areas[ 'audiomenu' ].init( menuDefs[ 'playingMenu' ], 0 )
pl= audio.player.getPlayList()
areas[ 'playerlistcontent' ].init( pl[ 0 ], pl[ 1 ] )
if len( pl[ 0 ].items() )> 0: 
	setActive( areas[ 'audioplayerdisplay' ] )
"""

# -----------------------------------------------------------------
,'playlistMenu.onKeyPress':"""

audio.processUniKeys( key )
if key in( pygame.K_ESCAPE, ):
	areas[ 'rootmenu' ].init( menuDefs[ 'rootMenu' ], 0 )
if key in ( pygame.K_RIGHT, pygame.K_TAB ):
	setActive( areas[ 'playlistcontent' ], areas[ 'audiomenu' ])
if key in( pygame.K_BACKSPACE, ):
	areas[ 'audiomenu' ].init( menuDefs[ 'rootPlaylistMenu' ], 0 )
"""

# -----------------------------------------------------------------
,'playlistMenu.onInit':"""

hideAll( areas )
showAll( ( 'playlistcontent', 'audiomenu', 'audiobgdisplay' ), areas  )
setActive( areas[ 'audiomenu' ] )
"""

# -----------------------------------------------------------------
,'playlistMenu.playAll':"""

wrapper= areas[ 'playlistcontent' ].getWrapper()
pos= audio.playLists.getPlayListPos( wrapper )
areas[ 'playlistcontent' ].moveTo( pos )
if len( wrapper.items() )> 0:
	areas[ 'playlistcontent' ].processKey( pygame.K_RETURN )
"""

# -----------------------------------------------------------------
,'playlistMenu.setAsCurrent':"""

audio.player.stopPlayback()
pl= areas[ 'playlistcontent' ].getWrapper()
audio.player.setPlayList( ( pl, audio.playLists.getPlayListPos( pl ) ))
"""

# -----------------------------------------------------------------
,'playlistMenu.clear':"""

pl= areas[ 'playlistcontent' ].getWrapper()
pl.clear()
if pl== audio.player.getPlayList()[ 0 ]:
	audio.player.stopPlayback()
audio.playLists.setPosition( ( pl, 0 ) )
"""

# -----------------------------------------------------------------
,'playingMenu.onKeyPress':"""

audio.processUniKeys( key )
if key in( pygame.K_ESCAPE, ):
	areas[ 'rootmenu' ].init( menuDefs[ 'rootMenu' ], 0 )
if key in ( pygame.K_RIGHT, pygame.K_TAB ):
	setActive( areas[ 'playerlistcontent' ], areas[ 'audiomenu' ])
if areas[ 'playerlistcontent' ].isActive():
	i= audio.player.getPlayList()[ 1 ]
	areas[ 'playerlistcontent' ].moveTo( i )
	areas[ 'playerlistcontent' ].setItems( i )
if key in( pygame.K_BACKSPACE, ):
	areas[ 'audiomenu' ].init( menuDefs[ 'rootPlaylistMenu' ], 0 )
"""

# -----------------------------------------------------------------
,'playingMenu.onInit':"""

hideAll( areas )
showAll( ( 'playerlistcontent', 'audioplayerdisplay', 'audiomenu'), areas )
setActive( areas[ 'audiomenu' ] )
pl= audio.player.getPlayList()
areas[ 'playerlistcontent' ].init( pl[ 0 ], pl[ 1 ] )
if len( pl[ 0 ].items() )> 0:
	setActive( areas[ 'audioplayerdisplay' ] )
"""

# -----------------------------------------------------------------
,'playingMenu.playAll':"""

pos= audio.playLists.getPlayListPos( areas[ 'playerlistcontent' ].getWrapper() )
areas[ 'playerlistcontent' ].moveTo( pos )
areas[ 'playerlistcontent' ].processKey( pygame.K_RETURN )
"""

# -----------------------------------------------------------------
,'playingMenu.clear':"""

pl= areas[ 'playerlistcontent' ].getWrapper()
pl.clear()
audio.player.stopPlayback()
audio.playLists.setPosition( ( pl, 0 ) )
"""

# -----------------------------------------------------------------
,'audiofilelist.onKeyPress':"""

audio.processUniKeys( key )
if key in( pygame.K_ESCAPE, ):
	areas[ 'rootmenu' ].init( menuDefs[ 'rootMenu' ], 0 )
if key in ( pygame.K_LEFT, pygame.K_TAB ): 
	setActive( areas[ 'audiomenu' ] )
"""

# -----------------------------------------------------------------
,'playlistcontent.onKeyPress':"""

audio.processUniKeys( key )
if key in( pygame.K_ESCAPE, ):
	areas[ 'rootmenu' ].init( menuDefs[ 'rootMenu' ], 0 )
if key in ( pygame.K_LEFT, pygame.K_TAB ):
	setActive( areas[ 'audiomenu' ] )
if key== pygame.K_DELETE:
	# Remove file from the playlist
	a= areas[ 'yesnodialog' ]
	file= areas[ 'playlistcontent' ].getWrapper().getFile( item )
	a.init( 'Delete ?', translate( file[ 'title' ] ), 'playlistcontent', ( item, ) )
	setActive( a )
if key in( pygame.K_BACKSPACE, ):
	areas[ 'audiomenu' ].init( menuDefs[ 'rootPlaylistMenu' ], 0 )
"""

# -----------------------------------------------------------------
,'playlistcontent.onDialogReturn':"""

a= areas[ 'yesnodialog' ]
a.setVisible( 0 )
if a.getChoice() and a.getChoice()[ 'caption' ]== 'yes':
	areas[ 'playlistcontent' ].getWrapper().remove( a.getParams()[ 0 ] )

setActive( areas[ 'playlistcontent' ] )
"""

# -----------------------------------------------------------------
,'playerlistcontent.onKeyPress':"""

audio.processUniKeys( key )
if key in( pygame.K_ESCAPE, ):
	areas[ 'rootmenu' ].init( menuDefs[ 'rootMenu' ], 0 )
if key in ( pygame.K_LEFT, ):
	setActive( areas[ 'audiomenu' ] )
if key== pygame.K_DELETE:
	file= areas[ 'playerlistcontent' ].getWrapper().getFile( item )
	a= areas[ 'yesnodialog' ]
	a.init( 'Delete ?', file[ 'title' ], 'playerlistcontent', ( item, ) )
	setActive( a )
if key in ( pygame.K_TAB, ):
	setActive( areas[ 'audioplayerdisplay' ] )
"""

# -----------------------------------------------------------------
,'playerlistcontent.onDialogReturn':"""

a= areas[ 'yesnodialog' ]
a.setVisible( 0 )
if a.getChoice() and a.getChoice()[ 'caption' ]== 'yes':
	# See if playing file is below the removed one
	item= a.getParams()[ 0 ]
	i= audio.player.getCurrentFileIndex()
	if i== item:
		audio.player.setCurrentFileIndex( -1 )
	elif i> item:
		audio.player.setCurrentFileIndex( i- 1, 0 )
	areas[ 'playerlistcontent' ].getWrapper().remove( item )

setActive( areas[ 'playerlistcontent' ] )
"""

# -----------------------------------------------------------------
,'audioplayerdisplay.onKeyPress':"""

audio.processUniKeys( key )
if key in( pygame.K_ESCAPE, ):
	areas[ 'rootmenu' ].init( menuDefs[ 'rootMenu' ], 0 )
if key in ( pygame.K_TAB, ):
	setActive( areas[ 'audiomenu' ] )
if key in ( pygame.K_LEFT, ):
	setActive( areas[ 'playerlistcontent' ] )
"""

# -----------------------------------------------------------------
,'audioplaylist.onKeyPress':"""

audio.processUniKeys( key )
if key in( pygame.K_ESCAPE, ):
	areas[ 'rootmenu' ].init( menuDefs[ 'rootMenu' ], 0 )
if key in ( pygame.K_LEFT, pygame.K_TAB, pygame.K_BACKSPACE ):
	setActive( areas[ 'audiomenu' ] )
if key== pygame.K_DELETE:
	pl= audio.playLists.getPlayList( item[ 'caption' ] )
	if pl[ 0 ]!= audio.player.getPlayList()[ 0 ]:
		a= areas[ 'yesnodialog' ]
		a.init( 'Delete ?', item[ 'caption' ], 'audioplaylist', ( pl, ) )
		setActive( a )
"""

# -----------------------------------------------------------------
,'audioplaylist.onDialogReturn':"""

a= areas[ 'yesnodialog' ]
a.setVisible( 0 )
if a.getChoice() and a.getChoice()[ 'caption' ]== 'yes':
	pl= a.getParams()[ 0 ]
	audio.playLists.remove( pl[ 0 ] )
	areas[ 'audioplaylist' ].init( audio.playLists, -1 )

setActive( areas[ 'audioplaylist' ] )
"""

# -----------------------------------------------------------------
,'audioplaylist.onEnter':"""

areas[ 'audiomenu' ].init( menuDefs[ 'playlistMenu' ], 0 )
pl= audio.playLists.getPlayList( item[ 'caption' ] )
areas[ 'playlistcontent' ].init( pl[ 0 ], pl[ 1 ] )
if len( pl[ 0 ].items() )!= 0:
	setActive( areas[ 'playlistcontent' ] )
"""
}

