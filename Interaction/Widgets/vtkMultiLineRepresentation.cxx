// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkMultiLineRepresentation.h"

#include "vtkBox.h"
#include "vtkDoubleArray.h"
#include "vtkLineRepresentation.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkMultiLineRepresentation);

//------------------------------------------------------------------------------
vtkMultiLineRepresentation::vtkMultiLineRepresentation()
{
  this->Point1WorldPositions->SetNumberOfComponents(3);
  this->Point1DisplayPositions->SetNumberOfComponents(3);
  this->Point2WorldPositions->SetNumberOfComponents(3);
  this->Point2DisplayPositions->SetNumberOfComponents(3);

  // Handle size is in pixels for this widget
  this->HandleSize = 5.0;

  // Miscellaneous parameters
  this->Placed = 0;

  this->CreateDefaultProperties();

  this->SetLineCount(4);

  // Define the point coordinates
  double bounds[6];
  bounds[0] = -0.5;
  bounds[1] = 0.5;
  bounds[2] = -0.5;
  bounds[3] = 0.5;
  bounds[4] = -0.5;
  bounds[5] = 0.5;
  this->PlaceFactor = 1.0; // overload parent's value

  this->PlaceWidget(bounds);
}

//------------------------------------------------------------------------------
vtkMultiLineRepresentation::~vtkMultiLineRepresentation() = default;

//------------------------------------------------------------------------------
void vtkMultiLineRepresentation::SetLineCount(int newLineCount)
{
  if (newLineCount < 0)
  {
    vtkWarningMacro(<< newLineCount << " is an invalid line count.");
    return;
  }

  if (newLineCount == this->LineCount)
  {
    return;
  }

  this->LineRepresentationVector.resize(newLineCount);
  this->Point1WorldPositions->SetNumberOfTuples(newLineCount);
  this->Point1DisplayPositions->SetNumberOfTuples(newLineCount);
  this->Point2WorldPositions->SetNumberOfTuples(newLineCount);
  this->Point2DisplayPositions->SetNumberOfTuples(newLineCount);

  for (int i = this->LineCount; i < newLineCount; i++)
  {
    this->AddNewLine(i);
  }
  this->LineCount = newLineCount;
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkMultiLineRepresentation::AddNewLine(int index)
{
  auto lineRepr = vtkSmartPointer<vtkLineRepresentation>::New();
  this->LineRepresentationVector[index] = lineRepr;

  this->ApplyProperties(index);

  lineRepr->SetRenderer(this->Renderer);
  lineRepr->SetDirectionalLine(this->DirectionalLine);
  lineRepr->SetResolution(this->Resolution);
  lineRepr->SetTolerance(this->Tolerance);

  // If it's the first line, we don't need to modify its position
  if (index == 0)
  {
    return;
  }

  // Otherwise, we shift the new line from the position of the last line
  vtkLineRepresentation* prevLineRepr = this->LineRepresentationVector[index - 1];

  double p1[3], p2[3], *prevP1, *prevP2;

  prevP1 = prevLineRepr->GetPoint1WorldPosition();
  prevP2 = prevLineRepr->GetPoint2WorldPosition();

  p1[0] = prevP1[0];
  p2[0] = prevP2[0];
  p1[1] = prevP1[1] + 1.0;
  p2[1] = prevP2[1] + 1.0;
  p1[2] = prevP1[2] + 1.0;
  p2[2] = prevP2[2] + 1.0;

  lineRepr->SetPoint1WorldPosition(p1);
  lineRepr->SetPoint2WorldPosition(p2);
}

//------------------------------------------------------------------------------
void vtkMultiLineRepresentation::ApplyProperties(int index)
{
  // We can't use LineCount here because it is not yet updated when we add a new line
  if (index < 0 || index >= static_cast<int>(this->LineRepresentationVector.size()))
  {
    vtkWarningMacro("The index " << index << " is not valid.");
    return;
  }

  this->LineRepresentationVector[index]->SetLineProperty(this->LineProperty);
  this->LineRepresentationVector[index]->SetSelectedLineProperty(this->SelectedLineProperty);
  this->LineRepresentationVector[index]->SetEndPointProperty(this->EndPointProperty);
  this->LineRepresentationVector[index]->SetSelectedEndPointProperty(
    this->SelectedEndPointProperty);
  this->LineRepresentationVector[index]->SetEndPoint2Property(this->EndPoint2Property);
  this->LineRepresentationVector[index]->SetSelectedEndPoint2Property(
    this->SelectedEndPoint2Property);
}

//------------------------------------------------------------------------------
void vtkMultiLineRepresentation::SetDirectionalLine(bool val)
{
  if (this->DirectionalLine == val)
  {
    return;
  }

  this->DirectionalLine = val;
  for (vtkLineRepresentation* lineRepr : this->LineRepresentationVector)
  {
    lineRepr->SetDirectionalLine(val);
  }
  this->Modified();
}

//------------------------------------------------------------------------------
double vtkMultiLineRepresentation::GetDistance(int index)
{
  if (index < 0 || index >= this->LineCount)
  {
    vtkWarningMacro("The index " << index << " is not valid.");
    return 0;
  }
  return GetDistance(index);
}

//------------------------------------------------------------------------------
void vtkMultiLineRepresentation::SetTolerance(int tol)
{
  int newTol = std::clamp(tol, 1, 100);
  if (this->Tolerance == newTol)
  {
    return;
  }

  this->Tolerance = newTol;
  for (vtkLineRepresentation* lineRepr : this->LineRepresentationVector)
  {
    lineRepr->SetTolerance(this->Tolerance);
  }
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkMultiLineRepresentation::SetResolution(int res)
{
  int newRes = std::max(1, res);
  if (this->Resolution == newRes)
  {
    return;
  }

  this->Resolution = newRes;
  for (vtkLineRepresentation* lineRepr : this->LineRepresentationVector)
  {
    lineRepr->SetResolution(this->Resolution);
  }
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkMultiLineRepresentation::GetPolyData(int index, vtkPolyData* pd)
{
  if (index < 0 || index >= this->LineCount)
  {
    vtkWarningMacro("The index " << index << " is not valid.");
    return;
  }
  this->LineRepresentationVector[index]->GetPolyData(pd);
}

//------------------------------------------------------------------------------
void vtkMultiLineRepresentation::UpdatePoint1Positions()
{
  for (int i = 0; i < this->LineCount; i++)
  {
    for (int j = 0; j < 3; j++)
    {
      Point1WorldPositions->SetComponent(
        i, j, this->LineRepresentationVector[i]->GetPoint1WorldPosition()[j]);
      Point1DisplayPositions->SetComponent(
        i, j, this->LineRepresentationVector[i]->GetPoint1DisplayPosition()[j]);
    }
  }
}

//------------------------------------------------------------------------------
void vtkMultiLineRepresentation::UpdatePoint2Positions()
{
  for (int i = 0; i < this->LineCount; i++)
  {
    for (int j = 0; j < 3; j++)
    {
      Point2WorldPositions->SetComponent(
        i, j, this->LineRepresentationVector[i]->GetPoint2WorldPosition()[j]);
      Point2DisplayPositions->SetComponent(
        i, j, this->LineRepresentationVector[i]->GetPoint2DisplayPosition()[j]);
    }
  }
}

//------------------------------------------------------------------------------
vtkDoubleArray* vtkMultiLineRepresentation::GetPoint1WorldPositions()
{
  this->UpdatePoint1Positions();
  return this->Point1WorldPositions;
}

//------------------------------------------------------------------------------
void vtkMultiLineRepresentation::GetPoint1WorldPosition(int index, double pos[3]) VTK_FUTURE_CONST
{
  if (index < 0 || index >= this->LineCount)
  {
    vtkWarningMacro("The index " << index << " is not valid.");
    return;
  }
  this->LineRepresentationVector[index]->GetPoint1WorldPosition(pos);
}

//------------------------------------------------------------------------------
double* vtkMultiLineRepresentation::GetPoint1WorldPosition(int index)
{
  if (index < 0 || index >= this->LineCount)
  {
    vtkWarningMacro("The index " << index << " is not valid.");
    return nullptr;
  }
  return this->LineRepresentationVector[index]->GetPoint1WorldPosition();
}

//------------------------------------------------------------------------------
vtkDoubleArray* vtkMultiLineRepresentation::GetPoint1DisplayPositions()
{
  this->UpdatePoint1Positions();
  return this->Point1DisplayPositions;
}

//------------------------------------------------------------------------------
void vtkMultiLineRepresentation::GetPoint1DisplayPosition(int index, double pos[3]) VTK_FUTURE_CONST
{
  if (index < 0 || index >= this->LineCount)
  {
    vtkWarningMacro("The index " << index << " is not valid.");
    return;
  }
  this->LineRepresentationVector[index]->GetPoint1DisplayPosition(pos);
}

//------------------------------------------------------------------------------
double* vtkMultiLineRepresentation::GetPoint1DisplayPosition(int index)
{
  if (index < 0 || index >= this->LineCount)
  {
    vtkWarningMacro("The index " << index << " is not valid.");
    return nullptr;
  }
  return this->LineRepresentationVector[index]->GetPoint1DisplayPosition();
}

//------------------------------------------------------------------------------
void vtkMultiLineRepresentation::SetPoint1WorldPosition(int index, double x[3])
{
  if (index < 0 || index >= this->LineCount)
  {
    vtkWarningMacro("The index " << index << " is not valid.");
    return;
  }
  this->LineRepresentationVector[index]->SetPoint1WorldPosition(x);
}

//------------------------------------------------------------------------------
void vtkMultiLineRepresentation::SetPoint1WorldPosition(int index, double x, double y, double z)
{
  double p[3] = { x, y, z };
  this->SetPoint1WorldPosition(index, p);
}

//------------------------------------------------------------------------------
void vtkMultiLineRepresentation::SetPoint1DisplayPosition(int index, double x[3])
{
  if (index < 0 || index >= this->LineCount)
  {
    vtkWarningMacro("The index " << index << " is not valid.");
    return;
  }
  this->LineRepresentationVector[index]->SetPoint1DisplayPosition(x);
}

//------------------------------------------------------------------------------
void vtkMultiLineRepresentation::SetPoint1DisplayPosition(int index, double x, double y, double z)
{
  double p[3] = { x, y, z };
  this->SetPoint1DisplayPosition(index, p);
}

//------------------------------------------------------------------------------
vtkDoubleArray* vtkMultiLineRepresentation::GetPoint2WorldPositions()
{
  this->UpdatePoint2Positions();
  return this->Point2WorldPositions;
}

//------------------------------------------------------------------------------
void vtkMultiLineRepresentation::GetPoint2WorldPosition(int index, double pos[3]) VTK_FUTURE_CONST
{
  if (index < 0 || index >= this->LineCount)
  {
    vtkWarningMacro("The index " << index << " is not valid.");
    return;
  }
  this->LineRepresentationVector[index]->GetPoint2WorldPosition(pos);
}

//------------------------------------------------------------------------------
double* vtkMultiLineRepresentation::GetPoint2WorldPosition(int index)
{
  if (index < 0 || index >= this->LineCount)
  {
    vtkWarningMacro("The index " << index << " is not valid.");
    return nullptr;
  }
  return this->LineRepresentationVector[index]->GetPoint2WorldPosition();
}

//------------------------------------------------------------------------------
vtkDoubleArray* vtkMultiLineRepresentation::GetPoint2DisplayPositions()
{
  this->UpdatePoint2Positions();
  return this->Point2DisplayPositions;
}

//------------------------------------------------------------------------------
void vtkMultiLineRepresentation::GetPoint2DisplayPosition(int index, double pos[3]) VTK_FUTURE_CONST
{
  if (index < 0 || index >= this->LineCount)
  {
    vtkWarningMacro("The index " << index << " is not valid.");
    return;
  }
  this->LineRepresentationVector[index]->GetPoint2DisplayPosition(pos);
}

//------------------------------------------------------------------------------
double* vtkMultiLineRepresentation::GetPoint2DisplayPosition(int index)
{
  if (index < 0 || index >= this->LineCount)
  {
    vtkWarningMacro("The index " << index << " is not valid.");
    return nullptr;
  }
  return this->LineRepresentationVector[index]->GetPoint2DisplayPosition();
}

//------------------------------------------------------------------------------
void vtkMultiLineRepresentation::SetPoint2WorldPosition(int index, double x[3])
{
  if (index < 0 || index >= this->LineCount)
  {
    vtkWarningMacro("The index " << index << " is not valid.");
    return;
  }
  this->LineRepresentationVector[index]->SetPoint2WorldPosition(x);
}

//------------------------------------------------------------------------------
void vtkMultiLineRepresentation::SetPoint2WorldPosition(int index, double x, double y, double z)
{
  double p[3] = { x, y, z };
  this->SetPoint2WorldPosition(index, p);
}

//------------------------------------------------------------------------------
void vtkMultiLineRepresentation::SetPoint2DisplayPosition(int index, double x[3])
{
  if (index < 0 || index >= this->LineCount)
  {
    vtkWarningMacro("The index " << index << " is not valid.");
    return;
  }
  this->LineRepresentationVector[index]->SetPoint2DisplayPosition(x);
}

//------------------------------------------------------------------------------
void vtkMultiLineRepresentation::SetPoint2DisplayPosition(int index, double x, double y, double z)
{
  double p[3] = { x, y, z };
  this->SetPoint2DisplayPosition(index, p);
}

//------------------------------------------------------------------------------
void vtkMultiLineRepresentation::SetRenderer(vtkRenderer* ren)
{
  for (vtkLineRepresentation* lineRepr : this->LineRepresentationVector)
  {
    lineRepr->SetRenderer(ren);
  }
  this->Superclass::SetRenderer(ren);
}

//------------------------------------------------------------------------------
void vtkMultiLineRepresentation::PlaceWidget(double bds[6])
{
  double initialBounds[6] = { VTK_DOUBLE_MAX, VTK_DOUBLE_MIN, VTK_DOUBLE_MAX, VTK_DOUBLE_MIN,
    VTK_DOUBLE_MAX, VTK_DOUBLE_MIN };
  double initialScaleFactor[3], newScaleFactor[3];

  for (vtkLineRepresentation* lineRepr : this->LineRepresentationVector)
  {
    for (int i = 0; i < 3; i++)
    {
      initialBounds[2 * i] = std::min(initialBounds[2 * i], lineRepr->GetPoint1WorldPosition()[i]);
      initialBounds[2 * i] = std::min(initialBounds[2 * i], lineRepr->GetPoint2WorldPosition()[i]);
      initialBounds[2 * i + 1] =
        std::max(initialBounds[2 * i + 1], lineRepr->GetPoint1WorldPosition()[i]);
      initialBounds[2 * i + 1] =
        std::max(initialBounds[2 * i + 1], lineRepr->GetPoint2WorldPosition()[i]);
    }
  }

  for (int i = 0; i < 3; i++)
  {
    initialScaleFactor[i] = initialBounds[2 * i + 1] - initialBounds[2 * i];
    newScaleFactor[i] = bds[2 * i + 1] - bds[2 * i];
  }

  for (vtkLineRepresentation* lineRepr : this->LineRepresentationVector)
  {
    double newP1[3], newP2[3];
    double* p1 = lineRepr->GetPoint1WorldPosition();
    double* p2 = lineRepr->GetPoint2WorldPosition();
    for (int i = 0; i < 3; i++)
    {
      newP1[i] =
        newScaleFactor[i] * (p1[i] - initialBounds[2 * i]) / initialScaleFactor[i] + bds[2 * i];
      newP2[i] =
        newScaleFactor[i] * (p2[i] - initialBounds[2 * i]) / initialScaleFactor[i] + bds[2 * i];
    }

    lineRepr->SetPoint1WorldPosition(newP1);
    lineRepr->SetPoint2WorldPosition(newP2);
  }

  this->Placed = 1;
  this->ValidPick = 1;
  this->BuildRepresentation();
}

//------------------------------------------------------------------------------
int vtkMultiLineRepresentation::ComputeInteractionState(int x, int y, int vtkNotUsed(modify))
{
  for (vtkLineRepresentation* lineRepr : this->LineRepresentationVector)
  {
    int lineState = lineRepr->ComputeInteractionState(x, y);
    if (lineState != vtkLineRepresentation::Outside)
    {
      this->InteractionState = lineState;
      this->SetRepresentationState(lineState);
      return lineState;
    }
  }

  this->InteractionState = vtkMultiLineRepresentation::MOUSE_OUTSIDE_LINES;
  this->SetRepresentationState(vtkMultiLineRepresentation::MOUSE_OUTSIDE_LINES);
  return this->InteractionState;
}

//------------------------------------------------------------------------------
void vtkMultiLineRepresentation::SetRepresentationState(int state)
{
  state = std::min<int>(std::max<int>(state, vtkMultiLineRepresentation::MOUSE_OUTSIDE_LINES),
    vtkMultiLineRepresentation::MOUSE_ON_LINE);

  if (this->RepresentationState == state)
  {
    return;
  }

  this->RepresentationState = state;
  this->Modified();
}

//------------------------------------------------------------------------------
double* vtkMultiLineRepresentation::GetBounds()
{
  this->BuildRepresentation();
  this->BoundingBox->SetBounds(this->LineRepresentationVector[0]->GetBounds());
  for (int i = 1; i < this->LineCount; i++)
  {
    this->BoundingBox->AddBounds(this->LineRepresentationVector[i]->GetBounds());
  }
  return this->BoundingBox->GetBounds();
}

//------------------------------------------------------------------------------
vtkLineRepresentation* vtkMultiLineRepresentation::GetLineRepresentation(int index)
{
  if (index < 0 || index >= this->LineCount)
  {
    vtkWarningMacro("The index " << index << " is not valid.");
    return nullptr;
  }
  return this->LineRepresentationVector[index];
}

//------------------------------------------------------------------------------
void vtkMultiLineRepresentation::CreateDefaultProperties()
{
  // Endpoint properties
  this->EndPointProperty->SetColor(1, 1, 1);

  this->SelectedEndPointProperty->SetColor(0, 1, 0);

  this->EndPoint2Property->SetColor(1, 1, 1);

  this->SelectedEndPoint2Property->SetColor(0, 1, 0);

  // Line properties
  this->LineProperty->SetAmbient(1.0);
  this->LineProperty->SetColor(1.0, 1.0, 1.0);
  this->LineProperty->SetLineWidth(2.0);

  this->SelectedLineProperty->SetAmbient(1.0);
  this->SelectedLineProperty->SetColor(0.0, 1.0, 0.0);
  this->SelectedLineProperty->SetLineWidth(2.0);

  for (int i = 0; i < this->LineCount; i++)
  {
    this->ApplyProperties(i);
  }
}

//------------------------------------------------------------------------------
void vtkMultiLineRepresentation::BuildRepresentation()
{
  // Rebuild only if necessary
  if (this->GetMTime() > this->BuildTime)
  {
    for (vtkLineRepresentation* lineRepresentation : this->LineRepresentationVector)
    {
      lineRepresentation->BuildRepresentation();
    }

    this->BuildTime.Modified();
  }
}

//------------------------------------------------------------------------------
void vtkMultiLineRepresentation::SetLineColor(double r, double g, double b)
{
  if (this->GetLineProperty())
  {
    this->GetLineProperty()->SetColor(r, g, b);
  }
}

//------------------------------------------------------------------------------
void vtkMultiLineRepresentation::SetInteractionColor(double r, double g, double b)
{
  this->SelectedEndPointProperty->SetColor(r, g, b);
  this->SelectedEndPoint2Property->SetColor(r, g, b);
  this->SelectedLineProperty->SetColor(r, g, b);
}

//------------------------------------------------------------------------------
void vtkMultiLineRepresentation::SetForegroundColor(double r, double g, double b)
{
  this->EndPointProperty->SetColor(r, g, b);
  this->EndPoint2Property->SetColor(r, g, b);
  this->LineProperty->SetColor(r, g, b);
}

//------------------------------------------------------------------------------
void vtkMultiLineRepresentation::GetActors(vtkPropCollection* pc)
{
  for (vtkLineRepresentation* lineRepresentation : this->LineRepresentationVector)
  {
    lineRepresentation->GetActors(pc);
  }
  this->Superclass::GetActors(pc);
}

//------------------------------------------------------------------------------
void vtkMultiLineRepresentation::ReleaseGraphicsResources(vtkWindow* window)
{
  for (vtkLineRepresentation* lineRepresentation : this->LineRepresentationVector)
  {
    lineRepresentation->ReleaseGraphicsResources(window);
  }
}

//------------------------------------------------------------------------------
int vtkMultiLineRepresentation::RenderOpaqueGeometry(vtkViewport* viewport)
{
  int count = 0;
  this->BuildRepresentation();
  for (vtkLineRepresentation* lineRepresentation : this->LineRepresentationVector)
  {
    count += lineRepresentation->RenderOpaqueGeometry(viewport);
  }

  return count;
}

//------------------------------------------------------------------------------
int vtkMultiLineRepresentation::RenderTranslucentPolygonalGeometry(vtkViewport* viewport)
{
  int count = 0;
  this->BuildRepresentation();
  for (vtkLineRepresentation* lineRepresentation : this->LineRepresentationVector)
  {
    count += lineRepresentation->RenderTranslucentPolygonalGeometry(viewport);
  }

  return count;
}

//------------------------------------------------------------------------------
vtkTypeBool vtkMultiLineRepresentation::HasTranslucentPolygonalGeometry()
{
  vtkTypeBool result = 0;
  this->BuildRepresentation();
  for (vtkLineRepresentation* lineRepresentation : this->LineRepresentationVector)
  {
    result |= lineRepresentation->HasTranslucentPolygonalGeometry();
  }

  return result;
}

//------------------------------------------------------------------------------
vtkMTimeType vtkMultiLineRepresentation::GetMTime()
{
  vtkMTimeType mTime = this->Superclass::GetMTime();
  for (vtkLineRepresentation* lineRepresentation : this->LineRepresentationVector)
  {
    mTime = std::max(mTime, lineRepresentation->GetMTime());
  }

  return mTime;
}

//------------------------------------------------------------------------------
void vtkMultiLineRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Line Count : " << this->LineCount << '\n';
  for (int i = 0; i < this->LineCount; i++)
  {
    os << indent << "Line " << i << ":" << '\n';
    this->LineRepresentationVector[i]->PrintSelf(os, indent);
  }
  os << "\n";
}
VTK_ABI_NAMESPACE_END
