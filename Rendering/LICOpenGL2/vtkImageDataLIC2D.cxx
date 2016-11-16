/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageDataLIC2D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageDataLIC2D.h"

#include "vtkStructuredData.h"
#include "vtkPointData.h"
#include "vtkCellData.h"
#include "vtkFloatArray.h"
#include "vtkUnsignedCharArray.h"
#include "vtkImageCast.h"
#include "vtkImageData.h"
#include "vtkImageNoiseSource.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkStructuredExtent.h"
#include "vtkObjectFactory.h"
#include "vtkLineIntegralConvolution2D.h"
#include "vtkOpenGLFramebufferObject.h"
#include "vtkRenderbuffer.h"
#include "vtkPixelBufferObject.h"
#include "vtkTextureObject.h"
#include "vtkPixelExtent.h"
#include "vtkPixelTransfer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkSmartPointer.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLError.h"
#include "vtkOpenGLShaderCache.h"
#include "vtkShaderProgram.h"

#include <deque>
using std::deque;

#include "vtkOpenGLHelper.h"
#include "vtkTextureObjectVS.h"

#define vtkImageDataLIC2DDEBUG 0
#if (vtkImageDataLIC2DDEBUG >= 1)
#include "vtkTextureIO.h"
#endif

#define PRINTEXTENT(ext) \
  ext[0] << ", " << ext[1] << ", " << ext[2] << ", " << ext[3] << ", " << ext[4] << ", " << ext[5]

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkImageDataLIC2D);

//----------------------------------------------------------------------------
vtkImageDataLIC2D::vtkImageDataLIC2D()
{
  this->Context = NULL;
  this->OwnWindow = false;
  this->OpenGLExtensionsSupported = 0;

  this->Steps = 20;
  this->StepSize = 1.0;
  this->Magnification = 1;

  this->NoiseSource = vtkImageNoiseSource::New();
  this->NoiseSource->SetWholeExtent(0, 127, 0, 127, 0, 0);
  this->NoiseSource->SetMinimum(0.0);
  this->NoiseSource->SetMaximum(1.0);

  this->ImageCast = vtkImageCast::New();
  this->ImageCast->SetOutputScalarTypeToFloat();
  this->ImageCast->SetInputConnection(this->NoiseSource->GetOutputPort(0));

  this->SetNumberOfInputPorts(2);

  // by default process active point vectors
  this->SetInputArrayToProcess(
        0,
        0,
        0,
        vtkDataObject::FIELD_ASSOCIATION_POINTS,
        vtkDataSetAttributes::VECTORS);
}

//----------------------------------------------------------------------------
vtkImageDataLIC2D::~vtkImageDataLIC2D()
{
  this->NoiseSource->Delete();
  this->ImageCast->Delete();
  this->SetContext(NULL);
}

//----------------------------------------------------------------------------
int vtkImageDataLIC2D::SetContext(vtkRenderWindow * renWin)
{
  vtkOpenGLRenderWindow *rw = vtkOpenGLRenderWindow::SafeDownCast(renWin);

  if (this->Context == rw)
  {
    return this->OpenGLExtensionsSupported;
  }

  if (this->Context && this->OwnWindow)
  {
    this->Context->Delete();
  }
  this->Modified();
  this->Context = NULL;
  this->OwnWindow = false;
  this->OpenGLExtensionsSupported = 0;

  vtkOpenGLRenderWindow *context = vtkOpenGLRenderWindow::SafeDownCast(renWin);
  if (context)
  {
    context->Render();
    context->MakeCurrent();

    bool featureSupport
      = vtkLineIntegralConvolution2D::IsSupported(context)
      && vtkPixelBufferObject::IsSupported(context)
      && vtkOpenGLFramebufferObject::IsSupported(context)
      && vtkRenderbuffer::IsSupported(context)
      && vtkTextureObject::IsSupported(context);

    if (!featureSupport)
    {
      vtkErrorMacro("Required OpenGL extensions not supported.");
      return 0;
    }

    this->OpenGLExtensionsSupported = 1;
    this->Context = context;
  }

  return 1;
}

//----------------------------------------------------------------------------
vtkRenderWindow* vtkImageDataLIC2D::GetContext()
{
  return this->Context;
}

//----------------------------------------------------------------------------
int vtkImageDataLIC2D::FillInputPortInformation(int port, vtkInformation *info)
{
  if (!this->Superclass::FillInputPortInformation(port, info))
  {
    return 0;
  }

  if (port == 1)
  {
    info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  }

  return 1;
}

//----------------------------------------------------------------------------
void vtkImageDataLIC2D::TranslateInputExtent(
        const int* inExt,
        const int* inWholeExt,
        int *resultExt)
{
  int nPlanar = 0;
  for (int q=0; q<3; ++q)
  {
    int qq = 2*q;
    if (inWholeExt[qq] == inWholeExt[qq+1])
    {
      resultExt[qq] = inExt[qq];
      resultExt[qq+1] = inExt[qq];
      nPlanar += 1;
    }
    else
    {
      resultExt[qq] = inExt[qq] * this->Magnification;
      resultExt[qq+1] = (inExt[qq+1] + 1) * this->Magnification - 1;
    }
  }
  if (nPlanar != 1)
  {
    vtkErrorMacro("Non-planar dataset");
  }
}

//----------------------------------------------------------------------------
int vtkImageDataLIC2D::RequestInformation(
      vtkInformation* vtkNotUsed(request),
      vtkInformationVector** inputVector,
      vtkInformationVector* outputVector)
{
  int ext[6];
  int wholeExtent[6];
  double spacing[3];

  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), wholeExtent);
  inInfo->Get(vtkDataObject::SPACING(), spacing);
  vtkDebugMacro( << "Input WHOLE_EXTENT: " << PRINTEXTENT( wholeExtent ) << endl );
  this->TranslateInputExtent(wholeExtent, wholeExtent, ext);

  for (int axis = 0; axis < 3; axis++)
  {
    // Change the data spacing
    spacing[axis] /= this->Magnification;
  }
  vtkDebugMacro( << "WHOLE_EXTENT: " << PRINTEXTENT( ext ) << endl );

  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), ext, 6);
  outInfo->Set(vtkDataObject::SPACING(), spacing, 3);

  return 1;
}

//----------------------------------------------------------------------------
int vtkImageDataLIC2D::RequestUpdateExtent (
      vtkInformation * vtkNotUsed(request),
      vtkInformationVector **inputVector,
      vtkInformationVector *outputVector)
{
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // Tell the vector field input the extents that we need from it.
  // The downstream request needs to be downsized based on the Magnification.
  int ext[6];
  outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), ext);

  vtkDebugMacro( << "Requested UPDATE_EXTENT: " <<  PRINTEXTENT( ext ) << endl );
  for (int axis = 0; axis < 3; axis++)
  {
    int wholeMin = ext[axis*2];
    int wholeMax = ext[axis*2+1];

    // Scale the output extent
    wholeMin = wholeMin / this->Magnification;
    wholeMax = wholeMax / this->Magnification;

    ext[axis*2] = wholeMin;
    ext[axis*2+1] = wholeMax;
  }
  vtkDebugMacro( << "UPDATE_EXTENT: " <<  PRINTEXTENT( ext ) << endl );

  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), ext, 6);

  inInfo = inputVector[1]->GetInformationObject(0);
  if (inInfo)
  {
    // always request the whole noise image.
    inInfo->Set(
        vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
        inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()),
        6);
  }

  return 1;
}

//----------------------------------------------------------------------------
int vtkImageDataLIC2D::RequestData(
      vtkInformation  *vtkNotUsed(request),
      vtkInformationVector **inputVector,
      vtkInformationVector *outputVector)
{
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);

  vtkImageData *input
     = vtkImageData::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  if ( !input )
  {
    vtkErrorMacro("Empty input");
    return 0;
  }

  int dims[3];
  input->GetDimensions(dims);

  int dataDescription = vtkStructuredData::GetDataDescription(dims);

  if (  vtkStructuredData::GetDataDimension( dataDescription ) != 2  )
  {
    vtkErrorMacro( "Input is not a 2D image." );
    return 0;
  }

  vtkIdType numPoints = input->GetNumberOfPoints();
  vtkDataArray *inVectors = this->GetInputArrayToProcess(0, inputVector);
  if ( !inVectors )
  {
    vtkErrorMacro("Vectors are required for line integral convolution.");
    return 0;
  }

  if ( inVectors->GetNumberOfTuples() != numPoints )
  {
    vtkErrorMacro( "Only point vectors are supported." );
    return 0;
  }

  if ( !this->Context )
  {
    vtkRenderWindow * renWin = vtkRenderWindow::New();
    if ( this->SetContext(renWin) == 0 )
    {
      vtkErrorMacro("Missing required OpenGL extensions");
      renWin->Delete();
      return 0;
    }
    this->OwnWindow = true;
  }

  this->Context->MakeCurrent();
  vtkOpenGLClearErrorMacro();

  // Noise.
  vtkInformation *noiseInfo = inputVector[1]->GetInformationObject(0);
  vtkImageData *noise = NULL;
  if ( noiseInfo )
  {
    noise
      = vtkImageData::SafeDownCast(noiseInfo->Get(vtkDataObject::DATA_OBJECT()));
    if ( !noise )
    {
      vtkErrorMacro(
        "Invalid noise dataset on input "
        "Default noise dataset is used");
    }

    if ( (noise->GetPointData()==0)
      || (noise->GetPointData()->GetScalars()==0) )
    {
      vtkErrorMacro(
        "Noise dataset missing point data scalars. "
        "Default noise dataset is used");
      noise = NULL;
    }

    double noiseRange[2];
    vtkDataArray *inVals = noise->GetPointData()->GetScalars();
    inVals->GetRange(noiseRange);
    if ( (noiseRange[0] < 0.0) || (noiseRange[1] > 1.0) )
    {
      vtkErrorMacro(
        "Noise dataset has values out of range 0.0 to 1.0."
        "Default noise dataset is used");
      noise = NULL;
    }
  }

  if ( !noise )
  {
    this->ImageCast->Update();
    noise = this->ImageCast->GetOutput();
  }

  int comp[3] = {0, 0, 0};
  switch (dataDescription)
  {
  case VTK_XY_PLANE:
    comp[0] = 0;
    comp[1] = 1;
    comp[2] = 2;
    break;

  case VTK_YZ_PLANE:
    comp[0] = 1;
    comp[1] = 2;
    comp[2] = 0;
    break;

  case VTK_XZ_PLANE:
    comp[0] = 0;
    comp[1] = 2;
    comp[2] = 1;
    break;
  }

  // size of output
  int magDims[3];
  magDims[0] = this->Magnification*dims[0];
  magDims[1] = this->Magnification*dims[1];
  magDims[2] = this->Magnification*dims[2];

  // send vector data to a texture
  int inputExt[6];
  input->GetExtent(inputExt);

  vtkPixelExtent inVectorExtent(dims[comp[0]], dims[comp[1]]);

  vtkPixelBufferObject *vecPBO = vtkPixelBufferObject::New();
  vecPBO->SetContext(this->Context);

  vtkPixelTransfer::Blit(
        inVectorExtent,
        inVectorExtent,
        inVectorExtent,
        inVectorExtent,
        3,
        inVectors->GetDataType(),
        inVectors->GetVoidPointer(0),
        4,
        VTK_FLOAT,
        vecPBO->MapUnpackedBuffer(
        VTK_FLOAT,
        static_cast<unsigned int>(inVectorExtent.Size()),
        4));

  vecPBO->UnmapUnpackedBuffer();

  vtkTextureObject *vectorTex = vtkTextureObject::New();
  vectorTex->SetContext(this->Context);
  vectorTex->Create2D(dims[comp[0]], dims[comp[1]], 4, vecPBO, false);
  vtkLineIntegralConvolution2D::SetVectorTexParameters(vectorTex);

  vecPBO->Delete();

  #if (vtkImageDataLIC2DDEBUG >= 1)
  vtkTextureIO::Write(
              "idlic2d_vectors.vtk",
              vectorTex, NULL, NULL);
  #endif

  // magnify vectors
  vtkPixelExtent magVectorExtent(magDims[comp[0]], magDims[comp[1]]);
  int magVectorSize[2];
  magVectorExtent.Size(magVectorSize);

  vtkTextureObject *magVectorTex = vectorTex;
  if (this->Magnification > 1)
  {
    magVectorTex = vtkTextureObject::New();
    magVectorTex->SetContext(this->Context);
    magVectorTex->Create2D(magVectorSize[0], magVectorSize[1], 4, VTK_FLOAT, false);
    vtkLineIntegralConvolution2D::SetVectorTexParameters(magVectorTex);

    vtkOpenGLFramebufferObject *drawFbo = vtkOpenGLFramebufferObject::New();
    drawFbo->SetContext(this->Context);
    drawFbo->SaveCurrentBindings();
    drawFbo->Bind(GL_FRAMEBUFFER);
    drawFbo->AddColorAttachment(GL_FRAMEBUFFER, 0U, magVectorTex);
    drawFbo->ActivateDrawBuffer(0U);
    //drawFbo->AddColorAttachment(vtkgl::FRAMEBUFFER_EXT, 0U, vectorTex);
    //drawFbo->ActivateReadBuffer(0U);
    drawFbo->CheckFrameBufferStatus(GL_FRAMEBUFFER);
    drawFbo->InitializeViewport(magVectorSize[0], magVectorSize[1]);

    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClear(GL_COLOR_BUFFER_BIT);

    float tcoords[] = {
      0.0f, 0.0f,
      1.0f, 0.0f,
      1.0f, 1.0f,
      0.0f, 1.0f};

    float verts[] = {
      -1.0f, -1.0f, 0.0f,
      1.0f, -1.0f, 0.0f,
      1.0f, 1.0f, 0.0f,
      -1.0f, 1.0f, 0.0f};

    vtkOpenGLHelper shaderHelper;

    // build the shader source code
    shaderHelper.Program =
      this->Context->GetShaderCache()->ReadyShaderProgram(
        vtkTextureObjectVS,
        "//VTK::System::Dec\n"
        "varying vec2 tcoordVC;\n"
        "uniform sampler2D source;\n"
        "//VTK::Output::Dec\n"
        "void main(void) {\n"
        "  gl_FragData[0] = texture2D(source,tcoordVC); }\n",
        "");

    // bind and activate this texture
    vectorTex->Activate();
    int sourceId = vectorTex->GetTextureUnit();
    shaderHelper.Program->SetUniformi("source",sourceId);
    vectorTex->CopyToFrameBuffer(tcoords, verts,
      shaderHelper.Program, shaderHelper.VAO);
    vectorTex->Deactivate();
    vectorTex->Delete();
    shaderHelper.ReleaseGraphicsResources(this->Context);

    drawFbo->UnBind(GL_FRAMEBUFFER);
    drawFbo->Delete();
  }

  #if (vtkImageDataLIC2DDEBUG >= 1)
  vtkTextureIO::Write(
              "idlic2d_magvectors.vtk",
              magVectorTex, NULL, NULL);
  #endif

  // send noise data to a texture
  vtkDataArray * inNoise = noise->GetPointData()->GetScalars();

  vtkPixelExtent noiseExt(noise->GetExtent());

  vtkPixelBufferObject *noisePBO = vtkPixelBufferObject::New();
  noisePBO->SetContext(this->Context);
  int noiseComp = inNoise->GetNumberOfComponents();

  if (inNoise->GetDataType() != VTK_FLOAT)
  {
    vtkErrorMacro("noise dataset was not float");
  }

  vtkPixelTransfer::Blit(
        noiseExt,
        noiseComp,
        inNoise->GetDataType(),
        inNoise->GetVoidPointer(0),
        VTK_FLOAT,
        noisePBO->MapUnpackedBuffer(
        VTK_FLOAT,
        static_cast<unsigned int>(noiseExt.Size()),
        noiseComp));

  noisePBO->UnmapUnpackedBuffer();

  int noiseTexSize[2];
  noiseExt.Size(noiseTexSize);

  vtkTextureObject *noiseTex = vtkTextureObject::New();
  noiseTex->SetContext(this->Context);
  noiseTex->Create2D(noiseTexSize[0], noiseTexSize[1],
    noiseComp, noisePBO, false);

  noisePBO->Delete();

  #if (vtkImageDataLIC2DDEBUG >= 1)
  vtkTextureIO::Write(
          "idlic2d_noise.vtk",
          noiseTex, NULL, NULL);
  #endif

  // step size conversion to normalize image space
  double *spacing = input->GetSpacing();
  spacing[comp[0]] /= this->Magnification;
  spacing[comp[1]] /= this->Magnification;

  double cellLength
    = sqrt(spacing[comp[0]]*spacing[comp[0]]+spacing[comp[1]]*spacing[comp[1]]);

  double w = spacing[comp[0]]*dims[comp[0]];
  double h = spacing[comp[1]]*dims[comp[1]];
  double normalizationFactor = sqrt(w*w+h*h);
  double stepSize = this->StepSize*cellLength/normalizationFactor;

  // compute the LIC
  int updateExt[6];
  inInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), updateExt);

  int magUpdateExt[6];
  magUpdateExt[2*comp[0]] = updateExt[2*comp[0]] * this->Magnification;
  magUpdateExt[2*comp[1]] = updateExt[2*comp[1]] * this->Magnification;
  magUpdateExt[2*comp[2]] = updateExt[comp[2]];
  magUpdateExt[2*comp[0]+1] = (updateExt[2*comp[0]+1] + 1) * this->Magnification - 1;
  magUpdateExt[2*comp[1]+1] = (updateExt[2*comp[1]+1] + 1) * this->Magnification - 1;
  magUpdateExt[2*comp[2]+1] = updateExt[comp[2]];

  vtkPixelExtent magLicExtent(
        magUpdateExt[2*comp[0]],
        magUpdateExt[2*comp[0]+1],
        magUpdateExt[2*comp[1]],
        magUpdateExt[2*comp[1]+1]);

  // add ghosts
  double rk4fac = 3.0;
  int nGhosts = static_cast<int>(this->Steps*this->StepSize*rk4fac);
  nGhosts = nGhosts < 1 ? 1 : nGhosts;
  nGhosts *= 2; // for second ee lic pass

  vtkPixelExtent magLicGuardExtent(magLicExtent);
  magLicGuardExtent.Grow(nGhosts);
  magLicGuardExtent &= magVectorExtent;

  vtkLineIntegralConvolution2D *LICer = vtkLineIntegralConvolution2D::New();
  LICer->SetContext(this->Context);
  LICer->SetNumberOfSteps(this->Steps);
  LICer->SetStepSize(stepSize);
  LICer->SetComponentIds(comp[0], comp[1]);
  //LICer->SetGridSpacings(spacing[comp[0]], spacing[comp[1]]);

  deque<vtkPixelExtent> magLicExtents(1, magLicExtent);
  deque<vtkPixelExtent> magLicGuardExtents(1, magLicGuardExtent);

  vtkTextureObject *licTex
     = LICer->Execute(
            magVectorExtent,
            magLicGuardExtents,
            magLicExtents,
            magVectorTex,
            NULL,
            noiseTex);

  LICer->Delete();
  noiseTex->Delete();
  magVectorTex->Delete();

  if ( !licTex )
  {
    vtkErrorMacro("Failed to compute LIC");
    return 0;
  }

  #if (vtkImageDataLIC2DDEBUG >= 1)
  vtkTextureIO::Write(
          "idlic2d_lic.vtk",
          licTex, NULL, NULL);
  #endif

  // transfer lic from texture to vtk array
  vtkIdType nOutTups = magLicExtent.Size();
  vtkFloatArray *licOut = vtkFloatArray::New();
  licOut->SetNumberOfComponents(3);
  licOut->SetNumberOfTuples(nOutTups);
  licOut->SetName("LIC");

  vtkPixelBufferObject *licPBO = licTex->Download();

  vtkPixelTransfer::Blit<float, float>(
        magVectorExtent,
        magLicExtent,
        magLicExtent,
        magLicExtent,
        4,
        (float*)licPBO->MapPackedBuffer(),
        3,
        licOut->GetPointer(0));

  licPBO->UnmapPackedBuffer();
  licPBO->Delete();
  licTex->Delete();

  // mask and convert to gray scale 3 components
  float *pLicOut = licOut->GetPointer(0);
  for (vtkIdType i=0; i<nOutTups; ++i)
  {
    float lic = pLicOut[3*i];
    float mask = pLicOut[3*i+1];
    if ( mask )
    {
      pLicOut[3*i+1] = pLicOut[3*i+2] = pLicOut[3*i] = 0.0f;
    }
    else
    {
      pLicOut[3*i+1] = pLicOut[3*i+2] = lic;
    }
  }

  // setup output
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  vtkImageData *output
    = vtkImageData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));
  if (!output)
  {
    vtkErrorMacro("Empty output");
    return 1;
  }

  output->SetExtent(magUpdateExt);
  output->SetSpacing(spacing);
  output->GetPointData()->SetScalars(licOut);
  licOut->Delete();

  // Ensures that the output extent is exactly same as what was asked for.
  //output->Crop(outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT()));

  return 1;
}

//----------------------------------------------------------------------------
void vtkImageDataLIC2D::PrintSelf( ostream & os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );

  os << indent << "Steps: "         << this->Steps          << "\n";
  os << indent << "StepSize: "      << this->StepSize       << "\n";
  os << indent << "Magnification: " << this->Magnification  << "\n";
  os << indent << "OpenGLExtensionsSupported: "
               << this->OpenGLExtensionsSupported << "\n";
}
