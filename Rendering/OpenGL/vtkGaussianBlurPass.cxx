/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGaussianBlurPass.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkGaussianBlurPass.h"
#include "vtkObjectFactory.h"
#include <assert.h>
#include "vtkRenderState.h"
#include "vtkRenderer.h"
#include "vtkgl.h"
#include "vtkFrameBufferObject.h"
#include "vtkTextureObject.h"
#include "vtkShaderProgram2.h"
#include "vtkShader2.h"
#include "vtkShader2Collection.h"
#include "vtkUniformVariables.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkTextureUnitManager.h"

// to be able to dump intermediate passes into png files for debugging.
// only for vtkGaussianBlurPass developers.
//#define VTK_GAUSSIAN_BLUR_PASS_DEBUG

#include "vtkPNGWriter.h"
#include "vtkImageImport.h"
#include "vtkPixelBufferObject.h"
#include "vtkPixelBufferObject.h"
#include "vtkImageExtractComponents.h"
#include "vtkCamera.h"
#include "vtkMath.h"

vtkStandardNewMacro(vtkGaussianBlurPass);

extern const char *vtkGaussianBlurPassShader_fs;


// ----------------------------------------------------------------------------
vtkGaussianBlurPass::vtkGaussianBlurPass()
{
  this->FrameBufferObject=0;
  this->Pass1=0;
  this->Pass2=0;
  this->BlurProgram=0;
  this->Supported=false;
  this->SupportProbed=false;
}

// ----------------------------------------------------------------------------
vtkGaussianBlurPass::~vtkGaussianBlurPass()
{
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
   if(this->BlurProgram!=0)
     {
     this->BlurProgram->Delete();
     }
}

// ----------------------------------------------------------------------------
void vtkGaussianBlurPass::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

// ----------------------------------------------------------------------------
// Description:
// Perform rendering according to a render state \p s.
// \pre s_exists: s!=0
void vtkGaussianBlurPass::Render(const vtkRenderState *s)
{
  assert("pre: s_exists" && s!=0);

  this->NumberOfRenderedProps=0;
  
  vtkRenderer *r=s->GetRenderer();
  
  if(this->DelegatePass!=0)
    {
    if(!this->SupportProbed)
      {
      this->SupportProbed=true;
      // Test for Hardware support. If not supported, just render the delegate.
      bool supported=vtkFrameBufferObject::IsSupported(r->GetRenderWindow());
      
      if(!supported)
        {
        vtkErrorMacro("FBOs are not supported by the context. Cannot blur the image.");
        }
      if(supported)
        {
        supported=vtkTextureObject::IsSupported(r->GetRenderWindow());
        if(!supported)
          {
          vtkErrorMacro("Texture Objects are not supported by the context. Cannot blur the image.");
          }
        }
      
      if(supported)
        {
        supported=
          vtkShaderProgram2::IsSupported(static_cast<vtkOpenGLRenderWindow *>(
                                           r->GetRenderWindow()));
        if(!supported)
          {
          vtkErrorMacro("GLSL is not supported by the context. Cannot blur the image.");
          }
        }
      
      if(supported)
        {
        // FBO extension is supported. Is the specific FBO format supported?
        if(this->FrameBufferObject==0)
          {
          this->FrameBufferObject=vtkFrameBufferObject::New();
          this->FrameBufferObject->SetContext(r->GetRenderWindow());
          }
        if(this->Pass1==0)
          {
          this->Pass1=vtkTextureObject::New();
          this->Pass1->SetContext(r->GetRenderWindow());
          }
        this->Pass1->Create2D(64,64,4,VTK_UNSIGNED_CHAR,false);
        this->FrameBufferObject->SetColorBuffer(0,this->Pass1);
        this->FrameBufferObject->SetNumberOfRenderTargets(1);
        this->FrameBufferObject->SetActiveBuffer(0);
        this->FrameBufferObject->SetDepthBufferNeeded(true);
        
        GLint savedCurrentDrawBuffer;
        glGetIntegerv(GL_DRAW_BUFFER,&savedCurrentDrawBuffer);
        supported=this->FrameBufferObject->StartNonOrtho(64,64,false);
        if(!supported)
          {
          vtkErrorMacro("The requested FBO format is not supported by the context. Cannot blur the image.");
          }
        else
          {
          this->FrameBufferObject->UnBind();
          glDrawBuffer(static_cast<GLenum>(savedCurrentDrawBuffer));
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
    
    GLint savedDrawBuffer;
    glGetIntegerv(GL_DRAW_BUFFER,&savedDrawBuffer);
    
    // 1. Create a new render state with an FBO.
    
    int width;
    int height;
    int size[2];
    s->GetWindowSize(size);
    width=size[0];
    height=size[1];
    
    const int extraPixels=2; // two on each side, as the kernel is 5x5
    
    int w=width+extraPixels*2;
    int h=height+extraPixels*2;
    
    if(this->Pass1==0)
      {
      this->Pass1=vtkTextureObject::New();
      this->Pass1->SetContext(r->GetRenderWindow());
      }
    
    if(this->FrameBufferObject==0)
      {
      this->FrameBufferObject=vtkFrameBufferObject::New();
      this->FrameBufferObject->SetContext(r->GetRenderWindow());
      }
    
    this->RenderDelegate(s,width,height,w,h,this->FrameBufferObject,
                         this->Pass1);

#ifdef VTK_GAUSSIAN_BLUR_PASS_DEBUG
    // Save first pass in file for debugging.
    vtkPixelBufferObject *pbo=this->Pass1->Download();
    
    unsigned char *openglRawData=new unsigned char[4*w*h];
    unsigned int dims[2];
    dims[0]=w;
    dims[1]=h;
    vtkIdType incs[2];
    incs[0]=0;
    incs[1]=0;
    bool status=pbo->Download2D(VTK_UNSIGNED_CHAR,openglRawData,dims,4,incs);
    assert("check" && status);
    pbo->Delete();
    
    // no pbo
    this->Pass1->Bind();
    glGetTexImage(GL_TEXTURE_2D,0,GL_RGBA,GL_UNSIGNED_BYTE,openglRawData);
    this->Pass1->UnBind();
    
    vtkImageImport *importer=vtkImageImport::New();
    importer->CopyImportVoidPointer(openglRawData,4*w*h*sizeof(unsigned char));
    importer->SetDataScalarTypeToUnsignedChar();
    importer->SetNumberOfScalarComponents(4);
    importer->SetWholeExtent(0,w-1,0,h-1,0,0);
    importer->SetDataExtentToWholeExtent();
    delete[] openglRawData;
    
    vtkImageExtractComponents *rgbaToRgb=vtkImageExtractComponents::New();
    rgbaToRgb->SetInputConnection(importer->GetOutputPort());
    rgbaToRgb->SetComponents(0,1,2);
    
    vtkPNGWriter *writer=vtkPNGWriter::New();
    writer->SetFileName("BlurPass1.png");
    writer->SetInputConnection(rgbaToRgb->GetOutputPort());
    importer->Delete();
    rgbaToRgb->Delete();
    writer->Write();
    writer->Delete();
#endif
    
    // 3. Same FBO, but new color attachment (new TO).
    if(this->Pass2==0)
      {
      this->Pass2=vtkTextureObject::New();
      this->Pass2->SetContext(this->FrameBufferObject->GetContext());
      }
    
    if(this->Pass2->GetWidth()!=static_cast<unsigned int>(w) ||
       this->Pass2->GetHeight()!=static_cast<unsigned int>(h))
      {
      this->Pass2->Create2D(static_cast<unsigned int>(w),
                            static_cast<unsigned int>(h),4,
                            VTK_UNSIGNED_CHAR,false);
      }
    
    this->FrameBufferObject->SetColorBuffer(0,this->Pass2);
    this->FrameBufferObject->Start(w,h,false);
    
#ifdef VTK_GAUSSIAN_BLUR_PASS_DEBUG
    cout << "gauss finish2" << endl;
    glFinish();
#endif
    
    // Use a blur shader, do it horizontally. this->Pass1 is the source
    // (this->Pass2 is the fbo render target)
    
    if(this->BlurProgram==0)
      {
      this->BlurProgram=vtkShaderProgram2::New();
      this->BlurProgram->SetContext(
        static_cast<vtkOpenGLRenderWindow *>(
          this->FrameBufferObject->GetContext()));
      vtkShader2 *shader=vtkShader2::New();
      shader->SetType(VTK_SHADER_TYPE_FRAGMENT);
      shader->SetSourceCode(vtkGaussianBlurPassShader_fs);
      shader->SetContext(this->BlurProgram->GetContext());
      this->BlurProgram->GetShaders()->AddItem(shader);
      shader->Delete();
      }
    
    this->BlurProgram->Build();
    
#ifdef VTK_GAUSSIAN_BLUR_PASS_DEBUG
    this->BlurProgram->PrintActiveUniformVariablesOnCout();
#endif
    
#ifdef VTK_GAUSSIAN_BLUR_PASS_DEBUG
    cout << "gauss finish build" << endl;
    glFinish();
#endif
    
    if(this->BlurProgram->GetLastBuildStatus()!=VTK_SHADER_PROGRAM2_LINK_SUCCEEDED)
      {
      vtkErrorMacro("Couldn't build the shader program. At this point , it can be an error in a shader or a driver bug.");
      
      // restore some state.
      this->FrameBufferObject->UnBind();
      glDrawBuffer(static_cast<GLenum>(savedDrawBuffer));
      return;
      }
    
    vtkUniformVariables *var=this->BlurProgram->GetUniformVariables();
    vtkTextureUnitManager *tu=
      static_cast<vtkOpenGLRenderWindow *>(r->GetRenderWindow())->GetTextureUnitManager();
    
    int sourceId=tu->Allocate();
    vtkgl::ActiveTexture(vtkgl::TEXTURE0+static_cast<GLenum>(sourceId));
    this->Pass1->Bind();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    var->SetUniformi("source",1,&sourceId);
    
    float fvalues[3];
    
    static float kernel[3]={5.0f,6.0f,5.0f};
    
    int i=0;
    float sum=0.0f;
    while(i<3)
      {
      sum+=kernel[i];
      ++i;
      }
    
    // kernel
    i=0;
    while(i<3)
      {
      fvalues[i]=kernel[i]/sum;
      ++i;
      }
    var->SetUniformf("coef[0]",1,fvalues);
    var->SetUniformf("coef[1]",1,fvalues+1);
    var->SetUniformf("coef[2]",1,fvalues+2);
    
    // horizontal offset
    fvalues[0]=static_cast<float>(1.2/w);
    var->SetUniformf("offsetx",1,fvalues);
    fvalues[0]=0.0f;
    var->SetUniformf("offsety",1,fvalues);
    
    this->BlurProgram->Use();
    
#ifdef VTK_GAUSSIAN_BLUR_PASS_DEBUG
    cout << "gauss finish use" << endl;
    glFinish();
#endif
    
#ifdef VTK_GAUSSIAN_BLUR_PASS_DEBUG
    this->BlurProgram->PrintActiveUniformVariablesOnCout();
#endif
    
    if(!this->BlurProgram->IsValid())
      {
      vtkErrorMacro(<<this->BlurProgram->GetLastValidateLog());
      }
#ifdef VTK_GAUSSIAN_BLUR_PASS_DEBUG
    cout << "gauss finish3-" << endl;
    glFinish();
#endif
    
    this->FrameBufferObject->RenderQuad(0,w-1,0,h-1);
    
#ifdef VTK_GAUSSIAN_BLUR_PASS_DEBUG
    cout << "gauss finish3" << endl;
    glFinish();
#endif
    
    this->Pass1->UnBind();
    
#ifdef VTK_GAUSSIAN_BLUR_PASS_DEBUG
    
    // Save second pass in file for debugging.
    pbo=this->Pass2->Download();
    openglRawData=new unsigned char[4*w*h];
    status=pbo->Download2D(VTK_UNSIGNED_CHAR,openglRawData,dims,4,incs);
    assert("check2" && status);
    pbo->Delete();
    
    importer=vtkImageImport::New();
    importer->CopyImportVoidPointer(openglRawData,4*w*h*sizeof(unsigned char));
    importer->SetDataScalarTypeToUnsignedChar();
    importer->SetNumberOfScalarComponents(4);
    importer->SetWholeExtent(0,w-1,0,h-1,0,0);
    importer->SetDataExtentToWholeExtent();
    delete[] openglRawData;
    
    rgbaToRgb=vtkImageExtractComponents::New();
    rgbaToRgb->SetInputConnection(importer->GetOutputPort());
    rgbaToRgb->SetComponents(0,1,2);
    
    writer=vtkPNGWriter::New();
    writer->SetFileName("BlurPass2.png");
    writer->SetInputConnection(rgbaToRgb->GetOutputPort());
    importer->Delete();
    rgbaToRgb->Delete();
    writer->Write();
    writer->Delete();
#endif
    
    // 4. Render in original FB (from renderstate in arg)
    
    this->FrameBufferObject->UnBind();
    
    glDrawBuffer(static_cast<GLenum>(savedDrawBuffer));
    
    // to2 is the source
    
    this->Pass2->Bind();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    
    // Use the same blur shader, do it vertically.
    
    // vertical offset.
    fvalues[0]=0.0f;
    var->SetUniformf("offsetx",1,fvalues);
    fvalues[0]=static_cast<float>(1.2/h);
    var->SetUniformf("offsety",1,fvalues);
    
    this->BlurProgram->SendUniforms();
    if(!this->BlurProgram->IsValid())
      {
      vtkErrorMacro(<<this->BlurProgram->GetLastValidateLog());
      }
    
    
    // Prepare blitting
    glDisable(GL_ALPHA_TEST);
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glDisable(GL_SCISSOR_TEST);
    
    this->Pass2->CopyToFrameBuffer(extraPixels,extraPixels,
                                   w-1-extraPixels,h-1-extraPixels,
                                   0,0,width,height);
    
    this->Pass2->UnBind();
    tu->Free(sourceId);
    vtkgl::ActiveTexture(vtkgl::TEXTURE0);
    
#ifdef VTK_GAUSSIAN_BLUR_PASS_DEBUG
    cout << "gauss finish4" << endl;
    glFinish();
#endif
    
    this->BlurProgram->Restore();
    }
  else
    {
    vtkWarningMacro(<<" no delegate.");
    }
}

// ----------------------------------------------------------------------------
// Description:
// Release graphics resources and ask components to release their own
// resources.
// \pre w_exists: w!=0
void vtkGaussianBlurPass::ReleaseGraphicsResources(vtkWindow *w)
{
  assert("pre: w_exists" && w!=0);
  
  this->Superclass::ReleaseGraphicsResources(w);
  
  if(this->BlurProgram!=0)
    {
    this->BlurProgram->ReleaseGraphicsResources();
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
}
