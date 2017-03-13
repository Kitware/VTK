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
#include <cassert>
#include "vtkRenderState.h"
#include "vtkRenderer.h"
#include "vtkOpenGLFramebufferObject.h"
#include "vtkTextureObject.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLError.h"
#include "vtkShaderProgram.h"
#include "vtkOpenGLShaderCache.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLVertexArrayObject.h"

#include "vtkOpenGLHelper.h"

// to be able to dump intermediate passes into png files for debugging.
// only for vtkGaussianBlurPass developers.
//#define VTK_GAUSSIAN_BLUR_PASS_DEBUG

#ifdef VTK_GAUSSIAN_BLUR_PASS_DEBUG
#include "vtkPNGWriter.h"
#include "vtkImageImport.h"
#include "vtkPixelBufferObject.h"
#include "vtkImageExtractComponents.h"
#endif

#include "vtkGaussianBlurPassFS.h"
#include "vtkGaussianBlurPassVS.h"

vtkStandardNewMacro(vtkGaussianBlurPass);

// ----------------------------------------------------------------------------
vtkGaussianBlurPass::vtkGaussianBlurPass()
{
  this->FrameBufferObject=0;
  this->Pass1=0;
  this->Pass2=0;
  this->BlurProgram = NULL;
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

  vtkOpenGLClearErrorMacro();

  this->NumberOfRenderedProps=0;

  vtkRenderer *r=s->GetRenderer();
  vtkOpenGLRenderWindow *renWin = static_cast<vtkOpenGLRenderWindow *>(r->GetRenderWindow());

  if(this->DelegatePass!=0)
  {

    // backup GL state
    GLboolean savedBlend = glIsEnabled(GL_BLEND);
    GLboolean savedDepthTest = glIsEnabled(GL_DEPTH_TEST);

    // 1. Create a new render state with an FBO.

    int width;
    int height;
    int size[2];
    s->GetWindowSize(size);
    width=size[0];
    height=size[1];


    // I suggest set this to 100 for debugging, makes some errors
    // much easier to find
    const int extraPixels=2; // two on each side, as the kernel is 5x5

    int w=width+extraPixels*2;
    int h=height+extraPixels*2;

    if(this->Pass1==0)
    {
      this->Pass1=vtkTextureObject::New();
      this->Pass1->SetContext(renWin);
    }

    if(this->FrameBufferObject==0)
    {
      this->FrameBufferObject=vtkOpenGLFramebufferObject::New();
      this->FrameBufferObject->SetContext(renWin);
    }

    this->FrameBufferObject->SaveCurrentBindingsAndBuffers();
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

    this->FrameBufferObject->AddColorAttachment(
      this->FrameBufferObject->GetBothMode(), 0,this->Pass2);
    this->FrameBufferObject->Start(w, h);

#ifdef VTK_GAUSSIAN_BLUR_PASS_DEBUG
    cout << "gauss finish2" << endl;
    glFinish();
#endif

    // Use a blur shader, do it horizontally. this->Pass1 is the source
    // (this->Pass2 is the fbo render target)

    // has something changed that would require us to recreate the shader?
    if (!this->BlurProgram)
    {
      this->BlurProgram = new vtkOpenGLHelper;
      // build the shader source code
      std::string VSSource = vtkGaussianBlurPassVS;
      std::string FSSource = vtkGaussianBlurPassFS;
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

    if(!this->BlurProgram->Program || this->BlurProgram->Program->GetCompiled() != true)
    {
      vtkErrorMacro("Couldn't build the shader program. At this point , it can be an error in a shader or a driver bug.");

      // restore some state.
      this->FrameBufferObject->UnBind();
      this->FrameBufferObject->RestorePreviousBindingsAndBuffers();
      return;
    }

    this->Pass1->Activate();
    int sourceId = this->Pass1->GetTextureUnit();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    this->BlurProgram->Program->SetUniformi("source",sourceId);
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
    this->BlurProgram->Program->SetUniform1fv("coef", 3, fvalues);

    // horizontal offset
    fvalues[0]=static_cast<float>(1.2/w);
    this->BlurProgram->Program->SetUniformf("offsetx",fvalues[0]);
    fvalues[0]=0.0f;
    this->BlurProgram->Program->SetUniformf("offsety",fvalues[0]);

#ifdef VTK_GAUSSIAN_BLUR_PASS_DEBUG
    cout << "gauss finish3-" << endl;
    glFinish();
#endif

    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);

    this->FrameBufferObject->RenderQuad(0,w-1,0,h-1,
      this->BlurProgram->Program, this->BlurProgram->VAO);

#ifdef VTK_GAUSSIAN_BLUR_PASS_DEBUG
    cout << "gauss finish3" << endl;
    glFinish();
#endif

    this->Pass1->Deactivate();

#ifdef VTK_GAUSSIAN_BLUR_PASS_DEBUG

    // Save second pass in file for debugging.
    pbo=this->Pass2->Download();
    openglRawData=new unsigned char[4*w*h];
    status=pbo->Download2D(VTK_UNSIGNED_CHAR,openglRawData,dims,4,incs);
    assert("check2" && status);
    pbo->Delete();

    // no pbo
    this->Pass2->Bind();
    glGetTexImage(GL_TEXTURE_2D,0,GL_RGBA,GL_UNSIGNED_BYTE,openglRawData);
    this->Pass2->UnBind();

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
    this->FrameBufferObject->RestorePreviousBindingsAndBuffers();

    // to2 is the source
    this->Pass2->Activate();
    sourceId = this->Pass2->GetTextureUnit();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    this->BlurProgram->Program->SetUniformi("source",sourceId);

    // Use the same blur shader, do it vertically.

    // vertical offset.
    fvalues[0]=0.0f;
    this->BlurProgram->Program->SetUniformf("offsetx",fvalues[0]);
    fvalues[0]=static_cast<float>(1.2/h);
    this->BlurProgram->Program->SetUniformf("offsety",fvalues[0]);

    this->Pass2->CopyToFrameBuffer(extraPixels, extraPixels,
                                  w-1-extraPixels,h-1-extraPixels,
                                  0,0, width, height,
                                  this->BlurProgram->Program,
                                  this->BlurProgram->VAO);

    this->Pass2->Deactivate();

    // restore GL state
    if(savedBlend)
    {
      glEnable(GL_BLEND);
    }
    if(savedDepthTest)
    {
      glEnable(GL_DEPTH_TEST);
    }

#ifdef VTK_GAUSSIAN_BLUR_PASS_DEBUG
    cout << "gauss finish4" << endl;
    glFinish();
#endif
  }
  else
  {
    vtkWarningMacro(<<" no delegate.");
  }

  vtkOpenGLCheckErrorMacro("failed after Render");
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
   if(this->Pass2!=0)
   {
    this->Pass2->Delete();
    this->Pass2=0;
   }
}
