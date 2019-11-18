/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLImageAlgorithmHelper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkOpenGLImageAlgorithmHelper.h"
#include "vtkDataArray.h"
#include "vtkImageData.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLFramebufferObject.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLShaderCache.h"
#include "vtkOpenGLState.h"
#include "vtkOpenGLVertexArrayObject.h"
#include "vtkPointData.h"
#include "vtkShaderProgram.h"
#include "vtkTextureObject.h"
#include "vtk_glew.h"

vtkStandardNewMacro(vtkOpenGLImageAlgorithmHelper);

// ----------------------------------------------------------------------------
vtkOpenGLImageAlgorithmHelper::vtkOpenGLImageAlgorithmHelper()
{
  this->RenderWindow = nullptr;
}

// ----------------------------------------------------------------------------
vtkOpenGLImageAlgorithmHelper::~vtkOpenGLImageAlgorithmHelper()
{
  this->SetRenderWindow(nullptr);
}

void vtkOpenGLImageAlgorithmHelper::SetRenderWindow(vtkRenderWindow* renWin)
{
  if (renWin == this->RenderWindow.GetPointer())
  {
    return;
  }

  vtkOpenGLRenderWindow* orw = nullptr;
  if (renWin)
  {
    orw = vtkOpenGLRenderWindow::SafeDownCast(renWin);
  }

  this->RenderWindow = orw;
  this->Modified();
}

void vtkOpenGLImageAlgorithmHelper::Execute(vtkOpenGLImageAlgorithmCallback* cb,
  vtkImageData* inImage, vtkDataArray* inArray, vtkImageData* outImage, int outExt[6],
  const char* vertexCode, const char* fragmentCode, const char* geometryCode)
{
  // make sure it is initialized
  if (!this->RenderWindow)
  {
    this->SetRenderWindow(vtkRenderWindow::New());
    this->RenderWindow->SetShowWindow(false);
    this->RenderWindow->UnRegister(this);
  }
  this->RenderWindow->Initialize();

  // Is it a 2D or 3D image
  int dims[3];
  inImage->GetDimensions(dims);
  int dimensions = 0;
  for (int i = 0; i < 3; i++)
  {
    if (dims[i] > 1)
    {
      dimensions++;
    }
  }

  // no 1D or 2D support yet
  if (dimensions < 3)
  {
    vtkErrorMacro("no 1D or 2D processing support yet");
    return;
  }

  // send vector data to a texture
  int inputExt[6];
  inImage->GetExtent(inputExt);
  void* inPtr = inArray->GetVoidPointer(0);

  // could do shortcut here if the input volume is
  // exactly what we want (updateExtent == wholeExtent)
  // vtkIdType incX, incY, incZ;
  // inImage->GetContinuousIncrements(inArray, extent, incX, incY, incZ);
  //  tmpImage->CopyAndCastFrom(inImage, inUpdateExtent)

  vtkNew<vtkTextureObject> inputTex;
  inputTex->SetContext(this->RenderWindow);
  inputTex->Create3DFromRaw(
    dims[0], dims[1], dims[2], inArray->GetNumberOfComponents(), inArray->GetDataType(), inPtr);

  float shift = 0.0;
  float scale = 1.0;
  inputTex->GetShiftAndScale(shift, scale);

  // now create the framebuffer for the output
  int outDims[3];
  outDims[0] = outExt[1] - outExt[0] + 1;
  outDims[1] = outExt[3] - outExt[2] + 1;
  outDims[2] = outExt[5] - outExt[4] + 1;

  vtkNew<vtkTextureObject> outputTex;
  outputTex->SetContext(this->RenderWindow);

  vtkNew<vtkOpenGLFramebufferObject> fbo;
  fbo->SetContext(this->RenderWindow);
  vtkOpenGLState* ostate = this->RenderWindow->GetState();
  ostate->PushFramebufferBindings();
  fbo->Bind();

  outputTex->Create2D(outDims[0], outDims[1], 4, VTK_FLOAT, false);
  fbo->AddColorAttachment(0, outputTex);

  // because the same FBO can be used in another pass but with several color
  // buffers, force this pass to use 1, to avoid side effects from the
  // render of the previous frame.
  fbo->ActivateDrawBuffer(0);

  fbo->StartNonOrtho(outDims[0], outDims[1]);
  ostate->vtkglViewport(0, 0, outDims[0], outDims[1]);
  ostate->vtkglScissor(0, 0, outDims[0], outDims[1]);
  ostate->vtkglDisable(GL_DEPTH_TEST);
  ostate->vtkglDepthMask(false);
  ostate->vtkglClearColor(0.0, 0.0, 0.0, 1.0);

  vtkShaderProgram* prog = this->RenderWindow->GetShaderCache()->ReadyShaderProgram(
    vertexCode, fragmentCode, geometryCode);
  if (prog != this->Quad.Program)
  {
    this->Quad.Program = prog;
    this->Quad.VAO->ShaderProgramChanged();
  }
  cb->InitializeShaderUniforms(prog);

  inputTex->Activate();
  int inputTexId = inputTex->GetTextureUnit();
  this->Quad.Program->SetUniformi("inputTex1", inputTexId);
  // shift and scale to get the data backing into its original units
  this->Quad.Program->SetUniformf("inputShift", shift);
  this->Quad.Program->SetUniformf("inputScale", scale);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  float* ftmp = new float[outDims[0] * outDims[1] * 4];
  int outNumComponents = outImage->GetNumberOfScalarComponents();

  // for each zslice in the output
  for (int i = outExt[4]; i <= outExt[5]; i++)
  {
    cb->UpdateShaderUniforms(prog, i);
    this->Quad.Program->SetUniformf("zPos", (i - outExt[4] + 0.5) / (outDims[2]));
    glClear(GL_COLOR_BUFFER_BIT);
    fbo->RenderQuad(0, outDims[0] - 1, 0, outDims[1] - 1, this->Quad.Program, this->Quad.VAO);
    glReadPixels(0, 0, outDims[0], outDims[1], GL_RGBA, GL_FLOAT, ftmp);

    double* outP = static_cast<double*>(outImage->GetScalarPointer(outExt[0], outExt[2], i));
    float* tmpP = ftmp;
    for (int j = 0; j < outDims[1] * outDims[0]; ++j)
    {
      for (int c = 0; c < outNumComponents; ++c)
      {
        outP[c] = tmpP[c];
      }
      tmpP += 4;
      outP += outNumComponents;
    }
  }

  inputTex->Deactivate();
  ostate->PopFramebufferBindings();
  delete[] ftmp;
}

// ----------------------------------------------------------------------------
void vtkOpenGLImageAlgorithmHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "RenderWindow:";
  if (this->RenderWindow != nullptr)
  {
    this->RenderWindow->PrintSelf(os, indent);
  }
  else
  {
    os << "(none)" << endl;
  }
}
