/************************************************************************/
/*                                                                      */
/*      memrchr -- Fast, right-to-left-scanning memory search           */
/*                                                                      */
/*      ANSI C provides memchr, which scans memory using ascending      */
/*      addresses (to the right), starting from the specified           */
/*      address.  However, there's no equivalent descending-address     */
/*      (right to left) function -- hence this module.                  */
/*                                                                      */
/*      Copyright (C) 1995-2000 Grouse Software.  All rights reserved.  */
/*      Written for Grouse by behoffski (Brenton Hoff).                 */
/*                                                                      */
/*      Free software: no warranty; use anywhere is ok; spread the      */
/*      sources; note any mods; share variations and derivatives        */
/*      (including sending to behoffski@grouse.com.au).                 */
/*                                                                      */
/************************************************************************/

#ifndef MEMRCHR_H
#define MEMRCHR_H

#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/

/************************************************************************/
/*                                                                      */
/*      memrchr -- Find last occurrence of byte in buffer               */
/*                                                                      */
/*      Scans the memory block and reports the last occurrence of       */
/*      the specified byte in the buffer.  Returns a pointer to         */
/*      the byte if found, or NULL if not found.                        */
/*                                                                      */
/************************************************************************/
void * 
memrchr(const void *buf, int c, size_t num);

#ifdef __cplusplus
}
#endif /*__cplusplus*/

#endif /*MEMRCHR_H*/
