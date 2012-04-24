#ifndef    __FTGL__
#define    __FTGL__

//#if defined(VTKFTGL)
#include "vtkftglConfig.h"
//#endif

#define FTGL_USE_NAMESPACE

// To include debug memory manager by Paul Nettle (midnight@FluidStudios.com)
// http://www.FluidStudios.com/publications.html
// Get this code and use it. It will open your eyes:)
// #define FTGL_DEBUG

typedef double   FTGL_DOUBLE;
typedef float    FTGL_FLOAT;

typedef void (*FTCallback)();

struct FTGLRenderContext
{
#ifdef FTGL_SUPPORT_MANGLE_MESA
  int UseMangleMesa;
#endif
};

#ifdef WIN32

    // Under windows avoid including <windows.h> is overrated.
  // Sure, it can be avoided and "name space pollution" can be
  // avoided, but why? It really doesn't make that much difference
  // these days.
    #define  WIN32_LEAN_AND_MEAN
    #include <windows.h>

#endif

// Compiler-specific conditional compilation
#ifdef _MSC_VER // MS Visual C++

  // Disable various warning.
  // 4786: template name too long
  #pragma warning( disable : 4251 )
  #pragma warning( disable : 4275 )
  #pragma warning( disable : 4786 )

  // Disable the usual'function ... not inlined' triggered by /W4
  #pragma warning( disable : 4710 )

#if ( _MSC_VER >= 1300 ) // Visual studio .NET

#pragma warning( disable : 4244 ) // conversion [...] possible loss of data
#pragma warning( disable : 4267 ) // same
#pragma warning( disable : 4311 ) // same for pointer
#pragma warning( disable : 4312 ) // same for pointer

#endif /* _MSC_VER */

#endif


#if defined(__BORLANDC__)
#pragma warn -8027 /* "functions containing for are not expanded inline" */
#endif


#ifdef WIN32

  // The following definitions control how symbols are exported.
  // If the target is a static library ensure that FTGL_LIBRARY_STATIC
  // is defined. If building a dynamic library (ie DLL) ensure the
  // FTGL_LIBRARY macro is defined, as it will mark symbols for
  // export. If compiling a project to _use_ the _dynamic_ library
  // version of the library, no definition is required.
  #ifdef FTGL_LIBRARY_STATIC    // static lib - no special export required
  #  define FTGL_EXPORT
  #else
    #ifdef vtkftgl_EXPORTS        // dynamic lib - must export/import symbols appropriately.
  #  define FTGL_EXPORT   __declspec(dllexport)
  #else
  #  define FTGL_EXPORT   __declspec(dllimport)
  #endif
  #endif

#else
  // Compiler that is not MS Visual C++.
  // Ensure that the export symbol is defined (and blank)
  #define FTGL_EXPORT
#endif

#endif  //  __FTGL__
