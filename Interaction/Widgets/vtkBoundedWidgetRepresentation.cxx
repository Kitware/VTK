// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkBoundedWidgetRepresentation.h"

#include "vtkActor.h"
#include "vtkBoundingBox.h"
#include "vtkImageData.h"
#include "vtkOutlineFilter.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkTransform.h"

#include <limits>

VTK_ABI_NAMESPACE_BEGIN

//------------------------------------------------------------------------------
vtkBoundedWidgetRepresentation::~vtkBoundedWidgetRepresentation() = default;

//------------------------------------------------------------------------------
vtkBoundedWidgetRepresentation::vtkBoundedWidgetRepresentation()
{
  this->Box->SetDimensions(2, 2, 2);
  this->Outline->SetInputData(this->Box);
  this->OutlineMapper->SetInputConnection(this->Outline->GetOutputPort());
  this->OutlineActor->SetMapper(this->OutlineMapper);
  this->OutlineActor->SetProperty(this->OutlineProperty);
}

//------------------------------------------------------------------------------
void vtkBoundedWidgetRepresentation::HighlightOutline(int highlight)
{
  if (highlight)
  {
    this->OutlineActor->SetProperty(this->SelectedOutlineProperty);
  }
  else
  {
    this->OutlineActor->SetProperty(this->OutlineProperty);
  }
}

//------------------------------------------------------------------------------
void vtkBoundedWidgetRepresentation::TranslateOutline(double* p1, double* p2)
{
  vtkVector3d motion = { 0, 0, 0 };

  if (!this->IsTranslationConstrained())
  {
    motion[0] = p2[0] - p1[0];
    motion[1] = p2[1] - p1[1];
    motion[2] = p2[2] - p1[2];
  }
  else
  {
    assert(this->TranslationAxis > -1 && this->TranslationAxis < 3 &&
      "this->TranslationAxis out of bounds");
    motion[this->TranslationAxis] = p2[this->TranslationAxis] - p1[this->TranslationAxis];
  }

  // Translate the bounding box
  double* origin = this->Box->GetOrigin();
  double oNew[3];
  oNew[0] = origin[0] + motion[0];
  oNew[1] = origin[1] + motion[1];
  oNew[2] = origin[2] + motion[2];
  this->Box->SetOrigin(oNew);

  this->UpdateWidgetBounds();

  this->TranslateRepresentation(motion);

  this->BuildRepresentation();
}

//------------------------------------------------------------------------------
double vtkBoundedWidgetRepresentation::GetDiagonalLength()
{
  return this->Outline->GetOutput()->GetLength();
}

//------------------------------------------------------------------------------
void vtkBoundedWidgetRepresentation::GetOutlineBounds(double bounds[6])
{
  this->Outline->GetOutput()->GetBounds(bounds);
}

//------------------------------------------------------------------------------
void vtkBoundedWidgetRepresentation::SetOutlineBounds(double bounds[6])
{
  this->Box->SetOrigin(bounds[0], bounds[2], bounds[4]);
  this->Box->SetSpacing((bounds[1] - bounds[0]), (bounds[3] - bounds[2]), (bounds[5] - bounds[4]));
  this->Outline->Update();
}

//------------------------------------------------------------------------------
void vtkBoundedWidgetRepresentation::UpdateCenterAndBounds(double center[6])
{
  double widgetBounds[6];
  this->GetWidgetBounds(widgetBounds);
  vtkBoundingBox widgetBox(widgetBounds);

  if (!this->GetOutsideBounds())
  {
    vtkBoundingBox initialBox(this->InitialBounds);
    initialBox.ClampPoint(center);

    if (this->GetConstrainToWidgetBounds())
    {
      // move box to still contains center.
      if (!widgetBox.ContainsPoint(center))
      {
        double distance[3];
        widgetBox.GetDistance(center, distance);
        widgetBox.Translate(distance);
      }
    }
  }

  if (this->GetConstrainToWidgetBounds())
  {
    widgetBox.ClampPoint(center);
  }
  else
  {
    widgetBox.AddPoint(center);
  }

  widgetBox.GetBounds(widgetBounds);
  this->SetOutlineBounds(widgetBounds);
}

//------------------------------------------------------------------------------
void vtkBoundedWidgetRepresentation::TransformBounds(vtkTransform* transform)
{
  double* origin = this->Box->GetOrigin();
  double* spacing = this->Box->GetSpacing();
  double oNew[3], p[3], pNew[3];
  p[0] = origin[0] + spacing[0];
  p[1] = origin[1] + spacing[1];
  p[2] = origin[2] + spacing[2];

  transform->TransformPoint(origin, oNew);
  transform->TransformPoint(p, pNew);

  this->Box->SetOrigin(oNew);
  this->Box->SetSpacing((pNew[0] - oNew[0]), (pNew[1] - oNew[1]), (pNew[2] - oNew[2]));

  this->UpdateWidgetBounds();
}

//------------------------------------------------------------------------------
void vtkBoundedWidgetRepresentation::UpdateWidgetBounds()
{
  this->Box->GetBounds(this->WidgetBounds);
}

//------------------------------------------------------------------------------
void vtkBoundedWidgetRepresentation::UpdateOutline()
{
  this->Outline->Update();
}

//------------------------------------------------------------------------------
void vtkBoundedWidgetRepresentation::CreateDefaultProperties()
{
  this->OutlineProperty->SetAmbient(1.0);
  this->OutlineProperty->SetColor(1.0, 1.0, 1.0);

  this->SelectedOutlineProperty->SetAmbient(1.0);
  this->SelectedOutlineProperty->SetColor(0.0, 1.0, 0.0);
}

//------------------------------------------------------------------------------
void vtkBoundedWidgetRepresentation::SetOutlineColor(double r, double g, double b)
{
  this->OutlineProperty->SetColor(r, g, b);
}

//------------------------------------------------------------------------------
void vtkBoundedWidgetRepresentation::SetSelectedOutlineColor(double r, double g, double b)
{
  this->SelectedOutlineProperty->SetColor(r, g, b);
}

//------------------------------------------------------------------------------
void vtkBoundedWidgetRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  if (this->OutlineProperty)
  {
    os << indent << "Outline Property: " << this->OutlineProperty << "\n";
  }
  else
  {
    os << indent << "Outline Property: (none)\n";
  }
  if (this->SelectedOutlineProperty)
  {
    os << indent << "Selected Outline Property: " << this->SelectedOutlineProperty << "\n";
  }
  else
  {
    os << indent << "Selected Outline Property: (none)\n";
  }
  os << indent << "Outline Translation: " << (this->OutlineTranslation ? "On" : "Off") << "\n";
  os << indent << "Outside Bounds: " << (this->OutsideBounds ? "On" : "Off") << "\n";
  os << indent << "Constrain to Widget Bounds: " << (this->ConstrainToWidgetBounds ? "On" : "Off")
     << "\n";

  os << indent << "Widget Bounds: " << this->WidgetBounds[0] << ", " << this->WidgetBounds[1]
     << ", " << this->WidgetBounds[2] << ", " << this->WidgetBounds[3] << ", "
     << this->WidgetBounds[4] << ", " << this->WidgetBounds[5] << "\n";

  this->Superclass::PrintSelf(os, indent);
}

VTK_ABI_NAMESPACE_END
