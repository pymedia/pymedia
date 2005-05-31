#!/bin/env python

import sys, config
from distutils.core import setup, Extension

def disable_fPIC():
  """Disable -fPIC in the c compiler command line.

  Some unix setups need -fPIC turned off to successfully compile
  video/libavcodec/i386/dsputil_mmx.c (and other files).  Distutils
  gets the flag by parsing python2.3/config/Makefile and storing the
  result in a cache.

  This function is a hack that depends on that internal distutils
  behavior, so it may someday stop working or corrupt distutils in an
  unexpected way."""

  import distutils.sysconfig
  distutils.sysconfig.get_config_vars() # parse Makefile and fill the _config_vars cache
  cv = distutils.sysconfig._config_vars
  for var in "CCSHARED","GCCSHARED","CFLAGS","OPT":
    if var in cv:
      cv[var] = cv[var].replace('-fPIC','')
  for var in ("CXX",):
    if var in cv:
      cv[var] = cv[var].replace('gcc','g++')

MODULE_NAME= 'pymedia'

CLE266_HW= {
	'video.ext_codecs.cle266':
	{
		'#dir': 'video',
		'':
		(
			'mem.c',
			'common.c'
		),
		'vcodec':
		[
			'hw_vcodec.c',
			'hw_mpeg.c',
			'cle266/cle266_hw_dec.c',
		],
	},
}

if sys.platform == 'win32':
		print 'No external codecs at this time exists for windows\n'
else:
		print 'Using UNIX configuration...\n'
		disable_fPIC()
		LIB_DIRS = [ '/usr/lib', '/usr/local/lib', ]
		INC_DIRS= [ '/usr/include', '/usr/local/include', '.' ]
		LIBS= []
		DEFINES= [
			('_FILE_OFFSET_BITS',64),
			('ACCEL_DETECT',1),	
			('HAVE_MMX', '1' ),
      ('HAVE_AV_CONFIG_H', None ),
		] 
		FILES= CLE266_HW

METADATA = {
		"name":             "pymedia-codecs",
		"version":          "1.2.3.0",
		"license":          "LGPL",
		"url":              "http://pymedia.sourceforge.net/",
		"author":           "Dmitry Borisov",
		"author_email":     "jbors@pymedia.org",
		"description":      "External video codecs for PyMedia"
}

PACKAGEDATA = {
				"packages":    ['pymedia.video.ext_codecs'],
				"package_dir": 
					{'pymedia.video.ext_codecs': 'inst_lib/video/ext_codecs'},
				"ext_modules": config.extensions( MODULE_NAME, FILES, INC_DIRS, LIB_DIRS, DEFINES, LIBS )
}

PACKAGEDATA.update(METADATA)
apply(setup, [], PACKAGEDATA)
