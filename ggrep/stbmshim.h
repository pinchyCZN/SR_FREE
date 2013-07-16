/************************************************************************/
/*                                                                      */
/*      STBMShim -- Interface module to join STBM to ScanFile           */
/*                                                                      */
/*      This module is a shim (interface adaptor) between the           */
/*      self-tuned Boyer-Moore search algorithm and the file            */
/*      scanning control interface wielded by ScanFile.  Using          */
/*      a shim like this is part of creating truly reusable             */
/*      software: this shim especially allows STBM to be written        */
/*      in a simple and portable fashion, which is useful as it         */
/*      will have application outside of ggrep.                         */
/*                                                                      */
/*      Copyright (C) 1995-2000 Grouse Software.  All rights reserved.  */
/*      Written for Grouse by behoffski (Brenton Hoff).                 */
/*                                                                      */
/*      Free software: no warranty; use anywhere is ok; spread the      */
/*      sources; note any mods; share variations and derivatives        */
/*      (including sending to behoffski@grouse.com.au).                 */
/*                                                                      */
/************************************************************************/

#ifndef STBMSHIM_H
#define STBMSHIM_H

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/

#include <compdef.h>
#include "matcheng.h"
#include "regexp00.h"
#include "stbm.h"

/************************************************************************/
/*                                                                      */
/*      Init -- Prepare module for operation                            */
/*                                                                      */
/************************************************************************/
void
STBMShim_Init(void);


/************************************************************************/
/*                                                                      */
/*      Pattern -- Convert RegExp pattern to STBM search spec           */
/*                                                                      */
/*      The conversion checks that the supplied RE is a simple          */
/*      string, where "simple string" means:                            */
/*              - All elements of the RE are literals, (no classes,     */
/*                    no anchors, no match-any components), and         */
/*              - All elements merely match themselves (no repeats      */
/*                    or optional matching), and                        */
/*              - Case insensitivity is consistent: either no           */
/*                    element is case-insensitive, or else all          */
/*                    components are case-insensitive.                  */
/*                                                                      */
/*      Returns NULL if unable to search for the specified RE           */
/*      using the STBM algorithm.                                       */
/*                                                                      */
/*      If successful, reports the pattern length, whether the          */
/*      search is case sensitive or case insensitive, and the           */
/*      last literal of the pattern.  The buffer must be conditioned    */
/*      to end with at least 64 copies of the trailing literal,         */
/*      as part of the aggressive optimisation is to only check         */
/*      for the end of the buffer where the last literal has matched.   */
/*                                                                      */
/************************************************************************/
STBM_SearchSpec *
STBMShim_Pattern(RegExp_Specification *pRESpec, UINT *pPatternLength, 
                 BOOL *pIgnoreCase, BYTE *pTrailingLiteral);


/************************************************************************/
/*                                                                      */
/*      Destroy -- Ret rid of STBM search spec supplied by Pattern      */
/*                                                                      */
/************************************************************************/
void
STBMShim_Destroy(STBM_SearchSpec *pSearchSpec);


/************************************************************************/
/*                                                                      */
/*      Search -- Search for pattern in buffer (case sensitive)         */
/*                                                                      */
/************************************************************************/
BOOL
STBMShim_Search(MatchEng_Spec *pSearchSpec, 
                BYTE *pTextStart, 
                MatchEng_Details *pDetails);


/************************************************************************/
/*                                                                      */
/*      SearchInCase -- Search for pattern in buffer (case INsensitive) */
/*                                                                      */
/************************************************************************/
BOOL
STBMShim_SearchInCase(MatchEng_Spec *pSearchSpec, 
                      BYTE *pTextStart, 
                      MatchEng_Details *pDetails);


/************************************************************************/
/*                                                                      */
/*      SearchTBM -- Search for pattern in buffer (use tuned BM)        */
/*                                                                      */
/*      This interface is provided merely for comparison/reference.     */
/*      It allows the Tuned Boyer-Moore algorithm to be used to         */
/*      perform fixed-string searches so that direct comparisons        */
/*      with behoffski's self-tuned version can be made.                */
/*      Otherwise, it's too hard to separate algorithm performance      */
/*      from the performance of the program control.                    */
/*                                                                      */
/*      The variant of the Tuned BM search implemented here is          */
/*      based on the version implemented by GNU Grep.  In               */
/*      particular, the static guard is always the first character      */
/*      of the string (we don't attempt to be smart when choosing       */
/*      the guard character position).                                  */
/*                                                                      */
/************************************************************************/
BOOL
STBMShim_SearchTBM(MatchEng_Spec *pSearchSpec, 
                BYTE *pTextStart, 
                MatchEng_Details *pDetails);


#ifdef __cplusplus
}
#endif /*__cplusplus*/
#endif /*STBMSHIM_H*/
