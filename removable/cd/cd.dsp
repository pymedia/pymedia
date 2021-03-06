# Microsoft Developer Studio Project File - Name="cd" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=cd - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "cd.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "cd.mak" CFG="cd - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "cd - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "cd - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "cd - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "PYCDDA_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "HAVE_DVDCSS_DVDCSS_H" /D "UDF_CACHE" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib /nologo /dll /machine:I386 /out:"Release/cd.pyd" /export:initcd
# SUBTRACT LINK32 /pdb:none
# Begin Special Build Tool
TargetPath=.\Release\cd.pyd
SOURCE="$(InputPath)"
PostBuild_Desc=Copying to destination
PostBuild_Cmds=copy $(TargetPath) c:\python23\lib\site-packages\pymedia\removable
# End Special Build Tool

!ELSEIF  "$(CFG)" == "cd - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "PYCDDA_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "PYCDDA_EXPORTS" /D "HAVE_DVDCSS_DVDCSS_H" /D "UDF_CACHE" /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib /nologo /dll /debug /machine:I386 /out:"Debug/cd_d.pyd" /pdbtype:sept /export:initcd
# SUBTRACT LINK32 /pdb:none
# Begin Special Build Tool
TargetPath=.\Debug\cd_d.pyd
SOURCE="$(InputPath)"
PostBuild_Desc=Copying to destination
PostBuild_Cmds=copy $(TargetPath) c:\python23\lib\site-packages\pymedia\removable
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "cd - Win32 Release"
# Name "cd - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Group "libdvdread"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\dvdlibs\dvdread\dvd_input.c
# End Source File
# Begin Source File

SOURCE=.\dvdlibs\dvdread\dvd_reader.c
# End Source File
# Begin Source File

SOURCE=.\dvdlibs\dvdread\dvd_udf.c
# End Source File
# Begin Source File

SOURCE=.\dvdlibs\dvdread\ifo_read.c
# End Source File
# Begin Source File

SOURCE=.\dvdlibs\dvdread\md5.c
# End Source File
# Begin Source File

SOURCE=.\dvdlibs\dvdread\nav_read.c
# End Source File
# End Group
# Begin Group "libdvdcss"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\dvdlibs\dvdcss\css.c
# End Source File
# Begin Source File

SOURCE=.\dvdlibs\dvdcss\device.c
# End Source File
# Begin Source File

SOURCE=.\dvdlibs\dvdcss\error.c
# End Source File
# Begin Source File

SOURCE=.\dvdlibs\dvdcss\ioctl.c
# End Source File
# Begin Source File

SOURCE=.\dvdlibs\dvdcss\libdvdcss.c
# End Source File
# End Group
# Begin Source File

SOURCE=.\audiocd.cpp
# End Source File
# Begin Source File

SOURCE=.\cd.cpp
# End Source File
# Begin Source File

SOURCE=.\dvdcd.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\cdcommon_win.h
# End Source File
# Begin Source File

SOURCE=.\cdda.h
# End Source File
# Begin Source File

SOURCE=.\cdda_win.h
# End Source File
# Begin Source File

SOURCE=.\cdgeneric.h
# End Source File
# Begin Source File

SOURCE=.\dvd.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=..\..\..\..\..\..\favicon.ico
# End Source File
# End Group
# End Target
# End Project
