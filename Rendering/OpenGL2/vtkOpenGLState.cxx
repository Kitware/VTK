// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkOpenGLState.h"
#include "vtk_glew.h"

#include "vtkObjectFactory.h"
#include "vtkOpenGLFramebufferObject.h"
#include "vtkOpenGLRenderUtilities.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLShaderCache.h"
#include "vtkOpenGLVertexBufferObjectCache.h"
#include "vtkRenderer.h"
#include "vtkTextureUnitManager.h"

// must be included after a vtkObject subclass
#include "vtkOpenGLError.h"

#include <cmath>

#include "vtksys/SystemInformation.hxx"

// If you define NO_CACHE then all state->vtkgl* calls
// will get passed down to OpenGL regardless of the current
// state. This basically bypasses the caching mechanism
// and is useful for tesing
// #define NO_CACHE 1

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
// #undef VTK_REPORT_OPENGL_ERRORS
#ifdef VTK_REPORT_OPENGL_ERRORS

// this method checks all the cached state to make sure
// nothing is out of sync. It can be slow.
VTK_ABI_NAMESPACE_BEGIN
void vtkOpenGLState::CheckState()
{
  bool error = false;

  vtkOpenGLRenderUtilities::MarkDebugEvent("Checking OpenGL State");

  GLboolean params[4];

  auto& cs = this->Stack.top();

  ::glGetBooleanv(GL_DEPTH_WRITEMASK, params);
  if (params[0] != cs.DepthMask)
  {
    vtkGenericWarningMacro("Error in cache state for GL_DEPTH_WRITEMASK");
    this->ResetGLDepthMaskState();
    error = true;
  }
  ::glGetBooleanv(GL_COLOR_WRITEMASK, params);
  if (params[0] != cs.ColorMask[0] || params[1] != cs.ColorMask[1] ||
    params[2] != cs.ColorMask[2] || params[3] != cs.ColorMask[3])
  {
    vtkGenericWarningMacro("Error in cache state for GL_COLOR_WRITEMASK");
    this->ResetGLColorMaskState();
    error = true;
  }
  ::glGetBooleanv(GL_BLEND, params);
  if ((params[0] != 0) != cs.Blend)
  {
    vtkGenericWarningMacro("Error in cache state for GL_BLEND");
    this->ResetEnumState(GL_BLEND);
    error = true;
  }
  ::glGetBooleanv(GL_DEPTH_TEST, params);
  if ((params[0] != 0) != cs.DepthTest)
  {
    vtkGenericWarningMacro("Error in cache state for GL_DEPTH_TEST");
    this->ResetEnumState(GL_DEPTH_TEST);
    error = true;
  }
  ::glGetBooleanv(GL_CULL_FACE, params);
  if ((params[0] != 0) != cs.CullFace)
  {
    vtkGenericWarningMacro("Error in cache state for GL_CULL_FACE");
    this->ResetEnumState(GL_CULL_FACE);
    error = true;
  }
#ifdef GL_MULTISAMPLE
  ::glGetBooleanv(GL_MULTISAMPLE, params);
  if ((params[0] != 0) != cs.MultiSample)
  {
    vtkGenericWarningMacro("Error in cache state for GL_MULTISAMPLE");
    this->ResetEnumState(GL_MULTISAMPLE);
    error = true;
  }
#endif
  ::glGetBooleanv(GL_SCISSOR_TEST, params);
  if ((params[0] != 0) != cs.ScissorTest)
  {
    vtkGenericWarningMacro("Error in cache state for GL_SCISSOR_TEST");
    this->ResetEnumState(GL_SCISSOR_TEST);
    error = true;
  }
  ::glGetBooleanv(GL_STENCIL_TEST, params);
  if ((params[0] != 0) != cs.StencilTest)
  {
    vtkGenericWarningMacro("Error in cache state for GL_STENCIL_TEST");
    this->ResetEnumState(GL_STENCIL_TEST);
    error = true;
  }
#ifdef GL_TEXTURE_CUBE_MAP_SEAMLESS
  ::glGetBooleanv(GL_TEXTURE_CUBE_MAP_SEAMLESS, params);
  if ((params[0] != 0) != cs.CubeMapSeamless)
  {
    vtkGenericWarningMacro("Error in cache state for GL_TEXTURE_CUBE_MAP_SEAMLESS");
    this->ResetEnumState(GL_TEXTURE_CUBE_MAP_SEAMLESS);
    error = true;
  }
#endif

  GLint iparams[4];
#if defined(__APPLE__)
  // OSX systems seem to change the glViewport upon a window resize
  // under the hood, so our viewport cache cannot be trusted
  this->ResetGLViewportState();
#endif
  ::glGetIntegerv(GL_VIEWPORT, iparams);
  if (iparams[0] != cs.Viewport[0] || iparams[1] != cs.Viewport[1] ||
    iparams[2] != cs.Viewport[2] || iparams[3] != cs.Viewport[3])
  {
    vtkGenericWarningMacro("Error in cache state for GL_VIEWPORT");
    this->ResetGLViewportState();
    error = true;
  }
  ::glGetIntegerv(GL_SCISSOR_BOX, iparams);
  if (iparams[0] != cs.Scissor[0] || iparams[1] != cs.Scissor[1] || iparams[2] != cs.Scissor[2] ||
    iparams[3] != cs.Scissor[3])
  {
    vtkGenericWarningMacro("Error in cache state for GL_SCISSOR_BOX");
    this->ResetGLScissorState();
    error = true;
  }
  ::glGetIntegerv(GL_CULL_FACE_MODE, iparams);
  if (iparams[0] != static_cast<int>(cs.CullFaceMode))
  {
    vtkGenericWarningMacro("Error in cache state for GL_CULL_FACE_MODE");
    this->ResetGLCullFaceState();
    error = true;
  }
  ::glGetIntegerv(GL_ACTIVE_TEXTURE, iparams);
  if (iparams[0] != static_cast<int>(cs.ActiveTexture))
  {
    vtkGenericWarningMacro("Error in cache state for GL_ACTIVE_TEXTURE");
    this->ResetGLActiveTexture();
    error = true;
  }
  ::glGetIntegerv(GL_DEPTH_FUNC, iparams);
  if (iparams[0] != static_cast<int>(cs.DepthFunc))
  {
    vtkGenericWarningMacro("Error in cache state for GL_DEPTH_FUNC");
    this->ResetGLDepthFuncState();
    error = true;
  }
  ::glGetIntegerv(GL_BLEND_SRC_RGB, iparams);
  if (iparams[0] != static_cast<int>(cs.BlendFunc[0]))
  {
    vtkGenericWarningMacro("Error in cache state for GL_BLEND_SRC_RGB");
    this->ResetGLBlendFuncState();
    error = true;
  }
  ::glGetIntegerv(GL_BLEND_SRC_ALPHA, iparams);
  if (iparams[0] != static_cast<int>(cs.BlendFunc[2]))
  {
    vtkGenericWarningMacro("Error in cache state for GL_BLEND_SRC_ALPHA");
    this->ResetGLBlendFuncState();
    error = true;
  }
  ::glGetIntegerv(GL_BLEND_DST_RGB, iparams);
  if (iparams[0] != static_cast<int>(cs.BlendFunc[1]))
  {
    vtkGenericWarningMacro("Error in cache state for GL_BLEND_DST_RGB");
    this->ResetGLBlendFuncState();
    error = true;
  }
  ::glGetIntegerv(GL_BLEND_DST_ALPHA, iparams);
  if (iparams[0] != static_cast<int>(cs.BlendFunc[3]))
  {
    vtkGenericWarningMacro("Error in cache state for GL_BLEND_DST_ALPHA");
    this->ResetGLBlendFuncState();
    error = true;
  }
  ::glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, iparams);
  if (iparams[0] != static_cast<int>(cs.DrawBinding.GetBinding()))
  {
    vtkGenericWarningMacro("Error in cache state for GL_DRAW_FRAMEBUFFER_BINDING");
    this->ResetFramebufferBindings();
    error = true;
  }
  ::glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, iparams);
  if (iparams[0] != static_cast<int>(cs.ReadBinding.GetBinding()))
  {
    vtkGenericWarningMacro("Error in cache state for GL_READ_FRAMEBUFFER_BINDING");
    this->ResetFramebufferBindings();
    error = true;
  }
  unsigned int sval;
#ifdef GL_DRAW_BUFFER
  ::glGetIntegerv(GL_DRAW_BUFFER, iparams);
  sval = cs.DrawBinding.GetDrawBuffer(0);
  if (sval == GL_BACK_LEFT)
  {
    sval = GL_BACK;
  }
  if (iparams[0] == GL_BACK_LEFT)
  {
    iparams[0] = GL_BACK;
  }
  if (iparams[0] != static_cast<int>(sval))
  {
    vtkGenericWarningMacro("Error in cache state for GL_DRAW_BUFFER got "
      << iparams[0] << " expected" << cs.DrawBinding.GetDrawBuffer(0));
    this->ResetFramebufferBindings();
    error = true;
  }
#endif
  ::glGetIntegerv(GL_READ_BUFFER, iparams);
  sval = cs.ReadBinding.GetReadBuffer();
  // handle odd left right stuff
  if (sval == GL_BACK_LEFT)
  {
    sval = GL_BACK;
  }
  if (iparams[0] == GL_BACK_LEFT)
  {
    iparams[0] = GL_BACK;
  }
  if (iparams[0] != static_cast<int>(sval))
  {
    vtkGenericWarningMacro("Error in cache state for GL_READ_BUFFER");
    this->ResetFramebufferBindings();
    error = true;
  }

  GLfloat fparams[4];
  // note people do set this to nan
  ::glGetFloatv(GL_COLOR_CLEAR_VALUE, fparams);
  if ((!(std::isnan(fparams[0]) && std::isnan(cs.ClearColor[0])) &&
        fparams[0] != cs.ClearColor[0]) ||
    (!(std::isnan(fparams[1]) && std::isnan(cs.ClearColor[1])) && fparams[1] != cs.ClearColor[1]) ||
    (!(std::isnan(fparams[2]) && std::isnan(cs.ClearColor[2])) && fparams[2] != cs.ClearColor[2]) ||
    (!(std::isnan(fparams[3]) && std::isnan(cs.ClearColor[3])) && fparams[3] != cs.ClearColor[3]))
  {
    vtkGenericWarningMacro("Error in cache state for GL_COLOR_CLEAR_VALUE");
    this->ResetGLClearColorState();
    error = true;
  }

  if (error)
  {
    std::string msg = vtksys::SystemInformation::GetProgramStack(0, 0);
    vtkGenericWarningMacro("at stack loc\n" << msg);
  }
  vtkOpenGLRenderUtilities::MarkDebugEvent("Finished Checking OpenGL State");
}

namespace
{
bool reportOpenGLErrors(std::string& result)
{
  const int maxErrors = 16;
  unsigned int errCode[maxErrors] = { 0 };
  const char* errDesc[maxErrors] = { nullptr };

  int numErrors = vtkGetOpenGLErrors(maxErrors, errCode, errDesc);

  if (numErrors)
  {
    std::ostringstream oss;
    vtkPrintOpenGLErrors(oss, maxErrors, numErrors, errCode, errDesc);

    oss << "\n with stack trace of\n" << vtksys::SystemInformation::GetProgramStack(0, 0);
    result = oss.str();
    return true;
  }
  return false;
}

} // anon namespace

#define vtkOpenGLCheckStateMacro() this->CheckState()

#define vtkCheckOpenGLErrorsWithStack(message)                                                     \
  {                                                                                                \
    std::string _tmp;                                                                              \
    if (reportOpenGLErrors(_tmp))                                                                  \
    {                                                                                              \
      vtkGenericWarningMacro("Error " << message << _tmp);                                         \
      vtkOpenGLClearErrorMacro();                                                                  \
    }                                                                                              \
  }

VTK_ABI_NAMESPACE_END
#else // VTK_REPORT_OPENGL_ERRORS

#define vtkCheckOpenGLErrorsWithStack(message)
#define vtkOpenGLCheckStateMacro()

#endif // VTK_REPORT_OPENGL_ERRORS

//
//////////////////////////////////////////////////////////////////////////////

VTK_ABI_NAMESPACE_BEGIN
vtkOpenGLState::BufferBindingState::BufferBindingState()
{
  // this->Framebuffer = nullptr;
  this->ReadBuffer = GL_NONE;
  this->DrawBuffers[0] = GL_BACK;
}

// bool vtkOpenGLState::BufferBindingState::operator==(
//   const BufferBindingState& a, const BufferBindingState& b)
// {
//   return a.Framebuffer == b.Framebuffer &&
// }

unsigned int vtkOpenGLState::BufferBindingState::GetBinding()
{
  // if (this->Framebuffer)
  // {
  //   return this->Framebuffer->GetFBOIndex();
  // }
  return this->Binding;
}

unsigned int vtkOpenGLState::BufferBindingState::GetDrawBuffer(unsigned int val)
{
  // if (this->Framebuffer)
  // {
  //   return this->Framebuffer->GetActiveDrawBuffer(val);
  // }
  return this->DrawBuffers[val];
}

unsigned int vtkOpenGLState::BufferBindingState::GetReadBuffer()
{
  // if (this->Framebuffer)
  // {
  //   return this->Framebuffer->GetActiveReadBuffer();
  // }
  return this->ReadBuffer;
}

vtkOpenGLState::ScopedglDepthMask::ScopedglDepthMask(vtkOpenGLState* s)
{
  this->State = s;
  this->Value = this->State->Stack.top().DepthMask;
  this->Method = &vtkOpenGLState::vtkglDepthMask;
}

vtkOpenGLState::ScopedglColorMask::ScopedglColorMask(vtkOpenGLState* s)
{
  this->State = s;
  this->Value = this->State->Stack.top().ColorMask;
  this->Method = &vtkOpenGLState::ColorMask;
}

vtkOpenGLState::ScopedglDepthFunc::ScopedglDepthFunc(vtkOpenGLState* s)
{
  this->State = s;
  this->Value = this->State->Stack.top().DepthFunc;
  this->Method = &vtkOpenGLState::vtkglDepthFunc;
}

vtkOpenGLState::ScopedglClearColor::ScopedglClearColor(vtkOpenGLState* s)
{
  this->State = s;
  this->Value = this->State->Stack.top().ClearColor;
  this->Method = &vtkOpenGLState::ClearColor;
}

vtkOpenGLState::ScopedglScissor::ScopedglScissor(vtkOpenGLState* s)
{
  this->State = s;
  this->Value = this->State->Stack.top().Scissor;
  this->Method = &vtkOpenGLState::Scissor;
}

vtkOpenGLState::ScopedglViewport::ScopedglViewport(vtkOpenGLState* s)
{
  this->State = s;
  this->Value = this->State->Stack.top().Viewport;
  this->Method = &vtkOpenGLState::Viewport;
}

vtkOpenGLState::ScopedglBlendFuncSeparate::ScopedglBlendFuncSeparate(vtkOpenGLState* s)
{
  this->State = s;
  this->Value = this->State->Stack.top().BlendFunc;
  this->Method = &vtkOpenGLState::BlendFuncSeparate;
}

vtkOpenGLState::ScopedglActiveTexture::ScopedglActiveTexture(vtkOpenGLState* s)
{
  this->State = s;
  this->Value = this->State->Stack.top().ActiveTexture;
  this->Method = &vtkOpenGLState::vtkglActiveTexture;
}

void vtkOpenGLState::ColorMask(std::array<GLboolean, 4> val)
{
  this->vtkglColorMask(val[0], val[1], val[2], val[3]);
}

void vtkOpenGLState::vtkglColorMask(GLboolean r, GLboolean g, GLboolean b, GLboolean a)
{
  vtkOpenGLCheckStateMacro();
  auto& cs = this->Stack.top();

#ifndef NO_CACHE
  if (cs.ColorMask[0] != r || cs.ColorMask[1] != g || cs.ColorMask[2] != b || cs.ColorMask[3] != a)
#endif
  {
    cs.ColorMask[0] = r;
    cs.ColorMask[1] = g;
    cs.ColorMask[2] = b;
    cs.ColorMask[3] = a;
    ::glColorMask(r, g, b, a);
  }

  vtkCheckOpenGLErrorsWithStack("glColorMask");
}

void vtkOpenGLState::ClearColor(std::array<GLclampf, 4> val)
{
  this->vtkglClearColor(val[0], val[1], val[2], val[3]);
}

void vtkOpenGLState::vtkglClearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)
{
  vtkOpenGLCheckStateMacro();
  auto& cs = this->Stack.top();

#ifndef NO_CACHE
  if (cs.ClearColor[0] != red || cs.ClearColor[1] != green || cs.ClearColor[2] != blue ||
    cs.ClearColor[3] != alpha)
#endif
  {
    cs.ClearColor[0] = red;
    cs.ClearColor[1] = green;
    cs.ClearColor[2] = blue;
    cs.ClearColor[3] = alpha;
    ::glClearColor(red, green, blue, alpha);
  }

  vtkCheckOpenGLErrorsWithStack("glClearColor");
}

void vtkOpenGLState::vtkglClearDepth(double val)
{
  vtkOpenGLCheckStateMacro();
  auto& cs = this->Stack.top();

#ifndef NO_CACHE
  if (cs.ClearDepth != val)
#endif
  {
    cs.ClearDepth = val;
#ifdef GL_ES_VERSION_3_0
    ::glClearDepthf(static_cast<GLclampf>(val));
#else
    ::glClearDepth(val);
#endif
  }
  vtkCheckOpenGLErrorsWithStack("glClearDepth");
}

void vtkOpenGLState::vtkBindFramebuffer(unsigned int target, vtkOpenGLFramebufferObject* fo)
{
  this->vtkglBindFramebuffer(target, fo ? fo->GetFBOIndex() : 0);
}

void vtkOpenGLState::vtkglBindFramebuffer(unsigned int target, unsigned int val)
{
  vtkOpenGLCheckStateMacro();
  auto& cs = this->Stack.top();

  if (target == GL_DRAW_FRAMEBUFFER || target == GL_FRAMEBUFFER)
  {
#ifndef NO_CACHE
    if (cs.DrawBinding.Binding != val)
#endif
    {
      cs.DrawBinding.Binding = val;
      ::glBindFramebuffer(GL_DRAW_FRAMEBUFFER, val);
#ifdef GL_DRAW_BUFFER
      ::glGetIntegerv(GL_DRAW_BUFFER, (int*)&cs.DrawBinding.DrawBuffers[0]);
#endif
    }
  }

  if (target == GL_READ_FRAMEBUFFER || target == GL_FRAMEBUFFER)
  {
#ifndef NO_CACHE
    if (cs.ReadBinding.Binding != val)
#endif
    {
      cs.ReadBinding.Binding = val;
      ::glBindFramebuffer(GL_READ_FRAMEBUFFER, val);
      ::glGetIntegerv(GL_READ_BUFFER, (int*)&cs.ReadBinding.ReadBuffer);
    }
  }

  vtkCheckOpenGLErrorsWithStack("glBindFramebuffer");
}

void vtkOpenGLState::vtkglDrawBuffer(unsigned int val)
{
  vtkOpenGLCheckStateMacro();
  auto& cs = this->Stack.top();

  if (cs.DrawBinding.Binding && val < GL_COLOR_ATTACHMENT0 && val != GL_NONE)
  {
    // todo get rid of the && and make this always an error if FO is set
    vtkGenericWarningMacro(
      "A vtkOpenGLFramebufferObject is currently bound but a hardware draw buffer was requested.");
    std::string msg = vtksys::SystemInformation::GetProgramStack(0, 0);
    vtkGenericWarningMacro("at stack loc\n" << msg);
  }

#ifndef NO_CACHE
  if (cs.DrawBinding.DrawBuffers[0] != val)
#endif
  {
    cs.DrawBinding.DrawBuffers[0] = val;
    ::glDrawBuffers(1, cs.DrawBinding.DrawBuffers);
  }

  // change all stack entries for the same framebuffer
  for (auto& se : this->DrawBindings)
  {
    if (se.Binding == cs.DrawBinding.Binding)
    {
      se.DrawBuffers[0] = val;
    }
  }

  vtkCheckOpenGLErrorsWithStack("glDrawBuffer");
}

void vtkOpenGLState::vtkglDrawBuffers(unsigned int count, unsigned int* vals)
{
  vtkOpenGLCheckStateMacro();

  if (count <= 0)
  {
    return;
  }

  auto& cs = this->Stack.top();
  if (cs.DrawBinding.Binding && vals[0] < GL_COLOR_ATTACHMENT0 && vals[0] != GL_NONE)
  {
    // todo get rid of the && and make this always an error if FO is set
    vtkGenericWarningMacro(
      "A vtkOpenGLFramebufferObject is currently bound but hardware draw buffers were requested.");
  }

#ifndef NO_CACHE
  bool changed = false;
  for (int i = 0; i < static_cast<int>(count) && i < 10; ++i)
  {
    if (vals[i] != cs.DrawBinding.DrawBuffers[i])
    {
      changed = true;
    }
  }
  if (count > 10)
  {
    changed = true;
  }
  if (changed)
#endif
  {
    for (unsigned int i = 0; i < count && i < 10; ++i)
    {
      cs.DrawBinding.DrawBuffers[i] = vals[i];
    }
    ::glDrawBuffers(count, vals);
  }

  // change all stack entries for the same framebuffer
  for (auto& se : this->DrawBindings)
  {
    if (se.Binding == cs.DrawBinding.Binding)
    {
      for (unsigned int i = 0; i < count && i < 10; ++i)
      {
        se.DrawBuffers[i] = vals[i];
      }
    }
  }

  vtkCheckOpenGLErrorsWithStack("glDrawBuffers");
}

void vtkOpenGLState::vtkDrawBuffers(
  unsigned int count, unsigned int* vals, vtkOpenGLFramebufferObject* fo)
{
  vtkOpenGLCheckStateMacro();

  if (count <= 0)
  {
    return;
  }

  auto& cs = this->Stack.top();
  if (fo->GetFBOIndex() != cs.DrawBinding.Binding)
  {
    vtkGenericWarningMacro(
      "Attempt to set draw buffers from a Framebuffer Object that is not bound.");
  }

  this->vtkglDrawBuffers(count, vals);
}

void vtkOpenGLState::vtkglReadBuffer(unsigned int val)
{
  vtkOpenGLCheckStateMacro();
  auto& cs = this->Stack.top();

  if (cs.ReadBinding.Binding && val < GL_COLOR_ATTACHMENT0 && val != GL_NONE)
  {
    vtkGenericWarningMacro(
      "A vtkOpenGLFramebufferObject is currently bound but a hardware read buffer was requested.");
  }

#ifndef NO_CACHE
  if (cs.ReadBinding.ReadBuffer != val)
#endif
  {
    cs.ReadBinding.ReadBuffer = val;
    ::glReadBuffer(val);
  }

  // change all stack entries for the same framebuffer
  for (auto& se : this->ReadBindings)
  {
    if (se.Binding == cs.ReadBinding.Binding)
    {
      se.ReadBuffer = val;
    }
  }

  vtkCheckOpenGLErrorsWithStack("glReadBuffer");
}

void vtkOpenGLState::vtkReadBuffer(unsigned int val, vtkOpenGLFramebufferObject* fo)
{
  vtkOpenGLCheckStateMacro();
  auto& cs = this->Stack.top();

  if (fo->GetFBOIndex() != cs.ReadBinding.Binding)
  {
    vtkGenericWarningMacro(
      "Attempt to set read buffer from a Framebuffer Object that is not bound.");
  }

  this->vtkglReadBuffer(val);
}

void vtkOpenGLState::vtkglDepthFunc(GLenum val)
{
  vtkOpenGLCheckStateMacro();
  auto& cs = this->Stack.top();

#ifndef NO_CACHE
  if (cs.DepthFunc != val)
#endif
  {
    cs.DepthFunc = val;
    ::glDepthFunc(val);
  }
  vtkCheckOpenGLErrorsWithStack("glDepthFunc");
}

void vtkOpenGLState::vtkglDepthMask(GLboolean val)
{
  vtkOpenGLCheckStateMacro();
  auto& cs = this->Stack.top();

#ifndef NO_CACHE
  if (cs.DepthMask != val)
#endif
  {
    cs.DepthMask = val;
    ::glDepthMask(val);
  }
  vtkCheckOpenGLErrorsWithStack("glDepthMask");
}

void vtkOpenGLState::vtkglPointSize(GLfloat val)
{
  vtkOpenGLCheckStateMacro();
  auto& cs = this->Stack.top();

#ifndef NO_CACHE
  if (cs.PointSize != val)
#endif
  {
    cs.PointSize = val;
#ifndef GL_ES_VERSION_3_0
    ::glPointSize(val);
#endif
  }
  vtkCheckOpenGLErrorsWithStack("glPointSize");
}

void vtkOpenGLState::vtkglLineWidth(GLfloat val)
{
  vtkOpenGLCheckStateMacro();
  auto& cs = this->Stack.top();

#ifndef NO_CACHE
  if (cs.LineWidth != val)
#endif
  {
    cs.LineWidth = val;
    ::glLineWidth(val);
  }
  vtkCheckOpenGLErrorsWithStack("glLineWidth");
}

void vtkOpenGLState::vtkglStencilMask(GLuint mask)
{
  this->vtkglStencilMaskSeparate(GL_FRONT_AND_BACK, mask);
}

void vtkOpenGLState::vtkglStencilMaskSeparate(GLuint face, GLuint mask)
{
  vtkOpenGLCheckStateMacro();
  auto& cs = this->Stack.top();

  if (face == GL_FRONT || face == GL_FRONT_AND_BACK)
  {
#ifndef NO_CACHE
    if (cs.StencilMaskFront != mask)
#endif
    {
      cs.StencilMaskFront = mask;
      ::glStencilMaskSeparate(GL_FRONT, mask);
    }
  }

  if (face == GL_BACK || face == GL_FRONT_AND_BACK)
  {
#ifndef NO_CACHE
    if (cs.StencilMaskBack != mask)
#endif
    {
      cs.StencilMaskBack = mask;
      ::glStencilMaskSeparate(GL_BACK, mask);
    }
  }

  vtkCheckOpenGLErrorsWithStack("glStencilMaskSeparate");
}

void vtkOpenGLState::vtkglStencilOp(GLuint sfail, GLuint dpfail, GLuint dppass)
{
  this->vtkglStencilOpSeparate(GL_FRONT_AND_BACK, sfail, dpfail, dppass);
}

void vtkOpenGLState::vtkglStencilOpSeparate(GLuint face, GLuint sfail, GLuint dpfail, GLuint dppass)
{
  vtkOpenGLCheckStateMacro();
  auto& cs = this->Stack.top();

  std::array<unsigned int, 3> vals = { sfail, dpfail, dppass };

  if (face == GL_FRONT || face == GL_FRONT_AND_BACK)
  {
#ifndef NO_CACHE
    if (cs.StencilOpFront != vals)
#endif
    {
      cs.StencilOpFront = vals;
      ::glStencilOpSeparate(GL_FRONT, vals[0], vals[1], vals[2]);
    }
  }

  if (face == GL_BACK || face == GL_FRONT_AND_BACK)
  {
#ifndef NO_CACHE
    if (cs.StencilOpBack != vals)
#endif
    {
      cs.StencilOpBack = vals;
      ::glStencilOpSeparate(GL_BACK, vals[0], vals[1], vals[2]);
    }
  }

  vtkCheckOpenGLErrorsWithStack("glStencilOpSeparate");
}

void vtkOpenGLState::vtkglStencilFunc(GLuint func, GLint ref, GLuint mask)
{
  this->vtkglStencilFuncSeparate(GL_FRONT_AND_BACK, func, ref, mask);
}

void vtkOpenGLState::vtkglStencilFuncSeparate(
  unsigned int face, unsigned int func, int ref, unsigned int mask)
{
  vtkOpenGLCheckStateMacro();
  auto& cs = this->Stack.top();

  std::array<unsigned int, 3> vals = { func, static_cast<unsigned int>(ref), mask };

  if (face == GL_FRONT || face == GL_FRONT_AND_BACK)
  {
#ifndef NO_CACHE
    if (cs.StencilFuncFront != vals)
#endif
    {
      cs.StencilFuncFront = vals;
      ::glStencilFuncSeparate(GL_FRONT, vals[0], static_cast<GLint>(vals[1]), vals[2]);
    }
  }

  if (face == GL_BACK || face == GL_FRONT_AND_BACK)
  {
#ifndef NO_CACHE
    if (cs.StencilFuncBack != vals)
#endif
    {
      cs.StencilFuncBack = vals;
      ::glStencilFuncSeparate(GL_BACK, vals[0], static_cast<GLint>(vals[1]), vals[2]);
    }
  }

  vtkCheckOpenGLErrorsWithStack("glStencilFuncSeparate");
}

void vtkOpenGLState::BlendFuncSeparate(std::array<GLenum, 4> val)
{
  this->vtkglBlendFuncSeparate(val[0], val[1], val[2], val[3]);
}

void vtkOpenGLState::vtkglBlendFuncSeparate(GLenum val1, GLenum val2, GLenum val3, GLenum val4)
{
  vtkOpenGLCheckStateMacro();
  auto& cs = this->Stack.top();

#ifndef NO_CACHE
  if (cs.BlendFunc[0] != val1 || cs.BlendFunc[1] != val2 || cs.BlendFunc[2] != val3 ||
    cs.BlendFunc[3] != val4)
#endif
  {
    cs.BlendFunc[0] = val1;
    cs.BlendFunc[1] = val2;
    cs.BlendFunc[2] = val3;
    cs.BlendFunc[3] = val4;
    ::glBlendFuncSeparate(val1, val2, val3, val4);
  }
  vtkCheckOpenGLErrorsWithStack("glBlendFuncSeparate");
}

void vtkOpenGLState::vtkglBlendEquation(GLenum val)
{
  this->vtkglBlendEquationSeparate(val, val);
}

void vtkOpenGLState::vtkglBlendEquationSeparate(GLenum val, GLenum val2)
{
  vtkOpenGLCheckStateMacro();
  auto& cs = this->Stack.top();

#ifndef NO_CACHE
  if (cs.BlendEquationValue1 != val || cs.BlendEquationValue2 != val2)
#endif
  {
    cs.BlendEquationValue1 = val;
    cs.BlendEquationValue2 = val2;
    ::glBlendEquationSeparate(val, val2);
  }

  vtkCheckOpenGLErrorsWithStack("glBlendEquationSeparate");
}

void vtkOpenGLState::vtkglCullFace(GLenum val)
{
  vtkOpenGLCheckStateMacro();
  auto& cs = this->Stack.top();

#ifndef NO_CACHE
  if (cs.CullFaceMode != val)
#endif
  {
    cs.CullFaceMode = val;
    ::glCullFace(val);
  }
  vtkCheckOpenGLErrorsWithStack("glCullFace");
}

void vtkOpenGLState::vtkglActiveTexture(unsigned int val)
{
  vtkOpenGLCheckStateMacro();
  auto& cs = this->Stack.top();

#ifndef NO_CACHE
  if (cs.ActiveTexture != val)
#endif
  {
    cs.ActiveTexture = val;
    ::glActiveTexture(val);
  }
  vtkCheckOpenGLErrorsWithStack("glActiveTexture");
}

void vtkOpenGLState::Viewport(std::array<GLint, 4> val)
{
  this->vtkglViewport(val[0], val[1], val[2], val[3]);
}

void vtkOpenGLState::vtkglViewport(GLint x, GLint y, GLsizei width, GLsizei height)
{
  vtkOpenGLCheckStateMacro();
  auto& cs = this->Stack.top();

#if !defined(NO_CACHE) && !defined(__APPLE__)
  if (cs.Viewport[0] != x || cs.Viewport[1] != y || cs.Viewport[2] != width ||
    cs.Viewport[3] != height)
#endif
  {
    cs.Viewport[0] = x;
    cs.Viewport[1] = y;
    cs.Viewport[2] = width;
    cs.Viewport[3] = height;
    ::glViewport(x, y, width, height);
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
  auto& cs = this->Stack.top();

#ifndef NO_CACHE
  if (cs.Scissor[0] != x || cs.Scissor[1] != y || cs.Scissor[2] != width || cs.Scissor[3] != height)
#endif
  {
    cs.Scissor[0] = x;
    cs.Scissor[1] = y;
    cs.Scissor[2] = width;
    cs.Scissor[3] = height;
    ::glScissor(x, y, width, height);
  }
  vtkCheckOpenGLErrorsWithStack("glScissor");
}

void vtkOpenGLState::SetEnumState(GLenum cap, bool val)
{
  vtkOpenGLCheckStateMacro();
  auto& cs = this->Stack.top();

#ifndef NO_CACHE
  bool changed = false;
#else
  bool changed = true;
#endif
  switch (cap)
  {
    case GL_BLEND:
      if (cs.Blend != val)
      {
        cs.Blend = val;
        changed = true;
      }
      break;
    case GL_CULL_FACE:
      if (cs.CullFace != val)
      {
        cs.CullFace = val;
        changed = true;
      }
      break;
    case GL_DEPTH_TEST:
      if (cs.DepthTest != val)
      {
        cs.DepthTest = val;
        changed = true;
      }
      break;
#ifdef GL_LINE_SMOOTH
    case GL_LINE_SMOOTH:
      if (cs.LineSmooth != val)
      {
        cs.LineSmooth = val;
        changed = true;
      }
      break;
#endif
#ifdef GL_MULTISAMPLE
    case GL_MULTISAMPLE:
      if (cs.MultiSample != val)
      {
        cs.MultiSample = val;
        changed = true;
      }
      break;
#endif
    case GL_SCISSOR_TEST:
      if (cs.ScissorTest != val)
      {
        cs.ScissorTest = val;
        changed = true;
      }
      break;
    case GL_STENCIL_TEST:
      if (cs.StencilTest != val)
      {
        cs.StencilTest = val;
        changed = true;
      }
      break;
#ifdef GL_TEXTURE_CUBE_MAP_SEAMLESS
    case GL_TEXTURE_CUBE_MAP_SEAMLESS:
      if (cs.CubeMapSeamless != val)
      {
        cs.CubeMapSeamless = val;
        changed = true;
      }
      break;
#endif
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
  auto& cs = this->Stack.top();

  switch (cap)
  {
    case GL_BLEND:
      cs.Blend = params != 0;
      break;
    case GL_CULL_FACE:
      cs.CullFace = params != 0;
      break;
    case GL_DEPTH_TEST:
      cs.DepthTest = params != 0;
      break;
#ifdef GL_LINE_SMOOTH
    case GL_LINE_SMOOTH:
      cs.LineSmooth = params != 0;
      break;
#endif
#ifdef GL_MULTISAMPLE
    case GL_MULTISAMPLE:
      cs.MultiSample = params != 0;
      break;
#endif
    case GL_SCISSOR_TEST:
      cs.ScissorTest = params != 0;
      break;
    case GL_STENCIL_TEST:
      cs.StencilTest = params != 0;
      break;
#ifdef GL_TEXTURE_CUBE_MAP_SEAMLESS
    case GL_TEXTURE_CUBE_MAP_SEAMLESS:
      cs.CubeMapSeamless = params != 0;
      break;
#endif
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
void vtkOpenGLState::vtkglGetBooleanv(GLenum pname, GLboolean* params)
{
  vtkOpenGLCheckStateMacro();
  auto& cs = this->Stack.top();

  switch (pname)
  {
    case GL_DEPTH_WRITEMASK:
      *params = cs.DepthMask;
      break;
    case GL_COLOR_WRITEMASK:
      params[0] = cs.ColorMask[0];
      params[1] = cs.ColorMask[1];
      params[2] = cs.ColorMask[2];
      params[3] = cs.ColorMask[3];
      break;
    case GL_BLEND:
      *params = cs.Blend;
      break;
    case GL_CULL_FACE:
      *params = cs.CullFace;
      break;
    case GL_DEPTH_TEST:
      *params = cs.DepthTest;
      break;
#ifdef GL_LINE_SMOOTH
    case GL_LINE_SMOOTH:
      *params = cs.LineSmooth;
      break;
#endif
#ifdef GL_MULTISAMPLE
    case GL_MULTISAMPLE:
      *params = cs.MultiSample;
      break;
#endif
    case GL_SCISSOR_TEST:
      *params = cs.ScissorTest;
      break;
    case GL_STENCIL_TEST:
      *params = cs.StencilTest;
      break;
#ifdef GL_TEXTURE_CUBE_MAP_SEAMLESS
    case GL_TEXTURE_CUBE_MAP_SEAMLESS:
      *params = cs.CubeMapSeamless;
      break;
#endif
    default:
      ::glGetBooleanv(pname, params);
  }
  vtkCheckOpenGLErrorsWithStack("glGetBoolean");
}

void vtkOpenGLState::vtkglGetIntegerv(GLenum pname, GLint* params)
{
  vtkOpenGLCheckStateMacro();
  auto& cs = this->Stack.top();

  switch (pname)
  {
    case GL_VIEWPORT:
      params[0] = cs.Viewport[0];
      params[1] = cs.Viewport[1];
      params[2] = cs.Viewport[2];
      params[3] = cs.Viewport[3];
      break;
    case GL_SCISSOR_BOX:
      params[0] = cs.Scissor[0];
      params[1] = cs.Scissor[1];
      params[2] = cs.Scissor[2];
      params[3] = cs.Scissor[3];
      break;
    case GL_CULL_FACE_MODE:
      *params = cs.CullFaceMode;
      break;
    case GL_DEPTH_FUNC:
      *params = cs.DepthFunc;
      break;
    case GL_BLEND_SRC_RGB:
      *params = cs.BlendFunc[0];
      break;
    case GL_BLEND_SRC_ALPHA:
      *params = cs.BlendFunc[2];
      break;
    case GL_BLEND_DST_RGB:
      *params = cs.BlendFunc[1];
      break;
    case GL_BLEND_DST_ALPHA:
      *params = cs.BlendFunc[3];
      break;
#ifdef GL_ARB_tessellation_shader
    case GL_MAX_TESS_GEN_LEVEL:
      *params = this->MaxTessellationLevel;
      break;
#endif
    case GL_MAX_TEXTURE_SIZE:
      *params = this->MaxTextureSize;
      break;
    case GL_MAJOR_VERSION:
      *params = this->MajorVersion;
      break;
    case GL_MINOR_VERSION:
      *params = this->MinorVersion;
      break;
    default:
      ::glGetIntegerv(pname, params);
  }

  vtkCheckOpenGLErrorsWithStack("glGetInteger");
}

#ifdef GL_ES_VERSION_3_0
void vtkOpenGLState::vtkglGetDoublev(GLenum pname, double*)
{
  vtkGenericWarningMacro("glGetDouble not supported on OpenGL ES, requested: " << pname);
}
#else
void vtkOpenGLState::vtkglGetDoublev(GLenum pname, double* params)
{
  vtkOpenGLCheckStateMacro();
  ::glGetDoublev(pname, params);
  vtkCheckOpenGLErrorsWithStack("glGetDouble");
}
#endif

void vtkOpenGLState::vtkglGetFloatv(GLenum pname, GLfloat* params)
{
  vtkOpenGLCheckStateMacro();
  auto& cs = this->Stack.top();

  switch (pname)
  {
    case GL_COLOR_CLEAR_VALUE:
      params[0] = cs.ClearColor[0];
      params[1] = cs.ClearColor[1];
      params[2] = cs.ClearColor[2];
      params[3] = cs.ClearColor[3];
      break;
    default:
      ::glGetFloatv(pname, params);
  }
  vtkCheckOpenGLErrorsWithStack("glGetFloat");
}

void vtkOpenGLState::GetBlendFuncState(int* v)
{
  auto& cs = this->Stack.top();
  v[0] = cs.BlendFunc[0];
  v[1] = cs.BlendFunc[1];
  v[2] = cs.BlendFunc[2];
  v[3] = cs.BlendFunc[3];
}

bool vtkOpenGLState::GetEnumState(GLenum cap)
{
  vtkOpenGLCheckStateMacro();
  auto& cs = this->Stack.top();

  switch (cap)
  {
    case GL_BLEND:
      return cs.Blend;
    case GL_CULL_FACE:
      return cs.CullFace;
    case GL_DEPTH_TEST:
      return cs.DepthTest;
#ifdef GL_LINE_SMOOTH
    case GL_LINE_SMOOTH:
      return cs.LineSmooth;
#endif
#ifdef GL_MULTISAMPLE
    case GL_MULTISAMPLE:
      return cs.MultiSample;
#endif
    case GL_SCISSOR_TEST:
      return cs.ScissorTest;
    case GL_STENCIL_TEST:
      return cs.StencilTest;
#ifdef GL_TEXTURE_CUBE_MAP_SEAMLESS
    case GL_TEXTURE_CUBE_MAP_SEAMLESS:
      return cs.CubeMapSeamless;
#endif
    default:
      vtkGenericWarningMacro("Bad request for enum status");
  }
  return false;
}

void vtkOpenGLState::vtkglDisable(GLenum cap)
{
  this->SetEnumState(cap, false);
}

void vtkOpenGLState::vtkglPixelStorei(unsigned int cap, int val)
{
  vtkOpenGLCheckStateMacro();
  auto& cs = this->Stack.top();

#ifndef NO_CACHE
  bool changed = false;
#else
  bool changed = true;
#endif
  switch (cap)
  {
    case GL_PACK_ALIGNMENT:
      if (cs.PackAlignment != val)
      {
        cs.PackAlignment = val;
        changed = true;
      }
      break;
    case GL_UNPACK_ALIGNMENT:
      if (cs.UnpackAlignment != val)
      {
        cs.UnpackAlignment = val;
        changed = true;
      }
      break;
    case GL_UNPACK_ROW_LENGTH:
      if (cs.UnpackRowLength != val)
      {
        cs.UnpackRowLength = val;
        changed = true;
      }
      break;
    case GL_UNPACK_IMAGE_HEIGHT:
      if (cs.UnpackImageHeight != val)
      {
        cs.UnpackImageHeight = val;
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

  ::glPixelStorei(cap, val);
  vtkCheckOpenGLErrorsWithStack("glPixelStorei");
}

// make the recorded state match OpenGL driver
// Initialize makes OpenGL match the state
// This makes the state match OpenGL
void vtkOpenGLState::Reset()
{
  vtkOpenGLRenderUtilities::MarkDebugEvent("Resetting OpenGL State");
  this->ResetGLClearColorState();
  this->ResetGLClearDepthState();
  this->ResetGLDepthFuncState();
  this->ResetGLDepthMaskState();
  this->ResetGLColorMaskState();
  this->ResetGLViewportState();
  this->ResetGLScissorState();
  this->ResetGLBlendFuncState();
  this->ResetGLBlendEquationState();
  this->ResetGLCullFaceState();
  this->ResetGLActiveTexture();
  this->ResetFramebufferBindings();

  this->ResetEnumState(GL_BLEND);
  this->ResetEnumState(GL_CULL_FACE);
  this->ResetEnumState(GL_DEPTH_TEST);
#ifdef GL_LINE_SMOOTH
  this->ResetEnumState(GL_LINE_SMOOTH);
#endif
  this->ResetEnumState(GL_STENCIL_TEST);
  this->ResetEnumState(GL_SCISSOR_TEST);
#ifdef GL_TEXTURE_CUBE_MAP_SEAMLESS
  this->ResetEnumState(GL_TEXTURE_CUBE_MAP_SEAMLESS);
#endif
#ifdef GL_MULTISAMPLE
  this->ResetEnumState(GL_MULTISAMPLE);
#endif

  auto& cs = this->Stack.top();

  GLint ival;

#ifdef GL_POINT_SIZE
  ::glGetFloatv(GL_POINT_SIZE, &cs.PointSize);
#endif
  ::glGetFloatv(GL_LINE_WIDTH, &cs.LineWidth);

  ::glGetIntegerv(GL_PACK_ALIGNMENT, &cs.PackAlignment);
  ::glGetIntegerv(GL_UNPACK_ALIGNMENT, &cs.UnpackAlignment);
  ::glGetIntegerv(GL_UNPACK_ROW_LENGTH, &cs.UnpackRowLength);
  ::glGetIntegerv(GL_UNPACK_IMAGE_HEIGHT, &cs.UnpackImageHeight);

  ::glGetIntegerv(GL_STENCIL_BACK_WRITEMASK, &ival);
  cs.StencilMaskBack = static_cast<unsigned int>(ival);
  ::glGetIntegerv(GL_STENCIL_WRITEMASK, &ival);
  cs.StencilMaskFront = static_cast<unsigned int>(ival);

  ::glGetIntegerv(GL_STENCIL_BACK_FAIL, &ival);
  cs.StencilOpBack[0] = static_cast<unsigned int>(ival);
  ::glGetIntegerv(GL_STENCIL_BACK_PASS_DEPTH_FAIL, &ival);
  cs.StencilOpBack[1] = static_cast<unsigned int>(ival);
  ::glGetIntegerv(GL_STENCIL_BACK_PASS_DEPTH_PASS, &ival);
  cs.StencilOpBack[2] = static_cast<unsigned int>(ival);

  ::glGetIntegerv(GL_STENCIL_FAIL, &ival);
  cs.StencilOpFront[0] = static_cast<unsigned int>(ival);
  ::glGetIntegerv(GL_STENCIL_PASS_DEPTH_FAIL, &ival);
  cs.StencilOpFront[1] = static_cast<unsigned int>(ival);
  ::glGetIntegerv(GL_STENCIL_PASS_DEPTH_PASS, &ival);
  cs.StencilOpFront[2] = static_cast<unsigned int>(ival);

  ::glGetIntegerv(GL_STENCIL_BACK_FUNC, &ival);
  cs.StencilFuncBack[0] = static_cast<unsigned int>(ival);
  ::glGetIntegerv(GL_STENCIL_BACK_REF, &ival);
  cs.StencilFuncBack[1] = static_cast<unsigned int>(ival);
  ::glGetIntegerv(GL_STENCIL_BACK_VALUE_MASK, &ival);
  cs.StencilFuncBack[2] = static_cast<unsigned int>(ival);

  ::glGetIntegerv(GL_STENCIL_FUNC, &ival);
  cs.StencilFuncFront[0] = static_cast<unsigned int>(ival);
  ::glGetIntegerv(GL_STENCIL_REF, &ival);
  cs.StencilFuncFront[1] = static_cast<unsigned int>(ival);
  ::glGetIntegerv(GL_STENCIL_VALUE_MASK, &ival);
  cs.StencilFuncFront[2] = static_cast<unsigned int>(ival);

  ::glGetIntegerv(GL_CURRENT_PROGRAM, &cs.BoundProgram);
  ::glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &cs.BoundVAO);
  ::glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &cs.BoundArrayBuffer);
  ::glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &cs.BoundElementArrayBuffer);
  vtkOpenGLRenderUtilities::MarkDebugEvent("Finished Resetting OpenGL State");
}

void vtkOpenGLState::Push()
{
  vtkOpenGLRenderUtilities::MarkDebugEvent("Pushing OpenGL State");
  this->Stack.push(this->Stack.top());
  this->PushFramebufferBindings();
  vtkOpenGLRenderUtilities::MarkDebugEvent("Finished Pushing OpenGL State");
}

void vtkOpenGLState::Pop()
{
  vtkOpenGLRenderUtilities::MarkDebugEvent("Popping OpenGL State");
  this->Stack.pop();
  auto& cs = this->Stack.top();

  this->PopFramebufferBindings();

  cs.Blend ? ::glEnable(GL_BLEND) : ::glDisable(GL_BLEND);
  cs.DepthTest ? ::glEnable(GL_DEPTH_TEST) : ::glDisable(GL_DEPTH_TEST);
#ifdef GL_LINE_SMOOTH
  cs.LineSmooth ? ::glEnable(GL_LINE_SMOOTH) : ::glDisable(GL_LINE_SMOOTH);
#endif
  cs.StencilTest ? ::glEnable(GL_STENCIL_TEST) : ::glDisable(GL_STENCIL_TEST);
  cs.ScissorTest ? ::glEnable(GL_SCISSOR_TEST) : ::glDisable(GL_SCISSOR_TEST);
  cs.CullFace ? ::glEnable(GL_CULL_FACE) : ::glDisable(GL_CULL_FACE);
#ifdef GL_TEXTURE_CUBE_MAP_SEAMLESS
  cs.CubeMapSeamless ? ::glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS)
                     : ::glDisable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
#endif
#ifdef GL_MULTISAMPLE
  cs.MultiSample = glIsEnabled(GL_MULTISAMPLE) == GL_TRUE;
#endif

  ::glBlendFuncSeparate(cs.BlendFunc[0], cs.BlendFunc[1], cs.BlendFunc[2], cs.BlendFunc[3]);

  ::glClearColor(cs.ClearColor[0], cs.ClearColor[1], cs.ClearColor[2], cs.ClearColor[3]);

  ::glColorMask(cs.ColorMask[0], cs.ColorMask[1], cs.ColorMask[2], cs.ColorMask[3]);

  ::glDepthFunc(cs.DepthFunc);

#ifdef GL_ES_VERSION_3_0
  ::glClearDepthf(cs.ClearDepth);
#else
  ::glClearDepth(cs.ClearDepth);
#endif

  ::glDepthMask(cs.DepthMask);

#ifndef GL_ES_VERSION_3_0
  ::glPointSize(cs.PointSize);
#endif
  ::glLineWidth(cs.LineWidth);

  ::glPixelStorei(GL_PACK_ALIGNMENT, cs.PackAlignment);
  ::glPixelStorei(GL_UNPACK_ALIGNMENT, cs.UnpackAlignment);
  ::glPixelStorei(GL_UNPACK_ROW_LENGTH, cs.UnpackRowLength);
  ::glPixelStorei(GL_UNPACK_IMAGE_HEIGHT, cs.UnpackImageHeight);

  ::glStencilMaskSeparate(GL_FRONT, cs.StencilMaskFront);
  ::glStencilMaskSeparate(GL_BACK, cs.StencilMaskBack);
  ::glStencilOpSeparate(GL_FRONT, cs.StencilOpFront[0], cs.StencilOpFront[1], cs.StencilOpFront[2]);
  ::glStencilOpSeparate(GL_BACK, cs.StencilOpBack[0], cs.StencilOpBack[1], cs.StencilOpBack[2]);
  ::glStencilFuncSeparate(GL_FRONT, cs.StencilFuncFront[0],
    static_cast<GLint>(cs.StencilFuncFront[1]), cs.StencilFuncFront[2]);
  ::glStencilFuncSeparate(GL_BACK, cs.StencilFuncBack[0], static_cast<GLint>(cs.StencilFuncBack[1]),
    cs.StencilFuncBack[2]);

  ::glViewport(cs.Viewport[0], cs.Viewport[1], cs.Viewport[2], cs.Viewport[3]);

  ::glScissor(cs.Scissor[0], cs.Scissor[1], cs.Scissor[2], cs.Scissor[3]);

  ::glCullFace(cs.CullFaceMode);

  ::glBlendEquationSeparate(cs.BlendEquationValue1, cs.BlendEquationValue2);

  if (this->ShaderCache)
  {
    this->ShaderCache->ReleaseCurrentShader();
  }
  ::glUseProgram(cs.BoundProgram);

  ::glActiveTexture(cs.ActiveTexture);

  ::glBindVertexArray(cs.BoundVAO);
  ::glBindBuffer(GL_ARRAY_BUFFER, cs.BoundArrayBuffer);
  ::glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cs.BoundElementArrayBuffer);
  vtkOpenGLRenderUtilities::MarkDebugEvent("Finished Popping OpenGL State");
}

// make the hardware openglstate match the
// state ivars
void vtkOpenGLState::Initialize(vtkOpenGLRenderWindow*)
{
  this->TextureUnitManager->Initialize();
  this->InitializeTextureInternalFormats();
  auto& cs = this->Stack.top();

  cs.Blend ? ::glEnable(GL_BLEND) : ::glDisable(GL_BLEND);
  cs.CullFace ? ::glEnable(GL_CULL_FACE) : ::glDisable(GL_CULL_FACE);
  cs.DepthTest ? ::glEnable(GL_DEPTH_TEST) : ::glDisable(GL_DEPTH_TEST);
#ifdef GL_LINE_SMOOTH
  cs.LineSmooth ? ::glEnable(GL_LINE_SMOOTH) : ::glDisable(GL_LINE_SMOOTH);
#endif
#ifdef GL_MULTISAMPLE
  cs.MultiSample = glIsEnabled(GL_MULTISAMPLE) == GL_TRUE;
#endif
  cs.StencilTest ? ::glEnable(GL_STENCIL_TEST) : ::glDisable(GL_STENCIL_TEST);
  cs.ScissorTest ? ::glEnable(GL_SCISSOR_TEST) : ::glDisable(GL_SCISSOR_TEST);

  // initialize blending for transparency
  ::glBlendFuncSeparate(cs.BlendFunc[0], cs.BlendFunc[1], cs.BlendFunc[2], cs.BlendFunc[3]);

  ::glClearColor(cs.ClearColor[0], cs.ClearColor[1], cs.ClearColor[2], cs.ClearColor[3]);

  ::glColorMask(cs.ColorMask[0], cs.ColorMask[1], cs.ColorMask[2], cs.ColorMask[3]);

  ::glDepthFunc(cs.DepthFunc);

#ifndef GL_ES_VERSION_3_0
  ::glPointSize(cs.PointSize);
#endif
  ::glLineWidth(cs.LineWidth);

  ::glPixelStorei(GL_PACK_ALIGNMENT, cs.PackAlignment);
  ::glPixelStorei(GL_UNPACK_ALIGNMENT, cs.UnpackAlignment);
  ::glPixelStorei(GL_UNPACK_ROW_LENGTH, cs.UnpackRowLength);
  ::glPixelStorei(GL_UNPACK_IMAGE_HEIGHT, cs.UnpackImageHeight);

  ::glStencilMaskSeparate(GL_FRONT, cs.StencilMaskFront);
  ::glStencilMaskSeparate(GL_BACK, cs.StencilMaskBack);
  ::glStencilOpSeparate(GL_FRONT, cs.StencilOpFront[0], cs.StencilOpFront[1], cs.StencilOpFront[2]);
  ::glStencilOpSeparate(GL_BACK, cs.StencilOpBack[0], cs.StencilOpBack[1], cs.StencilOpBack[2]);
  ::glStencilFuncSeparate(GL_FRONT, cs.StencilFuncFront[0],
    static_cast<GLint>(cs.StencilFuncFront[1]), cs.StencilFuncFront[2]);
  ::glStencilFuncSeparate(GL_BACK, cs.StencilFuncBack[0], static_cast<GLint>(cs.StencilFuncBack[1]),
    cs.StencilFuncBack[2]);

#ifdef GL_ES_VERSION_3_0
  ::glClearDepthf(cs.ClearDepth);
#else
  ::glClearDepth(cs.ClearDepth);
#endif

  ::glDepthMask(cs.DepthMask);

  ::glViewport(cs.Viewport[0], cs.Viewport[1], cs.Viewport[2], cs.Viewport[3]);

  ::glScissor(cs.Scissor[0], cs.Scissor[1], cs.Scissor[2], cs.Scissor[3]);

  ::glCullFace(cs.CullFaceMode);

  ::glBlendEquationSeparate(cs.BlendEquationValue1, cs.BlendEquationValue2);

  // strictly query values below here
#ifdef GL_ARB_tessellation_shader
  ::glGetIntegerv(GL_MAX_TESS_GEN_LEVEL, &this->MaxTessellationLevel);
#endif
  ::glGetIntegerv(GL_MAX_TEXTURE_SIZE, &this->MaxTextureSize);
  ::glGetIntegerv(GL_MAJOR_VERSION, &this->MajorVersion);
  ::glGetIntegerv(GL_MINOR_VERSION, &this->MinorVersion);
  char const* tmp = reinterpret_cast<const char*>(::glGetString(GL_VENDOR));
  this->Vendor = tmp ? tmp : std::string();
  tmp = reinterpret_cast<char const*>(::glGetString(GL_RENDERER));
  this->Renderer = tmp ? tmp : std::string();
  tmp = reinterpret_cast<char const*>(::glGetString(GL_VERSION));
  this->Version = tmp ? tmp : std::string();

  this->ResetFramebufferBindings();
}

void vtkOpenGLState::GetCurrentDrawFramebufferState(
  unsigned int& drawBinding, unsigned int& drawBuffer)
{
  auto& cs = this->Stack.top();
  drawBinding = cs.DrawBinding.Binding;
  drawBuffer = cs.DrawBinding.DrawBuffers[0];
}

void vtkOpenGLState::ResetFramebufferBindings()
{
  auto& cs = this->Stack.top();
  ::glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, (int*)&cs.DrawBinding.Binding);
#ifdef GL_DRAW_BUFFER
  ::glGetIntegerv(GL_DRAW_BUFFER, (int*)&cs.DrawBinding.DrawBuffers[0]);
#endif

  ::glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, (int*)&cs.ReadBinding.Binding);
  ::glGetIntegerv(GL_READ_BUFFER, (int*)&cs.ReadBinding.ReadBuffer);
}

void vtkOpenGLState::ResetGLClearColorState()
{
  auto& cs = this->Stack.top();
  GLfloat fparams[4];
  ::glGetFloatv(GL_COLOR_CLEAR_VALUE, fparams);
  cs.ClearColor[0] = fparams[0];
  cs.ClearColor[1] = fparams[1];
  cs.ClearColor[2] = fparams[2];
  cs.ClearColor[3] = fparams[3];
}

void vtkOpenGLState::ResetGLClearDepthState()
{
  auto& cs = this->Stack.top();
  GLfloat fparams;
  ::glGetFloatv(GL_DEPTH_CLEAR_VALUE, &fparams);
  cs.ClearDepth = fparams;
}

void vtkOpenGLState::ResetGLDepthFuncState()
{
  auto& cs = this->Stack.top();
  GLint iparams;
  ::glGetIntegerv(GL_DEPTH_FUNC, &iparams);
  cs.DepthFunc = static_cast<unsigned int>(iparams);
}

void vtkOpenGLState::ResetGLDepthMaskState()
{
  auto& cs = this->Stack.top();
  GLboolean params;
  ::glGetBooleanv(GL_DEPTH_WRITEMASK, &params);
  cs.DepthMask = params;
}

void vtkOpenGLState::ResetGLColorMaskState()
{
  auto& cs = this->Stack.top();
  GLboolean params[4];
  ::glGetBooleanv(GL_COLOR_WRITEMASK, params);
  cs.ColorMask[0] = params[0];
  cs.ColorMask[1] = params[1];
  cs.ColorMask[2] = params[2];
  cs.ColorMask[3] = params[3];
}

void vtkOpenGLState::ResetGLViewportState()
{
  auto& cs = this->Stack.top();
  GLint iparams[4];
  ::glGetIntegerv(GL_VIEWPORT, iparams);
  cs.Viewport[0] = iparams[0];
  cs.Viewport[1] = iparams[1];
  cs.Viewport[2] = iparams[2];
  cs.Viewport[3] = iparams[3];
}

void vtkOpenGLState::ResetGLScissorState()
{
  auto& cs = this->Stack.top();
  GLint iparams[4];
  ::glGetIntegerv(GL_SCISSOR_BOX, iparams);
  cs.Scissor[0] = iparams[0];
  cs.Scissor[1] = iparams[1];
  cs.Scissor[2] = iparams[2];
  cs.Scissor[3] = iparams[3];
}

void vtkOpenGLState::ResetGLBlendFuncState()
{
  auto& cs = this->Stack.top();
  GLint iparams;
  ::glGetIntegerv(GL_BLEND_SRC_RGB, &iparams);
  cs.BlendFunc[0] = static_cast<unsigned int>(iparams);
  ::glGetIntegerv(GL_BLEND_SRC_ALPHA, &iparams);
  cs.BlendFunc[2] = static_cast<unsigned int>(iparams);
  ::glGetIntegerv(GL_BLEND_DST_RGB, &iparams);
  cs.BlendFunc[1] = static_cast<unsigned int>(iparams);
  ::glGetIntegerv(GL_BLEND_DST_ALPHA, &iparams);
  cs.BlendFunc[3] = static_cast<unsigned int>(iparams);
}

void vtkOpenGLState::ResetGLBlendEquationState()
{
  auto& cs = this->Stack.top();
  GLint iparams;
  ::glGetIntegerv(GL_BLEND_EQUATION_RGB, &iparams);
  cs.BlendEquationValue1 = static_cast<unsigned int>(iparams);
  ::glGetIntegerv(GL_BLEND_EQUATION_ALPHA, &iparams);
  cs.BlendEquationValue2 = static_cast<unsigned int>(iparams);
}

void vtkOpenGLState::ResetGLCullFaceState()
{
  auto& cs = this->Stack.top();
  GLint iparams;
  ::glGetIntegerv(GL_CULL_FACE_MODE, &iparams);
  cs.CullFaceMode = static_cast<unsigned int>(iparams);
}

void vtkOpenGLState::ResetGLActiveTexture()
{
  auto& cs = this->Stack.top();
  GLint iparams;
  ::glGetIntegerv(GL_ACTIVE_TEXTURE, &iparams);
  cs.ActiveTexture = static_cast<unsigned int>(iparams);
}

void vtkOpenGLState::vtkglClear(GLbitfield val)
{
  ::glClear(val);
}

void vtkOpenGLState::vtkglBlitFramebuffer(int srcX0, int srcY0, int srcX1, int srcY1, int dstX0,
  int dstY0, int dstX1, int dstY1, unsigned int mask, unsigned int filter)
{
  // ON APPLE MACOS you must turn off scissor test for DEPTH blits to work
  vtkOpenGLState::ScopedglEnableDisable stsaver(this, GL_SCISSOR_TEST);
  this->vtkglDisable(GL_SCISSOR_TEST);

  ::glBlitFramebuffer(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter);
  vtkCheckOpenGLErrorsWithStack("glBlitFramebuffer");
}

//------------------------------------------------------------------------------
// Description:
// Returns its texture unit manager object.
vtkTextureUnitManager* vtkOpenGLState::GetTextureUnitManager()
{
  return this->TextureUnitManager;
}

void vtkOpenGLState::SetTextureUnitManager(vtkTextureUnitManager* tum)
{
  if (this->TextureUnitManager == tum)
  {
    return;
  }
  if (tum)
  {
    tum->Register(nullptr);
  }
  if (this->TextureUnitManager)
  {
    this->TextureUnitManager->Delete();
  }
  this->TextureUnitManager = tum;
}

void vtkOpenGLState::ActivateTexture(vtkTextureObject* texture)
{
  // Only add if it isn't already there
  typedef std::map<const vtkTextureObject*, int>::const_iterator TRIter;
  TRIter found = this->TextureResourceIds.find(texture);
  if (found == this->TextureResourceIds.end())
  {
    int activeUnit = this->GetTextureUnitManager()->Allocate();
    if (activeUnit < 0)
    {
      vtkGenericWarningMacro("Hardware does not support the number of textures defined.");
      return;
    }
    this->TextureResourceIds.insert(std::make_pair(texture, activeUnit));
    this->vtkglActiveTexture(GL_TEXTURE0 + activeUnit);
  }
  else
  {
    this->vtkglActiveTexture(GL_TEXTURE0 + found->second);
  }
}

void vtkOpenGLState::DeactivateTexture(vtkTextureObject* texture)
{
  // Only deactivate if it isn't already there
  typedef std::map<const vtkTextureObject*, int>::iterator TRIter;
  TRIter found = this->TextureResourceIds.find(texture);
  if (found != this->TextureResourceIds.end())
  {
    this->GetTextureUnitManager()->Free(found->second);
    this->TextureResourceIds.erase(found);
  }
}

int vtkOpenGLState::GetTextureUnitForTexture(vtkTextureObject* texture)
{
  // Only deactivate if it isn't already there
  typedef std::map<const vtkTextureObject*, int>::const_iterator TRIter;
  TRIter found = this->TextureResourceIds.find(texture);
  if (found != this->TextureResourceIds.end())
  {
    return found->second;
  }

  return -1;
}

void vtkOpenGLState::VerifyNoActiveTextures()
{
  if (!this->TextureResourceIds.empty())
  {
    vtkGenericWarningMacro("There are still active textures when there should not be.");
    typedef std::map<const vtkTextureObject*, int>::const_iterator TRIter;
    TRIter found = this->TextureResourceIds.begin();
    for (; found != this->TextureResourceIds.end(); ++found)
    {
      vtkGenericWarningMacro(
        "Leaked for texture object: " << const_cast<vtkTextureObject*>(found->first));
    }
  }
}

vtkStandardNewMacro(vtkOpenGLState);

void vtkOpenGLState::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "MajorVersion: " << this->MajorVersion << '\n'
     << indent << "MinorVersion: " << this->MinorVersion << '\n'
     << indent << "MaxTessellationLevel: " << this->MaxTessellationLevel << '\n'
     << indent << "MaxTextureSize: " << this->MaxTextureSize << '\n'
     << indent << "Vendor: " << this->Vendor << '\n'
     << indent << "Renderer: " << this->Renderer << '\n'
     << indent << "Version: " << this->Version << '\n';
}

vtkCxxSetObjectMacro(vtkOpenGLState, VBOCache, vtkOpenGLVertexBufferObjectCache);

// initialize all state values. This is important so that in
// ::Initialize we can just set the state to the current
// values (knowing that they are set). The reason we want
// Initialize to set to the current values is to reduce
// OpenGL churn in cases where applications call Initialize
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
  : MajorVersion(-1)
  , MinorVersion(-1)
  , MaxTessellationLevel(-1)
  , MaxTextureSize(-1)
{
  this->ShaderCache = vtkOpenGLShaderCache::New();
  this->VBOCache = vtkOpenGLVertexBufferObjectCache::New();

  this->TextureUnitManager = vtkTextureUnitManager::New();

  this->Stack.emplace();

  auto& cs = this->Stack.top();

  cs.Blend = true;
  cs.DepthTest = true;
  cs.StencilTest = false;
  cs.ScissorTest = true;
  cs.CullFace = false;
  cs.CubeMapSeamless = false;
  cs.LineSmooth = false;
  cs.MultiSample = false;

  cs.LineWidth = 1.0;
  cs.PointSize = 1.0;

  cs.StencilMaskFront = 0xFF;
  cs.StencilMaskBack = 0xFF;

  cs.StencilOpFront[0] = GL_KEEP;
  cs.StencilOpFront[1] = GL_KEEP;
  cs.StencilOpFront[2] = GL_KEEP;
  cs.StencilOpBack[0] = GL_KEEP;
  cs.StencilOpBack[1] = GL_KEEP;
  cs.StencilOpBack[2] = GL_KEEP;

  cs.StencilFuncFront[0] = GL_ALWAYS;
  cs.StencilFuncFront[1] = 0;
  cs.StencilFuncFront[2] = 0xFF;
  cs.StencilFuncBack[0] = GL_ALWAYS;
  cs.StencilFuncBack[1] = 0;
  cs.StencilFuncBack[2] = 0xFF;

  cs.PackAlignment = 1;
  cs.UnpackAlignment = 1;
  cs.UnpackRowLength = 0;
  cs.UnpackImageHeight = 0;

  // initialize blending for transparency
  cs.BlendFunc[0] = GL_SRC_ALPHA;
  cs.BlendFunc[1] = GL_ONE_MINUS_SRC_ALPHA;
  cs.BlendFunc[2] = GL_ONE;
  cs.BlendFunc[3] = GL_ONE_MINUS_SRC_ALPHA;

  cs.ClearColor[0] = 0.0;
  cs.ClearColor[1] = 0.0;
  cs.ClearColor[2] = 0.0;
  cs.ClearColor[3] = 0.0;

  cs.ColorMask[0] = GL_TRUE;
  cs.ColorMask[1] = GL_TRUE;
  cs.ColorMask[2] = GL_TRUE;
  cs.ColorMask[3] = GL_TRUE;

  cs.DepthFunc = GL_LEQUAL;

  cs.ClearDepth = 1.0;

  cs.DepthMask = GL_TRUE;

  cs.Viewport[0] = 0;
  cs.Viewport[1] = 0;
  cs.Viewport[2] = 1;
  cs.Viewport[3] = 1;

  cs.Scissor[0] = 0;
  cs.Scissor[1] = 0;
  cs.Scissor[2] = 1;
  cs.Scissor[3] = 1;

  cs.CullFaceMode = GL_BACK;
  cs.ActiveTexture = GL_TEXTURE0;

  cs.BlendEquationValue1 = GL_FUNC_ADD;
  cs.BlendEquationValue2 = GL_FUNC_ADD;

  cs.DrawBinding.Binding = 0;
  cs.ReadBinding.Binding = 0;
  cs.DrawBinding.DrawBuffers[0] = GL_BACK_LEFT;
  for (int i = 1; i < 10; ++i)
  {
    cs.DrawBinding.DrawBuffers[i] = GL_NONE;
  }
  cs.ReadBinding.ReadBuffer = GL_BACK_LEFT;
}

vtkOpenGLState::~vtkOpenGLState()
{
  this->TextureResourceIds.clear();
  this->SetTextureUnitManager(nullptr);
  this->VBOCache->Delete();
  this->ShaderCache->Delete();
}

void vtkOpenGLState::PushDrawFramebufferBinding()
{
  auto& cs = this->Stack.top();
  this->DrawBindings.push_back(cs.DrawBinding);
}

void vtkOpenGLState::PushReadFramebufferBinding()
{
  auto& cs = this->Stack.top();
  this->ReadBindings.push_back(cs.ReadBinding);
}

void vtkOpenGLState::PopDrawFramebufferBinding()
{
  if (!this->DrawBindings.empty())
  {
    auto& cs = this->Stack.top();
    BufferBindingState& bbs = this->DrawBindings.back();
    ::glBindFramebuffer(GL_DRAW_FRAMEBUFFER, bbs.GetBinding());
    cs.DrawBinding = bbs;
    this->DrawBindings.pop_back();
  }
  else
  {
    vtkGenericWarningMacro("Attempt to pop framebuffer beyond beginning of the stack.");
    abort();
  }
}

void vtkOpenGLState::PopReadFramebufferBinding()
{
  if (!this->ReadBindings.empty())
  {
    auto& cs = this->Stack.top();
    BufferBindingState& bbs = this->ReadBindings.back();
    ::glBindFramebuffer(GL_READ_FRAMEBUFFER, bbs.GetBinding());
    cs.ReadBinding = bbs;
    this->ReadBindings.pop_back();
  }
  else
  {
    vtkGenericWarningMacro("Attempt to pop framebuffer beyond beginning of the stack.");
    abort();
  }
}

int vtkOpenGLState::GetDefaultTextureInternalFormat(
  int vtktype, int numComponents, bool needInt, bool needFloat, bool needSRGB)
{
  // 0 = none
  // 1 = float
  // 2 = int
  int dtypeCount = sizeof(this->TextureInternalFormats) / sizeof(this->TextureInternalFormats[0]);
  if (vtktype >= dtypeCount)
  {
    return 0;
  }
  if (needInt)
  {
    return this->TextureInternalFormats[vtktype][2][numComponents];
  }
  if (needFloat)
  {
    return this->TextureInternalFormats[vtktype][1][numComponents];
  }
  int result = this->TextureInternalFormats[vtktype][0][numComponents];
  if (needSRGB)
  {
    switch (result)
    {
#ifdef GL_ES_VERSION_3_0
      case GL_RGB:
        result = GL_SRGB8;
        break;
      case GL_RGBA:
        result = GL_SRGB8_ALPHA8;
        break;
#else
      case GL_RGB:
        result = GL_SRGB;
        break;
      case GL_RGBA:
        result = GL_SRGB_ALPHA;
        break;
#endif
      case GL_RGB8:
        result = GL_SRGB8;
        break;
      case GL_RGBA8:
        result = GL_SRGB8_ALPHA8;
        break;
      default:
        break;
    }
  }
  return result;
}

void vtkOpenGLState::InitializeTextureInternalFormats()
{
  // 0 = none
  // 1 = float
  // 2 = int

  // initialize to zero
  int dtypeCount = sizeof(this->TextureInternalFormats) / sizeof(this->TextureInternalFormats[0]);
  for (int dtype = 0; dtype < dtypeCount; dtype++)
  {
    for (int ctype = 0; ctype < 3; ctype++)
    {
      for (int comp = 0; comp < 5; comp++)
      {
        this->TextureInternalFormats[dtype][ctype][comp] = 0;
      }
    }
  }

  this->TextureInternalFormats[VTK_VOID][0][1] = GL_DEPTH_COMPONENT;

#ifdef GL_R8
  this->TextureInternalFormats[VTK_UNSIGNED_CHAR][0][1] = GL_R8;
  this->TextureInternalFormats[VTK_UNSIGNED_CHAR][0][2] = GL_RG8;
  this->TextureInternalFormats[VTK_UNSIGNED_CHAR][0][3] = GL_RGB8;
  this->TextureInternalFormats[VTK_UNSIGNED_CHAR][0][4] = GL_RGBA8;
#else
  this->TextureInternalFormats[VTK_UNSIGNED_CHAR][0][1] = GL_LUMINANCE;
  this->TextureInternalFormats[VTK_UNSIGNED_CHAR][0][2] = GL_LUMINANCE_ALPHA;
  this->TextureInternalFormats[VTK_UNSIGNED_CHAR][0][3] = GL_RGB;
  this->TextureInternalFormats[VTK_UNSIGNED_CHAR][0][4] = GL_RGBA;
#endif

#ifdef GL_R16
  this->TextureInternalFormats[VTK_UNSIGNED_SHORT][0][1] = GL_R16;
  this->TextureInternalFormats[VTK_UNSIGNED_SHORT][0][2] = GL_RG16;
  this->TextureInternalFormats[VTK_UNSIGNED_SHORT][0][3] = GL_RGB16;
  this->TextureInternalFormats[VTK_UNSIGNED_SHORT][0][4] = GL_RGBA16;
#endif

#ifdef GL_R8_SNORM
  this->TextureInternalFormats[VTK_SIGNED_CHAR][0][1] = GL_R8_SNORM;
  this->TextureInternalFormats[VTK_SIGNED_CHAR][0][2] = GL_RG8_SNORM;
  this->TextureInternalFormats[VTK_SIGNED_CHAR][0][3] = GL_RGB8_SNORM;
  this->TextureInternalFormats[VTK_SIGNED_CHAR][0][4] = GL_RGBA8_SNORM;
#endif

#ifdef GL_R16_SNORM
  this->TextureInternalFormats[VTK_SHORT][0][1] = GL_R16_SNORM;
  this->TextureInternalFormats[VTK_SHORT][0][2] = GL_RG16_SNORM;
  this->TextureInternalFormats[VTK_SHORT][0][3] = GL_RGB16_SNORM;
  this->TextureInternalFormats[VTK_SHORT][0][4] = GL_RGBA16_SNORM;
#endif

#ifdef GL_R8I
  this->TextureInternalFormats[VTK_SIGNED_CHAR][2][1] = GL_R8I;
  this->TextureInternalFormats[VTK_SIGNED_CHAR][2][2] = GL_RG8I;
  this->TextureInternalFormats[VTK_SIGNED_CHAR][2][3] = GL_RGB8I;
  this->TextureInternalFormats[VTK_SIGNED_CHAR][2][4] = GL_RGBA8I;
  this->TextureInternalFormats[VTK_UNSIGNED_CHAR][2][1] = GL_R8UI;
  this->TextureInternalFormats[VTK_UNSIGNED_CHAR][2][2] = GL_RG8UI;
  this->TextureInternalFormats[VTK_UNSIGNED_CHAR][2][3] = GL_RGB8UI;
  this->TextureInternalFormats[VTK_UNSIGNED_CHAR][2][4] = GL_RGBA8UI;

  this->TextureInternalFormats[VTK_SHORT][2][1] = GL_R16I;
  this->TextureInternalFormats[VTK_SHORT][2][2] = GL_RG16I;
  this->TextureInternalFormats[VTK_SHORT][2][3] = GL_RGB16I;
  this->TextureInternalFormats[VTK_SHORT][2][4] = GL_RGBA16I;
  this->TextureInternalFormats[VTK_UNSIGNED_SHORT][2][1] = GL_R16UI;
  this->TextureInternalFormats[VTK_UNSIGNED_SHORT][2][2] = GL_RG16UI;
  this->TextureInternalFormats[VTK_UNSIGNED_SHORT][2][3] = GL_RGB16UI;
  this->TextureInternalFormats[VTK_UNSIGNED_SHORT][2][4] = GL_RGBA16UI;

  this->TextureInternalFormats[VTK_INT][2][1] = GL_R32I;
  this->TextureInternalFormats[VTK_INT][2][2] = GL_RG32I;
  this->TextureInternalFormats[VTK_INT][2][3] = GL_RGB32I;
  this->TextureInternalFormats[VTK_INT][2][4] = GL_RGBA32I;
  this->TextureInternalFormats[VTK_UNSIGNED_INT][2][1] = GL_R32UI;
  this->TextureInternalFormats[VTK_UNSIGNED_INT][2][2] = GL_RG32UI;
  this->TextureInternalFormats[VTK_UNSIGNED_INT][2][3] = GL_RGB32UI;
  this->TextureInternalFormats[VTK_UNSIGNED_INT][2][4] = GL_RGBA32UI;
#endif

  // on mesa we may not have float textures even though we think we do
  // this is due to Mesa being impacted by a patent issue with SGI
  // that is due to expire in the US in summer 2018
#ifndef GL_ES_VERSION_3_0
  const char* glVersion = reinterpret_cast<const char*>(glGetString(GL_VERSION));
  if (glVersion && strstr(glVersion, "Mesa") != nullptr && !GLEW_ARB_texture_float)
  {
    // mesa without float support cannot even use
    // uchar textures with underlying float data
    // so pretty much anything with float data
    // is out of luck so return
    return;
  }
#endif

#ifdef GL_R32F
  this->TextureInternalFormats[VTK_FLOAT][1][1] = GL_R32F;
  this->TextureInternalFormats[VTK_FLOAT][1][2] = GL_RG32F;
  this->TextureInternalFormats[VTK_FLOAT][1][3] = GL_RGB32F;
  this->TextureInternalFormats[VTK_FLOAT][1][4] = GL_RGBA32F;

  this->TextureInternalFormats[VTK_SHORT][1][1] = GL_R32F;
  this->TextureInternalFormats[VTK_SHORT][1][2] = GL_RG32F;
  this->TextureInternalFormats[VTK_SHORT][1][3] = GL_RGB32F;
  this->TextureInternalFormats[VTK_SHORT][1][4] = GL_RGBA32F;
#endif
}
VTK_ABI_NAMESPACE_END
