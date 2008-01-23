/*
 * tclInt.h --
 *
 *  Declarations of things used internally by the Tcl interpreter.
 *
 * Copyright (c) 1987-1993 The Regents of the University of California.
 * Copyright (c) 1993-1997 Lucent Technologies.
 * Copyright (c) 1994-1998 Sun Microsystems, Inc.
 * Copyright (c) 1998-1999 by Scriptics Corporation.
 * Copyright (c) 2001, 2002 by Kevin B. Kenny.  All rights reserved.
 * Copyright (c) 2007 Daniel A. Steffen <das@users.sourceforge.net>
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#ifndef _TCLINT
#define _TCLINT

/*
 * Some numerics configuration options
 */

#undef NO_WIDE_TYPE
#undef ACCEPT_NAN

/*
 * Common include files needed by most of the Tcl source files are included
 * here, so that system-dependent personalizations for the include files only
 * have to be made in once place. This results in a few extra includes, but
 * greater modularity. The order of the three groups of #includes is
 * important. For example, stdio.h is needed by tcl.h, and the _ANSI_ARGS_
 * declaration in tcl.h is needed by stdlib.h in some configurations.
 */

#ifdef HAVE_TCL_CONFIG_H
#include "tclConfig.h"
#endif
#ifndef _TCL
#include "tcl.h"
#endif

#include <stdio.h>

#include <ctype.h>
#ifdef NO_LIMITS_H
#   include "../compat/limits.h"
#else
#   include <limits.h>
#endif
#ifdef NO_STDLIB_H
#   include "../compat/stdlib.h"
#else
#   include <stdlib.h>
#endif
#ifdef NO_STRING_H
#include "../compat/string.h"
#else
#include <string.h>
#endif
#ifdef STDC_HEADERS
#include <stddef.h>
#else
typedef int ptrdiff_t;
#endif

/*
 * Ensure WORDS_BIGENDIAN is defined correcly:
 * Needs to happen here in addition to configure to work with fat compiles on
 * Darwin (where configure runs only once for multiple architectures).
 */

#ifdef HAVE_SYS_TYPES_H
#    include <sys/types.h>
#endif
#ifdef HAVE_SYS_PARAM_H
#    include <sys/param.h>
#endif
#ifdef BYTE_ORDER
#    ifdef BIG_ENDIAN
#   if BYTE_ORDER == BIG_ENDIAN
#       undef WORDS_BIGENDIAN
#       define WORDS_BIGENDIAN 1
#   endif
#    endif
#    ifdef LITTLE_ENDIAN
#   if BYTE_ORDER == LITTLE_ENDIAN
#       undef WORDS_BIGENDIAN
#   endif
#    endif
#endif

/*
 * Used to tag functions that are only to be visible within the module being
 * built and not outside it (where this is supported by the linker).
 */

#ifndef MODULE_SCOPE
#   ifdef __cplusplus
#  define MODULE_SCOPE extern "C"
#   else
#  define MODULE_SCOPE extern
#   endif
#endif

/*
 * When Tcl_WideInt and long are the same type, there's no value in
 * having a tclWideIntType separate from the tclIntType.
 */
#ifdef TCL_WIDE_INT_IS_LONG
#define NO_WIDE_TYPE
#endif

/*
 * Macros used to cast between pointers and integers (e.g. when storing an int
 * in ClientData), on 64-bit architectures they avoid gcc warning about "cast
 * to/from pointer from/to integer of different size".
 */

#if !defined(INT2PTR) && !defined(PTR2INT)
#   if defined(HAVE_INTPTR_T) || defined(intptr_t)
#  define INT2PTR(p) ((void*)(intptr_t)(p))
#  define PTR2INT(p) ((int)(intptr_t)(p))
#   else
#  define INT2PTR(p) ((void*)(p))
#  define PTR2INT(p) ((int)(p))
#   endif
#endif
#if !defined(UINT2PTR) && !defined(PTR2UINT)
#   if defined(HAVE_UINTPTR_T) || defined(uintptr_t)
#  define UINT2PTR(p) ((void*)(uintptr_t)(p))
#  define PTR2UINT(p) ((unsigned int)(uintptr_t)(p))
#   else
#  define UINT2PTR(p) ((void*)(p))
#  define PTR2UINT(p) ((unsigned int)(p))
#   endif
#endif

/*
 * The following procedures allow namespaces to be customized to support
 * special name resolution rules for commands/variables.
 */

struct Tcl_ResolvedVarInfo;

typedef Tcl_Var (Tcl_ResolveRuntimeVarProc)(Tcl_Interp *interp,
  struct Tcl_ResolvedVarInfo *vinfoPtr);

typedef void (Tcl_ResolveVarDeleteProc)(struct Tcl_ResolvedVarInfo *vinfoPtr);

/*
 * The following structure encapsulates the routines needed to resolve a
 * variable reference at runtime. Any variable specific state will typically
 * be appended to this structure.
 */

typedef struct Tcl_ResolvedVarInfo {
    Tcl_ResolveRuntimeVarProc *fetchProc;
    Tcl_ResolveVarDeleteProc *deleteProc;
} Tcl_ResolvedVarInfo;

typedef int (Tcl_ResolveCompiledVarProc) (Tcl_Interp *interp,
  CONST84 char *name, int length, Tcl_Namespace *context,
  Tcl_ResolvedVarInfo **rPtr);

typedef int (Tcl_ResolveVarProc) (Tcl_Interp *interp, CONST84 char *name,
  Tcl_Namespace *context, int flags, Tcl_Var *rPtr);

typedef int (Tcl_ResolveCmdProc) (Tcl_Interp *interp, CONST84 char *name,
  Tcl_Namespace *context, int flags, Tcl_Command *rPtr);

typedef struct Tcl_ResolverInfo {
    Tcl_ResolveCmdProc *cmdResProc;
        /* Procedure handling command name
         * resolution. */
    Tcl_ResolveVarProc *varResProc;
        /* Procedure handling variable name resolution
         * for variables that can only be handled at
         * runtime. */
    Tcl_ResolveCompiledVarProc *compiledVarResProc;
        /* Procedure handling variable name resolution
         * at compile time. */
} Tcl_ResolverInfo;

/*
 *----------------------------------------------------------------
 * Data structures related to namespaces.
 *----------------------------------------------------------------
 */

typedef struct Tcl_Ensemble Tcl_Ensemble;
typedef struct NamespacePathEntry NamespacePathEntry;

/*
 * Special hashtable for variables: this is just a Tcl_HashTable with an nsPtr
 * field added at the end: in this way variables can find their namespace
 * without having to copy a pointer in their struct: they can access it via
 * their hPtr->tablePtr.
 */

typedef struct TclVarHashTable {
    Tcl_HashTable table;
    struct Namespace *nsPtr;
} TclVarHashTable;

/*
 * This is for itcl - it likes to search our varTables directly :(
 */

#define TclVarHashFindVar(tablePtr, key) \
    TclVarHashCreateVar((tablePtr), (key), NULL)


/*
 * The structure below defines a namespace.
 * Note: the first five fields must match exactly the fields in a
 * Tcl_Namespace structure (see tcl.h). If you change one, be sure to change
 * the other.
 */

typedef struct Namespace {
    char *name;      /* The namespace's simple (unqualified) name.
         * This contains no ::'s. The name of the
         * global namespace is "" although "::" is an
         * synonym. */
    char *fullName;    /* The namespace's fully qualified name. This
         * starts with ::. */
    ClientData clientData;  /* An arbitrary value associated with this
         * namespace. */
    Tcl_NamespaceDeleteProc *deleteProc;
        /* Procedure invoked when deleting the
         * namespace to, e.g., free clientData. */
    struct Namespace *parentPtr;/* Points to the namespace that contains this
         * one. NULL if this is the global
         * namespace. */
    Tcl_HashTable childTable;  /* Contains any child namespaces. Indexed by
         * strings; values have type (Namespace *). */
    long nsId;      /* Unique id for the namespace. */
    Tcl_Interp *interp;    /* The interpreter containing this
         * namespace. */
    int flags;      /* OR-ed combination of the namespace status
         * flags NS_DYING and NS_DEAD listed below. */
    int activationCount;  /* Number of "activations" or active call
         * frames for this namespace that are on the
         * Tcl call stack. The namespace won't be
         * freed until activationCount becomes zero. */
    int refCount;    /* Count of references by namespaceName
         * objects. The namespace can't be freed until
         * refCount becomes zero. */
    Tcl_HashTable cmdTable;  /* Contains all the commands currently
         * registered in the namespace. Indexed by
         * strings; values have type (Command *).
         * Commands imported by Tcl_Import have
         * Command structures that point (via an
         * ImportedCmdRef structure) to the Command
         * structure in the source namespace's command
         * table. */
    TclVarHashTable varTable;  /* Contains all the (global) variables
         * currently in this namespace. Indexed by
         * strings; values have type (Var *). */
    char **exportArrayPtr;  /* Points to an array of string patterns
         * specifying which commands are exported. A
         * pattern may include "string match" style
         * wildcard characters to specify multiple
         * commands; however, no namespace qualifiers
         * are allowed. NULL if no export patterns are
         * registered. */
    int numExportPatterns;  /* Number of export patterns currently
         * registered using "namespace export". */
    int maxExportPatterns;  /* Mumber of export patterns for which space
         * is currently allocated. */
    int cmdRefEpoch;    /* Incremented if a newly added command
         * shadows a command for which this namespace
         * has already cached a Command * pointer;
         * this causes all its cached Command*
         * pointers to be invalidated. */
    int resolverEpoch;    /* Incremented whenever (a) the name
         * resolution rules change for this namespace
         * or (b) a newly added command shadows a
         * command that is compiled to bytecodes. This
         * invalidates all byte codes compiled in the
         * namespace, causing the code to be
         * recompiled under the new rules.*/
    Tcl_ResolveCmdProc *cmdResProc;
        /* If non-null, this procedure overrides the
         * usual command resolution mechanism in Tcl.
         * This procedure is invoked within
         * Tcl_FindCommand to resolve all command
         * references within the namespace. */
    Tcl_ResolveVarProc *varResProc;
        /* If non-null, this procedure overrides the
         * usual variable resolution mechanism in Tcl.
         * This procedure is invoked within
         * Tcl_FindNamespaceVar to resolve all
         * variable references within the namespace at
         * runtime. */
    Tcl_ResolveCompiledVarProc *compiledVarResProc;
        /* If non-null, this procedure overrides the
         * usual variable resolution mechanism in Tcl.
         * This procedure is invoked within
         * LookupCompiledLocal to resolve variable
         * references within the namespace at compile
         * time. */
    int exportLookupEpoch;  /* Incremented whenever a command is added to
         * a namespace, removed from a namespace or
         * the exports of a namespace are changed.
         * Allows TIP#112-driven command lists to be
         * validated efficiently. */
    Tcl_Ensemble *ensembles;  /* List of structures that contain the details
         * of the ensembles that are implemented on
         * top of this namespace. */
    Tcl_Obj *unknownHandlerPtr;  /* A script fragment to be used when command
         * resolution in this namespace fails. TIP
         * 181. */
    int commandPathLength;  /* The length of the explicit path. */
    NamespacePathEntry *commandPathArray;
        /* The explicit path of the namespace as an
         * array. */
    NamespacePathEntry *commandPathSourceList;
        /* Linked list of path entries that point to
         * this namespace. */
} Namespace;

/*
 * An entry on a namespace's command resolution path.
 */

struct NamespacePathEntry {
    Namespace *nsPtr;    /* What does this path entry point to? If it
         * is NULL, this path entry points is
         * redundant and should be skipped. */
    Namespace *creatorNsPtr;  /* Where does this path entry point from? This
         * allows for efficient invalidation of
         * references when the path entry's target
         * updates its current list of defined
         * commands. */
    NamespacePathEntry *prevPtr, *nextPtr;
        /* Linked list pointers or NULL at either end
         * of the list that hangs off Namespace's
         * commandPathSourceList field. */
};

/*
 * Flags used to represent the status of a namespace:
 *
 * NS_DYING -  1 means Tcl_DeleteNamespace has been called to delete the
 *    namespace but there are still active call frames on the Tcl
 *    stack that refer to the namespace. When the last call frame
 *    referring to it has been popped, it's variables and command
 *    will be destroyed and it will be marked "dead" (NS_DEAD). The
 *    namespace can no longer be looked up by name.
 * NS_DEAD -  1 means Tcl_DeleteNamespace has been called to delete the
 *    namespace and no call frames still refer to it. Its variables
 *    and command have already been destroyed. This bit allows the
 *    namespace resolution code to recognize that the namespace is
 *    "deleted". When the last namespaceName object in any byte code
 *    unit that refers to the namespace has been freed (i.e., when
 *    the namespace's refCount is 0), the namespace's storage will
 *    be freed.
 * NS_KILLED    1 means that TclTeardownNamespace has already been called on
 *              this namespace and it should not be called again [Bug 1355942]
 */

#define NS_DYING  0x01
#define NS_DEAD    0x02
#define NS_KILLED       0x04

/*
 * Flags passed to TclGetNamespaceForQualName:
 *
 * TCL_GLOBAL_ONLY    - (see tcl.h) Look only in the global ns.
 * TCL_NAMESPACE_ONLY    - (see tcl.h) Look only in the context ns.
 * TCL_CREATE_NS_IF_UNKNOWN  - Create unknown namespaces.
 * TCL_FIND_ONLY_NS    - The name sought is a namespace name.
 */

#define TCL_CREATE_NS_IF_UNKNOWN  0x800
#define TCL_FIND_ONLY_NS    0x1000

/*
 * The data cached in an ensemble subcommand's Tcl_Obj rep (reference in
 * otherValuePtr field). This structure is not shared between Tcl_Objs
 * referring to the same subcommand, even where one is a duplicate of another.
 */

typedef struct {
    Namespace *nsPtr;    /* The namespace backing the ensemble which
         * this is a subcommand of. */
    int epoch;      /* Used to confirm when the data in this
         * really structure matches up with the
         * ensemble. */
    Tcl_Command token;    /* Reference to the comamnd for which this
         * structure is a cache of the resolution. */
    char *fullSubcmdName;  /* The full (local) name of the subcommand,
         * allocated with ckalloc(). */
    Tcl_Obj *realPrefixObj;  /* Object containing the prefix words of the
         * command that implements this ensemble
         * subcommand. */
} EnsembleCmdRep;

/*
 * Flag to enable bytecode compilation of an ensemble.
 */

#define ENSEMBLE_COMPILE 0x4

/*
 *----------------------------------------------------------------
 * Data structures related to variables. These are used primarily in tclVar.c
 *----------------------------------------------------------------
 */

/*
 * The following structure defines a variable trace, which is used to invoke a
 * specific C procedure whenever certain operations are performed on a
 * variable.
 */

typedef struct VarTrace {
    Tcl_VarTraceProc *traceProc;/* Procedure to call when operations given by
         * flags are performed on variable. */
    ClientData clientData;  /* Argument to pass to proc. */
    int flags;      /* What events the trace procedure is
         * interested in: OR-ed combination of
         * TCL_TRACE_READS, TCL_TRACE_WRITES,
         * TCL_TRACE_UNSETS and TCL_TRACE_ARRAY. */
    struct VarTrace *nextPtr;  /* Next in list of traces associated with a
         * particular variable. */
} VarTrace;

/*
 * The following structure defines a command trace, which is used to invoke a
 * specific C procedure whenever certain operations are performed on a
 * command.
 */

typedef struct CommandTrace {
    Tcl_CommandTraceProc *traceProc;
        /* Procedure to call when operations given by
         * flags are performed on command. */
    ClientData clientData;  /* Argument to pass to proc. */
    int flags;      /* What events the trace procedure is
         * interested in: OR-ed combination of
         * TCL_TRACE_RENAME, TCL_TRACE_DELETE. */
    struct CommandTrace *nextPtr;
        /* Next in list of traces associated with a
         * particular command. */
    int refCount;    /* Used to ensure this structure is not
         * deleted too early. Keeps track of how many
         * pieces of code have a pointer to this
         * structure. */
} CommandTrace;

/*
 * When a command trace is active (i.e. its associated procedure is executing)
 * one of the following structures is linked into a list associated with the
 * command's interpreter. The information in the structure is needed in order
 * for Tcl to behave reasonably if traces are deleted while traces are active.
 */

typedef struct ActiveCommandTrace {
    struct Command *cmdPtr;  /* Command that's being traced. */
    struct ActiveCommandTrace *nextPtr;
        /* Next in list of all active command traces
         * for the interpreter, or NULL if no more. */
    CommandTrace *nextTracePtr;  /* Next trace to check after current trace
         * procedure returns; if this trace gets
         * deleted, must update pointer to avoid using
         * free'd memory. */
    int reverseScan;    /* Boolean set true when traces are scanning
         * in reverse order. */
} ActiveCommandTrace;

/*
 * When a variable trace is active (i.e. its associated procedure is
 * executing) one of the following structures is linked into a list associated
 * with the variable's interpreter. The information in the structure is needed
 * in order for Tcl to behave reasonably if traces are deleted while traces
 * are active.
 */

typedef struct ActiveVarTrace {
    struct Var *varPtr;    /* Variable that's being traced. */
    struct ActiveVarTrace *nextPtr;
        /* Next in list of all active variable traces
         * for the interpreter, or NULL if no more. */
    VarTrace *nextTracePtr;  /* Next trace to check after current trace
         * procedure returns; if this trace gets
         * deleted, must update pointer to avoid using
         * free'd memory. */
} ActiveVarTrace;

/*
 * The following structure describes an enumerative search in progress on an
 * array variable; this are invoked with options to the "array" command.
 */

typedef struct ArraySearch {
    int id;      /* Integer id used to distinguish among
         * multiple concurrent searches for the same
         * array. */
    struct Var *varPtr;    /* Pointer to array variable that's being
         * searched. */
    Tcl_HashSearch search;  /* Info kept by the hash module about progress
         * through the array. */
    Tcl_HashEntry *nextEntry;  /* Non-null means this is the next element to
         * be enumerated (it's leftover from the
         * Tcl_FirstHashEntry call or from an "array
         * anymore" command). NULL means must call
         * Tcl_NextHashEntry to get value to
         * return. */
    struct ArraySearch *nextPtr;/* Next in list of all active searches for
         * this variable, or NULL if this is the last
         * one. */
} ArraySearch;

/*
 * The structure below defines a variable, which associates a string name with
 * a Tcl_Obj value. These structures are kept in procedure call frames (for
 * local variables recognized by the compiler) or in the heap (for global
 * variables and any variable not known to the compiler). For each Var
 * structure in the heap, a hash table entry holds the variable name and a
 * pointer to the Var structure.
 */

typedef struct Var {
    int flags;      /* Miscellaneous bits of information about
         * variable. See below for definitions. */
    union {
  Tcl_Obj *objPtr;  /* The variable's object value. Used for
         * scalar variables and array elements. */
  TclVarHashTable *tablePtr;/* For array variables, this points to
         * information about the hash table used to
         * implement the associative array. Points to
         * ckalloc-ed data. */
  struct Var *linkPtr;  /* If this is a global variable being referred
         * to in a procedure, or a variable created by
         * "upvar", this field points to the
         * referenced variable's Var struct. */
    } value;
} Var;

typedef struct VarInHash {
    Var var;
    int refCount;    /* Counts number of active uses of this
         * variable: 1 for the entry in the hash
         * table, 1 for each additional variable whose
         * linkPtr points here, 1 for each nested
         * trace active on variable, and 1 if the
         * variable is a namespace variable. This
         * record can't be deleted until refCount
         * becomes 0. */
    Tcl_HashEntry entry;  /* The hash table entry that refers to this
         * variable. This is used to find the name of
         * the variable and to delete it from its
         * hashtable if it is no longer needed. It
         * also holds the variable's name. */
} VarInHash;

/*
 * Flag bits for variables. The first two (VAR_ARRAY and VAR_LINK) are
 * mutually exclusive and give the "type" of the variable. If none is set,
 * this is a scalar variable.
 *
 * VAR_ARRAY -      1 means this is an array variable rather than
 *        a scalar variable or link. The "tablePtr"
 *        field points to the array's hashtable for its
 *        elements.
 * VAR_LINK -      1 means this Var structure contains a pointer
 *        to another Var structure that either has the
 *        real value or is itself another VAR_LINK
 *        pointer. Variables like this come about
 *        through "upvar" and "global" commands, or
 *        through references to variables in enclosing
 *        namespaces.
 *
 * Flags that indicate the type and status of storage; none is set for
 * compiled local variables (Var structs).
 *
 * VAR_IN_HASHTABLE -    1 means this variable is in a hashtable and
 *        the Var structure is malloced. 0 if it is a
 *        local variable that was assigned a slot in a
 *        procedure frame by the compiler so the Var
 *        storage is part of the call frame.
 * VAR_DEAD_HASH                1 means that this var's entry in the hashtable
 *                              has already been deleted.
 * VAR_ARRAY_ELEMENT -    1 means that this variable is an array
 *        element, so it is not legal for it to be an
 *        array itself (the VAR_ARRAY flag had better
 *        not be set).
 * VAR_NAMESPACE_VAR -    1 means that this variable was declared as a
 *        namespace variable. This flag ensures it
 *        persists until its namespace is destroyed or
 *        until the variable is unset; it will persist
 *        even if it has not been initialized and is
 *        marked undefined. The variable's refCount is
 *        incremented to reflect the "reference" from
 *        its namespace.
 *
 * Flag values relating to the variable's trace and search status.
 *
 * VAR_TRACED_READ
 * VAR_TRACED_WRITE
 * VAR_TRACED_UNSET
 * VAR_TRACED_ARRAY
 * VAR_TRACE_ACTIVE -    1 means that trace processing is currently
 *        underway for a read or write access, so new
 *        read or write accesses should not cause trace
 *        procedures to be called and the variable can't
 *        be deleted.
 * VAR_SEARCH_ACTIVE
 *
 * The following additional flags are used with the CompiledLocal type defined
 * below:
 *
 * VAR_ARGUMENT -    1 means that this variable holds a procedure
 *        argument.
 * VAR_TEMPORARY -    1 if the local variable is an anonymous
 *        temporary variable. Temporaries have a NULL
 *        name.
 * VAR_RESOLVED -    1 if name resolution has been done for this
 *        variable.
 * VAR_IS_ARGS                  1 if this variable is the last argument and is
 *                              named "args".
 */

/*
 * FLAGS RENUMBERED: everything breaks already, make things simpler.
 *
 * IMPORTANT: skip the values 0x10, 0x20, 0x40, 0x800 corresponding to
 * TCL_TRACE_(READS/WRITES/UNSETS/ARRAY): makes code simpler in tclTrace.c
 *
 * Keep the flag values for VAR_ARGUMENT and VAR_TEMPORARY so that old values
 * in precompiled scripts keep working.
 */


/* Type of value (0 is scalar) */
#define VAR_ARRAY    0x1
#define VAR_LINK    0x2

/* Type of storage (0 is compiled local) */
#define VAR_IN_HASHTABLE  0x4
#define VAR_DEAD_HASH           0x8
#define VAR_ARRAY_ELEMENT  0x1000
#define VAR_NAMESPACE_VAR  0x80      /* KEEP OLD VALUE for Itcl */

#define VAR_ALL_HASH \
  (VAR_IN_HASHTABLE|VAR_DEAD_HASH|VAR_NAMESPACE_VAR|VAR_ARRAY_ELEMENT)

/* Trace and search state */

#define VAR_TRACED_READ        0x10       /* TCL_TRACE_READS  */
#define VAR_TRACED_WRITE       0x20       /* TCL_TRACE_WRITES */
#define VAR_TRACED_UNSET       0x40       /* TCL_TRACE_UNSETS */
#define VAR_TRACED_ARRAY       0x800    /* TCL_TRACE_ARRAY  */
#define VAR_TRACE_ACTIVE       0x2000
#define VAR_SEARCH_ACTIVE      0x4000
#define VAR_ALL_TRACES \
  (VAR_TRACED_READ|VAR_TRACED_WRITE|VAR_TRACED_ARRAY|VAR_TRACED_UNSET)


/* Special handling on initialisation (only CompiledLocal) */
#define VAR_ARGUMENT    0x100    /* KEEP OLD VALUE! See tclProc.c */
#define VAR_TEMPORARY    0x200    /* KEEP OLD VALUE! See tclProc.c */
#define VAR_IS_ARGS    0x400
#define VAR_RESOLVED    0x8000

/*
 * Macros to ensure that various flag bits are set properly for variables.
 * The ANSI C "prototypes" for these macros are:
 *
 * MODULE_SCOPE void  TclSetVarScalar(Var *varPtr);
 * MODULE_SCOPE void  TclSetVarArray(Var *varPtr);
 * MODULE_SCOPE void  TclSetVarLink(Var *varPtr);
 * MODULE_SCOPE void  TclSetVarArrayElement(Var *varPtr);
 * MODULE_SCOPE void  TclSetVarUndefined(Var *varPtr);
 * MODULE_SCOPE void  TclClearVarUndefined(Var *varPtr);
 */

#define TclSetVarScalar(varPtr) \
    (varPtr)->flags &= ~(VAR_ARRAY|VAR_LINK)

#define TclSetVarArray(varPtr) \
    (varPtr)->flags = ((varPtr)->flags & ~VAR_LINK) | VAR_ARRAY

#define TclSetVarLink(varPtr) \
    (varPtr)->flags = ((varPtr)->flags & ~VAR_ARRAY) | VAR_LINK

#define TclSetVarArrayElement(varPtr) \
    (varPtr)->flags = ((varPtr)->flags & ~VAR_ARRAY) | VAR_ARRAY_ELEMENT

#define TclSetVarUndefined(varPtr) \
    (varPtr)->flags &= ~(VAR_ARRAY|VAR_LINK);\
    (varPtr)->value.objPtr = NULL

#define TclClearVarUndefined(varPtr)

#define TclSetVarTraceActive(varPtr) \
    (varPtr)->flags |= VAR_TRACE_ACTIVE

#define TclClearVarTraceActive(varPtr) \
    (varPtr)->flags &= ~VAR_TRACE_ACTIVE

#define TclSetVarNamespaceVar(varPtr) \
    if (!TclIsVarNamespaceVar(varPtr)) {\
  (varPtr)->flags |= VAR_NAMESPACE_VAR;\
  ((VarInHash *)(varPtr))->refCount++;\
    }

#define TclClearVarNamespaceVar(varPtr) \
    if (TclIsVarNamespaceVar(varPtr)) {\
  (varPtr)->flags &= ~VAR_NAMESPACE_VAR;\
  ((VarInHash *)(varPtr))->refCount--;\
    }

/*
 * Macros to read various flag bits of variables.
 * The ANSI C "prototypes" for these macros are:
 *
 * MODULE_SCOPE int  TclIsVarScalar(Var *varPtr);
 * MODULE_SCOPE int  TclIsVarLink(Var *varPtr);
 * MODULE_SCOPE int  TclIsVarArray(Var *varPtr);
 * MODULE_SCOPE int  TclIsVarUndefined(Var *varPtr);
 * MODULE_SCOPE int  TclIsVarArrayElement(Var *varPtr);
 * MODULE_SCOPE int  TclIsVarTemporary(Var *varPtr);
 * MODULE_SCOPE int  TclIsVarArgument(Var *varPtr);
 * MODULE_SCOPE int  TclIsVarResolved(Var *varPtr);
 */

#define TclIsVarScalar(varPtr) \
    !((varPtr)->flags & (VAR_ARRAY|VAR_LINK))

#define TclIsVarLink(varPtr) \
    ((varPtr)->flags & VAR_LINK)

#define TclIsVarArray(varPtr) \
    ((varPtr)->flags & VAR_ARRAY)

#define TclIsVarUndefined(varPtr) \
    ((varPtr)->value.objPtr == NULL)

#define TclIsVarArrayElement(varPtr) \
    ((varPtr)->flags & VAR_ARRAY_ELEMENT)

#define TclIsVarNamespaceVar(varPtr) \
    ((varPtr)->flags & VAR_NAMESPACE_VAR)

#define TclIsVarTemporary(varPtr) \
    ((varPtr)->flags & VAR_TEMPORARY)

#define TclIsVarArgument(varPtr) \
    ((varPtr)->flags & VAR_ARGUMENT)

#define TclIsVarResolved(varPtr) \
    ((varPtr)->flags & VAR_RESOLVED)

#define TclIsVarTraceActive(varPtr) \
    ((varPtr)->flags & VAR_TRACE_ACTIVE)

#define TclIsVarTraced(varPtr) \
   ((varPtr)->flags & VAR_ALL_TRACES)

#define TclIsVarInHash(varPtr) \
    ((varPtr)->flags & VAR_IN_HASHTABLE)

#define TclIsVarDeadHash(varPtr) \
    ((varPtr)->flags & VAR_DEAD_HASH)

#define TclGetVarNsPtr(varPtr) \
    (TclIsVarInHash(varPtr) \
         ? ((TclVarHashTable *) ((((VarInHash *) (varPtr))->entry.tablePtr)))->nsPtr \
   : NULL)

#define VarHashRefCount(varPtr) \
    ((VarInHash *) (varPtr))->refCount

/*
 * Macros for direct variable access by TEBC
 */

#define TclIsVarDirectReadable(varPtr) \
    (   !((varPtr)->flags & (VAR_ARRAY|VAR_LINK|VAR_TRACED_READ)) \
    &&  (varPtr)->value.objPtr)

#define TclIsVarDirectWritable(varPtr) \
    !((varPtr)->flags & (VAR_ARRAY|VAR_LINK|VAR_TRACED_WRITE|VAR_DEAD_HASH))

#define TclIsVarDirectModifyable(varPtr) \
    (   !((varPtr)->flags & (VAR_ARRAY|VAR_LINK|VAR_TRACED_READ|VAR_TRACED_WRITE)) \
    &&  (varPtr)->value.objPtr)

#define TclIsVarDirectReadable2(varPtr, arrayPtr) \
    (TclIsVarDirectReadable(varPtr) &&\
        (!(arrayPtr) || !((arrayPtr)->flags & VAR_TRACED_READ)))

#define TclIsVarDirectWritable2(varPtr, arrayPtr) \
    (TclIsVarDirectWritable(varPtr) &&\
        (!(arrayPtr) || !((arrayPtr)->flags & VAR_TRACED_WRITE)))

#define TclIsVarDirectModifyable2(varPtr, arrayPtr) \
    (TclIsVarDirectModifyable(varPtr) &&\
        (!(arrayPtr) || !((arrayPtr)->flags & (VAR_TRACED_READ|VAR_TRACED_WRITE))))


/*
 *----------------------------------------------------------------
 * Data structures related to procedures. These are used primarily in
 * tclProc.c, tclCompile.c, and tclExecute.c.
 *----------------------------------------------------------------
 */

/*
 * Forward declaration to prevent an error when the forward reference to
 * Command is encountered in the Proc and ImportRef types declared below.
 */

struct Command;

/*
 * The variable-length structure below describes a local variable of a
 * procedure that was recognized by the compiler. These variables have a name,
 * an element in the array of compiler-assigned local variables in the
 * procedure's call frame, and various other items of information. If the
 * local variable is a formal argument, it may also have a default value. The
 * compiler can't recognize local variables whose names are expressions (these
 * names are only known at runtime when the expressions are evaluated) or
 * local variables that are created as a result of an "upvar" or "uplevel"
 * command. These other local variables are kept separately in a hash table in
 * the call frame.
 */

typedef struct CompiledLocal {
    struct CompiledLocal *nextPtr;
        /* Next compiler-recognized local variable for
         * this procedure, or NULL if this is the last
         * local. */
    int nameLength;    /* The number of characters in local
         * variable's name. Used to speed up variable
         * lookups. */
    int frameIndex;    /* Index in the array of compiler-assigned
         * variables in the procedure call frame. */
    int flags;      /* Flag bits for the local variable. Same as
         * the flags for the Var structure above,
         * although only VAR_ARGUMENT, VAR_TEMPORARY,
         * and VAR_RESOLVED make sense. */
    Tcl_Obj *defValuePtr;  /* Pointer to the default value of an
         * argument, if any. NULL if not an argument
         * or, if an argument, no default value. */
    Tcl_ResolvedVarInfo *resolveInfo;
        /* Customized variable resolution info
         * supplied by the Tcl_ResolveCompiledVarProc
         * associated with a namespace. Each variable
         * is marked by a unique ClientData tag during
         * compilation, and that same tag is used to
         * find the variable at runtime. */
    char name[4];    /* Name of the local variable starts here. If
         * the name is NULL, this will just be '\0'.
         * The actual size of this field will be large
         * enough to hold the name. MUST BE THE LAST
         * FIELD IN THE STRUCTURE! */
} CompiledLocal;

/*
 * The structure below defines a command procedure, which consists of a
 * collection of Tcl commands plus information about arguments and other local
 * variables recognized at compile time.
 */

typedef struct Proc {
    struct Interp *iPtr;  /* Interpreter for which this command is
         * defined. */
    int refCount;    /* Reference count: 1 if still present in
         * command table plus 1 for each call to the
         * procedure that is currently active. This
         * structure can be freed when refCount
         * becomes zero. */
    struct Command *cmdPtr;  /* Points to the Command structure for this
         * procedure. This is used to get the
         * namespace in which to execute the
         * procedure. */
    Tcl_Obj *bodyPtr;    /* Points to the ByteCode object for
         * procedure's body command. */
    int numArgs;    /* Number of formal parameters. */
    int numCompiledLocals;  /* Count of local variables recognized by the
         * compiler including arguments and
         * temporaries. */
    CompiledLocal *firstLocalPtr;
        /* Pointer to first of the procedure's
         * compiler-allocated local variables, or NULL
         * if none. The first numArgs entries in this
         * list describe the procedure's formal
         * arguments. */
    CompiledLocal *lastLocalPtr;/* Pointer to the last allocated local
         * variable or NULL if none. This has frame
         * index (numCompiledLocals-1). */
} Proc;

/*
 * The type of functions called to process errors found during the execution
 * of a procedure (or lambda term or ...).
 */

typedef void (*ProcErrorProc)(Tcl_Interp *interp, Tcl_Obj *procNameObj);

/*
 * The structure below defines a command trace. This is used to allow Tcl
 * clients to find out whenever a command is about to be executed.
 */

typedef struct Trace {
    int level;      /* Only trace commands at nesting level less
         * than or equal to this. */
    Tcl_CmdObjTraceProc *proc;  /* Procedure to call to trace command. */
    ClientData clientData;  /* Arbitrary value to pass to proc. */
    struct Trace *nextPtr;  /* Next in list of traces for this interp. */
    int flags;      /* Flags governing the trace - see
         * Tcl_CreateObjTrace for details */
    Tcl_CmdObjTraceDeleteProc* delProc;
        /* Procedure to call when trace is deleted */
} Trace;

/*
 * When an interpreter trace is active (i.e. its associated procedure is
 * executing), one of the following structures is linked into a list
 * associated with the interpreter. The information in the structure is needed
 * in order for Tcl to behave reasonably if traces are deleted while traces
 * are active.
 */

typedef struct ActiveInterpTrace {
    struct ActiveInterpTrace *nextPtr;
        /* Next in list of all active command traces
         * for the interpreter, or NULL if no more. */
    Trace *nextTracePtr;  /* Next trace to check after current trace
         * procedure returns; if this trace gets
         * deleted, must update pointer to avoid using
         * free'd memory. */
    int reverseScan;    /* Boolean set true when traces are scanning
         * in reverse order. */
} ActiveInterpTrace;

/*
 * Flag values designating types of execution traces. See tclTrace.c for
 * related flag values.
 *
 * TCL_TRACE_ENTER_EXEC    - triggers enter/enterstep traces.
 *         - passed to Tcl_CreateObjTrace to set up
 *          "enterstep" traces.
 * TCL_TRACE_LEAVE_EXEC    - triggers leave/leavestep traces.
 *         - passed to Tcl_CreateObjTrace to set up
 *          "leavestep" traces.
 *
 */
#define TCL_TRACE_ENTER_EXEC  1
#define TCL_TRACE_LEAVE_EXEC  2

/*
 * The structure below defines an entry in the assocData hash table which is
 * associated with an interpreter. The entry contains a pointer to a function
 * to call when the interpreter is deleted, and a pointer to a user-defined
 * piece of data.
 */

typedef struct AssocData {
    Tcl_InterpDeleteProc *proc;  /* Proc to call when deleting. */
    ClientData clientData;  /* Value to pass to proc. */
} AssocData;

/*
 * The structure below defines a call frame. A call frame defines a naming
 * context for a procedure call: its local naming scope (for local variables)
 * and its global naming scope (a namespace, perhaps the global :: namespace).
 * A call frame can also define the naming context for a namespace eval or
 * namespace inscope command: the namespace in which the command's code should
 * execute. The Tcl_CallFrame structures exist only while procedures or
 * namespace eval/inscope's are being executed, and provide a kind of Tcl call
 * stack.
 *
 * WARNING!! The structure definition must be kept consistent with the
 * Tcl_CallFrame structure in tcl.h. If you change one, change the other.
 */

/*
 * Will be grown to contain: pointers to the varnames (allocated at the end),
 * plus the init values for each variable (suitable to be memcopied on init)
 */

typedef struct LocalCache {
    int refCount;
    int numVars;
    Tcl_Obj *varName0;
} LocalCache;

#define localName(framePtr, i) \
    ((&((framePtr)->localCachePtr->varName0))[(i)])

MODULE_SCOPE void  TclFreeLocalCache(Tcl_Interp *interp,
          LocalCache *localCachePtr);

typedef struct CallFrame {
    Namespace *nsPtr;    /* Points to the namespace used to resolve
         * commands and global variables. */
    int isProcCallFrame;  /* If 0, the frame was pushed to execute a
         * namespace command and var references are
         * treated as references to namespace vars;
         * varTablePtr and compiledLocals are ignored.
         * If FRAME_IS_PROC is set, the frame was
         * pushed to execute a Tcl procedure and may
         * have local vars. */
    int objc;      /* This and objv below describe the arguments
         * for this procedure call. */
    Tcl_Obj *const *objv;  /* Array of argument objects. */
    struct CallFrame *callerPtr;
        /* Value of interp->framePtr when this
         * procedure was invoked (i.e. next higher in
         * stack of all active procedures). */
    struct CallFrame *callerVarPtr;
        /* Value of interp->varFramePtr when this
         * procedure was invoked (i.e. determines
         * variable scoping within caller). Same as
         * callerPtr unless an "uplevel" command or
         * something equivalent was active in the
         * caller). */
    int level;      /* Level of this procedure, for "uplevel"
         * purposes (i.e. corresponds to nesting of
         * callerVarPtr's, not callerPtr's). 1 for
         * outermost procedure, 0 for top-level. */
    Proc *procPtr;    /* Points to the structure defining the called
         * procedure. Used to get information such as
         * the number of compiled local variables
         * (local variables assigned entries ["slots"]
         * in the compiledLocals array below). */
    TclVarHashTable *varTablePtr;
                                /* Hash table containing local variables not
         * recognized by the compiler, or created at
         * execution time through, e.g., upvar.
         * Initially NULL and created if needed. */
    int numCompiledLocals;  /* Count of local variables recognized by the
         * compiler including arguments. */
    Var *compiledLocals;  /* Points to the array of local variables
         * recognized by the compiler. The compiler
         * emits code that refers to these variables
         * using an index into this array. */
    ClientData clientData;  /* Pointer to some context that is used by
         * object systems. The meaning of the contents
         * of this field is defined by the code that
         * sets it, and it should only ever be set by
         * the code that is pushing the frame. In that
         * case, the code that sets it should also
         * have some means of discovering what the
         * meaning of the value is, which we do not
         * specify. */
    LocalCache *localCachePtr;
} CallFrame;

#define FRAME_IS_PROC   0x1
#define FRAME_IS_LAMBDA 0x2

/*
 * TIP #280
 * The structure below defines a command frame. A command frame provides
 * location information for all commands executing a tcl script (source, eval,
 * uplevel, procedure bodies, ...). The runtime structure essentially contains
 * the stack trace as it would be if the currently executing command were to
 * throw an error.
 *
 * For commands where it makes sense it refers to the associated CallFrame as
 * well.
 *
 * The structures are chained in a single list, with the top of the stack
 * anchored in the Interp structure.
 *
 * Instances can be allocated on the C stack, or the heap, the former making
 * cleanup a bit simpler.
 */

typedef struct CmdFrame {
    /*
     * General data. Always available.
     */

    int type;      /* Values see below. */
    int level;      /* #Frames in stack, prevent O(n) scan of
         * list. */
    int *line;      /* Lines the words of the command start on. */
    int nline;

    CallFrame *framePtr;  /* Procedure activation record, may be
         * NULL. */
    struct CmdFrame *nextPtr;  /* Link to calling frame */

    /*
     * Data needed for Eval vs TEBC
     *
     * EXECUTION CONTEXTS and usage of CmdFrame
     *
     * Field    TEBC      EvalEx    EvalObjEx
     * =======    ====      ======    =========
     * level    yes      yes      yes
     * type    BC/PREBC    SRC/EVAL    EVAL_LIST
     * line0    yes      yes      yes
     * framePtr    yes      yes      yes
     * =======    ====      ======    =========
     *
     * =======    ====      ======    ========= union data
     * line1    -      yes      -
     * line3    -      yes      -
     * path    -      yes      -
     * -------    ----      ------    ---------
     * codePtr    yes      -      -
     * pc    yes      -      -
     * =======    ====      ======    =========
     *
     * =======    ====      ======    ========= | union cmd
     * listPtr    -      -      yes      |
     * -------    ----      ------    --------- |
     * cmd    yes      yes      -      |
     * cmdlen    yes      yes      -      |
     * -------    ----      ------    --------- |
     */

    union {
  struct {
      Tcl_Obj *path;  /* Path of the sourced file the command is
         * in. */
  } eval;
  struct {
      const void *codePtr;/* Byte code currently executed */
      const char *pc;  /* and instruction pointer. */
  } tebc;
    } data;
    union {
  struct {
      const char *cmd;  /* The executed command, if possible */
      int len;    /* And its length */
  } str;
  Tcl_Obj *listPtr;  /* Tcl_EvalObjEx, cmd list */
    } cmd;
} CmdFrame;

/*
 * The following macros define the allowed values for the type field of the
 * CmdFrame structure above. Some of the values occur only in the extended
 * location data referenced via the 'baseLocPtr'.
 *
 * TCL_LOCATION_EVAL    : Frame is for a script evaluated by EvalEx.
 * TCL_LOCATION_EVAL_LIST : Frame is for a script evaluated by the list
 *          optimization path of EvalObjEx.
 * TCL_LOCATION_BC    : Frame is for bytecode.
 * TCL_LOCATION_PREBC    : Frame is for precompiled bytecode.
 * TCL_LOCATION_SOURCE    : Frame is for a script evaluated by EvalEx, from a
 *          sourced file.
 * TCL_LOCATION_PROC    : Frame is for bytecode of a procedure.
 *
 * A TCL_LOCATION_BC type in a frame can be overridden by _SOURCE and _PROC
 * types, per the context of the byte code in execution.
 */

#define TCL_LOCATION_EVAL      (0) /* Location in a dynamic eval script */
#define TCL_LOCATION_EVAL_LIST (1) /* Location in a dynamic eval script,
            * list-path */
#define TCL_LOCATION_BC         (2) /* Location in byte code */
#define TCL_LOCATION_PREBC     (3) /* Location in precompiled byte code, no
            * location */
#define TCL_LOCATION_SOURCE    (4) /* Location in a file */
#define TCL_LOCATION_PROC      (5) /* Location in a dynamic proc */

#define TCL_LOCATION_LAST      (6) /* Number of values in the enum */

/*
 * Structure passed to describe procedure-like "procedures" that are not
 * procedures (e.g. a lambda) so that their details can be reported correctly
 * by [info frame]. Contains a sub-structure for each extra field.
 */

typedef Tcl_Obj *(*GetFrameInfoValueProc)(ClientData clientData);
typedef struct {
    const char *name;    /* Name of this field. */
    GetFrameInfoValueProc proc;  /* Function to generate a Tcl_Obj* from the
         * clientData, or just use the clientData
         * directly (after casting) if NULL. */
    ClientData clientData;  /* Context for above function, or Tcl_Obj* if
         * proc field is NULL. */
} ExtraFrameInfoField;
typedef struct {
    int length;      /* Length of array. */
    ExtraFrameInfoField fields[2];
        /* Really as long as necessary, but this is
         * long enough for nearly anything. */
} ExtraFrameInfo;

/*
 *----------------------------------------------------------------
 * Data structures and procedures related to TclHandles, which are a very
 * lightweight method of preserving enough information to determine if an
 * arbitrary malloc'd block has been deleted.
 *----------------------------------------------------------------
 */

typedef void **TclHandle;

/*
 *----------------------------------------------------------------
 * Experimental flag value passed to Tcl_GetRegExpFromObj. Intended for use
 * only by Expect. It will probably go away in a later release.
 *----------------------------------------------------------------
 */

#define TCL_REG_BOSONLY 002000  /* Prepend \A to pattern so it only matches at
         * the beginning of the string. */

/*
 * These are a thin layer over TclpThreadKeyDataGet and TclpThreadKeyDataSet
 * when threads are used, or an emulation if there are no threads. These are
 * really internal and Tcl clients should use Tcl_GetThreadData.
 */

MODULE_SCOPE void *  TclThreadDataKeyGet(Tcl_ThreadDataKey *keyPtr);
MODULE_SCOPE void  TclThreadDataKeySet(Tcl_ThreadDataKey *keyPtr,
          void *data);

/*
 * This is a convenience macro used to initialize a thread local storage ptr.
 */

#define TCL_TSD_INIT(keyPtr) \
  (ThreadSpecificData *)Tcl_GetThreadData((keyPtr), sizeof(ThreadSpecificData))


/*
 *----------------------------------------------------------------
 * Data structures related to bytecode compilation and execution. These are
 * used primarily in tclCompile.c, tclExecute.c, and tclBasic.c.
 *----------------------------------------------------------------
 */

/*
 * Forward declaration to prevent errors when the forward references to
 * Tcl_Parse and CompileEnv are encountered in the procedure type CompileProc
 * declared below.
 */

struct CompileEnv;

/*
 * The type of procedures called by the Tcl bytecode compiler to compile
 * commands. Pointers to these procedures are kept in the Command structure
 * describing each command. The integer value returned by a CompileProc must
 * be one of the following:
 *
 * TCL_OK    Compilation completed normally.
 * TCL_ERROR     Compilation could not be completed. This can be just a
 *       judgment by the CompileProc that the command is too
 *       complex to compile effectively, or it can indicate
 *       that in the current state of the interp, the command
 *       would raise an error. The bytecode compiler will not
 *       do any error reporting at compiler time. Error
 *       reporting is deferred until the actual runtime,
 *       because by then changes in the interp state may allow
 *       the command to be successfully evaluated.
 * TCL_OUT_LINE_COMPILE  A source-compatible alias for TCL_ERROR, kept for the
 *       sake of old code only.
 */

#define TCL_OUT_LINE_COMPILE  TCL_ERROR

typedef int (CompileProc) (Tcl_Interp *interp, Tcl_Parse *parsePtr,
  struct Command *cmdPtr, struct CompileEnv *compEnvPtr);

/*
 * The type of procedure called from the compilation hook point in
 * SetByteCodeFromAny.
 */

typedef int (CompileHookProc) (Tcl_Interp *interp,
  struct CompileEnv *compEnvPtr, ClientData clientData);

/*
 * The data structure for a (linked list of) execution stacks.
 */

typedef struct ExecStack {
    struct ExecStack *prevPtr;
    struct ExecStack *nextPtr;
    Tcl_Obj **markerPtr;
    Tcl_Obj **endPtr;
    Tcl_Obj **tosPtr;
    Tcl_Obj *stackWords[1];
} ExecStack;

/*
 * The data structure defining the execution environment for ByteCode's.
 * There is one ExecEnv structure per Tcl interpreter. It holds the evaluation
 * stack that holds command operands and results. The stack grows towards
 * increasing addresses. The member stackPtr points to the stackItems of the
 * currently active execution stack.
 */

typedef struct ExecEnv {
    ExecStack *execStackPtr;  /* Points to the first item in the evaluation
         * stack on the heap. */
    Tcl_Obj *constants[2];  /* Pointers to constant "0" and "1" objs. */
} ExecEnv;

/*
 * The definitions for the LiteralTable and LiteralEntry structures. Each
 * interpreter contains a LiteralTable. It is used to reduce the storage
 * needed for all the Tcl objects that hold the literals of scripts compiled
 * by the interpreter. A literal's object is shared by all the ByteCodes that
 * refer to the literal. Each distinct literal has one LiteralEntry entry in
 * the LiteralTable. A literal table is a specialized hash table that is
 * indexed by the literal's string representation, which may contain null
 * characters.
 *
 * Note that we reduce the space needed for literals by sharing literal
 * objects both within a ByteCode (each ByteCode contains a local
 * LiteralTable) and across all an interpreter's ByteCodes (with the
 * interpreter's global LiteralTable).
 */

typedef struct LiteralEntry {
    struct LiteralEntry *nextPtr;
        /* Points to next entry in this hash bucket or
         * NULL if end of chain. */
    Tcl_Obj *objPtr;    /* Points to Tcl object that holds the
         * literal's bytes and length. */
    int refCount;    /* If in an interpreter's global literal
         * table, the number of ByteCode structures
         * that share the literal object; the literal
         * entry can be freed when refCount drops to
         * 0. If in a local literal table, -1. */
    Namespace *nsPtr;    /* Namespace in which this literal is used. We
         * try to avoid sharing literal non-FQ command
         * names among different namespaces to reduce
         * shimmering. */
} LiteralEntry;

typedef struct LiteralTable {
    LiteralEntry **buckets;  /* Pointer to bucket array. Each element
         * points to first entry in bucket's hash
         * chain, or NULL. */
    LiteralEntry *staticBuckets[TCL_SMALL_HASH_TABLE];
        /* Bucket array used for small tables to avoid
         * mallocs and frees. */
    int numBuckets;    /* Total number of buckets allocated at
         * **buckets. */
    int numEntries;    /* Total number of entries present in
         * table. */
    int rebuildSize;    /* Enlarge table when numEntries gets to be
         * this large. */
    int mask;      /* Mask value used in hashing function. */
} LiteralTable;

/*
 * The following structure defines for each Tcl interpreter various
 * statistics-related information about the bytecode compiler and
 * interpreter's operation in that interpreter.
 */

#ifdef TCL_COMPILE_STATS
typedef struct ByteCodeStats {
    long numExecutions;    /* Number of ByteCodes executed. */
    long numCompilations;  /* Number of ByteCodes created. */
    long numByteCodesFreed;  /* Number of ByteCodes destroyed. */
    long instructionCount[256];  /* Number of times each instruction was
         * executed. */

    double totalSrcBytes;  /* Total source bytes ever compiled. */
    double totalByteCodeBytes;  /* Total bytes for all ByteCodes. */
    double currentSrcBytes;  /* Src bytes for all current ByteCodes. */
    double currentByteCodeBytes;/* Code bytes in all current ByteCodes. */

    long srcCount[32];    /* Source size distribution: # of srcs of
         * size [2**(n-1)..2**n), n in [0..32). */
    long byteCodeCount[32];  /* ByteCode size distribution. */
    long lifetimeCount[32];  /* ByteCode lifetime distribution (ms). */

    double currentInstBytes;  /* Instruction bytes-current ByteCodes. */
    double currentLitBytes;  /* Current literal bytes. */
    double currentExceptBytes;  /* Current exception table bytes. */
    double currentAuxBytes;  /* Current auxiliary information bytes. */
    double currentCmdMapBytes;  /* Current src<->code map bytes. */

    long numLiteralsCreated;  /* Total literal objects ever compiled. */
    double totalLitStringBytes;  /* Total string bytes in all literals. */
    double currentLitStringBytes;
        /* String bytes in current literals. */
    long literalCount[32];  /* Distribution of literal string sizes. */
} ByteCodeStats;
#endif /* TCL_COMPILE_STATS */

/*
 * Structure used in implementation of those core ensembles which are
 * partially compiled.
 */

typedef struct {
    const char *name;    /* The name of the subcommand. */
    Tcl_ObjCmdProc *proc;  /* The implementation of the subcommand. */
    CompileProc *compileProc;  /* The compiler for the subcommand. */
} EnsembleImplMap;

/*
 *----------------------------------------------------------------
 * Data structures related to commands.
 *----------------------------------------------------------------
 */

/*
 * An imported command is created in an namespace when it imports a "real"
 * command from another namespace. An imported command has a Command structure
 * that points (via its ClientData value) to the "real" Command structure in
 * the source namespace's command table. The real command records all the
 * imported commands that refer to it in a list of ImportRef structures so
 * that they can be deleted when the real command is deleted.
 */

typedef struct ImportRef {
    struct Command *importedCmdPtr;
        /* Points to the imported command created in
         * an importing namespace; this command
         * redirects its invocations to the "real"
         * command. */
    struct ImportRef *nextPtr;  /* Next element on the linked list of imported
         * commands that refer to the "real" command.
         * The real command deletes these imported
         * commands on this list when it is
         * deleted. */
} ImportRef;

/*
 * Data structure used as the ClientData of imported commands: commands
 * created in an namespace when it imports a "real" command from another
 * namespace.
 */

typedef struct ImportedCmdData {
    struct Command *realCmdPtr;  /* "Real" command that this imported command
         * refers to. */
    struct Command *selfPtr;  /* Pointer to this imported command. Needed
         * only when deleting it in order to remove it
         * from the real command's linked list of
         * imported commands that refer to it. */
} ImportedCmdData;

/*
 * A Command structure exists for each command in a namespace. The Tcl_Command
 * opaque type actually refers to these structures.
 */

typedef struct Command {
    Tcl_HashEntry *hPtr;  /* Pointer to the hash table entry that refers
         * to this command. The hash table is either a
         * namespace's command table or an
         * interpreter's hidden command table. This
         * pointer is used to get a command's name
         * from its Tcl_Command handle. NULL means
         * that the hash table entry has been removed
         * already (this can happen if deleteProc
         * causes the command to be deleted or
         * recreated). */
    Namespace *nsPtr;    /* Points to the namespace containing this
         * command. */
    int refCount;    /* 1 if in command hashtable plus 1 for each
         * reference from a CmdName Tcl object
         * representing a command's name in a ByteCode
         * instruction sequence. This structure can be
         * freed when refCount becomes zero. */
    int cmdEpoch;    /* Incremented to invalidate any references
         * that point to this command when it is
         * renamed, deleted, hidden, or exposed. */
    CompileProc *compileProc;  /* Procedure called to compile command. NULL
         * if no compile proc exists for command. */
    Tcl_ObjCmdProc *objProc;  /* Object-based command procedure. */
    ClientData objClientData;  /* Arbitrary value passed to object proc. */
    Tcl_CmdProc *proc;    /* String-based command procedure. */
    ClientData clientData;  /* Arbitrary value passed to string proc. */
    Tcl_CmdDeleteProc *deleteProc;
        /* Procedure invoked when deleting command to,
         * e.g., free all client data. */
    ClientData deleteData;  /* Arbitrary value passed to deleteProc. */
    int flags;      /* Miscellaneous bits of information about
         * command. See below for definitions. */
    ImportRef *importRefPtr;  /* List of each imported Command created in
         * another namespace when this command is
         * imported. These imported commands redirect
         * invocations back to this command. The list
         * is used to remove all those imported
         * commands when deleting this "real"
         * command. */
    CommandTrace *tracePtr;  /* First in list of all traces set for this
         * command. */
} Command;

/*
 * Flag bits for commands.
 *
 * CMD_IS_DELETED -    Means that the command is in the process of
 *        being deleted (its deleteProc is currently
 *        executing). Other attempts to delete the
 *        command should be ignored.
 * CMD_TRACE_ACTIVE -    1 means that trace processing is currently
 *        underway for a rename/delete change. See the
 *        two flags below for which is currently being
 *        processed.
 * CMD_HAS_EXEC_TRACES -  1 means that this command has at least one
 *        execution trace (as opposed to simple
 *        delete/rename traces) in its tracePtr list.
 * TCL_TRACE_RENAME -    A rename trace is in progress. Further
 *        recursive renames will not be traced.
 * TCL_TRACE_DELETE -    A delete trace is in progress. Further
 *        recursive deletes will not be traced.
 * (these last two flags are defined in tcl.h)
 */

#define CMD_IS_DELETED    0x1
#define CMD_TRACE_ACTIVE  0x2
#define CMD_HAS_EXEC_TRACES  0x4

/*
 *----------------------------------------------------------------
 * Data structures related to name resolution procedures.
 *----------------------------------------------------------------
 */

/*
 * The interpreter keeps a linked list of name resolution schemes. The scheme
 * for a namespace is consulted first, followed by the list of schemes in an
 * interpreter, followed by the default name resolution in Tcl. Schemes are
 * added/removed from the interpreter's list by calling Tcl_AddInterpResolver
 * and Tcl_RemoveInterpResolver.
 */

typedef struct ResolverScheme {
    char *name;      /* Name identifying this scheme. */
    Tcl_ResolveCmdProc *cmdResProc;
        /* Procedure handling command name
         * resolution. */
    Tcl_ResolveVarProc *varResProc;
        /* Procedure handling variable name resolution
         * for variables that can only be handled at
         * runtime. */
    Tcl_ResolveCompiledVarProc *compiledVarResProc;
        /* Procedure handling variable name resolution
         * at compile time. */

    struct ResolverScheme *nextPtr;
        /* Pointer to next record in linked list. */
} ResolverScheme;

/*
 * Forward declaration of the TIP#143 limit handler structure.
 */

typedef struct LimitHandler LimitHandler;

/*
 * TIP #268.
 * Values for the selection mode, i.e the package require preferences.
 */

enum PkgPreferOptions {
    PKG_PREFER_LATEST, PKG_PREFER_STABLE
};

/*
 *----------------------------------------------------------------
 * This structure defines an interpreter, which is a collection of commands
 * plus other state information related to interpreting commands, such as
 * variable storage. Primary responsibility for this data structure is in
 * tclBasic.c, but almost every Tcl source file uses something in here.
 *----------------------------------------------------------------
 */

typedef struct Interp {
    /*
     * Note: the first three fields must match exactly the fields in a
     * Tcl_Interp struct (see tcl.h). If you change one, be sure to change the
     * other.
     *
     * The interpreter's result is held in both the string and the
     * objResultPtr fields. These fields hold, respectively, the result's
     * string or object value. The interpreter's result is always in the
     * result field if that is non-empty, otherwise it is in objResultPtr.
     * The two fields are kept consistent unless some C code sets
     * interp->result directly. Programs should not access result and
     * objResultPtr directly; instead, they should always get and set the
     * result using procedures such as Tcl_SetObjResult, Tcl_GetObjResult, and
     * Tcl_GetStringResult. See the SetResult man page for details.
     */

    char *result;    /* If the last command returned a string
         * result, this points to it. Should not be
         * accessed directly; see comment above. */
    Tcl_FreeProc *freeProc;  /* Zero means a string result is statically
         * allocated. TCL_DYNAMIC means string result
         * was allocated with ckalloc and should be
         * freed with ckfree. Other values give
         * address of procedure to invoke to free the
         * string result. Tcl_Eval must free it before
         * executing next command. */
    int errorLine;    /* When TCL_ERROR is returned, this gives the
         * line number in the command where the error
         * occurred (1 means first line). */
    struct TclStubs *stubTable;  /* Pointer to the exported Tcl stub table. On
         * previous versions of Tcl this is a pointer
         * to the objResultPtr or a pointer to a
         * buckets array in a hash table. We therefore
         * have to do some careful checking before we
         * can use this. */

    TclHandle handle;    /* Handle used to keep track of when this
         * interp is deleted. */

    Namespace *globalNsPtr;  /* The interpreter's global namespace. */
    Tcl_HashTable *hiddenCmdTablePtr;
        /* Hash table used by tclBasic.c to keep track
         * of hidden commands on a per-interp
         * basis. */
    ClientData interpInfo;  /* Information used by tclInterp.c to keep
         * track of master/slave interps on a
         * per-interp basis. */
    Tcl_HashTable unused2;  /* No longer used (was mathFuncTable) */

    /*
     * Information related to procedures and variables. See tclProc.c and
     * tclVar.c for usage.
     */

    int numLevels;    /* Keeps track of how many nested calls to
         * Tcl_Eval are in progress for this
         * interpreter. It's used to delay deletion of
         * the table until all Tcl_Eval invocations
         * are completed. */
    int maxNestingDepth;  /* If numLevels exceeds this value then Tcl
         * assumes that infinite recursion has
         * occurred and it generates an error. */
    CallFrame *framePtr;  /* Points to top-most in stack of all nested
         * procedure invocations. */
    CallFrame *varFramePtr;  /* Points to the call frame whose variables
         * are currently in use (same as framePtr
         * unless an "uplevel" command is
         * executing). */
    ActiveVarTrace *activeVarTracePtr;
        /* First in list of active traces for interp,
         * or NULL if no active traces. */
    int returnCode;    /* [return -code] parameter */
    CallFrame *rootFramePtr;  /* Global frame pointer for this interpreter */
    Namespace *lookupNsPtr;  /* Namespace to use ONLY on the next
         * TCL_EVAL_INVOKE call to Tcl_EvalObjv */

    /*
     * Information used by Tcl_AppendResult to keep track of partial results.
     * See Tcl_AppendResult code for details.
     */

    char *appendResult;    /* Storage space for results generated by
         * Tcl_AppendResult. Ckalloc-ed. NULL means
         * not yet allocated. */
    int appendAvl;    /* Total amount of space available at
         * partialResult. */
    int appendUsed;    /* Number of non-null bytes currently stored
         * at partialResult. */

    /*
     * Information about packages. Used only in tclPkg.c.
     */

    Tcl_HashTable packageTable;  /* Describes all of the packages loaded in or
         * available to this interpreter. Keys are
         * package names, values are (Package *)
         * pointers. */
    char *packageUnknown;  /* Command to invoke during "package require"
         * commands for packages that aren't described
         * in packageTable. Ckalloc'ed, may be
         * NULL. */
    /*
     * Miscellaneous information:
     */

    int cmdCount;    /* Total number of times a command procedure
         * has been called for this interpreter. */
    int evalFlags;    /* Flags to control next call to Tcl_Eval.
         * Normally zero, but may be set before
         * calling Tcl_Eval. See below for valid
         * values. */
    int unused1;    /* No longer used (was termOffset) */
    LiteralTable literalTable;  /* Contains LiteralEntry's describing all Tcl
         * objects holding literals of scripts
         * compiled by the interpreter. Indexed by the
         * string representations of literals. Used to
         * avoid creating duplicate objects. */
    int compileEpoch;    /* Holds the current "compilation epoch" for
         * this interpreter. This is incremented to
         * invalidate existing ByteCodes when, e.g., a
         * command with a compile procedure is
         * redefined. */
    Proc *compiledProcPtr;  /* If a procedure is being compiled, a pointer
         * to its Proc structure; otherwise, this is
         * NULL. Set by ObjInterpProc in tclProc.c and
         * used by tclCompile.c to process local
         * variables appropriately. */
    ResolverScheme *resolverPtr;
        /* Linked list of name resolution schemes
         * added to this interpreter. Schemes are
         * added and removed by calling
         * Tcl_AddInterpResolvers and
         * Tcl_RemoveInterpResolver respectively. */
    Tcl_Obj *scriptFile;  /* NULL means there is no nested source
         * command active; otherwise this points to
         * pathPtr of the file being sourced. */
    int flags;      /* Various flag bits. See below. */
    long randSeed;    /* Seed used for rand() function. */
    Trace *tracePtr;    /* List of traces for this interpreter. */
    Tcl_HashTable *assocData;  /* Hash table for associating data with this
         * interpreter. Cleaned up when this
         * interpreter is deleted. */
    struct ExecEnv *execEnvPtr;  /* Execution environment for Tcl bytecode
         * execution. Contains a pointer to the Tcl
         * evaluation stack. */
    Tcl_Obj *emptyObjPtr;  /* Points to an object holding an empty
         * string. Returned by Tcl_ObjSetVar2 when
         * variable traces change a variable in a
         * gross way. */
    char resultSpace[TCL_RESULT_SIZE+1];
        /* Static space holding small results. */
    Tcl_Obj *objResultPtr;  /* If the last command returned an object
         * result, this points to it. Should not be
         * accessed directly; see comment above. */
    Tcl_ThreadId threadId;  /* ID of thread that owns the interpreter */

    ActiveCommandTrace *activeCmdTracePtr;
        /* First in list of active command traces for
         * interp, or NULL if no active traces. */
    ActiveInterpTrace *activeInterpTracePtr;
        /* First in list of active traces for interp,
         * or NULL if no active traces. */

    int tracesForbiddingInline; /* Count of traces (in the list headed by
         * tracePtr) that forbid inline bytecode
         * compilation */

    /* Fields used to manage extensible return options (TIP 90) */
    Tcl_Obj *returnOpts;  /* A dictionary holding the options to the
         * last [return] command */

    Tcl_Obj *errorInfo;    /* errorInfo value (now as a Tcl_Obj) */
    Tcl_Obj *eiVar;    /* cached ref to ::errorInfo variable */
    Tcl_Obj *errorCode;    /* errorCode value (now as a Tcl_Obj) */
    Tcl_Obj *ecVar;    /* cached ref to ::errorInfo variable */
    int returnLevel;    /* [return -level] parameter */

    /*
     * Resource limiting framework support (TIP#143).
     */

    struct {
  int active;    /* Flag values defining which limits have been
         * set. */
  int granularityTicker;  /* Counter used to determine how often to
         * check the limits. */
  int exceeded;    /* Which limits have been exceeded, described
         * as flag values the same as the 'active'
         * field. */

  int cmdCount;    /* Limit for how many commands to execute in
         * the interpreter. */
  LimitHandler *cmdHandlers;
        /* Handlers to execute when the limit is
         * reached. */
  int cmdGranularity;  /* Mod factor used to determine how often to
         * evaluate the limit check. */

  Tcl_Time time;    /* Time limit for execution within the
         * interpreter. */
  LimitHandler *timeHandlers;
        /* Handlers to execute when the limit is
         * reached. */
  int timeGranularity;  /* Mod factor used to determine how often to
         * evaluate the limit check. */
  Tcl_TimerToken timeEvent;
        /* Handle for a timer callback that will occur
         * when the time-limit is exceeded. */

  Tcl_HashTable callbacks;/* Mapping from (interp,type) pair to data
         * used to install a limit handler callback to
         * run in _this_ interp when the limit is
         * exceeded. */
    } limit;

    /*
     * Information for improved default error generation from ensembles
     * (TIP#112).
     */

    struct {
  Tcl_Obj *const *sourceObjs;
        /* What arguments were actually input into the
         * *root* ensemble command? (Nested ensembles
         * don't rewrite this.) NULL if we're not
         * processing an ensemble. */
  int numRemovedObjs;  /* How many arguments have been stripped off
         * because of ensemble processing. */
  int numInsertedObjs;  /* How many of the current arguments were
         * inserted by an ensemble. */
    } ensembleRewrite;

    /*
     * TIP #219 ... Global info for the I/O system ...
     */

    Tcl_Obj *chanMsg;    /* Error message set by channel drivers, for
         * the propagation of arbitrary Tcl errors.
         * This information, if present (chanMsg not
         * NULL), takes precedence over a POSIX error
         * code returned by a channel operation. */

    /* TIP #280 */
    CmdFrame *cmdFramePtr;  /* Points to the command frame containing
         * the location information for the current
         * command. */
    const CmdFrame *invokeCmdFramePtr;
        /* Points to the command frame which is the
         * invoking context of the bytecode compiler.
         * NULL when the byte code compiler is not
         * active */
    int invokeWord;    /* Index of the word in the command which
         * is getting compiled. */
    Tcl_HashTable *linePBodyPtr;/* This table remembers for each statically
         * defined procedure the location information
         * for its body. It is keyed by the address of
         * the Proc structure for a procedure. */
    Tcl_HashTable *lineBCPtr;  /* This table remembers for each ByteCode
         * object the location information for its
         * body. It is keyed by the address of the
         * Proc structure for a procedure. */
    /*
     * TIP #268. The currently active selection mode, i.e. the package require
     * preferences.
     */

    int packagePrefer;    /* Current package selection mode. */

    /*
     * Hashtables for variable traces and searches
     */

    Tcl_HashTable varTraces;  /* Hashtable holding the start of a variable's
         * active trace list; varPtr is the key. */
    Tcl_HashTable varSearches;  /* Hashtable holding the start of a variable's
         * active searches list; varPtr is the key */
    /*
     * The thread-specific data ekeko: cache pointers or values that
     *  (a) do not change during the thread's lifetime
     *  (b) require access to TSD to determine at runtime
     *  (c) are accessed very often (e.g., at each command call)
     *
     * Note that these are the same for all interps in the same thread. They
     * just have to be initialised for the thread's master interp, slaves
     * inherit the value.
     *
     * They are used by the macros defined below.
     */

    void *allocCache;
    void *pendingObjDataPtr;  /* Pointer to the Cache and PendingObjData
         * structs for this interp's thread; see
         * tclObj.c and tclThreadAlloc.c */
    int *asyncReadyPtr;    /* Pointer to the asyncReady indicator for
         * this interp's thread; see tclAsync.c */
    int *stackBound;    /* Pointer to the limit stack address
         * allowable for invoking a new command
         * without "risking" a C-stack overflow; see
         * TclpCheckStackSpace in the platform's
         * directory. */


#ifdef TCL_COMPILE_STATS
    /*
     * Statistical information about the bytecode compiler and interpreter's
     * operation.
     */

    ByteCodeStats stats;  /* Holds compilation and execution statistics
         * for this interpreter. */
#endif /* TCL_COMPILE_STATS */
} Interp;

/*
 * Macros that use the TSD-ekeko.
 */

#define TclAsyncReady(iPtr) \
    *((iPtr)->asyncReadyPtr)


/*
 * General list of interpreters. Doubly linked for easier removal of items
 * deep in the list.
 */

typedef struct InterpList {
    Interp *interpPtr;
    struct InterpList *prevPtr;
    struct InterpList *nextPtr;
} InterpList;

/*
 * Macros for splicing into and out of doubly linked lists. They assume
 * existence of struct items 'prevPtr' and 'nextPtr'.
 *
 * a = element to add or remove.
 * b = list head.
 *
 * TclSpliceIn adds to the head of the list.
 */

#define TclSpliceIn(a,b)      \
    (a)->nextPtr = (b);        \
    if ((b) != NULL) {        \
  (b)->prevPtr = (a);      \
    }            \
    (a)->prevPtr = NULL, (b) = (a);

#define TclSpliceOut(a,b)      \
    if ((a)->prevPtr != NULL) {      \
  (a)->prevPtr->nextPtr = (a)->nextPtr;  \
    } else {          \
  (b) = (a)->nextPtr;      \
    }            \
    if ((a)->nextPtr != NULL) {      \
  (a)->nextPtr->prevPtr = (a)->prevPtr;  \
    }

/*
 * EvalFlag bits for Interp structures:
 *
 * TCL_ALLOW_EXCEPTIONS  1 means it's OK for the script to terminate with a
 *      code other than TCL_OK or TCL_ERROR; 0 means codes
 *      other than these should be turned into errors.
 */

#define TCL_ALLOW_EXCEPTIONS  4
#define TCL_EVAL_FILE             2
#define TCL_EVAL_CTX              8

/*
 * Flag bits for Interp structures:
 *
 * DELETED:    Non-zero means the interpreter has been deleted:
 *      don't process any more commands for it, and destroy
 *      the structure as soon as all nested invocations of
 *      Tcl_Eval are done.
 * ERR_ALREADY_LOGGED:  Non-zero means information has already been logged in
 *      iPtr->errorInfo for the current Tcl_Eval instance, so
 *      Tcl_Eval needn't log it (used to implement the "error
 *      message log" command).
 * DONT_COMPILE_CMDS_INLINE: Non-zero means that the bytecode compiler should
 *      not compile any commands into an inline sequence of
 *      instructions. This is set 1, for example, when command
 *      traces are requested.
 * RAND_SEED_INITIALIZED: Non-zero means that the randSeed value of the interp
 *      has not be initialized. This is set 1 when we first
 *      use the rand() or srand() functions.
 * SAFE_INTERP:    Non zero means that the current interp is a safe
 *      interp (i.e. it has only the safe commands installed,
 *      less priviledge than a regular interp).
 * INTERP_TRACE_IN_PROGRESS: Non-zero means that an interp trace is currently
 *      active; so no further trace callbacks should be
 *      invoked.
 * INTERP_ALTERNATE_WRONG_ARGS: Used for listing second and subsequent forms
 *      of the wrong-num-args string in Tcl_WrongNumArgs.
 *      Makes it append instead of replacing and uses
 *      different intermediate text.
 *
 * WARNING: For the sake of some extensions that have made use of former
 * internal values, do not re-use the flag values 2 (formerly ERR_IN_PROGRESS)
 * or 8 (formerly ERROR_CODE_SET).
 */

#define DELETED            1
#define ERR_ALREADY_LOGGED        4
#define DONT_COMPILE_CMDS_INLINE   0x20
#define RAND_SEED_INITIALIZED     0x40
#define SAFE_INTERP       0x80
#define INTERP_TRACE_IN_PROGRESS  0x200
#define INTERP_ALTERNATE_WRONG_ARGS  0x400
#define ERR_LEGACY_COPY      0x800

/*
 * Maximum number of levels of nesting permitted in Tcl commands (used to
 * catch infinite recursion).
 */

#define MAX_NESTING_DEPTH  1000

/*
 * TIP#143 limit handler internal representation.
 */

struct LimitHandler {
    int flags;      /* The state of this particular handler. */
    Tcl_LimitHandlerProc *handlerProc;
        /* The handler callback. */
    ClientData clientData;  /* Opaque argument to the handler callback. */
    Tcl_LimitHandlerDeleteProc *deleteProc;
        /* How to delete the clientData */
    LimitHandler *prevPtr;  /* Previous item in linked list of handlers */
    LimitHandler *nextPtr;  /* Next item in linked list of handlers */
};

/*
 * Values for the LimitHandler flags field.
 *  LIMIT_HANDLER_ACTIVE - Whether the handler is currently being
 *    processed; handlers are never to be entered reentrantly.
 *  LIMIT_HANDLER_DELETED - Whether the handler has been deleted. This
 *    should not normally be observed because when a handler is
 *    deleted it is also spliced out of the list of handlers, but
 *    even so we will be careful.
 */

#define LIMIT_HANDLER_ACTIVE  0x01
#define LIMIT_HANDLER_DELETED  0x02

/*
 * The macro below is used to modify a "char" value (e.g. by casting it to an
 * unsigned character) so that it can be used safely with macros such as
 * isspace.
 */

#define UCHAR(c) ((unsigned char) (c))

/*
 * This macro is used to properly align the memory allocated by Tcl, giving
 * the same alignment as the native malloc
 */

#if defined(__APPLE__)
#define TCL_ALLOCALIGN  16
#else
#define TCL_ALLOCALIGN  (2*sizeof(void *))
#endif

/*
 * This macro is used to determine the offset needed to safely allocate any
 * data structure in memory. Given a starting offset or size, it "rounds up"
 * or "aligns" the offset to the next 8-byte boundary so that any data
 * structure can be placed at the resulting offset without fear of an
 * alignment error.
 *
 * WARNING!! DO NOT USE THIS MACRO TO ALIGN POINTERS: it will produce the
 * wrong result on platforms that allocate addresses that are divisible by 4
 * or 2. Only use it for offsets or sizes.
 *
 * This macro is only used by tclCompile.c in the core (Bug 926445). It
 * however not be made file static, as extensions that touch bytecodes
 * (notably tbcload) require it.
 */

#define TCL_ALIGN(x) (((int)(x) + 7) & ~7)


/*
 * The following enum values are used to specify the runtime platform setting
 * of the tclPlatform variable.
 */

typedef enum {
    TCL_PLATFORM_UNIX = 0,  /* Any Unix-like OS. */
    TCL_PLATFORM_WINDOWS = 2  /* Any Microsoft Windows OS. */
} TclPlatformType;

/*
 * The following enum values are used to indicate the translation of a Tcl
 * channel. Declared here so that each platform can define
 * TCL_PLATFORM_TRANSLATION to the native translation on that platform
 */

typedef enum TclEolTranslation {
    TCL_TRANSLATE_AUTO,    /* Eol == \r, \n and \r\n. */
    TCL_TRANSLATE_CR,    /* Eol == \r. */
    TCL_TRANSLATE_LF,    /* Eol == \n. */
    TCL_TRANSLATE_CRLF    /* Eol == \r\n. */
} TclEolTranslation;

/*
 * Flags for TclInvoke:
 *
 * TCL_INVOKE_HIDDEN    Invoke a hidden command; if not set, invokes
 *        an exposed command.
 * TCL_INVOKE_NO_UNKNOWN  If set, "unknown" is not invoked if the
 *        command to be invoked is not found. Only has
 *        an effect if invoking an exposed command,
 *        i.e. if TCL_INVOKE_HIDDEN is not also set.
 * TCL_INVOKE_NO_TRACEBACK  Does not record traceback information if the
 *        invoked command returns an error. Used if the
 *        caller plans on recording its own traceback
 *        information.
 */

#define  TCL_INVOKE_HIDDEN  (1<<0)
#define TCL_INVOKE_NO_UNKNOWN  (1<<1)
#define TCL_INVOKE_NO_TRACEBACK  (1<<2)

/*
 * The structure used as the internal representation of Tcl list objects. This
 * struct is grown (reallocated and copied) as necessary to hold all the
 * list's element pointers. The struct might contain more slots than currently
 * used to hold all element pointers. This is done to make append operations
 * faster.
 */

typedef struct List {
    int refCount;
    int maxElemCount;    /* Total number of element array slots. */
    int elemCount;    /* Current number of list elements. */
    int canonicalFlag;    /* Set if the string representation was
         * derived from the list representation. May
         * be ignored if there is no string rep at
         * all.*/
    Tcl_Obj *elements;    /* First list element; the struct is grown to
         * accomodate all elements. */
} List;

/*
 * Macro used to get the elements of a list object.
 */

#define ListRepPtr(listPtr) \
    ((List *) (listPtr)->internalRep.twoPtrValue.ptr1)

#define ListObjGetElements(listPtr, objc, objv) \
    ((objv) = &(ListRepPtr(listPtr)->elements), \
     (objc) = ListRepPtr(listPtr)->elemCount)

#define ListObjLength(listPtr, len) \
    ((len) = ListRepPtr(listPtr)->elemCount)

#define TclListObjGetElements(interp, listPtr, objcPtr, objvPtr) \
    (((listPtr)->typePtr == &tclListType) \
      ? ((ListObjGetElements((listPtr), *(objcPtr), *(objvPtr))), TCL_OK)\
      : Tcl_ListObjGetElements((interp), (listPtr), (objcPtr), (objvPtr)))

#define TclListObjLength(interp, listPtr, lenPtr) \
    (((listPtr)->typePtr == &tclListType) \
      ? ((ListObjLength((listPtr), *(lenPtr))), TCL_OK)\
      : Tcl_ListObjLength((interp), (listPtr), (lenPtr)))

/*
 * Macros providing a faster path to integers: Tcl_GetLongFromObj everywhere,
 * Tcl_GetIntFromObj and TclGetIntForIndex on platforms where longs are ints.
 *
 * WARNING: these macros eval their args more than once.
 */

#define TclGetLongFromObj(interp, objPtr, longPtr) \
    (((objPtr)->typePtr == &tclIntType)  \
      ? ((*(longPtr) = (long) (objPtr)->internalRep.otherValuePtr), TCL_OK) \
      : Tcl_GetLongFromObj((interp), (objPtr), (longPtr)))

#if (LONG_MAX == INT_MAX)
#define TclGetIntFromObj(interp, objPtr, intPtr) \
    (((objPtr)->typePtr == &tclIntType)  \
      ? ((*(intPtr) = (long) (objPtr)->internalRep.otherValuePtr), TCL_OK) \
      : Tcl_GetIntFromObj((interp), (objPtr), (intPtr)))
#define TclGetIntForIndexM(interp, objPtr, endValue, idxPtr) \
    (((objPtr)->typePtr == &tclIntType)  \
      ? ((*(idxPtr) = (long) (objPtr)->internalRep.otherValuePtr), TCL_OK) \
      : TclGetIntForIndex((interp), (objPtr), (endValue), (idxPtr)))
#else
#define TclGetIntFromObj(interp, objPtr, intPtr) \
    Tcl_GetIntFromObj((interp), (objPtr), (intPtr))
#define TclGetIntForIndexM(interp, objPtr, ignore, idxPtr)  \
    TclGetIntForIndex(interp, objPtr, ignore, idxPtr)
#endif

/*
 * Flag values for TclTraceDictPath().
 *
 * DICT_PATH_READ indicates that all entries on the path must exist but no
 * updates will be needed.
 *
 * DICT_PATH_UPDATE indicates that we are going to be doing an update at the
 * tip of the path, so duplication of shared objects should be done along the
 * way.
 *
 * DICT_PATH_EXISTS indicates that we are performing an existance test and a
 * lookup failure should therefore not be an error. If (and only if) this flag
 * is set, TclTraceDictPath() will return the special value
 * DICT_PATH_NON_EXISTENT if the path is not traceable.
 *
 * DICT_PATH_CREATE (which also requires the DICT_PATH_UPDATE bit to be set)
 * indicates that we are to create non-existant dictionaries on the path.
 */

#define DICT_PATH_READ    0
#define DICT_PATH_UPDATE  1
#define DICT_PATH_EXISTS  2
#define DICT_PATH_CREATE  5

#define DICT_PATH_NON_EXISTENT  ((Tcl_Obj *) (void *) 1)

/*
 *----------------------------------------------------------------
 * Data structures related to the filesystem internals
 *----------------------------------------------------------------
 */

/*
 * The version_2 filesystem is private to Tcl. As and when these changes have
 * been thoroughly tested and investigated a new public filesystem interface
 * will be released. The aim is more versatile virtual filesystem interfaces,
 * more efficiency in 'path' manipulation and usage, and cleaner filesystem
 * code internally.
 */

#define TCL_FILESYSTEM_VERSION_2  ((Tcl_FSVersion) 0x2)
typedef ClientData (TclFSGetCwdProc2) (ClientData clientData);

/*
 * The following types are used for getting and storing platform-specific file
 * attributes in tclFCmd.c and the various platform-versions of that file.
 * This is done to have as much common code as possible in the file attributes
 * code. For more information about the callbacks, see TclFileAttrsCmd in
 * tclFCmd.c.
 */

typedef int (TclGetFileAttrProc) (Tcl_Interp *interp, int objIndex,
  Tcl_Obj *fileName, Tcl_Obj **attrObjPtrPtr);
typedef int (TclSetFileAttrProc) (Tcl_Interp *interp, int objIndex,
  Tcl_Obj *fileName, Tcl_Obj *attrObjPtr);

typedef struct TclFileAttrProcs {
    TclGetFileAttrProc *getProc;/* The procedure for getting attrs. */
    TclSetFileAttrProc *setProc;/* The procedure for setting attrs. */
} TclFileAttrProcs;

/*
 * Opaque handle used in pipeline routines to encapsulate platform-dependent
 * state.
 */

typedef struct TclFile_ *TclFile;

/*
 * The "globParameters" argument of the function TclGlob is an or'ed
 * combination of the following values:
 */

#define TCL_GLOBMODE_NO_COMPLAIN  1
#define TCL_GLOBMODE_JOIN    2
#define TCL_GLOBMODE_DIR    4
#define TCL_GLOBMODE_TAILS    8

typedef enum Tcl_PathPart {
    TCL_PATH_DIRNAME,
    TCL_PATH_TAIL,
    TCL_PATH_EXTENSION,
    TCL_PATH_ROOT
} Tcl_PathPart;

/*
 *----------------------------------------------------------------
 * Data structures related to obsolete filesystem hooks
 *----------------------------------------------------------------
 */

typedef int (TclStatProc_) (CONST char *path, struct stat *buf);
typedef int (TclAccessProc_) (CONST char *path, int mode);
typedef Tcl_Channel (TclOpenFileChannelProc_) (Tcl_Interp *interp,
  CONST char *fileName, CONST char *modeString, int permissions);

/*
 *----------------------------------------------------------------
 * Data structures related to procedures
 *----------------------------------------------------------------
 */

typedef Tcl_CmdProc *TclCmdProcType;
typedef Tcl_ObjCmdProc *TclObjCmdProcType;

/*
 *----------------------------------------------------------------
 * Data structures for process-global values.
 *----------------------------------------------------------------
 */

typedef void (TclInitProcessGlobalValueProc) (char **valuePtr, int *lengthPtr,
  Tcl_Encoding *encodingPtr);

/*
 * A ProcessGlobalValue struct exists for each internal value in Tcl that is
 * to be shared among several threads. Each thread sees a (Tcl_Obj) copy of
 * the value, and the master is kept as a counted string, with epoch and mutex
 * control. Each ProcessGlobalValue struct should be a static variable in some
 * file.
 */

typedef struct ProcessGlobalValue {
    int epoch;      /* Epoch counter to detect changes in the
         * master value. */
    int numBytes;    /* Length of the master string. */
    char *value;    /* The master string value. */
    Tcl_Encoding encoding;  /* system encoding when master string was
         * initialized. */
    TclInitProcessGlobalValueProc *proc;
            /* A procedure to initialize the master string
         * copy when a "get" request comes in before
         * any "set" request has been received. */
    Tcl_Mutex mutex;    /* Enforce orderly access from multiple
         * threads. */
    Tcl_ThreadDataKey key;  /* Key for per-thread data holding the
         * (Tcl_Obj) copy for each thread. */
} ProcessGlobalValue;

/*
 *----------------------------------------------------------------------
 * Flags for TclParseNumber
 *----------------------------------------------------------------------
 */

#define TCL_PARSE_DECIMAL_ONLY    1
        /* Leading zero doesn't denote octal or hex */
#define TCL_PARSE_OCTAL_ONLY    2
        /* Parse octal even without prefix */
#define TCL_PARSE_HEXADECIMAL_ONLY  4
        /* Parse hexadecimal even without prefix */
#define TCL_PARSE_INTEGER_ONLY    8
        /* Disable floating point parsing */
#define TCL_PARSE_SCAN_PREFIXES    16
        /* Use [scan] rules dealing with 0? prefixes */
#define TCL_PARSE_NO_WHITESPACE    32
        /* Reject leading/trailing whitespace */

/*
 *----------------------------------------------------------------------
 * Type values TclGetNumberFromObj
 *----------------------------------------------------------------------
 */

#define TCL_NUMBER_LONG    1
#define TCL_NUMBER_WIDE    2
#define TCL_NUMBER_BIG    3
#define TCL_NUMBER_DOUBLE  4
#define TCL_NUMBER_NAN    5

/*
 *----------------------------------------------------------------
 * Variables shared among Tcl modules but not used by the outside world.
 *----------------------------------------------------------------
 */

MODULE_SCOPE char *  tclNativeExecutableName;
MODULE_SCOPE int  tclFindExecutableSearchDone;
MODULE_SCOPE char *  tclMemDumpFileName;
MODULE_SCOPE TclPlatformType tclPlatform;
MODULE_SCOPE Tcl_NotifierProcs tclOriginalNotifier;

/*
 * TIP #233 (Virtualized Time)
 * Data for the time hooks, if any.
 */

MODULE_SCOPE Tcl_GetTimeProc*   tclGetTimeProcPtr;
MODULE_SCOPE Tcl_ScaleTimeProc* tclScaleTimeProcPtr;
MODULE_SCOPE ClientData    tclTimeClientData;

/*
 * Variables denoting the Tcl object types defined in the core.
 */

MODULE_SCOPE Tcl_ObjType tclBignumType;
MODULE_SCOPE Tcl_ObjType tclBooleanType;
MODULE_SCOPE Tcl_ObjType tclByteArrayType;
MODULE_SCOPE Tcl_ObjType tclByteCodeType;
MODULE_SCOPE Tcl_ObjType tclDoubleType;
MODULE_SCOPE Tcl_ObjType tclEndOffsetType;
MODULE_SCOPE Tcl_ObjType tclIntType;
MODULE_SCOPE Tcl_ObjType tclListType;
MODULE_SCOPE Tcl_ObjType tclDictType;
MODULE_SCOPE Tcl_ObjType tclProcBodyType;
MODULE_SCOPE Tcl_ObjType tclStringType;
MODULE_SCOPE Tcl_ObjType tclArraySearchType;
MODULE_SCOPE Tcl_ObjType tclEnsembleCmdType;
#ifndef NO_WIDE_TYPE
MODULE_SCOPE Tcl_ObjType tclWideIntType;
#endif
MODULE_SCOPE Tcl_ObjType tclRegexpType;

/*
 * Variables denoting the hash key types defined in the core.
 */

MODULE_SCOPE Tcl_HashKeyType tclArrayHashKeyType;
MODULE_SCOPE Tcl_HashKeyType tclOneWordHashKeyType;
MODULE_SCOPE Tcl_HashKeyType tclStringHashKeyType;
MODULE_SCOPE Tcl_HashKeyType tclObjHashKeyType;

/*
 * The head of the list of free Tcl objects, and the total number of Tcl
 * objects ever allocated and freed.
 */

MODULE_SCOPE Tcl_Obj *  tclFreeObjList;

#ifdef TCL_COMPILE_STATS
MODULE_SCOPE long  tclObjsAlloced;
MODULE_SCOPE long  tclObjsFreed;
#define TCL_MAX_SHARED_OBJ_STATS 5
MODULE_SCOPE long  tclObjsShared[TCL_MAX_SHARED_OBJ_STATS];
#endif /* TCL_COMPILE_STATS */

/*
 * Pointer to a heap-allocated string of length zero that the Tcl core uses as
 * the value of an empty string representation for an object. This value is
 * shared by all new objects allocated by Tcl_NewObj.
 */

MODULE_SCOPE char *  tclEmptyStringRep;
MODULE_SCOPE char  tclEmptyString;

/*
 *----------------------------------------------------------------
 * Procedures shared among Tcl modules but not used by the outside world:
 *----------------------------------------------------------------
 */

MODULE_SCOPE void       TclAdvanceLines(int *line, const char *start,
          const char *end);
MODULE_SCOPE int  TclArraySet(Tcl_Interp *interp,
          Tcl_Obj *arrayNameObj, Tcl_Obj *arrayElemObj);
MODULE_SCOPE double  TclBignumToDouble(mp_int *bignum);
MODULE_SCOPE int  TclByteArrayMatch(const unsigned char *string,
          int strLen, const unsigned char *pattern,
          int ptnLen, int flags);
MODULE_SCOPE double  TclCeil(mp_int *a);
MODULE_SCOPE int  TclCheckBadOctal(Tcl_Interp *interp,const char *value);
MODULE_SCOPE int  TclChanCaughtErrorBypass(Tcl_Interp *interp,
          Tcl_Channel chan);
MODULE_SCOPE void  TclCleanupLiteralTable(Tcl_Interp *interp,
          LiteralTable *tablePtr);
MODULE_SCOPE int  TclDoubleDigits(char *buf, double value, int *signum);
MODULE_SCOPE void       TclDeleteNamespaceVars(Namespace *nsPtr);
/* TIP #280 - Modified token based evulation, with line information */
MODULE_SCOPE int        TclEvalEx(Tcl_Interp *interp, const char *script,
          int numBytes, int flags, int line);
MODULE_SCOPE void  TclExpandTokenArray(Tcl_Parse *parsePtr);
MODULE_SCOPE int  TclFileAttrsCmd(Tcl_Interp *interp,
          int objc, Tcl_Obj *const objv[]);
MODULE_SCOPE int  TclFileCopyCmd(Tcl_Interp *interp,
          int objc, Tcl_Obj *const objv[]);
MODULE_SCOPE int  TclFileDeleteCmd(Tcl_Interp *interp,
          int objc, Tcl_Obj *const objv[]);
MODULE_SCOPE int  TclFileMakeDirsCmd(Tcl_Interp *interp,
          int objc, Tcl_Obj *const objv[]);
MODULE_SCOPE int  TclFileRenameCmd(Tcl_Interp *interp,
          int objc, Tcl_Obj *const objv[]);
MODULE_SCOPE void  TclFinalizeAllocSubsystem(void);
MODULE_SCOPE void  TclFinalizeAsync(void);
MODULE_SCOPE void  TclFinalizeDoubleConversion(void);
MODULE_SCOPE void  TclFinalizeEncodingSubsystem(void);
MODULE_SCOPE void  TclFinalizeEnvironment(void);
MODULE_SCOPE void  TclFinalizeExecution(void);
MODULE_SCOPE void  TclFinalizeIOSubsystem(void);
MODULE_SCOPE void  TclFinalizeFilesystem(void);
MODULE_SCOPE void  TclResetFilesystem(void);
MODULE_SCOPE void  TclFinalizeLoad(void);
MODULE_SCOPE void  TclFinalizeLock(void);
MODULE_SCOPE void  TclFinalizeMemorySubsystem(void);
MODULE_SCOPE void  TclFinalizeNotifier(void);
MODULE_SCOPE void  TclFinalizeObjects(void);
MODULE_SCOPE void  TclFinalizePreserve(void);
MODULE_SCOPE void  TclFinalizeSynchronization(void);
MODULE_SCOPE void  TclFinalizeThreadAlloc(void);
MODULE_SCOPE void  TclFinalizeThreadData(void);
MODULE_SCOPE double  TclFloor(mp_int *a);
MODULE_SCOPE void  TclFormatNaN(double value, char *buffer);
MODULE_SCOPE int  TclFSFileAttrIndex(Tcl_Obj *pathPtr,
          const char *attributeName, int *indexPtr);
MODULE_SCOPE int *      TclGetAsyncReadyPtr(void);
MODULE_SCOPE Tcl_Obj *  TclGetBgErrorHandler(Tcl_Interp *interp);
MODULE_SCOPE int  TclGetChannelFromObj(Tcl_Interp *interp,
          Tcl_Obj *objPtr, Tcl_Channel *chanPtr,
          int *modePtr, int flags);
MODULE_SCOPE int  TclGetNumberFromObj(Tcl_Interp *interp,
          Tcl_Obj *objPtr, ClientData *clientDataPtr,
          int *typePtr);
MODULE_SCOPE int  TclGetOpenModeEx(Tcl_Interp *interp,
          const char *modeString, int *seekFlagPtr,
          int *binaryPtr);
MODULE_SCOPE Tcl_Obj *  TclGetProcessGlobalValue(ProcessGlobalValue *pgvPtr);
MODULE_SCOPE const char *TclGetSrcInfoForCmd(Interp *iPtr, int *lenPtr);
MODULE_SCOPE int  TclGlob(Tcl_Interp *interp, char *pattern,
          Tcl_Obj *unquotedPrefix, int globFlags,
          Tcl_GlobTypeData *types);
MODULE_SCOPE int  TclIncrObj(Tcl_Interp *interp, Tcl_Obj *valuePtr,
          Tcl_Obj *incrPtr);
MODULE_SCOPE Tcl_Obj *  TclIncrObjVar2(Tcl_Interp *interp, Tcl_Obj *part1Ptr,
          Tcl_Obj *part2Ptr, Tcl_Obj *incrPtr, int flags);
MODULE_SCOPE int  TclInfoExistsCmd(ClientData dummy, Tcl_Interp *interp,
          int objc, Tcl_Obj *const objv[]);
MODULE_SCOPE Tcl_Obj *  TclInfoFrame(Tcl_Interp *interp, CmdFrame *framePtr);
MODULE_SCOPE int  TclInfoGlobalsCmd(ClientData dummy, Tcl_Interp *interp,
          int objc, Tcl_Obj *const objv[]);
MODULE_SCOPE int  TclInfoLocalsCmd(ClientData dummy, Tcl_Interp *interp,
          int objc, Tcl_Obj *const objv[]);
MODULE_SCOPE int  TclInfoVarsCmd(ClientData dummy, Tcl_Interp *interp,
          int objc, Tcl_Obj *const objv[]);
MODULE_SCOPE void  TclInitAlloc(void);
MODULE_SCOPE void  TclInitDbCkalloc(void);
MODULE_SCOPE void  TclInitDoubleConversion(void);
MODULE_SCOPE void  TclInitEmbeddedConfigurationInformation(
          Tcl_Interp *interp);
MODULE_SCOPE void  TclInitEncodingSubsystem(void);
MODULE_SCOPE void  TclInitIOSubsystem(void);
MODULE_SCOPE void  TclInitLimitSupport(Tcl_Interp *interp);
MODULE_SCOPE void  TclInitNamespaceSubsystem(void);
MODULE_SCOPE void  TclInitNotifier(void);
MODULE_SCOPE void  TclInitObjSubsystem(void);
MODULE_SCOPE void  TclInitSubsystems(void);
MODULE_SCOPE int  TclInterpReady(Tcl_Interp *interp);
MODULE_SCOPE int  TclIsLocalScalar(const char *src, int len);
MODULE_SCOPE int  TclJoinThread(Tcl_ThreadId id, int *result);
MODULE_SCOPE void  TclLimitRemoveAllHandlers(Tcl_Interp *interp);
MODULE_SCOPE Tcl_Obj *  TclLindexList(Tcl_Interp *interp,
          Tcl_Obj *listPtr, Tcl_Obj *argPtr);
MODULE_SCOPE Tcl_Obj *  TclLindexFlat(Tcl_Interp *interp, Tcl_Obj *listPtr,
          int indexCount, Tcl_Obj *const indexArray[]);
/* TIP #280 */
MODULE_SCOPE void       TclListLines(const char *listStr, int line, int n,
          int *lines);
MODULE_SCOPE Tcl_Obj *  TclListObjCopy(Tcl_Interp *interp, Tcl_Obj *listPtr);
MODULE_SCOPE int  TclLoadFile(Tcl_Interp *interp, Tcl_Obj *pathPtr,
          int symc, const char *symbols[],
          Tcl_PackageInitProc **procPtrs[],
          Tcl_LoadHandle *handlePtr,
          ClientData *clientDataPtr,
          Tcl_FSUnloadFileProc **unloadProcPtr);
MODULE_SCOPE Tcl_Obj *  TclLsetList(Tcl_Interp *interp, Tcl_Obj *listPtr,
          Tcl_Obj *indexPtr, Tcl_Obj *valuePtr);
MODULE_SCOPE Tcl_Obj *  TclLsetFlat(Tcl_Interp *interp, Tcl_Obj *listPtr,
          int indexCount, Tcl_Obj *const indexArray[],
          Tcl_Obj *valuePtr);
MODULE_SCOPE Tcl_Command TclMakeEnsemble(Tcl_Interp *interp, const char *name,
          const EnsembleImplMap map[]);
MODULE_SCOPE int        TclMarkList(Tcl_Interp *interp, const char *list,
          const char *end, int *argcPtr,
          const int **argszPtr, const char ***argvPtr);
MODULE_SCOPE int  TclMergeReturnOptions(Tcl_Interp *interp, int objc,
          Tcl_Obj *const objv[], Tcl_Obj **optionsPtrPtr,
          int *codePtr, int *levelPtr);
MODULE_SCOPE int  TclNokia770Doubles();
MODULE_SCOPE void       TclObjVarErrMsg(Tcl_Interp *interp, Tcl_Obj *part1Ptr,
                      Tcl_Obj *part2Ptr, const char *operation,
                      const char *reason, int index);
MODULE_SCOPE int  TclObjInvokeNamespace(Tcl_Interp *interp,
          int objc, Tcl_Obj *const objv[],
          Tcl_Namespace *nsPtr, int flags);
MODULE_SCOPE int  TclObjUnsetVar2(Tcl_Interp *interp,
          Tcl_Obj *part1Ptr, Tcl_Obj *part2Ptr, int flags);
MODULE_SCOPE int  TclParseBackslash(const char *src,
          int numBytes, int *readPtr, char *dst);
MODULE_SCOPE int  TclParseHex(const char *src, int numBytes,
          Tcl_UniChar *resultPtr);
MODULE_SCOPE int  TclParseNumber(Tcl_Interp *interp, Tcl_Obj *objPtr,
          const char *expected, const char *bytes,
          int numBytes, const char **endPtrPtr, int flags);
MODULE_SCOPE void  TclParseInit(Tcl_Interp *interp, const char *string,
          int numBytes, Tcl_Parse *parsePtr);
MODULE_SCOPE int  TclParseAllWhiteSpace(const char *src, int numBytes);
MODULE_SCOPE int  TclProcessReturn(Tcl_Interp *interp,
          int code, int level, Tcl_Obj *returnOpts);
#ifndef TCL_NO_STACK_CHECK
MODULE_SCOPE int        TclpGetCStackParams(int **stackBoundPtr);
#endif
MODULE_SCOPE int  TclpObjLstat(Tcl_Obj *pathPtr, Tcl_StatBuf *buf);
MODULE_SCOPE Tcl_Obj *  TclpTempFileName(void);
MODULE_SCOPE Tcl_Obj *  TclNewFSPathObj(Tcl_Obj *dirPtr, const char *addStrRep,
          int len);
MODULE_SCOPE int  TclpDeleteFile(const char *path);
MODULE_SCOPE void  TclpFinalizeCondition(Tcl_Condition *condPtr);
MODULE_SCOPE void  TclpFinalizeMutex(Tcl_Mutex *mutexPtr);
MODULE_SCOPE void  TclpFinalizePipes(void);
MODULE_SCOPE void  TclpFinalizeSockets(void);
MODULE_SCOPE int  TclpThreadCreate(Tcl_ThreadId *idPtr,
          Tcl_ThreadCreateProc proc, ClientData clientData,
          int stackSize, int flags);
MODULE_SCOPE int  TclpFindVariable(const char *name, int *lengthPtr);
MODULE_SCOPE void  TclpInitLibraryPath(char **valuePtr,
          int *lengthPtr, Tcl_Encoding *encodingPtr);
MODULE_SCOPE void  TclpInitLock(void);
MODULE_SCOPE void  TclpInitPlatform(void);
MODULE_SCOPE void  TclpInitUnlock(void);
MODULE_SCOPE int  TclpLoadFile(Tcl_Interp *interp, Tcl_Obj *pathPtr,
          const char *sym1, const char *sym2,
          Tcl_PackageInitProc **proc1Ptr,
          Tcl_PackageInitProc **proc2Ptr,
          ClientData *clientDataPtr,
          Tcl_FSUnloadFileProc **unloadProcPtr);
MODULE_SCOPE Tcl_Obj *  TclpObjListVolumes(void);
MODULE_SCOPE void  TclpMasterLock(void);
MODULE_SCOPE void  TclpMasterUnlock(void);
MODULE_SCOPE int  TclpMatchFiles(Tcl_Interp *interp, char *separators,
          Tcl_DString *dirPtr, char *pattern, char *tail);
MODULE_SCOPE int  TclpObjNormalizePath(Tcl_Interp *interp,
          Tcl_Obj *pathPtr, int nextCheckpoint);
MODULE_SCOPE void  TclpNativeJoinPath(Tcl_Obj *prefix, char *joining);
MODULE_SCOPE Tcl_Obj *  TclpNativeSplitPath(Tcl_Obj *pathPtr, int *lenPtr);
MODULE_SCOPE Tcl_PathType TclpGetNativePathType(Tcl_Obj *pathPtr,
          int *driveNameLengthPtr, Tcl_Obj **driveNameRef);
MODULE_SCOPE int   TclCrossFilesystemCopy(Tcl_Interp *interp,
          Tcl_Obj *source, Tcl_Obj *target);
MODULE_SCOPE int  TclpMatchInDirectory(Tcl_Interp *interp,
          Tcl_Obj *resultPtr, Tcl_Obj *pathPtr,
          const char *pattern, Tcl_GlobTypeData *types);
MODULE_SCOPE ClientData  TclpGetNativeCwd(ClientData clientData);
MODULE_SCOPE Tcl_FSDupInternalRepProc TclNativeDupInternalRep;
MODULE_SCOPE Tcl_Obj*  TclpObjLink(Tcl_Obj *pathPtr, Tcl_Obj *toPtr,
          int linkType);
MODULE_SCOPE int  TclpObjChdir(Tcl_Obj *pathPtr);
MODULE_SCOPE Tcl_Obj *  TclPathPart(Tcl_Interp *interp, Tcl_Obj *pathPtr,
          Tcl_PathPart portion);
MODULE_SCOPE void  TclpPanic(const char *format, ...);
MODULE_SCOPE char *  TclpReadlink(const char *fileName,
          Tcl_DString *linkPtr);
MODULE_SCOPE void  TclpReleaseFile(TclFile file);
MODULE_SCOPE void  TclpSetInterfaces(void);
MODULE_SCOPE void  TclpSetVariables(Tcl_Interp *interp);
MODULE_SCOPE void  TclpUnloadFile(Tcl_LoadHandle loadHandle);
MODULE_SCOPE void *  TclpThreadDataKeyGet(Tcl_ThreadDataKey *keyPtr);
MODULE_SCOPE void  TclpThreadDataKeySet(Tcl_ThreadDataKey *keyPtr,
          void *data);
MODULE_SCOPE void  TclpThreadExit(int status);
MODULE_SCOPE size_t  TclpThreadGetStackSize(void);
MODULE_SCOPE void  TclRememberCondition(Tcl_Condition *mutex);
MODULE_SCOPE void  TclRememberJoinableThread(Tcl_ThreadId id);
MODULE_SCOPE void  TclRememberMutex(Tcl_Mutex *mutex);
MODULE_SCOPE void  TclRemoveScriptLimitCallbacks(Tcl_Interp *interp);
MODULE_SCOPE int  TclReToGlob(Tcl_Interp *interp, const char *reStr,
          int reStrLen, Tcl_DString *dsPtr, int *flagsPtr);
MODULE_SCOPE void  TclSetBgErrorHandler(Tcl_Interp *interp,
          Tcl_Obj *cmdPrefix);
MODULE_SCOPE void  TclSetBignumIntRep(Tcl_Obj *objPtr,
          mp_int *bignumValue);
MODULE_SCOPE void  TclSetCmdNameObj(Tcl_Interp *interp, Tcl_Obj *objPtr,
          Command *cmdPtr);
MODULE_SCOPE void  TclSetProcessGlobalValue(ProcessGlobalValue *pgvPtr,
          Tcl_Obj *newValue, Tcl_Encoding encoding);
MODULE_SCOPE void  TclSignalExitThread(Tcl_ThreadId id, int result);
MODULE_SCOPE void *  TclStackRealloc(Tcl_Interp *interp, void *ptr,
          int numBytes);
MODULE_SCOPE int  TclStringMatch(const char *str, int strLen,
          const char *pattern, int ptnLen, int flags);
MODULE_SCOPE int  TclStringMatchObj(Tcl_Obj *stringObj,
          Tcl_Obj *patternObj, int flags);
MODULE_SCOPE Tcl_Obj *  TclStringObjReverse(Tcl_Obj *objPtr);
MODULE_SCOPE int  TclSubstTokens(Tcl_Interp *interp, Tcl_Token *tokenPtr,
          int count, int *tokensLeftPtr, int line);
MODULE_SCOPE void  TclTransferResult(Tcl_Interp *sourceInterp, int result,
          Tcl_Interp *targetInterp);
MODULE_SCOPE Tcl_Obj *  TclpNativeToNormalized(ClientData clientData);
MODULE_SCOPE Tcl_Obj *  TclpFilesystemPathType(Tcl_Obj *pathPtr);
MODULE_SCOPE Tcl_PackageInitProc *TclpFindSymbol(Tcl_Interp *interp,
          Tcl_LoadHandle loadHandle, const char *symbol);
MODULE_SCOPE int  TclpDlopen(Tcl_Interp *interp, Tcl_Obj *pathPtr,
          Tcl_LoadHandle *loadHandle,
          Tcl_FSUnloadFileProc **unloadProcPtr);
MODULE_SCOPE int  TclpUtime(Tcl_Obj *pathPtr, struct utimbuf *tval);
#ifdef TCL_LOAD_FROM_MEMORY
MODULE_SCOPE void*  TclpLoadMemoryGetBuffer(Tcl_Interp *interp, int size);
MODULE_SCOPE int  TclpLoadMemory(Tcl_Interp *interp, void *buffer,
          int size, int codeSize, Tcl_LoadHandle *loadHandle,
          Tcl_FSUnloadFileProc **unloadProcPtr);
#endif
MODULE_SCOPE void  TclInitThreadStorage(void);
MODULE_SCOPE void  TclpFinalizeThreadDataThread(void);
MODULE_SCOPE void  TclFinalizeThreadStorage(void);
#ifdef TCL_WIDE_CLICKS
MODULE_SCOPE Tcl_WideInt TclpGetWideClicks(void);
MODULE_SCOPE double  TclpWideClicksToNanoseconds(Tcl_WideInt clicks);
#endif
MODULE_SCOPE Tcl_Obj *  TclDisassembleByteCodeObj(Tcl_Obj *objPtr);

/*
 *----------------------------------------------------------------
 * Command procedures in the generic core:
 *----------------------------------------------------------------
 */

MODULE_SCOPE int  Tcl_AfterObjCmd(ClientData clientData,
          Tcl_Interp *interp, int objc,
          Tcl_Obj *const objv[]);
MODULE_SCOPE int  Tcl_AppendObjCmd(ClientData clientData,
          Tcl_Interp *interp, int objc,
          Tcl_Obj *const objv[]);
MODULE_SCOPE int  Tcl_ApplyObjCmd(ClientData clientData,
          Tcl_Interp *interp, int objc,
          Tcl_Obj *const objv[]);
MODULE_SCOPE int  Tcl_ArrayObjCmd(ClientData clientData,
          Tcl_Interp *interp, int objc,
          Tcl_Obj *const objv[]);
MODULE_SCOPE int  Tcl_BinaryObjCmd(ClientData clientData,
          Tcl_Interp *interp, int objc,
          Tcl_Obj *const objv[]);
MODULE_SCOPE int  Tcl_BreakObjCmd(ClientData clientData,
          Tcl_Interp *interp, int objc,
          Tcl_Obj *const objv[]);
MODULE_SCOPE int  Tcl_CaseObjCmd(ClientData clientData,
          Tcl_Interp *interp, int objc,
          Tcl_Obj *const objv[]);
MODULE_SCOPE int  Tcl_CatchObjCmd(ClientData clientData,
          Tcl_Interp *interp, int objc,
          Tcl_Obj *const objv[]);
MODULE_SCOPE int  Tcl_CdObjCmd(ClientData clientData,
          Tcl_Interp *interp, int objc,
          Tcl_Obj *const objv[]);
MODULE_SCOPE Tcl_Command TclInitChanCmd(Tcl_Interp *interp);
MODULE_SCOPE int  TclChanCreateObjCmd(ClientData clientData,
          Tcl_Interp *interp, int objc,
          Tcl_Obj *const objv[]);
MODULE_SCOPE int  TclChanPostEventObjCmd(ClientData clientData,
          Tcl_Interp *interp, int objc,
          Tcl_Obj *const objv[]);
MODULE_SCOPE void  TclClockInit(Tcl_Interp *interp);
MODULE_SCOPE int  TclClockOldscanObjCmd(
          ClientData clientData, Tcl_Interp *interp,
          int objc, Tcl_Obj *const objv[]);
MODULE_SCOPE int  Tcl_CloseObjCmd(ClientData clientData,
          Tcl_Interp *interp, int objc,
          Tcl_Obj *const objv[]);
MODULE_SCOPE int  Tcl_ConcatObjCmd(ClientData clientData,
          Tcl_Interp *interp, int objc,
          Tcl_Obj *const objv[]);
MODULE_SCOPE int  Tcl_ContinueObjCmd(ClientData clientData,
          Tcl_Interp *interp, int objc,
          Tcl_Obj *const objv[]);
MODULE_SCOPE Tcl_TimerToken TclCreateAbsoluteTimerHandler(
          Tcl_Time *timePtr, Tcl_TimerProc *proc,
          ClientData clientData);
MODULE_SCOPE int  TclDefaultBgErrorHandlerObjCmd(
          ClientData clientData, Tcl_Interp *interp,
          int objc, Tcl_Obj *const objv[]);
MODULE_SCOPE Tcl_Command TclInitDictCmd(Tcl_Interp *interp);
MODULE_SCOPE int  Tcl_DisassembleObjCmd(ClientData clientData,
          Tcl_Interp *interp, int objc,
          Tcl_Obj *const objv[]);
MODULE_SCOPE int  Tcl_EncodingObjCmd(ClientData clientData,
          Tcl_Interp *interp, int objc,
          Tcl_Obj *const objv[]);
MODULE_SCOPE int  Tcl_EofObjCmd(ClientData clientData,
          Tcl_Interp *interp, int objc,
          Tcl_Obj *const objv[]);
MODULE_SCOPE int  Tcl_ErrorObjCmd(ClientData clientData,
          Tcl_Interp *interp, int objc,
          Tcl_Obj *const objv[]);
MODULE_SCOPE int  Tcl_EvalObjCmd(ClientData clientData,
          Tcl_Interp *interp, int objc,
          Tcl_Obj *const objv[]);
MODULE_SCOPE int  Tcl_ExecObjCmd(ClientData clientData,
          Tcl_Interp *interp, int objc,
          Tcl_Obj *const objv[]);
MODULE_SCOPE int  Tcl_ExitObjCmd(ClientData clientData,
          Tcl_Interp *interp, int objc,
          Tcl_Obj *const objv[]);
MODULE_SCOPE int  Tcl_ExprObjCmd(ClientData clientData,
          Tcl_Interp *interp, int objc,
          Tcl_Obj *const objv[]);
MODULE_SCOPE int  Tcl_FblockedObjCmd(ClientData clientData,
          Tcl_Interp *interp, int objc,
          Tcl_Obj *const objv[]);
MODULE_SCOPE int  Tcl_FconfigureObjCmd(
          ClientData clientData, Tcl_Interp *interp,
          int objc, Tcl_Obj *const objv[]);
MODULE_SCOPE int  Tcl_FcopyObjCmd(ClientData dummy,
          Tcl_Interp *interp, int objc,
          Tcl_Obj *const objv[]);
MODULE_SCOPE int  Tcl_FileObjCmd(ClientData dummy,
          Tcl_Interp *interp, int objc,
          Tcl_Obj *const objv[]);
MODULE_SCOPE int  Tcl_FileEventObjCmd(ClientData clientData,
          Tcl_Interp *interp, int objc,
          Tcl_Obj *const objv[]);
MODULE_SCOPE int  Tcl_FlushObjCmd(ClientData clientData,
          Tcl_Interp *interp, int objc,
          Tcl_Obj *const objv[]);
MODULE_SCOPE int  Tcl_ForObjCmd(ClientData clientData,
          Tcl_Interp *interp, int objc,
          Tcl_Obj *const objv[]);
MODULE_SCOPE int  Tcl_ForeachObjCmd(ClientData clientData,
          Tcl_Interp *interp, int objc,
          Tcl_Obj *const objv[]);
MODULE_SCOPE int  Tcl_FormatObjCmd(ClientData dummy,
          Tcl_Interp *interp, int objc,
          Tcl_Obj *const objv[]);
MODULE_SCOPE int  Tcl_GetsObjCmd(ClientData clientData,
          Tcl_Interp *interp, int objc,
          Tcl_Obj *const objv[]);
MODULE_SCOPE int  Tcl_GlobalObjCmd(ClientData clientData,
          Tcl_Interp *interp, int objc,
          Tcl_Obj *const objv[]);
MODULE_SCOPE int  Tcl_GlobObjCmd(ClientData clientData,
          Tcl_Interp *interp, int objc,
          Tcl_Obj *const objv[]);
MODULE_SCOPE int  Tcl_IfObjCmd(ClientData clientData,
          Tcl_Interp *interp, int objc,
          Tcl_Obj *const objv[]);
MODULE_SCOPE int  Tcl_IncrObjCmd(ClientData clientData,
          Tcl_Interp *interp, int objc,
          Tcl_Obj *const objv[]);
MODULE_SCOPE Tcl_Command TclInitInfoCmd(Tcl_Interp *interp);
MODULE_SCOPE int  Tcl_InterpObjCmd(ClientData clientData,
          Tcl_Interp *interp, int argc,
          Tcl_Obj *const objv[]);
MODULE_SCOPE int  Tcl_JoinObjCmd(ClientData clientData,
          Tcl_Interp *interp, int objc,
          Tcl_Obj *const objv[]);
MODULE_SCOPE int  Tcl_LappendObjCmd(ClientData clientData,
          Tcl_Interp *interp, int objc,
          Tcl_Obj *const objv[]);
MODULE_SCOPE int  Tcl_LassignObjCmd(ClientData clientData,
          Tcl_Interp *interp, int objc,
          Tcl_Obj *const objv[]);
MODULE_SCOPE int  Tcl_LindexObjCmd(ClientData clientData,
          Tcl_Interp *interp, int objc,
          Tcl_Obj *const objv[]);
MODULE_SCOPE int  Tcl_LinsertObjCmd(ClientData clientData,
          Tcl_Interp *interp, int objc,
          Tcl_Obj *const objv[]);
MODULE_SCOPE int  Tcl_LlengthObjCmd(ClientData clientData,
          Tcl_Interp *interp, int objc,
          Tcl_Obj *const objv[]);
MODULE_SCOPE int  Tcl_ListObjCmd(ClientData clientData,
          Tcl_Interp *interp, int objc,
          Tcl_Obj *const objv[]);
MODULE_SCOPE int  Tcl_LoadObjCmd(ClientData clientData,
          Tcl_Interp *interp, int objc,
          Tcl_Obj *const objv[]);
MODULE_SCOPE int  Tcl_LrangeObjCmd(ClientData clientData,
          Tcl_Interp *interp, int objc,
          Tcl_Obj *const objv[]);
MODULE_SCOPE int  Tcl_LrepeatObjCmd(ClientData clientData,
          Tcl_Interp *interp, int objc,
          Tcl_Obj *const objv[]);
MODULE_SCOPE int  Tcl_LreplaceObjCmd(ClientData clientData,
          Tcl_Interp *interp, int objc,
          Tcl_Obj *const objv[]);
MODULE_SCOPE int  Tcl_LreverseObjCmd(ClientData clientData,
          Tcl_Interp *interp, int objc,
          Tcl_Obj *const objv[]);
MODULE_SCOPE int  Tcl_LsearchObjCmd(ClientData clientData,
          Tcl_Interp *interp, int objc,
          Tcl_Obj *const objv[]);
MODULE_SCOPE int  Tcl_LsetObjCmd(ClientData clientData,
          Tcl_Interp *interp, int objc,
          Tcl_Obj *const objv[]);
MODULE_SCOPE int  Tcl_LsortObjCmd(ClientData clientData,
          Tcl_Interp *interp, int objc,
          Tcl_Obj *const objv[]);
MODULE_SCOPE int  Tcl_NamespaceObjCmd(ClientData clientData,
          Tcl_Interp *interp, int objc,
          Tcl_Obj *const objv[]);
MODULE_SCOPE int  Tcl_OpenObjCmd(ClientData clientData,
          Tcl_Interp *interp, int objc,
          Tcl_Obj *const objv[]);
MODULE_SCOPE int  Tcl_PackageObjCmd(ClientData clientData,
          Tcl_Interp *interp, int objc,
          Tcl_Obj *const objv[]);
MODULE_SCOPE int  Tcl_PidObjCmd(ClientData clientData,
          Tcl_Interp *interp, int objc,
          Tcl_Obj *const objv[]);
MODULE_SCOPE int  Tcl_PutsObjCmd(ClientData clientData,
          Tcl_Interp *interp, int objc,
          Tcl_Obj *const objv[]);
MODULE_SCOPE int  Tcl_PwdObjCmd(ClientData clientData,
          Tcl_Interp *interp, int objc,
          Tcl_Obj *const objv[]);
MODULE_SCOPE int  Tcl_ReadObjCmd(ClientData clientData,
          Tcl_Interp *interp, int objc,
          Tcl_Obj *const objv[]);
MODULE_SCOPE int  Tcl_RegexpObjCmd(ClientData clientData,
          Tcl_Interp *interp, int objc,
          Tcl_Obj *const objv[]);
MODULE_SCOPE int  Tcl_RegsubObjCmd(ClientData clientData,
          Tcl_Interp *interp, int objc,
          Tcl_Obj *const objv[]);
MODULE_SCOPE int  Tcl_RenameObjCmd(ClientData clientData,
          Tcl_Interp *interp, int objc,
          Tcl_Obj *const objv[]);
MODULE_SCOPE int  Tcl_ReturnObjCmd(ClientData clientData,
          Tcl_Interp *interp, int objc,
          Tcl_Obj *const objv[]);
MODULE_SCOPE int  Tcl_ScanObjCmd(ClientData clientData,
          Tcl_Interp *interp, int objc,
          Tcl_Obj *const objv[]);
MODULE_SCOPE int  Tcl_SeekObjCmd(ClientData clientData,
          Tcl_Interp *interp, int objc,
          Tcl_Obj *const objv[]);
MODULE_SCOPE int  Tcl_SetObjCmd(ClientData clientData,
          Tcl_Interp *interp, int objc,
          Tcl_Obj *const objv[]);
MODULE_SCOPE int  Tcl_SplitObjCmd(ClientData clientData,
          Tcl_Interp *interp, int objc,
          Tcl_Obj *const objv[]);
MODULE_SCOPE int  Tcl_SocketObjCmd(ClientData clientData,
          Tcl_Interp *interp, int objc,
          Tcl_Obj *const objv[]);
MODULE_SCOPE int  Tcl_SourceObjCmd(ClientData clientData,
          Tcl_Interp *interp, int objc,
          Tcl_Obj *const objv[]);
MODULE_SCOPE Tcl_Command TclInitStringCmd(Tcl_Interp *interp);
MODULE_SCOPE int  Tcl_SubstObjCmd(ClientData clientData,
          Tcl_Interp *interp, int objc,
          Tcl_Obj *const objv[]);
MODULE_SCOPE int  Tcl_SwitchObjCmd(ClientData clientData,
          Tcl_Interp *interp, int objc,
          Tcl_Obj *const objv[]);
MODULE_SCOPE int  Tcl_TellObjCmd(ClientData clientData,
          Tcl_Interp *interp, int objc,
          Tcl_Obj *const objv[]);
MODULE_SCOPE int  Tcl_TimeObjCmd(ClientData clientData,
          Tcl_Interp *interp, int objc,
          Tcl_Obj *const objv[]);
MODULE_SCOPE int  Tcl_TraceObjCmd(ClientData clientData,
          Tcl_Interp *interp, int objc,
          Tcl_Obj *const objv[]);
MODULE_SCOPE int  Tcl_UnloadObjCmd(ClientData clientData,
          Tcl_Interp *interp, int objc,
          Tcl_Obj *const objv[]);
MODULE_SCOPE int  Tcl_UnsetObjCmd(ClientData clientData,
          Tcl_Interp *interp, int objc,
          Tcl_Obj *const objv[]);
MODULE_SCOPE int  Tcl_UpdateObjCmd(ClientData clientData,
          Tcl_Interp *interp, int objc,
          Tcl_Obj *const objv[]);
MODULE_SCOPE int  Tcl_UplevelObjCmd(ClientData clientData,
          Tcl_Interp *interp, int objc,
          Tcl_Obj *const objv[]);
MODULE_SCOPE int  Tcl_UpvarObjCmd(ClientData clientData,
          Tcl_Interp *interp, int objc,
          Tcl_Obj *const objv[]);
MODULE_SCOPE int  Tcl_VariableObjCmd(ClientData clientData,
          Tcl_Interp *interp, int objc,
          Tcl_Obj *const objv[]);
MODULE_SCOPE int  Tcl_VwaitObjCmd(ClientData clientData,
          Tcl_Interp *interp, int objc,
          Tcl_Obj *const objv[]);
MODULE_SCOPE int  Tcl_WhileObjCmd(ClientData clientData,
          Tcl_Interp *interp, int objc,
          Tcl_Obj *const objv[]);

/*
 *----------------------------------------------------------------
 * Compilation procedures for commands in the generic core:
 *----------------------------------------------------------------
 */

MODULE_SCOPE int  TclCompileAppendCmd(Tcl_Interp *interp,
          Tcl_Parse *parsePtr, Command *cmdPtr,
          struct CompileEnv *envPtr);
MODULE_SCOPE int  TclCompileBreakCmd(Tcl_Interp *interp,
          Tcl_Parse *parsePtr, Command *cmdPtr,
          struct CompileEnv *envPtr);
MODULE_SCOPE int  TclCompileCatchCmd(Tcl_Interp *interp,
          Tcl_Parse *parsePtr, Command *cmdPtr,
          struct CompileEnv *envPtr);
MODULE_SCOPE int  TclCompileContinueCmd(Tcl_Interp *interp,
          Tcl_Parse *parsePtr, Command *cmdPtr,
          struct CompileEnv *envPtr);
MODULE_SCOPE int  TclCompileDictAppendCmd(Tcl_Interp *interp,
          Tcl_Parse *parsePtr, Command *cmdPtr,
          struct CompileEnv *envPtr);
MODULE_SCOPE int  TclCompileDictForCmd(Tcl_Interp *interp,
          Tcl_Parse *parsePtr, Command *cmdPtr,
          struct CompileEnv *envPtr);
MODULE_SCOPE int  TclCompileDictGetCmd(Tcl_Interp *interp,
          Tcl_Parse *parsePtr, Command *cmdPtr,
          struct CompileEnv *envPtr);
MODULE_SCOPE int  TclCompileDictIncrCmd(Tcl_Interp *interp,
          Tcl_Parse *parsePtr, Command *cmdPtr,
          struct CompileEnv *envPtr);
MODULE_SCOPE int  TclCompileDictLappendCmd(Tcl_Interp *interp,
          Tcl_Parse *parsePtr, Command *cmdPtr,
          struct CompileEnv *envPtr);
MODULE_SCOPE int  TclCompileDictSetCmd(Tcl_Interp *interp,
          Tcl_Parse *parsePtr, Command *cmdPtr,
          struct CompileEnv *envPtr);
MODULE_SCOPE int  TclCompileDictUpdateCmd(Tcl_Interp *interp,
          Tcl_Parse *parsePtr, Command *cmdPtr,
          struct CompileEnv *envPtr);
MODULE_SCOPE int  TclCompileEnsemble(Tcl_Interp *interp,
          Tcl_Parse *parsePtr, Command *cmdPtr,
          struct CompileEnv *envPtr);
MODULE_SCOPE int  TclCompileExprCmd(Tcl_Interp *interp,
          Tcl_Parse *parsePtr, Command *cmdPtr,
          struct CompileEnv *envPtr);
MODULE_SCOPE int  TclCompileForCmd(Tcl_Interp *interp,
          Tcl_Parse *parsePtr, Command *cmdPtr,
          struct CompileEnv *envPtr);
MODULE_SCOPE int  TclCompileForeachCmd(Tcl_Interp *interp,
          Tcl_Parse *parsePtr, Command *cmdPtr,
          struct CompileEnv *envPtr);
MODULE_SCOPE int  TclCompileGlobalCmd(Tcl_Interp *interp,
          Tcl_Parse *parsePtr, Command *cmdPtr,
          struct CompileEnv *envPtr);
MODULE_SCOPE int  TclCompileIfCmd(Tcl_Interp *interp,
          Tcl_Parse *parsePtr, Command *cmdPtr,
          struct CompileEnv *envPtr);
MODULE_SCOPE int  TclCompileInfoExistsCmd(Tcl_Interp *interp,
          Tcl_Parse *parsePtr, Command *cmdPtr,
          struct CompileEnv *envPtr);
MODULE_SCOPE int  TclCompileIncrCmd(Tcl_Interp *interp,
          Tcl_Parse *parsePtr, Command *cmdPtr,
          struct CompileEnv *envPtr);
MODULE_SCOPE int  TclCompileLappendCmd(Tcl_Interp *interp,
          Tcl_Parse *parsePtr, Command *cmdPtr,
          struct CompileEnv *envPtr);
MODULE_SCOPE int  TclCompileLassignCmd(Tcl_Interp *interp,
          Tcl_Parse *parsePtr, Command *cmdPtr,
          struct CompileEnv *envPtr);
MODULE_SCOPE int  TclCompileLindexCmd(Tcl_Interp *interp,
          Tcl_Parse *parsePtr, Command *cmdPtr,
          struct CompileEnv *envPtr);
MODULE_SCOPE int  TclCompileListCmd(Tcl_Interp *interp,
          Tcl_Parse *parsePtr, Command *cmdPtr,
          struct CompileEnv *envPtr);
MODULE_SCOPE int  TclCompileLlengthCmd(Tcl_Interp *interp,
          Tcl_Parse *parsePtr, Command *cmdPtr,
          struct CompileEnv *envPtr);
MODULE_SCOPE int  TclCompileLsetCmd(Tcl_Interp *interp,
          Tcl_Parse *parsePtr, Command *cmdPtr,
          struct CompileEnv *envPtr);
MODULE_SCOPE int  TclCompileNamespaceCmd(Tcl_Interp *interp,
                      Tcl_Parse *parsePtr, Command *cmdPtr,
          struct CompileEnv *envPtr);
MODULE_SCOPE int  TclCompileNoOp(Tcl_Interp *interp,
                      Tcl_Parse *parsePtr, Command *cmdPtr,
          struct CompileEnv *envPtr);
MODULE_SCOPE int  TclCompileRegexpCmd(Tcl_Interp *interp,
          Tcl_Parse *parsePtr, Command *cmdPtr,
          struct CompileEnv *envPtr);
MODULE_SCOPE int  TclCompileReturnCmd(Tcl_Interp *interp,
          Tcl_Parse *parsePtr, Command *cmdPtr,
          struct CompileEnv *envPtr);
MODULE_SCOPE int  TclCompileSetCmd(Tcl_Interp *interp,
          Tcl_Parse *parsePtr, Command *cmdPtr,
          struct CompileEnv *envPtr);
MODULE_SCOPE int  TclCompileStringCmpCmd(Tcl_Interp *interp,
          Tcl_Parse *parsePtr, Command *cmdPtr,
          struct CompileEnv *envPtr);
MODULE_SCOPE int  TclCompileStringEqualCmd(Tcl_Interp *interp,
          Tcl_Parse *parsePtr, Command *cmdPtr,
          struct CompileEnv *envPtr);
MODULE_SCOPE int  TclCompileStringIndexCmd(Tcl_Interp *interp,
          Tcl_Parse *parsePtr, Command *cmdPtr,
          struct CompileEnv *envPtr);
MODULE_SCOPE int  TclCompileStringLenCmd(Tcl_Interp *interp,
          Tcl_Parse *parsePtr, Command *cmdPtr,
          struct CompileEnv *envPtr);
MODULE_SCOPE int  TclCompileStringMatchCmd(Tcl_Interp *interp,
          Tcl_Parse *parsePtr, Command *cmdPtr,
          struct CompileEnv *envPtr);
MODULE_SCOPE int  TclCompileSwitchCmd(Tcl_Interp *interp,
          Tcl_Parse *parsePtr, Command *cmdPtr,
          struct CompileEnv *envPtr);
MODULE_SCOPE int  TclCompileUpvarCmd(Tcl_Interp *interp,
                      Tcl_Parse *parsePtr, Command *cmdPtr,
          struct CompileEnv *envPtr);
MODULE_SCOPE int  TclCompileVariableCmd(Tcl_Interp *interp,
                      Tcl_Parse *parsePtr, Command *cmdPtr,
          struct CompileEnv *envPtr);
MODULE_SCOPE int  TclCompileWhileCmd(Tcl_Interp *interp,
          Tcl_Parse *parsePtr, Command *cmdPtr,
          struct CompileEnv *envPtr);

MODULE_SCOPE int  TclInvertOpCmd(ClientData clientData,
          Tcl_Interp *interp, int objc,
          Tcl_Obj *const objv[]);
MODULE_SCOPE int  TclCompileInvertOpCmd(Tcl_Interp *interp,
          Tcl_Parse *parsePtr, Command *cmdPtr,
          struct CompileEnv *envPtr);
MODULE_SCOPE int  TclNotOpCmd(ClientData clientData,
          Tcl_Interp *interp, int objc,
          Tcl_Obj *const objv[]);
MODULE_SCOPE int  TclCompileNotOpCmd(Tcl_Interp *interp,
          Tcl_Parse *parsePtr, Command *cmdPtr,
          struct CompileEnv *envPtr);
MODULE_SCOPE int  TclAddOpCmd(ClientData clientData,
          Tcl_Interp *interp, int objc,
          Tcl_Obj *const objv[]);
MODULE_SCOPE int  TclCompileAddOpCmd(Tcl_Interp *interp,
          Tcl_Parse *parsePtr, Command *cmdPtr,
          struct CompileEnv *envPtr);
MODULE_SCOPE int  TclMulOpCmd(ClientData clientData,
          Tcl_Interp *interp, int objc,
          Tcl_Obj *const objv[]);
MODULE_SCOPE int  TclCompileMulOpCmd(Tcl_Interp *interp,
          Tcl_Parse *parsePtr, Command *cmdPtr,
          struct CompileEnv *envPtr);
MODULE_SCOPE int  TclAndOpCmd(ClientData clientData,
          Tcl_Interp *interp, int objc,
          Tcl_Obj *const objv[]);
MODULE_SCOPE int  TclCompileAndOpCmd(Tcl_Interp *interp,
          Tcl_Parse *parsePtr, Command *cmdPtr,
          struct CompileEnv *envPtr);
MODULE_SCOPE int  TclOrOpCmd(ClientData clientData,
          Tcl_Interp *interp, int objc,
          Tcl_Obj *const objv[]);
MODULE_SCOPE int  TclCompileOrOpCmd(Tcl_Interp *interp,
          Tcl_Parse *parsePtr, Command *cmdPtr,
          struct CompileEnv *envPtr);
MODULE_SCOPE int  TclXorOpCmd(ClientData clientData,
          Tcl_Interp *interp, int objc,
          Tcl_Obj *const objv[]);
MODULE_SCOPE int  TclCompileXorOpCmd(Tcl_Interp *interp,
          Tcl_Parse *parsePtr, Command *cmdPtr,
          struct CompileEnv *envPtr);
MODULE_SCOPE int  TclPowOpCmd(ClientData clientData,
          Tcl_Interp *interp, int objc,
          Tcl_Obj *const objv[]);
MODULE_SCOPE int  TclCompilePowOpCmd(Tcl_Interp *interp,
          Tcl_Parse *parsePtr, Command *cmdPtr,
          struct CompileEnv *envPtr);
MODULE_SCOPE int  TclLshiftOpCmd(ClientData clientData,
          Tcl_Interp *interp, int objc,
          Tcl_Obj *const objv[]);
MODULE_SCOPE int  TclCompileLshiftOpCmd(Tcl_Interp *interp,
          Tcl_Parse *parsePtr, Command *cmdPtr,
          struct CompileEnv *envPtr);
MODULE_SCOPE int  TclRshiftOpCmd(ClientData clientData,
          Tcl_Interp *interp, int objc,
          Tcl_Obj *const objv[]);
MODULE_SCOPE int  TclCompileRshiftOpCmd(Tcl_Interp *interp,
          Tcl_Parse *parsePtr, Command *cmdPtr,
          struct CompileEnv *envPtr);
MODULE_SCOPE int  TclModOpCmd(ClientData clientData,
          Tcl_Interp *interp, int objc,
          Tcl_Obj *const objv[]);
MODULE_SCOPE int  TclCompileModOpCmd(Tcl_Interp *interp,
          Tcl_Parse *parsePtr, Command *cmdPtr,
          struct CompileEnv *envPtr);
MODULE_SCOPE int  TclNeqOpCmd(ClientData clientData,
          Tcl_Interp *interp, int objc,
          Tcl_Obj *const objv[]);
MODULE_SCOPE int  TclCompileNeqOpCmd(Tcl_Interp *interp,
          Tcl_Parse *parsePtr, Command *cmdPtr,
          struct CompileEnv *envPtr);
MODULE_SCOPE int  TclStrneqOpCmd(ClientData clientData,
          Tcl_Interp *interp, int objc,
          Tcl_Obj *const objv[]);
MODULE_SCOPE int  TclCompileStrneqOpCmd(Tcl_Interp *interp,
          Tcl_Parse *parsePtr, Command *cmdPtr,
          struct CompileEnv *envPtr);
MODULE_SCOPE int  TclInOpCmd(ClientData clientData,
          Tcl_Interp *interp, int objc,
          Tcl_Obj *const objv[]);
MODULE_SCOPE int  TclCompileInOpCmd(Tcl_Interp *interp,
          Tcl_Parse *parsePtr, Command *cmdPtr,
          struct CompileEnv *envPtr);
MODULE_SCOPE int  TclNiOpCmd(ClientData clientData,
          Tcl_Interp *interp, int objc,
          Tcl_Obj *const objv[]);
MODULE_SCOPE int  TclCompileNiOpCmd(Tcl_Interp *interp,
          Tcl_Parse *parsePtr, Command *cmdPtr,
          struct CompileEnv *envPtr);
MODULE_SCOPE int  TclMinusOpCmd(ClientData clientData,
          Tcl_Interp *interp, int objc,
          Tcl_Obj *const objv[]);
MODULE_SCOPE int  TclCompileMinusOpCmd(Tcl_Interp *interp,
          Tcl_Parse *parsePtr, Command *cmdPtr,
          struct CompileEnv *envPtr);
MODULE_SCOPE int  TclDivOpCmd(ClientData clientData,
          Tcl_Interp *interp, int objc,
          Tcl_Obj *const objv[]);
MODULE_SCOPE int  TclCompileDivOpCmd(Tcl_Interp *interp,
          Tcl_Parse *parsePtr, Command *cmdPtr,
          struct CompileEnv *envPtr);
MODULE_SCOPE int  TclLessOpCmd(ClientData clientData,
          Tcl_Interp *interp, int objc,
          Tcl_Obj *const objv[]);
MODULE_SCOPE int  TclCompileLessOpCmd(Tcl_Interp *interp,
          Tcl_Parse *parsePtr, Command *cmdPtr,
          struct CompileEnv *envPtr);
MODULE_SCOPE int  TclLeqOpCmd(ClientData clientData,
          Tcl_Interp *interp, int objc,
          Tcl_Obj *const objv[]);
MODULE_SCOPE int  TclCompileLeqOpCmd(Tcl_Interp *interp,
          Tcl_Parse *parsePtr, Command *cmdPtr,
          struct CompileEnv *envPtr);
MODULE_SCOPE int  TclGreaterOpCmd(ClientData clientData,
          Tcl_Interp *interp, int objc,
          Tcl_Obj *const objv[]);
MODULE_SCOPE int  TclCompileGreaterOpCmd(Tcl_Interp *interp,
          Tcl_Parse *parsePtr, Command *cmdPtr,
          struct CompileEnv *envPtr);
MODULE_SCOPE int  TclGeqOpCmd(ClientData clientData,
          Tcl_Interp *interp, int objc,
          Tcl_Obj *const objv[]);
MODULE_SCOPE int  TclCompileGeqOpCmd(Tcl_Interp *interp,
          Tcl_Parse *parsePtr, Command *cmdPtr,
          struct CompileEnv *envPtr);
MODULE_SCOPE int  TclEqOpCmd(ClientData clientData,
          Tcl_Interp *interp, int objc,
          Tcl_Obj *const objv[]);
MODULE_SCOPE int  TclCompileEqOpCmd(Tcl_Interp *interp,
          Tcl_Parse *parsePtr, Command *cmdPtr,
          struct CompileEnv *envPtr);
MODULE_SCOPE int  TclStreqOpCmd(ClientData clientData,
          Tcl_Interp *interp, int objc,
          Tcl_Obj *const objv[]);
MODULE_SCOPE int  TclCompileStreqOpCmd(Tcl_Interp *interp,
          Tcl_Parse *parsePtr, Command *cmdPtr,
          struct CompileEnv *envPtr);

/*
 * Functions defined in generic/tclVar.c and currenttly exported only for use
 * by the bytecode compiler and engine. Some of these could later be placed in
 * the public interface.
 */

MODULE_SCOPE Var *  TclObjLookupVarEx(Tcl_Interp * interp,
          Tcl_Obj *part1Ptr, Tcl_Obj *part2Ptr, int flags,
          const char * msg, const int createPart1,
          const int createPart2, Var **arrayPtrPtr);
MODULE_SCOPE Var *  TclLookupArrayElement(Tcl_Interp *interp,
          Tcl_Obj *arrayNamePtr, Tcl_Obj *elNamePtr,
          const int flags, const char *msg,
          const int createPart1, const int createPart2,
          Var *arrayPtr, int index);
MODULE_SCOPE Tcl_Obj *  TclPtrGetVar(Tcl_Interp *interp,
          Var *varPtr, Var *arrayPtr, Tcl_Obj *part1Ptr,
          Tcl_Obj *part2Ptr, const int flags, int index);
MODULE_SCOPE Tcl_Obj *  TclPtrSetVar(Tcl_Interp *interp,
          Var *varPtr, Var *arrayPtr, Tcl_Obj *part1Ptr,
          Tcl_Obj *part2Ptr, Tcl_Obj *newValuePtr,
          const int flags, int index);
MODULE_SCOPE Tcl_Obj *  TclPtrIncrObjVar(Tcl_Interp *interp,
          Var *varPtr, Var *arrayPtr, Tcl_Obj *part1Ptr,
          Tcl_Obj *part2Ptr, Tcl_Obj *incrPtr,
          const int flags, int index);
MODULE_SCOPE int        TclPtrObjMakeUpvar(Tcl_Interp *interp, Var *otherPtr,
                      Tcl_Obj *myNamePtr, int myFlags, int index);
MODULE_SCOPE void  TclInvalidateNsPath(Namespace *nsPtr);

/*
 * The new extended interface to the variable traces.
 */

MODULE_SCOPE int  TclObjCallVarTraces(Interp *iPtr, Var *arrayPtr,
          Var *varPtr, Tcl_Obj *part1Ptr, Tcl_Obj *part2Ptr,
          int flags, int leaveErrMsg, int index);

/*
 * So tclObj.c and tclDictObj.c can share these implementations.
 */

MODULE_SCOPE int  TclCompareObjKeys(void *keyPtr, Tcl_HashEntry *hPtr);
MODULE_SCOPE void  TclFreeObjEntry(Tcl_HashEntry *hPtr);
MODULE_SCOPE unsigned  TclHashObjKey(Tcl_HashTable *tablePtr, void *keyPtr);

/*
 *----------------------------------------------------------------
 * Macros used by the Tcl core to create and release Tcl objects.
 * TclNewObj(objPtr) creates a new object denoting an empty string.
 * TclDecrRefCount(objPtr) decrements the object's reference count, and frees
 * the object if its reference count is zero. These macros are inline versions
 * of Tcl_NewObj() and Tcl_DecrRefCount(). Notice that the names differ in not
 * having a "_" after the "Tcl". Notice also that these macros reference their
 * argument more than once, so you should avoid calling them with an
 * expression that is expensive to compute or has side effects. The ANSI C
 * "prototypes" for these macros are:
 *
 * MODULE_SCOPE void  TclNewObj(Tcl_Obj *objPtr);
 * MODULE_SCOPE void  TclDecrRefCount(Tcl_Obj *objPtr);
 *
 * These macros are defined in terms of two macros that depend on memory
 * allocator in use: TclAllocObjStorage, TclFreeObjStorage. They are defined
 * below.
 *----------------------------------------------------------------
 */

/*
 * DTrace object allocation probe macros.
 */

#ifdef USE_DTRACE
#include "tclDTrace.h"
#define  TCL_DTRACE_OBJ_CREATE(objPtr)  TCL_OBJ_CREATE(objPtr)
#define  TCL_DTRACE_OBJ_FREE(objPtr)  TCL_OBJ_FREE(objPtr)
#else /* USE_DTRACE */
#define  TCL_DTRACE_OBJ_CREATE(objPtr)  {}
#define  TCL_DTRACE_OBJ_FREE(objPtr)  {}
#endif /* USE_DTRACE */

#ifdef TCL_COMPILE_STATS
#  define TclIncrObjsAllocated() \
    tclObjsAlloced++
#  define TclIncrObjsFreed() \
    tclObjsFreed++
#else
#  define TclIncrObjsAllocated()
#  define TclIncrObjsFreed()
#endif /* TCL_COMPILE_STATS */

#ifndef TCL_MEM_DEBUG
# define TclNewObj(objPtr) \
    TclIncrObjsAllocated(); \
    TclAllocObjStorage(objPtr); \
    (objPtr)->refCount = 0; \
    (objPtr)->bytes    = tclEmptyStringRep; \
    (objPtr)->length   = 0; \
    (objPtr)->typePtr  = NULL; \
    TCL_DTRACE_OBJ_CREATE(objPtr)

/*
 * Invalidate the string rep first so we can use the bytes value for our
 * pointer chain, and signal an obj deletion (as opposed to shimmering) with
 * 'length == -1'.
 * Use empty 'if ; else' to handle use in unbraced outer if/else conditions
 */

# define TclDecrRefCount(objPtr) \
    if (--(objPtr)->refCount > 0) ; else { \
  if (!(objPtr)->typePtr || !(objPtr)->typePtr->freeIntRepProc) { \
      TCL_DTRACE_OBJ_FREE(objPtr); \
        if ((objPtr)->bytes \
              && ((objPtr)->bytes != tclEmptyStringRep)) { \
          ckfree((char *) (objPtr)->bytes); \
      } \
            (objPtr)->length = -1; \
      TclFreeObjStorage(objPtr); \
      TclIncrObjsFreed(); \
  } else { \
      TclFreeObj(objPtr); \
  } \
    }

#if defined(PURIFY)

/*
 * The PURIFY mode is like the regular mode, but instead of doing block
 * Tcl_Obj allocation and keeping a freed list for efficiency, it always
 * allocates and frees a single Tcl_Obj so that tools like Purify can better
 * track memory leaks
 */

#  define TclAllocObjStorage(objPtr) \
  (objPtr) = (Tcl_Obj *) Tcl_Alloc(sizeof(Tcl_Obj))

#  define TclFreeObjStorage(objPtr) \
  ckfree((char *) (objPtr))

#undef USE_THREAD_ALLOC
#elif defined(TCL_THREADS) && defined(USE_THREAD_ALLOC)

/*
 * The TCL_THREADS mode is like the regular mode but allocates Tcl_Obj's from
 * per-thread caches.
 */

MODULE_SCOPE Tcl_Obj *  TclThreadAllocObj(void);
MODULE_SCOPE void  TclThreadFreeObj(Tcl_Obj *);
MODULE_SCOPE Tcl_Mutex *TclpNewAllocMutex(void);
MODULE_SCOPE void  TclFreeAllocCache(void *);
MODULE_SCOPE void *  TclpGetAllocCache(void);
MODULE_SCOPE void  TclpSetAllocCache(void *);
MODULE_SCOPE void  TclpFreeAllocMutex(Tcl_Mutex *mutex);
MODULE_SCOPE void  TclpFreeAllocCache(void *);

#  define TclAllocObjStorage(objPtr) \
  (objPtr) = TclThreadAllocObj()

#  define TclFreeObjStorage(objPtr) \
  TclThreadFreeObj((objPtr))

#else /* not PURIFY or USE_THREAD_ALLOC */

#ifdef TCL_THREADS
/* declared in tclObj.c */
MODULE_SCOPE Tcl_Mutex  tclObjMutex;
#endif

#  define TclAllocObjStorage(objPtr) \
  Tcl_MutexLock(&tclObjMutex); \
  if (tclFreeObjList == NULL) { \
      TclAllocateFreeObjects(); \
  } \
  (objPtr) = tclFreeObjList; \
  tclFreeObjList = (Tcl_Obj *) \
    tclFreeObjList->internalRep.otherValuePtr; \
  Tcl_MutexUnlock(&tclObjMutex)

#  define TclFreeObjStorage(objPtr) \
  Tcl_MutexLock(&tclObjMutex); \
  (objPtr)->internalRep.otherValuePtr = (void *) tclFreeObjList; \
  tclFreeObjList = (objPtr); \
  Tcl_MutexUnlock(&tclObjMutex)
#endif

#else /* TCL_MEM_DEBUG */
MODULE_SCOPE void  TclDbInitNewObj(Tcl_Obj *objPtr);

# define TclDbNewObj(objPtr, file, line) \
    TclIncrObjsAllocated(); \
    (objPtr) = (Tcl_Obj *) Tcl_DbCkalloc(sizeof(Tcl_Obj), (file), (line)); \
    TclDbInitNewObj(objPtr); \
    TCL_DTRACE_OBJ_CREATE(objPtr)

# define TclNewObj(objPtr) \
    TclDbNewObj(objPtr, __FILE__, __LINE__);

# define TclDecrRefCount(objPtr) \
    Tcl_DbDecrRefCount(objPtr, __FILE__, __LINE__)

# define TclNewListObjDirect(objc, objv) \
    TclDbNewListObjDirect(objc, objv, __FILE__, __LINE__)

#undef USE_THREAD_ALLOC
#endif /* TCL_MEM_DEBUG */

/*
 *----------------------------------------------------------------
 * Macro used by the Tcl core to set a Tcl_Obj's string representation to a
 * copy of the "len" bytes starting at "bytePtr". This code works even if the
 * byte array contains NULLs as long as the length is correct. Because "len"
 * is referenced multiple times, it should be as simple an expression as
 * possible. The ANSI C "prototype" for this macro is:
 *
 * MODULE_SCOPE void TclInitStringRep(Tcl_Obj *objPtr, char *bytePtr, int len);
 *
 * This macro should only be called on an unshared objPtr where
 *  objPtr->typePtr->freeIntRepProc == NULL
 *----------------------------------------------------------------
 */

#define TclInitStringRep(objPtr, bytePtr, len) \
    if ((len) == 0) { \
  (objPtr)->bytes   = tclEmptyStringRep; \
  (objPtr)->length = 0; \
    } else { \
  (objPtr)->bytes = (char *) ckalloc((unsigned) ((len) + 1)); \
  memcpy((void *) (objPtr)->bytes, (void *) (bytePtr), \
    (unsigned) (len)); \
  (objPtr)->bytes[len] = '\0'; \
  (objPtr)->length = (len); \
    }

/*
 *----------------------------------------------------------------
 * Macro used by the Tcl core to get the string representation's byte array
 * pointer from a Tcl_Obj. This is an inline version of Tcl_GetString(). The
 * macro's expression result is the string rep's byte pointer which might be
 * NULL. The bytes referenced by this pointer must not be modified by the
 * caller. The ANSI C "prototype" for this macro is:
 *
 * MODULE_SCOPE char *  TclGetString(Tcl_Obj *objPtr);
 *----------------------------------------------------------------
 */

#define TclGetString(objPtr) \
    ((objPtr)->bytes? (objPtr)->bytes : Tcl_GetString((objPtr)))


#define TclGetStringFromObj(objPtr, lenPtr) \
    ((objPtr)->bytes \
      ? (*(lenPtr) = (objPtr)->length, (objPtr)->bytes)  \
      : Tcl_GetStringFromObj((objPtr), (lenPtr)))

/*
 *----------------------------------------------------------------
 * Macro used by the Tcl core to clean out an object's internal
 * representation. Does not actually reset the rep's bytes. The ANSI C
 * "prototype" for this macro is:
 *
 * MODULE_SCOPE void  TclFreeIntRep(Tcl_Obj *objPtr);
 *----------------------------------------------------------------
 */

#define TclFreeIntRep(objPtr) \
    if ((objPtr)->typePtr != NULL && \
      (objPtr)->typePtr->freeIntRepProc != NULL) { \
  (objPtr)->typePtr->freeIntRepProc(objPtr); \
    }

/*
 *----------------------------------------------------------------
 * Macro used by the Tcl core to clean out an object's string representation.
 * The ANSI C "prototype" for this macro is:
 *
 * MODULE_SCOPE void  TclInvalidateStringRep(Tcl_Obj *objPtr);
 *----------------------------------------------------------------
 */

#define TclInvalidateStringRep(objPtr) \
    if (objPtr->bytes != NULL) { \
  if (objPtr->bytes != tclEmptyStringRep) {\
      ckfree((char *) objPtr->bytes);\
  }\
  objPtr->bytes = NULL;\
    }\

/*
 *----------------------------------------------------------------
 * Macro used by the Tcl core get a unicode char from a utf string. It checks
 * to see if we have a one-byte utf char before calling the real
 * Tcl_UtfToUniChar, as this will save a lot of time for primarily ascii
 * string handling. The macro's expression result is 1 for the 1-byte case or
 * the result of Tcl_UtfToUniChar. The ANSI C "prototype" for this macro is:
 *
 * MODULE_SCOPE int  TclUtfToUniChar(const char *string, Tcl_UniChar *ch);
 *----------------------------------------------------------------
 */

#define TclUtfToUniChar(str, chPtr) \
  ((((unsigned char) *(str)) < 0xC0) ? \
      ((*(chPtr) = (Tcl_UniChar) *(str)), 1) \
      : Tcl_UtfToUniChar(str, chPtr))

/*
 *----------------------------------------------------------------
 * Macro used by the Tcl core to compare Unicode strings. On big-endian
 * systems we can use the more efficient memcmp, but this would not be
 * lexically correct on little-endian systems. The ANSI C "prototype" for
 * this macro is:
 *
 * MODULE_SCOPE int  TclUniCharNcmp(const Tcl_UniChar *cs,
 *          const Tcl_UniChar *ct, unsigned long n);
 *----------------------------------------------------------------
 */

#ifdef WORDS_BIGENDIAN
#   define TclUniCharNcmp(cs,ct,n) memcmp((cs),(ct),(n)*sizeof(Tcl_UniChar))
#else /* !WORDS_BIGENDIAN */
#   define TclUniCharNcmp Tcl_UniCharNcmp
#endif /* WORDS_BIGENDIAN */

/*
 *----------------------------------------------------------------
 * Macro used by the Tcl core to increment a namespace's export export epoch
 * counter. The ANSI C "prototype" for this macro is:
 *
 * MODULE_SCOPE void  TclInvalidateNsCmdLookup(Namespace *nsPtr);
 *----------------------------------------------------------------
 */

#define TclInvalidateNsCmdLookup(nsPtr) \
    if ((nsPtr)->numExportPatterns) { \
  (nsPtr)->exportLookupEpoch++; \
    }

/*
 *----------------------------------------------------------------------
 *
 * Core procedures added to libtommath for bignum manipulation.
 *
 *----------------------------------------------------------------------
 */

MODULE_SCOPE int  TclTommath_Init(Tcl_Interp *interp);
MODULE_SCOPE void  TclBNInitBignumFromLong(mp_int *bignum, long initVal);
MODULE_SCOPE void  TclBNInitBignumFromWideInt(mp_int *bignum,
          Tcl_WideInt initVal);
MODULE_SCOPE void  TclBNInitBignumFromWideUInt(mp_int *bignum,
          Tcl_WideUInt initVal);

/*
 *----------------------------------------------------------------
 * Macro used by the Tcl core to check whether a pattern has any characters
 * special to [string match]. The ANSI C "prototype" for this macro is:
 *
 * MODULE_SCOPE int  TclMatchIsTrivial(const char *pattern);
 *----------------------------------------------------------------
 */

#define TclMatchIsTrivial(pattern)  strpbrk((pattern), "*[?\\") == NULL

/*
 *----------------------------------------------------------------
 * Macro used by the Tcl core to write the string rep of a long integer to a
 * character buffer. The ANSI C "prototype" for this macro is:
 *
 * MODULE_SCOPE int  TclFormatInt(char *buf, long n);
 *----------------------------------------------------------------
 */

#define TclFormatInt(buf, n)    sprintf((buf), "%ld", (long)(n))

/*
 *----------------------------------------------------------------
 * Macros used by the Tcl core to set a Tcl_Obj's numeric representation
 * avoiding the corresponding function calls in time critical parts of the
 * core. They should only be called on unshared objects. The ANSI C
 * "prototypes" for these macros are:
 *
 * MODULE_SCOPE void  TclSetIntObj(Tcl_Obj *objPtr, int intValue);
 * MODULE_SCOPE void  TclSetLongObj(Tcl_Obj *objPtr, long longValue);
 * MODULE_SCOPE void  TclSetBooleanObj(Tcl_Obj *objPtr, long boolValue);
 * MODULE_SCOPE void  TclSetWideIntObj(Tcl_Obj *objPtr, Tcl_WideInt w);
 * MODULE_SCOPE void  TclSetDoubleObj(Tcl_Obj *objPtr, double d);
 *----------------------------------------------------------------
 */

#define TclSetIntObj(objPtr, i) \
    TclInvalidateStringRep(objPtr);\
    TclFreeIntRep(objPtr); \
    (objPtr)->internalRep.longValue = (long)(i); \
    (objPtr)->typePtr = &tclIntType

#define TclSetLongObj(objPtr, l) \
    TclSetIntObj((objPtr), (l))

/*
 * NOTE: There is to be no such thing as a "pure" boolean. Boolean values set
 * programmatically go straight to being "int" Tcl_Obj's, with value 0 or 1.
 * The only "boolean" Tcl_Obj's shall be those holding the cached boolean
 * value of strings like: "yes", "no", "true", "false", "on", "off".
 */

#define TclSetBooleanObj(objPtr, b) \
    TclSetIntObj((objPtr), ((b)? 1 : 0));

#ifndef NO_WIDE_TYPE
#define TclSetWideIntObj(objPtr, w) \
    TclInvalidateStringRep(objPtr);\
    TclFreeIntRep(objPtr); \
    (objPtr)->internalRep.wideValue = (Tcl_WideInt)(w); \
    (objPtr)->typePtr = &tclWideIntType
#endif

#define TclSetDoubleObj(objPtr, d) \
    TclInvalidateStringRep(objPtr);\
    TclFreeIntRep(objPtr); \
    (objPtr)->internalRep.doubleValue = (double)(d); \
    (objPtr)->typePtr = &tclDoubleType

/*
 *----------------------------------------------------------------
 * Macros used by the Tcl core to create and initialise objects of standard
 * types, avoiding the corresponding function calls in time critical parts of
 * the core. The ANSI C "prototypes" for these macros are:
 *
 * MODULE_SCOPE void  TclNewIntObj(Tcl_Obj *objPtr, int i);
 * MODULE_SCOPE void  TclNewLongObj(Tcl_Obj *objPtr, long l);
 * MODULE_SCOPE void  TclNewBooleanObj(Tcl_Obj *objPtr, int b);
 * MODULE_SCOPE void  TclNewWideObj(Tcl_Obj *objPtr, Tcl_WideInt w);
 * MODULE_SCOPE void  TclNewDoubleObj(Tcl_Obj *objPtr, double d);
 * MODULE_SCOPE void  TclNewStringObj(Tcl_Obj *objPtr, char *s, int len);
 * MODULE_SCOPE void  TclNewLiteralStringObj(Tcl_Obj*objPtr, char*sLiteral);
 *
 *----------------------------------------------------------------
 */

#ifndef TCL_MEM_DEBUG
#define TclNewIntObj(objPtr, i) \
    TclIncrObjsAllocated(); \
    TclAllocObjStorage(objPtr); \
    (objPtr)->refCount = 0; \
    (objPtr)->bytes = NULL; \
    (objPtr)->internalRep.longValue = (long)(i); \
    (objPtr)->typePtr = &tclIntType; \
    TCL_DTRACE_OBJ_CREATE(objPtr)

#define TclNewLongObj(objPtr, l) \
    TclNewIntObj((objPtr), (l))

/*
 * NOTE: There is to be no such thing as a "pure" boolean.
 * See comment above TclSetBooleanObj macro above.
 */
#define TclNewBooleanObj(objPtr, b) \
    TclNewIntObj((objPtr), ((b)? 1 : 0))

#define TclNewDoubleObj(objPtr, d) \
    TclIncrObjsAllocated(); \
    TclAllocObjStorage(objPtr); \
    (objPtr)->refCount = 0; \
    (objPtr)->bytes = NULL; \
    (objPtr)->internalRep.doubleValue = (double)(d); \
    (objPtr)->typePtr = &tclDoubleType; \
    TCL_DTRACE_OBJ_CREATE(objPtr)

#define TclNewStringObj(objPtr, s, len) \
    TclIncrObjsAllocated(); \
    TclAllocObjStorage(objPtr); \
    (objPtr)->refCount = 0; \
    TclInitStringRep((objPtr), (s), (len));\
    (objPtr)->typePtr = NULL; \
    TCL_DTRACE_OBJ_CREATE(objPtr)

#else /* TCL_MEM_DEBUG */
#define TclNewIntObj(objPtr, i)   \
    (objPtr) = Tcl_NewIntObj(i)

#define TclNewLongObj(objPtr, l) \
    (objPtr) = Tcl_NewLongObj(l)

#define TclNewBooleanObj(objPtr, b) \
    (objPtr) = Tcl_NewBooleanObj(b)

#define TclNewDoubleObj(objPtr, d) \
    (objPtr) = Tcl_NewDoubleObj(d)

#define TclNewStringObj(objPtr, s, len) \
    (objPtr) = Tcl_NewStringObj((s), (len))
#endif /* TCL_MEM_DEBUG */

/*
 * The sLiteral argument *must* be a string literal; the incantation with
 * sizeof(sLiteral "") will fail to compile otherwise.
 */
#define TclNewLiteralStringObj(objPtr, sLiteral) \
    TclNewStringObj((objPtr), (sLiteral), (int) (sizeof(sLiteral "") - 1))

/*
 *----------------------------------------------------------------
 * Macros used by the Tcl core to test for some special double values.
 * The ANSI C "prototypes" for these macros are:
 *
 * MODULE_SCOPE int  TclIsInfinite(double d);
 * MODULE_SCOPE int  TclIsNaN(double d);
 */

#ifdef _MSC_VER
#define TclIsInfinite(d)  ( ! (_finite((d))) )
#define TclIsNaN(d)    (_isnan((d)))
#else
#define TclIsInfinite(d)  ( (d) > DBL_MAX || (d) < -DBL_MAX )
#define TclIsNaN(d)    ((d) != (d))
#endif

/*
 * ----------------------------------------------------------------------
 * Macro to use to find the offset of a field in a structure.
 * Computes number of bytes from beginning of structure to a given field.
 */

#ifdef offsetof
#define TclOffset(type, field) ((int) offsetof(type, field))
#else
#define TclOffset(type, field) ((int) ((char *) &((type *) 0)->field))
#endif

/*
 *----------------------------------------------------------------
 * Inline version of Tcl_GetCurrentNamespace and Tcl_GetGlobalNamespace
 */

#define TclGetCurrentNamespace(interp) \
    (Tcl_Namespace *) ((Interp *)(interp))->varFramePtr->nsPtr

#define TclGetGlobalNamespace(interp) \
    (Tcl_Namespace *) ((Interp *)(interp))->globalNsPtr

/*
 *----------------------------------------------------------------
 * Inline version of TclCleanupCommand; still need the function as it is in
 * the internal stubs, but the core can use the macro instead.
 */

#define TclCleanupCommandMacro(cmdPtr) \
    if (--(cmdPtr)->refCount <= 0) { \
  ckfree((char *) (cmdPtr));\
    }

/*
 *----------------------------------------------------------------
 * Inline versions of Tcl_LimitReady() and Tcl_LimitExceeded to limit number
 * of calls out of the critical path. Note that this code isn't particularly
 * readable; the non-inline version (in tclInterp.c) is much easier to
 * understand. Note also that these macros takes different args (iPtr->limit)
 * to the non-inline version.
 */

#define TclLimitExceeded(limit) ((limit).exceeded != 0)

#define TclLimitReady(limit)            \
    (((limit).active == 0) ? 0 :          \
    (++(limit).granularityTicker,          \
    ((((limit).active & TCL_LIMIT_COMMANDS) &&        \
      (((limit).cmdGranularity == 1) ||        \
      ((limit).granularityTicker % (limit).cmdGranularity == 0)))  \
      ? 1 :              \
    (((limit).active & TCL_LIMIT_TIME) &&        \
      (((limit).timeGranularity == 1) ||        \
      ((limit).granularityTicker % (limit).timeGranularity == 0)))\
      ? 1 : 0)))


#include "tclPort.h"
#include "tclIntDecls.h"
#include "tclIntPlatDecls.h"
#include "tclTomMathDecls.h"

#endif /* _TCLINT */

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
