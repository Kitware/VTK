/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSobelGradientMagnitudePass.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkSobelGradientMagnitudePass.h"
#include "vtkObjectFactory.h"
#include <cassert>

#include "vtkRenderState.h"
#include "vtkRenderer.h"
#include "vtkFrameBufferObject.h"
#include "vtkTextureObject.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLShaderCache.h"
#include "vtkOpenGLError.h"
#include "vtkOpenGLVertexArrayObject.h"
#include "vtkShaderProgram.h"

#include "vtkOpenGLHelper.h"

// to be able to dump intermediate passes into png files for debugging.
// only for vtkSobelGradientMagnitudePass developers.
//#define VTK_SOBEL_PASS_DEBUG

#ifdef VTK_SOBEL_BLUR_PASS_DEBUG
#include "vtkPNGWriter.h"
#include "vtkImageImport.h"
#include "vtkPixelBufferObject.h"
#include "vtkImageExtractComponents.h"
#endif

#include "vtkSobelGradientMagnitudePass1FS.h"
#include "vtkSobelGradientMagnitudePass2FS.h"
#include "vtkTextureObjectVS.h"  // a pass through shader

vtkStandardNewMacro(vtkSobelGradientMagnitudePass);

// ----------------------------------------------------------------------------
vtkSobelGradientMagnitudePass::vtkSobelGradientMagnitudePass()
{
  this->FrameBufferObject=0;
  this->Pass1=0;
  this->Gx1=0;
  this->Gy1=0;
  this->Program1 =0;
  this->Program2 =0;
}

// ----------------------------------------------------------------------------
vtkSobelGradientMagnitudePass::~vtkSobelGradientMagnitudePass()
{
  if(this->FrameBufferObject!=0)
  {
    vtkErrorMacro(<<"FrameBufferObject should have been deleted in ReleaseGraphicsResources().");
  }
   if(this->Pass1!=0)
   {
    vtkErrorMacro(<<"Pass1 should have been deleted in ReleaseGraphicsResources().");
   }
   if(this->Gx1!=0)
   {
    vtkErrorMacro(<<"Gx1 should have been deleted in ReleaseGraphicsResources().");
   }
   if(this->Gy1!=0)
   {
     vtkErrorMacro(<<"Gx1 should have been deleted in ReleaseGraphicsResources().");
   }
}

// ----------------------------------------------------------------------------
void vtkSobelGradientMagnitudePass::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

// ----------------------------------------------------------------------------
// Description:
// Perform rendering according to a render state \p s.
// \pre s_exists: s!=0
void vtkSobelGradientMagnitudePass::Render(const vtkRenderState *s)
{
  assert("pre: s_exists" && s!=0);

  this->NumberOfRenderedProps=0;

  if(this->DelegatePass!=0)
  {
    vtkRenderer *renderer = s->GetRenderer();

    vtkOpenGLRenderWindow *context
      = vtkOpenGLRenderWindow::SafeDownCast(renderer->GetRenderWindow());

    // Test for Hardware support. If not supported, just render the delegate.
    bool fbo_support = vtkFrameBufferObject::IsSupported(context)!=0;
    bool texture_support = vtkTextureObject::IsSupported(context)!=0;

    bool supported = fbo_support && texture_support;

    if (!supported)
    {
      vtkErrorMacro(
        << "The required extensions are not supported."
        << " fbo_support=" << fbo_support
        << " texture_support=" << texture_support);

      this->DelegatePass->Render(s);
      this->NumberOfRenderedProps
        += this->DelegatePass->GetNumberOfRenderedProps();

      return;
    }

    vtkOpenGLClearErrorMacro();

#if GL_ES_VERSION_2_0 != 1
    GLint savedDrawBuffer;
    glGetIntegerv(GL_DRAW_BUFFER,&savedDrawBuffer);
#endif

    // 1. Create a new render state with an FBO.
    int width=0;
    int height=0;
    int size[2];
    s->GetWindowSize(size);
    width=size[0];
    height=size[1];

    const int extraPixels=1; // one on each side

    int w=width+2*extraPixels;
    int h=height+2*extraPixels;

    if(this->Pass1==0)
    {
      this->Pass1=vtkTextureObject::New();
      this->Pass1->SetContext(context);
    }

    if(this->FrameBufferObject==0)
    {
      this->FrameBufferObject=vtkFrameBufferObject::New();
      this->FrameBufferObject->SetContext(context);
    }

    this->RenderDelegate(s,width,height,w,h,this->FrameBufferObject,
                         this->Pass1);

#ifdef VTK_SOBEL_PASS_DEBUG
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
    writer->SetFileName("SobelPass1.png");
    writer->SetInputConnection(rgbaToRgb->GetOutputPort());
    importer->Delete();
    rgbaToRgb->Delete();
    writer->Write();
    writer->Delete();
#endif

    // 3. Same FBO, but two color attachments (new TOs gx1 and gy1).

    if(this->Gx1==0)
    {
      this->Gx1=vtkTextureObject::New();
      this->Gx1->SetContext(this->FrameBufferObject->GetContext());
    }

    if(this->Gx1->GetWidth()!=static_cast<unsigned int>(w) ||
       this->Gx1->GetHeight()!=static_cast<unsigned int>(h))
    {
      this->Gx1->Create2D(w,h,4,VTK_UNSIGNED_CHAR,false);
    }

    if(this->Gy1==0)
    {
      this->Gy1=vtkTextureObject::New();
      this->Gy1->SetContext(this->FrameBufferObject->GetContext());
    }

    if(this->Gy1->GetWidth()!=static_cast<unsigned int>(w) ||
       this->Gy1->GetHeight()!=static_cast<unsigned int>(h))
    {
      this->Gy1->Create2D(w,h,4,VTK_UNSIGNED_CHAR,false);
    }
#ifdef VTK_SOBEL_PASS_DEBUG
    cout << "gx1 TOid=" << this->Gx1->GetHandle() <<endl;
    cout << "gy1 TOid=" << this->Gy1->GetHandle() <<endl;
#endif
    this->FrameBufferObject->SetNumberOfRenderTargets(2);
    this->FrameBufferObject->SetColorBuffer(0,this->Gx1);
    this->FrameBufferObject->SetColorBuffer(1,this->Gy1);
    unsigned int indices[2]={0,1};
    this->FrameBufferObject->SetActiveBuffers(2,indices);

    this->FrameBufferObject->Start(w,h,false);

#ifdef VTK_SOBEL_PASS_DEBUG
    cout << "sobel finish2" << endl;
    glFinish();
#endif

    // Use the horizontal shader to compute the first pass of Gx and Gy.
    // this->Pass1 is the source
    // (this->Gx1 and this->Gy1 are fbo render targets)

    // has something changed that would require us to recreate the shaders?
    if (!this->Program1)
    {
      this->Program1 = new vtkOpenGLHelper;

      // build the shader source code
      std::string VSSource = vtkTextureObjectVS;
      std::string FSSource = vtkSobelGradientMagnitudePass1FS;
      std::string GSSource;

      // compile and bind it if needed
      vtkShaderProgram *newShader =
        context->GetShaderCache()->ReadyShaderProgram(
          VSSource.c_str(),
          FSSource.c_str(),
          GSSource.c_str());

      // if the shader changed reinitialize the VAO
      if (newShader != this->Program1->Program)
      {
        this->Program1->Program = newShader;
        this->Program1->VAO->ShaderProgramChanged(); // reset the VAO as the shader has changed
      }

      this->Program1->ShaderSourceTime.Modified();
    }
    else
    {
      context->GetShaderCache()->ReadyShaderProgram(this->Program1->Program);
    }

#ifdef VTK_SOBEL_PASS_DEBUG
    cout << "sobel finish build 1" << endl;
    glFinish();
#endif

    if(this->Program1->Program->GetCompiled() != true)
    {
      vtkErrorMacro("Couldn't build the shader program. At this point , it can be an error in a shader or a driver bug.");

      // restore some state.
      this->FrameBufferObject->UnBind();
#if GL_ES_VERSION_2_0 != 1
      glDrawBuffer(savedDrawBuffer);
#endif
      return;
    }

    this->Pass1->Activate();
    int sourceId=this->Pass1->GetTextureUnit();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    this->Program1->Program->SetUniformi("source",sourceId);

    float fvalue=static_cast<float>(1.0/w);
    this->Program1->Program->SetUniformf("stepSize",fvalue);

#ifdef VTK_SOBEL_PASS_DEBUG
    cout << "sobel finish3-" << endl;
    glFinish();
#endif

    this->FrameBufferObject->RenderQuad(0,w-1,0,h-1,
      this->Program1->Program, this->Program1->VAO);

#ifdef VTK_SOBEL_PASS_DEBUG
    cout << "sobel finish3" << endl;
    glFinish();
#endif

    this->Pass1->Deactivate();

#ifdef VTK_SOBEL_PASS_DEBUG

    // Save second pass in file for debugging.
    pbo=this->Gx1->Download();
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
    writer->SetFileName("SobelPass2Gx1.png");
    writer->SetInputConnection(rgbaToRgb->GetOutputPort());
    importer->Delete();
    rgbaToRgb->Delete();
    writer->Write();
    writer->Delete();

    pbo=this->Gy1->Download();
    openglRawData=new unsigned char[4*w*h];
    status=pbo->Download2D(VTK_UNSIGNED_CHAR,openglRawData,dims,4,incs);
    assert("check3" && status);
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
    writer->SetFileName("SobelPass2Gy1.png");
    writer->SetInputConnection(rgbaToRgb->GetOutputPort());
    importer->Delete();
    rgbaToRgb->Delete();
    writer->Write();
    writer->Delete();

#endif

    // 4. Render in original FB (from renderstate in arg)

    this->FrameBufferObject->UnBind();

    glDrawBuffer(savedDrawBuffer);

    // has something changed that would require us to recreate the shaders?
    if (!this->Program2)
    {
      this->Program2 = new vtkOpenGLHelper;

      // build the shader source code
      std::string VSSource = vtkTextureObjectVS;
      std::string FSSource = vtkSobelGradientMagnitudePass2FS;
      std::string GSSource;

      // compile and bind it if needed
      vtkShaderProgram *newShader =
        context->GetShaderCache()->ReadyShaderProgram(
          VSSource.c_str(),
          FSSource.c_str(),
          GSSource.c_str());

      // if the shader changed reinitialize the VAO
      if (newShader != this->Program2->Program)
      {
        this->Program2->Program = newShader;
        this->Program2->VAO->ShaderProgramChanged(); // reset the VAO as the shader has changed
      }

      this->Program2->ShaderSourceTime.Modified();
    }
    else
    {
      context->GetShaderCache()->ReadyShaderProgram(this->Program2->Program);
    }

#ifdef VTK_SOBEL_PASS_DEBUG
    cout << "sobel finish build 2" << endl;
    glFinish();
#endif

    if(this->Program2->Program->GetCompiled() != true)
    {
      vtkErrorMacro("Couldn't build the shader program. At this point , it can be an error in a shader or a driver bug.");
#if GL_ES_VERSION_2_0 != 1
      glDrawBuffer(savedDrawBuffer);
#endif
      return;
    }

#ifdef VTK_SOBEL_PASS_DEBUG
    cout << "sobel finish build 2" << endl;
    glFinish();
#endif

    // this->Gx1 and this->Gy1 are the sources
    this->Gx1->Activate();
    int id0 = this->Gx1->GetTextureUnit();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    this->Gy1->Activate();
    int id1 = this->Gy1->GetTextureUnit();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    this->Program2->Program->SetUniformi("gx1",id0);
    this->Program2->Program->SetUniformi("gy1",id1);

    fvalue=static_cast<float>(1.0/h);
    this->Program2->Program->SetUniformf("stepSize",fvalue);

#ifdef VTK_SOBEL_PASS_DEBUG
    cout << "sobel finish use 2" << endl;
    glFinish();
#endif

#ifdef VTK_SOBEL_PASS_DEBUG
    cout << "sobel finish 4-" << endl;
    glFinish();
#endif

    // Prepare blitting
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_SCISSOR_TEST);

    // Trigger a draw on Gy1 (could be called on Gx1).
    this->Gy1->CopyToFrameBuffer(extraPixels, extraPixels,
                                  w-1-extraPixels,h-1-extraPixels,
                                  0, 0, width, height,
                                  this->Program2->Program,
                                  this->Program2->VAO);

    this->Gy1->Deactivate();
    this->Gx1->Deactivate();

#ifdef VTK_SOBEL_PASS_DEBUG
    cout << "sobel finish4" << endl;
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
void vtkSobelGradientMagnitudePass::ReleaseGraphicsResources(vtkWindow *w)
{
  assert("pre: w_exists" && w!=0);

  this->Superclass::ReleaseGraphicsResources(w);

  if (this->Program1!=0)
  {
    this->Program1->ReleaseGraphicsResources(w);
    delete this->Program1;
    this->Program1 = 0;
  }
  if (this->Program2!=0)
  {
    this->Program2->ReleaseGraphicsResources(w);
    delete this->Program2;
    this->Program2 = 0;
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

  if(this->Gx1!=0)
  {
    this->Gx1->Delete();
    this->Gx1=0;
  }

  if(this->Gy1!=0)
  {
    this->Gy1->Delete();
    this->Gy1=0;
  }
}
