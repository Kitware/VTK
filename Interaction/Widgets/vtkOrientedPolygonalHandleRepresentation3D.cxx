/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOrientedPolygonalHandleRepresentation3D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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

vtkStandardNewMacro(vtkOrientedPolygonalHandleRepresentation3D);

//----------------------------------------------------------------------
vtkOrientedPolygonalHandleRepresentation3D ::vtkOrientedPolygonalHandleRepresentation3D()
{
  this->Actor = vtkFollower::New();
  this->Actor->SetMapper(this->Mapper);
  this->Actor->SetProperty(this->Property);
  this->HandlePicker->AddPickList(this->Actor);
}

//----------------------------------------------------------------------
vtkOrientedPolygonalHandleRepresentation3D ::~vtkOrientedPolygonalHandleRepresentation3D() =
  default;

//----------------------------------------------------------------------
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

//----------------------------------------------------------------------
void vtkOrientedPolygonalHandleRepresentation3D::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
