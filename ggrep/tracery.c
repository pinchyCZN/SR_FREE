/************************************************************************/
/*                                                                      */
/*      Tracery -- Allow access to module trace facilities              */
/*                                                                      */
/*      This (slightly misnamed) module helps greatly in the            */
/*      creation of reusable software.  It collects the trace           */
/*      facilities defined by each module into a coherent               */
/*      whole that may be easily manipulated by outsiders --            */
/*      typically via command-line switches.                            */
/*                                                                      */
/*      The basic idea is that each module defines one or more          */
/*      ways of tracing its behaviour and/or reporting data             */
/*      during operation.  The tracing "views" that each module         */
/*      defines are totally independent -- there is no external         */
/*      framework imposed by any outsider.  This is essential           */
/*      as no conceivable pre-defined framework could                   */
/*      adequately cover the vast range of software modules             */
/*      in existence.  Having a set of independent views is             */
/*      more powerful and expressive than having a hierarchy of         */
/*      trace levels, as the relative importance of the trace           */
/*      information may be different for the creator and the            */
/*      user.  In addition, the trace selection in a level-             */
/*      oriented scheme is usually very crude: if you enable            */
/*      "Level 3" tracing, you usually get "Level 1" and                */
/*      "Level 2" traces, whether you want them or not.                 */
/*                                                                      */
/*      Capturing these views within the module implementation          */
/*      makes the tracing more powerful than traces generated           */
/*      by debuggers.  This is partially because the designer's         */
/*      intent is captured, and partially because the traces            */
/*      can be tailored in ways that may be awkward for the             */
/*      debugger.  This reduces the wasteful (and error-prone)          */
/*      reengineering that would otherwise be incurred by the           */
/*      maintainer and/or user.  The reengineering saved is:            */
/*          - deciding where and when to observe the system,            */
/*          - creation of an accurate model of the system, and          */
/*          - specification of monitor activities to the debugger.      */
/*                                                                      */
/*      Another benefit of capturing the traces in the module           */
/*      is that the trace code probably is more portable than           */
/*      traces written for a specific debugger.                         */
/*                                                                      */
/*      Another part of the design is that the trace flags              */
/*      enable or disable selected blocks of code -- they do            */
/*      not concern themselves with the processing required             */
/*      to generate the traces, nor do they proscribe how the           */
/*      trace is to be presented.  This allows the traces to            */
/*      be far less invasive when present.  In addition, each           */
/*      trace is defined using a macro, so the entire trace             */
/*      code may be excluded from the compilation simply by             */
/*      defining the macro(s) to expand to nothing.                     */
/*                                                                      */
/*      The trace flags themselves are implemented as simply as         */
/*      possible: each trace flag register is a (32-bit)                */
/*      longword, and each view is assigned one bit within the          */
/*      register.  Testing whether a trace is to be generated           */
/*      is merely seeing if any of the views associated with            */
/*      that trace have bits set in the trace flags register --         */
/*      typically 3-5 instructions per trace test.  Defining            */
/*      the register storage is the responsibility of the               */
/*      module -- typically each instantiation of each object           */
/*      in the system will have its own trace flags register.           */
/*                                                                      */
/*      Once all the views and trace flags have been defined,           */
/*      the one remaining management problem is providing               */
/*      access to all the flag registers so that outsiders may          */
/*      operate the trace facility.  This is where this module          */
/*      comes in: It provides a place for details of all the            */
/*      flags, together with simple instructions for setting            */
/*      and clearing flag bits, to be collected together and            */
/*      operated via a uniform interface.  The modules and              */
/*      objects don't contain any code to set/clear flag bits --        */
/*      they merely provide means for the address of each               */
/*      flag register to be obtained, and details of simple             */
/*      mapping between ASCII characters and flag bits.                 */
/*                                                                      */
/*      The main danger of explicitly writing traces in the             */
/*      sources is that some code may be incorrectly placed             */
/*      in the trace block which is needed when trace statements        */
/*      aren't invoked.  This can only be found by test and             */
/*      and inspection, or avoided by reusing trustworthy               */
/*      code.  Hopefully the increase in reuse will offset              */
/*      this risk.                                                      */
/*                                                                      */
/*      So this module, Tracery, collects all the flag register         */
/*      descriptions, together with the name of each flag               */
/*      register, then manipulates flags based on commands              */
/*      from external sources (typically via debug directives           */
/*      on the command line found by the main program).                 */
/*                                                                      */
/*      Note that each module does not register itself -- this          */
/*      is left to the main program to do.  This is so that as          */
/*      modules are reused for different jobs and in different          */
/*      applications, the name used can be chosen to be relevant        */
/*      to the job at hand, and namespace collisions can be             */
/*      avoided.  In addition, the registration requires                */
/*      merely that the module advertise a link function to             */
/*      be handed to Tracery.  This design allows the platform          */
/*      to hook all the relevant bits of the system together            */
/*      without needing to know about unnecessary details.              */
/*                                                                      */
/*      Specifying flag names and edits is done by a very simple        */
/*      list of name/mask/set specifications.                           */
/*                                                                      */
/*      The classification implied by the bits selecting each           */
/*      piece of trace code are often worth reporting in their          */
/*      own right.  If all else fails, start tracing EVERYTHING         */
/*      and then use a worthwhile grep(!) to scan the output for        */
/*      patterns.  Tracery caters for this by defining the              */
/*      variable TRACERY_FLAGS each time a block of code is             */
/*      expanded.  This variable contains all the flags marking         */
/*      that block -- not merely the bits that matched the              */
/*      flag register.  This variable can either be reported            */
/*      as a number, or the block can use Decode and Name               */
/*      to report the flags and the traced object in human-             */
/*      readable form.  Some GCC-specific incantations have been        */
/*      used, however, to stop warnings if the variable is unused.      */
/*                                                                      */
/*      Copyright (C) 1999-2000 Grouse Software.  All rights reserved.  */
/*      Written for Grouse by behoffski (Brenton Hoff).                 */
/*                                                                      */
/*      Free software: no warranty; use anywhere is ok; spread the      */
/*      sources; note any mods; share variations and derivatives        */
/*      (including sending to behoffski@grouse.com.au).                 */
/*                                                                      */
/************************************************************************/

#include <compdef.h>
#include <fnmatch.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tracery.h"

#define TRACERY_DISPLAY_COLUMN          16
#define TRACERY_DISPLAY_RIGHT_MARGIN    77

/*Storage for individual registrations*/

typedef struct Tracery_StructType {
        /*Link to next member of list*/
        struct Tracery_StructType *pNext;

        /*Information provided upon registration*/
        CHAR *pName;
        UINT NameLength;
        CHAR *pDescription;
        Tracery_ObjectInfo *pClientInfo;
        LWORD DefaultFlags;
        Tracery_EditEntry *pEditList;

} Tracery_Entry;

/*Module-wide globals*/

typedef struct {
        /*Dummy entry serving as list head*/
        Tracery_Entry Head;

} TRACERY_MODULE_CONTEXT;

module_scope TRACERY_MODULE_CONTEXT gTracery;

/************************************************************************/
/*                                                                      */
/*      Init -- Prepare module for operation                            */
/*                                                                      */
/************************************************************************/
public_scope void
Tracery_Init(void)
{
        /*No members, yet*/
        gTracery.Head.pNext = NULL;

} /*Init*/


/************************************************************************/
/*                                                                      */
/*      Register -- Receive flags register and manipulation details     */
/*                                                                      */
/*      This function collects the details needed to operate the        */
/*      trace flags for the specified entity.  It returns FALSE         */
/*      if unable to accept the entry.  Once registration is            */
/*      accepted, the flag register may be modified at any time         */
/*      by any party calling Tracery (until the flag is                 */
/*      deregistered).  The text referenced by pName, and the           */
/*      bit editing list named by pEditList, needs to be                */
/*      maintained by the caller: this module merely remembers          */
/*      the pointers without bothering to make a private copy.          */
/*                                                                      */
/*      The edit list must be terminated by TRACERY_EDIT_LIST_END.      */
/*      An example of calling Register might look a bit like            */
/*      the following:                                                  */
/*                                                                      */
/*      static struct Tracery_EditEntry Image_EditDefs[] = {            */
/*               {"w",    BIT15, 0x00,  "Ignore warnings"}.             */
/*               {"W",    BIT15, BIT15  "Trace  warnings"}.             */
/*               {"f",    BIT14, 0x00,  "Ignore failures"}.             */
/*               {"F",    BIT14, BIT14, "Trace  failures"}.             */
/*               {"m",    BIT13, 0x00,  "Ignore memory"}.               */
/*               {"M",    BIT13, BIT13, "Trace  memory"}.               */
/*               {"c",    BIT12, 0x00,  "Ignore calls"}.                */
/*               {"C",    BIT12, BIT12, "Trace  calls"}.                */
/*               {"q",    BIT0,  0x00,  "Ignore quadtree"}.             */
/*               {"Q",    BIT0,  BIT0,  "Trace  quadtree"}.             */
/*               {"l",    BIT1,  0x00,  "Ignore linked list"}.          */
/*               {"L",    BIT1,  BIT1,  "Trace  linked list"}.          */
/*               {"g",    BIT2,  0x00,  "Ignore corner stitching"}.     */
/*               {"G",    BIT2,  BIT2,  "Trace  corner stitching"}.     */
/*               {"Tx",   BIT3,  BIT3,  "Trace  network Tx"}.           */
/*               {"noTx", BIT3,  0x00,  "Ignore network Tx"}.           */
/*               {"Rx",   BIT4,  BIT4,  "Trace  network Rx"}.           */
/*               {"noRx", BIT4,  0x00,  "Ignore network Rx"}.           */
/*               TRACERY_EDIT_LIST_END                                  */
/*      };                                                              */
/*                                                                      */
/*      Although the details shown above need to be given to            */
/*      Tracery, we don't want to burden the outside party              */
/*      in charge of the registrations with too much detail.            */
/*      So, the registration below merely takes an object               */
/*      pointer and a registration function.  The object pointer        */
/*      allows the environment to register traces for either            */
/*      an entire module (pObject == NULL), or for a single             */
/*      object created by that module.                                  */
/*                                                                      */
/*      Tracery_Register("ps", "Pixel Server",                          */
/*                       NULL, Image_TraceryLink);                      */
/*                                                                      */
/************************************************************************/
public_scope BOOL
Tracery_Register(char *pName, 
                 CHAR *pDescription, 
                 void *pObject, 
                 Tracery_RegistrationFn *pRegFn)
{
        Tracery_Entry *pSearch;
        Tracery_Entry *pNew;

        /*Start search at head of list*/
        pSearch = &gTracery.Head;

        /*Look through list for correct insertion point*/
        for (;;) {
                /*Have we exhausted the list?*/
                if (pSearch->pNext == NULL) {
                        /*Yes, add item to the end*/
                        break;
                }

                /*Is this name already present in the list?*/
                if (strcmp(pName, pSearch->pNext->pName) == 0) {
                        /*Yes, merely update entry with provided details*/
                        pSearch = pSearch->pNext;
                        pSearch->pDescription = pDescription;
                        if (! pRegFn(pObject, 
                                     TRACERY_REGCMD_GET_INFO_BLOCK, 
                                     &pSearch->pClientInfo)) {
                                /*Sorry, can't update if no client info*/
                                return FALSE;
                        }
                        pRegFn(pObject, 
                               TRACERY_REGCMD_GET_DEFAULT_FLAGS, 
                               &pSearch->DefaultFlags);
                        pRegFn(pObject, 
                               TRACERY_REGCMD_GET_EDIT_LIST, 
                               &pSearch->pEditList);

                        return TRUE;

                }

                /*Move to the next entry of the list*/
                pSearch = pSearch->pNext;

        }

        /*Create an entry to store this registration*/
        pNew = (Tracery_Entry *) malloc(sizeof(Tracery_Entry));
        if (pNew == NULL) {
                /*Sorry, unable to create entry*/
                return FALSE;
        }
        pNew->pName         = pName;
        pNew->NameLength    = strlen(pName);
        pNew->pDescription  = pDescription;

        /*Now work with registration function to find object's details*/
        if (! pRegFn(pObject, 
                     TRACERY_REGCMD_GET_INFO_BLOCK, 
                     &pNew->pClientInfo)) {
                /*Sorry, can't update if no client info*/
                return FALSE;
        }
        pRegFn(pObject, 
               TRACERY_REGCMD_GET_DEFAULT_FLAGS, 
               &pNew->DefaultFlags);
        pRegFn(pObject, 
               TRACERY_REGCMD_GET_EDIT_LIST, 
               &pNew->pEditList);

        /*Write cross-reference so we can quickly find ourself when asked*/
        pNew->pClientInfo->pTraceryRef = pNew;

        /*Insert new entry after entry found by search*/
        pNew->pNext = pSearch->pNext;
        pSearch->pNext = pNew;

        /*Registered successfully*/
        return TRUE;

} /*Register*/


/************************************************************************/
/*                                                                      */
/*      EditFlags -- Handle detailed flag edit specification            */
/*                                                                      */
/*      Edits the entry's flag register according to the edit           */
/*      specification given.  pAfterEnd points to the next              */
/*      character after the end of the specification.                   */
/*                                                                      */
/************************************************************************/
module_scope void
Tracery_EditFlags(Tracery_Entry *pEntry, CHAR *pSpec, CHAR *pAfterEnd)
{
        Tracery_EditEntry *pEdits;
        LWORD Flags = pEntry->pClientInfo->Flags;

        while (pSpec < pAfterEnd) {
                /*Search for specification in edit list*/
                pEdits = pEntry->pEditList;
                for (;;) {
                        /*Have we reached the end of the list?*/
                        if (pEdits->pName == NULL) {
                                /*Yes, nothing matched: try next position*/
                                pSpec++;
                                break;
                        }

                        /*Does this entry apply?*/
                        if (strncmp(pSpec, 
                                    pEdits->pName, 
                                    strlen(pEdits->pName)) == 0) {
                                /*Yes, edit module-local flags*/
                                Flags &= ~ pEdits->MaskBits;
                                Flags |=   pEdits->SetBits;

                                /*Skip past specification*/
                                pSpec += strlen(pEdits->pName);
                                break;
                        }

                        /*Move to next entry, if any*/
                        pEdits++;

                }
                
        }

        /*Write edited flag settings back into client's register*/
        pEntry->pClientInfo->Flags = Flags;

} /*EditFlags*/


/************************************************************************/
/*                                                                      */
/*      Configure -- Manipulate flags based on configuration            */
/*                                                                      */
/*      This is the main interface used by text-driven interfaces,      */
/*      including command-line switches.  It expects to see a list      */
/*      of trace module names, separated by commas.  The names          */
/*      may be modified in a couple of ways; the meanings are:          */
/*           name        -- Turn on  default trace flags for object     */
/*          +name        -- Turn on  all     trace flags for object     */
/*          -name        -- Turn off all     trace flags for object     */
/*           name(Chars) -- Set flags based on on/off chars             */
/*                                                                      */
/*      Returns FALSE if the configuration string didn't make sense.    */
/*                                                                      */
/************************************************************************/
public_scope BOOL
Tracery_Configure(CHAR *pConfiguration)
{
        Tracery_Entry *pSearch;
        BOOL Select;
        BOOL Invert;
        CHAR *pSpecEnd;
        CHAR *pAfterName;
        CHAR Delim;
        CHAR *pCopiedConfig;
        CHAR *pConfig;

        /*Copy string to private memory as we edit it during parsing*/
        pCopiedConfig = strdup(pConfiguration);
        if (pCopiedConfig == NULL) {
                /*Sorry, not enough memory to copy config string*/
                return FALSE;
        }
        pConfig = pCopiedConfig;

        /*Work through configuration text*/
        for (;;) {
                /*Skip over separators and white space, if any*/
                while ((*pConfig == ',') || 
                       (*pConfig == ' ')) {
                        pConfig++;
                }

                /*Does the next name start with '-'?*/
                Invert = FALSE;
                if (*pConfig == '-') {
                        /*Yes, user wants to defeat all flags*/
                        Invert = TRUE;
                        pConfig++;
                }

                /*Have we reached the end of the config string?*/
                if (*pConfig == '\0') {
                        /*Yes, finished processing*/
                        break;
                }

                /*Find the end of the name*/
                pAfterName = pConfig;
                while ((*pAfterName != ',') && 
                       (*pAfterName != '\0') && 
                       (*pAfterName != '(') && 
                       (*pAfterName != ' ')) {
                        pAfterName++;
                }

                /*Remember name delimiter and replace it with NUL terminator*/
                Delim = *pAfterName;
                *pAfterName++ = '\0';

                /*Look for the facility name in the list*/
                for (pSearch = gTracery.Head.pNext; 
                                pSearch != NULL; 
                                pSearch = pSearch->pNext) {
                        /*Does this name match the specified facility?*/
                        if (fnmatch(pConfig, 
                                    pSearch->pName, 
                                    FNM_PERIOD) != 0) {
                                /*No, move to the next name, if any*/
                                /*?? We ignore errors for now*/
                                continue;
                        }

                        /*Matched name: are more detailed specifiers given?*/
                        Select = FALSE;
                        if (Delim == '(') {
                                Select = TRUE;
                        }
                        
                        /*Apply edits as specified*/
                        if ((! Select) && (! Invert)) {
                                /*No selection: enable EVERYTHING*/
                                pSearch->pClientInfo->Flags = ~0uL;
                                
                        } else if ((! Select) && (Invert)) {
                                /*Module disabled: Revert to global*/
                                pSearch->pClientInfo->Flags = 0;
                                
                        } else {
                                /*Find the extent of the detailed selection*/
                                pSpecEnd = strstr(pAfterName, ")");
                                if (pSpecEnd == NULL) {
                                        /*Sorry, badly-formed spec*/
                                        free(pCopiedConfig);
                                        return FALSE;
                                }

                                /*Handle more detailed spec*/
                                Tracery_EditFlags(pSearch, 
                                                  pAfterName, 
                                                  pSpecEnd);

                        }

                }

                /*Note: We don't complain if no-one matches spec*/
                /* -- this might be changed if it's too confusing*/

                /*Reinstate character delimiting name*/
                *--pAfterName = Delim;
                pConfig = pAfterName;

                /*Move to the next component of the list, if any*/
                pConfig = pAfterName;
                while ((*pConfig != '\0') && 
                       (*pConfig != ',')) {
                        pConfig++;
                }

                /*Did we hit the end of the list?*/
                if (*pConfig == '\0') {
                        /*Yes, finished specifier*/
                        break;
                }

        }

        /*Finished implementing configuration*/
        free(pCopiedConfig);
        return TRUE;

} /*Configure*/


/************************************************************************/
/*                                                                      */
/*      Deregister -- Cancel module registration                        */
/*                                                                      */
/*      Removes the specified flag register from the list               */
/*      of registers able to be manipulated by this facility.           */
/*      Once deregistration is complete, this module will not           */
/*      generate any more references to the flag register.              */
/*      Always remember to deregister as otherwise dangling             */
/*      references may cause havoc!                                     */
/*                                                                      */
/************************************************************************/
public_scope void
Tracery_Deregister(Tracery_ObjectInfo *pInfoBlock)
{
        /*Not implemented yet*/

} /*Deregister*/


/************************************************************************/
/*                                                                      */
/*      Name -- Report the name associated with a flag register         */
/*                                                                      */
/*      Returns a pointer to the name of the object associated          */
/*      with the specified flag register, or a pointer to the           */
/*      string "(?object)" if the flag register isn't known.            */
/*      Used where trace action code wishes to report information       */
/*      using the name assigned to the object or module at              */
/*      run time.                                                       */
/*                                                                      */
/************************************************************************/
public_scope char *
Tracery_Name(Tracery_ObjectInfo *pInfoBlock)
{
        Tracery_Entry *pEntry;

        /*Find internal entry assoiated with specified block*/
        pEntry = (Tracery_Entry *) pInfoBlock->pTraceryRef;

        /*Report object's name obtained when object was registered*/
        return pEntry->pName;

} /*Name*/


/************************************************************************/
/*                                                                      */
/*      Decode -- Convert flag bits into text specifiers                */
/*                                                                      */
/*      This function finds the list of text specifiers                 */
/*      associated with the specified flag register, and then           */
/*      uses the specifications to decode the flag bits.                */
/*      This can be used to display the bits associated with            */
/*      a trace message in a human-readable format.                     */
/*                                                                      */
/*      Returns FALSE if unable to fit all the decoded specs            */
/*      into the supplied string.                                       */
/*                                                                      */
/************************************************************************/
public_scope BOOL
Tracery_Decode(Tracery_ObjectInfo *pInfoBlock, 
               LWORD FlagBits, CHAR *pString, UINT StringSize)
{
        Tracery_Entry *pEntry;
        Tracery_EditEntry *pEdits;
        UINT NameLen;
        BOOL SpaceOkay = TRUE;
        CHAR TempBuf[12];
        LWORD ReportedBits = 0;

        /*Find internal entry assoiated with specified block*/
        pEntry = (Tracery_Entry *) pInfoBlock->pTraceryRef;

        /*Now loop through edit specifications, reporting what's set*/
        for (pEdits = pEntry->pEditList; pEdits->pName != NULL; pEdits++) {
                /*Is this option set?*/
                if ((FlagBits & pEdits->MaskBits) == pEdits->SetBits) {
                        /*Yes, is space available to report it?*/
                        NameLen = strlen(pEdits->pName);
                        if (StringSize > NameLen) {
                                /*Yes, add it to string*/
                                ReportedBits |= pEdits->MaskBits;
                                strcpy(pString, pEdits->pName);
                                pString += NameLen;
                                StringSize -= NameLen;

                        } else {
                                /*Sorry, item didn't fit*/
                                SpaceOkay = FALSE;
                        }
                }

        }

        /*Do we have any bits unreported?*/
        FlagBits &= ~ReportedBits;
        if (FlagBits != 0) {
                /*Yes, format them for output as a hex number*/
                sprintf(TempBuf, "+0x%08lx", FlagBits);
                if (StringSize > strlen(TempBuf)) {
                        /*Space is available, add number to string*/
                        strcpy(pString, TempBuf);
                        pString += strlen(TempBuf);
                        StringSize -= strlen(TempBuf);
                } else {
                        /*Sorry, didn't fit*/
                        SpaceOkay = FALSE;
                }
        }

        /*Report whether we decoded everything correctly*/
        return SpaceOkay;

} /*Decode*/


/************************************************************************/
/*                                                                      */
/*      DisplayObjectBrief -- Show brief details of object              */
/*                                                                      */
/*      Prints out the object's name and lists its edit specifiers.     */
/*                                                                      */
/************************************************************************/
module_scope void
Tracery_DisplayObjectBrief(Tracery_Entry *pEntry)
{
        Tracery_EditEntry *pEdits;
        UINT Col;

        /*Display name*/
        printf("%*s", -TRACERY_DISPLAY_COLUMN, pEntry->pName);
        if (strlen(pEntry->pName) >= (TRACERY_DISPLAY_COLUMN - 1)) {
                /*Long name, wrap description on to next line*/
                printf("\n%*s", TRACERY_DISPLAY_COLUMN, "");
        }

        Col = TRACERY_DISPLAY_COLUMN;

        /*Display list of edit specs associated with object*/
        for (pEdits = pEntry->pEditList; pEdits->pName != NULL; pEdits++) {
                /*Are there items already on the line?*/
                if (Col > TRACERY_DISPLAY_COLUMN) {
                        /*Yes, add a list separator*/
                        printf(", ");
                        Col += 2;
                }

                /*Would this item overrun the right margin?*/
                if (((Col + strlen(pEdits->pName)) > 
                     TRACERY_DISPLAY_RIGHT_MARGIN) && 
                    (Col > TRACERY_DISPLAY_COLUMN)) {
                        /*Yes, break line now*/
                        printf("\n%*s", -TRACERY_DISPLAY_COLUMN, "");
                        Col = TRACERY_DISPLAY_COLUMN;
                }

                /*Add item to the display*/
                printf("%s", pEdits->pName);
                Col += strlen(pEdits->pName);

        }

        printf("\n");

} /*DisplayObjectBrief*/


/************************************************************************/
/*                                                                      */
/*      DisplayObjectFull -- Show full details of object                */
/*                                                                      */
/*      Prints out a lot of information about the object's options      */
/*      and its current state.                                          */
/*                                                                      */
/************************************************************************/
module_scope void
Tracery_DisplayObjectFull(Tracery_Entry *pEntry)
{
        Tracery_EditEntry *pEdits;
        CHAR DecodeBuf[4096];

        /*Display name*/
        printf("%s: %s\n", pEntry->pName, pEntry->pDescription);
        printf("%-8s (0x%08lx: \"", "", pEntry->pClientInfo->Flags);
        Tracery_Decode(pEntry->pClientInfo, pEntry->pClientInfo->Flags,
                       DecodeBuf, sizeof(DecodeBuf));
        printf("%s\")\n", DecodeBuf);

        /*Display list of edit specs associated with object*/
        for (pEdits = pEntry->pEditList; pEdits->pName != NULL; pEdits++) {
                /*Display each entry, one per line*/
                printf("%-8s %-8s %s\n", "", 
                       pEdits->pName, 
                       pEdits->pDescription);

        }

        printf("\n");

} /*DisplayObjectFull*/


/************************************************************************/
/*                                                                      */
/*      DisplayRegistrations -- Display details of registrations        */
/*                                                                      */
/*      Prints out the name of each component registered with           */
/*      Tracery, along with a summary of the edit specifiers            */
/*      supported by that object.                                       */
/*                                                                      */
/*      The pFocus parameter provides some control over the level       */
/*      of detail reported.  If pFocus is NULL, a brief listing         */
/*      of all modules and flags is provided.  If you want more         */
/*      details about one or more modules, specify a name or a          */
/*      wildcard as the pFocus string.                                  */
/*                                                                      */
/************************************************************************/
public_scope void
Tracery_DisplayRegistrations(CHAR *pFocus)
{
        Tracery_Entry *pSearch;

        /*Start search at head of list*/
        pSearch = &gTracery.Head;

        /*Loop through each registered module*/
        for (pSearch = gTracery.Head.pNext; 
             pSearch != NULL;
             pSearch = pSearch->pNext) {
                /*Do we want brief details for all modules?*/
                if (pFocus == NULL) {
                        /*Yes, display this object*/
                        Tracery_DisplayObjectBrief(pSearch);

                } else {
                        /*No, does the caller want to report this object?*/
                        if (fnmatch(pFocus, 
                                    pSearch->pName, 
                                    FNM_PERIOD) != 0) {
                                /*No, move to the next name, if any*/
                                /*?? We ignore errors for now*/
                                continue;
                        }

                        /*Report full details of object*/
                        Tracery_DisplayObjectFull(pSearch);
                }

        }


} /*DisplayRegistrations*/
