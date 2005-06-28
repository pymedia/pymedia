#! /bin/env python

import sys, time, traceback

########################################################################3
# Simple  audio encoder 
def recodeAudio( fName, fOutput, type, bitrate= None ):
  # ------------------------------------
  import pymedia.audio.acodec as acodec
  import pymedia.muxer as muxer
  # Open demuxer
  dm= muxer.Demuxer( fName.split( '.' )[ -1 ].lower() )
  f= open( fName, 'rb' )
  s= f.read( 90000 )
  dec= enc= mx= None
  print 'Recoding %s into %s' % ( fName, fOutput )
  while len( s ):
    frames= dm.parse( s )
    if frames:
      for fr in frames:
        # Assume for now only audio streams
        if dec== None:
          # Open decoder
          dec= acodec.Decoder( dm.streams[ fr[ 0 ] ] )
          print 'Decoder params:', dm.streams[ fr[ 0 ] ]
        
        # Decode audio frame
        r= dec.decode( fr[ 1 ] )
        if r:
          if bitrate== None:
            bitrate= r.bitrate
          
          # Open muxer and encoder
          if enc== None:
            params= { 'id': acodec.getCodecID(type),
                      'bitrate': bitrate,
                      'sample_rate': r.sample_rate,
                      'channels': r.channels }
            print 'Encoder params:', params
            mx= muxer.Muxer( type )
            stId= mx.addStream( muxer.CODEC_TYPE_AUDIO, params )
            enc= acodec.Encoder( params )
            fw= open(fOutput, 'wb')
            ss= mx.start()
            fw.write(ss)
        
          enc_frames= enc.encode( r.data )
          if enc_frames:
            for efr in enc_frames:
              ss= mx.write( stId, efr )
              if ss:
                fw.write(ss)
    
    s= f.read( 100000 )
  
  f.close()
  
  if fw:
    if mx:
      ss= mx.end()
      if ss:
        fw.write(ss)
    fw.close()

# ----------------------------------------------------------------------------------
# Change the format of your compressed audio files to something different
# http://pymedia.org/
if __name__== '__main__':
  if len( sys.argv )< 4 or len( sys.argv )> 5:
    print "Usage: recode_audio.py <audio_input_file> <audio_output_file> <format_name> [ <bitrate> ]"
  else:
    if len( sys.argv )== 4:
      recodeAudio( sys.argv[1], sys.argv[2], sys.argv[3] )
    else:
      recodeAudio( sys.argv[1], sys.argv[2], sys.argv[3], int( sys.argv[4] )* 1000 )


