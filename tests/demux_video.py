import pymedia.muxer as muxer

def demuxVideo( inFile ):
	dm= muxer.Demuxer( inFile.split( '.' )[ -1 ].lower() )
	f= open( inFile, 'rb' )
	s= f.read( 400000 )
	r= dm.parse( s )
	while len( s )> 0:
		print len( r ),
		s= f.read( 40000 )
		r= dm.parse( s )

demuxVideo( 'c:\\movies\\Lost.In.Translation\\Lost.In.Translation.CD2.avi' )


