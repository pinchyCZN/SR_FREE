/************************************************************************/
/*                                                                      */
/*      Main -- Functions to share by main program code                 */
/*                                                                      */
/*      Although the program return code is usually selected by the     */
/*      main routine, other modules may wish to signal faults           */
/*      to be reported in the return code, but otherwise wish to        */
/*      keep operating.  This module provides an interface for other    */
/*      modules to report codes to the main program.                    */
/*                                                                      */
/*      Copyright (C) 1995-2000 Grouse Software.  All rights reserved.  */
/*      Written for Grouse by behoffski (Brenton Hoff).                 */
/*                                                                      */
/*      Free software: no warranty; use anywhere is ok; spread the      */
/*      sources; note any mods; share variations and derivatives        */
/*      (including sending to behoffski@grouse.com.au).                 */
/*                                                                      */
/************************************************************************/

#ifndef MAIN_H
#define MAIN_H

#include <compdef.h>

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/

/*Program return codes*/

#define MAIN_RETURN_MATCHED                     0
#define MAIN_RETURN_NOMATCH                     1
#define MAIN_RETURN_FAULT                       2

/************************************************************************/
/*                                                                      */
/*      ReturnCode -- Allow other modules to write program return       */
/*                                                                      */
/*      Other modules may find faults during operation but be able      */
/*      keep going.  These modules must be able to report the faults    */
/*      to the main program so that the program return code can         */
/*      be updated correctly.                                           */
/*                                                                      */
/************************************************************************/
void
Main_ReturnCode(UINT Code);

#ifdef __cplusplus
}
#endif /*__cplusplus*/

#endif /*MAIN_H*/
