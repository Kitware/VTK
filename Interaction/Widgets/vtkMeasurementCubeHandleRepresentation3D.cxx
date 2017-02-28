/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMeasurementCubeHandleRepresentation3D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkMeasurementCubeHandleRepresentation3D.h"
#include "vtkCubeSource.h"
#include "vtkCursor3D.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty2D.h"
#include "vtkActor.h"
#include "vtkCellPicker.h"
#include "vtkRenderer.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkObjectFactory.h"
#include "vtkPickingManager.h"
#include "vtkProperty.h"
#include "vtkAssemblyPath.h"
#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkInteractorObserver.h"
#include "vtkLine.h"
#include "vtkCoordinate.h"
#include "vtkRenderWindow.h"
#include "vtkFocalPlanePointPlacer.h"
#include "vtkCamera.h"
#include "vtkTransformPolyDataFilter.h"
#include "vtkMatrixToLinearTransform.h"
#include "vtkMatrix4x4.h"
#include "vtkBillboardTextActor3D.h"
#include "vtkTextProperty.h"
#include "vtkFollower.h"

#include <sstream>

vtkStandardNewMacro(vtkMeasurementCubeHandleRepresentation3D);
vtkCxxSetObjectMacro(vtkMeasurementCubeHandleRepresentation3D,Property,
                     vtkProperty);
vtkCxxSetObjectMacro(vtkMeasurementCubeHandleRepresentation3D,SelectedProperty,
                     vtkProperty);

//----------------------------------------------------------------------
vtkMeasurementCubeHandleRepresentation3D::vtkMeasurementCubeHandleRepresentation3D()
{
  this->InteractionState = vtkHandleRepresentation::Outside;

  this->HandleTransformFilter = vtkTransformPolyDataFilter::New();
  this->HandleTransform       = vtkMatrixToLinearTransform::New();
  this->HandleTransformMatrix = vtkMatrix4x4::New();
  this->HandleTransformMatrix->Identity();
  this->HandleTransform->SetInput( this->HandleTransformMatrix );
  this->HandleTransformFilter->SetTransform( this->HandleTransform );

  // initialized because it is used in PrintSelf
  this->LastPickPosition[0]=0.0;
  this->LastPickPosition[1]=0.0;
  this->LastPickPosition[2]=0.0;

  // initialized because it is used in PrintSelf
  this->LastEventPosition[0]=0.0;
  this->LastEventPosition[1]=0.0;

  this->Mapper = vtkPolyDataMapper::New();
  this->Mapper->ScalarVisibilityOff();
  this->Mapper->SetInputConnection(
    this->HandleTransformFilter->GetOutputPort());

  // Set up the initial properties
  this->CreateDefaultProperties();

  // Manage the picking stuff
  this->HandlePicker = vtkCellPicker::New();
  this->HandlePicker->PickFromListOn();
  this->HandlePicker->SetTolerance(0.01); //need some fluff

  this->Actor = vtkActor::New();
  this->Actor->SetMapper(this->Mapper);
  this->Actor->SetProperty(this->Property);
  this->Property->SetColor(.5,.5,.5);
  this->HandlePicker->AddPickList(this->Actor);

  // Override superclass
  this->PlaceFactor = 1.0;
  this->WaitingForMotion = 0;

  vtkFocalPlanePointPlacer *pointPlacer = vtkFocalPlanePointPlacer::New();
  this->SetPointPlacer( pointPlacer );
  pointPlacer->Delete();

  // Label stuff
  this->LabelAnnotationTextScaleInitialized = false;
  this->LabelVisibility = 1;
  this->SelectedLabelVisibility = 0;
  this->HandleVisibility = 1;
  this->LabelText = vtkBillboardTextActor3D::New();
  this->LabelText->SetVisibility(true);
  this->LabelText->GetTextProperty()->SetFontSize( 20 );
  this->LabelText->GetTextProperty()->SetColor( 1.0, 1.0, 1.0 );
  this->LabelText->GetTextProperty()->SetJustificationToCentered();
  this->LengthUnit = NULL;
  this->SetLengthUnit("unit");

  // Cube parameters
  this->AdaptiveScaling = 1;
  this->MinRelativeCubeScreenArea = .001; // .1 % of the total viewier window
  this->MaxRelativeCubeScreenArea = .02; // 2 % of the total viewier window
  this->RescaleFactor = 2; // volume changes by 8 on update
  this->SideLength = 1.; // without any other input, default to unit size

  this->SmoothMotion = 1;

  vtkNew<vtkCubeSource> cubeSource;
  cubeSource->Update();
  this->HandleTransformFilter->SetInputData( cubeSource->GetOutput() );
}

//----------------------------------------------------------------------
vtkMeasurementCubeHandleRepresentation3D
::~vtkMeasurementCubeHandleRepresentation3D()
{
  this->SetLengthUnit(NULL);
  this->HandleTransformFilter->Delete();
  this->HandleTransform->Delete();
  this->HandleTransformMatrix->Delete();
  this->HandlePicker->Delete();
  this->Mapper->Delete();
  this->Actor->Delete();
  this->Property->Delete();
  this->SelectedProperty->Delete();
  this->LabelText->Delete();
}

//----------------------------------------------------------------------
void vtkMeasurementCubeHandleRepresentation3D::RegisterPickers()
{
  this->Renderer->GetRenderWindow()->GetInteractor()->GetPickingManager()
    ->AddPicker(this->HandlePicker, this);
}

//----------------------------------------------------------------------
vtkPolyData * vtkMeasurementCubeHandleRepresentation3D::GetHandle()
{
  return vtkPolyData::SafeDownCast(this->HandleTransformFilter->GetInput());
}

//-------------------------------------------------------------------------
void vtkMeasurementCubeHandleRepresentation3D::SetWorldPosition(double p[3])
{
  if (!this->Renderer || !this->PointPlacer ||
                          this->PointPlacer->ValidateWorldPosition( p ))
  {
    this->HandleTransformMatrix->SetElement(0, 3, p[0]);
    this->HandleTransformMatrix->SetElement(1, 3, p[1]);
    this->HandleTransformMatrix->SetElement(2, 3, p[2]);

    this->WorldPosition->SetValue(
      this->HandleTransformMatrix->GetElement(0, 3),
      this->HandleTransformMatrix->GetElement(1, 3),
      this->HandleTransformMatrix->GetElement(2, 3));

    this->WorldPositionTime.Modified();
  }
}

//-------------------------------------------------------------------------
void vtkMeasurementCubeHandleRepresentation3D::SetDisplayPosition(double p[3])
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
int vtkMeasurementCubeHandleRepresentation3D
::ComputeInteractionState(int X, int Y, int vtkNotUsed(modify))
{
  this->VisibilityOn(); //actor must be on to be picked
  vtkAssemblyPath* path = this->GetAssemblyPath(X, Y, 0., this->HandlePicker);

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

//----------------------------------------------------------------------
// Record the current event position, and the rectilinear wipe position.
void vtkMeasurementCubeHandleRepresentation3D::StartWidgetInteraction(
  double startEventPos[2])
{
  this->StartEventPosition[0] = startEventPos[0];
  this->StartEventPosition[1] = startEventPos[1];
  this->StartEventPosition[2] = 0.0;

  this->LastEventPosition[0] = startEventPos[0];
  this->LastEventPosition[1] = startEventPos[1];

  vtkAssemblyPath* path = this->GetAssemblyPath(
    startEventPos[0], startEventPos[1], 0., this->HandlePicker);

  // Did we pick the handle ?
  if ( path )
  {
    this->InteractionState = vtkHandleRepresentation::Nearby;
    this->HandlePicker->GetPickPosition(this->LastPickPosition);
  }
  else
  {
    this->InteractionState = vtkHandleRepresentation::Outside;
  }
  this->WaitCount = 0;
}

//----------------------------------------------------------------------
// Based on the displacement vector (computed in display coordinates) and
// the cursor state (which corresponds to which part of the widget has been
// selected), the widget points are modified.
// First construct a local coordinate system based on the display coordinates
// of the widget.
void vtkMeasurementCubeHandleRepresentation3D::WidgetInteraction(
  double eventPos[2])
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

    vtkInteractorObserver::ComputeDisplayToWorld(
      this->Renderer,
      this->StartEventPosition[0],
      this->StartEventPosition[1], z, startPickPoint);

    if ( this->InteractionState == vtkHandleRepresentation::Selecting )
    {
      double newCenterPointRequested[3]; // displayPosition
      double newCenterPoint[3], worldOrient[9];

      // Make a request for the new position.
      this->MoveFocusRequest( prevPickPoint,
                              pickPoint,
                              eventPos,
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
    else
    {
      double newCenterPointRequested[3]; // displayPosition
      double newCenterPoint[3], worldOrient[9];

      // Make a request for the new position.
      this->MoveFocusRequest( prevPickPoint,
                              pickPoint,
                              eventPos,
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
void vtkMeasurementCubeHandleRepresentation3D
::MoveFocusRequest(double *p1, double *p2,
                   double currPos[2], double center[3])
{
  if (this->SmoothMotion)
  {
    double focus[4];
    this->GetWorldPosition(focus);

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
  else
  {
    center[0] = currPos[0];
    center[1] = currPos[1];
    center[2] = 1.0;
  }
}

//----------------------------------------------------------------------
void vtkMeasurementCubeHandleRepresentation3D::MoveFocus(double *p1, double *p2)
{
  //Get the motion vector
  double v[3];
  v[0] = p2[0] - p1[0];
  v[1] = p2[1] - p1[1];
  v[2] = p2[2] - p1[2];

  double focus[3];
  this->GetWorldPosition( focus );
  focus[0] += v[0];
  focus[1] += v[1];
  focus[2] += v[2];

  this->SetWorldPosition(focus);
}

//----------------------------------------------------------------------
// Translate everything
void vtkMeasurementCubeHandleRepresentation3D::Translate(double *p1, double *p2)
{
  //Get the motion vector
  double v[3], pos[3];
  v[0] = p2[0] - p1[0];
  v[1] = p2[1] - p1[1];
  v[2] = p2[2] - p1[2];

  this->GetWorldPosition( pos );
  double newFocus[3];
  int i;

  for (i=0; i<3; i++)
  {
    newFocus[i] = pos[i] + v[i];
  }

  this->SetWorldPosition(newFocus);
}

//----------------------------------------------------------------------
void vtkMeasurementCubeHandleRepresentation3D
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

  this->SetUniformScale( handleSize );
}

//----------------------------------------------------------------------
void vtkMeasurementCubeHandleRepresentation3D
::SetUniformScale(double handleSize)
{
  this->HandleTransformMatrix->SetElement(0, 0, handleSize);
  this->HandleTransformMatrix->SetElement(1, 1, handleSize);
  this->HandleTransformMatrix->SetElement(2, 2, handleSize);
}

//----------------------------------------------------------------------
void vtkMeasurementCubeHandleRepresentation3D::Highlight(int highlight)
{
  this->Actor->SetProperty(highlight ? this->SelectedProperty : this->Property);
  this->LabelText->SetVisibility(highlight ? this->SelectedLabelVisibility :
                                 this->LabelVisibility);
}

//----------------------------------------------------------------------
void vtkMeasurementCubeHandleRepresentation3D::CreateDefaultProperties()
{
  this->Property = vtkProperty::New();
  this->Property->SetLineWidth(0.5);

  this->SelectedProperty = vtkProperty::New();
  this->SelectedProperty->SetAmbient(1.0);
  this->SelectedProperty->SetAmbientColor(0.0,1.0,0.0);
  this->SelectedProperty->SetLineWidth(2.0);
}

//----------------------------------------------------------------------
void vtkMeasurementCubeHandleRepresentation3D::UpdateHandle()
{
  this->HandleTransformFilter->Update();
}

namespace
{
static const int OpposingDiagonals[4][2][3] = {{{0,2,4},{1,3,5}},
                                               {{1,2,4},{0,3,5}},
                                               {{0,3,4},{1,2,5}},
                                               {{0,2,5},{1,3,4}}};
}

//----------------------------------------------------------------------
void vtkMeasurementCubeHandleRepresentation3D::ScaleIfNecessary(
  vtkViewport* viewport)
{
  // Scaling is performed relative to the viewport window, so if there is no
  // window then there is nothing to do
  if (!viewport->GetVTKWindow())
  {
    return;
  }

  // A quick approximation of the cube's viewing area is computed using the
  // maximal distance on screen between the opposing diagonal points of the cube
  double bounds[6];
  this->Mapper->GetBounds(bounds);

  double displayMin[3], displayMax[3];

  double relativeArea = 0.;

  for (vtkIdType i=0;i<4;i++)
  {
    viewport->SetWorldPoint(bounds[OpposingDiagonals[i][0][0]],
                            bounds[OpposingDiagonals[i][0][1]],
                            bounds[OpposingDiagonals[i][0][2]], 1.);
    viewport->WorldToDisplay();
    viewport->GetDisplayPoint(displayMin);
    viewport->DisplayToNormalizedDisplay(displayMin[0],displayMin[1]);

    viewport->SetWorldPoint(bounds[OpposingDiagonals[i][1][0]],
                            bounds[OpposingDiagonals[i][1][1]],
                            bounds[OpposingDiagonals[i][1][2]], 1.);
    viewport->WorldToDisplay();
    viewport->GetDisplayPoint(displayMax);
    viewport->DisplayToNormalizedDisplay(displayMax[0],displayMax[1]);

    double relArea = fabs((displayMax[0] - displayMin[0]) *
                          (displayMax[1] - displayMin[1]));

    if (relArea > relativeArea)
    {
      relativeArea = relArea;
    }
  }

  // We rescale our cube using powers of our rescaling factor if it falls
  // outside of our bounds
  if (relativeArea > this->MaxRelativeCubeScreenArea)
  {
    int n = log(relativeArea/this->MaxRelativeCubeScreenArea);
    this->SideLength /= pow(this->RescaleFactor, n);
    this->SetUniformScale(this->SideLength);
    this->Modified();
  }
  else if (relativeArea < this->MinRelativeCubeScreenArea)
  {
    int n = ceil(log(this->MinRelativeCubeScreenArea/relativeArea));
    this->SideLength *= pow(this->RescaleFactor,n);
    this->SetUniformScale(this->SideLength);
    this->Modified();
  }
 }

//----------------------------------------------------------------------
void vtkMeasurementCubeHandleRepresentation3D::BuildRepresentation()
{
  // This method is called for two reasons: to prepare the geometry for
  // rendering and for bounds computation. In the former case, the renderer
  // pointer is set to a valid vtkRenderer, but in the latter case this may not
  // be so. Since the label requires the renderer to correctly compute its
  // offsets, we just skip the label update if the renderer is not set.

  if ( this->GetMTime() > this->BuildTime ||
       (this->Renderer && this->Renderer->GetVTKWindow() &&
        this->Renderer->GetVTKWindow()->GetMTime() > this->BuildTime) )
  {

    // Update the handle
    this->UpdateHandle();

    // Update the label
    this->UpdateLabel();

    this->BuildTime.Modified();
  }
}

//----------------------------------------------------------------------
void vtkMeasurementCubeHandleRepresentation3D::UpdateLabel()
{
  // Display the label if needed.
  if (this->LabelVisibility)
  {
    {
      std::stringstream s;
      s << "(" << this->SideLength << " " << std::string(this->LengthUnit)
        << std::string(")\xc2\xb3");
      this->LabelText->SetInput(s.str().c_str());
    }

    double labelPosition[3];
    this->GetWorldPosition(labelPosition);

    if (this->Renderer)
    {
      // Place the label in front of and below the cube. We need to take into
      // account the viewup vector and the direction of the camera.
      double vup[3], directionOfProjection[3], bounds[6];

      this->Renderer->GetActiveCamera()->GetViewUp(vup);
      this->Renderer->GetActiveCamera()->
        GetDirectionOfProjection(directionOfProjection);
      this->Mapper->GetBounds(bounds);
      double width = sqrt( (bounds[1] - bounds[0]) * (bounds[1] - bounds[0]) +
                           (bounds[3] - bounds[2]) * (bounds[3] - bounds[2]) +
                           (bounds[5] - bounds[4]) * (bounds[5] - bounds[4]) );

      for (int i=0; i<3; i++)
      {
        // place the label below the cube
        labelPosition[i] -= .33*width * vup[i];

        // place the label in front of the cube
        labelPosition[i] -= width/2.0 * directionOfProjection[i];
      }
    }
    else
    {
      // place the label in front of the cube, and guess that "in front" is in
      // the positive z-direction.
      labelPosition[2] += this->SideLength*.5;
    }

    this->LabelText->SetPosition(labelPosition);
  }
}

//----------------------------------------------------------------------
void vtkMeasurementCubeHandleRepresentation3D::ShallowCopy(vtkProp *prop)
{
  vtkMeasurementCubeHandleRepresentation3D *rep =
    vtkMeasurementCubeHandleRepresentation3D::SafeDownCast(prop);
  if ( rep )
  {
    this->SetProperty(rep->GetProperty());
    this->SetSelectedProperty(rep->GetSelectedProperty());
    this->Actor->SetProperty(this->Property);

    // copy the handle shape
    this->HandleTransformFilter->SetInputConnection(
      rep->HandleTransformFilter->GetInputConnection(0, 0));

    this->LabelVisibility = rep->LabelVisibility;
    this->SetLabelTextInput( rep->GetLabelTextInput() );
  }
  this->Superclass::ShallowCopy(prop);
}

//----------------------------------------------------------------------
void vtkMeasurementCubeHandleRepresentation3D::DeepCopy(vtkProp *prop)
{
  vtkMeasurementCubeHandleRepresentation3D *rep =
    vtkMeasurementCubeHandleRepresentation3D::SafeDownCast(prop);
  if ( rep )
  {
    this->Property->DeepCopy(rep->GetProperty());
    this->SelectedProperty->DeepCopy(rep->GetSelectedProperty());
    this->Actor->SetProperty(this->Property);

    // copy the handle shape
    vtkPolyData *pd = vtkPolyData::New();
    pd->DeepCopy( rep->HandleTransformFilter->GetInput() );
    this->HandleTransformFilter->SetInputData(pd);
    pd->Delete();

    this->LabelVisibility = rep->LabelVisibility;
    this->SetLabelTextInput( rep->GetLabelTextInput() );
  }
  this->Superclass::DeepCopy(prop);
}

//----------------------------------------------------------------------
void vtkMeasurementCubeHandleRepresentation3D::GetActors(vtkPropCollection *pc)
{
  this->Actor->GetActors(pc);
  this->LabelText->GetActors(pc);
}

//----------------------------------------------------------------------
void vtkMeasurementCubeHandleRepresentation3D::ReleaseGraphicsResources(
  vtkWindow *win)
{
  this->Actor->ReleaseGraphicsResources(win);
  this->LabelText->ReleaseGraphicsResources(win);
}

//----------------------------------------------------------------------
int vtkMeasurementCubeHandleRepresentation3D::RenderOpaqueGeometry(
  vtkViewport *viewport)
{
  int count=0;
  this->Renderer = vtkRenderer::SafeDownCast(viewport);
  this->BuildRepresentation();
  if (this->HandleVisibility)
  {
    if (this->AdaptiveScaling)
    {
      this->ScaleIfNecessary(viewport);
    }
    this->Actor->SetPropertyKeys(this->GetPropertyKeys());
    count += this->Actor->RenderOpaqueGeometry(viewport);
  }
  if (this->LabelVisibility)
  {
    this->LabelText->SetPropertyKeys(this->GetPropertyKeys());
    count += this->LabelText->RenderOpaqueGeometry(viewport);
  }
  return count;
}

//-----------------------------------------------------------------------------
int vtkMeasurementCubeHandleRepresentation3D::RenderTranslucentPolygonalGeometry(
  vtkViewport *viewport)
{
  int count=0;
  if (this->HandleVisibility)
  {
    this->Actor->SetPropertyKeys(this->GetPropertyKeys());
    count += this->Actor->RenderTranslucentPolygonalGeometry(viewport);
  }
  if (this->LabelVisibility)
  {
    this->LabelText->SetPropertyKeys(this->GetPropertyKeys());
    count += this->LabelText->RenderTranslucentPolygonalGeometry(viewport);
  }
  return count;
}

//-----------------------------------------------------------------------------
int vtkMeasurementCubeHandleRepresentation3D::HasTranslucentPolygonalGeometry()
{
  int result=0;
  this->BuildRepresentation();
  if (this->HandleVisibility)
  {
    result |= this->Actor->HasTranslucentPolygonalGeometry();
  }
  if (this->LabelVisibility)
  {
    result |= this->LabelText->HasTranslucentPolygonalGeometry();
  }
  return result;
}

//-----------------------------------------------------------------------------
double* vtkMeasurementCubeHandleRepresentation3D::GetBounds()
{
  this->BuildRepresentation();
  return this->Actor->GetBounds();
}

//-----------------------------------------------------------------------------
vtkAbstractTransform* vtkMeasurementCubeHandleRepresentation3D::GetTransform()
{
  return this->HandleTransform;
}

//------------------------------------------------------------------------------
void vtkMeasurementCubeHandleRepresentation3D::SetLabelTextInput(
  const char *s )
{
  this->LabelText->SetInput(s);
}

//------------------------------------------------------------------------------
char * vtkMeasurementCubeHandleRepresentation3D::GetLabelTextInput()
{
  return this->LabelText->GetInput();
}

//------------------------------------------------------------------------------
void vtkMeasurementCubeHandleRepresentation3D::SetSideLength(double d)
{
  if (this->SideLength != (d > 0. ? d : 0.))
  {
    this->SideLength = d;
    this->SetUniformScale(this->SideLength);
    this->Modified();
  }
}

//------------------------------------------------------------------------------
namespace
{
  const double RelativeCubeScreenAreaUpperLimit = 1.;
  const double RelativeCubeScreenAreaLowerLimit = 1.e-6;
}

//------------------------------------------------------------------------------
void vtkMeasurementCubeHandleRepresentation3D::SetMinRelativeCubeScreenArea(
  double d)
{
  if (this->MinRelativeCubeScreenArea !=
      (d < RelativeCubeScreenAreaLowerLimit ?
       RelativeCubeScreenAreaLowerLimit :
       (d > RelativeCubeScreenAreaUpperLimit ?
        RelativeCubeScreenAreaUpperLimit : d)))
  {
    this->MinRelativeCubeScreenArea = d;
    if (this->MaxRelativeCubeScreenArea <
        this->RescaleFactor*this->MinRelativeCubeScreenArea)
    {
      this->MaxRelativeCubeScreenArea =
        1.1*this->RescaleFactor*this->MinRelativeCubeScreenArea;
      if (this->MaxRelativeCubeScreenArea > RelativeCubeScreenAreaUpperLimit)
      {
        this->MaxRelativeCubeScreenArea = RelativeCubeScreenAreaUpperLimit;
        this->MinRelativeCubeScreenArea =
          .9*this->RescaleFactor*this->MaxRelativeCubeScreenArea;
      }
    }
    this->Modified();
  }
}

//------------------------------------------------------------------------------
void vtkMeasurementCubeHandleRepresentation3D::SetMaxRelativeCubeScreenArea(
  double d)
{
  if (this->MaxRelativeCubeScreenArea !=
      (d < RelativeCubeScreenAreaLowerLimit ?
       RelativeCubeScreenAreaLowerLimit :
       (d > RelativeCubeScreenAreaUpperLimit ?
        RelativeCubeScreenAreaUpperLimit : d)))
  {
    this->MaxRelativeCubeScreenArea = d;
    if (this->MaxRelativeCubeScreenArea <
        this->RescaleFactor*this->MinRelativeCubeScreenArea)
    {
      this->MinRelativeCubeScreenArea =
        .9*this->RescaleFactor*this->MaxRelativeCubeScreenArea;
      if (this->MinRelativeCubeScreenArea < RelativeCubeScreenAreaLowerLimit)
      {
        this->MinRelativeCubeScreenArea = RelativeCubeScreenAreaLowerLimit;
        this->MaxRelativeCubeScreenArea =
          1.1*this->RescaleFactor*this->MinRelativeCubeScreenArea;
      }
    }
    this->Modified();
  }
}

//----------------------------------------------------------------------
void vtkMeasurementCubeHandleRepresentation3D::PrintSelf(ostream& os,
                                                         vtkIndent indent)
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
  os << indent << "LabelVisibility: " << this->LabelVisibility << endl;
  os << indent << "HandleVisibility: " << this->HandleVisibility << endl;
  os << indent << "Actor: " << this->Actor << "\n";
  this->Actor->PrintSelf(os,indent.GetNextIndent());
  os << indent << "LabelText: " << this->LabelText << endl;
  this->LabelText->PrintSelf(os,indent.GetNextIndent());
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
  os << indent << "SmoothMotion: " << this->SmoothMotion << endl;
  os << indent << "AdaptiveScaling: "<<this->AdaptiveScaling << "\n";
  os << indent << "SideLength: "<<this->SideLength << "\n";
  os << indent << "LengthUnit: "<<this->LengthUnit << "\n";
}
