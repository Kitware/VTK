/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSplineRepresentation.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSplineRepresentation.h"

#include "vtkActor.h"
#include "vtkAssemblyNode.h"
#include "vtkAssemblyPath.h"
#include "vtkBoundingBox.h"
#include "vtkCallbackCommand.h"
#include "vtkCamera.h"
#include "vtkCellArray.h"
#include "vtkCellPicker.h"
#include "vtkInteractorObserver.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkParametricFunctionSource.h"
#include "vtkParametricSpline.h"
#include "vtkPlaneSource.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"
#include "vtkTransform.h"
#include "vtkDoubleArray.h"

vtkStandardNewMacro(vtkSplineRepresentation);
//----------------------------------------------------------------------------
vtkSplineRepresentation::vtkSplineRepresentation()
{
  this->HandleSize = 5.0;

  this->InteractionState = vtkSplineRepresentation::Outside;
  this->ProjectToPlane = 0;  //default off
  this->ProjectionNormal = 0;  //default YZ not used
  this->ProjectionPosition = 0.0;
  this->PlaneSource = NULL;
  this->Closed = 0;

  // Build the representation of the widget

  // Default bounds to get started
  double bounds[6] = { -0.5, 0.5, -0.5, 0.5, -0.5, 0.5 };

  // Create the handles along a straight line within the bounds of a unit cube
  this->NumberOfHandles = 5;
  this->Handle         = new vtkActor* [this->NumberOfHandles];
  this->HandleGeometry = new vtkSphereSource* [this->NumberOfHandles];
  int i;
  double u[3];
  double x0 = bounds[0];
  double x1 = bounds[1];
  double y0 = bounds[2];
  double y1 = bounds[3];
  double z0 = bounds[4];
  double z1 = bounds[5];
  double x;
  double y;
  double z;
  vtkPoints* points = vtkPoints::New(VTK_DOUBLE);
  points->SetNumberOfPoints(this->NumberOfHandles);

  for ( i = 0; i < this->NumberOfHandles; ++i )
    {
    this->HandleGeometry[i] = vtkSphereSource::New();
    this->HandleGeometry[i]->SetThetaResolution(16);
    this->HandleGeometry[i]->SetPhiResolution(8);
    vtkPolyDataMapper* handleMapper = vtkPolyDataMapper::New();
    handleMapper->SetInput(this->HandleGeometry[i]->GetOutput());
    this->Handle[i] = vtkActor::New();
    this->Handle[i]->SetMapper(handleMapper);
    handleMapper->Delete();
    u[0] = i/(this->NumberOfHandles - 1.0);
    x = (1.0 - u[0])*x0 + u[0]*x1;
    y = (1.0 - u[0])*y0 + u[0]*y1;
    z = (1.0 - u[0])*z0 + u[0]*z1;
    points->SetPoint(i, x, y, z);
    this->HandleGeometry[i]->SetCenter(x,y,z);
    }

  // vtkParametric spline acts as the interpolating engine
  this->ParametricSpline = vtkParametricSpline::New();
  this->ParametricSpline->Register(this);
  this->ParametricSpline->SetPoints(points);
  //this->ParametricSpline->ParameterizeByLengthOff();
  points->Delete();
  this->ParametricSpline->Delete();

  // Define the points and line segments representing the spline
  this->Resolution = 499;

  this->ParametricFunctionSource = vtkParametricFunctionSource::New();
  this->ParametricFunctionSource->SetParametricFunction(this->ParametricSpline);
  this->ParametricFunctionSource->SetScalarModeToNone();
  this->ParametricFunctionSource->GenerateTextureCoordinatesOff();
  this->ParametricFunctionSource->SetUResolution( this->Resolution );
  this->ParametricFunctionSource->Update();

  vtkPolyDataMapper* lineMapper = vtkPolyDataMapper::New();
  lineMapper->SetInput( this->ParametricFunctionSource->GetOutput() ) ;
  lineMapper->ImmediateModeRenderingOn();
  lineMapper->SetResolveCoincidentTopologyToPolygonOffset();

  this->LineActor = vtkActor::New();
  this->LineActor->SetMapper( lineMapper );
  lineMapper->Delete();

  // Initial creation of the widget, serves to initialize it
  this->PlaceFactor = 1.0;
  this->PlaceWidget(bounds);

  // Manage the picking stuff
  this->HandlePicker = vtkCellPicker::New();
  this->HandlePicker->SetTolerance(0.005);

  for ( i = 0; i < this->NumberOfHandles; ++i )
    {
    this->HandlePicker->AddPickList(this->Handle[i]);
    }
  this->HandlePicker->PickFromListOn();

  this->LinePicker = vtkCellPicker::New();
  this->LinePicker->SetTolerance(0.01);
  this->LinePicker->AddPickList(this->LineActor);
  this->LinePicker->PickFromListOn();

  this->CurrentHandle = NULL;
  this->CurrentHandleIndex = -1;

  this->Transform = vtkTransform::New();

  // Set up the initial properties
  this->HandleProperty = NULL;
  this->SelectedHandleProperty = NULL;
  this->LineProperty = NULL;
  this->SelectedLineProperty = NULL;
  this->CreateDefaultProperties();
}

//----------------------------------------------------------------------------
vtkSplineRepresentation::~vtkSplineRepresentation()
{
  if ( this->ParametricSpline )
    {
    this->ParametricSpline->UnRegister(this);
    }

  this->ParametricFunctionSource->Delete();

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
void vtkSplineRepresentation::SetClosed(int closed)
{
  if ( this->Closed == closed )
    {
    return;
    }
  this->Closed = closed;
  this->ParametricSpline->SetClosed(this->Closed);

  this->BuildRepresentation();
}

//----------------------------------------------------------------------------
void vtkSplineRepresentation::SetParametricSpline(vtkParametricSpline* spline)
{
  if ( this->ParametricSpline != spline )
    {
    // to avoid destructor recursion
    vtkParametricSpline *temp = this->ParametricSpline;
    this->ParametricSpline = spline;
    if (temp != NULL)
      {
      temp->UnRegister(this);
      }
    if (this->ParametricSpline != NULL)
      {
      this->ParametricSpline->Register(this);
      this->ParametricFunctionSource->SetParametricFunction(this->ParametricSpline);
      }
    }
}

//----------------------------------------------------------------------------
vtkDoubleArray* vtkSplineRepresentation::GetHandlePositions()
{
  return vtkDoubleArray::SafeDownCast(
    this->ParametricSpline->GetPoints()->GetData());
}

//----------------------------------------------------------------------------
void vtkSplineRepresentation::SetHandlePosition(int handle, double x,
                                        double y, double z)
{
  if ( handle < 0 || handle >= this->NumberOfHandles )
    {
    vtkErrorMacro(<<"vtkSplineRepresentation: handle index out of range.");
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
void vtkSplineRepresentation::SetHandlePosition(int handle, double xyz[3])
{
  this->SetHandlePosition(handle,xyz[0],xyz[1],xyz[2]);
}

//----------------------------------------------------------------------------
void vtkSplineRepresentation::GetHandlePosition(int handle, double xyz[3])
{
  if ( handle < 0 || handle >= this->NumberOfHandles )
    {
    vtkErrorMacro(<<"vtkSplineRepresentation: handle index out of range.");
    return;
    }

  this->HandleGeometry[handle]->GetCenter(xyz);
}

//----------------------------------------------------------------------------
double* vtkSplineRepresentation::GetHandlePosition(int handle)
{
  if ( handle < 0 || handle >= this->NumberOfHandles )
    {
    vtkErrorMacro(<<"vtkSplineRepresentation: handle index out of range.");
    return NULL;
    }

  return this->HandleGeometry[handle]->GetCenter();
}

//----------------------------------------------------------------------------
void vtkSplineRepresentation::ProjectPointsToPlane()
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
void vtkSplineRepresentation::ProjectPointsToObliquePlane()
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
void vtkSplineRepresentation::ProjectPointsToOrthoPlane()
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
void vtkSplineRepresentation::BuildRepresentation()
{
  this->ValidPick = 1;
  // TODO: Avoid unnecessary rebuilds.
  // Handles have changed position, re-compute the spline coeffs
  vtkPoints* points = this->ParametricSpline->GetPoints();
  if ( points->GetNumberOfPoints() != this->NumberOfHandles )
    {
    points->SetNumberOfPoints( this->NumberOfHandles );
    }

  double pt[3];
  int i;
  vtkBoundingBox bbox;
  for ( i = 0; i < this->NumberOfHandles; ++i )
    {
    this->HandleGeometry[i]->GetCenter(pt);
    points->SetPoint(i, pt);
    bbox.AddPoint(pt);
    }
  this->ParametricSpline->Modified();

  double bounds[6];
  bbox.GetBounds(bounds);
  this->InitialLength = sqrt((bounds[1]-bounds[0])*(bounds[1]-bounds[0]) +
                             (bounds[3]-bounds[2])*(bounds[3]-bounds[2]) +
                             (bounds[5]-bounds[4])*(bounds[5]-bounds[4]));
  this->SizeHandles();
}

//----------------------------------------------------------------------------
int vtkSplineRepresentation::HighlightHandle(vtkProp *prop)
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
void vtkSplineRepresentation::HighlightLine(int highlight)
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
void vtkSplineRepresentation::MovePoint(double *p1, double *p2)
{
  if ( this->CurrentHandleIndex < 0 || this->CurrentHandleIndex >= this->NumberOfHandles )
    {
    vtkGenericWarningMacro(<<"Spline handle index out of range.");
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
void vtkSplineRepresentation::Translate(double *p1, double *p2)
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
void vtkSplineRepresentation::Scale(double *p1, double *p2, int vtkNotUsed(X), int Y)
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
void vtkSplineRepresentation::Spin(double *p1, double *p2, double *vpn)
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
    if ( this->ProjectionNormal == VTK_PROJECTION_OBLIQUE && \
         this->PlaneSource != NULL )
      {
      double* normal = this->PlaneSource->GetNormal();
      axis[0] = normal[0];
      axis[1] = normal[1];
      axis[2] = normal[2];
      vtkMath::Normalize(axis);
      }
    else
      {
      axis[ this->ProjectionNormal ] = 1.0;
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
void vtkSplineRepresentation::CreateDefaultProperties()
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
void vtkSplineRepresentation::SetProjectionPosition(double position)
{
  this->ProjectionPosition = position; 
  if ( this->ProjectToPlane )
    {
    this->ProjectPointsToPlane();
    }
  this->BuildRepresentation();
}

//----------------------------------------------------------------------------
void vtkSplineRepresentation::SetPlaneSource(vtkPlaneSource* plane)
{
  if (this->PlaneSource == plane)
    {
    return;
    }
  this->PlaneSource = plane;
}

//----------------------------------------------------------------------------
void vtkSplineRepresentation::SetNumberOfHandles(int npts)
{
  if ( this->NumberOfHandles == npts )
    {
    return;
    }
  if (npts < 1)
    {
    vtkGenericWarningMacro(<<"vtkSplineRepresentation: minimum of 1 points required.");
    return;
    }

  // Ensure that no handle is current
  this->HighlightHandle(NULL);
      
  double radius = this->HandleGeometry[0]->GetRadius();
  this->Initialize();

  this->NumberOfHandles = npts;

  // Create the handles
  this->Handle         = new vtkActor* [this->NumberOfHandles];
  this->HandleGeometry = new vtkSphereSource* [this->NumberOfHandles];

  int i;
  double pt[3];
  double u[3];
  for ( i = 0; i < this->NumberOfHandles; ++i )
    {
    this->HandleGeometry[i] = vtkSphereSource::New();
    this->HandleGeometry[i]->SetThetaResolution(16);
    this->HandleGeometry[i]->SetPhiResolution(8);
    vtkPolyDataMapper* handleMapper = vtkPolyDataMapper::New();
    handleMapper->SetInput(this->HandleGeometry[i]->GetOutput());
    this->Handle[i] = vtkActor::New();
    this->Handle[i]->SetMapper(handleMapper);
    handleMapper->Delete();
    this->Handle[i]->SetProperty(this->HandleProperty);
    u[0] = i/(this->NumberOfHandles - 1.0);
    this->ParametricSpline->Evaluate(u, pt, NULL);
    this->HandleGeometry[i]->SetCenter(pt);
    this->HandleGeometry[i]->SetRadius(radius);
    this->HandlePicker->AddPickList(this->Handle[i]);
    }

  if (this->CurrentHandleIndex >=0 &&
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
void vtkSplineRepresentation::Initialize(void)
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
void vtkSplineRepresentation::SetResolution(int resolution)
{
  if ( this->Resolution == resolution || resolution < (this->NumberOfHandles-1) )
    {
    return;
    }

  this->Resolution = resolution;
  this->ParametricFunctionSource->SetUResolution( this->Resolution );
  this->ParametricFunctionSource->Modified();
}

//----------------------------------------------------------------------------
void vtkSplineRepresentation::GetPolyData(vtkPolyData *pd)
{
  pd->ShallowCopy( this->ParametricFunctionSource->GetOutput() );
}

//----------------------------------------------------------------------------
void vtkSplineRepresentation::SizeHandles()
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
double vtkSplineRepresentation::GetSummedLength()
{
  vtkPoints* points = this->ParametricFunctionSource->GetOutput()->GetPoints();
  int npts = points->GetNumberOfPoints();

  if ( npts < 2 ) { return 0.0; }

  double a[3];
  double b[3];
  double sum = 0.0;
  int i = 0;
  points->GetPoint(i, a);
  int imax = (npts%2 == 0) ? npts-2 : npts-1;

  while ( i < imax )
    {
    points->GetPoint(i+1, b);
    sum += sqrt(vtkMath::Distance2BetweenPoints(a, b));
    i = i + 2;
    points->GetPoint(i, a);
    sum = sum + sqrt(vtkMath::Distance2BetweenPoints(a, b));
    }

  if ( npts%2 == 0 )
    {
    points->GetPoint(i+1, b);
    sum += sqrt(vtkMath::Distance2BetweenPoints(a, b));
    }

  return sum;
}

//----------------------------------------------------------------------------
void vtkSplineRepresentation::CalculateCentroid()
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
void vtkSplineRepresentation::InsertHandleOnLine(double* pos)
{
  if (this->NumberOfHandles < 2) { return; }

  vtkIdType id = this->LinePicker->GetCellId();
  if (id == -1){ return; }

  vtkIdType subid = this->LinePicker->GetSubId();

  vtkPoints* newpoints = vtkPoints::New(VTK_DOUBLE);
  newpoints->SetNumberOfPoints(this->NumberOfHandles+1);

  int istart = vtkMath::Floor(subid*(this->NumberOfHandles + this->Closed - 1.0)/
    static_cast<double>(this->Resolution));
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
void vtkSplineRepresentation::EraseHandle(const int& index)
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
void vtkSplineRepresentation::InitializeHandles(vtkPoints* points)
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
    this->ParametricSpline->ClosedOn();
    }

  this->SetNumberOfHandles(npts);
  int i;
  for ( i = 0; i < npts; ++i )
    {
    this->SetHandlePosition(i,points->GetPoint(i));
    }
}

//----------------------------------------------------------------------------
int vtkSplineRepresentation::IsClosed()
{
  if ( this->NumberOfHandles < 3 || !this->Closed ) { return 0; }

  vtkPolyData* lineData = this->ParametricFunctionSource->GetOutput();
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
void vtkSplineRepresentation::ReleaseGraphicsResources(vtkWindow* win)
{
  this->LineActor->ReleaseGraphicsResources(win);
  for (int cc=0; cc < this->NumberOfHandles; cc++)
    {
    this->Handle[cc]->ReleaseGraphicsResources(win);
    }
}

//----------------------------------------------------------------------------
int vtkSplineRepresentation::RenderOpaqueGeometry(vtkViewport* win)
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
int vtkSplineRepresentation::RenderTranslucentPolygonalGeometry(
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
int vtkSplineRepresentation::RenderOverlay(vtkViewport* win)
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
int vtkSplineRepresentation::HasTranslucentPolygonalGeometry()
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
int vtkSplineRepresentation::ComputeInteractionState(int X, int Y,
  int vtkNotUsed(modify))
{
  this->InteractionState = vtkSplineRepresentation::Outside;
  if (!this->Renderer || !this->Renderer->IsInViewport(X, Y))
    {
    return this->InteractionState;
    }

  // Try and pick a handle first. This allows the picking of the handle even
  // if it is "behind" the spline.
  vtkAssemblyPath *path;
  int handlePicked = 0;

  this->HandlePicker->Pick(X,Y,0.0,this->Renderer);
  path = this->HandlePicker->GetPath();
  if ( path != NULL )
    {
    this->ValidPick = 1;
    this->InteractionState = vtkSplineRepresentation::OnHandle;
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
    this->LinePicker->Pick(X,Y,0.0,this->Renderer);
    path = this->LinePicker->GetPath();
    if ( path != NULL )
      {
      this->ValidPick = 1;
      this->LinePicker->GetPickPosition(this->LastPickPosition);
      this->HighlightLine(1);
      this->InteractionState = vtkSplineRepresentation::OnLine;
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
void vtkSplineRepresentation::StartWidgetInteraction(double e[2])
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
void vtkSplineRepresentation::WidgetInteraction(double e[2])
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
  if (this->InteractionState == vtkSplineRepresentation::Moving)
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
  else if (this->InteractionState == vtkSplineRepresentation::Scaling)
    {
    this->Scale(prevPickPoint, pickPoint,
      static_cast<int>(e[0]), static_cast<int>(e[1]));
    }
  else if (this->InteractionState == vtkSplineRepresentation::Spinning)
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
void vtkSplineRepresentation::EndWidgetInteraction(double[2])
{
  switch (this->InteractionState) 
    {
  case vtkSplineRepresentation::Inserting:
    this->InsertHandleOnLine(this->LastPickPosition);
    break;

  case vtkSplineRepresentation::Erasing:
    if (this->CurrentHandleIndex)
      {
      int index = this->CurrentHandleIndex;
      this->CurrentHandleIndex = this->HighlightHandle(NULL);
      this->EraseHandle(index);
      }
    }

  this->HighlightLine(0);
  this->InteractionState = vtkSplineRepresentation::Outside;
}

//----------------------------------------------------------------------------
double* vtkSplineRepresentation::GetBounds()
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
void vtkSplineRepresentation::SetLineColor(double r, double g, double b)
{
  this->GetLineProperty()->SetColor(r, g, b);
}

//----------------------------------------------------------------------------
void vtkSplineRepresentation::PrintSelf(ostream& os, vtkIndent indent)
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
  if ( this->ParametricSpline )
    {
    os << indent << "ParametricSpline: "
       << this->ParametricSpline << "\n";
    }
  else
    {
    os << indent << "ParametricSpline: (none)\n";
    }

  os << indent << "Project To Plane: "
     << (this->ProjectToPlane ? "On" : "Off") << "\n";
  os << indent << "Projection Normal: " << this->ProjectionNormal << "\n";
  os << indent << "Projection Position: " << this->ProjectionPosition << "\n";
  os << indent << "Resolution: " << this->Resolution << "\n";
  os << indent << "Number Of Handles: " << this->NumberOfHandles << "\n";
  os << indent << "Closed: "
     << (this->Closed ? "On" : "Off") << "\n";
  os << indent << "InteractionState: " << this->InteractionState << endl;
}
