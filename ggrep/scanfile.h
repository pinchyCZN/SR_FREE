/************************************************************************/
/*                                                                      */
/*      ScanFile -- Manage RE search for a single file                  */
/*                                                                      */
/*      This module accepts the regular expression to be used           */
/*      plus various search and display options, and performs           */
/*      the requested search on each file given.  It uses LineFile      */
/*      to bring the file in a suitably-edited memory buffer,           */
/*      and RETable to execute the search on each buffer.               */
/*                                                                      */
/*      Copyright (C) 1995-2000 Grouse Software.  All rights reserved.  */
/*      Written for Grouse by behoffski (Brenton Hoff).                 */
/*                                                                      */
/*      Free software: no warranty; use anywhere is ok; spread the      */
/*      sources; note any mods; share variations and derivatives        */
/*      (including sending to behoffski@grouse.com.au).                 */
/*                                                                      */
/************************************************************************/

#ifndef SCANFILE_H
#define SCANFILE_H

#include <compdef.h>
#include "matcheng.h"
#include "regexp00.h"
#include "tracery.h"

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/

/*Configuration options*/
#define SCANFILE_NUMBER_MATCHING_LINES  BIT0
#define SCANFILE_NUMBER_NONMATCH_LINES  BIT1
#define SCANFILE_MATCH_WORDS            BIT2
#define SCANFILE_DEBUG_DISPLAY          BIT3
#define SCANFILE_OPT_APPROXIMATE        BIT4
#define SCANFILE_OPT_EASIEST_FIRST      BIT5
/* #define SCANFILE_OPT_BUFFER             BIT6 -- now always use buffer*/
#define SCANFILE_OPT_SKIP               BIT7
#define SCANFILE_CR_IS_TERMINATOR       BIT8
#define SCANFILE_OPT_SELF_TUNED_BM      BIT9
#define SCANFILE_DEBUG_COMPILED         BIT10 /*Replace with Tracery?*/
#define SCANFILE_OPT_CR_IS_TERMINATOR   BIT11
#define SCANFILE_OPT_RECURSE_DIRECTORIES BIT12
#define SCANFILE_OPT_TUNED_BM           BIT13


/************************************************************************/
/*                                                                      */
/*      Init -- Prepare module for operation                            */
/*                                                                      */
/************************************************************************/
void
ScanFile_Init(void);

/************************************************************************/
/*                                                                      */
/*      Start -- Begin managing what has to be managed                  */
/*                                                                      */
/************************************************************************/
BOOL
ScanFile_Start(void);


/************************************************************************/
/*                                                                      */
/*      OutputFunctions -- Specify functions to perform match output    */
/*                                                                      */
/*      In order to keep ScanFile as portable as possible, the          */
/*      match and filename display functions are provided by            */
/*      an external party, since generation and display of              */
/*      output (and especially highlighting) is platform-specific.      */
/*                                                                      */
/*      This function must be called after Init but before any          */
/*      RE specification or module configuration.                       */
/*                                                                      */
/************************************************************************/
void
ScanFile_OutputFunctions(MatchEng_SelectFunction *pNormal,
                         MatchEng_SelectFunction *pHighlight,
                         MatchEng_SelectFunction *pFilenameOnly);


/************************************************************************/
/*                                                                      */
/*      MatchFunction -- Define routine to perform match                */
/*                                                                      */
/*      ScanFile wishes to provide very high-performance searches       */
/*      but do so in a very portable fashion.  This function is the     */
/*      result: ScanFile receives the address of the function that      */
/*      implements the match from an outsider (usually Platform).       */
/*      This function must be called before Pattern.                    */
/*                                                                      */
/************************************************************************/
void
ScanFile_MatchFunction(MatchEng_MatchFunction pMatchFunc);


/************************************************************************/
/*                                                                      */
/*      Pattern -- Specify RE to be searched                            */
/*                                                                      */
/*      pPattern is the "compiled" version created by RegExp.           */
/*      ScanOptions allows modifications to the pattern such as         */
/*      case insensitivity, word match and inverted match sense         */
/*      to be specified.                                                */
/*                                                                      */
/*      Pattern expands the RE into a version optimised for speed,      */
/*      and return FALSE if it is unable to handle the RE               */
/*      (for example, if it runs out of RAM).                           */
/*                                                                      */
/*      This function takes control of the RE specification             */
/*      supplied and deletes it when it is no longer required.          */
/*                                                                      */
/************************************************************************/
BOOL
ScanFile_Pattern(RegExp_Specification *pPattern, LWORD ScanOptions);


/************************************************************************/
/*                                                                      */
/*      Configure -- Define how the module searches and reports matches */
/*                                                                      */
/*      ReportingOptions defines the way matching lines are displayed.  */
/*                                                                      */
/************************************************************************/
BOOL
ScanFile_Configure(LWORD ReportingOptions);


/************************************************************************/
/*                                                                      */
/*      Search -- Perform specified search on a file                    */
/*                                                                      */
/*      This function searches the specified file (or stdin if          */
/*      pFilename is NULL) using the search options specified           */
/*      by Configure.  The function returns FALSE if the search         */
/*      has determined that there's no benefit in examining any         */
/*      more files.                                                     */
/*                                                                      */
/*      Parameter pParent is used for recursive searches, so that       */
/*      circular loops in the directory heirarchy can be detected       */
/*      and avoided.  External callers must specify NULL for this       */
/*      parameter.                                                      */
/*                                                                      */
/************************************************************************/
BOOL
ScanFile_Search(CHAR *pFilename, void *pParent);


/************************************************************************/
/*                                                                      */
/*      MatchedAny -- Report if any files matched search criteria       */
/*                                                                      */
/************************************************************************/
BOOL
ScanFile_MatchedAny(void);


#ifdef TRACERY_ENABLED
/************************************************************************/
/*                                                                      */
/*      TraceryLink -- Tell Tracery how to deal with us                 */
/*                                                                      */
/*      This procedure is used by Tracery to find out how to            */
/*      manipulate the trace flags for this module and/or object.       */
/*      The platform should be able to hand this routine to             */
/*      Tracery when setting up the system without needing to           */
/*      know too many details about how the traces are to be            */
/*      set up.                                                         */
/*                                                                      */
/*      This function may be used to get the flags for the              */
/*      module, or for any object created by the module.                */
/*      If the pObject parameter is NULL, the module information        */
/*      is returned; otherwise, the object's info is returned.          */
/*      Currently we report our flag register, our preferred            */
/*      set of default flags, and a list of edit specifiers and         */
/*      bits to edit in the flag register.  In the future this          */
/*      may change: Tracery is still rather tentative.                  */
/*                                                                      */
/************************************************************************/
BOOL
ScanFile_TraceryLink(void *pObject, UINT Opcode, ...);

#endif /*TRACERY_ENABLED*/


#ifdef __cplusplus
}
#endif /*__cplusplus*/

#endif /*SCANFILE_H*/
