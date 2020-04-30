/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGPUVolumeRayCastMapper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include <algorithm>
#include <cassert>

#include "vtkMatrix3x3.h"
#include <vtkCamera.h>
#include <vtkCellData.h>
#include <vtkCommand.h>
#include <vtkContourValues.h>
#include <vtkDataArray.h>
#include <vtkGPUInfo.h>
#include <vtkGPUInfoList.h>
#include <vtkGPUVolumeRayCastMapper.h>
#include <vtkImageData.h>
#include <vtkImageResample.h>
#include <vtkInformation.h>
#include <vtkMultiVolume.h>
#include <vtkObjectFactory.h>
#include <vtkPointData.h>
#include <vtkRenderWindow.h>
#include <vtkRenderer.h>
#include <vtkRendererCollection.h>
#include <vtkTimerLog.h>
#include <vtkVolumeProperty.h>

// Return nullptr if no override is supplied.
vtkAbstractObjectFactoryNewMacro(vtkGPUVolumeRayCastMapper);
vtkCxxSetObjectMacro(vtkGPUVolumeRayCastMapper, MaskInput, vtkImageData);

vtkGPUVolumeRayCastMapper::vtkGPUVolumeRayCastMapper()
  : LockSampleDistanceToInputSpacing(0)
{
  this->AutoAdjustSampleDistances = 1;
  this->ImageSampleDistance = 1.0;
  this->MinimumImageSampleDistance = 1.0;
  this->MaximumImageSampleDistance = 10.0;
  this->RenderToImage = 0;
  this->DepthImageScalarType = VTK_FLOAT;
  this->ClampDepthToBackface = 0;
  this->UseJittering = 0;
  this->UseDepthPass = 0;
  this->DepthPassContourValues = nullptr;
  this->SampleDistance = 1.0;
  this->SmallVolumeRender = 0;
  this->BigTimeToDraw = 0.0;
  this->SmallTimeToDraw = 0.0;
  this->FinalColorWindow = 1.0;
  this->FinalColorLevel = 0.5;
  this->GeneratingCanonicalView = 0;
  this->CanonicalViewImageData = nullptr;

  this->MaskInput = nullptr;
  this->MaskBlendFactor = 1.0f;
  this->MaskType = vtkGPUVolumeRayCastMapper::LabelMapMaskType;

  this->ColorRangeType = TFRangeType::SCALAR;
  this->ScalarOpacityRangeType = TFRangeType::SCALAR;
  this->GradientOpacityRangeType = TFRangeType::SCALAR;

  this->AMRMode = 0;
  this->CellFlag = 0;

  this->ClippedCroppingRegionPlanes[0] = VTK_DOUBLE_MAX;
  this->ClippedCroppingRegionPlanes[1] = VTK_DOUBLE_MIN;
  this->ClippedCroppingRegionPlanes[2] = VTK_DOUBLE_MAX;
  this->ClippedCroppingRegionPlanes[3] = VTK_DOUBLE_MIN;
  this->ClippedCroppingRegionPlanes[4] = VTK_DOUBLE_MAX;
  this->ClippedCroppingRegionPlanes[5] = VTK_DOUBLE_MIN;

  this->MaxMemoryInBytes = 0;
  vtkGPUInfoList* l = vtkGPUInfoList::New();
  l->Probe();
  if (l->GetNumberOfGPUs() > 0)
  {
    vtkGPUInfo* info = l->GetGPUInfo(0);
    this->MaxMemoryInBytes = info->GetDedicatedVideoMemory();
    if (this->MaxMemoryInBytes == 0)
    {
      this->MaxMemoryInBytes = info->GetDedicatedSystemMemory();
    }
    // we ignore info->GetSharedSystemMemory(); as this is very slow.
  }
  l->Delete();

  if (this->MaxMemoryInBytes == 0) // use some default value: 128MB.
  {
    this->MaxMemoryInBytes = 128 * 1024 * 1024;
  }

  this->MaxMemoryFraction = 0.75;

  this->ReportProgress = true;

  this->SetNumberOfInputPorts(10);
}

// ----------------------------------------------------------------------------
vtkGPUVolumeRayCastMapper::~vtkGPUVolumeRayCastMapper()
{
  this->SetMaskInput(nullptr);

  for (auto& input : this->TransformedInputs)
  {
    input.second->Delete();
  }
  this->TransformedInputs.clear();

  this->LastInputs.clear();

  if (this->DepthPassContourValues)
  {
    this->DepthPassContourValues->Delete();
  }
}

// ----------------------------------------------------------------------------
// The render method that is called from the volume. If this is a canonical
// view render, a specialized version of this method will be called instead.
// Otherwise we will
//   - Invoke a start event
//   - Start timing
//   - Check that everything is OK for rendering
//   - Render
//   - Stop the timer and record results
//   - Invoke an end event
// ----------------------------------------------------------------------------
void vtkGPUVolumeRayCastMapper::Render(vtkRenderer* ren, vtkVolume* vol)
{
  // Catch renders that are happening due to a canonical view render and
  // handle them separately.
  if (this->GeneratingCanonicalView)
  {
    this->CanonicalViewRender(ren, vol);
    return;
  }

  // Invoke a VolumeMapperRenderStartEvent
  this->InvokeEvent(vtkCommand::VolumeMapperRenderStartEvent, nullptr);

  // Start the timer to time the length of this render
  vtkTimerLog* timer = vtkTimerLog::New();
  timer->StartTimer();

  // Make sure everything about this render is OK.
  // This is where the input is updated.
  if (this->ValidateRender(ren, vol))
  {
    // Everything is OK - so go ahead and really do the render
    this->GPURender(ren, vol);
  }

  // Stop the timer
  timer->StopTimer();
  double t = timer->GetElapsedTime();

  //  cout << "Render Timer " << t << " seconds, " << 1.0/t << " frames per second" << endl;

  this->TimeToDraw = t;
  timer->Delete();

  if (vol->GetAllocatedRenderTime() < 1.0)
  {
    this->SmallTimeToDraw = t;
  }
  else
  {
    this->BigTimeToDraw = t;
  }

  // Invoke a VolumeMapperRenderEndEvent
  this->InvokeEvent(vtkCommand::VolumeMapperRenderEndEvent, nullptr);
}

// ----------------------------------------------------------------------------
// Special version for rendering a canonical view - we don't do things like
// invoke start or end events, and we don't capture the render time.
// ----------------------------------------------------------------------------
void vtkGPUVolumeRayCastMapper::CanonicalViewRender(vtkRenderer* ren, vtkVolume* vol)
{
  // Make sure everything about this render is OK
  if (this->ValidateRender(ren, vol))
  {
    // Everything is OK - so go ahead and really do the render
    this->GPURender(ren, vol);
  }
}

// ----------------------------------------------------------------------------
// This method us used by the render method to validate everything before
// attempting to render. This method returns 0 if something is not right -
// such as missing input, a null renderer or a null volume, no scalars, etc.
// In some cases it will produce a vtkErrorMacro message, and in others
// (for example, in the case of cropping planes that define a region with
// a volume or 0 or less) it will fail silently. If everything is OK, it will
// return with a value of 1.
// ----------------------------------------------------------------------------
int vtkGPUVolumeRayCastMapper::ValidateRender(vtkRenderer* ren, vtkVolume* vol)
{
  // Check that we have everything we need to render.
  int goodSoFar = 1;

  // Check for a renderer - we MUST have one
  if (!ren)
  {
    goodSoFar = 0;
    vtkErrorMacro("Renderer cannot be null.");
  }

  // Check for the volume - we MUST have one
  if (goodSoFar && !vol)
  {
    goodSoFar = 0;
    vtkErrorMacro("Volume cannot be null.");
  }

  // Check the cropping planes. If they are invalid, just silently
  // fail. This will happen when an interactive widget is dragged
  // such that it defines 0 or negative volume - this can happen
  // and should just not render the volume.
  // Check the cropping planes
  if (goodSoFar && this->Cropping &&
    (this->CroppingRegionPlanes[0] >= this->CroppingRegionPlanes[1] ||
      this->CroppingRegionPlanes[2] >= this->CroppingRegionPlanes[3] ||
      this->CroppingRegionPlanes[4] >= this->CroppingRegionPlanes[5]))
  {
    // No error message here - we want to be silent
    goodSoFar = 0;
  }

  if (!goodSoFar)
  {
    return 0;
  }

  auto multiVol = vtkMultiVolume::SafeDownCast(vol);
  bool success = true;
  for (const auto& port : this->Ports)
  {
    auto currentVol = multiVol ? multiVol->GetVolume(port) : vol;
    success &= this->ValidateInput(currentVol->GetProperty(), port) == 1;
  }
  return (success ? 1 : 0);
}

vtkImageData* vtkGPUVolumeRayCastMapper::FindData(int port, DataMap& container)
{
  const auto it = container.find(port);
  if (it == container.cend())
  {
    return nullptr;
  }
  return it->second;
}

void vtkGPUVolumeRayCastMapper::CloneInputs()
{
  for (const auto& port : this->Ports)
  {
    vtkImageData* input = this->GetInput(port);
    this->CloneInput(input, port);
  }
}

void vtkGPUVolumeRayCastMapper::CloneInput(vtkImageData* input, const int port)
{
  // Clone input into a transformed input
  vtkImageData* clone;
  vtkImageData* currentData = this->FindData(port, this->TransformedInputs);
  if (!currentData)
  {
    clone = vtkImageData::New();
    clone->Register(this);
    this->TransformedInputs[port] = clone;
    clone->Delete();

    this->LastInputs[port] = nullptr;
  }
  else
  {
    clone = this->TransformedInputs[port];
  }

  // If we have a timestamp change or data change then create a new clone
  if (input != this->LastInputs[port] || input->GetMTime() > clone->GetMTime())
  {
    this->LastInputs[port] = input;
    this->TransformInput(port);
  }
}

int vtkGPUVolumeRayCastMapper::ValidateInput(vtkVolumeProperty* property, const int port)
{
  vtkImageData* input = this->GetInput(port);

  vtkTypeBool goodSoFar = 1;
  if (input == nullptr)
  {
    vtkErrorMacro("Input is nullptr but is required");
    goodSoFar = 0;
  }

  if (goodSoFar)
  {
    this->GetInputAlgorithm(port, 0)->Update();
  }

  if (goodSoFar)
  {
    this->CloneInput(input, port);
  }

  // Update the date then make sure we have scalars. Note
  // that we must have point or cell scalars because field
  // scalars are not supported.
  vtkDataArray* scalars = nullptr;
  if (goodSoFar)
  {
    // Now make sure we can find scalars
    scalars = this->GetScalars(this->TransformedInputs[port], this->ScalarMode,
      this->ArrayAccessMode, this->ArrayId, this->ArrayName, this->CellFlag);

    // We couldn't find scalars
    if (!scalars)
    {
      vtkErrorMacro("No scalars named \"" << this->ArrayName << "\" or with id " << this->ArrayId
                                          << " found on input.");
      goodSoFar = 0;
    }
    // Even if we found scalars, if they are field data scalars that isn't good
    else if (this->CellFlag == 2)
    {
      vtkErrorMacro("Only point or cell scalar support - found field scalars instead.");
      goodSoFar = 0;
    }
  }

  // Make sure the scalar type is actually supported. This mappers supports
  // almost all standard scalar types.
  if (goodSoFar)
  {
    switch (scalars->GetDataType())
    {
      case VTK_CHAR:
        vtkErrorMacro(<< "scalar of type VTK_CHAR is not supported "
                      << "because this type is platform dependent. "
                      << "Use VTK_SIGNED_CHAR or VTK_UNSIGNED_CHAR instead.");
        goodSoFar = 0;
        break;
      case VTK_BIT:
        vtkErrorMacro("scalar of type VTK_BIT is not supported by this mapper.");
        goodSoFar = 0;
        break;
      case VTK_ID_TYPE:
        vtkErrorMacro("scalar of type VTK_ID_TYPE is not supported by this mapper.");
        goodSoFar = 0;
        break;
      case VTK_STRING:
        vtkErrorMacro("scalar of type VTK_STRING is not supported by this mapper.");
        goodSoFar = 0;
        break;
      default:
        // Don't need to do anything here
        break;
    }
  }

  // Check on the blending type - we support composite, additive, average
  // and min / max intensity
  if (goodSoFar)
  {
    if (this->BlendMode != vtkVolumeMapper::COMPOSITE_BLEND &&
      this->BlendMode != vtkVolumeMapper::MAXIMUM_INTENSITY_BLEND &&
      this->BlendMode != vtkVolumeMapper::MINIMUM_INTENSITY_BLEND &&
      this->BlendMode != vtkVolumeMapper::AVERAGE_INTENSITY_BLEND &&
      this->BlendMode != vtkVolumeMapper::ADDITIVE_BLEND &&
      this->BlendMode != vtkVolumeMapper::ISOSURFACE_BLEND &&
      this->BlendMode != vtkVolumeMapper::SLICE_BLEND)
    {
      goodSoFar = 0;
      vtkErrorMacro(<< "Selected blend mode not supported. "
                    << "Only Composite, MIP, MinIP, averageIP and additive modes "
                    << "are supported by the current implementation.");
    }
  }

  int numberOfComponents = goodSoFar ? scalars->GetNumberOfComponents() : 0;

  // This mapper supports anywhere from 1-4 components. Number of components
  // outside this range is not supported.
  if (goodSoFar)
  {
    if (numberOfComponents <= 0 || numberOfComponents > 4)
    {
      goodSoFar = 0;
      vtkErrorMacro(<< "Only 1 - 4 component scalars "
                    << "are supported by this mapper."
                    << "The input data has " << numberOfComponents << " component(s).");
    }
  }

  // If the dataset has dependent components (as set in the volume property),
  // only 2 or 4 component scalars are supported.
  if (goodSoFar)
  {
    if (!(property->GetIndependentComponents()) &&
      (numberOfComponents == 1 || numberOfComponents == 3))
    {
      goodSoFar = 0;
      vtkErrorMacro(<< "If IndependentComponents is Off in the "
                    << "volume property, then the data must have "
                    << "either 2 or 4 component scalars. "
                    << "The input data has " << numberOfComponents << " component(s).");
    }
  }
  // return our status
  return goodSoFar;
}

// ----------------------------------------------------------------------------
// Description:
// Called by the AMR Volume Mapper.
// Set the flag that tells if the scalars are on point data (0) or
// cell data (1).
void vtkGPUVolumeRayCastMapper::SetCellFlag(int cellFlag)
{
  this->CellFlag = cellFlag;
}

// ----------------------------------------------------------------------------
void vtkGPUVolumeRayCastMapper::CreateCanonicalView(vtkRenderer* ren, vtkVolume* volume,
  vtkImageData* image, int vtkNotUsed(blend_mode), double viewDirection[3], double viewUp[3])
{
  this->GeneratingCanonicalView = 1;
  int oldSwap = ren->GetRenderWindow()->GetSwapBuffers();
  ren->GetRenderWindow()->SwapBuffersOff();

  int dim[3];
  image->GetDimensions(dim);
  const int* size = ren->GetRenderWindow()->GetSize();

  vtkImageData* bigImage = vtkImageData::New();
  bigImage->SetDimensions(size[0], size[1], 1);
  bigImage->AllocateScalars(VTK_UNSIGNED_CHAR, 3);

  this->CanonicalViewImageData = bigImage;

  double scale[2];
  scale[0] = dim[0] / static_cast<double>(size[0]);
  scale[1] = dim[1] / static_cast<double>(size[1]);

  // Save the visibility flags of the renderers and set all to false except
  // for the ren.
  vtkRendererCollection* renderers = ren->GetRenderWindow()->GetRenderers();
  int numberOfRenderers = renderers->GetNumberOfItems();

  bool* rendererVisibilities = new bool[numberOfRenderers];
  renderers->InitTraversal();
  int i = 0;
  while (i < numberOfRenderers)
  {
    vtkRenderer* r = renderers->GetNextItem();
    rendererVisibilities[i] = r->GetDraw() == 1;
    if (r != ren)
    {
      r->SetDraw(false);
    }
    ++i;
  }

  // Save the visibility flags of the props and set all to false except
  // for the volume.

  vtkPropCollection* props = ren->GetViewProps();
  int numberOfProps = props->GetNumberOfItems();

  bool* propVisibilities = new bool[numberOfProps];
  props->InitTraversal();
  i = 0;
  while (i < numberOfProps)
  {
    vtkProp* p = props->GetNextProp();
    propVisibilities[i] = p->GetVisibility() == 1;
    if (p != volume)
    {
      p->SetVisibility(false);
    }
    ++i;
  }

  vtkCamera* savedCamera = ren->GetActiveCamera();
  savedCamera->Modified();
  vtkCamera* canonicalViewCamera = vtkCamera::New();

  // Code from vtkFixedPointVolumeRayCastMapper:
  double* center = volume->GetCenter();
  double bounds[6];
  volume->GetBounds(bounds);
#if 0
  double d=sqrt((bounds[1]-bounds[0])*(bounds[1]-bounds[0]) +
                (bounds[3]-bounds[2])*(bounds[3]-bounds[2]) +
                (bounds[5]-bounds[4])*(bounds[5]-bounds[4]));
#endif

  // For now use x distance - need to change this
  double d = bounds[1] - bounds[0];

  // Set up the camera in parallel
  canonicalViewCamera->SetFocalPoint(center);
  canonicalViewCamera->ParallelProjectionOn();
  canonicalViewCamera->SetPosition(center[0] - d * viewDirection[0],
    center[1] - d * viewDirection[1], center[2] - d * viewDirection[2]);
  canonicalViewCamera->SetViewUp(viewUp);
  canonicalViewCamera->SetParallelScale(d / 2);

  ren->SetActiveCamera(canonicalViewCamera);
  ren->GetRenderWindow()->Render();

  ren->SetActiveCamera(savedCamera);
  canonicalViewCamera->Delete();

  // Shrink to image to the desired size
  vtkImageResample* resample = vtkImageResample::New();
  resample->SetInputData(bigImage);
  resample->SetAxisMagnificationFactor(0, scale[0]);
  resample->SetAxisMagnificationFactor(1, scale[1]);
  resample->SetAxisMagnificationFactor(2, 1);
  resample->UpdateWholeExtent();

  // Copy the pixels over
  image->DeepCopy(resample->GetOutput());

  bigImage->Delete();
  resample->Delete();

  // Restore the visibility flags of the props
  props->InitTraversal();
  i = 0;
  while (i < numberOfProps)
  {
    vtkProp* p = props->GetNextProp();
    p->SetVisibility(propVisibilities[i]);
    ++i;
  }

  delete[] propVisibilities;

  // Restore the visibility flags of the renderers
  renderers->InitTraversal();
  i = 0;
  while (i < numberOfRenderers)
  {
    vtkRenderer* r = renderers->GetNextItem();
    r->SetDraw(rendererVisibilities[i]);
    ++i;
  }

  delete[] rendererVisibilities;

  ren->GetRenderWindow()->SetSwapBuffers(oldSwap);
  this->CanonicalViewImageData = nullptr;
  this->GeneratingCanonicalView = 0;
}

// ----------------------------------------------------------------------------
// Print method for vtkGPUVolumeRayCastMapper
void vtkGPUVolumeRayCastMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "AutoAdjustSampleDistances: " << this->AutoAdjustSampleDistances << endl;
  os << indent << "MinimumImageSampleDistance: " << this->MinimumImageSampleDistance << endl;
  os << indent << "MaximumImageSampleDistance: " << this->MaximumImageSampleDistance << endl;
  os << indent << "ImageSampleDistance: " << this->ImageSampleDistance << endl;
  os << indent << "SampleDistance: " << this->SampleDistance << endl;
  os << indent << "FinalColorWindow: " << this->FinalColorWindow << endl;
  os << indent << "FinalColorLevel: " << this->FinalColorLevel << endl;
  os << indent << "MaskInput: " << this->MaskInput << endl;
  os << indent << "MaskType: " << this->MaskType << endl;
  os << indent << "MaskBlendFactor: " << this->MaskBlendFactor << endl;
  os << indent << "MaxMemoryInBytes: " << this->MaxMemoryInBytes << endl;
  os << indent << "MaxMemoryFraction: " << this->MaxMemoryFraction << endl;
  os << indent << "ReportProgress: " << this->ReportProgress << endl;
}

// ----------------------------------------------------------------------------
// Description:
// Compute the cropping planes clipped by the bounds of the volume.
// The result is put into this->ClippedCroppingRegionPlanes.
// NOTE: IT WILL BE MOVED UP TO vtkVolumeMapper after bullet proof usage
// in this mapper. Other subclasses will use the ClippedCroppingRegionsPlanes
// members instead of CroppingRegionPlanes.
// \pre volume_exists: this->GetInput()!=0
// \pre valid_cropping: this->Cropping &&
//             this->CroppingRegionPlanes[0]<this->CroppingRegionPlanes[1] &&
//             this->CroppingRegionPlanes[2]<this->CroppingRegionPlanes[3] &&
//             this->CroppingRegionPlanes[4]<this->CroppingRegionPlanes[5])
void vtkGPUVolumeRayCastMapper::ClipCroppingRegionPlanes()
{
  assert("pre: volume_exists" && this->GetInput() != nullptr);
  assert("pre: valid_cropping" && this->Cropping &&
    this->CroppingRegionPlanes[0] < this->CroppingRegionPlanes[1] &&
    this->CroppingRegionPlanes[2] < this->CroppingRegionPlanes[3] &&
    this->CroppingRegionPlanes[4] < this->CroppingRegionPlanes[5]);

  // vtkVolumeMapper::Render() will have something like:
  //  if(this->Cropping && (this->CroppingRegionPlanes[0]>=this->CroppingRegionPlanes[1] ||
  //                        this->CroppingRegionPlanes[2]>=this->CroppingRegionPlanes[3] ||
  //                        this->CroppingRegionPlanes[4]>=this->CroppingRegionPlanes[5]))
  //    {
  //    // silently stop because the cropping is not valid.
  //    return;
  //    }

  double volBounds[6];
  this->GetInput()->GetBounds(volBounds);

  int i = 0;
  while (i < 6)
  {
    // max of the mins
    if (this->CroppingRegionPlanes[i] < volBounds[i])
    {
      this->ClippedCroppingRegionPlanes[i] = volBounds[i];
    }
    else
    {
      this->ClippedCroppingRegionPlanes[i] = this->CroppingRegionPlanes[i];
    }
    ++i;
    // min of the maxs
    if (this->CroppingRegionPlanes[i] > volBounds[i])
    {
      this->ClippedCroppingRegionPlanes[i] = volBounds[i];
    }
    else
    {
      this->ClippedCroppingRegionPlanes[i] = this->CroppingRegionPlanes[i];
    }
    ++i;
  }
}

//----------------------------------------------------------------------------
void vtkGPUVolumeRayCastMapper::SetMaskTypeToBinary()
{
  this->MaskType = vtkGPUVolumeRayCastMapper::BinaryMaskType;
}

//----------------------------------------------------------------------------
void vtkGPUVolumeRayCastMapper::SetMaskTypeToLabelMap()
{
  this->MaskType = vtkGPUVolumeRayCastMapper::LabelMapMaskType;
}

//----------------------------------------------------------------------------
vtkContourValues* vtkGPUVolumeRayCastMapper::GetDepthPassContourValues()
{
  if (!this->DepthPassContourValues)
  {
    this->DepthPassContourValues = vtkContourValues::New();
  }

  return this->DepthPassContourValues;
}

//----------------------------------------------------------------------------
void vtkGPUVolumeRayCastMapper::SetDepthImageScalarTypeToUnsignedChar()
{
  this->SetDepthImageScalarType(VTK_UNSIGNED_CHAR);
}

//----------------------------------------------------------------------------
void vtkGPUVolumeRayCastMapper::SetDepthImageScalarTypeToUnsignedShort()
{
  this->SetDepthImageScalarType(VTK_UNSIGNED_SHORT);
}

//----------------------------------------------------------------------------
void vtkGPUVolumeRayCastMapper::SetDepthImageScalarTypeToFloat()
{
  this->SetDepthImageScalarType(VTK_FLOAT);
}

//----------------------------------------------------------------------------
int vtkGPUVolumeRayCastMapper::FillInputPortInformation(int port, vtkInformation* info)
{
  if (port > 0)
  {
    info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), port);
  }

  return this->Superclass::FillInputPortInformation(port, info);
}

//----------------------------------------------------------------------------
vtkImageData* vtkGPUVolumeRayCastMapper::GetInput(const int port)
{
  return static_cast<vtkImageData*>(this->GetInputDataObject(port, 0));
}

//----------------------------------------------------------------------------
void vtkGPUVolumeRayCastMapper::TransformInput(const int port)
{
  vtkImageData* clone = this->TransformedInputs[port];
  clone->ShallowCopy(this->GetInput(port));

  // Get the current extents.
  int extents[6], real_extents[6];
  clone->GetExtent(extents);
  clone->GetExtent(real_extents);

  // Get the current origin and spacing.
  double origin[3], spacing[3];
  clone->GetOrigin(origin);
  clone->GetSpacing(spacing);
  double* direction = clone->GetDirectionMatrix()->GetData();

  // find the location of the min extent
  double blockOrigin[3];
  vtkImageData::TransformContinuousIndexToPhysicalPoint(
    extents[0], extents[2], extents[4], origin, spacing, direction, blockOrigin);

  // make it so that the clone starts with extent 0,0,0
  for (int cc = 0; cc < 3; cc++)
  {
    // Transform the origin and the extents.
    origin[cc] = blockOrigin[cc];
    extents[2 * cc + 1] -= extents[2 * cc];
    extents[2 * cc] = 0;
  }

  clone->SetOrigin(origin);
  clone->SetExtent(extents);
}

vtkImageData* vtkGPUVolumeRayCastMapper::GetTransformedInput(const int port)
{
  const auto data = this->FindData(port, this->TransformedInputs);
  return data;
}

void vtkGPUVolumeRayCastMapper::SetInputConnection(int port, vtkAlgorithmOutput* input)
{
  Superclass::SetInputConnection(port, input);
  const auto it = std::find(this->Ports.begin(), this->Ports.end(), port);
  if (it == this->Ports.cend())
  {
    this->Ports.push_back(port);
  }
  this->Modified();
}

void vtkGPUVolumeRayCastMapper::RemoveInputConnection(int port, vtkAlgorithmOutput* input)
{
  Superclass::RemoveInputConnection(port, input);
  this->RemovePortInternal(port);
}

void vtkGPUVolumeRayCastMapper::RemoveInputConnection(int port, int idx)
{
  Superclass::RemoveInputConnection(port, idx);
  this->RemovePortInternal(port);
}

void vtkGPUVolumeRayCastMapper::RemovePortInternal(const int port)
{
  const auto it = std::find(this->Ports.begin(), this->Ports.end(), port);
  if (it != this->Ports.end())
  {
    this->Ports.erase(it);
  }
  this->RemovedPorts.push_back(port);
  this->Modified();
}

double* vtkGPUVolumeRayCastMapper::GetBoundsFromPort(const int port)
{
  this->CloneInputs();

  // Use bounds of a specific input
  auto it = this->TransformedInputs.find(port);
  if (it == this->TransformedInputs.cend())
  {
    vtkAbstractVolumeMapper::GetDataSetInput()->GetBounds(this->Bounds);
    return this->Bounds;
  }

  return it->second->GetBounds();
}

int vtkGPUVolumeRayCastMapper::GetInputCount()
{
  return static_cast<int>(this->Ports.size());
}
