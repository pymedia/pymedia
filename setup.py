#!/bin/env python

import sys, config
from distutils.core import setup, Extension

MODULE_NAME= 'pymedia'
MMX_FILES= [
			'i386/cputest.c',
			'i386/dsputil_mmx.c',
			'i386/fdct_mmx.c',
			'i386/idct_mmx.c',
			'i386/motion_est_mmx.c',
			'i386/mpegvideo_mmx.c',
			'i386/simple_idct_mmx.c',
]

NONMMX_FILES= [
			'fdctref.c',
			'fft.c',
			'mdct.c',
]

FILES={
	'audio.acodec':
	{
		'#dir': 'audio',
		'':
		(
			'acodec/acodec.c',
			'mem.c'
		),
		'libavformat':
		(
			'utils.c',
			'raw.c',
			'avienc.c',
			'asf.c',
			'aviobuf.c',
			'ogg.c',
			'wav.c',
		),
		'libavcodec':
		(
			'utils.c',
			'wmadec.c',
			'a52dec.c',
			'liba52/bit_allocate.c',
			'liba52/bitstream.c',
			'liba52/downmix.c',
			'liba52/imdct.c',
			'liba52/parse.c',
			'liba52/crc.c',
			'faad.c',
			'oggvorbis.c',
			'common.c',
			'mpegaudiodec.c',
			'fft.c',
			'mdct.c',
			'mpegaudio.c',
			'ac3enc.c',
			'mp3lameaudio.c',
			'resample.c',

		),
	},
	'video.muxer':
	{
		'#dir': 'video',
		'':
		(
			'muxer/demuxer.cpp',
			'muxer/muxer.cpp',
			'mem.c',
			'common.c'
		),
		'libavformat':
		(
			'cutils.c',
			'utils.c',
			'mov.c',
			'avienc.c',
			'avidec.c',
			'asf.c',
			'aviobuf.c',
			'mpegts.c',
			'mpeg.c',
		),
	},
	'video.vcodec':
	{
		'#dir': 'video',
		'':
		(
			'vcodec/vcodec.c',
			'mem.c',
			'common.c'
		),
		'libavcodec':
		[
			'cabac.c',
			'error_resilience.c',
			'eval.c',
			'golomb.c',
			'h263.c',
			'h263dec.c',
			'h264.c',
			'imgconvert.c',
			'imgresample.c',
			'jfdctfst.c',
			'jfdctint.c',
			'jrevdct.c',
			'mjpeg.c',
			'mpeg12.c',
			'msmpeg4.c',
			'rv10.c',
			'simple_idct.c',
			'svq1.c',
			'utils.c',
			'xvmcvideo.c',
			'mpegvideo.c',
			'dsputil.c',
			'motion_est.c',
			'ratecontrol.c',
			'opts.c',
		],
	},
	'audio.cd':
	{
		'#dir': 'cd',
		'': ( 'cd.cpp', )
	},
	'audio.sound':
	{
		'#dir': 'sound',
		'': ( 'sound.cpp', )
	}
}

try:
  f= open( 'build.dat' )
  buildNum= f.readlines()[ 0 ]
  f.close()
except:
  buildNum= 1

f= open( 'build.dat', 'wt' )
f.write( str( int( buildNum )+ 1 ) )
f.close()

DEFINES= [( 'BUILD_NUM', buildNum ),]

if sys.platform == 'win32':
    print 'Using WINDOWS configuration...\n'
    dep= config.Dependency_win
    inc_hunt = ['include']
    lib_hunt = [
    	'win32\\Static_Release',
    	'win32\\VorbisEnc_Static_Release',
    	'win32\\Vorbis_Static_Release',
    	'libmp3lame\\Release',
			'libfaad\\Release',
			'VisualC\\SDL\Release']
    DEFINES.append( ('WIN32', None ) )
    LIBS= [ 'winmm' ]
    FILES[ 'video.vcodec' ][ 'libavcodec' ]+= NONMMX_FILES
else:
    print 'Using UNIX configuration...\n'
    dep= config.Dependency_unix
    inc_hunt = [ 
    	'/usr/include', 
    	'/usr/local/include', 
    	'/usr/local/include/lame',]
    lib_hunt = [ '/usr/lib', '/usr/local/lib', ]
    LIBS= []
    DEFINES+= [
    	( 'PATH_DEV_DSP', '"/dev/dsp"' ), 
    	( 'PATH_DEV_MIXER','"/dev/mixer"' ), 
    	('_FILE_OFFSET_BITS',64),
    	('ACCEL_DETECT',1),	
    	('HAVE_MMX', '1' ),] 
    FILES[ 'video.vcodec' ][ 'libavcodec' ]+= MMX_FILES

DEPS = [
    dep('OGG', 'libogg-[1-9].*', 'ogg/ogg.h', 'libogg', 'CONFIG_VORBIS').configure(inc_hunt,lib_hunt),
    dep('VORBIS', 'libvorbis-[1-9].*', 'vorbis/codec.h', 'libvorbis', 'CONFIG_VORBIS' ).configure(inc_hunt,lib_hunt),
    dep('FAAD', 'libfaad2', 'faad.h', 'libfaad', 'CONFIG_FAAD').configure(inc_hunt,lib_hunt),
    dep('MP3LAME', 'lame-3.95.*', 'lame.h', 'libmp3lame', 'CONFIG_MP3LAME').configure(inc_hunt,lib_hunt),
    dep('VORBISENC', 'libvorbis-[1-9].*','vorbis/vorbisenc.h','libvorbisenc', 'CONFIG_VORBIS').configure(inc_hunt,lib_hunt),
]

DEPS= filter( lambda x: x.found, DEPS )
INC_DIRS= [ x.inc_dir for x in DEPS ]
LIB_DIRS= [ x.lib_dir for x in DEPS ]
DEFINES+= [ ( x.define, None ) for x in DEPS ]+ [ ( 'HAVE_AV_CONFIG_H', None ) ]
LIBS+= [ x.lib for x in DEPS ]

choice = raw_input('Continue building '+MODULE_NAME+' ? [Y,n]:')
if choice== 'n':
    print 'To start installation please run: \n\tsetup.py install and press Enter when prompted\n'
    sys.exit()

METADATA = {
    "name":             "pymedia",
    "version":          "1.2.0",
    "license":          "LGPL",
    "url":              "http://pymedia.sourceforge.net/",
    "author":           "Dmitry Borisov",
    "author_email":     "jbors@users.sourceforge.net",
    "description":      "Pymedia library for mutlimedia easy experience"
}

PACKAGEDATA = {
        "packages":    ['pymedia','pymedia.audio','pymedia.video'],
        "package_dir": 
        	{'pymedia': 'inst_lib','pymedia.audio': 'inst_lib/audio','pymedia.video': 'inst_lib/video'},
        "ext_modules": config.extensions( MODULE_NAME, FILES, INC_DIRS, LIB_DIRS, DEFINES, LIBS )
}

PACKAGEDATA.update(METADATA)
apply(setup, [], PACKAGEDATA)
