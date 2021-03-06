# Microsoft Developer Studio Project File - Name="acodec" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=acodec - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "acodec.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "acodec.mak" CFG="acodec - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "acodec - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "acodec - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "acodec - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "acodec\Release"
# PROP BASE Intermediate_Dir "acodec\Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "acodec\Release"
# PROP Intermediate_Dir "acodec\Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "PYMPG_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /Od /I "." /I "../" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "PYMPG_EXPORTS" /D "CONFIG_VORBIS" /D "CONFIG_FAAD" /D "CONFIG_MP3LAME" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 libmp3lame.lib vorbisenc_static.lib /nologo /dll /debug /machine:I386 /out:"acodec/Release/acodec.pyd" /libpath:"../../libvorbis-1.0.1/win32/VorbisEnc_Static_Release" /libpath:"..\..\lame-3.95.1\libmp3lame\Release" /export:initacodec
# SUBTRACT LINK32 /pdb:none
# Begin Special Build Tool
TargetPath=.\acodec\Release\acodec.pyd
SOURCE="$(InputPath)"
PostBuild_Desc=Copying to destination
PostBuild_Cmds=copy $(TargetPath) c:\python24\lib\site-packages\pymedia\audio
# End Special Build Tool

!ELSEIF  "$(CFG)" == "acodec - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "acodec\Debug"
# PROP BASE Intermediate_Dir "acodec\Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "acodec\Debug"
# PROP Intermediate_Dir "acodec\Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "PYMPG_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "." /I "../" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "PYMPG_EXPORTS" /D _WIN32_WINNT=0x400 /D "HAVE_AV_CONFIG_H" /D "CONFIG_VORBIS" /D "CONFIG_FAAD" /D "CONFIG_MP3LAME" /YX /FD /GZ /c
# SUBTRACT CPP /Fr
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 libmp3lame.lib vorbisenc_static_d.lib /nologo /dll /debug /machine:I386 /out:"acodec/Debug/acodec_d.pyd" /pdbtype:sept /libpath:"../../libvorbis-1.0.1/win32/VorbisEnc_Static_Debug" /libpath:"..\..\lame-3.95.1\libmp3lame\Debug" /export:initacodec
# SUBTRACT LINK32 /pdb:none /nodefaultlib
# Begin Special Build Tool
TargetPath=.\acodec\Debug\acodec_d.pyd
SOURCE="$(InputPath)"
PostBuild_Desc=Copying to destination
PostBuild_Cmds=copy $(TargetPath) c:\python23\lib\site-packages\pymedia\audio
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "acodec - Win32 Release"
# Name "acodec - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\acodec\acodec.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
