# Microsoft Developer Studio Project File - Name="vcodec_lib" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=vcodec_lib - Win32 Release_Static
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "vcodec_lib.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "vcodec_lib.mak" CFG="vcodec_lib - Win32 Release_Static"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "vcodec_lib - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "vcodec_lib - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE "vcodec_lib - Win32 Debug_Static" (based on "Win32 (x86) Static Library")
!MESSAGE "vcodec_lib - Win32 Release_Static" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "vcodec_lib - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "libavcodec/Release"
# PROP BASE Intermediate_Dir "libavcodec/Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "libavcodec/Release"
# PROP Intermediate_Dir "libavcodec/Release"
# PROP Target_Dir ""
MTL=midl.exe
LINK32=link.exe
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "." /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /D "HAVE_AV_CONFIG_H" /D "HAVE_MMX" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "vcodec_lib - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "libavcodec/Debug"
# PROP BASE Intermediate_Dir "libavcodec/Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "libavcodec/Debug"
# PROP Intermediate_Dir "libavcodec/Debug"
# PROP Target_Dir ""
MTL=midl.exe
LINK32=link.exe
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /Gm /GX /ZI /Od /I "." /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /D "HAVE_AV_CONFIG_H" /D "HAVE_MMX" /YX /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "vcodec_lib - Win32 Debug_Static"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "vcodec_lib___Win32_Debug_Static"
# PROP BASE Intermediate_Dir "vcodec_lib___Win32_Debug_Static"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "libavcodec/Debug_Static"
# PROP Intermediate_Dir "libavcodec/Debug_Static"
# PROP Target_Dir ""
MTL=midl.exe
LINK32=link.exe
# ADD BASE CPP /nologo /MDd /Gm /GX /ZI /Od /I "." /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /D "HAVE_AV_CONFIG_H" /D "HAVE_MMX" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /GX- /Od /I "." /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /D "HAVE_AV_CONFIG_H" /D "HAVE_MMX" /D "_STATIC" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "vcodec_lib - Win32 Release_Static"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "vcodec_lib___Win32_Release_Static"
# PROP BASE Intermediate_Dir "vcodec_lib___Win32_Release_Static"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "vcodec/Release_Static"
# PROP Intermediate_Dir "vcodec/Release_Static"
# PROP Target_Dir ""
MTL=midl.exe
LINK32=link.exe
# ADD BASE CPP /nologo /MDd /GX /O2 /I "." /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /D "HAVE_AV_CONFIG_H" /D "HAVE_MMX" /D "_STATIC" /Fp"libavcodec/Debug_Static/vcodec_lib.pch" /YX /FD /c
# ADD CPP /nologo /ML /GX /O2 /Ob2 /I "." /D "WIN32" /D "_NDEBUG" /D "_MBCS" /D "_LIB" /D "HAVE_AV_CONFIG_H" /D "HAVE_MMX" /D "_STATIC" /Fp"libavcodec/Debug_Static/vcodec_lib.pch" /YX /FD /c
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

# Name "vcodec_lib - Win32 Release"
# Name "vcodec_lib - Win32 Debug"
# Name "vcodec_lib - Win32 Debug_Static"
# Name "vcodec_lib - Win32 Release_Static"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Group "mmx"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\libavcodec\i386\cputest.c

!IF  "$(CFG)" == "vcodec_lib - Win32 Release"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
OutDir=.\libavcodec/Release
ProjDir=.
InputPath=.\libavcodec\i386\cputest.c
InputName=cputest

"$(OutDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	PATH=%PATH%;c:\cygwin\bin 
	gcc -c -O3 -march=i586 -mcpu=i686 -fomit-frame-pointer -finline -finline-functions -DSYS_CYGWIN -DHAVE_AV_CONFIG_H -DHAVE_MMX -pipe -mno-cygwin -mdll -I$(ProjDir) $(InputPath) -o $(OutDir)\$(InputName).obj 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "vcodec_lib - Win32 Debug"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
OutDir=.\libavcodec/Debug
ProjDir=.
InputPath=.\libavcodec\i386\cputest.c
InputName=cputest

"$(OutDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	PATH=%PATH%;c:\cygwin\bin 
	gcc -c -O3 -march=i586 -mcpu=i686 -fomit-frame-pointer -finline -finline-functions -DSYS_CYGWIN -DHAVE_AV_CONFIG_H -DHAVE_MMX -pipe -mno-cygwin -mdll -I$(ProjDir) $(InputPath) -o $(OutDir)\$(InputName).obj 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "vcodec_lib - Win32 Debug_Static"

# PROP BASE Ignore_Default_Tool 1
# PROP Ignore_Default_Tool 1
# Begin Custom Build
OutDir=.\libavcodec/Debug_Static
ProjDir=.
InputPath=.\libavcodec\i386\cputest.c
InputName=cputest

"$(OutDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	PATH=%PATH%;c:\cygwin\bin 
	gcc -c -O3 -march=i586 -mcpu=i686 -fomit-frame-pointer -finline -finline-functions -DSYS_CYGWIN -DHAVE_AV_CONFIG_H -DHAVE_MMX -pipe -mno-cygwin -mdll -I$(ProjDir) $(InputPath) -o $(OutDir)\$(InputName).obj 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "vcodec_lib - Win32 Release_Static"

# PROP BASE Ignore_Default_Tool 1
# PROP Ignore_Default_Tool 1
# Begin Custom Build
OutDir=.\vcodec/Release_Static
ProjDir=.
InputPath=.\libavcodec\i386\cputest.c
InputName=cputest

"$(OutDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	PATH=%PATH%;c:\cygwin\bin 
	gcc -c -O3 -march=i586 -mcpu=i686 -fomit-frame-pointer -finline -finline-functions -DSYS_CYGWIN -DHAVE_AV_CONFIG_H -DHAVE_MMX -pipe -mno-cygwin -mdll -I$(ProjDir) $(InputPath) -o $(OutDir)\$(InputName).obj 
	
# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\libavcodec\dsputil.c

!IF  "$(CFG)" == "vcodec_lib - Win32 Release"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
OutDir=.\libavcodec/Release
ProjDir=.
InputPath=.\libavcodec\dsputil.c
InputName=dsputil

"$(OutDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	PATH=%PATH%;c:\cygwin\bin 
	gcc -c -O3 -march=i586 -mcpu=i686 -fomit-frame-pointer -finline -finline-functions -DSYS_CYGWIN -DHAVE_AV_CONFIG_H -DHAVE_MMX -pipe -mno-cygwin -mdll -I$(ProjDir) $(InputPath) -o $(OutDir)\$(InputName).obj 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "vcodec_lib - Win32 Debug"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
OutDir=.\libavcodec/Debug
ProjDir=.
InputPath=.\libavcodec\dsputil.c
InputName=dsputil

"$(OutDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	PATH=%PATH%;c:\cygwin\bin 
	gcc -c -O3 -march=i586 -mcpu=i686 -fomit-frame-pointer -finline -finline-functions -DSYS_CYGWIN -DHAVE_AV_CONFIG_H -DHAVE_MMX -pipe -mno-cygwin -mdll -I$(ProjDir) $(InputPath) -o $(OutDir)\$(InputName).obj 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "vcodec_lib - Win32 Debug_Static"

# PROP BASE Ignore_Default_Tool 1
# PROP Ignore_Default_Tool 1
# Begin Custom Build
OutDir=.\libavcodec/Debug_Static
ProjDir=.
InputPath=.\libavcodec\dsputil.c
InputName=dsputil

"$(OutDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	PATH=%PATH%;c:\cygwin\bin 
	gcc -c -O3 -march=i586 -mcpu=i686 -fomit-frame-pointer -finline -finline-functions -DSYS_CYGWIN -DHAVE_AV_CONFIG_H -DHAVE_MMX -pipe -mno-cygwin -mdll -I$(ProjDir) $(InputPath) -o $(OutDir)\$(InputName).obj 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "vcodec_lib - Win32 Release_Static"

# PROP BASE Ignore_Default_Tool 1
# PROP Ignore_Default_Tool 1
# Begin Custom Build
OutDir=.\vcodec/Release_Static
ProjDir=.
InputPath=.\libavcodec\dsputil.c
InputName=dsputil

"$(OutDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	PATH=%PATH%;c:\cygwin\bin 
	gcc -c -O3 -march=i586 -mcpu=i686 -fomit-frame-pointer -finline -finline-functions -DSYS_CYGWIN -DHAVE_AV_CONFIG_H -DHAVE_MMX -pipe -mno-cygwin -mdll -I$(ProjDir) $(InputPath) -o $(OutDir)\$(InputName).obj 
	
# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\libavcodec\i386\dsputil_mmx.c

!IF  "$(CFG)" == "vcodec_lib - Win32 Release"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
OutDir=.\libavcodec/Release
ProjDir=.
InputPath=.\libavcodec\i386\dsputil_mmx.c
InputName=dsputil_mmx

"$(OutDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	PATH=%PATH%;c:\cygwin\bin 
	gcc -c -O3 -march=i586 -mcpu=i686 -fomit-frame-pointer -finline -finline-functions -DSYS_CYGWIN -DHAVE_AV_CONFIG_H -DHAVE_MMX -pipe -mno-cygwin -mdll -I$(ProjDir) $(InputPath) -o $(OutDir)\$(InputName).obj 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "vcodec_lib - Win32 Debug"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
OutDir=.\libavcodec/Debug
ProjDir=.
InputPath=.\libavcodec\i386\dsputil_mmx.c
InputName=dsputil_mmx

"$(OutDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	PATH=%PATH%;c:\cygwin\bin 
	gcc -c -O3 -march=i586 -mcpu=i686 -fomit-frame-pointer -finline -finline-functions -DSYS_CYGWIN -DHAVE_AV_CONFIG_H -DHAVE_MMX -pipe -mno-cygwin -mdll -I$(ProjDir) $(InputPath) -o $(OutDir)\$(InputName).obj 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "vcodec_lib - Win32 Debug_Static"

# PROP BASE Ignore_Default_Tool 1
# PROP Ignore_Default_Tool 1
# Begin Custom Build
OutDir=.\libavcodec/Debug_Static
ProjDir=.
InputPath=.\libavcodec\i386\dsputil_mmx.c
InputName=dsputil_mmx

"$(OutDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	PATH=%PATH%;c:\cygwin\bin 
	gcc -c -O3 -march=i586 -mcpu=i686 -fomit-frame-pointer -finline -finline-functions -DSYS_CYGWIN -DHAVE_AV_CONFIG_H -DHAVE_MMX -pipe -mno-cygwin -mdll -I$(ProjDir) $(InputPath) -o $(OutDir)\$(InputName).obj 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "vcodec_lib - Win32 Release_Static"

# PROP BASE Ignore_Default_Tool 1
# PROP Ignore_Default_Tool 1
# Begin Custom Build
OutDir=.\vcodec/Release_Static
ProjDir=.
InputPath=.\libavcodec\i386\dsputil_mmx.c
InputName=dsputil_mmx

"$(OutDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	PATH=%PATH%;c:\cygwin\bin 
	gcc -c -O3 -march=i586 -mcpu=i686 -fomit-frame-pointer -finline -finline-functions -DSYS_CYGWIN -DHAVE_AV_CONFIG_H -DHAVE_MMX -pipe -mno-cygwin -mdll -I$(ProjDir) $(InputPath) -o $(OutDir)\$(InputName).obj 
	
# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\libavcodec\i386\fdct_mmx.c

!IF  "$(CFG)" == "vcodec_lib - Win32 Release"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
OutDir=.\libavcodec/Release
ProjDir=.
InputPath=.\libavcodec\i386\fdct_mmx.c
InputName=fdct_mmx

"$(OutDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	PATH=%PATH%;c:\cygwin\bin 
	gcc -c -O3 -march=i586 -mcpu=i686 -fomit-frame-pointer -finline -finline-functions -DSYS_CYGWIN -DHAVE_AV_CONFIG_H -DHAVE_MMX -pipe -mno-cygwin -mdll -I$(ProjDir) $(InputPath) -o $(OutDir)\$(InputName).obj 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "vcodec_lib - Win32 Debug"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
OutDir=.\libavcodec/Debug
ProjDir=.
InputPath=.\libavcodec\i386\fdct_mmx.c
InputName=fdct_mmx

"$(OutDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	PATH=%PATH%;c:\cygwin\bin 
	gcc -c -O3 -march=i586 -mcpu=i686 -fomit-frame-pointer -finline -finline-functions -DSYS_CYGWIN -DHAVE_AV_CONFIG_H -DHAVE_MMX -pipe -mno-cygwin -mdll -I$(ProjDir) $(InputPath) -o $(OutDir)\$(InputName).obj 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "vcodec_lib - Win32 Debug_Static"

# PROP BASE Ignore_Default_Tool 1
# PROP Ignore_Default_Tool 1
# Begin Custom Build
OutDir=.\libavcodec/Debug_Static
ProjDir=.
InputPath=.\libavcodec\i386\fdct_mmx.c
InputName=fdct_mmx

"$(OutDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	PATH=%PATH%;c:\cygwin\bin 
	gcc -c -O3 -march=i586 -mcpu=i686 -fomit-frame-pointer -finline -finline-functions -DSYS_CYGWIN -DHAVE_AV_CONFIG_H -DHAVE_MMX -pipe -mno-cygwin -mdll -I$(ProjDir) $(InputPath) -o $(OutDir)\$(InputName).obj 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "vcodec_lib - Win32 Release_Static"

# PROP BASE Ignore_Default_Tool 1
# PROP Ignore_Default_Tool 1
# Begin Custom Build
OutDir=.\vcodec/Release_Static
ProjDir=.
InputPath=.\libavcodec\i386\fdct_mmx.c
InputName=fdct_mmx

"$(OutDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	PATH=%PATH%;c:\cygwin\bin 
	gcc -c -O3 -march=i586 -mcpu=i686 -fomit-frame-pointer -finline -finline-functions -DSYS_CYGWIN -DHAVE_AV_CONFIG_H -DHAVE_MMX -pipe -mno-cygwin -mdll -I$(ProjDir) $(InputPath) -o $(OutDir)\$(InputName).obj 
	
# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\libavcodec\i386\idct_mmx.c

!IF  "$(CFG)" == "vcodec_lib - Win32 Release"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
OutDir=.\libavcodec/Release
ProjDir=.
InputPath=.\libavcodec\i386\idct_mmx.c
InputName=idct_mmx

"$(OutDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	PATH=%PATH%;c:\cygwin\bin 
	gcc -c -O3 -march=i586 -mcpu=i686 -fomit-frame-pointer -finline -finline-functions -DSYS_CYGWIN -DHAVE_AV_CONFIG_H -DHAVE_MMX -pipe -mno-cygwin -mdll -I$(ProjDir) $(InputPath) -o $(OutDir)\$(InputName).obj 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "vcodec_lib - Win32 Debug"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
OutDir=.\libavcodec/Debug
ProjDir=.
InputPath=.\libavcodec\i386\idct_mmx.c
InputName=idct_mmx

"$(OutDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	PATH=%PATH%;c:\cygwin\bin 
	gcc -c -O3 -march=i586 -mcpu=i686 -fomit-frame-pointer -finline -finline-functions -DSYS_CYGWIN -DHAVE_AV_CONFIG_H -DHAVE_MMX -pipe -mno-cygwin -mdll -I$(ProjDir) $(InputPath) -o $(OutDir)\$(InputName).obj 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "vcodec_lib - Win32 Debug_Static"

# PROP BASE Ignore_Default_Tool 1
# PROP Ignore_Default_Tool 1
# Begin Custom Build
OutDir=.\libavcodec/Debug_Static
ProjDir=.
InputPath=.\libavcodec\i386\idct_mmx.c
InputName=idct_mmx

"$(OutDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	PATH=%PATH%;c:\cygwin\bin 
	gcc -c -O3 -march=i586 -mcpu=i686 -fomit-frame-pointer -finline -finline-functions -DSYS_CYGWIN -DHAVE_AV_CONFIG_H -DHAVE_MMX -pipe -mno-cygwin -mdll -I$(ProjDir) $(InputPath) -o $(OutDir)\$(InputName).obj 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "vcodec_lib - Win32 Release_Static"

# PROP BASE Ignore_Default_Tool 1
# PROP Ignore_Default_Tool 1
# Begin Custom Build
OutDir=.\vcodec/Release_Static
ProjDir=.
InputPath=.\libavcodec\i386\idct_mmx.c
InputName=idct_mmx

"$(OutDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	PATH=%PATH%;c:\cygwin\bin 
	gcc -c -O3 -march=i586 -mcpu=i686 -fomit-frame-pointer -finline -finline-functions -DSYS_CYGWIN -DHAVE_AV_CONFIG_H -DHAVE_MMX -pipe -mno-cygwin -mdll -I$(ProjDir) $(InputPath) -o $(OutDir)\$(InputName).obj 
	
# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\libavcodec\imgconvert.c

!IF  "$(CFG)" == "vcodec_lib - Win32 Release"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
OutDir=.\libavcodec/Release
ProjDir=.
InputPath=.\libavcodec\imgconvert.c
InputName=imgconvert

"$(OutDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	PATH=%PATH%;c:\cygwin\bin 
	gcc -c -O3 -march=i586 -mcpu=i686 -fomit-frame-pointer -finline -finline-functions -DSYS_CYGWIN -DHAVE_AV_CONFIG_H -DHAVE_MMX -pipe -mno-cygwin -mdll -I$(ProjDir) $(InputPath) -o $(OutDir)\$(InputName).obj 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "vcodec_lib - Win32 Debug"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
OutDir=.\libavcodec/Debug
ProjDir=.
InputPath=.\libavcodec\imgconvert.c
InputName=imgconvert

"$(OutDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	PATH=%PATH%;c:\cygwin\bin 
	gcc -c -O3 -march=i586 -mcpu=i686 -fomit-frame-pointer -finline -finline-functions -DSYS_CYGWIN -DHAVE_AV_CONFIG_H -DHAVE_MMX -pipe -mno-cygwin -mdll -I$(ProjDir) $(InputPath) -o $(OutDir)\$(InputName).obj 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "vcodec_lib - Win32 Debug_Static"

# PROP BASE Ignore_Default_Tool 1
# PROP Ignore_Default_Tool 1
# Begin Custom Build
OutDir=.\libavcodec/Debug_Static
ProjDir=.
InputPath=.\libavcodec\imgconvert.c
InputName=imgconvert

"$(OutDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	PATH=%PATH%;c:\cygwin\bin 
	gcc -c -O3 -march=i586 -mcpu=i686 -fomit-frame-pointer -finline -finline-functions -DSYS_CYGWIN -DHAVE_AV_CONFIG_H -DHAVE_MMX -pipe -mno-cygwin -mdll -I$(ProjDir) $(InputPath) -o $(OutDir)\$(InputName).obj 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "vcodec_lib - Win32 Release_Static"

# PROP BASE Ignore_Default_Tool 1
# PROP Ignore_Default_Tool 1
# Begin Custom Build
OutDir=.\vcodec/Release_Static
ProjDir=.
InputPath=.\libavcodec\imgconvert.c
InputName=imgconvert

"$(OutDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	PATH=%PATH%;c:\cygwin\bin 
	gcc -c -O3 -march=i586 -mcpu=i686 -fomit-frame-pointer -finline -finline-functions -DSYS_CYGWIN -DHAVE_AV_CONFIG_H -DHAVE_MMX -pipe -mno-cygwin -mdll -I$(ProjDir) $(InputPath) -o $(OutDir)\$(InputName).obj 
	
# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\libavcodec\imgresample.c

!IF  "$(CFG)" == "vcodec_lib - Win32 Release"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
OutDir=.\libavcodec/Release
ProjDir=.
InputPath=.\libavcodec\imgresample.c
InputName=imgresample

"$(OutDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	PATH=%PATH%;c:\cygwin\bin 
	gcc -c -O3 -march=i586 -mcpu=i686 -fomit-frame-pointer -finline -finline-functions -DSYS_CYGWIN -DHAVE_AV_CONFIG_H -DHAVE_MMX -pipe -mno-cygwin -mdll -I$(ProjDir) $(InputPath) -o $(OutDir)\$(InputName).obj 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "vcodec_lib - Win32 Debug"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
OutDir=.\libavcodec/Debug
ProjDir=.
InputPath=.\libavcodec\imgresample.c
InputName=imgresample

"$(OutDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	PATH=%PATH%;c:\cygwin\bin 
	gcc -c -O3 -march=i586 -mcpu=i686 -fomit-frame-pointer -finline -finline-functions -DSYS_CYGWIN -DHAVE_AV_CONFIG_H -DHAVE_MMX -pipe -mno-cygwin -mdll -I$(ProjDir) $(InputPath) -o $(OutDir)\$(InputName).obj 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "vcodec_lib - Win32 Debug_Static"

# PROP BASE Ignore_Default_Tool 1
# PROP Ignore_Default_Tool 1
# Begin Custom Build
OutDir=.\libavcodec/Debug_Static
ProjDir=.
InputPath=.\libavcodec\imgresample.c
InputName=imgresample

"$(OutDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	PATH=%PATH%;c:\cygwin\bin 
	gcc -c -O3 -march=i586 -mcpu=i686 -fomit-frame-pointer -finline -finline-functions -DSYS_CYGWIN -DHAVE_AV_CONFIG_H -DHAVE_MMX -pipe -mno-cygwin -mdll -I$(ProjDir) $(InputPath) -o $(OutDir)\$(InputName).obj 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "vcodec_lib - Win32 Release_Static"

# PROP BASE Ignore_Default_Tool 1
# PROP Ignore_Default_Tool 1
# Begin Custom Build
OutDir=.\vcodec/Release_Static
ProjDir=.
InputPath=.\libavcodec\imgresample.c
InputName=imgresample

"$(OutDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	PATH=%PATH%;c:\cygwin\bin 
	gcc -c -O3 -march=i586 -mcpu=i686 -fomit-frame-pointer -finline -finline-functions -DSYS_CYGWIN -DHAVE_AV_CONFIG_H -DHAVE_MMX -pipe -mno-cygwin -mdll -I$(ProjDir) $(InputPath) -o $(OutDir)\$(InputName).obj 
	
# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\libavcodec\i386\motion_est_mmx.c

!IF  "$(CFG)" == "vcodec_lib - Win32 Release"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
OutDir=.\libavcodec/Release
ProjDir=.
InputPath=.\libavcodec\i386\motion_est_mmx.c
InputName=motion_est_mmx

"$(OutDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	PATH=%PATH%;c:\cygwin\bin 
	gcc -c -O3 -march=i586 -mcpu=i686 -fomit-frame-pointer -finline -finline-functions -DSYS_CYGWIN -DHAVE_AV_CONFIG_H -DHAVE_MMX -pipe -mno-cygwin -mdll -I$(ProjDir) $(InputPath) -o $(OutDir)\$(InputName).obj 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "vcodec_lib - Win32 Debug"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
OutDir=.\libavcodec/Debug
ProjDir=.
InputPath=.\libavcodec\i386\motion_est_mmx.c
InputName=motion_est_mmx

"$(OutDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	PATH=%PATH%;c:\cygwin\bin 
	gcc -c -O3 -march=i586 -mcpu=i686 -fomit-frame-pointer -finline -finline-functions -DSYS_CYGWIN -DHAVE_AV_CONFIG_H -DHAVE_MMX -pipe -mno-cygwin -mdll -I$(ProjDir) $(InputPath) -o $(OutDir)\$(InputName).obj 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "vcodec_lib - Win32 Debug_Static"

# PROP BASE Ignore_Default_Tool 1
# PROP Ignore_Default_Tool 1
# Begin Custom Build
OutDir=.\libavcodec/Debug_Static
ProjDir=.
InputPath=.\libavcodec\i386\motion_est_mmx.c
InputName=motion_est_mmx

"$(OutDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	PATH=%PATH%;c:\cygwin\bin 
	gcc -c -O3 -march=i586 -mcpu=i686 -fomit-frame-pointer -finline -finline-functions -DSYS_CYGWIN -DHAVE_AV_CONFIG_H -DHAVE_MMX -pipe -mno-cygwin -mdll -I$(ProjDir) $(InputPath) -o $(OutDir)\$(InputName).obj 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "vcodec_lib - Win32 Release_Static"

# PROP BASE Ignore_Default_Tool 1
# PROP Ignore_Default_Tool 1
# Begin Custom Build
OutDir=.\vcodec/Release_Static
ProjDir=.
InputPath=.\libavcodec\i386\motion_est_mmx.c
InputName=motion_est_mmx

"$(OutDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	PATH=%PATH%;c:\cygwin\bin 
	gcc -c -O3 -march=i586 -mcpu=i686 -fomit-frame-pointer -finline -finline-functions -DSYS_CYGWIN -DHAVE_AV_CONFIG_H -DHAVE_MMX -pipe -mno-cygwin -mdll -I$(ProjDir) $(InputPath) -o $(OutDir)\$(InputName).obj 
	
# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\libavcodec\i386\mpegvideo_mmx.c

!IF  "$(CFG)" == "vcodec_lib - Win32 Release"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
OutDir=.\libavcodec/Release
ProjDir=.
InputPath=.\libavcodec\i386\mpegvideo_mmx.c
InputName=mpegvideo_mmx

"$(OutDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	PATH=%PATH%;c:\cygwin\bin 
	gcc -c -O3 -march=i586 -mcpu=i686 -fomit-frame-pointer -finline -finline-functions -DSYS_CYGWIN -DHAVE_AV_CONFIG_H -DHAVE_MMX -pipe -mno-cygwin -mdll -I$(ProjDir) $(InputPath) -o $(OutDir)\$(InputName).obj 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "vcodec_lib - Win32 Debug"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
OutDir=.\libavcodec/Debug
ProjDir=.
InputPath=.\libavcodec\i386\mpegvideo_mmx.c
InputName=mpegvideo_mmx

"$(OutDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	PATH=%PATH%;c:\cygwin\bin 
	gcc -c -O3 -march=i586 -mcpu=i686 -fomit-frame-pointer -finline -finline-functions -DSYS_CYGWIN -DHAVE_AV_CONFIG_H -DHAVE_MMX -pipe -mno-cygwin -mdll -I$(ProjDir) $(InputPath) -o $(OutDir)\$(InputName).obj 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "vcodec_lib - Win32 Debug_Static"

# PROP BASE Ignore_Default_Tool 1
# PROP Ignore_Default_Tool 1
# Begin Custom Build
OutDir=.\libavcodec/Debug_Static
ProjDir=.
InputPath=.\libavcodec\i386\mpegvideo_mmx.c
InputName=mpegvideo_mmx

"$(OutDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	PATH=%PATH%;c:\cygwin\bin 
	gcc -c -O3 -march=i586 -mcpu=i686 -fomit-frame-pointer -finline -finline-functions -DSYS_CYGWIN -DHAVE_AV_CONFIG_H -DHAVE_MMX -pipe -mno-cygwin -mdll -I$(ProjDir) $(InputPath) -o $(OutDir)\$(InputName).obj 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "vcodec_lib - Win32 Release_Static"

# PROP BASE Ignore_Default_Tool 1
# PROP Ignore_Default_Tool 1
# Begin Custom Build
OutDir=.\vcodec/Release_Static
ProjDir=.
InputPath=.\libavcodec\i386\mpegvideo_mmx.c
InputName=mpegvideo_mmx

"$(OutDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	PATH=%PATH%;c:\cygwin\bin 
	gcc -c -O3 -march=i586 -mcpu=i686 -fomit-frame-pointer -finline -finline-functions -DSYS_CYGWIN -DHAVE_AV_CONFIG_H -DHAVE_MMX -pipe -mno-cygwin -mdll -I$(ProjDir) $(InputPath) -o $(OutDir)\$(InputName).obj 
	
# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\libavcodec\i386\simple_idct_mmx.c

!IF  "$(CFG)" == "vcodec_lib - Win32 Release"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
OutDir=.\libavcodec/Release
ProjDir=.
InputPath=.\libavcodec\i386\simple_idct_mmx.c
InputName=simple_idct_mmx

"$(OutDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	PATH=%PATH%;c:\cygwin\bin 
	gcc -c -O3 -march=i586 -mcpu=i686 -fomit-frame-pointer -finline -finline-functions -DSYS_CYGWIN -DHAVE_AV_CONFIG_H -DHAVE_MMX -pipe -mno-cygwin -mdll -I$(ProjDir) $(InputPath) -o $(OutDir)\$(InputName).obj 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "vcodec_lib - Win32 Debug"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
OutDir=.\libavcodec/Debug
ProjDir=.
InputPath=.\libavcodec\i386\simple_idct_mmx.c
InputName=simple_idct_mmx

"$(OutDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	PATH=%PATH%;c:\cygwin\bin 
	gcc -c -O3 -march=i586 -mcpu=i686 -fomit-frame-pointer -finline -finline-functions -DSYS_CYGWIN -DHAVE_AV_CONFIG_H -DHAVE_MMX -pipe -mno-cygwin -mdll -I$(ProjDir) $(InputPath) -o $(OutDir)\$(InputName).obj 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "vcodec_lib - Win32 Debug_Static"

# PROP BASE Ignore_Default_Tool 1
# PROP Ignore_Default_Tool 1
# Begin Custom Build
OutDir=.\libavcodec/Debug_Static
ProjDir=.
InputPath=.\libavcodec\i386\simple_idct_mmx.c
InputName=simple_idct_mmx

"$(OutDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	PATH=%PATH%;c:\cygwin\bin 
	gcc -c -O3 -march=i586 -mcpu=i686 -fomit-frame-pointer -finline -finline-functions -DSYS_CYGWIN -DHAVE_AV_CONFIG_H -DHAVE_MMX -pipe -mno-cygwin -mdll -I$(ProjDir) $(InputPath) -o $(OutDir)\$(InputName).obj 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "vcodec_lib - Win32 Release_Static"

# PROP BASE Ignore_Default_Tool 1
# PROP Ignore_Default_Tool 1
# Begin Custom Build
OutDir=.\vcodec/Release_Static
ProjDir=.
InputPath=.\libavcodec\i386\simple_idct_mmx.c
InputName=simple_idct_mmx

"$(OutDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	PATH=%PATH%;c:\cygwin\bin 
	gcc -c -O3 -march=i586 -mcpu=i686 -fomit-frame-pointer -finline -finline-functions -DSYS_CYGWIN -DHAVE_AV_CONFIG_H -DHAVE_MMX -pipe -mno-cygwin -mdll -I$(ProjDir) $(InputPath) -o $(OutDir)\$(InputName).obj 
	
# End Custom Build

!ENDIF 

# End Source File
# End Group
# Begin Group "nonmmx"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\libavcodec\fdctref.c

!IF  "$(CFG)" == "vcodec_lib - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "vcodec_lib - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "vcodec_lib - Win32 Debug_Static"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "vcodec_lib - Win32 Release_Static"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\libavcodec\fft.c

!IF  "$(CFG)" == "vcodec_lib - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "vcodec_lib - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "vcodec_lib - Win32 Debug_Static"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "vcodec_lib - Win32 Release_Static"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\libavcodec\mdct.c

!IF  "$(CFG)" == "vcodec_lib - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "vcodec_lib - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "vcodec_lib - Win32 Debug_Static"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "vcodec_lib - Win32 Release_Static"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# End Group
# Begin Source File

SOURCE=.\libavcodec\cabac.c
# End Source File
# Begin Source File

SOURCE=.\common.c
# End Source File
# Begin Source File

SOURCE=.\libavcodec\error_resilience.c
# End Source File
# Begin Source File

SOURCE=.\libavcodec\eval.c
# End Source File
# Begin Source File

SOURCE=.\libavcodec\golomb.c
# End Source File
# Begin Source File

SOURCE=.\libavcodec\h263.c
# End Source File
# Begin Source File

SOURCE=.\libavcodec\h263dec.c
# End Source File
# Begin Source File

SOURCE=.\libavcodec\h264.c
# End Source File
# Begin Source File

SOURCE=.\libavcodec\jfdctfst.c
# End Source File
# Begin Source File

SOURCE=.\libavcodec\jfdctint.c
# End Source File
# Begin Source File

SOURCE=.\libavcodec\jrevdct.c
# End Source File
# Begin Source File

SOURCE=.\mem.c
# End Source File
# Begin Source File

SOURCE=.\libavcodec\mjpeg.c
# End Source File
# Begin Source File

SOURCE=.\libavcodec\motion_est.c
# End Source File
# Begin Source File

SOURCE=.\libavcodec\mpeg12.c
# End Source File
# Begin Source File

SOURCE=.\libavcodec\mpegvideo.c
# End Source File
# Begin Source File

SOURCE=.\libavcodec\msmpeg4.c
# End Source File
# Begin Source File

SOURCE=.\libavcodec\opts.c
# End Source File
# Begin Source File

SOURCE=.\libavcodec\ratecontrol.c
# End Source File
# Begin Source File

SOURCE=.\libavcodec\rv10.c
# End Source File
# Begin Source File

SOURCE=.\libavcodec\simple_idct.c
# End Source File
# Begin Source File

SOURCE=.\libavcodec\svq1.c
# End Source File
# Begin Source File

SOURCE=.\libavcodec\utils.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# End Group
# End Target
# End Project
