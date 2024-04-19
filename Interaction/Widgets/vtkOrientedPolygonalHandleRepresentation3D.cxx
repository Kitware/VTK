// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkOrientedPolygonalHandleRepresentation3D.h"
#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkCellPicker.h"
#include "vtkFollower.h"
#include "vtkObjectFactory.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkTransformPolyDataFilter.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkOrientedPolygonalHandleRepresentation3D);

//------------------------------------------------------------------------------
vtkOrientedPolygonalHandleRepresentation3D ::vtkOrientedPolygonalHandleRepresentation3D()
{
  this->Actor = vtkFollower::New();
  this->Actor->SetMapper(this->Mapper);
  this->Actor->SetProperty(this->Property);
  this->HandlePicker->AddPickList(this->Actor);
}

//------------------------------------------------------------------------------
vtkOrientedPolygonalHandleRepresentation3D ::~vtkOrientedPolygonalHandleRepresentation3D() =
  default;

//------------------------------------------------------------------------------
void vtkOrientedPolygonalHandleRepresentation3D::UpdateHandle()
{
  this->Superclass::UpdateHandle();

  // Our handle actor is a follower. It follows the camera set on it.
  if (this->Renderer)
  {
    vtkFollower* follower = vtkFollower::SafeDownCast(this->Actor);
    if (follower)
    {
      follower->SetCamera(this->Renderer->GetActiveCamera());
    }
  }

  // Update the actor position
  double handlePosition[3];
  this->GetWorldPosition(handlePosition);
  this->Actor->SetPosition(handlePosition);
}

//------------------------------------------------------------------------------
void vtkOrientedPolygonalHandleRepresentation3D::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
