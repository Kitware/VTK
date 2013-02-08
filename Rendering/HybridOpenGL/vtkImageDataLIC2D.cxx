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

#include "vtkFloatArray.h"
#include "vtkImageData.h"
#include "vtkImageNoiseSource.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkDataTransferHelper.h"
#include "vtkImageDataLIC2DExtentTranslator.h"
#include "vtkLineIntegralConvolution2D.h"
#include "vtkStructuredExtent.h"
#include "vtkTextureObject.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLExtensionManager.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkPointData.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStructuredData.h"
#include "vtkFrameBufferObject.h"

#include "vtkgl.h"

#define PRINTEXTENT(ext) \
  ext[0] << ", " << ext[1] << ", " << ext[2] << ", " << ext[3] << ", " << ext[4] << ", " << ext[5]

vtkStandardNewMacro(vtkImageDataLIC2D);
//----------------------------------------------------------------------------
vtkImageDataLIC2D::vtkImageDataLIC2D()
{
  this->Context    = 0;
  this->Steps      = 20;
  this->StepSize   = 1.0;
  this->FBOSuccess = 0;
  this->LICSuccess = 0;
  this->Magnification = 1;

  this->NoiseSource = vtkImageNoiseSource::New();
  this->NoiseSource->SetWholeExtent(0, 127, 0, 127, 0, 0);
  this->NoiseSource->SetMinimum(0.0);
  this->NoiseSource->SetMaximum(1.0);

  this->SetNumberOfInputPorts(2);
  // by default process active point vectors
  this->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS,
    vtkDataSetAttributes::VECTORS);

  this->OwnWindow = false;
  this->OpenGLExtensionsSupported    = 0;
  this->ARBColorBufferFloatSupported = false;
}

//----------------------------------------------------------------------------
vtkImageDataLIC2D::~vtkImageDataLIC2D()
{
  this->NoiseSource->Delete();
  this->SetContext( NULL );
}

//----------------------------------------------------------------------------
int vtkImageDataLIC2D::SetContext( vtkRenderWindow * context )
{
  if ( this->Context == context )
    {
    return this->OpenGLExtensionsSupported;
    }

  if ( this->Context && this->OwnWindow )
    {
    this->Context->Delete();
    this->Context = NULL;
    }
  this->OwnWindow = false;

  vtkOpenGLRenderWindow * openGLRenWin =
  vtkOpenGLRenderWindow::SafeDownCast( context );
  this->Context = openGLRenWin;

  if ( openGLRenWin )
    {
    openGLRenWin->Render();
    openGLRenWin->MakeCurrent();
    vtkOpenGLExtensionManager * mgr = openGLRenWin->GetExtensionManager();

    // optional for texture objects.
    mgr->LoadSupportedExtension( "GL_EXT_texture_integer" );

    //this->ARBColorBufferFloatSupported =
    //  mgr->LoadSupportedExtension("GL_ARB_color_buffer_float");

    if (  !mgr->LoadSupportedExtension( "GL_VERSION_1_3" ) ||
          !mgr->LoadSupportedExtension( "GL_VERSION_1_2" ) ||
          !mgr->LoadSupportedExtension( "GL_VERSION_2_0" ) ||
          !mgr->LoadSupportedExtension( "GL_ARB_texture_float" ) ||
          !mgr->LoadSupportedExtension( "GL_ARB_texture_non_power_of_two" )
       )
      {
      vtkErrorMacro( "Required OpenGL extensions not supported." );
      mgr = NULL;
      this->Context = 0;
      openGLRenWin  = NULL;
      return 0;
      }

    mgr = NULL;
    }

  openGLRenWin = NULL;
  this->Modified();

  this->OpenGLExtensionsSupported = 1;
  return 1;
}

//----------------------------------------------------------------------------
vtkRenderWindow* vtkImageDataLIC2D::GetContext()
{
  return this->Context;
}

//----------------------------------------------------------------------------
// Description:
// Fill the input port information objects for this algorithm.  This
// is invoked by the first call to GetInputPortInformation for each
// port so subclasses can specify what they can handle.
// Redefined from the superclass.
int vtkImageDataLIC2D::FillInputPortInformation(int port,
                                          vtkInformation *info)
{
  if (!this->Superclass::FillInputPortInformation(port, info))
    {
    return 0;
    }

  if (port==1)
    {
    info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
    }

  return 1;
}

//----------------------------------------------------------------------------
void vtkImageDataLIC2D::TranslateInputExtent(const int* inExt,
  const int* inWholeExtent, int *resultExtent)
{
  for (int axis = 0; axis < 3; axis++)
    {
    int whole_dim = (inWholeExtent[axis*2+1] - inWholeExtent[axis*2]) + 1;
    int dim = (inExt[axis*2+1] - inExt[axis*2]) + 1;

    // Scale the output extent
    int min = inExt[axis*2] * this->Magnification;
    int max = min + (whole_dim>1? (dim * this->Magnification - 1) : 0);

    resultExtent[axis*2] = min;
    resultExtent[axis*2+1] = max;
    }
}

//----------------------------------------------------------------------------
// We need to report output extent after taking into consideration the
// magnification.
int vtkImageDataLIC2D::RequestInformation(vtkInformation* vtkNotUsed(request),
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

  // Setup ExtentTranslator
  vtkImageDataLIC2DExtentTranslator* extTranslator =
    vtkImageDataLIC2DExtentTranslator::SafeDownCast(
      vtkStreamingDemandDrivenPipeline::GetExtentTranslator(outInfo));

  if (!extTranslator)
    {
    extTranslator = vtkImageDataLIC2DExtentTranslator::New();
    vtkStreamingDemandDrivenPipeline::SetExtentTranslator(outInfo, extTranslator);
    extTranslator->Delete();
    }
  extTranslator->SetAlgorithm(this);
  extTranslator->SetInputWholeExtent(wholeExtent);
  extTranslator->SetInputExtentTranslator(
    vtkExtentTranslator::SafeDownCast(
    inInfo->Get(vtkStreamingDemandDrivenPipeline::EXTENT_TRANSLATOR())));
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
    inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
      inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()),
      6);
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkImageDataLIC2D::RequestData(
  vtkInformation        * vtkNotUsed(request),
  vtkInformationVector ** inputVector,
  vtkInformationVector  * outputVector )
{
  vtkInformation * inInfo= inputVector[0]->GetInformationObject(0);
  vtkImageData   * input = vtkImageData::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));

  // Check if the input image is a 2D image (not 0D, not 1D, not 3D)
  int dims[3];
  input->GetDimensions(dims);

  int dataDescription = vtkStructuredData::GetDataDescription(dims);
  if (  vtkStructuredData::GetDataDimension( dataDescription ) != 2  )
    {
    vtkErrorMacro( "Input is not a 2D image." );
    input  = NULL;
    inInfo = NULL;
    return 0;
    }

  vtkIdType numPoints = input->GetNumberOfPoints();
  vtkSmartPointer<vtkDataArray> inVectors =
    this->GetInputArrayToProcess( 0, inputVector );

  if ( inVectors.GetPointer() == 0 )
    {
    vtkErrorMacro( "No input vectors selected. "
                   "Vectors are required for line integral convolution." );
    input  = NULL;
    inInfo = NULL;
    return 0;
    }

  if ( inVectors->GetNumberOfTuples() != numPoints )
    {
    vtkErrorMacro( "Only point vectors are supported." );
    input  = NULL;
    inInfo = NULL;
    return 0;
    }

  if ( !this->Context )
    {
    vtkRenderWindow * renWin = vtkRenderWindow::New();
    if (  this->SetContext( renWin ) == 0  )
      {
      renWin->Delete();
      renWin = NULL;
      input  = NULL;
      inInfo = NULL;
      return 0;
      }

    renWin = NULL; // will be released via this->Context
    this->OwnWindow = true;
    }

  this->Context->MakeCurrent();
  this->Context->SetReportGraphicErrors(1);

  // Noise.
  vtkInformation *noiseInfo = inputVector[1]->GetInformationObject(0);
  vtkImageData *noise=0;
  if (noiseInfo==0)
    {
    this->NoiseSource->Update();
    noise = this->NoiseSource->GetOutput();
    }
  else
    {
    noise = vtkImageData::SafeDownCast(
      noiseInfo->Get(vtkDataObject::DATA_OBJECT()));

    if (noise->GetPointData()==0)
      {
      vtkErrorMacro("Provided noise does not have point data.");
      return 0;
      }
    if (noise->GetPointData()->GetScalars()==0)
      {
      vtkErrorMacro("Provided noise does not have scalars on point data.");
      return 0;
      }
    }

  int firstComponent;
  int secondComponent;
  switch (dataDescription)
    {
  case VTK_XY_PLANE:
    firstComponent = 0;
    secondComponent = 1;
    break;

  case VTK_YZ_PLANE:
    firstComponent = 1;
    secondComponent = 2;
    break;

  case VTK_XZ_PLANE:
    firstComponent = 0;
    secondComponent = 2;
    break;

  default:
    vtkErrorMacro("Sanity check failed. This filter can only handle 2D inputs.");
    return 0;
    }

  double *spacing = input->GetSpacing();
  // this->StepSize is specified as a unit of the cell length, while
  // vtkLineIntegralConvolution2D expects step size in normalized image space,
  // hence we do the conversion.
  double cellLength = sqrt(spacing[firstComponent] * spacing[firstComponent] +
    spacing[secondComponent] * spacing[secondComponent]);

  double w = spacing[firstComponent] * dims[firstComponent];
  double h = spacing[secondComponent] * dims[secondComponent];
  double normalizationFactor = sqrt(w*w + h*h);
  double stepSize = this->StepSize * cellLength / normalizationFactor;
  vtkDebugMacro( << "** StepSize (Normalized Image Space): " << stepSize << endl );

  vtkLineIntegralConvolution2D * internal=vtkLineIntegralConvolution2D::New();
  if (  !internal->IsSupported( this->Context )  )
    {
    internal->Delete();
    internal   = NULL;
    input      = NULL;
    noise      = NULL;
    inInfo     = NULL;
    spacing    = NULL;
    noiseInfo  = NULL;

    this->LICSuccess = 0;
    return 0;
    }
  internal->SetNumberOfSteps(this->Steps);
  internal->SetLICStepSize(stepSize);
  internal->SetComponentIds(firstComponent,secondComponent);
  internal->SetGridSpacings(spacing[firstComponent], spacing[secondComponent]);
  internal->SetMagnification(this->Magnification);

  vtkDataTransferHelper *vectorBus = vtkDataTransferHelper::New();
  vectorBus->SetContext(this->Context);
  vectorBus->SetCPUExtent(input->GetExtent());
  vectorBus->SetGPUExtent(input->GetExtent());
  //  vectorBus->SetTextureExtent(input->GetExtent());
  vectorBus->SetArray(inVectors);

  vtkDataTransferHelper *noiseBus = vtkDataTransferHelper::New();
  noiseBus->SetContext(this->Context);
  noiseBus->SetCPUExtent(noise->GetExtent());
  noiseBus->SetGPUExtent(noise->GetExtent());
  //  noiseBus->SetTextureExtent(noise->GetExtent());
  noiseBus->SetArray(noise->GetPointData()->GetScalars());

  // For uploading of float textures without clamping, we create a FBO with a
  // float color buffer.
  // Vector field in image space.
  vtkTextureObject *tempBuffer = vtkTextureObject::New();
  tempBuffer->SetContext(this->Context);
  tempBuffer->Create2D(128, 128, 3, VTK_FLOAT, false);

  vtkFrameBufferObject *fbo = vtkFrameBufferObject::New();
  fbo->SetContext(this->Context);
  fbo->SetColorBuffer(0, tempBuffer);
  fbo->SetNumberOfRenderTargets(1);
  fbo->SetActiveBuffer(0);
  if (  !fbo->Start( 128, 128, false )  )
    {
    fbo->Delete();
    internal->Delete();
    noiseBus->Delete();
    vectorBus->Delete();
    tempBuffer->Delete();
    fbo        = NULL;
    internal   = NULL;
    noiseBus   = NULL;
    vectorBus  = NULL;
    tempBuffer = NULL;

    input      = NULL;
    noise      = NULL;
    inInfo     = NULL;
    spacing    = NULL;
    noiseInfo  = NULL;

    this->FBOSuccess = 0;
    return 0;
    }
  this->FBOSuccess = 1;
  tempBuffer->Delete();
  tempBuffer = NULL;

  vtkgl::ActiveTexture(vtkgl::TEXTURE0);
  vectorBus->Upload(0,0);
  vectorBus->GetTexture()->Bind();
  glTexParameteri(vectorBus->GetTexture()->GetTarget(),GL_TEXTURE_WRAP_S,
                  GL_CLAMP);
  glTexParameteri(vectorBus->GetTexture()->GetTarget(),GL_TEXTURE_WRAP_T,
                  GL_CLAMP);
  glTexParameteri(vectorBus->GetTexture()->GetTarget(), vtkgl::TEXTURE_WRAP_R,
                  GL_CLAMP);
  glTexParameteri(vectorBus->GetTexture()->GetTarget(), GL_TEXTURE_MIN_FILTER,
                  GL_LINEAR);
  glTexParameteri(vectorBus->GetTexture()->GetTarget(), GL_TEXTURE_MAG_FILTER,
                  GL_LINEAR);

  internal->SetVectorField(vectorBus->GetTexture());

  vtkgl::ActiveTexture(vtkgl::TEXTURE1);
  noiseBus->Upload(0,0);
  noiseBus->GetTexture()->Bind();
  glTexParameteri(noiseBus->GetTexture()->GetTarget(),GL_TEXTURE_WRAP_S,
                  GL_REPEAT);
  glTexParameteri(noiseBus->GetTexture()->GetTarget(),GL_TEXTURE_WRAP_T,
                  GL_REPEAT);
  glTexParameteri(noiseBus->GetTexture()->GetTarget(),vtkgl::TEXTURE_WRAP_R,
                  GL_REPEAT);
  glTexParameteri(noiseBus->GetTexture()->GetTarget(),GL_TEXTURE_MIN_FILTER,
                  GL_NEAREST);
  glTexParameteri(noiseBus->GetTexture()->GetTarget(),GL_TEXTURE_MAG_FILTER,
                  GL_NEAREST);
  internal->SetNoise(noiseBus->GetTexture());
  fbo->Delete();
  fbo = NULL;

  int inputRequestedExtent[6];
  inInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), inputRequestedExtent);
  // transform inputRequestedExtent relative to the input's actual extent since the
  // vtkLineIntegralConvolution2D needs extents relative to the input vector
  // field.
  vtkStructuredExtent::Transform(inputRequestedExtent, input->GetExtent());
  int licextent[4];
  switch (dataDescription)
    {
  case VTK_XY_PLANE:
    licextent[0] = inputRequestedExtent[0];
    licextent[1] = inputRequestedExtent[1];
    licextent[2] = inputRequestedExtent[2];
    licextent[3] = inputRequestedExtent[3];
    break;

  case VTK_YZ_PLANE:
    licextent[0] = inputRequestedExtent[2];
    licextent[1] = inputRequestedExtent[3];
    licextent[2] = inputRequestedExtent[4];
    licextent[3] = inputRequestedExtent[5];
    break;

  case VTK_XZ_PLANE:
    licextent[0] = inputRequestedExtent[0];
    licextent[1] = inputRequestedExtent[1];
    licextent[2] = inputRequestedExtent[4];
    licextent[3] = inputRequestedExtent[5];
    break;
    }

  if (  internal->Execute(licextent) == 0  )
    {
    internal->Delete();
    noiseBus->Delete();
    vectorBus->Delete();
    internal   = NULL;
    noiseBus   = NULL;
    vectorBus  = NULL;

    input      = NULL;
    noise      = NULL;
    inInfo     = NULL;
    spacing    = NULL;
    noiseInfo  = NULL;

    this->LICSuccess = 0;
    return 0;
    }
  this->LICSuccess = 1;

  glFlush(); // breakpoint for debugging.

  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  vtkImageData *output = vtkImageData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkDataTransferHelper *outputBus=vtkDataTransferHelper::New();
  outputBus->SetContext(this->Context);

  vtkTextureObject* licTexture = internal->GetLIC();
  int gpuExtent[6];
  inInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), gpuExtent);
  gpuExtent[0] *= this->Magnification;
  gpuExtent[2] *= this->Magnification;
  gpuExtent[4] *= this->Magnification;
  switch (dataDescription)
    {
  case VTK_XY_PLANE:
    gpuExtent[1] = gpuExtent[0] + licTexture->GetWidth() - 1;
    gpuExtent[3] = gpuExtent[2] + licTexture->GetHeight() - 1;
    break;

  case VTK_YZ_PLANE:
    gpuExtent[3] = gpuExtent[2] + licTexture->GetWidth() - 1;
    gpuExtent[5] = gpuExtent[4] + licTexture->GetHeight() -1;
    break;

  case VTK_XZ_PLANE:
    gpuExtent[1] = gpuExtent[0] + licTexture->GetWidth() -1;
    gpuExtent[5] = gpuExtent[4] + licTexture->GetHeight() - 1;
    break;

    }
  vtkDebugMacro( << "GPU Extent: " << PRINTEXTENT( gpuExtent ) << endl );
  // It is possible that GPU extent is larger than what the output expected,
  // hence we allocate the output using the GPU extent and then crop it.

  output->SetExtent(gpuExtent);
  output->AllocateScalars(VTK_FLOAT, 3);
  outputBus->SetCPUExtent(gpuExtent);
  outputBus->SetGPUExtent(gpuExtent);
  outputBus->SetTexture(internal->GetLIC());
  outputBus->SetArray(output->GetPointData()->GetScalars());
  output->GetPointData()->GetScalars()->SetName("LIC");
  outputBus->Download();
  outputBus->Delete();
  internal->Delete();
  vectorBus->Delete();
  noiseBus->Delete();

  // Ensures that the output extent is exactly same as what was asked for.
  output->Crop(outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT()));
  return 1;
}

//----------------------------------------------------------------------------
void vtkImageDataLIC2D::PrintSelf( ostream & os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );

  os << indent << "Steps: "         << this->Steps          << "\n";
  os << indent << "StepSize: "      << this->StepSize       << "\n";
  os << indent << "FBOSuccess: "    << this->FBOSuccess     << "\n";
  os << indent << "LICSuccess: "    << this->LICSuccess     << "\n";
  os << indent << "Magnification: " << this->Magnification  << "\n";
  os << indent << "OpenGLExtensionsSupported: "
               << this->OpenGLExtensionsSupported << "\n";
}
