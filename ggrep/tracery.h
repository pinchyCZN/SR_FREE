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
/*      list of name/mask/set specifications.  Each entry               */
/*      includes a description so that the user may find out            */
/*      what flags are available.                                       */
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
/*      Another result of adding this extra feature is that we          */
/*      need to find the information about the flag register            */
/*      while running without imposing too much cost on the             */
/*      client modules.  Since CPU cycles are the main resource         */
/*      we're trying too optimise and we're less worried about          */
/*      memory usage, we append a pointer to the client's flag          */
/*      register so that we may perform this mapping very easily.       */
/*      In other environments where memory is a scarcer resource,       */
/*      we may need to use a function to search Tracery's private       */
/*      list of registered objects.                                     */
/*                                                                      */
/*      Copyright (C) 1999-2000 Grouse Software.  All rights reserved.  */
/*      Written for Grouse by behoffski (Brenton Hoff).                 */
/*                                                                      */
/*      Free software: no warranty; use anywhere is ok; spread the      */
/*      sources; note any mods; share variations and derivatives        */
/*      (including sending to behoffski@grouse.com.au).                 */
/*                                                                      */
/************************************************************************/

#ifndef TRACERY_H
#define TRACERY_H

#include <compdef.h>

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/

/*Information maintained for each client object*/
typedef struct {
        /*Current trace bits active for this object*/
        LWORD Flags;

        /*Backreference to Tracery's control block (not for client use)*/
        void *pTraceryRef;

} Tracery_ObjectInfo;

/*NOTE: Don't reference or modify fields directly -- use Tracery interface*/

/*Endmarker for flag editing list entries*/
#define TRACERY_EDIT_LIST_END           {NULL, 0x0uL, 0x0uL, NULL}

/*Macros to use for tracing*/

#ifdef TRACERY_ENABLED

/*Macro to clear object flags in a civilised fashion*/
#define TRACERY_CLEAR_ALL_FLAGS(pObject) ((pObject)->Flags = 0)

/*Tracing enabled, generate code to check flags and execute if enabled*/
#define TRACERY(SelectFlags, Actions) do { \
                        if ((SelectFlags) & TRACERY_MODULE_INFO.Flags) { \
                                LWORD TRACERY_FLAGS \
                                       __attribute__((__unused__)) \
                                                  = (SelectFlags); \
                                Actions \
                        }} while (0)
#define TRACERY_FACE(Interface, SelectFlags, Actions) do { \
                        if ((SelectFlags) & (((Interface)->TraceInfo.Flags) \
                                             | TRACERY_MODULE_INFO.Flags)) { \
                                LWORD TRACERY_FLAGS \
                                       __attribute__((__unused__)) \
                                                  = (SelectFlags); \
                                Actions \
                        }} while (0)

#else
/*Tracing not to be compiled in: get macros to expand to nothing*/
#define TRACERY_CLEAR_ALL_FLAGS(pObject)
#define TRACERY(Flags, Actions)
#define TRACERY_FACE(Interface, Flags, Actions)

#endif /*TRACERY_ENABLED*/

/*Structure naming trace component and describing how to edit flag register*/
typedef struct {
        char *pName;
        LWORD MaskBits;
        LWORD SetBits;
        CHAR *pDescription;
} Tracery_EditEntry;

/************************************************************************/
/*                                                                      */
/*      RegistrationFn -- Link procedure to negotiate trace function    */
/*                                                                      */
/*      The registration function allows Tracery and the object         */
/*      being observed to exchange information needed to trace          */
/*      without requiring any outside party to be aware of the          */
/*      details of the conversation.  The function's interface          */
/*      was changed at the last moment to use IOCTL-style calls         */
/*      as this provides room for expansion in the future.              */
/*                                                                      */
/*      The client module must offer a link function supporting         */
/*      this interface in order to integrate with Tracery.  The         */
/*      function should return FALSE if the specified opcode isn't      */
/*      supported, or should return TRUE if the opcode was executed.    */
/*                                                                      */
/*      The registration opcodes are:                                   */
/*                                                                      */
/*      RegFn(pObject, GET_INFO_BLOCK,                                  */
/*                     Tracery_ObjectInfo **ppInfoBlock);               */
/*              Retrieve a pointer to the object's block                */
/*              which stores  Tracery-related information.              */
/*                                                                      */
/*      RegFn(pObject, GET_DEFAULT_FLAGS,                               */
/*                     LWORD *pDefaultFlags);                           */
/*              Retrieve the default flags to use if object             */
/*              tracing is requested without any more specific          */
/*              flag set/clear directives.                           .  */
/*                                                                      */
/*      RegFn(pObject, GET_EDIT_LIST,                                   */
/*                     Tracery_EditEntry **ppEditList);                 */
/*              Retrieve the list of edit specifiers for the            */
/*              flag register.  This list must be maintained            */
/*              by the caller (Tracery does not take a private          */
/*              copy).                                                  */
/*                                                                      */
/************************************************************************/

/*Opcodes used by registration interface*/
#define TRACERY_REGCMD_GET_INFO_BLOCK   0
#define TRACERY_REGCMD_GET_DEFAULT_FLAGS 1
#define TRACERY_REGCMD_GET_EDIT_LIST    2

/*Prototype for module/object registration function*/
typedef BOOL (Tracery_RegistrationFn)(void *pObject, UINT Opcode, ...);

/************************************************************************/
/*                                                                      */
/*      Init -- Prepare module for operation                            */
/*                                                                      */
/************************************************************************/
void
Tracery_Init(void);


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
BOOL
Tracery_Register(char *pName, 
                 CHAR *pDescription, 
                 void *pObject, 
                 Tracery_RegistrationFn *pRegFn);
                 

/************************************************************************/
/*                                                                      */
/*      Configure -- Manipulate flags based on configuration            */
/*                                                                      */
/*      This is the main interface used by text-driven interfaces,      */
/*      including command-line switches.  It expects to see a list      */
/*      of trace module names, separated by commas.  The names          */
/*      may be modified in a couple of ways; the meanings are:          */
/*           name        -- Turn on  all trace flags for object         */
/*          -name        -- Turn off all trace flags for object         */
/*           name(Chars) -- Set flags based on on/off chars             */
/*                                                                      */
/*      Returns FALSE if the configuration string didn't make sense.    */
/*                                                                      */
/************************************************************************/
BOOL
Tracery_Configure(CHAR *pConfiguration);

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
void
Tracery_Deregister(Tracery_ObjectInfo *pInfoBlock);


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
char *
Tracery_Name(Tracery_ObjectInfo *pInfoBlock);


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
BOOL
Tracery_Decode(Tracery_ObjectInfo *pInfoBlock, 
               LWORD FlagBits, CHAR *pString, UINT StringSize);


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
void
Tracery_DisplayRegistrations(CHAR *pFocus);


#ifdef __cplusplus
}
#endif /*__cplusplus*/

#endif /*TRACERY_H*/
