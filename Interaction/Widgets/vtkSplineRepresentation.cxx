// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSplineRepresentation.h"

#include "vtkCellPicker.h"
#include "vtkParametricFunctionSource.h"
#include "vtkParametricSpline.h"
#include "vtkPointHandleSource.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkRenderer.h"
#include "vtkVectorOperators.h"

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkSplineRepresentation);

//------------------------------------------------------------------------------
vtkSplineRepresentation::vtkSplineRepresentation()
{
  // Allocate 5 handles by default
  this->SetNumberOfHandles(5);
  this->HandlePicker->PickFromListOn();
}

//------------------------------------------------------------------------------
vtkSplineRepresentation::~vtkSplineRepresentation()
{
  this->ClearHandles();
}

//------------------------------------------------------------------------------
void vtkSplineRepresentation::BuildRepresentation()
{
  if (this->NumberOfHandles < 1)
  {
    return;
  }
  this->ValidPick = 1;
  // TODO: Avoid unnecessary rebuilds.
  // Handles have changed position, re-compute the spline coeffs
  vtkPoints* points = this->ParametricSpline->GetPoints();
  if (points->GetNumberOfPoints() != this->NumberOfHandles)
  {
    points->SetNumberOfPoints(this->NumberOfHandles);
  }

  vtkBoundingBox bbox;
  for (int i = 0; i < this->NumberOfHandles; ++i)
  {
    double pt[3];
    this->PointHandles[i]->GetPosition(pt);
    points->SetPoint(i, pt);
    bbox.AddPoint(pt);
  }
  this->ParametricSpline->SetClosed(this->Closed);
  this->ParametricSpline->Modified();

  this->ParametricFunctionSource->Update();

  // Update end arrow direction
  if (this->Directional && this->NumberOfHandles >= 2)
  {
    vtkVector3d pt1, pt2;
    vtkIdType npts = this->ParametricFunctionSource->GetOutput()->GetNumberOfPoints();
    this->ParametricFunctionSource->GetOutput()->GetPoint(npts - 1, pt1.GetData());
    this->ParametricFunctionSource->GetOutput()->GetPoint(npts - 2, pt2.GetData());
    pt1 = pt1 - pt2;
    this->PointHandles[this->NumberOfHandles - 1]->SetDirection(pt1.GetData());
  }

  double bounds[6];
  bbox.GetBounds(bounds);
  this->InitialLength = sqrt((bounds[1] - bounds[0]) * (bounds[1] - bounds[0]) +
    (bounds[3] - bounds[2]) * (bounds[3] - bounds[2]) +
    (bounds[5] - bounds[4]) * (bounds[5] - bounds[4]));
  this->SizeHandles();
}

//------------------------------------------------------------------------------
void vtkSplineRepresentation::RebuildRepresentation()
{
  if (this->Directional && this->NumberOfHandles >= 2)
  {
    this->PointHandles[this->NumberOfHandles - 1]->SetDirectional(true);
  }

  if (this->CurrentHandleIndex >= 0 && this->CurrentHandleIndex < this->NumberOfHandles)
  {
    this->CurrentHandleIndex = this->HighlightHandle(this->HandleActors[this->CurrentHandleIndex]);
  }
  else
  {
    this->CurrentHandleIndex = this->HighlightHandle(nullptr);
  }

  this->BuildRepresentation();
}

//------------------------------------------------------------------------------
void vtkSplineRepresentation::SetNumberOfHandles(int npts)
{
  if (this->NumberOfHandles == npts)
  {
    return;
  }

  if (npts < 0)
  {
    vtkErrorMacro(<< "ERROR: Invalid npts, must be >= 0\n");
    return;
  }

  if (npts == 0)
  {
    this->ClearHandles();
    this->NumberOfHandles = 0;
    this->CleanRepresentation();
    vtkGenericWarningMacro(
      << "vtkSplineRepresentation: there is not any point defined at the moment.");
    return;
  }

  // Ensure no handle is highlighted
  this->HighlightHandle(nullptr);

  if (this->GetParametricSpline() && this->NumberOfHandles > 1)
  {
    this->ReconfigureHandles(npts);
  }
  else
  {
    // reallocate the handles
    this->CreateDefaultHandles(npts);
  }

  this->NumberOfHandles = npts;

  this->RebuildRepresentation();
}

//------------------------------------------------------------------------------
void vtkSplineRepresentation::SetParametricSpline(vtkParametricSpline* spline)
{
  this->SetParametricSplineInternal(spline);

  if (!spline || !spline->GetPoints() || spline->GetPoints()->GetNumberOfPoints() < 1)
  {
    this->SetNumberOfHandles(0);
    return;
  }

  // Ensure no handle is highlighted
  this->HighlightHandle(nullptr);
  int npts = spline->GetPoints()->GetNumberOfPoints();
  this->ReconfigureHandles(npts);
  this->NumberOfHandles = npts;
  this->RebuildRepresentation();
}

//------------------------------------------------------------------------------
int vtkSplineRepresentation::InsertHandleOnLine(double* pos)
{
  if (this->NumberOfHandles < 2 || pos == nullptr)
  {
    return -1;
  }

  vtkIdType id = this->LinePicker->GetCellId();
  if (id == -1)
  {
    return -1;
  }

  vtkIdType subid = this->LinePicker->GetSubId();

  vtkNew<vtkPoints> newpoints;
  newpoints->SetDataType(VTK_DOUBLE);
  newpoints->SetNumberOfPoints(this->NumberOfHandles + 1);

  int istart = vtkMath::Floor(
    subid * (this->NumberOfHandles + this->Closed - 1.0) / static_cast<double>(this->Resolution));
  int istop = istart + 1;
  int count = 0;
  for (int i = 0; i <= istart; ++i)
  {
    newpoints->SetPoint(count++, this->PointHandles[i]->GetPosition());
  }

  const int insert_index = count;
  newpoints->SetPoint(count++, pos);

  for (int i = istop; i < this->NumberOfHandles; ++i)
  {
    newpoints->SetPoint(count++, this->PointHandles[i]->GetPosition());
  }

  this->InitializeHandles(newpoints);

  return insert_index;
}

//------------------------------------------------------------------------------
void vtkSplineRepresentation::ClearHandles()
{
  for (vtkActor* actor : this->HandleActors)
  {
    this->HandlePicker->DeletePickList(actor);
  }
  this->HandleActors.clear();
  this->PointHandles.clear();
}

//------------------------------------------------------------------------------
void vtkSplineRepresentation::AllocateHandles(int npts)
{
  if (npts == this->NumberOfHandles)
  {
    return;
  }

  this->ClearHandles();

  for (int h = 0; h < npts; h++)
  {
    vtkSmartPointer<vtkPointHandleSource> pointHandle =
      vtkSmartPointer<vtkPointHandleSource>::New();
    vtkSmartPointer<vtkActor> handleActor = vtkSmartPointer<vtkActor>::New();
    vtkNew<vtkPolyDataMapper> handleMapper;
    handleMapper->SetInputConnection(pointHandle->GetOutputPort());
    handleActor->SetMapper(handleMapper);
    handleActor->SetProperty(this->HandleProperty);
    this->HandlePicker->AddPickList(handleActor);

    this->PointHandles.push_back(pointHandle);
    this->HandleActors.push_back(handleActor);
  }
}

//------------------------------------------------------------------------------
void vtkSplineRepresentation::CreateDefaultHandles(int npts)
{
  this->AllocateHandles(npts);

  vtkNew<vtkPoints> points;
  points->SetDataType(VTK_DOUBLE);
  points->SetNumberOfPoints(npts);

  if (npts == 1)
  {
    points->SetPoint(0, 0, 0, 0);
    this->PointHandles[0]->SetPosition(0, 0, 0);
  }
  else
  {
    // Create the handles along a straight line within the bounds of a unit cube
    double x0, y0, z0;
    x0 = y0 = z0 = -0.5;
    double x1, y1, z1;
    x1 = y1 = z1 = 0.5;

    for (int i = 0; i < npts; ++i)
    {
      double u = i / (npts - 1.0);
      double x = (1.0 - u) * x0 + u * x1;
      double y = (1.0 - u) * y0 + u * y1;
      double z = (1.0 - u) * z0 + u * z1;
      points->SetPoint(i, x, y, z);
      this->PointHandles[i]->SetPosition(x, y, z);
    }
  }

  if (!this->GetParametricSpline())
  {
    vtkNew<vtkParametricSpline> spline;
    spline->SetPoints(points);
    this->SetParametricSplineInternal(spline);
    this->LineMapper->SetInputConnection(this->ParametricFunctionSource->GetOutputPort());
  }
  else
  {
    this->GetParametricSpline()->SetPoints(points);
  }
}

//------------------------------------------------------------------------------
void vtkSplineRepresentation::ReconfigureHandles(int npts)
{
  this->AllocateHandles(npts);

  double u[3], pt[3];

  if (npts == 1)
  {
    // take the point at the middle.
    u[0] = 0.5;
    this->ParametricSpline->Evaluate(u, pt, nullptr);
    this->PointHandles[0]->SetPosition(pt[0], pt[1], pt[2]);
  }
  else
  {
    for (int i = 0; i < npts; ++i)
    {
      u[0] = i / (npts - 1.0);
      this->ParametricSpline->Evaluate(u, pt, nullptr);
      this->PointHandles[i]->SetPosition(pt[0], pt[1], pt[2]);
    }
  }
}

//------------------------------------------------------------------------------
vtkActor* vtkSplineRepresentation::GetHandleActor(int index)
{
  if (index < 0 || index >= this->NumberOfHandles)
  {
    return nullptr;
  }
  return this->HandleActors[index];
}

//------------------------------------------------------------------------------
vtkHandleSource* vtkSplineRepresentation::GetHandleSource(int index)
{
  if (index < 0 || index >= this->NumberOfHandles)
  {
    return nullptr;
  }
  return this->PointHandles[index];
}

//------------------------------------------------------------------------------
int vtkSplineRepresentation::GetHandleIndex(vtkProp* prop)
{
  if (!prop)
  {
    return -1;
  }

  auto iter = std::find(this->HandleActors.begin(),
    this->HandleActors.begin() + this->NumberOfHandles, static_cast<vtkActor*>(prop));
  return (iter != this->HandleActors.begin() + NumberOfHandles)
    ? static_cast<int>(std::distance(this->HandleActors.begin(), iter))
    : -1;
}

//------------------------------------------------------------------------------
void vtkSplineRepresentation::InitializeHandles(vtkPoints* points)
{
  if (!points)
  {
    vtkErrorMacro(<< "ERROR: Invalid or nullptr points\n");
    return;
  }

  int npts = points->GetNumberOfPoints();
  if (npts < 2)
  {
    return;
  }

  double p0[3];
  double p1[3];

  points->GetPoint(0, p0);
  points->GetPoint(npts - 1, p1);

  if (vtkMath::Distance2BetweenPoints(p0, p1) == 0.0)
  {
    --npts;
    this->Closed = 1;
    this->ParametricSpline->ClosedOn();
  }

  this->SetNumberOfHandles(npts);
  for (int i = 0; i < npts; ++i)
  {
    this->SetHandlePosition(i, points->GetPoint(i));
  }
}

//------------------------------------------------------------------------------
void vtkSplineRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  for (int i = 0; i < this->NumberOfHandles; ++i)
  {
    os << indent << "PointHandle " << i << ": (" << this->PointHandles[i] << "\n";
    this->PointHandles[i]->PrintSelf(os, indent.GetNextIndent());
    os << indent << ")\n";
  }
}
VTK_ABI_NAMESPACE_END
