#! /bin/env python

import sys


########################################################################3
# Simple video player 
def encodeVideo( fName, fOutFile, codecName ):
	# ------------------------------------
	import time, traceback
	import pymedia.video.muxer as muxer
	import pymedia.audio.acodec as acodec
	import pymedia.video.vcodec as vcodec
	
	format= fName.split( '.' )[ -1 ].lower()
	dm= muxer.Demuxer( format )
	f=  open( fName, 'rb' )
	fw= open(fOutFile, 'wb')
	s=  f.read( 390000 )
	r=  dm.parse( s )
	
	print "Streams found:", dm.streams
	ss= filter( lambda x: x[ 'type' ]== muxer.CODEC_TYPE_AUDIO, dm.streams )
	if len( ss )== 0:
		raise 'No suitable audio streams in a file'
	a_id= ss[ 0 ][ 'index' ]
	ac= acodec.Decoder( ss[ 0 ] )
	
	ss= filter( lambda x: x[ 'type' ]== muxer.CODEC_TYPE_VIDEO, dm.streams )
	if len( ss )== 0:
		raise 'No video streams in a file'
	v_id= ss[ 0 ][ 'index' ]
	vc = vcodec.Decoder( ss[ 0 ] )
	
	#create muxer
	mux= muxer.Muxer( 'mpg' )
	header= a_id1= v_id1= None
	m_data= []
	while len(s):
		for d in r:
			if d[ 0 ]== v_id:
				vfr= vc.decode( d[ 1 ] )
				if vfr:
					if v_id1== None:
						params= vc.getParams()
						params[ 'id' ]= vcodec.getCodecID( codecName )
						if codecName== 'mpeg1video':
							params[ 'bitrate' ]= 2700000
						elif codecName== 'mpeg2video':
							params[ 'bitrate' ]= 9800000
						
						print 'Video: encoder params ', params
						vc1= vcodec.Encoder( params )
						v_id1 = mux.addStream( muxer.CODEC_TYPE_VIDEO, vc1.getParams() )
						print "video stream id in muxer:", v_id1
					
					s1= vc1.encode( vfr )
					m_data.append( ( v_id1, s1 ))
			elif d[ 0 ]== a_id:
				frame= ac.decode( d[ 1 ] )
				if a_id1== None:
					params= { 'channels': 		frame.channels,
										'sample_rate': 	frame.sample_rate,
										'bitrate': 			frame.bitrate,
										'id': 					acodec.getCodecID( 'mp3' ) }
					print 'Audio: encoder params ', params
					ac1= acodec.Encoder( params ) 				#or any other supported audio encoder
					a_id1 = mux.addStream( muxer.CODEC_TYPE_AUDIO, ac1.getParams() )
					print "audio stream id in muxer:", a_id1
				
				encFrames= ac1.encode( frame.data )
				for fr in encFrames:
					m_data.append( ( a_id1, fr ) )
		
		# We know that video and audio exists already
		if a_id1!= None and v_id1!= None:
			# Start header when all streams are captured
			if header== None:
				header= mux.start()
				fw.write( header )
			
			for st_id, ss in m_data:
				s1= mux.write( st_id, ss )
				if s1:
					fw.write( s1 )
			
			m_data= []
			
		s= f.read( 100000 )
		r= dm.parse( s )
	
	fw.close()

if __name__== '__main__':
  if len( sys.argv )!= 4:
    print "Usage: encode_video <file> <output file> <format>\n\tformat = { mpeg1video | mpeg2video }"
  else:
    encodeVideo( sys.argv[1], sys.argv[2], sys.argv[3] )
