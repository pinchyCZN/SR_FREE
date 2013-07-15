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
/*      provides substantial facilities for manipulating the            */
/*      memory immediately before and after the file buffer, as         */
/*      this flexibility allows the application to optimise its         */
/*      buffer handling.                                                */
/*                                                                      */
/*      <-------------------- File data ------------------->            */
/*      | (1) | (2) |     ...                        | (n) |            */
/*      +-----+-----+                                +-----+            */
/*               v                                                      */
/*               +------------------+                                   */
/*                                  v                                   */
/*      ------ memory map ------------------------------------------    */
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

#ifndef FASTFILE_H
#define FASTFILE_H

#include <compdef.h>
#include <stdlib.h>
#include <sys/stat.h>
#include "tracery.h"
#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/

/*Flags to configure operation (used when opening file)*/

#define FASTFILE_P_MODE_MASK            BIT0
#define FASTFILE_P_MODE_RAW             0x00uL
#define FASTFILE_P_MODE_LINE            BIT0

/*Handle containing context for a file*/

typedef struct FastFile_FileContextStruct FastFile_Context;


/*Prototype for "read" function (ANSI-compatible)*/
typedef int (FastFile_ReadFn)(int Handle, void *pBuf, unsigned NrBytes);

/************************************************************************/
/*                                                                      */
/*      Init -- Prepare module for operation                            */
/*                                                                      */
/************************************************************************/
void
FastFile_Init(void);


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
FastFile_Start(size_t BufferSize);


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
/*      whereas _read worked correctly (for TopSpeed C v3.10).          */
/*                                                                      */
/************************************************************************/
void
FastFile_ReadFunction(FastFile_ReadFn *pReadFn);


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
/*      the pStat parameter if desired, or may specify NULL             */
/*      if not interested.  This parameter is provided so the           */
/*      application may minimise to an absurd degree the code           */
/*      execution times.                                                */
/*                                                                      */
/*      Returns FALSE if the file was not able to be opened.            */
/*                                                                      */
/************************************************************************/
BOOL
FastFile_Open(CHAR *pFilename, 
              size_t BufferSizeMin, 
              LWORD Flags, 
              struct stat *pStat, 
              FastFile_Context **ppHandle);


/************************************************************************/
/*                                                                      */
/*      Reopen -- Close existing file and open new file for access      */
/*                                                                      */
/*      This function only exists to reduce the overall cost of         */
/*      file handling where multiple files each use the same            */
/*      buffer conditioning.  The new file receives the same            */
/*      configuration settings as the previous file.                    */
/*      Returns FALSE if the file was not able to be opened (the        */
/*      existing file may *not* be accessed again).                     */
/*                                                                      */
/*      The client may receive details of the file's stauus via         */
/*      the pStat parameter if desired, or may specify NULL             */
/*      if not interested.  This parameter is provided so the           */
/*      application may minimise to an absurd degree the code           */
/*      execution times.                                                */
/*                                                                      */
/************************************************************************/
BOOL
FastFile_Reopen(FastFile_Context *pHandle, 
              CHAR *pFilename, 
              struct stat *pStat);


/************************************************************************/
/*                                                                      */
/*      StartCondition -- Specify how to set up start of buffer         */
/*                                                                      */
/*      This function configures how FastFile sets up the start         */
/*      of each buffer before it is retrieved by the client.            */
/*      SpaceBeforeStart specifies the minimum bytes before the         */
/*      buffer start address that must be available for the client      */
/*      to use.  PreBytesNr and pPreData describe values that           */
/*      will be written immediately preceding the buffer start          */
/*      (note this area overlaps SpaceBeforeStart).                     */
/*                                                                      */
/*      SpaceBeforeStart may be 0 if no memory reservation is           */
/*      required, and similarly PreBytesNr may be 0 if no memory        */
/*      needs to be set.  The client must maintain the memory           */
/*      referenced by pPreData unchanged; if the data is changed,       */
/*      StartCondition should be called again.                          */
/*                                                                      */
/*      The parameters specified apply to subsequent buffers            */
/*      retrieved by Read; typically this function is called            */
/*      once immediately after Open.                                    */
/*                                                                      */
/************************************************************************/
BOOL
FastFile_StartCondition(FastFile_Context *pHandle, 
                        UINT SpaceBeforeStart, 
                        UINT PreBytesNr, 
                        BYTE *pPreData);


/************************************************************************/
/*                                                                      */
/*      EndCondition -- Specify how to set up end of buffer             */
/*                                                                      */
/*      Specifies how FastFile will prepare the end of each             */
/*      buffer.  LineMode selects Raw mode or Line mode.                */
/*      SpaceAfterEnd specifies how many bytes after the last           */
/*      byte of file data are available for client to use.              */
/*      PostBytesNr and pPostData specify bytes to write                */
/*      immediately following the buffer (note overlap of this          */
/*      area with SpaceAfterEnd).  As with StartCondition, 0 may        */
/*      be specified for either SpaceAfterEnd and/or PostBytesNr.       */
/*      The client must maintain the memory referenced by               */
/*      pPostData unchanged; if the data is changed, EndCondition       */
/*      should be called again.                                         */
/*                                                                      */
/*      The parameters specified apply to subsequent buffers            */
/*      retrieved by Read; typically this function is called            */
/*      once immediately after Open.                                    */
/*                                                                      */
/************************************************************************/
BOOL
FastFile_EndCondition(FastFile_Context *pHandle, 
                        UINT SpaceAfterEnd, 
                        UINT PostBytesNr, 
                        BYTE *pPostData);


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
              UINT32 *pBufferOffset);


/************************************************************************/
/*                                                                      */
/*      Stats -- Report file stats (from fstat(2))                      */
/*                                                                      */
/************************************************************************/
void
FastFile_Stats(FastFile_Context *pHandle, struct stat **ppStats);


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
FastFile_TraceryLink(void *pObject, UINT Opcode, ...);

#endif /*TRACERY_ENABLED*/


#ifdef __cplusplus
}
#endif /*__cplusplus*/

#endif /*FASTFILE_H*/
