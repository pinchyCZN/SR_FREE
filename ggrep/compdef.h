/************************************************************************/
/*                                                                      */
/*      CompDef -- Extend compiler environment                          */
/*                                                                      */
/*      This header extends the environment provided by the compiler    */
/*      to assist portable, readable and correct coding by:             */
/*      - isolating data types from different compiler environments     */
/*      - providing simple helper macros for common transformations     */
/*                                                                      */
/*      Copyright (C) 1995-2000 Grouse Software.  All rights reserved.  */
/*      Written for Grouse by behoffski (Brenton Hoff).                 */
/*                                                                      */
/*      Free software: no warranty; use anywhere is ok; spread the      */
/*      sources; note any mods; share variations and derivatives        */
/*      (including sending to behoffski@grouse.com.au).                 */
/*                                                                      */
/************************************************************************/

#ifndef COMPDEF_H
#define COMPDEF_H

/*Boolean enumeration values*/

#ifndef FALSE
#define FALSE                           0
#endif /*FALSE*/
#ifndef TRUE
#define TRUE                            1
#endif /*TRUE*/

/*Standard pointers -- Both should cause a system exception if dereferenced*/
/*NULL is expected as end-of-list marker: NIL is for invalid pointers*/

#ifndef NULL
#define NULL                            ((void *) 0)
#endif /*NULL*/

#ifndef NIL
#define NIL                             ((void *) 0xeeeeeeeeuL)
#endif /*NIL*/

/*The null-function definition is somewhat tacky, oh well...*/

#ifndef NULLFUNC
#define NULLFUNC                        0
#endif /*NULLFUNC*/

/*General, high-performance types: at least 16 bits wide*/

typedef signed int                      BOOL;
typedef signed int                      INT;
typedef unsigned int                    UINT;

/*Fixed-width types that do not have two's-complement arithmetic capability*/
/*WORD/LWORD as 16/32 bits is a bit tacky, but what's better?  BI_OCTET????*/
/*(Size is a storage optimisation hint only: may be larger if needed)*/

typedef unsigned char                   BOOL8;
typedef char                            CHAR;   /*Any sign,  8 bits*/
typedef signed char                     SCHAR;  /*Signed, 8 bits*/
typedef unsigned char                   UCHAR;  /*Unsigned, 8 bits*/
typedef unsigned char                   BYTE;   /*Unsigned,  8 bits*/
typedef unsigned short                  WORD;   /*Unsigned, 16 bits*/
typedef unsigned long int               LWORD;  /*Unsigned, 32 bits*/

/*Fixed-width types with two's complement arithmetic included*/
/*(Size is a storage optimisation hint only: may be larger if needed)*/

typedef signed char                     INT8;
typedef unsigned char                   UINT8;
typedef signed short                    INT16;
typedef unsigned short                  UINT16;

//typedef signed long int                 INT32;
//typedef unsigned long int               UINT32;


/*Fixed-width types but with implementation trickery based on size*/
/*(Implementing these types with more bits will lead to incorrect results)*/

typedef unsigned char                   BOOL8T;
typedef char                            CHART;
typedef signed char                     SCHART;
typedef unsigned char                   UCHART;
typedef unsigned char                   BYTET;
typedef unsigned short                  WORDT;
typedef unsigned long int               LWORDT;

typedef signed char                     INT8T;
typedef unsigned char                   UINT8T;
typedef signed short                    INT16T;
typedef unsigned short                  UINT16T;
typedef signed long int                 INT32T;
typedef unsigned long int               UINT32T;

/*Some common byte/word transformations*/

#define LOBYTE(w)                       ((BYTE)          (w)            )
#define HIBYTE(w)                       ((BYTE)  (((WORD)(w)) >> 8)     )

#define LOWORD(l)                       ((WORD)          (l)            )
#define HIWORD(l)                       ((WORD) (((LWORD)(l)) >> 16)    )

#define BYTES2WORD(a, b)                (        (((WORD)(a)) <<  8) + \
                                                         (b)            )

#define BYTES2LWORD(a, b, c, d)         (       (((LWORD)(a)) << 24) + \
                                                (((LWORD)(b)) << 16) + \
                                                (((LWORD)(c)) <<  8) + \
                                                         (d)            )

/*Other helper macros*/

#define DIM(a) (sizeof(a)/sizeof((a)[0]))

/*Bit field labels for readability*/

#define BIT0                                0x0001u
#define BIT1                                0x0002u
#define BIT2                                0x0004u
#define BIT3                                0x0008u
#define BIT4                                0x0010u
#define BIT5                                0x0020u
#define BIT6                                0x0040u
#define BIT7                                0x0080u
#define BIT8                                0x0100u
#define BIT9                                0x0200u
#define BIT10                               0x0400u
#define BIT11                               0x0800u
#define BIT12                               0x1000u
#define BIT13                               0x2000u
#define BIT14                               0x4000u
#define BIT15                               0x8000u
#define BIT16                           0x00010000uL
#define BIT17                           0x00020000uL
#define BIT18                           0x00040000uL
#define BIT19                           0x00080000uL
#define BIT20                           0x00100000uL
#define BIT21                           0x00200000uL
#define BIT22                           0x00400000uL
#define BIT23                           0x00800000uL
#define BIT24                           0x01000000uL
#define BIT25                           0x02000000uL
#define BIT26                           0x04000000uL
#define BIT27                           0x08000000uL
#define BIT28                           0x10000000uL
#define BIT29                           0x20000000uL
#define BIT30                           0x40000000uL
#define BIT31                           0x80000000uL

/*Specifiers to clarify local/global scope of symbols in implementations*/
/*(really wanted "private" and "public" but they collide with others)*/
/*(maybe I should have used "invisible" and "visible"?)*/
/*(the names chosen are rather horrible -- deliberately)*/

#define module_scope                    static
#define public_scope

/*?? Definition of PAGE_ALIGNED removed from here: Is this wise?*/

#endif /*COMPDEF_H*/
