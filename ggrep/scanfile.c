/************************************************************************/
/*                                                                      */
/*      ScanFile -- Manage RE search for a single file                  */
/*                                                                      */
/*      This module accepts the regular expression to be used           */
/*      plus various search and display options, and performs           */
/*      the requested search on each file given.  It uses FastFile      */
/*      to bring the file into a memory buffer in a line-oriented       */
/*      fashion, and RETable and/or the self-tuned Boyer-Moore          */
/*      search algorithms to implement the search on each buffer.       */
/*                                                                      */
/*      The search effort is partitioned into a fast file scan          */
/*      search and a slower match portion.  This division is            */
/*      made so that the file may be searched with the least            */
/*      effort.  However, the decision on what's appropriate            */
/*      as the scan RE is partially dependent on the nature of          */
/*      the file being searched: we may perform fairly poorly           */
/*      if our guesses are wrong.                                       */
/*                                                                      */
/*      File offsets are handled by 32-bit integers, which is           */
/*      inadequate for really big files.                                */
/*                                                                      */
/*      The whole structure of this module, which in some               */
/*      distorted way extends to include matcheng.h, is a bit of        */
/*      a mess.  It needs to be split into smaller and more             */
/*      coherent pieces, but exactly how isn't clear.                   */
/*                                                                      */
/*      Another problem is that while almost everything else in         */
/*      ggrep is reentrant, this module most certainly isn't.           */
/*                                                                      */
/*      Copyright (C) Grouse Software 1995-2000.  All rights reserved.  */
/*      Written for Grouse by behoffski (Brenton Hoff).                 */
/*                                                                      */
/*      Free software: no warranty; use anywhere is ok; spread the      */
/*      sources; note any mods; share variations and derivatives        */
/*      (including sending to behoffski@grouse.com.au).                 */
/*                                                                      */
/************************************************************************/

#include "ascii.h"
#include <compdef.h>
#include <dirent.h>
#include <errno.h>
#include "fastfile.h"
#include "main.h"
#include "matcheng.h"
#include "memrchr.h"
#include <memory.h>
#include "platform.h"
#include "retable.h"
#include "scanfile.h"
#include "stbm.h"
#include "stbmshim.h"
#include <stdio.h>
#include <sys/types.h>
#include "tbldisp.h"
#include "tracery.h"
#include <stdarg.h>

/*Parameters for buffering file into lines*/
#define FILE_BUFFER_SIZE                (4096uL * 14)
#define BYTES_BEFORE_BUFFER             8
#define BYTES_AFTER_BUFFER              (64 + 4)
#define SCANFILE_DIR_NAME_SIZE_DEFAULT  16384

/*Note: BYTES_AFTER_BUFFER must be >= BOYER_MOORE_LOOKAHEAD_MAX*/

/*Use behoffski's favourite byte value as an endmarker*/
#define SCANFILE_ENDMARKER_DEFAULT      0xee

/*File stats plus parent pointer so we may search for recursion loops*/
typedef struct {
        void *pParent;
        struct stat stat;
} ScanFile_Stats;


typedef BOOL (FILE_SCANNER)(void);

typedef struct {
        /*------------Variables controlling matching each buffer-----------*/

        /*Tracery control block for this module*/
        Tracery_ObjectInfo TraceInfo;

        /*Function+context for fast buffer scanning*/
        MatchEng_MatchFunction pScan;
        MatchEng_Spec *pScanContext;

        /*Search/match context shared between modules (used by fast scan)*/
        MatchEng_Details Details;

        /*Function+context for completing matching once scan text found*/
        MatchEng_MatchFunction pMatch;
        MatchEng_Spec *pMatchContext;

        /*Duplicate context used by slower match attempts*/
        MatchEng_Details Details2;

        /*Function to handle lines selected by search*/
        MatchEng_SelectFunction *pSelect;

        /*Match sense -- line selection may be inverted by caller*/
        BOOL SelectMatchingLines;

        /*Flags recording if any lines matched overall and for current file*/
        BOOL MatchedAny;

        /*Flag indicating whether inverted blocks need to be unpacked*/
        BOOL UnpackBlocks;

        /*Flag indicating whether to recurse directories*/
        BOOL RecurseDir;

        /*Flag naming if we want to find the line start*/
        BOOL FindLineStart;

        /*------------------File buffer conditioning---------------*/

        /*FastFile file handle*/
        FastFile_Context *pHandle;

        /*Variables for conditioning start of memory buffer*/
        CHAR PrecedingLF;

        /*Memory specifying bytes after buffer to optimise search*/
        UINT EndLength;
        CHAR EndBytes[BYTES_AFTER_BUFFER];

        /*------------Treely-ruly-module-related variables--------------*/

        /*Platform-specific functions to display matches*/
        MatchEng_SelectFunction *pNormalOut;
        MatchEng_SelectFunction *pHighlightOut;
        MatchEng_SelectFunction *pFilenameOut;

        /*RE match function provided by client*/
        MatchEng_MatchFunction pExternMatchFunc;

        /*Debugging options*/
        LWORD Debug;

} SCANFILE_MODULE_CONTEXT;

module_scope SCANFILE_MODULE_CONTEXT gScanFile;

/*Extra information for Tracery operation*/

#ifdef TRACERY_ENABLED
#define TRACERY_MODULE_INFO             (gScanFile.TraceInfo)

/*Debugging/tracing flags*/

#define SCANFILE_T_BUFFER               BIT0
#define SCANFILE_T_SCAN                 BIT1
#define SCANFILE_T_MATCH                BIT2
#define SCANFILE_T_DIR                  BIT3

module_scope Tracery_EditEntry gScanFile_TraceryEditDefs[] = {
        {"B", SCANFILE_T_BUFFER, SCANFILE_T_BUFFER, "Trace  buffer"}, 
        {"b", SCANFILE_T_BUFFER, 0x00,              "Ignore buffer"}, 
        {"S", SCANFILE_T_SCAN,   SCANFILE_T_SCAN,   "Trace  scanner"}, 
        {"s", SCANFILE_T_SCAN,   0x00,              "Ignore scanner"}, 
        {"M", SCANFILE_T_MATCH,  SCANFILE_T_MATCH,  "Trace  matcher"}, 
        {"m", SCANFILE_T_MATCH,  0x00,              "Ignore matcher"}, 
        {"D", SCANFILE_T_DIR,    SCANFILE_T_DIR,    "Trace  directory"}, 
        {"d", SCANFILE_T_DIR,    0x00,              "Ignore directory"}, 
        TRACERY_EDIT_LIST_END
};

#endif /*TRACERY_ENABLED*/


/************************************************************************/
/*                                                                      */
/*      Start -- Begin managing what has to be managed                  */
/*                                                                      */
/************************************************************************/
public_scope BOOL
ScanFile_Start(void)
{
        /*Make sure FastFile starts first*/
        if (! FastFile_Start(FILE_BUFFER_SIZE)) {
                return FALSE;
        }

        return TRUE;

} /*Start*/


/************************************************************************/
/*                                                                      */
/*      NewScanContext -- Prepare blank scan context block              */
/*                                                                      */
/*      Typically we get out scan context from RETable, as we set       */
/*      to use the table-driven architecture.  However, in some         */
/*      cases we use an alterative scan engine (e.g. STBM).             */
/*      This function provides a basic scan context block for           */
/*      alternate searches to use.                                      */
/*                                                                      */
/*      The whole implementation of scan context is rather klunky       */
/*      and would benefit from a careful restructuring.                 */
/*                                                                      */
/************************************************************************/
module_scope BOOL
ScanFile_NewScanContext(MatchEng_Spec **ppScanContext)
{
        MatchEng_Spec *pScanContext;

        /*Destroy return arguments to reduce chance of being misunderstood*/
        *ppScanContext = (MatchEng_Spec *) NULL;

        /*Acquire memory to store context*/
        pScanContext = (MatchEng_Spec *) 
                   Platform_SmallMalloc(sizeof(*pScanContext));
        if (pScanContext == NULL) {
                /*Sorry, no memory available*/
                return FALSE;
        }

        /*Okay, set up reasonable defaults for context block*/
        /* ?? */
        memset(pScanContext, 0, sizeof(*pScanContext));

        /*Created context, write to caller and report success*/
        *ppScanContext = pScanContext;
        return TRUE;

} /*NewScanContext*/


/************************************************************************/
/*                                                                      */
/*      MatchedAbandon -- Halt search if matching line found            */
/*                                                                      */
/*      This function is used for the -L search option.                 */
/*                                                                      */
/************************************************************************/
module_scope BOOL
ScanFile_MatchedAbandon(MatchEng_Details *pDetails)
{
        /*Search can abandon current file*/
        return FALSE;

} /*MatchedAbandon*/


/************************************************************************/
/*                                                                      */
/*      Open -- Prepare file for scanning                               */
/*                                                                      */
/************************************************************************/
module_scope BOOL 
ScanFile_Open(CHAR *pFilename, struct stat *pStat)
{
        BOOL FirstFile;
        BOOL Opened;

        /*Is this the second or later file to be searched?*/
        FirstFile = TRUE;
        if (gScanFile.pHandle != NULL) {
                /*Yes, remember this for later*/
                FirstFile = FALSE;
        }

        /*Is this the first file being searched?*/
        if (FirstFile) {
                /*Yes, open a new FastFile handle*/
                Opened = FastFile_Open(pFilename, 
                                       FILE_BUFFER_SIZE, 
                                       FASTFILE_P_MODE_LINE, 
                                       pStat, 
                                       &gScanFile.pHandle);

        } else {
                /*No, reuse existing handle -- it's faster*/
                Opened = FastFile_Reopen(gScanFile.pHandle, pFilename, pStat);

        }

        /*Did we succeed?*/
        if (! Opened) {
                /*No, unable to open file*/
                fprintf(stderr, "%s: %s: %s\n",
                                Platform_ProgramName(), 
                                pFilename, 
				strerror(errno));

                /*Record problem for exit value reporting*/
                Main_ReturnCode(MAIN_RETURN_FAULT);

                /*Skip to next file to process*/
                return FALSE;

        }

        /*Is the filename anything other than stdin?*/
        if (pFilename != NULL) {
                /*Yes, record filename for reporting*/
                gScanFile.Details.pFilename = pFilename;

        } else {
                /*Standard input -- plug in name ourselves*/
                gScanFile.Details.pFilename = "(standard input)";

        }

        /*Is this the second or later file?*/
        if (! FirstFile) {
                /*Yes, handle is already configured, so we're done*/
                return TRUE;
        }

        /*Configure FastFile to reserve space for LF before buffer*/
        if (! FastFile_StartCondition(gScanFile.pHandle, 
                                      BYTES_BEFORE_BUFFER, 
                                      1, &gScanFile.PrecedingLF)) {
                /*Error configuring buffer: not enough memory, perhaps?*/
                fprintf(stderr, "%s: Unable to condition start\n", 
                                Platform_ProgramName());
                Main_ReturnCode(MAIN_RETURN_FAULT);
                return FALSE;
        }

        /*Configure FastFile to prepare end of buffer*/
        switch (gScanFile.pScanContext->EndCondition) {
        case MATCHENG_CONDITION_TRAILING_LITERAL:
                /*Add literal to simplify memory search specification*/
                gScanFile.EndBytes[0] = LF;
                memset(&gScanFile.EndBytes[1], 
                       gScanFile.pScanContext->TrailingLiteral, 
                       gScanFile.pScanContext->PatternLength);
                gScanFile.EndLength = 
                        gScanFile.pScanContext->PatternLength + 1;

                break;

        default:
                gScanFile.EndBytes[0] = LF;
                gScanFile.EndLength = 1;
                break;

        }

        if (! FastFile_EndCondition(gScanFile.pHandle, 
                                    BYTES_AFTER_BUFFER, 
                                    gScanFile.EndLength, 
                                    gScanFile.EndBytes)) {
                /*Error configuring buffer: not enough memory, perhaps?*/
                fprintf(stderr, "%s: Unable to condition end\n", 
                                Platform_ProgramName());
                Main_ReturnCode(MAIN_RETURN_FAULT);
                return FALSE;
        }

        /*Opened successfully*/
        return TRUE;

} /*Open*/


/************************************************************************/
/*                                                                      */
/*      ExpandNames -- Build a list of all files in a directory         */
/*                                                                      */
/*      Prepares a list of all files in the specified directory,        */
/*      with a NUL terminating each name and consecutive NULs           */
/*      (a zero-length string) marking the end of the list. No          */
/*      attempt is made to sort names into alphabetical order.          */
/*                                                                      */
/*      Returns the first name in the list, or NULL if the function     */
/*      was unable to build the list successfully.  The list is         */
/*      allocated out of the heap, so if a pointer is returned,         */
/*      the caller must free the memory to avoid memory leaks.          */
/*                                                                      */
/*      We expect that the caller will want to prepend the              */
/*      directory name, and possibly a trailing slash, to the           */
/*      file, so we add space at the start of the list to allow         */
/*      for this case, and report the start of the prepended area       */
/*      as our return value.  The caller must add                       */
/*      strlen(pDirname) + 1 bytes to the returned pointer to find      */
/*      the first name.  We return this earlier pointer so that         */
/*      the caller can free the memory block correctly.                 */
/*                                                                      */
/************************************************************************/
module_scope CHAR *
ScanFile_ExpandNames(CHAR *pDirname)
{
        DIR *pDir;
        struct dirent *pEntry;
        UINT BlockSize;
        CHAR *pMem;
        CHAR *pBiggerMem;
        CHAR *pFile;
        UINT NameSize;

        BlockSize = SCANFILE_DIR_NAME_SIZE_DEFAULT;

        /*Open the directory for enumeration*/
        pDir = opendir(pDirname);
        if (pDir == NULL) {
                /*Failed to access directory*/
                return NULL;
        }

        /*Acquire initial space to store names*/
        pMem = malloc(BlockSize);
        if (pMem == NULL) {
                /*Sorry, unable to acquire space to store names*/
                closedir(pDir);
                return FALSE;
        }

        /*Add an offset to allow directory name to be prepended*/
        pFile = pMem + strlen(pDirname) + 1;

        /*Work through each entry in the directory*/
        for (;;) {
                /*Read next entry of the file, if any*/
                pEntry = readdir(pDir);
                if (pEntry == NULL) {
                        /*Finished enumerating files*/
                        break;
                }

                /*Skip "." and ".." entries if found*/
                if ((strcmp(pEntry->d_name, ".") == 0) ||
                    (strcmp(pEntry->d_name, "..") == 0)) {
                        continue;
                }

                /*While the name wouldn't fit into the memory block...*/
                NameSize = strlen(pEntry->d_name);
                while ((pFile + NameSize + 2) >= (pMem + BlockSize)) {
                        /*...Allocate a larger block*/
                        BlockSize *= 2;
                        pBiggerMem = realloc(pMem, BlockSize);
                        if (pBiggerMem == NULL) {
                                /*Sorry, ran out of memory to store names*/
                                free(pMem);
                                return FALSE;
                        }

                        /*Change pointers to use newly-acquired space*/
                        pFile = pBiggerMem + (pFile - pMem);
                        pMem = pBiggerMem;
                }

                /*Add the name to the block*/
                memcpy(pFile, pEntry->d_name, NameSize + 1);
                pFile += NameSize + 1;

        }


        /*Terminate the list with a 0-length entry*/
        *pFile++ = NUL;

        /*Okay, report results to caller*/
        return pMem;

} /*ExpandNames*/


/************************************************************************/
/*                                                                      */
/*      RecurseDir -- Enumerate and search files in directory           */
/*                                                                      */
/*      Finds the names of all the files in the specified directory,    */
/*      and executes the search on each file found.  This routine       */
/*      is modelled closely on the directory recursion facility         */
/*      in GNU Grep, including checking for circular references         */
/*      in the directory heirarchy.                                     */
/*                                                                      */
/*      Returns FALSE if multi-file searches are to be skipped.         */
/*                                                                      */
/************************************************************************/
module_scope BOOL
ScanFile_RecurseDir(CHAR *pDirname, ScanFile_Stats *pStats)
{
        ScanFile_Stats *pSearch;
        CHAR *pNames;
        CHAR *pFile;
        UINT SlashSpace;
        UINT DirLen;

        TRACERY(SCANFILE_T_DIR, {
                printf("ScanFile_RecurseDir(%s, ...)\n", pDirname);
        });

        /*Loop through all parent directories of this one*/
        for (pSearch = pStats->pParent; 
             pSearch != NULL; 
             pSearch = pSearch->pParent) {
                /*Does this predecessor match this directory?*/
                if ((pSearch->stat.st_ino == pStats->stat.st_ino) && 
                    (pSearch->stat.st_dev == pStats->stat.st_dev)) {
                        /*Yes, we've detected a loop: Abandon this directory*/
                        return TRUE;
                }
        }

        /*Okay, we haven't encountered a loop*/

        /*Expand the directory into a list of names*/
        pNames = ScanFile_ExpandNames(pDirname);
        if (pNames == NULL) {
                /*Sorry, no memory to list files of this directory*/
                return FALSE;
        }

        /*Prepare to prepend name (and optional slash) to each filename*/
        SlashSpace = 0;
        DirLen = strlen(pDirname);
        if (pDirname[DirLen - 1] != '/') {
                SlashSpace = 1;
        }

        /*Loop through each file in the expanded list*/
        for (pFile = pNames + DirLen + 1; 
             *pFile != '\0'; 
             pFile += strlen(pFile) + 1) {
                /*Prepend the directory name to the filename*/
                memcpy(pFile - DirLen - SlashSpace, 
                       pDirname, DirLen);
                if (SlashSpace == 1) {
                        pFile[-1] = '/';
                }

                /*Okay, search the complete path*/
                if (! ScanFile_Search(pFile - DirLen - SlashSpace, pStats)) {
                        /*Search isn't interested in any more files*/
                        free(pNames);
                        return FALSE;
                }

        }

        /*Free space acquired for directory names and report success*/
        free(pNames);
        return TRUE;

} /*RecurseDir*/


/************************************************************************/
/*                                                                      */
/*      DisplayBlock -- Display block of lines (for inverted match)     */
/*                                                                      */
/*      Displays a block of lines up to but not including the           */
/*      matching line specified in pDetails.  Also calculates           */
/*      match counts and buffer line counts if these details are        */
/*      to be reported.                                                 */
/*                                                                      */
/*      The function returns TRUE if the file scan may continue,        */
/*      and returns FALSE if we're no longer interested in the          */
/*      remainder of the file.                                          */
/*                                                                      */
/************************************************************************/
public_scope BOOL
ScanFile_DisplayBlock(MatchEng_Details *pDetails, 
                      BYTE *pBlockStart,
                      BYTE *pBlockEnd)
{
        BYTE *pNextLF;

        TRACERY(SCANFILE_T_MATCH, {
                printf("\nScanfile_DisplayBlock(%p, %p (%d))\n", 
                       pBlockStart, 
                       pBlockEnd,
                       pBlockEnd - pBlockStart);
        });

        /*Is there any data to report?*/
        if (pBlockEnd != pBlockStart) {
                /*Yes, note that lines were matched*/
                gScanFile.MatchedAny = TRUE;
        }

        /*Do we need to do any line-by-line analysis or reporting?*/
        if (! gScanFile.UnpackBlocks) {
                /*No, merely dump entire block to the output*/
                fwrite(pBlockStart, 
                       1, 
                       pBlockEnd - pBlockStart, 
                       stdout);

                /*Finished reporting*/
                return TRUE;

        }

        /*Loop through each line of the block*/
        while (pBlockStart < pBlockEnd) {
                /*Split block into lines at each LF and report/count lines*/
                TRACERY(SCANFILE_T_MATCH, {
                        printf("\nmemchr(%p, LF, %u)", 
                               pBlockStart, 
                               pBlockEnd - pBlockStart);
                });

                gScanFile.Details.LineMatchCount++;

                /*Look for next line separator*/
                pNextLF = memchr(pBlockStart, LF, pBlockEnd - pBlockStart);

                /*Did we find a separator?*/
                if (pNextLF == NULL) {
                        /*No, block ends without LF*/
                        pNextLF = pBlockEnd - 1;

                }

                /*Do we have a function to report the line?*/
                if (gScanFile.pSelect != NULLFUNC) {
                        /*Yes, fill in details and report line*/
                        gScanFile.Details.pLineStart  = pBlockStart;
                        gScanFile.Details.pMatchStart = pBlockStart;
                        gScanFile.Details.pLineEnd    = pNextLF + 1;
                        gScanFile.Details.pMatchEnd   = pNextLF;

                        if (! gScanFile.pSelect(&gScanFile.Details)) {
                                /*We may abandon this file*/
                                return FALSE;
                        }

                        /*Line number*/
                        gScanFile.Details.LineNr++;

                }

                pBlockStart = pNextLF + 1;

        }

        return TRUE;

} /*DisplayBlock*/


/************************************************************************/
/*                                                                      */
/*      SearchBuffer -- Search one buffer of file                       */
/*                                                                      */
/************************************************************************/
module_scope BOOL
ScanFile_SearchBuffer(BYTE *pInBuf)
{
        BYTE *pBufCurr;
        BOOL Found;
        UINT32 BufferLines;

        /*Start scan at first byte of buffer*/
        pBufCurr = pInBuf;
        BufferLines = 0;

        /*Loop through buffer, looking for RE matches*/
        for (;;) {
                /*Search buffer for matching text*/
                Found = gScanFile.pScan(gScanFile.pScanContext, 
                                        pBufCurr,
                                        &gScanFile.Details);
                BufferLines += gScanFile.Details.BufLineNr;

                if (! Found) {
                        /*Scan portion of RE not found within buffer*/
                        TRACERY(SCANFILE_T_SCAN, {
                                printf("Scan not found, buflines: %u\n", 
                                       gScanFile.Details.BufLineNr);
                        });
                        BufferLines++;
                        break;
                }

                /*Do we need the line start but haven't found it?*/
                if ((gScanFile.Details.pLineStart == NULL) && 
                    gScanFile.FindLineStart) {
                        /*Yes, find start of matching line*/
                        gScanFile.Details.pLineStart = 
                          ((CHAR *) memrchr(gScanFile.Details.pMatchStart - 1, 
                                            LF, ~0)) + 1;
                }

                TRACERY(SCANFILE_T_SCAN, {
                        CHAR s[42];
                        Tracery_Decode(&gScanFile.TraceInfo, 
                                   TRACERY_FLAGS, s, sizeof(s));
                        printf("\n%s (%s): ", 
                               Tracery_Name(&gScanFile.TraceInfo), s);
                                              
                        printf("Found: %p, %p(%02x)..%p(%02x) ", 
                                gScanFile.Details.pLineStart, 
                                gScanFile.Details.pMatchStart, 
                               *gScanFile.Details.pMatchStart, 
                                gScanFile.Details.pMatchEnd, 
                               *gScanFile.Details.pMatchEnd);

                });

                /*Find the end of the line found by the scan*/
                gScanFile.Details.pLineEnd = memchr(
                        gScanFile.Details.pMatchEnd, LF, ~0);

                TRACERY(SCANFILE_T_SCAN, {
                        printf(" End: %p\n", gScanFile.Details.pLineEnd);
                });

                /*Fast scan succeeded: is there a slow match as well?*/
                if (gScanFile.pMatch == NULLFUNC) {
                        /*No slow match: search is complete*/
                        goto Matched;
                }

                /*Find the end of the line found by the scan*/
                gScanFile.pMatchContext->pAfterEndOfBuffer = 
                        gScanFile.Details.pLineEnd;

                Found = gScanFile.pMatch(gScanFile.pMatchContext,
                                         gScanFile.Details.pLineStart,
                                         &gScanFile.Details2);

                /*Did we match the harder (starting) bit?*/
                if (! Found) {
                        /*No, revert to scanning for easier bit*/
                        pBufCurr = gScanFile.Details.pLineEnd + 1;
                        BufferLines++;

                        /*Have we hit the end of the buffer?*/
                        if (pBufCurr >= 
                            gScanFile.pScanContext->pAfterEndOfBuffer) {
                                /*Yes, finished this buffer*/
                                break;
                        }

                        continue;
                }

                /*Copy full details of match into main buffer*/
                gScanFile.Details.pMatchStart = 
                        gScanFile.Details2.pMatchStart;
                gScanFile.Details.pMatchEnd = 
                        gScanFile.Details2.pMatchEnd;

Matched:

                /*Are we reporting a line with normal termination?*/
                if (gScanFile.Details.pLineEnd != 
                          gScanFile.pScanContext->pAfterEndOfBuffer) {
                        /*Yes, include the terminator in the display*/
                        if (*gScanFile.Details.pLineEnd++ == CR) {
                                gScanFile.Details.pLineEnd++;
                        }
                }

                /*Are we selecting matching lines?*/
                if (gScanFile.SelectMatchingLines) {
                        /*Yes, remember that we've found at least one match*/
                        gScanFile.Details.LineMatchCount++;
                        gScanFile.Details.LineNr += BufferLines;
                        BufferLines = 1;

                        /*Handle selected line*/
                        if ((gScanFile.pSelect) && 
                            (! gScanFile.pSelect(&gScanFile.Details))) {
                                /*Function advises we may skip to next file*/
                                return FALSE;

                        }

                } else {
                        /*No, inverted match: are there preceding lines?*/
                        if (pInBuf != gScanFile.Details.pLineStart) {
                                /*Yes, display them*/
                                if (! ScanFile_DisplayBlock(
                                               &gScanFile.Details, 
                                               pInBuf, 
                                               gScanFile.Details.pLineStart)) {
                                        /*We may skip to next file*/
                                        return FALSE;
                                }
                        } else {
                                /*No, still count this line*/
                                gScanFile.Details.LineNr++;
                        }

                }

                /*Update search to start of next line*/
                pBufCurr = gScanFile.Details.pLineEnd;
                pInBuf = pBufCurr;

                /*Have we hit the end of the buffer?*/
                if (gScanFile.Details.pLineEnd >= 
                    gScanFile.pScanContext->pAfterEndOfBuffer) {
                        /*Yes, finished this buffer*/
                        break;
                }

        }

        /*Is match sense inverted?*/
        if (! gScanFile.SelectMatchingLines) {
                /*Yes, is there any unmatched text at the end of the buffer?*/
                TRACERY(SCANFILE_T_SCAN, {
                        printf("InvAtEnd: pInBuf, pAfterEnd: %p %p\n", 
                               pInBuf, 
                               gScanFile.pScanContext->pAfterEndOfBuffer);
                });
                if (pInBuf < gScanFile.pScanContext->pAfterEndOfBuffer) {
                        /*Yes, display it now*/
                        if (! ScanFile_DisplayBlock(&gScanFile.Details, 
                                 pInBuf, 
                                 gScanFile.pScanContext->pAfterEndOfBuffer)) {
                                /*Display advises we may skip to next file*/
                                return FALSE;
                        }

                }

        } else {
                /*Add in any remaining lines we counted at the end*/
                gScanFile.Details.LineNr += BufferLines;
        }

        return TRUE;

} /*SearchBuffer*/


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
public_scope BOOL
ScanFile_Search(CHAR *pFilename, void *pParent)
{
        BOOL Matched;
        UINT32 NrChars;
        BYTE *pInBuf;
        ScanFile_Stats Stats;

        /*Open file for scanning*/
        if (! ScanFile_Open(pFilename, &Stats.stat)) {
                /*Unable to access file: skip to next file, if any*/
                return TRUE;
        }

        /*Is this file a directory (and we are recursing directories)?*/
        if (S_ISDIR(Stats.stat.st_mode)) {
                /*Yes, have we been asked to recurse directories?*/
                if (gScanFile.RecurseDir) {
                        /*Yes, enumerate files in this directory*/
                        Stats.pParent = pParent;

                        /*?? Should close the opened handle?*/

                        return ScanFile_RecurseDir(pFilename, &Stats);
                }

                /*Sorry, we don't grep directories as binary files (yet)*/

                /*Sorry, don't report skipped directories, either*/

                return TRUE;

        }

        /*Initialise line match counter*/
        gScanFile.Details.LineMatchCount = 0;

        /*Set up match details structure for reporting*/
        gScanFile.Details.LineNr = 1;

ReadFile:
        /*Get next buffer of file, if any*/
        if (! FastFile_Read(gScanFile.pHandle, &pInBuf, &NrChars, 
                            &gScanFile.Details.BufferOffset)) {
                /*Error while reading buffer*/
                printf("?? ScanFile_Search: FastFile read error\n");
                return FALSE;
        }

        /*Did we read any characters?*/
        if (NrChars != 0) {
                /*Yes, search the buffer we've received*/
                TRACERY(SCANFILE_T_BUFFER, {
                        UINT i;
                        printf("ScanFile: Buffer %p..%p, %lu chars:", pInBuf, 
                               &pInBuf[NrChars], NrChars);
                        for (i = 0; i < 6; i++) {
                                printf(" %02x", pInBuf[i]);
                        }
                        printf("...\n");
                });

                gScanFile.Details.pBufferStart = pInBuf;

                /*Set up buffer end ptr (byte search and/or inverted match)*/
                gScanFile.pScanContext->pAfterEndOfBuffer = &pInBuf[NrChars];

                /*Destroy line end pointer in case there's no match*/
                gScanFile.Details.pLineEnd = NULL;

                /*Is the buffer bigger than the backtracking size?*/
                if (NrChars > gScanFile.pScanContext->BacktrackSize) {
                        /*Yes, get RETable to allocate a suitable space*/
                        if (! RETable_AllocBacktrack(gScanFile.pScanContext, 
                                                     NrChars)) {
                                /*Sorry, ran out of resources*/
                                fprintf(stderr, 
                                        "%s: Not enough backtrack memory\n", 
                                        Platform_ProgramName());
                                return TRUE;
                        }
                }

                /*Search this buffer*/
                if (ScanFile_SearchBuffer(pInBuf)) {
                        /*Handle next buffer of file (if any)*/
                        goto ReadFile;

                }

                /*If we reach here, search isn't interested in file any more*/

        }

        Matched = gScanFile.Details.LineMatchCount != 0;

        /*Were we asked to report if no lines matched within file?*/
        if ((gScanFile.Details.ReportingOptions & 
             MATCHENG_RPT_NONMATCH_FILES) && ! Matched) {
                /*Yes, report filename now and proceed to next file*/
                gScanFile.pFilenameOut(&gScanFile.Details);
                gScanFile.MatchedAny = TRUE;
                return TRUE;
        }

        /*Accumulate status of match across all files*/
        if (Matched) {
                gScanFile.MatchedAny = TRUE;
        }

        /*Were we asked to count lines?*/
        if (gScanFile.Details.ReportingOptions &
            MATCHENG_RPT_LINECOUNT) {
                /*Yes, report count now (include filename if selected)*/
                gScanFile.pFilenameOut(&gScanFile.Details);
        }

        /*Request that enumeration continue*/
        return TRUE;

} /*Search*/


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
/************************************************************************/
public_scope BOOL
ScanFile_Pattern(RegExp_Specification *pPattern, LWORD ScanOptions)
{
        RegExp_Specification *pEasyBit = NULL;
        RegExp_Specification *pHarderBit = NULL;
        BOOL IgnoreCase;
        STBM_SearchSpec *pSTBM;
        LWORD ScanFlags = MATCHENG_SPEC_SKIP_BYTES | 
                MATCHENG_SPEC_ENDMARKER(SCANFILE_ENDMARKER_DEFAULT);
        LWORD MatchFlags = 0;
        UINT PatternLength;
        BYTE TrailingLiteral;

        /*Does the main search reference a valid RE specification?*/
        if (pPattern == NULL) {
                /*No, fault in configuration specification*/
                return FALSE;
        }

        /*Allowable optimisations given in ScanOptions*/
        if (ScanOptions & SCANFILE_DEBUG_COMPILED) {
                RegExp_ShowCodes("RE.Original: ", pPattern);
        }

        /*Does the client want CR/LF line termination as well as LF?*/
        if (ScanOptions & SCANFILE_OPT_CR_IS_TERMINATOR) {
                /*Yes, tell match engine to set up to support this*/
                MatchFlags |= MATCHENG_SPEC_CR_IS_TERMINATOR;
                ScanFlags |= MATCHENG_SPEC_CR_IS_TERMINATOR;
        }

        /*Does the client want to recurse directories?*/
        gScanFile.RecurseDir = FALSE;
        if (ScanOptions & SCANFILE_OPT_RECURSE_DIRECTORIES) {
                /*Yes, remember this for when we're dealing with files*/
                gScanFile.RecurseDir = TRUE;
        }

        /*Default to no "easy" search before match search*/
        gScanFile.pMatchContext = NULL;

        /*Does the client want us to count matching lines?*/
        if (ScanOptions & SCANFILE_NUMBER_MATCHING_LINES) {
                /*Yes, can't use search optimisations that skip bytes*/
                ScanOptions &= ~SCANFILE_OPT_SKIP;

                /*Tell RE engine to include line counting and disallow skip*/
                ScanFlags &= ~MATCHENG_SPEC_SKIP_BYTES;
                ScanFlags |= MATCHENG_SPEC_COUNT_LINES;
        }

        /*Does the client want us to count nonmatching lines?*/
        if (ScanOptions & SCANFILE_NUMBER_NONMATCH_LINES) {
                /*Need to break blocks of text into lines*/
                gScanFile.UnpackBlocks = TRUE;
        }

        /*Is the client happy with only an approximate match?*/
        if (ScanOptions & SCANFILE_OPT_APPROXIMATE) {
                /*Yes, remove optional first/last elements that slow us down*/
                if (RegExp_SlashEnds(pPattern)) {
                        /*RE has been modified to simplify things*/
                        if (ScanOptions & SCANFILE_DEBUG_COMPILED) {
                                RegExp_ShowCodes("RE.Approx: ", pPattern);
                        }
                }
        }

        /*Default to no match function*/
        gScanFile.pMatch = NULLFUNC;

        /*Can the entire RE be searched using STBM?*/
        pSTBM = NULL;
        if ((ScanOptions & SCANFILE_OPT_SKIP) && 
            (ScanOptions & SCANFILE_OPT_SELF_TUNED_BM)) {
                pSTBM = STBMShim_Pattern(pPattern, 
                                         &PatternLength, 
                                         &IgnoreCase, 
                                         &TrailingLiteral);
        }
        if (pSTBM != NULL) {
                /*Yes, does the caller want to display the tables?*/
                if (! (ScanOptions & SCANFILE_DEBUG_DISPLAY)) {
                        /*No, can skip a lot of unnecessary setup code*/
                        goto AfterTableAnalysis;
                }
        }

        /*Is there an easier bit to search in the middle of the RE?*/
        gScanFile.pScan = gScanFile.pExternMatchFunc;
        if ((ScanOptions & SCANFILE_OPT_EASIEST_FIRST) &&
                        RegExp_EasiestFirst(pPattern, &pEasyBit)) {
                /*Yes, modify search to scan for that part first*/
                pHarderBit = pPattern;
                pPattern = pEasyBit;
                gScanFile.FindLineStart = TRUE;

                if (ScanOptions & SCANFILE_DEBUG_COMPILED) {
                        RegExp_ShowCodes("RE.Easy: ", pPattern);
                }

                /*Use function to check for full match after easy bit found*/
                gScanFile.pMatch = gScanFile.pExternMatchFunc;

                /*Expand hard bit to line-based search*/
                MatchFlags |= MATCHENG_SPEC_ENDMARKER(LF);
                if (! RETable_Expand(pHarderBit, 
                                     MatchFlags, 
                                     &gScanFile.pMatchContext)) {
                        /*Expansion failed for some reason*/
                        printf("RETable.Expand (match) failed\n");
                        return FALSE;
                }

        }

        /*Are we allowed to attempt optimisations that skip bytes?*/
        if (ScanOptions & SCANFILE_OPT_SKIP) {
                /*Yes, may we try to use self-tuning Boyer-Moore algorithm?*/
                if (ScanOptions & SCANFILE_OPT_SELF_TUNED_BM) {
                        /*Yes, see if the algorithm can handle the search*/
                        pSTBM = STBMShim_Pattern(pPattern, 
                             &PatternLength, 
                             &IgnoreCase, 
                             &TrailingLiteral);
                }

        }

        /*FALLTHROUGH*/

AfterTableAnalysis:
        /*If no STBM or if table display, expand RE into table-driven format*/
        if ((pSTBM == NULL) || (ScanOptions & SCANFILE_DEBUG_DISPLAY)) {
                /*Expand compact RE spec into table-driven version*/
                if (! RETable_Expand(pPattern, ScanFlags, 
                                     &gScanFile.pScanContext)) {
                        /*Expansion failed for some reason*/
                        fprintf(stderr, "RETable.Expand (scan) failed\n");
                        return FALSE;
                }
        } else {
                /*Using STBM, allocate scan context*/
                if (! ScanFile_NewScanContext(&gScanFile.pScanContext)) {
                        /*Sorry, unable to set up scan context*/
                        fprintf(stderr, "STBM scan context error\n");
                        return FALSE;
                }

        }

        /*Are we using STBM?*/
        if (pSTBM != NULL) {
                /*Yes, set up scan context*/
                gScanFile.pScanContext->PatternLength = PatternLength;
                gScanFile.pScanContext->TrailingLiteral = 
                        TrailingLiteral;

                gScanFile.pScanContext->EndCondition = 
                        MATCHENG_CONDITION_TRAILING_LITERAL;

                /*Configure search to use STBM interface*/
                if (IgnoreCase) {
                        gScanFile.pScan = STBMShim_SearchInCase;
                } else {
                        /*Select STBM or TBM as appropriate*/
                        if (ScanOptions & SCANFILE_OPT_TUNED_BM) {
                                /*Caller wants Tuned BM for comparison*/
                                gScanFile.pScan = STBMShim_SearchTBM;
                        } else {
                                /*Use behoffski's self-tuned BM*/
                                gScanFile.pScan = STBMShim_Search;
                        }
                }

                gScanFile.pScanContext->pSpare1 = pSTBM;
                TRACERY(SCANFILE_T_SCAN, {
                        printf("\nScanfile: Using STBM");
                });
        }

        /*Display tables if requested*/
        if (ScanOptions & SCANFILE_DEBUG_DISPLAY) {
                TblDisp_Describe(gScanFile.pScanContext, "Scan");

                /*Does the RE have a match component as well?*/
                if (gScanFile.pMatchContext != NULL) {
                        /*Yes, display it*/
                        TblDisp_Describe(gScanFile.pMatchContext, 
                                         "Match");
                }
        }

        /*Report success to caller*/
        return TRUE;

} /*Pattern*/


/************************************************************************/
/*                                                                      */
/*      Configure -- Define how the module searches and reports matches */
/*                                                                      */
/************************************************************************/
public_scope BOOL
ScanFile_Configure(LWORD ReportingOptions)
{
        TRACERY(SCANFILE_T_MATCH, {
                printf("Scanfile_Configure(%08lx)\n", ReportingOptions);
        });

        /*Record reporting options and select functions accordingly*/
        gScanFile.Details.ReportingOptions = ReportingOptions;

        /*Default to normal reporting of lines*/
        gScanFile.pSelect = gScanFile.pNormalOut;
        gScanFile.FindLineStart = TRUE;

        /*Does the client want to see lines?*/
        if (! (ReportingOptions & MATCHENG_RPT_LINE)) {
                /*No, just show filename on match and finish file*/
                gScanFile.pSelect = gScanFile.pFilenameOut;
                gScanFile.FindLineStart = FALSE;
        }

        /*Does the client want line counting?*/
        if (ReportingOptions & MATCHENG_RPT_LINECOUNT) {
                /*Yes, use match count function*/
                gScanFile.pSelect = NULLFUNC;
        }

        /*Does the client want highlighted matches?*/
        if (ReportingOptions & MATCHENG_RPT_HIGHLIGHT) {
                /*Yes, use platform-specific function*/
                gScanFile.pSelect = gScanFile.pHighlightOut;
        }

        /*Has the client requested that matches be reported with delimiters?*/
        if (ReportingOptions & MATCHENG_RPT_MARKER_FLAG) {
                /*Yes, use platform-specific function and unpack char*/
                gScanFile.pSelect = gScanFile.pHighlightOut;
                gScanFile.Details.MarkerChar =
                                MATCHENG_RPT_MARKER_UNPACK(ReportingOptions);
        }

        /*Does the client want to see nonmatch files?*/
        if (ReportingOptions & MATCHENG_RPT_NONMATCH_FILES) {
                /*Yes, abandon file as soon as match found*/
                gScanFile.pSelect = ScanFile_MatchedAbandon;
                gScanFile.FindLineStart = FALSE;
        }

        /*Does the client want to report non-matching lines?*/
        gScanFile.SelectMatchingLines = TRUE;
        if (ReportingOptions & MATCHENG_RPT_INVERT_MATCH_SENSE) {
                /*Yes, select inverted match sense*/
                gScanFile.SelectMatchingLines = FALSE;
                gScanFile.FindLineStart = TRUE;

        }

        /*Does the client want to add information to each line?*/
        if (ReportingOptions & (MATCHENG_RPT_LINENUMBER | 
                                MATCHENG_RPT_BYTEOFFSET | 
                                MATCHENG_RPT_FILENAME | 
                                MATCHENG_RPT_LINECOUNT | 
                                MATCHENG_RPT_REMOVE_TRAILING_CR)) {
                /*Yes, remember to break apart blocks if inverted sense*/
                gScanFile.UnpackBlocks = TRUE;
                gScanFile.FindLineStart = TRUE;
        }

        /*Configured module successfully*/
        return TRUE;

} /*Configure*/


/************************************************************************/
/*                                                                      */
/*      MatchedAny -- Report if any files matched search criteria       */
/*                                                                      */
/************************************************************************/
public_scope BOOL
ScanFile_MatchedAny(void)
{
        /*Was internal flag set?*/
        if (gScanFile.MatchedAny) {
                /*Yes, clear it and then report state to caller*/
                gScanFile.MatchedAny = FALSE;
                return TRUE;
        }

        /*Internal flag was not set*/
        return FALSE;

} /*MatchedAny*/


/************************************************************************/
/*                                                                      */
/*      MatchFunction -- Define routine to perform match                */
/*                                                                      */
/*      ScanFile wishes to provide extreme high-performance searches    */
/*      to do so in a very portable fashion.  This function is the      */
/*      result: ScanFile receives the address of the function that      */
/*      implements the match from an outsider (usually Platform).       */
/*      This function must be called before Pattern.                    */
/*                                                                      */
/************************************************************************/
public_scope void
ScanFile_MatchFunction(MatchEng_MatchFunction pMatchFunc)
{
        /*Remember function for operation*/
        gScanFile.pExternMatchFunc = pMatchFunc;

} /*MatchFunction*/


/************************************************************************/
/*                                                                      */
/*      NoMatchFunction -- Place-holder to warn of incorrect config     */
/*                                                                      */
/*      This function is called if ScanFile does not receive a          */
/*      match function appropriate to the platform.                     */
/*                                                                      */
/************************************************************************/
module_scope BOOL 
ScanFile_NoMatchFunction(MatchEng_Spec *pTable, 
                         BYTE *pText,
                         MatchEng_Details *pDetails)
{
        fprintf(stderr, "Scanfile: No match function provided!");

        return FALSE;

} /*NoMatchFunction*/


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
public_scope void
ScanFile_OutputFunctions(MatchEng_SelectFunction *pNormal,
                         MatchEng_SelectFunction *pHighlight,
                         MatchEng_SelectFunction *pFilenameOut)
{
        /*Record the functions to use for later*/
        gScanFile.pNormalOut = pNormal;
        gScanFile.pHighlightOut = pHighlight;
        gScanFile.pFilenameOut = pFilenameOut;

} /*OutputFunctions*/


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
public_scope BOOL
ScanFile_TraceryLink(void *pObject, UINT Opcode, ...)
{
        Tracery_ObjectInfo **ppInfoBlock;
        LWORD *pDefaultFlags;
        Tracery_EditEntry **ppEditList;
        va_list ap;

        va_start(ap, Opcode);

        switch (Opcode) {
        case TRACERY_REGCMD_GET_INFO_BLOCK:
                /*Report module's block (we don't support objects as yet)*/
                ppInfoBlock  = va_arg(ap, Tracery_ObjectInfo **);
                *ppInfoBlock = &gScanFile.TraceInfo;
                break;

        case TRACERY_REGCMD_GET_DEFAULT_FLAGS:
                pDefaultFlags  = va_arg(ap, LWORD *);
                *pDefaultFlags = 
                        SCANFILE_T_BUFFER | 
                        SCANFILE_T_SCAN | 
                        SCANFILE_T_MATCH |
                        SCANFILE_T_DIR;
                break;

        case TRACERY_REGCMD_GET_EDIT_LIST:
                ppEditList  = va_arg(ap, Tracery_EditEntry **);
                *ppEditList = gScanFile_TraceryEditDefs;
                break;

        default:
                /*Unsupported opcode*/
                va_end(ap);
                return FALSE;

        }

        va_end(ap);
        return TRUE;

} /*TraceryLink*/
#endif /*TRACERY_ENABLED*/


/************************************************************************/
/*                                                                      */
/*      Init -- Prepare module for operation                            */
/*                                                                      */
/************************************************************************/
public_scope void
ScanFile_Init(void)
{
        /*No traces enabled by default*/
        TRACERY_CLEAR_ALL_FLAGS(&TRACERY_MODULE_INFO);

        /*Initialise selection reporting details*/
        gScanFile.Details.LineMatchCount = 0;

        /*No match function provided initially*/
        gScanFile.pMatch = ScanFile_NoMatchFunction;
        gScanFile.pScanContext = NIL;

        /*Configure default scanning options (display matching lines)*/
        gScanFile.SelectMatchingLines = TRUE;

        /*By default, we don't recurse directories*/
        gScanFile.RecurseDir = FALSE;

        /*Assume that we don't need to find the line start*/
        gScanFile.FindLineStart = FALSE;

        /*No platform-specific display functions provided yet*/
        gScanFile.pNormalOut = NULLFUNC;
        gScanFile.pHighlightOut = NULLFUNC;
        gScanFile.pFilenameOut = NULLFUNC;
        gScanFile.pSelect = NULLFUNC;

        /*Initialise FastFile configuration memory constant*/
        gScanFile.PrecedingLF = LF;

        /*Don't display any debug information*/
        gScanFile.Debug = 0;

        /*Default to treating blocks of file with minimal overhead*/
        gScanFile.UnpackBlocks = FALSE;

} /*Init*/
