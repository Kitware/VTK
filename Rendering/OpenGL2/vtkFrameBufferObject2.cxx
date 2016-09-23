/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFrameBufferObject2.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkFrameBufferObject2.h"

#include "vtk_glew.h"

#include "vtkObjectFactory.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkTextureObject.h"
#include "vtkRenderbuffer.h"
#include "vtkPixelBufferObject.h"
#include "vtkOpenGLError.h"


#include <cassert>
#include <vector>
using std::vector;

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkFrameBufferObject2);

//----------------------------------------------------------------------------
vtkFrameBufferObject2::vtkFrameBufferObject2()
{
  this->FBOIndex = 0;
  this->PreviousDrawFBO = 0;
  this->PreviousReadFBO = 0;
  this->PreviousDrawBuffer = GL_NONE;
  this->PreviousReadBuffer = GL_NONE;
  this->LastViewportSize[0] = -1;
  this->LastViewportSize[1] = -1;
}

//----------------------------------------------------------------------------
vtkFrameBufferObject2::~vtkFrameBufferObject2()
{
  this->DestroyFBO();
}

//----------------------------------------------------------------------------
void vtkFrameBufferObject2::CreateFBO()
{
  this->FBOIndex=0;
  GLuint temp;
  glGenFramebuffers(1,&temp);
  vtkOpenGLCheckErrorMacro("failed at glGenFramebuffers");
  this->FBOIndex=temp;
}

//----------------------------------------------------------------------------
void vtkFrameBufferObject2::DestroyFBO()
{
  // because we don't hold a reference to the render
  // context we don't have any control on when it is
  // destroyed. In fact it may be destroyed before
  // we are(eg smart pointers), in which case we should
  // do nothing.
  if (this->Context && (this->FBOIndex!=0))
  {
    GLuint fbo=static_cast<GLuint>(this->FBOIndex);
    glDeleteFramebuffers(1,&fbo);
    vtkOpenGLCheckErrorMacro("failed at glDeleteFramebuffers");
    this->FBOIndex=0;
  }
}

//----------------------------------------------------------------------------
bool vtkFrameBufferObject2::IsSupported(vtkRenderWindow *win)
{
  vtkOpenGLRenderWindow *renWin=vtkOpenGLRenderWindow::SafeDownCast(win);
  if(renWin!=0)
  {
    return true;
  }
  return false;
}

//----------------------------------------------------------------------------
bool vtkFrameBufferObject2::LoadRequiredExtensions(
  vtkRenderWindow *vtkNotUsed(win))
{
  return true;
}

//----------------------------------------------------------------------------
void vtkFrameBufferObject2::SetContext(vtkRenderWindow *renWin)
{
  // avoid pointless re-assignment
  if (this->Context==renWin)
  {
    return;
  }
  // free previous resources
  this->DestroyFBO();
  this->Context = NULL;
  this->Modified();
  // all done if assigned null
  if (!renWin)
  {
    return;
  }
  // check for support
  vtkOpenGLRenderWindow *context
    = dynamic_cast<vtkOpenGLRenderWindow*>(renWin);
  if ( !context
    || !this->LoadRequiredExtensions(renWin))
  {
    vtkErrorMacro("Context does not support the required extensions");
    return;
  }
  // intialize
  this->Context=renWin;
  this->Context->MakeCurrent();
  this->CreateFBO();
}

//----------------------------------------------------------------------------
vtkRenderWindow *vtkFrameBufferObject2::GetContext()
{
  return this->Context;
}

//----------------------------------------------------------------------------
void vtkFrameBufferObject2::SaveCurrentBindings()
{
  glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, (int*)&this->PreviousDrawFBO);
  glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, (int*)&this->PreviousReadFBO);
}

//----------------------------------------------------------------------------
void vtkFrameBufferObject2::SaveCurrentBuffers()
{
#ifdef GL_DRAW_BUFFER
  glGetIntegerv(GL_DRAW_BUFFER, (int*)&this->PreviousDrawBuffer);
#endif
  glGetIntegerv(GL_READ_BUFFER, (int*)&this->PreviousReadBuffer);
}

//----------------------------------------------------------------------------
void vtkFrameBufferObject2::RestorePreviousBuffers(unsigned int mode)
{
  switch((GLenum)mode)
  {
    case GL_FRAMEBUFFER:
      glDrawBuffer((GLenum)this->PreviousDrawBuffer);
      vtkOpenGLCheckErrorMacro("failed at glDrawBuffer");

      glReadBuffer((GLenum)this->PreviousReadBuffer);
      vtkOpenGLCheckErrorMacro("failed at glReadBuffer");
      break;

    case GL_DRAW_FRAMEBUFFER:
      glDrawBuffer((GLenum)this->PreviousDrawBuffer);
      vtkOpenGLCheckErrorMacro("failed at glDrawBuffer");
      break;

    case GL_READ_FRAMEBUFFER:
      glReadBuffer((GLenum)this->PreviousReadBuffer);
      vtkOpenGLCheckErrorMacro("failed at glReadBuffer");
      break;
  }
}

//----------------------------------------------------------------------------
void vtkFrameBufferObject2::Bind(unsigned int mode)
{
  assert(this->FBOIndex!=0); // need to call glGenFramebuffers first

  // need to ensure that binding is established *every* time because
  // if other code binds over us then all of our subsequent calls
  // will affect that fbo not ours.
  glBindFramebuffer((GLenum)mode, this->FBOIndex);
  vtkOpenGLCheckErrorMacro("failed at glBindFramebuffer");
}

//----------------------------------------------------------------------------
void vtkFrameBufferObject2::UnBind(unsigned int mode)
{
  assert(this->FBOIndex!=0); // need to GenFramebuffers first

  bool drawing
    =  ((GLenum)mode)==GL_DRAW_FRAMEBUFFER
    || ((GLenum)mode)==GL_FRAMEBUFFER;

  GLuint prevFbo
    = (drawing ? this->PreviousDrawFBO : this->PreviousReadFBO);

  glBindFramebuffer((GLenum)mode, prevFbo);
  vtkOpenGLCheckErrorMacro("failed at glBindFramebuffer");
}

//----------------------------------------------------------------------------
void vtkFrameBufferObject2::ActivateDrawBuffers(unsigned int *ids, int num)
{
  assert(num<17); // a practical limit, increase if needed
  GLenum colorAtts[16];
  for (int i=0; i<num; ++i)
  {
    colorAtts[i] = GL_COLOR_ATTACHMENT0 + ids[i];
  }
  glDrawBuffers(num, &colorAtts[0]);
  vtkOpenGLCheckErrorMacro("failed at glDrawBuffers");
}

//----------------------------------------------------------------------------
void vtkFrameBufferObject2::ActivateDrawBuffers(unsigned int num)
{
  assert(num<17); // a practical limit, increase if needed
  GLenum colorAtts[16];
  for (unsigned int i=0; i<num; ++i)
  {
    colorAtts[i] = GL_COLOR_ATTACHMENT0 + i;
  }
  glDrawBuffers(num, &colorAtts[0]);
  vtkOpenGLCheckErrorMacro("failed at glDrawBuffers");
}

//----------------------------------------------------------------------------
void vtkFrameBufferObject2::DeactivateDrawBuffers()
{
  GLenum att = GL_NONE;
  glDrawBuffers(1, &att);
  vtkOpenGLCheckErrorMacro("failed at glDrawBuffers(GL_NONE)");
}

//----------------------------------------------------------------------------
void vtkFrameBufferObject2::ActivateDrawBuffer(
      unsigned int colorAtt)
{
  colorAtt += GL_COLOR_ATTACHMENT0;
  glDrawBuffers(1, &colorAtt);
  vtkOpenGLCheckErrorMacro("failed at glDrawBuffers");
}

//----------------------------------------------------------------------------
void vtkFrameBufferObject2::ActivateReadBuffer(
      unsigned int colorAtt)
{
  colorAtt += GL_COLOR_ATTACHMENT0;
  glReadBuffer((GLenum)colorAtt);
  vtkOpenGLCheckErrorMacro("failed at glReadBuffer");
}

//----------------------------------------------------------------------------
void vtkFrameBufferObject2::DeactivateReadBuffer()
{
  glReadBuffer(GL_NONE);
  vtkOpenGLCheckErrorMacro("failed at glReadBuffer(GL_NONE)");
}

//----------------------------------------------------------------------------
void vtkFrameBufferObject2::AddTexColorAttachment(
        unsigned int mode,
        unsigned int i,
        unsigned int handle)
{
  glFramebufferTexture2D(
        (GLenum)mode,
        GL_COLOR_ATTACHMENT0+i,
        GL_TEXTURE_2D,
        handle,
        0);
  vtkOpenGLCheckErrorMacro("failed at glFramebufferTexture2D");
}

//----------------------------------------------------------------------------
void vtkFrameBufferObject2::RemoveTexColorAttachments(
      unsigned int mode,
      unsigned int num)
{
  for (unsigned int i=0; i<num; ++i)
  {
    this->AddTexColorAttachment(mode, i, 0U);
  }
}

//----------------------------------------------------------------------------
void vtkFrameBufferObject2::AddColorAttachment(
        unsigned int mode,
        unsigned int i,
        vtkTextureObject* tex)
{
  unsigned int handle = (tex==NULL) ? 0 : tex->GetHandle();
  this->AddTexColorAttachment(mode,i,handle);
}

//----------------------------------------------------------------------------
void vtkFrameBufferObject2::AddRenColorAttachment(
        unsigned int mode,
        unsigned int i,
        unsigned int handle)
{
  glFramebufferRenderbuffer(
        (GLenum)mode,
        GL_COLOR_ATTACHMENT0+i,
        GL_RENDERBUFFER,
        handle);
  vtkOpenGLCheckErrorMacro("failed at glFramebufferRenderbuffer");
}

//----------------------------------------------------------------------------
void vtkFrameBufferObject2::AddColorAttachment(
        unsigned int mode,
        unsigned int i,
        vtkRenderbuffer* renbuf)
{
  unsigned int handle = (renbuf==NULL) ? 0 : renbuf->GetHandle();
  this->AddRenColorAttachment(mode, i, handle);
}

//----------------------------------------------------------------------------
void vtkFrameBufferObject2::RemoveRenColorAttachments(
      unsigned int mode,
      unsigned int num)
{
  for (unsigned int i=0; i<num; ++i)
  {
    this->AddRenColorAttachment(mode, i, 0U);
  }
}

//----------------------------------------------------------------------------
void vtkFrameBufferObject2::AddTexDepthAttachment(
        unsigned int mode,
        unsigned int handle)
{
  glFramebufferTexture2D(
        (GLenum)mode,
        GL_DEPTH_ATTACHMENT,
        GL_TEXTURE_2D,
        handle,
        0);
  vtkOpenGLCheckErrorMacro("failed at glFramebufferTexture2D");
}

//----------------------------------------------------------------------------
void vtkFrameBufferObject2::AddDepthAttachment(
        unsigned int mode,
        vtkTextureObject* tex)
{
  unsigned int handle = (tex==NULL) ? 0 : tex->GetHandle();
  this->AddTexDepthAttachment(mode,handle);
}

//----------------------------------------------------------------------------
void vtkFrameBufferObject2::AddRenDepthAttachment(
        unsigned int mode,
        unsigned int handle)
{
  glFramebufferRenderbuffer(
        (GLenum)mode,
        GL_DEPTH_ATTACHMENT,
        GL_RENDERBUFFER,
        handle);
  vtkOpenGLCheckErrorMacro("failed at glFramebufferRenderbuffer");
}

//----------------------------------------------------------------------------
void vtkFrameBufferObject2::AddDepthAttachment(
        unsigned int mode,
        vtkRenderbuffer* renbuf)
{
  unsigned int handle = (renbuf==NULL) ? 0 : renbuf->GetHandle();
  this->AddRenDepthAttachment(mode, handle);
}

//----------------------------------------------------------------------------
void vtkFrameBufferObject2::InitializeViewport(int width, int height)
{
  glDisable(GL_BLEND);
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_SCISSOR_TEST);

  // Viewport transformation for 1:1 'pixel=texel=data' mapping.
  // Note this is not enough for 1:1 mapping, because depending on the
  // primitive displayed (point,line,polygon), the rasterization rules
  // are different.
  glViewport(0, 0, width, height);

  vtkOpenGLStaticCheckErrorMacro("failed after InitializeViewport");
}

//----------------------------------------------------------------------------
int vtkFrameBufferObject2::Blit(
        int srcExt[4],
        int destExt[4],
        unsigned int bits,
        unsigned int mapping)
{
  glBlitFramebuffer(
        (GLint)srcExt[0],
        (GLint)srcExt[2],
        (GLint)srcExt[1],
        (GLint)srcExt[3],
        (GLint)destExt[0],
        (GLint)destExt[2],
        (GLint)destExt[1],
        (GLint)destExt[3],
        (GLbitfield)bits,
        (GLenum)mapping);

  vtkOpenGLStaticCheckErrorMacro("failed at glBlitFramebuffer");

  return 1;
}

//-----------------------------------------------------------------------------
vtkPixelBufferObject *vtkFrameBufferObject2::DownloadDepth(
      int extent[4],
      int vtkType)
{
  assert(this->Context);

  return this->Download(
      extent,
      vtkType,
      1,
      this->GetOpenGLType(vtkType),
      GL_DEPTH_COMPONENT);
}

//-----------------------------------------------------------------------------
vtkPixelBufferObject *vtkFrameBufferObject2::DownloadColor4(
      int extent[4],
      int vtkType)
{
  assert(this->Context);

  return this->Download(
      extent,
      vtkType,
      4,
      this->GetOpenGLType(vtkType),
      GL_RGBA);
}

//-----------------------------------------------------------------------------
vtkPixelBufferObject *vtkFrameBufferObject2::DownloadColor3(
      int extent[4],
      int vtkType)
{
  assert(this->Context);

  return this->Download(
      extent,
      vtkType,
      3,
      this->GetOpenGLType(vtkType),
      GL_RGB);
}

//-----------------------------------------------------------------------------
vtkPixelBufferObject *vtkFrameBufferObject2::DownloadColor1(
      int extent[4],
      int vtkType,
      int channel)
{
  assert(this->Context);
  GLenum oglChannel = 0;
  switch (channel)
  {
    case 0:
      oglChannel = GL_RED;
      break;
    case 1:
      oglChannel = GL_GREEN;
      break;
    case 2:
      oglChannel = GL_BLUE;
      break;
    default:
      vtkErrorMacro("Inavlid channel");
      return NULL;
  }

  return this->Download(
      extent,
      vtkType,
      1,
      this->GetOpenGLType(vtkType),
      oglChannel);
}

//-----------------------------------------------------------------------------
vtkPixelBufferObject *vtkFrameBufferObject2::Download(
      int extent[4],
      int vtkType,
      int nComps,
      int oglType,
      int oglFormat)
{
  vtkPixelBufferObject *pbo = vtkPixelBufferObject::New();
  pbo->SetContext(this->Context);

  this->Download(
        extent,
        vtkType,
        nComps,
        oglType,
        oglFormat,
        pbo);

  return pbo;
}
//-----------------------------------------------------------------------------
void vtkFrameBufferObject2::Download(
      int extent[4],
      int vtkType,
      int nComps,
      int oglType,
      int oglFormat,
      vtkPixelBufferObject *pbo)
{
  unsigned int extentSize[2] = {
        static_cast<unsigned int>(extent[1] - extent[0] + 1),
        static_cast<unsigned int>(extent[3] - extent[2] + 1)
        };

  unsigned int nTups = extentSize[0]*extentSize[1];

  pbo->Allocate(
        vtkType,
        nTups,
        nComps,
        vtkPixelBufferObject::PACKED_BUFFER);

  pbo->Bind(vtkPixelBufferObject::PACKED_BUFFER);

  glPixelStorei(GL_PACK_ALIGNMENT, 1);
  glReadPixels(
        extent[0],
        extent[2],
        extentSize[0],
        extentSize[1],
        oglFormat,
        oglType,
        NULL);

  vtkOpenGLStaticCheckErrorMacro("failed at glReadPixels");

  pbo->UnBind();
}

//-----------------------------------------------------------------------------
int* vtkFrameBufferObject2::GetLastSize(bool forceUpdate)
{
  if (forceUpdate)
    this->QueryViewportSize();

  return this->LastViewportSize;
}

//-----------------------------------------------------------------------------
int* vtkFrameBufferObject2::GetLastSize()
{
  this->QueryViewportSize();
  return this->LastViewportSize;
}

//-----------------------------------------------------------------------------
void vtkFrameBufferObject2::GetLastSize(int &width, int &height)
{
  this->QueryViewportSize();
  width = this->LastViewportSize[0];
  height = this->LastViewportSize[1];
}

//-----------------------------------------------------------------------------
void vtkFrameBufferObject2::GetLastSize(int size[2])
{
  this->GetLastSize(size[0], size[1]);
}

//-----------------------------------------------------------------------------
void vtkFrameBufferObject2::QueryViewportSize()
{
  if (!this->Context)
  {
    vtkErrorMacro("Failed to query viewport size because"
      "there is no context set!");
    return;
  }

  GLint vp[4];
  glGetIntegerv(GL_VIEWPORT, vp);
  vtkOpenGLStaticCheckErrorMacro("Error querying viewport size!");

  this->LastViewportSize[0] = vp[2];
  this->LastViewportSize[1] = vp[3];
}

//-----------------------------------------------------------------------------
int vtkFrameBufferObject2::GetOpenGLType(int vtkType)
{
  // convert vtk type to open gl type
  int oglType = 0;
  switch (vtkType)
  {
    case VTK_FLOAT:
      oglType = GL_FLOAT;
      break;
    case VTK_INT:
      oglType = GL_INT;
      break;
    case VTK_UNSIGNED_INT:
      oglType = GL_UNSIGNED_INT;
      break;
    case VTK_CHAR:
      oglType = GL_BYTE;
      break;
    case VTK_UNSIGNED_CHAR:
      oglType = GL_UNSIGNED_BYTE;
      break;
    default:
      vtkErrorMacro("Unsupported type");
      return 0;
  }
  return oglType;
}

// Description:
// Common switch for parsing fbo status return.
#define vtkFBOStrErrorMacro(status, str, ok) \
  ok = false; \
  switch(status) \
  { \
    case GL_FRAMEBUFFER_COMPLETE: \
      str = "FBO complete"; \
      ok = true; \
      break; \
    case GL_FRAMEBUFFER_UNSUPPORTED: \
      str = "FRAMEBUFFER_UNSUPPORTED"; \
      break; \
    case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT: \
      str = "FRAMEBUFFER_INCOMPLETE_ATTACHMENT"; \
      break; \
    case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT: \
      str = "FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT"; \
      break; \
    case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER: \
      str = "FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER"; \
      break; \
    case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER: \
      str = "FRAMEBUFFER_INCOMPLETE_READ_BUFFER"; \
      break; \
    default: \
      str = "Unknown status"; \
  }

// ----------------------------------------------------------------------------
bool vtkFrameBufferObject2::GetFrameBufferStatus(
      unsigned int mode,
      const char *&desc)
{
  bool ok = false;
  desc = "error";
  GLenum status = glCheckFramebufferStatus((GLenum)mode);
  switch(status)
  {
    case GL_FRAMEBUFFER_COMPLETE:
      desc = "FBO complete";
      ok = true;
      break;
    case GL_FRAMEBUFFER_UNSUPPORTED:
      desc = "FRAMEBUFFER_UNSUPPORTED";
      break;
    case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
      desc = "FRAMEBUFFER_INCOMPLETE_ATTACHMENT";
      break;
    case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
      desc = "FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT";
      break;
#ifdef GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS
    case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS:
      desc = "FRAMEBUFFER_INCOMPLETE_DIMENSIONS";
      break;
#endif
#ifdef GL_FRAMEBUFFER_INCOMPLETE_FORMATS
    case GL_FRAMEBUFFER_INCOMPLETE_FORMATS:
      desc = "FRAMEBUFFER_INCOMPLETE_FORMATS";
      break;
#endif
#ifdef GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER
    case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
      desc = "FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER";
      break;
#endif
#ifdef GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER
    case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
      desc = "FRAMEBUFFER_INCOMPLETE_READ_BUFFER";
      break;
#endif
    default:
      desc = "Unknown status";
  }
  if (!ok)
  {
    return false;
  }
  return true;
}

// ----------------------------------------------------------------------------
int vtkFrameBufferObject2::CheckFrameBufferStatus(unsigned int mode)
{
  bool ok = false;
  const char *str = "error";
  GLenum status = glCheckFramebufferStatus((GLenum)mode);
  vtkOpenGLCheckErrorMacro("failed at glCheckFramebufferStatus");
  switch(status)
  {
    case GL_FRAMEBUFFER_COMPLETE:
      str = "FBO complete";
      ok = true;
      break;
    case GL_FRAMEBUFFER_UNSUPPORTED:
      str = "FRAMEBUFFER_UNSUPPORTED";
      break;
    case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
      str = "FRAMEBUFFER_INCOMPLETE_ATTACHMENT";
      break;
    case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
      str = "FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT";
      break;
#ifdef GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS
    case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS:
      str = "FRAMEBUFFER_INCOMPLETE_DIMENSIONS";
      break;
#endif
#ifdef GL_FRAMEBUFFER_INCOMPLETE_FORMATS
    case GL_FRAMEBUFFER_INCOMPLETE_FORMATS:
      str = "FRAMEBUFFER_INCOMPLETE_FORMATS";
      break;
#endif
#ifdef GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER
    case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
      str = "FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER";
      break;
#endif
#ifdef GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER
    case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
      str = "FRAMEBUFFER_INCOMPLETE_READ_BUFFER";
      break;
#endif
    default:
      str = "Unknown status";
  }
  if (!ok)
  {
    vtkErrorMacro("The framebuffer is incomplete : " << str);
    return 0;
  }
  return 1;
}

// ----------------------------------------------------------------------------
void vtkFrameBufferObject2::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os
    << indent << "Context=" << this->Context << endl
    << indent << "FBOIndex=" << this->FBOIndex << endl
    << indent << "PreviousDrawFBO=" << this->PreviousDrawFBO << endl
    << indent << "PreviousReadFBO=" << this->PreviousReadFBO << endl
    << indent << "PreviousDrawBuffer=" << this->PreviousDrawBuffer << endl
    << indent << "PreviousReadBuffer=" << this->PreviousReadBuffer << endl
    << indent << "Last Viewport Size =" << "[" << this->LastViewportSize[0] << ", "
      << this->LastViewportSize[1] << "]" << endl
    << endl;
}
