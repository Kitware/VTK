// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkWebGPUVolumeRayCastMapper.h"

#include "Private/vtkWebGPUVolumeRayCastMapperInternals.h"
#include "vtkWebGPUVolumeRayCastCoordinator.h"

#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkOverrideAttribute.h"
#include "vtkVolumeProperty.h"
#include "vtkWebGPURenderWindow.h"
#include "vtkWebGPURenderer.h"

#include <algorithm>

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
  if (ports.size() < 2)
  {
    return this->Superclass::GetBounds();
  }

  vtkMath::UninitializeBounds(this->Bounds);
  bool first = true;
  for (int port : ports)
  {
    double bounds[6];
    std::copy_n(this->GetBoundsFromPort(port), 6, bounds);
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
