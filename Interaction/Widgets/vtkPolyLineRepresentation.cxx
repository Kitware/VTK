/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolyLineRepresentation.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPolyLineRepresentation.h"

#include "vtkActor.h"
#include "vtkBoundingBox.h"
#include "vtkCellPicker.h"
#include "vtkDoubleArray.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkPolyLineSource.h"
#include "vtkSphereSource.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPolyLineRepresentation);

//----------------------------------------------------------------------------
vtkPolyLineRepresentation::vtkPolyLineRepresentation()
{
  // Build the representation of the widget

  // Default bounds to get started
  double x0 = -0.5;
  double x1 =  0.5;
  double y0 = -0.5;
  double y1 =  0.5;
  double z0 = -0.5;
  double z1 =  0.5;

  vtkPoints* points = vtkPoints::New(VTK_DOUBLE);
  points->SetNumberOfPoints(this->NumberOfHandles);

  for ( int i = 0; i < this->NumberOfHandles; ++i )
  {
    double u = i / (this->NumberOfHandles - 1.0);
    double x = (1.0 - u)*x0 + u*x1;
    double y = (1.0 - u)*y0 + u*y1;
    double z = (1.0 - u)*z0 + u*z1;
    points->SetPoint(i, x, y, z);
    this->HandleGeometry[i]->SetCenter(x,y,z);
  }

  this->PolyLineSource = vtkPolyLineSource::New();
  this->PolyLineSource->SetPoints(points);
  points->Delete();
  this->PolyLineSource->Update();

  vtkPolyDataMapper* lineMapper = vtkPolyDataMapper::New();
  lineMapper->SetInputConnection(
    this->PolyLineSource->GetOutputPort()) ;
  lineMapper->ImmediateModeRenderingOn();
  lineMapper->SetResolveCoincidentTopologyToPolygonOffset();

  this->LineActor->SetMapper( lineMapper );
  lineMapper->Delete();
}

//----------------------------------------------------------------------------
vtkPolyLineRepresentation::~vtkPolyLineRepresentation()
{
  this->PolyLineSource->Delete();
}

//----------------------------------------------------------------------------
vtkDoubleArray* vtkPolyLineRepresentation::GetHandlePositions()
{
  return vtkArrayDownCast<vtkDoubleArray>(
    this->PolyLineSource->GetPoints()->GetData());
}

//----------------------------------------------------------------------------
void vtkPolyLineRepresentation::BuildRepresentation()
{
  this->ValidPick = 1;
  // TODO: Avoid unnecessary rebuilds.
  // Handles have changed position, re-compute the points
  vtkPoints* points = this->PolyLineSource->GetPoints();
  if ( points->GetNumberOfPoints() != this->NumberOfHandles )
  {
    points->SetNumberOfPoints( this->NumberOfHandles );
  }

  vtkBoundingBox bbox;
  for ( int i = 0; i < this->NumberOfHandles; ++i )
  {
    double pt[3];
    this->HandleGeometry[i]->GetCenter(pt);
    points->SetPoint(i, pt);
    bbox.AddPoint(pt);
  }
  this->PolyLineSource->SetClosed(this->Closed);
  this->PolyLineSource->Modified();

  double bounds[6];
  bbox.GetBounds(bounds);
  this->InitialLength = sqrt((bounds[1]-bounds[0])*(bounds[1]-bounds[0]) +
                             (bounds[3]-bounds[2])*(bounds[3]-bounds[2]) +
                             (bounds[5]-bounds[4])*(bounds[5]-bounds[4]));
  this->SizeHandles();
}
//----------------------------------------------------------------------------
void vtkPolyLineRepresentation::SetNumberOfHandles(int npts)
{
  if ( this->NumberOfHandles == npts )
  {
    return;
  }
  if (npts < 1)
  {
    vtkGenericWarningMacro(<<"vtkPolyLineRepresentation: minimum of 1 points required.");
    return;
  }

  // Ensure that no handle is current
  this->HighlightHandle(NULL);

  double radius = this->HandleGeometry[0]->GetRadius();
  this->Initialize();

  this->NumberOfHandles = npts;

  vtkIdType prevNumPoints = this->PolyLineSource->GetNumberOfPoints();
  if (this->PolyLineSource->GetNumberOfPoints() != npts)
  {
    this->PolyLineSource->Resize(npts);
    for (vtkIdType i = prevNumPoints; i < npts; ++i)
    {
      double pt[3] = {0.0, 0.0, 0.0};
      this->PolyLineSource->GetPoints()->SetPoint(i, pt);
    }
  }

  // Create the handles
  this->Handle         = new vtkActor* [this->NumberOfHandles];
  this->HandleGeometry = new vtkSphereSource* [this->NumberOfHandles];

  for ( int i = 0; i < this->NumberOfHandles; ++i )
  {
    this->HandleGeometry[i] = vtkSphereSource::New();
    this->HandleGeometry[i]->SetThetaResolution(16);
    this->HandleGeometry[i]->SetPhiResolution(8);
    vtkPolyDataMapper* handleMapper = vtkPolyDataMapper::New();
    handleMapper->SetInputConnection(
      this->HandleGeometry[i]->GetOutputPort());
    this->Handle[i] = vtkActor::New();
    this->Handle[i]->SetMapper(handleMapper);
    handleMapper->Delete();
    this->Handle[i]->SetProperty(this->HandleProperty);
    double pt[3];
    this->PolyLineSource->GetPoints()->GetPoint(i, pt);
    this->HandleGeometry[i]->SetCenter(pt);
    this->HandleGeometry[i]->SetRadius(radius);
    this->HandlePicker->AddPickList(this->Handle[i]);
  }

  if (this->CurrentHandleIndex >= 0 &&
    this->CurrentHandleIndex < this->NumberOfHandles)
  {
    this->CurrentHandleIndex =
      this->HighlightHandle(this->Handle[this->CurrentHandleIndex]);
  }
  else
  {
    this->CurrentHandleIndex = this->HighlightHandle(NULL);
  }

  this->BuildRepresentation();
}

//----------------------------------------------------------------------------
void vtkPolyLineRepresentation::GetPolyData(vtkPolyData *pd)
{
  pd->ShallowCopy( this->PolyLineSource->GetOutput() );
}


//----------------------------------------------------------------------------
double vtkPolyLineRepresentation::GetSummedLength()
{
  vtkPoints* points = this->PolyLineSource->GetOutput()->GetPoints();
  int npts = points->GetNumberOfPoints();

  if ( npts < 2 ) { return 0.0; }

  double a[3];
  double b[3];
  double sum = 0.0;
  int i = 0;
  points->GetPoint(i, a);
  int imax = (npts % 2 == 0) ? npts-2 : npts-1;

  while ( i < imax )
  {
    points->GetPoint(i+1, b);
    sum += sqrt(vtkMath::Distance2BetweenPoints(a, b));
    i = i + 2;
    points->GetPoint(i, a);
    sum = sum + sqrt(vtkMath::Distance2BetweenPoints(a, b));
  }

  if ( npts % 2 == 0 )
  {
    points->GetPoint(i+1, b);
    sum += sqrt(vtkMath::Distance2BetweenPoints(a, b));
  }

  return sum;
}

//----------------------------------------------------------------------------
void vtkPolyLineRepresentation::InsertHandleOnLine(double* pos)
{
  if (this->NumberOfHandles < 2) { return; }

  vtkIdType id = this->LinePicker->GetCellId();
  if (id == -1){ return; }

  vtkIdType subid = this->LinePicker->GetSubId();

  vtkPoints* newpoints = vtkPoints::New(VTK_DOUBLE);
  newpoints->SetNumberOfPoints(this->NumberOfHandles+1);

  int istart = subid;
  int istop = istart + 1;
  int count = 0;
  for ( int i = 0; i <= istart; ++i )
  {
    newpoints->SetPoint(count++,this->HandleGeometry[i]->GetCenter());
  }

  newpoints->SetPoint(count++,pos);

  for ( int i = istop; i < this->NumberOfHandles; ++i )
  {
    newpoints->SetPoint(count++,this->HandleGeometry[i]->GetCenter());
  }

  this->InitializeHandles(newpoints);
  newpoints->Delete();
}

//----------------------------------------------------------------------------
void vtkPolyLineRepresentation::InitializeHandles(vtkPoints* points)
{
  if ( !points ){ return; }

  int npts = points->GetNumberOfPoints();
  if ( npts < 2 ){ return; }

  double p0[3];
  double p1[3];

  points->GetPoint(0,p0);
  points->GetPoint(npts-1,p1);

  if ( vtkMath::Distance2BetweenPoints(p0,p1) == 0.0 )
  {
    --npts;
    this->Closed = 1;
    this->PolyLineSource->ClosedOn();
  }

  this->SetNumberOfHandles(npts);
  for ( int i = 0; i < npts; ++i )
  {
    this->SetHandlePosition(i,points->GetPoint(i));
  }
}

//----------------------------------------------------------------------------
void vtkPolyLineRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  if ( this->PolyLineSource )
  {
    os << indent << "PolyLineSource: "
       << this->PolyLineSource << "\n";
  }
  else
  {
    os << indent << "PolyLineSource: (none)\n";
  }
}
