#! /bin/env python

import sys, time, traceback

########################################################################3
# Simple  audio encoder 
def recodeAudio( fName, fOutput, type, bitrate= None ):
	# ------------------------------------
	import pymedia.audio.acodec as acodec
	
	ac= acodec.Decoder( fName.split( '.' )[ -1 ].lower() ) 
	print 'Recoding %s into %s' % ( fName, fOutput )
	f= open( fName, 'rb' )
	s= f.read( 90000 )
	fr= ac.decode( s ) 
	print "Decoder params:", ac.getParams()
	fw= ac1= None
	while len(s):
		if ac1== None:
			if bitrate== None:
				bitrate= fr.bitrate
			params= { 'id': acodec.getCodecID(type),
								'bitrate': bitrate,
								'sample_rate': fr.sample_rate,
								'channels': fr.channels }
			print "Encoder params:", params
			ac1= acodec.Encoder( params ) #or any other supported audio encoder
			fw= open(fOutput, 'wb')
		
		e_frames = ac1.encode( fr.data )
		if e_frames:
			for str in e_frames:
				fw.write(str)
		
		s= f.read( 100000 )
		fr= ac.decode( s )
	
	if fw: fw.close()
	f.close()


#psyco.full()
# Test all modules
if __name__== '__main__':
  if len( sys.argv )< 4 or len( sys.argv )> 5:
    print "Usage: recode_audio.py <audio_input_file> <audio_output_file> <format_name> [ <bitrate> ]"
  else:
    if len( sys.argv )== 4:
      recodeAudio( sys.argv[1], sys.argv[2], sys.argv[3] )
    else:
      recodeAudio( sys.argv[1], sys.argv[2], sys.argv[3], int( sys.argv[4] )* 1000 )


