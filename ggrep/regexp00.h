/************************************************************************/
/*                                                                      */
/*      RegExp -- Functions to assist manipulating regular expressions  */
/*                                                                      */
/*      This module provides assistance to programs that offer RE       */
/*      facilities to the Real World.  It provides functions to         */
/*      describe regular expressions in a compact and convenient        */
/*      fashion.                                                        */
/*                                                                      */
/*      The files have been renamed to regexp00.[ch] to avoid           */
/*      any confusion with the far more important standard              */
/*      regular expression facilities in Unix provided by               */
/*      Henry Spencer and others.  However, the module name             */
/*      used inside the file has not been changed.                      */
/*                                                                      */
/*      "All but" classes are given a code of their own separate        */
/*      to "any of" classes so that case insensitivity can be           */
/*      deferred until the class is used.                               */
/*                                                                      */
/*      This module now uses 32-bit codes; the original used            */
/*      16-bit codes.  This should make RE elements easier to           */
/*      manage.  Hopefully, this format will also permit                */
/*      storage of Unicode glyphs.                                      */
/*                                                                      */
/*      Copyright (C) 1995-2000 Grouse Software.  All rights reserved.  */
/*      Written for Grouse by behoffski (Brenton Hoff).                 */
/*                                                                      */
/*      Free software: no warranty; use anywhere is ok; spread the      */
/*      sources; note any mods; share variations and derivatives        */
/*      (including sending to behoffski@grouse.com.au).                 */
/*                                                                      */
/************************************************************************/

#ifndef REGEXP_H
#define REGEXP_H

#include <compdef.h>
#include <limits.h>

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/

/*Configuration options modifying RE interpretation*/

#define REGEXP_CONFIG_FULL_LINE_ONLY     BIT0
#define REGEXP_CONFIG_IGNORE_CASE        BIT1
#define REGEXP_CONFIG_WORD_EDGES         BIT2 /* for -w and -W switches*/
#define REGEXP_CONFIG_PLUS               BIT3 /* "+"  means "1 or more"*/
#define REGEXP_CONFIG_QUESTION_MARK      BIT4 /* "?"  means "0 or 1"*/
#define REGEXP_CONFIG_ESC_PLUS           BIT5 /* "\+" means "1 or more"*/
#define REGEXP_CONFIG_ESC_QUESTION_MARK  BIT6 /* "\?" means "0 or 1"*/
#define REGEXP_CONFIG_NAMED_CLASSES      BIT7 /* [[:alnum:]], etc*/
#define REGEXP_CONFIG_ESC_LESS_THAN      BIT8 /* "\<" means word beginning*/
#define REGEXP_CONFIG_ESC_GREATER_THAN   BIT9 /* "\>" means word end*/
#define REGEXP_CONFIG_WORD_BOUNDARY      BIT10 /* "\b" means word boundary*/
#define REGEXP_CONFIG_WORD_NONBOUNDARY   BIT11 /* "\B" means word nonbndry*/
#define REGEXP_CONFIG_WORD_CONSTRAIN     BIT12 /* for -W switch*/
#define REGEXP_CONFIG_CLASS_CONV_CASE    BIT13 /* fold [Aa] to literal+CI*/
#define REGEXP_CONFIG_OPS_LITERAL_START  BIT14 /* *+? literals at RE start*/

/*Don't select CLASS_CONV_CASE if locale could change after RE compilation*/

/*

Layout of regular expression codes:

33222222 22221111 111111
10987654 32109876 5432109 876543210

                  lllllll lllllllll    Literal (possibly Unicode glyph?)
             nnnn nnnnnnn nnnnnnnnn    Number
                                       ?? Could be used for organising 
                                       extended REs (parentheses, alternation)
                                       e.g. storing sizes/offsets

ttttttrr riww                          tttttt = type
                                              0: end of RE
                                              1: literal
                                              2: class
                                              3: class_all_but
                                              4: match_any
                                             63: anchor_start
                                             62: anchor_end
                                             61: word_beginning
                                             60: word_end
                                             59: word_boundary
                                             58: word_nonboundary
                                             57: word_prev_nonword
                                             (others reserved for extended REs)
                                       rrr = repeat
                                             0: {1, 1} times (no modifier)
                                             1: {0, 1} times ("?" modifier)
                                             2: {1, *} times ("+" modifier)
                                             3: {0, *} times ("*" modifier)
                                             (4-7 reserved e.g. {n, m}, {n, *})
                                       i = case insensitivity flag
                                             0: case sensitive
                                             1: case insensitive
                                       w = word flag
                                             0: not restricted to word chars
                                             1: restrict match to word chars
                                             2: restrict match to nonword chars
                                             (3 is reserved)
                                             where word char = [A-Za-z_0-9]

*/

/*Codes for specifying regular expression elements*/

typedef LWORD RE_ELEMENT;

/*Bits 0..15 represent Unicode (or probably just ASCII) characters*/
/*Bits 16..31 are used for flags such as case insensitivity*/
#define REGEXP_CHAR_MAX                 255
/*WARNING: Unicode support (CHAR_MAX > 255) is NOT yet supported*/

/*Each class code is followed by codes containing bitmask of members*/
#define REGEXP_BITS_PER_CODE            32

#define REGEXP_BITMASK_NRCODES          ((REGEXP_CHAR_MAX + 1) / \
                                             REGEXP_BITS_PER_CODE)

/*Macros to help manipulate bitmask*/
#define REGEXP_BIT_SET(pCode, Ch)     (pCode[(Ch) / REGEXP_BITS_PER_CODE] |= \
                                         (1uL << (Ch % REGEXP_BITS_PER_CODE)))
#define REGEXP_BIT_CLEAR(pCode, Ch)   (pCode[(Ch) / REGEXP_BITS_PER_CODE] &= \
                                        ~(1uL << (Ch % REGEXP_BITS_PER_CODE)))
#define REGEXP_BIT_TOGGLE(pCode, Ch)  (pCode[(Ch) / REGEXP_BITS_PER_CODE] ^= \
                                        ~(1uL << (Ch % REGEXP_BITS_PER_CODE)))
#define REGEXP_BIT_TEST(pCode, Ch)    (pCode[(Ch) / REGEXP_BITS_PER_CODE] & \
                                         (1uL << (Ch % REGEXP_BITS_PER_CODE)))

/*Type definitions for each code of RE*/

#define REGEXP_TYPE_MASK                0xfc000000uL
#define REGEXP_TYPE(t)                  ((((LWORD) (t)) << 26) & \
                                                REGEXP_TYPE_MASK)
#define REGEXP_TYPE_R(Element)          ((UINT) ((((LWORD) (Element)) & \
                                                REGEXP_TYPE_MASK) >> 26))

/*Control specifiers (help describe RE structure)*/
#define REGEXP_TYPE_END_OF_EXPR         0
/*?? Add specifiers for alternation and grouping? -- later*/

/*Normal specifiers (add matching character to matched text)*/
#define REGEXP_TYPE_LITERAL             2
#define REGEXP_TYPE_CLASS               3
#define REGEXP_TYPE_CLASS_ALL_BUT       4
#define REGEXP_TYPE_MATCH_ANY           5

/*Meta-specifiers (test characters but don't lengthen matched text)*/
#define REGEXP_TYPE_ANCHOR_START        63
#define REGEXP_TYPE_ANCHOR_END          62
#define REGEXP_TYPE_WORD_BEGINNING      61 /*for \< specifier*/
#define REGEXP_TYPE_WORD_END            60
#define REGEXP_TYPE_WORD_BOUNDARY       59
#define REGEXP_TYPE_WORD_NONBOUNDARY    58
#define REGEXP_TYPE_WORD_PREV_NONWORD   57 /*for -w start-of-RE check*/
#define REGEXP_TYPE_DEAD_END            56

#define REGEXP_TYPE_IS_META(t)          ((t) > 48)

#define REGEXP_CASE_MASK                BIT22
#define REGEXP_CASE_SENSITIVE           0x00
#define REGEXP_CASE_INSENSITIVE         BIT22

#define REGEXP_WORD_MASK                (BIT21 | BIT20)
#define REGEXP_WORD(w)                  ((((LWORD) (w)) << 20) & \
                                                REGEXP_WORD_MASK)
#define REGEXP_WORD_R(Element)          ((UINT) ((((LWORD) (Element)) & \
                                                REGEXP_WORD_MASK) >> 20))
#define REGEXP_WORD_UNRESTRICTED        0
#define REGEXP_WORD_WORD_CHARS          1
#define REGEXP_WORD_NONWORD_CHARS       2

#define REGEXP_REPEAT_MASK              0x03800000uL
#define REGEXP_REPEAT(r)                ((((LWORD) (r)) << 23) & \
                                                REGEXP_REPEAT_MASK)
#define REGEXP_REPEAT_R(Element)        ((UINT) ((((LWORD) (Element)) & \
                                                REGEXP_REPEAT_MASK) >> 23))

#define REGEXP_REPEAT_MATCH_SELF        0x00   /*{1, 1}, no modifier*/
#define REGEXP_REPEAT_0_OR_1            BIT0   /*{0, 1}, "?" modifier*/
#define REGEXP_REPEAT_1_OR_MORE         BIT1   /*{1, *}, "+" modifier*/
#define REGEXP_REPEAT_0_OR_MORE         (BIT0 | BIT1) /*{0, *}, "*" modifier*/

#define REGEXP_REPEAT_IS_ZERO_TRIP(Code) (REGEXP_REPEAT_R(Code) & BIT0)

/*Repeat codes 4..7 reserved for extended REs, e.g. {n, m}, {n, *}*/

#define REGEXP_LITERAL_MASK             0xffffuL
#define REGEXP_LITERAL(l)               ((l) & REGEXP_LITERAL_MASK)
#define REGEXP_LITERAL_R(Element)       ((UINT) ((Element) & \
                                                  REGEXP_LITERAL_MASK))

#define REGEXP_NUMBER_MASK              0x0fffffuL
#define REGEXP_NUMBER(n)                (((LWORD) (n)) & REGEXP_NUMBER_MASK)
#define REGEXP_NUMBER_R(Element)        ((UINT) ((Element) & \
                                                 REGEXP_NUMBER_MASK))

/*Codes summarising why operations (especially Compile) failed*/
typedef enum {
        REGEXP_FAIL_NO_FAILURE,
        REGEXP_FAIL_REGEXP_TOO_LARGE,
        REGEXP_FAIL_UNMATCHED_CLASS_OPEN,
        REGEXP_FAIL_NOT_ENOUGH_MEMORY,
        REGEXP_FAIL_RE_IS_OPTIMAL,
        REGEXP_FAIL_REQUEST_UNREASONABLE

} RegExp_FailureCode;

/*Specifier which both identifies expression and provides return information*/
typedef struct {
        /*Number of states within RE*/
        UINT NrStates;

        /*Reason for last failure (if any)*/
        RegExp_FailureCode FailureCode;

        /*Compiled form of expression*/
        RE_ELEMENT CodeList[1];
        /*... actual size is greater -- allocated at run time*/


} RegExp_Specification;


/************************************************************************/
/*                                                                      */
/*      Init -- Prepare module for operation                            */
/*                                                                      */
/************************************************************************/
void
RegExp_Init(void);


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
BOOL
RegExp_Compile(CHAR *pText, LWORD Flags, 
               RegExp_Specification **ppSpec);


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
BOOL
RegExp_CompileString(CHAR *pText, LWORD Flags, 
                     RegExp_Specification**ppSpec);


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
/*      This function determines whether this optimisation is worth     */
/*      while.  If not, the function returns FALSE.                     */
/*                                                                      */
/*      If the search can be optimised by the split, EasiestFirst       */
/*      splits the original RE so that it matches the complicated       */
/*      starting bit, and creates a new RE spec which begins with       */
/*      the easiest bit.                                                */
/*                                                                      */
/************************************************************************/
BOOL
RegExp_EasiestFirst(RegExp_Specification *pSpec, 
                    RegExp_Specification **ppEasierBit);


/************************************************************************/
/*                                                                      */
/*      SlashEnds -- Remove start/end elements for inexact matches      */
/*                                                                      */
/*      Where the client does not care to know the exact string         */
/*      that matched a regular expression, iterative or optional        */
/*      elements at the start or end of a regular exression can         */
/*      be eliminated, reducing the backtracking required to            */
/*      perform the search.  For example:                               */
/*              a*.*b*.*Quasimodo's Dream[^a]+                          */
/*      can be reduced to:                                              */
/*              Quasimodo's Dream[^a]                                   */
/*                                                                      */
/*      This function finds cases like this and edits the RE to         */
/*      remove unnecessary elements.  The function returns TRUE         */
/*      if optimisations were found.  Remember that the resulting       */
/*      match correctly selects lines but is otherwise inexact.         */
/*                                                                      */
/************************************************************************/
BOOL
RegExp_SlashEnds(RegExp_Specification *pRE);


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
BOOL
RegExp_AllOptional(RE_ELEMENT *pRE);


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
void
RegExp_Diagnose(RegExp_Specification *pSpec, CHAR *pDiagnosis);


/************************************************************************/
/*                                                                      */
/*      ShowCodes -- Disassemble RE coding (for debugging)              */
/*                                                                      */
/************************************************************************/
void
RegExp_ShowCodes(CHAR *pLabel, RegExp_Specification *pRE);


#ifdef __cplusplus
}
#endif /*__cplusplus*/

#endif /*REGEXP_H*/
