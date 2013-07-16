/************************************************************************/
/*                                                                      */
/*      Bakslash -- Provide C-style "\" escape encode/decode            */
/*                                                                      */
/*      Although this module pretends to be a generic, reusable         */
/*      module, it is in fact specific to the interface offered         */
/*      by "grep" in more than a few ways.  Requires some more          */
/*      interface routines and/or configuration options to be           */
/*      more useful.                                                    */
/*                                                                      */
/*      (Note for the pedantic: I've used "Bakslash" instead of         */
/*      "Backslash" since I'm trying to keep a strict one-to-one        */
/*      correspondence between module names and file names.)            */
/*                                                                      */
/*      The implementation of this module could be optimised by         */
/*      using the Grouse FSA.  behoffski has not done so because        */
/*      this is an after-hours project and he's focussing on the        */
/*      major function instead of nuances.  Would be nice, though.      */
/*                                                                      */
/*      Copyright (C) Grouse Software 1995-9.  All rights reserved.     */
/*      Written for Grouse by behoffski (Brenton Hoff).                 */
/*                                                                      */
/*      Free software: no warranty; use anywhere is ok; spread the      */
/*      sources; note any mods; share variations and derivatives        */
/*      (including sending to behoffski@grouse.com.au).                 */
/*                                                                      */
/************************************************************************/

#include "ascii.h"
#include "bakslash.h"
#include <compdef.h>
#include "ctype.h"
#include "stdlib.h"
#include "stdio.h"
#include "string.h"

#define HEX2BYTE(ch) ((BYTE) (((ch)<='9') ? (ch)-'0' : tolower(ch) - 'a' + 10))


/************************************************************************/
/*                                                                      */
/*      Decode -- Convert text containing \ escapes to binary           */
/*                                                                      */
/************************************************************************/
public_scope BYTE
Bakslash_Decode(CHAR **ppLine)
{
        CHAR *pLine = *ppLine;
        CHAR ch;

        /*Default to consuming one character*/
        *ppLine = pLine + 1;

        switch (*pLine) {
        case '\\':
                /*"\" literal*/
                return (BYTE) '\\';
                break;

        case 'a':
                /*Alert (bell)*/
                return (BYTE) '\a';
                break;

        case 'b':
                /*Backspace*/
                return (BYTE) '\b';
                break;

        case 'f':
                /*Form feed*/
                return (BYTE) '\f';
                break;

        case 'n':
                /*Newline*/
                return (BYTE) '\n';
                break;

        case 'r':
                /*Carriage return*/
                return (BYTE) '\r';
                break;

        case 't':
                /*Horizontal tab*/
                return (BYTE) '\t';
                break;

        case 'v':
                /*Vertical tab*/
                return (BYTE) '\v';
                break;

        case 'x':
        case 'X':
                /*One- or two-digit hex sequence*/
                ch = *++pLine;
                if (! isxdigit(ch)) {
                        /*?? Not properly formed... treat as escaped "x"?*/
                        return (BYTE) pLine[-1];
                }
                /*Does the sequence contain two digits?*/
                if (isxdigit(pLine[1])) {
                        /*Yes, consume them and decode now*/
                        *ppLine += 2;

                        /*?? Should consume ALL hex digits*/

                        return (HEX2BYTE(ch) << 4) | HEX2BYTE(pLine[1]);

                } else {
                        /*No, consume one digit and return conversion*/
                        *ppLine += 1;
                        return HEX2BYTE(pLine[0]);

                }
                break;

        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
                /*First digit of octal sequence... is second also octal?*/
                ch = *++pLine;
                if ((ch < '0') || (ch > '7')) {
                        /*No, merely single octal digit*/
                        return (BYTE) (pLine[-1] - '0');
                }

                /*Two-digit sequence... are all three chars one octal number?*/
                if ((pLine[-1] > '3') || (pLine[1] < '0') ||
                                (pLine[1] > '7')) {
                        /*No, report two-digit code*/
                        *ppLine += 1;
                        return ((((BYTE) (pLine[-1] - '0')) << 3) |
                                      ((BYTE) (ch - '0')));

                }

                /*Three-digit octal sequence found*/
                *ppLine += 2;
                return (((BYTE) (pLine[-1] - '0')) << 6) |
                       (((BYTE) (       ch - '0')) << 3) |
                        ((BYTE) ( pLine[1] - '0'));

                break;

        case NUL:
                /*Unexpected end of line*/
                fprintf(stderr, "%s: trailing backslash\n", 
                        "ggrep");
                exit(2);
                break;

        default:
                /*Return character WITHOUT special meaning*/
                return (BYTE) *pLine;
        }

        /*NOTREACHED*/
        return 0xff;

} /*Decode*/


/************************************************************************/
/*                                                                      */
/*      DecodeLine -- Edit line to replace escape seq's                 */
/*                                                                      */
/*      Returns FALSE if the decoding was abandoned for any reason.     */
/*                                                                      */
/************************************************************************/
public_scope BOOL
Bakslash_DecodeLine(CHAR *pLine)
{
        CHAR *pDecoded = pLine;

        for (;;) {
                switch (*pLine) {
                case NUL:
                        /*End of line*/
                        *pDecoded = NUL;
                        return TRUE;
                        break;

                case '\\':
                        /*Start of escape sequence found*/
                        pLine++;
                        *pDecoded++ = Bakslash_Decode(&pLine);
                        break;

                default:
                        /*Copy literal to decoded string*/
                        *pDecoded++ = *pLine++;
                        break;

                }

        }

        return TRUE;

} /*DecodeLine*/


/************************************************************************/
/*                                                                      */
/*      EncodeByte -- Provide printable encoding for byte               */
/*                                                                      */
/*      Encoding will be typically two to no more than six bytes.       */
/*                                                                      */
/************************************************************************/
public_scope void
Bakslash_EncodeByte(BYTE b, CHAR *pEncoding)
{
        /*Select encoding according to value*/
        switch (b) {
        case NUL:
                strcpy(pEncoding, "\\0");
                break;

        case BEL:
                strcpy(pEncoding, "\\a");
                break;

        case BS:
                strcpy(pEncoding, "\\b");
                break;

        case FF:
                strcpy(pEncoding, "\\f");
                break;

        case CR:
                strcpy(pEncoding, "\\r");
                break;

        case LF:
                strcpy(pEncoding, "\\n");
                break;

        case TAB:
                strcpy(pEncoding, "\\t");
                break;

        case VT:
                strcpy(pEncoding, "\\v");
                break;

        case '\\':
                strcpy(pEncoding, "\\\\");
                break;

        case '"':
                strcpy(pEncoding, "\\\"");
                break;

        case '\'':
                strcpy(pEncoding, "\\'");
                break;

        default:
                /*Is the character printable?*/
                if (isprint(b)) {
                        /*Yes, report it as-is*/
                        sprintf(pEncoding, "%c", b);
                        break;
                }

                /*Report character as a \x escape*/
                sprintf(pEncoding, "\\x%02x", b);
                break;

        }

} /*EncodeByte*/


/************************************************************************/
/*                                                                      */
/*      Init -- Prepare module for operation                            */
/*                                                                      */
/************************************************************************/
public_scope void
Bakslash_Init(void)
{
        /*No initialisation required*/

} /*Init*/


