// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPolyLineRepresentation.h"

#include "vtkActor.h"
#include "vtkCellPicker.h"
#include "vtkDoubleArray.h"
#include "vtkPointHandleSource.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkPolyLineSource.h"
#include "vtkVectorOperators.h"

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkPolyLineRepresentation);

//------------------------------------------------------------------------------
vtkPolyLineRepresentation::vtkPolyLineRepresentation()
{
  vtkNew<vtkPolyDataMapper> lineMapper;
  lineMapper->SetInputConnection(this->PolyLineSource->GetOutputPort());
  lineMapper->SetResolveCoincidentTopologyToPolygonOffset();
  this->LineActor->SetMapper(lineMapper);

  this->SetNumberOfHandles(5);

  this->HandlePicker->PickFromListOn();
}

//------------------------------------------------------------------------------
vtkPolyLineRepresentation::~vtkPolyLineRepresentation()
{
  this->ClearHandles();
}

//------------------------------------------------------------------------------
vtkDoubleArray* vtkPolyLineRepresentation::GetHandlePositions()
{
  return vtkArrayDownCast<vtkDoubleArray>(this->PolyLineSource->GetPoints()->GetData());
}

//------------------------------------------------------------------------------
void vtkPolyLineRepresentation::BuildRepresentation()
{
  this->ValidPick = 1;
  // TODO: Avoid unnecessary rebuilds.
  // Handles have changed position, re-compute the points
  vtkPoints* points = this->PolyLineSource->GetPoints();
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
  this->PolyLineSource->SetClosed(this->Closed);
  this->PolyLineSource->Modified();
  points->Modified();

  // Update end arrow direction
  if (this->Directional && this->NumberOfHandles >= 2)
  {
    vtkVector3d pt1, pt2;
    this->PointHandles[this->NumberOfHandles - 1]->GetPosition(pt1.GetData());
    this->PointHandles[this->NumberOfHandles - 2]->GetPosition(pt2.GetData());
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
void vtkPolyLineRepresentation::SetNumberOfHandles(int npts)
{
  if (this->NumberOfHandles == npts)
  {
    return;
  }
  if (npts < 1)
  {
    vtkGenericWarningMacro(<< "vtkPolyLineRepresentation: minimum of 1 points required.");
    return;
  }

  // Ensure that no handle is current
  this->HighlightHandle(nullptr);

  if (this->PolyLineSource->GetPoints())
  {
    this->ReconfigureHandles(npts);
  }
  else
  {
    // allocate the handles
    this->CreateDefaultHandles(npts);
  }

  this->NumberOfHandles = npts;

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
void vtkPolyLineRepresentation::ClearHandles()
{
  for (vtkActor* actor : this->HandleActors)
  {
    this->HandlePicker->DeletePickList(actor);
  }
  this->HandleActors.clear();
  this->PointHandles.clear();
}

//------------------------------------------------------------------------------
void vtkPolyLineRepresentation::AllocateHandles(int npts)
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
void vtkPolyLineRepresentation::CreateDefaultHandles(int npts)
{
  this->AllocateHandles(npts);

  // Default bounds to get started
  double x0, y0, z0;
  x0 = y0 = z0 = -0.5;
  double x1, y1, z1;
  x1 = y1 = z1 = 0.5;

  vtkNew<vtkPoints> points;
  points->SetDataType(VTK_DOUBLE);
  points->SetNumberOfPoints(npts);

  for (int i = 0; i < npts; ++i)
  {
    double u = i / (npts - 1.0);
    double x = (1.0 - u) * x0 + u * x1;
    double y = (1.0 - u) * y0 + u * y1;
    double z = (1.0 - u) * z0 + u * z1;
    points->SetPoint(i, x, y, z);
    this->PointHandles[i]->SetPosition(x, y, z);
  }

  this->PolyLineSource->SetPoints(points);
  this->PolyLineSource->Update();
}

//------------------------------------------------------------------------------
void vtkPolyLineRepresentation::ReconfigureHandles(int npts)
{
  vtkIdType prevNumPoints = this->PolyLineSource->GetNumberOfPoints();
  if (this->PolyLineSource->GetNumberOfPoints() != npts)
  {
    this->PolyLineSource->Resize(npts);
    for (vtkIdType i = prevNumPoints; i < npts; ++i)
    {
      double pt[3] = { 0.0, 0.0, 0.0 };
      this->PolyLineSource->GetPoints()->SetPoint(i, pt);
    }
  }

  this->AllocateHandles(npts);

  double pt[3];

  for (int i = 0; i < npts; ++i)
  {
    this->PolyLineSource->GetPoints()->GetPoint(i, pt);
    this->PointHandles[i]->SetPosition(pt);
  }
}

//------------------------------------------------------------------------------
vtkActor* vtkPolyLineRepresentation::GetHandleActor(int index)
{
  return this->HandleActors[index];
}

//------------------------------------------------------------------------------
vtkHandleSource* vtkPolyLineRepresentation::GetHandleSource(int index)
{
  return this->PointHandles[index];
}

//------------------------------------------------------------------------------
int vtkPolyLineRepresentation::GetHandleIndex(vtkProp* prop)
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
void vtkPolyLineRepresentation::GetPolyData(vtkPolyData* pd)
{
  this->PolyLineSource->Update();
  pd->ShallowCopy(this->PolyLineSource->GetOutput());
}

//------------------------------------------------------------------------------
double vtkPolyLineRepresentation::GetSummedLength()
{
  vtkPoints* points = this->PolyLineSource->GetOutput()->GetPoints();
  int npts = points->GetNumberOfPoints();

  if (npts < 2)
  {
    return 0.0;
  }

  double a[3];
  double b[3];
  double sum = 0.0;
  int i = 0;
  points->GetPoint(i, a);
  int imax = (npts % 2 == 0) ? npts - 2 : npts - 1;

  while (i < imax)
  {
    points->GetPoint(i + 1, b);
    sum += sqrt(vtkMath::Distance2BetweenPoints(a, b));
    i = i + 2;
    points->GetPoint(i, a);
    sum = sum + sqrt(vtkMath::Distance2BetweenPoints(a, b));
  }

  if (npts % 2 == 0)
  {
    points->GetPoint(i + 1, b);
    sum += sqrt(vtkMath::Distance2BetweenPoints(a, b));
  }

  return sum;
}

//------------------------------------------------------------------------------
int vtkPolyLineRepresentation::InsertHandleOnLine(double* pos)
{
  if (this->NumberOfHandles < 2)
  {
    return -1;
  }

  vtkIdType id = this->LinePicker->GetCellId();

  vtkNew<vtkPoints> newpoints;
  newpoints->SetDataType(VTK_DOUBLE);
  newpoints->SetNumberOfPoints(this->NumberOfHandles + 1);

  int insert_index = -1;
  if (id == -1)
  {
    // not on a line, add to the end
    for (int i = 0; i < this->NumberOfHandles; ++i)
    {
      newpoints->SetPoint(i, this->PointHandles[i]->GetPosition());
    }
    newpoints->SetPoint(this->NumberOfHandles, pos);
    insert_index = this->NumberOfHandles;
  }
  else
  {
    vtkIdType subid = this->LinePicker->GetSubId();

    int istart = subid;
    int istop = istart + 1;
    int count = 0;
    for (int i = 0; i <= istart; ++i)
    {
      newpoints->SetPoint(count++, this->PointHandles[i]->GetPosition());
    }

    insert_index = count;
    newpoints->SetPoint(count++, pos);

    for (int i = istop; i < this->NumberOfHandles; ++i)
    {
      newpoints->SetPoint(count++, this->PointHandles[i]->GetPosition());
    }
  }

  this->InitializeHandles(newpoints);

  return insert_index;
}

//------------------------------------------------------------------------------
void vtkPolyLineRepresentation::InitializeHandles(vtkPoints* points)
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
    this->PolyLineSource->ClosedOn();
  }

  this->SetNumberOfHandles(npts);
  for (int i = 0; i < npts; ++i)
  {
    this->SetHandlePosition(i, points->GetPoint(i));
  }
}

//------------------------------------------------------------------------------
void vtkPolyLineRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "PolyLineSource: ";
  if (this->PolyLineSource)
  {
    this->PolyLineSource->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << "(none)\n";
  }
}
VTK_ABI_NAMESPACE_END
