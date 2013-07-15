/************************************************************************/
/*                                                                      */
/*      MatchEng -- Match engine interface and supporting definitions   */
/*                                                                      */
/*      This interface file describes the language used to specify      */
/*      how to perform the pattern match and to report match results.   */
/*      The language is closely related to the assembly-language        */
/*      match engine since the search outperforms conventional          */
/*      coding techniques only where the assembly is used.              */
/*                                                                      */
/*      However, this version for GNU C uses the "computed goto"        */
/*      extension, and so may vary from the assembly original.          */
/*                                                                      */
/*      Copyright (C) 1995-2000 Grouse Software.  All rights reserved.  */
/*      Written for Grouse by behoffski (Brenton Hoff).                 */
/*                                                                      */
/*      Free software: no warranty; use anywhere is ok; spread the      */
/*      sources; note any mods; share variations and derivatives        */
/*      (including sending to behoffski@grouse.com.au).                 */
/*                                                                      */
/************************************************************************/

#ifndef MATCHENG_H
#define MATCHENG_H

#include <compdef.h>
#include <limits.h>
#include "tracery.h"

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/

#define MATCHENG_TABLE_SIZE             (UCHAR_MAX + 1)

#define MATCHENG_BOYER_MOORE_LENGTH     18

/*Actions to use while searching... threaded GCC version*/

typedef void *MatchEng_Action;

typedef struct {
        MatchEng_Action pSkipMax;
        MatchEng_Action pSkip17;
        MatchEng_Action pSkip16;
        MatchEng_Action pSkip15;
        MatchEng_Action pSkip14;
        MatchEng_Action pSkip13;
        MatchEng_Action pSkip12;
        MatchEng_Action pSkip11;
        MatchEng_Action pSkip10;
        MatchEng_Action pSkip9;
        MatchEng_Action pSkip8;
        MatchEng_Action pSkip7;
        MatchEng_Action pSkip6;
        MatchEng_Action pSkip5;
        MatchEng_Action pSkip4;
        MatchEng_Action pSkip3;
        MatchEng_Action pSkip2;
        MatchEng_Action pAgain;
        MatchEng_Action pAdvance;
        MatchEng_Action pStartMatchPush;
        MatchEng_Action pStartMatchPushAdvance;
        MatchEng_Action pAgainPushAdvance;
        MatchEng_Action pBackAndAdvance;
        MatchEng_Action pAdvancePushZero;
        MatchEng_Action pNoMatch;
        MatchEng_Action pNoteLine;
        MatchEng_Action pNoteLineStartPush;
        MatchEng_Action pStartOffsetMatch;
        MatchEng_Action pByteSearch;
        MatchEng_Action pAbandon;
        MatchEng_Action pCheckBuffer;
        MatchEng_Action pPrevNonword;
        MatchEng_Action pPrevWord;
        MatchEng_Action pSpecial;
        MatchEng_Action pCompleted;
        MatchEng_Action pEndOfLineSearch;
        MatchEng_Action pEndOfLineMatch;
        MatchEng_Action pStartOfLineSearch;
        MatchEng_Action pFoundLineStart;
        MatchEng_Action pIncLineCount;
} MatchEng_ActionCodes;

public_scope MatchEng_ActionCodes gMatchEngAct;

#define SKIP_MAX                        gMatchEngAct.pSkipMax
#define SKIP17                          gMatchEngAct.pSkip17
#define SKIP16                          gMatchEngAct.pSkip16
#define SKIP15                          gMatchEngAct.pSkip15
#define SKIP14                          gMatchEngAct.pSkip14
#define SKIP13                          gMatchEngAct.pSkip13
#define SKIP12                          gMatchEngAct.pSkip12
#define SKIP11                          gMatchEngAct.pSkip11
#define SKIP10                          gMatchEngAct.pSkip10
#define SKIP9                           gMatchEngAct.pSkip9
#define SKIP8                           gMatchEngAct.pSkip8
#define SKIP7                           gMatchEngAct.pSkip7
#define SKIP6                           gMatchEngAct.pSkip6
#define SKIP5                           gMatchEngAct.pSkip5
#define SKIP4                           gMatchEngAct.pSkip4
#define SKIP3                           gMatchEngAct.pSkip3
#define SKIP2                           gMatchEngAct.pSkip2
#define AGAIN                           gMatchEngAct.pAgain
#define ADVANCE                         gMatchEngAct.pAdvance
#define START_MATCH_PUSH                gMatchEngAct.pStartMatchPush
#define START_MATCH_PUSH_ADVANCE        gMatchEngAct.pStartMatchPushAdvance
#define AGAIN_PUSH_ADVANCE              gMatchEngAct.pAgainPushAdvance
#define BACK_AND_ADVANCE                gMatchEngAct.pBackAndAdvance
#define ADVANCE_PUSH_ZERO               gMatchEngAct.pAdvancePushZero
#define NO_MATCH                        gMatchEngAct.pNoMatch
#define NOTE_LINE                       gMatchEngAct.pNoteLine
#define NOTE_LINE_START_PUSH            gMatchEngAct.pNoteLineStartPush
#define START_OFFSET_MATCH              gMatchEngAct.pStartOffsetMatch
#define BYTE_SEARCH                     gMatchEngAct.pByteSearch
#define ABANDON                         gMatchEngAct.pAbandon
#define CHECK_BUFFER                    gMatchEngAct.pCheckBuffer
#define PREV_NONWORD                    gMatchEngAct.pPrevNonword
#define PREV_WORD                       gMatchEngAct.pPrevWord
#define SPECIAL                         gMatchEngAct.pSpecial
#define COMPLETED                       gMatchEngAct.pCompleted
#define END_OF_LINE_MATCH               gMatchEngAct.pEndOfLineMatch
#define END_OF_LINE_SEARCH              gMatchEngAct.pEndOfLineSearch
#define START_OF_LINE_SEARCH            gMatchEngAct.pStartOfLineSearch
#define FOUND_LINE_START                gMatchEngAct.pFoundLineStart
#define INC_LINE_COUNT                  gMatchEngAct.pIncLineCount

#define MATCHENG_ACTIONS_MAX            50 /*Actually 35 as of June 1997*/

/*Flags summarising individual table properties*/

typedef BYTE MatchEng_TableFlags;

#define MATCHENG_TABLE_ITERATIVE        BIT7
#define MATCHENG_TABLE_ZERO_TRIP        BIT6
#define MATCHENG_TABLE_WIDTH_META       BIT5
#define MATCHENG_TABLE_TYPE_MASK        0x1f
#define MATCHENG_TABLE_TYPE_R(Flag)     ((Flag) & MATCHENG_TABLE_TYPE_MASK)
#define MATCHENG_TABLE_TYPE_FAIL        0x00
#define MATCHENG_TABLE_TYPE_START       0x01
#define MATCHENG_TABLE_TYPE_LITERAL     0x02
#define MATCHENG_TABLE_TYPE_CLASS       0x03
#define MATCHENG_TABLE_TYPE_ALL_BUT     0x04
#define MATCHENG_TABLE_TYPE_MATCH_ANY   0x05
#define MATCHENG_TABLE_TYPE_SUCCESS     0x06
#define MATCHENG_TABLE_TYPE_START_LINE  0x07
#define MATCHENG_TABLE_TYPE_WORD_BEGINNING  0x08
#define MATCHENG_TABLE_TYPE_WORD_END    0x09
#define MATCHENG_TABLE_TYPE_WORD_BOUNDARY 0x0a
#define MATCHENG_TABLE_TYPE_WORD_NONBOUNDARY 0x0b
#define MATCHENG_TABLE_TYPE_PREV_NONWORD 0x0c
#define MATCHENG_TABLE_TYPE_SUCCESS_WORD_END 0x0d
#define MATCHENG_TABLE_TYPE_DEAD_END    0x0e
#define MATCHENG_TABLE_TYPE_THIS_IS_NOT_A_TYPE   0x1f

/*Flags specifying additional search requirements not relevant to RegExp*/

#define MATCHENG_SPEC_ENDMARKER(c)      (((LWORD) (c)) & UCHAR_MAX)

#define MATCHENG_SPEC_ENMARKER_MASK     ((LWORD) UCHAR_MAX)
#define MATCHENG_SPEC_ENDMARKER_R(Spec) ((BYTET) ((Spec) & UCHAR_MAX))
#define MATCHENG_SPEC_SKIP_BYTES        BIT16
#define MATCHENG_SPEC_CR_IS_TERMINATOR  BIT17
#define MATCHENG_SPEC_COUNT_LINES       BIT18

/*Codes to specify how to condition the end of the buffer*/

typedef enum {
        MATCHENG_CONDITION_ENDMARKER,
        MATCHENG_CONDITION_TRAILING_LITERAL
} MatchEng_EndConditioning;

/* SIGH: Should probably do word spec element-by-element*/

/*Information for table-driven match engine*/
typedef struct {
        /*----- General information required by all search engines -----*/

        /*Tracery control block for this object*/
        Tracery_ObjectInfo TraceInfo;

        /*Overall options for the specification*/
        LWORD SpecFlags;

        /*Buffer conditioning required by search*/
        MatchEng_EndConditioning EndCondition;
        BYTE TrailingLiteral;
        UINT PatternLength;

        /*Somewhat hacky variables for anchored searches*/
        UINT ByteSearchAdvance;
        UINT StartAdjustment;

        /*Pointer indicating where buffer ends*/
        BYTE *pAfterEndOfBuffer;

        /*Spare pointer for match engine's use (NB: self-tuned BM)*/
        void *pSpare1;

        /*----- Information specific to table-driven engine -----*/

        /*Flags summarising contents of each table*/
        MatchEng_TableFlags *pTableFlags;

        /*Offset for Boyer-Moore search optimisation*/
        BYTET SearchSkipOffset;

        UINT NrTables;

        /*Actions to perform when endmarker found within buffer*/
        MatchEng_Action *pEndmarkerActions;
        MatchEng_Action *pNotEndmarkerActions;

        /*State tables containing threaded codes*/
        MatchEng_Action *pTables;

        /*Additional table used if LF start match and we count lines*/
        MatchEng_Action *pIncLineTable;

        /*First match state (after meta-specifiers)*/
        UINT FirstRealStateNr;

        /*Starting state table (for anchored searches)*/
        UINT StartTableNr;

        /*Backtracking stack memory (very, very hacky, sorry)*/
        MatchEng_Action *pStack;
        void *pBacktrackMem;
        UINT32 BacktrackSize;

} MatchEng_Spec;


/*Flags to describe how match is reported*/

#define MATCHENG_RPT_FILENAME                BIT0
#define MATCHENG_RPT_LINENUMBER              BIT1
#define MATCHENG_RPT_BYTEOFFSET              BIT2
#define MATCHENG_RPT_LINECOUNT               BIT3
#define MATCHENG_RPT_HIGHLIGHT               BIT4
#define MATCHENG_RPT_SUPPRESS_FILENAME       BIT5
#define MATCHENG_RPT_NONMATCH_FILES          BIT6
#define MATCHENG_RPT_LINE                    BIT7
#define MATCHENG_RPT_MARKER_FLAG             BIT8
#define MATCHENG_RPT_REMOVE_TRAILING_CR      BIT9
#define MATCHENG_RPT_INVERT_MATCH_SENSE      BIT10
#define MATCHENG_RPT_NORMAL_MATCH_SENSE      BIT11
#define MATCHENG_RPT_MARKER(Ch)              (MATCHENG_RPT_MARKER_FLAG | \
                                                   (((LWORD) (Ch)) << 16))
#define MATCHENG_RPT_MARKER_UNPACK(Flags)    ((UINT) ((Flags) >> 16))

/*Structure for match engine to report details of successful match*/

typedef struct {
        /*Match details*/
        BYTET *pMatchStart;
        BYTET *pMatchEnd;

        /*Line details (written by match if buffer search used)*/
        UINT16T BufLineNr;
        BYTET *pLineStart;

        /*Warning: Variables above this line (were) referenced by assembly!*/

        /*Other information about match used for reporting*/
        LWORD ReportingOptions;
        CHAR *pFilename;
        UINT32 LineNr;
        BYTE *pLineEnd;

        /*Match counter: only valid if selection function supports it*/
        UINT32 LineMatchCount;

        /*Buffer details for offset reporting*/
        BYTE *pBufferStart;
        UINT32 BufferOffset;

        /*Character for delimited output*/
        BYTE MarkerChar;

} MatchEng_Details;


/************************************************************************/
/*                                                                      */
/*      Prototype for select action by search engine                    */
/*                                                                      */
/*      In order to keep the code simple and portable (and also to      */
/*      improve the speed of the code in some cases), the function      */
/*      that acts on selected lines is configured via a variable.       */
/*      This prototype describes the format of the function call.       */
/*      The function returns TRUE if search is to continue or FALSE     */
/*      if the search can be abandoned.                                 */
/*                                                                      */
/*      Only minimal details about the match are available here --      */
/*      the match start and end, and the end of the line.  Currently    */
/*      the line start is provided; this may be removed in the          */
/*      future (in which case the line start pointer will be NULL       */
/*      and the line start is defined as the first LF preceding         */
/*      the match start).  Note also that in earlier versions, the      */
/*      end of the line had been conditioned with NUL and with          */
/*      CR/LF line termination removed if configured; this has been     */
/*      removed in the interests of speed where all the user wishes     */
/*      to do is count matches, not use them.                           */
/*                                                                      */
/************************************************************************/
typedef BOOL
(MatchEng_SelectFunction)(MatchEng_Details *pDetails);


/************************************************************************/
/*                                                                      */
/*      Prototype for match action by search engine                     */
/*                                                                      */
/*      In order to improve the portability of the code (and also to    */
/*      allow some trickery to optimise match operation), the           */
/*      match function is configured via a variable.  This prototype    */
/*      describes the calling sequence for the function.                */
/*                                                                      */
/*      You might be wondering why this function type is declared       */
/*      as a pointer while SelectFunction isn't.  The reason            */
/*      is that behoffski doesn't like formalising "pointer to..."      */
/*      as an explicit type, and this includes function definitions.    */
/*      However, the PAGE_ALIGNED attribute vital to the assembly       */
/*      version needs to be attached to the type: declaring the         */
/*      function type as a pointer here is the only reliable way        */
/*      to do it.                                                       */
/*                                                                      */
/*      The PAGE_ALIGNED attribute is no longer required in the         */
/*      GNU/Linux version, as we use GCC's computed goto extension.     */
/*      Even so, the type is still defined as a function pointer.       */
/*                                                                      */
/************************************************************************/
typedef BOOL
(*MatchEng_MatchFunction)(MatchEng_Spec *pTable,
                BYTE *pTextStart,
                MatchEng_Details *pDetails);


#ifdef __cplusplus
}
#endif /*__cplusplus*/


#endif /*MATCHENG_H*/
