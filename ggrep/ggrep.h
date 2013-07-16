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
/*      Copyright (C) 1995-2000 Grouse Software.  All rights reserved.  */
/*      Written for Grouse by behoffski (Brenton Hoff).                 */
/*                                                                      */
/*      Free software: no warranty; use anywhere is ok; spread the      */
/*      sources; note any mods; share variations and derivatives        */
/*      (including sending to behoffski@grouse.com.au).                 */
/*                                                                      */
/************************************************************************/

#ifndef GGREP_H
#define GGREP_H

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
GGrep_Init(void);

/************************************************************************/
/*                                                                      */
/*      main -- Co-ordinate execution of program                        */
/*                                                                      */
/************************************************************************/
INT
GGrep_main(UINT argc, CHAR **argv);

#ifdef __cplusplus
}
#endif /*__cplusplus*/

#endif /*GGREP_H*/
