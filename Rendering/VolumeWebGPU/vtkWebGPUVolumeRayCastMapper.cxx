// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkWebGPUVolumeRayCastMapper.h"

#include "Private/vtkWebGPUVolumeRayCastMapperInternals.h"
#include "vtkWebGPUVolumeRayCastCoordinator.h"

#include "vtkCommand.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkMultiVolume.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkOverrideAttribute.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkVolumeProperty.h"
#include "vtkWebGPURenderWindow.h"
#include "vtkWebGPURenderer.h"

#include <algorithm>
#include <chrono>

VTK_ABI_NAMESPACE_BEGIN

vtkStandardNewMacro(vtkWebGPUVolumeRayCastMapper);

//------------------------------------------------------------------------------
vtkOverrideAttribute* vtkWebGPUVolumeRayCastMapper::CreateOverrideAttributes()
{
  return vtkOverrideAttribute::CreateAttributeChain("RenderingBackend", "WebGPU", nullptr);
}

//------------------------------------------------------------------------------
vtkWebGPUVolumeRayCastMapper::vtkWebGPUVolumeRayCastMapper()
  : Internals(new vtkInternals(this))
{
}

//------------------------------------------------------------------------------
vtkWebGPUVolumeRayCastMapper::~vtkWebGPUVolumeRayCastMapper() = default;

//------------------------------------------------------------------------------
void vtkWebGPUVolumeRayCastMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "AdaptiveSampling: " << this->AdaptiveSampling << "\n";
  os << indent << "EmptySpaceSkipping: " << this->EmptySpaceSkipping << "\n";
  os << indent << "BrickSize: " << this->BrickSize << "\n";
  os << indent << "MinimumSampleDistanceFactor: " << this->MinimumSampleDistanceFactor << "\n";
  os << indent << "MaximumSampleDistanceFactor: " << this->MaximumSampleDistanceFactor << "\n";
  os << indent << "EmptySpaceOpacityThreshold: " << this->EmptySpaceOpacityThreshold << "\n";
  os << indent << "InteractiveMaximumNumberOfSamples: " << this->InteractiveMaximumNumberOfSamples
     << "\n";
}

//------------------------------------------------------------------------------
int vtkWebGPUVolumeRayCastMapper::IsRenderSupported(
  vtkRenderWindow* window, vtkVolumeProperty* vtkNotUsed(property))
{
  return vtkWebGPURenderWindow::SafeDownCast(window) != nullptr ? 1 : 0;
}

//------------------------------------------------------------------------------
void vtkWebGPUVolumeRayCastMapper::ReleaseGraphicsResources(vtkWindow* vtkNotUsed(window))
{
  this->Internals->ReleaseGraphicsResources();
}

//------------------------------------------------------------------------------
void vtkWebGPUVolumeRayCastMapper::ReleaseGraphicsResources(vtkRenderer* renderer)
{
  this->Internals->ReleaseGraphicsResources();
  if (auto* coordinator = vtkWebGPUVolumeRayCastCoordinator::GetForRenderer(renderer))
  {
    coordinator->ReleaseGraphicsResources(this);
  }
}

//------------------------------------------------------------------------------
void vtkWebGPUVolumeRayCastMapper::GetReductionRatio(double ratio[3])
{
  ratio[0] = ratio[1] = ratio[2] = 1.0;
}

//------------------------------------------------------------------------------
void vtkWebGPUVolumeRayCastMapper::SetVolumeProperty(int port, vtkVolumeProperty* property)
{
  auto& current = this->Internals->PortProperties[port];
  if (current == property)
  {
    return;
  }
  current = property;
  this->Modified();
}

//------------------------------------------------------------------------------
vtkVolumeProperty* vtkWebGPUVolumeRayCastMapper::GetVolumeProperty(int port)
{
  auto it = this->Internals->PortProperties.find(port);
  return it == this->Internals->PortProperties.end() ? nullptr : it->second;
}

//------------------------------------------------------------------------------
double* vtkWebGPUVolumeRayCastMapper::GetBounds()
{
  const std::vector<int> ports = this->GetPorts();
  // A streamed input answers from its pipeline information: asking its data would read it whole.
  bool streaming = false;
  for (int port : ports)
  {
    streaming |= this->Internals->UpdateStream(port);
  }
  if (ports.size() < 2 && !streaming)
  {
    return this->Superclass::GetBounds();
  }

  vtkMath::UninitializeBounds(this->Bounds);
  bool first = true;
  for (int port : ports)
  {
    double bounds[6];
    const auto stream = this->Internals->Streams.find(port);
    if (stream != this->Internals->Streams.end() && stream->second.Active)
    {
      stream->second.Shell->GetBounds(bounds);
    }
    else
    {
      std::copy_n(this->GetBoundsFromPort(port), 6, bounds);
    }
    if (bounds[1] < bounds[0])
    {
      continue;
    }
    for (int i = 0; i < 3; ++i)
    {
      this->Bounds[2 * i] = first ? bounds[2 * i] : std::min(this->Bounds[2 * i], bounds[2 * i]);
      this->Bounds[2 * i + 1] =
        first ? bounds[2 * i + 1] : std::max(this->Bounds[2 * i + 1], bounds[2 * i + 1]);
    }
    first = false;
  }
  return this->Bounds;
}

//------------------------------------------------------------------------------
namespace
{
bool IsEmptyExtent(const int* extent)
{
  return extent[0] > extent[1] || extent[2] > extent[3] || extent[4] > extent[5];
}

// Ask for the whole of an input whose request was left empty by an earlier frame that streamed
// it. A request that was set on purpose is left alone.
void RestoreWholeExtent(vtkInformation* info)
{
  if (info == nullptr || !info->Has(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT()) ||
    !info->Has(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()) ||
    !IsEmptyExtent(info->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT())))
  {
    return;
  }
  info->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
    info->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()), 6);
}
}

//------------------------------------------------------------------------------
vtkTypeBool vtkWebGPUVolumeRayCastMapper::ProcessRequest(
  vtkInformation* request, vtkInformationVector** inInfo, vtkInformationVector* outInfo)
{
  const vtkTypeBool result = this->Superclass::ProcessRequest(request, inInfo, outInfo);
  if (!request->Has(vtkStreamingDemandDrivenPipeline::REQUEST_UPDATE_EXTENT()))
  {
    return result;
  }
  static const int nothing[6] = { 0, -1, 0, -1, 0, -1 };
  for (int port : this->GetPorts())
  {
    if (port >= this->GetNumberOfInputPorts())
    {
      continue;
    }
    vtkInformation* info = inInfo[port]->GetInformationObject(0);
    if (this->Internals->WantsStreaming(port, info))
    {
      info->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), nothing, 6);
    }
    else
    {
      RestoreWholeExtent(info);
    }
  }
  return result;
}

//------------------------------------------------------------------------------
bool vtkWebGPUVolumeRayCastMapper::IsStreaming(int port)
{
  return this->Internals->UpdateStream(port);
}

//------------------------------------------------------------------------------
void vtkWebGPUVolumeRayCastMapper::Render(vtkRenderer* ren, vtkVolume* vol)
{
  bool streaming = false;
  for (int port : this->GetPorts())
  {
    if (this->Internals->UpdateStream(port))
    {
      streaming = true;
    }
    else if (this->GetNumberOfInputConnections(port) > 0)
    {
      // This frame's update may have asked it for nothing, before it turned out not to stream.
      RestoreWholeExtent(this->GetInputInformation(port, 0));
    }
  }
  if (!streaming || this->GeneratingCanonicalView)
  {
    this->Superclass::Render(ren, vol);
    return;
  }

  // What vtkGPUVolumeRayCastMapper::Render does, except that a streamed input is not updated:
  // updating it would read the whole volume, and its bricks are read as the coordinator asks.
  this->InvokeEvent(vtkCommand::VolumeMapperRenderStartEvent, nullptr);
  const auto start = std::chrono::steady_clock::now();

  bool valid = ren != nullptr && vol != nullptr;
  if (valid && this->Cropping &&
    (this->CroppingRegionPlanes[0] >= this->CroppingRegionPlanes[1] ||
      this->CroppingRegionPlanes[2] >= this->CroppingRegionPlanes[3] ||
      this->CroppingRegionPlanes[4] >= this->CroppingRegionPlanes[5]))
  {
    valid = false;
  }
  if (valid)
  {
    auto* multiVolume = vtkMultiVolume::SafeDownCast(vol);
    for (int port : this->GetPorts())
    {
      const auto stream = this->Internals->Streams.find(port);
      if (stream != this->Internals->Streams.end() && stream->second.Active)
      {
        continue;
      }
      vtkVolume* current = multiVolume != nullptr ? multiVolume->GetVolume(port) : vol;
      valid &= this->ValidateInput(current->GetProperty(), port) == 1;
    }
  }
  if (valid)
  {
    this->GPURender(ren, vol);
  }

  this->TimeToDraw =
    std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
  if (vol != nullptr && vol->GetAllocatedRenderTime() < 1.0)
  {
    this->SmallTimeToDraw = this->TimeToDraw;
  }
  else
  {
    this->BigTimeToDraw = this->TimeToDraw;
  }
  this->InvokeEvent(vtkCommand::VolumeMapperRenderEndEvent, nullptr);
}

//------------------------------------------------------------------------------
void vtkWebGPUVolumeRayCastMapper::GetColorImage(vtkImageData* output)
{
  if (auto* coordinator = vtkWebGPUVolumeRayCastCoordinator::GetForRenderer(this->LastRenderer))
  {
    coordinator->GetImageColor(output);
  }
}

//------------------------------------------------------------------------------
void vtkWebGPUVolumeRayCastMapper::GetDepthImage(vtkImageData* output)
{
  if (auto* coordinator = vtkWebGPUVolumeRayCastCoordinator::GetForRenderer(this->LastRenderer))
  {
    coordinator->GetImageDepth(this->GetDepthImageScalarType(), output);
  }
}

//------------------------------------------------------------------------------
void vtkWebGPUVolumeRayCastMapper::GPURender(vtkRenderer* ren, vtkVolume* vol)
{
  this->LastRenderer = ren;
  auto* window = vtkWebGPURenderWindow::SafeDownCast(ren->GetRenderWindow());
  auto* renderer = vtkWebGPURenderer::SafeDownCast(ren);
  if (window == nullptr || renderer == nullptr)
  {
    vtkErrorMacro(<< "A WebGPU render window and renderer are required.");
    return;
  }

  // The volumes of a renderer are marched together, so the mapper prepares its own inputs and
  // hands them over rather than drawing them itself.
  auto* coordinator = vtkWebGPUVolumeRayCastCoordinator::GetForRenderer(ren);

  switch (renderer->GetRenderStage())
  {
    case vtkWebGPURenderer::RenderStageEnum::SyncDeviceResources:
      this->Internals->SyncDeviceResources(window, coordinator, vol);
      break;

    case vtkWebGPURenderer::RenderStageEnum::RecordingCommands:
      // The ray march reads the depth the opaque geometry left behind, which is still being
      // written into at this point, so draw once that pass has been flushed.
      renderer->AddPostRasterizationProp(vol);
      break;

    case vtkWebGPURenderer::RenderStageEnum::RenderPostRasterization:
      // Whichever volume gets here first draws them all; the rest find the work already done.
      if (coordinator != nullptr)
      {
        coordinator->Render(ren);
      }
      break;

    default:
      break;
  }
}

//------------------------------------------------------------------------------
void vtkWebGPUVolumeRayCastMapper::PreRender(vtkRenderer* vtkNotUsed(ren),
  vtkVolume* vtkNotUsed(vol), double vtkNotUsed(datasetBounds)[6],
  double vtkNotUsed(scalarRange)[2], int vtkNotUsed(numberOfScalarComponents),
  unsigned int vtkNotUsed(numberOfLevels))
{
}

//------------------------------------------------------------------------------
void vtkWebGPUVolumeRayCastMapper::RenderBlock(
  vtkRenderer* vtkNotUsed(ren), vtkVolume* vtkNotUsed(vol), unsigned int vtkNotUsed(level))
{
}

//------------------------------------------------------------------------------
void vtkWebGPUVolumeRayCastMapper::PostRender(
  vtkRenderer* vtkNotUsed(ren), int vtkNotUsed(numberOfScalarComponents))
{
}

VTK_ABI_NAMESPACE_END
