@echo Running '%FORMAT%' format
@python aplayer.py c:\bors\media\test.%FORMAT% 0 1 2 >NUL
@IF ERRORLEVEL 1 ECHO failed
@IF NOT ERRORLEVEL 1 ECHO passed 