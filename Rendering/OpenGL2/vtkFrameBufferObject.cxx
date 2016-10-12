/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFrameBufferObject.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkFrameBufferObject.h"

#include "vtk_glew.h"

#include "vtkObjectFactory.h"
#include "vtkOpenGLRenderUtilities.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkTextureObject.h"
#include "vtkPixelBufferObject.h"
#include "vtkOpenGLBufferObject.h"
#include "vtkOpenGLError.h"
#include "vtkOpenGLResourceFreeCallback.h"

#include <cassert>
#include <vector>
using std::vector;

// #define VTK_FBO_DEBUG // display info on RenderQuad()

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkFrameBufferObject);

//----------------------------------------------------------------------------
vtkFrameBufferObject::vtkFrameBufferObject()
{
  this->ColorBuffersDirty = true;
  this->DepthBufferNeeded = true;
  this->FBOIndex = 0;
  this->PreviousFBOIndex = -1;
  this->DepthBuffer = 0;
  this->LastSize[0] = this->LastSize[1] = -1;
  this->SetActiveBuffer(0);
  this->NumberOfRenderTargets = 1;
  this->ResourceCallback = new vtkOpenGLResourceFreeCallback<vtkFrameBufferObject>(this,
    &vtkFrameBufferObject::ReleaseGraphicsResources);
}

//----------------------------------------------------------------------------
vtkFrameBufferObject::~vtkFrameBufferObject()
{
  if (this->ResourceCallback)
  {
    this->ResourceCallback->Release();
    delete this->ResourceCallback;
    this->ResourceCallback = NULL;
  }
  this->DestroyFBO();
  this->DestroyDepthBuffer();
  this->DestroyColorBuffers();
}

//----------------------------------------------------------------------------
void vtkFrameBufferObject::CreateFBO()
{
  this->FBOIndex=0;
  GLuint temp;
  glGenFramebuffers(1,&temp);
  vtkOpenGLCheckErrorMacro("failed at glGenFramebuffers");
  this->FBOIndex=temp;
}

//----------------------------------------------------------------------------
void vtkFrameBufferObject::DestroyFBO()
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
bool vtkFrameBufferObject::IsSupported(vtkOpenGLRenderWindow *)
{
  if (vtkOpenGLRenderWindow::GetContextSupportsOpenGL32())
  {
    return true;
  }

#if GL_ES_VERSION_2_0 == 1
  bool fbo = true;
  bool fboBlit = true;
#else
  bool fbo = (glewIsSupported("GL_EXT_framebuffer_object") != 0);
  bool fboBlit = (glewIsSupported("GL_EXT_framebuffer_blit") != 0);
#endif

  return fbo && fboBlit;
}

//----------------------------------------------------------------------------
bool vtkFrameBufferObject::LoadRequiredExtensions(vtkOpenGLRenderWindow *)
{
   if (vtkOpenGLRenderWindow::GetContextSupportsOpenGL32())
   {
    return true;
   }

#if GL_ES_VERSION_2_0 == 1
  bool fbo = true;
  bool fboBlit = true;
#else
  bool fbo = (glewIsSupported("GL_EXT_framebuffer_object") != 0);
  bool fboBlit = (glewIsSupported("GL_EXT_framebuffer_blit") != 0);
#endif

  return fbo && fboBlit;
}

void vtkFrameBufferObject::ReleaseGraphicsResources(vtkWindow *)
{
  if (!this->ResourceCallback->IsReleasing())
  {
    this->ResourceCallback->Release();
    return;
  }

  // free previous resources
  this->DestroyDepthBuffer();
  this->DestroyColorBuffers();
  this->DestroyFBO();
  this->Context = NULL;
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkFrameBufferObject::SetContext(vtkOpenGLRenderWindow *renWin)
{
  // avoid pointless re-assignment
  if (this->Context==renWin)
  {
    return;
  }

  this->ResourceCallback->RegisterGraphicsResources(renWin);

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
  // intialize
  this->Context=renWin;
  this->Context->MakeCurrent();
  this->CreateFBO();
}

//----------------------------------------------------------------------------
vtkOpenGLRenderWindow *vtkFrameBufferObject::GetContext()
{
  return this->Context;
}

//----------------------------------------------------------------------------
bool vtkFrameBufferObject::StartNonOrtho(int width,
                                         int height,
                                         bool shaderSupportsTextureInt)
{
  this->Bind();

  // If width/height changed since last render, we need to resize the
  // buffers.
  if (this->LastSize[0] != width || this->LastSize[1] != height ||
    (this->DepthBuffer && !this->DepthBufferNeeded) ||
    (this->DepthBufferNeeded && !this->DepthBuffer))
  {
    this->DestroyDepthBuffer();
    this->DestroyColorBuffers();
  }

  if (this->LastSize[0] != width || this->LastSize[1] != height
    || this->ColorBuffersDirty
    || this->DepthBufferNeeded)
  {
    this->CreateDepthBuffer(
          width,
          height,
          GL_DRAW_FRAMEBUFFER);

    this->CreateColorBuffers(
          width,
          height,
          GL_DRAW_FRAMEBUFFER,
          shaderSupportsTextureInt);
  }

  this->LastSize[0] = width;
  this->LastSize[1] = height;

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

//----------------------------------------------------------------------------
bool vtkFrameBufferObject::Start(int width,
                                 int height,
                                 bool shaderSupportsTextureInt)
{
  if (!this->StartNonOrtho(width, height, shaderSupportsTextureInt))
  {
    return false;
  }

  glDisable(GL_BLEND);
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_SCISSOR_TEST);

  glViewport(0, 0, width, height);

  return true;
}

//----------------------------------------------------------------------------
void vtkFrameBufferObject::SetActiveBuffers(int num, unsigned int indices[])
{
  this->ActiveBuffers.clear();
  for (int cc=0; cc < num; cc++)
  {
    this->ActiveBuffers.push_back(indices[cc]);
  }
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkFrameBufferObject::ActivateBuffers()
{
#if GL_ES_VERSION_2_0 == 1 && GL_ES_VERSION_3_0 != 1
  if (this->ActiveBuffers.size() > 1)
  {
    vtkErrorMacro("OpenGL ES2 only supports one FBO destinaiton");
  }
#else
  GLint maxbuffers;
  glGetIntegerv(GL_MAX_DRAW_BUFFERS, &maxbuffers);

  GLenum *buffers = new GLenum[maxbuffers];
  GLint count=0;
  for(unsigned int cc=0;
      cc < this->ActiveBuffers.size() && count < maxbuffers; cc++)
  {
    buffers[cc] = GL_COLOR_ATTACHMENT0 + this->ActiveBuffers[cc];
    count++;
  }

  glDrawBuffers(count, buffers);
  vtkOpenGLCheckErrorMacro("failed at glDrawBuffers");

  delete[] buffers;
#endif
}

//----------------------------------------------------------------------------
void vtkFrameBufferObject::Bind()
{
  if(this->FBOIndex!=0 && this->PreviousFBOIndex==-1)
  {
    this->Context->MakeCurrent();
    GLint framebufferBinding;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING,&framebufferBinding);
    this->PreviousFBOIndex=framebufferBinding;
    glBindFramebuffer(GL_FRAMEBUFFER, this->FBOIndex);
  }
}

//----------------------------------------------------------------------------
void vtkFrameBufferObject::UnBind()
{
  if (this->FBOIndex!=0 && this->PreviousFBOIndex!=-1)
  {
    glBindFramebuffer(GL_FRAMEBUFFER,this->PreviousFBOIndex);
    this->PreviousFBOIndex=-1;
  }
}

//----------------------------------------------------------------------------
void vtkFrameBufferObject::CreateDepthBuffer(
        int width,
        int height,
        unsigned int mode)
{
  this->DestroyDepthBuffer();

  if (this->UserDepthBuffer)
  {
    // Attach the depth buffer to the FBO.
    glFramebufferTexture2D(
          (GLenum)mode,
          GL_DEPTH_ATTACHMENT,
          GL_TEXTURE_2D,
          this->UserDepthBuffer->GetHandle(),
          0);

    vtkOpenGLCheckErrorMacro("failed at glFramebufferTexture2D");
  }
  else
  if (this->DepthBufferNeeded)
  {
    // Create render buffers which are independent of render targets.
    GLuint temp;
    glGenRenderbuffers(1, &temp);
    vtkOpenGLCheckErrorMacro("failed at glGenRenderbuffers");

    this->DepthBuffer = temp;
    glBindRenderbuffer(GL_RENDERBUFFER, this->DepthBuffer);
    vtkOpenGLCheckErrorMacro("failed at glBindRenderbuffer");

    // Assign storage to this depth buffer.
#if GL_ES_VERSION_2_0 == 1 && GL_ES_VERSION_3_0 != 1
    glRenderbufferStorage(
          GL_RENDERBUFFER,
          GL_DEPTH_COMPONENT,
          width,
          height);
#else
    glRenderbufferStorage(
          GL_RENDERBUFFER,
          GL_DEPTH_COMPONENT24,
          width,
          height);
#endif

    vtkOpenGLCheckErrorMacro("failed at glRenderbufferStorage");

    // Attach the depth buffer to the FBO.
    glFramebufferRenderbuffer(
          (GLenum)mode,
          GL_DEPTH_ATTACHMENT,
          GL_RENDERBUFFER,
          this->DepthBuffer);

    vtkOpenGLCheckErrorMacro("failed at glFramebufferRenderbuffer");
  }
}

//----------------------------------------------------------------------------
void vtkFrameBufferObject::DestroyDepthBuffer()
{
  // because we don't hold a reference to the render
  // context we don't have any control on when it is
  // destroyed. In fact it may be destroyed before
  // we are(eg smart pointers), in which case we should
  // do nothing.
  if(this->Context && this->DepthBuffer)
  {
    GLuint temp = static_cast<GLuint>(this->DepthBuffer);
    glDeleteRenderbuffers(1, &temp);
    vtkOpenGLCheckErrorMacro("failed at glDeleteRenderbuffers");
    this->DepthBuffer = 0;
  }
}

//----------------------------------------------------------------------------
void vtkFrameBufferObject::DestroyColorBuffers()
{
  this->ColorBuffers.clear();
  this->ColorBuffersDirty = true;
}

//----------------------------------------------------------------------------
void vtkFrameBufferObject::CreateColorBuffers(
    int iwidth,
    int iheight,
    unsigned int mode,
    bool shaderSupportsTextureInt)
{
  unsigned int width = static_cast<unsigned int>(iwidth);
  unsigned int height = static_cast <unsigned int>(iheight);

  unsigned int nUserColorBuffers
    = static_cast<unsigned int>(this->UserColorBuffers.size());

  this->ColorBuffers.resize(this->NumberOfRenderTargets);
  unsigned int cc;
  for (cc=0; cc<this->NumberOfRenderTargets && cc<nUserColorBuffers; cc++)
  {
    vtkTextureObject *userBuffer=this->UserColorBuffers[cc];
    if (userBuffer)
    {
      assert(userBuffer->GetWidth()==width);
      assert(userBuffer->GetHeight()==height);
      this->ColorBuffers[cc] = this->UserColorBuffers[cc];
    }
  }

  for (cc=0; cc < this->NumberOfRenderTargets; cc++)
  {
    vtkSmartPointer<vtkTextureObject> colorBuffer = this->ColorBuffers[cc];
    if (!colorBuffer)
    {
      // create a new color buffer for the user.
      colorBuffer = vtkSmartPointer<vtkTextureObject>::New();
      colorBuffer->SetContext(this->Context);
      colorBuffer->SetMinificationFilter(vtkTextureObject::Nearest);
      colorBuffer->SetLinearMagnification(false);
      colorBuffer->SetWrapS(vtkTextureObject::ClampToEdge);
      colorBuffer->SetWrapT(vtkTextureObject::ClampToEdge);
      if (!colorBuffer->Create2D(
                width,
                height,
                4,
                VTK_UNSIGNED_CHAR,
                shaderSupportsTextureInt))
      {
        vtkErrorMacro("Failed to create texture for color buffer.");
        return;
      }
    }

    // attach the buffer
    if (colorBuffer->GetNumberOfDimensions() == 2)
    {
      glFramebufferTexture2D(
            (GLenum)mode,
            GL_COLOR_ATTACHMENT0 + cc,
            GL_TEXTURE_2D,
            colorBuffer->GetHandle(),
            0);

      vtkOpenGLCheckErrorMacro("failed at glFramebufferTexture2D");
    }
    else
    if (colorBuffer->GetNumberOfDimensions() == 3)
    {
      assert(this->UserZSlices[cc]<colorBuffer->GetDepth());
#if GL_ES_VERSION_2_0 != 1
      glFramebufferTexture3D(
            (GLenum)mode,
            GL_COLOR_ATTACHMENT0 + cc,
            GL_TEXTURE_3D,
            colorBuffer->GetHandle(),
            0,
            this->UserZSlices[cc]);
#else
     vtkErrorMacro("Attempt to use 3D frame buffer texture in OpenGL ES 2 or 3");
#endif
      vtkOpenGLCheckErrorMacro("failed at glFramebufferTexture3D");
    }
    this->ColorBuffers[cc] = colorBuffer;
  }

  // unbind the remainder
  unsigned int attachments=this->GetMaximumNumberOfRenderTargets();
  while(cc<attachments)
  {
    glFramebufferRenderbuffer(
          (GLenum)mode,
          GL_COLOR_ATTACHMENT0 + cc,
          GL_RENDERBUFFER,
          0);

    vtkOpenGLCheckErrorMacro("failed at glFramebufferRenderbuffer");
    ++cc;
  }

  // color buffers are allocated and attached
  this->ColorBuffersDirty = false;
}


//----------------------------------------------------------------------------
unsigned int vtkFrameBufferObject::GetMaximumNumberOfActiveTargets()
{
  unsigned int result = 0;
  if (this->Context)
  {
#if GL_ES_VERSION_2_0 == 1 && GL_ES_VERSION_3_0 != 1
    result = 1;
#else
    GLint maxbuffers;
    glGetIntegerv(GL_MAX_DRAW_BUFFERS, &maxbuffers);
    result = static_cast<unsigned int>(maxbuffers);
#endif
  }
  return result;
}

//----------------------------------------------------------------------------
unsigned int vtkFrameBufferObject::GetMaximumNumberOfRenderTargets()
{
  unsigned int result = 0;
  if (this->Context)
  {
#if GL_ES_VERSION_2_0 == 1 && GL_ES_VERSION_3_0 != 1
    result = 1;
#else
    GLint maxColorAttachments;
    glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS,&maxColorAttachments);
    result = static_cast<unsigned int>(maxColorAttachments);
#endif
  }
  return result;
}

//----------------------------------------------------------------------------
void vtkFrameBufferObject::SetNumberOfRenderTargets(unsigned int num)
{
  assert(num>0);
  this->NumberOfRenderTargets = num;
  this->ColorBuffersDirty = true;
}

//----------------------------------------------------------------------------
void vtkFrameBufferObject::SetDepthBuffer(vtkTextureObject* tex)
{
  if (this->UserDepthBuffer != tex)
  {
    this->UserDepthBuffer = tex;
    this->DepthBufferDirty = true;
  }
}

//----------------------------------------------------------------------------
void vtkFrameBufferObject::RemoveDepthBuffer()
{
  this->SetDepthBuffer(0);
}

//----------------------------------------------------------------------------
void vtkFrameBufferObject::SetColorBuffer(
        unsigned int index,
        vtkTextureObject* tex,
        unsigned int zslice/*=0*/)
{
  if (this->UserColorBuffers.size() <= index)
  {
    this->UserColorBuffers.resize(index+1);
    this->UserZSlices.resize(index+1);
  }
  if (this->UserColorBuffers[index] != tex ||
    this->UserZSlices[index] != zslice)
  {
    this->UserColorBuffers[index] = tex;
    this->UserZSlices[index] = zslice;
    this->ColorBuffersDirty = true;
  }
}

//----------------------------------------------------------------------------
vtkTextureObject* vtkFrameBufferObject::GetColorBuffer(unsigned int index)
{
  assert(this->UserColorBuffers.size()>index);
  return this->UserColorBuffers[index];
}

//----------------------------------------------------------------------------
void vtkFrameBufferObject::RemoveColorBuffer(unsigned int index)
{
  if (index < this->UserColorBuffers.size())
  {
    this->UserColorBuffers[index] = 0;
    this->UserZSlices[index] = 0;
    this->ColorBuffersDirty = true;
  }
}

//----------------------------------------------------------------------------
void vtkFrameBufferObject::RemoveAllColorBuffers()
{
  this->UserColorBuffers.clear();
  this->UserZSlices.clear();
  this->ColorBuffersDirty = true;
}

// ----------------------------------------------------------------------------
// Description:
// Display all the attachments of the current framebuffer object.
void vtkFrameBufferObject::DisplayFrameBufferAttachments()
{
  GLint framebufferBinding;
  glGetIntegerv(GL_FRAMEBUFFER_BINDING,&framebufferBinding);
  vtkOpenGLCheckErrorMacro("after getting FRAMEBUFFER_BINDING");
  if(framebufferBinding==0)
  {
    cout<<"Current framebuffer is bind to the system one"<<endl;
  }
  else
  {
    cout<<"Current framebuffer is bind to framebuffer object "
        <<framebufferBinding<<endl;

#if GL_ES_VERSION_2_0 == 1 && GL_ES_VERSION_3_0 != 1
    this->DisplayFrameBufferAttachment(GL_COLOR_ATTACHMENT0);
#else
    GLint maxColorAttachments;
    glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS,&maxColorAttachments);
    vtkOpenGLCheckErrorMacro("after getting MAX_COLOR_ATTACHMENTS");
    int i=0;
    while(i<maxColorAttachments)
    {
      cout<<"color attachement "<<i<<":"<<endl;
      this->DisplayFrameBufferAttachment(GL_COLOR_ATTACHMENT0+i);
      ++i;
    }
#endif
    cout<<"depth attachement :"<<endl;
    this->DisplayFrameBufferAttachment(GL_DEPTH_ATTACHMENT);
    cout<<"stencil attachement :"<<endl;
    this->DisplayFrameBufferAttachment(GL_STENCIL_ATTACHMENT);
  }
}

// ----------------------------------------------------------------------------
// Description:
// Display a given attachment for the current framebuffer object.
void vtkFrameBufferObject::DisplayFrameBufferAttachment(
  unsigned int uattachment)
{
  GLenum attachment=static_cast<GLenum>(uattachment);

  GLint params;
  glGetFramebufferAttachmentParameteriv(
    GL_FRAMEBUFFER,attachment,
    GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE,&params);

  vtkOpenGLCheckErrorMacro("after getting FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE");

  switch(params)
  {
    case GL_NONE:
      cout<<" this attachment is empty"<<endl;
      break;
    case GL_TEXTURE:
      glGetFramebufferAttachmentParameteriv(
        GL_FRAMEBUFFER,attachment,
        GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME,&params);
       vtkOpenGLCheckErrorMacro("after getting FRAMEBUFFER_ATTACHMENT_OBJECT_NAME");
      cout<<" this attachment is a texture with name: "<<params<<endl;
      glGetFramebufferAttachmentParameteriv(
        GL_FRAMEBUFFER,attachment,
        GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL,&params);
      vtkOpenGLCheckErrorMacro("after getting FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL");
      cout<<" its mipmap level is: "<<params<<endl;
      glGetFramebufferAttachmentParameteriv(
        GL_FRAMEBUFFER,attachment,
        GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACE,&params);
      vtkOpenGLCheckErrorMacro("after getting FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACE");
      if(params==0)
      {
        cout<<" this is not a cube map texture."<<endl;
      }
      else
      {
        cout<<" this is a cube map texture and the image is contained in face "
            <<params<<endl;
      }
#ifdef GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_3D_ZOFFSET
       glGetFramebufferAttachmentParameteriv(
         GL_FRAMEBUFFER,attachment,
         GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_3D_ZOFFSET,&params);
#endif
       vtkOpenGLCheckErrorMacro("after getting FRAMEBUFFER_ATTACHMENT_TEXTURE_3D_ZOFFSET");
      if(params==0)
      {
        cout<<" this is not 3D texture."<<endl;
      }
      else
      {
        cout<<" this is a 3D texture and the zoffset of the attached image is "
            <<params<<endl;
      }
      break;
    case GL_RENDERBUFFER:
      cout<<" this attachment is a renderbuffer"<<endl;
      glGetFramebufferAttachmentParameteriv(
        GL_FRAMEBUFFER,attachment,
        GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME,&params);
//      this->PrintError("after getting FRAMEBUFFER_ATTACHMENT_OBJECT_NAME");
      cout<<" this attachment is a renderbuffer with name: "<<params<<endl;

      glBindRenderbuffer(GL_RENDERBUFFER,params);
//      this->PrintError(
//        "after getting binding the current RENDERBUFFER to params");

      glGetRenderbufferParameteriv(GL_RENDERBUFFER,
                                           GL_RENDERBUFFER_WIDTH,
                                           &params);
//      this->PrintError("after getting RENDERBUFFER_WIDTH");
      cout<<" renderbuffer width="<<params<<endl;
      glGetRenderbufferParameteriv(GL_RENDERBUFFER,
                                           GL_RENDERBUFFER_HEIGHT,
                                           &params);
//      this->PrintError("after getting RENDERBUFFER_HEIGHT");
      cout<<" renderbuffer height="<<params<<endl;
      glGetRenderbufferParameteriv(
        GL_RENDERBUFFER,GL_RENDERBUFFER_INTERNAL_FORMAT,
        &params);
//      this->PrintError("after getting RENDERBUFFER_INTERNAL_FORMAT");

      cout<<" renderbuffer internal format=0x"<< std::hex<<params<<std::dec<<endl;

      glGetRenderbufferParameteriv(GL_RENDERBUFFER,
                                   GL_RENDERBUFFER_RED_SIZE,
                                   &params);
//      this->PrintError("after getting RENDERBUFFER_RED_SIZE");
      cout<<" renderbuffer actual resolution for the red component="<<params
          <<endl;
      glGetRenderbufferParameteriv(GL_RENDERBUFFER,
                                   GL_RENDERBUFFER_GREEN_SIZE,
                                   &params);
//      this->PrintError("after getting RENDERBUFFER_GREEN_SIZE");
      cout<<" renderbuffer actual resolution for the green component="<<params
          <<endl;
      glGetRenderbufferParameteriv(GL_RENDERBUFFER,
                                   GL_RENDERBUFFER_BLUE_SIZE,
                                   &params);
//      this->PrintError("after getting RENDERBUFFER_BLUE_SIZE");
      cout<<" renderbuffer actual resolution for the blue component="<<params
          <<endl;
      glGetRenderbufferParameteriv(GL_RENDERBUFFER,
                                   GL_RENDERBUFFER_ALPHA_SIZE,
                                   &params);
//      this->PrintError("after getting RENDERBUFFER_ALPHA_SIZE");
      cout<<" renderbuffer actual resolution for the alpha component="<<params
          <<endl;
      glGetRenderbufferParameteriv(GL_RENDERBUFFER,
                                   GL_RENDERBUFFER_DEPTH_SIZE,
                                   &params);
//      this->PrintError("after getting RENDERBUFFER_DEPTH_SIZE");
      cout<<" renderbuffer actual resolution for the depth component="<<params
          <<endl;
      glGetRenderbufferParameteriv(
        GL_RENDERBUFFER,GL_RENDERBUFFER_STENCIL_SIZE,&params);
//      this->PrintError("after getting RENDERBUFFER_STENCIL_SIZE");
      cout<<" renderbuffer actual resolution for the stencil component="
          <<params<<endl;
      break;
    default:
      cout<<" unexcepted value."<<endl;
      break;
  }
}

// ----------------------------------------------------------------------------
// Description:
// Display the draw buffers.
void vtkFrameBufferObject::DisplayDrawBuffers()
{
#if GL_ES_VERSION_2_0 != 1 || GL_ES_VERSION_3_0 == 1
  GLint ivalue = 1;
  glGetIntegerv(GL_MAX_DRAW_BUFFERS,&ivalue);

  cout<<"there ";
  if(ivalue==1)
  {
    cout << "is ";
  }
  else
  {
    cout << "are ";
  }
  cout << ivalue << " draw buffer";
  if(ivalue!=1)
  {
    cout<<"s";
  }
  cout<<". "<<endl;

  GLint i=0;
  int c=ivalue;
  while(i<c)
  {
    glGetIntegerv(GL_DRAW_BUFFER0+i,&ivalue);

    cout << "draw buffer["<<i<<"]=";
    this->DisplayBuffer(ivalue);
    cout << endl;
    ++i;
  }
#else
  cout << "there is 1 draw buffer." << endl;
#endif
}

// ----------------------------------------------------------------------------
// Description:
// Display the read buffer.
void vtkFrameBufferObject::DisplayReadBuffer()
{
#if GL_ES_VERSION_2_0 != 1 || GL_ES_VERSION_3_0 == 1
  GLint ivalue;
  glGetIntegerv(GL_READ_BUFFER,&ivalue);
  cout << "read buffer=";
  this->DisplayBuffer(ivalue);
  cout<<endl;
#endif
}

// ----------------------------------------------------------------------------
// Description:
// Display any buffer (convert value into string).
void vtkFrameBufferObject::DisplayBuffer(int value)
{
  if(value>=static_cast<int>(GL_COLOR_ATTACHMENT0) &&
     value<=static_cast<int>(GL_COLOR_ATTACHMENT0+15))
  {
    cout << "GL_COLOR_ATTACHMENT" << (value-GL_COLOR_ATTACHMENT0);
  }
  else
  {
#if GL_ES_VERSION_2_0 == 1
      vtkErrorMacro("Attempt to use bad display destintation");
#else
    if(value>=GL_AUX0)
    {
      int b=value-GL_AUX0;
      GLint ivalue;
      glGetIntegerv(GL_AUX_BUFFERS,&ivalue);
      if(b<ivalue)
      {
        cout << "GL_AUX" << b;
      }
      else
      {
        cout << "invalid aux buffer: " << b << ", upper limit is "
             << (ivalue-1) << ", raw value is 0x" << std::hex << (GL_AUX0+b)
             << std::dec;
      }
    }
    else
    {
      switch(value)
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
void vtkFrameBufferObject::RenderQuad(int minX, int maxX, int minY, int maxY,
        vtkShaderProgram *program, vtkOpenGLVertexArrayObject *vao)
{
  assert("pre positive_minX" && minX>=0);
  assert("pre increasing_x" && minX<=maxX);
  assert("pre valid_maxX" && maxX<this->LastSize[0]);
  assert("pre positive_minY" && minY>=0);
  assert("pre increasing_y" && minY<=maxY);
  assert("pre valid_maxY" && maxY<this->LastSize[1]);

#ifdef VTK_FBO_DEBUG
  cout<<"render quad: minX="<<minX<<" maxX="<<maxX<<" minY="<<minY<<
    " maxY="<<maxY<<endl;

  GLuint queryId;
  GLuint nbPixels=0;
  glGenQueries(1,&queryId);
  glBeginQuery(GL_SAMPLES_PASSED,queryId);
#endif

  float maxYTexCoord;
  if(minY==maxY)
  {
    maxYTexCoord=0.0;
  }
  else
  {
    maxYTexCoord=1.0;
  }

  float fminX = 2.0*minX/(this->LastSize[0]-1.0) - 1.0;
  float fminY = 2.0*minY/(this->LastSize[1]-1.0) - 1.0;
  float fmaxX = 2.0*maxX/(this->LastSize[0]-1.0) - 1.0;
  float fmaxY = 2.0*maxY/(this->LastSize[1]-1.0) - 1.0;

  float verts[] =  {
    fminX, fminY, 0,
    fmaxX, fminY, 0,
    fmaxX, fmaxY, 0,
    fminX, fmaxY, 0};

  float tcoords[] = {
    0, 0,
    1.0, 0,
    1.0, maxYTexCoord,
    0, maxYTexCoord};
  vtkOpenGLRenderUtilities::RenderQuad(verts, tcoords, program, vao);

  vtkOpenGLCheckErrorMacro("failed after Render");

#ifdef VTK_FBO_DEBUG
  glEndQuery(GL_SAMPLES_PASSED);
  glGetQueryObjectuiv(queryId,GL_QUERY_RESULT,&nbPixels);
  cout<<nbPixels<<" have been modified."<<endl;
#endif
}

// ----------------------------------------------------------------------------
void vtkFrameBufferObject::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "LastSize : " << this->LastSize[0] << this->LastSize[1]
     << endl;
  os << indent << "DepthBufferNeeded:";
  if(this->DepthBufferNeeded)
  {
      os << "true";
  }
  else
  {
      os << "false";
  }
  os <<endl;
  os << indent << "NumberOfRenderTargets:" << this->NumberOfRenderTargets
     << endl;
}

// ----------------------------------------------------------------------------
int vtkFrameBufferObject::CheckFrameBufferStatus(unsigned int mode)
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
