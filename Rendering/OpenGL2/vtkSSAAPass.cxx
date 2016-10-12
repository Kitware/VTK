/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSSAAPass.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkSSAAPass.h"
#include "vtkObjectFactory.h"
#include <cassert>
#include "vtkRenderState.h"
#include "vtkRenderer.h"
#include "vtkFrameBufferObject.h"
#include "vtkTextureObject.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLError.h"
#include "vtkShaderProgram.h"
#include "vtkOpenGLShaderCache.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLVertexArrayObject.h"

#include "vtkOpenGLHelper.h"

#include "vtkSSAAPassFS.h"
#include "vtkTextureObjectVS.h"  // a pass through shader

vtkStandardNewMacro(vtkSSAAPass);

vtkCxxSetObjectMacro(vtkSSAAPass,DelegatePass,vtkRenderPass);

// ----------------------------------------------------------------------------
vtkSSAAPass::vtkSSAAPass()
{
  this->FrameBufferObject = 0;
  this->Pass1 = 0;
  this->Pass2 = 0;
  this->Supported = false;
  this->SupportProbed = false;
  this->SSAAProgram = NULL;
  this->DelegatePass = 0;
}

// ----------------------------------------------------------------------------
vtkSSAAPass::~vtkSSAAPass()
{
  if(this->DelegatePass!=0)
  {
      this->DelegatePass->Delete();
  }

  if(this->FrameBufferObject!=0)
  {
    vtkErrorMacro(<<"FrameBufferObject should have been deleted in ReleaseGraphicsResources().");
  }
   if(this->Pass1!=0)
   {
    vtkErrorMacro(<<"Pass1 should have been deleted in ReleaseGraphicsResources().");
   }
   if(this->Pass2!=0)
   {
    vtkErrorMacro(<<"Pass2 should have been deleted in ReleaseGraphicsResources().");
   }
}

// ----------------------------------------------------------------------------
void vtkSSAAPass::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "DelegatePass:";
  if(this->DelegatePass!=0)
  {
    this->DelegatePass->PrintSelf(os,indent);
  }
  else
  {
    os << "(none)" <<endl;
  }
}

// ----------------------------------------------------------------------------
// Description:
// Perform rendering according to a render state \p s.
// \pre s_exists: s!=0
void vtkSSAAPass::Render(const vtkRenderState *s)
{
  assert("pre: s_exists" && s!=0);

  vtkOpenGLClearErrorMacro();

  this->NumberOfRenderedProps=0;

  vtkRenderer *r=s->GetRenderer();
  vtkOpenGLRenderWindow *renWin = static_cast<vtkOpenGLRenderWindow *>(r->GetRenderWindow());

  if(this->DelegatePass == 0)
  {
    vtkWarningMacro(<<" no delegate.");
    return;
  }

  if(!this->SupportProbed)
  {
    this->SupportProbed=true;
    // Test for Hardware support. If not supported, just render the delegate.
    bool supported=vtkFrameBufferObject::IsSupported(renWin);

    if(!supported)
    {
      vtkErrorMacro("FBOs are not supported by the context. Cannot blur the image.");
    }

    if(supported)
    {
      // FBO extension is supported. Is the specific FBO format supported?
      if(this->FrameBufferObject==0)
      {
        this->FrameBufferObject=vtkFrameBufferObject::New();
        this->FrameBufferObject->SetContext(renWin);
      }
      if(this->Pass1==0)
      {
        this->Pass1=vtkTextureObject::New();
        this->Pass1->SetContext(renWin);
      }
      this->Pass1->Create2D(64,64,4,VTK_UNSIGNED_CHAR,false);
      this->FrameBufferObject->SetColorBuffer(0,this->Pass1);
      this->FrameBufferObject->SetNumberOfRenderTargets(1);
      this->FrameBufferObject->SetActiveBuffer(0);
      this->FrameBufferObject->SetDepthBufferNeeded(true);

#if GL_ES_VERSION_2_0 != 1
      GLint savedCurrentDrawBuffer;
      glGetIntegerv(GL_DRAW_BUFFER,&savedCurrentDrawBuffer);
#endif
      supported=this->FrameBufferObject->StartNonOrtho(64,64,false);
      if(!supported)
      {
        vtkErrorMacro("The requested FBO format is not supported by the context. Cannot blur the image.");
      }
      else
      {
        this->FrameBufferObject->UnBind();
#if GL_ES_VERSION_2_0 != 1
        glDrawBuffer(static_cast<GLenum>(savedCurrentDrawBuffer));
#endif
      }
    }

    this->Supported=supported;
  }

  if(!this->Supported)
  {
    this->DelegatePass->Render(s);
    this->NumberOfRenderedProps+=
      this->DelegatePass->GetNumberOfRenderedProps();
    return;
  }

#if GL_ES_VERSION_2_0 != 1
  GLint savedDrawBuffer;
  glGetIntegerv(GL_DRAW_BUFFER,&savedDrawBuffer);
#endif

  // 1. Create a new render state with an FBO.
  int width;
  int height;
  int size[2];
  s->GetWindowSize(size);
  width=size[0];
  height=size[1];

  int w = width*sqrt(5.0);
  int h = height*sqrt(5.0);

  if(this->Pass1==0)
  {
    this->Pass1=vtkTextureObject::New();
    this->Pass1->SetContext(renWin);
  }

  if(this->FrameBufferObject==0)
  {
    this->FrameBufferObject=vtkFrameBufferObject::New();
    this->FrameBufferObject->SetContext(renWin);
  }

  if(this->Pass1->GetWidth() != static_cast<unsigned int>(w) ||
     this->Pass1->GetHeight() != static_cast<unsigned int>(h))
  {
    this->Pass1->Create2D(static_cast<unsigned int>(w),
                          static_cast<unsigned int>(h),4,
                          VTK_UNSIGNED_CHAR,false);
  }

  vtkRenderState s2(r);
  s2.SetPropArrayAndCount(s->GetPropArray(),s->GetPropArrayCount());
  s2.SetFrameBuffer(this->FrameBufferObject);

  this->FrameBufferObject->SetNumberOfRenderTargets(1);
  this->FrameBufferObject->SetColorBuffer(0,this->Pass1);
  this->FrameBufferObject->SetActiveBuffer(0);

  this->FrameBufferObject->SetDepthBufferNeeded(true);
  this->FrameBufferObject->StartNonOrtho(w,h,false);
  glViewport(0, 0, w, h);
  glScissor(0, 0, w, h);

  glEnable(GL_DEPTH_TEST);
  this->DelegatePass->Render(&s2);
  this->NumberOfRenderedProps +=
    this->DelegatePass->GetNumberOfRenderedProps();

  // 3. Same FBO, but new color attachment (new TO).
  if(this->Pass2==0)
  {
    this->Pass2=vtkTextureObject::New();
    this->Pass2->SetContext(this->FrameBufferObject->GetContext());
  }

  if(this->Pass2->GetWidth()!=static_cast<unsigned int>(width) ||
     this->Pass2->GetHeight()!=static_cast<unsigned int>(h))
  {
    this->Pass2->Create2D(static_cast<unsigned int>(width),
                          static_cast<unsigned int>(h),4,
                          VTK_UNSIGNED_CHAR,false);
  }

  this->FrameBufferObject->SetColorBuffer(0,this->Pass2);
  this->FrameBufferObject->Start(width,h,false);

  // Use a subsample shader, do it horizontally. this->Pass1 is the source
  // (this->Pass2 is the fbo render target)

  if (!this->SSAAProgram)
  {
    this->SSAAProgram = new vtkOpenGLHelper;
    // build the shader source code
//    std::string VSSource = vtkSSAAPassVS;
    std::string VSSource = vtkTextureObjectVS;
    std::string FSSource = vtkSSAAPassFS;
    std::string GSSource;

    // compile and bind it if needed
    vtkShaderProgram *newShader =
      renWin->GetShaderCache()->ReadyShaderProgram(
        VSSource.c_str(),
        FSSource.c_str(),
        GSSource.c_str());

    // if the shader changed reinitialize the VAO
    if (newShader != this->SSAAProgram->Program)
    {
      this->SSAAProgram->Program = newShader;
      this->SSAAProgram->VAO->ShaderProgramChanged(); // reset the VAO as the shader has changed
    }

    this->SSAAProgram->ShaderSourceTime.Modified();
  }
  else
  {
    renWin->GetShaderCache()->ReadyShaderProgram(this->SSAAProgram->Program);
  }

  if(this->SSAAProgram->Program->GetCompiled() != true)
  {
    vtkErrorMacro("Couldn't build the shader program. At this point , it can be an error in a shader or a driver bug.");

    // restore some state.
    this->FrameBufferObject->UnBind();
#if GL_ES_VERSION_2_0 != 1
    glDrawBuffer(static_cast<GLenum>(savedDrawBuffer));
#endif
    return;
  }

  this->Pass1->Activate();
  int sourceId = this->Pass1->GetTextureUnit();
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  this->SSAAProgram->Program->SetUniformi("source",sourceId);
  // The implementation uses four steps to cover 1.5 destination pixels
  // so the offset is 1.5/4.0 = 0.375
  this->SSAAProgram->Program->SetUniformf("texelWidthOffset", 0.375/width);
  this->SSAAProgram->Program->SetUniformf("texelHeightOffset", 0.0);

  glDisable(GL_BLEND);
  glDisable(GL_DEPTH_TEST);

  this->FrameBufferObject->RenderQuad(0,width-1,0,h-1,
    this->SSAAProgram->Program, this->SSAAProgram->VAO);

  this->Pass1->Deactivate();

  // 4. Render in original FB (from renderstate in arg)

  this->FrameBufferObject->UnBind();

#if GL_ES_VERSION_2_0 != 1
  glDrawBuffer(static_cast<GLenum>(savedDrawBuffer));
#endif

  // to2 is the source
  this->Pass2->Activate();
  sourceId = this->Pass2->GetTextureUnit();
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  this->SSAAProgram->Program->SetUniformi("source",sourceId);
  this->SSAAProgram->Program->SetUniformf("texelWidthOffset", 0.0);
  this->SSAAProgram->Program->SetUniformf("texelHeightOffset", 0.375/height);

  // Use the same sample shader, this time vertical

  this->Pass2->CopyToFrameBuffer(0, 0,
                                width-1,h-1,
                                0,0, width-1, height-1,
                                width, height,
                                this->SSAAProgram->Program,
                                this->SSAAProgram->VAO);

  this->Pass2->Deactivate();

  vtkOpenGLCheckErrorMacro("failed after Render");
}

// ----------------------------------------------------------------------------
// Description:
// Release graphics resources and ask components to release their own
// resources.
// \pre w_exists: w!=0
void vtkSSAAPass::ReleaseGraphicsResources(vtkWindow *w)
{
  assert("pre: w_exists" && w!=0);

  this->Superclass::ReleaseGraphicsResources(w);

  if (this->SSAAProgram !=0)
  {
    this->SSAAProgram->ReleaseGraphicsResources(w);
    delete this->SSAAProgram;
    this->SSAAProgram = 0;
  }
  if(this->FrameBufferObject!=0)
  {
    this->FrameBufferObject->Delete();
    this->FrameBufferObject=0;
  }
   if(this->Pass1!=0)
   {
    this->Pass1->Delete();
    this->Pass1=0;
   }
   if(this->Pass2!=0)
   {
    this->Pass2->Delete();
    this->Pass2=0;
   }
  if(this->DelegatePass!=0)
  {
    this->DelegatePass->ReleaseGraphicsResources(w);
  }
}
