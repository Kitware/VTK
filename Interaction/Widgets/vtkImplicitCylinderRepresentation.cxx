/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImplicitCylinderRepresentation.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImplicitCylinderRepresentation.h"

#include "vtkActor.h"
#include "vtkAssemblyNode.h"
#include "vtkAssemblyPath.h"
#include "vtkCallbackCommand.h"
#include "vtkCamera.h"
#include "vtkCellPicker.h"
#include "vtkConeSource.h"
#include "vtkFeatureEdges.h"
#include "vtkImageData.h"
#include "vtkLineSource.h"
#include "vtkLookupTable.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkOutlineFilter.h"
#include "vtkPickingManager.h"
#include "vtkCylinder.h"
#include "vtkPlane.h"
#include "vtkPolyData.h"
#include "vtkPoints.h"
#include "vtkCellArray.h"
#include "vtkDoubleArray.h"
#include "vtkPointData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSmartPointer.h"
#include "vtkSphereSource.h"
#include "vtkTransform.h"
#include "vtkTubeFilter.h"
#include "vtkInteractorObserver.h"
#include "vtkBox.h"
#include "vtkCommand.h"
#include "vtkWindow.h"

#include <algorithm>
#include <cfloat> //for FLT_EPSILON

vtkStandardNewMacro(vtkImplicitCylinderRepresentation);

//----------------------------------------------------------------------------
vtkImplicitCylinderRepresentation::vtkImplicitCylinderRepresentation()
{
  this->AlongXAxis = 0;
  this->AlongYAxis = 0;
  this->AlongZAxis = 0;

  // Handle size is in pixels for this widget
  this->HandleSize = 5.0;

  // Pushing operation
  this->BumpDistance = 0.01;

  // Build the representation of the widget
  //
  this->Cylinder = vtkCylinder::New();
  this->Cylinder->SetAxis(0,0,1);
  this->Cylinder->SetCenter(0,0,0);
  this->Cylinder->SetRadius(0.5);

  this->MinRadius = 0.01;
  this->MaxRadius = 1.00;

  this->Resolution = 128;

  this->Box = vtkImageData::New();
  this->Box->SetDimensions(2,2,2);
  this->Outline = vtkOutlineFilter::New();
  this->Outline->SetInputData(this->Box);
  this->OutlineMapper = vtkPolyDataMapper::New();
  this->OutlineMapper->SetInputConnection(
    this->Outline->GetOutputPort());
  this->OutlineActor = vtkActor::New();
  this->OutlineActor->SetMapper(this->OutlineMapper);
  this->OutlineTranslation = 1;
  this->ScaleEnabled = 1;
  this->OutsideBounds = 1;
  this->ConstrainToWidgetBounds = 1;

  this->Cyl = vtkPolyData::New();
  vtkPoints *pts = vtkPoints::New();
  pts->SetDataTypeToDouble();
  this->Cyl->SetPoints(pts);
  pts->Delete();
  vtkCellArray *polys = vtkCellArray::New();
  this->Cyl->SetPolys(polys);
  polys->Delete();
  vtkDoubleArray *normals = vtkDoubleArray::New();
  normals->SetNumberOfComponents(3);
  this->Cyl->GetPointData()->SetNormals(normals);
  normals->Delete();
  this->CylMapper = vtkPolyDataMapper::New();
  this->CylMapper->SetInputData(this->Cyl);
  this->CylActor = vtkActor::New();
  this->CylActor->SetMapper(this->CylMapper);
  this->DrawCylinder = 1;

  this->Edges = vtkFeatureEdges::New();
  this->Edges->SetInputData(this->Cyl);
  this->EdgesTuber = vtkTubeFilter::New();
  this->EdgesTuber->SetInputConnection(
    this->Edges->GetOutputPort());
  this->EdgesTuber->SetNumberOfSides(12);
  this->EdgesMapper = vtkPolyDataMapper::New();
  this->EdgesMapper->SetInputConnection(
    this->EdgesTuber->GetOutputPort());
  this->EdgesActor = vtkActor::New();
  this->EdgesActor->SetMapper(this->EdgesMapper);
  this->Tubing = 1; //control whether tubing is on

  // Create the + cylinder axis
  this->LineSource = vtkLineSource::New();
  this->LineSource->SetResolution(1);
  this->LineMapper = vtkPolyDataMapper::New();
  this->LineMapper->SetInputConnection(
    this->LineSource->GetOutputPort());
  this->LineActor = vtkActor::New();
  this->LineActor->SetMapper(this->LineMapper);

  this->ConeSource = vtkConeSource::New();
  this->ConeSource->SetResolution(12);
  this->ConeSource->SetAngle(25.0);
  this->ConeMapper = vtkPolyDataMapper::New();
  this->ConeMapper->SetInputConnection(
    this->ConeSource->GetOutputPort());
  this->ConeActor = vtkActor::New();
  this->ConeActor->SetMapper(this->ConeMapper);

  // Create the - cylinder axis
  this->LineSource2 = vtkLineSource::New();
  this->LineSource2->SetResolution(1);
  this->LineMapper2 = vtkPolyDataMapper::New();
  this->LineMapper2->SetInputConnection(
    this->LineSource2->GetOutputPort());
  this->LineActor2 = vtkActor::New();
  this->LineActor2->SetMapper(this->LineMapper2);

  this->ConeSource2 = vtkConeSource::New();
  this->ConeSource2->SetResolution(12);
  this->ConeSource2->SetAngle(25.0);
  this->ConeMapper2 = vtkPolyDataMapper::New();
  this->ConeMapper2->SetInputConnection(
    this->ConeSource2->GetOutputPort());
  this->ConeActor2 = vtkActor::New();
  this->ConeActor2->SetMapper(this->ConeMapper2);

  // Create the center handle
  this->Sphere = vtkSphereSource::New();
  this->Sphere->SetThetaResolution(16);
  this->Sphere->SetPhiResolution(8);
  this->SphereMapper = vtkPolyDataMapper::New();
  this->SphereMapper->SetInputConnection(
    this->Sphere->GetOutputPort());
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
  this->Picker->AddPickList(this->LineActor);
  this->Picker->AddPickList(this->ConeActor);
  this->Picker->AddPickList(this->LineActor2);
  this->Picker->AddPickList(this->ConeActor2);
  this->Picker->AddPickList(this->SphereActor);
  this->Picker->AddPickList(this->OutlineActor);
  this->Picker->PickFromListOn();

  this->CylPicker = vtkCellPicker::New();
  this->CylPicker->SetTolerance(0.005);
  this->CylPicker->AddPickList(this->CylActor);
  this->CylPicker->AddPickList(this->EdgesActor);
  this->CylPicker->PickFromListOn();

  // Set up the initial properties
  this->CreateDefaultProperties();

  // Pass the initial properties to the actors.
  this->LineActor->SetProperty(this->AxisProperty);
  this->ConeActor->SetProperty(this->AxisProperty);
  this->LineActor2->SetProperty(this->AxisProperty);
  this->ConeActor2->SetProperty(this->AxisProperty);
  this->SphereActor->SetProperty(this->AxisProperty);
  this->CylActor->SetProperty(this->CylinderProperty);
  this->OutlineActor->SetProperty(this->OutlineProperty);

  // The bounding box
  this->BoundingBox = vtkBox::New();

  this->RepresentationState = vtkImplicitCylinderRepresentation::Outside;
}

//----------------------------------------------------------------------------
vtkImplicitCylinderRepresentation::~vtkImplicitCylinderRepresentation()
{
  this->Cylinder->Delete();
  this->Box->Delete();
  this->Outline->Delete();
  this->OutlineMapper->Delete();
  this->OutlineActor->Delete();

  this->Cyl->Delete();
  this->CylMapper->Delete();
  this->CylActor->Delete();

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
  this->CylPicker->Delete();

  this->AxisProperty->Delete();
  this->SelectedAxisProperty->Delete();
  this->CylinderProperty->Delete();
  this->SelectedCylinderProperty->Delete();
  this->OutlineProperty->Delete();
  this->SelectedOutlineProperty->Delete();
  this->EdgesProperty->Delete();
  this->BoundingBox->Delete();
}

//----------------------------------------------------------------------------
int vtkImplicitCylinderRepresentation::ComputeInteractionState(int X, int Y,
                                                            int vtkNotUsed(modify))
{
  // See if anything has been selected
  vtkAssemblyPath* path = this->GetAssemblyPath(X, Y, 0., this->Picker);

  // The second picker may need to be called. This is done because the cylinder
  // wraps around things that can be picked; thus the cylinder is the selection
  // of last resort.
  if ( path == NULL )
  {
    this->CylPicker->Pick(X, Y, 0., this->Renderer);
    path = this->CylPicker->GetPath();
  }

  if ( path == NULL ) // Nothing picked
  {
    this->SetRepresentationState(vtkImplicitCylinderRepresentation::Outside);
    this->InteractionState = vtkImplicitCylinderRepresentation::Outside;
    return this->InteractionState;
  }

  // Something picked, continue
  this->ValidPick = 1;

  // Depending on the interaction state (set by the widget) we modify
  // this state based on what is picked.
  if ( this->InteractionState == vtkImplicitCylinderRepresentation::Moving )
  {
    vtkProp *prop = path->GetFirstNode()->GetViewProp();
    if ( prop == this->ConeActor || prop == this->LineActor ||
         prop == this->ConeActor2 || prop == this->LineActor2 )
    {
      this->InteractionState = vtkImplicitCylinderRepresentation::RotatingAxis;
      this->SetRepresentationState(vtkImplicitCylinderRepresentation::RotatingAxis);
    }
    else if ( prop == this->CylActor || prop == EdgesActor )
    {
      this->InteractionState = vtkImplicitCylinderRepresentation::AdjustingRadius;
      this->SetRepresentationState(vtkImplicitCylinderRepresentation::AdjustingRadius);
    }
    else if ( prop == this->SphereActor )
    {
      this->InteractionState = vtkImplicitCylinderRepresentation::MovingCenter;
      this->SetRepresentationState(vtkImplicitCylinderRepresentation::MovingCenter);
    }
    else
    {
      if ( this->OutlineTranslation )
      {
        this->InteractionState = vtkImplicitCylinderRepresentation::MovingOutline;
        this->SetRepresentationState(vtkImplicitCylinderRepresentation::MovingOutline);
      }
      else
      {
        this->InteractionState = vtkImplicitCylinderRepresentation::Outside;
        this->SetRepresentationState(vtkImplicitCylinderRepresentation::Outside);
      }
    }
  }

  // We may add a condition to allow the camera to work IO scaling
  else if ( this->InteractionState != vtkImplicitCylinderRepresentation::Scaling )
  {
    this->InteractionState = vtkImplicitCylinderRepresentation::Outside;
  }

  return this->InteractionState;
}

//----------------------------------------------------------------------------
void vtkImplicitCylinderRepresentation::SetRepresentationState(int state)
{
  if (this->RepresentationState == state)
  {
    return;
  }

  // Clamp the state
  state = (state < vtkImplicitCylinderRepresentation::Outside ?
           vtkImplicitCylinderRepresentation::Outside :
           (state > vtkImplicitCylinderRepresentation::Scaling ?
            vtkImplicitCylinderRepresentation::Scaling : state));

  this->RepresentationState = state;
  this->Modified();

  if ( state == vtkImplicitCylinderRepresentation::RotatingAxis )
  {
    this->HighlightNormal(1);
    this->HighlightCylinder(1);
  }
  else if ( state == vtkImplicitCylinderRepresentation::AdjustingRadius )
  {
    this->HighlightCylinder(1);
  }
  else if ( state == vtkImplicitCylinderRepresentation::MovingCenter )
  {
    this->HighlightNormal(1);
  }
  else if ( state == vtkImplicitCylinderRepresentation::MovingOutline )
  {
    this->HighlightOutline(1);
  }
  else if ( state == vtkImplicitCylinderRepresentation::Scaling &&
            this->ScaleEnabled )
  {
    this->HighlightNormal(1);
    this->HighlightCylinder(1);
    this->HighlightOutline(1);
  }
  else if ( state == vtkImplicitCylinderRepresentation::TranslatingCenter )
  {
    this->HighlightNormal(1);
  }
  else
  {
    this->HighlightNormal(0);
    this->HighlightCylinder(0);
    this->HighlightOutline(0);
  }
}

//----------------------------------------------------------------------------
void vtkImplicitCylinderRepresentation::StartWidgetInteraction(double e[2])
{
  this->StartEventPosition[0] = e[0];
  this->StartEventPosition[1] = e[1];
  this->StartEventPosition[2] = 0.0;

  this->LastEventPosition[0] = e[0];
  this->LastEventPosition[1] = e[1];
  this->LastEventPosition[2] = 0.0;
}

//----------------------------------------------------------------------------
void vtkImplicitCylinderRepresentation::WidgetInteraction(double e[2])
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
  double pos[3];
  this->Picker->GetPickPosition(pos);
  vtkInteractorObserver::ComputeWorldToDisplay(this->Renderer, pos[0], pos[1], pos[2],
                                               focalPoint);
  z = focalPoint[2];
  vtkInteractorObserver::ComputeDisplayToWorld(this->Renderer,this->LastEventPosition[0],
                                               this->LastEventPosition[1], z, prevPickPoint);
  vtkInteractorObserver::ComputeDisplayToWorld(this->Renderer, e[0], e[1], z, pickPoint);

  // Process the motion
  if ( this->InteractionState == vtkImplicitCylinderRepresentation::MovingOutline )
  {
    this->TranslateOutline(prevPickPoint, pickPoint);
  }
  else if ( this->InteractionState == vtkImplicitCylinderRepresentation::MovingCenter )
  {
    this->TranslateCenter(prevPickPoint, pickPoint);
  }
  else if ( this->InteractionState == vtkImplicitCylinderRepresentation::TranslatingCenter )
  {
    this->TranslateCenterOnAxis(prevPickPoint, pickPoint);
  }
  else if ( this->InteractionState == vtkImplicitCylinderRepresentation::AdjustingRadius )
  {
    this->AdjustRadius(e[0], e[1], prevPickPoint, pickPoint);
  }
  else if ( this->InteractionState == vtkImplicitCylinderRepresentation::Scaling &&
    this->ScaleEnabled )
  {
    this->Scale(prevPickPoint, pickPoint, e[0], e[1]);
  }
  else if ( this->InteractionState == vtkImplicitCylinderRepresentation::RotatingAxis )
  {
    camera->GetViewPlaneNormal(vpn);
    this->Rotate(e[0], e[1], prevPickPoint, pickPoint, vpn);
  }

  this->LastEventPosition[0] = e[0];
  this->LastEventPosition[1] = e[1];
  this->LastEventPosition[2] = 0.0;
}

//----------------------------------------------------------------------------
void vtkImplicitCylinderRepresentation::EndWidgetInteraction(double vtkNotUsed(e)[2])
{
  this->SetRepresentationState(vtkImplicitCylinderRepresentation::Outside);
}

//----------------------------------------------------------------------
double *vtkImplicitCylinderRepresentation::GetBounds()
{
  this->BuildRepresentation();
  this->BoundingBox->SetBounds(this->OutlineActor->GetBounds());
  this->BoundingBox->AddBounds(this->CylActor->GetBounds());
  this->BoundingBox->AddBounds(this->EdgesActor->GetBounds());
  this->BoundingBox->AddBounds(this->ConeActor->GetBounds());
  this->BoundingBox->AddBounds(this->LineActor->GetBounds());
  this->BoundingBox->AddBounds(this->ConeActor2->GetBounds());
  this->BoundingBox->AddBounds(this->LineActor2->GetBounds());
  this->BoundingBox->AddBounds(this->SphereActor->GetBounds());

  return this->BoundingBox->GetBounds();
}

//----------------------------------------------------------------------------
void vtkImplicitCylinderRepresentation::GetActors(vtkPropCollection *pc)
{
  this->OutlineActor->GetActors(pc);
  this->CylActor->GetActors(pc);
  this->EdgesActor->GetActors(pc);
  this->ConeActor->GetActors(pc);
  this->LineActor->GetActors(pc);
  this->ConeActor2->GetActors(pc);
  this->LineActor2->GetActors(pc);
  this->SphereActor->GetActors(pc);
}

//----------------------------------------------------------------------------
void vtkImplicitCylinderRepresentation::ReleaseGraphicsResources(vtkWindow *w)
{
  this->OutlineActor->ReleaseGraphicsResources(w);
  this->CylActor->ReleaseGraphicsResources(w);
  this->EdgesActor->ReleaseGraphicsResources(w);
  this->ConeActor->ReleaseGraphicsResources(w);
  this->LineActor->ReleaseGraphicsResources(w);
  this->ConeActor2->ReleaseGraphicsResources(w);
  this->LineActor2->ReleaseGraphicsResources(w);
  this->SphereActor->ReleaseGraphicsResources(w);
}

//----------------------------------------------------------------------------
int vtkImplicitCylinderRepresentation::RenderOpaqueGeometry(vtkViewport *v)
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

  if ( this->DrawCylinder )
  {
    count += this->CylActor->RenderOpaqueGeometry(v);
  }

  return count;
}

//-----------------------------------------------------------------------------
int vtkImplicitCylinderRepresentation::RenderTranslucentPolygonalGeometry(
  vtkViewport *v)
{
  int count=0;
  this->BuildRepresentation();
  count += this->OutlineActor->RenderTranslucentPolygonalGeometry(v);
  count += this->EdgesActor->RenderTranslucentPolygonalGeometry(v);
  count += this->ConeActor->RenderTranslucentPolygonalGeometry(v);
  count += this->LineActor->RenderTranslucentPolygonalGeometry(v);
  count += this->ConeActor2->RenderTranslucentPolygonalGeometry(v);
  count += this->LineActor2->RenderTranslucentPolygonalGeometry(v);
  count += this->SphereActor->RenderTranslucentPolygonalGeometry(v);

  if ( this->DrawCylinder )
  {
    count += this->CylActor->RenderTranslucentPolygonalGeometry(v);
  }

  return count;
}

//-----------------------------------------------------------------------------
int vtkImplicitCylinderRepresentation::HasTranslucentPolygonalGeometry()
{
  int result=0;
  result |= this->OutlineActor->HasTranslucentPolygonalGeometry();
  result |= this->EdgesActor->HasTranslucentPolygonalGeometry();
  result |= this->ConeActor->HasTranslucentPolygonalGeometry();
  result |= this->LineActor->HasTranslucentPolygonalGeometry();
  result |= this->ConeActor2->HasTranslucentPolygonalGeometry();
  result |= this->LineActor2->HasTranslucentPolygonalGeometry();
  result |= this->SphereActor->HasTranslucentPolygonalGeometry();

  if ( this->DrawCylinder )
  {
    result |= this->CylActor->HasTranslucentPolygonalGeometry();
  }

  return result;
}

//----------------------------------------------------------------------------
void vtkImplicitCylinderRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Min Radius: " << this->MinRadius << "\n";
  os << indent << "Max Radius: " << this->MaxRadius << "\n";

  os << indent << "Resolution: " << this->Resolution << "\n";

  if ( this->AxisProperty )
  {
    os << indent << "Axis Property: " << this->AxisProperty << "\n";
  }
  else
  {
    os << indent << "Axis Property: (none)\n";
  }
  if ( this->SelectedAxisProperty )
  {
    os << indent << "Selected Axis Property: "
       << this->SelectedAxisProperty << "\n";
  }
  else
  {
    os << indent << "Selected Axis Property: (none)\n";
  }

  if ( this->CylinderProperty )
  {
    os << indent << "Cylinder Property: " << this->CylinderProperty << "\n";
  }
  else
  {
    os << indent << "Cylinder Property: (none)\n";
  }
  if ( this->SelectedCylinderProperty )
  {
    os << indent << "Selected Cylinder Property: "
       << this->SelectedCylinderProperty << "\n";
  }
  else
  {
    os << indent << "Selected Cylinder Property: (none)\n";
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

  os << indent << "Along X Axis: "
     << (this->AlongXAxis ? "On" : "Off") << "\n";
  os << indent << "Along Y Axis: "
     << (this->AlongYAxis ? "On" : "Off") << "\n";
  os << indent << "ALong Z Axis: "
     << (this->AlongZAxis ? "On" : "Off") << "\n";

  os << indent << "Widget Bounds: " << this->WidgetBounds[0] << ", "
                                    << this->WidgetBounds[1] << ", "
                                    << this->WidgetBounds[2] << ", "
                                    << this->WidgetBounds[3] << ", "
                                    << this->WidgetBounds[4] << ", "
                                    << this->WidgetBounds[5] << "\n";

  os << indent << "Tubing: " << (this->Tubing ? "On" : "Off") << "\n";
  os << indent << "Outline Translation: "
     << (this->OutlineTranslation ? "On" : "Off") << "\n";
  os << indent << "Outside Bounds: "
     << (this->OutsideBounds ? "On" : "Off") << "\n";
  os << indent << "Constrain to Widget Bounds: "
     << (this->ConstrainToWidgetBounds ? "On" : "Off") << "\n";
  os << indent << "Scale Enabled: "
     << (this->ScaleEnabled ? "On" : "Off") << "\n";
  os << indent << "Draw Cylinder: " << (this->DrawCylinder ? "On" : "Off") << "\n";
  os << indent << "Bump Distance: " << this->BumpDistance << "\n";

  os << indent << "Representation State: ";
  switch ( this->RepresentationState )
  {
    case Outside:
      os << "Outside\n";
      break;
    case Moving:
      os << "Moving\n";
      break;
    case MovingOutline:
      os << "MovingOutline\n";
      break;
    case MovingCenter:
      os << "MovingCenter\n";
      break;
    case RotatingAxis:
      os << "RotatingAxis\n";
      break;
    case AdjustingRadius:
      os << "AdjustingRadius\n";
      break;
    case Scaling:
      os << "Scaling\n";
      break;
    case TranslatingCenter:
      os << "TranslatingCenter\n";
      break;
  }

  // this->InteractionState is printed in superclass
  // this is commented to avoid PrintSelf errors
}


//----------------------------------------------------------------------------
void vtkImplicitCylinderRepresentation::HighlightNormal(int highlight)
{
  if ( highlight )
  {
    this->LineActor->SetProperty(this->SelectedAxisProperty);
    this->ConeActor->SetProperty(this->SelectedAxisProperty);
    this->LineActor2->SetProperty(this->SelectedAxisProperty);
    this->ConeActor2->SetProperty(this->SelectedAxisProperty);
    this->SphereActor->SetProperty(this->SelectedAxisProperty);
  }
  else
  {
    this->LineActor->SetProperty(this->AxisProperty);
    this->ConeActor->SetProperty(this->AxisProperty);
    this->LineActor2->SetProperty(this->AxisProperty);
    this->ConeActor2->SetProperty(this->AxisProperty);
    this->SphereActor->SetProperty(this->AxisProperty);
  }
}


//----------------------------------------------------------------------------
void vtkImplicitCylinderRepresentation::HighlightCylinder(int highlight)
{
  if ( highlight )
  {
    this->CylActor->SetProperty(this->SelectedCylinderProperty);
  }
  else
  {
    this->CylActor->SetProperty(this->CylinderProperty);
  }
}


//----------------------------------------------------------------------------
void vtkImplicitCylinderRepresentation::HighlightOutline(int highlight)
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
void vtkImplicitCylinderRepresentation::Rotate(double X, double Y,
                                               double *p1, double *p2, double *vpn)
{
  double v[3]; //vector of motion
  double axis[3]; //axis of rotation
  double theta; //rotation angle

  // mouse motion vector in world space
  v[0] = p2[0] - p1[0];
  v[1] = p2[1] - p1[1];
  v[2] = p2[2] - p1[2];

  double *center = this->Cylinder->GetCenter();
  double *cylAxis = this->Cylinder->GetAxis();

  // Create axis of rotation and angle of rotation
  vtkMath::Cross(vpn,v,axis);
  if ( vtkMath::Normalize(axis) == 0.0 )
  {
    return;
  }
  int *size = this->Renderer->GetSize();
  double l2 = (X-this->LastEventPosition[0])*(X-this->LastEventPosition[0]) +
    (Y-this->LastEventPosition[1])*(Y-this->LastEventPosition[1]);
  theta = 360.0 * sqrt(l2/(size[0]*size[0]+size[1]*size[1]));

  // Manipulate the transform to reflect the rotation
  this->Transform->Identity();
  this->Transform->Translate(center[0],center[1],center[2]);
  this->Transform->RotateWXYZ(theta,axis);
  this->Transform->Translate(-center[0],-center[1],-center[2]);

  //Set the new normal
  double aNew[3];
  this->Transform->TransformNormal(cylAxis,aNew);
  this->SetAxis(aNew);
}

//----------------------------------------------------------------------------
// Loop through all points and translate them
void vtkImplicitCylinderRepresentation::TranslateOutline(double *p1, double *p2)
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
  this->Box->GetBounds(this->WidgetBounds);

  //Translate the cylinder
  origin = this->Cylinder->GetCenter();
  oNew[0] = origin[0] + v[0];
  oNew[1] = origin[1] + v[1];
  oNew[2] = origin[2] + v[2];
  this->Cylinder->SetCenter(oNew);

  this->BuildRepresentation();
}

//----------------------------------------------------------------------------
// Loop through all points and translate them
void vtkImplicitCylinderRepresentation::TranslateCenter(double *p1, double *p2)
{
  //Get the motion vector
  double v[3];
  v[0] = p2[0] - p1[0];
  v[1] = p2[1] - p1[1];
  v[2] = p2[2] - p1[2];

  //Add to the current point, project back down onto plane
  double *c = this->Cylinder->GetCenter();
  double *a = this->Cylinder->GetAxis();
  double newCenter[3];

  newCenter[0] = c[0] + v[0];
  newCenter[1] = c[1] + v[1];
  newCenter[2] = c[2] + v[2];

  vtkPlane::ProjectPoint(newCenter,c,a,newCenter);
  this->SetCenter(newCenter[0],newCenter[1],newCenter[2]);
  this->BuildRepresentation();
}

//----------------------------------------------------------------------------
// Translate the center on the axis
void vtkImplicitCylinderRepresentation::TranslateCenterOnAxis(double *p1, double *p2)
{
  // Get the motion vector
  double v[3];
  v[0] = p2[0] - p1[0];
  v[1] = p2[1] - p1[1];
  v[2] = p2[2] - p1[2];

  // Add to the current point, project back down onto plane
  double *c = this->Cylinder->GetCenter();
  double *a = this->Cylinder->GetAxis();
  double newCenter[3];

  newCenter[0] = c[0] + v[0];
  newCenter[1] = c[1] + v[1];
  newCenter[2] = c[2] + v[2];

  // Normalize the axis vector
  const double imag = 1. /
    std::max(1.0e-100, sqrt(a[0] * a[0] + a[1] * a[1] + a[2] * a[2]));
  double an[3];
  an[0] = a[0] * imag;
  an[1] = a[1] * imag;
  an[2] = a[2] * imag;

  // Project the point on the axis vector
  double u[3];
  u[0] = newCenter[0] - c[0];
  u[1] = newCenter[1] - c[1];
  u[2] = newCenter[2] - c[2];
  double dot = an[0] * u[0] + an[1] * u[1] + an[2] * u[2];
  newCenter[0] = c[0] + an[0] * dot;
  newCenter[1] = c[1] + an[1] * dot;
  newCenter[2] = c[2] + an[2] * dot;

  this->SetCenter(newCenter[0],newCenter[1],newCenter[2]);
  this->BuildRepresentation();
}

//----------------------------------------------------------------------------
void vtkImplicitCylinderRepresentation::Scale(double *p1, double *p2,
                                              double vtkNotUsed(X), double Y)
{
  //Get the motion vector
  double v[3];
  v[0] = p2[0] - p1[0];
  v[1] = p2[1] - p1[1];
  v[2] = p2[2] - p1[2];

  double *o = this->Cylinder->GetCenter();

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
  this->Box->GetBounds(this->WidgetBounds);

  this->BuildRepresentation();
}

//----------------------------------------------------------------------------
void vtkImplicitCylinderRepresentation::
AdjustRadius(double vtkNotUsed(X), double Y, double *p1, double *p2)
{
  if ( Y == this->LastEventPosition[1] )
  {
    return;
  }

  double dr, radius = this->Cylinder->GetRadius();
  double v[3]; //vector of motion
  v[0] = p2[0] - p1[0];
  v[1] = p2[1] - p1[1];
  v[2] = p2[2] - p1[2];
  double l = sqrt( vtkMath::Dot(v,v) );

  dr = l / this->Outline->GetOutput()->GetLength();
  if ( Y < this->LastEventPosition[1] )
  {
    dr *= -1.0;
  }

  this->SetRadius(radius + dr);
  this->BuildRepresentation();
}

//----------------------------------------------------------------------------
void vtkImplicitCylinderRepresentation::CreateDefaultProperties()
{
  // Cylinder properties
  this->CylinderProperty = vtkProperty::New();
  this->CylinderProperty->SetAmbient(1.0);
  this->CylinderProperty->SetAmbientColor(1.0,1.0,1.0);
  this->CylinderProperty->SetOpacity(0.5);
  this->CylActor->SetProperty(this->CylinderProperty);

  this->SelectedCylinderProperty = vtkProperty::New();
  this->SelectedCylinderProperty->SetAmbient(1.0);
  this->SelectedCylinderProperty->SetAmbientColor(0.0,1.0,0.0);
  this->SelectedCylinderProperty->SetOpacity(0.25);

  // Cylinder axis properties
  this->AxisProperty = vtkProperty::New();
  this->AxisProperty->SetColor(1,1,1);
  this->AxisProperty->SetLineWidth(2);

  this->SelectedAxisProperty = vtkProperty::New();
  this->SelectedAxisProperty->SetColor(1,0,0);
  this->SelectedAxisProperty->SetLineWidth(2);

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
void vtkImplicitCylinderRepresentation::SetEdgeColor(vtkLookupTable* lut)
{
  this->EdgesMapper->SetLookupTable(lut);
}

//----------------------------------------------------------------------------
void vtkImplicitCylinderRepresentation::SetEdgeColor(double r, double g, double b)
{
  vtkSmartPointer<vtkLookupTable> lookupTable =
    vtkSmartPointer<vtkLookupTable>::New();

  lookupTable->SetTableRange(0.0, 1.0);
  lookupTable->SetNumberOfTableValues(1);
  lookupTable->SetTableValue(0, r, g, b);
  lookupTable->Build();

  this->SetEdgeColor(lookupTable);
}

//----------------------------------------------------------------------------
void vtkImplicitCylinderRepresentation::SetEdgeColor(double c[3])
{
  this->SetEdgeColor(c[0], c[1], c[2]);
}

//----------------------------------------------------------------------------
void vtkImplicitCylinderRepresentation::PlaceWidget(double bds[6])
{
  int i;
  double bounds[6], origin[3];

  this->AdjustBounds(bds, bounds, origin);

  // Set up the bounding box
  this->Box->SetOrigin(bounds[0],bounds[2],bounds[4]);
  this->Box->SetSpacing((bounds[1]-bounds[0]),(bounds[3]-bounds[2]),
                        (bounds[5]-bounds[4]));
  this->Outline->Update();

  this->LineSource->SetPoint1(this->Cylinder->GetCenter());
  if ( this->AlongYAxis )
  {
    this->Cylinder->SetAxis(0,1,0);
    this->LineSource->SetPoint2(0,1,0);
  }
  else if ( this->AlongZAxis )
  {
    this->Cylinder->SetAxis(0,0,1);
    this->LineSource->SetPoint2(0,0,1);
  }
  else //default or x-normal
  {
    this->Cylinder->SetAxis(1,0,0);
    this->LineSource->SetPoint2(1,0,0);
  }

  for (i=0; i<6; i++)
  {
    this->InitialBounds[i] = bounds[i];
    this->WidgetBounds[i] = bounds[i];
  }

  this->InitialLength = sqrt((bounds[1]-bounds[0])*(bounds[1]-bounds[0]) +
                             (bounds[3]-bounds[2])*(bounds[3]-bounds[2]) +
                             (bounds[5]-bounds[4])*(bounds[5]-bounds[4]));

  this->ValidPick = 1; // since we have positioned the widget successfully
  this->BuildRepresentation();
}

//----------------------------------------------------------------------------
// Set the center of the cylinder.
void vtkImplicitCylinderRepresentation::SetCenter(double x, double y, double z)
{
  double center[3];
  center[0] = x;
  center[1] = y;
  center[2] = z;
  this->SetCenter(center);
}

//----------------------------------------------------------------------------
// Set the center of the cylinder. Note that the center is clamped slightly inside
// the bounding box or the cylinder tends to disappear as it hits the boundary.
void vtkImplicitCylinderRepresentation::SetCenter(double x[3])
{
  this->Cylinder->SetCenter(x);
  this->BuildRepresentation();
}

//----------------------------------------------------------------------------
// Get the center of the cylinder.
double* vtkImplicitCylinderRepresentation::GetCenter()
{
  return this->Cylinder->GetCenter();
}

//----------------------------------------------------------------------------
void vtkImplicitCylinderRepresentation::GetCenter(double xyz[3])
{
  return this->Cylinder->GetCenter(xyz);
}

//----------------------------------------------------------------------------
// Set the axis of the cylinder.
void vtkImplicitCylinderRepresentation::SetAxis(double x, double y, double z)
{
  double n[3], n2[3];
  n[0] = x;
  n[1] = y;
  n[2] = z;
  vtkMath::Normalize(n);

  this->Cylinder->GetAxis(n2);
  if ( n[0] != n2[0] || n[1] != n2[1] || n[2] != n2[2] )
  {
    this->Cylinder->SetAxis(n);
    this->Modified();
  }
}

//----------------------------------------------------------------------------
// Set the axis the cylinder.
void vtkImplicitCylinderRepresentation::SetAxis(double n[3])
{
  this->SetAxis(n[0], n[1], n[2]);
}

//----------------------------------------------------------------------------
// Get the axis of the cylinder.
double* vtkImplicitCylinderRepresentation::GetAxis()
{
  return this->Cylinder->GetAxis();
}

//----------------------------------------------------------------------------
void vtkImplicitCylinderRepresentation::GetAxis(double xyz[3])
{
  return this->Cylinder->GetAxis(xyz);
}

//----------------------------------------------------------------------------
// Set the radius the cylinder. The radius must be a positive number.
void vtkImplicitCylinderRepresentation::SetRadius(double radius)
{
  if (this->ConstrainToWidgetBounds)
  {
    double minRadius = this->Outline->GetOutput()->GetLength() * this->MinRadius;
    double maxRadius = this->Outline->GetOutput()->GetLength() * this->MaxRadius;

    radius = std::min(maxRadius, std::max(minRadius, radius));
  }
  this->Cylinder->SetRadius(radius);
  this->BuildRepresentation();
}

//----------------------------------------------------------------------------
// Get the radius the cylinder.
double vtkImplicitCylinderRepresentation::GetRadius()
{
  return this->Cylinder->GetRadius();
}

//----------------------------------------------------------------------------
void vtkImplicitCylinderRepresentation::SetDrawCylinder(int drawCyl)
{
  if ( drawCyl == this->DrawCylinder )
  {
    return;
  }

  this->Modified();
  this->DrawCylinder = drawCyl;
  this->BuildRepresentation();
}

//----------------------------------------------------------------------------
void vtkImplicitCylinderRepresentation::SetAlongXAxis (int var)
{
  if (this->AlongXAxis != var)
  {
    this->AlongXAxis = var;
    this->Modified();
  }
  if (var)
  {
    this->AlongYAxisOff();
    this->AlongZAxisOff();
  }
}

//----------------------------------------------------------------------------
void vtkImplicitCylinderRepresentation::SetAlongYAxis (int var)
{
  if (this->AlongYAxis != var)
  {
    this->AlongYAxis = var;
    this->Modified();
  }
  if (var)
  {
    this->AlongXAxisOff();
    this->AlongZAxisOff();
  }
}

//----------------------------------------------------------------------------
void vtkImplicitCylinderRepresentation::SetAlongZAxis (int var)
{
  if (this->AlongZAxis != var)
  {
    this->AlongZAxis = var;
    this->Modified();
  }
  if (var)
  {
    this->AlongXAxisOff();
    this->AlongYAxisOff();
  }
}

//----------------------------------------------------------------------------
void vtkImplicitCylinderRepresentation::GetPolyData(vtkPolyData *pd)
{
  pd->ShallowCopy(this->Cyl);
}

//----------------------------------------------------------------------------
void vtkImplicitCylinderRepresentation::GetCylinder(vtkCylinder *cyl)
{
  if ( cyl == NULL )
  {
    return;
  }

  cyl->SetAxis(this->Cylinder->GetAxis());
  cyl->SetCenter(this->Cylinder->GetCenter());
  cyl->SetRadius(this->Cylinder->GetRadius());
}

//----------------------------------------------------------------------------
void vtkImplicitCylinderRepresentation::UpdatePlacement()
{
  this->BuildRepresentation();
  this->Outline->Update();
  this->Edges->Update();
}

//----------------------------------------------------------------------------
void vtkImplicitCylinderRepresentation::BumpCylinder(int dir, double factor)
{
  // Compute the distance
  double d = this->InitialLength * this->BumpDistance * factor;

  // Push the cylinder
  this->PushCylinder( (dir > 0 ? d : -d) );
}

//----------------------------------------------------------------------------
void vtkImplicitCylinderRepresentation::PushCylinder(double d)
{
  vtkCamera *camera = this->Renderer->GetActiveCamera();
  if ( !camera )
  {
    return;
  }
  double vpn[3], center[3];
  camera->GetViewPlaneNormal(vpn);
  this->Cylinder->GetCenter(center);

  center[0] += d*vpn[0];
  center[1] += d*vpn[1];
  center[2] += d*vpn[2];

  this->Cylinder->SetCenter(center);
  this->BuildRepresentation();
}

//----------------------------------------------------------------------------
void vtkImplicitCylinderRepresentation::BuildRepresentation()
{
  if ( ! this->Renderer )
  {
    return;
  }

  vtkInformation *info = this->GetPropertyKeys();
  this->OutlineActor->SetPropertyKeys(info);
  this->CylActor->SetPropertyKeys(info);
  this->EdgesActor->SetPropertyKeys(info);
  this->ConeActor->SetPropertyKeys(info);
  this->LineActor->SetPropertyKeys(info);
  this->ConeActor2->SetPropertyKeys(info);
  this->LineActor2->SetPropertyKeys(info);
  this->SphereActor->SetPropertyKeys(info);

  if ( this->GetMTime() > this->BuildTime ||
       this->Cylinder->GetMTime() > this->BuildTime )
  {
    double *center = this->Cylinder->GetCenter();
    double *axis = this->Cylinder->GetAxis();


    double bounds[6];
    std::copy(this->WidgetBounds, this->WidgetBounds + 6, bounds);

    double p2[3];
    if ( !this->OutsideBounds )
    {
      // restrict the center inside InitialBounds
      double *ibounds = this->InitialBounds;
      for (int i=0; i<3; i++)
      {
        if ( center[i] < ibounds[2*i] )
        {
          center[i] = ibounds[2*i];
        }
        else if ( center[i] > ibounds[2*i+1] )
        {
          center[i] = ibounds[2*i+1];
        }
      }
    }

    if ( this->ConstrainToWidgetBounds )
    {
      if ( !this->OutsideBounds )
      {
        // center cannot move outside InitialBounds. Therefore, restrict
        // movement of the Box.
        double v[3] = { 0.0, 0.0, 0.0 };
        for (int i = 0; i < 3; ++i)
        {
          if (center[i] <= bounds[2*i])
          {
            v[i] = center[i] - bounds[2*i] - FLT_EPSILON;
          }
          else if (center[i] >= bounds[2*i + 1])
          {
            v[i] = center[i] - bounds[2*i + 1] + FLT_EPSILON;
          }
          bounds[2*i] += v[i];
          bounds[2*i + 1] += v[i];
        }
      }

      // restrict center inside bounds
      for (int i = 0; i < 3; ++i)
      {
        if (center[i] <= bounds[2*i])
        {
          center[i] = bounds[2*i] + FLT_EPSILON;
        }
        if (center[i] >= bounds[2*i + 1])
        {
          center[i] = bounds[2*i + 1] - FLT_EPSILON;
        }
      }
    }
    else // cylinder can move freely, adjust the bounds to change with it
    {
      double offset = this->Cylinder->GetRadius() * 1.2;
      for (int i = 0; i < 3; ++i)
      {
        bounds[2*i] = vtkMath::Min(center[i] - offset, this->WidgetBounds[2*i]);
        bounds[2*i + 1] = vtkMath::Max(center[i] + offset, this->WidgetBounds[2*i + 1]);
      }
    }

    this->Box->SetOrigin(bounds[0],bounds[2],bounds[4]);
    this->Box->SetSpacing((bounds[1]-bounds[0]),(bounds[3]-bounds[2]),
                          (bounds[5]-bounds[4]));
    this->Outline->Update();


    // Setup the cylinder axis
    double d = this->Outline->GetOutput()->GetLength();

    p2[0] = center[0] + 0.30 * d * axis[0];
    p2[1] = center[1] + 0.30 * d * axis[1];
    p2[2] = center[2] + 0.30 * d * axis[2];

    this->LineSource->SetPoint1(center);
    this->LineSource->SetPoint2(p2);
    this->ConeSource->SetCenter(p2);
    this->ConeSource->SetDirection(axis);

    p2[0] = center[0] - 0.30 * d * axis[0];
    p2[1] = center[1] - 0.30 * d * axis[1];
    p2[2] = center[2] - 0.30 * d * axis[2];

    this->LineSource2->SetPoint1(center[0],center[1],center[2]);
    this->LineSource2->SetPoint2(p2);
    this->ConeSource2->SetCenter(p2);
    this->ConeSource2->SetDirection(axis[0],axis[1],axis[2]);

    // Set up the position handle
    this->Sphere->SetCenter(center[0],center[1],center[2]);

    // Control the look of the edges
    if ( this->Tubing )
    {
      this->EdgesMapper->SetInputConnection(
        this->EdgesTuber->GetOutputPort());
    }
    else
    {
      this->EdgesMapper->SetInputConnection(
        this->Edges->GetOutputPort());
    }

    // Construct intersected cylinder
    this->BuildCylinder();

    this->SizeHandles();
    this->BuildTime.Modified();
  }

}

//----------------------------------------------------------------------------
void vtkImplicitCylinderRepresentation::SizeHandles()
{
  double radius =
    this->vtkWidgetRepresentation::SizeHandlesInPixels(1.5,this->Sphere->GetCenter());

  this->ConeSource->SetHeight(2.0*radius);
  this->ConeSource->SetRadius(radius);
  this->ConeSource2->SetHeight(2.0*radius);
  this->ConeSource2->SetRadius(radius);

  this->Sphere->SetRadius(radius);

  this->EdgesTuber->SetRadius(0.25*radius);
}

//----------------------------------------------------------------------
// Create cylinder polydata.  Basically build an oriented cylinder of
// specified resolution.  Trim cylinder facets by performing
// intersection tests. Note that some facets may be outside the
// bounding box, in which cases they are discarded.
void vtkImplicitCylinderRepresentation::BuildCylinder()
{
  // Initialize the polydata
  this->Cyl->Reset();
  vtkPoints *pts = this->Cyl->GetPoints();
  vtkDataArray *normals = this->Cyl->GetPointData()->GetNormals();
  vtkCellArray *polys = this->Cyl->GetPolys();

  // Retrieve relevant parameters
  double *center = this->Cylinder->GetCenter();
  double *axis = this->Cylinder->GetAxis();
  double radius = this->Cylinder->GetRadius();
  int res = this->Resolution;
  double d = this->Outline->GetOutput()->GetLength();

  // We're gonna need a local coordinate system. Find a normal to the
  // cylinder axis. Then use cross product to find a third orthogonal
  // axis.
  int i;
  double n1[3], n2[3];
  for (i=0; i<3; i++)
  {
    // a little trick to find an othogonal normal
    if ( axis[i] != 0.0 )
    {
      n1[(i+2)%3] = 0.0;
      n1[(i+1)%3] = 1.0;
      n1[i] = -axis[(i+1)%3]/axis[i];
      break;
    }
  }
  vtkMath::Normalize(n1);
  vtkMath::Cross(axis,n1,n2);

  // Now create Resolution line segments. Initially the line segments
  // are made a little long to extend outside of the bounding
  // box. Later on we'll trim them to the bounding box.
  pts->SetNumberOfPoints(2*res);
  normals->SetNumberOfTuples(2*res);

  vtkIdType pid;
  double x[3], n[3], theta;
  double v[3]; v[0] = d*axis[0]; v[1] = d*axis[1]; v[2] = d*axis[2];
  for (pid=0; pid < res; ++pid)
  {
    theta = static_cast<double>(pid)/static_cast<double>(res) * 2.0*vtkMath::Pi();
    for (i=0; i<3; ++i)
    {
      n[i] = n1[i]*cos(theta) + n2[i]*sin(theta);
      x[i] = center[i] + radius*n[i] + v[i];
    }
    pts->SetPoint(pid,x);
    normals->SetTuple(pid,n);

    for (i=0; i<3; ++i)
    {
      x[i] = center[i] + radius*n[i] - v[i];
    }
    pts->SetPoint(res+pid,x);
    normals->SetTuple(res+pid,n);
  }

  // Now trim the cylinder against the bounding box. Mark edges that do not
  // intersect the bounding box.
  bool edgeInside[VTK_MAX_CYL_RESOLUTION];
  double x1[3], x2[3], p1[3], p2[3], t1, t2;
  double *bounds = this->Outline->GetOutput()->GetBounds();
  int plane1, plane2;
  for (pid=0; pid < res; ++pid)
  {
    pts->GetPoint(pid,x1);
    pts->GetPoint(pid+res,x2);
    if ( ! vtkBox::IntersectWithLine(bounds,x1,x2,t1,t2,p1,p2,plane1,plane2) )
    {
      edgeInside[pid] = false;
    }
    else
    {
      edgeInside[pid] = true;
      pts->SetPoint(pid,p1);
      pts->SetPoint(pid+res,p2);
    }
  }

  // Create polygons around cylinder. Make sure the edges of the polygon
  // are inside the widget's bounding box.
  vtkIdType ptIds[4];
  for (pid=0; pid < res; ++pid)
  {
    if ( edgeInside[pid] && edgeInside[(pid+1)%res] )
    {
      ptIds[0] = pid;
      ptIds[3] = (pid + 1) % res;
      ptIds[1] = ptIds[0] + res;
      ptIds[2] = ptIds[3] + res;
      polys->InsertNextCell(4,ptIds);
    }
  }
  polys->Modified();
}

//----------------------------------------------------------------------
void vtkImplicitCylinderRepresentation::RegisterPickers()
{
  this->Renderer->GetRenderWindow()->GetInteractor()->
    GetPickingManager()->AddPicker(this->Picker, this);
}
