/************************************************************************/
/*                                                                      */
/*      MatchGCC -- GNU C version of match engine                       */
/*                                                                      */
/*      This module implements the table-driven search using the        */
/*      "computed goto" extension of the GNU C compiler.  The           */
/*      result is code that is much faster than the ANSI C              */
/*      version, but which typically falls short of a fully             */
/*      hand-optimised version, as the compiler is not as               */
/*      aggressive as it could be in adopting the byte-oriented         */
/*      dispatch at the end of each action.                             */
/*                                                                      */
/*      Copyright (C) 1995-2000 Grouse Software.  All rights reserved.  */
/*      Written for Grouse by behoffski (Brenton Hoff).                 */
/*                                                                      */
/*      Free software: no warranty; use anywhere is ok; spread the      */
/*      sources; note any mods; share variations and derivatives        */
/*      (including sending to behoffski@grouse.com.au).                 */
/*                                                                      */
/************************************************************************/

#ifndef MATCHGCC_H
#define MATCHGCC_H

#include <compdef.h>
#include "matcheng.h"
#include "tracery.h"

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/

/************************************************************************/
/*                                                                      */
/*      Init -- Prepare module for operation                            */
/*                                                                      */
/************************************************************************/
void
MatchGCC_Init(void);


/************************************************************************/
/*                                                                      */
/*      Match -- Perform search and report success                      */
/*                                                                      */
/*      The caller must obtain the addresses of the action entry        */
/*      points from this routine, and use these addresses when          */
/*      creating the jump tables.  In order to obtain these             */
/*      addresses, call this function with pTestStart set to NULL       */
/*      and with the pSpec parameter pointing to the the                */
/*      MatchEng_ActionCodes struct to receive the addresses.           */
/*      This is a bit hacky, I know, but it keeps the code              */
/*      disruption and the runtime performance penalty low.             */
/*                                                                      */
/************************************************************************/
BOOL 
MatchGCC_Match(MatchEng_Spec *pSpec, BYTE *pTextStart,
                MatchEng_Details *pDetails);


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
MatchGCC_TraceryLink(void *pObject, UINT Opcode, ...);

#endif /*TRACERY_ENABLED*/


#ifdef __cplusplus
}
#endif /*__cplusplus*/

#endif /*MATCHGCC_H*/
