/************************************************************************/
/*                                                                      */
/*      TblDisp -- Describe state tables in concise, readable format    */
/*                                                                      */
/*      This module presents the state tables created for a             */
/*      search in a human-readable format.  This is useful for          */
/*      understanding, debugging and testing the software.              */
/*                                                                      */
/*      Describe was created very late in the development --            */
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

#ifndef TBLDISP_H
#define TBLDISP_H

#include <compdef.h>
#include "matcheng.h"

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/


/************************************************************************/
/*                                                                      */
/*      Init -- Prepare module for operation                            */
/*                                                                      */
/************************************************************************/
void
TblDisp_Init(void);


/************************************************************************/
/*                                                                      */
/*      Start -- Begin managing what has to be managed                  */
/*                                                                      */
/************************************************************************/
BOOL
TblDisp_Start(void);


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
void 
TblDisp_PrepareLabels(void);


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
void
TblDisp_Describe(MatchEng_Spec *pSpec, CHAR *pTitle);


#endif /*TBLDISP_H*/
