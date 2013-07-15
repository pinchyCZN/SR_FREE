/************************************************************************/
/*                                                                      */
/*      Platform -- Handle platform-specific functions (Linux version)  */
/*                                                                      */
/*      This module is part of behoffski's attempt to provide           */
/*      utterly portable sources using link-time or run-time binding    */
/*      instead of using the preprocessor at compile time.              */
/*                                                                      */
/*      Copyright (C) 1995-2000 Grouse Software.  All rights reserved.  */
/*      Written for Grouse by behoffski (Brenton Hoff).                 */
/*                                                                      */
/*      Free software: no warranty; use anywhere is ok; spread the      */
/*      sources; note any mods; share variations and derivatives        */
/*      (including sending to behoffski@grouse.com.au).                 */
/*                                                                      */
/************************************************************************/

#ifndef PLATFORM_H
#define PLATFORM_H

#include "compdef.h"
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/

/************************************************************************/
/*                                                                      */
/*      ProgramName -- Report the name used to invoke us                */
/*                                                                      */
/*      Reports the program name, based on the non-directory bits       */
/*      of the filename reported by argv[0].                            */
/*                                                                      */
/************************************************************************/
CHAR *
Platform_ProgramName(void);


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
void *
Platform_SmallMalloc(size_t size);


/************************************************************************/
/*                                                                      */
/*      SmallFree -- Free space issued by SmallMalloc                   */
/*                                                                      */
/************************************************************************/
public_scope void
Platform_SmallFree(void *pMem);



#ifdef __cplusplus
}
#endif /*__cplusplus*/

#endif /*PLATFORM_H*/
