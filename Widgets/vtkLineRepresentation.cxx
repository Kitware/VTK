/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLineRepresentation.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkLineRepresentation.h"
#include "vtkPointHandleRepresentation3D.h"
#include "vtkActor.h"
#include "vtkLineSource.h"
#include "vtkSphereSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkPolyData.h"
#include "vtkCallbackCommand.h"
#include "vtkBox.h"
#include "vtkPolyData.h"
#include "vtkProperty.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkInteractorObserver.h"
#include "vtkMath.h"
#include "vtkLine.h"
#include "vtkWindow.h"
#include "vtkObjectFactory.h"


vtkCxxRevisionMacro(vtkLineRepresentation, "1.1");
vtkStandardNewMacro(vtkLineRepresentation);

vtkCxxSetObjectMacro(vtkLineRepresentation,HandleRepresentation,vtkPointHandleRepresentation3D);

//----------------------------------------------------------------------------
vtkLineRepresentation::vtkLineRepresentation()
{
  // Handle size is in pixels for this widget
  this->HandleSize = 5.0;

  // By default, use one of these handles
  this->HandleRepresentation  = vtkPointHandleRepresentation3D::New();
  this->HandleRepresentation->AllOff();
  this->HandleRepresentation->SetHotSpotSize(1.0);
  this->HandleRepresentation->SetPlaceFactor(1.0);
  this->HandleRepresentation->TranslationModeOn();
  this->Point1Representation = NULL;
  this->Point2Representation = NULL;
  this->LineHandleRepresentation = NULL;
  this->InstantiateHandleRepresentation();

  // Miscellaneous parameters
  this->Tolerance = 5;
  this->Placed = 0;

  // Represent the line
  this->LineSource = vtkLineSource::New();
  this->LineSource->SetResolution(5);
  this->LineMapper = vtkPolyDataMapper::New();
  this->LineMapper->SetInput(this->LineSource->GetOutput());
  this->LineActor = vtkActor::New();
  this->LineActor->SetMapper(this->LineMapper);

  // Create the handles
  this->Handle = new vtkActor* [2];
  this->HandleMapper = new vtkPolyDataMapper* [2];
  this->HandleGeometry = new vtkSphereSource* [2];
  for (int i=0; i<2; i++)
    {
    this->HandleGeometry[i] = vtkSphereSource::New();
    this->HandleGeometry[i]->SetThetaResolution(16);
    this->HandleGeometry[i]->SetPhiResolution(8);
    this->HandleMapper[i] = vtkPolyDataMapper::New();
    this->HandleMapper[i]->SetInput(this->HandleGeometry[i]->GetOutput());
    this->Handle[i] = vtkActor::New();
    this->Handle[i]->SetMapper(this->HandleMapper[i]);
    }

  // Set up the initial properties
  this->CreateDefaultProperties();

  // Define the point coordinates
  double bounds[6];
  bounds[0] = -0.5;
  bounds[1] = 0.5;
  bounds[2] = -0.5;
  bounds[3] = 0.5;
  bounds[4] = -0.5;
  bounds[5] = 0.5;
  this->PlaceFactor = 1.0; //overload parent's value

  // Initial creation of the widget, serves to initialize it
  this->PlaceWidget(bounds);
  this->ClampToBounds = 0;

  // The bounding box
  this->BoundingBox = vtkBox::New();

  this->RepresentationState = vtkLineRepresentation::Outside;
  this->InitializedDisplayPosition = 0;
}

//----------------------------------------------------------------------------
vtkLineRepresentation::~vtkLineRepresentation()
{  
  if ( this->HandleRepresentation )
    {
    this->HandleRepresentation->Delete();
    }
  if ( this->Point1Representation )
    {
    this->Point1Representation->Delete();
    }
  if ( this->Point2Representation )
    {
    this->Point2Representation->Delete();
    }
  if ( this->LineHandleRepresentation )
    {
    this->LineHandleRepresentation->Delete();
    }

  this->LineActor->Delete();
  this->LineMapper->Delete();
  this->LineSource->Delete();

  for (int i=0; i<2; i++)
    {
    this->HandleGeometry[i]->Delete();
    this->HandleMapper[i]->Delete();
    this->Handle[i]->Delete();
    }
  delete [] this->Handle;
  delete [] this->HandleMapper;
  delete [] this->HandleGeometry;

  this->EndPointProperty->Delete();
  this->SelectedEndPointProperty->Delete();
  this->LineProperty->Delete();
  this->SelectedLineProperty->Delete();

  this->BoundingBox->Delete();
}

//----------------------------------------------------------------------
void vtkLineRepresentation::InstantiateHandleRepresentation()
{
  if ( ! this->Point1Representation )
    {
    this->Point1Representation = this->HandleRepresentation->NewInstance();
    this->Point1Representation->ShallowCopy(this->HandleRepresentation);
    }
  
  if ( ! this->Point2Representation )
    {
    this->Point2Representation = this->HandleRepresentation->NewInstance();
    this->Point2Representation->ShallowCopy(this->HandleRepresentation);
    }

  if ( ! this->LineHandleRepresentation )
    {
    this->LineHandleRepresentation = this->HandleRepresentation->NewInstance();
    this->LineHandleRepresentation->ShallowCopy(this->HandleRepresentation);
    }
}
  

//----------------------------------------------------------------------
void vtkLineRepresentation::SetResolution(int r)
{ 
  this->LineSource->SetResolution(r); 
}

//----------------------------------------------------------------------
int vtkLineRepresentation::GetResolution()
{ 
  return this->LineSource->GetResolution(); 
}

//----------------------------------------------------------------------
void vtkLineRepresentation::GetPolyData(vtkPolyData *pd)
{
  pd->ShallowCopy(this->LineSource->GetOutput());
}

//-- Set/Get position of the three handles -----------------------------
// Point1
//----------------------------------------------------------------------
void vtkLineRepresentation::GetPoint1WorldPosition(double pos[3])
{
  this->Point1Representation->GetWorldPosition(pos);
}

//----------------------------------------------------------------------
void vtkLineRepresentation::GetPoint1DisplayPosition(double pos[3])
{
  this->Point1Representation->GetDisplayPosition(pos);
}

//----------------------------------------------------------------------
void vtkLineRepresentation::SetPoint1WorldPosition(double x[3])
{
  this->Point1Representation->SetWorldPosition(x);
  double p[3];
  this->Point1Representation->GetDisplayPosition(p);
  this->Point1Representation->SetDisplayPosition(p);
}

//----------------------------------------------------------------------
void vtkLineRepresentation::SetPoint1DisplayPosition(double x[3])
{
  this->Point1Representation->SetDisplayPosition(x);
  double p[3];
  this->Point1Representation->GetWorldPosition(p);
  this->Point1Representation->SetWorldPosition(p);
}


// Point2
//----------------------------------------------------------------------
void vtkLineRepresentation::GetPoint2WorldPosition(double pos[3])
{
  this->Point2Representation->GetWorldPosition(pos);
}

//----------------------------------------------------------------------
void vtkLineRepresentation::GetPoint2DisplayPosition(double pos[3])
{
  this->Point2Representation->GetDisplayPosition(pos);
}

//----------------------------------------------------------------------
void vtkLineRepresentation::SetPoint2WorldPosition(double x[3])
{
  this->Point2Representation->SetWorldPosition(x);
  double p[3];
  this->Point2Representation->GetDisplayPosition(p);
  this->Point2Representation->SetDisplayPosition(p);
}

//----------------------------------------------------------------------
void vtkLineRepresentation::SetPoint2DisplayPosition(double x[3])
{
  this->Point2Representation->SetDisplayPosition(x);
  double p[3];
  this->Point2Representation->GetWorldPosition(p);
  this->Point2Representation->SetWorldPosition(p);
}

//----------------------------------------------------------------------
void vtkLineRepresentation::StartWidgetInteraction(double e[2])
{
  this->StartEventPosition[0] = e[0];
  this->StartEventPosition[1] = e[1];
  this->StartEventPosition[2] = 0.0;
  
  this->LastEventPosition[0] = e[0];
  this->LastEventPosition[1] = e[1];
  this->LastEventPosition[2] = 0.0;
}

//----------------------------------------------------------------------
void vtkLineRepresentation::WidgetInteraction(double e[2])
{
  // Process the motion
  if ( this->InteractionState == vtkLineRepresentation::OnLine )
    {
    double x[3], center[3], p1[3], p2[3], delta[3];

    // Get the new position
    this->LineHandleRepresentation->GetWorldPosition(x);

    // Compute the previous position
    this->Point1Representation->GetWorldPosition(p1);
    this->Point2Representation->GetWorldPosition(p2);
    center[0] = (p1[0]+p2[0]) / 2.0;
    center[1] = (p1[1]+p2[1]) / 2.0;
    center[2] = (p1[2]+p2[2]) / 2.0;

    delta[0] = x[0] - center[0];
    delta[1] = x[1] - center[1];
    delta[2] = x[2] - center[2];

    p1[0] += delta[0];
    p1[1] += delta[1];
    p1[2] += delta[2];

    p2[0] += delta[0];
    p2[1] += delta[1];
    p2[2] += delta[2];

    this->Point1Representation->SetWorldPosition(p1);
    this->Point2Representation->SetWorldPosition(p2);
    this->LineSource->SetPoint2(p1);
    this->LineSource->SetPoint2(p2);
    }

  this->LastEventPosition[0] = e[0];
  this->LastEventPosition[1] = e[1];
  this->LastEventPosition[2] = 0.0;
}

//----------------------------------------------------------------------------
void vtkLineRepresentation::PlaceWidget(double bds[6])
{
  int i;
  double bounds[6], center[3];

  double placeFactor = this->PlaceFactor;
  this->PlaceFactor = 1.0;
  this->AdjustBounds(bds, bounds, center);
  this->PlaceFactor = placeFactor;
  
  for (i=0; i<6; i++)
    {
    this->InitialBounds[i] = bounds[i];
    }
  this->InitialLength = sqrt((bounds[1]-bounds[0])*(bounds[1]-bounds[0]) +
                             (bounds[3]-bounds[2])*(bounds[3]-bounds[2]) +
                             (bounds[5]-bounds[4])*(bounds[5]-bounds[4]));

  // When PlaceWidget() is invoked, the widget orientation is preserved, but it
  // is allowed to translate and scale. This means it is centered in the
  // bounding box, and the representation scales itself to intersect the sides
  // of the bounding box. Thus we have to determine where Point1 and Point2
  // intersect the bounding box.
  double p1[3], p2[3], r[3], o[3], t, placedP1[3], placedP2[3];
  this->LineSource->GetPoint1(p1);
  this->LineSource->GetPoint2(p2);

  // Okay, this looks really weird, we are shooting rays from OUTSIDE
  // the bounding box back towards it. This is because the IntersectBox()
  // method computes intersections only if the ray originates outside the
  // bounding box.
  r[0] = this->InitialLength * (p1[0] - p2[0]);
  r[1] = this->InitialLength * (p1[1] - p2[1]);
  r[2] = this->InitialLength * (p1[2] - p2[2]);
  o[0] = center[0] - r[0];
  o[1] = center[1] - r[1];
  o[2] = center[2] - r[2];
  vtkBox::IntersectBox(bounds,o,r,placedP1,t);
  this->SetPoint1WorldPosition(placedP1);

  r[0] = this->InitialLength * (p2[0] - p1[0]);
  r[1] = this->InitialLength * (p2[1] - p1[1]);
  r[2] = this->InitialLength * (p2[2] - p1[2]);
  o[0] = center[0] - r[0];
  o[1] = center[1] - r[1];
  o[2] = center[2] - r[2];
  vtkBox::IntersectBox(bounds,o,r,placedP2,t);
  this->SetPoint2WorldPosition(placedP2);

  // Initialize the center point
  this->LineHandleRepresentation->SetWorldPosition(center);

  // Position the handles at the end of the lines
  this->Placed = 1;
  this->BuildRepresentation();
}


//----------------------------------------------------------------------------
int vtkLineRepresentation::ComputeInteractionState(int X, int Y, int vtkNotUsed(modify))
{
  // See if we are near one of the end points or outside
  double pos1[3], pos2[3];
  this->GetPoint1DisplayPosition(pos1);
  this->GetPoint2DisplayPosition(pos2);
  
  double p1[3], p2[3], xyz[3];
  double t, closest[3];
  xyz[0] = static_cast<double>(X);
  xyz[1] = static_cast<double>(Y);
  p1[0] = static_cast<double>(pos1[0]);
  p1[1] = static_cast<double>(pos1[1]);
  p2[0] = static_cast<double>(pos2[0]);
  p2[1] = static_cast<double>(pos2[1]);
  xyz[2] = p1[2] = p2[2] = 0.0;

  double tol2 = this->Tolerance*this->Tolerance;
  // Check if we are on end points
  if ( vtkMath::Distance2BetweenPoints(xyz,p1) <= tol2 )
    {
    this->InteractionState = vtkLineRepresentation::NearP1;
    this->SetRepresentationState(vtkLineRepresentation::NearP1);
    return this->InteractionState;
    }
  else if ( vtkMath::Distance2BetweenPoints(xyz,p2) <= tol2 )
    {
    this->InteractionState = vtkLineRepresentation::NearP2;
    this->SetRepresentationState(vtkLineRepresentation::NearP2);
    return this->InteractionState;
    }

  // Check if we are on edges
  int onLine = (vtkLine::DistanceToLine(xyz,p1,p2,t,closest) <= tol2);
  if ( onLine )
    {
    this->InteractionState = vtkLineRepresentation::OnLine;
    this->SetRepresentationState(vtkLineRepresentation::OnLine);
    this->GetPoint1WorldPosition(pos1);
    this->GetPoint2WorldPosition(pos2);
    closest[0] = pos1[0] + t*(pos2[0]-pos1[0]);
    closest[1] = pos1[1] + t*(pos2[1]-pos1[1]);
    closest[2] = pos1[2] + t*(pos2[2]-pos1[2]);
    this->LineHandleRepresentation->SetWorldPosition(closest);
    }
  else
    {
    this->InteractionState = vtkLineRepresentation::Outside;
    this->SetRepresentationState(vtkLineRepresentation::Outside);
    }
  
  return this->InteractionState;
}

//----------------------------------------------------------------------------
void vtkLineRepresentation::SetRepresentationState(int state)
{
  if (this->RepresentationState == state)
    {
    return;
    }
  this->RepresentationState = state;
  this->Modified();
  
  if ( state == vtkLineRepresentation::NearP1 )
    {
    this->HighlightPoint(0,1);
    this->HighlightPoint(1,0);
    this->HighlightLine(0);
    }
  else if ( state == vtkLineRepresentation::NearP2 )
    {
    this->HighlightPoint(0,0);
    this->HighlightPoint(1,1);
    this->HighlightLine(0);
    }
  else if ( state == vtkLineRepresentation::OnLine )
    {
    this->HighlightPoint(0,0);
    this->HighlightPoint(1,0);
    this->HighlightLine(1);
    }
  else
    {
    this->HighlightPoint(0,0);
    this->HighlightPoint(1,0);
    this->HighlightLine(0);
    }
}

//----------------------------------------------------------------------
double *vtkLineRepresentation::GetBounds()
{
  this->BuildRepresentation();
  this->BoundingBox->SetBounds(this->LineActor->GetBounds());
  this->BoundingBox->AddBounds(this->Handle[0]->GetBounds());
  this->BoundingBox->AddBounds(this->Handle[1]->GetBounds());

  return this->BoundingBox->GetBounds();
}

//----------------------------------------------------------------------------
void vtkLineRepresentation::CreateDefaultProperties()
{
  // Endpoint properties
  this->EndPointProperty = vtkProperty::New();
  this->EndPointProperty->SetColor(1,1,1);

  this->SelectedEndPointProperty = vtkProperty::New();
  this->SelectedEndPointProperty->SetColor(0,1,0);

  // Line properties
  this->LineProperty = vtkProperty::New();
  this->LineProperty->SetAmbient(1.0);
  this->LineProperty->SetAmbientColor(1.0,1.0,1.0);
  this->LineProperty->SetLineWidth(2.0);

  this->SelectedLineProperty = vtkProperty::New();
  this->SelectedLineProperty->SetAmbient(1.0);
  this->SelectedLineProperty->SetAmbientColor(0.0,1.0,0.0);
  this->SelectedLineProperty->SetLineWidth(2.0);
}

//----------------------------------------------------------------------------
void vtkLineRepresentation::SizeHandles()
{
  // The SizeHandles() method depends on the LastPickPosition data member.
  double radius = this->vtkWidgetRepresentation::SizeHandlesInPixels(1.35,this->LineSource->GetPoint1());
  this->HandleGeometry[0]->SetRadius(radius);

  radius = this->vtkWidgetRepresentation::SizeHandlesInPixels(1.35,this->LineSource->GetPoint2());
  this->HandleGeometry[1]->SetRadius(radius);
}

//----------------------------------------------------------------------------
void vtkLineRepresentation::BuildRepresentation()
{
  // The net effect is to resize the handle
  if ( this->GetMTime() > this->BuildTime ||
       this->Point1Representation->GetMTime() > this->BuildTime ||
       this->Point2Representation->GetMTime() > this->BuildTime ||
       this->LineHandleRepresentation->GetMTime() > this->BuildTime ||
       (this->Renderer && this->Renderer->GetVTKWindow() &&
        this->Renderer->GetVTKWindow()->GetMTime() > this->BuildTime) )
    {
    if ( ! this->InitializedDisplayPosition && this->Renderer )
      {
      this->SetPoint1WorldPosition(this->LineSource->GetPoint1());
      this->SetPoint2WorldPosition(this->LineSource->GetPoint2());
      this->ValidPick = 1;
      this->InitializedDisplayPosition = 1;
      }

    double x[3];
    this->GetPoint1WorldPosition(x);
    this->LineSource->SetPoint1(x);
    this->GetPoint2WorldPosition(x);
    this->LineSource->SetPoint2(x);

    this->HandleGeometry[0]->SetCenter(this->LineSource->GetPoint1());
    this->HandleGeometry[1]->SetCenter(this->LineSource->GetPoint2());

    this->SizeHandles();
    this->BuildTime.Modified();
    }
}

//----------------------------------------------------------------------------
void vtkLineRepresentation::HighlightPoint(int ptId, int highlight)
{
  if ( ptId == 0 )
    {
    if ( highlight )
      {
      this->Handle[0]->SetProperty(this->SelectedEndPointProperty);
      this->Point1Representation->SetSelectedProperty(this->SelectedEndPointProperty);
      }
    else
      {
      this->Handle[0]->SetProperty(this->EndPointProperty);
      this->Point1Representation->SetProperty(this->EndPointProperty);
      }
    }
  else if ( ptId == 1 )
    {
    if ( highlight )
      {
      this->Handle[1]->SetProperty(this->SelectedEndPointProperty);
      this->Point2Representation->SetSelectedProperty(this->SelectedEndPointProperty);
      }
    else
      {
      this->Handle[1]->SetProperty(this->EndPointProperty);
      this->Point2Representation->SetProperty(this->EndPointProperty);
      }
    }
  else //if ( ptId == 2 )
    {
    if ( highlight )
      {
      this->LineHandleRepresentation->SetSelectedProperty(this->SelectedEndPointProperty);
      }
    else
      {
      this->LineHandleRepresentation->SetProperty(this->EndPointProperty);
      }
    }
}

//----------------------------------------------------------------------------
void vtkLineRepresentation::HighlightLine(int highlight)
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
void vtkLineRepresentation::ClampPosition(double x[3])
{
  for (int i=0; i<3; i++)
    {
    if ( x[i] < this->InitialBounds[2*i] )
      {
      x[i] = this->InitialBounds[2*i];
      }
    if ( x[i] > this->InitialBounds[2*i+1] )
      {
      x[i] = this->InitialBounds[2*i+1];
      }
    }
}

//----------------------------------------------------------------------------
int vtkLineRepresentation::InBounds(double x[3])
{
  for (int i=0; i<3; i++)
    {
    if ( x[i] < this->InitialBounds[2*i] ||
         x[i] > this->InitialBounds[2*i+1] )
      {
      return 0;
      }
    }
  return 1;
}

//----------------------------------------------------------------------------
void vtkLineRepresentation::ReleaseGraphicsResources(vtkWindow *w)
{
  this->LineActor->ReleaseGraphicsResources(w);
  this->Handle[0]->ReleaseGraphicsResources(w);
  this->Handle[1]->ReleaseGraphicsResources(w);
}

//----------------------------------------------------------------------------
int vtkLineRepresentation::RenderOpaqueGeometry(vtkViewport *v)
{
  int count=0;
  this->BuildRepresentation();
  count += this->LineActor->RenderOpaqueGeometry(v);
  count += this->Handle[0]->RenderOpaqueGeometry(v);
  count += this->Handle[1]->RenderOpaqueGeometry(v);
  
  return count;
}

//----------------------------------------------------------------------------
int vtkLineRepresentation::RenderTranslucentGeometry(vtkViewport *v)
{
  int count=0;
  this->BuildRepresentation();
  count += this->LineActor->RenderOpaqueGeometry(v);
  count += this->Handle[0]->RenderTranslucentGeometry(v);
  count += this->Handle[1]->RenderTranslucentGeometry(v);
  
  return count;
}

//----------------------------------------------------------------------------
unsigned long vtkLineRepresentation::GetMTime()
{
  unsigned long mTime=this->Superclass::GetMTime();
  unsigned long mTime2=this->Point1Representation->GetMTime();
  mTime = ( mTime2 > mTime ? mTime2 : mTime );
  mTime2=this->Point2Representation->GetMTime();
  mTime = ( mTime2 > mTime ? mTime2 : mTime );
  mTime2=this->LineHandleRepresentation->GetMTime();
  mTime = ( mTime2 > mTime ? mTime2 : mTime );
  
  return mTime;
}

//----------------------------------------------------------------------------
void vtkLineRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

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

  os << indent << "Constrain To Bounds: "
     << (this->ClampToBounds ? "On\n" : "Off\n");

  int res = this->LineSource->GetResolution();
  double *pt1 = this->LineSource->GetPoint1();
  double *pt2 = this->LineSource->GetPoint2();

  os << indent << "Resolution: " << res << "\n";
  os << indent << "Point 1: (" << pt1[0] << ", "
                               << pt1[1] << ", "
                               << pt1[2] << ")\n";
  os << indent << "Point 2: (" << pt2[0] << ", "
                               << pt2[1] << ", "
                               << pt2[2] << ")\n";
}


