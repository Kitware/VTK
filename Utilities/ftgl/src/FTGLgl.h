#ifndef    __FTGLgl__
#define    __FTGLgl__

#include "FTGL.h"

#ifdef WIN32

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

// lifted from glext.h, to remove dependancy on glext.h
#ifndef GL_EXT_texture_object
    #define GL_TEXTURE_PRIORITY_EXT           0x8066
    #define GL_TEXTURE_RESIDENT_EXT           0x8067
    #define GL_TEXTURE_1D_BINDING_EXT         0x8068
    #define GL_TEXTURE_2D_BINDING_EXT         0x8069
    #define GL_TEXTURE_3D_BINDING_EXT         0x806A
#endif

#endif  //  __FTGLgl__
