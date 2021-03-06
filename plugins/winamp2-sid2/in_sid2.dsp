# Microsoft Developer Studio Project File - Name="in_sid2" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=in_sid2 - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "in_sid2.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "in_sid2.mak" CFG="in_sid2 - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "in_sid2 - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "in_sid2 - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "in_sid2 - Win32 Release"

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
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "IN_SID2_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /Zi /O2 /I "../../libsidplay/include" /I "../../libsidplay/include/sidplay" /I "../../libsidplay/src" /I "../../builders/resid-builder/include/sidplay/builders" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "HAVE_MSWINDOWS" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x407 /d "NDEBUG"
# ADD RSC /l 0x407 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib advapi32.lib libsidplay_static.lib resid_builder.lib /nologo /dll /debug /machine:I386 /out:"../../bin_vc6/Release/in_sid2.dll" /libpath:"../../bin_vc6/Release"
# SUBTRACT LINK32 /map

!ELSEIF  "$(CFG)" == "in_sid2 - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "in_sid2___Win32_Debug"
# PROP BASE Intermediate_Dir "in_sid2___Win32_Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "IN_SID2_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "../../libsidplay/include" /I "../../libsidplay/include/sidplay" /I "../../libsidplay/src" /I "../../builders/resid-builder/include/sidplay/builders" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "HAVE_MSWINDOWS" /FR /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x407 /d "_DEBUG"
# ADD RSC /l 0x407 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib advapi32.lib libsidplay_static.lib resid_builder.lib /nologo /dll /debug /machine:I386 /out:"../../bin_vc6/Debug/in_sid2.dll" /pdbtype:sept /libpath:"../../bin_vc6/Debug"
# SUBTRACT LINK32 /nodefaultlib

!ENDIF 

# Begin Target

# Name "in_sid2 - Win32 Release"
# Name "in_sid2 - Win32 Debug"
# Begin Group "winamp2 files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\in2.h
# End Source File
# Begin Source File

SOURCE=.\src\out.h
# End Source File
# End Group
# Begin Group "source files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\aboutdlg.cpp
# End Source File
# Begin Source File

SOURCE=.\src\configdlg.cpp
# End Source File
# Begin Source File

SOURCE=.\src\in_sid2.cpp
# End Source File
# Begin Source File

SOURCE=.\src\infodlg.cpp
# End Source File
# Begin Source File

SOURCE=.\src\wa2player.cpp
# End Source File
# End Group
# Begin Group "header files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\resource.h
# End Source File
# Begin Source File

SOURCE=.\src\wa2player.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\in_sid2.txt
# End Source File
# Begin Source File

SOURCE=.\src\in_sid2_res.rc
# End Source File
# End Target
# End Project
