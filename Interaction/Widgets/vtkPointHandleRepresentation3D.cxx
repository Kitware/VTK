/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPointHandleRepresentation3D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPointHandleRepresentation3D.h"
#include "vtkActor.h"
#include "vtkAssemblyPath.h"
#include "vtkCamera.h"
#include "vtkCellPicker.h"
#include "vtkCoordinate.h"
#include "vtkCursor3D.h"
#include "vtkEventData.h"
#include "vtkFocalPlanePointPlacer.h"
#include "vtkInteractorObserver.h"
#include "vtkLine.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPickingManager.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"

#include <assert.h>

vtkStandardNewMacro(vtkPointHandleRepresentation3D);

vtkCxxSetObjectMacro(vtkPointHandleRepresentation3D, Property, vtkProperty);
vtkCxxSetObjectMacro(vtkPointHandleRepresentation3D, SelectedProperty, vtkProperty);

//----------------------------------------------------------------------
vtkPointHandleRepresentation3D::vtkPointHandleRepresentation3D()
{
  // Initialize state
  this->InteractionState = vtkHandleRepresentation::Outside;

  // Represent the line
  this->Cursor3D = vtkCursor3D::New();
  this->Cursor3D->AllOff();
  this->Cursor3D->AxesOn();
  this->Cursor3D->TranslationModeOn();

  this->Mapper = vtkPolyDataMapper::New();
  this->Mapper->SetInputConnection(this->Cursor3D->GetOutputPort());

  // Set up the initial properties
  this->CreateDefaultProperties();

  this->Actor = vtkActor::New();
  this->Actor->SetMapper(this->Mapper);
  this->Actor->SetProperty(this->Property);

  // Manage the picking stuff
  this->CursorPicker = vtkCellPicker::New();
  this->CursorPicker->PickFromListOn();
  this->CursorPicker->AddPickList(this->Actor);
  this->CursorPicker->SetTolerance(0.01); // need some fluff

  // Override superclass'
  this->PlaceFactor = 1.0;

  // The size of the hot spot
  this->HotSpotSize = 0.05;
  this->WaitingForMotion = 0;
  this->ConstraintAxis = -1;

  // Current handle size
  this->HandleSize = 15.0; // in pixels
  this->CurrentHandleSize = this->HandleSize;

  // Translation control
  this->TranslationMode = 1;

  vtkFocalPlanePointPlacer* pointPlacer = vtkFocalPlanePointPlacer::New();
  this->SetPointPlacer(pointPlacer);
  pointPlacer->Delete();

  // Continuous moves
  this->SmoothMotion = 1;
}

//----------------------------------------------------------------------
vtkPointHandleRepresentation3D::~vtkPointHandleRepresentation3D()
{
  this->Cursor3D->Delete();
  this->CursorPicker->Delete();
  this->Mapper->Delete();
  this->Actor->Delete();
  this->Property->Delete();
  this->SelectedProperty->Delete();
}

//----------------------------------------------------------------------
void vtkPointHandleRepresentation3D::RegisterPickers()
{
  vtkPickingManager* pm = this->GetPickingManager();
  if (!pm)
  {
    return;
  }
  pm->AddPicker(this->CursorPicker, this);
}

//-------------------------------------------------------------------------
void vtkPointHandleRepresentation3D::PlaceWidget(double bds[6])
{
  int i;
  double bounds[6], center[3];

  this->AdjustBounds(bds, bounds, center);

  this->Cursor3D->SetModelBounds(bounds);
  this->SetWorldPosition(center);

  for (i = 0; i < 6; i++)
  {
    this->InitialBounds[i] = bounds[i];
  }
  this->InitialLength = sqrt((bounds[1] - bounds[0]) * (bounds[1] - bounds[0]) +
    (bounds[3] - bounds[2]) * (bounds[3] - bounds[2]) +
    (bounds[5] - bounds[4]) * (bounds[5] - bounds[4]));
}

//-------------------------------------------------------------------------
double* vtkPointHandleRepresentation3D::GetBounds()
{
  return this->Cursor3D->GetModelBounds();
}

//-------------------------------------------------------------------------
void vtkPointHandleRepresentation3D::SetWorldPosition(double p[3])
{
  if (this->Renderer && this->PointPlacer)
  {
    if (this->PointPlacer->ValidateWorldPosition(p))
    {
      this->Cursor3D->SetFocalPoint(p); // this may clamp the point
      this->WorldPosition->SetValue(this->Cursor3D->GetFocalPoint());
      this->WorldPositionTime.Modified();
    }
  }
  else
  {
    this->Cursor3D->SetFocalPoint(p); // this may clamp the point
    this->WorldPosition->SetValue(this->Cursor3D->GetFocalPoint());
    this->WorldPositionTime.Modified();
  }
}

//-------------------------------------------------------------------------
void vtkPointHandleRepresentation3D::SetDisplayPosition(double p[3])
{
  if (this->Renderer && this->PointPlacer)
  {
    if (this->PointPlacer->ValidateDisplayPosition(this->Renderer, p))
    {
      double worldPos[3], worldOrient[9];
      if (this->PointPlacer->ComputeWorldPosition(this->Renderer, p, worldPos, worldOrient))
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
void vtkPointHandleRepresentation3D::SetHandleSize(double size)
{
  this->Superclass::SetHandleSize(size);
  this->CurrentHandleSize = this->HandleSize;
}

//-------------------------------------------------------------------------
int vtkPointHandleRepresentation3D ::ComputeInteractionState(int X, int Y, int vtkNotUsed(modify))
{
  this->VisibilityOn(); // actor must be on to be picked

  // First make sure that the cursor is within the bounding sphere of the
  // representation in display space.
  double d[3], bounds[6];
  this->Cursor3D->GetModelBounds(bounds);
  this->GetDisplayPosition(d);

  if (!this->NearbyEvent(X, Y, bounds))
  {
    this->InteractionState = vtkHandleRepresentation::Outside;
    return this->InteractionState;
  }

  // Now see if anything is picked
  vtkAssemblyPath* path = this->GetAssemblyPath(X, Y, 0., this->CursorPicker);

  if (path != nullptr)
  {
    this->InteractionState = vtkHandleRepresentation::Nearby;
  }
  else
  {
    this->InteractionState = vtkHandleRepresentation::Outside;
    if (this->ActiveRepresentation)
    {
      this->VisibilityOff();
    }
  }

  return this->InteractionState;
}

//-------------------------------------------------------------------------
int vtkPointHandleRepresentation3D::ComputeComplexInteractionState(
  vtkRenderWindowInteractor*, vtkAbstractWidget*, unsigned long, void* calldata, int)
{
  this->VisibilityOn(); // actor must be on to be picked

  vtkEventData* edata = static_cast<vtkEventData*>(calldata);
  vtkEventDataDevice3D* edd = edata->GetAsEventDataDevice3D();
  if (edd)
  {
    double pos[3];
    edd->GetWorldPosition(pos);
    vtkAssemblyPath* path = this->GetAssemblyPath3DPoint(pos, this->CursorPicker);
    double focus[3];
    this->Cursor3D->GetFocalPoint(focus);
    double d[3];
    this->GetDisplayPosition(d);

    if (path != nullptr)
    {
      this->InteractionState = vtkHandleRepresentation::Nearby;
    }
    else
    {
      this->InteractionState = vtkHandleRepresentation::Outside;
      if (this->ActiveRepresentation)
      {
        this->VisibilityOff();
      }
    }
  }

  return this->InteractionState;
}

//-------------------------------------------------------------------------
int vtkPointHandleRepresentation3D::DetermineConstraintAxis(
  int constraint, double* x, double* startPickPoint)
{
  // Look for trivial cases
  if (!this->Constrained)
  {
    return -1;
  }
  else if (constraint >= 0 && constraint < 3)
  {
    return constraint;
  }

  // Okay, figure out constraint. First see if the choice is
  // outside the hot spot
  if (!x)
  {
    double p[3], d2, tol;
    this->CursorPicker->GetPickPosition(p);
    d2 = vtkMath::Distance2BetweenPoints(p, this->LastPickPosition);
    tol = this->HotSpotSize * this->InitialLength;
    if (d2 > (tol * tol))
    {
      this->WaitingForMotion = 0;
      return this->CursorPicker->GetCellId();
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
    return (v[0] > v[1] ? (v[0] > v[2] ? 0 : 2) : (v[1] > v[2] ? 1 : 2));
  }
  else
  {
    return -1;
  }
}

//----------------------------------------------------------------------
// Record the current event position, and the translation state
void vtkPointHandleRepresentation3D::StartWidgetInteraction(double startEventPos[2])
{
  this->StartEventPosition[0] = startEventPos[0];
  this->StartEventPosition[1] = startEventPos[1];
  this->StartEventPosition[2] = 0.0;

  this->LastEventPosition[0] = startEventPos[0];
  this->LastEventPosition[1] = startEventPos[1];

  // Make sure events are close to widget and something is picked
  double bounds[6];
  this->Cursor3D->GetModelBounds(bounds);
  bool nearby = this->NearbyEvent(startEventPos[0], startEventPos[1], bounds);
  vtkAssemblyPath* path =
    this->GetAssemblyPath(startEventPos[0], startEventPos[1], 0., this->CursorPicker);

  if (nearby && path != nullptr)
  {
    this->InteractionState = vtkHandleRepresentation::Nearby;
    this->ConstraintAxis = -1;
    this->CursorPicker->GetPickPosition(this->LastPickPosition);
  }
  else
  {
    this->InteractionState = vtkHandleRepresentation::Outside;
    this->ConstraintAxis = -1;
  }
  this->Cursor3D->SetTranslationMode(this->TranslationMode);
  this->WaitCount = 0;
}

//----------------------------------------------------------------------
void vtkPointHandleRepresentation3D::StartComplexInteraction(
  vtkRenderWindowInteractor*, vtkAbstractWidget*, unsigned long, void* calldata)
{
  vtkEventData* edata = static_cast<vtkEventData*>(calldata);
  vtkEventDataDevice3D* edd = edata->GetAsEventDataDevice3D();
  if (edd)
  {
    edd->GetWorldPosition(this->StartEventPosition);
    this->LastEventPosition[0] = this->StartEventPosition[0];
    this->LastEventPosition[1] = this->StartEventPosition[1];
    this->LastEventPosition[2] = this->StartEventPosition[2];

    double bounds[6];
    this->Cursor3D->GetModelBounds(bounds);
    bool nearby =
      this->NearbyEvent(this->StartEventPosition[0], this->StartEventPosition[1], bounds);
    vtkAssemblyPath* path =
      this->GetAssemblyPath3DPoint(this->StartEventPosition, this->CursorPicker);

    if (nearby && path != nullptr)
    {
      this->InteractionState = vtkHandleRepresentation::Nearby;
      this->ConstraintAxis = -1;
      this->CursorPicker->GetPickPosition(this->LastPickPosition);
    }
    else
    {
      this->InteractionState = vtkHandleRepresentation::Outside;
      this->ConstraintAxis = -1;
    }
    this->Cursor3D->SetTranslationMode(this->TranslationMode);
    this->WaitCount = 0;
  }
}

//----------------------------------------------------------------------
// Based on the displacement vector (computed in display coordinates) and
// the cursor state (which corresponds to which part of the widget has been
// selected), the widget points are modified.
// First construct a local coordinate system based on the display coordinates
// of the widget.
void vtkPointHandleRepresentation3D::WidgetInteraction(double eventPos[2])
{
  // Do different things depending on state
  // Calculations everybody does
  double focalPoint[4], pickPoint[4], prevPickPoint[4], startPickPoint[4], z;

  // Compute the two points defining the motion vector
  vtkInteractorObserver::ComputeWorldToDisplay(this->Renderer, this->LastPickPosition[0],
    this->LastPickPosition[1], this->LastPickPosition[2], focalPoint);
  z = focalPoint[2];
  vtkInteractorObserver::ComputeDisplayToWorld(
    this->Renderer, this->LastEventPosition[0], this->LastEventPosition[1], z, prevPickPoint);
  vtkInteractorObserver::ComputeDisplayToWorld(
    this->Renderer, eventPos[0], eventPos[1], z, pickPoint);

  // Process the motion
  if (this->InteractionState == vtkHandleRepresentation::Selecting ||
    this->InteractionState == vtkHandleRepresentation::Translating)
  {
    this->WaitCount++;

    if (this->WaitCount > 3 || !this->Constrained)
    {
      vtkInteractorObserver::ComputeDisplayToWorld(this->Renderer, this->StartEventPosition[0],
        this->StartEventPosition[1], z, startPickPoint);

      this->ConstraintAxis =
        this->DetermineConstraintAxis(this->ConstraintAxis, pickPoint, startPickPoint);

      if (this->InteractionState == vtkHandleRepresentation::Selecting && !this->TranslationMode)
      {
        vtkDebugMacro(<< "Processing widget interaction for Select mode");

        // If we are doing axis constrained motion, ignore the placer.
        // Can't have both the placer and an axis constraint dictating
        // handle placement.
        if (this->ConstraintAxis >= 0 || this->Constrained || !this->PointPlacer)
        {
          this->MoveFocus(prevPickPoint, pickPoint);
        }
        else
        {
          double newCenterPointRequested[3]; // displayPosition
          double newCenterPoint[3], worldOrient[9];

          // Make a request for the new position.
          this->MoveFocusRequest(prevPickPoint, pickPoint, eventPos, newCenterPointRequested);

          vtkFocalPlanePointPlacer* fPlacer =
            vtkFocalPlanePointPlacer::SafeDownCast(this->PointPlacer);
          if (fPlacer)
          {
            // Offset the placer plane to one that passes through the current
            // world position and is parallel to the focal plane. Offset =
            // the distance currentWorldPos is from the focal plane
            //
            double currentWorldPos[3], projDir[3], fp[3];
            this->GetWorldPosition(currentWorldPos);
            this->Renderer->GetActiveCamera()->GetFocalPoint(fp);
            double vec[3] = { currentWorldPos[0] - fp[0], currentWorldPos[1] - fp[1],
              currentWorldPos[2] - fp[2] };
            this->Renderer->GetActiveCamera()->GetDirectionOfProjection(projDir);
            fPlacer->SetOffset(vtkMath::Dot(vec, projDir));
          }

          vtkDebugMacro(<< "Request for computing world position at "
                        << "display position of " << newCenterPointRequested[0] << ","
                        << newCenterPointRequested[1]);

          // See what the placer says.
          if (this->PointPlacer->ComputeWorldPosition(
                this->Renderer, newCenterPointRequested, newCenterPoint, worldOrient))
          {
            // Once the placer has validated us, update the handle position
            this->SetWorldPosition(newCenterPoint);
          }
        }
      }
      else
      {
        vtkDebugMacro(<< "Processing widget interaction for translate");

        // If we are doing axis constrained motion, ignore the placer.
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
          this->MoveFocusRequest(prevPickPoint, pickPoint, eventPos, newCenterPointRequested);

          vtkFocalPlanePointPlacer* fPlacer =
            vtkFocalPlanePointPlacer::SafeDownCast(this->PointPlacer);
          if (fPlacer)
          {
            // Offset the placer plane to one that passes through the current
            // world position and is parallel to the focal plane. Offset =
            // the distance currentWorldPos is from the focal plane
            //
            double currentWorldPos[3], projDir[3], fp[3];
            this->GetWorldPosition(currentWorldPos);
            this->Renderer->GetActiveCamera()->GetFocalPoint(fp);
            double vec[3] = { currentWorldPos[0] - fp[0], currentWorldPos[1] - fp[1],
              currentWorldPos[2] - fp[2] };
            this->Renderer->GetActiveCamera()->GetDirectionOfProjection(projDir);
            fPlacer->SetOffset(vtkMath::Dot(vec, projDir));
          }

          vtkDebugMacro(<< "Request for computing world position at "
                        << "display position of " << newCenterPointRequested[0] << ","
                        << newCenterPointRequested[1]);

          // See what the placer says.
          if (this->PointPlacer->ComputeWorldPosition(
                this->Renderer, newCenterPointRequested, newCenterPoint, worldOrient))
          {

            // Once the placer has validated us, update the handle
            // position and its bounds.
            double* p = this->GetWorldPosition();

            // Get the motion vector
            double v[3] = { newCenterPoint[0] - p[0], newCenterPoint[1] - p[1],
              newCenterPoint[2] - p[2] };
            double *bounds = this->Cursor3D->GetModelBounds(), newBounds[6];
            for (int i = 0; i < 3; i++)
            {
              newBounds[2 * i] = bounds[2 * i] + v[i];
              newBounds[2 * i + 1] = bounds[2 * i + 1] + v[i];
            }

            this->Cursor3D->SetModelBounds(newBounds);
            this->SetWorldPosition(newCenterPoint);
          }
        }
      }
    }
  }

  else if (this->InteractionState == vtkHandleRepresentation::Scaling)
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

void vtkPointHandleRepresentation3D::ComplexInteraction(
  vtkRenderWindowInteractor*, vtkAbstractWidget*, unsigned long, void* calldata)
{
  vtkEventData* edata = static_cast<vtkEventData*>(calldata);
  vtkEventDataDevice3D* edd = edata->GetAsEventDataDevice3D();
  if (edd)
  {
    double eventPos[3];
    edd->GetWorldPosition(eventPos);
    // Process the motion
    if (this->InteractionState == vtkHandleRepresentation::Selecting ||
      this->InteractionState == vtkHandleRepresentation::Translating)
    {
      this->WaitCount++;

      if (this->WaitCount > 3 || !this->Constrained)
      {
        this->ConstraintAxis =
          this->DetermineConstraintAxis(this->ConstraintAxis, eventPos, this->StartEventPosition);

        if (this->InteractionState == vtkHandleRepresentation::Selecting && !this->TranslationMode)
        {
          vtkDebugMacro(<< "Processing widget interaction for Select mode");

          this->MoveFocus(this->LastEventPosition, eventPos);
        }
        else
        {
          vtkDebugMacro(<< "Processing widget interaction for translate");
          this->Translate(this->LastEventPosition, eventPos);
        }
      }
    }

    // Book keeping
    this->LastEventPosition[0] = eventPos[0];
    this->LastEventPosition[1] = eventPos[1];
    this->LastEventPosition[2] = eventPos[2];

    this->Modified();
  }
}

//----------------------------------------------------------------------
void vtkPointHandleRepresentation3D ::MoveFocusRequest(
  const double* p1, const double* p2, const double eventPos[2], double center[3])
{
  if (this->SmoothMotion)
  {
    double focus[4], v[3];
    this->Cursor3D->GetFocalPoint(focus);
    this->GetTranslationVector(p1, p2, v);

    // Move the center of the handle along the motion vector
    focus[0] += v[0];
    focus[1] += v[1];
    focus[2] += v[2];
    focus[3] = 1.0;

    // Get the display position that this center would fall on.
    this->Renderer->SetWorldPoint(focus);
    this->Renderer->WorldToDisplay();
    this->Renderer->GetDisplayPoint(center);
  }
  else
  {
    center[0] = eventPos[0];
    center[1] = eventPos[1];
    center[2] = 1.0;
  }
}

//----------------------------------------------------------------------
void vtkPointHandleRepresentation3D::MoveFocus(const double* p1, const double* p2)
{
  this->Translate(p1, p2);
}

//----------------------------------------------------------------------
void vtkPointHandleRepresentation3D::SetTranslationMode(vtkTypeBool mode)
{
  if (this->TranslationMode != mode)
  {
    this->TranslationMode = mode;
    // Pass new setting to Cursor3D, otherwise PlaceWidget will not work
    // as it should when TranslationMode is off.
    this->Cursor3D->SetTranslationMode(mode);
    this->Modified();
  }
}

//----------------------------------------------------------------------
// Translate everything
void vtkPointHandleRepresentation3D::Translate(const double* p1, const double* p2)
{
  double v[3] = { 0, 0, 0 };
  vtkHandleRepresentation::Translate(p1, p2);
  this->GetTranslationVector(p1, p2, v);

  double* bounds = this->Cursor3D->GetModelBounds();
  double* pos = this->Cursor3D->GetFocalPoint();
  double newBounds[6], newFocus[3];
  int i;

  if (this->ConstraintAxis >= 0)
  { // move along axis
    for (i = 0; i < 3; i++)
    {
      if (i != this->ConstraintAxis)
      {
        v[i] = 0.0;
      }
    }
  }

  for (i = 0; i < 3; i++)
  {
    newBounds[2 * i] = bounds[2 * i] + v[i];
    newBounds[2 * i + 1] = bounds[2 * i + 1] + v[i];
    newFocus[i] = pos[i] + v[i];
  }

  this->Cursor3D->SetModelBounds(newBounds);
  this->Cursor3D->SetFocalPoint(newFocus);
}

//----------------------------------------------------------------------
void vtkPointHandleRepresentation3D::SizeBounds()
{
  // Only change the size of the bounding box if translation mode is on.
  if (this->TranslationMode)
  {
    double center[3], bounds[6];
    this->Cursor3D->GetFocalPoint(center);
    double radius = this->SizeHandlesInPixels(1.0, center);
    radius *= this->CurrentHandleSize / this->HandleSize;

    for (int i = 0; i < 3; i++)
    {
      bounds[2 * i] = center[i] - radius;
      bounds[2 * i + 1] = center[i] + radius;
    }
    this->Cursor3D->SetModelBounds(bounds);
  }
}

//----------------------------------------------------------------------
void vtkPointHandleRepresentation3D::Scale(
  const double* p1, const double* p2, const double eventPos[2])
{
  // Get the motion vector
  double v[3];
  v[0] = p2[0] - p1[0];
  v[1] = p2[1] - p1[1];
  v[2] = p2[2] - p1[2];

  double* bounds = this->Cursor3D->GetModelBounds();

  // Compute the scale factor
  double sf = vtkMath::Norm(v) /
    sqrt((bounds[1] - bounds[0]) * (bounds[1] - bounds[0]) +
      (bounds[3] - bounds[2]) * (bounds[3] - bounds[2]) +
      (bounds[5] - bounds[4]) * (bounds[5] - bounds[4]));

  if (eventPos[1] > this->LastEventPosition[1])
  {
    sf = 1.0 + sf;
  }
  else
  {
    sf = 1.0 - sf;
  }

  this->CurrentHandleSize *= sf;
  this->CurrentHandleSize = (this->CurrentHandleSize < 0.001 ? 0.001 : this->CurrentHandleSize);

  this->SizeBounds();
}

//----------------------------------------------------------------------
void vtkPointHandleRepresentation3D::Highlight(int highlight)
{
  if (highlight)
  {
    this->Actor->SetProperty(this->SelectedProperty);
  }
  else
  {
    this->Actor->SetProperty(this->Property);
  }
}

//----------------------------------------------------------------------
void vtkPointHandleRepresentation3D::CreateDefaultProperties()
{
  this->Property = vtkProperty::New();
  this->Property->SetAmbient(1.0);
  this->Property->SetAmbientColor(1.0, 1.0, 1.0);
  this->Property->SetLineWidth(0.5);

  this->SelectedProperty = vtkProperty::New();
  this->SelectedProperty->SetAmbient(1.0);
  this->SelectedProperty->SetAmbientColor(0.0, 1.0, 0.0);
  this->SelectedProperty->SetLineWidth(2.0);
}

//----------------------------------------------------------------------
void vtkPointHandleRepresentation3D::SetVisibility(vtkTypeBool visible)
{
  this->Actor->SetVisibility(visible);
  // Forward to superclass
  this->Superclass::SetVisibility(visible);
}

//----------------------------------------------------------------------
void vtkPointHandleRepresentation3D::BuildRepresentation()
{
  // The net effect is to resize the handle
  if (this->GetMTime() > this->BuildTime ||
    (this->Renderer && this->Renderer->GetVTKWindow() &&
      this->Renderer->GetVTKWindow()->GetMTime() > this->BuildTime))
  {
    if (!this->Placed)
    {
      this->ValidPick = 1;
      this->Placed = 1;
    }

    this->SizeBounds();
    this->Cursor3D->Update();
    this->BuildTime.Modified();
  }
}

//----------------------------------------------------------------------
void vtkPointHandleRepresentation3D::ShallowCopy(vtkProp* prop)
{
  vtkPointHandleRepresentation3D* rep = vtkPointHandleRepresentation3D::SafeDownCast(prop);
  if (rep)
  {
    this->SetOutline(rep->GetOutline());
    this->SetXShadows(rep->GetXShadows());
    this->SetYShadows(rep->GetYShadows());
    this->SetZShadows(rep->GetZShadows());
    this->SetTranslationMode(rep->GetTranslationMode());
    this->SetProperty(rep->GetProperty());
    this->Actor->SetProperty(rep->GetProperty());
    this->SetSelectedProperty(rep->GetSelectedProperty());
    this->SetHotSpotSize(rep->GetHotSpotSize());
  }
  this->Superclass::ShallowCopy(prop);
}

//----------------------------------------------------------------------
void vtkPointHandleRepresentation3D::DeepCopy(vtkProp* prop)
{
  vtkPointHandleRepresentation3D* rep = vtkPointHandleRepresentation3D::SafeDownCast(prop);
  if (rep)
  {
    this->SetOutline(rep->GetOutline());
    this->SetXShadows(rep->GetXShadows());
    this->SetYShadows(rep->GetYShadows());
    this->SetZShadows(rep->GetZShadows());
    this->SetTranslationMode(rep->GetTranslationMode());
    this->SetProperty(rep->GetProperty());
    this->Actor->SetProperty(rep->GetProperty());
    this->SetSelectedProperty(rep->GetSelectedProperty());
    this->SetHotSpotSize(rep->GetHotSpotSize());
  }
  this->Superclass::DeepCopy(prop);
}

//----------------------------------------------------------------------
void vtkPointHandleRepresentation3D::GetActors(vtkPropCollection* pc)
{
  this->Actor->GetActors(pc);
}

//----------------------------------------------------------------------
void vtkPointHandleRepresentation3D::ReleaseGraphicsResources(vtkWindow* win)
{
  this->Actor->ReleaseGraphicsResources(win);
}

//----------------------------------------------------------------------
int vtkPointHandleRepresentation3D::RenderOpaqueGeometry(vtkViewport* viewport)
{
  this->BuildRepresentation();

  // Sanity check
  double worldPos[3];
  this->GetWorldPosition(worldPos);
  if (worldPos[0] == VTK_DOUBLE_MAX)
  {
    return 0;
  }

  return this->Actor->RenderOpaqueGeometry(viewport);
}

//-----------------------------------------------------------------------------
int vtkPointHandleRepresentation3D::RenderTranslucentPolygonalGeometry(vtkViewport* viewport)
{
  this->BuildRepresentation();

  // Sanity check
  double worldPos[3];
  this->GetWorldPosition(worldPos);
  if (worldPos[0] == VTK_DOUBLE_MAX)
  {
    return 0;
  }

  return this->Actor->RenderTranslucentPolygonalGeometry(viewport);
}
//-----------------------------------------------------------------------------
vtkTypeBool vtkPointHandleRepresentation3D::HasTranslucentPolygonalGeometry()
{
  this->BuildRepresentation();
  return this->Actor->HasTranslucentPolygonalGeometry();
}

//----------------------------------------------------------------------
void vtkPointHandleRepresentation3D::PrintSelf(ostream& os, vtkIndent indent)
{
  // Superclass typedef defined in vtkTypeMacro() found in vtkSetGet.h
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Hot Spot Size: " << this->HotSpotSize << "\n";
  if (this->Property)
  {
    os << indent << "Property: " << this->Property << "\n";
  }
  else
  {
    os << indent << "Property: (none)\n";
  }
  if (this->SelectedProperty)
  {
    os << indent << "Selected Property: " << this->SelectedProperty << "\n";
  }
  else
  {
    os << indent << "Selected Property: (none)\n";
  }

  os << indent << "Outline: " << (this->GetOutline() ? "On\n" : "Off\n");
  os << indent << "XShadows: " << (this->GetXShadows() ? "On\n" : "Off\n");
  os << indent << "YShadows: " << (this->GetYShadows() ? "On\n" : "Off\n");
  os << indent << "ZShadows: " << (this->GetZShadows() ? "On\n" : "Off\n");

  os << indent << "Translation Mode: " << (this->TranslationMode ? "On\n" : "Off\n");
  os << indent << "SmoothMotion: " << this->SmoothMotion << endl;
}
