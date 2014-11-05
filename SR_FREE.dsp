# Microsoft Developer Studio Project File - Name="SR_FREE" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 60000
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=SR_FREE - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "SR_FREE.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "SR_FREE.mak" CFG="SR_FREE - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "SR_FREE - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "SR_FREE - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "SR_FREE - Win32 Release"

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
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I ".\\" /I ".\grep\\" /I ".\grep\lib\\" /FI"pragma.h" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "HAVE_CONFIG_H" /FR /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /profile /machine:I386
# Begin Special Build Tool
TargetPath=.\Release\SR_FREE.exe
SOURCE="$(InputPath)"
PostBuild_Cmds=upx $(TargetPath)	copy $(TargetPath) "c:\Program Files\SR_FREE\"
# End Special Build Tool

!ELSEIF  "$(CFG)" == "SR_FREE - Win32 Debug"

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
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I ".\\" /I ".\grep\\" /I ".\grep\lib\\" /FI"pragma.h" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "HAVE_CONFIG_H" /FR /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "SR_FREE - Win32 Release"
# Name "SR_FREE - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Group "grep"

# PROP Default_Filter ""
# Begin Group "lib"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\grep\lib\alloca.c

!IF  "$(CFG)" == "SR_FREE - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "SR_FREE - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\grep\lib\atexit.c

!IF  "$(CFG)" == "SR_FREE - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "SR_FREE - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\grep\lib\closeout.c

!IF  "$(CFG)" == "SR_FREE - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "SR_FREE - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\grep\lib\error.c

!IF  "$(CFG)" == "SR_FREE - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "SR_FREE - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\grep\lib\exclude.c

!IF  "$(CFG)" == "SR_FREE - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "SR_FREE - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\grep\lib\fnmatch.c

!IF  "$(CFG)" == "SR_FREE - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "SR_FREE - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\grep\lib\getopt.c

!IF  "$(CFG)" == "SR_FREE - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "SR_FREE - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\grep\lib\getopt1.c

!IF  "$(CFG)" == "SR_FREE - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "SR_FREE - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=".\grep\lib\hard-locale.c"

!IF  "$(CFG)" == "SR_FREE - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "SR_FREE - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\grep\lib\isdir.c

!IF  "$(CFG)" == "SR_FREE - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "SR_FREE - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\grep\lib\malloc.c

!IF  "$(CFG)" == "SR_FREE - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "SR_FREE - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\grep\lib\memchr.c

!IF  "$(CFG)" == "SR_FREE - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "SR_FREE - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\grep\lib\obstack.c

!IF  "$(CFG)" == "SR_FREE - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "SR_FREE - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\grep\lib\quotearg.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\grep\lib\realloc.c

!IF  "$(CFG)" == "SR_FREE - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "SR_FREE - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\grep\lib\regex.c

!IF  "$(CFG)" == "SR_FREE - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "SR_FREE - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\grep\lib\savedir.c

!IF  "$(CFG)" == "SR_FREE - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "SR_FREE - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\grep\lib\stpcpy.c

!IF  "$(CFG)" == "SR_FREE - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "SR_FREE - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\grep\lib\strtol.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\grep\lib\strtoul.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\grep\lib\strtoull.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\grep\lib\strtoumax.c

!IF  "$(CFG)" == "SR_FREE - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "SR_FREE - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\grep\lib\xmalloc.c

!IF  "$(CFG)" == "SR_FREE - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "SR_FREE - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\grep\lib\xstrtol.c

!IF  "$(CFG)" == "SR_FREE - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "SR_FREE - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\grep\lib\xstrtoumax.c

!IF  "$(CFG)" == "SR_FREE - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "SR_FREE - Win32 Debug"

!ENDIF 

# End Source File
# End Group
# Begin Source File

SOURCE=.\grep\dfa.c

!IF  "$(CFG)" == "SR_FREE - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "SR_FREE - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\grep\grep.c

!IF  "$(CFG)" == "SR_FREE - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "SR_FREE - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\grep\grep_search.c

!IF  "$(CFG)" == "SR_FREE - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "SR_FREE - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\grep\grepmat.c

!IF  "$(CFG)" == "SR_FREE - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "SR_FREE - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\grep\kwset.c

!IF  "$(CFG)" == "SR_FREE - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "SR_FREE - Win32 Debug"

!ENDIF 

# End Source File
# End Group
# Begin Source File

SOURCE=.\custom_text_dlg.c
# End Source File
# Begin Source File

SOURCE=.\debug_print.c
# End Source File
# Begin Source File

SOURCE=.\dragdrop.c
# End Source File
# Begin Source File

SOURCE=.\favorites.c
# End Source File
# Begin Source File

SOURCE=.\ini_file.c
# End Source File
# Begin Source File

SOURCE=.\listbox_draw.c
# End Source File
# Begin Source File

SOURCE=.\options.c
# End Source File
# Begin Source File

SOURCE=.\replace.c
# End Source File
# Begin Source File

SOURCE=.\resize.c
# End Source File
# Begin Source File

SOURCE=.\search.c
# End Source File
# Begin Source File

SOURCE=.\SR_FREE.c
# End Source File
# Begin Source File

SOURCE=.\string_stuff.c
# End Source File
# Begin Source File

SOURCE=.\view_context.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\pragma.h
# End Source File
# Begin Source File

SOURCE=.\ram_ini_file.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\img_collapsed.bmp
# End Source File
# Begin Source File

SOURCE=.\img_continue.bmp
# End Source File
# Begin Source File

SOURCE=.\img_empty.bmp
# End Source File
# Begin Source File

SOURCE=.\img_expanded.bmp
# End Source File
# Begin Source File

SOURCE=.\img_found.bmp
# End Source File
# Begin Source File

SOURCE=.\img_line.bmp
# End Source File
# Begin Source File

SOURCE=.\resource.rc
# End Source File
# Begin Source File

SOURCE=".\search-b-icon64.ico"
# End Source File
# End Group
# End Target
# End Project
