// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkOpenXRSceneComponent.h"

#include "vtkMatrix4x4.h"
#include "vtkObjectFactory.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkOpenXRSceneComponent);

namespace
{

struct MarkerInfo
{
  double Width{ 0.0 };
  double Height{ 0.0 };
  std::string Text{};
};

}

struct vtkOpenXRSceneComponent::vtkInternals
{
  vtkSmartPointer<vtkObject> Representation{};
  MarkerInfo Marker{}; // additional info if Type is Marker
};

//------------------------------------------------------------------------------
vtkOpenXRSceneComponent::vtkOpenXRSceneComponent() = default;

//------------------------------------------------------------------------------
vtkOpenXRSceneComponent::~vtkOpenXRSceneComponent() = default;

//------------------------------------------------------------------------------
vtkMatrix4x4* vtkOpenXRSceneComponent::GetMarkerPose() const
{
  if (this->Type != Marker)
  {
    return nullptr;
  }

  return static_cast<vtkMatrix4x4*>(this->Impl->Representation.Get());
}

//------------------------------------------------------------------------------
const std::string& vtkOpenXRSceneComponent::GetMarkerText() const
{
  return this->Impl->Marker.Text;
}

//------------------------------------------------------------------------------
double vtkOpenXRSceneComponent::GetMarkerWidth() const
{
  return this->Impl->Marker.Width;
}

//------------------------------------------------------------------------------
double vtkOpenXRSceneComponent::GetMarkerHeight() const
{
  return this->Impl->Marker.Height;
}

//------------------------------------------------------------------------------
void vtkOpenXRSceneComponent::Initialize(ComponentTypes type)
{
  this->Impl.reset(new vtkInternals{});

  switch (type)
  {
    case Marker:
      this->Impl->Representation = vtkSmartPointer<vtkMatrix4x4>::New();
      break;
    default:
      vtkErrorMacro("Wrong ComponentTypes value");
      return;
  }

  this->Type = type;
}

//------------------------------------------------------------------------------
void vtkOpenXRSceneComponent::UpdateMarkerRepresentation(
  int64_t lastModifiedTime, vtkMatrix4x4* matrix, double width, double height, std::string text)
{
  if (this->Type != Marker)
  {
    vtkErrorMacro("UpdateMarkerRepresentation called on non-marker component");
    return;
  }

  this->LastModifiedTime = lastModifiedTime;
  this->Impl->Marker.Width = width;
  this->Impl->Marker.Height = height;
  this->Impl->Marker.Text = std::move(text);
  // Event order is:
  // Representation -> Component -> Scene (if new)
  this->GetMarkerPose()->DeepCopy(matrix);
  this->Modified();
}

VTK_ABI_NAMESPACE_END
