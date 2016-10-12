/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCurveRepresentation.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCurveRepresentation.h"

#include "vtkActor.h"
#include "vtkAssemblyPath.h"
#include "vtkBoundingBox.h"
#include "vtkCallbackCommand.h"
#include "vtkCamera.h"
#include "vtkCellArray.h"
#include "vtkCellPicker.h"
#include "vtkInteractorObserver.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPickingManager.h"
#include "vtkPlaneSource.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSphereSource.h"
#include "vtkTransform.h"

//----------------------------------------------------------------------------
vtkCurveRepresentation::vtkCurveRepresentation()
{
  this->LastEventPosition[0] = VTK_DOUBLE_MAX;
  this->LastEventPosition[1] = VTK_DOUBLE_MAX;
  this->LastEventPosition[2] = VTK_DOUBLE_MAX;

  this->Bounds[0] =  VTK_DOUBLE_MAX;
  this->Bounds[1] = -VTK_DOUBLE_MAX;
  this->Bounds[2] =  VTK_DOUBLE_MAX;
  this->Bounds[3] = -VTK_DOUBLE_MAX;
  this->Bounds[4] =  VTK_DOUBLE_MAX;
  this->Bounds[5] = -VTK_DOUBLE_MAX;

  this->HandleSize = 5.0;

  this->InteractionState = vtkCurveRepresentation::Outside;
  this->ProjectToPlane = 0;  //default off
  this->ProjectionNormal = 0;  //default YZ not used
  this->ProjectionPosition = 0.0;
  this->PlaneSource = NULL;
  this->Closed = 0;

  // Build the representation of the widget

  // Create the handles along a straight line within the bounds of a unit cube
  this->NumberOfHandles = 5;
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
  }

  this->LineActor = vtkActor::New();

  // Default bounds to get started
  double bounds[6] = { -0.5, 0.5, -0.5, 0.5, -0.5, 0.5 };

  // Initial creation of the widget, serves to initialize it
  this->PlaceFactor = 1.0;
  this->PlaceWidget(bounds);

  // Manage the picking stuff
  this->HandlePicker = vtkCellPicker::New();
  this->HandlePicker->SetTolerance(0.005);

  for ( int i = 0; i < this->NumberOfHandles; ++i )
  {
    this->HandlePicker->AddPickList(this->Handle[i]);
  }
  this->HandlePicker->PickFromListOn();

  this->LinePicker = vtkCellPicker::New();
  this->LinePicker->SetTolerance(0.01);
  this->LinePicker->AddPickList(this->LineActor);
  this->LinePicker->PickFromListOn();

  this->LastPickPosition[0] = VTK_DOUBLE_MAX;
  this->LastPickPosition[1] = VTK_DOUBLE_MAX;
  this->LastPickPosition[2] = VTK_DOUBLE_MAX;

  this->CurrentHandle = NULL;
  this->CurrentHandleIndex = -1;

  this->Transform = vtkTransform::New();

  // Set up the initial properties
  this->HandleProperty = NULL;
  this->SelectedHandleProperty = NULL;
  this->LineProperty = NULL;
  this->SelectedLineProperty = NULL;
  this->CreateDefaultProperties();

  this->Centroid[0] = 0.0;
  this->Centroid[1] = 0.0;
  this->Centroid[2] = 0.0;
}

//----------------------------------------------------------------------------
vtkCurveRepresentation::~vtkCurveRepresentation()
{
  this->LineActor->Delete();

  for ( int i = 0; i < this->NumberOfHandles; ++i )
  {
    this->HandleGeometry[i]->Delete();
    this->Handle[i]->Delete();
  }
  delete [] this->Handle;
  delete [] this->HandleGeometry;

  this->HandlePicker->Delete();
  this->LinePicker->Delete();

  if ( this->HandleProperty )
  {
    this->HandleProperty->Delete();
  }
  if ( this->SelectedHandleProperty )
  {
    this->SelectedHandleProperty->Delete();
  }
  if ( this->LineProperty )
  {
    this->LineProperty->Delete();
  }
  if ( this->SelectedLineProperty )
  {
    this->SelectedLineProperty->Delete();
  }

  this->Transform->Delete();
}

//----------------------------------------------------------------------------
void vtkCurveRepresentation::SetClosed(int closed)
{
  if ( this->Closed == closed )
  {
    return;
  }
  this->Closed = closed;

  this->BuildRepresentation();
}

//----------------------------------------------------------------------
void vtkCurveRepresentation::RegisterPickers()
{
  this->Renderer->GetRenderWindow()->GetInteractor()->GetPickingManager()
    ->AddPicker(this->HandlePicker, this);
  this->Renderer->GetRenderWindow()->GetInteractor()->GetPickingManager()
    ->AddPicker(this->LinePicker, this);
}

//----------------------------------------------------------------------------
void vtkCurveRepresentation::SetHandlePosition(int handle, double x,
                                        double y, double z)
{
  if ( handle < 0 || handle >= this->NumberOfHandles )
  {
    vtkErrorMacro(<<"vtkCurveRepresentation: handle index out of range.");
    return;
  }
  this->HandleGeometry[handle]->SetCenter(x,y,z);
  this->HandleGeometry[handle]->Update();
  if ( this->ProjectToPlane )
  {
    this->ProjectPointsToPlane();
  }
  this->BuildRepresentation();
}

//----------------------------------------------------------------------------
void vtkCurveRepresentation::SetHandlePosition(int handle, double xyz[3])
{
  this->SetHandlePosition(handle,xyz[0],xyz[1],xyz[2]);
}

//----------------------------------------------------------------------------
void vtkCurveRepresentation::GetHandlePosition(int handle, double xyz[3])
{
  if ( handle < 0 || handle >= this->NumberOfHandles )
  {
    vtkErrorMacro(<<"vtkCurveRepresentation: handle index out of range.");
    return;
  }

  this->HandleGeometry[handle]->GetCenter(xyz);
}

//----------------------------------------------------------------------------
double* vtkCurveRepresentation::GetHandlePosition(int handle)
{
  if ( handle < 0 || handle >= this->NumberOfHandles )
  {
    vtkErrorMacro(<<"vtkCurveRepresentation: handle index out of range.");
    return NULL;
  }

  return this->HandleGeometry[handle]->GetCenter();
}

//----------------------------------------------------------------------------
void vtkCurveRepresentation::ProjectPointsToPlane()
{
  if ( this->ProjectionNormal == VTK_PROJECTION_OBLIQUE )
  {
    if ( this->PlaneSource != NULL )
    {
      this->ProjectPointsToObliquePlane();
    }
    else
    {
      vtkGenericWarningMacro(<<"Set the plane source for oblique projections...");
    }
  }
  else
  {
    this->ProjectPointsToOrthoPlane();
  }
}

//----------------------------------------------------------------------------
void vtkCurveRepresentation::ProjectPointsToObliquePlane()
{
  double o[3];
  double u[3];
  double v[3];

  this->PlaneSource->GetPoint1(u);
  this->PlaneSource->GetPoint2(v);
  this->PlaneSource->GetOrigin(o);

  int i;
  for ( i = 0; i < 3; ++i )
  {
    u[i] = u[i] - o[i];
    v[i] = v[i] - o[i];
  }
  vtkMath::Normalize(u);
  vtkMath::Normalize(v);

  double o_dot_u = vtkMath::Dot(o,u);
  double o_dot_v = vtkMath::Dot(o,v);
  double fac1;
  double fac2;
  double ctr[3];
  for ( i = 0; i < this->NumberOfHandles; ++i )
  {
    this->HandleGeometry[i]->GetCenter(ctr);
    fac1 = vtkMath::Dot(ctr,u) - o_dot_u;
    fac2 = vtkMath::Dot(ctr,v) - o_dot_v;
    ctr[0] = o[0] + fac1*u[0] + fac2*v[0];
    ctr[1] = o[1] + fac1*u[1] + fac2*v[1];
    ctr[2] = o[2] + fac1*u[2] + fac2*v[2];
    this->HandleGeometry[i]->SetCenter(ctr);
    this->HandleGeometry[i]->Update();
  }
}

//----------------------------------------------------------------------------
void vtkCurveRepresentation::ProjectPointsToOrthoPlane()
{
  double ctr[3];
  for ( int i = 0; i < this->NumberOfHandles; ++i )
  {
    this->HandleGeometry[i]->GetCenter(ctr);
    ctr[this->ProjectionNormal] = this->ProjectionPosition;
    this->HandleGeometry[i]->SetCenter(ctr);
    this->HandleGeometry[i]->Update();
  }
}

//----------------------------------------------------------------------------
int vtkCurveRepresentation::HighlightHandle(vtkProp *prop)
{
  // First unhighlight anything picked
  if ( this->CurrentHandle )
  {
    this->CurrentHandle->SetProperty(this->HandleProperty);
  }

  this->CurrentHandle = static_cast<vtkActor *>(prop);

  if ( this->CurrentHandle )
  {
    for ( int i = 0; i < this->NumberOfHandles; ++i ) // find handle
    {
      if ( this->CurrentHandle == this->Handle[i] )
      {
        this->CurrentHandle->SetProperty(this->SelectedHandleProperty);
        return i;
      }
    }
  }
  return -1;
}

//----------------------------------------------------------------------------
void vtkCurveRepresentation::HighlightLine(int highlight)
{
  if ( highlight )
  {
    this->LineActor->SetProperty(this->SelectedLineProperty);
  }
  else
  {
    this->LineActor->SetProperty(this->LineProperty);
  }
}

//----------------------------------------------------------------------------
void vtkCurveRepresentation::MovePoint(double *p1, double *p2)
{
  if ( this->CurrentHandleIndex < 0 || this->CurrentHandleIndex >= this->NumberOfHandles )
  {
    vtkGenericWarningMacro(<<"Poly line handle index out of range.");
    return;
  }
  // Get the motion vector
  double v[3];
  v[0] = p2[0] - p1[0];
  v[1] = p2[1] - p1[1];
  v[2] = p2[2] - p1[2];

  double *ctr = this->HandleGeometry[this->CurrentHandleIndex]->GetCenter();

  double newCtr[3];
  newCtr[0] = ctr[0] + v[0];
  newCtr[1] = ctr[1] + v[1];
  newCtr[2] = ctr[2] + v[2];

  this->HandleGeometry[this->CurrentHandleIndex]->SetCenter(newCtr);
  this->HandleGeometry[this->CurrentHandleIndex]->Update();
}

//----------------------------------------------------------------------------
void vtkCurveRepresentation::Translate(double *p1, double *p2)
{
  // Get the motion vector
  double v[3];
  v[0] = p2[0] - p1[0];
  v[1] = p2[1] - p1[1];
  v[2] = p2[2] - p1[2];

  double newCtr[3];
  for ( int i = 0; i < this->NumberOfHandles; ++i )
  {
    double* ctr =  this->HandleGeometry[i]->GetCenter();
    for ( int j = 0; j < 3; ++j )
    {
      newCtr[j] = ctr[j] + v[j];
    }
     this->HandleGeometry[i]->SetCenter(newCtr);
     this->HandleGeometry[i]->Update();
  }
}

//----------------------------------------------------------------------------
void vtkCurveRepresentation::Scale(double *p1, double *p2, int vtkNotUsed(X), int Y)
{
  // Get the motion vector
  double v[3];
  v[0] = p2[0] - p1[0];
  v[1] = p2[1] - p1[1];
  v[2] = p2[2] - p1[2];

  double center[3] = {0.0,0.0,0.0};
  double avgdist = 0.0;
  double *prevctr = this->HandleGeometry[0]->GetCenter();
  double *ctr;

  center[0] += prevctr[0];
  center[1] += prevctr[1];
  center[2] += prevctr[2];

  int i;
  for ( i = 1; i < this->NumberOfHandles; ++i )
  {
    ctr = this->HandleGeometry[i]->GetCenter();
    center[0] += ctr[0];
    center[1] += ctr[1];
    center[2] += ctr[2];
    avgdist += sqrt(vtkMath::Distance2BetweenPoints(ctr,prevctr));
    prevctr = ctr;
  }

  avgdist /= this->NumberOfHandles;

  center[0] /= this->NumberOfHandles;
  center[1] /= this->NumberOfHandles;
  center[2] /= this->NumberOfHandles;

  // Compute the scale factor
  double sf = vtkMath::Norm(v) / avgdist;
  if ( Y > this->LastEventPosition[1] )
  {
    sf = 1.0 + sf;
  }
  else
  {
    sf = 1.0 - sf;
  }

  // Move the handle points
  double newCtr[3];
  for ( i = 0; i < this->NumberOfHandles; ++i )
  {
    ctr = this->HandleGeometry[i]->GetCenter();
    for ( int j = 0; j < 3; ++j )
    {
      newCtr[j] = sf * (ctr[j] - center[j]) + center[j];
    }
    this->HandleGeometry[i]->SetCenter(newCtr);
    this->HandleGeometry[i]->Update();
  }
}

//----------------------------------------------------------------------------
void vtkCurveRepresentation::Spin(double *p1, double *p2, double *vpn)
{
  // Mouse motion vector in world space
  double v[3];
  v[0] = p2[0] - p1[0];
  v[1] = p2[1] - p1[1];
  v[2] = p2[2] - p1[2];

  // Axis of rotation
  double axis[3] = {0.0,0.0,0.0};

  if ( this->ProjectToPlane )
  {
    if ( this->ProjectionNormal == VTK_PROJECTION_OBLIQUE)
    {
      if (this->PlaneSource != NULL )
      {
        double* normal = this->PlaneSource->GetNormal();
        axis[0] = normal[0];
        axis[1] = normal[1];
        axis[2] = normal[2];
        vtkMath::Normalize( axis );
      }
      else
      {
        axis[0] = 1.;
      }
    }
    else
    {
      axis[ this->ProjectionNormal ] = 1.;
    }
  }
  else
  {
  // Create axis of rotation and angle of rotation
    vtkMath::Cross(vpn,v,axis);
    if ( vtkMath::Normalize(axis) == 0.0 )
    {
      return;
    }
  }

  // Radius vector (from mean center to cursor position)
  double rv[3] = {p2[0] - this->Centroid[0],
                  p2[1] - this->Centroid[1],
                  p2[2] - this->Centroid[2]};

  // Distance between center and cursor location
  double rs = vtkMath::Normalize(rv);

  // Spin direction
  double ax_cross_rv[3];
  vtkMath::Cross(axis,rv,ax_cross_rv);

  // Spin angle
  double theta = 360.0 * vtkMath::Dot(v,ax_cross_rv) / rs;

  // Manipulate the transform to reflect the rotation
  this->Transform->Identity();
  this->Transform->Translate(this->Centroid[0],this->Centroid[1],this->Centroid[2]);
  this->Transform->RotateWXYZ(theta,axis);
  this->Transform->Translate(-this->Centroid[0],-this->Centroid[1],-this->Centroid[2]);

  // Set the handle points
  double newCtr[3];
  double ctr[3];
  for ( int i = 0; i < this->NumberOfHandles; ++i )
  {
    this->HandleGeometry[i]->GetCenter(ctr);
    this->Transform->TransformPoint(ctr,newCtr);
    this->HandleGeometry[i]->SetCenter(newCtr);
    this->HandleGeometry[i]->Update();
  }
}

//----------------------------------------------------------------------------
void vtkCurveRepresentation::CreateDefaultProperties()
{
  this->HandleProperty = vtkProperty::New();
  this->HandleProperty->SetColor(1,1,1);

  this->SelectedHandleProperty = vtkProperty::New();
  this->SelectedHandleProperty->SetColor(1,0,0);

  this->LineProperty = vtkProperty::New();
  this->LineProperty->SetRepresentationToWireframe();
  this->LineProperty->SetAmbient(1.0);
  this->LineProperty->SetColor(1.0,1.0,0.0);
  this->LineProperty->SetLineWidth(2.0);

  this->SelectedLineProperty = vtkProperty::New();
  this->SelectedLineProperty->SetRepresentationToWireframe();
  this->SelectedLineProperty->SetAmbient(1.0);
  this->SelectedLineProperty->SetAmbientColor(0.0,1.0,0.0);
  this->SelectedLineProperty->SetLineWidth(2.0);
}

//----------------------------------------------------------------------------
void vtkCurveRepresentation::SetProjectionPosition(double position)
{
  this->ProjectionPosition = position;
  if ( this->ProjectToPlane )
  {
    this->ProjectPointsToPlane();
  }
  this->BuildRepresentation();
}

//----------------------------------------------------------------------------
void vtkCurveRepresentation::SetPlaneSource(vtkPlaneSource* plane)
{
  if (this->PlaneSource == plane)
  {
    return;
  }
  this->PlaneSource = plane;
}

//----------------------------------------------------------------------------
void vtkCurveRepresentation::Initialize(void)
{
  int i;
  for ( i = 0; i < this->NumberOfHandles; ++i )
  {
    this->HandlePicker->DeletePickList(this->Handle[i]);
    this->HandleGeometry[i]->Delete();
    this->Handle[i]->Delete();
  }

  this->NumberOfHandles = 0;

  delete [] this->Handle;
  delete [] this->HandleGeometry;
}

//----------------------------------------------------------------------------
void vtkCurveRepresentation::SizeHandles()
{
  if (this->NumberOfHandles > 0)
  {
    double radius = this->SizeHandlesInPixels(1.5,
      this->HandleGeometry[0]->GetCenter());
    //cout << "Raduis: " << radius << endl;
    for ( int i = 0; i < this->NumberOfHandles; ++i )
    {
      this->HandleGeometry[i]->SetRadius(radius);
    }
  }
}

//----------------------------------------------------------------------------
void vtkCurveRepresentation::CalculateCentroid()
{
  this->Centroid[0] = 0.0;
  this->Centroid[1] = 0.0;
  this->Centroid[2] = 0.0;

  double ctr[3];
  for ( int i = 0; i < this->NumberOfHandles; ++i )
  {
    this->HandleGeometry[i]->GetCenter(ctr);
    this->Centroid[0] += ctr[0];
    this->Centroid[1] += ctr[1];
    this->Centroid[2] += ctr[2];
  }

  this->Centroid[0] /= this->NumberOfHandles;
  this->Centroid[1] /= this->NumberOfHandles;
  this->Centroid[2] /= this->NumberOfHandles;
}

//----------------------------------------------------------------------------
void vtkCurveRepresentation::InsertHandleOnLine(double* pos)
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
  int i;
  for ( i = 0; i <= istart; ++i )
  {
    newpoints->SetPoint(count++,this->HandleGeometry[i]->GetCenter());
  }

  newpoints->SetPoint(count++,pos);

  for ( i = istop; i < this->NumberOfHandles; ++i )
  {
    newpoints->SetPoint(count++,this->HandleGeometry[i]->GetCenter());
  }

  this->InitializeHandles(newpoints);
  newpoints->Delete();
}

//----------------------------------------------------------------------------
void vtkCurveRepresentation::EraseHandle(const int& index)
{
  if ( this->NumberOfHandles < 3 || index < 0 || index >= this->NumberOfHandles )
  {
    return;
  }

  vtkPoints* newpoints = vtkPoints::New(VTK_DOUBLE);
  newpoints->SetNumberOfPoints(this->NumberOfHandles-1);
  int count = 0;
  for (int i = 0; i < this->NumberOfHandles; ++i )
  {
    if ( i != index )
    {
      newpoints->SetPoint(count++,this->HandleGeometry[i]->GetCenter());
    }
  }

  this->InitializeHandles(newpoints);
  newpoints->Delete();
}

//----------------------------------------------------------------------------
int vtkCurveRepresentation::IsClosed()
{
  if ( this->NumberOfHandles < 3 || !this->Closed ) { return 0; }

  vtkPolyData* lineData = vtkPolyData::New();
  this->GetPolyData(lineData);
  if ( !lineData || !(lineData->GetPoints()) )
  {
    vtkErrorMacro(<<"No line data to query geometric closure");
    return 0;
  }

  vtkPoints *points = lineData->GetPoints();
  int numPoints = points->GetNumberOfPoints();

  if ( numPoints < 3 )
  {
    return 0;
  }

  int numEntries = lineData->GetLines()->GetNumberOfConnectivityEntries();

  double p0[3];
  double p1[3];

  points->GetPoint( 0, p0 );
  points->GetPoint( numPoints - 1, p1 );
  int minusNth = ( p0[0] == p1[0] && p0[1] == p1[1] && p0[2] == p1[2] ) ? 1 : 0;
  int result;
  if ( minusNth ) //definitely closed
  {
    result = 1;
  }
  else       // not physically closed, check connectivity
  {
    result = ( ( numEntries - numPoints ) == 2 ) ? 1 : 0;
  }

  return result;
}

//----------------------------------------------------------------------------
void vtkCurveRepresentation::ReleaseGraphicsResources(vtkWindow* win)
{
  this->LineActor->ReleaseGraphicsResources(win);
  for (int cc=0; cc < this->NumberOfHandles; cc++)
  {
    this->Handle[cc]->ReleaseGraphicsResources(win);
  }
}

//----------------------------------------------------------------------------
int vtkCurveRepresentation::RenderOpaqueGeometry(vtkViewport* win)
{
  this->BuildRepresentation();

  int count = 0;
  count += this->LineActor->RenderOpaqueGeometry(win);
  for (int cc=0; cc < this->NumberOfHandles; cc++)
  {
    count+= this->Handle[cc]->RenderOpaqueGeometry(win);
  }
  return count;
}

//----------------------------------------------------------------------------
int vtkCurveRepresentation::RenderTranslucentPolygonalGeometry(
  vtkViewport* win)
{
  int count = 0;
  count += this->LineActor->RenderTranslucentPolygonalGeometry(win);
  for (int cc=0; cc < this->NumberOfHandles; cc++)
  {
    count += this->Handle[cc]->RenderTranslucentPolygonalGeometry(win);
  }
  return count;
}

//----------------------------------------------------------------------------
int vtkCurveRepresentation::RenderOverlay(vtkViewport* win)
{
  int count = 0;
  count += this->LineActor->RenderOverlay(win);
  for (int cc=0; cc < this->NumberOfHandles; cc++)
  {
    count += this->Handle[cc]->RenderOverlay(win);
  }
  return count;
}

//----------------------------------------------------------------------------
int vtkCurveRepresentation::HasTranslucentPolygonalGeometry()
{
  this->BuildRepresentation();
  int count = 0;
  count |= this->LineActor->HasTranslucentPolygonalGeometry();
  for (int cc=0; cc < this->NumberOfHandles; cc++)
  {
    count |= this->Handle[cc]->HasTranslucentPolygonalGeometry();
  }
  return count;
}

//----------------------------------------------------------------------------
int vtkCurveRepresentation::ComputeInteractionState(int X, int Y,
  int vtkNotUsed(modify))
{
  this->InteractionState = vtkCurveRepresentation::Outside;
  if (!this->Renderer || !this->Renderer->IsInViewport(X, Y))
  {
    return this->InteractionState;
  }

  // Try and pick a handle first. This allows the picking of the handle even
  // if it is "behind" the poly line.
  int handlePicked = 0;

  vtkAssemblyPath* path = this->GetAssemblyPath(X, Y, 0., this->HandlePicker);

  if ( path != NULL )
  {
    this->ValidPick = 1;
    this->InteractionState = vtkCurveRepresentation::OnHandle;
    this->CurrentHandleIndex =
      this->HighlightHandle(path->GetFirstNode()->GetViewProp());
    this->HandlePicker->GetPickPosition(this->LastPickPosition);
    handlePicked = 1;
  }
  else
  {
    this->CurrentHandleIndex = this->HighlightHandle(NULL);
  }

  if (!handlePicked)
  {
    path = this->GetAssemblyPath(X, Y, 0., this->LinePicker);

    if ( path != NULL )
    {
      this->ValidPick = 1;
      this->LinePicker->GetPickPosition(this->LastPickPosition);
      this->HighlightLine(1);
      this->InteractionState = vtkCurveRepresentation::OnLine;
    }
    else
    {
      this->HighlightLine(0);
    }
  }
  else
  {
    this->HighlightLine(0);
  }

  return this->InteractionState;
}

//----------------------------------------------------------------------------
void vtkCurveRepresentation::StartWidgetInteraction(double e[2])
{
  // Store the start position
  this->StartEventPosition[0] = e[0];
  this->StartEventPosition[1] = e[1];
  this->StartEventPosition[2] = 0.0;

  // Store the start position
  this->LastEventPosition[0] = e[0];
  this->LastEventPosition[1] = e[1];
  this->LastEventPosition[2] = 0.0;

  this->ComputeInteractionState(static_cast<int>(e[0]),static_cast<int>(e[1]),0);
}

//----------------------------------------------------------------------------
void vtkCurveRepresentation::WidgetInteraction(double e[2])
{
  // Convert events to appropriate coordinate systems
  vtkCamera *camera = this->Renderer->GetActiveCamera();
  if ( !camera )
  {
    return;
  }
  double focalPoint[4], pickPoint[4], prevPickPoint[4];
  double z, vpn[3];

  // Compute the two points defining the motion vector
  vtkInteractorObserver::ComputeWorldToDisplay(this->Renderer,
    this->LastPickPosition[0], this->LastPickPosition[1], this->LastPickPosition[2],
    focalPoint);
  z = focalPoint[2];
  vtkInteractorObserver::ComputeDisplayToWorld(this->Renderer,this->LastEventPosition[0],
                                               this->LastEventPosition[1], z, prevPickPoint);
  vtkInteractorObserver::ComputeDisplayToWorld(this->Renderer, e[0], e[1], z, pickPoint);

  // Process the motion
  if (this->InteractionState == vtkCurveRepresentation::Moving)
  {
    if (this->CurrentHandleIndex != -1)
    {
      this->MovePoint(prevPickPoint, pickPoint);
    }
    else
    {
       this->Translate(prevPickPoint, pickPoint);
    }
  }
  else if (this->InteractionState == vtkCurveRepresentation::Scaling)
  {
    this->Scale(prevPickPoint, pickPoint,
      static_cast<int>(e[0]), static_cast<int>(e[1]));
  }
  else if (this->InteractionState == vtkCurveRepresentation::Spinning)
  {
    camera->GetViewPlaneNormal(vpn);
    this->Spin(prevPickPoint, pickPoint, vpn);
  }

  if (this->ProjectToPlane)
  {
    this->ProjectPointsToPlane();
  }

  this->BuildRepresentation();

  // Store the position
  this->LastEventPosition[0] = e[0];
  this->LastEventPosition[1] = e[1];
  this->LastEventPosition[2] = 0.0;
}

//----------------------------------------------------------------------------
void vtkCurveRepresentation::EndWidgetInteraction(double[2])
{
  switch (this->InteractionState)
  {
  case vtkCurveRepresentation::Inserting:
    this->InsertHandleOnLine(this->LastPickPosition);
    break;

  case vtkCurveRepresentation::Erasing:
    if (this->CurrentHandleIndex)
    {
      int index = this->CurrentHandleIndex;
      this->CurrentHandleIndex = this->HighlightHandle(NULL);
      this->EraseHandle(index);
    }
  }

  this->HighlightLine(0);
  this->InteractionState = vtkCurveRepresentation::Outside;
}

//----------------------------------------------------------------------------
double* vtkCurveRepresentation::GetBounds()
{
  this->BuildRepresentation();

  vtkBoundingBox bbox;
  bbox.AddBounds(this->LineActor->GetBounds());
  for (int cc=0; cc < this->NumberOfHandles; cc++)
  {
    bbox.AddBounds(this->HandleGeometry[cc]->GetOutput()->GetBounds());
  }
  bbox.GetBounds(this->Bounds);
  return this->Bounds;
}


//----------------------------------------------------------------------------
void vtkCurveRepresentation::SetLineColor(double r, double g, double b)
{
  this->GetLineProperty()->SetColor(r, g, b);
}

//----------------------------------------------------------------------------
void vtkCurveRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  if ( this->HandleProperty )
  {
    os << indent << "Handle Property: " << this->HandleProperty << "\n";
  }
  else
  {
    os << indent << "Handle Property: (none)\n";
  }
  if ( this->SelectedHandleProperty )
  {
    os << indent << "Selected Handle Property: "
       << this->SelectedHandleProperty << "\n";
  }
  else
  {
    os << indent << "Selected Handle Property: (none)\n";
  }
  if ( this->LineProperty )
  {
    os << indent << "Line Property: " << this->LineProperty << "\n";
  }
  else
  {
    os << indent << "Line Property: (none)\n";
  }
  if ( this->SelectedLineProperty )
  {
    os << indent << "Selected Line Property: "
       << this->SelectedLineProperty << "\n";
  }
  else
  {
    os << indent << "Selected Line Property: (none)\n";
  }

  os << indent << "Project To Plane: "
     << (this->ProjectToPlane ? "On" : "Off") << "\n";
  os << indent << "Projection Normal: " << this->ProjectionNormal << "\n";
  os << indent << "Projection Position: " << this->ProjectionPosition << "\n";
  os << indent << "Number Of Handles: " << this->NumberOfHandles << "\n";
  os << indent << "Closed: "
     << (this->Closed ? "On" : "Off") << "\n";
  os << indent << "InteractionState: " << this->InteractionState << endl;
}
