/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLState.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtk_glew.h"
#include "vtkOpenGLState.h"

#include "vtkOpenGLRenderWindow.h"
#include "vtkRenderer.h"

// must be included after a vtkObject subclass
#include "vtkOpenGLError.h"

// If you define NO_CACHE then all state->vtkgl* calls
// will get passed down to OpenGL regardless of the current
// state. This basically bypasses the caching mechanism
// and is useful for tesing
#define NO_CACHE 1

//////////////////////////////////////////////////////////////////////////////
//
// if VTK_REPORT_OPENGL_ERRORS is defined (done in CMake and vtkOpenGLError.h)
// then a few things change here.
//
// 1) the error code of OpenGL will be checked after every call.
// 2) On an error the full stack trace available will be
//    printed.
// 3) All methods check the cache state to see if anything is out of sync
//
#ifdef VTK_REPORT_OPENGL_ERRORS
#include "vtksys/SystemInformation.hxx"

// on msvc add in stack trace info as systeminformation
// does not seem to include it.
//
#if defined(VTK_COMPILER_MSVC) && defined(WIN32)
#define TRACE_MAX_STACK_FRAMES 1024
#define TRACE_MAX_FUNCTION_NAME_LENGTH 1024
#include "dbghelp.h"
std::string getProgramStack()
{
  void *stack[TRACE_MAX_STACK_FRAMES];
  HANDLE process = GetCurrentProcess();
  SymInitialize(process, NULL, TRUE);
  WORD numberOfFrames = CaptureStackBackTrace(0, TRACE_MAX_STACK_FRAMES, stack, NULL);
  SYMBOL_INFO *symbol = (SYMBOL_INFO *)malloc(sizeof(SYMBOL_INFO)+(TRACE_MAX_FUNCTION_NAME_LENGTH - 1) * sizeof(TCHAR));
  symbol->MaxNameLen = TRACE_MAX_FUNCTION_NAME_LENGTH;
  symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
  DWORD displacement;
  IMAGEHLP_LINE64 *line = (IMAGEHLP_LINE64 *)malloc(sizeof(IMAGEHLP_LINE64));
  line->SizeOfStruct = sizeof(IMAGEHLP_LINE64);
  std::ostringstream oss;
  for (int i = 1; i < numberOfFrames - 2; i++)
  {
    DWORD64 address = (DWORD64)(stack[i]);
    SymFromAddr(process, address, NULL, symbol);
    if (SymGetLineFromAddr64(process, address, &displacement, line))
    {
      oss << " at " << symbol->Name << " in " <<  line->FileName << " line " << line->LineNumber << "\n";
    }
    else
    {
      oss << "\tSymGetLineFromAddr64 returned error code " << GetLastError() << "\n";
      oss << "\tat " << symbol->Name << "\n";
    }
  }
  return oss.str();
}

#else
#define getProgramStack()  vtksys::SystemInformation::GetProgramStack(0, 0)
#endif

// this method checks all the cached state to make sure
// nothing is out of sync. It can be slow.
void vtkOpenGLState::CheckState()
{
  bool error = false;

  GLboolean params[4];

  ::glGetBooleanv(GL_DEPTH_WRITEMASK, params);
  if (params[0] != this->CurrentState.DepthMask)
  {
    vtkGenericWarningMacro("Error in cache state for GL_DEPTH_WRITEMASK");
    error = true;
  }
  ::glGetBooleanv(GL_COLOR_WRITEMASK, params);
  if (
      params[0] != this->CurrentState.ColorMask[0] ||
      params[1] != this->CurrentState.ColorMask[1] ||
      params[2] != this->CurrentState.ColorMask[2] ||
      params[3] != this->CurrentState.ColorMask[3]
      )
  {
    vtkGenericWarningMacro("Error in cache state for GL_COLOR_WRITEMASK");
    error = true;
  }
  ::glGetBooleanv(GL_BLEND, params);
  if ((params[0] != 0) != this->CurrentState.Blend)
  {
    vtkGenericWarningMacro("Error in cache state for GL_BLEND");
    error = true;
  }
  ::glGetBooleanv(GL_DEPTH_TEST, params);
  if ((params[0] != 0) != this->CurrentState.DepthTest)
  {
    vtkGenericWarningMacro("Error in cache state for GL_DEPTH_TEST");
    error = true;
  }
  ::glGetBooleanv(GL_CULL_FACE, params);
  if ((params[0] != 0) != this->CurrentState.CullFace)
  {
    vtkGenericWarningMacro("Error in cache state for GL_CULL_FACE");
    error = true;
  }
#ifdef GL_MULTISAMPLE
  ::glGetBooleanv(GL_MULTISAMPLE, params);
  if ((params[0] != 0) != this->CurrentState.MultiSample)
  {
    vtkGenericWarningMacro("Error in cache state for GL_MULTISAMPLE");
    error = true;
  }
#endif
  ::glGetBooleanv(GL_SCISSOR_TEST, params);
  if ((params[0] != 0) != this->CurrentState.ScissorTest)
  {
    vtkGenericWarningMacro("Error in cache state for GL_SCISSOR_TEST");
    error = true;
  }
  ::glGetBooleanv(GL_STENCIL_TEST, params);
  if ((params[0] != 0) != this->CurrentState.StencilTest)
  {
    vtkGenericWarningMacro("Error in cache state for GL_STENCIL_TEST");
    error = true;
  }

  GLint iparams[4];
  ::glGetIntegerv(GL_VIEWPORT, iparams);
  if (
      iparams[0] != this->CurrentState.Viewport[0] ||
      iparams[1] != this->CurrentState.Viewport[1] ||
      iparams[2] != this->CurrentState.Viewport[2] ||
      iparams[3] != this->CurrentState.Viewport[3]
      )
  {
    vtkGenericWarningMacro("Error in cache state for GL_VIEWPORT");
    error = true;
  }
  ::glGetIntegerv(GL_SCISSOR_BOX, iparams);
  if (
      iparams[0] != this->CurrentState.Scissor[0] ||
      iparams[1] != this->CurrentState.Scissor[1] ||
      iparams[2] != this->CurrentState.Scissor[2] ||
      iparams[3] != this->CurrentState.Scissor[3]
      )
  {
    vtkGenericWarningMacro("Error in cache state for GL_SCISSOR_BOX");
    error = true;
  }
  ::glGetIntegerv(GL_CULL_FACE_MODE, iparams);
  if (iparams[0] != static_cast<int>(this->CurrentState.CullFaceMode))
  {
    vtkGenericWarningMacro("Error in cache state for GL_CULL_FACE_MODE");
    error = true;
  }
  ::glGetIntegerv(GL_DEPTH_FUNC, iparams);
  if (iparams[0] != static_cast<int>(this->CurrentState.DepthFunc))
  {
    vtkGenericWarningMacro("Error in cache state for GL_DEPTH_FUNC");
    error = true;
  }
  ::glGetIntegerv(GL_BLEND_SRC_RGB, iparams);
  if (iparams[0] != static_cast<int>(this->CurrentState.BlendFunc[0]))
  {
    vtkGenericWarningMacro("Error in cache state for GL_BLEND_SRC_RGB");
    error = true;
  }
  ::glGetIntegerv(GL_BLEND_SRC_ALPHA, iparams);
  if (iparams[0] != static_cast<int>(this->CurrentState.BlendFunc[2]))
  {
    vtkGenericWarningMacro("Error in cache state for GL_BLEND_SRC_ALPHA");
    error = true;
  }
  ::glGetIntegerv(GL_BLEND_DST_RGB, iparams);
  if (iparams[0] != static_cast<int>(this->CurrentState.BlendFunc[1]))
  {
    vtkGenericWarningMacro("Error in cache state for GL_BLEND_DST_RGB");
    error = true;
  }
  ::glGetIntegerv(GL_BLEND_DST_ALPHA, iparams);
  if (iparams[0] != static_cast<int>(this->CurrentState.BlendFunc[3]))
  {
    vtkGenericWarningMacro("Error in cache state for GL_BLEND_DST_ALPHA");
    error = true;
  }

  GLfloat fparams[4];
  ::glGetFloatv(GL_COLOR_CLEAR_VALUE, fparams);
  if (
      fparams[0] != this->CurrentState.ClearColor[0] ||
      fparams[1] != this->CurrentState.ClearColor[1] ||
      fparams[2] != this->CurrentState.ClearColor[2] ||
      fparams[3] != this->CurrentState.ClearColor[3]
      )
  {
    vtkGenericWarningMacro("Error in cache state for GL_COLOR_CLEAR_VALUE");
    error = true;
  }

  if (error)
  {
    std::string msg = getProgramStack();
    vtkGenericWarningMacro("at stack loc\n" << msg);
  }
}

namespace {
bool reportOpenGLErrors(std::string &result)
{
  const int maxErrors = 16;
  unsigned int errCode[maxErrors] = {0};
  const char *errDesc[maxErrors] = {NULL};

  int numErrors
    = vtkGetOpenGLErrors(
        maxErrors,
        errCode,
        errDesc);

  if (numErrors)
  {
    std::ostringstream oss;
    vtkPrintOpenGLErrors(
          oss,
          maxErrors,
          numErrors,
          errCode,
          errDesc);

    oss << "\n with stack trace of\n" << getProgramStack();
    result = oss.str();
    return true;
  }
  return false;
}


} // anon namespace

#define vtkOpenGLCheckStateMacro() this->CheckState()

#define vtkCheckOpenGLErrorsWithStack(message) \
{ \
  std::string _tmp; \
  if (reportOpenGLErrors(_tmp)) \
  { \
    vtkGenericWarningMacro("Error " << message << _tmp); \
    vtkOpenGLClearErrorMacro(); \
  } \
}

#else // VTK_REPORT_OPENGL_ERRORS

#define vtkCheckOpenGLErrorsWithStack(message)
#define vtkOpenGLCheckStateMacro()

#endif // VTK_REPORT_OPENGL_ERRORS

//
//////////////////////////////////////////////////////////////////////////////

vtkOpenGLState::ScopedglDepthMask::ScopedglDepthMask(vtkOpenGLState *s)
{
  this->State = s;
  this->Value = this->State->CurrentState.DepthMask;
  this->Method = &vtkOpenGLState::vtkglDepthMask;
}

vtkOpenGLState::ScopedglColorMask::ScopedglColorMask(vtkOpenGLState *s)
{
  this->State = s;
  this->Value = this->State->CurrentState.ColorMask;
  this->Method = &vtkOpenGLState::ColorMask;
}

vtkOpenGLState::ScopedglClearColor::ScopedglClearColor(vtkOpenGLState *s)
{
  this->State = s;
  this->Value = this->State->CurrentState.ClearColor;
  this->Method = &vtkOpenGLState::ClearColor;
}

vtkOpenGLState::ScopedglScissor::ScopedglScissor(vtkOpenGLState *s)
{
  this->State = s;
  this->Value = this->State->CurrentState.Scissor;
  this->Method = &vtkOpenGLState::Scissor;
}

vtkOpenGLState::ScopedglViewport::ScopedglViewport(vtkOpenGLState *s)
{
  this->State = s;
  this->Value = this->State->CurrentState.Viewport;
  this->Method = &vtkOpenGLState::Viewport;
}

vtkOpenGLState::ScopedglBlendFuncSeparate::ScopedglBlendFuncSeparate(vtkOpenGLState *s)
{
  this->State = s;
  this->Value = this->State->CurrentState.BlendFunc;
  this->Method = &vtkOpenGLState::BlendFuncSeparate;
}

void vtkOpenGLState::ColorMask(std::array<GLboolean, 4> val)
{
  this->vtkglColorMask(val[0], val[1], val[2], val[3]);
}

void vtkOpenGLState::vtkglColorMask(
  GLboolean r, GLboolean g, GLboolean b, GLboolean a
  )
{
  vtkOpenGLCheckStateMacro();

#ifndef NO_CACHE
  if (this->CurrentState.ColorMask[0] != r ||
      this->CurrentState.ColorMask[1] != g ||
      this->CurrentState.ColorMask[2] != b ||
      this->CurrentState.ColorMask[3] != a)
#endif
  {
    this->CurrentState.ColorMask[0] = r;
    this->CurrentState.ColorMask[1] = g;
    this->CurrentState.ColorMask[2] = b;
    this->CurrentState.ColorMask[3] = a;
    ::glColorMask(r,g,b,a);
  }

  vtkCheckOpenGLErrorsWithStack("glColorMask");
}

void vtkOpenGLState::ClearColor(std::array<GLclampf, 4> val)
{
  this->vtkglClearColor(val[0], val[1], val[2], val[3]);
}

void vtkOpenGLState::vtkglClearColor(
  GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha
  )
{
  vtkOpenGLCheckStateMacro();

#ifndef NO_CACHE
  if (this->CurrentState.ClearColor[0] != red ||
      this->CurrentState.ClearColor[1] != green ||
      this->CurrentState.ClearColor[2] != blue ||
      this->CurrentState.ClearColor[3] != alpha)
#endif
  {
    this->CurrentState.ClearColor[0] = red;
    this->CurrentState.ClearColor[1] = green;
    this->CurrentState.ClearColor[2] = blue;
    this->CurrentState.ClearColor[3] = alpha;
    ::glClearColor(red,green,blue,alpha);
  }

  vtkCheckOpenGLErrorsWithStack("glClearColor");
}

void vtkOpenGLState::vtkglClearDepth(double val)
{
  vtkOpenGLCheckStateMacro();

#ifndef NO_CACHE
  if (this->CurrentState.ClearDepth != val)
#endif
  {
    this->CurrentState.ClearDepth = val;
#if GL_ES_VERSION_3_0 == 1
    ::glClearDepthf(static_cast<GLclampf>(val));
#else
    ::glClearDepth(val);
#endif
  }
  vtkCheckOpenGLErrorsWithStack("glClearDepth");
}

void vtkOpenGLState::vtkglDepthFunc(GLenum val)
{
#ifndef NO_CACHE
  if (this->CurrentState.DepthFunc != val)
#endif
  {
    this->CurrentState.DepthFunc = val;
    ::glDepthFunc(val);
  }
  vtkCheckOpenGLErrorsWithStack("glDepthFunc");
}

void vtkOpenGLState::vtkglDepthMask(GLboolean val)
{
  vtkOpenGLCheckStateMacro();

#ifndef NO_CACHE
  if (this->CurrentState.DepthMask != val)
#endif
  {
    this->CurrentState.DepthMask = val;
    ::glDepthMask(val);
  }
  vtkCheckOpenGLErrorsWithStack("glDepthMask");
}

void vtkOpenGLState::BlendFuncSeparate(std::array<GLenum, 4> val)
{
  this->vtkglBlendFuncSeparate(val[0], val[1], val[2], val[3]);
}

void vtkOpenGLState::vtkglBlendFuncSeparate(
  GLenum val1, GLenum val2,
  GLenum val3, GLenum val4
  )
{
  vtkOpenGLCheckStateMacro();

#ifndef NO_CACHE
  if (this->CurrentState.BlendFunc[0] != val1 ||
      this->CurrentState.BlendFunc[1] != val2 ||
      this->CurrentState.BlendFunc[2] != val3 ||
      this->CurrentState.BlendFunc[3] != val4)
#endif
  {
    this->CurrentState.BlendFunc[0] = val1;
    this->CurrentState.BlendFunc[1] = val2;
    this->CurrentState.BlendFunc[2] = val3;
    this->CurrentState.BlendFunc[3] = val4;
    ::glBlendFuncSeparate(val1, val2, val3, val4);
  }
  vtkCheckOpenGLErrorsWithStack("glBlendFuncSeparate");
}

void vtkOpenGLState::vtkglBlendEquation(GLenum val)
{
  this->vtkglBlendEquationSeparate(val,val);
}

void vtkOpenGLState::vtkglBlendEquationSeparate(GLenum val, GLenum val2)
{
  vtkOpenGLCheckStateMacro();

#ifndef NO_CACHE
  if (this->CurrentState.BlendEquationValue1 != val ||
      this->CurrentState.BlendEquationValue2 != val2)
#endif
  {
    this->CurrentState.BlendEquationValue1 =  val;
    this->CurrentState.BlendEquationValue2 =  val2;
    ::glBlendEquationSeparate(val, val2);
  }

  vtkCheckOpenGLErrorsWithStack("glBlendEquationSeparate");
}

void vtkOpenGLState::vtkglCullFace(GLenum val)
{
  vtkOpenGLCheckStateMacro();

#ifndef NO_CACHE
  if (this->CurrentState.CullFaceMode != val)
#endif
  {
    this->CurrentState.CullFaceMode =  val;
    ::glCullFace(val);
  }
  vtkCheckOpenGLErrorsWithStack("glCullFace");
}

void vtkOpenGLState::Viewport(std::array<GLint, 4> val)
{
  this->vtkglViewport(val[0], val[1], val[2], val[3]);
}

void vtkOpenGLState::vtkglViewport(GLint x, GLint y, GLsizei width, GLsizei height)
{
  vtkOpenGLCheckStateMacro();

#ifndef NO_CACHE
  if (this->CurrentState.Viewport[0] != x ||
      this->CurrentState.Viewport[1] != y ||
      this->CurrentState.Viewport[2] != width ||
      this->CurrentState.Viewport[3] != height)
#endif
  {
    this->CurrentState.Viewport[0] = x;
    this->CurrentState.Viewport[1] = y;
    this->CurrentState.Viewport[2] = width;
    this->CurrentState.Viewport[3] = height;
    ::glViewport(x,y,width,height);
  }

  vtkCheckOpenGLErrorsWithStack("glViewport");
}

void vtkOpenGLState::Scissor(std::array<GLint, 4> val)
{
  this->vtkglScissor(val[0], val[1], val[2], val[3]);
}

void vtkOpenGLState::vtkglScissor(GLint x, GLint y, GLsizei width, GLsizei height)
{
  vtkOpenGLCheckStateMacro();

#ifndef NO_CACHE
  if (this->CurrentState.Scissor[0] != x ||
      this->CurrentState.Scissor[1] != y ||
      this->CurrentState.Scissor[2] != width ||
      this->CurrentState.Scissor[3] != height)
#endif
  {
    this->CurrentState.Scissor[0] = x;
    this->CurrentState.Scissor[1] = y;
    this->CurrentState.Scissor[2] = width;
    this->CurrentState.Scissor[3] = height;
    ::glScissor(x,y,width,height);
  }
  vtkCheckOpenGLErrorsWithStack("glScissor");
}

void vtkOpenGLState::SetEnumState(GLenum cap, bool val)
{
  vtkOpenGLCheckStateMacro();

#ifndef NO_CACHE
  bool changed = false;
#else
  bool changed = true;
#endif
  switch (cap)
  {
    case GL_BLEND:
      if (this->CurrentState.Blend != val)
      {
        this->CurrentState.Blend = val;
        changed = true;
      }
      break;
    case GL_DEPTH_TEST:
      if (this->CurrentState.DepthTest != val)
      {
        this->CurrentState.DepthTest = val;
        changed = true;
      }
      break;
    case GL_CULL_FACE:
      if (this->CurrentState.CullFace != val)
      {
        this->CurrentState.CullFace = val;
        changed = true;
      }
      break;
#ifdef GL_MULTISAMPLE
    case GL_MULTISAMPLE:
      if (this->CurrentState.MultiSample != val)
      {
        this->CurrentState.MultiSample = val;
        changed = true;
      }
      break;
#endif
    case GL_SCISSOR_TEST:
      if (this->CurrentState.ScissorTest != val)
      {
        this->CurrentState.ScissorTest = val;
        changed = true;
      }
      break;
    case GL_STENCIL_TEST:
      if (this->CurrentState.StencilTest != val)
      {
        this->CurrentState.StencilTest = val;
        changed = true;
      }
      break;
    default:
      changed = true;
    }

  if (!changed)
  {
    return;
  }

  if (val)
  {
    ::glEnable(cap);
  }
  else
  {
    ::glDisable(cap);
  }
  vtkCheckOpenGLErrorsWithStack("glEnable/Disable");
}

void vtkOpenGLState::ResetEnumState(GLenum cap)
{
  GLboolean params;
  ::glGetBooleanv(cap, &params);

  switch (cap)
  {
    case GL_BLEND:
      this->CurrentState.Blend = params != 0;
      break;
    case GL_DEPTH_TEST:
      this->CurrentState.DepthTest = params != 0;
      break;
    case GL_CULL_FACE:
      this->CurrentState.CullFace = params != 0;
      break;
#ifdef GL_MULTISAMPLE
    case GL_MULTISAMPLE:
      this->CurrentState.MultiSample = params != 0;
      break;
#endif
    case GL_SCISSOR_TEST:
      this->CurrentState.ScissorTest = params != 0;
      break;
    case GL_STENCIL_TEST:
      this->CurrentState.StencilTest = params != 0;
      break;
    default:
      break;
    }
}

void vtkOpenGLState::vtkglEnable(GLenum cap)
{
  this->SetEnumState(cap, true);
}

// return cached value if we have it
// otherwise forward to opengl
void vtkOpenGLState::vtkglGetBooleanv(GLenum pname, GLboolean *params)
{
  vtkOpenGLCheckStateMacro();

  switch (pname)
  {
    case GL_DEPTH_WRITEMASK:
      *params = this->CurrentState.DepthMask;
      break;
    case GL_COLOR_WRITEMASK:
      params[0] = this->CurrentState.ColorMask[0];
      params[1] = this->CurrentState.ColorMask[1];
      params[2] = this->CurrentState.ColorMask[2];
      params[3] = this->CurrentState.ColorMask[3];
      break;
    case GL_BLEND:
      *params = this->CurrentState.Blend;
      break;
    case GL_DEPTH_TEST:
      *params = this->CurrentState.DepthTest;
      break;
    case GL_CULL_FACE:
      *params = this->CurrentState.CullFace;
      break;
#ifdef GL_MULTISAMPLE
    case GL_MULTISAMPLE:
      *params = this->CurrentState.MultiSample;
      break;
#endif
    case GL_SCISSOR_TEST:
      *params = this->CurrentState.ScissorTest;
      break;
    case GL_STENCIL_TEST:
      *params = this->CurrentState.StencilTest;
      break;
    default:
      ::glGetBooleanv(pname, params);
  }
  vtkCheckOpenGLErrorsWithStack("glGetBoolean");
}

void vtkOpenGLState::vtkglGetIntegerv(GLenum pname, GLint *params)
{
  vtkOpenGLCheckStateMacro();

  switch (pname)
  {
    case GL_VIEWPORT:
      params[0] = this->CurrentState.Viewport[0];
      params[1] = this->CurrentState.Viewport[1];
      params[2] = this->CurrentState.Viewport[2];
      params[3] = this->CurrentState.Viewport[3];
      break;
    case GL_SCISSOR_BOX:
      params[0] = this->CurrentState.Scissor[0];
      params[1] = this->CurrentState.Scissor[1];
      params[2] = this->CurrentState.Scissor[2];
      params[3] = this->CurrentState.Scissor[3];
      break;
    case GL_CULL_FACE_MODE:
      *params = this->CurrentState.CullFaceMode;
      break;
    case GL_DEPTH_FUNC:
      *params = this->CurrentState.DepthFunc;
      break;
    case GL_BLEND_SRC_RGB:
      *params = this->CurrentState.BlendFunc[0];
      break;
    case GL_BLEND_SRC_ALPHA:
      *params = this->CurrentState.BlendFunc[2];
      break;
    case GL_BLEND_DST_RGB:
      *params = this->CurrentState.BlendFunc[1];
      break;
    case GL_BLEND_DST_ALPHA:
      *params = this->CurrentState.BlendFunc[3];
      break;
    case GL_MAX_TEXTURE_SIZE:
      *params = this->CurrentState.MaxTextureSize;
      break;
    case GL_MAJOR_VERSION:
      *params = this->CurrentState.MajorVersion;
      break;
    case GL_MINOR_VERSION:
      *params = this->CurrentState.MinorVersion;
      break;
    default:
      ::glGetIntegerv(pname, params);
  }

  vtkCheckOpenGLErrorsWithStack("glGetInteger");
}

#if GL_ES_VERSION_3_0 == 1
void vtkOpenGLState::vtkglGetDoublev(GLenum pname, double *)
{
  vtkGenericWarningMacro("glGetDouble not supported on OpenGL ES, requested: " << pname);
}
#else
void vtkOpenGLState::vtkglGetDoublev(GLenum pname, double *params)
{
  vtkOpenGLCheckStateMacro();
  ::glGetDoublev(pname, params);
  vtkCheckOpenGLErrorsWithStack("glGetDouble");
}
#endif

void vtkOpenGLState::vtkglGetFloatv(GLenum pname, GLfloat *params)
{
  vtkOpenGLCheckStateMacro();

  switch (pname)
  {
    case GL_COLOR_CLEAR_VALUE:
      params[0] = this->CurrentState.ClearColor[0];
      params[1] = this->CurrentState.ClearColor[1];
      params[2] = this->CurrentState.ClearColor[2];
      params[3] = this->CurrentState.ClearColor[3];
      break;
    default:
      ::glGetFloatv(pname, params);
  }
  vtkCheckOpenGLErrorsWithStack("glGetFloat");
}

void vtkOpenGLState::GetBlendFuncState(int *v)
{
  v[0] = this->CurrentState.BlendFunc[0];
  v[1] = this->CurrentState.BlendFunc[1];
  v[2] = this->CurrentState.BlendFunc[2];
  v[3] = this->CurrentState.BlendFunc[3];
}

bool vtkOpenGLState::GetEnumState(GLenum cap)
{
  vtkOpenGLCheckStateMacro();

  switch (cap)
  {
    case GL_BLEND:
      return this->CurrentState.Blend;
    case GL_DEPTH_TEST:
      return this->CurrentState.DepthTest;
    case GL_CULL_FACE:
      return this->CurrentState.CullFace;
#ifdef GL_MULTISAMPLE
    case GL_MULTISAMPLE:
      return this->CurrentState.MultiSample;
#endif
    case GL_SCISSOR_TEST:
      return this->CurrentState.ScissorTest;
    case GL_STENCIL_TEST:
      return this->CurrentState.StencilTest;
    default:
      vtkGenericWarningMacro("Bad request for enum status");
  }
  return false;
}

void vtkOpenGLState::vtkglDisable(GLenum cap)
{
  this->SetEnumState(cap, false);
}

// make the hardware openglstate match the
// state ivars
void vtkOpenGLState::Initialize(vtkOpenGLRenderWindow *)
{
  this->CurrentState.Blend
    ? ::glEnable(GL_BLEND) : ::glDisable(GL_BLEND);
  this->CurrentState.DepthTest
    ? ::glEnable(GL_DEPTH_TEST) : ::glDisable(GL_DEPTH_TEST);
  this->CurrentState.StencilTest
    ? ::glEnable(GL_STENCIL_TEST) : ::glDisable(GL_STENCIL_TEST);
  this->CurrentState.ScissorTest
    ? ::glEnable(GL_SCISSOR_TEST) : ::glDisable(GL_SCISSOR_TEST);
  this->CurrentState.CullFace
    ? ::glEnable(GL_CULL_FACE) : ::glDisable(GL_CULL_FACE);

#ifdef GL_MULTISAMPLE
  this->CurrentState.MultiSample
    ?  glEnable(GL_MULTISAMPLE) : glDisable(GL_MULTISAMPLE);
#endif

  // initialize blending for transparency
  ::glBlendFuncSeparate(
    this->CurrentState.BlendFunc[0],
    this->CurrentState.BlendFunc[1],
    this->CurrentState.BlendFunc[2],
    this->CurrentState.BlendFunc[3]);

  ::glClearColor(
    this->CurrentState.ClearColor[0],
    this->CurrentState.ClearColor[1],
    this->CurrentState.ClearColor[2],
    this->CurrentState.ClearColor[3]);

  ::glColorMask(
    this->CurrentState.ColorMask[0],
    this->CurrentState.ColorMask[1],
    this->CurrentState.ColorMask[2],
    this->CurrentState.ColorMask[3]);

  ::glDepthFunc( this->CurrentState.DepthFunc );

#if GL_ES_VERSION_3_0 == 1
  ::glClearDepthf(this->CurrentState.ClearDepth);
#else
  ::glClearDepth(this->CurrentState.ClearDepth);
#endif

  ::glDepthMask(this->CurrentState.DepthMask);

  ::glViewport(
    this->CurrentState.Viewport[0],
    this->CurrentState.Viewport[1],
    this->CurrentState.Viewport[2],
    this->CurrentState.Viewport[3]);

  ::glScissor(
    this->CurrentState.Scissor[0],
    this->CurrentState.Scissor[1],
    this->CurrentState.Scissor[2],
    this->CurrentState.Scissor[3]);

  ::glCullFace(this->CurrentState.CullFaceMode);

  ::glBlendEquationSeparate(
    this->CurrentState.BlendEquationValue1,
    this->CurrentState.BlendEquationValue2);

  // strictly query values below here
  ::glGetIntegerv(GL_MAX_TEXTURE_SIZE,
    &this->CurrentState.MaxTextureSize);
  ::glGetIntegerv(GL_MAJOR_VERSION,
    &this->CurrentState.MajorVersion);
  ::glGetIntegerv(GL_MINOR_VERSION,
    &this->CurrentState.MinorVersion);
}

void vtkOpenGLState::ResetGlClearColorState()
{
  GLfloat fparams[4];
  ::glGetFloatv(GL_COLOR_CLEAR_VALUE, fparams);
  this->CurrentState.ClearColor[0] = fparams[0];
  this->CurrentState.ClearColor[1] = fparams[1];
  this->CurrentState.ClearColor[2] = fparams[2];
  this->CurrentState.ClearColor[3] = fparams[3];
}

void vtkOpenGLState::ResetGlClearDepthState()
{
  GLfloat fparams;
  ::glGetFloatv(GL_DEPTH_CLEAR_VALUE, &fparams);
  this->CurrentState.ClearDepth = fparams;
}

void vtkOpenGLState::ResetGlDepthFuncState()
{
  GLint iparams;
  ::glGetIntegerv(GL_DEPTH_FUNC, &iparams);
  this->CurrentState.DepthFunc = static_cast<unsigned int>(iparams);
}

void vtkOpenGLState::ResetGlDepthMaskState()
{
  GLboolean params;
  ::glGetBooleanv(GL_DEPTH_WRITEMASK, &params);
  this->CurrentState.DepthMask = params;
}

void vtkOpenGLState::ResetGlColorMaskState()
{
  GLboolean params[4];
  ::glGetBooleanv(GL_COLOR_WRITEMASK, params);
  this->CurrentState.ColorMask[0] = params[0];
  this->CurrentState.ColorMask[1] = params[1];
  this->CurrentState.ColorMask[2] = params[2];
  this->CurrentState.ColorMask[3] = params[3];

}

void vtkOpenGLState::ResetGlViewportState()
{
  GLint iparams[4];
  ::glGetIntegerv(GL_VIEWPORT, iparams);
  this->CurrentState.Viewport[0] = iparams[0];
  this->CurrentState.Viewport[1] = iparams[1];
  this->CurrentState.Viewport[2] = iparams[2];
  this->CurrentState.Viewport[3] = iparams[3];
}

void vtkOpenGLState::ResetGlScissorState()
{
  GLint iparams[4];
  ::glGetIntegerv(GL_SCISSOR_BOX, iparams);
  this->CurrentState.Scissor[0] = iparams[0];
  this->CurrentState.Scissor[1] = iparams[1];
  this->CurrentState.Scissor[2] = iparams[2];
  this->CurrentState.Scissor[3] = iparams[3];
}

void vtkOpenGLState::ResetGlBlendFuncState()
{
  GLint iparams;
  ::glGetIntegerv(GL_BLEND_SRC_RGB, &iparams);
  this->CurrentState.BlendFunc[0] = static_cast<unsigned int>(iparams);
  ::glGetIntegerv(GL_BLEND_SRC_ALPHA, &iparams);
  this->CurrentState.BlendFunc[2] = static_cast<unsigned int>(iparams);
  ::glGetIntegerv(GL_BLEND_DST_RGB, &iparams);
  this->CurrentState.BlendFunc[1] = static_cast<unsigned int>(iparams);
  ::glGetIntegerv(GL_BLEND_DST_ALPHA, &iparams);
  this->CurrentState.BlendFunc[3] = static_cast<unsigned int>(iparams);
}

void vtkOpenGLState::ResetGlBlendEquationState()
{
  GLint iparams;
  ::glGetIntegerv(GL_BLEND_EQUATION_RGB, &iparams);
  this->CurrentState.BlendEquationValue1 = static_cast<unsigned int>(iparams);
  ::glGetIntegerv(GL_BLEND_EQUATION_ALPHA, &iparams);
  this->CurrentState.BlendEquationValue2 = static_cast<unsigned int>(iparams);
}

void vtkOpenGLState::ResetGlCullFaceState()
{
  GLint iparams;
  ::glGetIntegerv(GL_CULL_FACE_MODE, &iparams);
  this->CurrentState.CullFaceMode = static_cast<unsigned int>(iparams);
}

void vtkOpenGLState::vtkglClear(GLbitfield val)
{
  ::glClear(val);
}

// initialize all state values. This is important so that in
// ::Initialize we can just set the state to the current
// values (knowing that they are set). The reason we want
// Initialize to set to the current values is to reduce
// OpenGL churn in cases where application call Initialize
// often without really changing many of the values. For example
//
// viewport(0,0,100,100);
// Initialize(0,0,1,1); // using hardcoded initialization
// viewport(0,0,100,100);
//
// versus
//
// viewport(0,0,100,100);
// Initialize(0,0,100,100); // using last value
// viewport(0,0,100,100); // cache will skip this line
//
// Using current values avoids extra state changes when
// not required.
//
vtkOpenGLState::vtkOpenGLState()
{
  this->CurrentState.Blend = true;
  this->CurrentState.DepthTest = true;
  this->CurrentState.StencilTest = false;
  this->CurrentState.ScissorTest = true;
  this->CurrentState.CullFace = false;

  this->CurrentState.MultiSample = false;

  // initialize blending for transparency
  this->CurrentState.BlendFunc[0] = GL_SRC_ALPHA;
  this->CurrentState.BlendFunc[1] = GL_ONE_MINUS_SRC_ALPHA;
  this->CurrentState.BlendFunc[2] = GL_ONE;
  this->CurrentState.BlendFunc[3] = GL_ONE_MINUS_SRC_ALPHA;

  this->CurrentState.ClearColor[0] = 0.0;
  this->CurrentState.ClearColor[1] = 0.0;
  this->CurrentState.ClearColor[2] = 0.0;
  this->CurrentState.ClearColor[3] = 0.0;

  this->CurrentState.ColorMask[0] = GL_TRUE;
  this->CurrentState.ColorMask[1] = GL_TRUE;
  this->CurrentState.ColorMask[2] = GL_TRUE;
  this->CurrentState.ColorMask[3] = GL_TRUE;

  this->CurrentState.DepthFunc = GL_LEQUAL;

  this->CurrentState.ClearDepth = 1.0;

  this->CurrentState.DepthMask = GL_TRUE;

  this->CurrentState.Viewport[0] = 0;
  this->CurrentState.Viewport[1] = 0;
  this->CurrentState.Viewport[2] = 1;
  this->CurrentState.Viewport[3] = 1;

  this->CurrentState.Scissor[0] = 0;
  this->CurrentState.Scissor[1] = 0;
  this->CurrentState.Scissor[2] = 1;
  this->CurrentState.Scissor[3] = 1;

  this->CurrentState.CullFaceMode = GL_BACK;

  this->CurrentState.BlendEquationValue1 = GL_FUNC_ADD;
  this->CurrentState.BlendEquationValue2 = GL_FUNC_ADD;
}
