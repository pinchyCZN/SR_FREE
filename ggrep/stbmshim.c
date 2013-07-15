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

#include "ascii.h"
#include <compdef.h>
#include "matcheng.h"
#include "platform.h"
#include "regexp00.h"
#include "stbm.h"
#include "stbmshim.h"
#include <stdlib.h>

/************************************************************************/
/*                                                                      */
/*      Init -- Prepare module for operation                            */
/*                                                                      */
/************************************************************************/
void
STBMShim_Init(void)
{
} /*Init*/


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
/*      The check tries to be as light and fast as possible, as it      */
/*      can be called multiple times.                                   */
/*                                                                      */
/*      Returns NULL if unable to search for the specified RE           */
/*      using the STBM algorithm.                                       */
/*                                                                      */
/************************************************************************/
public_scope STBM_SearchSpec *
STBMShim_Pattern(RegExp_Specification *pRESpec, UINT *pPatternLength, 
                 BOOL *pIgnoreCase, BYTE *pTrailingLiteral)
{
        RE_ELEMENT *pInitialRE = &pRESpec->CodeList[0];
        RE_ELEMENT *pRE = pInitialRE;
        CHAR *pPattern;
        char *pPat;
        BOOL IgnoreCase = FALSE;
        STBM_SearchSpec *pSpec;
        LWORD Flags;
        RE_ELEMENT *pEndRE;

        /*Is the first element a literal?*/
        if (REGEXP_TYPE_R(*pRE) != REGEXP_TYPE_LITERAL) {
                /*No, cannot optimise*/
                return NULL;
        }

        /*Is the pattern too short to be searched efficiently?*/
        if (pRESpec->NrStates < 3) {
                /*Yes, cannot optimise*/
                return NULL;
        }

        /*Obtain case-sensitivity sense from first literal*/
        if ((*pRE & REGEXP_CASE_MASK) == REGEXP_CASE_INSENSITIVE) {
                IgnoreCase = TRUE;
        }
        *pIgnoreCase = IgnoreCase;

        /*Loop through all components, checking that STBM is applicable*/
        for (;;) {
                switch (REGEXP_TYPE_R(*pRE)) {
                case REGEXP_TYPE_END_OF_EXPR:
                        /*Okay, entire pattern can be searched using STBM*/
                        pEndRE = pRE;
                        goto PatternOkay;
                        /*break;*/

                case REGEXP_TYPE_LITERAL:
                        /*Found literal: consider it further below*/
                        break;

                default:
                        /*Anything else is too hard for STBM search*/
                        return NULL;

                }

                /*Literal found: does it have nonlinear modifications?*/
                if (REGEXP_REPEAT_R(*pRE) != REGEXP_REPEAT_MATCH_SELF) {
                        /*Yes, cannot match as a simple string*/
                        return NULL;
                }

                /*Is literal's case sensitivity selection consistent?*/
                if ((*pRE & REGEXP_CASE_MASK) == REGEXP_CASE_INSENSITIVE) {
                        if (! IgnoreCase) {
                                /*Sorry, case sensitivity isn't consistent*/
                                return FALSE;
                        }
                } else {
                        if (IgnoreCase) {
                                /*Sorry, case sensitivity isn't consistent*/
                                return FALSE;
                        }
                }

                /*Does literal require more than an octet to be stored?*/
                if (REGEXP_LITERAL_R(*pRE) > 255) {
                        /*Sorry, cannot treat Unicode characters simply*/
                        return FALSE;
                }

                /*This element is okay: Look at next element, if any*/
                pRE++;

        }                

        /*Control should never reach here*/

PatternOkay:
        /*Extract text from literals into simple string for STBM*/
        pPattern = Platform_SmallMalloc(pRESpec->NrStates + 1);
        pPat = pPattern;
        for (pRE = pInitialRE; pRE != pEndRE; ) {
                /*Add literal to string*/
                *pPat++ = REGEXP_LITERAL_R(*pRE++);
        }

        /*Trim pattern to 64 characters as that's all ScanFile handles (sigh)*/
        if ((pPat - pPattern) > 64) {
                pPat = pPattern + 64;
        }

        /*Add NUL to pattern string in case a C user tries to print it*/
        *pPat = NUL;

        Flags = 0;
        if (IgnoreCase) {
                Flags |= STBM_SEARCH_CASE_INSENSITIVE;
        }

        /*Prepare data structures for optimised search*/
        if (! STBM_Compile(pPat - pPattern, pPattern, Flags, &pSpec)) {
                /*Sorry, unable to prepare optimised version*/
                Platform_SmallFree(pPattern);
                return FALSE;
        }

        /*Prepared search successfully, report search handle to caller*/
        *pPatternLength = pPat - pPattern;
        *pTrailingLiteral = pPat[-1];
        return pSpec;

} /*Pattern*/


/************************************************************************/
/*                                                                      */
/*      Destroy -- Ret rid of STBM search spec supplied by Pattern      */
/*                                                                      */
/************************************************************************/
public_scope void
STBMShim_Destroy(STBM_SearchSpec *pSearchSpec)
{
        CHAR *pPattern;

        /*Obtain pattern we malloc'd during creation*/
        STBM_GetPattern(pSearchSpec, &pPattern);

        /*Get rid of search spec and pattern text*/
        STBM_Destroy(pSearchSpec);
        Platform_SmallFree(pPattern);

} /*Destroy*/


/************************************************************************/
/*                                                                      */
/*      Search -- Search for pattern in buffer (case sensitive)         */
/*                                                                      */
/************************************************************************/
public_scope BOOL
STBMShim_Search(MatchEng_Spec *pSearchSpec, 
                BYTE *pTextStart, 
                MatchEng_Details *pDetails)
{
        BOOL Found;
        UINT BufferLength;

        STBM_SearchSpec *pSTBM = (STBM_SearchSpec *) pSearchSpec->pSpare1;
        CHAR *pText;

        /*Obtain buffer length using buffer current and end pointers*/
        BufferLength = pSearchSpec->pAfterEndOfBuffer - (BYTE *) pTextStart;

        pText = STBM_Search(pSTBM, pTextStart, BufferLength);
        Found = (pText != NULL);
        pDetails->pLineStart = NULL;
        pDetails->pMatchStart = pText;
        pDetails->pMatchEnd   = pText + pSearchSpec->PatternLength;

        return Found;

} /*Search*/


/************************************************************************/
/*                                                                      */
/*      SearchInCase -- Search for pattern in buffer (case INsensitive) */
/*                                                                      */
/************************************************************************/
public_scope BOOL
STBMShim_SearchInCase(MatchEng_Spec *pSearchSpec, 
                      BYTE *pTextStart, 
                      MatchEng_Details *pDetails)
{
        BOOL Found;
        UINT BufferLength;

        STBM_SearchSpec *pSTBM = (STBM_SearchSpec *) pSearchSpec->pSpare1;
        CHAR *pText;

        /*Obtain buffer length using buffer current and end pointers*/
        BufferLength = pSearchSpec->pAfterEndOfBuffer - (BYTE *) pTextStart;

        pText = STBM_SearchCI(pSTBM, pTextStart, BufferLength);
        Found = (pText != NULL);
        pDetails->pLineStart = NULL;
        pDetails->pMatchStart = pText;
        pDetails->pMatchEnd   = pText + pSearchSpec->PatternLength;

        return Found;

} /*SearchInCase*/


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
/*      particular, the static guard is always the second-to-last       */
/*      character of the string.                                        */
/*                                                                      */
/************************************************************************/
public_scope BOOL
STBMShim_SearchTBM(MatchEng_Spec *pSearchSpec, 
                BYTE *pTextStart, 
                MatchEng_Details *pDetails)
{
        BOOL Found;
        UINT BufferLength;

        STBM_SearchSpec *pSTBM = (STBM_SearchSpec *) pSearchSpec->pSpare1;
        CHAR *pText;

        /*Obtain buffer length using buffer current and end pointers*/
        BufferLength = pSearchSpec->pAfterEndOfBuffer - (BYTE *) pTextStart;

        pText = STBM_SearchTBM(pSTBM, pTextStart, BufferLength);
        Found = (pText != NULL);
        pDetails->pLineStart = NULL;
        pDetails->pMatchStart = pText;
        pDetails->pMatchEnd   = pText + pSearchSpec->PatternLength;

        return Found;

} /*SearchTBM*/


