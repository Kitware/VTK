/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolygonalHandleRepresentation3D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPolygonalHandleRepresentation3D.h"
#include "vtkCursor3D.h"
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"
#include "vtkCellPicker.h"
#include "vtkRenderer.h"
#include "vtkObjectFactory.h"
#include "vtkProperty.h"
#include "vtkAssemblyPath.h"
#include "vtkMath.h"
#include "vtkInteractorObserver.h"
#include "vtkLine.h"
#include "vtkCoordinate.h"
#include "vtkRenderWindow.h"
#include "vtkFocalPlanePointPlacer.h"
#include "vtkCamera.h"
#include "vtkTransformPolyDataFilter.h"
#include "vtkMatrixToLinearTransform.h"
#include "vtkMatrix4x4.h"

vtkCxxRevisionMacro(vtkPolygonalHandleRepresentation3D, "1.5");
vtkStandardNewMacro(vtkPolygonalHandleRepresentation3D);

vtkCxxSetObjectMacro(vtkPolygonalHandleRepresentation3D,Property,vtkProperty);
vtkCxxSetObjectMacro(vtkPolygonalHandleRepresentation3D,SelectedProperty,vtkProperty);

//----------------------------------------------------------------------
vtkPolygonalHandleRepresentation3D::vtkPolygonalHandleRepresentation3D()
{
  this->InteractionState = vtkHandleRepresentation::Outside;
  
  this->HandleTransformFilter = vtkTransformPolyDataFilter::New();
  this->HandleTransform       = vtkMatrixToLinearTransform::New(); 
  this->HandleTransformMatrix = vtkMatrix4x4::New();
  this->HandleTransform->SetInput( this->HandleTransformMatrix );
  this->HandleTransformFilter->SetTransform( this->HandleTransform );

  this->Offset[0] = this->Offset[1] = this->Offset[2] = 0.0;

  this->Mapper = vtkPolyDataMapper::New();
  this->Mapper->SetInput(this->HandleTransformFilter->GetOutput());

  // Set up the initial properties
  this->CreateDefaultProperties();

  this->Actor = vtkActor::New();
  this->Actor->SetMapper(this->Mapper);
  this->Actor->SetProperty(this->Property);

  // Manage the picking stuff
  this->HandlePicker = vtkCellPicker::New();
  this->HandlePicker->PickFromListOn();
  this->HandlePicker->AddPickList(this->Actor);
  this->HandlePicker->SetTolerance(0.01); //need some fluff

  // Override superclass'
  this->PlaceFactor = 1.0;
  this->WaitingForMotion = 0;
  this->ConstraintAxis = -1;

  vtkFocalPlanePointPlacer *pointPlacer = vtkFocalPlanePointPlacer::New();
  this->SetPointPlacer( pointPlacer );
  pointPlacer->Delete();
}

//----------------------------------------------------------------------
vtkPolygonalHandleRepresentation3D::~vtkPolygonalHandleRepresentation3D()
{
  this->HandleTransformFilter->Delete();
  this->HandleTransform->Delete();
  this->HandleTransformMatrix->Delete();
  this->HandlePicker->Delete();
  this->Mapper->Delete();
  this->Actor->Delete();
  this->Property->Delete();
  this->SelectedProperty->Delete();
}

//----------------------------------------------------------------------
void vtkPolygonalHandleRepresentation3D::SetHandle( vtkPolyData * pd )
{
  this->HandleTransformFilter->SetInput( pd );
}

//----------------------------------------------------------------------
vtkPolyData * vtkPolygonalHandleRepresentation3D::GetHandle()
{
  return vtkPolyData::SafeDownCast(this->HandleTransformFilter->GetInput());
}

//-------------------------------------------------------------------------
void vtkPolygonalHandleRepresentation3D::SetWorldPosition(double p[3])
{
  if (!this->Renderer || !this->PointPlacer || 
                          this->PointPlacer->ValidateWorldPosition( p ))
    {
    this->HandleTransformMatrix->SetElement(0, 3, p[0] - this->Offset[0]);
    this->HandleTransformMatrix->SetElement(1, 3, p[1] - this->Offset[1]);
    this->HandleTransformMatrix->SetElement(2, 3, p[2] - this->Offset[2]);

    this->WorldPosition->SetValue( (*(this->HandleTransformMatrix))[0][3],
                                   (*(this->HandleTransformMatrix))[1][3],
                                   (*(this->HandleTransformMatrix))[2][3] );
    this->WorldPositionTime.Modified();
    }
}

//-------------------------------------------------------------------------
void vtkPolygonalHandleRepresentation3D::SetDisplayPosition(double p[3])
{
  if (this->Renderer && this->PointPlacer)
    { 
    if (this->PointPlacer->ValidateDisplayPosition( this->Renderer, p))
      {
      double worldPos[3], worldOrient[9];
      if (this->PointPlacer->ComputeWorldPosition( 
            this->Renderer, p, worldPos, worldOrient ))
        {
        this->DisplayPosition->SetValue(p);
        this->WorldPosition->SetValue(worldPos);
        this->DisplayPositionTime.Modified();
        this->SetWorldPosition(this->WorldPosition->GetValue());
        }
      }
    }
  else 
    {
    this->DisplayPosition->SetValue(p);
    this->DisplayPositionTime.Modified();
    }
}

//-------------------------------------------------------------------------
int vtkPolygonalHandleRepresentation3D
::ComputeInteractionState(int X, int Y, int vtkNotUsed(modify))
{
  this->VisibilityOn(); //actor must be on to be picked
  this->HandlePicker->Pick(X,Y,0.0,this->Renderer);
  vtkAssemblyPath *path = this->HandlePicker->GetPath();

  if ( path != NULL )
    {
    this->InteractionState = vtkHandleRepresentation::Nearby;
    }
  else
    {
    this->InteractionState = vtkHandleRepresentation::Outside;
    if ( this->ActiveRepresentation )
      {
      this->VisibilityOff();
      }
    }

  return this->InteractionState;
}

//-------------------------------------------------------------------------
int vtkPolygonalHandleRepresentation3D::DetermineConstraintAxis(
  int constraint, double *x, double *startPickPoint)
{
  // Look for trivial cases
  if ( ! this->Constrained )
    {
    return -1;
    }
  else if ( constraint >= 0 && constraint < 3 )
    {
    return constraint;
    }

  // Okay, figure out constraint. First see if the choice is
  // outside the hot spot
  if ( ! x )
    {
    double p[3];
    this->HandlePicker->GetPickPosition(p);
    if ( vtkMath::Distance2BetweenPoints(p,this->LastPickPosition) > 0.0)
      {
      this->WaitingForMotion = 0;
      return 0;
      }
    else
      {
      this->WaitingForMotion = 1;
      this->WaitCount = 0;
      return -1;
      }
    }
  else if (x) 
    {
    this->WaitingForMotion = 0;
    double v[3];
    v[0] = fabs(x[0] - startPickPoint[0]);
    v[1] = fabs(x[1] - startPickPoint[1]);
    v[2] = fabs(x[2] - startPickPoint[2]);
    return ( v[0]>v[1] ? (v[0]>v[2]?0:2) : (v[1]>v[2]?1:2));
    }
  else
    {
    return -1;
    }
}

//----------------------------------------------------------------------
// Record the current event position, and the rectilinear wipe position.
void vtkPolygonalHandleRepresentation3D::StartWidgetInteraction(double startEventPos[2])
{
  this->StartEventPosition[0] = startEventPos[0];
  this->StartEventPosition[1] = startEventPos[1];
  this->StartEventPosition[2] = 0.0;

  this->LastEventPosition[0] = startEventPos[0];
  this->LastEventPosition[1] = startEventPos[1];

  this->HandlePicker->Pick(startEventPos[0],startEventPos[1],0.0,this->Renderer);

  // Did we pick the handle ?
  if ( this->HandlePicker->GetPath() )
    {
    this->InteractionState = vtkHandleRepresentation::Nearby;
    this->ConstraintAxis = -1;
    this->HandlePicker->GetPickPosition(this->LastPickPosition);
    }
  else
    {
    this->InteractionState = vtkHandleRepresentation::Outside;
    this->ConstraintAxis = -1;
    }
  this->WaitCount = 0;
}

//----------------------------------------------------------------------
// Based on the displacement vector (computed in display coordinates) and
// the cursor state (which corresponds to which part of the widget has been
// selected), the widget points are modified.
// First construct a local coordinate system based on the display coordinates
// of the widget.
void vtkPolygonalHandleRepresentation3D::WidgetInteraction(double eventPos[2])
{
  // Do different things depending on state
  // Calculations everybody does
  double focalPoint[4], pickPoint[4], prevPickPoint[4], startPickPoint[4], z;

  // Compute the two points defining the motion vector
  vtkInteractorObserver::ComputeWorldToDisplay(
      this->Renderer,
      this->LastPickPosition[0], 
      this->LastPickPosition[1],
      this->LastPickPosition[2], focalPoint);
  z = focalPoint[2];
  vtkInteractorObserver::ComputeDisplayToWorld(
      this->Renderer, 
      this->LastEventPosition[0], 
      this->LastEventPosition[1], z, prevPickPoint);
  vtkInteractorObserver::ComputeDisplayToWorld(
      this->Renderer, eventPos[0], eventPos[1], z, pickPoint);

  // Process the motion
  if ( this->InteractionState == vtkHandleRepresentation::Selecting ||
       this->InteractionState == vtkHandleRepresentation::Translating )
    {
    this->WaitCount++;

    if ( this->WaitCount > 3 || !this->Constrained )
      {
      vtkInteractorObserver::ComputeDisplayToWorld(
          this->Renderer, 
          this->StartEventPosition[0], 
          this->StartEventPosition[1], z, startPickPoint);

      this->ConstraintAxis = this->DetermineConstraintAxis(
          this->ConstraintAxis,pickPoint, startPickPoint);

      if (    this->InteractionState == vtkHandleRepresentation::Selecting )
        {
        // If we are doing axis constrained motion, igonore the placer.
        // Can't have both the placer and an axis constraint dictating
        // handle placement.
        if (this->ConstraintAxis >= 0 || this->Constrained || !this->PointPlacer)
          {
          this->MoveFocus( prevPickPoint, pickPoint );
          }
        else
          {
          double newCenterPointRequested[3]; // displayPosition
          double newCenterPoint[3], worldOrient[9];

          // Make a request for the new position.
          this->MoveFocusRequest( prevPickPoint,
                                  pickPoint,
                                  newCenterPointRequested );

          vtkFocalPlanePointPlacer * fPlacer 
            = vtkFocalPlanePointPlacer::SafeDownCast( this->PointPlacer );
          if (fPlacer)
            {
            // Offset the placer plane to one that passes through the current 
            // world position and is parallel to the focal plane. Offset =
            // the distance currentWorldPos is from the focal plane
            //
            double currentWorldPos[3], projDir[3], fp[3];
            this->GetWorldPosition( currentWorldPos );
            this->Renderer->GetActiveCamera()->GetFocalPoint(fp);
            double vec[3] = { currentWorldPos[0] - fp[0],
                              currentWorldPos[1] - fp[1],
                              currentWorldPos[2] - fp[2]};
            this->Renderer->GetActiveCamera()->GetDirectionOfProjection(projDir);
            fPlacer->SetOffset( vtkMath::Dot( vec, projDir ) );
            }


          // See what the placer says.
          if (this->PointPlacer->ComputeWorldPosition( 
                this->Renderer, newCenterPointRequested, newCenterPoint,
                worldOrient ))
            {
            // Once the placer has validated us, update the handle position 
            this->SetWorldPosition( newCenterPoint );
            }
          }
        }
      else
        {
        // If we are doing axis constrained motion, igonore the placer.
        // Can't have both the placer and the axis constraint dictating
        // handle placement.
        if (this->ConstraintAxis >= 0 || this->Constrained || !this->PointPlacer)
          {
          this->Translate(prevPickPoint, pickPoint);
          }
        else
          {
          double newCenterPointRequested[3]; // displayPosition
          double newCenterPoint[3], worldOrient[9];

          // Make a request for the new position.
          this->MoveFocusRequest( prevPickPoint, 
                                  pickPoint, 
                                  newCenterPointRequested);

          vtkFocalPlanePointPlacer * fPlacer 
            = vtkFocalPlanePointPlacer::SafeDownCast( this->PointPlacer );
          if (fPlacer)
            {
            // Offset the placer plane to one that passes through the current 
            // world position and is parallel to the focal plane. Offset =
            // the distance currentWorldPos is from the focal plane
            //
            double currentWorldPos[3], projDir[3], fp[3];
            this->GetWorldPosition( currentWorldPos );
            this->Renderer->GetActiveCamera()->GetFocalPoint(fp);
            double vec[3] = { currentWorldPos[0] - fp[0],
                              currentWorldPos[1] - fp[1],
                              currentWorldPos[2] - fp[2]};
            this->Renderer->GetActiveCamera()->GetDirectionOfProjection(projDir);
            fPlacer->SetOffset( vtkMath::Dot( vec, projDir ) );
            }
          
          // See what the placer says.
          if (this->PointPlacer->ComputeWorldPosition( 
                this->Renderer, newCenterPointRequested, newCenterPoint,
                worldOrient ))
            {
            this->SetWorldPosition( newCenterPoint );
            }
          }
        }
      }
    }

  else if ( this->InteractionState == vtkHandleRepresentation::Scaling )
    {
    // Scaling does not change the position of the handle, we needn't
    // ask the placer..
    this->Scale(prevPickPoint, pickPoint, eventPos);
    }

  // Book keeping
  this->LastEventPosition[0] = eventPos[0];
  this->LastEventPosition[1] = eventPos[1];
  
  this->Modified();
}

//----------------------------------------------------------------------
void vtkPolygonalHandleRepresentation3D
::MoveFocusRequest(double *p1, double *p2, double center[3])
{
  double focus[4];
  this->GetWorldPosition( focus );

  // Move the center of the handle along the motion vector
  focus[0] += (p2[0] - p1[0]);
  focus[1] += (p2[1] - p1[1]);
  focus[2] += (p2[2] - p1[2]);
  focus[3] = 1.0;

  // Get the display position that this center would fall on.
  this->Renderer->SetWorldPoint( focus );
  this->Renderer->WorldToDisplay();
  this->Renderer->GetDisplayPoint( center );
}

//----------------------------------------------------------------------
void vtkPolygonalHandleRepresentation3D::MoveFocus(double *p1, double *p2)
{
  //Get the motion vector
  double v[3];
  v[0] = p2[0] - p1[0];
  v[1] = p2[1] - p1[1];
  v[2] = p2[2] - p1[2];

  double focus[3];
  this->GetWorldPosition( focus );
  if ( this->ConstraintAxis >= 0 )
    {
    focus[this->ConstraintAxis] += v[this->ConstraintAxis];
    }
  else
    {
    focus[0] += v[0];
    focus[1] += v[1];
    focus[2] += v[2];
    }
  
  this->SetWorldPosition(focus);
}

//----------------------------------------------------------------------
// Translate everything
void vtkPolygonalHandleRepresentation3D::Translate(double *p1, double *p2)
{
  //Get the motion vector
  double v[3], pos[3];
  v[0] = p2[0] - p1[0];
  v[1] = p2[1] - p1[1];
  v[2] = p2[2] - p1[2];
  
  this->GetWorldPosition( pos );
  double newFocus[3];
  int i;

  if ( this->ConstraintAxis >= 0 )
    {//move along axis
    for (i=0; i<3; i++)
      {
      if ( i != this->ConstraintAxis )
        {
        v[i] = 0.0;
        }
      }
    }
  
  for (i=0; i<3; i++)
    {
    newFocus[i] = pos[i] + v[i];
    }
  
  this->SetWorldPosition(newFocus);
}

//----------------------------------------------------------------------
void vtkPolygonalHandleRepresentation3D
::Scale(double *, double *, double eventPos[2])
{
  double sf = 1.0 + (eventPos[1] - this->LastEventPosition[1])
                   / this->Renderer->GetSize()[1];
  if (sf == 1.0)
    {
    return;
    }

  double handleSize = this->HandleTransformMatrix->GetElement(0,0) * sf;
  handleSize = (handleSize < 0.001 ? 0.001 : handleSize);

  this->HandleTransformMatrix->SetElement(0, 0, handleSize);
  this->HandleTransformMatrix->SetElement(1, 1, handleSize);
  this->HandleTransformMatrix->SetElement(2, 2, handleSize);
}

//----------------------------------------------------------------------
void vtkPolygonalHandleRepresentation3D::Highlight(int highlight)
{
 this->Actor->SetProperty(highlight ? this->SelectedProperty : this->Property);
}

//----------------------------------------------------------------------
void vtkPolygonalHandleRepresentation3D::CreateDefaultProperties()
{
  this->Property = vtkProperty::New();
  this->Property->SetLineWidth(0.5);

  this->SelectedProperty = vtkProperty::New();
  this->SelectedProperty->SetAmbient(1.0);
  this->SelectedProperty->SetAmbientColor(0.0,1.0,0.0);
  this->SelectedProperty->SetLineWidth(2.0);
}

//----------------------------------------------------------------------
void vtkPolygonalHandleRepresentation3D::BuildRepresentation()
{
  // The net effect is to resize the handle
  if ( this->GetMTime() > this->BuildTime || 
       (this->Renderer && this->Renderer->GetVTKWindow() &&
        this->Renderer->GetVTKWindow()->GetMTime() > this->BuildTime) )
    {
    this->HandleTransformFilter->Update();
    this->BuildTime.Modified();
    }
}

//----------------------------------------------------------------------
void vtkPolygonalHandleRepresentation3D::ShallowCopy(vtkProp *prop)
{
  vtkPolygonalHandleRepresentation3D *rep = 
    vtkPolygonalHandleRepresentation3D::SafeDownCast(prop);
  if ( rep )
    {
    this->SetProperty(rep->GetProperty());
    this->SetSelectedProperty(rep->GetSelectedProperty());
    }
  this->Superclass::ShallowCopy(prop);
}

//----------------------------------------------------------------------
void vtkPolygonalHandleRepresentation3D::GetActors(vtkPropCollection *pc)
{
  this->Actor->GetActors(pc);
}

//----------------------------------------------------------------------
void vtkPolygonalHandleRepresentation3D::ReleaseGraphicsResources(vtkWindow *win)
{
  this->Actor->ReleaseGraphicsResources(win);
}

//----------------------------------------------------------------------
int vtkPolygonalHandleRepresentation3D::RenderOpaqueGeometry(vtkViewport *viewport)
{
  this->BuildRepresentation();
  return this->Actor->RenderOpaqueGeometry(viewport);
}

//-----------------------------------------------------------------------------
int vtkPolygonalHandleRepresentation3D::RenderTranslucentPolygonalGeometry(
  vtkViewport *viewport)
{
  this->BuildRepresentation();
  return this->Actor->RenderTranslucentPolygonalGeometry(viewport);
}

//-----------------------------------------------------------------------------
int vtkPolygonalHandleRepresentation3D::HasTranslucentPolygonalGeometry()
{
  this->BuildRepresentation();
  return this->Actor->HasTranslucentPolygonalGeometry();
}

//-----------------------------------------------------------------------------
double* vtkPolygonalHandleRepresentation3D::GetBounds()
{
  return this->Actor ? this->Actor->GetBounds() : NULL;
}

//-----------------------------------------------------------------------------
vtkAbstractTransform* vtkPolygonalHandleRepresentation3D::GetTransform()
{
  return this->HandleTransform;
}

//----------------------------------------------------------------------
void vtkPolygonalHandleRepresentation3D::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  
  if ( this->Property )
    {
    os << indent << "Property: " << this->Property << "\n";
    }
  else
    {
    os << indent << "Property: (none)\n";
    }
  if ( this->SelectedProperty )
    {
    os << indent << "Selected Property: " << this->SelectedProperty << "\n";
    }
  else
    {
    os << indent << "Selected Property: (none)\n";
    }
  os << indent << "Actor: " << this->Actor << "\n";
  this->Actor->PrintSelf(os,indent.GetNextIndent());
  os << indent << "Mapper: " << this->Mapper << "\n";
  this->Mapper->PrintSelf(os,indent.GetNextIndent());
  os << indent << "HandleTransformFilter: " << this->HandleTransformFilter << "\n";
  this->HandleTransformFilter->PrintSelf(os,indent.GetNextIndent());
  os << indent << "HandleTransform: " << this->HandleTransform << "\n";
  this->HandleTransform->PrintSelf(os,indent.GetNextIndent());
  os << indent << "HandleTransformMatrix: " << this->HandleTransformMatrix << "\n";
  this->HandleTransformMatrix->PrintSelf(os,indent.GetNextIndent());
  os << indent << "HandlePicker: " << this->HandlePicker << "\n";
  this->HandlePicker->PrintSelf(os,indent.GetNextIndent());
  os << indent << "LastPickPosition: (" << this->LastPickPosition[0] 
     << "," << this->LastPickPosition[1] << ")\n";
  os << indent << "LastEventPosition: (" << this->LastEventPosition[0] 
     << "," << this->LastEventPosition[1] << ")\n";
  os << indent << "Offset: (" << this->Offset[0] << "," 
     << this->Offset[1] << ")\n";
  // ConstraintAxis;
  // WaitingForMotion;
  // WaitCount;
}
