
import traceback, os

def demuxer_test( name ):
  import pymedia.muxer as muxer
  try:
    dm= muxer.Demuxer( str.split( name, '.' )[ -1 ].lower() )
    f= open( name, 'rb' )
    s= f.read( 300000 )
    r= dm.parse( s )
    print dm.streams
    if dm.hasHeader():
      print dm.getInfo()
    print 'Passed'
  except:
    traceback.print_exc()
    print '!!!!!!!!!!!!!!!!!!!!!Failed'

PATH= 'c:\\bors\\media'
files= [ 
'test.flac',
'test.aac',
'test.avi',
'test.mp2',
'test.mp3',
'test.mp4',
'test.mpg',
'test.ogg',
'test.vob',
#'test.wav',
'test.wma',
'test.wmv',
'test1.mpg',
'test1.wmv',
]

for f in files:
  name= os.path.join( PATH, f )
  print 'Demuxing file with %s extension' % str.split( name, '.' )[ -1 ].lower()
  demuxer_test( name )
