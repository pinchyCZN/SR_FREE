/************************************************************************/
/*                                                                      */
/*      RegExp -- Create and manipulate regular expressions             */
/*                                                                      */
/*      This module compresses the ASCII text form of regular           */
/*      expressions into a much more compact and manageable form.       */
/*      The compact form can be used to drive a match engine,           */
/*      or can be expanded where table-driven searching is used.        */
/*                                                                      */
/*      The files have been renamed to regexp00.[ch] to avoid           */
/*      any confusion with the far more important standard              */
/*      regular expression facilities in standard Unixes.               */
/*      However, the module name used inside the file has not           */
/*      been changed.  Apologies if this causes any confusion.          */
/*                                                                      */
/*      The code could also be improved by separating the               */
/*      bitmasks accompanying each class specifier into a separate      */
/*      array.  This would allow the RE encoding to be traversed        */
/*      backwards as well as forwards.  Requires (a little) more        */
/*      memory management than existing sequential scheme.              */
/*      I suspect that it would be still better to use                  */
/*      doubly-linked lists.  However, these sorts of issues            */
/*      can wait until the next version (supporting extended RE         */
/*      syntax, and hopefully also DFAs?) is attempted.                 */
/*                                                                      */
/*      This module now uses 32-bit codes; the original used            */
/*      16-bit codes.  This should make RE elements easier to           */
/*      manage.  Hopefully, this format will also permit                */
/*      storage of Unicode glyphs.                                      */
/*                                                                      */
/*      Some portions of this code were derived after reading           */
/*      GNU Sed v1.x by Eric S. Raymond (modified by Hern Chen).        */
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
#include <memory.h>
#include "platform.h"
#include "regexp00.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/*Limit on number of codes allowed in a compiled format (sigh)*/
#define REGEXP_CODES_MAX                8192

/*Estimate of worst-case overhead added to RE-specified codes (e.g. -w, -x)*/
#define REGEXP_CODE_OVERHEAD            20

/*Rough character frequency table to hopefully improve EasiestFirst*/

module_scope UINT RegExp_ByteFrequency[256] = {
         2358,   252,   196,   345,   159,   164,   232,   143, /*  0-  7*/
          225,  1468,  7426,    93,   119,  6618,   123,   242, /*  8- 15*/
          110,    61,   106,    65,    84,    76,    90,    52, /* 16- 23*/
           73,    52,    97,   130,    66,    49,    89,    72, /* 24- 31*/
       65535u,   248,   994,   374,   278,   366,   485,   651, /* 32- 39*/
         1380,  1362,  4321,   370,  2892,  3537,  4792,  2544, /* 40- 47*/
         8016,  4254,  3861,  2130,  2600,  1925,  1866,  1569, /* 48- 55*/
         2066,  2143,  1295,  1019,   759,  1038,   837,   198, /* 56- 63*/
          247,  2432,  1170,  2261,  1831,  2773,  1926,   927, /* 64- 71*/
          846,  2201,   269,   411,  1373,  1404,  1468,  1712, /* 72- 79*/
         1950,   226,  2075,  2469,  2169,   801,   641,   701, /* 80- 87*/
          414,   412,   627,   525,   775,   449,   284,  1557, /* 88- 95*/
           70,  9743,  2214,  4936,  5133, 15939,  3254,  2665, /* 96-103*/
         4340,  9307,   387,  1206,  5902,  3765,  8797,  9854, /*104-111*/
         3935,   260,  9408,  8591, 11138,  4156,  1738,  1756, /*112-119*/
         1168,  2075,   518,   327,   346,   334,   255,    75, /*120-127*/
          134,    71,    99,   243,    88,    59,   113,    66, /*128-135*/
           88,   243,    92,   368,   109,    91,   121,    52, /*136-143*/
           86,    49,    60,    47,    56,    44,    63,    53, /*144-151*/
           62,    66,   219,    70,    59,    45,    65,    55, /*152-159*/
           60,    57,    56,    55,    51,    49,    50,    45, /*160-167*/
           59,    48,    70,    52,    56,    53,    59,    58, /*168-175*/
           53,    48,    55,    51,    56,    40,    66,    52, /*176-183*/
          127,    60,    70,    55,    62,    44,    61,    52, /*184-191*/
           76,    68,    56,    58,   326,    51,    69,   131, /*192-199*/
           91,    68,    70,    67,    63,   100,    58,    48, /*200-207*/
           60,    61,    82,    49,    49,    44,    51,    44, /*208-215*/
          109,    52,    68,    61,    66,    54,    62,    80, /*216-223*/
           75,    45,    68,    48,    63,    47,    71,    49, /*224-231*/
           88,   260,    65,    66,    95,    48,    78,    52, /*232-239*/
           90,    56,    78,    45,    88,    42,    89,    62, /*240-247*/
          100,    67,   104,   202,   109,    71,   110,   660  /*248-255*/
};

typedef struct {
        /*Reason for last failure (if not associated with an existing RE)*/
        RegExp_FailureCode FailureCode;

} REGEXP_MODULE_CONTEXT;

module_scope REGEXP_MODULE_CONTEXT gRegExp;


/************************************************************************/
/*                                                                      */
/*      ShowCodes -- Disassemble RE coding (for debugging)              */
/*                                                                      */
/************************************************************************/
public_scope void
RegExp_ShowCodes(CHAR *pLabel, RegExp_Specification *pRE)
{
        RE_ELEMENT *pElem = pRE->CodeList;
        RE_ELEMENT Elem;
        UINT CodeIx;
        UINT Value;

        /*Display information carried around about RE*/
        printf("%s(%u)", pLabel, pRE->NrStates);

        /*Loop through each code element*/
        for (;;) {
                /*Display element*/
                Elem = *pElem++;
                switch (REGEXP_TYPE_R(Elem)) {
                case REGEXP_TYPE_MATCH_ANY:
                        printf(" _ANY");
                        break;

                case REGEXP_TYPE_CLASS:
                        printf(" _CLASS[");
                        for (CodeIx = 0; CodeIx < REGEXP_BITMASK_NRCODES;
                                        CodeIx++) {
                                printf("%08lx", *pElem++);
                        }
                        printf("]");
                        break;

                case REGEXP_TYPE_CLASS_ALL_BUT:
                        printf(" _ALLBUT[");
                        for (CodeIx = 0; CodeIx < REGEXP_BITMASK_NRCODES;
                                        CodeIx++) {
                                printf("%08lx", *pElem++);
                        }
                        printf("]");
                        break;

                case REGEXP_TYPE_ANCHOR_START:
                        printf(" _ANCHOR");
                        break;

                case REGEXP_TYPE_ANCHOR_END:
                        printf(" _EOL");
                        break;

                case REGEXP_TYPE_END_OF_EXPR:
                        printf(" _END");
                        break;

                case REGEXP_TYPE_LITERAL:
                        /*Display it directly (?)*/
                        Value = REGEXP_LITERAL_R(Elem);
                        if (Value > 255) {
                                /*Unicode value*/
                                printf(" \\u%04x", Value);
                        } else if (isprint(Value)) {
                                printf(" %c", Value);
                        } else {
                                printf(" \\x%02x", Value);
                        }
                        break;

                case REGEXP_TYPE_WORD_BEGINNING:
                        printf(" _WordBeginning");
                        break;

                case REGEXP_TYPE_WORD_END:
                        printf(" _WordEnd");
                        break;

                case REGEXP_TYPE_WORD_BOUNDARY:
                        printf(" _WordBoundary");
                        break;

                case REGEXP_TYPE_WORD_NONBOUNDARY:
                        printf(" _WordNonboundary");
                        break;

                case REGEXP_TYPE_WORD_PREV_NONWORD:
                        printf(" _WordPrevNonword");
                        break;

                case REGEXP_TYPE_DEAD_END:
                        printf(" _DeadEnd");
                        break;

                default:
                        /*Unrecognised type*/
                        printf(" _Unknown(%u)", 
                               REGEXP_TYPE_R(Elem));
                        break;

                }

                /*Display element repeat modifier*/
                switch (REGEXP_REPEAT_R(Elem)) {
                case REGEXP_REPEAT_MATCH_SELF:
                        /*Default: merely show no modifier*/
                        break;

                case REGEXP_REPEAT_0_OR_1:
                        printf(":?");
                        break;

                case REGEXP_REPEAT_0_OR_MORE:
                        printf(":*");
                        break;

                case REGEXP_REPEAT_1_OR_MORE:
                        printf(":+");
                        break;

                default:
                        printf(":UnknownRepeat(%u)", 
                               REGEXP_REPEAT_R(Elem));
                        break;

                }

                /*Display case insensitivty modifier, if given*/
                if ((Elem & REGEXP_CASE_MASK) == REGEXP_CASE_INSENSITIVE) {
                        printf(":i");
                }

                /*Display word modifier, if it's interesting*/
                switch (REGEXP_WORD_R(Elem)) {
                case REGEXP_WORD_UNRESTRICTED:
                        break;

                case REGEXP_WORD_WORD_CHARS:
                        printf(":w");
                        break;

                case REGEXP_WORD_NONWORD_CHARS:
                        printf(":W");
                        break;

                default:
                        printf(":UnknownWord(%u)", 
                               REGEXP_WORD_R(Elem));
                        break;

                }

                /*Did we hit the end of the RE?*/
                if (REGEXP_TYPE_R(Elem) == REGEXP_TYPE_END_OF_EXPR) {
                        /*Yes, finished displaying codes*/
                        printf("\n");
                        return;
                }


        }

} /*ShowCodes*/


/************************************************************************/
/*                                                                      */
/*      ClassName -- Handle named classes (e.g. [:punct:])              */
/*                                                                      */
/************************************************************************/
module_scope BOOL
RegExp_ClassName(BYTE **ppText, LWORD Flags, RE_ELEMENT *pCode)
{
        BYTE *pText;
        UINT ch;
        BOOL Recognised = FALSE;

        /*Set flags based on name*/
        pText = *ppText + 1;

        /*Does the word seem to be five characters long?*/
        if ((pText[5] == ':') && (pText[6] == ']')) {
                /*Yes, look for 5-letter names*/
                if (strncmp(pText, "alnum", 5) == 0) {
                        /*Add members to list (being wary of 16-bit chars)*/
                        Recognised = TRUE;
                        ch = 0; 
                        do {
                                /*Is this character a member of the class?*/
                                if (isalnum(ch)) {
                                        /*Yes, mark it in our list*/
                                        REGEXP_BIT_SET(pCode, ch);
                                }
                        } while (ch++ != REGEXP_CHAR_MAX);

                } else if (strncmp(pText, "alpha", 5) == 0) {
                        Recognised = TRUE;
                        ch = 0; 
                        do {
                                if (isalpha(ch)) {
                                        REGEXP_BIT_SET(pCode, ch);
                                }
                        } while (ch++ != REGEXP_CHAR_MAX);

                } else if (strncmp(pText, "cntrl", 5) == 0) {
                        Recognised = TRUE;
                        ch = 0; 
                        do {
                                if (iscntrl(ch)) {
                                        REGEXP_BIT_SET(pCode, ch);
                                }
                        } while (ch++ != REGEXP_CHAR_MAX);

                } else if (strncmp(pText, "digit", 5) == 0) {
                        Recognised = TRUE;
                        ch = 0; 
                        do {
                                if (isdigit(ch)) {
                                        REGEXP_BIT_SET(pCode, ch);
                                }
                        } while (ch++ != REGEXP_CHAR_MAX);

                } else if (strncmp(pText, "graph", 5) == 0) {
                        Recognised = TRUE;
                        ch = 0; 
                        do {
                                if (isgraph(ch)) {
                                        REGEXP_BIT_SET(pCode, ch);
                                }
                        } while (ch++ != REGEXP_CHAR_MAX);

                } else if (strncmp(pText, "lower", 5) == 0) {
                        Recognised = TRUE;
                        ch = 0; 
                        do {
                                if (islower(ch)) {
                                        REGEXP_BIT_SET(pCode, ch);
                                }
                        } while (ch++ != REGEXP_CHAR_MAX);

                } else if (strncmp(pText, "print", 5) == 0) {
                        Recognised = TRUE;
                        ch = 0; 
                        do {
                                if (isprint(ch)) {
                                        REGEXP_BIT_SET(pCode, ch);
                                }
                        } while (ch++ != REGEXP_CHAR_MAX);

                } else if (strncmp(pText, "punct", 5) == 0) {
                        Recognised = TRUE;
                        ch = 0; 
                        do {
                                if (ispunct(ch)) {
                                        REGEXP_BIT_SET(pCode, ch);
                                }
                        } while (ch++ != REGEXP_CHAR_MAX);

                } else if (strncmp(pText, "space", 5) == 0) {
                        Recognised = TRUE;
                        ch = 0; 
                        do {
                                if (isspace(ch)) {
                                        REGEXP_BIT_SET(pCode, ch);
                                }
                        } while (ch++ != REGEXP_CHAR_MAX);

                } else if (strncmp(pText, "upper", 5) == 0) {
                        Recognised = TRUE;
                        ch = 0; 
                        do {
                                if (isupper(ch)) {
                                        REGEXP_BIT_SET(pCode, ch);
                                }
                        } while (ch++ != REGEXP_CHAR_MAX);

                }

                /*Did we recognise the name?*/
                if (Recognised) {
                        /*Yes, step past name and delimiter*/
                        *ppText = pText + 7;

                }

        /*No, does code seem to be 6 characters long?*/
        } else if ((pText[6] == ':') && (pText[7] == ']')) {
                /*Yes, look for 6-character names*/
                if (strncmp(pText, "xdigit", 6) == 0) {
                        Recognised = TRUE;
                        ch = 0; 
                        do {
                                if (isxdigit(ch)) {
                                        REGEXP_BIT_SET(pCode, ch);
                                }
                        } while (ch++ != REGEXP_CHAR_MAX);

                }

                /*Did we recognise the name?*/
                if (Recognised) {
                        /*Yes, step past name and delimiter*/
                        *ppText = pText + 8;

                }

        }

        /*Report whether the class name was recognised*/
        return Recognised;

} /*ClassName*/


/************************************************************************/
/*                                                                      */
/*      CountMembers -- Report how many members are in the set          */
/*                                                                      */
/************************************************************************/
module_scope UINT
RegExp_CountMembers(RE_ELEMENT *pCode)
{
        UINT Count;
        UINT ElementNr;
        UINT32 BitSet;

        /*Loop across all members of the set*/
        Count = 0;
        for (ElementNr = 0; ElementNr < REGEXP_BITMASK_NRCODES; ElementNr++) {
                /*Get the next element of the bitmask, and skip it if 0*/
                BitSet = (UINT32) *pCode++;
                if (BitSet == 0) {
                        continue;
                }

                /*Count how many bits are set in this element*/
                BitSet = ((BitSet & 0xaaaaaaaa) >>  1) + (BitSet & 0x55555555);
                BitSet = ((BitSet & 0xcccccccc) >>  2) + (BitSet & 0x33333333);
                BitSet = ((BitSet & 0xf0f0f0f0) >>  4) + (BitSet & 0x0f0f0f0f);
                BitSet = ((BitSet & 0xff00ff00) >>  8) + (BitSet & 0x00ff00ff);
                BitSet = ((BitSet & 0xffff0000) >> 16) + (BitSet & 0x0000ffff);

                /*Add the element's bit count to the total bit count*/
                Count += BitSet;
        }

        return Count;

} /*CountMembers*/


/************************************************************************/
/*                                                                      */
/*      ClassCompile -- Decode class specifier and emit RE codes        */
/*                                                                      */
/************************************************************************/
module_scope BOOL
RegExp_ClassCompile(BYTE **ppText, LWORD Flags, RE_ELEMENT **ppCode)
{
        BYTE ch;
        UINT Start;
        BYTE *pText;
        BYTE *pStart;
        RE_ELEMENT *pCode;
        RE_ELEMENT CaseFlag = 0;

        /*Set up case-insensitivity flag for RE elements if specified*/
        if (Flags & REGEXP_CONFIG_IGNORE_CASE) {
                CaseFlag = REGEXP_CASE_INSENSITIVE;
        }

        /*Copy pointed-to pointers into local variables*/
        pText = *ppText;
        pCode = *ppCode;

        /*Collect characters specified in a bit set*/
        memset(pCode + 1, 0, REGEXP_BITMASK_NRCODES * sizeof(RE_ELEMENT));

        /*Start of class of characters: is first char "^"?*/
        if (*pText == ((BYTE) '^')) {
                /*Yes, match all but named characters*/
                *pCode++ = REGEXP_TYPE(REGEXP_TYPE_CLASS_ALL_BUT) | 
                        CaseFlag;
                pText++;

                /*Are we limiting "[^...]" matches to word chars?*/
                if (Flags & REGEXP_CONFIG_WORD_CONSTRAIN) {
                        /*Yes, mark all non-word chars as non-matching*/
                        ch = 0; 
                        do {
                                if (! isalnum(ch)) {
                                        REGEXP_BIT_SET(pCode, ch);
                                }
                        } while (ch++ != REGEXP_CHAR_MAX);
                        REGEXP_BIT_CLEAR(pCode, '_');

                }

        } else {
                /*Normal class*/
                *pCode++ = REGEXP_TYPE(REGEXP_TYPE_CLASS) | 
                        CaseFlag;

        }

        /*Remember RE start so we may handle [] correctly*/
        pStart = pText;

        /*Loop until end-of-set specifier found*/
        for (;;) {
                /*Does this char close the RE? (Note: [] is special)*/
                if ((*pText == ((BYTE) ']')) && 
                                (pText != pStart)) {
                        /*Yes, break out of processing loop*/
                        break;
                }

                /*Deal with the next byte of the class*/
                ch = *pText++;

                /*Has the class terminated abnormally?*/
                if (ch == ((BYTE) NUL)) {
                        /*Yes, bad RE syntax*/
                        gRegExp.FailureCode = REGEXP_FAIL_UNMATCHED_CLASS_OPEN;
                        return FALSE;
                }

                /*Do we have a "[:" sequence introducing a class name?*/
                if ((Flags & REGEXP_CONFIG_NAMED_CLASSES) && 
                    (ch == '[') && (*pText == ':')) {
                        /*Yes, handle class name (if present)*/
                        if (RegExp_ClassName(&pText, Flags, pCode)) {
                                /*Dealt with name*/
                                continue;
                        }
                }

                /*Have we found a character range?*/
                if ((pText[0] == ((BYTE) '-')) && (pText[1] != ']')) {
                        /*Yes, find start and end of the range*/
                        Start = (UINT) ch;
                        pText++;
                        ch = (BYTE) *pText++;

                        /*Is the start after the end when this is forbidden?*/
                                /*Yes, remember failure and abandon class*/
                                /*(not implemented yet)*/

                        /*Expand range now (Note: Start, ch unsigned)*/
                        for (; Start <= (UINT) ch; Start++) {
                                REGEXP_BIT_SET(pCode, Start);
                        }

                        /*Process next component of class*/
                        continue;

                }

                /*Add char to the set*/
                REGEXP_BIT_SET(pCode, ch);

        }

        /*Was this a normal class?*/
        if (REGEXP_TYPE_R(pCode[-1]) == REGEXP_TYPE_CLASS) {
                /*Yes, did the class only contain one member?*/
                switch (RegExp_CountMembers(pCode)) {
                case 1:
                        /*Yes, convert from class entry to literal*/
                        pCode[-1] = REGEXP_TYPE(REGEXP_TYPE_LITERAL) |
                                CaseFlag |
                                REGEXP_LITERAL(ch);
                        break;

                case 2:
                        /*Is the class equivalent to a case-insens. literal?*/
                        if ((Flags & REGEXP_CONFIG_CLASS_CONV_CASE) &&
                            (tolower(ch) != toupper(ch)) &&
                            REGEXP_BIT_TEST(pCode, tolower(ch)) &&
                            REGEXP_BIT_TEST(pCode, toupper(ch))) {
                                /*Yes, use literal as it's easier to handle*/
                                pCode[-1] = REGEXP_TYPE(REGEXP_TYPE_LITERAL) |
                                        REGEXP_CASE_INSENSITIVE |
                                        REGEXP_LITERAL(ch);
                                break;

                        }

                        /*Ensure LF is not present*/
                        REGEXP_BIT_CLEAR(pCode, LF);

                        /*Skip over class elements in RE*/
                        pCode += REGEXP_BITMASK_NRCODES;

                        break;

                case 0:
                        /*Yes, match can't progress (unless ? or * option)*/
                        /*We could use REGEXP_TYPE_DEAD_END here*/
                        /*FALLTHROUGH*/

                default:

                        /*Ensure LF is not present*/
                        REGEXP_BIT_CLEAR(pCode, LF);

                        /*Skip over class elements in RE*/
                        pCode += REGEXP_BITMASK_NRCODES;

                        break;

                }

        } else {
                /*All-but class: Ensure LF doesn't match*/
                REGEXP_BIT_SET(pCode, LF);

                /*Skip over class elements in RE*/
                pCode += REGEXP_BITMASK_NRCODES;

        }

        /*Write updated pointers to caller*/
        *ppCode = pCode;
        *ppText = pText + 1;

        /*Converted class to compact form successfully*/
        return TRUE;

} /*ClassCompile*/


/************************************************************************/
/*                                                                      */
/*      Compact -- Render text expression in compact form               */
/*                                                                      */
/*      This function is the first phase of handling a text regular     */
/*      expression.  It does a fairly simple and fast parse of the      */
/*      text to produce a set of codes that describe each element       */
/*      of the expression in a single item.  In particular, this        */
/*      means that multi-character specifiers such as "[A-Za-z]"        */
/*      are packaged into a convenient code.                            */
/*                                                                      */
/************************************************************************/
module_scope BOOL
RegExp_Compact(BYTE *pText, LWORD Flags, RegExp_Specification *pRE)
{
        BYTE ch;
        RE_ELEMENT *pCode = pRE->CodeList;
        RE_ELEMENT *pPrev = NULL;
        UINT NrStates = 0;
        RE_ELEMENT CaseFlag = 0;

        /*Is the RE anchored to the start of the space?*/
        if (*pText == '^') {
                /*Yes, insert anchor element and skip over char*/
                *pCode++ = REGEXP_TYPE(REGEXP_TYPE_ANCHOR_START);
                pText++;

        /*No anchor: was one selected by an option anyway?*/
        } else if (Flags & REGEXP_CONFIG_FULL_LINE_ONLY) {
                /*Yes, insert specifier so RE matches correctly*/
                *pCode++ = REGEXP_TYPE(REGEXP_TYPE_ANCHOR_START);

        /*Is match constrained to word boundaries?*/
        } else if (Flags & REGEXP_CONFIG_WORD_EDGES) {
                /*Yes, check for non-word char before match start*/
                *pCode++ = REGEXP_TYPE(REGEXP_TYPE_WORD_PREV_NONWORD);
                NrStates++;

        }

        /*Set up case-insensitivity flag for RE elements if specified*/
        if (Flags & REGEXP_CONFIG_IGNORE_CASE) {
                CaseFlag = REGEXP_CASE_INSENSITIVE;
        }

        /*Later: Entry point for groups/alternation might go here*/

        /*Work through each part of expression*/
ConversionLoop:

        /*Presume that specifier creates a match state*/
        NrStates++;

        ch = (BYTE) *pText++;
        switch ((CHAR) ch) {
        case '.':
                /*Match anything except EOL*/

                /*Are we limiting "." matches to word chars?*/
                if (Flags & REGEXP_CONFIG_WORD_CONSTRAIN) {
                        /*Yes, pretend we saw "\w" instead*/
                        ch = 'w';
                        goto AddWordClass;
                }

                /*Add match-any code to compiled version*/
                pPrev = pCode;
                *pCode++ = REGEXP_TYPE(REGEXP_TYPE_MATCH_ANY);
                break;

        case '$':
                /*Are we at the end of the pattern?*/
                if (*pText == '\0') {
                        /*Yes, Match end of line*/
                        *pCode++ = REGEXP_TYPE(REGEXP_TYPE_ANCHOR_END);
                        pPrev = NULL;
                } else {
                        /*No, treat $ as a literal (sigh)*/
                        goto Literal;
                }
                break;

        case '^':
                /*Start-of-line anchor*/
                if (pPrev == NULL) {
                        /*Already handled "real" ^ above: literal*/
                        goto Literal;
                }

                /*sudden editorial outburst:

                  The treatment of ^ in GNU grep is obscure.  The 
                  rules here are behoffski's crude guess as to what's 
                  happening.  It's unlikely to be identical, so 
                  unfortunately discrepancies may appear.  Compare 
                  "r^" = "'r' BEGLINE" and "[a-z]^" = "CSET '^'" 
                  to see what I mean.  
                */

                /*Is the preceding item a literal?*/
                if ((REGEXP_TYPE_R(*pPrev) == REGEXP_TYPE_LITERAL) && 
                    (REGEXP_REPEAT_R(*pPrev) == REGEXP_REPEAT_MATCH_SELF)) {
                        /*Yes, ^ is line beginning, but can't match*/
                        pPrev = pCode;
                        *pCode++ = REGEXP_TYPE(REGEXP_TYPE_DEAD_END);
                        break;
                }

                /*Treat character as a literal*/
                goto Literal;

                /*break;*/

        case '\0':
                /*Were we asked to limit matches to full lines?*/
                if (Flags & REGEXP_CONFIG_FULL_LINE_ONLY) {
                        /*Yes, was an end-of-line anchor given?*/
                        if (REGEXP_TYPE_R(pCode[-1]) != 
                                          REGEXP_TYPE_ANCHOR_END) {
                                /*No, add one now*/
                                *pCode++ = 
                                        REGEXP_TYPE(REGEXP_TYPE_ANCHOR_END);
                                NrStates++;
                        }

                        /*Add RE terminator*/
                        *pCode = REGEXP_TYPE(REGEXP_TYPE_END_OF_EXPR);

                        /*Are we constrained to matching word edges?*/
                } else if (Flags & REGEXP_CONFIG_WORD_EDGES) {
                        /*Yes, add endmarker with nonword restrictions*/
                        *pCode = REGEXP_TYPE(REGEXP_TYPE_END_OF_EXPR) | 
                                REGEXP_WORD(REGEXP_WORD_NONWORD_CHARS);

                } else {
                        /*No, merely mark end of RE*/
                        *pCode = REGEXP_TYPE(REGEXP_TYPE_END_OF_EXPR);
                }

                /*End of expression... write result to caller and exit*/
                pRE->NrStates = NrStates;
                return TRUE;
                break;

        case '\\':
                /*Decode character, handling escapes specially if selected*/
                ch = *pText++;
                switch (ch) {
                case '+':
                        /*Escaped plus: is this combination special?*/
                        if (! (Flags & REGEXP_CONFIG_ESC_PLUS)) {
                                /*No, treat character as a literal*/
                                goto Literal;
                        }

                        /*Was there a preceding RE?*/
                        if (pPrev == NULL) {
                                /*No, treat as literal*/
                                goto Literal;
                        }

                        /*Is the previous component a meta RE?*/
                        if (REGEXP_TYPE_IS_META(REGEXP_TYPE_R(*pPrev))) {
                                /*Yes, modifier doesn't have any effect*/
                                NrStates--;
                                break;
                        }

                        /*Modify previous element's iteration*/
                        if (REGEXP_REPEAT_R(*pPrev) != 
                                        REGEXP_REPEAT_MATCH_SELF) {
                                NrStates--;
                        }
                        *pPrev |= REGEXP_REPEAT(REGEXP_REPEAT_1_OR_MORE);
                        break;

                case '?':
                        /*Escaped question mark: is this combination special?*/
                        if (! (Flags & REGEXP_CONFIG_ESC_QUESTION_MARK)) {
                                /*No, treat character as a literal*/
                                goto Literal;
                        }

                        /*Was there a preceding RE?*/
                        if (pPrev == NULL) {
                                /*No, treat as literal*/
                                goto Literal;
                        }

                        /*Yes, make previous element optional*/
                        *pPrev |= REGEXP_REPEAT(REGEXP_REPEAT_0_OR_1);
                        break;

                case '<':
                        /*Escaped less-than: is this special?*/
                        if (! (Flags & REGEXP_CONFIG_ESC_LESS_THAN)) {
                                /*No, treat character as a literal*/
                                goto Literal;
                        }

                        /*Add meta-match to check word beginning*/
                        if (pPrev != NULL) {
                                pPrev = pCode;
                        }
                        *pCode++ = REGEXP_TYPE(REGEXP_TYPE_WORD_BEGINNING);
                        break;

                case '>':
                        /*Escaped greater-than: is this special?*/
                        if (! (Flags & REGEXP_CONFIG_ESC_LESS_THAN)) {
                                /*No, treat character as a literal*/
                                goto Literal;
                        }

                        /*Add meta-match to check word end*/
                        if (pPrev != NULL) {
                                pPrev = pCode;
                        }
                        *pCode++ = REGEXP_TYPE(REGEXP_TYPE_WORD_END);
                        break;

                case 'b':
                        /*Escaped 'b': is this special?*/
                        if (! (Flags & REGEXP_CONFIG_WORD_BOUNDARY)) {
                                /*No, treat character as a literal*/
                                goto Literal;
                        }

                        /*Add meta-match to check word boundary*/
                        if (pPrev != NULL) {
                                pPrev = pCode;
                        }
                        *pCode++ = REGEXP_TYPE(REGEXP_TYPE_WORD_BOUNDARY);
                        break;

                case 'B':
                        /*Escaped 'B': is this special?*/
                        if (! (Flags & REGEXP_CONFIG_WORD_NONBOUNDARY)) {
                                /*No, treat character as a literal*/
                                goto Literal;
                        }

                        /*Add meta-match to check word NONboundary*/
                        if (pPrev != NULL) {
                                pPrev = pCode;
                        }
                        *pCode++ = REGEXP_TYPE(REGEXP_TYPE_WORD_NONBOUNDARY);
                        break;

                case 'w':
                case 'W':
                        /*\\w equivalent to [[:alnum:]_]; \W to [^[:alnum:]_]*/
AddWordClass:
                        /*Collect characters specified in a bit set*/
                        memset(pCode + 1, 
                               0, 
                               REGEXP_BITMASK_NRCODES * sizeof(RE_ELEMENT));

                        pPrev = pCode;
                        if (ch == 'w') {
                                *pCode++ = REGEXP_TYPE(REGEXP_TYPE_CLASS);
                        } else {
                                *pCode++ = REGEXP_TYPE(
                                                REGEXP_TYPE_CLASS_ALL_BUT);

                                /*Ensure LF doesn't match (is this correct?)*/
                                REGEXP_BIT_SET(pCode, LF);
                        }
                        ch = 0; 
                        do {
                                /*Is this character a member of the class?*/
                                if (isalnum(ch)) {
                                        /*Yes, mark it in our list*/
                                        REGEXP_BIT_SET(pCode, ch);
                                }
                        } while (ch++ != REGEXP_CHAR_MAX);
                        REGEXP_BIT_SET(pCode, '_');

                        /*Skip over class elements in RE*/
                        pCode += REGEXP_BITMASK_NRCODES;

                        break;

                case NUL:
                        /*Sorry, don't support trailing backslash*/
                        fprintf(stderr, "%s: Trailing backslash\n", 
                                Platform_ProgramName());
                        exit(2);
                        /*break;*/

                default:
                        /*Backslash merely makes next character a literal*/
                        goto Literal;
                        /*break;*/

                }
                break;

        case '[':
                /*Handle class specifier*/
                pPrev = pCode;
                if (! RegExp_ClassCompile(&pText, Flags, &pCode)) {
                        /*Sorry, error in class specification*/
                        return FALSE;
                }
                break;

        case '*':
                /*Repeat 0 or more times*/

                /*Was there a preceding RE?*/
                if (pPrev == NULL) {
                        /*No, treat as literal*/
                        goto Literal;
                }

                /*Modify prev code to repeat 0 or more times*/
                *pPrev |= REGEXP_REPEAT(REGEXP_REPEAT_0_OR_MORE);
                NrStates--;
                break;

        case '+':
                /*Repeat 1 or more times*/

                /*Is + a special character?*/
                if (! (Flags & REGEXP_CONFIG_PLUS)) {
                        /*No, treat character as a literal*/
                        goto Literal;
                }

                /*Was there a preceding RE?*/
                if (pPrev == NULL) {
                        /*No, treat as literal*/
                        goto Literal;
                }

                /*Is the previous component a meta RE?*/
                if (REGEXP_TYPE_IS_META(REGEXP_TYPE_R(*pPrev))) {
                        /*Yes, modifier doen't have any effect: Ignore it*/
                        NrStates--;
                        break;
                }

                /*Modify prev code to repeat 1 or more times*/
                if (REGEXP_REPEAT_R(*pPrev) != REGEXP_REPEAT_MATCH_SELF) {
                        NrStates--;
                }
                *pPrev |= REGEXP_REPEAT(REGEXP_REPEAT_1_OR_MORE);
                break;

        case '?':
                /*Repeat 0 or 1 times*/

                /*Is '?' a special character?*/
                if (! (Flags & REGEXP_CONFIG_QUESTION_MARK)) {
                        /*No, treat character as a literal*/
                        goto Literal;
                }

                /*Was there a preceding RE?*/
                if (pPrev == NULL) {
                        /*No, treat as literal*/
                        goto Literal;
                }

                /*Modify prev code to repeat zero or one times*/
                *pPrev |= REGEXP_REPEAT(REGEXP_REPEAT_0_OR_1);
                NrStates--;
                break;

        default:
Literal:
                /*Character isn't recognised as special: handle as a literal*/
                pPrev = pCode;
                *pCode++ = REGEXP_TYPE(REGEXP_TYPE_LITERAL) | 
                           CaseFlag | 
                           REGEXP_LITERAL(ch);
                break;

        }

        goto ConversionLoop;

} /*Compact*/


/************************************************************************/
/*                                                                      */
/*      Compile -- Check and convert text expression to compact form    */
/*                                                                      */
/*      The Flags parameter contains settings modifying the RE          */
/*      such as case insensitivity, match entire line and               */
/*      match word.                                                     */
/*                                                                      */
/*      The function returns FALSE if the text form was badly           */
/*      formatted.                                                      */
/*                                                                      */
/************************************************************************/
public_scope BOOL
RegExp_Compile(CHAR *pText, LWORD Flags, 
               RegExp_Specification **ppSpec)
{
        UINT RESize;

        RegExp_Specification *pRE = NIL;

        /*Destroy return value to reduce chances of being misunderstood*/
        *ppSpec = NIL;

        /*Find worst-case code size (smallest class spec is 4 chars)*/
        RESize = ((strlen(pText) / 4) + 1) * (REGEXP_BITMASK_NRCODES + 1) +
                REGEXP_CODE_OVERHEAD;

        /*Allocate a specifier/full structure block*/
        pRE = (RegExp_Specification *) 
                        Platform_SmallMalloc(RESize * sizeof(RE_ELEMENT));
        if (pRE == NULL) {
                /*Unable to obtain memory for compilation*/
                gRegExp.FailureCode = REGEXP_FAIL_NOT_ENOUGH_MEMORY;
                return FALSE;
        }

        /*Work through string, converting each portion to compact form*/
        if (! RegExp_Compact((BYTE *) pText, Flags, pRE)) {
                /*Errors in RE syntax... discard buffer and report failure*/
                Platform_SmallFree(pRE);
                return FALSE;
        }

        /*No failures associated with this RE*/
        pRE->FailureCode = REGEXP_FAIL_NO_FAILURE;

        /*Write handle to caller and report success*/
        *ppSpec = pRE;
        return TRUE;

} /*Compile*/


/************************************************************************/
/*                                                                      */
/*      CompileString -- Convert string without looking for RE syntax   */
/*                                                                      */
/*      The Flags parameter contains settings modifying the RE          */
/*      such as case insensitivity, match entire line and               */
/*      match word.                                                     */
/*                                                                      */
/*      The function returns FALSE if the conversion failed.            */
/*                                                                      */
/************************************************************************/
public_scope BOOL
RegExp_CompileString(CHAR *pText, LWORD Flags, 
                     RegExp_Specification **ppSpec)
{
        UINT RESize;
        RegExp_Specification *pRE = NIL;
        RE_ELEMENT *pCode;
        RE_ELEMENT Code;

        /*Destroy return value to reduce chances of being misunderstood*/
        *ppSpec = NIL;

        /*Estimate storage needed to store compiled RE*/
        RESize = strlen(pText) + REGEXP_CODE_OVERHEAD;

        /*Allocate a specifier/full structure block*/
        pRE = (RegExp_Specification *) 
                Platform_SmallMalloc(RESize * sizeof(RE_ELEMENT));
        if (pRE == NULL) {
                /*Unable to obtain memory for compilation*/
                gRegExp.FailureCode = REGEXP_FAIL_NOT_ENOUGH_MEMORY;
                return FALSE;
        }
        pCode = pRE->CodeList;

        /*Is match required to span entire line?*/
        if (Flags & REGEXP_CONFIG_FULL_LINE_ONLY) {
                /*Yes, insert start anchor so RE matches correctly*/
                *pCode++ = REGEXP_TYPE(REGEXP_TYPE_ANCHOR_START);
                
        /*No, but is match constrained to word boundaries?*/
        } else if (Flags & REGEXP_CONFIG_WORD_EDGES) {
                /*Yes, check for non-word char before match start*/
                *pCode++ = REGEXP_TYPE(REGEXP_TYPE_WORD_PREV_NONWORD);

        }

        /*Prepare code describing each element of the string*/
        Code = REGEXP_TYPE(REGEXP_TYPE_LITERAL);
        if (Flags & REGEXP_CONFIG_IGNORE_CASE) {
                Code |= REGEXP_CASE_INSENSITIVE;
        }

        /*Convert each character to code*/
        for (; *pText != NUL; pText++) {
                /*Convert this character*/
                *pCode++ = Code | REGEXP_LITERAL((BYTE) *pText);

        }

        /*Is match required to span entire line?*/
        if (Flags & REGEXP_CONFIG_FULL_LINE_ONLY) {
                /*Yes, append end anchor so RE matches correctly*/
                *pCode++ = REGEXP_TYPE(REGEXP_TYPE_ANCHOR_END);
                *pCode = REGEXP_TYPE(REGEXP_TYPE_END_OF_EXPR);
                
        /*No, but are we constrained to matching word edges?*/
        } else if (Flags & REGEXP_CONFIG_WORD_EDGES) {
                /*Yes, add endmarker with nonword restrictions*/
                *pCode = REGEXP_TYPE(REGEXP_TYPE_END_OF_EXPR) | 
                        REGEXP_WORD(REGEXP_WORD_NONWORD_CHARS);

        } else {
                /*No, merely mark end of RE*/
                *pCode = REGEXP_TYPE(REGEXP_TYPE_END_OF_EXPR);
        }

        /*Add end-of-list descriptor*/
        pRE->NrStates = (UINT) (pCode - pRE->CodeList) + 1;
        pRE->FailureCode = REGEXP_FAIL_NO_FAILURE;

        /*Write handle to caller and report success*/
        *ppSpec = pRE;
        return TRUE;

} /*CompileString*/


/************************************************************************/
/*                                                                      */
/*      CountStates -- Work out how many states are in RE               */
/*                                                                      */
/*      The number of states in a regular expression is a very          */
/*      useful piece of information.  Most importantly, it allows       */
/*      the table-driven expansion of the RE to use the least memory    */
/*      possible.  However, some RE operations such as the split        */
/*      in EasiestFirst modify this value.  Rather than include         */
/*      counting states in each operation, this function surveys        */
/*      the RE and builds the correct count of states.                  */
/*                                                                      */
/************************************************************************/
module_scope void
RegExp_CountStates(RegExp_Specification *pRE)
{
        RE_ELEMENT *pCode = pRE->CodeList;
        RE_ELEMENT Elem;
        UINT NrStates = 0;

        /*Loop through spec and interpret each code*/
        for (;;) {
                Elem = *pCode++;
                switch (REGEXP_TYPE_R(Elem)) {
                case REGEXP_TYPE_END_OF_EXPR:
                        /*Code terminating RE found: write states and exit*/
                        pRE->NrStates = NrStates + 1;
                        return;
                        break;

                case REGEXP_TYPE_CLASS:
                case REGEXP_TYPE_CLASS_ALL_BUT:
                        /*Next REGEXP_BITMASK_NRCODES codes detail members*/
                        pCode += REGEXP_BITMASK_NRCODES;
                        /*FALLTHROUGH*/

                case REGEXP_TYPE_LITERAL:
                case REGEXP_TYPE_MATCH_ANY:
                        /*Most codes are counted as states*/
                        NrStates++;

                        /*Add a state if "+" iteration applied*/
                        switch(REGEXP_REPEAT_R(Elem)) {
                        case REGEXP_REPEAT_1_OR_MORE:
                                NrStates++;
                                break;

                        default:
                                break;

                        }

                        break;

                case REGEXP_TYPE_WORD_BEGINNING:
                case REGEXP_TYPE_WORD_END:
                case REGEXP_TYPE_WORD_BOUNDARY:
                case REGEXP_TYPE_WORD_NONBOUNDARY:
                case REGEXP_TYPE_WORD_PREV_NONWORD:
                case REGEXP_TYPE_DEAD_END:
                        /*Takes 1 state to evaluate text*/
                        NrStates++;
                        break;

                case REGEXP_TYPE_ANCHOR_START:
                case REGEXP_TYPE_ANCHOR_END:
                        /*These do not warrant a state of their own*/
                        break;

                default:
                        printf("RegExp_CountStates: Unknown: %u", 
                               REGEXP_TYPE_R(pCode[-1]));
                        break;

                }

        }

} /*CountStates*/


/************************************************************************/
/*                                                                      */
/*      ExtractRE -- Copy specified RE components into new spec         */
/*                                                                      */
/*      This function creates a new regular expression containing       */
/*      a string of RE elements copied from the specified RE.           */
/*      It is used by EasiestFirst to create a RE containing the        */
/*      cheapest (hopefully) portion of the RE to find first.           */
/*      The function returns FALSE if unable to perform the             */
/*      extraction as requested.                                        */
/*                                                                      */
/*      If the last element in the list has '+' iteration, the          */
/*      copied element has the iteration removed.                       */
/*                                                                      */
/************************************************************************/
module_scope BOOL
RegExp_ExtractRE(RegExp_Specification *pSpec, 
                 RE_ELEMENT *pStartCode, UINT NrElements, 
                 RegExp_Specification **ppExtractedRE)
{
        RegExp_Specification *pRE2;
        RE_ELEMENT *pCode;
        RE_ELEMENT *pLastCode;
        RE_ELEMENT Code;
        UINT NrStates;
        UINT NrCodes;

        /*Destroy return value to reduce possibility of confusion*/
        *ppExtractedRE = (RegExp_Specification *) NIL;

        /*Is the number of elements unreasonable?*/
        if ((NrElements == 0) || (NrElements == UINT_MAX)) {
                /*Yes, reject request*/
                pSpec->FailureCode = REGEXP_FAIL_REQUEST_UNREASONABLE;
                return FALSE;
        }

        /*Find out how many RE words are used to describe target area*/
        NrStates = NrElements + 1;
        pCode = pStartCode;
        pLastCode = pCode;
        while (NrElements-- > 0) {
                /*Does this element have '+' iteration?*/
                pLastCode = pCode;
                Code = *pCode++;
                if (REGEXP_REPEAT_R(Code) == REGEXP_REPEAT_1_OR_MORE) {
                        /*Yes, remember that we need an extra state*/
                        NrStates++;
                }

                /*Skip over code plus any associated data*/
                switch (REGEXP_TYPE_R(Code)) {
                case REGEXP_TYPE_END_OF_EXPR:
                        /*Fail: Counter should have stopped us earlier*/
                        return FALSE;
                        /*break;*/

                case REGEXP_TYPE_CLASS:
                case REGEXP_TYPE_CLASS_ALL_BUT:
                        /*Next REGEXP_BITMASK_NRCODES codes detail members*/
                        pCode += REGEXP_BITMASK_NRCODES;
                        break;
                        
                }

        }


        /*Get buffer to store easier part of RE*/
        NrCodes = (pCode - pStartCode) + 1;

        pRE2 = (RegExp_Specification *) malloc(sizeof(RegExp_Specification) +
                (NrCodes) * sizeof(RE_ELEMENT));
        if (pRE2 == NULL) {
                /*Unable to obtain memory to perform extraction*/
                pSpec->FailureCode = REGEXP_FAIL_NOT_ENOUGH_MEMORY;
                return FALSE;
        }
        pRE2->FailureCode = REGEXP_FAIL_NO_FAILURE;

        /*Copy codes from RE and add an endmarker*/
        memcpy(&pRE2->CodeList[0], 
               pStartCode, 
               (NrCodes - 1) * sizeof(*pStartCode));
        pRE2->CodeList[NrCodes - 1] = REGEXP_TYPE(REGEXP_TYPE_END_OF_EXPR);

        /*Did the last code specify '+' iteration?*/
        if (REGEXP_REPEAT_R(*pLastCode) == REGEXP_REPEAT_1_OR_MORE) {
                /*Yes, edit copy to remove iteration*/
                pCode = &pRE2->CodeList[pLastCode - pStartCode];
                *pCode &= ~REGEXP_REPEAT_MASK;
                *pCode |= REGEXP_REPEAT_MATCH_SELF;
                NrStates--;
        }


        pRE2->NrStates = NrStates;

        /*Write extracted RE to caller and report success*/
        *ppExtractedRE = pRE2;
        return TRUE;

} /*ExtractRE*/


/************************************************************************/
/*                                                                      */
/*      Frequency -- Estimate how frequently we might match string      */
/*                                                                      */
/*      This function inspects the supplied string of pattern           */
/*      elements and estimates how often the pattern would appear       */
/*      in "typical" files.  While this estimate is amazingly crude     */
/*      it is in fact worth while, since patterns like "e.*q"           */
/*      benefit from searching for "q" first instead of "e".            */
/*                                                                      */
/*      This function assumes that the pattern elements presented       */
/*      are simple, such as literals and classes.                       */
/*                                                                      */
/*      This function used to be called Score, and returned a UINT16    */
/*      with the interpretation that higher was better (the higher      */
/*      the score, the less likely we were to match the string).        */
/*      However, the routine is now called Frequency, returns UINT32    */
/*      and the meanings are reversed: the lower the frequency,         */
/*      the less likely we believe we will match the string).  The      */
/*      reworked routine also anticipates the use of the Boyer-Moore    */
/*      algorithm and increases the input from the last element.        */
/*                                                                      */
/*      In some cases we might be better off shortening the string      */
/*      if the last element is really likely, but this important        */
/*      improvement is not included here, mainly because we're          */
/*      not confident that our scoring truly matches the input.         */
/*                                                                      */
/************************************************************************/
module_scope UINT32
RegExp_Frequency(RE_ELEMENT *pCode, UINT NrCodes)
{
        UINT32 TotalFreq = 0;
        UINT32 CodeFreq = 0;
        UINT c;
        UINT CodeNr;

        /*Loop through each code, estimating how often the code appears*/
        for (CodeNr = 0; CodeNr < NrCodes; CodeNr++) {
                switch (REGEXP_TYPE_R(*pCode++)) {
                case REGEXP_TYPE_CLASS:
                        /*Examine class members to estimate likelihood*/
                        CodeFreq = 0;
                        for (c = 0; c < 255; c++) {
                                /*Skip character if not member of class*/
                                if (! REGEXP_BIT_TEST(pCode, c)) {
                                        continue;
                                }
                                CodeFreq += RegExp_ByteFrequency[c];
                        }

                        /*Skip past bitset describing class members*/
                        pCode += REGEXP_BITMASK_NRCODES;
                        TotalFreq += CodeFreq;

                        break;

                case REGEXP_TYPE_LITERAL:
                        /*Code represents literal*/
                        c = REGEXP_LITERAL_R(pCode[-1]);
                        CodeFreq = 1;
                        if (c < DIM(RegExp_ByteFrequency)) {
                                CodeFreq = RegExp_ByteFrequency[c];
                        }
                        TotalFreq += CodeFreq;
                        break;

                }

        }

        /*Likelihood of last code affects Boyer-Moore*/
        /*So we increase its relative importance to the score*/
        TotalFreq = (TotalFreq + CodeFreq * 3) / (NrCodes + 3);

        return TotalFreq;

} /*Frequency*/


/************************************************************************/
/*                                                                      */
/*      EasiestFirst -- Improve search by deferring complex starts      */
/*                                                                      */
/*      An expression like "[^a]*.*[^b]*.*c.*Quasimodo's Dream.*g"      */
/*      is very expensive to search if you simply use the expression    */
/*      as given, as the first elements involve lots of backtracking.   */
/*      However, most regular expressions like this have a              */
/*      relatively easy middle bit (e.g. "Quasimodo's Dream") that      */
/*      is required for the match and can be searched first instead.    */
/*                                                                      */
/*      If the search can be optimised by the split, EasiestFirst       */
/*      copies the easy bit into a separate RE specification,           */
/*      leaving the existing RE untouched.  The extracted RE is         */
/*      written back to the caller, and the function returns TRUE.      */
/*      If no worthwhile optimisation can be found, the function        */
/*      returns FALSE.                                                  */
/*                                                                      */
/*      Leaving the original untouched is a change from the             */
/*      previous version: we no longer break the RE into two            */
/*      pieces: this is because:                                        */
/*           (a) In a few cases we match incorrectly, e.g. word         */
/*               boundary test at the split boundary;                   */
/*           (b) Writing into the file buffer (to add the LF            */
/*               marker to complete the match) degrades                 */
/*               performance where the file is mmap'd as the            */
/*               operating system generates page faults due to          */
/*               copy-on-write markers on the memory pages;             */
/*           (c) We can optimise the easier bit more aggressively       */
/*               if we aren't burdened with more complex RE             */
/*               components appended to the easy bit.                   */
/*                                                                      */
/*      A rather crude scoring system is used as a tie-breaker          */
/*      where multiple longest strings are the same length.             */
/*      This tie-breaker attempts to select the string containing       */
/*      characters less likely to occur in "typical" text files.        */
/*                                                                      */
/************************************************************************/
public_scope BOOL
RegExp_EasiestFirst(RegExp_Specification *pRE, 
                    RegExp_Specification **ppEasierBit)
{
        RE_ELEMENT *pCode;
        RE_ELEMENT *pEasyCode = pRE->CodeList;
        UINT NrEasyCodes = 0;
        RE_ELEMENT *pEasiestCode = pRE->CodeList;
        RE_ELEMENT *pFirstIter = NULL;
        UINT Longest = 0;
        BOOL Tricky = TRUE;
        BOOL Scanning = TRUE;
        BOOL AnchorStart = FALSE;
        BOOL AnchorEnd = FALSE;
        RegExp_Specification *pRE2;

        /*Destroy return value to reduce chances of being misinterpreted*/
        *ppEasierBit = (RegExp_Specification *) NIL;

        /*Step through RE, looking for literals*/
        pCode = pRE->CodeList;
        do {
                /*Decode the next RE element*/
                switch (REGEXP_TYPE_R(*pCode++)) {
                case REGEXP_TYPE_CLASS_ALL_BUT:
                        /*Next REGEXP_BITMASK_NRCODES codes give membership*/
                        pCode += REGEXP_BITMASK_NRCODES;
                        Tricky = TRUE;
                        break;

                case REGEXP_TYPE_ANCHOR_START:
                        /*Start anchor can perform poorly sometimes*/
                        AnchorStart = TRUE;
                        Tricky = TRUE;
                        break;

                case REGEXP_TYPE_ANCHOR_END:
                        /*Skip forward to the end of the expression*/
                        AnchorEnd = TRUE;
                        while (REGEXP_TYPE_R(*pCode++) != 
                                                 REGEXP_TYPE_END_OF_EXPR) {
                        }
                        Tricky = TRUE;
                        Scanning = FALSE;
                        break;

                case REGEXP_TYPE_WORD_BEGINNING:
                case REGEXP_TYPE_WORD_END:
                case REGEXP_TYPE_WORD_BOUNDARY:
                case REGEXP_TYPE_WORD_NONBOUNDARY:
                case REGEXP_TYPE_WORD_PREV_NONWORD:
                        /*Test will slow things down*/
                        Tricky = TRUE;
                        break;

                case REGEXP_TYPE_END_OF_EXPR:
                        /*Terminate any easy bit and prepare to exit*/
                        Tricky = TRUE;
                        Scanning = FALSE;
                        break;

                case REGEXP_TYPE_MATCH_ANY:
                        /*Not easy to optimise*/
                        Tricky = TRUE;

                        /*Check whether code is modified by repeat element*/
                        switch (REGEXP_REPEAT_R(pCode[-1])) {
                        case REGEXP_REPEAT_MATCH_SELF:
                        case REGEXP_REPEAT_1_OR_MORE:
                        case REGEXP_REPEAT_0_OR_MORE:
                                /*Remember element if first iteration*/
                                if (pFirstIter == NULL) {
                                        pFirstIter = pCode - 1;
                                }
                                break;

                        /*default:*/
                                /*break;*/
                        }
                        break;

                case REGEXP_TYPE_CLASS:
                case REGEXP_TYPE_LITERAL:
                        /*Literal or (hopefully easy) class*/

                        /*Check whether code is modified by repeat element*/
                        switch (REGEXP_REPEAT_R(pCode[-1])) {
                        case REGEXP_REPEAT_MATCH_SELF:
                                /*Unmodified code*/
                                Tricky = FALSE;
                                NrEasyCodes++;
                                if (NrEasyCodes == 1) {
                                        /*Remember start in case it's longest*/
                                        pEasyCode = pCode - 1;
                                }
                                break;

                        case REGEXP_REPEAT_0_OR_1:
                                Tricky = TRUE;
                                break;

                        case REGEXP_REPEAT_1_OR_MORE:
                                /*Initial match attempt may be optimised*/
                                NrEasyCodes++;
                                if (NrEasyCodes == 1) {
                                        /*Remember start in case it's longest*/
                                        pEasyCode = pCode - 1;
                                }

                                /*FALLTHROUGH*/

                        case REGEXP_REPEAT_0_OR_MORE:
                                /*Tricky code; also check if first iteration*/
                                Tricky = TRUE;
                                if (pFirstIter == NULL) {
                                        pFirstIter = pCode - 1;
                                }
                                break;

                        default:
                                printf(
                       "?? RegExp_EasiestFirst: Unrecognised repeat: %u\n", 
                                       REGEXP_REPEAT_R(pCode[-1]));
                                
                        }

                        if (REGEXP_TYPE_R(pCode[-1]) == REGEXP_TYPE_CLASS) {
                                /*Skip class membership bitmask*/
                                pCode += REGEXP_BITMASK_NRCODES;
                        }

                        break;

                case REGEXP_TYPE_DEAD_END:
                        /*Is this code non-optional?*/
                        switch (REGEXP_REPEAT_R(pCode[-1])) {
                        case REGEXP_REPEAT_MATCH_SELF:
                        case REGEXP_REPEAT_1_OR_MORE:
                                /*Yes, RE cannot match!  This is very easy!*/
                                if (! RegExp_ExtractRE(pRE, 
                                                       &pCode[-1], 
                                                       1, 
                                                       &pRE2)) {
                                        /*Sorry, unable to create extraction*/
                                        return FALSE;
                                }
                                *ppEasierBit = pRE2;
                                return TRUE;

                        default:
                                /*Dead end includes zero-trip case*/
                                Tricky = TRUE;
                                break;
                        }
                        break;

                default:
                        printf("?? RegExp_EasiestFirst: Unknown type: %u\n", 
                               REGEXP_TYPE_R(pCode[-1]));
                        break;

                }

                /*Did we see an anchor at the end of the RE?*/
                if (AnchorEnd) {
                        /*Yes, use last easy bit to reduce backtracking*/
                        pEasiestCode = pEasyCode;
                        break;
                }

                /*Did we find the end of some easy codes?*/
                if ((! Tricky) || (NrEasyCodes == 0)) {
                        /*No, continue search*/
                        continue;
                }

                /*Yes, is the run shorter than the best previous?*/
                if (NrEasyCodes < Longest) {
                        /*Yes, ignore it*/
                        NrEasyCodes = 0;
                        continue;

                }

                /*Is this string the same length as the best so far?*/
                if ((Longest != 0) && (NrEasyCodes == Longest)) {
                        /*Yes, does new string seem more likely to appear?*/
                        if (RegExp_Frequency(pEasyCode, NrEasyCodes) >
                                 RegExp_Frequency(pEasiestCode, Longest)) {
                                /*Yes, not worth changing selection*/
                                NrEasyCodes = 0;
                                continue;
                        }

                }


                /*String just found seems to be the easiest to search*/
                pEasiestCode = pEasyCode;
                Longest = NrEasyCodes;
                NrEasyCodes = 0;

        } while (Scanning);

        /*Code exploits knowledge that pCode points to end of RE (+ 1)*/

        /*Was our easiest part at the start of the RE anyway?*/
        if (pEasiestCode == pRE->CodeList) {
                /*Yes, don't muck around with RE*/
                pRE->FailureCode = REGEXP_FAIL_RE_IS_OPTIMAL;
                return FALSE;
        }

        /*Does the easy bit merely skip the test for -w?*/
        if ((pEasiestCode == &pRE->CodeList[1]) && 
            (REGEXP_TYPE_R(pRE->CodeList[0]) == 
             REGEXP_TYPE_WORD_PREV_NONWORD)) {
                /*Yes, not worth splitting*/
                pRE->FailureCode = REGEXP_FAIL_RE_IS_OPTIMAL;
                return FALSE;
        }

        /*Is the search anchored to the start of the line?*/
        if (AnchorStart) {
                /*Yes, state tables do a good job of this already*/
                pRE->FailureCode = REGEXP_FAIL_RE_IS_OPTIMAL;
                return FALSE;
        }

        /*Note: The definition of "good" used above isn't optimal*/

        /*Create a new RE containing easier bit*/
        if (! RegExp_ExtractRE(pRE, pEasiestCode, Longest, &pRE2)) {
                /*Sorry, unable to perform extraction (memory problem?)*/
                return FALSE;
        }

        /*Write easier spec back to caller*/
        *ppEasierBit = pRE2;

        /*Tell caller that optimisation has been performed*/
        return TRUE;

} /*EasiestFirst*/


/************************************************************************/
/*                                                                      */
/*      FindOptional -- Find sequence of optional elements in RE        */
/*                                                                      */
/*      This function assists SlashEnds by locating a string of         */
/*      optional elements, if present, in the RE.  The function         */
/*      starts from a nominated position in the RE, and reports         */
/*      the start and code after the optional bit, so that the          */
/*      function can work through all the optional components of        */
/*      the RE in order.                                                */
/*                                                                      */
/*      pStart names where to start the search.  ppFirstCode            */
/*      receives the start of the optional bit, if found.               */
/*                                                                      */
/*      The function returns TRUE if an optional bit was found.         */
/*      First/next pointers are preserved if no optional bit found.     */
/*                                                                      */
/*      Groups of elements including zero-trip options are              */
/*      reported as a unit.  Each ONE_OR_MORE element is also           */
/*      reported separately.                                            */
/*                                                                      */
/************************************************************************/
module_scope BOOL
RegExp_FindOptional(RE_ELEMENT *pStart,
                    RE_ELEMENT **ppFirstCode, 
                    RE_ELEMENT **ppNextCode)
{
        RE_ELEMENT *pCode = pStart;
        RE_ELEMENT *pPrev = pCode;

        /*Find element containing next *, ? or + modifier (if any)*/
        for (;;) {
                /*Remember prev code for when we find code/modifier pair*/
                pPrev = pCode;

                /*Analyse next component of RE*/
                switch (REGEXP_TYPE_R(*pCode++)) {
                case REGEXP_TYPE_END_OF_EXPR:
                        /*Didn't find any optional elements*/
                        return FALSE;
                        /*break;*/

                case REGEXP_TYPE_CLASS:
                case REGEXP_TYPE_CLASS_ALL_BUT:
                        /*Skip over codes describing set membership*/
                        pCode += REGEXP_BITMASK_NRCODES;

                        /*Found element matching character*/
                        break;

                case REGEXP_TYPE_LITERAL:
                case REGEXP_TYPE_MATCH_ANY:
                        /*Found element matching character*/
                        break;

                case REGEXP_TYPE_DEAD_END:
                        /*See if this component is optional*/
                        break;

                case REGEXP_TYPE_ANCHOR_START:
                case REGEXP_TYPE_ANCHOR_END:
                case REGEXP_TYPE_WORD_BEGINNING:
                case REGEXP_TYPE_WORD_END:
                case REGEXP_TYPE_WORD_BOUNDARY:
                case REGEXP_TYPE_WORD_NONBOUNDARY:
                case REGEXP_TYPE_WORD_PREV_NONWORD:
                        /*Ignore meta-specifiers*/
                        continue;

                default:
                        printf("?? RegExp_FindOptional: Unknown type: %u\n", 
                               REGEXP_TYPE_R(pCode[-1]));
                        continue;

                }

                /*See if element has modifier that makes it optional*/
                switch (REGEXP_REPEAT_R(*pPrev)) {
                case REGEXP_REPEAT_MATCH_SELF:
                        break;

                case REGEXP_REPEAT_1_OR_MORE:
                        /*Found partially-optional element: report to client*/
                        *ppFirstCode = pPrev;
                        *ppNextCode  = pCode;
                        return TRUE;
                        /*break;*/

                case REGEXP_REPEAT_0_OR_MORE:
                case REGEXP_REPEAT_0_OR_1:
                        goto FoundOptional;
                        /*break;*/

                default:
                        printf(
                       "RegExp_FindOptional: Unrecognised repeat: %u\n", 
                               REGEXP_REPEAT_R(*pPrev));
                        break;
                        
                }

                /*Found matching element, but not optional: continue search*/
                continue;

        }

FoundOptional:

        /*Extend string of fully-optional elements as far as possible*/
        *ppFirstCode = pPrev;
        for (;;) {

                /*Decode next element of RE*/
                pPrev = pCode;
                switch (REGEXP_TYPE_R(*pCode++)) {
                case REGEXP_TYPE_END_OF_EXPR:
                case REGEXP_TYPE_ANCHOR_END:
                        /*String of codes finishes with previous element*/
                        *ppNextCode = pPrev;
                        return TRUE;
                        /*break;*/

                case REGEXP_TYPE_CLASS:
                case REGEXP_TYPE_CLASS_ALL_BUT:
                        /*Skip over codes describing set*/
                        pCode += REGEXP_BITMASK_NRCODES;
                        break;

                case REGEXP_TYPE_LITERAL:
                case REGEXP_TYPE_MATCH_ANY:
                case REGEXP_TYPE_WORD_BEGINNING:
                case REGEXP_TYPE_WORD_END:
                case REGEXP_TYPE_WORD_BOUNDARY:
                case REGEXP_TYPE_WORD_NONBOUNDARY:
                case REGEXP_TYPE_WORD_PREV_NONWORD:
                        break;

                default:
                        /*Ignore meta-specifiers (?)*/
                        continue;
                        /*break;*/

                }

                /*Look for modifier to continue fully-optional string*/
                switch (REGEXP_REPEAT_R(*pPrev)) {
                case REGEXP_REPEAT_0_OR_MORE:
                case REGEXP_REPEAT_0_OR_1:
                        /*Optional element... continue extending string*/
                        break;

                case REGEXP_REPEAT_1_OR_MORE:
                case REGEXP_REPEAT_MATCH_SELF:
                        /*Finished fully-optional elements*/
                        *ppNextCode = pPrev;
                        return TRUE;
                        /*break;*/

                default:
                        printf(
                        "RegExp_FindOptional: Unrecognised repeat: %u\n", 
                               REGEXP_REPEAT_R(*pPrev));
                        continue;

                }

        }

        /*Control should not reach here*/
        return FALSE;

} /*FindOptional*/


/************************************************************************/
/*                                                                      */
/*      FindEnd -- Find end of RE                                       */
/*                                                                      */
/*      This function finds and reports the position of the end         */
/*      of the regular expression.                                      */
/*                                                                      */
/************************************************************************/
module_scope void
RegExp_FindEnd(RegExp_Specification *pRE, RE_ELEMENT **ppEnd)
{
        RE_ELEMENT *pCode = pRE->CodeList;

        /*Step through codes until end found*/
        for (;;) {
                /*Decode code*/
                switch (REGEXP_TYPE_R(*pCode++)) {
                case REGEXP_TYPE_END_OF_EXPR:
                        /*Found the end*/
                        *ppEnd = pCode - 1;
                        return;
                        /*break*/

                case REGEXP_TYPE_CLASS:
                case REGEXP_TYPE_CLASS_ALL_BUT:
                        pCode += REGEXP_BITMASK_NRCODES;
                        break;

                }

        }

} /*FindEnd*/


/************************************************************************/
/*                                                                      */
/*      RemoveCodes -- Remove optional codes from start or end          */
/*                                                                      */
/*      This function removes optional components at the start or       */
/*      the end of the RE.                                              */
/*                                                                      */
/************************************************************************/
module_scope void
RegExp_RemoveCodes(RegExp_Specification *pRE, 
                   RE_ELEMENT *pStart, RE_ELEMENT *pNext)
{
        RE_ELEMENT *pEnd;

        /*Find end of RE*/
        RegExp_FindEnd(pRE, &pEnd);

        /*Move elements up*/
        memmove(pStart, pNext, (pEnd-pNext + 1) * sizeof(RE_ELEMENT));

        /*Count how many states are in reduced RE*/
        RegExp_CountStates(pRE);

} /*RemoveCodes*/


/************************************************************************/
/*                                                                      */
/*      SlashEnds -- Remove start/end elements for inexact matches      */
/*                                                                      */
/*      Where the client does not care to know the exact string         */
/*      that matched a regular expression, iterative or optional        */
/*      elements at the start or end of a regular exression can         */
/*      be eliminated, reducing the backtracking required to            */
/*      perform the search.  For example:                               */
/*              a*.*b*.+Quasimodo's Dream[^a]+                          */
/*      can be reduced to:                                              */
/*              .Quasimodo's Dream[^a]                                  */
/*                                                                      */
/*      This function finds cases like this and edits the RE to         */
/*      remove unnecessary elements.  The function returns TRUE         */
/*      if optimisations were found.  Remember that the resulting       */
/*      match correctly selects lines but is otherwise inexact.         */
/*                                                                      */
/*      The optimisation consists of editing the RE for each of         */
/*      four cases, in turn:                                            */
/*              1. Remove optional elements at the front.               */
/*                 e.g. "a*b?c+defgh+i*j?" becomes "c+defgh+i*j?"       */
/*              2. Edit any leading ONE_OR_MORE element to              */
/*                 remove the iteration.                                */
/*                 e.g. "c+defgh+i*j*" becomes "cdefgh+i*j?"            */
/*              3. Remove optional elements from the back.              */
/*                 e.g. "cdefgh+i*j?" becomes "cdefgh+"                 */
/*              4. Edit any trailing ONE_OR_MORE element to             */
/*                 remove the iteration.                                */
/*                 e.g. "cdefgh+" becomes "cdefgh"                      */
/*                                                                      */
/*      I chose "slash" because I used to play lacrosse!                */
/*                                                                      */
/************************************************************************/
public_scope BOOL
RegExp_SlashEnds(RegExp_Specification *pRE)
{
        RE_ELEMENT *pStart = pRE->CodeList;
        RE_ELEMENT *pNext = NULL;
        RE_ELEMENT *pLast1OrMore = NULL;
        RE_ELEMENT *pLast1Next   = NULL;
        BOOL Trimmed = FALSE;

        /*Find first optional section of RE (if any)*/
        if (! RegExp_FindOptional(pRE->CodeList, &pStart, &pNext)) {
                /*Did not find any optional components*/
                return FALSE;
        }

        /*Did we find a fully-optional string at the start of the RE?*/
        if ((pStart == pRE->CodeList) && 
            (REGEXP_REPEAT_R(*pStart) != REGEXP_REPEAT_1_OR_MORE)) {
                /*Yes, was entire RE optional?*/
                if (REGEXP_TYPE_R(*pNext) == REGEXP_TYPE_END_OF_EXPR) {
                        /*Yes, merely match all lines using "^" pattern*/
                        pStart[0] = REGEXP_TYPE(REGEXP_TYPE_ANCHOR_START);
                        pStart[1] = REGEXP_TYPE(REGEXP_TYPE_END_OF_EXPR);
                        RegExp_CountStates(pRE);
                        return TRUE;
                }

                /*Remove redundant codes at start*/
                RegExp_RemoveCodes(pRE, pStart, pNext);
                Trimmed = TRUE;

                /*Look for next optional section of RE, if any*/
                if (! RegExp_FindOptional(pStart, &pStart, &pNext)) {
                        /*No more optional components found*/
                        return TRUE;
                }

        }

        /*Did we find a ONE_OR_MORE element at the start?*/
        if ((pStart == pRE->CodeList) && 
            (REGEXP_REPEAT_R(*pStart) == REGEXP_REPEAT_1_OR_MORE)) {
                /*Yes, edit first element to remove iteration*/
                *pStart &= ~REGEXP_REPEAT_MASK;
                *pStart |= REGEXP_REPEAT(REGEXP_REPEAT_MATCH_SELF);
                Trimmed = TRUE;

                /*Look for next optional section of RE, if any*/
                if (! RegExp_FindOptional(pNext, &pStart, &pNext)) {
                        /*No more optional components found*/
                        return TRUE;
                }

        }

        /*Look for the last string of optional elements of RE*/
        for (;;) {

                /*Does this item start with a ONE_OR_MORE modifier?*/
                if (REGEXP_REPEAT_R(*pStart) == REGEXP_REPEAT_1_OR_MORE) {
                        /*Yes, remember last ONE_OR_MORE item seen*/
                        pLast1OrMore = pStart;
                        pLast1Next   = pNext;
                }

                /*Find next optional section, if any*/
                if (! RegExp_FindOptional(pNext, &pStart, &pNext)) {
                        /*No more optional components found*/
                        break;
                }

        }

        /*Did we find a string of options at the end of the RE?*/
        if (REGEXP_TYPE_R(*pNext) != REGEXP_TYPE_END_OF_EXPR) {
                /*No, cannot trim any options off of the end*/
                return Trimmed;
        }

        /*Does the end-of-expr marker indicate word restrictions?*/
        if (REGEXP_WORD_R(*pNext) != REGEXP_WORD_UNRESTRICTED) {
                /*Yes, cannot trim from the end*/
                return Trimmed;
        }

        /*Did the last optional section use ONE_OR_MORE modification?*/
        if (REGEXP_REPEAT_R(*pStart) == REGEXP_REPEAT_1_OR_MORE) {
                /*Yes, remove 1ormore modifier*/
                *pStart &= ~ REGEXP_REPEAT_MASK;
                *pStart |= REGEXP_REPEAT(REGEXP_REPEAT_MATCH_SELF);
                Trimmed = TRUE;

        } else {
                /*Fully optional last section: remove it from RE*/
                RegExp_RemoveCodes(pRE, pStart, pNext);
                Trimmed = TRUE;

                /*Was there a 1ormore iteration just before deleted section?*/
                if (pStart == pLast1Next) {
                        /*Yes, convert preceding 1ormore into simple match*/
                        *pLast1OrMore &= ~REGEXP_REPEAT_MASK;
                        *pLast1OrMore |= 
                                REGEXP_REPEAT(REGEXP_REPEAT_MATCH_SELF);
                }

        }

        /*Report whether expression has been optimised*/
        return Trimmed;

} /*SlashEnds*/


/************************************************************************/
/*                                                                      */
/*      AllOptional -- Check RE specs after end-of-line spec            */
/*                                                                      */
/*      The pattern "$.*" matches the end of line; "$b" must not.       */
/*      This function examines all the RE codes presented after         */
/*      the end-of-line anchor but before the end-of-expression         */
/*      marker, and returns TRUE if the null string matches the         */
/*      list (all the RE elements are optional).                        */
/*                                                                      */
/************************************************************************/
public_scope BOOL
RegExp_AllOptional(RE_ELEMENT *pRE)
{
        BOOL Class;

        /*Loop through codes until end of RE reported*/
        for (;;) {

                Class = FALSE;
                switch (REGEXP_TYPE_R(*pRE++)) {
                case REGEXP_TYPE_END_OF_EXPR:
                        /*Search completed all elements were optional*/
                        return TRUE;
                        break;

                case REGEXP_TYPE_CLASS:
                case REGEXP_TYPE_CLASS_ALL_BUT:
                        Class = TRUE;
                        /*FALLTHOUGH*/

                case REGEXP_TYPE_MATCH_ANY:
                case REGEXP_TYPE_LITERAL:
                        /*Check whether these match types are optional*/
                        break;

                default:
                        /*Anything else doesn't compel a match*/
                        continue;
                        /*break;*/

                }

                /*Found match, check repeat modifier*/
                switch (REGEXP_REPEAT_R(pRE[-1])) {
                case REGEXP_REPEAT_0_OR_MORE:
                case REGEXP_REPEAT_0_OR_1:
                        /*These are entirely optional, so okay*/
                        break;

                default:
                        /*Anything else is presumed to be non-optional*/
                        return FALSE;
                }

                /*Skip membership specifier if a class*/
                if (Class) {
                        pRE += REGEXP_BITMASK_NRCODES;
                }

        }

        /*Control should not reach here*/
        return FALSE;

} /*AllOptional*/


/************************************************************************/
/*                                                                      */
/*      Diagnose -- Report reason for last failure                      */
/*                                                                      */
/*      If pSpec is not NULL, reports the last failure (if any)         */
/*      for that specification.  If NULL, reports the last failure      */
/*      where no RE was associated (e.g. where Compile did not          */
/*      return a specification).                                        */
/*                                                                      */
/************************************************************************/
public_scope void
RegExp_Diagnose(RegExp_Specification *pSpec, CHAR *pDiagnosis)
{
        RegExp_FailureCode FailureCode;

        /*Obtain failure code from module, or from specification if given*/
        FailureCode = gRegExp.FailureCode;
        if (pSpec != NULL) {
                FailureCode = pSpec->FailureCode;
        }

        /*Convert internal code into explanation understood by others*/
        switch (FailureCode) {
        case REGEXP_FAIL_NO_FAILURE:
               strcpy(pDiagnosis, "(no failure)");
               break;

        case REGEXP_FAIL_REGEXP_TOO_LARGE:
               strcpy(pDiagnosis, "RE too big for buffer");
               break;

        case REGEXP_FAIL_UNMATCHED_CLASS_OPEN:
               strcpy(pDiagnosis, "Unmatched [ or [^");
               break;

        case REGEXP_FAIL_NOT_ENOUGH_MEMORY:
               strcpy(pDiagnosis, "Not enough memory");
               break;

        case REGEXP_FAIL_RE_IS_OPTIMAL:
               strcpy(pDiagnosis, "RE is optimal");
               break;

        case REGEXP_FAIL_REQUEST_UNREASONABLE:
               strcpy(pDiagnosis, "Request isn't reasonable");
               break;

        default:
               strcpy(pDiagnosis, "RegExp: ??");
               break;

        }

} /*Diagnose*/


/************************************************************************/
/*                                                                      */
/*      Init -- Prepare module for operation                            */
/*                                                                      */
/************************************************************************/
public_scope void
RegExp_Init(void)
{
        /*No failure known initially*/
        gRegExp.FailureCode = REGEXP_FAIL_NO_FAILURE;

} /*Init*/
