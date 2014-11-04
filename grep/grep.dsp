# Microsoft Developer Studio Project File - Name="grep" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 60000
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=grep - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "grep.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "grep.mak" CFG="grep - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "grep - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "grep - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "grep - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /I ".\lib\\" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "HAVE_CONFIG_H" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib  kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib  kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386

!ELSEIF  "$(CFG)" == "grep - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ  /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /I ".\\" /I ".\lib\\" /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "HAVE_CONFIG_H" /FR /YX /FD /GZ  /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib  kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib  kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "grep - Win32 Release"
# Name "grep - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Group "lib"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\lib\alloca.c

!IF  "$(CFG)" == "grep - Win32 Release"

!ELSEIF  "$(CFG)" == "grep - Win32 Debug"

# ADD CPP /I ".\lib\\" /D "HAVE_CONFIG_H"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\lib\atexit.c

!IF  "$(CFG)" == "grep - Win32 Release"

!ELSEIF  "$(CFG)" == "grep - Win32 Debug"

# ADD CPP /I ".\lib\\" /D "HAVE_CONFIG_H"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\lib\closeout.c

!IF  "$(CFG)" == "grep - Win32 Release"

!ELSEIF  "$(CFG)" == "grep - Win32 Debug"

# ADD CPP /I ".\lib\\" /D "HAVE_CONFIG_H"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\lib\error.c

!IF  "$(CFG)" == "grep - Win32 Release"

!ELSEIF  "$(CFG)" == "grep - Win32 Debug"

# ADD CPP /I ".\lib\\" /D "HAVE_CONFIG_H"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\lib\exclude.c

!IF  "$(CFG)" == "grep - Win32 Release"

!ELSEIF  "$(CFG)" == "grep - Win32 Debug"

# ADD CPP /I ".\lib\\" /D "HAVE_CONFIG_H"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\lib\fnmatch.c

!IF  "$(CFG)" == "grep - Win32 Release"

!ELSEIF  "$(CFG)" == "grep - Win32 Debug"

# ADD CPP /I ".\lib\\" /D "HAVE_CONFIG_H"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\lib\getopt.c

!IF  "$(CFG)" == "grep - Win32 Release"

!ELSEIF  "$(CFG)" == "grep - Win32 Debug"

# ADD CPP /I ".\lib\\" /D "HAVE_CONFIG_H"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\lib\getopt1.c

!IF  "$(CFG)" == "grep - Win32 Release"

!ELSEIF  "$(CFG)" == "grep - Win32 Debug"

# ADD CPP /I ".\lib\\" /D "HAVE_CONFIG_H"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=".\lib\hard-locale.c"

!IF  "$(CFG)" == "grep - Win32 Release"

!ELSEIF  "$(CFG)" == "grep - Win32 Debug"

# ADD CPP /I ".\lib\\" /D "HAVE_CONFIG_H"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\lib\isdir.c

!IF  "$(CFG)" == "grep - Win32 Release"

!ELSEIF  "$(CFG)" == "grep - Win32 Debug"

# ADD CPP /I ".\lib\\" /D "HAVE_CONFIG_H"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\lib\malloc.c

!IF  "$(CFG)" == "grep - Win32 Release"

!ELSEIF  "$(CFG)" == "grep - Win32 Debug"

# ADD CPP /I ".\lib\\" /D "HAVE_CONFIG_H"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\lib\memchr.c

!IF  "$(CFG)" == "grep - Win32 Release"

!ELSEIF  "$(CFG)" == "grep - Win32 Debug"

# ADD CPP /I ".\lib\\" /D "HAVE_CONFIG_H"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\lib\obstack.c

!IF  "$(CFG)" == "grep - Win32 Release"

!ELSEIF  "$(CFG)" == "grep - Win32 Debug"

# ADD CPP /I ".\lib\\" /D "HAVE_CONFIG_H"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\lib\quotearg.c

!IF  "$(CFG)" == "grep - Win32 Release"

!ELSEIF  "$(CFG)" == "grep - Win32 Debug"

# PROP Exclude_From_Build 1
# ADD CPP /I ".\lib\\" /D "HAVE_CONFIG_H"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\lib\realloc.c

!IF  "$(CFG)" == "grep - Win32 Release"

!ELSEIF  "$(CFG)" == "grep - Win32 Debug"

# ADD CPP /I ".\lib\\" /D "HAVE_CONFIG_H"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\lib\regex.c

!IF  "$(CFG)" == "grep - Win32 Release"

!ELSEIF  "$(CFG)" == "grep - Win32 Debug"

# ADD CPP /I ".\lib\\" /D "HAVE_CONFIG_H"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\lib\savedir.c

!IF  "$(CFG)" == "grep - Win32 Release"

!ELSEIF  "$(CFG)" == "grep - Win32 Debug"

# ADD CPP /I ".\lib\\" /D "HAVE_CONFIG_H"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\lib\stpcpy.c

!IF  "$(CFG)" == "grep - Win32 Release"

!ELSEIF  "$(CFG)" == "grep - Win32 Debug"

# ADD CPP /I ".\lib\\" /D "HAVE_CONFIG_H"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\lib\strtol.c

!IF  "$(CFG)" == "grep - Win32 Release"

!ELSEIF  "$(CFG)" == "grep - Win32 Debug"

# PROP Exclude_From_Build 1
# ADD CPP /I ".\lib\\" /D "HAVE_CONFIG_H"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\lib\strtoul.c

!IF  "$(CFG)" == "grep - Win32 Release"

!ELSEIF  "$(CFG)" == "grep - Win32 Debug"

# PROP Exclude_From_Build 1
# ADD CPP /I ".\lib\\" /D "HAVE_CONFIG_H"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\lib\strtoull.c

!IF  "$(CFG)" == "grep - Win32 Release"

!ELSEIF  "$(CFG)" == "grep - Win32 Debug"

# PROP Exclude_From_Build 1
# ADD CPP /I ".\lib\\" /D "HAVE_CONFIG_H"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\lib\strtoumax.c

!IF  "$(CFG)" == "grep - Win32 Release"

!ELSEIF  "$(CFG)" == "grep - Win32 Debug"

# ADD CPP /I ".\lib\\" /D "HAVE_CONFIG_H"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\lib\xmalloc.c

!IF  "$(CFG)" == "grep - Win32 Release"

!ELSEIF  "$(CFG)" == "grep - Win32 Debug"

# ADD CPP /I ".\lib\\" /D "HAVE_CONFIG_H"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\lib\xstrtol.c

!IF  "$(CFG)" == "grep - Win32 Release"

!ELSEIF  "$(CFG)" == "grep - Win32 Debug"

# ADD CPP /I ".\lib\\" /D "HAVE_CONFIG_H"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\lib\xstrtoumax.c

!IF  "$(CFG)" == "grep - Win32 Release"

!ELSEIF  "$(CFG)" == "grep - Win32 Debug"

# ADD CPP /I ".\lib\\" /D "HAVE_CONFIG_H"

!ENDIF 

# End Source File
# End Group
# Begin Source File

SOURCE=.\_search.c
# End Source File
# Begin Source File

SOURCE=.\dfa.c
# End Source File
# Begin Source File

SOURCE=.\grep.c
# End Source File
# Begin Source File

SOURCE=.\grepmat.c
# End Source File
# Begin Source File

SOURCE=.\kwset.c
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
