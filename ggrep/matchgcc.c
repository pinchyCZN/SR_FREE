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
/*      See behoffski's Perl script for optimisations applied           */
/*      to the output of GCC to reinstate some of the benefits          */
/*      of the original architecture.  Although this approach           */
/*      isn't optimal, it represents the best engineering               */
/*      tradeoff at this time: Freedom to play with this                */
/*      routine is more important than taking any particular            */
/*      version and attempting to optimise it to the max.               */
/*      Later, once the dust has settled and everyone has               */
/*      finished playing, the result can be hand-optimised for          */
/*      release.                                                        */
/*                                                                      */
/*      In the long run, the best engineering compromise would          */
/*      be to upgrade GCC to be smarter about the optimisations         */
/*      identified here, and further to be able to identify and         */
/*      explicitly implement the Grouse FSA, but this is                */
/*      certainly not simple.                                           */
/*                                                                      */
/*      Copyright (C) 1995-2000 Grouse Software.  All rights reserved.  */
/*      Written for Grouse by behoffski (Brenton Hoff).                 */
/*                                                                      */
/*      Free software: no warranty; use anywhere is ok; spread the      */
/*      sources; note any mods; share variations and derivatives        */
/*      (including sending to behoffski@grouse.com.au).                 */
/*                                                                      */
/************************************************************************/

#include <ctype.h>
#include "ascii.h"
#include <compdef.h>
#include "matcheng.h"
#include "matchgcc.h"
#include <memory.h>
#include <stdarg.h>
#include <stdio.h>
#include "tracery.h"

#define IS_WORD(ch)  (isalnum(ch) || ((ch) == '_'))

#ifdef TRACERY_ENABLED

typedef struct {
        /*Tracery control block for this object*/
        Tracery_ObjectInfo TraceInfo;

} MATCHGCC_MODULE_CONTEXT;

module_scope MATCHGCC_MODULE_CONTEXT gMatchGCC;

#define TRACERY_MODULE_INFO             (gMatchGCC.TraceInfo)

/*Trace debug options*/
#define MATCHGCC_T_CONTROL              BIT0
#define MATCHGCC_T_ACTION               BIT1
#define MATCHGCC_T_START_BUFFER         BIT2

module_scope Tracery_EditEntry gMatchGCC_TraceryEditDefs[] = {
        {"A", MATCHGCC_T_ACTION,       MATCHGCC_T_ACTION,  "Trace  actions"}, 
        {"a", MATCHGCC_T_ACTION,       0x00,               "Ignore actions"}, 
        {"C", MATCHGCC_T_CONTROL,      MATCHGCC_T_CONTROL, "Trace  control"}, 
        {"c", MATCHGCC_T_CONTROL,      0x00,               "Ignore control"}, 
        {"S", MATCHGCC_T_START_BUFFER, MATCHGCC_T_START_BUFFER, 
                                                           "Trace  startup"}, 
        {"s", MATCHGCC_T_START_BUFFER, 0x00,               "Ignore startup"}, 
        TRACERY_EDIT_LIST_END
};

#endif /*TRACERY_ENABLED*/

/*Macros to perform threaded dispatch so we may play with the compiler*/

#define NEXT_ACTION do{BYTE NextCh = *pText++; goto *pTab[NextCh]; } while (0)

#define DO_SKIP(n) do {pText += (n) - 1; goto *pTab[*pText++]; } while (0)




/************************************************************************/
/*                                                                      */
/*      Init -- Prepare module for operation                            */
/*                                                                      */
/************************************************************************/
public_scope void
MatchGCC_Init(void)
{
        /*Traces disabled by default*/
        TRACERY_CLEAR_ALL_FLAGS(&TRACERY_MODULE_INFO);

} /*Init*/


/************************************************************************/
/*                                                                      */
/*      Match -- Perform search and report success                      */
/*                                                                      */
/*      The caller must obtain the addresses of the action entry        */
/*      points from this routine, and use these addresses when          */
/*      creating the jump tables.  In order to obtain these             */
/*      addresses, call this function with pTestStart set to NULL       */
/*      and with the pSpec parameter pointing to the                    */
/*      MatchEng_ActionCodes struct to receive the addresses.           */
/*      This is a bit hacky, I know, but it keeps the code              */
/*      disruption and the runtime performance penalty low.             */
/*                                                                      */
/*      Note that the Perl script exploits the knowledge that           */
/*      this initialisation is at the start of the routine, and         */
/*      uses the initialisation to determine the FSA entry              */
/*      points that may be optimised.                                   */
/*                                                                      */
/************************************************************************/
public_scope BOOL 
MatchGCC_Match(MatchEng_Spec *pSpec, BYTE *pTextStart,
                MatchEng_Details *pDetails)
{
        /*Stack area for storing search context*/
        void **pStack = pSpec->pStack;
        BYTE *pText = (BYTE *) pTextStart;
        void **pTab = pSpec->pTables;
        BYTE *pStart = pText;
        UINT LineNr;
        UINT StrOptOffset;
        MatchEng_ActionCodes *pActions;
        UINT TableNr;
        UINT ByteStartJump;

        /*Have we been invoked without text to search?*/
        if (pTextStart == NULL) {
                /*Yes, this means to write action addresses via pSpec*/
                pActions = (MatchEng_ActionCodes *) pSpec;
                
                pActions->pSkipMax           = &&entrySKIP_MAX;
                pActions->pSkip17            = &&entrySKIP17;
                pActions->pSkip16            = &&entrySKIP16;
                pActions->pSkip15            = &&entrySKIP15;
                pActions->pSkip14            = &&entrySKIP14;
                pActions->pSkip13            = &&entrySKIP13;
                pActions->pSkip12            = &&entrySKIP12;
                pActions->pSkip11            = &&entrySKIP11;
                pActions->pSkip10            = &&entrySKIP10;
                pActions->pSkip9             = &&entrySKIP9;
                pActions->pSkip8             = &&entrySKIP8;
                pActions->pSkip7             = &&entrySKIP7;
                pActions->pSkip6             = &&entrySKIP6;
                pActions->pSkip5             = &&entrySKIP5;
                pActions->pSkip4             = &&entrySKIP4;
                pActions->pSkip3             = &&entrySKIP3;
                pActions->pSkip2             = &&entrySKIP2;
                pActions->pAgain             = &&entryAGAIN;
                pActions->pAdvance           = &&entryADVANCE;
                pActions->pStartMatchPush    = &&entrySTART_MATCH_PUSH;
                pActions->pStartMatchPushAdvance = 
                                        &&entrySTART_MATCH_PUSH_ADVANCE;
                pActions->pAgainPushAdvance  = &&entryAGAIN_PUSH_ADVANCE;
                pActions->pBackAndAdvance    = &&entryBACK_AND_ADVANCE;
                pActions->pAdvancePushZero   = &&entryADVANCE_PUSH_ZERO;
                pActions->pNoMatch           = &&entryNO_MATCH;
                pActions->pNoteLine          = &&entryNOTE_LINE;
                pActions->pNoteLineStartPush = 
                                           &&entryNOTE_LINE_START_PUSH;
                pActions->pStartOffsetMatch  = &&entrySTART_OFFSET_MATCH;
                pActions->pByteSearch        = &&entryBYTE_SEARCH;
                pActions->pAbandon           = &&entryABANDON;
                pActions->pCheckBuffer       = &&entryCHECK_BUFFER;
                pActions->pPrevNonword       = &&entryPREV_NONWORD;
                pActions->pPrevWord          = &&entryPREV_WORD;
                pActions->pCompleted         = &&entryCOMPLETED;
                pActions->pEndOfLineSearch   = &&entryEND_OF_LINE_SEARCH;
                pActions->pEndOfLineMatch    = &&entryEND_OF_LINE_MATCH;
                pActions->pStartOfLineSearch = &&entrySTART_OF_LINE_SEARCH;
                pActions->pFoundLineStart    = &&entryFOUND_LINE_START;
                pActions->pIncLineCount      = &&entryINC_LINE_COUNT;

                return TRUE;
        }

        /*Pick up FixedString search offset in case optimisation used*/
        StrOptOffset = pSpec->SearchSkipOffset;

        /*Push failure in case search attempts to backtrack past start*/
        *--pStack = pTab;
        *--pStack = pTextStart;
        pTab += MATCHENG_TABLE_SIZE;

        /*LineNr counts lines relative to the start of the buffer*/
        LineNr = 0;

        /*Does the RE begin with meta-states?*/
        ByteStartJump = 1;
        if (pSpec->FirstRealStateNr != 2) {
                /*Yes, byte search mustn't skip them*/
                ByteStartJump = 0;
        }

        /*Are we assuming buffer matches on entry (for anchored searches)?*/
        if (pSpec->StartTableNr != 0) {
                /*Yes, push backtrack to scan state, then select start state*/
                *--pStack = pTab;
                *--pStack = pTextStart;
                pTab += MATCHENG_TABLE_SIZE * pSpec->StartTableNr;

        }

        /*No line start found (match engine finds this in some cases)*/
        pDetails->pLineStart = NULL;

        TRACERY_FACE(pSpec, MATCHGCC_T_START_BUFFER, {
                UINT i;
                printf("Start, pText: %p ", pText);
                for (i = 0; i < 4; i++) {
                        printf("%02x ", pText[i]);
                }
        });

        /*Kick off the machine by acting on the first character of the buffer*/
        NEXT_ACTION;

entrySKIP_MAX: 
        pText += StrOptOffset;
        TRACERY_FACE(pSpec, MATCHGCC_T_ACTION, printf(" _SkipMax");); 
        NEXT_ACTION;

entrySKIP17: 
        TRACERY_FACE(pSpec, MATCHGCC_T_ACTION, printf(" _Skip17"););
        DO_SKIP(17);

entrySKIP16: 
        TRACERY_FACE(pSpec, MATCHGCC_T_ACTION, printf(" _Skip16");); 
        DO_SKIP(16);

entrySKIP15: 
        TRACERY_FACE(pSpec, MATCHGCC_T_ACTION, printf(" _Skip15");); 
        DO_SKIP(15);

entrySKIP14: 
        TRACERY_FACE(pSpec, MATCHGCC_T_ACTION, printf(" _Skip14");); 
        DO_SKIP(14);

entrySKIP13: 
        TRACERY_FACE(pSpec, MATCHGCC_T_ACTION, printf(" _Skip13");); 
        DO_SKIP(13);

entrySKIP12: 
        TRACERY_FACE(pSpec, MATCHGCC_T_ACTION, printf(" _Skip12");); 
        DO_SKIP(12);

entrySKIP11: 
        TRACERY_FACE(pSpec, MATCHGCC_T_ACTION, printf(" _Skip11");); 
        DO_SKIP(11);

entrySKIP10: 
        TRACERY_FACE(pSpec, MATCHGCC_T_ACTION, printf(" _Skip10");); 
        DO_SKIP(10);

entrySKIP9:  
        TRACERY_FACE(pSpec, MATCHGCC_T_ACTION, printf(" _Skip9");); 
        DO_SKIP(9);

entrySKIP8:  
        TRACERY_FACE(pSpec, MATCHGCC_T_ACTION, printf(" _Skip8");); 
        DO_SKIP(8);

entrySKIP7:  
        TRACERY_FACE(pSpec, MATCHGCC_T_ACTION, printf(" _Skip7");); 
        DO_SKIP(7);

entrySKIP6:  
        TRACERY_FACE(pSpec, MATCHGCC_T_ACTION, printf(" _Skip6");); 
        DO_SKIP(6);

entrySKIP5:  
        TRACERY_FACE(pSpec, MATCHGCC_T_ACTION, printf(" _Skip5");); 
        DO_SKIP(5);

entrySKIP4:  
        TRACERY_FACE(pSpec, MATCHGCC_T_ACTION, printf(" _Skip4");); 
        DO_SKIP(4);

entrySKIP3:  
        TRACERY_FACE(pSpec, MATCHGCC_T_ACTION, printf(" _Skip3");); 
        DO_SKIP(3);

entrySKIP2:  
        TRACERY_FACE(pSpec, MATCHGCC_T_ACTION, printf(" _Skip2");); 
        DO_SKIP(2);

entryAGAIN:
        TRACERY_FACE(pSpec, MATCHGCC_T_ACTION, printf(" _Again"););
        NEXT_ACTION;

entryADVANCE:
        TRACERY_FACE(pSpec, MATCHGCC_T_ACTION, {
                printf(" _Advance[%u]", 
                       (pTab - pSpec->pTables) / MATCHENG_TABLE_SIZE);
                fflush(stdout);
        });
        pTab += MATCHENG_TABLE_SIZE;
        NEXT_ACTION;

entrySTART_MATCH_PUSH:
        /*Save match start and push backtrack of next char*/
        TRACERY_FACE(pSpec, MATCHGCC_T_ACTION, {
                     printf("\n_StartPush(%p)", pText);
        });
        *--pStack = pTab;
        *--pStack = pText;
        pTab += MATCHENG_TABLE_SIZE;
        pStart = pText;
        pText--;
        NEXT_ACTION;

entrySTART_MATCH_PUSH_ADVANCE:
        /*START_MATCH_PUSH and include AGAIN to 2nd state*/
        TRACERY_FACE(pSpec, MATCHGCC_T_ACTION, {
                printf("\n_StartPushAdvance(%p)", pText);
        });
        *--pStack = pTab;
        *--pStack = pText;
        pTab += MATCHENG_TABLE_SIZE * 2;
        pStart = pText;
        NEXT_ACTION;

entryAGAIN_PUSH_ADVANCE:
        /*Save "backtrack" of next state, and repeat current state*/
        TRACERY_FACE(pSpec, MATCHGCC_T_ACTION, printf(" _AgainPushAdvance"););
        *--pStack = pTab + MATCHENG_TABLE_SIZE;
        *--pStack = pText - 1;
        NEXT_ACTION;

entryBACK_AND_ADVANCE:
        /*Move text back 1 char and advance to next table*/
        TRACERY_FACE(pSpec, MATCHGCC_T_ACTION, printf(" _BackAndAdvance"););
        pTab += MATCHENG_TABLE_SIZE;
        pText--;
        NEXT_ACTION;

entryADVANCE_PUSH_ZERO:
        /*Next table but allow match to be skipped via backtrack*/
        TRACERY_FACE(pSpec, MATCHGCC_T_ACTION, printf(" _AdvancePushZero"););
        pTab += MATCHENG_TABLE_SIZE;
        *--pStack = pTab;
        *--pStack = pText - 1;
        NEXT_ACTION;

entryNO_MATCH:
        /*No match: pop to previous state*/
        TRACERY_FACE(pSpec, MATCHGCC_T_ACTION, printf(" _NoMatch"););
        pText = *pStack++;
        pTab = *pStack++;
        NEXT_ACTION;

entryNOTE_LINE:
        /*Remember line start for buffer search*/
        TRACERY_FACE(pSpec, MATCHGCC_T_ACTION, printf(" _NoteLine"););
        pDetails->pLineStart = pText;
        LineNr++;
        NEXT_ACTION;

entryNOTE_LINE_START_PUSH:
        /*Remember line start and attempt match*/
        TRACERY_FACE(pSpec, MATCHGCC_T_ACTION, 
                     printf(" _NoteLineStartPush"););

        /*We defer counting the line until AFTER the match attempt*/
        *--pStack = pSpec->pIncLineTable;
        *--pStack = pText;
        pTab += MATCHENG_TABLE_SIZE;
        pStart = pText;
        pText--;
        NEXT_ACTION;

entryINC_LINE_COUNT:
        /*Increment line count, then reenter start state*/
        TRACERY_FACE(pSpec, MATCHGCC_T_ACTION, {printf(" IncLineCount");});

        LineNr++;
        pText--;
        pDetails->pLineStart = pText;
        pTab = &pSpec->pTables[MATCHENG_TABLE_SIZE];
        NEXT_ACTION;

entrySTART_OFFSET_MATCH:
        /*Save optimisation search point and attempt RE match*/
        TRACERY_FACE(pSpec, MATCHGCC_T_ACTION, printf(" _StartOffsetMatch"););
        *--pStack = pTab;
        *--pStack = pText;
        pText -= StrOptOffset + 1;
        pStart = pText + 1;
        pTab += MATCHENG_TABLE_SIZE;
        NEXT_ACTION;

entryBYTE_SEARCH:
        /*Use library's byte search (uses CPU instruction if available)*/
        TRACERY_FACE(pSpec, MATCHGCC_T_ACTION, printf(" _ByteSearch"););
        pText = ((BYTE *) memchr(pText, pSpec->TrailingLiteral, ~0));

        if (pText >= pSpec->pAfterEndOfBuffer) {
                /*Byte not found in remainder of buffer*/
                return FALSE;
        }

        /*First byte found, try to match the rest of the RE*/
        pStart = pText + 1;
        pText += ByteStartJump;
        *--pStack = pTab;
        *--pStack = pText;
        pTab += MATCHENG_TABLE_SIZE * pSpec->ByteSearchAdvance;

        NEXT_ACTION;

entryEND_OF_LINE_SEARCH:
        /*Use library's byte search (uses CPU instruction if available)*/
        TRACERY_FACE(pSpec, MATCHGCC_T_ACTION, printf(" _EOLSearch"););
        pText = ((BYTE *) memchr(pText, LF, ~0));

        if (pText > pSpec->pAfterEndOfBuffer) {
                /*Byte not found in remainder of buffer*/
                pDetails->BufLineNr = LineNr - 1;

                return FALSE;
        }

        /*Have we hit the LF appended to the buffer?*/
        if (pText == pSpec->pAfterEndOfBuffer) {
                /*Yes, did the last line end with LF anyway?*/
                if (pText[-1] == LF) {
                        /*Yes, don't match here*/
                        pDetails->BufLineNr = LineNr - 1;
                        return FALSE;
                }

                /*Push backtrack to abandon search*/
                *--pStack = pSpec->pTables;
                *--pStack = pText + 1;

                /*?? Could just let the stack underflow anyway?*/

        } else {
                /*Push backtrack in case search fails*/
                *--pStack = pSpec->pIncLineTable;
                *--pStack = pText + 1;
        }

        /*Step back past previous CR if permitting CR/LF lines*/
        /*?? */

        /*End of line found, set up match details*/
        pStart = pText + 1;

        /*Look at next table in case meta-tests included (e.g. -w)*/
        pTab += MATCHENG_TABLE_SIZE;

        NEXT_ACTION;


entrySTART_OF_LINE_SEARCH:
        /*Use library's byte search (uses CPU instruction if available)*/
        TRACERY_FACE(pSpec, MATCHGCC_T_ACTION, printf(" _StartOfLineSearch"););
        pText = ((BYTE *) memchr(pText, LF, ~0)) + 1;

        if (pText >= pSpec->pAfterEndOfBuffer) {
                /*Byte not found in remainder of buffer*/
                pDetails->BufLineNr = LineNr;
                return FALSE;
        }

        /*LF preceding (i.e. starting) line found, try to match rest of line*/
        LineNr++;
        pStart = pText;
        *--pStack = pTab;
        *--pStack = pText;
        pTab += MATCHENG_TABLE_SIZE;

        NEXT_ACTION;

entryFOUND_LINE_START:
        /*Save match start and push backtrack of next char*/
        TRACERY_FACE(pSpec, MATCHGCC_T_ACTION, {
                     printf("\n_FoundLineStart(%p)", pText);
        });
        *--pStack = pTab;
        *--pStack = pText;
        pTab += MATCHENG_TABLE_SIZE;
        pStart = pText;
        LineNr++;

        /*Note: We match LF for line start but it isn't part of matched text*/

        NEXT_ACTION;

entryABANDON:
        /*Buffer exhausted without finding match*/
        TRACERY_FACE(pSpec, MATCHGCC_T_ACTION, printf(" _Abandon"););

        /*Write count of lines seen so these are tracked correctly*/
        pDetails->BufLineNr = LineNr - 1;

        return FALSE;

entryCHECK_BUFFER:
        /*Marker character found: have we reached the end of the buffer?*/
        TRACERY_FACE(pSpec, MATCHGCC_T_ACTION, printf(" _CheckBuffer"););
        TableNr = (pTab - pSpec->pTables) / MATCHENG_TABLE_SIZE;
        if (pText > pSpec->pAfterEndOfBuffer) {
                /*Yes, execute nonmatch action*/
                goto *pSpec->pEndmarkerActions[TableNr];
        } else {
                /*No, execute action appropriate for this state*/
                goto *pSpec->pNotEndmarkerActions[TableNr];
        }
        /* NOTREACHED */

entryPREV_NONWORD:
        /*Check that preceding char isn't a word char*/
        TRACERY_FACE(pSpec, MATCHGCC_T_ACTION, printf(" PrevNonword"););

        /*Is the preceding character part of a word?*/
        if (IS_WORD(pText[-2])) {
                /*Yes, this isn't acceptable: backtrack*/
                pText = *pStack++;
                pTab = *pStack++;

                NEXT_ACTION;

        }

        /*Check is okay: back up to prev char and advance to next state*/
        pText--;
        pTab += MATCHENG_TABLE_SIZE;
        NEXT_ACTION;

entryPREV_WORD:
        /*Check that preceding char is a word char*/
        TRACERY_FACE(pSpec, MATCHGCC_T_ACTION, printf(" PrevWord"););

        /*Is the preceding character not part of a word?*/
        if (! IS_WORD(pText[-2])) {
                /*Yes, this isn't acceptable: backtrack*/
                pText = *pStack++;
                pTab = *pStack++;

                NEXT_ACTION;

        }

        /*Check is okay: back up to prev char and advance to next state*/
        pText--;
        pTab += MATCHENG_TABLE_SIZE;
        NEXT_ACTION;

entryEND_OF_LINE_MATCH:
        /*We matched LF (or possibly CR) terminating line: point to LF again*/
        pText--;

        if (pText > pSpec->pAfterEndOfBuffer) {
                /*Byte not found in remainder of buffer*/
                pDetails->BufLineNr = LineNr - 1;

                return FALSE;
        }

        /*Have we hit the LF appended to the buffer?*/
        if (pText == pSpec->pAfterEndOfBuffer) {
                /*Yes, did the last line end with LF anyway?*/
                if (pText[-1] == LF) {
                        /*Yes, don't match here*/
                        pDetails->BufLineNr = LineNr - 1;
                        return FALSE;
                }

                /*Push backtrack to abandon search*/
                *--pStack = pSpec->pTables;
                *--pStack = pText + 1;

                /*?? Could just let the stack underflow anyway?*/

        } else {
                /*Push backtrack in case search fails*/
                *--pStack = pSpec->pIncLineTable;
                *--pStack = pText + 1;
        }

        /*Step back past previous CR if permitting CR/LF lines*/
        /*?? */

        /*End of line found, set up match details*/
        pStart = pText + 1;

        /*Look at next table in case meta-tests included (e.g. -w)*/
        pTab += MATCHENG_TABLE_SIZE;

        NEXT_ACTION;

entryCOMPLETED:
        TRACERY_FACE(pSpec, MATCHGCC_T_ACTION, printf(" Yay! completed\n"););

        /*Compensate for characteristics of the match engine*/
        pStart = (pStart - 1) + pSpec->StartAdjustment;

        /*Write details of match to caller*/
        pDetails->pMatchStart = pStart;
        pDetails->pMatchEnd   = pText - 1;
        pDetails->BufLineNr   = LineNr;

        return TRUE;

} /*Match*/


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
MatchGCC_TraceryLink(void *pObject, UINT Opcode, ...)
{
        Tracery_ObjectInfo **ppInfoBlock;
        LWORD *pDefaultFlags;
        Tracery_EditEntry **ppEditList;
        va_list ap;

        va_start(ap, Opcode);

        switch (Opcode) {
        case TRACERY_REGCMD_GET_INFO_BLOCK:
                /*Was an object supplied?*/
                ppInfoBlock  = va_arg(ap, Tracery_ObjectInfo **);
                if (pObject == NULL) {
                        /*No, report module's trace flag register*/
                        *ppInfoBlock = &gMatchGCC.TraceInfo;

                } else {
                        /*Yes, must be a MatchEng RE: report its trace flag*/
                        *ppInfoBlock = &((MatchEng_Spec *) pObject)->TraceInfo;
                }
                break;

        case TRACERY_REGCMD_GET_DEFAULT_FLAGS:
                pDefaultFlags  = va_arg(ap, LWORD *);
                *pDefaultFlags = 
                        MATCHGCC_T_ACTION | 
                        MATCHGCC_T_START_BUFFER;
                break;

        case TRACERY_REGCMD_GET_EDIT_LIST:
                ppEditList  = va_arg(ap, Tracery_EditEntry **);
                *ppEditList = gMatchGCC_TraceryEditDefs;
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
