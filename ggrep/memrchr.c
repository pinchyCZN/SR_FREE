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

#include "memrchr.h"
#include <string.h>

void * 
memrchr(const void *buf, int c, size_t num)
{
        unsigned char *pMem = (unsigned char *) buf;

        for (;;) {
                if (num-- == 0) {
                        return NULL;
                }

                if (*pMem-- == (unsigned char) c) {
                        break;
                }

        }

        return (void *) (pMem + 1);

}
