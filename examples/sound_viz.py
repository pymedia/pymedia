import math, sys, thread, time
import pymedia.audio.sound as sound
import pymedia.audio.acodec as acodec
import psyco
SAMPLES= 500
BANDS= 20
NUM_FREQS= 256
# Fade out interval for frequency bars
FADE_OUT= 1
FREQ_GAP= 2
RES= (200,200)

# ----------------------------------------------------
def aPlayer( name ):
  global sampleFreqs, exit, snd
  # Open analyzer for a mono signal
  analyzer= sound.SpectrAnalyzer( 1, SAMPLES, NUM_FREQS )
  resampler= None
  
  dec= acodec.Decoder( str.split( name, '.' )[ -1 ].lower() )
  f= open( name, 'rb' )
  snd= None
  s= f.read( 20000 )
  while len( s ) and exit== 0:
    r= dec.decode( s )
    if snd== None:
      print 'Opening sound with sample rate %d and channels %d' % ( r.sample_rate, r.channels )
      snd= sound.Output( r.sample_rate, r.channels, sound.AFMT_S16_LE )
      resampler= sound.Resampler( (r.sample_rate,r.channels), (r.sample_rate,1) )
    
    if len( r.data ):
      s1= resampler.resample( r.data )
      fTmp= analyzer.asBands( BANDS, s1 )
      sampleFreqs.append( ( snd.getPosition()+ snd.getLeft(), fTmp ) )
      snd.play( r.data )
      pp= snd.getPosition()
      x= 0
      while x< len( sampleFreqs ) and pp> sampleFreqs[ x ][ 0 ]:
        x+= 1
      
      for i in range( 0, x- 1 ):
        del( sampleFreqs[ i ] )
      
    s= f.read( 512 )
  
  # Wait until complete all sounds
  while snd.isPlaying():
    time.sleep( .05 )
  
  exit= 1

#aPlayer( 'c:\\music\\Test\\Roots.mp3' )

import pygame 

def loadBar():
  global bar
  line= pygame.image.load( 'line.png' )
  w,h= pygame.display.get_surface().get_size()
  l= w/ BANDS- FREQ_GAP
  bar= pygame.Surface( (l,h) )
  for i in xrange( l ):
    bar.blit( line, (i,0) )

# ----------------------------------------------------
def showBars( pos, freqs ):
  #print [ '%.2f' % x for x in freqs ]
  global lastFreqs, bar
  freqs= [ x[ 1 ] for x in freqs ]
  if lastFreqs== None:
    lastFreqs= ( pos, freqs )
  s= pygame.display.get_surface()
  s.fill( (0,0,0) )
  w,h= s.get_size()
  sc= float(h)/ 600
  l= w/ len( freqs )- FREQ_GAP
  for i in xrange( len( freqs )):
    f= freqs[ i ]* sc
    if f> h:
      f= h
    if f< lastFreqs[ 1 ][ i ]:
      # Do the fade out
      fade= ( h/ FADE_OUT )* ( pos- lastFreqs[ 0 ] )
      if ( lastFreqs[ 1 ][ i ]- f )> fade:
        f= lastFreqs[ 1 ][ i ]- fade
    h1= f
    x= i*( l+ FREQ_GAP )
    s.blit( bar, ( x, h- f ), ( 0,h- f,l,f ) )
    freqs[ i ]= f
  
  lastFreqs= ( pos, freqs )

# ----------------------------------------------------
if __name__ == "__main__":
  if len( sys.argv )!= 2:
      print 'Usage: sound_viz <file_name>'
  else:
    global sampleFreqs, exit, snd, lastFreqs
    psyco.full()
    exit= 0
    sampleFreqs= []
    lastFreqs= None
    snd= None
    vt= thread.start_new_thread( aPlayer, ( sys.argv[ 1 ], ))
    pygame.init()
    pygame.display.set_mode( RES, 0 )
    # Load vertical line and create bar
    loadBar()
    while exit== 0:
      time.sleep( .04 )
      if len( sampleFreqs ):
        # Get data for analyzer
        x= 0
        pp= snd.getPosition()+.3
        while x< len( sampleFreqs ) and pp>= sampleFreqs[ x ][ 0 ]:
          x+= 1
        
        # Find the index within the samples
        if x> len( sampleFreqs )- 1:
          continue 
        d, dd= sampleFreqs[ x- 1 ], sampleFreqs[ x ]
        ind= int( ( pp- d[ 0 ] )* snd.getRate()/ SAMPLES )
        #print ind, len( d[ 1 ] ), d[ 0 ], dd[ 0 ], pp
        if ind>= 0:
          # Ugly hack...
          ind= min( ( ind, len( d[ 1 ] )- 1 ) )
          #print x1, pos- x1, snd.getRate()/ ( pos- x1 )
          ff= d[ 1 ][ ind ]
          showBars( pp, ff )
          pygame.display.update()
      
      ev= pygame.event.get() 
      for e in ev:
        if e.type== pygame.KEYDOWN: 
          if e.key== pygame.K_ESCAPE: 
            exit= 1
