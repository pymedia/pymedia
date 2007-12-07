#! /bin/env python
import sys
import pymedia.muxer as muxer
import pymedia.video.vcodec as vcodec
import pymedia.audio.acodec as acodec

def recodeVideo( inFile, outFile, outCodec ):
	dm= muxer.Demuxer( inFile.split( '.' )[ -1 ] )
	f= open( inFile, 'rb' )
	open( outFile, 'wb' ).close()
	s= f.read( 600000 )
	r= dm.parse( s )
	v= [ x for x in dm.streams if x[ 'type' ]== muxer.CODEC_TYPE_VIDEO ]
	if len( v )== 0:
		raise 'There is no video stream in a file %s' % inFile
	
	v_id= v[ 0 ][ 'index' ]
	print 'Video stream at %d index: ' % v_id
	
	a_id= -1
	a= [ x for x in dm.streams if x[ 'type' ]== muxer.CODEC_TYPE_AUDIO ]
	if len( a )!= 0:
		a_id= a[ 0 ][ 'index' ]
		print 'Assume audio stream at %d index: ' % a_id
	
	vc= vcodec.Decoder( v[ 0 ] )
	if a_id!= -1:
		ac= acodec.Decoder( a[ 0 ] )
	ea= ev= None
	bA= bV= 0
	# Create muxer first
	mx= muxer.Muxer( outFile.split( '.' )[ -1 ] )
	for fr in r:
		if fr[ 0 ]== a_id and not bA:
			d= ac.decode( fr[ 1 ] )
			if d and d.data:
				# --------- Just a test !
				params= ac.getParams()
				# --------- End of test
				
				#params= { 'id': acodec.getCodecID('ac3'),
				#		'bitrate': d.bitrate,
				#		'sample_rate': d.sample_rate,
				#		'channels': d.channels }
				
				# It should be some logic to work with frame rates and such.
				# I'm not aware of what it would be...
				print 'Setting audio codec to ', params
				#ea= acodec.Encoder( params )
				aId= mx.addStream( muxer.CODEC_TYPE_AUDIO, params )
				bA= 1
		
		if fr[ 0 ]== v_id and not bV:
			d= vc.decode( fr[ 1 ] )
			if d and d.data:
				# --------- Just a test !
				params= vc.getParams()
				# --------- End of test
				
				#params[ 'id' ]= vcodec.getCodecID( outCodec )
				# Just try to achive max quality( 2.7 MB/sec mpeg1 and 9.8 for mpeg2 )
				#if outCodec== 'mpeg1video':
				#	params[ 'bitrate' ]= 2700000
				#else:
				#	params[ 'bitrate' ]= 9800000
				# It should be some logic to work with frame rates and such.
				# I'm not aware of what it would be...
				print 'Setting video codec to ', params
				#ev= vcodec.Encoder( params )
				vId= mx.addStream( muxer.CODEC_TYPE_VIDEO, params )
				bV= 1
	
	# Reset decoders
	#if a_id!= -1:
	#	ac= acodec.Decoder( a[ 0 ] )
	#vc= vcodec.Decoder( v[ 0 ] )
	
	# Start muxer			
	fw= open( outFile, 'ab' )
	fw.write( mx.start() )				
	fw.close()
	
	while len( s )> 0:
		for fr in r:
			ss= None
			if fr[ 3 ]> 0:
				#print 'PTS', fr[ 0 ], fr[ 3 ], fr[ 4 ]
				pass
				#if fr[ 3 ]> 3000000:
				#	return
			
			if fr[ 0 ]== a_id:
				# --------- Just a test !
				ss= mx.write( aId, fr[ 1 ], fr[ 3 ], fr[ 4 ] )
				print len(fr[ 1 ]), fr[ 3 ]
				# --------- End of test
				#d= ac.decode( fr[ 1 ] )
				#if d and d.data:
				#	enc_frames= ea.encode( d.data )
				#	if enc_frames:
				#		for efr in enc_frames:
				#			ss= mx.write( aId, efr )
			
			if fr[ 0 ]== v_id:
				# --------- Just a test !
				ss= mx.write( vId, fr[ 1 ], fr[ 3 ], fr[ 4 ] )
				#if ss: fw.write(ss)
				#continue
				# --------- End of test
				#d= vc.decode( fr[ 1 ] )
				#if d and d.data:
				#	efr= ev.encode( d )
				#	if efr and efr.data:
				#		ss= mx.write( vId, efr.data )
			
			if ss:
				fw= open( outFile, 'ab' )
				fw.write( ss )				
				fw.close()
		
		
		s= f.read( 4000 )
		r= dm.parse( s )
	
	if fw and mx:
		ss= mx.end()
		if ss: 
			fw= open( outFile, 'ab' )
			fw.write( ss )				
			fw.close()

#recodeVideo( "c:\\bors\\TellyTopia\\SMIL\\code\\data\\TTOP2595318279758871_122370\\TTOP1206586244906320.ts", "c:\\tt.ts", "mpeg1video" )
#recodeVideo( "c:\\tt.ts", "c:\\tt1.ts", "mpeg1video" )

#recodeVideo( "c:\\bors\\TellyTopia\\SMIL\\code\\data\\demo\\VideoOfOriginalSize\\Mommy 480x360.ts", "c:\\tt.ts", "mpeg1video" )
#recodeVideo( "c:\\bors\\TellyTopia\\SMIL\\code\\data\\demo\\VideoOfOriginalSize\\elvis 416x240.ts", "c:\\tt.ts", "mpeg1video" )

recodeVideo( "c:\\bors\\TellyTopia\\SMIL\\code\\data\\TTOP2595318279758871_122370\\TTOP1206586244906320.ts", "c:\\tt.ts", "mpeg1video" )

#recodeVideo( "c:\\bors\\TellyTopia\\SMIL\\code\\data\\demo1\\TTOP7058272542511953.ts", "c:\\tt1.ts", "mpeg1video" )

#if __name__== '__main__':
#  if len( sys.argv )!= 4:
#    print "Usage: recode_video <in_file> <out_file> <format>\n\tformat= { mpeg1video | mpeg2video }"
#  else:
#    recodeVideo( sys.argv[ 1 ], sys.argv[ 2 ], sys.argv[ 3 ] )


