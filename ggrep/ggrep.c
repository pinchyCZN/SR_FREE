/************************************************************************/
/*                                                                      */
/*      GGrep -- Regular expression search using Grouse architecture    */
/*                                                                      */
/*      behoffski is seeing if his way of counting words using an       */
/*      optimised switch statement in assembly can be extended          */
/*      somewhat dramatically to include regular expressions.  If       */
/*      so, the performance should be rather astonishing.  This         */
/*      is the first step: to get a similar, slower table-driven        */
/*      version running in C.  This is to teach behoffski the full      */
/*      joys of regular expression searching.                           */
/*                                                                      */
/*      Started at 10pm on Saturday, 13-May-95.                         */
/*                                                                      */
/*      If you want to find the program's entry point (main),           */
/*      look in Platform.  The code is cut this way so that             */
/*      this module remains portable.                                   */
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
#include "main.h"
#include "platform.h"
#include "regexp00.h"
#include "scanfile.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tracery.h"

/*Maximum RE text that can be read from file*/
#define RE_TEXT_SIZE_MAX                10240

/*Parameter for module diagnosis strings (?? should be in compdef?)*/
#define DIAGNOSIS_LENGTH_MAX            128

/*Codes to select RE syntax and search behaviour*/
typedef enum {
        SEARCH_STYLE_UNSPECIFIED,
        SEARCH_AS_FGREP,
        SEARCH_AS_GREP,
        SEARCH_AS_EGREP
} SEARCH_STYLE;

/*Program options*/
typedef struct {
        SEARCH_STYLE SearchStyle;

} OPT_STRUCT;

/*Permanent variables referenced within this module*/
typedef struct {
        /*Options specified by client*/
        OPT_STRUCT Opt;

        /*Flags modifying regular expression interpretation*/
        LWORD RegexpConfig;

        /*Search and display options used to configure ScanFile*/
        LWORD ScanOptions;
        LWORD ReportingOptions;

        /*Program return code*/
        UINT ReturnCode;

} GREP_MODULE_CONTEXT;

module_scope GREP_MODULE_CONTEXT gGgrep;


/************************************************************************/
/*                                                                      */
/*      Init -- Prepare module for operation                            */
/*                                                                      */
/************************************************************************/
public_scope void
GGrep_Init(void)
{
        /*Program defaults to successful run*/
        gGgrep.ReturnCode = MAIN_RETURN_MATCHED;

} /*Init*/


/************************************************************************/
/*                                                                      */
/*      ReadREFromFile -- Use first line of file as RE                  */
/*                                                                      */
/*      This function is provided to support the -f option of           */
/*      GGrep.  It attempts to open the file specified, then            */
/*      read the first line as RE text.  Returns a pointer to the       */
/*      start of the buffer if successful (with NUL terminator),        */
/*      or else NULL if not successful.                                 */
/*                                                                      */
/************************************************************************/
module_scope CHAR *
GGrep_ReadREFromFile(CHAR *pFilename)
{
        FILE *f;
        CHAR *pREBuf;
        UINT len;

        /*Open named file for read*/
        f = fopen(pFilename, "r");
        if (f == NULL) {
                /*Sorry, unable to open file*/
                fprintf(stderr, "ggrep: Unable to open file %s", pFilename);
                return NULL;

        }

        /*Acquire buffer to store RE text*/
        pREBuf = malloc(RE_TEXT_SIZE_MAX);
        if (pREBuf == NULL) {
                /*Sorry, unable to acquire memory for line buffer*/
                fprintf(stderr, "ggrep: No memory for line buffer");
                return NULL;

        }

        /*Read line into buffer*/
        (void) fgets(pREBuf, RE_TEXT_SIZE_MAX, f);

        /*Does the line end with a LF?*/
        len = strlen(pREBuf);
        if (pREBuf[len - 1] != LF) {
                /*No, line is too big for buffer*/
                fprintf(stderr, "ggrep: line too long in file %s", pFilename);
                free(pREBuf);
                return NULL;

        } else {
                /*Yes, remove it*/
                pREBuf[len - 1] = NUL;
        }

        /*Finished with specifier file*/
        fclose(f);

        /*Report RE to caller*/
        return pREBuf;

} /*ReadREFromFile*/


/************************************************************************/
/*                                                                      */
/*      ReturnCode -- Allow other modules to write program return       */
/*                                                                      */
/*      Other modules may find faults during operation but be able      */
/*      keep going.  These modules must be able to report the faults    */
/*      to the main program so that the program return code can         */
/*      be updated correctly.                                           */
/*                                                                      */
/*      The use of module Main here is very dubious... sigh.            */
/*      This function might be more appropriate as part of Platform.    */
/*                                                                      */
/************************************************************************/
public_scope void
Main_ReturnCode(UINT Code)
{
        /*Is this code more severe than the current return code?*/
        if (Code > gGgrep.ReturnCode) {
                /*Yes, remember this code as program result*/
                gGgrep.ReturnCode = Code;

        }

} /*ReturnCode*/


/************************************************************************/
/*                                                                      */
/*      Usage -- Tell the user what we expect                           */
/*                                                                      */
/************************************************************************/
module_scope void
GGrep_Usage(void)
{
        /*Display usage text*/
        fprintf(stderr,
                "Usage: ggrep [-bcFGhHilLnrRvVwx]"
                " [-CDMcOx -Tx]"
                " ([-e] regexp | -f file) [files]\n");
        /*Sadly, the debug/optimisation/trace usage text is very cryptic*/

        exit(MAIN_RETURN_FAULT);

} /*Usage*/


/************************************************************************/
/*                                                                      */
/*      Version -- Report program version and other info                */
/*                                                                      */
/************************************************************************/
module_scope void
GGrep_Version(void)
{
        /*Display program name and version*/
        fprintf(stderr, "ggrep (Grouse Grep) v2.00  14-Feb-2000\n"
                "Copyright (c) Grouse Software 1995-2000.  " 
                "All rights reserved.\n"
                "Written for Grouse by behoffski (Brenton Hoff).\n");

        exit(0);

} /*Version*/


/************************************************************************/
/*                                                                      */
/*      OptiOpts -- Parse options to enable/disable optimisations       */
/*                                                                      */
/*      Yet another function inspired by the ludicrous lengths          */
/*      to which behoffski is going to build a (hopefully)              */
/*      worthwhile test rig.  The match reported by -M isn't            */
/*      strictly correct in some cases as optimisations can trim        */
/*      the ends away.  Being able to turn off optimisations at         */
/*      will allows this case to report correctly, as well as           */
/*      providing a way of checking that each layer of pattern          */
/*      interpretation is correct, not merely the final,                */
/*      optimised result.                                               */
/*                                                                      */
/************************************************************************/
module_scope void
GGrep_OptiOpts(CHAR *pOpt)
{
        for (;;) {
                if (*pOpt == NUL) {
                        break;
                }
                switch (*pOpt++) {
                case 'A':
                        gGgrep.ScanOptions |=  SCANFILE_OPT_APPROXIMATE;
                        break;
                case 'a':
                        gGgrep.ScanOptions &= ~SCANFILE_OPT_APPROXIMATE;
                        break;
                case 'B':
                        gGgrep.ScanOptions |=  SCANFILE_OPT_TUNED_BM;
                        break;
                case 'b':
                        gGgrep.ScanOptions &= ~SCANFILE_OPT_TUNED_BM;
                        break;
                case 'E':
                        gGgrep.ScanOptions |=  SCANFILE_OPT_EASIEST_FIRST;
                        break;
                case 'e':
                        gGgrep.ScanOptions &= ~SCANFILE_OPT_EASIEST_FIRST;
                        break;
                case 'S':
                        gGgrep.ScanOptions |=  SCANFILE_OPT_SKIP;
                        break;
                case 's':
                        gGgrep.ScanOptions &= ~SCANFILE_OPT_SKIP;
                        break;
                case 'T':
                        gGgrep.ScanOptions |=  SCANFILE_OPT_SELF_TUNED_BM;
                        break;
                case 't':
                        gGgrep.ScanOptions &= ~SCANFILE_OPT_SELF_TUNED_BM;
                        break;

                /*default: ignored*/
                }
        }

} /*OptiOpts*/


/************************************************************************/
/*                                                                      */
/*      ConvertLongOption -- Translate "--LongName" options             */
/*                                                                      */
/*      Converts most -- if not all -- long-style (GNU) option          */
/*      names to their shorter equivalents.  The function               */
/*      rewrites the in/out pointer to point to the rewritten           */
/*      option code(s).  If a long name has no short equivalent,        */
/*      the option should be implemented here, and pOpt changed         */
/*      to NULL to signal the end of the processing.                    */
/*                                                                      */
/*      Returns TRUE if the option was recognised and handled           */
/*      correctly, and FALSE otherwise (usually because the             */
/*      option name wasn't known).                                      */
/*                                                                      */
/************************************************************************/
module_scope BOOL
GGrep_ConvertLongOption(CHAR **ppOpt)
{
        CHAR *pOpt = *ppOpt;

        switch (strlen(pOpt)) {
        case 4:
                if (strcmp(pOpt,        "help") == 0) {
                        pOpt = NULL;
                        GGrep_Usage();
                        break;

                }
                return FALSE;
                /*break;*/

        case 5:
                if (strcmp(pOpt,        "quiet") == 0) {
                        pOpt = "q";
                        break;
                } else if (strcmp(pOpt, "count") == 0) {
                        pOpt = "c";
                        break;
                }
                return FALSE;

        case 6:
                if (strcmp(pOpt,        "silent") == 0) {
                        pOpt = "?";
                        break;
                }
                return FALSE;

        case 7:
                if (strcmp(pOpt,        "version") == 0) {
                        pOpt = "V";
                        break;
                }
                return FALSE;

        case 9:
                if (strcmp(pOpt,        "recursive") == 0) {
                        pOpt = "r";
                        break;
                }
                return FALSE;

        case 11:
                if (strcmp(pOpt,        "no-filename") == 0) {
                        pOpt = "h";
                        break;
                } else if (strcmp(pOpt, "ignore-case") == 0) {
                        pOpt = "i";
                        break;
                } else if (strcmp(pOpt, "byte-offset") == 0) {
                        pOpt = "b";
                        break;
                } else if (strcmp(pOpt, "line-number") == 0) {
                        pOpt = "n";
                        break;
                } else if (strcmp(pOpt, "word-regexp") == 0) {
                        pOpt = "w";
                        break;
                } else if (strcmp(pOpt, "line-regexp") == 0) {
                        pOpt = "x";
                        break;
                }
                return FALSE;

        case 12:
                if (strcmp(pOpt,        "basic-regexp") == 0) {
                        pOpt = "G";
                        break;
                } else if (strcmp(pOpt, "revert-match") == 0) {
                        pOpt = "v";
                        break;
                }
                return FALSE;

        case 13:
                if (strcmp(pOpt,        "fixed-strings") == 0) {
                        pOpt = "F";
                        break;
                }
                return FALSE;

        case 18:
                if (strcmp(pOpt,        "files-with-matches") == 0) {
                        pOpt = "l";
                        break;
                }
                return FALSE;

        case 19:
                if (strcmp(pOpt,        "files-without-match") == 0) {
                        pOpt = "L";
                        break;
                }
                return FALSE;

        default:
                /*Unrecognised option*/
                return FALSE;

        }

        /*Option handled, write result to caller and report success*/
        *ppOpt = pOpt;
        return TRUE;


} /*ConvertLongOption*/


/************************************************************************/
/*                                                                      */
/*      main -- Co-ordinate execution of program                        */
/*                                                                      */
/************************************************************************/
public_scope INT
GGrep_main(UINT argc, CHAR **argv)
{
        RegExp_Specification *pCompactRE = NIL;
        CHAR *pRegexpText = NULL;
        CHAR *pArg;
        CHAR *pOpt;
        CHAR *pREFilename;
        CHAR Diagnosis[DIAGNOSIS_LENGTH_MAX];
        BOOL SuppressFilename = FALSE;
        CHAR *pProgName;

        /*If no arguments, give Usage message to guide user*/
        if (argc == 1) {
                GGrep_Usage();
        }
        argv++;

        /*Prepare default options*/
        gGgrep.Opt.SearchStyle = SEARCH_STYLE_UNSPECIFIED;
        gGgrep.ReportingOptions = MATCHENG_RPT_LINE;
        gGgrep.ScanOptions = (0 
                              | SCANFILE_OPT_APPROXIMATE
                              | SCANFILE_OPT_EASIEST_FIRST
                              | SCANFILE_OPT_SKIP 
                              | SCANFILE_OPT_SELF_TUNED_BM
                              );
        gGgrep.RegexpConfig = REGEXP_CONFIG_CLASS_CONV_CASE;

        /*Select search style if it's indicated by the program name*/
        pProgName = Platform_ProgramName();
        if (strcmp(pProgName, "fgrep") == 0) {
                gGgrep.Opt.SearchStyle = SEARCH_AS_FGREP;
        } else if (strcmp(pProgName, "egrep") == 0) {
                gGgrep.Opt.SearchStyle = SEARCH_AS_EGREP;
        }

        /*Loop through arguments, processing options etc*/
        for (;; argv++) {
                /*Have we run out of arguments?*/
                if (--argc == 0) {
                        /*Yes, was a RE specified by an option?*/
                        if (pRegexpText == NULL) {
                                /*No, a search without RE is meaningless*/
                                GGrep_Usage();
                        }

                        /*User wants to use stdin as input*/
                        break;

                }

                /*Refer to next argument via convenient pointer*/
                pArg = *argv;

                /*Is the next argument an option specifier?*/
                if (pArg[0] != '-') {
                        /*No, user has finished giving options*/
                        break;
                }

                /*Yes, decode option and record selection*/
                pOpt = &pArg[1];

                /*Did we find a "-" by itself?*/
                if (*pOpt == NUL) {
                        /*Yes, this is the RE to search for*/
                        break;
                }

                /*Do we have a "--longname"-style option?*/
                if ((pOpt[0] == '-') && (pOpt[1] != NUL)) {
                        /*Yes, convert long name to short name*/
                        pOpt++;
                        if (! GGrep_ConvertLongOption(&pOpt)) {
                                /*Sorry, option not recognised*/
                                fprintf(stderr, "ggrep: Unknown option: %s\n",
                                        pOpt);
                                GGrep_Usage();
                                break;
                        }

                        /*Did the conversion leave us with an option code?*/
                        if (pOpt == NULL) {
                                /*No, move to next option (if any)*/
                                continue;
                        }

                }

DecodeOption:
                /*Act on next option byte*/
                switch (*pOpt++) {
                case 'b':
                        /*Include byte offset in display*/
                        gGgrep.ReportingOptions |=
                                        MATCHENG_RPT_BYTEOFFSET;
                        break;

                case 'c':
                        /*Count selected lines and don't display them*/
                        gGgrep.ReportingOptions |= MATCHENG_RPT_LINECOUNT;
                        gGgrep.ReportingOptions &= ~MATCHENG_RPT_LINE;
                        break;

                case 'C':
                        /*Debug: Display compiled RE before expansion*/
                        gGgrep.ScanOptions |= SCANFILE_DEBUG_COMPILED;
                        break;

                case 'D':
                        /*Display state tables (should move over to Tracery)*/
                        gGgrep.ScanOptions |= SCANFILE_DEBUG_DISPLAY;
                        break;

                case 'e':
                        /*Have we exhausted the current option word?*/
                        if (*pOpt != NUL) {
                                /*No, use remainder of word as RE*/
                                pRegexpText = pOpt;

                        } else {
                                /*Yes, next arg is RE*/
                                if (--argc == 0) {
                                        GGrep_Usage();
                                }
                                pRegexpText = *++argv;
                        }

                        /*Finished handling current option word(s)*/
                        continue;
                        break;

                case 'E':
                        /*Select extended RE syntax*/
                        switch (gGgrep.Opt.SearchStyle) {
                        case SEARCH_STYLE_UNSPECIFIED:
                        case SEARCH_AS_EGREP:
                                gGgrep.Opt.SearchStyle = SEARCH_AS_EGREP;
                                break;

                        default:
                                fprintf(stderr, 
                             "you may specify only one of -E, -F or -G\n");
                                exit(MAIN_RETURN_FAULT);
                                break;
                        }
                        break;

                case 'f':
                        /*Have we exhausted the current option word?*/
                        if (*pOpt != NUL) {
                                /*No, use remainder of word as file name*/
                                pREFilename = pOpt;

                        } else {
                                /*Yes, next word contains RE file name*/
                                if (--argc == 0) {
                                        GGrep_Usage();
                                }
                                pREFilename = *++argv;
                        }
                        pRegexpText = GGrep_ReadREFromFile(pREFilename);
                        if (pRegexpText == NULL) {
                                /*Failed to get RE from file*/
                                return MAIN_RETURN_FAULT;
                        }

                        /*Finished handling current option word(s)*/
                        continue;
                        break;

                case 'F':
                        /*Select fixed-string RE syntax and operation*/
                        switch (gGgrep.Opt.SearchStyle) {
                        case SEARCH_STYLE_UNSPECIFIED:
                        case SEARCH_AS_FGREP:
                                gGgrep.Opt.SearchStyle = SEARCH_AS_FGREP;
                                break;

                        default:
                                fprintf(stderr, 
                             "you may specify only one of -E, -F or -G\n");
                                exit(MAIN_RETURN_FAULT);
                                break;
                        }
                        break;

                case 'G':
                        /*Select basic RE syntax*/
                        /*??*/
                        switch (gGgrep.Opt.SearchStyle) {
                        case SEARCH_STYLE_UNSPECIFIED:
                        case SEARCH_AS_GREP:
                                gGgrep.Opt.SearchStyle = SEARCH_AS_GREP;
                                break;

                        default:
                                fprintf(stderr, 
                             "you may specify only one of -E, -F or -G\n");
                                exit(MAIN_RETURN_FAULT);
                                break;
                        }
                        break;

                case 'h':
                        /*Suppress file name prefix if displaying lines*/
                        SuppressFilename = TRUE;
                        break;

                case 'H':
                        /*Highlight match as part of display*/
                        gGgrep.ReportingOptions |= MATCHENG_RPT_HIGHLIGHT;
                        break;

                case 'i':
                        /*RE becomes case-insensitive*/
                        gGgrep.RegexpConfig |= REGEXP_CONFIG_IGNORE_CASE;
                        break;

                case 'l':
                        /*Only show filename for matching files*/
                        gGgrep.ReportingOptions &= ~MATCHENG_RPT_LINE;
                        gGgrep.ReportingOptions |= MATCHENG_RPT_FILENAME | 
                                MATCHENG_RPT_NORMAL_MATCH_SENSE;
                        break;

                case 'L':
                        /*Only show filename for nonmatch files*/
                        gGgrep.ReportingOptions |= MATCHENG_RPT_NONMATCH_FILES;
                        break;

                case 'M':
                        /*Marker character to delimit match/nonmatch in line*/
                        gGgrep.ReportingOptions |= MATCHENG_RPT_MARKER(*pOpt);
                        if (*pOpt++ == NUL) pOpt--;
                        break;

                case 'n':
                        /*Include file line number in display*/
                        gGgrep.ScanOptions |= SCANFILE_NUMBER_MATCHING_LINES;
                        gGgrep.ReportingOptions |= MATCHENG_RPT_LINENUMBER;
                        break;

                case 'O':
                        /*Configure optimisations (mainly for test rig)*/
                        GGrep_OptiOpts(pOpt);
                        continue;
                        break;

                case 'r':
                        /*Recurse down subdirectories*/
                        gGgrep.ScanOptions |= SCANFILE_OPT_RECURSE_DIRECTORIES;
                        break;

                case 'R':
                        /*Let CR serve as a line terminator*/
                        gGgrep.ScanOptions      |= SCANFILE_CR_IS_TERMINATOR;
                        gGgrep.ReportingOptions |= 
                                MATCHENG_RPT_REMOVE_TRAILING_CR;
                        break;

                case 'T':
                        /*Trace module operations according to flags*/
                        if (*pOpt == NUL) {
                                if (--argc == 0) {
                                        GGrep_Usage();
                                }
                                pOpt = *++argv;
                        }

                        /*Does the caller want details about Tracery?*/
                        if (pOpt[0] == '?') {
                                /*Yes, was Tracery compiled in?*/
                                #if ! TRACERY_ENABLED
                                        /*No, issue a warning*/
                                        fprintf(stderr, "%s: "
                                                "(Tracery not compiled in?)\n", 
                                                Platform_ProgramName());
                                #endif

                                /*Was a module name or wildcard given?*/
                                if (pOpt[1] == NUL) {
                                        /*No, request a brief summary*/
                                        Tracery_DisplayRegistrations(NULL);
                                } else {
                                        /*Yes, request full details*/
                                        Tracery_DisplayRegistrations(
                                                &pOpt[1]);
                                }
                        } else {
                                Tracery_Configure(pOpt);
                        }
                        continue;
                        /*break;*/

                case 'v':
                        /*Invert match sense*/
                        gGgrep.ReportingOptions |= 
                                MATCHENG_RPT_INVERT_MATCH_SENSE;
                        break;

                case 'V':
                        /*Display program version info*/
                        GGrep_Version();
                        break;

                case 'w':
                        /*Match edges must have nonword chars outside*/
                        /*Note: NOT equivalent to \<expression\>*/
                        gGgrep.RegexpConfig |= REGEXP_CONFIG_WORD_EDGES;
                        break;

                case 'W':
                        /*-w, plus constrain "." and "[^...]" to word chars*/
                        gGgrep.RegexpConfig |= REGEXP_CONFIG_WORD_EDGES | 
                                REGEXP_CONFIG_WORD_CONSTRAIN;
                        break;

                case 'x':
                        /*Only match full lines*/
                        gGgrep.RegexpConfig |= REGEXP_CONFIG_FULL_LINE_ONLY;
                        break;

                default:
                        fprintf(stderr, "ggrep: Unknown option: %c\n",
                                        pOpt[-1]);
                        GGrep_Usage();
                        break;

                }

                /*Have we reached the end of the word?*/
                if (*pOpt != NUL) {
                        /*No, handle next option within this word*/
                        goto DecodeOption;
                }

                /*Loop will consider next word*/

        }


        /*Did we get a RE spec while handling the option list?*/
        if (pRegexpText == NULL) {
                /*No, use next parameter as RE text*/
                pRegexpText = *argv++;
                argc--;

        }

        /*Convert ASCII RE description into compact form*/
        switch (gGgrep.Opt.SearchStyle) {
        case SEARCH_AS_EGREP:
                /*Extended REs: ? and + special, \? and \+ literals*/
                gGgrep.RegexpConfig |= 
                        REGEXP_CONFIG_PLUS | 
                        REGEXP_CONFIG_QUESTION_MARK | 
                        REGEXP_CONFIG_NAMED_CLASSES | 
                        REGEXP_CONFIG_ESC_LESS_THAN | 
                        REGEXP_CONFIG_ESC_GREATER_THAN | 
                        REGEXP_CONFIG_WORD_BOUNDARY | 
                        REGEXP_CONFIG_WORD_NONBOUNDARY |
                        REGEXP_CONFIG_OPS_LITERAL_START;

                /*Sadly, we only support a subset of extended RE syntax*/
                if (! RegExp_Compile(pRegexpText, 
                                     gGgrep.RegexpConfig, 
                                     &pCompactRE)) {
                        /*Found some fault during conversion*/
                        RegExp_Diagnose(NULL, Diagnosis);
                        fprintf(stderr, "ggrep: %s\n", Diagnosis);
                        return MAIN_RETURN_FAULT;
                }

                break;

        case SEARCH_AS_FGREP:
                /*Treat pattern as a simple string without special chars*/
                if (! RegExp_CompileString(pRegexpText, 
                                           gGgrep.RegexpConfig, 
                                           &pCompactRE)) {
                        /*Found some fault during conversion*/
                        RegExp_Diagnose(NULL, Diagnosis);
                        fprintf(stderr, "ggrep: %s\n", Diagnosis);
                        return MAIN_RETURN_FAULT;
                }
                break;

        case SEARCH_AS_GREP:
        case SEARCH_STYLE_UNSPECIFIED:
                /*Basic REs: ? and + literals, \? and \+ special*/
                gGgrep.RegexpConfig |= 
                        REGEXP_CONFIG_ESC_PLUS | 
                        REGEXP_CONFIG_ESC_QUESTION_MARK | 
                        REGEXP_CONFIG_NAMED_CLASSES | 
                        REGEXP_CONFIG_ESC_LESS_THAN | 
                        REGEXP_CONFIG_ESC_GREATER_THAN | 
                        REGEXP_CONFIG_WORD_BOUNDARY | 
                        REGEXP_CONFIG_WORD_NONBOUNDARY |
                        REGEXP_CONFIG_OPS_LITERAL_START;

                /*Compile RE*/
                if (! RegExp_Compile(pRegexpText, 
                                     gGgrep.RegexpConfig, 
                                     &pCompactRE)) {
                        /*Found some fault during conversion*/
                        RegExp_Diagnose(NULL, Diagnosis);
                        fprintf(stderr, "ggrep: %s\n", Diagnosis);
                        return MAIN_RETURN_FAULT;
                }
                break;

        }

        /*Are we displaying any lines?*/
        if (! (gGgrep.ReportingOptions & (MATCHENG_RPT_LINE | 
                                          MATCHENG_RPT_HIGHLIGHT))) {
                /*No, don't count lines (in case we were asked to)*/
                gGgrep.ScanOptions &= ~SCANFILE_NUMBER_MATCHING_LINES;
        }

        /*Does the client want to see the exact matching text?*/
        if (gGgrep.ReportingOptions & MATCHENG_RPT_HIGHLIGHT) {
                /*Yes, scanfile can't use approximations at start and end*/
                gGgrep.ScanOptions &= ~SCANFILE_OPT_APPROXIMATE;
        }

        /*Are we counting lines for an inverted match?*/
        if ((gGgrep.ScanOptions & SCANFILE_NUMBER_MATCHING_LINES) && 
            (gGgrep.ReportingOptions & MATCHENG_RPT_INVERT_MATCH_SENSE)) {
                /*Yes, be very specific so search can optimise properly*/
                gGgrep.ScanOptions &= ~SCANFILE_NUMBER_MATCHING_LINES;
                gGgrep.ScanOptions |= SCANFILE_NUMBER_NONMATCH_LINES;
        }

        /*Give ScanFile regular expression pattern plus search modifications*/
        if (! ScanFile_Pattern(pCompactRE, gGgrep.ScanOptions)) {
                /*ScanFile was unable to accept pattern for search*/
                fprintf(stderr, "ggrep: Unable to set up search?!\n");
                return MAIN_RETURN_FAULT;
        }

        /*Discard compact pattern since it isn't used again*/
        /*RegExp_Discard(pCompactRE);*/

        /*Remaining arguments (if any) are files to search*/

        /*Was the only file name "-"?*/
        if ((argc == 1) && (strcmp(*argv, "-") == 0)) {
                /*Yes, delete it so that stdin is selected*/
                argc--;
                argv++;
        }

        /*Were any files given?*/
        if (argc == 0) {
                /*No, immediately configure file scan for search*/
                ScanFile_Configure(gGgrep.ReportingOptions);

                /*Scan stdin using line-by-line search*/
                ScanFile_Search(NULL, NULL);

        } else {
                /*Was more than one file given (and names aren't suppressed?)*/
                if ((argc > 1) && (! SuppressFilename)) {
                        /*Yes, display filename as part of match*/
                        gGgrep.ReportingOptions |= MATCHENG_RPT_FILENAME;
                }

                /*Configure file scan for search*/
                ScanFile_Configure(gGgrep.ReportingOptions);

                /*Scan each file named on command line*/
                while (argc--) {
                        if (! ScanFile_Search(*argv++, NULL)) {
                                /*We may skip remaining files*/
                                break;
                        }
                }

        }

        /*Did no lines match (and no other errors detected)?*/
        if ((! ScanFile_MatchedAny()) && (gGgrep.ReturnCode == 0)) {
                /*Yes, report failure to environment*/
                gGgrep.ReturnCode = 1;
        }

        /*Report success or failure*/
        return gGgrep.ReturnCode;

} /*main*/
