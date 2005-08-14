@echo Running %VIDEOFILE%
@python vplayer.py %VIDEOFILE% 2 >NUL
@IF ERRORLEVEL 1 ECHO failed
@IF NOT ERRORLEVEL 1 ECHO passed 