/************************************************************************/
/*                                                                      */
/*      TblDisp -- Describe state tables in concise, readable format    */
/*                                                                      */
/*      This module presents the state tables created for a             */
/*      search in a human-readable format.  This is useful for          */
/*      understanding, debugging and testing the software.              */
/*                                                                      */
/*      TblDisp was created very late in the development --             */
/*      over two years after the project started.  It has               */
/*      quickly become the main tool for measuring the program's        */
/*      implementation of the RE, and has highlighted a number          */
/*      of (minor) cases where the search could be optimised            */
/*      further.                                                        */
/*                                                                      */
/*      Copyright (C) 1995-2000 Grouse Software.  All rights reserved.  */
/*      Written for Grouse by behoffski (Brenton Hoff).                 */
/*                                                                      */
/*      Free software: no warranty; use anywhere is ok; spread the      */
/*      sources; note any mods; share variations and derivatives        */
/*      (including sending to behoffski@grouse.com.au).                 */
/*                                                                      */
/************************************************************************/

#include "bakslash.h"
#include "compdef.h"
#include <string.h>
#include <stdio.h>
#include "tbldisp.h"

/*Summary of action entries for Description*/
typedef struct {
        MatchEng_Action Code;
        UINT8 NrEntries;
        WORD RangeSpecs[MATCHENG_TABLE_SIZE / 2];

} ActionDescription;

/*Table of action entries to complete the description for each state*/
typedef struct {
        UINT NrActions;
        ActionDescription Action[MATCHENG_ACTIONS_MAX];

} TableDescription;

/*List of action codes and labels*/

typedef struct {
        MatchEng_Action Code;
        CHAR *pDesc;
} ActionLabelItem;

module_scope ActionLabelItem gTblDisp_Labels[MATCHENG_ACTIONS_MAX];


/************************************************************************/
/*                                                                      */
/*      PrepareLabels -- Set up labels used for table display           */
/*                                                                      */
/*      Since GCC uses run-time values for the state machine            */
/*      action coding, the previous static array has been replaced      */
/*      by a dynamic array that must be initialised once the            */
/*      actual values are known.  This function sets up the array.      */
/*                                                                      */
/************************************************************************/
public_scope void 
TblDisp_PrepareLabels(void)
{

        ActionLabelItem *pLabel = &gTblDisp_Labels[0];

        /*Actions for start-match state*/
        pLabel->Code  = SKIP_MAX;
        pLabel->pDesc = "SkipMax";
        pLabel++;
        pLabel->Code  = SKIP17;
        pLabel->pDesc = "Skip17";
        pLabel++;
        pLabel->Code  = SKIP16;
        pLabel->pDesc = "Skip16";
        pLabel++;
        pLabel->Code  = SKIP15;
        pLabel->pDesc = "Skip15";
        pLabel++;
        pLabel->Code  = SKIP14;
        pLabel->pDesc = "Skip14";
        pLabel++;
        pLabel->Code  = SKIP13;
        pLabel->pDesc = "Skip13";
        pLabel++;
        pLabel->Code  = SKIP12;
        pLabel->pDesc = "Skip12";
        pLabel++;
        pLabel->Code  = SKIP11;
        pLabel->pDesc = "Skip11";
        pLabel++;
        pLabel->Code  = SKIP10;
        pLabel->pDesc = "Skip10";
        pLabel++;
        pLabel->Code  = SKIP9;
        pLabel->pDesc = "Skip9";
        pLabel++;
        pLabel->Code  = SKIP8;
        pLabel->pDesc = "Skip8";
        pLabel++;
        pLabel->Code  = SKIP7;
        pLabel->pDesc = "Skip7";
        pLabel++;
        pLabel->Code  = SKIP6;
        pLabel->pDesc = "Skip6";
        pLabel++;
        pLabel->Code  = SKIP5;
        pLabel->pDesc = "Skip5";
        pLabel++;
        pLabel->Code  = SKIP4;
        pLabel->pDesc = "Skip4";
        pLabel++;
        pLabel->Code  = SKIP3;
        pLabel->pDesc = "Skip3";
        pLabel++;
        pLabel->Code  = SKIP2;
        pLabel->pDesc = "Skip2";
        pLabel++;
        pLabel->Code  = AGAIN;
        pLabel->pDesc = "Again";
        pLabel++;
        pLabel->Code  = BYTE_SEARCH;
        pLabel->pDesc = "ByteSearch";
        pLabel++;
        pLabel->Code  = START_MATCH_PUSH;
        pLabel->pDesc = "StartMatchPush";
        pLabel++;
        pLabel->Code  = START_MATCH_PUSH_ADVANCE;
        pLabel->pDesc = "StartMatchPushAdvance";
        pLabel++;
        pLabel->Code  = START_OFFSET_MATCH;
        pLabel->pDesc = "StartOffsetMatch";
        pLabel++;
        pLabel->Code  = NOTE_LINE;
        pLabel->pDesc = "NoteLine";
        pLabel++;
        pLabel->Code  = NOTE_LINE_START_PUSH;
        pLabel->pDesc = "NoteLineStartPush";
        pLabel++;
        pLabel->Code  = END_OF_LINE_SEARCH;
        pLabel->pDesc = "EOLSearch";
        pLabel++;
        pLabel->Code  = END_OF_LINE_MATCH;
        pLabel->pDesc = "EOLMatch";
        pLabel++;
        pLabel->Code  = START_OF_LINE_SEARCH;
        pLabel->pDesc = "StartOfLineSearch";
        pLabel++;
        pLabel->Code  = FOUND_LINE_START;
        pLabel->pDesc = "FoundLineStart";
        pLabel++;
        pLabel->Code  = INC_LINE_COUNT;
        pLabel->pDesc = "IncLineCount";
        pLabel++;

        /*Actions that advance the match*/
        pLabel->Code  = ADVANCE;
        pLabel->pDesc = "Advance";
        pLabel++;
        pLabel->Code  = AGAIN_PUSH_ADVANCE;
        pLabel->pDesc = "AgainPushAdvance";
        pLabel++;
        pLabel->Code  = BACK_AND_ADVANCE;
        pLabel->pDesc = "BackAndAdvance";
        pLabel++;
        pLabel->Code  = ADVANCE_PUSH_ZERO;
        pLabel->pDesc = "AdvancePushZero";
        pLabel++;

        /*Actions that complete search*/
        pLabel->Code  = COMPLETED;
        pLabel->pDesc = "Completed";
        pLabel++;

        /*Meta-match actions (test text but don't add to match length)*/
        pLabel->Code  = PREV_NONWORD;
        pLabel->pDesc = "PrevNonword";
        pLabel++;
        pLabel->Code  = PREV_WORD;
        pLabel->pDesc = "PrevWord";
        pLabel++;

        /*Entries that don't match*/
        pLabel->Code  = NO_MATCH;
        pLabel->pDesc = "NoMatch";
        pLabel++;
        pLabel->Code  = ABANDON;
        pLabel->pDesc = "Abandon";
        pLabel++;

        /*End-of-buffer handling actions*/
        pLabel->Code = CHECK_BUFFER;
        pLabel->pDesc = "CheckBuffer";
        pLabel++;

        /*And finally, action reserved for future special effects*/
        pLabel->Code  = SPECIAL;
        pLabel->pDesc = "Special";
        pLabel++;

        pLabel->Code  = (MatchEng_Action) NULL;
        pLabel->pDesc = NULL;

} /*PrepareLabels*/


/************************************************************************/
/*                                                                      */
/*      DispFlag -- Report table summary described by flag variable     */
/*                                                                      */
/************************************************************************/
module_scope void
TblDisp_DispFlag(MatchEng_TableFlags Flag)
{
        UINT TableType;

        /*Display flag summary*/
        TableType = MATCHENG_TABLE_TYPE_R(Flag);
        switch (TableType) {
        case MATCHENG_TABLE_TYPE_FAIL:
                fprintf(stdout, "Fail");
                break;

        case MATCHENG_TABLE_TYPE_START:
                fprintf(stdout, "Start");
                break;

        case MATCHENG_TABLE_TYPE_LITERAL:
                fprintf(stdout, "Literal");
                break;

        case MATCHENG_TABLE_TYPE_CLASS:
                fprintf(stdout, "Class");
                break;

        case MATCHENG_TABLE_TYPE_ALL_BUT:
                fprintf(stdout, "AllBut");
                break;

        case MATCHENG_TABLE_TYPE_MATCH_ANY:
                fprintf(stdout, "MatchAny");
                break;

        case MATCHENG_TABLE_TYPE_SUCCESS:
                fprintf(stdout, "Success");
                break;

        case MATCHENG_TABLE_TYPE_START_LINE:
                fprintf(stdout, "StartLine");
                break;

        case MATCHENG_TABLE_TYPE_WORD_BEGINNING:
                fprintf(stdout, "WordBeginning");
                break;

        case MATCHENG_TABLE_TYPE_WORD_END:
                fprintf(stdout, "WordEnd");
                break;

        case MATCHENG_TABLE_TYPE_WORD_BOUNDARY:
                fprintf(stdout, "WordBoundary");
                break;

        case MATCHENG_TABLE_TYPE_WORD_NONBOUNDARY:
                fprintf(stdout, "WordNonboundary");
                break;

        case MATCHENG_TABLE_TYPE_PREV_NONWORD:
                fprintf(stdout, "PrevNonword");
                break;

        case MATCHENG_TABLE_TYPE_SUCCESS_WORD_END:
                fprintf(stdout, "SuccessWordEnd");
                break;

        case MATCHENG_TABLE_TYPE_DEAD_END:
                fprintf(stdout, "DeadEnd");
                break;

        default:
                fprintf(stdout, "?? unknown type: 0x%02x", TableType);
                break;

        }

        /*Report options included in flag*/
        if (Flag & MATCHENG_TABLE_ITERATIVE) {
                fprintf(stdout, "+iter");
        }
        if (Flag & MATCHENG_TABLE_ZERO_TRIP) {
                fprintf(stdout, "+0trip");
        }
        if (Flag & MATCHENG_TABLE_WIDTH_META) {
                fprintf(stdout, "+meta");
        }

} /*DispFlag*/


/************************************************************************/
/*                                                                      */
/*      ActionSummary -- Display summary of entries matching action     */
/*                                                                      */
/*      If the action named here is present in the description          */
/*      table, the bytes matching the action are displayed.             */
/*      C-style escapes are used to describe nonprinting values.        */
/*                                                                      */
/************************************************************************/
module_scope void
TblDisp_ActionSummary(MatchEng_Action Code, 
                      CHAR *pLabel, 
                      TableDescription *pDesc)
{
        ActionDescription *pAct;
        ActionDescription *pEnd;
        UINT Range;
        BYTE High;
        BYTE Low;
        UINT TextLen;
        CHAR Encoding[8];

        /*Search for code within description table*/
        pEnd = &pDesc->Action[pDesc->NrActions];
        pAct = &pDesc->Action[0];
        for (;;) {
                /*Have we exhausted the list?*/
                if (pAct == pEnd) {
                       /*Yes, code isn't present in list*/
                       return;
                }

                /*Have we found an entry for this code?*/
                if (pAct->Code == Code) {
                        /*Yes, found entry*/
                        break;
                }

                /*Move to the next action, if any*/
                pAct++;

        }

        /*Found entry, report it*/
        TextLen = 20;
        fprintf(stdout, "%s: ", pLabel);
        if (strlen(pLabel) < 18) {
                /*Pad label to improve alignment of reports*/
                fprintf(stdout, "%*s", 18 - strlen(pLabel), "");

        } else {
                /*Keep track of line length*/
                TextLen += strlen(pLabel) + 2;

        }

        for (Range = 0; Range < pAct->NrEntries; Range++) {
                /*Do we have space for the next entry?*/
                if (TextLen > 66) {
                        /*Possibly not... move to a new line*/
                        fprintf(stdout, "\n%28s", "");
                        TextLen = 29;
                }

                /*Report entry as one of "'a'", "'a', 'b'" or "'a'-'b'"*/
                High = HIBYTE(pAct->RangeSpecs[Range]);
                Low =  LOBYTE(pAct->RangeSpecs[Range]);
                if (High == Low) {
                        /*Range represents a single item*/
                        Bakslash_EncodeByte(Low, Encoding);
                        TextLen += fprintf(stdout, "'%s'", Encoding);

                } else if (High == (Low + 1)) {
                        Bakslash_EncodeByte(Low, Encoding);
                        TextLen += fprintf(stdout, "'%s', ", Encoding);
                        Bakslash_EncodeByte(High, Encoding);
                        TextLen += fprintf(stdout, "'%s'", Encoding);

                } else if ((High == 0x00) && (Low == 0xff)) {
                        /*Dummy entry signalling endmarker value*/
                        TextLen += fprintf(stdout, "Endmarker");
                        
                } else if ((High == 0x00) && (Low == 0xfe)) {
                        /*Dummy entry signalling endmarker value*/
                        TextLen += fprintf(stdout, "NotEndmarker");
                        
                } else {
                        Bakslash_EncodeByte(Low, Encoding);
                        TextLen += fprintf(stdout, "'%s'-", Encoding);
                        Bakslash_EncodeByte(High, Encoding);
                        TextLen += fprintf(stdout, "'%s'", Encoding);
                }

                /*Add delimiter if not at the end of the list*/
                if ((Range + 1) < pAct->NrEntries) {
                        TextLen += fprintf(stdout, ", ");
                }

        }

        fprintf(stdout, "\n");

} /*ActionSummary*/


/************************************************************************/
/*                                                                      */
/*      DescribeTable -- Generate readable description of a table       */
/*                                                                      */
/*      The description is built by organising the entries by           */
/*      action code, and by identifying ranges within each action.      */
/*      The result is a table with variable-length rows: each row       */
/*      describes an action, and the entries in each row describe       */
/*      ranges of characters within the rows.                           */
/*                                                                      */
/************************************************************************/
module_scope void
TblDisp_DescribeTable(MatchEng_Action *pTab, 
                      MatchEng_Action Endmarker,
                      MatchEng_Action NotEndmarker)
{
        TableDescription Desc;
        ActionDescription *pAction;
        ActionDescription *pAfterLastAction;
        UINT i;
        MatchEng_Action Act;
        WORD *pRange;
        ActionLabelItem *pItem;
        BOOL EndmarkerUsed = FALSE;

        /*Initialise all description entries to 0*/
        memset(&Desc, 0, sizeof(Desc));

        /*Work through each entry of table, recording action occurrences*/
        for (i = 0; i < 256; i++) {
                Act = *pTab++;
                pAction = &Desc.Action[0];
                pAfterLastAction = &Desc.Action[Desc.NrActions];
                for (;;) {
                        /*Have we run out of actions?*/
                        if (pAction == pAfterLastAction) {
                                /*Yes, need to create a new entry*/
                                Desc.NrActions++;
                                break;
                        }

                        /*Have we found the action?*/
                        if (Act == pAction->Code) {
                                /*Yes, finish search with desired entry*/
                                break;
                        }

                        /*Next entry, if any*/
                        pAction++;

                }

                /*Is this the first entry for this description?*/
                pRange = &pAction->RangeSpecs[pAction->NrEntries - 1];
                if (pAction->NrEntries == 0) {
                        /*Yes, create the first entry now*/
                        pAction->Code = Act;
                        pAction->NrEntries = 1;
                        *++pRange = BYTES2WORD(i, i);
                        if (Act == CHECK_BUFFER) {
                                EndmarkerUsed = TRUE;
                        }
                        continue;
                }

                /*Does this entry extend the last range?*/
                if ((i - 1) == HIBYTE(*pRange)) {
                        /*Yes, merely extend range*/
                        *pRange = BYTES2WORD(i, LOBYTE(*pRange));
                        continue;
                }

                /*Need to add another range to action's description*/
                pAction->Code = Act;
                pAction->NrEntries++;
                *++pRange = BYTES2WORD(i, i);

        }

        /*Only describe endmarkers/notendmarkers if relevant*/
        if (EndmarkerUsed) {
                /*Add endmarker using range (high, low) of (0x00, 0xff)*/
                pAction = &Desc.Action[0];
                pAfterLastAction = &Desc.Action[Desc.NrActions];
                for (;;) {
                        /*Have we run out of actions?*/
                        if (pAction == pAfterLastAction) {
                                /*Yes, need to create a new entry*/
                                Desc.NrActions++;
                                pAction->Code = Endmarker;
                                pAction->NrEntries = 1;
                                pAction->RangeSpecs[0] = 
                                                BYTES2WORD(0x00, 0xff);
                                break;
                        }


                        /*Have we found the action?*/
                        if (Endmarker == pAction->Code) {
                                /*Yes, add entry signifying endmarker*/
                                pAction->RangeSpecs[pAction->NrEntries] = 
                                                BYTES2WORD(0x00, 0xff);
                                pAction->NrEntries++;
                                break;
                        }
                        
                        /*Next entry, if any*/
                        pAction++;
                
                }

                /*Add notendmarker using special range of (0x00, 0xfe)*/
                pAction = &Desc.Action[0];
                pAfterLastAction = &Desc.Action[Desc.NrActions];
                for (;;) {
                        /*Have we run out of actions?*/
                        if (pAction == pAfterLastAction) {
                                /*Yes, need to create a new entry*/
                                Desc.NrActions++;
                                pAction->Code = NotEndmarker;
                                pAction->NrEntries = 1;
                                pAction->RangeSpecs[0] = 
                                                BYTES2WORD(0x00, 0xfe);
                                break;
                }


                        /*Have we found the action?*/
                        if (NotEndmarker == pAction->Code) {
                                /*Yes, add entry signifying endmarker*/
                                pAction->RangeSpecs[pAction->NrEntries] = 
                                                BYTES2WORD(0x00, 0xfe);
                                pAction->NrEntries++;
                                break;
                        }

                        /*Next entry, if any*/
                        pAction++;
                
                }

        }



        /*Report table entries in fairly coherent order*/
        for (pItem = &gTblDisp_Labels[0]; pItem->pDesc != NULL; pItem++) {
                TblDisp_ActionSummary(pItem->Code, pItem->pDesc, &Desc);
        }

} /*DescribeTable*/


/************************************************************************/
/*                                                                      */
/*      Describe -- Report state tables in coherent fashion             */
/*                                                                      */
/*      This routine is springing into existence as part of the         */
/*      white-box testing of the system.  It describes each state       */
/*      table in concise but readable format.  This output is           */
/*      checked by part of the automated test rig, so don't             */
/*      change this routine unless you're willing to update the         */
/*      tests.                                                          */
/*                                                                      */
/************************************************************************/
public_scope void
TblDisp_Describe(MatchEng_Spec *pSpec, CHAR *pTitle)
{
        MatchEng_Action *pTab = pSpec->pTables;
        MatchEng_TableFlags *pFlags = pSpec->pTableFlags;
        UINT TableType;
        UINT TableNr = 0;

        /*Display the title for this specification*/
        fprintf(stdout, "\n========== %s ==========\n", pTitle);

        /*Work through each table of the spec in turn*/
        for (;;) {
                fprintf(stdout, "[Table %u, ", TableNr);
                TblDisp_DispFlag(*pFlags);
                fprintf(stdout, "]\n");

                /*Generate the description for this table*/
                TblDisp_DescribeTable(pTab, 
                                      pSpec->pEndmarkerActions[TableNr], 
                                      pSpec->pNotEndmarkerActions[TableNr]);


                fprintf(stdout, "\n");

                /*Was that the final table?*/
                TableType = MATCHENG_TABLE_TYPE_R(*pFlags++);
                if ((TableType == MATCHENG_TABLE_TYPE_SUCCESS) || 
                    (TableType == MATCHENG_TABLE_TYPE_SUCCESS_WORD_END)) {
                        /*Yes, finished displaying*/
                        break;
                }

                /*Move to the next table*/
                pTab += MATCHENG_TABLE_SIZE;
                TableNr++;

        }

} /*Describe*/


/************************************************************************/
/*                                                                      */
/*      Start -- Begin managing what has to be managed                  */
/*                                                                      */
/************************************************************************/
public_scope BOOL
TblDisp_Start(void)
{
        /*Started successfully*/
        return TRUE;

} /*Start*/


/************************************************************************/
/*                                                                      */
/*      Init -- Prepare module for operation                            */
/*                                                                      */
/************************************************************************/
public_scope void
TblDisp_Init(void)
{
} /*Init*/
