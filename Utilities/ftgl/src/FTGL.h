#ifndef    __FTGL__
#define    __FTGL__

// To include debug memory manager by Paul Nettle (midnight@FluidStudios.com)
// http://www.FluidStudios.com/publications.html
// Get this code and use it. It will open your eyes:)
// #define FTGL_DEBUG

typedef double   FTGL_DOUBLE;
typedef float    FTGL_FLOAT;

#ifdef WIN32

    // Under windows avoid including <windows.h> is overrated. 
  // Sure, it can be avoided and "name space pollution" can be
  // avoided, but why? It really doesn't make that much difference
  // these days.
    #define  WIN32_LEAN_AND_MEAN
    #include <windows.h>

    #ifndef __gl_h_
        #include <GL/gl.h>
        #ifndef FTGL_DO_NOT_USE_VECTORISER
          #include <GL/glu.h>
        #endif
    #endif

#else

    // Non windows platforms - don't require nonsense as seen above :-)    
    #ifndef __gl_h_
        #ifdef __APPLE_CC__
            #include <OpenGL/gl.h>
            #ifndef FTGL_DO_NOT_USE_VECTORISER
              #include <OpenGL/glu.h>
            #endif
        #else
          #include <GL/gl.h>
          #ifndef FTGL_DO_NOT_USE_VECTORISER
            #include <GL/glu.h>
          #endif
        #endif                

    #endif

    // Required for compatibility with glext.h style function definitions of 
    // OpenGL extensions, such as in src/osg/Point.cpp.
    #ifndef APIENTRY
        #define APIENTRY
    #endif
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
    #ifdef FTGL_LIBRARY        // dynamic lib - must export/import symbols appropriately.
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


// lifted from glext.h, to remove dependancy on glext.h
#ifndef GL_EXT_texture_object
    #define GL_TEXTURE_PRIORITY_EXT           0x8066
    #define GL_TEXTURE_RESIDENT_EXT           0x8067
    #define GL_TEXTURE_1D_BINDING_EXT         0x8068
    #define GL_TEXTURE_2D_BINDING_EXT         0x8069
    #define GL_TEXTURE_3D_BINDING_EXT         0x806A
#endif

#endif  //  __FTGL__
