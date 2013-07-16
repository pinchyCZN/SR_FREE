/************************************************************************/
/*                                                                      */
/*      RETable -- Efficient language for implementing regexp searches  */
/*                                                                      */
/*      This module converts a regular expression description           */
/*      from a compact form (as created by RegExp) to a                 */
/*      high-performance table-driven form (as described by             */
/*      MatchEng).  Several optimisations are available that            */
/*      can significantly improve search performance.  These            */
/*      optimisations fall into two categories: some try to             */
/*      minimise backtracking initiated by iterative states             */
/*      by analysing the state table following the iterative            */
/*      state; others use string or byte searches to improve            */
/*      the speed of scanning the text for possible matches.            */
/*                                                                      */
/*      Copyright (C) 1995-2000 Grouse Software.  All rights reserved.  */
/*      Written for Grouse by behoffski (Brenton Hoff).                 */
/*                                                                      */
/*      Free software: no warranty; use anywhere is ok; spread the      */
/*      sources; note any mods; share variations and derivatives        */
/*      (including sending to behoffski@grouse.com.au).                 */
/*                                                                      */
/************************************************************************/

#ifndef RETABLE_H
#define RETABLE_H

#include <compdef.h>
#include "matcheng.h"
#include "regexp00.h"

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/

/*Debugging options*/
#define RETABLE_D_CONTROL               BIT0

/************************************************************************/
/*                                                                      */
/*      Init -- Prepare module for operation                            */
/*                                                                      */
/************************************************************************/
void
RETable_Init(void);


/************************************************************************/
/*                                                                      */
/*      Start -- Begin managing what has to be managed                  */
/*                                                                      */
/************************************************************************/
BOOL
RETable_Start(void);


/************************************************************************/
/*                                                                      */
/*      AllocBacktrack -- (Re-)Allocate space for backtracking          */
/*                                                                      */
/*      The backtracking stack must have two entries per buffer byte    */
/*      (plus a couple of overhead entries) to handle the worst-case    */
/*      search request.  This function frees the previous stack         */
/*      (if allocated), then allocates a stack large enough to          */
/*      handle the specified size.  The memory consumption of this      */
/*      approach is horrifically large, but it's the best we can        */
/*      do until ggrep is rewritten to use DFAs.                        */
/*                                                                      */
/************************************************************************/
public_scope BOOL
RETable_AllocBacktrack(MatchEng_Spec *pSpec, UINT32 TextSize);


/************************************************************************/
/*                                                                      */
/*      Expand -- Convert "compact" RE codes to optimised table form    */
/*                                                                      */
/*      This function is used to prepare for table-driven pattern       */
/*      match searching.  It creates a fairly verbose form of the       */
/*      regular expression which is highly optimised for match          */
/*      searches.  A typical pattern will occupy 8 to 20 kbytes         */
/*      of RAM in its expanded form.                                    */
/*                                                                      */
/*      Flags allows configuration of RE interpretation,                */
/*      including selecting whether CR/LF is a line terminator.         */
/*      The flags also provide control over other match engine          */
/*      behaviours, such as line counting.  See definition of           */
/*      flag bits in matcheng.h.                                        */
/*                                                                      */
/*      The function returns FALSE if unable to prepare an              */
/*      expanded version of the RE.                                     */
/*                                                                      */
/************************************************************************/
BOOL
RETable_Expand(RegExp_Specification *pRESpec, 
               LWORD Flags, 
               MatchEng_Spec **ppSpec);


#ifdef __cplusplus
}
#endif /*__cplusplus*/

#endif /*RETABLE_H*/
