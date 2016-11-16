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
#include "vtkObjectFactory.h"
#include "vtkTextureObject.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkDataArray.h"
#include "vtkImageData.h"
#include "vtkNew.h"
#include "vtkOpenGLFramebufferObject.h"
#include "vtkOpenGLShaderCache.h"
#include "vtk_glew.h"
#include "vtkPixelTransfer.h"
#include "vtkPointData.h"
#include "vtkPixelBufferObject.h"
#include "vtkShaderProgram.h"
#include "vtkOpenGLVertexArrayObject.h"

vtkStandardNewMacro(vtkOpenGLImageAlgorithmHelper);

// ----------------------------------------------------------------------------
vtkOpenGLImageAlgorithmHelper::vtkOpenGLImageAlgorithmHelper()
{
  this->RenderWindow = 0;
}

// ----------------------------------------------------------------------------
vtkOpenGLImageAlgorithmHelper::~vtkOpenGLImageAlgorithmHelper()
{
  this->SetRenderWindow(0);
}

void vtkOpenGLImageAlgorithmHelper::SetRenderWindow(vtkRenderWindow *renWin)
{
  if (renWin == this->RenderWindow.Get())
  {
    return;
  }

  vtkOpenGLRenderWindow *orw  = NULL;
  if (renWin)
  {
    orw = vtkOpenGLRenderWindow::SafeDownCast(renWin);
  }

  this->RenderWindow = orw;
  this->Modified();
}

void vtkOpenGLImageAlgorithmHelper::Execute(
  vtkOpenGLImageAlgorithmCallback *cb,
  vtkImageData *inImage, vtkDataArray *inArray,
  vtkImageData *outImage, int outExt[6],
  const char *vertexCode,
  const char *fragmentCode,
  const char *geometryCode
  )
{
  // make sure it is initialized
  if (!this->RenderWindow)
  {
    this->SetRenderWindow(vtkRenderWindow::New());
    this->RenderWindow->SetOffScreenRendering(true);
    this->RenderWindow->UnRegister(this);
  }
  this->RenderWindow->Initialize();

  // Is it a 2D or 3D image
  int dims[3];
  inImage->GetDimensions(dims);
  int dimensions = 0;
  for (int i = 0; i < 3; i ++)
  {
    if (dims[i] > 1)
    {
      dimensions++;
    }
  }

  // no 1d or 2D supprt yet
  if (dimensions < 3)
  {
    vtkErrorMacro("no 1D or 2D processing support yet");
    return;
  }

  // send vector data to a texture
  int inputExt[6];
  inImage->GetExtent(inputExt);
  void *inPtr = inArray->GetVoidPointer(0);

  // could do shortcut here if the input volume is
  // exactly what we want (updateExtent == wholeExtent)
  // vtkIdType incX, incY, incZ;
  // inImage->GetContinuousIncrements(inArray, extent, incX, incY, incZ);
  //  tmpImage->CopyAndCastFrom(inImage, inUpdateExtent)

  vtkNew<vtkTextureObject> inputTex;
  inputTex->SetContext(this->RenderWindow);
  inputTex->Create3DFromRaw(
    dims[0], dims[1], dims[2],
    inArray->GetNumberOfComponents(),
    inArray->GetDataType(), inPtr);

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

  outputTex->Create2D(outDims[0], outDims[1], 4, VTK_FLOAT, false);
  fbo->AddColorAttachment(fbo->GetDrawMode(), 0, outputTex.Get());

  // because the same FBO can be used in another pass but with several color
  // buffers, force this pass to use 1, to avoid side effects from the
  // render of the previous frame.
  fbo->ActivateDrawBuffer(0);

  fbo->StartNonOrtho(outDims[0], outDims[1]);
  glViewport(0, 0, outDims[0], outDims[1]);
  glScissor(0, 0, outDims[0], outDims[1]);
  glDisable(GL_DEPTH_TEST);

  vtkShaderProgram *prog =
    this->RenderWindow->GetShaderCache()->ReadyShaderProgram(
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

  // for each zslice in the output
  vtkPixelExtent outputPixelExt(outExt);
  for (int i = outExt[4]; i <= outExt[5]; i++)
  {
    cb->UpdateShaderUniforms(prog, i);
    this->Quad.Program->SetUniformf("zPos", (i - outExt[4] + 0.5) / (outDims[2]));

    fbo->RenderQuad(
      0, outDims[0] - 1,
      0, outDims[1] - 1,
      this->Quad.Program, this->Quad.VAO);

    vtkPixelBufferObject *outPBO = outputTex->Download();

    vtkPixelTransfer::Blit<float, double>(
      outputPixelExt,
      outputPixelExt,
      outputPixelExt,
      outputPixelExt,
      4,
      (float*)outPBO->MapPackedBuffer(),
      outImage->GetPointData()->GetScalars()->GetNumberOfComponents(),
      static_cast<double *>(outImage->GetScalarPointer(outExt[0], outExt[2], i)));

    outPBO->UnmapPackedBuffer();
    outPBO->Delete();
  }
}

// ----------------------------------------------------------------------------
void vtkOpenGLImageAlgorithmHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "RenderWindow:";
  if(this->RenderWindow != 0)
  {
    this->RenderWindow->PrintSelf(os,indent);
  }
  else
  {
    os << "(none)" <<endl;
  }
}
