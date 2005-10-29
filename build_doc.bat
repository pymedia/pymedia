@cd docs
@rm *.html
@python c:\python23\lib\pydoc.py -w pymedia pymedia.audio pymedia.audio.sound pymedia.removable.cd pymedia.removable pymedia.audio.acodec pymedia.video pymedia.muxer pymedia.video.vcodec
@ren pymedia.html index.html
@cd ..