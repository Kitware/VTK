# Check state of glx.h
#
# This file depends on the value of OPENGL_INCLUDE_DIR (/usr/include on Linux,
# /usr/X11R6/include on Mac, ) defined by module FindOpenGL.cmake
# (FIND_PACKAGE(OpenGL))
#
#
# 1. GLX_DEFINES_MACRO_GLX_VERSION_1_1: is macro GLX_VERSION_1_1 defined?
# 2. GLX_DEFINES_MACRO_GLX_VERSION_1_2: is macro GLX_VERSION_1_2 defined?
# 3. GLX_DEFINES_MACRO_GLX_VERSION_1_3: is macro GLX_VERSION_1_3 defined?
# 4. GLX_DEFINES_MACRO_GLX_VERSION_1_4: is macro GLX_VERSION_1_4 defined?
# 5. GLX_DEFINES_MACRO_GLX_ARB_get_proc_address: is macro
#                                         GLX_ARB_get_proc_address defined?
# 6. GLX_INCLUDES_GLXEXT: does glx.h include glxext.h?
# 7. GLX_USES_MACRO_GLX_GLXEXT_LEGACY: does glx.h look for macro
#                                  GLX_GLXEXT_LEGACY before including glxext.h?
# 8. GLX_DEFINES_TYPE_GLXextFuncPtr: is type GLXextFuncPtr defined?
# 9. GLX_DECLARES_FUNCTION_glXGetProcAddressARB: is function
#                                                glXGetProcAddressARB declared?
# 10. GLX_DECLARES_FUNCTION_glXGetProcAddress: is function glXGetProcAddress
#                                                                     declared?
# 11. GLX_DEFINES_TYPE_GLXextFuncPtr_AS_EMPTY: is type GLXextFuncPtr defined
#          with an empty parameter list (true), or with a void keyword (false)
# 12. GLX_DECLARES_FUNCTION_glXGetProcAddress_AS_EMPTY: is function
#      glXGetProcAddress declared with an empty parameter list (true), or with
#      a void keyword (false)
# 13. GLX_DEFINES_TYPE_GLXFBConfig: is type GLXFBConfig defined?
#
# Regarding test 11 and 12, here is a quick reminder for a difference between
# C and C++:
# C: f() function with any number of parameter
# C: f(void) function with no parameter
# C++: f()=f(void) function with no parameter
# -ansi -pedantic

# Why all this?
# Some glx.h are bogus:
# - HP: HP-UX defines GLX_VERSION_1_3 but not type GLXFBConfig
#       GLX_DEFINES_TYPE_GLXFBconfig
#       GLX_HAS_GLXFBConfig_bug : GLX_DEFINES_MACRO_GLX_VERSION_1_3 && !GLX_DEFINES_TYPE_GLXFBconfig
# - sometimes typedef void (*__GLXextFuncPtr)(void); does not exist, or is
#   defined with () inside of (void), or only exits in glxext.h
# - glXGetProcAddressARB should be
#    extern void (*glXGetProcAddressARB(const GLubyte *procName))(void);
# -  glXGetProcAddress should be
#     extern __GLXextFuncPtr glXGetProcAddress (const GLubyte *);
#    The GLX spec is actually wrong because it uses () which means any
#    number of parameters, not (void), which means no parameter.
#    SunOS follows the spec, Linux does not
#    extern void (*glXGetProcAddress(const GLubyte *procname))();
# - Mac Tiger: GLX_ARB_get_proc_address macro is defined but it does not define
#              __GLXextFuncPtr.
#
#
#message("vtkTestGLX.cmake first line.")

include(CheckCSourceCompiles) # defines CHECK_C_SOURCE_COMPILES()
include(CheckCXXSourceCompiles)

# cmake debugging
#unset(GLX_DEFINES_MACRO_GLX_VERSION_1_1 CACHE)
#unset(GLX_DEFINES_MACRO_GLX_VERSION_1_2 CACHE)
#unset(GLX_DEFINES_MACRO_GLX_VERSION_1_3 CACHE)
#unset(GLX_DEFINES_MACRO_GLX_VERSION_1_4 CACHE)
#unset(GLX_DEFINES_MACRO_GLX_ARB_get_proc_address CACHE)
#unset(GLX_INCLUDES_GLXEXT CACHE)
#unset(GLX_USES_MACRO_GLX_GLXEXT_LEGACY CACHE)
#unset(GLX_DEFINES_TYPE_GLXextFuncPtr CACHE)
#unset(GLX_DECLARES_FUNCTION_glXGetProcAddressARB CACHE)
#unset(GLX_DECLARES_FUNCTION_glXGetProcAddress CACHE)

# False: void, True: empty
#unset(GLX_DEFINES_TYPE_GLXextFuncPtr_AS_EMPTY CACHE)
#unset(GLX_DECLARES_FUNCTION_glXGetProcAddressARB_AS_EMPTY CACHE)
#unset(GLX_DECLARES_FUNCTION_glXGetProcAddress_AS_EMPTY CACHE)

#unset(GLX_DEFINES_TYPE_GLXFBConfig CACHE)

# for CHECK_C_SOURCE_COMPILES
set(SAVE_CMAKE_REQUIRED_INCLUDES "${CMAKE_REQUIRED_INCLUDES}")
set(SAVE_CMAKE_REQUIRED_LIBRARIES "${CMAKE_REQUIRED_LIBRARIES}")

set(CMAKE_REQUIRED_INCLUDES ${OPENGL_INCLUDE_DIR})

# We cannot just use OPENGL_gl_LIBRARY: SunOS requires to link
# with X11, which is available only in OPENGL_LIBRARIES.
# Also, SunOS requires to explicitly link -lm
set(CMAKE_REQUIRED_LIBRARIES
     ${OPENGL_LIBRARIES}
     ${CMAKE_REQUIRED_LIBRARIES}
     -lm)

if(DEFINED GLX_DEFINES_MACRO_GLX_VERSION_1_1)
 set(REPORT_STATUS 0)
else()
 set(REPORT_STATUS 1)
endif()

# -----------------------------------------------------------------------------
# GLX_DEFINES_MACRO_GLX_VERSION_1_1
# -----------------------------------------------------------------------------
CHECK_C_SOURCE_COMPILES(
"
#define GLX_GLXEXT_LEGACY
#include <GL/glx.h>
int main()
{
#ifdef GLX_VERSION_1_1
 return 0;
#else
 a; /* will raise a syntax error */
#endif
}
"
 GLX_DEFINES_MACRO_GLX_VERSION_1_1)

# -----------------------------------------------------------------------------
# GLX_DEFINES_MACRO_GLX_VERSION_1_2
# -----------------------------------------------------------------------------
CHECK_C_SOURCE_COMPILES(
"
#define GLX_GLXEXT_LEGACY
#include <GL/glx.h>
int main()
{
#ifdef GLX_VERSION_1_2
 return 0;
#else
 a; /* will raise a syntax error */
#endif
}
"
 GLX_DEFINES_MACRO_GLX_VERSION_1_2)

# -----------------------------------------------------------------------------
# GLX_DEFINES_MACRO_GLX_VERSION_1_3
# -----------------------------------------------------------------------------
CHECK_C_SOURCE_COMPILES(
"
#define GLX_GLXEXT_LEGACY
#include <GL/glx.h>
int main()
{
#ifdef GLX_VERSION_1_3
 return 0;
#else
 a; /* will raise a syntax error */
#endif
}
"
 GLX_DEFINES_MACRO_GLX_VERSION_1_3)

# -----------------------------------------------------------------------------
# GLX_DEFINES_MACRO_GLX_VERSION_1_4
# -----------------------------------------------------------------------------
CHECK_C_SOURCE_COMPILES(
"
#define GLX_GLXEXT_LEGACY
#include <GL/glx.h>
int main()
{
#ifdef GLX_VERSION_1_4
 return 0;
#else
 a; /* will raise a syntax error */
#endif
}
"
 GLX_DEFINES_MACRO_GLX_VERSION_1_4)

# -----------------------------------------------------------------------------
# GLX_DEFINES_MACRO_GLX_ARB_get_proc_address
# -----------------------------------------------------------------------------
CHECK_C_SOURCE_COMPILES(
"
#define GLX_GLXEXT_LEGACY
#include <GL/glx.h>
int main()
{
#ifdef GLX_ARB_get_proc_address
 return 0;
#else
 a; /* will raise a syntax error */
#endif
}
"
 GLX_DEFINES_MACRO_GLX_ARB_get_proc_address)

# -----------------------------------------------------------------------------
# GLX_INCLUDES_GLXEXT
# -----------------------------------------------------------------------------
CHECK_C_SOURCE_COMPILES(
"
#include <GL/glx.h>
int main()
{
#ifdef __glxext_h_
 return 0;
#else
 a; /* will raise a syntax error */
#endif
}
"
 GLX_INCLUDES_GLXEXT)

# -----------------------------------------------------------------------------
# GLX_USES_MACRO_GLX_GLXEXT_LEGACY
# -----------------------------------------------------------------------------
set(glx_h_full_path "${OPENGL_INCLUDE_DIR}/GL/glx.h")

unset(GLX_GLXEXT_LEGACY_OCCURENCE)
file(STRINGS ${glx_h_full_path} GLX_GLXEXT_LEGACY_OCCURENCE REGEX GLX_GLXEXT_LEGACY)
if(GLX_GLXEXT_LEGACY_OCCURENCE)
 set(GLX_USES_MACRO_GLX_GLXEXT_LEGACY TRUE CACHE INTERNAL "")
else(GLX_GLXEXT_LEGACY_OCCURENCE)
 set(GLX_USES_MACRO_GLX_GLXEXT_LEGACY FALSE CACHE INTERNAL "")
endif(GLX_GLXEXT_LEGACY_OCCURENCE)


# -----------------------------------------------------------------------------
# GLX_DEFINES_TYPE_GLXextFuncPtr
# -----------------------------------------------------------------------------
CHECK_C_SOURCE_COMPILES(
"
#define GLX_GLXEXT_LEGACY
#include <GL/glx.h>
int main()
{
 __GLXextFuncPtr f;
 return 0;
}
"
GLX_DEFINES_TYPE_GLXextFuncPtr)

# -----------------------------------------------------------------------------
# GLX_DEFINES_TYPE_GLXextFuncPtr_AS_EMPTY
# -----------------------------------------------------------------------------
CHECK_C_SOURCE_COMPILES(
"
#define GLX_GLXEXT_LEGACY
#include <GL/glx.h>
int main()
{
 __GLXextFuncPtr f;
 int x;
 f(x); /* fails if prototype is void, pass if prototype is empty. */

 return 0;
}
"
GLX_DEFINES_TYPE_GLXextFuncPtr_AS_EMPTY)

# -----------------------------------------------------------------------------
# GLX_DECLARES_FUNCTION_glXGetProcAddressARB
# -----------------------------------------------------------------------------
CHECK_C_SOURCE_COMPILES(
"
#define GLX_GLXEXT_LEGACY
#include <GL/glx.h>
int main()
{
 glXGetProcAddressARB((const GLubyte *)(\"aRandomFunction\"));
 return 0;
}
"
GLX_DECLARES_FUNCTION_glXGetProcAddressARB)


# -----------------------------------------------------------------------------
# GLX_DECLARES_FUNCTION_glXGetProcAddress
# -----------------------------------------------------------------------------
CHECK_C_SOURCE_COMPILES(
"
#define GLX_GLXEXT_LEGACY
#include <GL/glx.h>
int main()
{
 glXGetProcAddress((const GLubyte *)(\"aRandomFunction\"));
 return 0;
}
"
GLX_DECLARES_FUNCTION_glXGetProcAddress)

# -----------------------------------------------------------------------------
# GLX_DECLARES_FUNCTION_glXGetProcAddressARB_AS_EMPTY
# -----------------------------------------------------------------------------
CHECK_C_SOURCE_COMPILES(
"
#define GLX_GLXEXT_LEGACY
#include <GL/glx.h>
int main()
{
 int x;
 /* fails if prototype is void, pass if prototype is empty. */
 glXGetProcAddressARB((const GLubyte *)(\"aRandomFunction\"))(x);
 return 0;
}
"
GLX_DECLARES_FUNCTION_glXGetProcAddressARB_AS_EMPTY)

# -----------------------------------------------------------------------------
# GLX_DECLARES_FUNCTION_glXGetProcAddress_AS_EMPTY
# -----------------------------------------------------------------------------
CHECK_C_SOURCE_COMPILES(
"
#define GLX_GLXEXT_LEGACY
#include <GL/glx.h>
int main()
{
 int x;
 /* fails if prototype is void, pass if prototype is empty. */
 glXGetProcAddress((const GLubyte *)(\"aRandomFunction\"))(x);
 return 0;
}
"
GLX_DECLARES_FUNCTION_glXGetProcAddress_AS_EMPTY)

# -----------------------------------------------------------------------------
# GLX_DEFINES_TYPE_GLXFBConfig
# -----------------------------------------------------------------------------

CHECK_C_SOURCE_COMPILES(
"
#define GLX_GLXEXT_LEGACY
#include <GL/glx.h>
int main()
{
 GLXFBConfig c;
 return 0;
}
"
GLX_DEFINES_TYPE_GLXFBConfig)

set(CMAKE_REQUIRED_INCLUDES "${SAVE_CMAKE_REQUIRED_INCLUDES}")
set(CMAKE_REQUIRED_LIBRARIES "${SAVE_CMAKE_REQUIRED_LIBRARIES}")


# -----------------------------------------------------------------------------
# cmake macro debugging
if(${REPORT_STATUS})
 message(STATUS "GLX_DEFINES_MACRO_GLX_VERSION_1_1=${GLX_DEFINES_MACRO_GLX_VERSION_1_1}")
 message(STATUS "GLX_DEFINES_MACRO_GLX_VERSION_1_2=${GLX_DEFINES_MACRO_GLX_VERSION_1_2}")
 message(STATUS "GLX_DEFINES_MACRO_GLX_VERSION_1_3=${GLX_DEFINES_MACRO_GLX_VERSION_1_3}")
 message(STATUS "GLX_DEFINES_MACRO_GLX_VERSION_1_4=${GLX_DEFINES_MACRO_GLX_VERSION_1_4}")
 message(STATUS "GLX_DEFINES_MACRO_GLX_ARB_get_proc_address=${GLX_DEFINES_MACRO_GLX_ARB_get_proc_address}")
 message(STATUS "GLX_INCLUDES_GLXEXT=${GLX_INCLUDES_GLXEXT}")
 message(STATUS "GLX_USES_MACRO_GLX_GLXEXT_LEGACY=${GLX_USES_MACRO_GLX_GLXEXT_LEGACY}")
 message(STATUS "GLX_DEFINES_TYPE_GLXextFuncPtr=${GLX_DEFINES_TYPE_GLXextFuncPtr}")
 message(STATUS "GLX_DEFINES_TYPE_GLXextFuncPtr_AS_EMPTY=${GLX_DEFINES_TYPE_GLXextFuncPtr_AS_EMPTY}")
 message(STATUS "GLX_DECLARES_FUNCTION_glXGetProcAddressARB=${GLX_DECLARES_FUNCTION_glXGetProcAddressARB}")
 message(STATUS "GLX_DECLARES_FUNCTION_glXGetProcAddress=${GLX_DECLARES_FUNCTION_glXGetProcAddress}")
 message(STATUS "GLX_DECLARES_FUNCTION_glXGetProcAddressARB_AS_EMPTY=${GLX_DECLARES_FUNCTION_glXGetProcAddressARB_AS_EMPTY}")
 message(STATUS "GLX_DECLARES_FUNCTION_glXGetProcAddress_AS_EMPTY=${GLX_DECLARES_FUNCTION_glXGetProcAddress_AS_EMPTY}")
 message(STATUS "GLX_DEFINES_TYPE_GLXFBConfig=${GLX_DEFINES_TYPE_GLXFBConfig}")
endif()
#message("vtkTestGLX.cmake last line.")
