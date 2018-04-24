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


// If you define NO_CACHE then all state->vtkgl* calls
// will get passed down to OpenGL regardless of the current
// state. This basically bypasses the caching mechanism
// and is useful for tesing
#define NO_CACHE 1

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
}

void vtkOpenGLState::ClearColor(std::array<GLclampf, 4> val)
{
  this->vtkglClearColor(val[0], val[1], val[2], val[3]);
}

void vtkOpenGLState::vtkglClearColor(
  GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha
  )
{
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
}

void vtkOpenGLState::vtkglClearDepth(double val)
{
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
}

void vtkOpenGLState::vtkglDepthMask(GLboolean val)
{
#ifndef NO_CACHE
  if (this->CurrentState.DepthMask != val)
#endif
  {
    this->CurrentState.DepthMask = val;
    ::glDepthMask(val);
  }
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
}

void vtkOpenGLState::vtkglBlendEquation(GLenum val)
{
#ifndef NO_CACHE
  if (this->CurrentState.BlendEquation != val)
#endif
  {
    this->CurrentState.BlendEquation =  val;
    ::glBlendEquation(val);
  }
}

void vtkOpenGLState::vtkglCullFace(GLenum val)
{
#ifndef NO_CACHE
  if (this->CurrentState.CullFaceMode != val)
#endif
  {
    this->CurrentState.CullFaceMode =  val;
    ::glCullFace(val);
  }
}

void vtkOpenGLState::Viewport(std::array<GLint, 4> val)
{
  this->vtkglViewport(val[0], val[1], val[2], val[3]);
}

void vtkOpenGLState::vtkglViewport(GLint x, GLint y, GLsizei width, GLsizei height)
{
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
}

void vtkOpenGLState::Scissor(std::array<GLint, 4> val)
{
  this->vtkglScissor(val[0], val[1], val[2], val[3]);
}

void vtkOpenGLState::vtkglScissor(GLint x, GLint y, GLsizei width, GLsizei height)
{
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
}

void vtkOpenGLState::SetEnumState(GLenum cap, bool val)
{
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
}

void vtkOpenGLState::vtkglEnable(GLenum cap)
{
  this->SetEnumState(cap, true);
}

// return cached value if we have it
// otherwise forward to opengl
void vtkOpenGLState::vtkglGetBooleanv(GLenum pname, GLboolean *params)
{
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
}

void vtkOpenGLState::vtkglGetIntegerv(GLenum pname, GLint *params)
{
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
    default:
      ::glGetIntegerv(pname, params);
  }
}

#if GL_ES_VERSION_3_0 == 1
void vtkOpenGLState::vtkglGetDoublev(GLenum pname, double *)
{
  vtkGenericWarningMacro("glGetDouble not supported on OpenGL ES, requested: " << pname);
}
#else
void vtkOpenGLState::vtkglGetDoublev(GLenum pname, double *params)
{
  ::glGetDoublev(pname, params);
}
#endif

void vtkOpenGLState::vtkglGetFloatv(GLenum pname, GLfloat *params)
{
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

void vtkOpenGLState::Initialize(vtkOpenGLRenderWindow *)
{
  ::glEnable(GL_BLEND);
  this->CurrentState.Blend = true;

  ::glEnable( GL_DEPTH_TEST );
  this->CurrentState.DepthTest = true;

  ::glDisable(GL_STENCIL_TEST);
  this->CurrentState.StencilTest = false;

  ::glEnable(GL_SCISSOR_TEST);
  this->CurrentState.ScissorTest = true;

  ::glDisable(GL_CULL_FACE);
  this->CurrentState.CullFace = false;

#ifdef GL_MULTISAMPLE
  this->CurrentState.MultiSample = glIsEnabled(GL_MULTISAMPLE) == GL_TRUE;
#else
  this->CurrentState.MultiSample = false;
#endif

  // initialize blending for transparency
  ::glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,
                      GL_ONE,GL_ONE_MINUS_SRC_ALPHA);
  this->CurrentState.BlendFunc[0] = GL_SRC_ALPHA;
  this->CurrentState.BlendFunc[1] = GL_ONE_MINUS_SRC_ALPHA;
  this->CurrentState.BlendFunc[2] = GL_ONE;
  this->CurrentState.BlendFunc[3] = GL_ONE_MINUS_SRC_ALPHA;

  ::glClearColor(0.0,0.0,0.0,0.0);
  this->CurrentState.ClearColor[0] = 0.0;
  this->CurrentState.ClearColor[1] = 0.0;
  this->CurrentState.ClearColor[2] = 0.0;
  this->CurrentState.ClearColor[3] = 0.0;

  ::glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
  this->CurrentState.ColorMask[0] = GL_TRUE;
  this->CurrentState.ColorMask[1] = GL_TRUE;
  this->CurrentState.ColorMask[2] = GL_TRUE;
  this->CurrentState.ColorMask[3] = GL_TRUE;

  ::glDepthFunc( GL_LEQUAL );
  this->CurrentState.DepthFunc = GL_LEQUAL;

#if GL_ES_VERSION_3_0 == 1
  ::glClearDepthf(1.0f);
#else
  ::glClearDepth(1.0);
#endif
  this->CurrentState.ClearDepth = 1.0;

  ::glDepthMask(GL_TRUE);
  this->CurrentState.DepthMask = GL_TRUE;

  ::glViewport(0,0,1,1);
  this->CurrentState.Viewport[0] = 0;
  this->CurrentState.Viewport[1] = 0;
  this->CurrentState.Viewport[2] = 1;
  this->CurrentState.Viewport[3] = 1;

  ::glScissor(0,0,1,1);
  this->CurrentState.Scissor[0] = 0;
  this->CurrentState.Scissor[1] = 0;
  this->CurrentState.Scissor[2] = 0;
  this->CurrentState.Scissor[3] = 0;

  ::glCullFace(GL_BACK);
  this->CurrentState.CullFaceMode = GL_BACK;
}

void vtkOpenGLState::vtkglClear(GLbitfield val)
{
  ::glClear(val);
}
