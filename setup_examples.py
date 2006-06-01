from distutils.core import setup
import py2exe
import glob

METADATA = {
    "name":             "Pymedia Examples",
    "version":          "1.3.5",
    "license":          "LGPL",
    "url":              "http://www.indashpc.org",
    "author":           "Dmitry Borisov",
    "author_email":     "dborisov@indashpc.org",
    "description":      "Pymedia examples for Windows",
    "long_description": "",
}

setup(name="examples-pymedia-1.3.7-exe", \
	console=[ \
		'examples\\dump_wav.py', \
		'examples\\voice_recorder_player.py', \
		'examples\\video_bench_ovl.py', \
		'examples\\video_bench.py', \
		'examples\\player.py', \
		'examples\\dump_video.py', \
		'examples\\vplayer.py', \
		'examples\\recode_audio.py', \
		'examples\\recode_video.py', \
		'examples\\demux_video.py', \
		'examples\\encode_video.py', \
		'examples\\demux_audio_video.py', \
		'examples\\demux_bench.py', \
		'examples\\play_cdda_track.py', \
		'examples\\read_cdda_track.py', \
		'examples\\make_video.py', \
		'examples\\voice_recorder.py', \
		'examples\\play_wav.py', \
		] )


