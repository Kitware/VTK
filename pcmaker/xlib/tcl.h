/*
 * tcl.h --
 *
 *	This header file describes the externally-visible facilities
 *	of the Tcl interpreter.
 *
 * Copyright (c) 1987-1994 The Regents of the University of California.
 * Copyright (c) 1993-1996 Lucent Technologies.
 * Copyright (c) 1994-1998 Sun Microsystems, Inc.
 * Copyright (c) 1998-1999 by Scriptics Corporation.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#ifndef _TCL
#define _TCL

/*
 * For C++ compilers, use extern "C"
 */

#ifdef __cplusplus
extern "C" {
#endif
    
/*
 * The following defines are used to indicate the various release levels.
 */

#define TCL_ALPHA_RELEASE	0
#define TCL_BETA_RELEASE	1
#define TCL_FINAL_RELEASE	2

/*
 * When version numbers change here, must also go into the following files
 * and update the version numbers:
 *
 * library/init.tcl	(only if Major.minor changes, not patchlevel) 1 LOC
 * unix/configure.in	(2 LOC Major, 2 LOC minor, 1 LOC patch)
 * win/configure.in	(as above)
 * win/makefile.vc	(not patchlevel) 2 LOC
 * win/pkgIndex.tcl	(not patchlevel, for tclregNN.dll)
 * README		(sections 0 and 2)
 * mac/README		(2 LOC)
 * win/README.binary	(sections 0-4)
 * win/README		(not patchlevel) (sections 0 and 2)
 * unix/README		(not patchlevel) (part (h))
 * tests/basic.test	(not patchlevel) (version checks)
 * tools/tcl.hpj.in	(not patchlevel, for windows installer)
 * tools/tcl.wse.in	(for windows installer)
 */

#define TCL_MAJOR_VERSION   8
#define TCL_MINOR_VERSION   2
#define TCL_RELEASE_LEVEL   TCL_FINAL_RELEASE
#define TCL_RELEASE_SERIAL  3

#define TCL_VERSION	    "8.2"
#define TCL_PATCH_LEVEL	    "8.2.3"

/*
 * The following definitions set up the proper options for Windows
 * compilers.  We use this method because there is no autoconf equivalent.
 */

#ifndef __WIN32__
#   if defined(_WIN32) || defined(WIN32)
#	define __WIN32__
#   endif
#endif

#ifdef __WIN32__
#   ifndef STRICT
#	define STRICT
#   endif
#   ifndef USE_PROTOTYPE
#	define USE_PROTOTYPE 1
#   endif
#   ifndef HAS_STDARG
#	define HAS_STDARG 1
#   endif
#   ifndef USE_PROTOTYPE
#	define USE_PROTOTYPE 1
#   endif

/*
 * Under Windows we need to call Tcl_Alloc in all cases to avoid competing
 * C run-time library issues.
 */

#   ifndef USE_TCLALLOC
#	define USE_TCLALLOC 1
#   endif
#endif /* __WIN32__ */

/*
 * The following definitions set up the proper options for Macintosh
 * compilers.  We use this method because there is no autoconf equivalent.
 */

#ifdef MAC_TCL
#   ifndef HAS_STDARG
#	define HAS_STDARG 1
#   endif
#   ifndef USE_TCLALLOC
#	define USE_TCLALLOC 1
#   endif
#   ifndef NO_STRERROR
#	define NO_STRERROR 1
#   endif
#   define INLINE 
#endif

/*
 * Utility macros: STRINGIFY takes an argument and wraps it in "" (double
 * quotation marks), JOIN joins two arguments.
 */

#define VERBATIM(x) x
#ifdef _MSC_VER
# define STRINGIFY(x) STRINGIFY1(x)
# define STRINGIFY1(x) #x
# define JOIN(a,b) JOIN1(a,b)
# define JOIN1(a,b) a##b
#else
# ifdef RESOURCE_INCLUDED
#  define STRINGIFY(x) STRINGIFY1(x)
#  define STRINGIFY1(x) #x
#  define JOIN(a,b) JOIN1(a,b)
#  define JOIN1(a,b) a##b
# else
#  ifdef __STDC__
#   define STRINGIFY(x) #x
#   define JOIN(a,b) a##b
#  else
#   define STRINGIFY(x) "x"
#   define JOIN(a,b) VERBATIM(a)VERBATIM(b)
#  endif
# endif
#endif

/*
 * Special macro to define mutexes, that doesn't do anything
 * if we are not using threads.
 */

#ifdef TCL_THREADS
#define TCL_DECLARE_MUTEX(name) static Tcl_Mutex name;
#else
#define TCL_DECLARE_MUTEX(name)
#endif

/*
 * Macros that eliminate the overhead of the thread synchronization
 * functions when compiling without thread support.
 */

#ifndef TCL_THREADS
#define Tcl_MutexLock(mutexPtr)
#define Tcl_MutexUnlock(mutexPtr)
#define Tcl_ConditionNotify(condPtr)
#define Tcl_ConditionWait(condPtr, mutexPtr, timePtr)
#endif /* TCL_THREADS */

/* 
 * A special definition used to allow this header file to be included 
 * in resource files so that they can get obtain version information from
 * this file.  Resource compilers don't like all the C stuff, like typedefs
 * and procedure declarations, that occur below.
 */

#ifndef RESOURCE_INCLUDED

#ifndef BUFSIZ
#include <stdio.h>
#endif

/*
 * Definitions that allow Tcl functions with variable numbers of
 * arguments to be used with either varargs.h or stdarg.h.  TCL_VARARGS
 * is used in procedure prototypes.  TCL_VARARGS_DEF is used to declare
 * the arguments in a function definiton: it takes the type and name of
 * the first argument and supplies the appropriate argument declaration
 * string for use in the function definition.  TCL_VARARGS_START
 * initializes the va_list data structure and returns the first argument.
 */

#if defined(__STDC__) || defined(HAS_STDARG)
#   include <stdarg.h>

#   define TCL_VARARGS(type, name) (type name, ...)
#   define TCL_VARARGS_DEF(type, name) (type name, ...)
#   define TCL_VARARGS_START(type, name, list) (va_start(list, name), name)
#else
#   include <varargs.h>

#   ifdef __cplusplus
#	define TCL_VARARGS(type, name) (type name, ...)
#	define TCL_VARARGS_DEF(type, name) (type va_alist, ...)
#   else
#	define TCL_VARARGS(type, name) ()
#	define TCL_VARARGS_DEF(type, name) (va_alist)
#   endif
#   define TCL_VARARGS_START(type, name, list) \
	(va_start(list), va_arg(list, type))
#endif

/*
 * Macros used to declare a function to be exported by a DLL.
 * Used by Windows, maps to no-op declarations on non-Windows systems.
 * The default build on windows is for a DLL, which causes the DLLIMPORT
 * and DLLEXPORT macros to be nonempty. To build a static library, the
 * macro STATIC_BUILD should be defined.
 */

#ifdef STATIC_BUILD
# define DLLIMPORT
# define DLLEXPORT
#else
# if defined(__WIN32__) && (defined(_MSC_VER) || (defined(__GNUC__) && defined(__declspec)))
#   define DLLIMPORT __declspec(dllimport)
#   define DLLEXPORT __declspec(dllexport)
# else
#  define DLLIMPORT
#  define DLLEXPORT
# endif
#endif

/*
 * These macros are used to control whether functions are being declared for
 * import or export.  If a function is being declared while it is being built
 * to be included in a shared library, then it should have the DLLEXPORT
 * storage class.  If is being declared for use by a module that is going to
 * link against the shared library, then it should have the DLLIMPORT storage
 * class.  If the symbol is beind declared for a static build or for use from a
 * stub library, then the storage class should be empty.
 *
 * The convention is that a macro called BUILD_xxxx, where xxxx is the
 * name of a library we are building, is set on the compile line for sources
 * that are to be placed in the library.  When this macro is set, the
 * storage class will be set to DLLEXPORT.  At the end of the header file, the
 * storage class will be reset to DLLIMPORt.
 */

#undef TCL_STORAGE_CLASS
#ifdef BUILD_tcl
# define TCL_STORAGE_CLASS DLLEXPORT
#else
# ifdef USE_TCL_STUBS
#  define TCL_STORAGE_CLASS
# else
#  define TCL_STORAGE_CLASS DLLIMPORT
# endif
#endif

/*
 * Definitions that allow this header file to be used either with or
 * without ANSI C features like function prototypes.
 */

#undef _ANSI_ARGS_
#undef CONST
#ifndef INLINE
#   define INLINE
#endif

#if ((defined(__STDC__) || defined(SABER)) && !defined(NO_PROTOTYPE)) || defined(__cplusplus) || defined(USE_PROTOTYPE)
#   define _USING_PROTOTYPES_ 1
#   define _ANSI_ARGS_(x)	x
#   define CONST const
#else
#   define _ANSI_ARGS_(x)	()
#   define CONST
#endif

#ifdef __cplusplus
#   define EXTERN extern "C" TCL_STORAGE_CLASS
#else
#   define EXTERN extern TCL_STORAGE_CLASS
#endif

/*
 * Macro to use instead of "void" for arguments that must have
 * type "void *" in ANSI C;  maps them to type "char *" in
 * non-ANSI systems.
 */
#ifndef __WIN32__
#ifndef VOID
#   ifdef __STDC__
#       define VOID void
#   else
#       define VOID char
#   endif
#endif
#else /* __WIN32__ */
/*
 * The following code is copied from winnt.h
 */
#ifndef VOID
#define VOID void
typedef char CHAR;
typedef short SHORT;
typedef long LONG;
#endif
#endif /* __WIN32__ */

/*
 * Miscellaneous declarations.
 */

#ifndef NULL
#define NULL 0
#endif

#ifndef _CLIENTDATA
#   if defined(__STDC__) || defined(__cplusplus)
    typedef void *ClientData;
#   else
    typedef int *ClientData;
#   endif /* __STDC__ */
#define _CLIENTDATA
#endif

/*
 * Data structures defined opaquely in this module. The definitions below
 * just provide dummy types. A few fields are made visible in Tcl_Interp
 * structures, namely those used for returning a string result from
 * commands. Direct access to the result field is discouraged in Tcl 8.0.
 * The interpreter result is either an object or a string, and the two
 * values are kept consistent unless some C code sets interp->result
 * directly. Programmers should use either the procedure Tcl_GetObjResult()
 * or Tcl_GetStringResult() to read the interpreter's result. See the
 * SetResult man page for details.
 * 
 * Note: any change to the Tcl_Interp definition below must be mirrored
 * in the "real" definition in tclInt.h.
 *
 * Note: Tcl_ObjCmdProc procedures do not directly set result and freeProc.
 * Instead, they set a Tcl_Obj member in the "real" structure that can be
 * accessed with Tcl_GetObjResult() and Tcl_SetObjResult().
 */

typedef struct Tcl_Interp {
    char *result;		/* If the last command returned a string
				 * result, this points to it. */
    void (*freeProc) _ANSI_ARGS_((char *blockPtr));
				/* Zero means the string result is
				 * statically allocated. TCL_DYNAMIC means
				 * it was allocated with ckalloc and should
				 * be freed with ckfree. Other values give
				 * the address of procedure to invoke to
				 * free the result. Tcl_Eval must free it
				 * before executing next command. */
    int errorLine;              /* When TCL_ERROR is returned, this gives
                                 * the line number within the command where
                                 * the error occurred (1 if first line). */
} Tcl_Interp;

typedef struct Tcl_AsyncHandler_ *Tcl_AsyncHandler;
typedef struct Tcl_Channel_ *Tcl_Channel;
typedef struct Tcl_Command_ *Tcl_Command;
typedef struct Tcl_Condition_ *Tcl_Condition;
typedef struct Tcl_EncodingState_ *Tcl_EncodingState;
typedef struct Tcl_Encoding_ *Tcl_Encoding;
typedef struct Tcl_Event Tcl_Event;
typedef struct Tcl_Mutex_ *Tcl_Mutex;
typedef struct Tcl_Pid_ *Tcl_Pid;
typedef struct Tcl_RegExp_ *Tcl_RegExp;
typedef struct Tcl_ThreadDataKey_ *Tcl_ThreadDataKey;
typedef struct Tcl_ThreadId_ *Tcl_ThreadId;
typedef struct Tcl_TimerToken_ *Tcl_TimerToken;
typedef struct Tcl_Trace_ *Tcl_Trace;
typedef struct Tcl_Var_ *Tcl_Var;

/*
 * Flag values passed to Tcl_GetRegExpFromObj.
 */

#define	TCL_REG_BASIC		000000	/* BREs (convenience) */
#define	TCL_REG_EXTENDED	000001	/* EREs */
#define	TCL_REG_ADVF		000002	/* advanced features in EREs */
#define	TCL_REG_ADVANCED	000003	/* AREs (which are also EREs) */
#define	TCL_REG_QUOTE		000004	/* no special characters, none */
#define	TCL_REG_NOCASE		000010	/* ignore case */
#define	TCL_REG_NOSUB		000020	/* don't care about subexpressions */
#define	TCL_REG_EXPANDED	000040	/* expanded format, white space &
					 * comments */
#define	TCL_REG_NLSTOP		000100  /* \n doesn't match . or [^ ] */
#define	TCL_REG_NLANCH		000200  /* ^ matches after \n, $ before */
#define	TCL_REG_NEWLINE		000300  /* newlines are line terminators */
#define	TCL_REG_CANMATCH	001000  /* report details on partial/limited
					 * matches */

/*
 * The following flag is experimental and only intended for use by Expect.  It
 * will probably go away in a later release.
 */

#define TCL_REG_BOSONLY		002000	/* prepend \A to pattern so it only
					 * matches at the beginning of the
					 * string. */

/*
 * Flags values passed to Tcl_RegExpExecObj.
 */

#define	TCL_REG_NOTBOL	0001	/* Beginning of string does not match ^.  */
#define	TCL_REG_NOTEOL	0002	/* End of string does not match $. */

/*
 * Structures filled in by Tcl_RegExpInfo.  Note that all offset values are
 * relative to the start of the match string, not the beginning of the
 * entire string.
 */

typedef struct Tcl_RegExpIndices {
    long start;		/* character offset of first character in match */
    long end;		/* character offset of first character after the
			 * match. */
} Tcl_RegExpIndices;

typedef struct Tcl_RegExpInfo {
    int nsubs;			/* number of subexpressions in the
				 * compiled expression */
    Tcl_RegExpIndices *matches;	/* array of nsubs match offset
				 * pairs */
    long extendStart;		/* The offset at which a subsequent
				 * match might begin. */
    long reserved;		/* Reserved for later use. */
} Tcl_RegExpInfo;

/*
 * Picky compilers complain if this typdef doesn't appear before the
 * struct's reference in tclDecls.h.
 */

typedef struct stat *Tcl_Stat_;

/*
 * When a TCL command returns, the interpreter contains a result from the
 * command. Programmers are strongly encouraged to use one of the
 * procedures Tcl_GetObjResult() or Tcl_GetStringResult() to read the
 * interpreter's result. See the SetResult man page for details. Besides
 * this result, the command procedure returns an integer code, which is 
 * one of the following:
 *
 * TCL_OK		Command completed normally; the interpreter's
 *			result contains	the command's result.
 * TCL_ERROR		The command couldn't be completed successfully;
 *			the interpreter's result describes what went wrong.
 * TCL_RETURN		The command requests that the current procedure
 *			return; the interpreter's result contains the
 *			procedure's return value.
 * TCL_BREAK		The command requests that the innermost loop
 *			be exited; the interpreter's result is meaningless.
 * TCL_CONTINUE		Go on to the next iteration of the current loop;
 *			the interpreter's result is meaningless.
 */

#define TCL_OK		0
#define TCL_ERROR	1
#define TCL_RETURN	2
#define TCL_BREAK	3
#define TCL_CONTINUE	4

#define TCL_RESULT_SIZE 200

/*
 * Argument descriptors for math function callbacks in expressions:
 */

typedef enum {TCL_INT, TCL_DOUBLE, TCL_EITHER} Tcl_ValueType;
typedef struct Tcl_Value {
    Tcl_ValueType type;		/* Indicates intValue or doubleValue is
				 * valid, or both. */
    long intValue;		/* Integer value. */
    double doubleValue;		/* Double-precision floating value. */
} Tcl_Value;

/*
 * Forward declaration of Tcl_Obj to prevent an error when the forward
 * reference to Tcl_Obj is encountered in the procedure types declared 
 * below.
 */

struct Tcl_Obj;

/*
 * Procedure types defined by Tcl:
 */

typedef int (Tcl_AppInitProc) _ANSI_ARGS_((Tcl_Interp *interp));
typedef int (Tcl_AsyncProc) _ANSI_ARGS_((ClientData clientData,
	Tcl_Interp *interp, int code));
typedef void (Tcl_ChannelProc) _ANSI_ARGS_((ClientData clientData, int mask));
typedef void (Tcl_CloseProc) _ANSI_ARGS_((ClientData data));
typedef void (Tcl_CmdDeleteProc) _ANSI_ARGS_((ClientData clientData));
typedef int (Tcl_CmdProc) _ANSI_ARGS_((ClientData clientData,
	Tcl_Interp *interp, int argc, char *argv[]));
typedef void (Tcl_CmdTraceProc) _ANSI_ARGS_((ClientData clientData,
	Tcl_Interp *interp, int level, char *command, Tcl_CmdProc *proc,
	ClientData cmdClientData, int argc, char *argv[]));
typedef void (Tcl_DupInternalRepProc) _ANSI_ARGS_((struct Tcl_Obj *srcPtr, 
        struct Tcl_Obj *dupPtr));
typedef int (Tcl_EncodingConvertProc)_ANSI_ARGS_((ClientData clientData,
	CONST char *src, int srcLen, int flags, Tcl_EncodingState *statePtr,
	char *dst, int dstLen, int *srcReadPtr, int *dstWrotePtr,
	int *dstCharsPtr));
typedef void (Tcl_EncodingFreeProc)_ANSI_ARGS_((ClientData clientData));
typedef int (Tcl_EventProc) _ANSI_ARGS_((Tcl_Event *evPtr, int flags));
typedef void (Tcl_EventCheckProc) _ANSI_ARGS_((ClientData clientData,
	int flags));
typedef int (Tcl_EventDeleteProc) _ANSI_ARGS_((Tcl_Event *evPtr,
        ClientData clientData));
typedef void (Tcl_EventSetupProc) _ANSI_ARGS_((ClientData clientData,
	int flags));
typedef void (Tcl_ExitProc) _ANSI_ARGS_((ClientData clientData));
typedef void (Tcl_FileProc) _ANSI_ARGS_((ClientData clientData, int mask));
typedef void (Tcl_FileFreeProc) _ANSI_ARGS_((ClientData clientData));
typedef void (Tcl_FreeInternalRepProc) _ANSI_ARGS_((struct Tcl_Obj *objPtr));
typedef void (Tcl_FreeProc) _ANSI_ARGS_((char *blockPtr));
typedef void (Tcl_IdleProc) _ANSI_ARGS_((ClientData clientData));
typedef void (Tcl_InterpDeleteProc) _ANSI_ARGS_((ClientData clientData,
	Tcl_Interp *interp));
typedef int (Tcl_MathProc) _ANSI_ARGS_((ClientData clientData,
	Tcl_Interp *interp, Tcl_Value *args, Tcl_Value *resultPtr));
typedef void (Tcl_NamespaceDeleteProc) _ANSI_ARGS_((ClientData clientData));
typedef int (Tcl_ObjCmdProc) _ANSI_ARGS_((ClientData clientData,
	Tcl_Interp *interp, int objc, struct Tcl_Obj *CONST objv[]));
typedef int (Tcl_PackageInitProc) _ANSI_ARGS_((Tcl_Interp *interp));
typedef void (Tcl_PanicProc) _ANSI_ARGS_(TCL_VARARGS(char *, format));
typedef void (Tcl_TcpAcceptProc) _ANSI_ARGS_((ClientData callbackData,
        Tcl_Channel chan, char *address, int port));
typedef void (Tcl_TimerProc) _ANSI_ARGS_((ClientData clientData));
typedef int (Tcl_SetFromAnyProc) _ANSI_ARGS_((Tcl_Interp *interp,
	struct Tcl_Obj *objPtr));
typedef void (Tcl_UpdateStringProc) _ANSI_ARGS_((struct Tcl_Obj *objPtr));
typedef char *(Tcl_VarTraceProc) _ANSI_ARGS_((ClientData clientData,
	Tcl_Interp *interp, char *part1, char *part2, int flags));
typedef void (Tcl_CreateFileHandlerProc) _ANSI_ARGS_((int fd, int mask,
	Tcl_FileProc *proc, ClientData clientData));
typedef void (Tcl_DeleteFileHandlerProc) _ANSI_ARGS_((int fd));

/*
 * The following structure represents a type of object, which is a
 * particular internal representation for an object plus a set of
 * procedures that provide standard operations on objects of that type.
 */

typedef struct Tcl_ObjType {
    char *name;			/* Name of the type, e.g. "int". */
    Tcl_FreeInternalRepProc *freeIntRepProc;
				/* Called to free any storage for the type's
				 * internal rep. NULL if the internal rep
				 * does not need freeing. */
    Tcl_DupInternalRepProc *dupIntRepProc;
    				/* Called to create a new object as a copy
				 * of an existing object. */
    Tcl_UpdateStringProc *updateStringProc;
    				/* Called to update the string rep from the
				 * type's internal representation. */
    Tcl_SetFromAnyProc *setFromAnyProc;
    				/* Called to convert the object's internal
				 * rep to this type. Frees the internal rep
				 * of the old type. Returns TCL_ERROR on
				 * failure. */
} Tcl_ObjType;

/*
 * One of the following structures exists for each object in the Tcl
 * system. An object stores a value as either a string, some internal
 * representation, or both.
 */

typedef struct Tcl_Obj {
    int refCount;		/* When 0 the object will be freed. */
    char *bytes;		/* This points to the first byte of the
				 * object's string representation. The array
				 * must be followed by a null byte (i.e., at
				 * offset length) but may also contain
				 * embedded null characters. The array's
				 * storage is allocated by ckalloc. NULL
				 * means the string rep is invalid and must
				 * be regenerated from the internal rep.
				 * Clients should use Tcl_GetStringFromObj
				 * or Tcl_GetString to get a pointer to the
				 * byte array as a readonly value. */
    int length;			/* The number of bytes at *bytes, not
				 * including the terminating null. */
    Tcl_ObjType *typePtr;	/* Denotes the object's type. Always
				 * corresponds to the type of the object's
				 * internal rep. NULL indicates the object
				 * has no internal rep (has no type). */
    union {			/* The internal representation: */
	long longValue;		/*   - an long integer value */
	double doubleValue;	/*   - a double-precision floating value */
	VOID *otherValuePtr;	/*   - another, type-specific value */
	struct {		/*   - internal rep as two pointers */
	    VOID *ptr1;
	    VOID *ptr2;
	} twoPtrValue;
    } internalRep;
} Tcl_Obj;

/*
 * Macros to increment and decrement a Tcl_Obj's reference count, and to
 * test whether an object is shared (i.e. has reference count > 1).
 * Note: clients should use Tcl_DecrRefCount() when they are finished using
 * an object, and should never call TclFreeObj() directly. TclFreeObj() is
 * only defined and made public in tcl.h to support Tcl_DecrRefCount's macro
 * definition. Note also that Tcl_DecrRefCount() refers to the parameter
 * "obj" twice. This means that you should avoid calling it with an
 * expression that is expensive to compute or has side effects.
 */

void		Tcl_IncrRefCount _ANSI_ARGS_((Tcl_Obj *objPtr));
void		Tcl_DecrRefCount _ANSI_ARGS_((Tcl_Obj *objPtr));
int		Tcl_IsShared _ANSI_ARGS_((Tcl_Obj *objPtr));

#ifdef TCL_MEM_DEBUG
#   define Tcl_IncrRefCount(objPtr) \
	Tcl_DbIncrRefCount(objPtr, __FILE__, __LINE__)
#   define Tcl_DecrRefCount(objPtr) \
	Tcl_DbDecrRefCount(objPtr, __FILE__, __LINE__)
#   define Tcl_IsShared(objPtr) \
	Tcl_DbIsShared(objPtr, __FILE__, __LINE__)
#else
#   define Tcl_IncrRefCount(objPtr) \
	++(objPtr)->refCount
#   define Tcl_DecrRefCount(objPtr) \
	if (--(objPtr)->refCount <= 0) TclFreeObj(objPtr)
#   define Tcl_IsShared(objPtr) \
	((objPtr)->refCount > 1)
#endif

/*
 * Macros and definitions that help to debug the use of Tcl objects.
 * When TCL_MEM_DEBUG is defined, the Tcl_New declarations are 
 * overridden to call debugging versions of the object creation procedures.
 */

#ifdef TCL_MEM_DEBUG
#  define Tcl_NewBooleanObj(val) \
     Tcl_DbNewBooleanObj(val, __FILE__, __LINE__)
#  define Tcl_NewByteArrayObj(bytes, len) \
     Tcl_DbNewByteArrayObj(bytes, len, __FILE__, __LINE__)
#  define Tcl_NewDoubleObj(val) \
     Tcl_DbNewDoubleObj(val, __FILE__, __LINE__)
#  define Tcl_NewIntObj(val) \
     Tcl_DbNewLongObj(val, __FILE__, __LINE__)
#  define Tcl_NewListObj(objc, objv) \
     Tcl_DbNewListObj(objc, objv, __FILE__, __LINE__)
#  define Tcl_NewLongObj(val) \
     Tcl_DbNewLongObj(val, __FILE__, __LINE__)
#  define Tcl_NewObj() \
     Tcl_DbNewObj(__FILE__, __LINE__)
#  define Tcl_NewStringObj(bytes, len) \
     Tcl_DbNewStringObj(bytes, len, __FILE__, __LINE__)
#endif /* TCL_MEM_DEBUG */

/*
 * The following structure contains the state needed by
 * Tcl_SaveResult.  No-one outside of Tcl should access any of these
 * fields.  This structure is typically allocated on the stack.
 */

typedef struct Tcl_SavedResult {
    char *result;
    Tcl_FreeProc *freeProc;
    Tcl_Obj *objResultPtr;
    char *appendResult;
    int appendAvl;
    int appendUsed;
    char resultSpace[TCL_RESULT_SIZE+1];
} Tcl_SavedResult;


/*
 * The following definitions support Tcl's namespace facility.
 * Note: the first five fields must match exactly the fields in a
 * Namespace structure (see tcl.h). 
 */

typedef struct Tcl_Namespace {
    char *name;                 /* The namespace's name within its parent
				 * namespace. This contains no ::'s. The
				 * name of the global namespace is ""
				 * although "::" is an synonym. */
    char *fullName;             /* The namespace's fully qualified name.
				 * This starts with ::. */
    ClientData clientData;      /* Arbitrary value associated with this
				 * namespace. */
    Tcl_NamespaceDeleteProc* deleteProc;
                                /* Procedure invoked when deleting the
				 * namespace to, e.g., free clientData. */
    struct Tcl_Namespace* parentPtr;
                                /* Points to the namespace that contains
				 * this one. NULL if this is the global
				 * namespace. */
} Tcl_Namespace;

/*
 * The following structure represents a call frame, or activation record.
 * A call frame defines a naming context for a procedure call: its local
 * scope (for local variables) and its namespace scope (used for non-local
 * variables; often the global :: namespace). A call frame can also define
 * the naming context for a namespace eval or namespace inscope command:
 * the namespace in which the command's code should execute. The
 * Tcl_CallFrame structures exist only while procedures or namespace
 * eval/inscope's are being executed, and provide a Tcl call stack.
 * 
 * A call frame is initialized and pushed using Tcl_PushCallFrame and
 * popped using Tcl_PopCallFrame. Storage for a Tcl_CallFrame must be
 * provided by the Tcl_PushCallFrame caller, and callers typically allocate
 * them on the C call stack for efficiency. For this reason, Tcl_CallFrame
 * is defined as a structure and not as an opaque token. However, most
 * Tcl_CallFrame fields are hidden since applications should not access
 * them directly; others are declared as "dummyX".
 *
 * WARNING!! The structure definition must be kept consistent with the
 * CallFrame structure in tclInt.h. If you change one, change the other.
 */

typedef struct Tcl_CallFrame {
    Tcl_Namespace *nsPtr;
    int dummy1;
    int dummy2;
    char *dummy3;
    char *dummy4;
    char *dummy5;
    int dummy6;
    char *dummy7;
    char *dummy8;
    int dummy9;
    char* dummy10;
} Tcl_CallFrame;

/*
 * Information about commands that is returned by Tcl_GetCommandInfo and
 * passed to Tcl_SetCommandInfo. objProc is an objc/objv object-based
 * command procedure while proc is a traditional Tcl argc/argv
 * string-based procedure. Tcl_CreateObjCommand and Tcl_CreateCommand
 * ensure that both objProc and proc are non-NULL and can be called to
 * execute the command. However, it may be faster to call one instead of
 * the other. The member isNativeObjectProc is set to 1 if an
 * object-based procedure was registered by Tcl_CreateObjCommand, and to
 * 0 if a string-based procedure was registered by Tcl_CreateCommand.
 * The other procedure is typically set to a compatibility wrapper that
 * does string-to-object or object-to-string argument conversions then
 * calls the other procedure.
 */
     
typedef struct Tcl_CmdInfo {
    int isNativeObjectProc;	 /* 1 if objProc was registered by a call to
				  * Tcl_CreateObjCommand; 0 otherwise.
				  * Tcl_SetCmdInfo does not modify this
				  * field. */
    Tcl_ObjCmdProc *objProc;	 /* Command's object-based procedure. */
    ClientData objClientData;	 /* ClientData for object proc. */
    Tcl_CmdProc *proc;		 /* Command's string-based procedure. */
    ClientData clientData;	 /* ClientData for string proc. */
    Tcl_CmdDeleteProc *deleteProc;
                                 /* Procedure to call when command is
                                  * deleted. */
    ClientData deleteData;	 /* Value to pass to deleteProc (usually
				  * the same as clientData). */
    Tcl_Namespace *namespacePtr; /* Points to the namespace that contains
				  * this command. Note that Tcl_SetCmdInfo
				  * will not change a command's namespace;
				  * use Tcl_RenameCommand to do that. */

} Tcl_CmdInfo;

/*
 * The structure defined below is used to hold dynamic strings.  The only
 * field that clients should use is the string field, and they should
 * never modify it.
 */

#define TCL_DSTRING_STATIC_SIZE 200
typedef struct Tcl_DString {
    char *string;		/* Points to beginning of string:  either
				 * staticSpace below or a malloced array. */
    int length;			/* Number of non-NULL characters in the
				 * string. */
    int spaceAvl;		/* Total number of bytes available for the
				 * string and its terminating NULL char. */
    char staticSpace[TCL_DSTRING_STATIC_SIZE];
				/* Space to use in common case where string
				 * is small. */
} Tcl_DString;

#define Tcl_DStringLength(dsPtr) ((dsPtr)->length)
#define Tcl_DStringValue(dsPtr) ((dsPtr)->string)
#define Tcl_DStringTrunc Tcl_DStringSetLength

/*
 * Definitions for the maximum number of digits of precision that may
 * be specified in the "tcl_precision" variable, and the number of
 * bytes of buffer space required by Tcl_PrintDouble.
 */
 
#define TCL_MAX_PREC 17
#define TCL_DOUBLE_SPACE (TCL_MAX_PREC+10)

/*
 * Definition for a number of bytes of buffer space sufficient to hold the
 * string representation of an integer in base 10 (assuming the existence
 * of 64-bit integers).
 */

#define TCL_INTEGER_SPACE	24

/*
 * Flag that may be passed to Tcl_ConvertElement to force it not to
 * output braces (careful!  if you change this flag be sure to change
 * the definitions at the front of tclUtil.c).
 */

#define TCL_DONT_USE_BRACES	1

/*
 * Flag that may be passed to Tcl_GetIndexFromObj to force it to disallow
 * abbreviated strings.
 */

#define TCL_EXACT	1

/*
 * Flag values passed to Tcl_RecordAndEval and/or Tcl_EvalObj.
 * WARNING: these bit choices must not conflict with the bit choices
 * for evalFlag bits in tclInt.h!!
 */

#define TCL_NO_EVAL		0x10000
#define TCL_EVAL_GLOBAL		0x20000
#define TCL_EVAL_DIRECT		0x40000

/*
 * Special freeProc values that may be passed to Tcl_SetResult (see
 * the man page for details):
 */

#define TCL_VOLATILE	((Tcl_FreeProc *) 1)
#define TCL_STATIC	((Tcl_FreeProc *) 0)
#define TCL_DYNAMIC	((Tcl_FreeProc *) 3)

/*
 * Flag values passed to variable-related procedures.
 */

#define TCL_GLOBAL_ONLY		 1
#define TCL_NAMESPACE_ONLY	 2
#define TCL_APPEND_VALUE	 4
#define TCL_LIST_ELEMENT	 8
#define TCL_TRACE_READS		 0x10
#define TCL_TRACE_WRITES	 0x20
#define TCL_TRACE_UNSETS	 0x40
#define TCL_TRACE_DESTROYED	 0x80
#define TCL_INTERP_DESTROYED	 0x100
#define TCL_LEAVE_ERR_MSG	 0x200
#define TCL_TRACE_ARRAY		 0x800

/*
 * The TCL_PARSE_PART1 flag is deprecated and has no effect. 
 * The part1 is now always parsed whenever the part2 is NULL.
 * (This is to avoid a common error when converting code to
 *  use the new object based APIs and forgetting to give the
 *  flag)
 */
#ifndef TCL_NO_DEPRECATED
#define TCL_PARSE_PART1          0x400
#endif


/*
 * Types for linked variables:
 */

#define TCL_LINK_INT		1
#define TCL_LINK_DOUBLE		2
#define TCL_LINK_BOOLEAN	3
#define TCL_LINK_STRING		4
#define TCL_LINK_READ_ONLY	0x80

/*
 * Forward declaration of Tcl_HashTable.  Needed by some C++ compilers
 * to prevent errors when the forward reference to Tcl_HashTable is
 * encountered in the Tcl_HashEntry structure.
 */

#ifdef __cplusplus
struct Tcl_HashTable;
#endif

/*
 * Structure definition for an entry in a hash table.  No-one outside
 * Tcl should access any of these fields directly;  use the macros
 * defined below.
 */

typedef struct Tcl_HashEntry {
    struct Tcl_HashEntry *nextPtr;	/* Pointer to next entry in this
					 * hash bucket, or NULL for end of
					 * chain. */
    struct Tcl_HashTable *tablePtr;	/* Pointer to table containing entry. */
    struct Tcl_HashEntry **bucketPtr;	/* Pointer to bucket that points to
					 * first entry in this entry's chain:
					 * used for deleting the entry. */
    ClientData clientData;		/* Application stores something here
					 * with Tcl_SetHashValue. */
    union {				/* Key has one of these forms: */
	char *oneWordValue;		/* One-word value for key. */
	int words[1];			/* Multiple integer words for key.
					 * The actual size will be as large
					 * as necessary for this table's
					 * keys. */
	char string[4];			/* String for key.  The actual size
					 * will be as large as needed to hold
					 * the key. */
    } key;				/* MUST BE LAST FIELD IN RECORD!! */
} Tcl_HashEntry;

/*
 * Structure definition for a hash table.  Must be in tcl.h so clients
 * can allocate space for these structures, but clients should never
 * access any fields in this structure.
 */

#define TCL_SMALL_HASH_TABLE 4
typedef struct Tcl_HashTable {
    Tcl_HashEntry **buckets;		/* Pointer to bucket array.  Each
					 * element points to first entry in
					 * bucket's hash chain, or NULL. */
    Tcl_HashEntry *staticBuckets[TCL_SMALL_HASH_TABLE];
					/* Bucket array used for small tables
					 * (to avoid mallocs and frees). */
    int numBuckets;			/* Total number of buckets allocated
					 * at **bucketPtr. */
    int numEntries;			/* Total number of entries present
					 * in table. */
    int rebuildSize;			/* Enlarge table when numEntries gets
					 * to be this large. */
    int downShift;			/* Shift count used in hashing
					 * function.  Designed to use high-
					 * order bits of randomized keys. */
    int mask;				/* Mask value used in hashing
					 * function. */
    int keyType;			/* Type of keys used in this table. 
					 * It's either TCL_STRING_KEYS,
					 * TCL_ONE_WORD_KEYS, or an integer
					 * giving the number of ints that
                                         * is the size of the key.
					 */
    Tcl_HashEntry *(*findProc) _ANSI_ARGS_((struct Tcl_HashTable *tablePtr,
	    CONST char *key));
    Tcl_HashEntry *(*createProc) _ANSI_ARGS_((struct Tcl_HashTable *tablePtr,
	    CONST char *key, int *newPtr));
} Tcl_HashTable;

/*
 * Structure definition for information used to keep track of searches
 * through hash tables:
 */

typedef struct Tcl_HashSearch {
    Tcl_HashTable *tablePtr;		/* Table being searched. */
    int nextIndex;			/* Index of next bucket to be
					 * enumerated after present one. */
    Tcl_HashEntry *nextEntryPtr;	/* Next entry to be enumerated in the
					 * the current bucket. */
} Tcl_HashSearch;

/*
 * Acceptable key types for hash tables:
 */

#define TCL_STRING_KEYS		0
#define TCL_ONE_WORD_KEYS	1

/*
 * Macros for clients to use to access fields of hash entries:
 */

#define Tcl_GetHashValue(h) ((h)->clientData)
#define Tcl_SetHashValue(h, value) ((h)->clientData = (ClientData) (value))
#define Tcl_GetHashKey(tablePtr, h) \
    ((char *) (((tablePtr)->keyType == TCL_ONE_WORD_KEYS) ? (h)->key.oneWordValue \
						: (h)->key.string))

/*
 * Macros to use for clients to use to invoke find and create procedures
 * for hash tables:
 */

#define Tcl_FindHashEntry(tablePtr, key) \
	(*((tablePtr)->findProc))(tablePtr, key)
#define Tcl_CreateHashEntry(tablePtr, key, newPtr) \
	(*((tablePtr)->createProc))(tablePtr, key, newPtr)

/*
 * Flag values to pass to Tcl_DoOneEvent to disable searches
 * for some kinds of events:
 */

#define TCL_DONT_WAIT		(1<<1)
#define TCL_WINDOW_EVENTS	(1<<2)
#define TCL_FILE_EVENTS		(1<<3)
#define TCL_TIMER_EVENTS	(1<<4)
#define TCL_IDLE_EVENTS		(1<<5)	/* WAS 0x10 ???? */
#define TCL_ALL_EVENTS		(~TCL_DONT_WAIT)

/*
 * The following structure defines a generic event for the Tcl event
 * system.  These are the things that are queued in calls to Tcl_QueueEvent
 * and serviced later by Tcl_DoOneEvent.  There can be many different
 * kinds of events with different fields, corresponding to window events,
 * timer events, etc.  The structure for a particular event consists of
 * a Tcl_Event header followed by additional information specific to that
 * event.
 */

struct Tcl_Event {
    Tcl_EventProc *proc;	/* Procedure to call to service this event. */
    struct Tcl_Event *nextPtr;	/* Next in list of pending events, or NULL. */
};

/*
 * Positions to pass to Tcl_QueueEvent:
 */

typedef enum {
    TCL_QUEUE_TAIL, TCL_QUEUE_HEAD, TCL_QUEUE_MARK
} Tcl_QueuePosition;

/*
 * Values to pass to Tcl_SetServiceMode to specify the behavior of notifier
 * event routines.
 */

#define TCL_SERVICE_NONE 0
#define TCL_SERVICE_ALL 1

/*
 * The following structure keeps is used to hold a time value, either as
 * an absolute time (the number of seconds from the epoch) or as an
 * elapsed time. On Unix systems the epoch is Midnight Jan 1, 1970 GMT.
 * On Macintosh systems the epoch is Midnight Jan 1, 1904 GMT.
 */

typedef struct Tcl_Time {
    long sec;			/* Seconds. */
    long usec;			/* Microseconds. */
} Tcl_Time;

typedef void (Tcl_SetTimerProc) _ANSI_ARGS_((Tcl_Time *timePtr));
typedef int (Tcl_WaitForEventProc) _ANSI_ARGS_((Tcl_Time *timePtr));

/*
 * Bits to pass to Tcl_CreateFileHandler and Tcl_CreateChannelHandler
 * to indicate what sorts of events are of interest:
 */

#define TCL_READABLE	(1<<1)
#define TCL_WRITABLE	(1<<2)
#define TCL_EXCEPTION	(1<<3)

/*
 * Flag values to pass to Tcl_OpenCommandChannel to indicate the
 * disposition of the stdio handles.  TCL_STDIN, TCL_STDOUT, TCL_STDERR,
 * are also used in Tcl_GetStdChannel.
 */

#define TCL_STDIN		(1<<1)	
#define TCL_STDOUT		(1<<2)
#define TCL_STDERR		(1<<3)
#define TCL_ENFORCE_MODE	(1<<4)

/*
 * Bits passed to Tcl_DriverClose2Proc to indicate which side of a channel
 * should be closed.
 */

#define TCL_CLOSE_READ		(1<<1)
#define TCL_CLOSE_WRITE	(1<<2)

/*
 * Value to use as the closeProc for a channel that supports the
 * close2Proc interface.
 */

#define TCL_CLOSE2PROC	((Tcl_DriverCloseProc *)1)

/*
 * Typedefs for the various operations in a channel type:
 */

typedef int	(Tcl_DriverBlockModeProc) _ANSI_ARGS_((
		    ClientData instanceData, int mode));
typedef int	(Tcl_DriverCloseProc) _ANSI_ARGS_((ClientData instanceData,
		    Tcl_Interp *interp));
typedef int	(Tcl_DriverClose2Proc) _ANSI_ARGS_((ClientData instanceData,
		    Tcl_Interp *interp, int flags));
typedef int	(Tcl_DriverInputProc) _ANSI_ARGS_((ClientData instanceData,
		    char *buf, int toRead, int *errorCodePtr));
typedef int	(Tcl_DriverOutputProc) _ANSI_ARGS_((ClientData instanceData,
		    char *buf, int toWrite, int *errorCodePtr));
typedef int	(Tcl_DriverSeekProc) _ANSI_ARGS_((ClientData instanceData,
		    long offset, int mode, int *errorCodePtr));
typedef int	(Tcl_DriverSetOptionProc) _ANSI_ARGS_((
		    ClientData instanceData, Tcl_Interp *interp,
	            char *optionName, char *value));
typedef int	(Tcl_DriverGetOptionProc) _ANSI_ARGS_((
		    ClientData instanceData, Tcl_Interp *interp,
		    char *optionName, Tcl_DString *dsPtr));
typedef void	(Tcl_DriverWatchProc) _ANSI_ARGS_((
		    ClientData instanceData, int mask));
typedef int	(Tcl_DriverGetHandleProc) _ANSI_ARGS_((
		    ClientData instanceData, int direction,
		    ClientData *handlePtr));

/*
 * The following declarations either map ckalloc and ckfree to
 * malloc and free, or they map them to procedures with all sorts
 * of debugging hooks defined in tclCkalloc.c.
 */

#ifdef TCL_MEM_DEBUG

#   define ckalloc(x) Tcl_DbCkalloc(x, __FILE__, __LINE__)
#   define ckfree(x)  Tcl_DbCkfree(x, __FILE__, __LINE__)
#   define ckrealloc(x,y) Tcl_DbCkrealloc((x), (y),__FILE__, __LINE__)

#else /* !TCL_MEM_DEBUG */

/*
 * If we are not using the debugging allocator, we should call the 
 * Tcl_Alloc, et al. routines in order to guarantee that every module
 * is using the same memory allocator both inside and outside of the
 * Tcl library.
 */

#   define ckalloc(x) Tcl_Alloc(x)
#   define ckfree(x) Tcl_Free(x)
#   define ckrealloc(x,y) Tcl_Realloc(x,y)
#   define Tcl_InitMemory(x)
#   define Tcl_DumpActiveMemory(x)
#   define Tcl_ValidateAllMemory(x,y)

#endif /* !TCL_MEM_DEBUG */

/*
 * Enum for different end of line translation and recognition modes.
 */

typedef enum Tcl_EolTranslation {
    TCL_TRANSLATE_AUTO,			/* Eol == \r, \n and \r\n. */
    TCL_TRANSLATE_CR,			/* Eol == \r. */
    TCL_TRANSLATE_LF,			/* Eol == \n. */
    TCL_TRANSLATE_CRLF			/* Eol == \r\n. */
} Tcl_EolTranslation;

/*
 * struct Tcl_ChannelType:
 *
 * One such structure exists for each type (kind) of channel.
 * It collects together in one place all the functions that are
 * part of the specific channel type.
 */

typedef struct Tcl_ChannelType {
    char *typeName;			/* The name of the channel type in Tcl
                                         * commands. This storage is owned by
                                         * channel type. */
    Tcl_DriverBlockModeProc *blockModeProc;
    					/* Set blocking mode for the
                                         * raw channel. May be NULL. */
    Tcl_DriverCloseProc *closeProc;	/* Procedure to call to close the
                                         * channel, or TCL_CLOSE2PROC if the
                                         * close2Proc should be used
                                         * instead. */
    Tcl_DriverInputProc *inputProc;	/* Procedure to call for input
                                         * on channel. */
    Tcl_DriverOutputProc *outputProc;	/* Procedure to call for output
                                         * on channel. */
    Tcl_DriverSeekProc *seekProc;	/* Procedure to call to seek
                                         * on the channel. May be NULL. */
    Tcl_DriverSetOptionProc *setOptionProc;
    					/* Set an option on a channel. */
    Tcl_DriverGetOptionProc *getOptionProc;
    					/* Get an option from a channel. */
    Tcl_DriverWatchProc *watchProc;	/* Set up the notifier to watch
                                         * for events on this channel. */
    Tcl_DriverGetHandleProc *getHandleProc;
					/* Get an OS handle from the channel
                                         * or NULL if not supported. */
    Tcl_DriverClose2Proc *close2Proc;   /* Procedure to call to close the
					 * channel if the device supports
					 * closing the read & write sides
					 * independently. */
} Tcl_ChannelType;

/*
 * The following flags determine whether the blockModeProc above should
 * set the channel into blocking or nonblocking mode. They are passed
 * as arguments to the blockModeProc procedure in the above structure.
 */

#define TCL_MODE_BLOCKING 0		/* Put channel into blocking mode. */
#define TCL_MODE_NONBLOCKING 1		/* Put channel into nonblocking
					 * mode. */

/*
 * Enum for different types of file paths.
 */

typedef enum Tcl_PathType {
    TCL_PATH_ABSOLUTE,
    TCL_PATH_RELATIVE,
    TCL_PATH_VOLUME_RELATIVE
} Tcl_PathType;

/*
 * The following structure represents the Notifier functions that
 * you can override with the Tcl_SetNotifier call.
 */

typedef struct Tcl_NotifierProcs {
    Tcl_SetTimerProc *setTimerProc;
    Tcl_WaitForEventProc *waitForEventProc;
    Tcl_CreateFileHandlerProc *createFileHandlerProc;
    Tcl_DeleteFileHandlerProc *deleteFileHandlerProc;
} Tcl_NotifierProcs;

/*
 * The following structure represents a user-defined encoding.  It collects
 * together all the functions that are used by the specific encoding.
 */

typedef struct Tcl_EncodingType {
    CONST char *encodingName;	/* The name of the encoding, e.g.  "euc-jp".
				 * This name is the unique key for this
				 * encoding type. */
    Tcl_EncodingConvertProc *toUtfProc;
				/* Procedure to convert from external
				 * encoding into UTF-8. */
    Tcl_EncodingConvertProc *fromUtfProc;
				/* Procedure to convert from UTF-8 into
				 * external encoding. */
    Tcl_EncodingFreeProc *freeProc;
				/* If non-NULL, procedure to call when this
				 * encoding is deleted. */
    ClientData clientData;	/* Arbitrary value associated with encoding
				 * type.  Passed to conversion procedures. */
    int nullSize;		/* Number of zero bytes that signify
				 * end-of-string in this encoding.  This
				 * number is used to determine the source
				 * string length when the srcLen argument is
				 * negative.  Must be 1 or 2. */
} Tcl_EncodingType;    

/*
 * The following definitions are used as values for the conversion control
 * flags argument when converting text from one character set to another:
 *
 * TCL_ENCODING_START:	     	Signifies that the source buffer is the first
 *				block in a (potentially multi-block) input
 *				stream.  Tells the conversion procedure to
 *				reset to an initial state and perform any
 *				initialization that needs to occur before the
 *				first byte is converted.  If the source
 *				buffer contains the entire input stream to be
 *				converted, this flag should be set.
 *
 * TCL_ENCODING_END:		Signifies that the source buffer is the last
 *				block in a (potentially multi-block) input
 *				stream.  Tells the conversion routine to
 *				perform any finalization that needs to occur
 *				after the last byte is converted and then to
 *				reset to an initial state.  If the source
 *				buffer contains the entire input stream to be
 *				converted, this flag should be set.
 *				
 * TCL_ENCODING_STOPONERROR:	If set, then the converter will return
 *				immediately upon encountering an invalid
 *				byte sequence or a source character that has
 *				no mapping in the target encoding.  If clear,
 *				then the converter will skip the problem,
 *				substituting one or more "close" characters
 *				in the destination buffer and then continue
 *				to sonvert the source.
 */

#define TCL_ENCODING_START		0x01
#define TCL_ENCODING_END		0x02
#define TCL_ENCODING_STOPONERROR	0x04

/*
 *----------------------------------------------------------------
 * The following data structures and declarations are for the new
 * Tcl parser.	This stuff should all move to tcl.h eventually.
 *----------------------------------------------------------------
 */

/*
 * For each word of a command, and for each piece of a word such as a
 * variable reference, one of the following structures is created to
 * describe the token.
 */

typedef struct Tcl_Token {
    int type;			/* Type of token, such as TCL_TOKEN_WORD;
				 * see below for valid types. */
    char *start;		/* First character in token. */
    int size;			/* Number of bytes in token. */
    int numComponents;		/* If this token is composed of other
				 * tokens, this field tells how many of
				 * them there are (including components of
				 * components, etc.).  The component tokens
				 * immediately follow this one. */
} Tcl_Token;

/*
 * Type values defined for Tcl_Token structures.  These values are
 * defined as mask bits so that it's easy to check for collections of
 * types.
 *
 * TCL_TOKEN_WORD -		The token describes one word of a command,
 *				from the first non-blank character of
 *				the word (which may be " or {) up to but
 *				not including the space, semicolon, or
 *				bracket that terminates the word. 
 *				NumComponents counts the total number of
 *				sub-tokens that make up the word.  This
 *				includes, for example, sub-tokens of
 *				TCL_TOKEN_VARIABLE tokens.
 * TCL_TOKEN_SIMPLE_WORD -	This token is just like TCL_TOKEN_WORD
 *				except that the word is guaranteed to
 *				consist of a single TCL_TOKEN_TEXT
 *				sub-token.
 * TCL_TOKEN_TEXT -		The token describes a range of literal
 *				text that is part of a word. 
 *				NumComponents is always 0.
 * TCL_TOKEN_BS -		The token describes a backslash sequence
 *				that must be collapsed.	 NumComponents
 *				is always 0.
 * TCL_TOKEN_COMMAND -		The token describes a command whose result
 *				must be substituted into the word.  The
 *				token includes the enclosing brackets. 
 *				NumComponents is always 0.
 * TCL_TOKEN_VARIABLE -		The token describes a variable
 *				substitution, including the dollar sign,
 *				variable name, and array index (if there
 *				is one) up through the right
 *				parentheses.  NumComponents tells how
 *				many additional tokens follow to
 *				represent the variable name.  The first
 *				token will be a TCL_TOKEN_TEXT token
 *				that describes the variable name.  If
 *				the variable is an array reference then
 *				there will be one or more additional
 *				tokens, of type TCL_TOKEN_TEXT,
 *				TCL_TOKEN_BS, TCL_TOKEN_COMMAND, and
 *				TCL_TOKEN_VARIABLE, that describe the
 *				array index; numComponents counts the
 *				total number of nested tokens that make
 *				up the variable reference, including
 *				sub-tokens of TCL_TOKEN_VARIABLE tokens.
 * TCL_TOKEN_SUB_EXPR -		The token describes one subexpression of a
 *				expression, from the first non-blank
 *				character of the subexpression up to but not
 *				including the space, brace, or bracket
 *				that terminates the subexpression. 
 *				NumComponents counts the total number of
 *				following subtokens that make up the
 *				subexpression; this includes all subtokens
 *				for any nested TCL_TOKEN_SUB_EXPR tokens.
 *				For example, a numeric value used as a
 *				primitive operand is described by a
 *				TCL_TOKEN_SUB_EXPR token followed by a
 *				TCL_TOKEN_TEXT token. A binary subexpression
 *				is described by a TCL_TOKEN_SUB_EXPR token
 *				followed by the	TCL_TOKEN_OPERATOR token
 *				for the operator, then TCL_TOKEN_SUB_EXPR
 *				tokens for the left then the right operands.
 * TCL_TOKEN_OPERATOR -		The token describes one expression operator.
 *				An operator might be the name of a math
 *				function such as "abs". A TCL_TOKEN_OPERATOR
 *				token is always preceeded by one
 *				TCL_TOKEN_SUB_EXPR token for the operator's
 *				subexpression, and is followed by zero or
 *				more TCL_TOKEN_SUB_EXPR tokens for the
 *				operator's operands. NumComponents is
 *				always 0.
 */

#define TCL_TOKEN_WORD		1
#define TCL_TOKEN_SIMPLE_WORD	2
#define TCL_TOKEN_TEXT		4
#define TCL_TOKEN_BS		8
#define TCL_TOKEN_COMMAND	16
#define TCL_TOKEN_VARIABLE	32
#define TCL_TOKEN_SUB_EXPR	64
#define TCL_TOKEN_OPERATOR	128

/*
 * Parsing error types.  On any parsing error, one of these values
 * will be stored in the error field of the Tcl_Parse structure
 * defined below.
 */

#define TCL_PARSE_SUCCESS		0
#define TCL_PARSE_QUOTE_EXTRA		1
#define TCL_PARSE_BRACE_EXTRA		2
#define TCL_PARSE_MISSING_BRACE		3
#define TCL_PARSE_MISSING_BRACKET	4
#define TCL_PARSE_MISSING_PAREN		5
#define TCL_PARSE_MISSING_QUOTE		6
#define TCL_PARSE_MISSING_VAR_BRACE	7
#define TCL_PARSE_SYNTAX		8
#define TCL_PARSE_BAD_NUMBER		9

/*
 * A structure of the following type is filled in by Tcl_ParseCommand.
 * It describes a single command parsed from an input string.
 */

#define NUM_STATIC_TOKENS 20

typedef struct Tcl_Parse {
    char *commentStart;		/* Pointer to # that begins the first of
				 * one or more comments preceding the
				 * command. */
    int commentSize;		/* Number of bytes in comments (up through
				 * newline character that terminates the
				 * last comment).  If there were no
				 * comments, this field is 0. */
    char *commandStart;		/* First character in first word of command. */
    int commandSize;		/* Number of bytes in command, including
				 * first character of first word, up
				 * through the terminating newline,
				 * close bracket, or semicolon. */
    int numWords;		/* Total number of words in command.  May
				 * be 0. */
    Tcl_Token *tokenPtr;	/* Pointer to first token representing
				 * the words of the command.  Initially
				 * points to staticTokens, but may change
				 * to point to malloc-ed space if command
				 * exceeds space in staticTokens. */
    int numTokens;		/* Total number of tokens in command. */
    int tokensAvailable;	/* Total number of tokens available at
				 * *tokenPtr. */
    int errorType;		/* One of the parsing error types defined
				 * above. */

    /*
     * The fields below are intended only for the private use of the
     * parser.	They should not be used by procedures that invoke
     * Tcl_ParseCommand.
     */

    char *string;		/* The original command string passed to
				 * Tcl_ParseCommand. */
    char *end;			/* Points to the character just after the
				 * last one in the command string. */
    Tcl_Interp *interp;		/* Interpreter to use for error reporting,
				 * or NULL. */
    char *term;			/* Points to character in string that
				 * terminated most recent token.  Filled in
				 * by ParseTokens.  If an error occurs,
				 * points to beginning of region where the
				 * error occurred (e.g. the open brace if
				 * the close brace is missing). */
    int incomplete;		/* This field is set to 1 by Tcl_ParseCommand
				 * if the command appears to be incomplete.
				 * This information is used by
				 * Tcl_CommandComplete. */
    Tcl_Token staticTokens[NUM_STATIC_TOKENS];
				/* Initial space for tokens for command.
				 * This space should be large enough to
				 * accommodate most commands; dynamic
				 * space is allocated for very large
				 * commands that don't fit here. */
} Tcl_Parse;

/*
 * The following definitions are the error codes returned by the conversion
 * routines:
 *
 * TCL_OK:			All characters were converted.
 *
 * TCL_CONVERT_NOSPACE:		The output buffer would not have been large
 *				enough for all of the converted data; as many
 *				characters as could fit were converted though.
 *
 * TCL_CONVERT_MULTIBYTE:	The last few bytes in the source string were
 *				the beginning of a multibyte sequence, but
 *				more bytes were needed to complete this
 *				sequence.  A subsequent call to the conversion
 *				routine should pass the beginning of this
 *				unconverted sequence plus additional bytes
 *				from the source stream to properly convert
 *				the formerly split-up multibyte sequence.
 *
 * TCL_CONVERT_SYNTAX:		The source stream contained an invalid
 *				character sequence.  This may occur if the
 *				input stream has been damaged or if the input
 *				encoding method was misidentified.  This error
 *				is reported only if TCL_ENCODING_STOPONERROR
 *				was specified.
 * 
 * TCL_CONVERT_UNKNOWN:		The source string contained a character
 *				that could not be represented in the target
 *				encoding.  This error is reported only if
 *				TCL_ENCODING_STOPONERROR was specified.
 */

#define TCL_CONVERT_MULTIBYTE		-1
#define TCL_CONVERT_SYNTAX		-2
#define TCL_CONVERT_UNKNOWN		-3
#define TCL_CONVERT_NOSPACE		-4

/*
 * The maximum number of bytes that are necessary to represent a single
 * Unicode character in UTF-8.
 */

#define TCL_UTF_MAX		3

/*
 * This represents a Unicode character.  
 */

typedef unsigned short Tcl_UniChar;

/*
 * Deprecated Tcl procedures:
 */

#ifndef TCL_NO_DEPRECATED
#define Tcl_EvalObj(interp,objPtr) Tcl_EvalObjEx((interp),(objPtr),0)
#define Tcl_GlobalEvalObj(interp,objPtr) \
	Tcl_EvalObjEx((interp),(objPtr),TCL_EVAL_GLOBAL)
#endif

/*
 * These function have been renamed. The old names are deprecated, but we
 * define these macros for backwards compatibilty.
 */

#define Tcl_Ckalloc Tcl_Alloc
#define Tcl_Ckfree Tcl_Free
#define Tcl_Ckrealloc Tcl_Realloc
#define Tcl_Return Tcl_SetResult
#define Tcl_TildeSubst Tcl_TranslateFileName
#define panic Tcl_Panic
#define panicVA Tcl_PanicVA

/*
 * The following constant is used to test for older versions of Tcl
 * in the stubs tables.
 *
 * Jan Nijtman's plus patch uses 0xFCA1BACF, so we need to pick a different
 * value since the stubs tables don't match.
 */

#define TCL_STUB_MAGIC 0xFCA3BACF

/*
 * The following function is required to be defined in all stubs aware
 * extensions.  The function is actually implemented in the stub
 * library, not the main Tcl library, although there is a trivial
 * implementation in the main library in case an extension is statically
 * linked into an application.
 */

EXTERN char *		Tcl_InitStubs _ANSI_ARGS_((Tcl_Interp *interp,
			    char *version, int exact));

#ifndef USE_TCL_STUBS

/*
 * When not using stubs, make it a macro.
 */

#define Tcl_InitStubs(interp, version, exact) \
    Tcl_PkgRequire(interp, "Tcl", version, exact)

#endif


/*
 * Include the public function declarations that are accessible via
 * the stubs table.
 */

#include "tclDecls.h"

/*
 * Public functions that are not accessible via the stubs table.
 */

EXTERN void Tcl_Main _ANSI_ARGS_((int argc, char **argv,
	Tcl_AppInitProc *appInitProc));

/*
 * Convenience declaration of Tcl_AppInit for backwards compatibility.
 * This function is not *implemented* by the tcl library, so the storage
 * class is neither DLLEXPORT nor DLLIMPORT
 */

#undef TCL_STORAGE_CLASS
#define TCL_STORAGE_CLASS

EXTERN int		Tcl_AppInit _ANSI_ARGS_((Tcl_Interp *interp));

#endif /* RESOURCE_INCLUDED */

#undef TCL_STORAGE_CLASS
#define TCL_STORAGE_CLASS DLLIMPORT

/*
 * end block for C++
 */
    
#ifdef __cplusplus
}
#endif
    
#endif /* _TCL */
