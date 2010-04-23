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

#include "vtkTextureObject.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLExtensionManager.h"
#include "vtkOpenGLRenderWindow.h"

#include "vtkgl.h"
#include <assert.h>

// #define VTK_FBO_DEBUG // display info on RenderQuad()

vtkStandardNewMacro(vtkFrameBufferObject);
//----------------------------------------------------------------------------
vtkFrameBufferObject::vtkFrameBufferObject()
{
  this->FBOIndex = 0;
  this->DepthBufferNeeded = true;
  this->DepthBuffer = 0;
  this->NumberOfRenderTargets = 1;
  this->LastSize[0] = this->LastSize[1] = -1;
  this->SetActiveBuffer(0);
  this->PreviousFBOIndex=-1; // -1 Bind hasn't been called yet.
}

//----------------------------------------------------------------------------
vtkFrameBufferObject::~vtkFrameBufferObject()
{
  if(this->Context!=0)
    {
      this->DestroyFBO();
    this->DestroyBuffers();
    this->DestroyColorBuffers();
    }
}


//----------------------------------------------------------------------------
  // Description:
  // Returns if the context supports the required extensions.
bool vtkFrameBufferObject::IsSupported(vtkRenderWindow *win)
{
  vtkOpenGLRenderWindow *renWin=vtkOpenGLRenderWindow::SafeDownCast(win);
  if(renWin!=0)
    {
      vtkOpenGLExtensionManager *mgr=renWin->GetExtensionManager();
      
      bool gl12=mgr->ExtensionSupported("GL_VERSION_1_2")==1;
      bool gl14=mgr->ExtensionSupported("GL_VERSION_1_4")==1;
      bool gl15=mgr->ExtensionSupported("GL_VERSION_1_5")==1;
      bool gl20=mgr->ExtensionSupported("GL_VERSION_2_0")==1;
      
      bool tex3D=gl12 || mgr->ExtensionSupported("GL_EXT_texture3D");
      
      bool depthTexture24=gl14 ||
        mgr->ExtensionSupported("GL_ARB_depth_texture");
      
      bool occlusion=gl15 ||
        mgr->ExtensionSupported("GL_ARB_occlusion_query");
      
      bool drawbuffers=gl20 || mgr->ExtensionSupported("GL_ARB_draw_buffers");
      
      bool fbo=mgr->ExtensionSupported("GL_EXT_framebuffer_object")==1;
      
      return tex3D && depthTexture24 && occlusion && drawbuffers && fbo;
    }
  return false;
}

//----------------------------------------------------------------------------
bool vtkFrameBufferObject::LoadRequiredExtensions(
                                              vtkOpenGLExtensionManager*mgr)
{
  // Load extensions using vtkOpenGLExtensionManager
  
  bool gl12=mgr->ExtensionSupported("GL_VERSION_1_2")==1;
  bool gl14=mgr->ExtensionSupported("GL_VERSION_1_4")==1;
  bool gl15=mgr->ExtensionSupported("GL_VERSION_1_5")==1;
  bool gl20=mgr->ExtensionSupported("GL_VERSION_2_0")==1;
  
  bool tex3D=gl12 || mgr->ExtensionSupported("GL_EXT_texture3D");
  
  bool depthTexture24=gl14 ||
    mgr->ExtensionSupported("GL_ARB_depth_texture");
  
  bool occlusion=gl15 ||
    mgr->ExtensionSupported("GL_ARB_occlusion_query");
  
  bool drawbuffers=gl20 || mgr->ExtensionSupported("GL_ARB_draw_buffers");
  
  bool fbo=mgr->ExtensionSupported("GL_EXT_framebuffer_object")==1;
  
  bool supported=tex3D && depthTexture24 && occlusion && drawbuffers && fbo;
  
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
    
    if(gl15)
      {
      mgr->LoadSupportedExtension("GL_VERSION_1_5");
      }
    else
      {
      mgr->LoadCorePromotedExtension("GL_ARB_occlusion_query");
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
    }
  
  return supported;
}

//----------------------------------------------------------------------------
void vtkFrameBufferObject::SetContext(vtkRenderWindow *renWin)
{
  if(this->Context==renWin)
    {
    return;
    }
  
  if(this->Context!=0)
    {
    this->DestroyFBO();
    this->DestroyBuffers();
    this->DestroyColorBuffers();
    }

  vtkOpenGLRenderWindow *openGLRenWin=
    vtkOpenGLRenderWindow::SafeDownCast(renWin);
  this->Context=openGLRenWin;
  if(openGLRenWin!=0)
    {
    if (!this->LoadRequiredExtensions(openGLRenWin->GetExtensionManager()))
      {
      this->Context=0;
      vtkErrorMacro("Required OpenGL extensions not supported by the context.");
      }
    }
  this->Modified();
}

//----------------------------------------------------------------------------
vtkRenderWindow *vtkFrameBufferObject::GetContext()
{
  return this->Context;
}

//----------------------------------------------------------------------------
bool vtkFrameBufferObject::StartNonOrtho(int width,
                                         int height,
                                         bool shaderSupportsTextureInt)
{
  this->Context->MakeCurrent();
  if(this->FBOIndex==0)
    {
      this->CreateFBO();
    }

  this->Bind();
  
  // this->CheckFrameBufferStatus();
  
  // If width/height changed since last render, we need to resize the
  // buffers.
  if (this->LastSize[0] != width || this->LastSize[1] != height || 
    (this->DepthBuffer && !this->DepthBufferNeeded) ||
    (this->DepthBufferNeeded && !this->DepthBuffer))
    {
    this->DestroyBuffers();
    this->DestroyColorBuffers();
    }

  if (this->LastSize[0] != width || this->LastSize[1] != height
    || this->ColorBuffersDirty 
    || this->DepthBufferNeeded)
    {
    this->CreateBuffers(width, height);
    //    this->CheckFrameBufferStatus();
    this->CreateColorBuffers(width, height,shaderSupportsTextureInt);
    }

  this->LastSize[0] = width;
  this->LastSize[1] = height;

  this->ActivateBuffers();
  // we cannot check the FBO status before calling this->ActivateBuffers()
  // because the draw buffer status is part of
  // the FBO status.
  // Note this is because we are using FBO through the EXT extension.
  // This is not true with the ARB extension (which we are not using here
  // as it exists only on OpenGL 3.0 drivers)
  
  GLenum status = vtkgl::CheckFramebufferStatusEXT(vtkgl::FRAMEBUFFER_EXT);
  if (status != vtkgl::FRAMEBUFFER_COMPLETE_EXT)
    {
    vtkErrorMacro("Frame buffer object was not initialized correctly.");
    this->CheckFrameBufferStatus();
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
  return true;
}

//----------------------------------------------------------------------------
void vtkFrameBufferObject::ActivateBuffers()
{
  GLint maxbuffers;
  glGetIntegerv(vtkgl::MAX_DRAW_BUFFERS, &maxbuffers);

  GLenum *buffers = new GLenum[maxbuffers];
  GLint count=0;
  for(unsigned int cc=0;
      cc < this->ActiveBuffers.size() && count < maxbuffers; cc++)
    {
    buffers[cc] = vtkgl::COLOR_ATTACHMENT0_EXT + this->ActiveBuffers[cc];
    count++;
    }

  vtkgl::DrawBuffers(count, buffers);
  delete[] buffers;
}

//----------------------------------------------------------------------------
void vtkFrameBufferObject::Bind()
{
  if(this->FBOIndex!=0 && this->PreviousFBOIndex==-1)
    {
    this->Context->MakeCurrent();
    GLint framebufferBinding;
    glGetIntegerv(vtkgl::FRAMEBUFFER_BINDING_EXT,&framebufferBinding);
    this->PreviousFBOIndex=framebufferBinding;
    vtkgl::BindFramebufferEXT(vtkgl::FRAMEBUFFER_EXT, this->FBOIndex);
    }
}

//----------------------------------------------------------------------------
void vtkFrameBufferObject::UnBind()
{
  if (this->FBOIndex!=0 && this->PreviousFBOIndex!=-1)
    {
    vtkgl::BindFramebufferEXT(vtkgl::FRAMEBUFFER_EXT,this->PreviousFBOIndex);
    this->PreviousFBOIndex=-1;
    }
}

//----------------------------------------------------------------------------
void vtkFrameBufferObject::SetActiveBuffers(int num,
                                            unsigned int indices[])
{
  this->ActiveBuffers.clear();
  for (int cc=0; cc < num; cc++)
    {
    this->ActiveBuffers.push_back(indices[cc]);
    }
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkFrameBufferObject::CreateFBO()
{
  this->FBOIndex=0;
  GLuint temp;
  vtkgl::GenFramebuffersEXT(1,&temp);
  this->FBOIndex=temp;
}

//----------------------------------------------------------------------------
void vtkFrameBufferObject::DestroyFBO()
{
  if(this->FBOIndex!=0)
    {
    GLuint fbo=static_cast<GLuint>(this->FBOIndex);
    vtkgl::DeleteFramebuffersEXT(1,&fbo);
    this->FBOIndex=0;
    }
}

//----------------------------------------------------------------------------
void vtkFrameBufferObject::CreateBuffers(int width, int height)
{
  // Create render buffers which are independent of render targets.
  this->DestroyBuffers();

  if (this->UserDepthBuffer)
    {
    // Attach the depth buffer to the FBO.
    vtkgl::FramebufferTexture2DEXT(vtkgl::FRAMEBUFFER_EXT,
                                   vtkgl::DEPTH_ATTACHMENT_EXT,
                                   GL_TEXTURE_2D,
                                   this->UserDepthBuffer->GetHandle(), 0);
    }
  else
    {
    if (!this->DepthBufferNeeded)
      {
      return;
      }

    GLuint temp;
    vtkgl::GenRenderbuffersEXT(1, &temp);
    this->DepthBuffer = temp;
    vtkgl::BindRenderbufferEXT(vtkgl::RENDERBUFFER_EXT, this->DepthBuffer);

    // Assign storage to this depth buffer.
    vtkgl::RenderbufferStorageEXT(vtkgl::RENDERBUFFER_EXT,
      vtkgl::DEPTH_COMPONENT24, width, height);
    // Attach the depth buffer to the FBO.
    vtkgl::FramebufferRenderbufferEXT(vtkgl::FRAMEBUFFER_EXT, 
      vtkgl::DEPTH_ATTACHMENT_EXT, 
      vtkgl::RENDERBUFFER_EXT, this->DepthBuffer);
    }
}

//----------------------------------------------------------------------------
void vtkFrameBufferObject::DestroyBuffers()
{
  if(this->DepthBuffer!=0)
    {
    GLuint temp = static_cast<GLuint>(this->DepthBuffer);
    vtkgl::DeleteRenderbuffersEXT(1, &temp);
    this->DepthBuffer = 0;
    }
}

//----------------------------------------------------------------------------
// Destroy color buffers 
void vtkFrameBufferObject::DestroyColorBuffers()
{
  this->ColorBuffers.clear();
  this->ColorBuffersDirty = true;
}

//----------------------------------------------------------------------------
void vtkFrameBufferObject::CreateColorBuffers(
  int iwidth,
  int iheight,
  bool shaderSupportsTextureInt)
{
  unsigned int width = static_cast<unsigned int>(iwidth);
  unsigned int height = static_cast <unsigned int>(iheight);

  this->ColorBuffers.resize(this->NumberOfRenderTargets);
  unsigned int cc;
  for (cc=0;
       cc < this->NumberOfRenderTargets && cc < this->UserColorBuffers.size();
       cc++)
    {
    vtkTextureObject *userBuffer=this->UserColorBuffers[cc];
    if (userBuffer)
      {
      if (userBuffer->GetNumberOfDimensions() != 2)
        {
        vtkWarningMacro("Skipping color buffer at index " << cc
          << " due to dimension mismatch.");
        continue;
        }
      if (userBuffer->GetWidth() != width || userBuffer->GetHeight() != height)
        {
        vtkWarningMacro("Skipping color buffer at index " << cc
          << " due to size mismatch.");
        continue;
        }
      this->ColorBuffers[cc] = this->UserColorBuffers[cc];
      }
    }

  for (cc=0; cc < this->NumberOfRenderTargets; cc++)
    {
    vtkSmartPointer<vtkTextureObject> colorBuffer = this->ColorBuffers[cc];
    if (!colorBuffer)
      {
      colorBuffer = vtkSmartPointer<vtkTextureObject>::New();
      colorBuffer->SetContext(this->Context);
      colorBuffer->SetMinificationFilter(vtkTextureObject::Nearest);
      colorBuffer->SetLinearMagnification(false);
      colorBuffer->SetWrapS(vtkTextureObject::Clamp);
      colorBuffer->SetWrapT(vtkTextureObject::Clamp);
      if (!colorBuffer->Create2D(width, height, 4, VTK_UNSIGNED_CHAR,
            shaderSupportsTextureInt))
        {
        vtkErrorMacro("Failed to create texture for color buffer.");
        return;
        }
      }
    //    colorBuffer->Bind(); // useless and actually error-prone.
    if (colorBuffer->GetNumberOfDimensions() == 2)
      {
      vtkgl::FramebufferTexture2DEXT(vtkgl::FRAMEBUFFER_EXT,
        vtkgl::COLOR_ATTACHMENT0_EXT+cc,
        GL_TEXTURE_2D, colorBuffer->GetHandle(), 0);
      vtkGraphicErrorMacro(this->Context,"__FILE__ __LINE__");
      }
    else if (colorBuffer->GetNumberOfDimensions() == 3)
      {
      unsigned int zSlice = this->UserZSlices[cc];
      if (zSlice >= static_cast<unsigned int>(colorBuffer->GetDepth()))
        {
        vtkErrorMacro("Invalid zSlice " << zSlice << ". Using 0.");
        zSlice = 0;
        }
      vtkgl::FramebufferTexture3DEXT(vtkgl::FRAMEBUFFER_EXT, 
        vtkgl::COLOR_ATTACHMENT0_EXT+cc,
        vtkgl::TEXTURE_3D, colorBuffer->GetHandle(), 0, zSlice);
      vtkGraphicErrorMacro(this->Context,"__FILE__ __LINE__");
      }
    this->ColorBuffers[cc] = colorBuffer;
    }
  
  unsigned int attachments=this->GetMaximumNumberOfRenderTargets();
  while(cc<attachments)
    {
    vtkgl::FramebufferRenderbufferEXT(vtkgl::FRAMEBUFFER_EXT,
                                      vtkgl::COLOR_ATTACHMENT0_EXT+cc,
                                      vtkgl::RENDERBUFFER_EXT,0);
    ++cc;
    }
  this->ColorBuffersDirty = false;
}

//----------------------------------------------------------------------------
unsigned int vtkFrameBufferObject::GetMaximumNumberOfActiveTargets()
{
  if (!this->Context)
    {
    return 0;
    }
  GLint maxbuffers;
  glGetIntegerv(vtkgl::MAX_DRAW_BUFFERS, &maxbuffers);
  vtkGraphicErrorMacro(this->Context,"__FILE__ __LINE__");
  return static_cast<unsigned int>(maxbuffers);
}

//----------------------------------------------------------------------------
unsigned int vtkFrameBufferObject::GetMaximumNumberOfRenderTargets()
{
  if (!this->Context)
    {
    return 0;
    }

  GLint maxColorAttachments;
  glGetIntegerv(vtkgl::MAX_COLOR_ATTACHMENTS_EXT,&maxColorAttachments);
  vtkGraphicErrorMacro(this->Context,"__FILE__ __LINE__");
  return static_cast<unsigned int>(maxColorAttachments);
}

//----------------------------------------------------------------------------
void vtkFrameBufferObject::SetNumberOfRenderTargets(unsigned int num)
{
  if (num == 0)
    {
    vtkErrorMacro("NumberOfRenderTargets must be >= 1");
    return;
    }
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
void vtkFrameBufferObject::SetColorBuffer(unsigned int index,
  vtkTextureObject* tex, unsigned int zslice/*=0*/)
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
  if (this->UserColorBuffers.size() > index)
    {
    return this->UserColorBuffers[index];
    }
  return 0;
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
// Display the status of the current framebuffer on the standard output.
void vtkFrameBufferObject::CheckFrameBufferStatus()
{
  GLenum status;
  status = vtkgl::CheckFramebufferStatusEXT(vtkgl::FRAMEBUFFER_EXT);
  vtkGraphicErrorMacro(this->Context,"__FILE__ __LINE__");
  switch(status)
    {
    case 0:
      cout << "call to vtkgl::CheckFramebufferStatusEXT generates an error."
           << endl;
      break;
    case vtkgl::FRAMEBUFFER_COMPLETE_EXT:
//      cout<<"framebuffer is complete"<<endl;
      break;
    case vtkgl::FRAMEBUFFER_UNSUPPORTED_EXT:
      cout << "framebuffer is unsupported" << endl;
      break;
    case vtkgl::FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT:
      cout << "framebuffer has an attachment error"<<endl;
      break;
    case vtkgl::FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT:
      cout << "framebuffer has a missing attachment"<<endl;
      break;
    case vtkgl::FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT:
      cout << "framebuffer has bad dimensions"<<endl;
      break;
    case vtkgl::FRAMEBUFFER_INCOMPLETE_FORMATS_EXT:
      cout << "framebuffer has bad formats"<<endl;
      break;
    case vtkgl::FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT:
      cout << "framebuffer has bad draw buffer"<<endl;
      break;
    case vtkgl::FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT:
      cout << "framebuffer has bad read buffer"<<endl;
      break;
    default:
      cout << "Unknown framebuffer status=0x" << hex<< status << dec << endl;
    }
  // DO NOT REMOVE THE FOLLOWING COMMENTED LINES. FOR DEBUGGING PURPOSE.
  this->DisplayFrameBufferAttachments();
  this->DisplayDrawBuffers();
  this->DisplayReadBuffer();
}

// ----------------------------------------------------------------------------
// Description:
// Display all the attachments of the current framebuffer object.
void vtkFrameBufferObject::DisplayFrameBufferAttachments()
{
  GLint framebufferBinding;
  glGetIntegerv(vtkgl::FRAMEBUFFER_BINDING_EXT,&framebufferBinding);
  vtkGraphicErrorMacro(this->Context,"after getting FRAMEBUFFER_BINDING_EXT");
  if(framebufferBinding==0)
    {
    cout<<"Current framebuffer is bind to the system one"<<endl;
    }
  else
    {
    cout<<"Current framebuffer is bind to framebuffer object "
        <<framebufferBinding<<endl;
    
    GLint maxColorAttachments;
    glGetIntegerv(vtkgl::MAX_COLOR_ATTACHMENTS_EXT,&maxColorAttachments);
    vtkGraphicErrorMacro(this->Context,"after getting MAX_COLOR_ATTACHMENTS_EXT");
    int i=0;
    while(i<maxColorAttachments)
      {
      cout<<"color attachement "<<i<<":"<<endl;
      this->DisplayFrameBufferAttachment(vtkgl::COLOR_ATTACHMENT0_EXT+i);
      ++i;
      }
    cout<<"depth attachement :"<<endl;
    this->DisplayFrameBufferAttachment(vtkgl::DEPTH_ATTACHMENT_EXT);
    cout<<"stencil attachement :"<<endl;
    this->DisplayFrameBufferAttachment(vtkgl::STENCIL_ATTACHMENT_EXT);
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
  vtkgl::GetFramebufferAttachmentParameterivEXT(
    vtkgl::FRAMEBUFFER_EXT,attachment,
    vtkgl::FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE_EXT,&params);
  
  vtkGraphicErrorMacro(this->Context,"after getting FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE_EXT");
  
  switch(params)
    {
    case GL_NONE:
      cout<<" this attachment is empty"<<endl;
      break;
    case GL_TEXTURE:
      vtkgl::GetFramebufferAttachmentParameterivEXT(
        vtkgl::FRAMEBUFFER_EXT,attachment,
        vtkgl::FRAMEBUFFER_ATTACHMENT_OBJECT_NAME_EXT,&params);
       vtkGraphicErrorMacro(this->Context,"after getting FRAMEBUFFER_ATTACHMENT_OBJECT_NAME_EXT");
      cout<<" this attachment is a texture with name: "<<params<<endl;
      vtkgl::GetFramebufferAttachmentParameterivEXT(
        vtkgl::FRAMEBUFFER_EXT,attachment,
        vtkgl::FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL_EXT,&params);
      vtkGraphicErrorMacro(this->Context,"after getting FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL_EXT");
      cout<<" its mipmap level is: "<<params<<endl;
      vtkgl::GetFramebufferAttachmentParameterivEXT(
        vtkgl::FRAMEBUFFER_EXT,attachment,
        vtkgl::FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACE_EXT,&params);
      vtkGraphicErrorMacro(this->Context,"after getting FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACE_EXT");
      if(params==0)
        {
        cout<<" this is not a cube map texture."<<endl;
        }
      else
        {
        cout<<" this is a cube map texture and the image is contained in face "
            <<params<<endl;
        }
       vtkgl::GetFramebufferAttachmentParameterivEXT(
         vtkgl::FRAMEBUFFER_EXT,attachment,
         vtkgl::FRAMEBUFFER_ATTACHMENT_TEXTURE_3D_ZOFFSET_EXT,&params);
       
       vtkGraphicErrorMacro(this->Context,"after getting FRAMEBUFFER_ATTACHMENT_TEXTURE_3D_ZOFFSET_EXT");
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
    case vtkgl::RENDERBUFFER_EXT:
      cout<<" this attachment is a renderbuffer"<<endl;
      vtkgl::GetFramebufferAttachmentParameterivEXT(
        vtkgl::FRAMEBUFFER_EXT,attachment,
        vtkgl::FRAMEBUFFER_ATTACHMENT_OBJECT_NAME_EXT,&params);
//      this->PrintError("after getting FRAMEBUFFER_ATTACHMENT_OBJECT_NAME_EXT");
      cout<<" this attachment is a renderbuffer with name: "<<params<<endl;
      
      vtkgl::BindRenderbufferEXT(vtkgl::RENDERBUFFER_EXT,params);
//      this->PrintError(
//        "after getting binding the current RENDERBUFFER_EXT to params");
      
      vtkgl::GetRenderbufferParameterivEXT(vtkgl::RENDERBUFFER_EXT,
                                           vtkgl::RENDERBUFFER_WIDTH_EXT,
                                           &params);
//      this->PrintError("after getting RENDERBUFFER_WIDTH_EXT");
      cout<<" renderbuffer width="<<params<<endl;
      vtkgl::GetRenderbufferParameterivEXT(vtkgl::RENDERBUFFER_EXT,
                                           vtkgl::RENDERBUFFER_HEIGHT_EXT,
                                           &params);
//      this->PrintError("after getting RENDERBUFFER_HEIGHT_EXT");
      cout<<" renderbuffer height="<<params<<endl;
      vtkgl::GetRenderbufferParameterivEXT(
        vtkgl::RENDERBUFFER_EXT,vtkgl::RENDERBUFFER_INTERNAL_FORMAT_EXT,
        &params);
//      this->PrintError("after getting RENDERBUFFER_INTERNAL_FORMAT_EXT");
      
      cout<<" renderbuffer internal format=0x"<< hex<<params<<dec<<endl;
      
      vtkgl::GetRenderbufferParameterivEXT(vtkgl::RENDERBUFFER_EXT,
                                           vtkgl::RENDERBUFFER_RED_SIZE_EXT,
                                           &params);
//      this->PrintError("after getting RENDERBUFFER_RED_SIZE_EXT");
      cout<<" renderbuffer actual resolution for the red component="<<params
          <<endl;
      vtkgl::GetRenderbufferParameterivEXT(vtkgl::RENDERBUFFER_EXT,
                                           vtkgl::RENDERBUFFER_GREEN_SIZE_EXT,
                                           &params);
//      this->PrintError("after getting RENDERBUFFER_GREEN_SIZE_EXT");
      cout<<" renderbuffer actual resolution for the green component="<<params
          <<endl;
      vtkgl::GetRenderbufferParameterivEXT(vtkgl::RENDERBUFFER_EXT,
                                           vtkgl::RENDERBUFFER_BLUE_SIZE_EXT,
                                           &params);
//      this->PrintError("after getting RENDERBUFFER_BLUE_SIZE_EXT");
      cout<<" renderbuffer actual resolution for the blue component="<<params
          <<endl;
      vtkgl::GetRenderbufferParameterivEXT(vtkgl::RENDERBUFFER_EXT,
                                           vtkgl::RENDERBUFFER_ALPHA_SIZE_EXT,
                                           &params);
//      this->PrintError("after getting RENDERBUFFER_ALPHA_SIZE_EXT");
      cout<<" renderbuffer actual resolution for the alpha component="<<params
          <<endl;
      vtkgl::GetRenderbufferParameterivEXT(vtkgl::RENDERBUFFER_EXT,
                                           vtkgl::RENDERBUFFER_DEPTH_SIZE_EXT,
                                           &params);
//      this->PrintError("after getting RENDERBUFFER_DEPTH_SIZE_EXT");
      cout<<" renderbuffer actual resolution for the depth component="<<params
          <<endl;
      vtkgl::GetRenderbufferParameterivEXT(
        vtkgl::RENDERBUFFER_EXT,vtkgl::RENDERBUFFER_STENCIL_SIZE_EXT,&params);
//      this->PrintError("after getting RENDERBUFFER_STENCIL_SIZE_EXT");
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
  GLint ivalue;
  glGetIntegerv(vtkgl::MAX_DRAW_BUFFERS,&ivalue);
  
  cout<<"there ";
  if(ivalue<=1)
    {
    cout << "is ";
    }
  else
    {
    cout << "are ";
    }
  cout << ivalue << " draw buffer";
  if(ivalue>1)
    {
    cout<<"s";
    }
  cout<<". "<<endl;
  GLint i=0;
  int c=ivalue;
  while(i<c)
    {
    glGetIntegerv(vtkgl::DRAW_BUFFER0+i,&ivalue);
    
    cout << "draw buffer["<<i<<"]=";
    this->DisplayBuffer(ivalue);
    cout << endl;
    ++i;
    }
}
  
// ----------------------------------------------------------------------------
// Description:
// Display the read buffer.
void vtkFrameBufferObject::DisplayReadBuffer()
{
  GLint ivalue;
  glGetIntegerv(GL_READ_BUFFER,&ivalue);
  cout << "read buffer=";
  this->DisplayBuffer(ivalue);
  cout<<endl;
}

// ----------------------------------------------------------------------------
// Description:
// Display any buffer (convert value into string).
void vtkFrameBufferObject::DisplayBuffer(int value)
{
  if(value>=static_cast<int>(vtkgl::COLOR_ATTACHMENT0_EXT) &&
     value<=static_cast<int>(vtkgl::COLOR_ATTACHMENT15_EXT))
    {
    cout << "GL_COLOR_ATTACHMENT" << (value-vtkgl::COLOR_ATTACHMENT0_EXT);
    }
  else
    {
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
             << (ivalue-1) << ", raw value is 0x" << hex << (GL_AUX0+b)
             << dec;
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
          cout << "unknown 0x" << hex << value << dec;
          break;
        }
      }
    }
}

// ---------------------------------------------------------------------------
void vtkFrameBufferObject::RenderQuad(int minX,
                                         int maxX,
                                         int minY,
                                         int maxY)
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
  vtkgl::GenQueries(1,&queryId);
  vtkgl::BeginQuery(vtkgl::SAMPLES_PASSED,queryId);
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
  glBegin(GL_QUADS);
  glTexCoord2f(0, 0);
  glVertex2f(minX, minY);
  glTexCoord2f(1.0, 0);
  glVertex2f(maxX+1, minY);
  glTexCoord2f(1.0, maxYTexCoord);
  glVertex2f(maxX+1, maxY+1);
  glTexCoord2f(0, maxYTexCoord);
  glVertex2f(minX, maxY+1);
  glEnd();
#ifdef VTK_FBO_DEBUG
  vtkgl::EndQuery(vtkgl::SAMPLES_PASSED);
  vtkgl::GetQueryObjectuiv(queryId,vtkgl::QUERY_RESULT,&nbPixels);
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
