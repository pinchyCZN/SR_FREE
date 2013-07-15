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

#include "ascii.h"
#include <compdef.h>
#include <ctype.h>
#include "matcheng.h"
#include "regexp00.h"
#include "retable.h"
#include <stdlib.h>
#include <stdio.h>

/*The curse of a simpleminded NFA engine: backtracking stacks may be huge*/
#define BACKTRACKING_STACK_SIZE_MAX     65536uL

/*We need to allocate MORE stack if FastFile presents an even bigger buffer*/

#define IS_WORD(ch)  (isalnum(ch) || ((ch) == '_'))

#define ROUND_PARAGRAPH(x)              (((x) + 15) & ~0x0fuL)

/*Module-wide variables*/

typedef struct {
        /*Additional state table(s) to handle special cases*/

        /*Increment line count after attempting matches triggered by LF*/
        BOOL IncLineTableInitialised;
        MatchEng_Action IncLineTable[MATCHENG_TABLE_SIZE];

} RETABLE_MODULE_CONTEXT;

module_scope RETABLE_MODULE_CONTEXT gRETable;

/************************************************************************/
/*                                                                      */
/*      Init -- Prepare module for operation                            */
/*                                                                      */
/************************************************************************/
public_scope void
RETable_Init(void)
{
        /*No line-count adjustment table set up by default*/
        gRETable.IncLineTableInitialised = FALSE;

} /*Init*/


/************************************************************************/
/*                                                                      */
/*      Start -- Begin managing what has to be managed                  */
/*                                                                      */
/************************************************************************/
public_scope BOOL
RETable_Start(void)
{
        /*No preparation needed*/
        return TRUE;

} /*Start*/


/************************************************************************/
/*                                                                      */
/*      SetState -- Write actions and flags for state                   */
/*                                                                      */
/*      Used to initialise state tables and flags with constants.       */
/*                                                                      */
/************************************************************************/
module_scope void
RETable_SetState(MatchEng_Spec *pSpec, UINT StateNr, 
                 MatchEng_Action Action, MatchEng_TableFlags Flags)
{
        MatchEng_Action *pAct;
        MatchEng_Action *pActEnd;

        /*Initialise table entries*/
        pAct = &pSpec->pTables[StateNr * MATCHENG_TABLE_SIZE];
        pActEnd = pAct + MATCHENG_TABLE_SIZE;
        while (pAct != pActEnd) {
                *pAct++ = Action;
        }

        /*Also write table flags*/
        pSpec->pTableFlags[StateNr] = Flags;

} /*SetState*/



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
RETable_AllocBacktrack(MatchEng_Spec *pSpec, UINT32 TextSize)
{
        UINT32 EntryPairsNeeded;

        /*Did we have a previous buffer?*/
        if (pSpec->pBacktrackMem != NULL) {
                /*Yes, free it now*/
                free(pSpec->pBacktrackMem);
        }

        /*Work out space needed and malloc it*/
        EntryPairsNeeded = TextSize + 4;
        pSpec->BacktrackSize = TextSize;
        pSpec->pBacktrackMem = malloc(EntryPairsNeeded * 2 * 
                                      sizeof(MatchEng_Action));
        if (pSpec->pBacktrackMem == NULL) {
                return FALSE;
        }

        /*Allocated memory successfully*/
        return TRUE;

} /*AllocBacktrack*/


/************************************************************************/
/*                                                                      */
/*      Create -- Acquire resources and initialise memory               */
/*                                                                      */
/*      Allocates memory, acquires any other needed resources,          */
/*      and initialises the table-driven RE search spec to a            */
/*      "blank" state.  Initialisation includes preparing the           */
/*      initial failure (backtracking underflow) state, and             */
/*      writing defaults for the remaining states.                      */
/*                                                                      */
/*      Returns FALSE if unable to obtain the required resources.       */
/*                                                                      */
/************************************************************************/
module_scope BOOL
RETable_Create(RegExp_Specification *pRE, MatchEng_Spec **ppSpec)
{
        UINT NrStates;
        BYTE *pMemory;
        MatchEng_Spec *pSpec;
        MatchEng_Action *pAct;
        MatchEng_Action *pActEnd;
        UINT SpecSize;
        UINT i;

        /*Adjust state count to include fail, start and success states*/
        NrStates = pRE->NrStates + 2;

        /*Acquire space for spec, tables, flags, endmarker actions etc*/
        /*All sizes are rounded up to the next paragraph (16-byte) boundary*/
        SpecSize =  ROUND_PARAGRAPH(sizeof(MatchEng_Spec));
        SpecSize += ROUND_PARAGRAPH(NrStates * (MATCHENG_TABLE_SIZE * 
                                                sizeof(MatchEng_Action)));
        SpecSize += ROUND_PARAGRAPH(NrStates * sizeof(MatchEng_TableFlags));
        SpecSize += ROUND_PARAGRAPH(NrStates * sizeof(MatchEng_Action));
        SpecSize += ROUND_PARAGRAPH(NrStates * sizeof(MatchEng_Action));

        pMemory = (BYTE *) malloc(SpecSize);
        if (pMemory == NULL) {
                /*Sorry, unable to acquire memory*/
                return FALSE;
        }

        /*Set up specification and pointers into working memory*/
        pSpec = (MatchEng_Spec *) pMemory;

        pMemory += ROUND_PARAGRAPH(sizeof(*pSpec));

        /*Create pointers into various sections of RAM*/
        pSpec->pEndmarkerActions  = (MatchEng_Action *) pMemory;
        pMemory += ROUND_PARAGRAPH(NrStates * sizeof(MatchEng_Action *));
        pSpec->pNotEndmarkerActions = (MatchEng_Action *) pMemory;
        pMemory += ROUND_PARAGRAPH(NrStates * sizeof(MatchEng_Action *));
        pSpec->pTables            = (MatchEng_Action *) pMemory;
        pMemory += ROUND_PARAGRAPH(NrStates * sizeof(MatchEng_Action *) *
                                   MATCHENG_TABLE_SIZE);
        pSpec->pTableFlags        = (MatchEng_TableFlags *) pMemory;

        /*Horrible hack: preallocate backtracking stack memory now*/
        if (! RETable_AllocBacktrack(pSpec, BACKTRACKING_STACK_SIZE_MAX)) {
                /*Sorry, unable to acquire backtrack stack memory*/
                free(pSpec);
                return FALSE;
        }

        /*Backtracking stack works from end of spec memory backwards*/
        pSpec->pStack = (MatchEng_Action *) pSpec->pBacktrackMem;
        pSpec->pStack += pSpec->BacktrackSize - 1;

        /*Default to starting on first table of RE (start table)*/
        pSpec->StartTableNr = 0;

        /*First table contains failure state to simplify backtracking*/
        RETable_SetState(pSpec, 0, ABANDON, MATCHENG_TABLE_TYPE_FAIL);

        /*?? Backtracking underflow should not happen with buffer search?*/

        /*Second table is start (buffer scan) state*/
        pSpec->pTableFlags[1] = MATCHENG_TABLE_TYPE_START;

        /*(Note: Flag for start table changes if search is anchored to start)*/

        /*Initialise remaining table entries to default to NO_MATCH*/
        pAct = pSpec->pTables + MATCHENG_TABLE_SIZE * 2;
        pActEnd = pAct + (pRE->NrStates - 1) * MATCHENG_TABLE_SIZE;
        while (pAct != pActEnd) {
                *pAct++ = NO_MATCH;
        }

        /*Final table (Success) ignored for now*/

        /*Initialise byte search*/
        pSpec->ByteSearchAdvance = 0;

        /*Adjustment to match used if BYTE_SEARCH looks for start anchor*/
        pSpec->StartAdjustment = 0;

        /*Initialise Endmarker/NotEndmarker actions (most remain unused)*/
        pSpec->pEndmarkerActions[0]    = ABANDON;
        pSpec->pNotEndmarkerActions[0] = ABANDON;
        for (i = 1; i < NrStates; i++) {
                pSpec->pEndmarkerActions[i]    = NO_MATCH;
                pSpec->pNotEndmarkerActions[i] = NO_MATCH;
        }

        /*Created specification successfully*/
        *ppSpec = pSpec;
        return TRUE;

} /*Create*/

/************************************************************************/
/*                                                                      */
/*      WordMetaState -- Set up entries to test word characteristics    */
/*                                                                      */
/*      Matching words can be achieved very efficiently using           */
/*      the threaded-table facility, but some analysis shows that       */
/*      there are a lot of variations on word matching:                 */
/*                                                                      */
/*      1. "\<" matches a word beginning: The previous char must        */
/*         be a nonword char and the current char must be a word        */
/*         char.  (Where "word" is defined as [A-Za-z0-9_].)            */
/*      2. "\>" matches a word end: Prev must be word and               */
/*         current must be nonword.                                     */
/*      3. "\b" matches a word boundary: Prev word and current          */
/*         nonword, or vice versa.                                      */
/*      4. "\B" matches a word NONboundary: both prev and current       */
/*         must be a word char or both must be nonword.                 */
/*      5. Specifying "-w" is not the same as adding "\<" to the        */
/*         start of the RE and "\>" to the end: Using -w means          */
/*         check that the characters adjacent to the match text         */
/*         are non-word chars without constraining the match            */
/*         text istelf.  We add a pure PREV_NONWORD state at            */
/*         start and modify the Success state at the end to only        */
/*         match nonword characters -- see Expand (below) for           */
/*         these cases.                                                 */
/*                                                                      */
/************************************************************************/
module_scope void
RETable_WordMetaState(MatchEng_Spec *pSpec, UINT StateNr, 
                      MatchEng_TableFlags Flags, 
                      MatchEng_Action NonwordAction, 
                      MatchEng_Action WordAction)
{
        UINT ByteIx;
        MatchEng_Action *pAction = &pSpec->pTables[StateNr * 
                                                  MATCHENG_TABLE_SIZE];

        /*Write flag for this state*/
        pSpec->pTableFlags[StateNr] = Flags;

        /*Set up word and nonword actions for the state*/
        for (ByteIx = 0; ByteIx < MATCHENG_TABLE_SIZE; ByteIx++) {
                /*Is this byte a word character?  (nb: ASSUMES ASCII)*/
                if (((ByteIx >= '0') && (ByteIx <= '9')) || 
                    ((ByteIx >= 'A') && (ByteIx <= 'Z')) || 
                    ((ByteIx >= 'a') && (ByteIx <= 'z')) || 
                    (ByteIx == '_')) {
                        /*Yes, current char is a word char*/
                        pAction[ByteIx] = WordAction;

                } else {
                        /*No, current char is non-word*/
                        pAction[ByteIx] = NonwordAction;
                }

        }

} /*WordMetaState*/


/************************************************************************/
/*                                                                      */
/*      AddMatchChars -- Write action for chars that match RE code      */
/*                                                                      */
/*      This function interprets what characters match the specified    */
/*      regular expression code, and changes the table entries for      */
/*      those characters to ADVANCE.  It returns a type code            */
/*      summarising table behaviour so that later users of the          */
/*      table don't need to reverse-engineer the table type.            */
/*                                                                      */
/************************************************************************/
module_scope void
RETable_AddMatchChars(MatchEng_Spec *pSpec, UINT StateNr, RE_ELEMENT **ppRE)
{
        UINT Lit;
        UINT ByteIx;
        BOOL CaseSensitive = FALSE;
        RE_ELEMENT *pRE = *ppRE;
        MatchEng_Action *pAction;

        pAction = &pSpec->pTables[StateNr * MATCHENG_TABLE_SIZE];

        /*Is this element marked as case-sensitive?*/
        if ((*pRE & REGEXP_CASE_MASK) == REGEXP_CASE_SENSITIVE) {
                /*Yes, remember to set up tables correctly*/
                CaseSensitive = TRUE;
        }

        switch (REGEXP_TYPE_R(*pRE++)) {
        case REGEXP_TYPE_MATCH_ANY:
                /*All entries except LF match*/
                RETable_SetState(pSpec, StateNr, ADVANCE, 
                                 MATCHENG_TABLE_TYPE_MATCH_ANY);
                pAction[LF] = NO_MATCH;
                break;

        case REGEXP_TYPE_CLASS:
                /*Match class: Decode bitmask specifier into table entries*/
                for (ByteIx = 0; ByteIx < MATCHENG_TABLE_SIZE; ByteIx++) {
                        if (REGEXP_BIT_TEST(pRE, ByteIx)) {
                                if (CaseSensitive) {
                                        pAction[ByteIx] = ADVANCE;
                                } else {
                                        pAction[tolower(ByteIx)] = ADVANCE;
                                        pAction[toupper(ByteIx)] = ADVANCE;
                                }
                        }
                }

                /*Set flag and move to next RE code*/
                pSpec->pTableFlags[StateNr] = MATCHENG_TABLE_TYPE_CLASS;
                pRE += REGEXP_BITMASK_NRCODES;
                break;

        case REGEXP_TYPE_CLASS_ALL_BUT:
                /*Decode bitmask specifier into table entries*/
                for (ByteIx = 0; ByteIx < MATCHENG_TABLE_SIZE; ByteIx++) {
                        if (CaseSensitive) {
                                if (! REGEXP_BIT_TEST(pRE, ByteIx)) {
                                        pAction[ByteIx] = ADVANCE;
                                }
                        } else {
                                if (! (REGEXP_BIT_TEST(pRE, 
                                                       tolower(ByteIx)) ||
                                       REGEXP_BIT_TEST(pRE, 
                                                       toupper(ByteIx)))) {
                                        pAction[ByteIx] = ADVANCE;
                                }
                        }
                }
                pSpec->pTableFlags[StateNr] = MATCHENG_TABLE_TYPE_ALL_BUT;
                pRE += REGEXP_BITMASK_NRCODES;
                break;

        case REGEXP_TYPE_LITERAL:
                /*Is the character handled directly by the tables?*/
                Lit = REGEXP_LITERAL_R(pRE[-1]);
                if (Lit >= MATCHENG_TABLE_SIZE) {
                        /*No, cannot expand*/
                        printf("?? RETable_AddMatchChars: " 
                                            "Literals >%u not handled: %u\n", 
                               MATCHENG_TABLE_SIZE, Lit);
                        break;
                }

                /*Is this search case-sensitive?*/
                if (CaseSensitive) {
                        /*Yes, merely let literal match*/
                        pAction[Lit] = ADVANCE;

                } else {
                        /*No, ensure both lower and upper case match*/
                        pAction[tolower(Lit)] = ADVANCE;
                        pAction[toupper(Lit)] = ADVANCE;
                }

                pSpec->pTableFlags[StateNr] = MATCHENG_TABLE_TYPE_LITERAL;
                break;

        case REGEXP_TYPE_DEAD_END:
                /*No characters match: fill table with ABANDON*/
                RETable_SetState(pSpec, StateNr, ABANDON, 
                                 MATCHENG_TABLE_TYPE_DEAD_END);
                break;

        case REGEXP_TYPE_WORD_BEGINNING:
                /*Matching word chars test nonword char in prev position*/
                RETable_WordMetaState(pSpec, StateNr, 
                                      MATCHENG_TABLE_TYPE_WORD_BEGINNING | 
                                              MATCHENG_TABLE_WIDTH_META, 
                                      NO_MATCH, 
                                      PREV_NONWORD);
                break;

        case REGEXP_TYPE_WORD_END:
                /*Matching nonword chars test word char in prev position*/
                RETable_WordMetaState(pSpec, StateNr, 
                                      MATCHENG_TABLE_TYPE_WORD_END | 
                                              MATCHENG_TABLE_WIDTH_META, 
                                      PREV_WORD, 
                                      NO_MATCH);
                break;

        case REGEXP_TYPE_WORD_BOUNDARY:
                /*Test word/prev nonword and nonword/prev word combinations*/
                RETable_WordMetaState(pSpec, StateNr, 
                                      MATCHENG_TABLE_TYPE_WORD_BOUNDARY | 
                                              MATCHENG_TABLE_WIDTH_META, 
                                      PREV_WORD, 
                                      PREV_NONWORD);
                break;

        case REGEXP_TYPE_WORD_NONBOUNDARY:
                /*Test word/prev word and nonword/prev nonword combinations*/
                RETable_WordMetaState(pSpec, StateNr, 
                                      MATCHENG_TABLE_TYPE_WORD_NONBOUNDARY | 
                                              MATCHENG_TABLE_WIDTH_META, 
                                      PREV_NONWORD, 
                                      PREV_WORD);
                break;

        default:
                /*Unknown RE element*/
                printf("?? RETable_AddMatchChars: Code %08lx not handled", 
                       pRE[-1]);
                break;
        }

        /*Write pointer to next code back to caller*/
        *ppRE = pRE;

} /*AddMatchChars*/


/************************************************************************/
/*                                                                      */
/*      ApplyIteration -- Apply RE iteration modifier to tables         */
/*                                                                      */
/*      Examines the iteration modifier(s) and modifies the             */
/*      current table to apply the iteration.  1-or-more iteration      */
/*      is implemented by cloning the table and then applying           */
/*      0-or-more iteration to the cloned table.                        */
/*                                                                      */
/*      The return value of the function reports how many state         */
/*      tables were added to the expanded form.                         */
/*                                                                      */
/************************************************************************/
module_scope UINT
RETable_ApplyIteration(MatchEng_Spec *pSpec, UINT StateNr, RE_ELEMENT *pRE)
{
        UINT i;
        UINT StatesAdded = 0;
        WORD RepeatSpec;
        MatchEng_Action *pAction;

        pAction = &pSpec->pTables[StateNr * MATCHENG_TABLE_SIZE];

        /*Handle repeat modifier*/
        RepeatSpec = REGEXP_REPEAT_R(*pRE);
        switch (RepeatSpec) {
        case REGEXP_REPEAT_1_OR_MORE:
                /*Duplicate details so copy may handle iteration*/
                for (i = 0; i < MATCHENG_TABLE_SIZE; i++) {
                        pAction[i + MATCHENG_TABLE_SIZE] = pAction[i];
                }
                pAction += MATCHENG_TABLE_SIZE;
                pSpec->pTableFlags[StateNr + 1] = 
                        pSpec->pTableFlags[StateNr];
                StateNr++;
                StatesAdded = 1;
                /*FALLTHROUGH*/

        case REGEXP_REPEAT_0_OR_MORE:
                /*Convert table to use 0-or-more iteration*/
                for (i = 0; i < MATCHENG_TABLE_SIZE; i++) {
                        /*Change this table action*/
                        if (*pAction == ADVANCE) {
                                *pAction++ = AGAIN_PUSH_ADVANCE;
                                
                        } else {
                                *pAction++ = BACK_AND_ADVANCE;
                                
                        }

                }

                pSpec->pTableFlags[StateNr] |= 
                        MATCHENG_TABLE_ITERATIVE | MATCHENG_TABLE_ZERO_TRIP;
                break;

        case REGEXP_REPEAT_0_OR_1:
                /*Change no-match cases to attempt zero-trip advance*/
                for (i = 0; i < MATCHENG_TABLE_SIZE; i++, pAction++) {
                        /*Change this table action*/
                        if (*pAction == NO_MATCH) {
                                *pAction = BACK_AND_ADVANCE;
                        
                        } else if (*pAction == ADVANCE) {
                                /*Matched, but also remember nonmatch case*/
                                *pAction = ADVANCE_PUSH_ZERO;

                        }

                }
                pSpec->pTableFlags[StateNr] |= MATCHENG_TABLE_ZERO_TRIP;
                break;

        case REGEXP_REPEAT_MATCH_SELF:
                /*No modifier*/
                break;

        default:
                printf("RETable_ApplyIteration: Repeat %u?\n", 
                       RepeatSpec);
                break;

        }

        return StatesAdded;

} /*ApplyIteration*/


/************************************************************************/
/*                                                                      */
/*      AnchoredToEnd -- Set up tables where RE is merely end anchor    */
/*                                                                      */
/*      For most searches anchored to the end, we merely change         */
/*      the success state so that only end-of-line characters           */
/*      are treated as success.  However, if the RE is merely           */
/*      the end anchor (e.g. /$/), then we use a tailored byte          */
/*      search to find the end of the line (including handling          */
/*      lines terminated by CR/LF if selected).                         */
/*                                                                      */
/************************************************************************/
module_scope void
RETable_AnchoredToEnd(MatchEng_Spec *pSpec)
{
        MatchEng_Action *pStart;
        UINT i;

        /*Modify start search actions to invoke EOL search*/
        RETable_SetState(pSpec, 1, END_OF_LINE_SEARCH, 
                         MATCHENG_TABLE_TYPE_START);
        pStart = &pSpec->pTables[MATCHENG_TABLE_SIZE];
        pStart[LF] = END_OF_LINE_MATCH;

        /*Have we initialised the deferred line inc table?*/
        if (! gRETable.IncLineTableInitialised) {
                /*No, initialise it now*/
                for (i = 0; i < MATCHENG_TABLE_SIZE; i++) {
                        gRETable.IncLineTable[i] = 
                                INC_LINE_COUNT;
                }
                gRETable.IncLineTableInitialised = TRUE;
        }

        /*Provide table reference for search to use*/
        pSpec->pIncLineTable = gRETable.IncLineTable;



} /*AnchoredToEnd*/


/************************************************************************/
/*                                                                      */
/*      ScanForLineStart -- Search for LF for anchored search           */
/*                                                                      */
/*      Sets up the search for searches anchored to the start           */
/*      of the line.  This entails:                                     */
/*           - knowing that buffer searches always start on a           */
/*                    line boundary,                                    */
/*           - starting on first match state since we know              */
/*                    anchor matches at buffer start,                   */
/*           - setting up the start state to become a "scan for         */
/*                    LF (line start)" state, and                       */
/*           - getting match engine to push a backtrack reference       */
/*                    pointing to start state when it starts            */
/*                    on the first match state.                         */
/*                                                                      */
/*      Since scanning for LF is very much like the BYTE_SEARCH         */
/*      operation, we use that action -- but with the following         */
/*      modifications:                                                  */
/*           - We advance 1 table instead of 2 when byte found.         */
/*           - The trailing literal for the byte search is LF.          */
/*           - We adjust any completed match to remove the LF           */
/*                  we found at the start of the RE.                    */
/*                                                                      */
/************************************************************************/
module_scope void
RETable_ScanForLineStart(MatchEng_Spec *pSpec)
{
        /*All chars except LF trigger search for LF*/
        RETable_SetState(pSpec, 1, START_OF_LINE_SEARCH, 
                         MATCHENG_TABLE_TYPE_START_LINE);
        pSpec->pTables[MATCHENG_TABLE_SIZE + LF] = CHECK_BUFFER;
        pSpec->pEndmarkerActions[1]    = ABANDON;
        pSpec->pNotEndmarkerActions[1] = FOUND_LINE_START;
        pSpec->TrailingLiteral = LF;
        pSpec->EndCondition = MATCHENG_CONDITION_TRAILING_LITERAL;
        pSpec->PatternLength = 1;
        pSpec->ByteSearchAdvance = 1;
        pSpec->StartTableNr = 1;
        pSpec->StartAdjustment = 1;

        /*SIGH: Non-zero offset used by match to indicate skip search*/
        pSpec->SearchSkipOffset = 1;

} /*ScanForLineStart*/


/************************************************************************/
/*                                                                      */
/*      AnalyseStart -- Look for easy(ish) components at start of RE    */
/*                                                                      */
/*      This function inspects the start of the table version of        */
/*      the regular expression, and counts how many elements can        */
/*      be scanned using skip-style search techniques such as           */
/*      the Boyer-Moore algorithm or the CPU string search              */
/*      instruction.  The analysis is used to help decide how to        */
/*      scan the buffer for potential RE match positions.               */
/*                                                                      */
/************************************************************************/
module_scope void
RETable_AnalyseStart(MatchEng_Spec *pSpec, UINT *pNrEasyStates, 
                     BOOL *pFirstIsLiteral, BYTE *pFirstLiteral)
{
        UINT States;
        MatchEng_TableFlags *pFlags;
        MatchEng_Action *pAction;
        UINT Entry;
        UINT MatchEntry = 0;
        UINT Matches;

        /*Start analysis with first match state (skip fail and start tables)*/
        pFlags  = &pSpec->pTableFlags[pSpec->FirstRealStateNr];
        States = 0;

        /*Use summary of search contained in state flags for analysis*/
        for (; ; pFlags++, States++) {
                /*Does this table contain nonlinear effects?*/
                if (*pFlags & (MATCHENG_TABLE_ITERATIVE | 
                               MATCHENG_TABLE_ZERO_TRIP)) {
                        /*Sorry, state table needs > 1 second's thought*/
                        break;

                }

                /*Is this table worth optimising?*/
                switch (MATCHENG_TABLE_TYPE_R(*pFlags)) {
                case MATCHENG_TABLE_TYPE_LITERAL:
                case MATCHENG_TABLE_TYPE_CLASS:
                        /*These are okay (but what if class matches lots?)*/
                        continue;

                }

                /*Any table which gets us here is NOT okay: finish summary*/
                break;

        }

        /*Count how many entries in the first RE state match*/
        Matches = 0;
        pAction = &pSpec->pTables[pSpec->FirstRealStateNr * 
                                  MATCHENG_TABLE_SIZE];
        for (Entry = 0; Entry < MATCHENG_TABLE_SIZE; Entry++) {
                if (pAction[Entry] != NO_MATCH) {
                        /*Count match and remember it in case there's only 1*/
                        Matches++;
                        MatchEntry = Entry;
                }
        }

        /*Did the first state only match one character?*/
        *pFirstIsLiteral = FALSE;
        if (Matches == 1) {
                /*Yes, tell client of opportunity for first table*/
                *pFirstIsLiteral = TRUE;
                *pFirstLiteral   = (BYTE) MatchEntry;
        }

        /*Report how many easy(ish) states we found*/
        *pNrEasyStates = States;

} /*AnalyseStart*/


/************************************************************************/
/*                                                                      */
/*      ScanByBoyerMoore -- Set up start state to use BM                */
/*                                                                      */
/*      This function sets up the starting state to use a Boyer-        */
/*      Moore style string search algorithm.  It is more powerful       */
/*      than most simpleminded BM searches as the string may            */
/*      include more sophisticated components such as classes           */
/*      and/or case insensitive literals.                               */
/*                                                                      */
/*      The starting table is set up by looking at the rightmost        */
/*      table of the string and working backwards towards the           */
/*      start of the RE.  If the last table matches, it triggers        */
/*      a match attempt; otherwise, we count how many preceding         */
/*      states don't match, and add a skip action for that number       */
/*      of states to the starting table.                                */
/*                                                                      */
/*      The match could be optimised further if we started              */
/*      rearranging the RE match tables, but this is too complex        */
/*      just now.  Optimisations could include not retesting the        */
/*      last character of the string during the match, and              */
/*      using a (self-tuning?) guard test.  Further optimisations       */
/*      could be added if we were confident about the input             */
/*      characteristics (e.g. shorten string if last component          */
/*      becomes substantially less likely to match).                    */
/*                                                                      */
/************************************************************************/
module_scope void
RETable_ScanByBoyerMoore(MatchEng_Spec *pSpec, UINT NrStates)
{
        MatchEng_Action *pLast;
        MatchEng_Action *pStart;
        MatchEng_Action *pSearch;
        MatchEng_Action *pFirstReal;
        UINT i;
        UINT SkipSize;
        UINT Endmarker;

        /*Set up to scan state tables and to build starting state*/
        pStart = &pSpec->pTables[MATCHENG_TABLE_SIZE];
        pFirstReal = &pSpec->pTables[pSpec->FirstRealStateNr * 
                                    MATCHENG_TABLE_SIZE];
        pLast = &pSpec->pTables[(NrStates + pSpec->FirstRealStateNr - 1) * 
                                MATCHENG_TABLE_SIZE];
        pSpec->SearchSkipOffset = NrStates - 1;

        /*Loop through each character of table, looking for improvements*/
        for (i = 0; i < MATCHENG_TABLE_SIZE; i++, pLast++, pStart++) {
                /*Does this character match in the last state of the string?*/
                if (*pLast != NO_MATCH) {
                        /*Yes, we pause scanning and attempt match*/
                        *pStart = START_OFFSET_MATCH;

                        /*Note char as terminator for search*/
                        pSpec->TrailingLiteral = (BYTE) i;

                        continue;
                }

                /*Find out how far non-match activity extends back*/
                pSearch = pLast - MATCHENG_TABLE_SIZE;
                *pStart = ADVANCE;
                for (;;) {
                        /*Does this character not match in this table?*/
                        if (*pSearch != NO_MATCH) {
                                /*No, finished looking*/
                                break;
                        }

                        /*Move to the next previous table*/
                        pSearch -= MATCHENG_TABLE_SIZE;

                }

                /*Work out how many characters can be skipped*/
                SkipSize = (UINT) ((pLast - pSearch) / 
                                            MATCHENG_TABLE_SIZE);

                /*Note: Can "see" more skip states if meta tables present*/

                /*Is this skip the maximum possible?*/
                if (SkipSize >= NrStates) {
                        /*Yes, use efficient action*/
                        *pStart = SKIP_MAX;
                        continue;
                }

                /*Write search action based on survey*/
                switch (SkipSize) {
                case  1: *pStart = AGAIN; break;
                case  2: *pStart = SKIP2;  break;
                case  3: *pStart = SKIP3;  break;
                case  4: *pStart = SKIP4;  break;
                case  5: *pStart = SKIP5;  break;
                case  6: *pStart = SKIP6;  break;
                case  7: *pStart = SKIP7;  break;
                case  8: *pStart = SKIP8;  break;
                case  9: *pStart = SKIP9;  break;
                case 10: *pStart = SKIP10; break;
                case 11: *pStart = SKIP11; break;
                case 12: *pStart = SKIP12; break;
                case 13: *pStart = SKIP13; break;
                case 14: *pStart = SKIP14; break;
                case 15: *pStart = SKIP15; break;
                case 16: *pStart = SKIP16; break;
                case 17:
                default:
                         *pStart = SKIP17; break;

                }

        }

        /*Use endmarker to terminate buffer search*/
        Endmarker = MATCHENG_SPEC_ENDMARKER_R(pSpec->SpecFlags);
        pSpec->EndCondition = MATCHENG_CONDITION_TRAILING_LITERAL;
        pSpec->PatternLength = NrStates;
        pSpec->TrailingLiteral = Endmarker;
        pStart = &pSpec->pTables[MATCHENG_TABLE_SIZE + Endmarker];
        pSpec->pEndmarkerActions[1] = ABANDON;
        pSpec->pNotEndmarkerActions[1] = *pStart;
        *pStart = CHECK_BUFFER;

} /*ScanByBoyerMoore*/


/************************************************************************/
/*                                                                      */
/*      ScanByByteSearch -- Set up start state to use CPU instruction   */
/*                                                                      */
/*      Although the threaded assembly technique used in the            */
/*      table-driven engine is extremely fast, it can't scan for        */
/*      bytes as quickly as the CPU byte search instruction.            */
/*      This function sets up the starting state to scan using          */
/*      the CPU instruction.                                            */
/*                                                                      */
/*      Parameter OptimiseFirst turns on an extra optimisation          */
/*      since matching a literal in the starting state often            */
/*      means we can take matching in the first state for               */
/*      granted.  The switch is needed where we need to perform         */
/*      meta-tests -- such as testing for word boundaries (-w           */
/*      switch or \< at the start of the RE).  In this case, we         */
/*      mustn't optimise away the match on the first state.             */
/*                                                                      */
/*                                                                      */
/************************************************************************/
module_scope void
RETable_ScanByByteSearch(MatchEng_Spec *pSpec, BYTE FirstLiteral, 
                         BOOL OptimiseFirst)
{
        MatchEng_Action *pStart;

        /*All entries except literal trigger CPU search instruction*/
        RETable_SetState(pSpec, 1, BYTE_SEARCH, MATCHENG_TABLE_TYPE_START);
        pStart = &pSpec->pTables[MATCHENG_TABLE_SIZE];
        pSpec->TrailingLiteral = FirstLiteral;
        pSpec->EndCondition = MATCHENG_CONDITION_TRAILING_LITERAL;
        pSpec->PatternLength = 1;

        /*Can we assume the start match fulfils the first state's match?*/
        if (OptimiseFirst) {
                /*Yes, get RE engine to advance to next state immediately*/
                pSpec->ByteSearchAdvance = 2;
                pStart[FirstLiteral] = START_MATCH_PUSH_ADVANCE;
        } else {
                /*No, ensure we evaluate the first state next*/
                pSpec->ByteSearchAdvance = 1;
                pStart[FirstLiteral] = START_MATCH_PUSH;
        }

        /*SIGH: Non-zero offset used by match to indicate skip search*/
        pSpec->SearchSkipOffset = 1;

        /*Are we scanning for LF (e.g. "ggrep -w $")?*/
        if (FirstLiteral == LF) {
                /*Yes, need more care at the end of the buffer*/
                pSpec->pEndmarkerActions[1] = ABANDON;
                pSpec->pNotEndmarkerActions[1] = pStart[LF];
                pStart[LF] = CHECK_BUFFER;
        }

} /*ScanByByteSearch*/


/************************************************************************/
/*                                                                      */
/*      ScanSequentially -- Set up start state to scan buffer           */
/*                                                                      */
/************************************************************************/
module_scope void
RETable_ScanSequentially(MatchEng_Spec *pSpec)
{
        UINT i;
        MatchEng_Action *pStart;
        MatchEng_Action *pFirstRealRE;
        UINT Endmarker;

        /*Default to skipping past nonmatch bytes*/
        RETable_SetState(pSpec, 1, AGAIN, MATCHENG_TABLE_TYPE_START);

        /*Start match wherever first RE state matches*/
        pStart = &pSpec->pTables[MATCHENG_TABLE_SIZE];
        pFirstRealRE = &pSpec->pTables[pSpec->FirstRealStateNr * 
                                   MATCHENG_TABLE_SIZE];

        /*Write actions for RE start search*/
        for (i = 0; i < MATCHENG_TABLE_SIZE; i++, pStart++, pFirstRealRE++) {
                /*Does the following table entry simply advance?*/
                if (pStart[MATCHENG_TABLE_SIZE] == ADVANCE) {
                        /*Yes, optimise RE match start to immediately advance*/
                        *pStart = START_MATCH_PUSH_ADVANCE;
                        continue;
                }

                /*Does the first real state match this byte?*/
                if ((*pFirstRealRE != NO_MATCH) && 
                                (*pFirstRealRE != ABANDON)) {
                        /*Yes, this entry triggers a match attempt*/
                        *pStart = START_MATCH_PUSH;

                }

        }

        /*Have we been asked to count lines?*/
        pStart = &pSpec->pTables[MATCHENG_TABLE_SIZE];
        if (pSpec->SpecFlags & MATCHENG_SPEC_COUNT_LINES) {
                /*Yes, does LF start a match?*/
                if (( pStart[LF] == START_MATCH_PUSH) || 
                    (pStart[LF] == START_MATCH_PUSH_ADVANCE)) {
                        /*Yes, note LF and start match*/
                        pStart[LF] = NOTE_LINE_START_PUSH;

                        /*Have we initialised the deferred line inc table?*/
                        if (! gRETable.IncLineTableInitialised) {
                                /*No, initialise it now*/
                                for (i = 0; i < MATCHENG_TABLE_SIZE; i++) {
                                        gRETable.IncLineTable[i] = 
                                                INC_LINE_COUNT;
                                }
                                gRETable.IncLineTableInitialised = TRUE;
                        }

                        /*Provide table reference for search to use*/
                        pSpec->pIncLineTable = gRETable.IncLineTable;

                } else {
                        /*No, simply count the LF and keep scanning*/
                        pStart[LF] = NOTE_LINE;
                }
        }

        /*Use endmarker to delimit buffer*/
        Endmarker = MATCHENG_SPEC_ENDMARKER_R(pSpec->SpecFlags);

        /*Does LF match in the starting state?*/
        if (pStart[LF] != AGAIN) {
                /*Yes, we must be careful (but slower): look for appended LF*/
                Endmarker = LF;
        }

        /*Set up start state to terminate search when endmarker found*/
        pSpec->pEndmarkerActions[1] = ABANDON;
        pSpec->pNotEndmarkerActions[1] = pStart[Endmarker];
        pStart[Endmarker] = CHECK_BUFFER;
        pSpec->TrailingLiteral = Endmarker;
        pSpec->EndCondition = MATCHENG_CONDITION_TRAILING_LITERAL;
        pSpec->PatternLength = 1;

        /*Use opt flag (hack) to get match engine to find line start*/
        pSpec->SearchSkipOffset = 1;

} /*ScanSequentially*/


/************************************************************************/
/*                                                                      */
/*      PrepareStart -- Set up state to scan buffer for match           */
/*                                                                      */
/*      We always scan the file as a buffer -- assuming that lines      */
/*      are terminated by LF (and, optionally, by CR/LF).  The          */
/*      "start" state is the state that scans the buffer looking        */
/*      for worthwhile positions to commence a match attempt.           */
/*      Other functions (such as line counting) may be implemented      */
/*      in this state.                                                  */
/*                                                                      */
/*      The key to the speed of the search is the speed with            */
/*      which you can skip over lines that don't match.  In             */
/*      particular, this means trying to use clever algorithms          */
/*      such as the Boyer-Moore string search algorithm, or by          */
/*      applying brute force techniques such as the processor's         */
/*      string search instruction.  This function analyses the          */
/*      start of the regular expression and selects the buffer          */
/*      scan technique that seems the best bet (based on the            */
/*      nature of the RE and possibly some guesses as to the            */
/*      likely nature of the data being searched).                      */
/*                                                                      */
/*      Note that we assume that the RE's easiest part to search        */
/*      is at the start of the RE -- that the caller has                */
/*      rearranged the search to ensure the easiest bit to find         */
/*      is searched first.                                              */
/*                                                                      */
/************************************************************************/
module_scope void
RETable_PrepareStart(MatchEng_Spec *pSpec)
{
        UINT NrSkipStates;
        BOOL FirstIsLiteral;
        BYTE FirstLiteral;

        /*Is the first state a dead-end state?*/
        if (MATCHENG_TABLE_TYPE_R(pSpec->pTableFlags[2]) ==
            MATCHENG_TABLE_TYPE_DEAD_END) {
                /*Yes, make starting state abandon instantly*/
                RETable_SetState(pSpec, 1, 
                                 ABANDON, 
                                 MATCHENG_TABLE_TYPE_START);
                return;
        }

        /*Is the search anchored to the start of the RE?*/
        if (pSpec->pTableFlags[1] == MATCHENG_TABLE_TYPE_START_LINE) {
                /*Yes, set up start table to search for LF as line start*/
                RETable_ScanForLineStart(pSpec);
                return;
        }

        /*Find out if starting states are amenable to skip-style searching*/
        NrSkipStates = 0;
        FirstIsLiteral = FALSE;
        if (pSpec->SpecFlags & MATCHENG_SPEC_SKIP_BYTES) {
                RETable_AnalyseStart(pSpec, &NrSkipStates, 
                                     &FirstIsLiteral, &FirstLiteral);
        }

        /*Set up start state to scan buffer based on analysis*/
        if (FirstIsLiteral && (NrSkipStates < 3)) {
                /*Literal: Does the nest state contain the literal match?*/
                if (pSpec->FirstRealStateNr == 2) {
                        /*Yes, scan buffer using CPU's byte search instr*/
                        RETable_ScanByByteSearch(pSpec, FirstLiteral, TRUE);
                } else {
                        /*No, scan with CPU instr but don't skip next state*/
                        RETable_ScanByByteSearch(pSpec, FirstLiteral, FALSE);
                }

        } else if (NrSkipStates >= 2) {
                /*Scan buffer using table-driven version of Boyer-Moore*/
                RETable_ScanByBoyerMoore(pSpec, NrSkipStates);
                pSpec->EndCondition = MATCHENG_CONDITION_TRAILING_LITERAL;

        } else {
                /*Slog through buffer as best we can*/
                RETable_ScanSequentially(pSpec);
        }

} /*PrepareStart*/


/************************************************************************/
/*                                                                      */
/*      IterationOpt -- Use following table to optimise backtracking    */
/*                                                                      */
/*      Each time an iterative part of the RE is reached, it is         */
/*      matched as far as possible, and then matching the rest of       */
/*      the expression is attempted.  If the next match fails, the      */
/*      last iteration is undone and matching the remainder is          */
/*      attempted again.  This is powerful but expensive.               */
/*                                                                      */
/*      IterationOpt finds significant savings by reducing the number   */
/*      of backtracking pathways that need to be considered.            */
/*      This is done by observing where the following table does        */
/*      not match, and editing the actions in the iteration table       */
/*      to not bother pushing a backtracking path for those bytes.      */
/*      For example, "fred.*bloggs" can be improved since we can        */
/*      see that during the ".*" iteration, it is only worth            */
/*      attempting to backtrack if the character was "b".               */
/*                                                                      */
/*      The optimisation is very easy to implement: for each byte       */
/*      of the input, if the iteration table matches with               */
/*      AGAIN_PUSH_ADVANCE but the following table has NO_MATCH,        */
/*      the backtracking push is worthless, so change the entry         */
/*      in the iteration table to AGAIN.                                */
/*                                                                      */
/************************************************************************/
module_scope void
RETable_IterationOpt(MatchEng_Action *pTable)
{
        UINT i;
        MatchEng_Action *pNext;

        /*Check each iteration table entry against next table*/
        pNext = pTable + MATCHENG_TABLE_SIZE;
        for (i = 0; i < MATCHENG_TABLE_SIZE; i++, pTable++, pNext++) {
                /*Does the current table entry include backtracking?*/
                if (*pTable != AGAIN_PUSH_ADVANCE) {
                        /*No, ignore entry*/
                        continue;
                }

                /*Does this char in the next table represent a dead-end?*/
                if (*pNext != NO_MATCH) {
                        /*No, optimisation is not possible*/
                        continue;
                }

                /*Change iteration to not bother backtracking*/
                *pTable = AGAIN;

        }

} /*IterationOpt*/


/************************************************************************/
/*                                                                      */
/*      ZeroTripOpt -- Use following table to optimise backtracking     */
/*                                                                      */
/*      Similar to the iteration optimisation above, this               */
/*      function seeks to reduce the effort expended by tables          */
/*      containing zero trip cases where the next table doesn't         */
/*      match.  In these cases, BACK_AND_ADVANCE becomes                */
/*      NO_MATCH, and ADVANCE_PUSH_ZERO becomes ADVANCE.                */
/*                                                                      */
/************************************************************************/
module_scope void
RETable_ZeroTripOpt(MatchEng_Action *pTable)
{
        UINT i;
        MatchEng_Action *pNext;

        /*Check each iteration table entry against next table*/
        pNext = pTable + MATCHENG_TABLE_SIZE;
        for (i = 0; i < MATCHENG_TABLE_SIZE; i++, pTable++, pNext++) {

                /*Does this char in the next table represent a dead-end?*/
                if (*pNext != NO_MATCH) {
                        /*No, optimisation is not possible*/
                        continue;
                }

                /*Optimise entry if it tries to advance*/
                if (*pTable == BACK_AND_ADVANCE) {
                        *pTable = NO_MATCH;

                } else if (*pTable == ADVANCE_PUSH_ZERO) {
                        *pTable = ADVANCE;
                }

        }

} /*ZeroTripOpt*/


/************************************************************************/
/*                                                                      */
/*      MetaMatchOpt -- Optimise zero-with (meta) tables                */
/*                                                                      */
/*      A meta-table implements a test without advancing the match      */
/*      position.  Therefore, any character that doesn't match in       */
/*      the next state also doesn't match in this state.                */
/*      This function applies this optimisation to the meta table.      */
/*                                                                      */
/************************************************************************/
module_scope void
RETable_MetaMatchOpt(MatchEng_Action *pTable)
{
        UINT i;
        MatchEng_Action *pNext;

        /*Check each meta table entry against next table*/
        pNext = pTable + MATCHENG_TABLE_SIZE;
        for (i = 0; i < MATCHENG_TABLE_SIZE; i++, pTable++, pNext++) {
                /*Does this char in the next table represent a dead-end?*/
                if (*pNext != NO_MATCH) {
                        /*No, optimisation is not possible*/
                        continue;
                }

                /*Since next can't match, no point in our matching, either*/
                *pTable = NO_MATCH;

        }

} /*MetaMatchOpt*/


/************************************************************************/
/*                                                                      */
/*      TrimWordAdvance -- Trim table since next comes a word test      */
/*                                                                      */
/*      In the case of (say) "[0-z]\<", we can see that the word        */
/*      boundary test reduces the set of matching characters in the     */
/*      preceding class.  This function optimises this case by          */
/*      rewriting actions that advance but we know won't match to       */
/*      non-match actions.                                              */
/*                                                                      */
/************************************************************************/
module_scope void
RETable_TrimWordAdvance(MatchEng_Action *pTable, BOOL NextMatchesWord)
{
        UINT i;
        BOOL IsWord;

        /*Check each meta table entry against next table*/
        for (i = 0; i < MATCHENG_TABLE_SIZE; i++, pTable++) {
                /*Decide whether this entry is a word char*/
                IsWord = isalnum(i) || (i == (UINT) '_');

                /*Would this char match in the next state?*/
                if (IsWord == NextMatchesWord) {
                        /*Yes, don't change its action*/
                        continue;
                }

                /*Change action if it tries to advance*/
                if (*pTable == ADVANCE) {
                        *pTable = NO_MATCH;
                }

        }

} /*TrimWordAdvance*/


/************************************************************************/
/*                                                                      */
/*      ZeroTripOptEnd -- Eliminate redundancy where z-trip at end      */
/*                                                                      */
/*      For each entry where the next table has COMPLETED, change       */
/*      ADVANCE_PUSH_ZERO to ADVANCE.  Not a really big win, and        */
/*      it's possible that this optimisation costs more to              */
/*      perform than it saves, but included anyway as it will           */
/*      certainly be worthwhile on files with many matches.             */
/*                                                                      */
/*      Only use this where the final state has COMPLETED for           */
/*      all actions -- otherwise the zero trip may be required.         */
/*      In particular, do not call this routine if the final state      */
/*      has been trimmed to match an end-of-word boundary.              */
/*                                                                      */
/************************************************************************/
module_scope void
RETable_ZeroTripOptEnd(MatchEng_Action *pTable)
{
        UINT i;

        /*Change all ADVANCE_PUSH_ZERO actions to ADVANCE*/
        for (i = 0; i < MATCHENG_TABLE_SIZE; i++, pTable++) {
                /*Is this item an optional element at the end of the RE?*/
                if (*pTable == ADVANCE_PUSH_ZERO) {
                        /*Yes, no point in pushing backtrack*/
                        *pTable = ADVANCE;
                        continue;
                }
        }

} /*ZeroTripOptEnd*/


/************************************************************************/
/*                                                                      */
/*      OptimiseTables -- Optimise by comparing table activities        */
/*                                                                      */
/*      This function works through the table-driven version of         */
/*      the RE from last state to first, looking for relationships      */
/*      between tables that allow work to be eliminated.  Most          */
/*      importantly, in expressions like x.*y, the .* state             */
/*      doesn't need to store any backtracking context unless the       */
/*      character matched is a y.  The last-to-first order may          */
/*      let some optimisations ripple up through multiple tables.       */
/*                                                                      */
/*      This function also helps implement the word search              */
/*      function, if specified, by trimming each table so that          */
/*      only word characters match.                                     */
/*                                                                      */
/************************************************************************/
module_scope void
RETable_OptimiseTables(MatchEng_Spec *pSpec)
{
        MatchEng_TableFlags *pFlags;
        MatchEng_Action *pTable;

        pFlags = &pSpec->pTableFlags[pSpec->NrTables];
        pTable = &pSpec->pTables[pSpec->NrTables * MATCHENG_TABLE_SIZE];

        /*Does final table before success table contain zero trip?*/
        pFlags--;
        pTable -= MATCHENG_TABLE_SIZE;
        if ((*pFlags & MATCHENG_TABLE_ZERO_TRIP) && 
                         (MATCHENG_TABLE_TYPE_R(pFlags[1]) == 
                                    MATCHENG_TABLE_TYPE_SUCCESS)) {
                /*Yes, optimise redundant final push where possible*/
                RETable_ZeroTripOptEnd(pTable);
        }

        /*Post-process tables in last-to-first order*/
        for (; pFlags > &pSpec->pTableFlags[1];
                        pFlags--, pTable -= MATCHENG_TABLE_SIZE) {
                /*Did this table contain iteration?*/
                if (*pFlags & MATCHENG_TABLE_ITERATIVE) {
                        /*Yes, optimise iteration backtracking*/
                        RETable_IterationOpt(pTable);
                }

                /*Did this table contain an optional (zero-trip) match?*/
                if (*pFlags & MATCHENG_TABLE_ZERO_TRIP) {
                        /*Yes, optimise backtracking as per iteration*/
                        RETable_ZeroTripOpt(pTable);
                }

                /*Does this table implement a meta-match (doesn't advance)?*/
                if (*pFlags & MATCHENG_TABLE_WIDTH_META) {
                        /*Yes, optimise nonmatches from next state*/
                        RETable_MetaMatchOpt(pTable);

                        /*Look for additional optimisations if word begin/end*/
                        switch (MATCHENG_TABLE_TYPE_R(*pFlags)) {
                        case MATCHENG_TABLE_TYPE_WORD_BEGINNING:
                                RETable_TrimWordAdvance(pTable - 
                                                        MATCHENG_TABLE_SIZE, 
                                                        FALSE);
                                break;

                        case MATCHENG_TABLE_TYPE_WORD_END:
                                RETable_TrimWordAdvance(pTable - 
                                                        MATCHENG_TABLE_SIZE, 
                                                        TRUE);
                                break;

                        default:
                                break;

                        }

                }

        }

} /*OptimiseTables*/


/************************************************************************/
/*                                                                      */
/*      WordFinalState -- Prepare success state for word match          */
/*                                                                      */
/*      Sets up the "success" state so that we only match if            */
/*      character immediately after matched text is a nonword           */
/*      character.                                                      */
/*                                                                      */
/************************************************************************/
module_scope void
RETable_WordFinalState(MatchEng_Spec *pSpec, UINT StateNr)
{
        UINT Ch;
        MatchEng_Action *pAction;

        /*Work through each character of table*/
        pAction = &pSpec->pTables[StateNr * MATCHENG_TABLE_SIZE];
        pSpec->pTableFlags[StateNr] = MATCHENG_TABLE_TYPE_SUCCESS_WORD_END;
        for (Ch = 0; Ch < MATCHENG_TABLE_SIZE; Ch++, pAction++) {
                /*Is this character a nonword?*/
                if (! IS_WORD(Ch)) {
                        /*Yes, it completes match satisfactorily*/
                        *pAction = COMPLETED;
                } else {
                        /*Sorry, match failed*/
                        *pAction = NO_MATCH;
                }
        }

} /*WordFinalState*/


/************************************************************************/
/*                                                                      */
/*      CopyLFtoCR -- Make CR serve as a line terminator                */
/*                                                                      */
/*      Copies the actions for LF in each state to CR.  This is         */
/*      so that we can treat lines that end with CR/LF equally to       */
/*      lines that end with LF.  However, note that our treatment       */
/*      isn't perfect: we don't check that an LF follows the CR.        */
/*      This can be fixed by changing the CR action into a              */
/*      CHECK_FOR_LF action, but behoffski's very weary at the          */
/*      moment and is too tired to go off and do this.  (You need       */
/*      to save the action replaced by CR for execution in case         */
/*      the next character isn't a LF.)                                 */
/*                                                                      */
/************************************************************************/
module_scope void
RETable_CopyLFtoCR(MatchEng_Spec *pSpec)
{
        MatchEng_Action *pTable;
        MatchEng_Action *pTableEnd;

        pTable = &pSpec->pTables[2 * MATCHENG_TABLE_SIZE];
        pTableEnd = &pSpec->pTables[(pSpec->NrTables + 1) * 
                                   MATCHENG_TABLE_SIZE];

        /*Post-process tables in last-to-first order*/
        for (; pTable != pTableEnd; pTable += MATCHENG_TABLE_SIZE) {
                /*Copy LF action to CR in this table*/
                pTable[CR] = pTable[LF];
        }

} /*CopyLFtoCR*/


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
public_scope BOOL
RETable_Expand(RegExp_Specification *pRESpec, 
               LWORD Flags, 
               MatchEng_Spec **ppSpec)
{
        MatchEng_Spec *pSpec;
        RE_ELEMENT *pRE;
        RE_ELEMENT *pRESave;
        UINT StateNr;
        BOOL AnchoredToEnd = FALSE;
        BOOL AnchoredToStart = FALSE;

        /*Check arguments*/
        if (pRESpec == NULL) {
                return FALSE;
        }

        /*Destroy return value to reduce chances of being misinterpreted*/
        *ppSpec = (MatchEng_Spec *) NIL;

        /*Acquire resources (e.g. memory) for optimised search*/
        if (! RETable_Create(pRESpec, &pSpec)) {
                /*Sorry, unable to get required resources*/
                return FALSE;
        }

        /*Record specification flags supplied by caller*/
        pSpec->SpecFlags = Flags;

        /*Work through RE specification, creating tables as we go*/
        pRE = pRESpec->CodeList;
        StateNr = 2;

        /*?? HACK? ALERT: We test FirstRealStateNr == 2 if byte search*/

        /*Do we match a word beginning at the start of the RE?*/
        if (REGEXP_TYPE_R(*pRE) == REGEXP_TYPE_WORD_PREV_NONWORD) {
                /*Yes, add state to test for this case*/
                RETable_WordMetaState(pSpec, 
                                 StateNr, 
                                 MATCHENG_TABLE_TYPE_PREV_NONWORD | 
                                      MATCHENG_TABLE_WIDTH_META, 
                                 PREV_NONWORD, 
                                 PREV_NONWORD);

                StateNr++;
                pRE++;
        }

        /*Remember first "real" state after meta-states*/
        pSpec->FirstRealStateNr = StateNr;

        for (;;) {
                switch (REGEXP_TYPE_R(*pRE)) {
                case REGEXP_TYPE_END_OF_EXPR:
                        /*Finished expanding*/
                        break;

                case REGEXP_TYPE_ANCHOR_START:
                        /*Set start table flag now as a signal for later*/
                        pSpec->pTableFlags[1] = 
                                MATCHENG_TABLE_TYPE_START_LINE;
                        AnchoredToStart = TRUE;
                        pRE++;
                        continue;

                case REGEXP_TYPE_ANCHOR_END:
                        /*Are all remaining RE elements optional?*/
                        if (! RegExp_AllOptional(pRE + 1)) {
                                /*No, can't match: destroy start and finish*/
                                RETable_SetState(pSpec, 
                                                 1, 
                                                 ABANDON, 
                                                 MATCHENG_TABLE_TYPE_START);
                                *ppSpec = pSpec;
                                return TRUE;
                        }

                        /*Search anchored to end: only EOL succeeds*/
                        AnchoredToEnd = TRUE;
                        RETable_SetState(pSpec, 
                                         StateNr, 
                                         NO_MATCH, 
                                         MATCHENG_TABLE_TYPE_SUCCESS);
                        pSpec->pTables[StateNr * 
                                      MATCHENG_TABLE_SIZE + LF] = 
                                COMPLETED;

                        goto FinishedExpanding;

                        break;

                case REGEXP_TYPE_WORD_BEGINNING:
                case REGEXP_TYPE_WORD_END:
                case REGEXP_TYPE_WORD_BOUNDARY:
                case REGEXP_TYPE_WORD_NONBOUNDARY:
                        /*Meta-match: don't expand if it is optional*/
                        switch (REGEXP_REPEAT_R(*pRE)) {
                        case REGEXP_REPEAT_0_OR_1:
                        case REGEXP_REPEAT_0_OR_MORE:
                                pRE++;
                                continue;
                        }

                        /*Does this meta-match occur at the start of the RE?*/
                        if (StateNr == pSpec->FirstRealStateNr) {
                                /*Yes, note leading meta-states*/
                                pSpec->FirstRealStateNr++;
                        }

                        /*FALLTHROUGH*/
                        
                default:
                        /*Handle specifier and any iterative modifiers*/
                        pRESave = pRE;
                        RETable_AddMatchChars(pSpec, StateNr, &pRE);
                        StateNr += RETable_ApplyIteration(pSpec, 
                                                          StateNr, 
                                                          pRESave);

                        /*Note: ApplyIteration might need to advance pRE*/
                        /*      when {n, m} iteration supported*/

                        StateNr++;
                        continue;

                }

                /*If we reach here, we've finished expanding RE*/
                break;

        }

        /*Note: end-anchored searches don't execute the following paragraph:*/

        /*Did the end-of-RE marker indicate nonword match?*/
        if (REGEXP_WORD_R(*pRE) == REGEXP_WORD_NONWORD_CHARS) {
                /*Yes, set last state so only nonword chars match*/
                RETable_WordFinalState(pSpec, StateNr);

        } else {
                /*No, any final character matches*/
                RETable_SetState(pSpec, 
                                 StateNr, 
                                 COMPLETED, 
                                 MATCHENG_TABLE_TYPE_SUCCESS);
        }


FinishedExpanding:

        /*Remember how many tables are in specification*/
        pSpec->NrTables = StateNr;

        /*Are we to treat CR/LF as line terminator as well as CR?*/
        if (Flags & MATCHENG_SPEC_CR_IS_TERMINATOR) {
                /*Yes, copy LF actions to CR in all tables*/
                RETable_CopyLFtoCR(pSpec);
        }

        /*Look for relationships between tables that we may optimise*/
        RETable_OptimiseTables(pSpec);

        /*Prepare starting state to scan buffer for RE start*/
        RETable_PrepareStart(pSpec);

        /*Was the RE merely an end-of-line anchor?*/
        if (AnchoredToEnd && 
            (StateNr == pSpec->FirstRealStateNr) && 
            (! AnchoredToStart)) {
                /*Yes, modify start+success states to match correctly*/
                RETable_AnchoredToEnd(pSpec);
        }

        /*Write optimised match specification to caller and report success*/
        *ppSpec = pSpec;
        return TRUE;

} /*Expand*/


