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

#include "vtkObjectFactory.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkTextureObject.h"
#include "vtkRenderbuffer.h"
#include "vtkPixelBufferObject.h"
#include "vtkOpenGLExtensionManager.h"
#include "vtkOpenGLError.h"

#include "vtkgl.h"

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
  vtkgl::GenFramebuffersEXT(1,&temp);
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
    vtkgl::DeleteFramebuffersEXT(1,&fbo);
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
    vtkOpenGLExtensionManager *mgr=renWin->GetExtensionManager();

    bool gl12 = mgr->ExtensionSupported("GL_VERSION_1_2")==1;
    bool tex3D = gl12 || mgr->ExtensionSupported("GL_EXT_texture3D");

    bool gl14 = mgr->ExtensionSupported("GL_VERSION_1_4")==1;
    bool depthTex = gl14 || mgr->ExtensionSupported("GL_ARB_depth_texture")==1;

    bool gl20 = mgr->ExtensionSupported("GL_VERSION_2_0")==1;
    bool drawBufs = gl20 || mgr->ExtensionSupported("GL_ARB_draw_buffers")==1;

    bool fbo = mgr->ExtensionSupported("GL_EXT_framebuffer_object")==1;
    bool fboBlit = mgr->ExtensionSupported("GL_EXT_framebuffer_blit")==1;

    return tex3D && depthTex && drawBufs && fbo && fboBlit;
  }
  return false;
}

//----------------------------------------------------------------------------
bool vtkFrameBufferObject2::LoadRequiredExtensions(vtkRenderWindow *win)
{
  vtkOpenGLRenderWindow *oglRenWin
    = dynamic_cast<vtkOpenGLRenderWindow*>(win);

  vtkOpenGLExtensionManager *mgr = oglRenWin->GetExtensionManager();

  bool gl12 = mgr->ExtensionSupported("GL_VERSION_1_2")==1;
  bool tex3D = gl12 || mgr->ExtensionSupported("GL_EXT_texture3D");

  bool gl14 = mgr->ExtensionSupported("GL_VERSION_1_4")==1;
  bool depthTex = gl14 || mgr->ExtensionSupported("GL_ARB_depth_texture")==1;

  bool gl20 = mgr->ExtensionSupported("GL_VERSION_2_0")==1;
  bool drawBufs = gl20 || mgr->ExtensionSupported("GL_ARB_draw_buffers");

  bool fbo = mgr->ExtensionSupported("GL_EXT_framebuffer_object")==1;
  bool fboBlit = mgr->ExtensionSupported("GL_EXT_framebuffer_blit")==1;

  bool supported = tex3D && depthTex && drawBufs && fbo && fboBlit;

  if(supported)
  {
    if(gl12)
    {
      mgr->LoadSupportedExtension("GL_VERSION_1_2");
    }
    else
    {
      mgr->LoadCorePromotedExtension("GL_EXT_texture3D");
    }

    if(gl14)
    {
      mgr->LoadSupportedExtension("GL_VERSION_1_4");
    }
    else
    {
      mgr->LoadCorePromotedExtension("GL_ARB_depth_texture");
    }

    if(gl20)
    {
      mgr->LoadSupportedExtension("GL_VERSION_2_0");
    }
    else
    {
      mgr->LoadCorePromotedExtension("GL_ARB_draw_buffers");
    }

    mgr->LoadSupportedExtension("GL_EXT_framebuffer_object");
    mgr->LoadSupportedExtension("GL_EXT_framebuffer_blit");
  }

  return supported;
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
  // initialize
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
  glGetIntegerv(vtkgl::DRAW_FRAMEBUFFER_BINDING_EXT, (int*)&this->PreviousDrawFBO);
  glGetIntegerv(vtkgl::READ_FRAMEBUFFER_BINDING_EXT, (int*)&this->PreviousReadFBO);
}

//----------------------------------------------------------------------------
void vtkFrameBufferObject2::SaveCurrentBuffers()
{
  glGetIntegerv(GL_DRAW_BUFFER, (int*)&this->PreviousDrawBuffer);
  glGetIntegerv(GL_READ_BUFFER, (int*)&this->PreviousReadBuffer);
}

//----------------------------------------------------------------------------
void vtkFrameBufferObject2::RestorePreviousBuffers(unsigned int mode)
{
  switch((GLenum)mode)
  {
    case vtkgl::FRAMEBUFFER_EXT:
      glDrawBuffer((GLenum)this->PreviousDrawBuffer);
      vtkOpenGLCheckErrorMacro("failed at glDrawBuffer");

      glReadBuffer((GLenum)this->PreviousReadBuffer);
      vtkOpenGLCheckErrorMacro("failed at glReadBuffer");
      break;

    case vtkgl::DRAW_FRAMEBUFFER_EXT:
      glDrawBuffer((GLenum)this->PreviousDrawBuffer);
      vtkOpenGLCheckErrorMacro("failed at glDrawBuffer");
      break;

    case vtkgl::READ_FRAMEBUFFER_EXT:
      glReadBuffer((GLenum)this->PreviousReadBuffer);
      vtkOpenGLCheckErrorMacro("failed at glReadBuffer");
      break;
  }
}

//----------------------------------------------------------------------------
void vtkFrameBufferObject2::Bind(unsigned int mode)
{
  assert(this->FBOIndex!=0); // need to call glGenFramebuffers first

  // need to ensure that binding is esxtablished *every* time because
  // if other code binds over us then all of our subsequent calls
  // will affect that fbo not ours.
  vtkgl::BindFramebufferEXT((GLenum)mode, this->FBOIndex);
  vtkOpenGLCheckErrorMacro("failed at glBindFramebuffer");
}

//----------------------------------------------------------------------------
void vtkFrameBufferObject2::UnBind(unsigned int mode)
{
  assert(this->FBOIndex!=0); // need to GenFramebuffers first

  bool drawing
    =  ((GLenum)mode)==vtkgl::DRAW_FRAMEBUFFER_EXT
    || ((GLenum)mode)==vtkgl::FRAMEBUFFER_EXT;

  GLuint prevFbo
    = (drawing ? this->PreviousDrawFBO : this->PreviousReadFBO);

  vtkgl::BindFramebufferEXT((GLenum)mode, prevFbo);
  vtkOpenGLCheckErrorMacro("failed at glBindFramebuffer");
}

//----------------------------------------------------------------------------
void vtkFrameBufferObject2::ActivateDrawBuffers(unsigned int *ids, int num)
{
  assert(num<17); // a practical limit, increase if needed
  GLenum colorAtts[16];
  for (int i=0; i<num; ++i)
  {
    colorAtts[i] = vtkgl::COLOR_ATTACHMENT0 + ids[i];
  }
  vtkgl::DrawBuffers(num, &colorAtts[0]);
  vtkOpenGLCheckErrorMacro("failed at glDrawBuffers");
}

//----------------------------------------------------------------------------
void vtkFrameBufferObject2::ActivateDrawBuffers(unsigned int num)
{
  assert(num<17); // a practical limit, increase if needed
  GLenum colorAtts[16];
  for (unsigned int i=0; i<num; ++i)
  {
    colorAtts[i] = vtkgl::COLOR_ATTACHMENT0 + i;
  }
  vtkgl::DrawBuffers(num, &colorAtts[0]);
  vtkOpenGLCheckErrorMacro("failed at glDrawBuffers");
}

//----------------------------------------------------------------------------
void vtkFrameBufferObject2::DeactivateDrawBuffers()
{
  GLenum att = GL_NONE;
  vtkgl::DrawBuffers(1, &att);
  vtkOpenGLCheckErrorMacro("failed at glDrawBuffers(GL_NONE)");
}

//----------------------------------------------------------------------------
void vtkFrameBufferObject2::ActivateDrawBuffer(
      unsigned int colorAtt)
{
  colorAtt += vtkgl::COLOR_ATTACHMENT0;
  vtkgl::DrawBuffers(1, &colorAtt);
  vtkOpenGLCheckErrorMacro("failed at glDrawBuffers");
}

//----------------------------------------------------------------------------
void vtkFrameBufferObject2::ActivateReadBuffer(
      unsigned int colorAtt)
{
  colorAtt += vtkgl::COLOR_ATTACHMENT0;
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
  vtkgl::FramebufferTexture2DEXT(
        (GLenum)mode,
        vtkgl::COLOR_ATTACHMENT0_EXT+i,
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
  vtkgl::FramebufferRenderbufferEXT(
        (GLenum)mode,
        vtkgl::COLOR_ATTACHMENT0_EXT+i,
        vtkgl::RENDERBUFFER,
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
  vtkgl::FramebufferTexture2DEXT(
        (GLenum)mode,
        vtkgl::DEPTH_ATTACHMENT,
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
  vtkgl::FramebufferRenderbufferEXT(
        (GLenum)mode,
        vtkgl::DEPTH_ATTACHMENT,
        vtkgl::RENDERBUFFER,
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
  glDisable(GL_ALPHA_TEST);
  glDisable(GL_BLEND);
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_LIGHTING);
  glDisable(GL_SCISSOR_TEST);

  // Viewport transformation for 1:1 'pixel=texel=data' mapping.
  // Note this note enough for 1:1 mapping, because depending on the
  // primitive displayed (point,line,polygon), the rasterization rules
  // are different.
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0.0, width, 0.0, height, -1, 1);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
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
  vtkgl::BlitFramebufferEXT(
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
    case vtkgl::FRAMEBUFFER_COMPLETE_EXT: \
      str = "FBO complete"; \
      ok = true; \
      break; \
    case vtkgl::FRAMEBUFFER_UNSUPPORTED_EXT: \
      str = "FRAMEBUFFER_UNSUPPORTED"; \
      break; \
    case vtkgl::FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT: \
      str = "FRAMEBUFFER_INCOMPLETE_ATTACHMENT"; \
      break; \
    case vtkgl::FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT: \
      str = "FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT"; \
      break; \
    case vtkgl::FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT: \
      str = "FRAMEBUFFER_INCOMPLETE_DIMENSIONS"; \
      break; \
    case vtkgl::FRAMEBUFFER_INCOMPLETE_FORMATS_EXT: \
      str = "FRAMEBUFFER_INCOMPLETE_FORMATS"; \
      break; \
    case vtkgl::FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT: \
      str = "FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER"; \
      break; \
    case vtkgl::FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT: \
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
  bool ok;
  GLenum status = vtkgl::CheckFramebufferStatusEXT((GLenum)mode);
  vtkFBOStrErrorMacro(status, desc, ok);
  return ok;
}

// ----------------------------------------------------------------------------
int vtkFrameBufferObject2::CheckFrameBufferStatus(unsigned int mode)
{
  bool ok;
  const char *desc = "error";
  GLenum status = vtkgl::CheckFramebufferStatusEXT((GLenum)mode);
  vtkOpenGLCheckErrorMacro("failed at glCheckFramebufferStatus");
  vtkFBOStrErrorMacro(status, desc, ok);
  if (!ok)
  {
    vtkErrorMacro("The framebuffer is incomplete : " << desc);
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
    << endl;
}
