/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCameraPathRepresentation.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCameraPathRepresentation.h"

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkCameraHandleSource.h"
#include "vtkCellPicker.h"
#include "vtkParametricFunctionSource.h"
#include "vtkParametricSpline.h"
#include "vtkPolyDataMapper.h"
#include "vtkVectorOperators.h"

constexpr double MINIMAL_SIZE_OFFSET = 0.001;

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkCameraPathRepresentation);

//------------------------------------------------------------------------------
vtkCameraPathRepresentation::vtkCameraPathRepresentation()
{
  // display cameras in directional mode (with arrows)
  this->SetDirectional(true);
  this->SetNumberOfHandles(5);
  this->HandlePicker->PickFromListOn();
}

//------------------------------------------------------------------------------
void vtkCameraPathRepresentation::SetDirectional(bool val)
{
  if (this->Directional == val)
  {
    return;
  }

  this->Directional = val;
  this->Modified();

  for (int i = 0; i < this->NumberOfHandles; ++i)
  {
    this->CameraHandles[i]->SetDirectional(this->Directional);
    this->CameraHandles[i]->Update();
  }
}

//------------------------------------------------------------------------------
void vtkCameraPathRepresentation::BuildRepresentation()
{
  if (this->NumberOfHandles < 1)
  {
    return;
  }
  this->ValidPick = 1;
  // change spline's number of points
  vtkPoints* points = this->ParametricSpline->GetPoints();
  if (points->GetNumberOfPoints() != this->NumberOfHandles)
  {
    points->SetNumberOfPoints(this->NumberOfHandles);
  }

  vtkBoundingBox bbox;
  for (int i = 0; i < this->NumberOfHandles; ++i)
  {
    double pt[3];
    this->CameraHandles[i]->GetPosition(pt);
    points->SetPoint(i, pt);
    bbox.AddPoint(pt);
  }
  this->ParametricSpline->SetClosed(this->Closed);
  this->ParametricSpline->Modified();

  this->ParametricFunctionSource->Update();

  double bounds[6];
  bbox.GetBounds(bounds);
  this->InitialLength = sqrt((bounds[1] - bounds[0]) * (bounds[1] - bounds[0]) +
    (bounds[3] - bounds[2]) * (bounds[3] - bounds[2]) +
    (bounds[5] - bounds[4]) * (bounds[5] - bounds[4]));
  this->SizeHandles();
}

//------------------------------------------------------------------------------
void vtkCameraPathRepresentation::AddCameraAt(vtkCamera* camera, int index)
{
  if (index < 0 || index > this->NumberOfHandles || camera == nullptr)
  {
    vtkErrorMacro(<< "ERROR: Invalid index or nullptr camera\n");
    return;
  }

  this->InsertCamera(camera, index);

  this->UpdateConfiguration(this->NumberOfHandles + 1);
}

//------------------------------------------------------------------------------
void vtkCameraPathRepresentation::InsertCamera(vtkCamera* camera, int index)
{
  if (index < 0 || (size_t)index > this->CameraHandles.size() || camera == nullptr)
  {
    vtkErrorMacro(<< "ERROR: Invalid index or nullptr camera\n");
    return;
  }

  vtkSmartPointer<vtkCameraHandleSource> camHandle = vtkSmartPointer<vtkCameraHandleSource>::New();
  camHandle->SetDirectional(this->Directional);
  camHandle->SetCamera(camera);
  this->CameraHandles.insert(this->CameraHandles.begin() + index, camHandle);

  vtkSmartPointer<vtkActor> handleActor = vtkSmartPointer<vtkActor>::New();
  this->HandleActors.insert(this->HandleActors.begin() + index, handleActor);

  vtkNew<vtkPolyDataMapper> handleMapper;
  handleMapper->SetInputConnection(camHandle->GetOutputPort());
  handleActor->SetMapper(handleMapper);
  handleActor->SetProperty(this->HandleProperty);
  this->HandlePicker->AddPickList(handleActor);
}

//------------------------------------------------------------------------------
void vtkCameraPathRepresentation::DeleteCameraAt(int index)
{
  if (index < 0 || index >= this->NumberOfHandles)
  {
    vtkErrorMacro(<< "ERROR: Invalid index\n");
    return;
  }

  this->CameraHandles.erase(this->CameraHandles.begin() + index);
  vtkActor* handleActor = this->HandleActors.at(index);
  this->HandlePicker->DeletePickList(handleActor);
  this->HandleActors.erase(this->HandleActors.begin() + index);

  this->UpdateConfiguration(this->NumberOfHandles - 1);
}

//------------------------------------------------------------------------------
void vtkCameraPathRepresentation::SetNumberOfHandles(int npts)
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
    this->ClearCameraHandles();
    this->NumberOfHandles = 0;
    this->CleanRepresentation();
    vtkGenericWarningMacro(
      << "vtkCameraPathRepresentation: there is not any camera handle defined at the moment.");
    return;
  }

  // Ensure no handle is highlighted
  this->HighlightHandle(nullptr);

  if (this->GetParametricSpline() && this->NumberOfHandles > 1)
  {
    this->ReconfigureHandles(npts, this->NumberOfHandles);
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
void vtkCameraPathRepresentation::SetParametricSpline(vtkParametricSpline* spline)
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
  this->ReconfigureHandles(npts, this->NumberOfHandles);
  this->NumberOfHandles = npts;
  this->RebuildRepresentation();
}

//------------------------------------------------------------------------------
void vtkCameraPathRepresentation::CreateDefaultHandles(int npts)
{
  this->ClearCameraHandles();

  vtkNew<vtkPoints> points;
  points->SetDataType(VTK_DOUBLE);
  points->SetNumberOfPoints(npts);

  if (npts == 1)
  {
    points->SetPoint(0, 0, 0, 0);
    vtkNew<vtkCamera> cam;
    cam->SetPosition(0, 0, 0);
    cam->SetFocalPoint(1, 0, 0);
    this->InsertCamera(cam, 0);
  }
  else
  {
    // Create the camera handles along a straight line within the bounds of a unit cube
    double x0, y0, z0;
    x0 = y0 = z0 = -0.5;
    double x1, y1, z1;
    x1 = y1 = z1 = 0.5;

    for (int i = 0; i < npts; ++i)
    {
      double u = i / (5 - 1.0);
      double x = (1.0 - u) * x0 + u * x1;
      double y = (1.0 - u) * y0 + u * y1;
      double z = (1.0 - u) * z0 + u * z1;
      vtkNew<vtkCamera> cam;
      cam->SetPosition(x, y, z);
      cam->SetFocalPoint(x + 1, y, z);
      this->InsertCamera(cam, i);
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
void vtkCameraPathRepresentation::ReconfigureHandles(int newNPts, int oldNPts)
{
  // get old focal points
  vtkNew<vtkPoints> points;
  points->SetDataType(VTK_DOUBLE);
  points->SetNumberOfPoints(oldNPts);
  for (int i = 0; i < oldNPts; ++i)
  {
    double pt[3];
    this->CameraHandles[i]->GetDirection(pt);
    points->SetPoint(i, pt);
  }
  vtkNew<vtkParametricSpline> focalPointsSpline;
  focalPointsSpline->SetPoints(points);

  this->ClearCameraHandles();

  double u[3], pt[3];

  if (newNPts == 1)
  {
    vtkNew<vtkCamera> cam;
    // take the point at the middle.
    u[0] = 0.5;
    this->ParametricSpline->Evaluate(u, pt, nullptr);
    cam->SetPosition(pt[0], pt[1], pt[2]);
    focalPointsSpline->Evaluate(u, pt, nullptr);
    cam->SetFocalPoint(pt[0], pt[1], pt[2]);
    this->InsertCamera(cam, 0);
  }
  else
  {
    for (int i = 0; i < newNPts; ++i)
    {
      vtkNew<vtkCamera> cam;
      u[0] = i / (newNPts - 1.0);
      this->ParametricSpline->Evaluate(u, pt, nullptr);
      cam->SetPosition(pt[0], pt[1], pt[2]);
      focalPointsSpline->Evaluate(u, pt, nullptr);
      cam->SetFocalPoint(pt[0], pt[1], pt[2]);
      this->InsertCamera(cam, i);
    }
  }
}

//------------------------------------------------------------------------------
void vtkCameraPathRepresentation::RebuildRepresentation()
{
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
void vtkCameraPathRepresentation::UpdateConfiguration(int npts)
{
  if (this->NumberOfHandles == npts)
  {
    return;
  }

  if (npts < 0)
  {
    return;
  }

  if (npts == 0)
  {
    this->NumberOfHandles = 0;
    this->CleanRepresentation();
    return;
  }

  // Ensure no handle is highlighted
  this->HighlightHandle(nullptr);

  // in case there was no spline before, reallocate it
  if (!this->GetParametricSpline())
  {
    vtkNew<vtkPoints> points;
    points->SetDataType(VTK_DOUBLE);
    points->SetNumberOfPoints(npts);
    vtkNew<vtkParametricSpline> spline;
    spline->SetPoints(points);
    this->SetParametricSplineInternal(spline);
    this->LineMapper->SetInputConnection(this->ParametricFunctionSource->GetOutputPort());
  }

  this->NumberOfHandles = npts;

  this->RebuildRepresentation();
}

//------------------------------------------------------------------------------
int vtkCameraPathRepresentation::InsertHandleOnLine(double* pos)
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

  int istart = vtkMath::Floor(
    subid * (this->NumberOfHandles + this->Closed - 1.0) / static_cast<double>(this->Resolution));

  istart++;

  // insert new camera
  vtkNew<vtkCamera> cam;
  cam->SetPosition(pos);

  // calculate the new focal point
  vtkVector3d focusPoint1(this->CameraHandles[istart - 1]->GetDirection());
  vtkVector3d focusPoint2(this->CameraHandles[istart % this->NumberOfHandles]->GetDirection());
  focusPoint1 = (focusPoint1 + focusPoint2) / vtkVector3d(2.0);
  cam->SetFocalPoint(focusPoint1.GetData());

  this->AddCameraAt(cam, istart);
  return istart;
}

//------------------------------------------------------------------------------
void vtkCameraPathRepresentation::EraseHandle(const int& index)
{
  if (this->NumberOfHandles < 3 || index < 0 || index >= this->NumberOfHandles)
  {
    return;
  }
  this->DeleteCameraAt(index);
}

//------------------------------------------------------------------------------
void vtkCameraPathRepresentation::ClearCameraHandles()
{
  for (vtkActor* actor : this->HandleActors)
  {
    this->HandlePicker->DeletePickList(actor);
  }
  this->HandleActors.clear();
  this->CameraHandles.clear();
}

//------------------------------------------------------------------------------
vtkActor* vtkCameraPathRepresentation::GetHandleActor(int index)
{
  if (index < 0 || index >= this->NumberOfHandles)
  {
    return nullptr;
  }
  return this->HandleActors[index];
}

//------------------------------------------------------------------------------
vtkHandleSource* vtkCameraPathRepresentation::GetHandleSource(int index)
{
  if (index < 0 || index >= this->NumberOfHandles)
  {
    return nullptr;
  }
  return this->CameraHandles[index];
}

//------------------------------------------------------------------------------
int vtkCameraPathRepresentation::GetHandleIndex(vtkProp* prop)
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
void vtkCameraPathRepresentation::InitializeHandles(vtkPoints* points)
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
void vtkCameraPathRepresentation::SizeHandles()
{
  if (this->NumberOfHandles > 0)
  {
    for (int i = 0; i < this->NumberOfHandles; ++i)
    {
      double width = this->SizeHandlesInPixels(3, CameraHandles[i]->GetPosition());
      double oldwidth = CameraHandles[i]->GetSize();
      // avoid size recalculations if the new size offset is very small
      if (std::abs(oldwidth - width) > MINIMAL_SIZE_OFFSET)
      {
        this->CameraHandles[i]->SetSize(width);
        this->CameraHandles[i]->Update();
      }
    }
  }
}

//------------------------------------------------------------------------------
void vtkCameraPathRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  for (int i = 0; i < this->NumberOfHandles; ++i)
  {
    os << indent << "CameraHandle " << i << ": (" << this->CameraHandles[i] << "\n";
    this->CameraHandles[i]->PrintSelf(os, indent.GetNextIndent());
    os << indent << ")\n";
  }
}
