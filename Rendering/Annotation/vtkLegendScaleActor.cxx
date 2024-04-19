// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkLegendScaleActor.h"
#include "vtkActor2D.h"
#include "vtkAxisActor2D.h"
#include "vtkAxisGridActorPrivate.h"
#include "vtkCamera.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkCommand.h"
#include "vtkCoordinate.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper2D.h"
#include "vtkProperty2D.h"
#include "vtkRenderer.h"
#include "vtkTextMapper.h"
#include "vtkTextProperty.h"
#include "vtkUnsignedCharArray.h"
#include "vtkWindow.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkLegendScaleActor);

//------------------------------------------------------------------------------
vtkLegendScaleActor::vtkLegendScaleActor()
{
  this->RightAxis->GetPositionCoordinate()->SetCoordinateSystemToViewport();
  this->RightAxis->GetPosition2Coordinate()->SetCoordinateSystemToViewport();
  this->RightAxis->GetPositionCoordinate()->SetReferenceCoordinate(nullptr);
  this->RightAxis->SetFontFactor(0.6);
  this->RightAxis->SetNumberOfLabels(5);
  this->RightAxis->AdjustLabelsOff();

  this->TopAxis->GetPositionCoordinate()->SetCoordinateSystemToViewport();
  this->TopAxis->GetPosition2Coordinate()->SetCoordinateSystemToViewport();
  this->TopAxis->GetPositionCoordinate()->SetReferenceCoordinate(nullptr);
  this->TopAxis->SetFontFactor(0.6);
  this->TopAxis->SetNumberOfLabels(5);
  this->TopAxis->AdjustLabelsOff();

  this->LeftAxis->GetPositionCoordinate()->SetCoordinateSystemToViewport();
  this->LeftAxis->GetPosition2Coordinate()->SetCoordinateSystemToViewport();
  this->LeftAxis->GetPositionCoordinate()->SetReferenceCoordinate(nullptr);
  this->LeftAxis->SetFontFactor(0.6);
  this->LeftAxis->SetNumberOfLabels(5);
  this->LeftAxis->AdjustLabelsOff();

  this->BottomAxis->GetPositionCoordinate()->SetCoordinateSystemToViewport();
  this->BottomAxis->GetPosition2Coordinate()->SetCoordinateSystemToViewport();
  this->BottomAxis->GetPositionCoordinate()->SetReferenceCoordinate(nullptr);
  this->BottomAxis->SetFontFactor(0.6);
  this->BottomAxis->SetNumberOfLabels(5);
  this->BottomAxis->AdjustLabelsOff();

  this->Legend->SetPoints(this->LegendPoints);
  this->LegendMapper->SetInputData(this->Legend);
  this->LegendActor->SetMapper(this->LegendMapper);

  this->GridActor->SetMapper(this->GridMapper);

  // Create the legend
  vtkIdType pts[4];
  this->LegendPoints->SetNumberOfPoints(10);
  vtkNew<vtkCellArray> legendPolys;
  legendPolys->AllocateEstimate(4, 4);
  pts[0] = 0;
  pts[1] = 1;
  pts[2] = 6;
  pts[3] = 5;
  legendPolys->InsertNextCell(4, pts);
  pts[0] = 1;
  pts[1] = 2;
  pts[2] = 7;
  pts[3] = 6;
  legendPolys->InsertNextCell(4, pts);
  pts[0] = 2;
  pts[1] = 3;
  pts[2] = 8;
  pts[3] = 7;
  legendPolys->InsertNextCell(4, pts);
  pts[0] = 3;
  pts[1] = 4;
  pts[2] = 9;
  pts[3] = 8;
  legendPolys->InsertNextCell(4, pts);
  this->Legend->SetPolys(legendPolys);

  // Create the cell data
  vtkUnsignedCharArray* colors = vtkUnsignedCharArray::New();
  colors->SetNumberOfComponents(3);
  colors->SetNumberOfTuples(4);
  colors->SetTuple3(0, 0, 0, 0);
  colors->SetTuple3(1, 255, 255, 255);
  colors->SetTuple3(2, 0, 0, 0);
  colors->SetTuple3(3, 255, 255, 255);
  this->Legend->GetCellData()->SetScalars(colors);
  colors->Delete();

  // Now the text. The first five are for the 0,1/4,1/2,3/4,1 labels.
  this->LegendTitleProperty->SetJustificationToCentered();
  this->LegendTitleProperty->SetVerticalJustificationToBottom();
  this->LegendTitleProperty->SetBold(1);
  this->LegendTitleProperty->SetItalic(1);
  this->LegendTitleProperty->SetShadow(1);
  this->LegendTitleProperty->SetFontFamilyToArial();
  this->LegendTitleProperty->SetFontSize(10);

  this->LegendLabelProperty->SetJustificationToCentered();
  this->LegendLabelProperty->SetVerticalJustificationToTop();
  this->LegendLabelProperty->SetBold(1);
  this->LegendLabelProperty->SetItalic(1);
  this->LegendLabelProperty->SetShadow(1);
  this->LegendLabelProperty->SetFontFamilyToArial();
  this->LegendLabelProperty->SetFontSize(8);
  for (int i = 0; i < 6; i++)
  {
    this->LabelMappers[i]->SetTextProperty(this->LegendLabelProperty);
    this->LabelActors[i]->SetMapper(this->LabelMappers[i]);
  }
  this->LabelMappers[5]->SetTextProperty(this->LegendTitleProperty);
  this->LabelMappers[0]->SetInput("0");
  this->LabelMappers[1]->SetInput("1/4");
  this->LabelMappers[2]->SetInput("1/2");
  this->LabelMappers[3]->SetInput("3/4");
  this->LabelMappers[4]->SetInput("1");

  this->Coordinate->SetCoordinateSystemToDisplay();
}

//------------------------------------------------------------------------------
vtkLegendScaleActor::~vtkLegendScaleActor() = default;

//------------------------------------------------------------------------------
void vtkLegendScaleActor::SetAdjustLabels(bool adjust)
{
  this->RightAxis->SetAdjustLabels(adjust);
  this->TopAxis->SetAdjustLabels(adjust);
  this->LeftAxis->SetAdjustLabels(adjust);
  this->BottomAxis->SetAdjustLabels(adjust);
}

//------------------------------------------------------------------------------
void vtkLegendScaleActor::SetSnapToGrid(bool adjust)
{
  this->RightAxis->SetSnapLabelsToGrid(adjust);
  this->TopAxis->SetSnapLabelsToGrid(adjust);
  this->LeftAxis->SetSnapLabelsToGrid(adjust);
  this->BottomAxis->SetSnapLabelsToGrid(adjust);
}

//------------------------------------------------------------------------------
void vtkLegendScaleActor::SetUseFontSizeFromProperty(bool fromProp)
{
  this->RightAxis->SetUseFontSizeFromProperty(fromProp);
  this->TopAxis->SetUseFontSizeFromProperty(fromProp);
  this->LeftAxis->SetUseFontSizeFromProperty(fromProp);
  this->BottomAxis->SetUseFontSizeFromProperty(fromProp);
}

//------------------------------------------------------------------------------
void vtkLegendScaleActor::SetAxesTextProperty(vtkTextProperty* prop)
{
  this->RightAxis->SetLabelTextProperty(prop);
  this->TopAxis->SetLabelTextProperty(prop);
  this->LeftAxis->SetLabelTextProperty(prop);
  this->BottomAxis->SetLabelTextProperty(prop);

  this->RightAxis->SetTitleTextProperty(prop);
  this->TopAxis->SetTitleTextProperty(prop);
  this->LeftAxis->SetTitleTextProperty(prop);
  this->BottomAxis->SetTitleTextProperty(prop);

  this->Modified();
}

//------------------------------------------------------------------------------
void vtkLegendScaleActor::SetAxesProperty(vtkProperty2D* prop)
{
  if (this->GetAxesProperty() != prop)
  {
    this->RightAxis->SetProperty(prop);
    this->TopAxis->SetProperty(prop);
    this->LeftAxis->SetProperty(prop);
    this->BottomAxis->SetProperty(prop);
    this->GridActor->SetProperty(prop);
    this->Modified();
  }
}

//------------------------------------------------------------------------------
vtkProperty2D* vtkLegendScaleActor::GetAxesProperty()
{
  return this->RightAxis->GetProperty();
}

//------------------------------------------------------------------------------
void vtkLegendScaleActor::GetActors2D(vtkPropCollection* pc)
{
  pc->AddItem(this->RightAxis);
  pc->AddItem(this->TopAxis);
  pc->AddItem(this->LeftAxis);
  pc->AddItem(this->BottomAxis);
}

//------------------------------------------------------------------------------
void vtkLegendScaleActor::ReleaseGraphicsResources(vtkWindow* w)
{
  this->RightAxis->ReleaseGraphicsResources(w);
  this->TopAxis->ReleaseGraphicsResources(w);
  this->LeftAxis->ReleaseGraphicsResources(w);
  this->BottomAxis->ReleaseGraphicsResources(w);

  this->LegendActor->ReleaseGraphicsResources(w);

  for (int i = 0; i < 6; i++)
  {
    this->LabelActors[i]->ReleaseGraphicsResources(w);
  }
}

//------------------------------------------------------------------------------
int vtkLegendScaleActor::RenderOpaqueGeometry(vtkViewport* viewport)
{
  this->BuildRepresentation(viewport);

  int renderedSomething = 0;
  renderedSomething +=
    this->RightAxis->UpdateGeometryAndRenderOpaqueGeometry(viewport, this->RightAxisVisibility);
  renderedSomething +=
    this->TopAxis->UpdateGeometryAndRenderOpaqueGeometry(viewport, this->TopAxisVisibility);
  renderedSomething +=
    this->LeftAxis->UpdateGeometryAndRenderOpaqueGeometry(viewport, this->LeftAxisVisibility);
  renderedSomething +=
    this->BottomAxis->UpdateGeometryAndRenderOpaqueGeometry(viewport, this->BottomAxisVisibility);

  if (this->LegendVisibility)
  {
    renderedSomething += this->LegendActor->RenderOpaqueGeometry(viewport);
    renderedSomething += this->LabelActors[0]->RenderOpaqueGeometry(viewport);
    renderedSomething += this->LabelActors[1]->RenderOpaqueGeometry(viewport);
    renderedSomething += this->LabelActors[2]->RenderOpaqueGeometry(viewport);
    renderedSomething += this->LabelActors[3]->RenderOpaqueGeometry(viewport);
    renderedSomething += this->LabelActors[4]->RenderOpaqueGeometry(viewport);
    renderedSomething += this->LabelActors[5]->RenderOpaqueGeometry(viewport);
  }
  if (this->GridVisibility)
  {
    renderedSomething += this->GridActor->RenderOpaqueGeometry(viewport);
  }

  return renderedSomething;
}

//------------------------------------------------------------------------------
int vtkLegendScaleActor::RenderOverlay(vtkViewport* viewport)
{
  int renderedSomething = 0;
  if (this->RightAxisVisibility)
  {
    renderedSomething = this->RightAxis->RenderOverlay(viewport);
  }
  if (this->TopAxisVisibility)
  {
    renderedSomething += this->TopAxis->RenderOverlay(viewport);
  }
  if (this->LeftAxisVisibility)
  {
    renderedSomething += this->LeftAxis->RenderOverlay(viewport);
  }
  if (this->BottomAxisVisibility)
  {
    renderedSomething += this->BottomAxis->RenderOverlay(viewport);
  }
  if (this->LegendVisibility)
  {
    renderedSomething += this->LegendActor->RenderOverlay(viewport);
    renderedSomething += this->LabelActors[0]->RenderOverlay(viewport);
    renderedSomething += this->LabelActors[1]->RenderOverlay(viewport);
    renderedSomething += this->LabelActors[2]->RenderOverlay(viewport);
    renderedSomething += this->LabelActors[3]->RenderOverlay(viewport);
    renderedSomething += this->LabelActors[4]->RenderOverlay(viewport);
    renderedSomething += this->LabelActors[5]->RenderOverlay(viewport);
  }
  if (this->GridVisibility)
  {
    renderedSomething += this->GridActor->RenderOverlay(viewport);
  }

  return renderedSomething;
}

//------------------------------------------------------------------------------
void vtkLegendScaleActor::UpdateAxisRange(vtkAxisActor2D* axis, vtkViewport* viewport, bool invert)
{
  double* minPoint = axis->GetPositionCoordinate()->GetComputedWorldValue(viewport);
  double* maxPoint = axis->GetPosition2Coordinate()->GetComputedWorldValue(viewport);

  double range[2];
  if (this->LabelMode == COORDINATES)
  {
    int mainOrientation = 0;
    double size = 0;
    for (int comp = 0; comp < 3; comp++)
    {
      // COORDINATES is expected to be used only if the screen is parallel
      // to one of the main planes, i.e. vtkAxisActor2D should be aligned with
      // one of the scene axes. So find it.
      double componentSize = std::abs(maxPoint[comp] - minPoint[comp]);
      if (componentSize > size)
      {
        mainOrientation = comp;
      }
    }

    range[0] = minPoint[mainOrientation] - this->Origin[mainOrientation];
    range[1] = maxPoint[mainOrientation] - this->Origin[mainOrientation];
  }
  else
  {
    double d = sqrt(vtkMath::Distance2BetweenPoints(minPoint, maxPoint));
    range[0] = -d / 2;
    range[1] = d / 2;
    if (invert)
    {
      range[0] = -range[0];
      range[1] = -range[1];
    }
  }

  axis->SetRange(range);
}

//------------------------------------------------------------------------------
void vtkLegendScaleActor::BuildRepresentation(vtkViewport* viewport)
{
  // Specify the locations of the axes.
  const int* size = viewport->GetSize();

  this->RightAxis->GetPositionCoordinate()->SetValue(
    size[0] - this->RightBorderOffset, this->CornerOffsetFactor * this->BottomBorderOffset, 0.0);
  this->RightAxis->GetPosition2Coordinate()->SetValue(size[0] - this->RightBorderOffset,
    size[1] - this->CornerOffsetFactor * this->TopBorderOffset, 0.0);

  this->TopAxis->GetPositionCoordinate()->SetValue(
    size[0] - this->CornerOffsetFactor * this->RightBorderOffset, size[1] - this->TopBorderOffset,
    0.0);
  this->TopAxis->GetPosition2Coordinate()->SetValue(
    this->CornerOffsetFactor * this->LeftBorderOffset, size[1] - this->TopBorderOffset, 0.0);

  this->LeftAxis->GetPositionCoordinate()->SetValue(
    this->LeftBorderOffset, size[1] - this->CornerOffsetFactor * this->TopBorderOffset, 0.0);
  this->LeftAxis->GetPosition2Coordinate()->SetValue(
    this->LeftBorderOffset, this->CornerOffsetFactor * this->BottomBorderOffset, 0.0);

  if (this->LegendVisibility)
  {
    this->BottomAxis->GetPositionCoordinate()->SetValue(
      this->CornerOffsetFactor * this->LeftBorderOffset, 2 * this->BottomBorderOffset, 0.0);
    this->BottomAxis->GetPosition2Coordinate()->SetValue(
      size[0] - this->CornerOffsetFactor * this->RightBorderOffset, 2 * this->BottomBorderOffset,
      0.0);
  }
  else
  {
    this->BottomAxis->GetPositionCoordinate()->SetValue(
      this->CornerOffsetFactor * this->LeftBorderOffset, this->BottomBorderOffset, 0.0);
    this->BottomAxis->GetPosition2Coordinate()->SetValue(
      size[0] - this->CornerOffsetFactor * this->RightBorderOffset, this->BottomBorderOffset, 0.0);
  }

  if (this->GridVisibility)
  {
    this->GridActor->SetHorizontalLinesLeftPoints(this->LeftAxis->GetTickPositions());
    this->GridActor->SetHorizontalLinesRightPoints(this->RightAxis->GetTickPositions());
    this->GridActor->SetVerticalLinesBottomPoints(this->BottomAxis->GetTickPositions());
    this->GridActor->SetVerticalLinesTopPoints(this->TopAxis->GetTickPositions());
  }

  this->UpdateAxisRange(this->RightAxis, viewport);
  this->UpdateAxisRange(this->TopAxis, viewport, true);
  this->UpdateAxisRange(this->LeftAxis, viewport, true);
  this->UpdateAxisRange(this->BottomAxis, viewport);

  if (this->LegendVisibility)
  {
    // Update the position
    double x1 = 0.33333 * size[0];
    double delX = x1 / 4.0;

    this->LegendPoints->SetPoint(0, x1, 10, 0);
    this->LegendPoints->SetPoint(1, x1 + delX, 10, 0);
    this->LegendPoints->SetPoint(2, x1 + 2 * delX, 10, 0);
    this->LegendPoints->SetPoint(3, x1 + 3 * delX, 10, 0);
    this->LegendPoints->SetPoint(4, x1 + 4 * delX, 10, 0);
    this->LegendPoints->SetPoint(5, x1, 20, 0);
    this->LegendPoints->SetPoint(6, x1 + delX, 20, 0);
    this->LegendPoints->SetPoint(7, x1 + 2 * delX, 20, 0);
    this->LegendPoints->SetPoint(8, x1 + 3 * delX, 20, 0);
    this->LegendPoints->SetPoint(9, x1 + 4 * delX, 20, 0);
    this->LegendPoints->Modified();

    // Specify the position of the legend title
    this->LabelActors[5]->SetPosition(0.5 * size[0], 22);
    this->Coordinate->SetValue(0.33333 * size[0], 15, 0.0);
    double* x = this->Coordinate->GetComputedWorldValue(viewport);
    double xL[3];
    xL[0] = x[0];
    xL[1] = x[1];
    xL[2] = x[2];
    this->Coordinate->SetValue(0.66667 * size[0], 15, 0.0);
    x = this->Coordinate->GetComputedWorldValue(viewport);
    double xR[3];
    xR[0] = x[0];
    xR[1] = x[1];
    xR[2] = x[2];
    double len = sqrt(vtkMath::Distance2BetweenPoints(xL, xR));
    char buf[256];
    snprintf(buf, sizeof(buf), "Scale 1 : %g", len);
    this->LabelMappers[5]->SetInput(buf);

    // Now specify the position of the legend labels
    x = this->LegendPoints->GetPoint(0);
    this->LabelActors[0]->SetPosition(x[0], x[1] - 1);
    x = this->LegendPoints->GetPoint(1);
    this->LabelActors[1]->SetPosition(x[0], x[1] - 1);
    x = this->LegendPoints->GetPoint(2);
    this->LabelActors[2]->SetPosition(x[0], x[1] - 1);
    x = this->LegendPoints->GetPoint(3);
    this->LabelActors[3]->SetPosition(x[0], x[1] - 1);
    x = this->LegendPoints->GetPoint(4);
    this->LabelActors[4]->SetPosition(x[0], x[1] - 1);
  }

  this->BuildTime.Modified();
}

//------------------------------------------------------------------------------
void vtkLegendScaleActor::AllAnnotationsOn()
{
  if (this->RightAxisVisibility && this->TopAxisVisibility && this->LeftAxisVisibility &&
    this->BottomAxisVisibility && this->LegendVisibility)
  {
    return;
  }

  // If here, we are modified and something gets turned on
  this->RightAxisVisibility = 1;
  this->TopAxisVisibility = 1;
  this->LeftAxisVisibility = 1;
  this->BottomAxisVisibility = 1;
  this->LegendVisibility = 1;
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkLegendScaleActor::AllAnnotationsOff()
{
  if (!this->RightAxisVisibility && !this->TopAxisVisibility && !this->LeftAxisVisibility &&
    !this->BottomAxisVisibility && !this->LegendVisibility)
  {
    return;
  }

  // If here, we are modified and something gets turned off
  this->RightAxisVisibility = 0;
  this->TopAxisVisibility = 0;
  this->LeftAxisVisibility = 0;
  this->BottomAxisVisibility = 0;
  this->LegendVisibility = 0;
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkLegendScaleActor::AllAxesOn()
{
  if (this->RightAxisVisibility && this->TopAxisVisibility && this->LeftAxisVisibility &&
    this->BottomAxisVisibility)
  {
    return;
  }

  // If here, we are modified and something gets turned on
  this->RightAxisVisibility = 1;
  this->TopAxisVisibility = 1;
  this->LeftAxisVisibility = 1;
  this->BottomAxisVisibility = 1;
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkLegendScaleActor::AllAxesOff()
{
  if (!this->RightAxisVisibility && !this->TopAxisVisibility && !this->LeftAxisVisibility &&
    !this->BottomAxisVisibility)
  {
    return;
  }

  // If here, we are modified and something gets turned off
  this->RightAxisVisibility = 0;
  this->TopAxisVisibility = 0;
  this->LeftAxisVisibility = 0;
  this->BottomAxisVisibility = 0;
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkLegendScaleActor::SetNotation(int notation)
{
  if (this->GetNotation() != notation)
  {
    this->RightAxis->SetNotation(notation);
    this->LeftAxis->SetNotation(notation);
    this->TopAxis->SetNotation(notation);
    this->BottomAxis->SetNotation(notation);
    this->Modified();
  }
}

//----------------------------------------------------------------------------
int vtkLegendScaleActor::GetNotation()
{
  return this->RightAxis->GetNotation();
}

//----------------------------------------------------------------------------
void vtkLegendScaleActor::SetPrecision(int val)
{
  if (this->GetPrecision() != val)
  {
    this->RightAxis->SetPrecision(val);
    this->LeftAxis->SetPrecision(val);
    this->TopAxis->SetPrecision(val);
    this->BottomAxis->SetPrecision(val);
    this->Modified();
  }
}

//----------------------------------------------------------------------------
int vtkLegendScaleActor::GetPrecision()
{
  return this->RightAxis->GetPrecision();
}

//----------------------------------------------------------------------------
void vtkLegendScaleActor::SetNumberOfHorizontalLabels(int val)
{
  if (this->GetNumberOfHorizontalLabels() != val)
  {
    this->TopAxis->SetNumberOfLabels(val);
    this->BottomAxis->SetNumberOfLabels(val);
    this->Modified();
  }
}

//----------------------------------------------------------------------------
int vtkLegendScaleActor::GetNumberOfHorizontalLabels()
{
  return this->TopAxis->GetNumberOfLabels();
}

//----------------------------------------------------------------------------
void vtkLegendScaleActor::SetNumberOfVerticalLabels(int val)
{
  if (this->GetNumberOfVerticalLabels() != val)
  {
    this->LeftAxis->SetNumberOfLabels(val);
    this->RightAxis->SetNumberOfLabels(val);
    this->Modified();
  }
}

//----------------------------------------------------------------------------
int vtkLegendScaleActor::GetNumberOfVerticalLabels()
{
  return this->LeftAxis->GetNumberOfLabels();
}

//------------------------------------------------------------------------------
void vtkLegendScaleActor::PrintSelf(ostream& os, vtkIndent indent)
{
  // Superclass typedef defined in vtkTypeMacro() found in vtkSetGet.h
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Label Mode: ";
  if (this->LabelMode == DISTANCE)
  {
    os << "Distance\n";
  }
  else // if ( this->LabelMode == DISTANCE )
  {
    os << "XY_Coordinates\n";
  }

  os << indent << "Right Axis Visibility: " << (this->RightAxisVisibility ? "On\n" : "Off\n");
  os << indent << "Top Axis Visibility: " << (this->TopAxisVisibility ? "On\n" : "Off\n");
  os << indent << "Left Axis Visibility: " << (this->LeftAxisVisibility ? "On\n" : "Off\n");
  os << indent << "Bottom Axis Visibility: " << (this->BottomAxisVisibility ? "On\n" : "Off\n");
  os << indent << "Legend Visibility: " << (this->LegendVisibility ? "On\n" : "Off\n");
  os << indent << "Grid Visibility: " << (this->GridVisibility ? "On\n" : "Off\n");

  os << indent << "Corner Offset Factor: " << this->CornerOffsetFactor << "\n";
  os << indent << "Right Border Offset: " << this->RightBorderOffset << "\n";
  os << indent << "Top Border Offset: " << this->TopBorderOffset << "\n";
  os << indent << "Left Border Offset: " << this->LeftBorderOffset << "\n";
  os << indent << "Bottom Border Offset: " << this->BottomBorderOffset << "\n";

  os << indent << "Label value notation: " << this->GetNotation() << "\n";
  os << indent << "Label value precision: " << this->GetPrecision() << "\n";

  os << indent << "Number of vertical labels: " << this->GetNumberOfVerticalLabels() << "\n";
  os << indent << "Number of horizontal labels: " << this->GetNumberOfHorizontalLabels() << "\n";

  os << indent << "Legend Title Property: ";
  if (this->LegendTitleProperty)
  {
    os << this->LegendTitleProperty << "\n";
  }
  else
  {
    os << "(none)\n";
  }
  os << indent << "Legend Label Property: ";
  if (this->LegendLabelProperty)
  {
    os << this->LegendLabelProperty << "\n";
  }
  else
  {
    os << "(none)\n";
  }
  os << indent << "Axes 2D Property: ";
  if (this->GetAxesProperty())
  {
    os << this->GetAxesProperty() << "\n";
  }
  else
  {
    os << "(none)\n";
  }

  os << indent << "Right Axis: ";
  if (this->RightAxis)
  {
    os << this->RightAxis << "\n";
  }
  else
  {
    os << "(none)\n";
  }
  os << indent << "Top Axis: ";
  if (this->TopAxis)
  {
    os << this->TopAxis << "\n";
  }
  else
  {
    os << "(none)\n";
  }
  os << indent << "Left Axis: ";
  if (this->LeftAxis)
  {
    os << this->LeftAxis << "\n";
  }
  else
  {
    os << "(none)\n";
  }
  os << indent << "Bottom Axis: ";
  if (this->BottomAxis)
  {
    os << this->BottomAxis << "\n";
  }
  else
  {
    os << "(none)\n";
  }
}
VTK_ABI_NAMESPACE_END
