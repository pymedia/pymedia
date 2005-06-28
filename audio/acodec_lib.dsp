# Microsoft Developer Studio Project File - Name="acodec_lib" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=acodec_lib - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "acodec_lib.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "acodec_lib.mak" CFG="acodec_lib - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "acodec_lib - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "acodec_lib - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "acodec_lib - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "libavcodec\Release"
# PROP Intermediate_Dir "libavcodec\Release"
# PROP Target_Dir ""
F90=df.exe
MTL=midl.exe
LINK32=link.exe
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /MD /GX /O2 /I "." /I "../../libvorbis-1.0.1/include" /I "../../libogg-1.1/include" /I "../../libfaad2/include" /I "../../lame-3.95.1/include" /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /D "HAVE_AV_CONFIG_H" /D "CONFIG_VORBIS" /D "CONFIG_FAAD" /D "CONFIG_MP3LAME" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "acodec_lib - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "libavcodec\Debug"
# PROP Intermediate_Dir "libavcodec\Debug"
# PROP Target_Dir ""
F90=df.exe
MTL=midl.exe
LINK32=link.exe
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /Gm /GX /ZI /Od /I "." /I "../../libvorbis-1.0.1/include" /I "../../libogg-1.1/include" /I "../../libfaad2/include" /I "../../lame-3.95.1/include" /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /D "HAVE_AV_CONFIG_H" /D "CONFIG_VORBIS" /D "CONFIG_FAAD" /D "CONFIG_MP3LAME" /YX /FD /GZ /c
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

# Name "acodec_lib - Win32 Release"
# Name "acodec_lib - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Group "liba52"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\libavcodec\liba52\bit_allocate.c
# End Source File
# Begin Source File

SOURCE=.\libavcodec\liba52\bitstream.c
# End Source File
# Begin Source File

SOURCE=.\libavcodec\liba52\downmix.c
# End Source File
# Begin Source File

SOURCE=.\libavcodec\liba52\imdct.c
# End Source File
# Begin Source File

SOURCE=.\libavcodec\liba52\parse.c
# End Source File
# End Group
# Begin Source File

SOURCE=.\libavcodec\a52dec.c
# End Source File
# Begin Source File

SOURCE=.\libavcodec\ac3enc.c
# End Source File
# Begin Source File

SOURCE=.\libavcodec\common.c
# End Source File
# Begin Source File

SOURCE=.\libavcodec\faad.c
# End Source File
# Begin Source File

SOURCE=.\libavcodec\fft.c
# End Source File
# Begin Source File

SOURCE=.\libavcodec\flac.c
# End Source File
# Begin Source File

SOURCE=.\libavcodec\mdct.c
# End Source File
# Begin Source File

SOURCE=.\mem.c
# End Source File
# Begin Source File

SOURCE=.\libavcodec\mp3lameaudio.c
# End Source File
# Begin Source File

SOURCE=.\libavcodec\mpegaudio.c
# End Source File
# Begin Source File

SOURCE=.\libavcodec\mpegaudiodec.c
# End Source File
# Begin Source File

SOURCE=.\libavcodec\oggvorbis.c
# End Source File
# Begin Source File

SOURCE=.\libavcodec\pcm.c
# End Source File
# Begin Source File

SOURCE=.\libavcodec\utils.c
# End Source File
# Begin Source File

SOURCE=.\libavcodec\wmadec.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# End Group
# End Target
# End Project
