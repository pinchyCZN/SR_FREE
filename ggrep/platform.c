/************************************************************************/
/*                                                                      */
/*      Platform -- Handle platform-specific functions (Linux version)  */
/*                                                                      */
/*      This module is part of behoffski's attempt to provide           */
/*      utterly portable sources using link-time or run-time binding    */
/*      instead of using the preprocessor at compile time.              */
/*                                                                      */
/*      This version uses ANSI escape sequences to select               */
/*      highlighting attributes, similar to (but less sophisticated     */
/*      than) the "colorized" ls.  However, in common with              */
/*      ls, it may leave the terminal with incorrect colour             */
/*      settings if output is interrupted while displaying              */
/*      highlighted text.                                               */
/*                                                                      */
/*      Copyright (C) 1995-2000 Grouse Software.  All rights reserved.  */
/*      Written for Grouse by behoffski (Brenton Hoff).                 */
/*                                                                      */
/*      Free software: no warranty; use anywhere is ok; spread the      */
/*      sources; note any mods; share variations and derivatives        */
/*      (including sending to behoffski@grouse.com.au).                 */
/*                                                                      */
/************************************************************************/

#include "ascii.h"
#include "bakslash.h"
#include "compdef.h"
#include "fastfile.h"
#include "ggrep.h"
#include "main.h"
#include "matchgcc.h"
#include "matcheng.h"
#include "platform.h"
#include "regexp00.h"
#include "retable.h"
#include "scanfile.h"
#include "stbm.h"
#include "stbmshim.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tbldisp.h"
#include "tracery.h"
#include <unistd.h>

/*Small memory pool parameters*/

#define PLATFORM_SMALL_POOL_SIZE        16384 /*bytes*/
#define PLATFORM_ALIGN_SIZE             32

/*Hacky console escapes for highlighting*/
#define HIGHLIGHT_ON()                  printf("\033[0;47;1;36m")
#define HIGHLIGHT_OFF()                 printf("\033[m")

typedef struct {
        /*Flag indicating whether output can comprehend ANSI colours*/
        BOOL ANSIColours;
        CHAR *pProgName;

        /*Small memory pool to optimise malloc calls*/
        void *pSmallPool;
        UINT PoolSpaceLeft;

} PLATFORM_MODULE_CONTEXT;

module_scope PLATFORM_MODULE_CONTEXT gPlatform;


/************************************************************************/
/*                                                                      */
/*      Display -- Display matching line                                */
/*                                                                      */
/************************************************************************/
module_scope BOOL
Platform_Display(MatchEng_Details *pDetails)
{
        UINT32 ByteOffset;

        /*Display extra identifying info requested by client*/
        if (pDetails->ReportingOptions & MATCHENG_RPT_FILENAME) {
                fputs(pDetails->pFilename, stdout);
                fputc(':', stdout);
        }
        if (pDetails->ReportingOptions & MATCHENG_RPT_LINENUMBER) {
                fprintf(stdout, "%lu:", pDetails->LineNr);
        }
        if (pDetails->ReportingOptions & MATCHENG_RPT_BYTEOFFSET) {
                /*Work out and report match line's byte offset*/
                ByteOffset = pDetails->BufferOffset + 
                        (pDetails->pLineStart - pDetails->pBufferStart);
                fprintf(stdout, "%lu:", ByteOffset);

        }

        /*Write line (plus trailing LF) to stdout as binary data*/
        fwrite(pDetails->pLineStart, 
               1,
               pDetails->pLineEnd - pDetails->pLineStart, 
               stdout);

        /*Tell search engine to keep going*/
        return TRUE;

} /*Display*/


/************************************************************************/
/*                                                                      */
/*      DisplayHighlighted -- Display line, highlighting matching text  */
/*                                                                      */
/************************************************************************/
module_scope BOOL
Platform_DisplayHighlighted(MatchEng_Details *pDetails)
{
        UINT32 ByteOffset;

        /*Display extra identifying info requested by client*/
        if (pDetails->ReportingOptions & MATCHENG_RPT_FILENAME) {
                fputs(pDetails->pFilename, stdout);
                putchar(':');
        }
        if (pDetails->ReportingOptions & MATCHENG_RPT_LINENUMBER) {
                /*Report total line count*/
                printf("%lu:", pDetails->LineNr);
        }
        if (pDetails->ReportingOptions & MATCHENG_RPT_BYTEOFFSET) {
                /*Work out and report match line's byte offset*/
                ByteOffset = pDetails->BufferOffset + 
                        (pDetails->pLineStart - pDetails->pBufferStart);
                fprintf(stdout, "%lu:", ByteOffset);
        }

        if (pDetails->ReportingOptions & MATCHENG_RPT_MARKER_FLAG) {
                /*Display line up to beginning of match with marker*/
                fputc(pDetails->MarkerChar, stdout);
                fwrite(pDetails->pLineStart, 
                       1, 
                       pDetails->pMatchStart - pDetails->pLineStart, 
                       stdout);

                /*?? Should encode binary characters*/

                /*Write matching part of line with markers*/
                fputc(pDetails->MarkerChar, stdout);
                fwrite(pDetails->pMatchStart, 
                       1, 
                       pDetails->pMatchEnd - pDetails->pMatchStart, 
                       stdout);
                fputc(pDetails->MarkerChar, stdout);

                /*Write remainder of line (include delimiter if appropriate)*/
                fwrite(pDetails->pMatchEnd, 
                       1,
                       pDetails->pLineEnd - pDetails->pMatchEnd, 
                       stdout);

        } else {
                /*Display line up to beginning of match*/
                fwrite(pDetails->pLineStart, 
                       1,
                       pDetails->pMatchStart - pDetails->pLineStart, 
                       stdout);

                /*Write matching part of line with highlighting*/
                if (gPlatform.ANSIColours) {
                        HIGHLIGHT_ON();
                }
                fwrite(pDetails->pMatchStart, 
                       1,
                       pDetails->pMatchEnd - pDetails->pMatchStart, 
                       stdout);
                if (gPlatform.ANSIColours) {
                        HIGHLIGHT_OFF();
                }

                /*Write remainder of line (include delimiter if appropriate)*/
                fwrite(pDetails->pMatchEnd, 
                       1,
                       pDetails->pLineEnd - pDetails->pMatchEnd + 1, 
                       stdout);

        }

        /*Tell search engine to keep going*/
        return TRUE;

} /*DisplayHighlighted*/


/************************************************************************/
/*                                                                      */
/*      DisplayFilename -- Display file name and match details          */
/*                                                                      */
/************************************************************************/
module_scope BOOL
Platform_DisplayFilename(MatchEng_Details *pDetails)
{

        /*Has filename output been selected?*/
        if (pDetails->ReportingOptions & MATCHENG_RPT_FILENAME) {
                /*Yes, is line counting enabled as well?*/
                if (pDetails->ReportingOptions & MATCHENG_RPT_LINECOUNT) {
                        /*Yes, report it now as well*/
                        printf("%s:%lu\n", pDetails->pFilename,
                                         pDetails->LineMatchCount);

                } else {
                        /*No, report file name only (plus LF)*/
                        printf("%s\n", pDetails->pFilename);

                }

        } else {
                /*Caller must want line count display without filename*/
                printf("%lu\n", pDetails->LineMatchCount);

        }

        /*Search should abandon this file as it has been reported*/
        return FALSE;

} /*DisplayFilename*/


/************************************************************************/
/*                                                                      */
/*      main -- Set up platform-specific things before invoking program */
/*                                                                      */
/************************************************************************/
public_scope int
main(int argc, char **argv)
{
        int Result;
        char *pProg;

        /*Phase 1: Prepare module internals without referencing others*/
        gPlatform.ANSIColours = FALSE;

        /*Was the program name supplied in the argument list?*/
        gPlatform.pProgName = "ggrep";
        pProg = argv[0];
        if (pProg != NULL) {
                /*Yes, use the filename (last) component of the name*/
                gPlatform.pProgName = pProg;
                pProg = strrchr(pProg, '/');
                if (pProg != NULL) {
                        gPlatform.pProgName = pProg + 1;
                }
        }

        /*Initialise the small malloc pool*/
        gPlatform.pSmallPool = malloc(PLATFORM_SMALL_POOL_SIZE);
        if (gPlatform.pSmallPool == NULL) {
                /*Sorry, not enough memory to run*/
                fprintf(stderr, "?? Unable to allocate small pool!");
                return MAIN_RETURN_FAULT;
        }
        gPlatform.PoolSpaceLeft = PLATFORM_SMALL_POOL_SIZE;

        /*Basic initialisation of each module*/
        Bakslash_Init();
        FastFile_Init();
        GGrep_Init();
        MatchGCC_Init();
        RegExp_Init();
        RETable_Init();
        ScanFile_Init();
        STBM_Init();
        STBMShim_Init();
        Tracery_Init();

        /*Phase 2: Notify other modules of existence as appropriate*/
        /*         Also acquire permanent resources*/

        /*Start modules going (get resources and chat to other modules)*/
        if (! RETable_Start()) {
                fprintf(stderr, "%s: Unable to start RETable!\n", 
                        gPlatform.pProgName);
                return MAIN_RETURN_FAULT;
        }

        /*Set up ScanFile for operation and acquire memory*/
        if (! ScanFile_Start()) 
        {
                /*Sorry, ScanFile/FastFile was unable to prepare*/
                fprintf(stderr, "%s: Not enough memory to scan input!\n",
                                gPlatform.pProgName);
                return MAIN_RETURN_FAULT;
        }

        /*Specify functions to display matches and/or filenames*/
        ScanFile_OutputFunctions(Platform_Display,
                                 Platform_DisplayHighlighted,
                                 Platform_DisplayFilename);

        /*We can only use GNU C search engine (faster but non-ANSI)*/
        ScanFile_MatchFunction(MatchGCC_Match);

        /*Acquire entry points for actions to use in state tables*/
        MatchGCC_Match((MatchEng_Spec *) &gMatchEngAct, NULL, NULL);

        /*Inform table display facility of action values*/
        TblDisp_PrepareLabels();

        /*Find out whether output is likely to want to see ANSI escape codes*/
        gPlatform.ANSIColours = isatty(fileno(stdout));

#ifdef TRACERY_ENABLED
        /*Create Tracery entries for all modules*/
        Tracery_Register("s", "Scanner",  NULL, ScanFile_TraceryLink);
        Tracery_Register("m", "Matcher",  NULL, MatchGCC_TraceryLink);
        Tracery_Register("f", "FastFile", NULL, FastFile_TraceryLink);
#endif /*TRACERY_ENABLED*/

        /*Phase 3: Analyse request and configure everything (see ggrep)*/
        /*Phase 4: Execute request (see ggrep)*/

        /*Invoke main routine and report return code*/
        Result = GGrep_main(argc, argv);
        return Result;

} /*main*/


/************************************************************************/
/*                                                                      */
/*      ProgramName -- Report the name used to invoke us                */
/*                                                                      */
/*      Reports the program name, based on the non-directory bits       */
/*      of the filename reported by argv[0].                            */
/*                                                                      */
/************************************************************************/
public_scope CHAR *
Platform_ProgramName(void)
{
        /*Merely report what main() recorded*/
        return gPlatform.pProgName;

} /*ProgramName*/


/************************************************************************/
/*                                                                      */
/*      SmallMalloc -- Optimised malloc for small areas                 */
/*                                                                      */
/*      malloc is a slow system call, and so we should try to           */
/*      minimise the number of times it's called if we're really        */
/*      desperate to optimise for speed.  This interface doles          */
/*      memory from a modest pool, and hands any request larger         */
/*      then the available pool to malloc to handle.  Hopefully         */
/*      this will speed things up somewhat.                             */
/*                                                                      */
/************************************************************************/
public_scope void *
Platform_SmallMalloc(size_t size)
{
        void *pPool;

        /*Round the request size up to the next alignment boundary*/
        size += PLATFORM_ALIGN_SIZE + 1;
        size -= size % PLATFORM_ALIGN_SIZE;

        /*Is this request larger than the available pool?*/
        if (size > gPlatform.PoolSpaceLeft) {
                /*Yes, use malloc to serve the request*/
                return malloc(size);
        }

        /*Okay, carve off pool space and report it to caller*/
        pPool = gPlatform.pSmallPool;
        gPlatform.pSmallPool = ((BYTE *) gPlatform.pSmallPool) + size;
        gPlatform.PoolSpaceLeft -= size;

        return pPool;

} /*SmallMalloc*/


/************************************************************************/
/*                                                                      */
/*      SmallFree -- Free space issued by SmallMalloc                   */
/*                                                                      */
/************************************************************************/
public_scope void
Platform_SmallFree(void *pMem)
{
        /*Sorry, we can't free at this stage*/
} /*SmallFree*/

