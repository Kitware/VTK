// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkWebGPUVolumeRayCastCoordinator.h"

#include "Private/vtkWebGPUVolumeRayCastCoordinatorInternals.h"

#include "vtkCallbackCommand.h"
#include "vtkCommand.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkWebGPURenderWindow.h"
#include "vtkWebGPURenderer.h"

#include <map>

VTK_ABI_NAMESPACE_BEGIN

vtkStandardNewMacro(vtkWebGPUVolumeRayCastCoordinator);

namespace
{
struct Registration
{
  vtkSmartPointer<vtkWebGPUVolumeRayCastCoordinator> Coordinator;
  unsigned long ObserverTag = 0;
};

// One coordinator per renderer. Kept here rather than on the renderer because
// VTK::RenderingWebGPU knows nothing about volume mappers.
std::map<vtkRenderer*, Registration>& Registry()
{
  static std::map<vtkRenderer*, Registration> registry;
  return registry;
}

void OnRendererDeleted(vtkObject* caller, unsigned long, void*, void*)
{
  Registry().erase(static_cast<vtkRenderer*>(caller));
}
} // anonymous namespace

//------------------------------------------------------------------------------
vtkWebGPUVolumeRayCastCoordinator::vtkWebGPUVolumeRayCastCoordinator()
  : Internals(new vtkInternals(this))
{
}

//------------------------------------------------------------------------------
vtkWebGPUVolumeRayCastCoordinator::~vtkWebGPUVolumeRayCastCoordinator() = default;

//------------------------------------------------------------------------------
void vtkWebGPUVolumeRayCastCoordinator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "NumberOfVolumes: " << this->NumberOfVolumes << "\n";
}

//------------------------------------------------------------------------------
vtkWebGPUVolumeRayCastCoordinator* vtkWebGPUVolumeRayCastCoordinator::GetForRenderer(
  vtkRenderer* renderer)
{
  if (vtkWebGPURenderer::SafeDownCast(renderer) == nullptr)
  {
    return nullptr;
  }

  auto& registry = Registry();
  auto it = registry.find(renderer);
  if (it != registry.end())
  {
    return it->second.Coordinator;
  }

  Registration registration;
  registration.Coordinator = vtkSmartPointer<vtkWebGPUVolumeRayCastCoordinator>::New();

  vtkNew<vtkCallbackCommand> onDeleted;
  onDeleted->SetCallback(OnRendererDeleted);
  registration.ObserverTag = renderer->AddObserver(vtkCommand::DeleteEvent, onDeleted);

  return registry.emplace(renderer, registration).first->second.Coordinator;
}

//------------------------------------------------------------------------------
void vtkWebGPUVolumeRayCastCoordinator::ReleaseAll()
{
  Registry().clear();
}

//------------------------------------------------------------------------------
void vtkWebGPUVolumeRayCastCoordinator::Render(vtkRenderer* renderer)
{
  auto* window = vtkWebGPURenderWindow::SafeDownCast(renderer->GetRenderWindow());
  if (window == nullptr)
  {
    return;
  }

  this->NumberOfVolumes = static_cast<int>(this->Internals->GetNumberOfContributions());
  this->Internals->Render(window, renderer);
}

//------------------------------------------------------------------------------
void vtkWebGPUVolumeRayCastCoordinator::ReleaseGraphicsResources(
  vtkWebGPUVolumeRayCastMapper* mapper)
{
  if (mapper == nullptr)
  {
    this->Internals->ReleaseGraphicsResources();
  }
  else
  {
    this->Internals->Forget(mapper);
  }
}

VTK_ABI_NAMESPACE_END
