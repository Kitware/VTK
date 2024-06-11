// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkRadialGridActor2D.h"

#include "vtkAxisActor2D.h"
#include "vtkMath.h"
#include "vtkNumberToString.h"
#include "vtkObjectFactory.h"
#include "vtkPropCollection.h"
#include "vtkTextProperty.h"
#include "vtkViewport.h"

#include <cmath>

VTK_ABI_NAMESPACE_BEGIN

vtkStandardNewMacro(vtkRadialGridActor2D);

vtkCxxSetSmartPointerMacro(vtkRadialGridActor2D, TextProperty, vtkTextProperty);

//------------------------------------------------------------------------------
vtkRadialGridActor2D::vtkRadialGridActor2D()
{
  this->TextProperty = vtkSmartPointer<vtkTextProperty>::New();
}

//------------------------------------------------------------------------------
vtkRadialGridActor2D::~vtkRadialGridActor2D() = default;

//------------------------------------------------------------------------------
void vtkRadialGridActor2D::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "NumberOfAxes: " << this->NumberOfAxes << "\n";
  os << indent << "StartAngle: " << this->StartAngle << "\n";
  os << indent << "EndAngle: " << this->EndAngle << "\n";
  os << indent << "Origin: " << this->Origin[0] << " " << this->Origin[1] << "\n";
  os << indent << "AxesViewportLength: " << this->AxesViewportLength << "\n";
  os << indent << "NumberOfTicks: " << this->NumberOfTicks << "\n";
  os << indent << "TextProperty:\n";
  if (this->TextProperty)
  {
    this->TextProperty->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << "(None)\n";
  }
}

//------------------------------------------------------------------------------
int vtkRadialGridActor2D::RenderOverlay(vtkViewport* viewport)
{
  this->SetupAxes(viewport);
  for (const auto& axis : this->Axes)
  {
    axis->RenderOverlay(viewport);
  }

  return 1;
}

//------------------------------------------------------------------------------
void vtkRadialGridActor2D::GetActors2D(vtkPropCollection* pc)
{
  for (vtkAxisActor2D* axis : this->Axes)
  {
    pc->AddItem(axis);
  }
}

//------------------------------------------------------------------------------
double vtkRadialGridActor2D::GetAxisAngle(int index)
{
  if (this->NumberOfAxes < 2)
  {
    return 0;
  }
  return (this->EndAngle - this->StartAngle) * index / (this->NumberOfAxes - 1) + this->StartAngle;
}

//------------------------------------------------------------------------------
void vtkRadialGridActor2D::ComputeAxisRelativeEndPosition(int index, double position[2])
{
  const double axesAngle = this->GetAxisAngle(index);
  double endX = std::cos(vtkMath::RadiansFromDegrees(axesAngle));
  double endY = std::sin(vtkMath::RadiansFromDegrees(axesAngle));

  position[0] = this->AxesViewportLength * endX;
  position[1] = this->AxesViewportLength * endY;
}

//------------------------------------------------------------------------------
void vtkRadialGridActor2D::ComputeAxisWorldRange(
  vtkViewport* viewport, vtkAxisActor2D* axis, double worldRange[2])
{
  double* startPos = axis->GetPositionCoordinate()->GetComputedWorldValue(viewport);
  double* endPos = axis->GetPosition2Coordinate()->GetComputedWorldValue(viewport);

  auto distance = vtkMath::Distance2BetweenPoints(startPos, endPos);
  worldRange[0] = 0;
  worldRange[1] = std::sqrt(distance);
}

//------------------------------------------------------------------------------
void vtkRadialGridActor2D::UpdateAxisTitle(vtkAxisActor2D* axis, double angle)
{
  vtkNumberToString converter;
  converter.SetNotation(vtkNumberToString::Fixed);
  converter.SetPrecision(0);
  std::string formattedString = converter.Convert(angle);
  std::string title = formattedString + " deg";

  axis->SetTitle(title.c_str());

  // keep text angle between -90 and 90 degrees for readability
  double textAngle = std::fmod(angle + 90, 180) - 90;

  vtkTextProperty* textProperty = axis->GetTitleTextProperty();
  textProperty->SetOrientation(textAngle);
}

//------------------------------------------------------------------------------
void vtkRadialGridActor2D::SetupAxes(vtkViewport* viewport)
{
  this->Axes.clear();

  for (int index = 0; index < this->NumberOfAxes; index++)
  {
    auto axis = vtkSmartPointer<vtkAxisActor2D>::New();
    this->Axes.push_back(axis);
    axis->AdjustLabelsOff();
    axis->SnapLabelsToGridOn();
    axis->SetNumberOfLabels(this->NumberOfTicks);
    axis->SetLabelVisibility(index == 0);
    axis->SkipFirstTickOn();

    axis->SetUseFontSizeFromProperty(true);
    axis->SetProperty(this->GetProperty());
    axis->SetLabelTextProperty(this->GetTextProperty());
    vtkNew<vtkTextProperty> titleProp;
    titleProp->ShallowCopy(this->GetTextProperty());
    axis->SetTitleTextProperty(titleProp);

    axis->GetPositionCoordinate()->SetCoordinateSystemToNormalizedViewport();
    axis->GetPositionCoordinate()->SetValue(this->Origin[0], this->Origin[1]);

    axis->GetPosition2Coordinate()->SetCoordinateSystemToViewport();
    axis->GetPosition2Coordinate()->SetReferenceCoordinate(axis->GetPositionCoordinate());

    double endPosition[2] = { 0, 0 };
    const double angle = this->GetAxisAngle(index);
    this->ComputeAxisRelativeEndPosition(index, endPosition);
    axis->GetPosition2Coordinate()->SetValue(endPosition[0], endPosition[1]);

    this->UpdateAxisTitle(axis, angle);

    double worldRange[2];
    this->ComputeAxisWorldRange(viewport, axis, worldRange);
    axis->SetRange(worldRange);
  }
}

//------------------------------------------------------------------------------
vtkPoints* vtkRadialGridActor2D::GetFirstAxesPoints()
{
  if (this->Axes.empty())
  {
    return nullptr;
  }

  return this->Axes.front()->GetTickPositions();
}

//------------------------------------------------------------------------------
vtkPoints* vtkRadialGridActor2D::GetLastAxesPoints()
{
  if (this->Axes.empty())
  {
    return nullptr;
  }

  return this->Axes.back()->GetTickPositions();
}

//------------------------------------------------------------------------------
vtkTextProperty* vtkRadialGridActor2D::GetTextProperty()
{
  return this->TextProperty;
}

VTK_ABI_NAMESPACE_END
