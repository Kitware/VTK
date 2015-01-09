/*******************************************************************/
/*                               XDMF                              */
/*                   eXtensible Data Model and Format              */
/*                                                                 */
/*  Id : Id  */
/*  Date : $Date$ */
/*  Version : $Revision$ */
/*                                                                 */
/*  Author:                                                        */
/*     Jerry A. Clarke                                             */
/*     clarke@arl.army.mil                                         */
/*     US Army Research Laboratory                                 */
/*     Aberdeen Proving Ground, MD                                 */
/*                                                                 */
/*     Copyright @ 2002 US Army Research Laboratory                */
/*     All Rights Reserved                                         */
/*     See Copyright.txt or http://www.arl.hpc.mil/ice for details */
/*                                                                 */
/*     This software is distributed WITHOUT ANY WARRANTY; without  */
/*     even the implied warranty of MERCHANTABILITY or FITNESS     */
/*     FOR A PARTICULAR PURPOSE.  See the above copyright notice   */
/*     for more information.                                       */
/*                                                                 */
/*******************************************************************/
/*! \mainpage XDMF Object Directory API
*
* \section intro Introduction
*
* The eXtensible Data Model and Format (XDMF) is a distributed data hub for accessing scientific data in
* High Performance Computing (HPC) applications. XDMF defines a data model and format as
* well as facilities for accessing the data in a distributed environment.
* The Data Format utilizes
* the Hierarchical Data Format Version 5 (HDF5) , from NCSA, to store large amount of data
* that can be accessed in a host independent fashion. The eXtensible Markup Language
* (XML) is used to describe the high level organization of the data in HDF5. Finally
* a built-in Distributed Shared Memory (DSM) system is used to allow distributed access to cooperating
* applications.

* Access to XDMF is provided via a C++ class library. 
* While usage of this library is not required it
* provides a convenience layer and error checking facility that 
* most users will find beneficial.
* For some users, however, accessing HDF5 via the NCSA provided Application Programmers
* Interface may be desirable in order to gain finer control
* over data access. In addition, XML is
* pervasive in WEB applications and there are a number of good, third party XML tools and
* libraries. These tools may provide useful functions not available in the XDMF convenience
* layer.

* XDMF differs from other "Data Model and Format" efforts in that the Light data is logically (
* and possibly physically ) separated from the Heavy data. Light data is considered to be both
* "data about the data" such as dimensions and name, as well as small quantities of computed
* data. Heavy data is considered to be large amounts of data. For example, in a three
* dimensional structural mechanics calculation, the size and dimensions of the computational
* grid are Light data while the actual X, Y, and Z values 
* for the grid are Heavy data. Calculated
* values like "Pressure at a node" are Heavy, while "Total Residual Mass" for the entire
* calculation is Light. As a rule of thumb, if the data is 
*  "less that 50-100" values, it's Light data.
* 
* This API is also "wrapped" using SWIG. This provides a scripting language
* interface to Tcl, Python, Perl, and Java.
*
*  
*/

#include "XdmfConfig.h"

#ifndef __XdmfObject_h
#define __XdmfObject_h

# ifndef SWIG
#if !defined( WIN32 ) || defined(__CYGWIN__)
#define UNIX
#endif /* WIN32 */

# ifdef __hpux
#  include <sys/param.h>
# endif

# ifdef UNIX
#  include "sys/file.h"
#  include "strings.h"
#  define STRCASECMP strcasecmp
#  define STRNCASECMP strncasecmp
#  define STRCMP strcmp
#  define STRNCMP strncmp
# endif

# if defined(WIN32) && !defined(__CYGWIN__)
/* winsock.h seems not being used in Xdmf, but
*  will cause conflicts with projects that includes "winsock2.h" */
/* #  include "winsock.h" */
/* String comparison routine. */
#  define STRCASECMP _stricmp
#  define STRNCASECMP _strnicmp
#  define STRCMP strcmp
#  define STRNCMP strncmp
# endif
#endif /* SWIG */

#include <string.h> /* strcmp for XdmfSetStringMacro */

#ifdef __cplusplus
/* Use ANSI C++ --------------------------------------------- */
# ifndef SWIG
#  ifdef _MSC_VER
#   pragma warning (push, 1)
#   pragma warning (disable: 4702)
#  endif
#  include <string>
#  ifdef _MSC_VER
#   pragma warning(pop)
#  endif
#  ifdef XDMF_USE_ANSI_STDLIB
#   include <iostream>
/* We'll need to migrate to sstream */
/* #   include <sstream> */
/* But for now use strstream */
/* RHEL give Deprecated warning ... ignore them for now */
#undef __DEPRECATED
#   include <strstream>
#define __DEPRECATED
#   include <fstream>
#   include <iomanip>
using std::cerr;
using std::cout;
using std::cin;
using std::ios;
using std::endl;
using std::ends;
using std::ostream;
using std::istream;
using std::ostrstream;
using std::istrstream;
using std::strstream;
using std::ofstream;
using std::ifstream;
using std::fstream;
using std::hex;



/* otherwise, non-ANSI ----------------------------------------------------- */
#  else /* XDMF_USE_ANSI_STDLIB */
#   include <iostream.h>
#   if defined(_MSC_VER)
#     include <strstrea.h>
#   else
#     include <strstream.h>
#   endif
#   include <fstream.h>
#   include <iomanip.h>
#  endif /* XDMF_USE_ANSI_STDLIB */
# endif /* SWIG */
#endif /* __cplusplus */

#ifdef XDMF_NO_STD_NAMESPACE
#  define xdmfstd
#else
#  define xdmfstd std
#endif

#include "XdmfExport.h"

#define XDMF_VERSION 2.1
#define XDMF_VERSION_STRING "2.1"

#define XDMF_SUCCESS  1
#define XDMF_FAIL  -1

#define XDMF_TRUE  1
#define XDMF_FALSE  0

#define XDMF_MAX_DIMENSION  10
#define XDMF_MAX_STRING_LENGTH  1024

/* #define XDMF_LIST_KEY  ICE_LIST_KEY */
#define XDMF_BIG_INT XDMF_64_INT 

#define XDMF_DEFAULT_INDEX  -1

#define XDMF_SELECT_SLAB    1
#define XDMF_SELECT_INDEX  2

#define  XDMF_UNKNOWN_TYPE   -1
#define  XDMF_INT8_TYPE      1
#define  XDMF_INT16_TYPE     6
#define  XDMF_INT32_TYPE     2
#define  XDMF_INT64_TYPE     3
#define  XDMF_FLOAT32_TYPE   4
#define  XDMF_FLOAT64_TYPE   5
#define  XDMF_UINT8_TYPE     7
#define  XDMF_UINT16_TYPE    8
#define  XDMF_UINT32_TYPE    9
#define XDMF_COMPOUND_TYPE  0x10

/* typedef ICE_CHAR       XDMF_CHAR; */
/* typedef ICE_PTR        XDMF_PTR; */
/* typedef ICE_8_INT      XDMF_8_INT; */
/* typedef ICE_8_U_INT    XDMF_8_UINT; */
/* typedef ICE_16_INT     XDMF_16_INT; */
/* typedef ICE_16_U_INT   XDMF_16_UINT; */
/* typedef ICE_32_INT     XDMF_32_INT; */
/* typedef ICE_32_U_INT   XDMF_32_UINT; */
/* typedef ICE_64_INT     XDMF_64_INT; */
/* typedef ICE_FLOAT      XDMF_FLOAT; */
/* typedef ICE_DOUBLE     XDMF_DOUBLE; */

#ifdef __cplusplus
# ifndef SWIG
#  include <string>
# endif 
//typedef xdmfstd::string XdmfSTDString;
#endif

#ifndef SWIG
typedef XDMF_32_INT  XdmfBoolean;
typedef XDMF_CHAR *  XdmfString;
typedef const XDMF_CHAR *  XdmfConstString;
typedef XDMF_PTR  XdmfPointer;
typedef XDMF_8_U_INT   XdmfUInt8;
typedef XDMF_16_U_INT  XdmfUInt16;
typedef XDMF_32_U_INT  XdmfUInt32;
typedef XDMF_8_INT   XdmfInt8;
typedef XDMF_16_INT  XdmfInt16;
typedef XDMF_32_INT  XdmfInt32;
typedef XDMF_64_INT  XdmfInt64;
typedef XDMF_FLOAT  XdmfFloat32;
typedef XDMF_DOUBLE  XdmfFloat64;

typedef XdmfInt8  XdmfByte;
typedef XdmfInt64  XdmfLength;
#else
/***************************************
Swig only understands certain types
and doesn't to well with typedefs
so we need to fool the interface code
***************************************/
typedef int             XdmfBoolean;
typedef char *          XdmfString;
typedef const char *    XdmfConstString;
typedef void *          XdmfPointer;
typedef unsigned char   XdmfUInt8;
typedef unsigned short  XdmfUInt16;
typedef unsigned int    XdmfUInt32;
typedef char            XdmfInt8;
typedef short           XdmfInt16;
typedef int             XdmfInt32;
typedef long long       XdmfInt64;
typedef float           XdmfFloat32;
typedef double          XdmfFloat64;

typedef unsigned char   XdmfByte;
typedef long long       XdmfLength;

#endif

#ifdef SWIG

#ifdef SWIGJAVA
%echo "Applying Special Java Typemaps .... "
#endif /* SWIGJAVA */
#ifdef SWIGTCL
%echo "Applying Special Tcl Typemaps ..... "
%include typemaps.i
/**************************************
Since the scripting view and the real
view of these types might not jive, we
need to make sure ....
**************************************/
//%typemap(tcl8, in) XdmfInt32, XdmfInt64, XdmfLength {
//  long  Value;
//  int  ArgIndex;

  // Really bad hack to get index
  // Assumes SWIG uses names : objc, objv
  // This is necessary because SWIG is messing up
  // Optional Arguments
//  sscanf("$arg", "objv[%d", &ArgIndex );
//  if( ArgIndex < objc ){
//    Tcl_GetLongFromObj( interp, $source, &Value );
//    $target = Value;
//  }
//  }
//%typemap(tcl8, out ) XdmfInt32, XdmfInt64, XdmfLength {
//  Tcl_Obj *Result;
//  long  Value;
//
//  Result = Tcl_GetObjResult(interp);
//  Value = $source;
//  Tcl_SetLongObj( Result, Value);
//  }
#endif /* SWIGTCL */

#ifdef SWIGCSHARP
%echo "Applying Special C# Typemaps .... "
%typemap(cscode) XdmfObject %{
  // Ensure that the GC doesn't collect any inserted XdmfElement instance set from C#
  public void setCMemOwn(bool val) {
    swigCMemOwn = val;
  }
%}
#endif

#ifdef SWIGPYTHON
%echo "Applying Python Typemaps ..... "
%include typemaps.i
%echo "No Special Python Typemaps Necessary"
/*
%typemap(python, in) XdmfInt32, XdmfInt64, XdmfLength {
  long  Value;

  Value = PyInt_AsLong( $source );
  $target = Value;
  }
%typemap(python, out ) XdmfInt32, XdmfInt64, XdmfLength {
  PyObject *Result;
  long  Value;

  Value = $source;
  Result = PyInt_FromLong( Value );
  $target = Result;
  }
%typemap(python, out ) XdmfString {
    PyObject *Result;

    if ($source == NULL) {
        Py_INCREF(Py_None);
        return Py_None;
    }
  Result = PyString_FromString( $source );
  $target = Result;
}
*/

#endif /* SWIGPYTHON */

#endif /* SWIG */

#ifndef SWIG
#ifndef MAX
#define MAX(a,b)  ((a) > (b) ? (a) : (b))
#endif /*  MAX */

#ifndef MIN
#define MIN(a,b)  ((a) < (b) ? (a) : (b))
#endif /* MIN */


/*! Used for Parsing */

#define XDMF_WORD_CMP( a, b )  ( (a) != NULL ) && ( STRCASECMP((a),(b)) == 0 )

#define XDMF_STRING_DUPLICATE( dest, src ) \
  if ( src ) { \
    dest = new char[strlen(src) + 1]; \
    strcpy(dest, src); \
  } else { \
    dest = 0; \
  }

#define XDMF_STRING_TRIM( str ) { \
  std::string::size_type spos = str.find_first_not_of(" \n\t"); \
  std::string::size_type epos = str.find_last_not_of(" \n\t"); \
  if ( spos == str.npos ) \
    { \
    str = ""; \
    } \
  else \
    { \
    if ( epos != str.npos ) \
      { \
      epos = epos - spos + 1; \
      } \
    str = str.substr(spos, epos); \
    } \
  }

#define XDMF_WORD_TRIM( a ) { \
  size_t          StringLength; \
  char            *fp; \
  char            *st; \
  char            *ed; \
   \
  fp = (a); \
  StringLength = strlen( ( a ) ); \
  while( ( *fp <= ' ' ) && ( StringLength > 0 ) ){ \
          fp++; \
          StringLength--; \
          } \
  ed = fp;\
  st = a;\
  while(*ed) {\
    *st = *ed;\
    st++;\
    ed++;\
    }\
  /*strcpy((a), fp );*/ \
  fp = &(a)[ StringLength - 1 ]; \
  while( ( *fp <= ' ' ) && ( StringLength > 0 ) ){ \
          fp--; \
          StringLength--; \
          } \
  fp++; \
  *fp = '\0'; \
  }
#ifndef  XDMF_WORD_CMP
#define XDMF_WORD_CMP( a, b )  ( (a) != NULL ) && ( STRCASECMP((a),(b)) == 0 )
#endif

#endif /* SWIG */

#ifdef __cplusplus

#define DebugIsOn ( this->Debug || XdmfObject::GetGlobalDebug() )

#define DebugIsAbove(a)  ( ( this->Debug >= (a) ) || ( XdmfObject::GetGlobalDebug() >= (a))) 

#define XdmfDebug(x) \
{ if ( this->Debug || XdmfObject::GetGlobalDebug() ) { \
  cerr << "XDMF Debug : " << __FILE__ << " line " << __LINE__ << " ("<< x << ")" << "\n"; \
  } \
}


#define XdmfErrorMessage(x) \
  cerr << "XDMF Error in " << __FILE__ << " line " << __LINE__ << " ("<< x << ")" << "\n";

#define XdmfSetValueMacro(var,type) \
XdmfInt32 Set##var (type _arg) \
  { \
  this->var = _arg; \
  return ( XDMF_SUCCESS ); \
  }

#define XdmfSetStringMacro(var) \
XdmfInt32 Set##var (XdmfConstString _arg) \
  { \
  if ( this->var == _arg ) { return XDMF_SUCCESS; } \
  if ( this->var && _arg && strcmp(this->var, _arg) == 0 ) { return XDMF_SUCCESS; } \
  if ( this->var ) { delete [] this->var; this->var = 0; } \
  if ( _arg ) { this->var = new char[ strlen(_arg) + 1 ]; strcpy(this->var, _arg); } \
  return ( XDMF_SUCCESS ); \
  }

#define XdmfGetStringMacro(var) \
XdmfConstString Get##var () \
  { \
  return ( this->var ); \
  }

#define XdmfSetIndexValueMacro(var,type) \
XdmfInt32 Set##var ( XdmfInt64 Index, type _arg) \
  { \
  this->var[ Index ]  = _arg; \
  return ( XDMF_SUCCESS ); \
  }


#define XdmfGetValueMacro(var,type) \
type Get##var () \
  { \
  return ( this->var ); \
  }

#define XdmfGetIndexValueMacro(var,type) \
type Get##var (XdmfInt64 Index) \
  { \
  return ( this->var[ Index ]  ); \
  }

namespace xdmf2
{

//! Base Class for All Xdmf Objects
class XDMF_EXPORT XdmfObject {
public:
  XdmfObject();
  ~XdmfObject();
  XdmfConstString GetClassName() { return("XdmfObject"); } ;

  XdmfSetValueMacro(Debug, XdmfBoolean);
  XdmfGetValueMacro(Debug, XdmfBoolean);

  XdmfBoolean GetGlobalDebug();
  void SetGlobalDebug( XdmfBoolean Value );

  void DebugOn() { XdmfObject::SetDebug( 1 ) ; };
  void DebugOff() { XdmfObject::SetDebug( 0 ) ; };

  void GlobalDebugOn() { XdmfObject::SetGlobalDebug( 1 ) ; };
  void GlobalDebugOff() { XdmfObject::SetGlobalDebug( 0 ) ; };

  XdmfConstString GetUniqueName(XdmfConstString NameBase=NULL);



protected:
  XdmfInt32 Debug;  

private:
};

XDMF_EXPORT void SetGlobalDebugOn();
XDMF_EXPORT void SetGlobalDebugOff();

XDMF_EXPORT XdmfInt32 GetGlobalDebug( void );
XDMF_EXPORT void SetGlobalDebug( XdmfInt32 DebugLevel );

XDMF_EXPORT XdmfString GetUnique( XdmfConstString Pattern = NULL );
extern XDMF_EXPORT XdmfString XdmfObjectToHandle( XdmfObject *Source );
extern XDMF_EXPORT XdmfObject *HandleToXdmfObject( XdmfConstString Source );
extern XDMF_EXPORT XdmfPointer VoidPointerHandleToXdmfPointer(XdmfConstString Source);

#ifndef SWIG
extern XDMF_EXPORT istrstream& XDMF_READ_STREAM64(istrstream& istr, XDMF_64_INT& i);
#endif

#endif /* __cplusplus */

}
#endif /* __XdmfObject_h */
