/************************************************************************/
/*                                                                      */
/*      FastFile -- Provide low-cost buffer-based file access           */
/*                                                                      */
/*      The typical file input/output facilities provided by            */
/*      the stdio library are more than adequate for the average        */
/*      user.  They are fast, flexible, reliable and coherent.          */
/*      However, while the services they provide do not cost            */
/*      much when measured relative to average applications, the        */
/*      costs do become substantial for applications that don't         */
/*      perform much processing yet strive to run very quickly.         */
/*                                                                      */
/*      FastFile is a module designed to provide extremely fast         */
/*      access to text-oriented files.  It works in partnership         */
/*      with the application to present the file in raw form            */
/*      in a memory-mapped buffer.  In addition, the module             */
/*      provides facilities for configuring and manipulating            */
/*      the memory immediately before and after the file buffer.        */
/*      This is because some of the biggest performance                 */
/*      improvements occur when the application can use markers         */
/*      and/or reserve memory such that it may ignore                   */
/*      edge-of-buffer issues while performing its work.                */
/*                                                                      */
/*      --------------------- File data --------------------            */
/*      | (1) | (2) |     ...                        | (n) |            */
/*      +-----+-----+                                +-----+            */
/*               v                                                      */
/*               +------------------+                                   */
/*                                  v                                   */
/*      <----- memory map ---------------------------------------->     */
/*                               | buffer |                             */
/*            | SpaceBeforeStart |        | SpaceAfterEnd |             */
/*                    | PreBytes |        | PostBytes |                 */
/*                                                                      */
/*      This version for Linux exploits mmap in order to reduce         */
/*      file handling costs, and is very similar to the way             */
/*      GNU Grep handles files.  A side-effect of this                  */
/*      optimisation is that the user must preserve the                 */
/*      trailing bytes of the buffer so that we do not need to          */
/*      re-read them here.                                              */
/*                                                                      */
/*      Copyright (C) 1995-2000 Grouse Software.  All rights reserved.  */
/*      Written for Grouse by behoffski (Brenton Hoff).                 */
/*                                                                      */
/*      Free software: no warranty; use anywhere is ok; spread the      */
/*      sources; note any mods; share variations and derivatives        */
/*      (including sending to behoffski@grouse.com.au).                 */
/*                                                                      */
/************************************************************************/

#ifndef MAP_FAILED
#define MAP_FAILED     (void *)-1L
#endif /* HP 10.20 */


#include "ascii.h"
#include <compdef.h>
#include <errno.h>
#include "fastfile.h"
#include <memrchr.h>
#include <memory.h>
#include "platform.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/times.h>
#include "tracery.h"
#include <unistd.h>

#include <sys/mman.h>
#include <sys/stat.h>

struct FastFile_FileContextStruct {
        /*Tracery control block for this object*/
        Tracery_ObjectInfo TraceInfo;

        /*stdio-related information*/
        INT FileNr;
        FILE *f;
        off_t BufPos;

        /*Buffer pointer and data prefix size storage*/
        BYTE *pBuf;
        size_t PrefixSize;

        /*Options specified by client*/
        size_t BufferSizeMin;
        size_t BufSize;
        LWORD Flags;

        /*Configuration of bytes before buffer*/
        size_t PreBytes;
        UINT PreSize;
        BYTE *pPreData;

        /*Configuration of bytes after buffer*/
        size_t SpaceAfterEnd;
        UINT PostSize;
        BYTE *pPostData;
        BYTE *pPostSave;

        /*Feedover bytes (from incomplete line at end of buffer)*/
        size_t FeedoverSize;
        BYTE *pFeedover;

        /*Additional information needed if using mmap*/
        struct stat FileStats;
        BOOL MayMap;

};

typedef struct {
        /*Tracery control block for this module*/
        Tracery_ObjectInfo TraceInfo;

        /*Operating system function to read bytes from a file*/
        FastFile_ReadFn *pReadFn;

        /*Buffer size suggestion provided when module started*/
        size_t BufferSizeSuggested;

        UINT32 PageSize;

} FASTFILE_MODULE_CONTEXT;

module_scope FASTFILE_MODULE_CONTEXT gFastFile;

/*There's no portable vfree at this stage*/

#define vfree_if_available(valloced_pointer)


/*Extra information for Tracery operation*/

#ifdef TRACERY_ENABLED

#define TRACERY_MODULE_INFO             (gFastFile.TraceInfo)

/*Debugging/tracing flags*/

#define FASTFILE_T_ENTRY                BIT0
#define FASTFILE_T_BUFFER               BIT1
#define FASTFILE_T_FEEDOVER             BIT2

module_scope Tracery_EditEntry gFastFile_TraceryEditDefs[] = {
        {"E", FASTFILE_T_ENTRY,    FASTFILE_T_ENTRY,    "Trace  entry"}, 
        {"e", FASTFILE_T_ENTRY,    0x00,                "Ignore entry"}, 
        {"B", FASTFILE_T_BUFFER,   FASTFILE_T_BUFFER,   "Trace  buffer"}, 
        {"b", FASTFILE_T_BUFFER,   0x00,                "Ignore buffer"}, 
        {"F", FASTFILE_T_FEEDOVER, FASTFILE_T_FEEDOVER, "Trace  feedover"}, 
        {"f", FASTFILE_T_FEEDOVER, 0x00,                "Ignore feedover"}, 
        TRACERY_EDIT_LIST_END
};

#endif /*TRACERY_ENABLED*/


/************************************************************************/
/*                                                                      */
/*      Init -- Prepare module for operation                            */
/*                                                                      */
/************************************************************************/
public_scope void
FastFile_Init(void)
{
        /*Use ANSI-compliant read function*/
        gFastFile.pReadFn = read;

        /*No tracing enabled by default*/
        TRACERY_CLEAR_ALL_FLAGS(&TRACERY_MODULE_INFO);

} /*Init*/


/************************************************************************/
/*                                                                      */
/*      Start -- Begin managing what has to be managed                  */
/*                                                                      */
/*      BufferSize names the default size (in bytes) to use for         */
/*      each file opened.  The client may choose a larger size          */
/*      for the file buffer when it is opened.                          */
/*                                                                      */
/************************************************************************/
BOOL
FastFile_Start(size_t BufferSize)
{
        TRACERY(FASTFILE_T_ENTRY, {
                printf("\nFastFile_Start()");
        });

        /*Get memory manager's page size so we may conform to mmap()*/
        gFastFile.PageSize = getpagesize();

        /*Did we receive a reasonable answer?*/
        if (gFastFile.PageSize == 0) {
                /*No, choose a page size somewhat arbitrarily*/
                gFastFile.PageSize = 1024;
        }

        TRACERY(FASTFILE_T_BUFFER, {
                printf("Page size: %lu\n", gFastFile.PageSize);
        });

        /*Remember buffer size suggested by caller*/
        gFastFile.BufferSizeSuggested = BufferSize;

        /*Started successfully*/
        return TRUE;

} /*Start*/


/************************************************************************/
/*                                                                      */
/*      Open -- Provide optimised access using specified file           */
/*                                                                      */
/*      Attempts to open the specified file for optimised access,       */
/*      and, if successful, returns a handle referencing the file.      */
/*      If the filename parameter is a NULL pointer, then standard      */
/*      input is used.                                                  */
/*                                                                      */
/*      BufferSizeMin names the minimum buffer size to use.  If         */
/*      size isn't important, specify 0 and FastFile will use           */
/*      a reasonable default.                                           */
/*                                                                      */
/*      Flags contains options, most notably Line mode selection.       */
/*      LineMode may be FALSE (Raw mode) or TRUE (Line mode).           */
/*      In Line mode, FastFile guarantees that (a) The start of         */
/*      each buffer is the start of a text line, and (b) The            */
/*      buffer ends at the end of a line (or, equivalently, at          */
/*      the end of the file).  The line terminator is LF.               */
/*      In Raw mode, the buffer does not have these properties:         */
/*      there isn't any relationship between the contents of the        */
/*      file and the portion of the file presented in each buffer.      */
/*                                                                      */
/*      The client may receive details of the file's stauus via         */
/*      the ppStat parameter if desired, or may specify NULL            */
/*      if not interested.  This parameter is provided so the           */
/*      application may minimise to an absurd degree the code           */
/*      execution times.  If we can't stat the file, any return         */
/*      stat pointer is set to NULL.                                    */
/*                                                                      */
/*      Returns FALSE if the file was not able to be opened.            */
/*                                                                      */
/************************************************************************/
BOOL
FastFile_Open(CHAR *pFilename, 
              size_t BufferSizeMin, 
              LWORD Flags, 
              struct stat *pStat, 
              FastFile_Context **ppHandle)
{
        FastFile_Context *pHandle;

        TRACERY(FASTFILE_T_ENTRY, {
                printf("\nFastFile_Open(%s, %u, %08lx, %p)", 
                       pFilename, BufferSizeMin, Flags, ppHandle);
        });

        /*Check arguments*/
        if (ppHandle == NULL) {
                /*gFastFile.Diagnosis = EARGS;*/
                return FALSE;
        }

        /*Destroy return value to reduce chances of being misunderstood*/
        *ppHandle = (FastFile_Context *) NIL;

        /*Obtain memory to use as file handle*/
        pHandle = (FastFile_Context *) Platform_SmallMalloc(sizeof(*pHandle));
        if (pHandle == NULL) {
                /*gFastFile.Diagnosis = EMEMORY;*/
                return FALSE;
        }

        /*Initialise memory to reduce indeterminism*/
        memset(pHandle, 0, sizeof(*pHandle));

        /*Round up buffer size if smaller than startup suggestion*/
        if (BufferSizeMin < gFastFile.BufferSizeSuggested) {
                BufferSizeMin = gFastFile.BufferSizeSuggested;
        }

        /*Does the client wish to use stdin?*/
        if (pFilename == NULL) {
                /*Yes, use stdin handle instead of opening another*/
                pHandle->f = stdin;

        } else {
                /*Attempt to open file specified by client*/
                pHandle->f = fopen(pFilename, "rb");
                if (pHandle->f == NULL) {
                        /*Open failed*/
                        Platform_SmallFree(pHandle);
                        /*gFastFile.Diagnosis = EOPEN;*/
                        return FALSE;
                }

        }

        pHandle->FileNr = fileno(pHandle->f);

        /*Remember buffer information but defer allocation til later*/
        pHandle->BufferSizeMin = BufferSizeMin;
        pHandle->BufSize       = BufferSizeMin;
        pHandle->pBuf          = NULL;
        pHandle->PrefixSize    = gFastFile.PageSize;
        while (pHandle->PrefixSize < (BufferSizeMin / 4)) {
                pHandle->PrefixSize *= 2;
        }

        /*Initialise file context*/
        pHandle->BufPos        = 0;
        pHandle->FeedoverSize  = 0;
        pHandle->pFeedover     = NULL;
        pHandle->Flags         = Flags;
        pHandle->PreSize       = 0;
        pHandle->pPreData      = NULL;
        pHandle->PostSize      = 0;
        pHandle->pPostData     = NULL;
        pHandle->pPostSave     = NULL;

        /*Obtain file stats so we can decide whether to use mem-mapped i/f*/
        pHandle->MayMap = FALSE;
        if (fstat(pHandle->FileNr, &pHandle->FileStats) == 0) {
                /*Obtained stats: Does client want to see stats as well?*/
                if (pStat != NULL) {
                        /*Yes, tell client where to find our structure*/
                        *pStat = pHandle->FileStats;
                }

                /*Is the file a regular file?*/
                if (S_ISREG(pHandle->FileStats.st_mode)) {
                        /*Yes, may try memory-mapped interface*/
                        pHandle->MayMap = TRUE;
                }
        }

        /*?? Warning: we don't handle error if can't stat file*/

        /*Report success to caller*/
        *ppHandle = pHandle;
        return TRUE;

} /*Open*/


/************************************************************************/
/*                                                                      */
/*      Reopen -- Close existing file and open new file for access      */
/*                                                                      */
/*      This function only exists to reduce the overall cost of         */
/*      file handling where multiple files each use the same            */
/*      buffer conditioning.  The new file receives the same            */
/*      configuration settings as the previous file.                    */
/*      Returns FALSE if the file was not able to be opened (the        */
/*      existing file may *not* be accessed again, but the handle       */
/*      may be reused in further Reopen calls).                         */
/*                                                                      */
/*      The client may receive details of the file's stauus via         */
/*      the pStat parameter if desired, or may specify NULL             */
/*      if not interested.  This parameter is provided so the           */
/*      application may minimise to an absurd degree the code           */
/*      execution times.  If we can't stat the file, any return         */
/*      stat pointer is set to NULL.                                    */
/*                                                                      */
/************************************************************************/
public_scope BOOL
FastFile_Reopen(FastFile_Context *pHandle, 
                CHAR *pFilename,
                struct stat *pStat)
{
        /*Does the client wish to use stdin?*/
        if (pFilename == NULL) {
                /*Yes, use stdin handle instead of opening another*/
                pHandle->f = stdin;

        } else {
                pHandle->f = freopen(pFilename, "rb", pHandle->f);
                if (pHandle->f == NULL) {
                        /*reopen failed*/
                        /*gFastFile.Diagnosis = EOPEN;*/
                        return FALSE;
                }

        }

        pHandle->FileNr = fileno(pHandle->f);

        /*Remember buffer information but defer allocation til later*/
        pHandle->BufSize = pHandle->BufferSizeMin;
        pHandle->PrefixSize  = gFastFile.PageSize;
        while (pHandle->PrefixSize < (pHandle->BufferSizeMin / 4)) {
                pHandle->PrefixSize *= 2;
        }

        /*Initialise file context*/
        pHandle->BufPos       = 0;
        pHandle->FeedoverSize = 0;
        pHandle->pFeedover    = NULL;

        /*Obtain file stats so we can decide whether to use mem-mapped i/f*/
        pHandle->MayMap = FALSE;
        if (fstat(pHandle->FileNr, &pHandle->FileStats) == 0) {
                /*Obtained stats: Does client want to see stats as well?*/
                if (pStat != NULL) {
                        /*Yes, tell client where to find our structure*/
                        *pStat = pHandle->FileStats;
                }

                /*Is the file a regular file?*/
                if (S_ISREG(pHandle->FileStats.st_mode)) {
                        /*Yes, may try memory-mapped interface*/
                        pHandle->MayMap = TRUE;
                }
        }

        /*?? Warning: we don't handle error if can't stat file*/

        /*Exchanged files successfully*/
        return TRUE;

} /*Reopen*/


/************************************************************************/
/*                                                                      */
/*      StartCondition -- Specify how to set up start of buffer         */
/*                                                                      */
/*      This function configures how FastFile sets up the start         */
/*      of each buffer before it is retrieved by the client.            */
/*      SpaceBeforeStart specifies the minimum bytes before the         */
/*      buffer start address that must be available for the client      */
/*      to use.  PreSize and pPreData describe values that              */
/*      will be written immediately preceding the buffer start          */
/*      (note this area overlaps SpaceBeforeStart).                     */
/*                                                                      */
/*      SpaceBeforeStart may be 0 if no memory reservation is           */
/*      required, and similarly PreSize may be 0 if no memory           */
/*      needs to be set.  The client must maintain the memory           */
/*      referenced by pPreData unchanged; if the data is changed,       */
/*      StartCondition should be called again.                          */
/*                                                                      */
/*      Currently this function may only be called after Open but       */
/*      before the first Read.  This restriction may be relaxed         */
/*      in the future.                                                  */
/*                                                                      */
/************************************************************************/
BOOL
FastFile_StartCondition(FastFile_Context *pHandle, 
                        UINT SpaceBeforeStart, 
                        UINT PreSize, 
                        BYTE *pPreData)
{
        TRACERY(FASTFILE_T_ENTRY, {
                printf("\nFastFile_StartCondition(%u, %u, %p)", 
                       SpaceBeforeStart, PreSize, pPreData);
        });

        /*Check arguments*/
        if ((PreSize != 0) && (pPreData == NULL)) {
                return FALSE;
        }

        /*Is SpaceBeforeStart smaller than PreSize?*/
        if (SpaceBeforeStart < PreSize) {
                /*Yes, increase it so we only need to think about one number*/
                SpaceBeforeStart = PreSize;
        }

        /*Okay, record new parameters*/
        pHandle->PreBytes = SpaceBeforeStart;
        pHandle->PreSize = PreSize;
        pHandle->pPreData = pPreData;

        return TRUE;

} /*StartCondition*/


/************************************************************************/
/*                                                                      */
/*      EndCondition -- Specify how to set up end of buffer             */
/*                                                                      */
/*      Specifies how FastFile will prepare the end of each             */
/*      buffer.  LineMode selects Raw mode or Line mode.                */
/*      SpaceAfterEnd specifies how many bytes after the last           */
/*      byte of file data are available for client to use.              */
/*      PostSize and pPostData specify bytes to write                   */
/*      immediately following the buffer (note overlap of this          */
/*      area with SpaceAfterEnd).  As with StartCondition, 0 may        */
/*      be specified for either SpaceAfterEnd and/or PostSize.          */
/*      The client must maintain the memory referenced by               */
/*      pPostData unchanged; if the data is changed, EndCondition       */
/*      should be called again.                                         */
/*                                                                      */
/*      Currently this function may only be called after Open but       */
/*      before the first Read.  This restriction may be relaxed         */
/*      in the future.                                                  */
/*                                                                      */
/************************************************************************/
BOOL
FastFile_EndCondition(FastFile_Context *pHandle, 
                        UINT SpaceAfterEnd, 
                        UINT PostSize, 
                        BYTE *pPostData)
{
        BYTE *pNewBuffer;

        TRACERY(FASTFILE_T_ENTRY, {
                printf("\nFastFile_EndCondition(%u, %u, %p", 
                       SpaceAfterEnd, PostSize, pPostData);
                printf(" [%02x %02x %02x...])", pPostData[0], 
                       pPostData[1], pPostData[2]);
        });

        /*Obtain memory to save buffer contents before overwriting*/
        pNewBuffer = realloc(pHandle->pPostSave, PostSize);
        if ((pNewBuffer == NULL) && (PostSize != 0)) {
                /*Sorry, unable to obtain memory for new buffer*/
                return FALSE;
        }

        /*Okay, record new parameters and report success*/
        pHandle->SpaceAfterEnd = SpaceAfterEnd;
        pHandle->PostSize      = PostSize;
        pHandle->pPostData     = pPostData;
        pHandle->pPostSave     = pNewBuffer;
        return TRUE;

} /*EndCondition*/


/************************************************************************/
/*                                                                      */
/*      Read -- Fetch next buffer of file (if any)                      */
/*                                                                      */
/*      This function retrieves a slab of file data and returns it      */
/*      in a contiguous memory space.                                   */
/*                                                                      */
/*      pBufferOffset receives the byte offset of the start of          */
/*      the buffer relative to the start of the file.  This allows      */
/*      the caller to work out the absolute offset of any byte          */
/*      in the buffer from the start of the file.  This parameter       */
/*      supercedes the Position function included in previous           */
/*      incarnations of this module.                                    */
/*                                                                      */
/*      Returns FALSE if unable to handle the request (usually          */
/*      because there isn't enough memory available).                   */
/*                                                                      */
/************************************************************************/
BOOL
FastFile_Read(FastFile_Context *pHandle, 
              BYTE **ppBuf, UINT32 *pBufferSize, 
              UINT32 *pBufferOffset)
{
        UINT NrChars;
        BYTE *pLastLF;
        BYTE *pLastByte;
        BYTE *pFirstByte;
        size_t AllocSize;
        BOOL UseMap;
        char *pMemBuf;
        BYTE *pDiscardedBuffer = NULL;

        TRACERY(FASTFILE_T_ENTRY, {
                printf("\nFastFile_Read()");
        });

        /*Did we write end-condition bytes over file data?*/
        if ((pHandle->FeedoverSize != 0) && (pHandle->PostSize != 0)) {
                /*Yes, reinstate the data at the end of the old buffer*/
                memcpy(pHandle->pFeedover, 
                       pHandle->pPostSave, 
                       pHandle->PostSize);

                TRACERY(FASTFILE_T_FEEDOVER, {
                        printf("\nReinstate %u end bytes at %p", 
                               pHandle->PostSize, 
                               pHandle->pFeedover);
                });

        }


GetBuffer:
        /*While prefix buffer is too small for precond bytes + feedover...*/
        while (pHandle->PrefixSize < (pHandle->PreBytes + 
                                      pHandle->FeedoverSize)) {
                /*... double the  prefix buffer size*/
                pHandle->PrefixSize *= 2;

                /*Have we overflowed the size variable?*/
                if (pHandle->PrefixSize == 0) {
                        /*Sorry, can't set up prefix properly*/
                        return FALSE;
                }

        }

        /*Ensure the file buffer is always 4 times larger than the prefix*/
        if (pHandle->PrefixSize != (pHandle->BufSize / 4)) {
                /*Yes, change (increase) the buffer size*/
                pHandle->BufSize = pHandle->PrefixSize * 4;
                if ((pHandle->BufSize / 4) != pHandle->PrefixSize) {
                        /*Sorry, unable to think about such large sizes*/
                        return FALSE;
                }

                /*Remember we're discarding the current buffer (if any)*/
                pDiscardedBuffer = pHandle->pBuf;
                pHandle->pBuf = NULL;

                TRACERY(FASTFILE_T_FEEDOVER, {
                        printf("\nLarge prefix, new buffer size: %u", 
                               pHandle->BufSize);
                });

        }

        /*Have we got a buffer to receive the file?*/
        if (pHandle->pBuf == NULL) {
                /*No, allocate one now, including prefix and post bytes*/
                AllocSize = pHandle->PrefixSize + pHandle->BufSize + 
                        pHandle->SpaceAfterEnd;

                /*Round up buffer size to be a multiple of page size*/
                AllocSize += gFastFile.PageSize - 1;
                AllocSize -= AllocSize % gFastFile.PageSize;

                pHandle->pBuf = (CHAR *) valloc(AllocSize);

                TRACERY(FASTFILE_T_BUFFER, {
                        printf("Valloc size:%u pBuf:%p\n", 
                               AllocSize, pHandle->pBuf);
                });

                /*Did we obtain memory to operate?*/
                if (pHandle->pBuf == NULL) {
                        /*Sorry, unable to obtain buffer*/
                        return FALSE;
                }

        }

        /*Determine actual start of buffer, counting bytes from prev buffer*/
        pFirstByte = pHandle->pBuf + pHandle->PrefixSize;
        *ppBuf = pFirstByte - pHandle->FeedoverSize;
        *pBufferOffset = pHandle->BufPos - pHandle->FeedoverSize;

        /*Are we feeding over bytes from the previous buffer?*/
        if (pHandle->FeedoverSize != 0) {
                /*Yes, move them now as otherwise we'd overwrite them*/
                memmove(pFirstByte - pHandle->FeedoverSize, 
                       pHandle->pFeedover, 
                       pHandle->FeedoverSize);

                /*Remember new position in case we need to move it again*/
                pHandle->pFeedover = pFirstByte - pHandle->FeedoverSize;

                TRACERY(FASTFILE_T_FEEDOVER, {
                        printf("\nFeedover data moved to %p: %u", 
                               pHandle->pFeedover, 
                               pHandle->FeedoverSize);
                });

        }

        /*Did we shift to a new buffer and still know about the old one?*/
        if (pDiscardedBuffer != NULL) {
                /*Yes, free it (now we've copied any feedover bytes from it)*/
                free(pDiscardedBuffer);
                pDiscardedBuffer = NULL;
        }

        /*Use map only if: (a) File is mappable,*/
        /*                 (b) File offset is on a page boundary, and*/
        /*                 (c) At least BufSize bytes remain to process*/
        UseMap = pHandle->MayMap;
        UseMap = UseMap && ((pHandle->BufPos % gFastFile.PageSize) == 0);
        UseMap = UseMap && (pHandle->FileStats.st_size > 
                                    (pHandle->BufPos + pHandle->BufSize));
        if (UseMap) {
                /*Okay, try to map file into memory buffer*/
                pMemBuf = mmap(pFirstByte, 
                               pHandle->BufSize, 
                               PROT_READ | PROT_WRITE, 
                               MAP_PRIVATE | MAP_FIXED, 
                               pHandle->FileNr, 
                               pHandle->BufPos);

                /*Did we succeed?*/
                if (pMemBuf != (char *) pFirstByte) {
                        /*No, but because we mapped at the wrong address?*/
                        UseMap = FALSE;
                        if (pMemBuf != MAP_FAILED) {
                                /*Yes, undo map so we may read*/
                                munmap(pMemBuf, pHandle->BufSize);
                        }
                }
                
                TRACERY(FASTFILE_T_BUFFER, {
                        printf("\nMap file at %p: %p", 
                               pFirstByte, pMemBuf);
                });

        }

        /*Did we use the memory-mapped interface?*/
        if (UseMap) {
                /*Yes, update buffer position*/
                pHandle->BufPos += pHandle->BufSize;
                NrChars = pHandle->BufSize;
                pLastByte = &pFirstByte[pHandle->BufSize - 1];

        } else {
                /*No, have we just abandoned the memory-mapped interface?*/
                if (pHandle->MayMap) {
                        /*Yes, issue file seek to sync normal file handing*/
                        lseek(pHandle->FileNr, pHandle->BufPos, SEEK_SET);
                        pHandle->MayMap = FALSE;

                }

                /*Read file using normal (ANSI standard) interface*/
                NrChars = gFastFile.pReadFn(pHandle->FileNr,
                                            pFirstByte, 
                                            pHandle->BufSize);
                pHandle->BufPos += NrChars;
                pLastByte = &pFirstByte[NrChars - 1];

        }

        /*Did we run out of file to read?*/
        if (NrChars == 0) {
                /*Yes, file finished*/
                *ppBuf = NIL;
                *pBufferSize = 0;
                pHandle->FeedoverSize = 0;
                pHandle->pFeedover = NULL;
                return TRUE;
        }

        /*Add feedover characters to buffer pointer and size*/
        NrChars += pHandle->FeedoverSize;
        pFirstByte -= pHandle->FeedoverSize;

        /*Do we have an incomplete last line (and client wants line mode)?*/
        if ((NrChars >= pHandle->BufSize) && 
                        ((pHandle->Flags & FASTFILE_P_MODE_MASK) == 
                         FASTFILE_P_MODE_LINE)) {
                /*Yes, use memrchr to search for last LF in buffer*/
                pLastLF = (CHAR *) memrchr(pLastByte, 
                                           LF, 
                                           pHandle->BufSize);

                /*Did we find any line break characters?*/
                if (pLastLF == NULL) {
                        /*No, increase buffer size and try again*/
                        pHandle->pBuf = NULL;
                        pHandle->BufSize *= 4;

                        TRACERY(FASTFILE_T_FEEDOVER, {
                                printf("\nNo LF, increase size");
                        });

                        goto GetBuffer;
                }

                /*Remember how many bytes are in first line of next buffer*/
                pHandle->FeedoverSize = (INT) (pLastByte - pLastLF);
                pHandle->pFeedover    = pLastLF + 1;

                TRACERY(FASTFILE_T_FEEDOVER, {
                        printf("\nFeedover size %u at %p", 
                               pHandle->FeedoverSize, 
                               pHandle->pFeedover);
                });

                /*Remove incomplete line from buffer*/
                NrChars -= pLastByte - pLastLF;
                pLastByte = pLastLF;

        } else {
                /*Buffer is unedited*/
                pHandle->FeedoverSize = 0;
                pHandle->pFeedover = NULL;

                TRACERY(FASTFILE_T_FEEDOVER, {
                        printf("\nNo feedover");
                });

        }

        /*Were we asked to perform any start-of-buffer conditioning?*/
        if (pHandle->PreSize != 0) {
                /*Yes, set bytes now*/
                memcpy(pFirstByte - pHandle->PreSize, 
                       pHandle->pPreData, 
                       pHandle->PreSize);

                /*?? Could optimise this copy if pFirstByte never varies*/
        }

        /*Were we asked to perform any end-of-buffer conditioning?*/
        if (pHandle->PostSize != 0) {
                /*Yes, are there bytes needing to be saved (partial line)?*/
                if (pHandle->FeedoverSize != 0) {
                        /*Yes, save read bytes before modifying*/
                        memcpy(pHandle->pPostSave, 
                               pHandle->pFeedover, 
                               pHandle->PostSize);

                        /*?? Could reduce copy if Save < PostSize*/
                }

                /*Write client's bytes to end of buffer*/
                memcpy(pLastByte + 1, 
                       pHandle->pPostData, 
                       pHandle->PostSize);

                /*?? Could possibly eliminate this copy in some cases*/
        }

        /*Report fetched characters to caller*/
        *pBufferSize = NrChars;
        return TRUE;

} /*Read*/


/************************************************************************/
/*                                                                      */
/*      ReadFunction -- Receive function to read bytes from a file      */
/*                                                                      */
/*      FastFile assumes that the ANSI read function is sufficient      */
/*      and correct to fetch file lines.  If this assumption is         */
/*      incorrect for some reason, the client may supply an             */
/*      alternative function via this interface.                        */
/*                                                                      */
/*      This interface was included when behoffski found that           */
/*      the operation of read for pipes was incorrect,                  */
/*      whereas _read worked correctly under TopSpeed C v3.10.          */
/*                                                                      */
/************************************************************************/
public_scope void
FastFile_ReadFunction(FastFile_ReadFn *pReadFn)
{
        /*Remember function in private variable*/
        gFastFile.pReadFn = pReadFn;

} /*ReadFunction*/


/************************************************************************/
/*                                                                      */
/*      Stats -- Report file stats (from fstat(2))                      */
/*                                                                      */
/************************************************************************/
public_scope void
FastFile_Stats(FastFile_Context *pHandle, struct stat **ppStats)
{
        /*Report stats structure obtained when we opened the file*/
        *ppStats = &pHandle->FileStats;

} /*Stats*/


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
FastFile_TraceryLink(void *pObject, UINT Opcode, ...)
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
                *ppInfoBlock = &gFastFile.TraceInfo;
                break;

        case TRACERY_REGCMD_GET_DEFAULT_FLAGS:
                pDefaultFlags  = va_arg(ap, LWORD *);
                *pDefaultFlags = 
                        FASTFILE_T_ENTRY;
                break;

        case TRACERY_REGCMD_GET_EDIT_LIST:
                ppEditList  = va_arg(ap, Tracery_EditEntry **);
                *ppEditList = gFastFile_TraceryEditDefs;
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

