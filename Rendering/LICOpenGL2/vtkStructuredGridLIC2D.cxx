// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkStructuredGridLIC2D.h"

#include "vtkOpenGLHelper.h"

#include "vtkDataTransferHelper.h"
#include "vtkFloatArray.h"
#include "vtkImageData.h"
#include "vtkImageNoiseSource.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkLineIntegralConvolution2D.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLError.h"
#include "vtkOpenGLFramebufferObject.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLShaderCache.h"
#include "vtkOpenGLState.h"
#include "vtkPointData.h"
#include "vtkShaderProgram.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStructuredExtent.h"
#include "vtkTextureObject.h"

#include "vtkPixelBufferObject.h"
#include "vtkPixelExtent.h"
#include "vtkPixelTransfer.h"

#include <cassert>

#include "vtkStructuredGridLIC2D_fs.h"
#include "vtkTextureObjectVS.h"

#define PRINTEXTENT(ext)                                                                           \
  ext[0] << ", " << ext[1] << ", " << ext[2] << ", " << ext[3] << ", " << ext[4] << ", " << ext[5]

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkStructuredGridLIC2D);
//------------------------------------------------------------------------------
vtkStructuredGridLIC2D::vtkStructuredGridLIC2D()
{
  this->Context = nullptr;
  this->Steps = 1;
  this->StepSize = 1.0;
  this->Magnification = 1;
  this->SetNumberOfInputPorts(2);
  this->SetNumberOfOutputPorts(2);
  this->OwnWindow = false;
  this->FBOSuccess = 0;
  this->LICSuccess = 0;

  this->NoiseSource = vtkImageNoiseSource::New();
  this->NoiseSource->SetWholeExtent(0, 127, 0, 127, 0, 0);
  this->NoiseSource->SetMinimum(0.0);
  this->NoiseSource->SetMaximum(1.0);

  this->LICProgram = nullptr;
}

//------------------------------------------------------------------------------
vtkStructuredGridLIC2D::~vtkStructuredGridLIC2D()
{
  this->NoiseSource->Delete();
  this->SetContext(nullptr);
}

//------------------------------------------------------------------------------
vtkRenderWindow* vtkStructuredGridLIC2D::GetContext()
{
  return this->Context;
}

//------------------------------------------------------------------------------
int vtkStructuredGridLIC2D::SetContext(vtkRenderWindow* context)
{
  if (this->Context && this->OwnWindow)
  {
    this->Context->Delete();
    this->Context = nullptr;
  }
  this->OwnWindow = false;

  vtkOpenGLRenderWindow* openGLRenWin = vtkOpenGLRenderWindow::SafeDownCast(context);
  this->Context = openGLRenWin;

  this->Modified();
  return 1;
}

//------------------------------------------------------------------------------
// Description:
// Fill the input port information objects for this algorithm.  This
// is invoked by the first call to GetInputPortInformation for each
// port so subclasses can specify what they can handle.
// Redefined from the superclass.
int vtkStructuredGridLIC2D::FillInputPortInformation(int port, vtkInformation* info)
{
  if (port == 0)
  {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkStructuredGrid");
    info->Set(vtkAlgorithm::INPUT_IS_REPEATABLE(), 0);
    info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 0);
  }
  else
  {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkImageData");
    info->Set(vtkAlgorithm::INPUT_IS_REPEATABLE(), 0);
    info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  }

  return 1;
}

//------------------------------------------------------------------------------
// Description:
// Fill the output port information objects for this algorithm.
// This is invoked by the first call to GetOutputPortInformation for
// each port so subclasses can specify what they can handle.
// Redefined from the superclass.
int vtkStructuredGridLIC2D::FillOutputPortInformation(int port, vtkInformation* info)
{
  if (port == 0)
  {
    // input+texcoords
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkStructuredGrid");
  }
  else
  {
    // LIC texture
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkImageData");
  }

  return 1;
}
//------------------------------------------------------------------------------
// We need to report output extent after taking into consideration the
// magnification.
int vtkStructuredGridLIC2D::RequestInformation(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  int ext[6];
  double spacing[3];

  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(1);

  inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), ext);

  spacing[0] = 1.0;
  spacing[1] = 1.0;
  spacing[2] = 1.0;

  for (int axis = 0; axis < 3; axis++)
  {
    int wholeMin = ext[axis * 2];
    int wholeMax = ext[axis * 2 + 1];
    int dimension = wholeMax - wholeMin + 1;

    // Scale the output extent
    wholeMin = static_cast<int>(ceil(static_cast<double>(wholeMin * this->Magnification)));
    wholeMax = (dimension != 1)
      ? wholeMin + static_cast<int>(floor(static_cast<double>(dimension * this->Magnification))) - 1
      : wholeMin;

    ext[axis * 2] = wholeMin;
    ext[axis * 2 + 1] = wholeMax;
  }

  vtkDebugMacro(<< "request info whole ext = " << PRINTEXTENT(ext) << endl);

  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), ext, 6);
  outInfo->Set(vtkDataObject::SPACING(), spacing, 3);

  return 1;
}

//------------------------------------------------------------------------------
int vtkStructuredGridLIC2D::RequestUpdateExtent(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(1);

  // Tell the vector field input the extents that we need from it.
  // The downstream request needs to be downsized based on the Magnification.
  int ext[6];
  outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), ext);

  vtkDebugMacro(<< "request update extent, update ext = " << PRINTEXTENT(ext) << endl);

  for (int axis = 0; axis < 3; axis++)
  {
    int wholeMin = ext[axis * 2];
    int wholeMax = ext[axis * 2 + 1];
    int dimension = wholeMax - wholeMin + 1;

    // Scale the output extent
    wholeMin = static_cast<int>(ceil(static_cast<double>(wholeMin / this->Magnification)));
    wholeMax = dimension != 1
      ? wholeMin + static_cast<int>(floor(static_cast<double>(dimension / this->Magnification))) - 1
      :

      ext[axis * 2] = wholeMin;
    ext[axis * 2 + 1] = wholeMax;
  }
  vtkDebugMacro(<< "UPDATE_EXTENT: " << ext[0] << ", " << ext[1] << ", " << ext[2] << ", " << ext[3]
                << ", " << ext[4] << ", " << ext[5] << endl);
  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), ext, 6);

  vtkDebugMacro(<< "request update extent, update ext2 = " << PRINTEXTENT(ext) << endl);

  if (inputVector[1] != nullptr &&
    inputVector[1]->GetInformationObject(0) != nullptr) // optional input
  {
    inInfo = inputVector[1]->GetInformationObject(0);
    // always request the whole extent
    inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
      inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()), 6);
  }

  return 1;
}

//------------------------------------------------------------------------------
// Stolen from vtkImageAlgorithm. Should be in vtkStructuredGridAlgorithm.
void vtkStructuredGridLIC2D::AllocateOutputData(vtkDataObject* output, vtkInformation* outInfo)
{
  // set the extent to be the update extent
  vtkStructuredGrid* out = vtkStructuredGrid::SafeDownCast(output);
  if (out)
  {
    out->SetExtent(outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT()));
  }
  else
  {
    vtkImageData* out2 = vtkImageData::SafeDownCast(output);
    if (out2)
    {
      out2->SetExtent(outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT()));
    }
  }
}

//------------------------------------------------------------------------------
// Stolen from vtkImageData. Should be in vtkStructuredGrid.
void vtkStructuredGridLIC2D::AllocateScalars(vtkStructuredGrid* sg, vtkInformation* outInfo)
{
  int newType = VTK_DOUBLE;
  int newNumComp = 1;

  vtkInformation* scalarInfo = vtkDataObject::GetActiveFieldInformation(
    outInfo, vtkDataObject::FIELD_ASSOCIATION_POINTS, vtkDataSetAttributes::SCALARS);
  if (scalarInfo)
  {
    newType = scalarInfo->Get(vtkDataObject::FIELD_ARRAY_TYPE());
    if (scalarInfo->Has(vtkDataObject::FIELD_NUMBER_OF_COMPONENTS()))
    {
      newNumComp = scalarInfo->Get(vtkDataObject::FIELD_NUMBER_OF_COMPONENTS());
    }
  }

  vtkDataArray* scalars;

  // if the scalar type has not been set then we have a problem
  if (newType == VTK_VOID)
  {
    vtkErrorMacro("Attempt to allocate scalars before scalar type was set!.");
    return;
  }

  const int* extent = sg->GetExtent();
  // Use vtkIdType to avoid overflow on large images
  vtkIdType dims[3];
  dims[0] = extent[1] - extent[0] + 1;
  dims[1] = extent[3] - extent[2] + 1;
  dims[2] = extent[5] - extent[4] + 1;
  vtkIdType imageSize = dims[0] * dims[1] * dims[2];

  // if we currently have scalars then just adjust the size
  scalars = sg->GetPointData()->GetScalars();
  if (scalars && scalars->GetDataType() == newType && scalars->GetReferenceCount() == 1)
  {
    scalars->SetNumberOfComponents(newNumComp);
    scalars->SetNumberOfTuples(imageSize);
    // Since the execute method will be modifying the scalars
    // directly.
    scalars->Modified();
    return;
  }

  // allocate the new scalars
  scalars = vtkDataArray::CreateDataArray(newType);
  scalars->SetNumberOfComponents(newNumComp);

  // allocate enough memory
  scalars->SetNumberOfTuples(imageSize);

  sg->GetPointData()->SetScalars(scalars);
  scalars->Delete();
}

//------------------------------------------------------------------------------
int vtkStructuredGridLIC2D::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // 3 passes:
  // pass 1: render to compute the transformed vector field for the points.
  // pass 2: perform LIC with the new vector field. This has to happen in a
  // different pass than computation of the transformed vector.
  // pass 3: Render structured slice quads with correct texture correct
  // tcoords and apply the LIC texture to it.

  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkStructuredGrid* input =
    vtkStructuredGrid::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));

  int inputRequestedExtent[6];
  inInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), inputRequestedExtent);

  // Check if the input image is a 2D image (not 0D, not 1D, not 3D)
  int dims[3];
  vtkStructuredExtent::GetDimensions(inputRequestedExtent, dims);

  vtkDebugMacro(<< "dims = " << dims[0] << " " << dims[1] << " " << dims[2] << endl);
  vtkDebugMacro(<< "requested ext = " << inputRequestedExtent[0] << " " << inputRequestedExtent[1]
                << " " << inputRequestedExtent[2] << " " << inputRequestedExtent[3] << " "
                << inputRequestedExtent[4] << " " << inputRequestedExtent[5] << endl);

  if (!((dims[0] == 1) && (dims[1] > 1) && (dims[2] > 1)) &&
    !((dims[1] == 1) && (dims[0] > 1) && (dims[2] > 1)) &&
    !((dims[2] == 1) && (dims[0] > 1) && (dims[1] > 1)))
  {
    vtkErrorMacro(<< "input is not a 2D image." << endl);
    input = nullptr;
    inInfo = nullptr;
    return 0;
  }
  if (input->GetPointData() == nullptr)
  {
    vtkErrorMacro(<< "input does not have point data.");
    input = nullptr;
    inInfo = nullptr;
    return 0;
  }
  if (input->GetPointData()->GetVectors() == nullptr)
  {
    vtkErrorMacro(<< "input does not vectors on point data.");
    input = nullptr;
    inInfo = nullptr;
    return 0;
  }

  if (!this->Context)
  {
    vtkRenderWindow* renWin = vtkRenderWindow::New();
    if (this->SetContext(renWin) == 0)
    {
      vtkErrorMacro("Invalid render window");
      renWin->Delete();
      renWin = nullptr;
      input = nullptr;
      inInfo = nullptr;
      return 0;
    }

    renWin = nullptr; // to be released via this->context
    this->OwnWindow = true;
  }

  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkStructuredGrid* output =
    vtkStructuredGrid::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));
  this->AllocateOutputData(output, outInfo);
  output->ShallowCopy(input);

  vtkInformation* outInfoTexture = outputVector->GetInformationObject(1);
  vtkImageData* outputTexture =
    vtkImageData::SafeDownCast(outInfoTexture->Get(vtkDataObject::DATA_OBJECT()));
  this->AllocateOutputData(outputTexture, outInfoTexture);

  // Noise.
  vtkInformation* noiseInfo = inputVector[1]->GetInformationObject(0);
  vtkImageData* noise = nullptr;
  if (noiseInfo == nullptr)
  {
    this->NoiseSource->Update();
    noise = this->NoiseSource->GetOutput();
  }
  else
  {
    noise = vtkImageData::SafeDownCast(noiseInfo->Get(vtkDataObject::DATA_OBJECT()));

    if (noise->GetPointData() == nullptr)
    {
      vtkErrorMacro(<< "provided noise does not have point data.");
      return 0;
    }
    if (noise->GetPointData()->GetScalars() == nullptr)
    {
      vtkErrorMacro(<< "provided noise does not have scalars on point data.");
      return 0;
    }
  }

  vtkOpenGLClearErrorMacro();

  int width;
  int height;
  int firstComponent;
  int secondComponent;
  int slice;
  if (dims[0] == 1)
  {
    vtkDebugMacro(<< "x" << endl);
    firstComponent = 1;
    secondComponent = 2;
    slice = 0;
  }
  else
  {
    if (dims[1] == 1)
    {
      vtkDebugMacro(<< "y" << endl);
      firstComponent = 0;
      secondComponent = 2;
      slice = 1;
    }
    else
    {
      vtkDebugMacro(<< "z" << endl);
      firstComponent = 0;
      secondComponent = 1;
      slice = 2;
    }
  }

  width = dims[firstComponent];
  height = dims[secondComponent];

  vtkDebugMacro(<< "w = " << width << " h = " << height << endl);

  vtkDataTransferHelper* vectorFieldBus = vtkDataTransferHelper::New();
  vectorFieldBus->SetContext(this->Context);
  vectorFieldBus->SetCPUExtent(inputRequestedExtent); // input->GetExtent());
  vectorFieldBus->SetGPUExtent(inputRequestedExtent); // input->GetExtent());
  //  vectorFieldBus->SetTextureExtent(input->GetExtent());
  vectorFieldBus->SetArray(input->GetPointData()->GetVectors());

  vtkDataTransferHelper* pointBus = vtkDataTransferHelper::New();
  pointBus->SetContext(this->Context);
  pointBus->SetCPUExtent(inputRequestedExtent); // input->GetExtent());
  pointBus->SetGPUExtent(inputRequestedExtent); // input->GetExtent());
  //  pointBus->SetTextureExtent(input->GetExtent());
  pointBus->SetArray(input->GetPoints()->GetData());

  // Vector field in image space.
  int magWidth = this->Magnification * width;
  int magHeight = this->Magnification * height;

  vtkOpenGLRenderWindow* renWin = vtkOpenGLRenderWindow::SafeDownCast(this->Context);
  vtkTextureObject* vector2 = vtkTextureObject::New();
  vector2->SetContext(renWin);
  vector2->Create2D(magWidth, magHeight, 3, VTK_FLOAT, false);

  vtkDebugMacro(<< "Vector field in image space (target) textureId = " << vector2->GetHandle()
                << endl);

  vtkOpenGLState* ostate = renWin->GetState();
  ostate->PushFramebufferBindings();
  vtkOpenGLFramebufferObject* fbo = vtkOpenGLFramebufferObject::New();
  fbo->SetContext(renWin);
  fbo->Bind();
  fbo->AddColorAttachment(0, vector2);
  fbo->ActivateDrawBuffer(0);
  fbo->ActivateReadBuffer(0);

  // TODO --
  // step size is incorrect here
  // guard pixels are needed for parallel operations

  if (!fbo->Start(magWidth, magHeight))
  {
    ostate->PopFramebufferBindings();
    fbo->Delete();
    vector2->Delete();
    pointBus->Delete();
    vectorFieldBus->Delete();

    fbo = nullptr;
    vector2 = nullptr;
    pointBus = nullptr;
    vectorFieldBus = nullptr;

    noise = nullptr;
    input = nullptr;
    inInfo = nullptr;
    output = nullptr;
    outInfo = nullptr;
    noiseInfo = nullptr;
    outputTexture = nullptr;
    outInfoTexture = nullptr;

    this->FBOSuccess = 0;
    return 0;
  }
  this->FBOSuccess = 1;

  // build the shader program
  this->LICProgram = new vtkOpenGLHelper;
  std::string VSSource = vtkTextureObjectVS;
  std::string FSSource = vtkStructuredGridLIC2D_fs;
  std::string GSSource;
  this->LICProgram->Program = renWin->GetShaderCache()->ReadyShaderProgram(
    VSSource.c_str(), FSSource.c_str(), GSSource.c_str());
  vtkShaderProgram* pgm = this->LICProgram->Program;

  float fvalues[3];
  fvalues[0] = static_cast<float>(dims[0]);
  fvalues[1] = static_cast<float>(dims[1]);
  fvalues[2] = static_cast<float>(dims[2]);
  pgm->SetUniform3f("uDimensions", fvalues);
  pgm->SetUniformi("uSlice", slice);

  pointBus->Upload(0, nullptr);
  vtkTextureObject* points = pointBus->GetTexture();
  points->SetWrapS(vtkTextureObject::ClampToEdge);
  points->SetWrapT(vtkTextureObject::ClampToEdge);
  points->SetWrapR(vtkTextureObject::ClampToEdge);

  vectorFieldBus->Upload(0, nullptr);
  vtkTextureObject* vectorField = vectorFieldBus->GetTexture();
  vectorField->SetWrapS(vtkTextureObject::ClampToEdge);
  vectorField->SetWrapT(vtkTextureObject::ClampToEdge);
  vectorField->SetWrapR(vtkTextureObject::ClampToEdge);

  points->Activate();
  pgm->SetUniformi("texPoints", points->GetTextureUnit());
  vectorField->Activate();
  pgm->SetUniformi("texVectorField", vectorField->GetTextureUnit());

  vtkOpenGLCheckErrorMacro("failed during config");

  vtkDebugMacro(<< "glFinish before rendering quad" << endl);

  fbo->RenderQuad(0, magWidth - 1, 0, magHeight - 1, pgm, this->LICProgram->VAO);
  vtkOpenGLCheckErrorMacro("StructuredGridLIC2D projection failed");

  vtkDebugMacro(<< "glFinish after rendering quad" << endl);

  vtkLineIntegralConvolution2D* internal = vtkLineIntegralConvolution2D::New();
  if (!vtkLineIntegralConvolution2D::IsSupported(this->Context))
  {
    this->LICProgram->ReleaseGraphicsResources(renWin);
    delete this->LICProgram;
    this->LICProgram = nullptr;

    ostate->PopFramebufferBindings();
    fbo->Delete();
    vector2->Delete();
    internal->Delete();
    pointBus->Delete();
    vectorFieldBus->Delete();

    fbo = nullptr;
    vector2 = nullptr;
    internal = nullptr;
    pointBus = nullptr;
    vectorFieldBus = nullptr;

    noise = nullptr;
    input = nullptr;
    inInfo = nullptr;
    points = nullptr;
    output = nullptr;
    outInfo = nullptr;
    noiseInfo = nullptr;
    vectorField = nullptr;
    outputTexture = nullptr;
    outInfoTexture = nullptr;

    this->LICSuccess = 0;
    return 0;
  }

  internal->SetContext(renWin);
  internal->SetNumberOfSteps(this->Steps);
  internal->SetStepSize(this->StepSize);
  internal->SetComponentIds(firstComponent, secondComponent);

  vtkDataTransferHelper* noiseBus = vtkDataTransferHelper::New();
  noiseBus->SetContext(this->Context);
  noiseBus->SetCPUExtent(noise->GetExtent());
  noiseBus->SetGPUExtent(noise->GetExtent());
  //  noiseBus->SetTextureExtent(noise->GetExtent());
  noiseBus->SetArray(noise->GetPointData()->GetScalars());
  noiseBus->Upload(0, nullptr);

  vtkTextureObject* licTex = internal->Execute(vector2, noiseBus->GetTexture());
  if (licTex == nullptr)
  {
    this->LICProgram->ReleaseGraphicsResources(renWin);
    delete this->LICProgram;
    this->LICProgram = nullptr;

    ostate->PopFramebufferBindings();
    fbo->Delete();
    vector2->Delete();
    internal->Delete();
    pointBus->Delete();
    noiseBus->Delete();
    vectorFieldBus->Delete();

    fbo = nullptr;
    vector2 = nullptr;
    internal = nullptr;
    pointBus = nullptr;
    noiseBus = nullptr;
    vectorFieldBus = nullptr;

    noise = nullptr;
    input = nullptr;
    inInfo = nullptr;
    points = nullptr;
    output = nullptr;
    outInfo = nullptr;
    noiseInfo = nullptr;
    vectorField = nullptr;
    outputTexture = nullptr;
    outInfoTexture = nullptr;

    this->LICSuccess = 0;
    return 0;
  }
  this->LICSuccess = 1;

  // transfer lic from texture to vtk array
  vtkPixelExtent magLicExtent(magWidth, magHeight);
  vtkIdType nOutTups = static_cast<vtkIdType>(magLicExtent.Size());

  vtkFloatArray* licOut = vtkFloatArray::New();
  licOut->SetNumberOfComponents(3);
  licOut->SetNumberOfTuples(nOutTups);
  licOut->SetName("LIC");

  vtkPixelBufferObject* licPBO = licTex->Download();

  vtkPixelTransfer::Blit<float, float>(magLicExtent, magLicExtent, magLicExtent, magLicExtent, 4,
    (float*)licPBO->MapPackedBuffer(), 3, licOut->GetPointer(0));

  licPBO->UnmapPackedBuffer();
  licPBO->Delete();
  licTex->Delete();

  // mask and convert to gray scale 3 components
  float* pLicOut = licOut->GetPointer(0);
  for (vtkIdType i = 0; i < nOutTups; ++i)
  {
    float lic = pLicOut[3 * i];
    float mask = pLicOut[3 * i + 1];
    if (mask)
    {
      pLicOut[3 * i + 1] = pLicOut[3 * i + 2] = pLicOut[3 * i] = 0.0f;
    }
    else
    {
      pLicOut[3 * i + 1] = pLicOut[3 * i + 2] = lic;
    }
  }

  outputTexture->GetPointData()->SetScalars(licOut);
  licOut->Delete();

  // Pass three. Generate texture coordinates. Software.
  vtkFloatArray* tcoords = vtkFloatArray::New();
  tcoords->SetNumberOfComponents(2);
  tcoords->SetNumberOfTuples(dims[0] * dims[1] * dims[2]);
  output->GetPointData()->SetTCoords(tcoords);
  tcoords->Delete();

  double ddim[3];
  ddim[0] = static_cast<double>(dims[0] - 1);
  ddim[1] = static_cast<double>(dims[1] - 1);
  ddim[2] = static_cast<double>(dims[2] - 1);

  int tz = 0;
  while (tz < dims[slice])
  {
    int ty = 0;
    while (ty < dims[secondComponent])
    {
      int tx = 0;
      while (tx < dims[firstComponent])
      {
        tcoords->SetTuple2((tz * dims[secondComponent] + ty) * dims[firstComponent] + tx,
          tx / ddim[firstComponent], ty / ddim[secondComponent]);
        ++tx;
      }
      ++ty;
    }
    ++tz;
  }

  ostate->PopFramebufferBindings();

  internal->Delete();
  noiseBus->Delete();
  vectorFieldBus->Delete();
  pointBus->Delete();
  vector2->Delete();
  fbo->Delete();

  this->LICProgram->ReleaseGraphicsResources(renWin);
  delete this->LICProgram;
  this->LICProgram = nullptr;

  vtkOpenGLCheckErrorMacro("failed after RequestData");

  return 1;
}

//------------------------------------------------------------------------------
void vtkStructuredGridLIC2D::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Steps: " << this->Steps << "\n";
  os << indent << "StepSize: " << this->StepSize << "\n";
  os << indent << "FBOSuccess: " << this->FBOSuccess << "\n";
  os << indent << "LICSuccess: " << this->LICSuccess << "\n";
  os << indent << "Magnification: " << this->Magnification << "\n";
}
VTK_ABI_NAMESPACE_END
