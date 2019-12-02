/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLFramebufferObject.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOpenGLFramebufferObject.h"

#include "vtk_glew.h"

#include "vtkObjectFactory.h"
#include "vtkOpenGLBufferObject.h"
#include "vtkOpenGLError.h"
#include "vtkOpenGLRenderUtilities.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLResourceFreeCallback.h"
#include "vtkOpenGLState.h"
#include "vtkPixelBufferObject.h"
#include "vtkRenderbuffer.h"
#include "vtkTextureObject.h"

#include <cassert>
#include <vector>
using std::vector;

class vtkFOInfo
{
public:
  unsigned int Attachment;
  unsigned int Target;
  unsigned int MipmapLevel;
  bool Attached;
  vtkTextureObject* Texture;
  vtkRenderbuffer* Renderbuffer;
  bool CreatedByFO;
  unsigned int ZSlice;

  vtkFOInfo()
  {
    this->Attachment = 0;
    this->Target = 0;
    this->MipmapLevel = 0;
    this->Texture = nullptr;
    this->Renderbuffer = nullptr;
    this->CreatedByFO = false;
    this->ZSlice = 0;
    this->Attached = false;
  }

  ~vtkFOInfo() { this->Clear(); }

  void Clear()
  {
    if (this->Texture)
    {
      this->Texture->Delete();
      this->Texture = nullptr;
    }
    if (this->Renderbuffer)
    {
      this->Renderbuffer->Delete();
      this->Renderbuffer = nullptr;
    }
    this->Attachment = 0;
    this->Target = 0;
    this->MipmapLevel = 0;
    this->CreatedByFO = false;
    this->ZSlice = 0;
    this->Attached = false;
  }

  bool IsSet() { return this->Texture || this->Renderbuffer; }

  void ReleaseGraphicsResources(vtkWindow* win)
  {
    if (this->Texture)
    {
      this->Texture->ReleaseGraphicsResources(win);
    }
    if (this->Renderbuffer)
    {
      this->Renderbuffer->ReleaseGraphicsResources(win);
    }
  }

  void Attach(int mode)
  {
    if (this->Attached)
    {
      return;
    }
    if (this->Texture)
    {
      if (this->Texture->GetNumberOfDimensions() == 3)
      {
#ifndef GL_ES_VERSION_3_0
        glFramebufferTexture3D((GLenum)mode, this->Attachment, this->Target,
          this->Texture->GetHandle(), this->MipmapLevel, this->ZSlice);
        this->Attached = true;
#else
        vtkGenericWarningMacro("Attempt to use 3D frame buffer texture in OpenGL ES 2 or 3");
#endif
      }
      else
      {
        glFramebufferTexture2D((GLenum)mode, this->Attachment, this->Target,
          this->Texture->GetHandle(), this->MipmapLevel);
        this->Attached = true;
      }
    }
    else if (this->Renderbuffer)
    {
      glFramebufferRenderbuffer(
        (GLenum)mode, this->Attachment, GL_RENDERBUFFER, this->Renderbuffer->GetHandle());
      this->Attached = true;
    }
  }

  void Detach(int mode)
  {
    if (!this->Attached)
    {
      return;
    }
    if (this->Texture)
    {
      if (this->Texture->GetNumberOfDimensions() == 3)
      {
#ifndef GL_ES_VERSION_3_0
        glFramebufferTexture3D(
          (GLenum)mode, this->Attachment, this->Target, 0, this->MipmapLevel, this->ZSlice);
        this->Attached = false;
#else
        vtkGenericWarningMacro("Attempt to use 3D frame buffer texture in OpenGL ES 2 or 3");
#endif
      }
      else
      {
        glFramebufferTexture2D((GLenum)mode, this->Attachment, this->Target, 0, this->MipmapLevel);
        this->Attached = false;
      }
    }
    else if (this->Renderbuffer)
    {
      glFramebufferRenderbuffer((GLenum)mode, this->Attachment, GL_RENDERBUFFER, 0);
      this->Attached = false;
    }
  }

  void SetTexture(vtkTextureObject* val, unsigned int attachment, unsigned int target = 0,
    unsigned int mipmapLevel = 0)
  {

    // always reset to false
    this->CreatedByFO = false;

    if (this->Texture == val && this->Attachment == attachment)
    {
      return;
    }
    this->Attached = false;
    val->Register(nullptr);
    if (this->Texture)
    {
      this->Texture->Delete();
      this->Texture = nullptr;
    }
    if (this->Renderbuffer)
    {
      this->Renderbuffer->Delete();
      this->Renderbuffer = nullptr;
    }
    this->Texture = val;
    this->Attachment = attachment;
    // if target not specified, used texture target
    // a custom target is useful for cubemap
    this->Target = target ? target : val->GetTarget();
    this->MipmapLevel = mipmapLevel;
  }

  void SetRenderbuffer(vtkRenderbuffer* val, unsigned int attachment)
  {

    // always reset to false
    this->CreatedByFO = false;

    if (this->Renderbuffer == val && this->Attachment == attachment)
    {
      return;
    }
    this->Attached = false;
    val->Register(nullptr);
    if (this->Texture)
    {
      this->Texture->Delete();
      this->Texture = nullptr;
    }
    if (this->Renderbuffer)
    {
      this->Renderbuffer->Delete();
      this->Renderbuffer = nullptr;
    }
    this->Renderbuffer = val;
    this->Attachment = attachment;
  }

  int GetSamples()
  {
    if (this->Texture)
    {
      return this->Texture->GetSamples();
    }
    if (this->Renderbuffer)
    {
      return this->Renderbuffer->GetSamples();
    }
    return 0;
  }

  void GetSize(int (&size)[2])
  {
    if (this->Texture)
    {
      size[0] = this->Texture->GetWidth() >> this->MipmapLevel;
      size[1] = this->Texture->GetHeight() >> this->MipmapLevel;
      return;
    }
    if (this->Renderbuffer)
    {
      size[0] = this->Renderbuffer->GetWidth();
      size[1] = this->Renderbuffer->GetHeight();
      return;
    }
  }

  void Resize(int size[2])
  {
    if (this->Texture)
    {
      this->Texture->Resize(size[0], size[1]);
    }
    if (this->Renderbuffer)
    {
      this->Renderbuffer->Resize(size[0], size[1]);
    }
  }
};

typedef std::map<unsigned int, vtkFOInfo*>::iterator foIter;

// #define VTK_FBO_DEBUG // display info on RenderQuad()

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkOpenGLFramebufferObject);

//----------------------------------------------------------------------------
vtkOpenGLFramebufferObject::vtkOpenGLFramebufferObject()
{
  this->Context = nullptr;
  this->FBOIndex = 0;

  this->DrawBindingSaved = false;
  this->ReadBindingSaved = false;
  this->DrawBufferSaved = false;
  this->ReadBufferSaved = false;

  this->ActiveReadBuffer = GL_COLOR_ATTACHMENT0;

  this->LastSize[0] = this->LastSize[1] = -1;

  this->DepthBuffer = new vtkFOInfo;

  this->ActiveBuffers.push_back(0);

  this->ResourceCallback = new vtkOpenGLResourceFreeCallback<vtkOpenGLFramebufferObject>(
    this, &vtkOpenGLFramebufferObject::ReleaseGraphicsResources);
}

//----------------------------------------------------------------------------
vtkOpenGLFramebufferObject::~vtkOpenGLFramebufferObject()
{
  if (this->ResourceCallback)
  {
    this->ResourceCallback->Release();
    delete this->ResourceCallback;
    this->ResourceCallback = nullptr;
  }
  delete this->DepthBuffer;
  for (foIter i = this->ColorBuffers.begin(); i != this->ColorBuffers.end(); ++i)
  {
    delete i->second;
  }
  this->ColorBuffers.clear();
  this->Context = nullptr;
}

//-----------------------------------------------------------------------------
int vtkOpenGLFramebufferObject::GetOpenGLType(int vtkType)
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

unsigned int vtkOpenGLFramebufferObject::GetDrawMode()
{
  return GL_DRAW_FRAMEBUFFER;
}
unsigned int vtkOpenGLFramebufferObject::GetReadMode()
{
  return GL_READ_FRAMEBUFFER;
}
unsigned int vtkOpenGLFramebufferObject::GetBothMode()
{
  return GL_FRAMEBUFFER;
}

//----------------------------------------------------------------------------
void vtkOpenGLFramebufferObject::CreateFBO()
{
  if (!this->FBOIndex)
  {
    this->ResourceCallback->RegisterGraphicsResources(this->Context);
    this->FBOIndex = 0;
    GLuint temp;
    glGenFramebuffers(1, &temp);
    vtkOpenGLCheckErrorMacro("failed at glGenFramebuffers");
    this->FBOIndex = temp;
  }
}

//----------------------------------------------------------------------------
void vtkOpenGLFramebufferObject::DestroyFBO()
{
  if (this->FBOIndex != 0)
  {
    GLuint fbo = static_cast<GLuint>(this->FBOIndex);
    glDeleteFramebuffers(1, &fbo);
    vtkOpenGLCheckErrorMacro("failed at glDeleteFramebuffers");
    this->FBOIndex = 0;
  }
}

void vtkOpenGLFramebufferObject::ReleaseGraphicsResources(vtkWindow* win)
{
  if (!this->ResourceCallback->IsReleasing())
  {
    this->ResourceCallback->Release();
    return;
  }

  // free previous resources
  this->DestroyDepthBuffer(win);
  this->DestroyColorBuffers(win);
  this->DestroyFBO();
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkOpenGLFramebufferObject::SetContext(vtkRenderWindow* rw)
{
  vtkOpenGLRenderWindow* renWin = static_cast<vtkOpenGLRenderWindow*>(rw);

  // avoid pointless re-assignment
  if (this->Context == renWin)
  {
    return;
  }

  // all done if assigned null
  if (!renWin)
  {
    return;
  }
  // check for support
  if (!this->LoadRequiredExtensions(renWin))
  {
    vtkErrorMacro("Context does not support the required extensions");
    return;
  }
  // initialize
  this->Context = renWin;
}

//----------------------------------------------------------------------------
vtkOpenGLRenderWindow* vtkOpenGLFramebufferObject::GetContext()
{
  return this->Context;
}

//----------------------------------------------------------------------------
void vtkOpenGLFramebufferObject::InitializeViewport(int width, int height)
{
  vtkOpenGLState* ostate = this->Context->GetState();
  ostate->vtkglDisable(GL_BLEND);
  ostate->vtkglDisable(GL_DEPTH_TEST);
  ostate->vtkglDisable(GL_SCISSOR_TEST);

  // Viewport transformation for 1:1 'pixel=texel=data' mapping.
  // Note this is not enough for 1:1 mapping, because depending on the
  // primitive displayed (point,line,polygon), the rasterization rules
  // are different.
  ostate->vtkglViewport(0, 0, width, height);

  vtkOpenGLStaticCheckErrorMacro("failed after InitializeViewport");
}

bool vtkOpenGLFramebufferObject::StartNonOrtho(int width, int height)
{
  this->Bind();

  // make sure sizes are consistent for all attachments
  // this will adjust the depth buffer size if we
  // created it.
  this->UpdateSize();

  // If width/height does not match attachments error
  if (this->LastSize[0] != width || this->LastSize[1] != height)
  {
    vtkErrorMacro("FBO size does not match the size of its attachments!.");
  }

  this->ActivateBuffers();

  GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
  if (status != GL_FRAMEBUFFER_COMPLETE)
  {
    vtkErrorMacro("Frame buffer object was not initialized correctly.");
    this->CheckFrameBufferStatus(GL_FRAMEBUFFER);
    this->DisplayFrameBufferAttachments();
    this->DisplayDrawBuffers();
    this->DisplayReadBuffer();
    return false;
  }

  return true;
}

void vtkOpenGLFramebufferObject::UpdateSize()
{
  bool foundSize = false;
  int size[2];
  int aSize[2];
  bool mismatch = false;

  size[0] = 0;
  size[1] = 0;
  aSize[0] = 0;
  aSize[1] = 0;

  // loop through all attachments and
  // verify they are of the same size.
  for (foIter i = this->ColorBuffers.begin(); i != this->ColorBuffers.end(); ++i)
  {
    if (!i->second->CreatedByFO && i->second->IsSet())
    {
      i->second->GetSize(aSize);
      if (!foundSize)
      {
        size[0] = aSize[0];
        size[1] = aSize[1];
        foundSize = true;
      }
      else
      {
        if (aSize[0] != size[0] || aSize[1] != size[1])
        {
          mismatch = true;
        }
      }
    }
  }

  if (!this->DepthBuffer->CreatedByFO && this->DepthBuffer->IsSet())
  {
    this->DepthBuffer->GetSize(aSize);
    if (!foundSize)
    {
      size[0] = aSize[0];
      size[1] = aSize[1];
      foundSize = true;
    }
    else
    {
      if (aSize[0] != size[0] || aSize[1] != size[1])
      {
        mismatch = true;
      }
    }
  }

  if (mismatch)
  {
    vtkErrorMacro("The framebuffer has mismatched attachments.");
  }

  // resize any FO created items
  this->LastSize[0] = size[0];
  this->LastSize[1] = size[1];

  // now resize any buffers we created that are the wrong size
  if (this->DepthBuffer->IsSet() && this->DepthBuffer->CreatedByFO)
  {
    this->DepthBuffer->Resize(this->LastSize);
  }
}

void vtkOpenGLFramebufferObject::Resize(int width, int height)
{
  // resize all items
  this->LastSize[0] = width;
  this->LastSize[1] = height;

  // loop through all attachments and
  // verify they are of the same size.
  for (foIter i = this->ColorBuffers.begin(); i != this->ColorBuffers.end(); ++i)
  {
    i->second->Resize(this->LastSize);
  }

  // now resize any buffers we created that are the wrong size
  if (this->DepthBuffer->IsSet())
  {
    this->DepthBuffer->Resize(this->LastSize);
  }
}

//----------------------------------------------------------------------------
bool vtkOpenGLFramebufferObject::Start(int width, int height)
{
  if (!this->StartNonOrtho(width, height))
  {
    return false;
  }

  this->InitializeViewport(width, height);
  return true;
}

//----------------------------------------------------------------------------
void vtkOpenGLFramebufferObject::ActivateBuffers()
{
  GLint maxbuffers;
  // todo move to cache
  glGetIntegerv(GL_MAX_DRAW_BUFFERS, &maxbuffers);

  GLenum* buffers = new GLenum[maxbuffers];
  GLint count = 0;
  for (unsigned int cc = 0; cc < this->ActiveBuffers.size() && count < maxbuffers; cc++)
  {
    buffers[cc] = GL_COLOR_ATTACHMENT0 + this->ActiveBuffers[cc];
    count++;
  }

  this->Context->GetState()->vtkDrawBuffers(count, buffers, this);

  delete[] buffers;
}

void vtkOpenGLFramebufferObject::ActivateDrawBuffer(unsigned int num)
{
  this->ActivateDrawBuffers(&num, 1);
}

//----------------------------------------------------------------------------
void vtkOpenGLFramebufferObject::ActivateReadBuffer(unsigned int colorAtt)
{
  colorAtt += GL_COLOR_ATTACHMENT0;
  this->Context->GetState()->vtkReadBuffer((GLenum)colorAtt, this);
  this->ActiveReadBuffer = colorAtt;
}

//----------------------------------------------------------------------------
void vtkOpenGLFramebufferObject::ActivateDrawBuffers(unsigned int num)
{
  GLint maxbuffers;
  glGetIntegerv(GL_MAX_DRAW_BUFFERS, &maxbuffers);

  GLenum* buffers = new GLenum[maxbuffers];
  GLint count = 0;
  for (unsigned int cc = 0; cc < num && count < maxbuffers; cc++)
  {
    buffers[cc] = GL_COLOR_ATTACHMENT0 + cc;
    count++;
  }

  this->Context->GetState()->vtkDrawBuffers(count, buffers, this);
  delete[] buffers;

  this->ActiveBuffers.clear();
  for (unsigned int cc = 0; cc < num; cc++)
  {
    this->ActiveBuffers.push_back(cc);
  }
  this->Modified();
}

unsigned int vtkOpenGLFramebufferObject::GetActiveDrawBuffer(unsigned int id)
{
  if (id >= this->ActiveBuffers.size())
  {
    return GL_NONE;
  }
  return GL_COLOR_ATTACHMENT0 + this->ActiveBuffers[id];
}

//----------------------------------------------------------------------------
void vtkOpenGLFramebufferObject::ActivateDrawBuffers(unsigned int* ids, int num)
{
  GLint maxbuffers;
  glGetIntegerv(GL_MAX_DRAW_BUFFERS, &maxbuffers);

  GLenum* buffers = new GLenum[maxbuffers];
  GLint count = 0;
  for (unsigned int cc = 0; cc < static_cast<unsigned int>(num) && count < maxbuffers; cc++)
  {
    buffers[cc] = GL_COLOR_ATTACHMENT0 + ids[cc];
    count++;
  }

  this->Context->GetState()->vtkDrawBuffers(count, buffers, this);
  delete[] buffers;

  this->ActiveBuffers.clear();
  for (int cc = 0; cc < num; cc++)
  {
    this->ActiveBuffers.push_back(ids[cc]);
  }
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkOpenGLFramebufferObject::DeactivateDrawBuffers()
{
  GLenum att = GL_NONE;
  this->Context->GetState()->vtkDrawBuffers(1, &att, this);
  this->ActiveBuffers.clear();
}

//----------------------------------------------------------------------------
void vtkOpenGLFramebufferObject::DeactivateReadBuffer()
{
  this->Context->GetState()->vtkReadBuffer(GL_NONE, this);
  this->ActiveReadBuffer = GL_NONE;
}

//----------------------------------------------------------------------------
void vtkOpenGLFramebufferObject::SaveCurrentBindingsAndBuffers()
{
  this->SaveCurrentBindingsAndBuffers(GL_FRAMEBUFFER);
}

void vtkOpenGLFramebufferObject::SaveCurrentBindingsAndBuffers(unsigned int mode)
{
  if (!this->Context)
  {
    vtkErrorMacro("Attempt to save bindings without a context");
    return;
  }
  if (mode == GL_FRAMEBUFFER || mode == GL_DRAW_FRAMEBUFFER)
  {
    this->Context->GetState()->PushDrawFramebufferBinding();
    this->DrawBindingSaved = true;
  }
  if (mode == GL_FRAMEBUFFER || mode == GL_READ_FRAMEBUFFER)
  {
    this->Context->GetState()->PushReadFramebufferBinding();
    this->ReadBindingSaved = true;
  }
}

void vtkOpenGLFramebufferObject::RestorePreviousBindingsAndBuffers()
{
  this->RestorePreviousBindingsAndBuffers(GL_FRAMEBUFFER);
}

void vtkOpenGLFramebufferObject::RestorePreviousBindingsAndBuffers(unsigned int mode)
{
  if (!this->Context)
  {
    vtkErrorMacro("Attempt to restore bindings without a context");
    return;
  }
  if ((mode == GL_FRAMEBUFFER || mode == GL_DRAW_FRAMEBUFFER) && this->DrawBindingSaved)
  {
    this->Context->GetState()->PopDrawFramebufferBinding();
    this->DrawBindingSaved = false;
    this->DrawBufferSaved = false;
  }
  if ((mode == GL_FRAMEBUFFER || mode == GL_READ_FRAMEBUFFER) && this->ReadBindingSaved)
  {
    this->Context->GetState()->PopReadFramebufferBinding();
    this->ReadBindingSaved = false;
    this->ReadBufferSaved = false;
  }
}

//----------------------------------------------------------------------------
void vtkOpenGLFramebufferObject::Bind()
{
  this->Bind(GL_FRAMEBUFFER);
}

void vtkOpenGLFramebufferObject::Bind(unsigned int mode)
{
  if (!this->Context)
  {
    vtkErrorMacro("Attempt to bind framebuffer without a context");
    return;
  }
  this->Context->MakeCurrent();
  this->CreateFBO();
  if (this->FBOIndex != 0)
  {
    // note this also changes the draw/read buffers as they are
    // tied to the binding
    this->Context->GetState()->vtkBindFramebuffer(mode, this);
  }
}

void vtkOpenGLFramebufferObject::AttachColorBuffer(unsigned int index)
{
  if (this->FBOIndex != 0)
  {
    foIter i = this->ColorBuffers.find(index);
    this->Context->GetState()->PushDrawFramebufferBinding();
    this->Context->GetState()->vtkBindFramebuffer(GL_DRAW_FRAMEBUFFER, this);
    if (i != this->ColorBuffers.end())
    {
      i->second->Attach(GL_DRAW_FRAMEBUFFER);
    }
    this->Context->GetState()->PopDrawFramebufferBinding();
  }
}

void vtkOpenGLFramebufferObject::AttachDepthBuffer()
{
  if (this->FBOIndex != 0)
  {
    this->Context->GetState()->PushDrawFramebufferBinding();
    this->Context->GetState()->vtkBindFramebuffer(GL_DRAW_FRAMEBUFFER, this);
    this->DepthBuffer->Attach(GL_DRAW_FRAMEBUFFER);
    this->Context->GetState()->PopDrawFramebufferBinding();
  }
}

//----------------------------------------------------------------------------
void vtkOpenGLFramebufferObject::UnBind()
{
  if (this->FBOIndex != 0)
  {
    this->Context->GetState()->vtkBindFramebuffer(GL_FRAMEBUFFER, nullptr);
  }
}

void vtkOpenGLFramebufferObject::UnBind(unsigned int mode)
{
  if (this->FBOIndex != 0)
  {
    this->Context->GetState()->vtkBindFramebuffer(mode, nullptr);
  }
}

void vtkOpenGLFramebufferObject::AddDepthAttachment()
{
  // create as needed
  if (!this->DepthBuffer->IsSet())
  {
    // create a renderbuffer
    vtkRenderbuffer* rb = vtkRenderbuffer::New();
    rb->SetContext(this->Context);
    rb->CreateDepthAttachment(this->LastSize[0], this->LastSize[1]);

    this->AddDepthAttachment(rb);
    this->DepthBuffer->CreatedByFO = true;
    rb->Delete();
  }
}

//----------------------------------------------------------------------------
void vtkOpenGLFramebufferObject::DestroyDepthBuffer(vtkWindow*)
{
  this->DepthBuffer->Clear();
}

//----------------------------------------------------------------------------
void vtkOpenGLFramebufferObject::DestroyColorBuffers(vtkWindow*)
{
  for (foIter i = this->ColorBuffers.begin(); i != this->ColorBuffers.end(); ++i)
  {
    i->second->Clear();
  }
}

//----------------------------------------------------------------------------
unsigned int vtkOpenGLFramebufferObject::GetMaximumNumberOfActiveTargets()
{
  unsigned int result = 0;
  if (this->Context)
  {
    GLint maxbuffers;
    glGetIntegerv(GL_MAX_DRAW_BUFFERS, &maxbuffers);
    result = static_cast<unsigned int>(maxbuffers);
  }
  return result;
}

//----------------------------------------------------------------------------
unsigned int vtkOpenGLFramebufferObject::GetMaximumNumberOfRenderTargets()
{
  unsigned int result = 0;
  if (this->Context)
  {
    GLint maxColorAttachments;
    glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &maxColorAttachments);
    result = static_cast<unsigned int>(maxColorAttachments);
  }
  return result;
}

//----------------------------------------------------------------------------
void vtkOpenGLFramebufferObject::RemoveDepthAttachment()
{
  if (this->FBOIndex != 0)
  {
    this->Context->GetState()->PushDrawFramebufferBinding();
    this->Context->GetState()->vtkBindFramebuffer(GL_DRAW_FRAMEBUFFER, this);
    this->DepthBuffer->Detach(GL_DRAW_FRAMEBUFFER);
    this->Context->GetState()->PopDrawFramebufferBinding();
  }
  delete this->DepthBuffer;
  this->DepthBuffer = new vtkFOInfo;
}

void vtkOpenGLFramebufferObject::AddDepthAttachment(vtkTextureObject* tex)
{
  this->DepthBuffer->SetTexture(tex, GL_DEPTH_ATTACHMENT);
  this->AttachDepthBuffer();
}

void vtkOpenGLFramebufferObject::AddDepthAttachment(vtkRenderbuffer* rb)
{
  this->DepthBuffer->SetRenderbuffer(rb, GL_DEPTH_ATTACHMENT);
  this->AttachDepthBuffer();
}

void vtkOpenGLFramebufferObject::AddColorAttachment(unsigned int index, vtkTextureObject* tex,
  unsigned int zslice, unsigned int format, unsigned int mipmapLevel)
{
  // is the fbo size is not set do it here
  if (this->LastSize[0] == -1)
  {
    this->LastSize[0] = tex->GetWidth();
    this->LastSize[1] = tex->GetHeight();
  }

  foIter i = this->ColorBuffers.find(index);
  if (i == this->ColorBuffers.end())
  {
    vtkFOInfo* foinfo = new vtkFOInfo;
    i = this->ColorBuffers.insert(std::make_pair(index, foinfo)).first;
  }
  i->second->SetTexture(tex, GL_COLOR_ATTACHMENT0 + index, format, mipmapLevel);
  i->second->ZSlice = zslice;
  this->AttachColorBuffer(index);
}

void vtkOpenGLFramebufferObject::AddColorAttachment(unsigned int index, vtkRenderbuffer* rb)
{
  // is the fbo size is not set do it here
  if (this->LastSize[0] == -1)
  {
    this->LastSize[0] = rb->GetWidth();
    this->LastSize[1] = rb->GetHeight();
  }

  foIter i = this->ColorBuffers.find(index);
  if (i == this->ColorBuffers.end())
  {
    vtkFOInfo* foinfo = new vtkFOInfo;
    i = this->ColorBuffers.insert(std::make_pair(index, foinfo)).first;
  }
  i->second->SetRenderbuffer(rb, GL_COLOR_ATTACHMENT0 + index);
  this->AttachColorBuffer(index);
}

//----------------------------------------------------------------------------
void vtkOpenGLFramebufferObject::RemoveColorAttachments(unsigned int num)
{
  for (unsigned int i = 0; i < num; ++i)
  {
    this->RemoveColorAttachment(i);
  }
}

void vtkOpenGLFramebufferObject::RemoveColorAttachment(unsigned int index)
{
  foIter i = this->ColorBuffers.find(index);

  if (i != this->ColorBuffers.end())
  {
    if (this->FBOIndex != 0)
    {
      this->Context->GetState()->PushDrawFramebufferBinding();
      this->Context->GetState()->vtkBindFramebuffer(GL_DRAW_FRAMEBUFFER, this);
      i->second->Detach(GL_DRAW_FRAMEBUFFER);
      this->Context->GetState()->PopDrawFramebufferBinding();
    }
    delete i->second;
    i->second = nullptr;
    this->ColorBuffers.erase(i);
  }
}

// ----------------------------------------------------------------------------
// Description:
// Display all the attachments of the current framebuffer object.
void vtkOpenGLFramebufferObject::DisplayFrameBufferAttachments()
{
  GLint framebufferBinding;
  glGetIntegerv(GL_FRAMEBUFFER_BINDING, &framebufferBinding);
  vtkOpenGLCheckErrorMacro("after getting FRAMEBUFFER_BINDING");
  if (framebufferBinding == 0)
  {
    cout << "Current framebuffer is bind to the system one" << endl;
  }
  else
  {
    cout << "Current framebuffer is bind to framebuffer object " << framebufferBinding << endl;

    GLint maxColorAttachments;
    glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &maxColorAttachments);
    vtkOpenGLCheckErrorMacro("after getting MAX_COLOR_ATTACHMENTS");
    int i = 0;
    while (i < maxColorAttachments)
    {
      cout << "color attachment " << i << ":" << endl;
      this->DisplayFrameBufferAttachment(GL_COLOR_ATTACHMENT0 + i);
      ++i;
    }
    cout << "depth attachment :" << endl;
    this->DisplayFrameBufferAttachment(GL_DEPTH_ATTACHMENT);
    cout << "stencil attachment :" << endl;
    this->DisplayFrameBufferAttachment(GL_STENCIL_ATTACHMENT);
  }
}

// ----------------------------------------------------------------------------
// Description:
// Display a given attachment for the current framebuffer object.
void vtkOpenGLFramebufferObject::DisplayFrameBufferAttachment(unsigned int uattachment)
{
  GLenum attachment = static_cast<GLenum>(uattachment);

  GLint params;
  glGetFramebufferAttachmentParameteriv(
    GL_FRAMEBUFFER, attachment, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, &params);

  vtkOpenGLCheckErrorMacro("after getting FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE");

  switch (params)
  {
    case GL_NONE:
      cout << " this attachment is empty" << endl;
      break;
    case GL_TEXTURE:
      glGetFramebufferAttachmentParameteriv(
        GL_FRAMEBUFFER, attachment, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &params);
      vtkOpenGLCheckErrorMacro("after getting FRAMEBUFFER_ATTACHMENT_OBJECT_NAME");
      cout << " this attachment is a texture with name: " << params << endl;
      glGetFramebufferAttachmentParameteriv(
        GL_FRAMEBUFFER, attachment, GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL, &params);
      vtkOpenGLCheckErrorMacro("after getting FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL");
      cout << " its mipmap level is: " << params << endl;
      glGetFramebufferAttachmentParameteriv(
        GL_FRAMEBUFFER, attachment, GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACE, &params);
      vtkOpenGLCheckErrorMacro("after getting FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACE");
      if (params == 0)
      {
        cout << " this is not a cube map texture." << endl;
      }
      else
      {
        cout << " this is a cube map texture and the image is contained in face " << params << endl;
      }
#ifdef GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_3D_ZOFFSET
      glGetFramebufferAttachmentParameteriv(
        GL_FRAMEBUFFER, attachment, GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_3D_ZOFFSET, &params);
#endif
      vtkOpenGLCheckErrorMacro("after getting FRAMEBUFFER_ATTACHMENT_TEXTURE_3D_ZOFFSET");
      if (params == 0)
      {
        cout << " this is not 3D texture." << endl;
      }
      else
      {
        cout << " this is a 3D texture and the zoffset of the attached image is " << params << endl;
      }
      break;
    case GL_RENDERBUFFER:
      cout << " this attachment is a renderbuffer" << endl;
      glGetFramebufferAttachmentParameteriv(
        GL_FRAMEBUFFER, attachment, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &params);
      //      this->PrintError("after getting FRAMEBUFFER_ATTACHMENT_OBJECT_NAME");
      cout << " this attachment is a renderbuffer with name: " << params << endl;

      glBindRenderbuffer(GL_RENDERBUFFER, params);
      //      this->PrintError(
      //        "after getting binding the current RENDERBUFFER to params");

      glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH, &params);
      //      this->PrintError("after getting RENDERBUFFER_WIDTH");
      cout << " renderbuffer width=" << params << endl;
      glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_HEIGHT, &params);
      //      this->PrintError("after getting RENDERBUFFER_HEIGHT");
      cout << " renderbuffer height=" << params << endl;
      glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_INTERNAL_FORMAT, &params);
      //      this->PrintError("after getting RENDERBUFFER_INTERNAL_FORMAT");

      cout << " renderbuffer internal format=0x" << std::hex << params << std::dec << endl;

      glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_RED_SIZE, &params);
      //      this->PrintError("after getting RENDERBUFFER_RED_SIZE");
      cout << " renderbuffer actual resolution for the red component=" << params << endl;
      glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_GREEN_SIZE, &params);
      //      this->PrintError("after getting RENDERBUFFER_GREEN_SIZE");
      cout << " renderbuffer actual resolution for the green component=" << params << endl;
      glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_BLUE_SIZE, &params);
      //      this->PrintError("after getting RENDERBUFFER_BLUE_SIZE");
      cout << " renderbuffer actual resolution for the blue component=" << params << endl;
      glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_ALPHA_SIZE, &params);
      //      this->PrintError("after getting RENDERBUFFER_ALPHA_SIZE");
      cout << " renderbuffer actual resolution for the alpha component=" << params << endl;
      glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_DEPTH_SIZE, &params);
      //      this->PrintError("after getting RENDERBUFFER_DEPTH_SIZE");
      cout << " renderbuffer actual resolution for the depth component=" << params << endl;
      glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_STENCIL_SIZE, &params);
      //      this->PrintError("after getting RENDERBUFFER_STENCIL_SIZE");
      cout << " renderbuffer actual resolution for the stencil component=" << params << endl;
      break;
    default:
      cout << " unexcepted value." << endl;
      break;
  }
}

// ----------------------------------------------------------------------------
// Description:
// Display the draw buffers.
void vtkOpenGLFramebufferObject::DisplayDrawBuffers()
{
  GLint ivalue = 1;
  glGetIntegerv(GL_MAX_DRAW_BUFFERS, &ivalue);

  cout << "there ";
  if (ivalue == 1)
  {
    cout << "is ";
  }
  else
  {
    cout << "are ";
  }
  cout << ivalue << " draw buffer";
  if (ivalue != 1)
  {
    cout << "s";
  }
  cout << ". " << endl;

  GLint i = 0;
  int c = ivalue;
  while (i < c)
  {
    glGetIntegerv(GL_DRAW_BUFFER0 + i, &ivalue);

    cout << "draw buffer[" << i << "]=";
    this->DisplayBuffer(ivalue);
    cout << endl;
    ++i;
  }
}

// ----------------------------------------------------------------------------
// Description:
// Display the read buffer.
void vtkOpenGLFramebufferObject::DisplayReadBuffer()
{
  GLint ivalue;
  glGetIntegerv(GL_READ_BUFFER, &ivalue);
  cout << "read buffer=";
  this->DisplayBuffer(ivalue);
  cout << endl;
}

// ----------------------------------------------------------------------------
// Description:
// Display any buffer (convert value into string).
void vtkOpenGLFramebufferObject::DisplayBuffer(int value)
{
  if (value >= static_cast<int>(GL_COLOR_ATTACHMENT0) &&
    value <= static_cast<int>(GL_COLOR_ATTACHMENT0 + 15))
  {
    cout << "GL_COLOR_ATTACHMENT" << (value - GL_COLOR_ATTACHMENT0);
  }
  else
  {
#ifdef GL_ES_VERSION_3_0
    vtkErrorMacro("Attempt to use bad display destintation");
#else
    if (value >= GL_AUX0)
    {
      int b = value - GL_AUX0;
      GLint ivalue;
      glGetIntegerv(GL_AUX_BUFFERS, &ivalue);
      if (b < ivalue)
      {
        cout << "GL_AUX" << b;
      }
      else
      {
        cout << "invalid aux buffer: " << b << ", upper limit is " << (ivalue - 1)
             << ", raw value is 0x" << std::hex << (GL_AUX0 + b) << std::dec;
      }
    }
    else
    {
      switch (value)
      {
        case GL_NONE:
          cout << "GL_NONE";
          break;
        case GL_FRONT_LEFT:
          cout << "GL_FRONT_LEFT";
          break;
        case GL_FRONT_RIGHT:
          cout << "GL_FRONT_RIGHT";
          break;
        case GL_BACK_LEFT:
          cout << "GL_BACK_LEFT";
          break;
        case GL_BACK_RIGHT:
          cout << "GL_BACK_RIGHT";
          break;
        case GL_FRONT:
          cout << "GL_FRONT";
          break;
        case GL_BACK:
          cout << "GL_BACK";
          break;
        case GL_LEFT:
          cout << "GL_LEFT";
          break;
        case GL_RIGHT:
          cout << "GL_RIGHT";
          break;
        case GL_FRONT_AND_BACK:
          cout << "GL_FRONT_AND_BACK";
          break;
        default:
          cout << "unknown 0x" << std::hex << value << std::dec;
          break;
      }
    }
#endif
  }
}

// ---------------------------------------------------------------------------
// a program must be bound
// a VAO must be bound
void vtkOpenGLFramebufferObject::RenderQuad(int minX, int maxX, int minY, int maxY,
  vtkShaderProgram* program, vtkOpenGLVertexArrayObject* vao)
{
  assert("pre positive_minX" && minX >= 0);
  assert("pre increasing_x" && minX <= maxX);
  assert("pre valid_maxX" && maxX < this->LastSize[0]);
  assert("pre positive_minY" && minY >= 0);
  assert("pre increasing_y" && minY <= maxY);
  assert("pre valid_maxY" && maxY < this->LastSize[1]);

#ifdef VTK_FBO_DEBUG
  cout << "render quad: minX=" << minX << " maxX=" << maxX << " minY=" << minY << " maxY=" << maxY
       << endl;

  GLuint queryId;
  GLuint nbPixels = 0;
  glGenQueries(1, &queryId);
  glBeginQuery(GL_SAMPLES_PASSED, queryId);
#endif

  float maxYTexCoord;
  if (minY == maxY)
  {
    maxYTexCoord = 0.0;
  }
  else
  {
    maxYTexCoord = 1.0;
  }

  float fminX = 2.0 * minX / (this->LastSize[0] - 1.0) - 1.0;
  float fminY = 2.0 * minY / (this->LastSize[1] - 1.0) - 1.0;
  float fmaxX = 2.0 * maxX / (this->LastSize[0] - 1.0) - 1.0;
  float fmaxY = 2.0 * maxY / (this->LastSize[1] - 1.0) - 1.0;

  float verts[] = { fminX, fminY, 0, fmaxX, fminY, 0, fmaxX, fmaxY, 0, fminX, fmaxY, 0 };

  float tcoords[] = { 0, 0, 1.0, 0, 1.0, maxYTexCoord, 0, maxYTexCoord };
  vtkOpenGLRenderUtilities::RenderQuad(verts, tcoords, program, vao);

  vtkOpenGLCheckErrorMacro("failed after Render");

#ifdef VTK_FBO_DEBUG
  glEndQuery(GL_SAMPLES_PASSED);
  glGetQueryObjectuiv(queryId, GL_QUERY_RESULT, &nbPixels);
  cout << nbPixels << " have been modified." << endl;
#endif
}

// ----------------------------------------------------------------------------
void vtkOpenGLFramebufferObject::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "LastSize : " << this->LastSize[0] << this->LastSize[1] << endl;
}

// ----------------------------------------------------------------------------
bool vtkOpenGLFramebufferObject::GetFrameBufferStatus(unsigned int mode, const char*& desc)
{
  bool ok = false;
  desc = "error";
  GLenum status = glCheckFramebufferStatus((GLenum)mode);
  switch (status)
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
#ifdef GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE
    case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
      desc = "GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE";
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
int vtkOpenGLFramebufferObject::CheckFrameBufferStatus(unsigned int mode)
{
  bool ok = false;
  const char* str = "error";
  GLenum status = glCheckFramebufferStatus((GLenum)mode);
  vtkOpenGLCheckErrorMacro("failed at glCheckFramebufferStatus");
  switch (status)
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
#ifdef GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE
    case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
      str = "GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE";
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

//----------------------------------------------------------------------------
int vtkOpenGLFramebufferObject::Blit(
  const int srcExt[4], const int destExt[4], unsigned int bits, unsigned int mapping)
{
  glBlitFramebuffer((GLint)srcExt[0], (GLint)srcExt[2], (GLint)srcExt[1], (GLint)srcExt[3],
    (GLint)destExt[0], (GLint)destExt[2], (GLint)destExt[1], (GLint)destExt[3], (GLbitfield)bits,
    (GLenum)mapping);

  vtkOpenGLStaticCheckErrorMacro("failed at glBlitFramebuffer");

  return 1;
}

//-----------------------------------------------------------------------------
vtkPixelBufferObject* vtkOpenGLFramebufferObject::DownloadDepth(int extent[4], int vtkType)
{
  assert(this->Context);

  return this->Download(extent, vtkType, 1, this->GetOpenGLType(vtkType), GL_DEPTH_COMPONENT);
}

//-----------------------------------------------------------------------------
vtkPixelBufferObject* vtkOpenGLFramebufferObject::DownloadColor4(int extent[4], int vtkType)
{
  assert(this->Context);

  return this->Download(extent, vtkType, 4, this->GetOpenGLType(vtkType), GL_RGBA);
}

//-----------------------------------------------------------------------------
vtkPixelBufferObject* vtkOpenGLFramebufferObject::DownloadColor3(int extent[4], int vtkType)
{
  assert(this->Context);

  return this->Download(extent, vtkType, 3, this->GetOpenGLType(vtkType), GL_RGB);
}

//-----------------------------------------------------------------------------
vtkPixelBufferObject* vtkOpenGLFramebufferObject::DownloadColor1(
  int extent[4], int vtkType, int channel)
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
      vtkErrorMacro("Invalid channel");
      return nullptr;
  }

  return this->Download(extent, vtkType, 1, this->GetOpenGLType(vtkType), oglChannel);
}

//-----------------------------------------------------------------------------
vtkPixelBufferObject* vtkOpenGLFramebufferObject::Download(
  int extent[4], int vtkType, int nComps, int oglType, int oglFormat)
{
  vtkPixelBufferObject* pbo = vtkPixelBufferObject::New();
  pbo->SetContext(this->Context);

  this->Download(extent, vtkType, nComps, oglType, oglFormat, pbo);

  return pbo;
}
//-----------------------------------------------------------------------------
void vtkOpenGLFramebufferObject::Download(
  int extent[4], int vtkType, int nComps, int oglType, int oglFormat, vtkPixelBufferObject* pbo)
{
  unsigned int extentSize[2] = { static_cast<unsigned int>(extent[1] - extent[0] + 1),
    static_cast<unsigned int>(extent[3] - extent[2] + 1) };

  unsigned int nTups = extentSize[0] * extentSize[1];

  pbo->Allocate(vtkType, nTups, nComps, vtkPixelBufferObject::PACKED_BUFFER);

  pbo->Bind(vtkPixelBufferObject::PACKED_BUFFER);

  glPixelStorei(GL_PACK_ALIGNMENT, 1);
  glReadPixels(extent[0], extent[2], extentSize[0], extentSize[1], oglFormat, oglType, nullptr);

  vtkOpenGLStaticCheckErrorMacro("failed at glReadPixels");

  pbo->UnBind();
}

int vtkOpenGLFramebufferObject::GetMultiSamples()
{
  int abuff = this->ActiveBuffers[0];
  return this->ColorBuffers[abuff]->GetSamples();
}

bool vtkOpenGLFramebufferObject::PopulateFramebuffer(int width, int height)
{
  return this->PopulateFramebuffer(width, height, true, 1, VTK_UNSIGNED_CHAR, true, 24, 0);
}

bool vtkOpenGLFramebufferObject::PopulateFramebuffer(int width, int height, bool useTextures,
  int numberOfColorAttachments, int colorDataType, bool wantDepthAttachment, int depthBitplanes,
  int multisamples, bool wantStencilAttachment)
{
  // MAX_DEPTH_TEXTURE_SAMPLES
  this->Bind();
  this->LastSize[0] = width;
  this->LastSize[1] = height;

  if (useTextures)
  {
    for (int i = 0; i < numberOfColorAttachments; i++)
    {
      vtkTextureObject* color = vtkTextureObject::New();
      color->SetContext(this->Context);
      color->SetSamples(multisamples);
      color->SetWrapS(vtkTextureObject::Repeat);
      color->SetWrapT(vtkTextureObject::Repeat);
      color->SetMinificationFilter(vtkTextureObject::Nearest);
      color->SetMagnificationFilter(vtkTextureObject::Nearest);
      color->Allocate2D(this->LastSize[0], this->LastSize[1], 4, colorDataType);
      this->AddColorAttachment(i, color);
      color->Delete();
    }

    if (wantDepthAttachment)
    {
      vtkTextureObject* depth = vtkTextureObject::New();
      depth->SetContext(this->Context);
      depth->SetSamples(multisamples);
      depth->SetWrapS(vtkTextureObject::Repeat);
      depth->SetWrapT(vtkTextureObject::Repeat);
      depth->SetMinificationFilter(vtkTextureObject::Nearest);
      depth->SetMagnificationFilter(vtkTextureObject::Nearest);
      if (wantStencilAttachment)
      {
        depth->AllocateDepthStencil(this->LastSize[0], this->LastSize[1]);
      }
      else
      {
        switch (depthBitplanes)
        {
          case 16:
            depth->AllocateDepth(this->LastSize[0], this->LastSize[1], vtkTextureObject::Fixed16);
            break;
          case 32:
            depth->AllocateDepth(this->LastSize[0], this->LastSize[1], vtkTextureObject::Fixed32);
            break;
          case 24:
          default:
            depth->AllocateDepth(this->LastSize[0], this->LastSize[1], vtkTextureObject::Fixed24);
            break;
        }
      }
      this->AddDepthAttachment(depth);
      depth->Delete();
    }
  }
  else
  {
    for (int i = 0; i < numberOfColorAttachments; i++)
    {
      vtkRenderbuffer* color = vtkRenderbuffer::New();
      color->SetContext(this->Context);
      switch (colorDataType)
      {
        case VTK_UNSIGNED_CHAR:
          color->Create(GL_RGBA8, this->LastSize[0], this->LastSize[1], multisamples);
          break;
        case VTK_FLOAT:
          color->Create(GL_RGBA32F, this->LastSize[0], this->LastSize[1], multisamples);
          break;
      }
      this->AddColorAttachment(i, color);
      color->Delete();
    }

    if (wantDepthAttachment)
    {
      vtkRenderbuffer* depth = vtkRenderbuffer::New();
      depth->SetContext(this->Context);
      if (wantStencilAttachment)
      {
        depth->Create(GL_DEPTH_STENCIL, this->LastSize[0], this->LastSize[1], multisamples);
      }
      else
      {
        switch (depthBitplanes)
        {
          case 16:
            depth->Create(GL_DEPTH_COMPONENT16, this->LastSize[0], this->LastSize[1], multisamples);
            break;
#ifdef GL_DEPTH_COMPONENT32
          case 32:
            depth->Create(GL_DEPTH_COMPONENT32, this->LastSize[0], this->LastSize[1], multisamples);
            break;
#endif
          case 24:
          default:
            depth->Create(GL_DEPTH_COMPONENT24, this->LastSize[0], this->LastSize[1], multisamples);
            break;
        }
      }
      this->AddDepthAttachment(depth);
      depth->Delete();
    }
  }

  const char* desc;
  if (this->GetFrameBufferStatus(this->GetBothMode(), desc))
  {
    this->ActivateDrawBuffer(0);
    this->ActivateReadBuffer(0);
    return true;
  }
  return false;
}

int vtkOpenGLFramebufferObject::GetNumberOfColorAttachments()
{
  return static_cast<int>(this->ColorBuffers.size());
}
