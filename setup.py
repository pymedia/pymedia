#!/usr/bin/env python
#
# This is the distutils setup script for PyMedia.
#
# To configure, compile, install, just run this script.

DESCRIPTION = """PyMedia is a Python/C++ modules to support 
media files in a different formats and allow you to build your own 
modules to work with a numerous types of files. """

METADATA = {
    "name":             "pymedia",
    "version":          "1.0",
    "license":          "LGPL",
    "url":              "http://www.pymedia.org",
    "author":           "Dmitry Borisov",
    "author_email":     "jbors@users.sourceforge.org",
    "description":      "Python Media Development Library",
    "long_description": DESCRIPTION,
}

try:
    import distutils
except ImportError:
    raise SystemExit, "PyMedia requires distutils to build and install."

try:
    import pygame
except ImportError:
    raise SystemExit, "PyMedia requires pygame to be installed."


#get us to the correct directory
import os, sys
path = os.path.split(os.path.abspath(sys.argv[0]))[0]
os.chdir(path)

import os.path, glob
import distutils.sysconfig
from distutils.core import setup, Extension
from distutils.extension import read_setup_file
from distutils.ccompiler import new_compiler
from distutils.command.install_data import install_data

#extra files to install
data_path = os.path.join(distutils.sysconfig.get_python_lib(), 'pymedia')

data_files= []

#add non .py files in audio directory
for f in glob.glob(os.path.join('audio', '*')):
    if f[-3:] =='pyd' and os.path.isfile(f):
        data_files.append(f)

#data installer with improved intelligence over distutils
#data files are copied into the project directory instead
#of willy-nilly
class smart_install_data(install_data):
    def run(self):
        #need to change self.install_dir to the actual library dir
        install_cmd = self.get_finalized_command('install')
        self.install_dir = getattr(install_cmd, 'install_lib')
        return install_data.run(self)


#finally,
#call distutils with all needed info
PACKAGEDATA = {
       "cmdclass":    {'install_data': smart_install_data},
       "packages":    ['pymedia.audio', 'pymedia.menu', 'pymedia'],
       "package_dir": {'pymedia.audio': 'audio', 'pymedia.menu': 'menu', 'pymedia': '.' },
       "headers":     None,
       "ext_modules": [],
       "data_files":  [['pymedia/audio', data_files], [ 'pymedia', ['pypower.pyd','README']]]
}
PACKAGEDATA.update(METADATA)
apply(setup, [], PACKAGEDATA)
 