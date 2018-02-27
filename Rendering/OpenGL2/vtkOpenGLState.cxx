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


// If you define NO_CACHE then all state->gl* calls
// will get passed down to OpenGL regardless of the current
// state. This basically bypasses the caching mechanism
// and is useful for tesing
#define NO_CACHE 1

vtkOpenGLState::ScopedglDepthMask::ScopedglDepthMask(vtkOpenGLState *s)
{
  this->State = s;
  this->Value = this->State->CurrentState.DepthMask;
  this->Method = &vtkOpenGLState::glDepthMask;
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
  this->glColorMask(val[0], val[1], val[2], val[3]);
}

void vtkOpenGLState::glColorMask(
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
  this->glClearColor(val[0], val[1], val[2], val[3]);
}

void vtkOpenGLState::glClearColor(
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

void vtkOpenGLState::glClearDepth(double val)
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

void vtkOpenGLState::glDepthFunc(GLenum val)
{
#ifndef NO_CACHE
  if (this->CurrentState.DepthFunc != val)
#endif
  {
    this->CurrentState.DepthFunc = val;
    ::glDepthFunc(val);
  }
}

void vtkOpenGLState::glDepthMask(GLboolean val)
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
  this->glBlendFuncSeparate(val[0], val[1], val[2], val[3]);
}

void vtkOpenGLState::glBlendFuncSeparate(
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

void vtkOpenGLState::glBlendEquation(GLenum val)
{
#ifndef NO_CACHE
  if (this->CurrentState.BlendEquation != val)
#endif
  {
    this->CurrentState.BlendEquation =  val;
    ::glBlendEquation(val);
  }
}

void vtkOpenGLState::glCullFace(GLenum val)
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
  this->glViewport(val[0], val[1], val[2], val[3]);
}

void vtkOpenGLState::glViewport(GLint x, GLint y, GLsizei width, GLsizei height)
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
  this->glScissor(val[0], val[1], val[2], val[3]);
}

void vtkOpenGLState::glScissor(GLint x, GLint y, GLsizei width, GLsizei height)
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
    case GL_MULTISAMPLE:
      if (this->CurrentState.MultiSample != val)
      {
        this->CurrentState.MultiSample = val;
        changed = true;
      }
      break;
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

void vtkOpenGLState::glEnable(GLenum cap)
{
  this->SetEnumState(cap, true);
}

void vtkOpenGLState::GetBlendFuncState(int *v)
{
  v[0] = this->CurrentState.BlendFunc[0];
  v[1] = this->CurrentState.BlendFunc[1];
  v[2] = this->CurrentState.BlendFunc[2];
  v[3] = this->CurrentState.BlendFunc[3];
}

void vtkOpenGLState::GetClearColor(GLclampf *v)
{
  v[0] = this->CurrentState.ClearColor[0];
  v[1] = this->CurrentState.ClearColor[1];
  v[2] = this->CurrentState.ClearColor[2];
  v[3] = this->CurrentState.ClearColor[3];
}

bool vtkOpenGLState::GetDepthMask()
{
  return this->CurrentState.DepthMask == GL_TRUE;
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
    case GL_MULTISAMPLE:
      return this->CurrentState.MultiSample;
    case GL_SCISSOR_TEST:
      return this->CurrentState.ScissorTest;
    case GL_STENCIL_TEST:
      return this->CurrentState.StencilTest;
    default:
      vtkGenericWarningMacro("Bad request for enum status");
  }
  return false;
}

void vtkOpenGLState::glDisable(GLenum cap)
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

  this->CurrentState.MultiSample = glIsEnabled(GL_MULTISAMPLE) == GL_TRUE;

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

  ::glClearDepth(1.0);
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

void vtkOpenGLState::glClear(GLbitfield val)
{
  ::glClear(val);
}
