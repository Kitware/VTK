// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkPolarAxesActor2D.h"

#include "Private/vtkArcGridActorInternal.h"
#include "vtkObjectFactory.h"
#include "vtkPolyDataMapper2D.h"
#include "vtkPropCollection.h"
#include "vtkProperty2D.h"
#include "vtkRadialGridActor2D.h"
#include "vtkViewport.h"

VTK_ABI_NAMESPACE_BEGIN

vtkStandardNewMacro(vtkPolarAxesActor2D);

//------------------------------------------------------------------------------
vtkPolarAxesActor2D::vtkPolarAxesActor2D()
{
  this->PositionCoordinate->SetCoordinateSystemToNormalizedViewport();
  this->PositionCoordinate->SetValue(0., 0.);
  this->Position2Coordinate->SetCoordinateSystemToNormalizedViewport();
  this->Position2Coordinate->SetValue(1., 1.);
  this->Position2Coordinate->SetReferenceCoordinate(nullptr);
  this->SetNumberOfAxes(6);
  this->SetOrigin(0.5, 0.5);
}

//------------------------------------------------------------------------------
void vtkPolarAxesActor2D::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "RadialGrid: \n";
  this->RadialGrid->PrintSelf(os, indent.GetNextIndent());
  os << indent << "ArcGrid: \n";
  this->ArcGrid->PrintSelf(os, indent.GetNextIndent());
}

//------------------------------------------------------------------------------
void vtkPolarAxesActor2D::GetActors2D(vtkPropCollection* pc)
{
  pc->AddItem(this->RadialGrid);
  pc->AddItem(this->ArcGrid);
}

//------------------------------------------------------------------------------
int vtkPolarAxesActor2D::RenderOverlay(vtkViewport* viewport)
{
  int renderedActors = 0;
  this->RadialGrid->SetProperty(this->GetProperty());
  renderedActors += this->RadialGrid->RenderOverlay(viewport);

  this->ArcGrid->SetProperty(this->GetProperty());
  this->ArcGrid->SetTicksStart(this->RadialGrid->GetFirstAxesPoints());
  this->ArcGrid->SetAngle(this->RadialGrid->GetEndAngle() - this->RadialGrid->GetStartAngle());

  renderedActors += this->ArcGrid->RenderOverlay(viewport);
  return renderedActors;
}

//------------------------------------------------------------------------------
void vtkPolarAxesActor2D::SetNumberOfAxes(int number)
{
  this->RadialGrid->SetNumberOfAxes(number);
  this->ArcGrid->SetResolution(number * 3 + 1);
}

//------------------------------------------------------------------------------
int vtkPolarAxesActor2D::GetNumberOfAxes()
{
  return this->RadialGrid->GetNumberOfAxes();
}

//------------------------------------------------------------------------------
void vtkPolarAxesActor2D::SetNumberOfAxesTicks(int number)
{
  this->RadialGrid->SetNumberOfTicks(number);
}

//------------------------------------------------------------------------------
int vtkPolarAxesActor2D::GetNumberOfAxesTicks()
{
  return this->RadialGrid->GetNumberOfTicks();
}

//------------------------------------------------------------------------------
void vtkPolarAxesActor2D::SetStartAngle(double angle)
{
  this->RadialGrid->SetStartAngle(angle);
}

//------------------------------------------------------------------------------
double vtkPolarAxesActor2D::GetStartAngle()
{
  return this->RadialGrid->GetStartAngle();
}

//------------------------------------------------------------------------------
void vtkPolarAxesActor2D::SetEndAngle(double angle)
{
  this->RadialGrid->SetEndAngle(angle);
}

//------------------------------------------------------------------------------
double vtkPolarAxesActor2D::GetEndAngle()
{
  return this->RadialGrid->GetEndAngle();
}

//------------------------------------------------------------------------------
void vtkPolarAxesActor2D::SetOrigin(double x, double y)
{
  this->RadialGrid->SetOrigin(x, y);
  this->ArcGrid->SetCenter(x, y);
}

//------------------------------------------------------------------------------
void vtkPolarAxesActor2D::GetOrigin(double origin[2])
{
  this->RadialGrid->GetOrigin(origin);
}

//------------------------------------------------------------------------------
void vtkPolarAxesActor2D::SetAxesLength(double length)
{
  this->RadialGrid->SetAxesViewportLength(length);
}

//------------------------------------------------------------------------------
double vtkPolarAxesActor2D::GetAxesLength()
{
  return this->RadialGrid->GetAxesViewportLength();
}

//------------------------------------------------------------------------------
vtkTextProperty* vtkPolarAxesActor2D::GetAxesTextProperty()
{
  return this->RadialGrid->GetTextProperty();
}

//------------------------------------------------------------------------------
void vtkPolarAxesActor2D::SetAxesTextProperty(vtkTextProperty* property)
{
  this->RadialGrid->SetTextProperty(property);
}

VTK_ABI_NAMESPACE_END
