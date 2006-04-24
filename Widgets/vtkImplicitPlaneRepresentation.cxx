/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImplicitPlaneRepresentation.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImplicitPlaneRepresentation.h"

#include "vtkActor.h"
#include "vtkAssemblyNode.h"
#include "vtkAssemblyPath.h"
#include "vtkCallbackCommand.h"
#include "vtkCamera.h"
#include "vtkCellPicker.h"
#include "vtkConeSource.h"
#include "vtkCutter.h"
#include "vtkFeatureEdges.h"
#include "vtkImageData.h"
#include "vtkLineSource.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkOutlineFilter.h"
#include "vtkPlane.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"
#include "vtkTransform.h"
#include "vtkTubeFilter.h"
#include "vtkInteractorObserver.h"

vtkCxxRevisionMacro(vtkImplicitPlaneRepresentation, "1.1");
vtkStandardNewMacro(vtkImplicitPlaneRepresentation);

//----------------------------------------------------------------------------
vtkImplicitPlaneRepresentation::vtkImplicitPlaneRepresentation()
{
  this->NormalToXAxis = 0;
  this->NormalToYAxis = 0;
  this->NormalToZAxis = 0;

  // Build the representation of the widget
  // 
  this->Plane = vtkPlane::New();
  this->Plane->SetNormal(0,0,1);
  this->Plane->SetOrigin(0,0,0);

  this->Box = vtkImageData::New();
  this->Box->SetDimensions(2,2,2);
  this->Outline = vtkOutlineFilter::New();
  this->Outline->SetInput(this->Box);
  this->OutlineMapper = vtkPolyDataMapper::New();
  this->OutlineMapper->SetInput(this->Outline->GetOutput());
  this->OutlineActor = vtkActor::New();
  this->OutlineActor->SetMapper(this->OutlineMapper);
  this->OutlineTranslation = 1;
  this->ScaleEnabled = 1;
  this->OutsideBounds = 1;
  
  this->Cutter = vtkCutter::New();
  this->Cutter->SetInput(this->Box);
  this->Cutter->SetCutFunction(this->Plane);
  this->CutMapper = vtkPolyDataMapper::New();
  this->CutMapper->SetInput(this->Cutter->GetOutput());
  this->CutActor = vtkActor::New();
  this->CutActor->SetMapper(this->CutMapper);
  this->DrawPlane = 1;
  
  this->Edges = vtkFeatureEdges::New();
  this->Edges->SetInput(this->Cutter->GetOutput());
  this->EdgesTuber = vtkTubeFilter::New();
  this->EdgesTuber->SetInput(this->Edges->GetOutput());
  this->EdgesTuber->SetNumberOfSides(12);
  this->EdgesMapper = vtkPolyDataMapper::New();
  this->EdgesMapper->SetInput(this->EdgesTuber->GetOutput());
  this->EdgesActor = vtkActor::New();
  this->EdgesActor->SetMapper(this->EdgesMapper);
  this->Tubing = 1; //control whether tubing is on

  // Create the + plane normal
  this->LineSource = vtkLineSource::New();
  this->LineSource->SetResolution(1);
  this->LineMapper = vtkPolyDataMapper::New();
  this->LineMapper->SetInput(this->LineSource->GetOutput());
  this->LineActor = vtkActor::New();
  this->LineActor->SetMapper(this->LineMapper);

  this->ConeSource = vtkConeSource::New();
  this->ConeSource->SetResolution(12);
  this->ConeSource->SetAngle(25.0);
  this->ConeMapper = vtkPolyDataMapper::New();
  this->ConeMapper->SetInput(this->ConeSource->GetOutput());
  this->ConeActor = vtkActor::New();
  this->ConeActor->SetMapper(this->ConeMapper);

  // Create the - plane normal
  this->LineSource2 = vtkLineSource::New();
  this->LineSource2->SetResolution(1);
  this->LineMapper2 = vtkPolyDataMapper::New();
  this->LineMapper2->SetInput(this->LineSource2->GetOutput());
  this->LineActor2 = vtkActor::New();
  this->LineActor2->SetMapper(this->LineMapper2);

  this->ConeSource2 = vtkConeSource::New();
  this->ConeSource2->SetResolution(12);
  this->ConeSource2->SetAngle(25.0);
  this->ConeMapper2 = vtkPolyDataMapper::New();
  this->ConeMapper2->SetInput(this->ConeSource2->GetOutput());
  this->ConeActor2 = vtkActor::New();
  this->ConeActor2->SetMapper(this->ConeMapper2);

  // Create the origin handle
  this->Sphere = vtkSphereSource::New();
  this->Sphere->SetThetaResolution(16);
  this->Sphere->SetPhiResolution(8);
  this->SphereMapper = vtkPolyDataMapper::New();
  this->SphereMapper->SetInput(this->Sphere->GetOutput());
  this->SphereActor = vtkActor::New();
  this->SphereActor->SetMapper(this->SphereMapper);

  this->Transform = vtkTransform::New();

  // Define the point coordinates
  double bounds[6];
  bounds[0] = -0.5;
  bounds[1] = 0.5;
  bounds[2] = -0.5;
  bounds[3] = 0.5;
  bounds[4] = -0.5;
  bounds[5] = 0.5;

  // Initial creation of the widget, serves to initialize it
  this->PlaceWidget(bounds);

  //Manage the picking stuff
  this->Picker = vtkCellPicker::New();
  this->Picker->SetTolerance(0.005);
  this->Picker->AddPickList(this->CutActor);
  this->Picker->AddPickList(this->LineActor);
  this->Picker->AddPickList(this->ConeActor);
  this->Picker->AddPickList(this->LineActor2);
  this->Picker->AddPickList(this->ConeActor2);
  this->Picker->AddPickList(this->SphereActor);
  this->Picker->AddPickList(this->OutlineActor);
  this->Picker->PickFromListOn();
  
  // Set up the initial properties
  this->CreateDefaultProperties();
}

//----------------------------------------------------------------------------
vtkImplicitPlaneRepresentation::~vtkImplicitPlaneRepresentation()
{  
  this->Plane->Delete();
  this->Box->Delete();
  this->Outline->Delete();
  this->OutlineMapper->Delete();
  this->OutlineActor->Delete();
  
  this->Cutter->Delete();
  this->CutMapper->Delete();
  this->CutActor->Delete();
  
  this->Edges->Delete();
  this->EdgesTuber->Delete();
  this->EdgesMapper->Delete();
  this->EdgesActor->Delete();
  
  this->LineSource->Delete();
  this->LineMapper->Delete();
  this->LineActor->Delete();

  this->ConeSource->Delete();
  this->ConeMapper->Delete();
  this->ConeActor->Delete();

  this->LineSource2->Delete();
  this->LineMapper2->Delete();
  this->LineActor2->Delete();

  this->ConeSource2->Delete();
  this->ConeMapper2->Delete();
  this->ConeActor2->Delete();

  this->Sphere->Delete();
  this->SphereMapper->Delete();
  this->SphereActor->Delete();

  this->Transform->Delete();

  this->Picker->Delete();

  this->NormalProperty->Delete();
  this->SelectedNormalProperty->Delete();
  this->PlaneProperty->Delete();
  this->SelectedPlaneProperty->Delete();
  this->OutlineProperty->Delete();
  this->SelectedOutlineProperty->Delete();
  this->EdgesProperty->Delete();
}

//----------------------------------------------------------------------------
int vtkImplicitPlaneRepresentation::ComputeInteractionState(int X, int Y, 
                                                            int vtkNotUsed(modify))
{
  // See if anything has been selected
  vtkAssemblyPath *path;
  this->Picker->Pick(X,Y,0.0,this->Renderer);
  path = this->Picker->GetPath();

  if ( path == NULL ) //not picking this widget
    {
    this->HighlightPlane(0);
    this->HighlightNormal(0);
    this->HighlightOutline(0);
    this->InteractionState = vtkImplicitPlaneRepresentation::Outside;
    return this->InteractionState;
    }
    
  // Something picked, continue
  this->ValidPick = 1;
  this->Picker->GetPickPosition(this->LastPickPosition);

  // Depending on the interaction state (set by the widget) we modify
  // this state based on what is picked.
  if ( this->InteractionState == vtkImplicitPlaneRepresentation::Moving )
    {
    vtkProp *prop = path->GetFirstNode()->GetViewProp();
    if ( prop == this->ConeActor || prop == this->LineActor ||
         prop == this->ConeActor2 || prop == this->LineActor2 )
      {
      this->HighlightPlane(1);
      this->HighlightNormal(1);
      this->InteractionState = vtkImplicitPlaneRepresentation::Rotating;
      }
    else if ( prop == this->CutActor )
      {
      this->HighlightPlane(1);
      this->InteractionState = vtkImplicitPlaneRepresentation::Pushing;
      }
    else if ( prop == this->SphereActor )
      {
      this->HighlightNormal(1);
      this->InteractionState = vtkImplicitPlaneRepresentation::MovingOrigin;
      }
    else
      {
      if ( this->OutlineTranslation )
        {
        this->HighlightOutline(1);
        this->InteractionState = vtkImplicitPlaneRepresentation::MovingOutline;
        }
      else
        {
        this->InteractionState = vtkImplicitPlaneRepresentation::Outside;
        }
      }
    }

  else if ( this->InteractionState == vtkImplicitPlaneRepresentation::MovingPlane )
    {
    return this->InteractionState;
    }
  
  else if ( this->InteractionState == vtkImplicitPlaneRepresentation::Scaling )
    {
    return this->InteractionState;
    }

  else
    {
    this->InteractionState = vtkImplicitPlaneRepresentation::Outside;
    }
  
  return this->InteractionState;
}


//----------------------------------------------------------------------------
void vtkImplicitPlaneRepresentation::StartWidgetInteraction(double e[2])
{
  this->StartEventPosition[0] = e[0];
  this->StartEventPosition[1] = e[1];
  this->StartEventPosition[2] = 0.0;
  
  this->LastEventPosition[0] = e[0];
  this->LastEventPosition[1] = e[1];
  this->LastEventPosition[2] = 0.0;
  
  if ( this->InteractionState == vtkImplicitPlaneRepresentation::Rotating )
    {
    this->HighlightNormal(1);
    this->HighlightPlane(1);
    }
  else if ( this->InteractionState == vtkImplicitPlaneRepresentation::Pushing )
    {
    this->HighlightPlane(1);
    }
  else if ( this->InteractionState == vtkImplicitPlaneRepresentation::MovingOrigin )
    {
    this->HighlightNormal(1);
    }
  else if ( this->InteractionState == vtkImplicitPlaneRepresentation::MovingOutline )
    {
    this->HighlightOutline(1);
    }
  else if ( this->InteractionState == vtkImplicitPlaneRepresentation::MovingPlane )
    {
    this->HighlightNormal(1);
    this->HighlightPlane(1);
    }
  else if ( this->InteractionState == vtkImplicitPlaneRepresentation::Scaling )
    {
    this->HighlightNormal(1);
    this->HighlightPlane(1);
    this->HighlightOutline(1);
    }
  else
    {
    this->HighlightNormal(0);
    this->HighlightPlane(0);
    this->HighlightOutline(0);
    }
}

//----------------------------------------------------------------------------
void vtkImplicitPlaneRepresentation::WidgetInteraction(double e[2])
{
  // Do different things depending on state
  // Calculations everybody does
  double focalPoint[4], pickPoint[4], prevPickPoint[4];
  double z, vpn[3];

  vtkCamera *camera = this->Renderer->GetActiveCamera();
  if ( !camera )
    {
    return;
    }

  // Compute the two points defining the motion vector
  vtkInteractorObserver::ComputeWorldToDisplay(this->Renderer,this->LastPickPosition[0], 
                                               this->LastPickPosition[1],
                                               this->LastPickPosition[2], focalPoint);
  z = focalPoint[2];
  vtkInteractorObserver::ComputeDisplayToWorld(this->Renderer,this->LastEventPosition[0],
                                               this->LastEventPosition[1], z, prevPickPoint);
  vtkInteractorObserver::ComputeDisplayToWorld(this->Renderer, e[0], e[1], z, pickPoint);

  // Process the motion
  if ( this->InteractionState == vtkImplicitPlaneRepresentation::MovingPlane )
    {
    this->TranslatePlane(prevPickPoint, pickPoint);
    }
  else if ( this->InteractionState == vtkImplicitPlaneRepresentation::MovingOutline )
    {
    this->TranslateOutline(prevPickPoint, pickPoint);
    }
  else if ( this->InteractionState == vtkImplicitPlaneRepresentation::MovingOrigin )
    {
    this->TranslateOrigin(prevPickPoint, pickPoint);
    }
  else if ( this->InteractionState == vtkImplicitPlaneRepresentation::Pushing )
    {
    this->Push(prevPickPoint, pickPoint);
    }
  else if ( this->InteractionState == vtkImplicitPlaneRepresentation::Scaling )
    {
    this->Scale(prevPickPoint, pickPoint, e[0], e[1]);
    }
  else if ( this->InteractionState == vtkImplicitPlaneRepresentation::Rotating )
    {
    camera->GetViewPlaneNormal(vpn);
    this->Rotate(e[0], e[1], prevPickPoint, pickPoint, vpn);
    }

  this->LastEventPosition[0] = e[0];
  this->LastEventPosition[1] = e[1];
  this->LastEventPosition[2] = 0.0;
}

//----------------------------------------------------------------------------
void vtkImplicitPlaneRepresentation::EndWidgetInteraction(double* vtkNotUsed(e[2]))
{
  this->HighlightPlane(0);
  this->HighlightOutline(0);
  this->HighlightNormal(0);
  this->SizeHandles();
}

//----------------------------------------------------------------------------
void vtkImplicitPlaneRepresentation::ReleaseGraphicsResources(vtkWindow *w)
{
  this->OutlineActor->ReleaseGraphicsResources(w);
  this->CutActor->ReleaseGraphicsResources(w);
  this->EdgesActor->ReleaseGraphicsResources(w);
  this->ConeActor->ReleaseGraphicsResources(w);
  this->LineActor->ReleaseGraphicsResources(w);
  this->ConeActor2->ReleaseGraphicsResources(w);
  this->LineActor2->ReleaseGraphicsResources(w);
  this->SphereActor->ReleaseGraphicsResources(w);
}

//----------------------------------------------------------------------------
int vtkImplicitPlaneRepresentation::RenderOpaqueGeometry(vtkViewport *v)
{
  int count=0;
  this->BuildRepresentation();
  count += this->OutlineActor->RenderOpaqueGeometry(v);
  count += this->EdgesActor->RenderOpaqueGeometry(v);
  count += this->ConeActor->RenderOpaqueGeometry(v);
  count += this->LineActor->RenderOpaqueGeometry(v);
  count += this->ConeActor2->RenderOpaqueGeometry(v);
  count += this->LineActor2->RenderOpaqueGeometry(v);
  count += this->SphereActor->RenderOpaqueGeometry(v);
  if ( this->DrawPlane )
    {
    count += this->CutActor->RenderOpaqueGeometry(v);
    }
  
  return count;
}

//----------------------------------------------------------------------------
int vtkImplicitPlaneRepresentation::RenderTranslucentGeometry(vtkViewport *v)
{
  int count=0;
  this->BuildRepresentation();
  count += this->OutlineActor->RenderTranslucentGeometry(v);
  count += this->EdgesActor->RenderTranslucentGeometry(v);
  count += this->ConeActor->RenderTranslucentGeometry(v);
  count += this->LineActor->RenderTranslucentGeometry(v);
  count += this->ConeActor2->RenderTranslucentGeometry(v);
  count += this->LineActor2->RenderTranslucentGeometry(v);
  count += this->SphereActor->RenderTranslucentGeometry(v);
  if ( this->DrawPlane )
    {
    count += this->CutActor->RenderTranslucentGeometry(v);
    }
  
  return count;
}

//----------------------------------------------------------------------------
void vtkImplicitPlaneRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  if ( this->NormalProperty )
    {
    os << indent << "Normal Property: " << this->NormalProperty << "\n";
    }
  else
    {
    os << indent << "Normal Property: (none)\n";
    }
  if ( this->SelectedNormalProperty )
    {
    os << indent << "Selected Normal Property: " 
       << this->SelectedNormalProperty << "\n";
    }
  else
    {
    os << indent << "Selected Normal Property: (none)\n";
    }

  if ( this->PlaneProperty )
    {
    os << indent << "Plane Property: " << this->PlaneProperty << "\n";
    }
  else
    {
    os << indent << "Plane Property: (none)\n";
    }
  if ( this->SelectedPlaneProperty )
    {
    os << indent << "Selected Plane Property: " 
       << this->SelectedPlaneProperty << "\n";
    }
  else
    {
    os << indent << "Selected Plane Property: (none)\n";
    }

  if ( this->OutlineProperty )
    {
    os << indent << "Outline Property: " << this->OutlineProperty << "\n";
    }
  else
    {
    os << indent << "Outline Property: (none)\n";
    }
  if ( this->SelectedOutlineProperty )
    {
    os << indent << "Selected Outline Property: " 
       << this->SelectedOutlineProperty << "\n";
    }
  else
    {
    os << indent << "Selected Outline Property: (none)\n";
    }

  if ( this->EdgesProperty )
    {
    os << indent << "Edges Property: " << this->EdgesProperty << "\n";
    }
  else
    {
    os << indent << "Edges Property: (none)\n";
    }

  os << indent << "Normal To X Axis: " 
     << (this->NormalToXAxis ? "On" : "Off") << "\n";
  os << indent << "Normal To Y Axis: " 
     << (this->NormalToYAxis ? "On" : "Off") << "\n";
  os << indent << "Normal To Z Axis: " 
     << (this->NormalToZAxis ? "On" : "Off") << "\n";

  os << indent << "Tubing: " << (this->Tubing ? "On" : "Off") << "\n";
  os << indent << "Outline Translation: " 
     << (this->OutlineTranslation ? "On" : "Off") << "\n";
  os << indent << "Outside Bounds: " 
     << (this->OutsideBounds ? "On" : "Off") << "\n";
  os << indent << "Scale Enabled: " 
     << (this->ScaleEnabled ? "On" : "Off") << "\n";
  os << indent << "Draw Plane: " << (this->DrawPlane ? "On" : "Off") << "\n";
}


//----------------------------------------------------------------------------
void vtkImplicitPlaneRepresentation::HighlightNormal(int highlight)
{
  if ( highlight )
    {
    this->LineActor->SetProperty(this->SelectedNormalProperty);
    this->ConeActor->SetProperty(this->SelectedNormalProperty);
    this->LineActor2->SetProperty(this->SelectedNormalProperty);
    this->ConeActor2->SetProperty(this->SelectedNormalProperty);
    this->SphereActor->SetProperty(this->SelectedNormalProperty);
    }
  else
    {
    this->LineActor->SetProperty(this->NormalProperty);
    this->ConeActor->SetProperty(this->NormalProperty);
    this->LineActor2->SetProperty(this->NormalProperty);
    this->ConeActor2->SetProperty(this->NormalProperty);
    this->SphereActor->SetProperty(this->NormalProperty);
    }
}


//----------------------------------------------------------------------------
void vtkImplicitPlaneRepresentation::HighlightPlane(int highlight)
{
  if ( highlight )
    {
    this->CutActor->SetProperty(this->SelectedPlaneProperty);
    }
  else
    {
    this->CutActor->SetProperty(this->PlaneProperty);
    }
}


//----------------------------------------------------------------------------
void vtkImplicitPlaneRepresentation::HighlightOutline(int highlight)
{
  if ( highlight )
    {
    this->OutlineActor->SetProperty(this->SelectedOutlineProperty);
    }
  else
    {
    this->OutlineActor->SetProperty(this->OutlineProperty);
    }
}

//----------------------------------------------------------------------------
void vtkImplicitPlaneRepresentation::Rotate(int X, int Y, double *p1, double *p2, double *vpn)
{
  double v[3]; //vector of motion
  double axis[3]; //axis of rotation
  double theta; //rotation angle

  // mouse motion vector in world space
  v[0] = p2[0] - p1[0];
  v[1] = p2[1] - p1[1];
  v[2] = p2[2] - p1[2];

  double *origin = this->Plane->GetOrigin();
  double *normal = this->Plane->GetNormal();

  // Create axis of rotation and angle of rotation
  vtkMath::Cross(vpn,v,axis);
  if ( vtkMath::Normalize(axis) == 0.0 )
    {
    return;
    }
  int *size = this->Renderer->GetSize();
  double l2 = (X-this->LastEventPosition[0])*(X-this->LastEventPosition[0]) + (Y-this->LastEventPosition[1])*(Y-this->LastEventPosition[1]);
  theta = 360.0 * sqrt(l2/((double)size[0]*size[0]+size[1]*size[1]));

  //Manipulate the transform to reflect the rotation
  this->Transform->Identity();
  this->Transform->Translate(origin[0],origin[1],origin[2]);
  this->Transform->RotateWXYZ(theta,axis);
  this->Transform->Translate(-origin[0],-origin[1],-origin[2]);

  //Set the new normal
  double nNew[3];
  this->Transform->TransformNormal(normal,nNew);
  this->Plane->SetNormal(nNew);
  
  this->BuildRepresentation();
}

//----------------------------------------------------------------------------
// Loop through all points and translate them
void vtkImplicitPlaneRepresentation::TranslatePlane(double *p1, double *p2)
{
  //Get the motion vector
  double v[3];
  v[0] = p2[0] - p1[0];
  v[1] = p2[1] - p1[1];
  v[2] = p2[2] - p1[2];
  
  //Translate the plane
  double oNew[3];
  double *origin = this->Plane->GetOrigin();
  oNew[0] = origin[0] + v[0];
  oNew[1] = origin[1] + v[1];
  oNew[2] = origin[2] + v[2];
  this->Plane->SetOrigin(oNew);

  this->BuildRepresentation();
}

//----------------------------------------------------------------------------
// Loop through all points and translate them
void vtkImplicitPlaneRepresentation::TranslateOutline(double *p1, double *p2)
{
  //Get the motion vector
  double v[3];
  v[0] = p2[0] - p1[0];
  v[1] = p2[1] - p1[1];
  v[2] = p2[2] - p1[2];
  
  //Translate the bounding box
  double *origin = this->Box->GetOrigin();
  double oNew[3];
  oNew[0] = origin[0] + v[0];
  oNew[1] = origin[1] + v[1];
  oNew[2] = origin[2] + v[2];
  this->Box->SetOrigin(oNew);

  //Translate the plane
  origin = this->Plane->GetOrigin();
  oNew[0] = origin[0] + v[0];
  oNew[1] = origin[1] + v[1];
  oNew[2] = origin[2] + v[2];
  this->Plane->SetOrigin(oNew);

  this->BuildRepresentation();
}

//----------------------------------------------------------------------------
// Loop through all points and translate them
void vtkImplicitPlaneRepresentation::TranslateOrigin(double *p1, double *p2)
{
  //Get the motion vector
  double v[3];
  v[0] = p2[0] - p1[0];
  v[1] = p2[1] - p1[1];
  v[2] = p2[2] - p1[2];
  
  //Add to the current point, project back down onto plane
  double *o = this->Plane->GetOrigin();
  double *n = this->Plane->GetNormal();
  double newOrigin[3];

  newOrigin[0] = o[0] + v[0];
  newOrigin[1] = o[1] + v[1];
  newOrigin[2] = o[2] + v[2];
  
  vtkPlane::ProjectPoint(newOrigin,o,n,newOrigin);
  this->SetOrigin(newOrigin[0],newOrigin[1],newOrigin[2]);
  this->BuildRepresentation();
}

//----------------------------------------------------------------------------
void vtkImplicitPlaneRepresentation::Scale(double *p1, double *p2, 
                                   int vtkNotUsed(X), int Y)
{
  //Get the motion vector
  double v[3];
  v[0] = p2[0] - p1[0];
  v[1] = p2[1] - p1[1];
  v[2] = p2[2] - p1[2];

  double *o = this->Plane->GetOrigin();

  // Compute the scale factor
  double sf = vtkMath::Norm(v) / this->Outline->GetOutput()->GetLength();
  if ( Y > this->LastEventPosition[1] )
    {
    sf = 1.0 + sf;
    }
  else
    {
    sf = 1.0 - sf;
    }
  
  this->Transform->Identity();
  this->Transform->Translate(o[0],o[1],o[2]);
  this->Transform->Scale(sf,sf,sf);
  this->Transform->Translate(-o[0],-o[1],-o[2]);

  double *origin = this->Box->GetOrigin();
  double *spacing = this->Box->GetSpacing();
  double oNew[3], p[3], pNew[3];
  p[0] = origin[0] + spacing[0];
  p[1] = origin[1] + spacing[1];
  p[2] = origin[2] + spacing[2];

  this->Transform->TransformPoint(origin,oNew);
  this->Transform->TransformPoint(p,pNew);

  this->Box->SetOrigin(oNew);
  this->Box->SetSpacing( (pNew[0]-oNew[0]), 
                         (pNew[1]-oNew[1]), 
                         (pNew[2]-oNew[2]) );

  this->BuildRepresentation();
}

//----------------------------------------------------------------------------
void vtkImplicitPlaneRepresentation::Push(double *p1, double *p2)
{
  //Get the motion vector
  double v[3];
  v[0] = p2[0] - p1[0];
  v[1] = p2[1] - p1[1];
  v[2] = p2[2] - p1[2];
  
  this->Plane->Push( vtkMath::Dot(v,this->Plane->GetNormal()) );
  this->SetOrigin(this->Plane->GetOrigin());
  this->BuildRepresentation();
}

//----------------------------------------------------------------------------
void vtkImplicitPlaneRepresentation::CreateDefaultProperties()
{
  // Normal properties
  this->NormalProperty = vtkProperty::New();
  this->NormalProperty->SetColor(1,1,1);
  this->NormalProperty->SetLineWidth(2);

  this->SelectedNormalProperty = vtkProperty::New();
  this->SelectedNormalProperty->SetColor(1,0,0);
  this->NormalProperty->SetLineWidth(2);

  // Plane properties
  this->PlaneProperty = vtkProperty::New();
  this->PlaneProperty->SetAmbient(1.0);
  this->PlaneProperty->SetAmbientColor(1.0,1.0,1.0);

  this->SelectedPlaneProperty = vtkProperty::New();
  this->SelectedPlaneProperty->SetAmbient(1.0);
  this->SelectedPlaneProperty->SetAmbientColor(0.0,1.0,0.0);
  this->SelectedPlaneProperty->SetOpacity(0.25);

  // Outline properties
  this->OutlineProperty = vtkProperty::New();
  this->OutlineProperty->SetAmbient(1.0);
  this->OutlineProperty->SetAmbientColor(1.0,1.0,1.0);

  this->SelectedOutlineProperty = vtkProperty::New();
  this->SelectedOutlineProperty->SetAmbient(1.0);
  this->SelectedOutlineProperty->SetAmbientColor(0.0,1.0,0.0);

  // Edge property
  this->EdgesProperty = vtkProperty::New();
  this->EdgesProperty->SetAmbient(1.0);
  this->EdgesProperty->SetAmbientColor(1.0,1.0,1.0);
}

//----------------------------------------------------------------------------
void vtkImplicitPlaneRepresentation::PlaceWidget(double bds[6])
{
  int i;
  double bounds[6], origin[3];

  this->AdjustBounds(bds, bounds, origin);

  // Set up the bounding box
  this->Box->SetOrigin(bounds[0],bounds[2],bounds[4]);
  this->Box->SetSpacing((bounds[1]-bounds[0]),(bounds[3]-bounds[2]),
                        (bounds[5]-bounds[4]));
  this->Outline->Update();

  this->LineSource->SetPoint1(this->Plane->GetOrigin());
  if ( this->NormalToYAxis )
    {
    this->Plane->SetNormal(0,1,0);
    this->LineSource->SetPoint2(0,1,0);
    }
  else if ( this->NormalToZAxis )
    {
    this->Plane->SetNormal(0,0,1);
    this->LineSource->SetPoint2(0,0,1);
    }
  else //default or x-normal
    {
    this->Plane->SetNormal(1,0,0);
    this->LineSource->SetPoint2(1,0,0);
    }

  for (i=0; i<6; i++)
    {
    this->InitialBounds[i] = bounds[i];
    }

  this->InitialLength = sqrt((bounds[1]-bounds[0])*(bounds[1]-bounds[0]) +
                             (bounds[3]-bounds[2])*(bounds[3]-bounds[2]) +
                             (bounds[5]-bounds[4])*(bounds[5]-bounds[4]));

  this->BuildRepresentation();

  this->SizeHandles();
}

//----------------------------------------------------------------------------
// Description:
// Set the origin of the plane.
void vtkImplicitPlaneRepresentation::SetOrigin(double x, double y, double z) 
{
  double origin[3];
  origin[0] = x;
  origin[1] = y;
  origin[2] = z;
  this->SetOrigin(origin);
}

//----------------------------------------------------------------------------
// Description:
// Set the origin of the plane.
void vtkImplicitPlaneRepresentation::SetOrigin(double x[3]) 
{
  double *bounds = this->Outline->GetOutput()->GetBounds();
  for (int i=0; i<3; i++)
    {
    if ( x[i] < bounds[2*i] )
      {
      x[i] = bounds[2*i];
      }
    else if ( x[i] > bounds[2*i+1] )
      {
      x[i] = bounds[2*i+1];
      }
    }
  this->Plane->SetOrigin(x);
  this->BuildRepresentation();
}

//----------------------------------------------------------------------------
// Description:
// Get the origin of the plane.
double* vtkImplicitPlaneRepresentation::GetOrigin() 
{
  return this->Plane->GetOrigin();
}

//----------------------------------------------------------------------------
void vtkImplicitPlaneRepresentation::GetOrigin(double xyz[3]) 
{
  this->Plane->GetOrigin(xyz);
}

//----------------------------------------------------------------------------
// Description:
// Set the normal to the plane.
void vtkImplicitPlaneRepresentation::SetNormal(double x, double y, double z) 
{
  double n[3];
  n[0] = x;
  n[1] = y;
  n[2] = z;
  vtkMath::Normalize(n);
  this->Plane->SetNormal(n);
  this->BuildRepresentation();
}

//----------------------------------------------------------------------------
// Description:
// Set the normal to the plane.
void vtkImplicitPlaneRepresentation::SetNormal(double n[3]) 
{
  this->SetNormal(n[0], n[1], n[2]);
}

//----------------------------------------------------------------------------
// Description:
// Get the normal to the plane.
double* vtkImplicitPlaneRepresentation::GetNormal() 
{
  return this->Plane->GetNormal();
}

//----------------------------------------------------------------------------
void vtkImplicitPlaneRepresentation::GetNormal(double xyz[3]) 
{
  this->Plane->GetNormal(xyz);
}

//----------------------------------------------------------------------------
void vtkImplicitPlaneRepresentation::SetDrawPlane(int drawPlane)
{
  if ( drawPlane == this->DrawPlane )
    {
    return;
    }

  this->Modified();
  this->DrawPlane = drawPlane;
  this->BuildRepresentation();
}

//----------------------------------------------------------------------------
void vtkImplicitPlaneRepresentation::SetNormalToXAxis (int var)
{
  if (this->NormalToXAxis != var)
    {
    this->NormalToXAxis = var;
    this->Modified();
    }
  if (var)
    {
    this->NormalToYAxisOff();
    this->NormalToZAxisOff();
    }
}

//----------------------------------------------------------------------------
void vtkImplicitPlaneRepresentation::SetNormalToYAxis (int var)
{
  if (this->NormalToYAxis != var)
    {
    this->NormalToYAxis = var;
    this->Modified();
    }
  if (var)
    {
    this->NormalToXAxisOff();
    this->NormalToZAxisOff();
    }
}

//----------------------------------------------------------------------------
void vtkImplicitPlaneRepresentation::SetNormalToZAxis (int var)
{
  if (this->NormalToZAxis != var)
    {
    this->NormalToZAxis = var;
    this->Modified();
    }
  if (var)
    {
    this->NormalToXAxisOff();
    this->NormalToYAxisOff();
    }
}

//----------------------------------------------------------------------------
void vtkImplicitPlaneRepresentation::GetPolyData(vtkPolyData *pd)
{ 
  pd->ShallowCopy(this->Cutter->GetOutput()); 
}

//----------------------------------------------------------------------------
vtkPolyDataAlgorithm *vtkImplicitPlaneRepresentation::GetPolyDataAlgorithm()
{
  return this->Cutter;
}

//----------------------------------------------------------------------------
void vtkImplicitPlaneRepresentation::GetPlane(vtkPlane *plane)
{
  if ( plane == NULL )
    {
    return;
    }
  
  plane->SetNormal(this->Plane->GetNormal());
  plane->SetOrigin(this->Plane->GetOrigin());
}

//----------------------------------------------------------------------------
void vtkImplicitPlaneRepresentation::UpdatePlacement()
{
  this->Outline->Update();
  this->Cutter->Update();
  this->Edges->Update();
  this->BuildRepresentation();
}

//----------------------------------------------------------------------------
void vtkImplicitPlaneRepresentation::BuildRepresentation()
{
  if ( ! this->Renderer )
    {
    return;
    }

  double *origin = this->Plane->GetOrigin();
  double *normal = this->Plane->GetNormal();
  double p2[3];
  if( !this->OutsideBounds )
    {
    double *bounds = this->InitialBounds;
    for (int i=0; i<3; i++)
      {
      if ( origin[i] < bounds[2*i] )
        {
        origin[i] = bounds[2*i];
        }
      else if ( origin[i] > bounds[2*i+1] )
        {
        origin[i] = bounds[2*i+1];
        }
      }
    }

  // Setup the plane normal
  double d = this->Outline->GetOutput()->GetLength();

  p2[0] = origin[0] + 0.30 * d * normal[0];
  p2[1] = origin[1] + 0.30 * d * normal[1];
  p2[2] = origin[2] + 0.30 * d * normal[2];

  this->LineSource->SetPoint1(origin);
  this->LineSource->SetPoint2(p2);
  this->ConeSource->SetCenter(p2);
  this->ConeSource->SetDirection(normal);

  p2[0] = origin[0] - 0.30 * d * normal[0];
  p2[1] = origin[1] - 0.30 * d * normal[1];
  p2[2] = origin[2] - 0.30 * d * normal[2];

  this->LineSource2->SetPoint1(origin[0],origin[1],origin[2]);
  this->LineSource2->SetPoint2(p2);
  this->ConeSource2->SetCenter(p2);
  this->ConeSource2->SetDirection(normal[0],normal[1],normal[2]);

  // Set up the position handle
  this->Sphere->SetCenter(origin[0],origin[1],origin[2]);

  // Control the look of the edges
  if ( this->Tubing )
    {
    this->EdgesMapper->SetInput(this->EdgesTuber->GetOutput());
    }
  else 
    {
    this->EdgesMapper->SetInput(this->Edges->GetOutput());
    }
}

//----------------------------------------------------------------------------
void vtkImplicitPlaneRepresentation::SizeHandles()
{
  double radius = this->vtkWidgetRepresentation::SizeHandles(1.35);

  this->ConeSource->SetHeight(2.0*radius);
  this->ConeSource->SetRadius(radius);
  this->ConeSource2->SetHeight(2.0*radius);
  this->ConeSource2->SetRadius(radius);
  
  this->Sphere->SetRadius(radius);

  this->EdgesTuber->SetRadius(0.25*radius);
}
