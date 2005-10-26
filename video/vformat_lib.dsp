# Microsoft Developer Studio Project File - Name="vformat_lib" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=vformat_lib - Win32 Release_Static
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "vformat_lib.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "vformat_lib.mak" CFG="vformat_lib - Win32 Release_Static"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "vformat_lib - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "vformat_lib - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE "vformat_lib - Win32 Debug_Static" (based on "Win32 (x86) Static Library")
!MESSAGE "vformat_lib - Win32 Release_Static" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "vformat_lib - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "libavformat/Release"
# PROP Intermediate_Dir "libavformat/Release"
# PROP Target_Dir ""
LINK32=link.exe
MTL=midl.exe
F90=df.exe
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "." /I "../../libvorbis-1.0.1/include" /I "../../libogg-1.1/include" /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /D "HAVE_AV_CONFIG_H" /D "CONFIG_VORBIS" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "vformat_lib - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "libavformat/Debug"
# PROP Intermediate_Dir "libavformat/Debug"
# PROP Target_Dir ""
LINK32=link.exe
MTL=midl.exe
F90=df.exe
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "." /I "../../libvorbis-1.0.1/include" /I "../../libogg-1.1/include" /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /D "HAVE_AV_CONFIG_H" /D "CONFIG_VORBIS" /YX /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "vformat_lib - Win32 Debug_Static"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "vformat_lib___Win32_Debug_Static"
# PROP BASE Intermediate_Dir "vformat_lib___Win32_Debug_Static"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "libavformat/Debug_Static"
# PROP Intermediate_Dir "libavformat/Debug_Static"
# PROP Target_Dir ""
LINK32=link.exe
MTL=midl.exe
F90=df.exe
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "." /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /D "HAVE_AV_CONFIG_H" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "." /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /D "HAVE_AV_CONFIG_H" /YX /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "vformat_lib - Win32 Release_Static"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "vformat_lib___Win32_Release_Static"
# PROP BASE Intermediate_Dir "vformat_lib___Win32_Release_Static"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "libavformat/Release_Static"
# PROP Intermediate_Dir "libavformat/Release_Static"
# PROP Target_Dir ""
LINK32=link.exe
MTL=midl.exe
F90=df.exe
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "." /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /D "HAVE_AV_CONFIG_H" /YX /FD /GZ /c
# ADD CPP /nologo /ML /W3 /GX /O2 /Ob2 /I "." /D "WIN32" /D "_NDEBUG" /D "_MBCS" /D "_LIB" /D "HAVE_AV_CONFIG_H" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ENDIF 

# Begin Target

# Name "vformat_lib - Win32 Release"
# Name "vformat_lib - Win32 Debug"
# Name "vformat_lib - Win32 Debug_Static"
# Name "vformat_lib - Win32 Release_Static"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=libavformat\asf.c
# End Source File
# Begin Source File

SOURCE=libavformat\avidec.c
# End Source File
# Begin Source File

SOURCE=libavformat\avienc.c
# End Source File
# Begin Source File

SOURCE=libavformat\aviobuf.c
# End Source File
# Begin Source File

SOURCE=.\common.c
# End Source File
# Begin Source File

SOURCE=libavformat\cutils.c
# End Source File
# Begin Source File

SOURCE=.\libavcodec\integer.c
# End Source File
# Begin Source File

SOURCE=.\mem.c
# End Source File
# Begin Source File

SOURCE=libavformat\mov.c
# End Source File
# Begin Source File

SOURCE=libavformat\mpeg.c
# End Source File
# Begin Source File

SOURCE=libavformat\mpegts.c
# End Source File
# Begin Source File

SOURCE=.\libavformat\ogg.c
# End Source File
# Begin Source File

SOURCE=.\libavformat\raw.c
# End Source File
# Begin Source File

SOURCE=libavformat\utils.c
# End Source File
# Begin Source File

SOURCE=.\libavformat\wav.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# End Group
# End Target
# End Project
