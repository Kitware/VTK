/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPointFillPass.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkPointFillPass.h"
#include "vtkObjectFactory.h"
#include <cassert>

#include "vtkCamera.h"
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

#include "vtkPointFillPassFS.h"
#include "vtkTextureObjectVS.h"

vtkStandardNewMacro(vtkPointFillPass);

// ----------------------------------------------------------------------------
vtkPointFillPass::vtkPointFillPass()
{
  this->FrameBufferObject=0;
  this->Pass1=0;
  this->Pass1Depth=0;
  this->Supported=false;
  this->SupportProbed=false;
  this->BlurProgram = NULL;
  this->MinimumCandidateAngle = 1.5*3.1415926;
  this->CandidatePointRatio = 0.99;
}

// ----------------------------------------------------------------------------
vtkPointFillPass::~vtkPointFillPass()
{
  if(this->FrameBufferObject!=0)
  {
    vtkErrorMacro(<<"FrameBufferObject should have been deleted in ReleaseGraphicsResources().");
  }
   if(this->Pass1!=0)
   {
    vtkErrorMacro(<<"Pass1 should have been deleted in ReleaseGraphicsResources().");
   }
   if(this->Pass1Depth!=0)
   {
    vtkErrorMacro(<<"Pass1Depth should have been deleted in ReleaseGraphicsResources().");
   }
}

// ----------------------------------------------------------------------------
void vtkPointFillPass::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

// ----------------------------------------------------------------------------
// Description:
// Perform rendering according to a render state \p s.
// \pre s_exists: s!=0
void vtkPointFillPass::Render(const vtkRenderState *s)
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
  width = size[0];
  height = size[1];

  const int extraPixels = 0;

  int w = width + extraPixels*2;
  int h = height + extraPixels*2;

  if(this->Pass1==0)
  {
    this->Pass1 = vtkTextureObject::New();
    this->Pass1->SetContext(renWin);
  }
  if(this->Pass1->GetWidth()!=static_cast<unsigned int>(w) ||
     this->Pass1->GetHeight()!=static_cast<unsigned int>(h))
  {
    this->Pass1->Create2D(static_cast<unsigned int>(w),
                          static_cast<unsigned int>(h),4,
                          VTK_UNSIGNED_CHAR,false);
  }

  // Depth texture
  if (this->Pass1Depth == 0)
  {
    this->Pass1Depth = vtkTextureObject::New();
    this->Pass1Depth->SetContext(renWin);
  }
  if (this->Pass1Depth->GetWidth() != static_cast<unsigned int> (w)
      || this->Pass1Depth->GetHeight() != static_cast<unsigned int> (h))
  {
    this->Pass1Depth->AllocateDepth(
      w, h, vtkTextureObject::Float32);
  }

  if(this->FrameBufferObject==0)
  {
    this->FrameBufferObject=vtkFrameBufferObject::New();
    this->FrameBufferObject->SetContext(renWin);
  }

  this->RenderDelegate(s,width,height,w,h,this->FrameBufferObject,
                       this->Pass1, this->Pass1Depth);

  this->FrameBufferObject->UnBind();

#if GL_ES_VERSION_2_0 != 1
  glDrawBuffer(static_cast<GLenum>(savedDrawBuffer));
#endif

  // has something changed that would require us to recreate the shader?
  if (!this->BlurProgram)
  {
    this->BlurProgram = new vtkOpenGLHelper;
    // build the shader source code
    std::string VSSource = vtkTextureObjectVS;
    std::string FSSource = vtkPointFillPassFS;
    std::string GSSource;

    // compile and bind it if needed
    vtkShaderProgram *newShader =
      renWin->GetShaderCache()->ReadyShaderProgram(
        VSSource.c_str(),
        FSSource.c_str(),
        GSSource.c_str());

    // if the shader changed reinitialize the VAO
    if (newShader != this->BlurProgram->Program)
    {
      this->BlurProgram->Program = newShader;
      this->BlurProgram->VAO->ShaderProgramChanged(); // reset the VAO as the shader has changed
    }

    this->BlurProgram->ShaderSourceTime.Modified();
  }
  else
  {
    renWin->GetShaderCache()->ReadyShaderProgram(this->BlurProgram->Program);
  }

  glDisable(GL_BLEND);
//  glDisable(GL_DEPTH_TEST);

  this->Pass1->Activate();
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  this->BlurProgram->Program->SetUniformi("source",this->Pass1->GetTextureUnit());

  this->Pass1Depth->Activate();
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  this->BlurProgram->Program->SetUniformi("depth",this->Pass1Depth->GetTextureUnit());

  vtkCamera *cam = r->GetActiveCamera();
  double *frange = cam->GetClippingRange();
  this->BlurProgram->Program->SetUniformf("nearC",frange[0]);
  this->BlurProgram->Program->SetUniformf("farC",frange[1]);
  this->BlurProgram->Program->SetUniformf("MinimumCandidateAngle",
    this->MinimumCandidateAngle);
  this->BlurProgram->Program->SetUniformf("CandidatePointRatio",
    this->CandidatePointRatio);
  float offset[2];
  offset[0] = 1.0/w;
  offset[1] = 1.0/h;
  this->BlurProgram->Program->SetUniform2f("pixelToTCoord", offset);

  this->Pass1->CopyToFrameBuffer(extraPixels, extraPixels,
                                w-1-extraPixels,h-1-extraPixels,
                                0,0, width, height,
                                this->BlurProgram->Program,
                                this->BlurProgram->VAO);

  this->Pass1->Deactivate();
  this->Pass1Depth->Deactivate();

  vtkOpenGLCheckErrorMacro("failed after Render");
}

// ----------------------------------------------------------------------------
// Description:
// Release graphics resources and ask components to release their own
// resources.
// \pre w_exists: w!=0
void vtkPointFillPass::ReleaseGraphicsResources(vtkWindow *w)
{
  assert("pre: w_exists" && w!=0);

  this->Superclass::ReleaseGraphicsResources(w);

  if (this->BlurProgram !=0)
  {
    this->BlurProgram->ReleaseGraphicsResources(w);
    delete this->BlurProgram;
    this->BlurProgram = 0;
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
   if(this->Pass1Depth!=0)
   {
    this->Pass1Depth->Delete();
    this->Pass1Depth=0;
   }
}
