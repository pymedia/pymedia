# Microsoft Developer Studio Project File - Name="aformat_lib" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=aformat_lib - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "aformat_lib.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "aformat_lib.mak" CFG="aformat_lib - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "aformat_lib - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "aformat_lib - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "aformat_lib - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "libavformat\Release"
# PROP Intermediate_Dir "libavformat\Release"
# PROP Target_Dir ""
LINK32=link.exe
MTL=midl.exe
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /GB /MD /GX /O2 /I "." /I "libvorbis-1.0/include" /I "libogg-1.0/include" /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /D "HAVE_AV_CONFIG_H" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "aformat_lib - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "libavformat\Debug"
# PROP Intermediate_Dir "libavformat\Debug"
# PROP Target_Dir ""
LINK32=link.exe
MTL=midl.exe
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "." /I "libvorbis-1.0/include" /I "libogg-1.0/include" /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /D "HAVE_AV_CONFIG_H" /YX /FD /GZ /c
# SUBTRACT CPP /X
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ENDIF 

# Begin Target

# Name "aformat_lib - Win32 Release"
# Name "aformat_lib - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=libavformat\asf.c
# End Source File
# Begin Source File

SOURCE=.\libavformat\avienc.c
# End Source File
# Begin Source File

SOURCE=libavformat\aviobuf.c
# End Source File
# Begin Source File

SOURCE=.\mem.c
# End Source File
# Begin Source File

SOURCE=libavformat\ogg.c
# End Source File
# Begin Source File

SOURCE=libavformat\raw.c
# End Source File
# Begin Source File

SOURCE=libavformat\utils.c
# End Source File
# Begin Source File

SOURCE=libavformat\wav.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=libavformat\avformat.h
# End Source File
# Begin Source File

SOURCE=libavformat\avi.h
# End Source File
# Begin Source File

SOURCE=libavformat\avio.h
# End Source File
# Begin Source File

SOURCE=libavformat\strptime.h
# End Source File
# End Group
# End Target
# End Project
