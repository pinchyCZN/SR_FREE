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
/*      Copyright (C) Grouse Software 1995-9.  All rights reserved.     */
/*      Written for Grouse by behoffski (Brenton Hoff).                 */
/*                                                                      */
/*      Free software: no warranty; use anywhere is ok; spread the      */
/*      sources; note any mods; share variations and derivatives        */
/*      (including sending to behoffski@grouse.com.au).                 */
/*                                                                      */
/************************************************************************/

#ifndef BAKSLASH_H
#define BAKSLASH_H

#include <compdef.h>

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/

/************************************************************************/
/*                                                                      */
/*      Init -- Prepare module for operation                            */
/*                                                                      */
/************************************************************************/
void
Bakslash_Init(void);


/************************************************************************/
/*                                                                      */
/*      Decode -- Convert text containing \ escapes to binary           */
/*                                                                      */
/************************************************************************/
BYTE
Bakslash_Decode(CHAR **ppLine);


/************************************************************************/
/*                                                                      */
/*      EncodeByte -- Provide printable encoding for byte               */
/*                                                                      */
/*      Encoding will be typically four to no more than eight bytes.    */
/*                                                                      */
/************************************************************************/
void
Bakslash_EncodeByte(BYTE b, CHAR *pEncoding);


/************************************************************************/
/*                                                                      */
/*      DecodeLine -- Edit line to replace escape seq's                 */
/*                                                                      */
/*      Returns FALSE if the decoding was abandoned for any reason.     */
/*                                                                      */
/************************************************************************/
BOOL
Bakslash_DecodeLine(CHAR *pLine);

#ifdef __cplusplus
}
#endif /*__cplusplus*/

#endif /*BAKSLASH_H*/
