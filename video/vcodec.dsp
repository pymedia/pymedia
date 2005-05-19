# Microsoft Developer Studio Project File - Name="vcodec" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102
# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=vcodec - Win32 Release_Static
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "vcodec.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "vcodec.mak" CFG="vcodec - Win32 Release_Static"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "vcodec - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "vcodec - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "vcodec - Win32 Debug_Static" (based on "Win32 (x86) Static Library")
!MESSAGE "vcodec - Win32 Release_Static" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""

!IF  "$(CFG)" == "vcodec - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "vcodec/Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "vcodec/Release"
# PROP Intermediate_Dir "vcodec/Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "VCODEC_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "." /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "VCODEC_EXPORTS" /D "HAVE_MMX" /YX /FD /c
MTL=midl.exe
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 /nologo /dll /machine:I386 /out:"vcodec/Release/vcodec.pyd" /export:initvcodec
# SUBTRACT LINK32 /pdb:none
# Begin Special Build Tool
TargetPath=.\vcodec\Release\vcodec.pyd
SOURCE="$(InputPath)"
PostBuild_Desc=Copying to destination
PostBuild_Cmds=copy $(TargetPath) c:\python23\lib\site-packages\pymedia\video
# End Special Build Tool

!ELSEIF  "$(CFG)" == "vcodec - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "vcodec/Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "vcodec/Debug"
# PROP Intermediate_Dir "vcodec/Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "VCODEC_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "." /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "HAVE_MMX" /YX /FD /GZ /c
MTL=midl.exe
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 /nologo /dll /debug /machine:I386 /out:"vcodec/Debug/vcodec_d.pyd" /pdbtype:sept /export:initvcodec
# SUBTRACT LINK32 /pdb:none
# Begin Special Build Tool
TargetPath=.\vcodec\Debug\vcodec_d.pyd
SOURCE="$(InputPath)"
PostBuild_Desc=Copying to destination
PostBuild_Cmds=copy $(TargetPath) c:\python23\lib\site-packages\pymedia\video
# End Special Build Tool

!ELSEIF  "$(CFG)" == "vcodec - Win32 Debug_Static"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "vcodec/Debug_Static"
# PROP BASE Intermediate_Dir "vcodec/Debug_Static"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "vcodec/Debug_Static"
# PROP Intermediate_Dir "vcodec/Debug_Static"
# PROP Target_Dir ""
F90=df.exe
MTL=midl.exe
LINK32=link.exe
# ADD BASE LINK32 /nologo /dll /debug /machine:I386 /out:"vcodec/Debug/vcodec_d.pyd" /pdbtype:sept /export:initvcodec
CPP=cl.exe
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "." /I "../SDL-1.2.5/include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "HAVE_MMX" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "." /I "../SDL-1.2.5/include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "HAVE_MMX" /YX /FD /GZ /c
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD LIB32 /nologo /out:"vcodec/Debug_Static/vcodec_d.lib"

!ELSEIF  "$(CFG)" == "vcodec - Win32 Release_Static"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "vcodec___Win32_Release_Static"
# PROP BASE Intermediate_Dir "vcodec___Win32_Release_Static"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "vcodec/Release_Static"
# PROP Intermediate_Dir "vcodec/Release_Static"
# PROP Target_Dir ""
F90=df.exe
MTL=midl.exe
LINK32=link.exe
CPP=cl.exe
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "." /I "../SDL-1.2.5/include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "HAVE_MMX" /YX /FD /GZ /c
# ADD CPP /nologo /ML /W3 /GX /O2 /Ob2 /I "." /I "../SDL-1.2.5/include" /D "WIN32" /D "_NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "HAVE_MMX" /YX /FD /c
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"vcodec/Debug_Static/vcodec_d.lib"
# ADD LIB32 /nologo /out:"vcodec/Release_Static/vcodec_d.lib"

!ENDIF 

# Begin Target

# Name "vcodec - Win32 Release"
# Name "vcodec - Win32 Debug"
# Name "vcodec - Win32 Debug_Static"
# Name "vcodec - Win32 Release_Static"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\vcodec\vcodec.c
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
