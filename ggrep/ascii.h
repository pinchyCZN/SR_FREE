/************************************************************************/
/*                                                                      */
/*      ASCII -- Give common names for ASCII byte values                */
/*                                                                      */
/*      This module provides widely-recognised names for common         */
/*      character constants found in the code.                          */
/*                                                                      */
/*      Written by Brenton Hoff of Grouse Software.                     */
/*                                                                      */
/************************************************************************/

#ifndef ASCII_H
#define ASCII_H

#include <compdef.h>

/*These escape sequences are defined by C for all character sets*/

#define NUL                             '\0'
#define BS                              '\b'
#define FF                              '\f'
#define LF                              '\n'
#define CR                              '\r'
#define TAB                             '\t'
#define HT                              '\t'

/*Escape sequences added by ANSI C but not in classic C*/

#define BEL                             '\a'
#define VT                              '\v'

/*ASCII codes not directly supported by the compiler*/

#define ASCII_SUB                       '\032' /*decimal 26 (CTRL/Z)*/

/*Note also that ANSI C has \xhh; C++ has \xhhhhh...; classic C has neither*/

#endif /*ASCII_H*/
