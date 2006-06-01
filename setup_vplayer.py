from distutils.core import setup
import py2exe
import glob

METADATA = {
    "name":             "VideoPlayer",
    "version":          "1.3.5",
    "license":          "LGPL",
    "url":              "http://www.indashpc.org",
    "author":           "Dmitry Borisov",
    "author_email":     "dborisov@indashpc.org",
    "description":      "Media player for car",
    "long_description": "",
}

setup(name="vplayer",console=[ 'examples\\vplayer.py' ] )


