/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAffineRepresentation2D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkAffineRepresentation2D.h"
#include "vtkPolyDataMapper2D.h"
#include "vtkActor2D.h"
#include "vtkRenderer.h"
#include "vtkObjectFactory.h"
#include "vtkProperty2D.h"
#include "vtkTextProperty.h"
#include "vtkAssemblyPath.h"
#include "vtkMath.h"
#include "vtkInteractorObserver.h"
#include "vtkLine.h"
#include "vtkCoordinate.h"
#include "vtkGlyph2D.h"
#include "vtkCursor2D.h"
#include "vtkPolyDataAlgorithm.h"
#include "vtkPoints.h"
#include "vtkCellArray.h"
#include "vtkLeaderActor2D.h"
#include "vtkTransform.h"
#include "vtkTextMapper.h"
#include "vtkActor2D.h"
#include "vtkWindow.h"


vtkStandardNewMacro(vtkAffineRepresentation2D);

vtkCxxSetObjectMacro(vtkAffineRepresentation2D,Property,vtkProperty2D);
vtkCxxSetObjectMacro(vtkAffineRepresentation2D,SelectedProperty,vtkProperty2D);
vtkCxxSetObjectMacro(vtkAffineRepresentation2D,TextProperty,vtkTextProperty);

#define VTK_CIRCLE_RESOLUTION 64

//----------------------------------------------------------------------
vtkAffineRepresentation2D::vtkAffineRepresentation2D()
{
  // It's best to have a small tolerance
  this->Tolerance = 3;

  // Initialize state
  this->InteractionState = vtkAffineRepresentation::Outside;

  // The width of the widget
  this->DisplayText = 1;
  this->BoxWidth = 100;
  this->CircleWidth = static_cast<int>(0.75 * this->BoxWidth);
  this->AxesWidth = static_cast<int>(0.60 * this->BoxWidth);
  this->CurrentWidth = 0.0;
  this->CurrentRadius = 0.0;
  this->CurrentAxesWidth = 0.0;

  // Keep track of transformations
  this->DisplayOrigin[0] = this->DisplayOrigin[1] = this->DisplayOrigin[2] = 0.0;
  this->Origin[0] = this->Origin[1] = this->Origin[2] = 0.0;

  // Create properties
  this->CreateDefaultProperties();

  // Text label
  this->TextMapper = vtkTextMapper::New();
  this->TextMapper->SetTextProperty(this->TextProperty);
  this->TextMapper->SetInput("foo");
  this->TextActor = vtkActor2D::New();
  this->TextActor->SetMapper(this->TextMapper);
  this->TextActor->VisibilityOff();

  // Box
  this->BoxPoints = vtkPoints::New();
  this->BoxPoints->SetNumberOfPoints(4);
  this->BoxCellArray = vtkCellArray::New();
  this->BoxCellArray->EstimateSize(1,4);
  this->BoxCellArray->InsertNextCell(5);
  this->BoxCellArray->InsertCellPoint(0);
  this->BoxCellArray->InsertCellPoint(1);
  this->BoxCellArray->InsertCellPoint(2);
  this->BoxCellArray->InsertCellPoint(3);
  this->BoxCellArray->InsertCellPoint(0);
  this->Box = vtkPolyData::New();
  this->Box->SetPoints(this->BoxPoints);
  this->Box->SetLines(this->BoxCellArray);
  this->BoxMapper = vtkPolyDataMapper2D::New();
  this->BoxMapper->SetInputData(this->Box);
  this->BoxActor = vtkActor2D::New();
  this->BoxActor->SetMapper(this->BoxMapper);
  this->BoxActor->SetProperty(this->Property);

  this->HBoxPoints = vtkPoints::New();
  this->HBoxPoints->SetNumberOfPoints(4);
  this->HBoxCellArray = vtkCellArray::New();
  this->HBoxCellArray->EstimateSize(1,4);
  this->HBoxCellArray->InsertNextCell(5);
  this->HBoxCellArray->InsertCellPoint(0);
  this->HBoxCellArray->InsertCellPoint(1);
  this->HBoxCellArray->InsertCellPoint(2);
  this->HBoxCellArray->InsertCellPoint(3);
  this->HBoxCellArray->InsertCellPoint(0);
  this->HBox = vtkPolyData::New();
  this->HBox->SetPoints(this->HBoxPoints);
  this->HBox->SetLines(this->HBoxCellArray);
  this->HBoxMapper = vtkPolyDataMapper2D::New();
  this->HBoxMapper->SetInputData(this->HBox);
  this->HBoxActor = vtkActor2D::New();
  this->HBoxActor->SetMapper(this->HBoxMapper);
  this->HBoxActor->VisibilityOff();
  this->HBoxActor->SetProperty(this->SelectedProperty);

  // Circle
  this->CirclePoints = vtkPoints::New();
  this->CirclePoints->SetNumberOfPoints(VTK_CIRCLE_RESOLUTION);
  this->CircleCellArray = vtkCellArray::New();
  this->CircleCellArray->EstimateSize(1,VTK_CIRCLE_RESOLUTION+1);
  this->Circle = vtkPolyData::New();
  this->Circle->SetPoints(this->CirclePoints);
  this->Circle->SetLines(this->CircleCellArray);
  this->CircleMapper = vtkPolyDataMapper2D::New();
  this->CircleMapper->SetInputData(this->Circle);
  this->CircleActor = vtkActor2D::New();
  this->CircleActor->SetMapper(this->CircleMapper);
  this->CircleActor->SetProperty(this->Property);

  this->HCirclePoints = vtkPoints::New();
  this->HCircleCellArray = vtkCellArray::New();
  this->HCircleCellArray->EstimateSize(1,VTK_CIRCLE_RESOLUTION+1);
  this->HCircle = vtkPolyData::New();
  this->HCircle->SetPoints(this->HCirclePoints);
  this->HCircle->SetLines(this->HCircleCellArray);
  this->HCircleMapper = vtkPolyDataMapper2D::New();
  this->HCircleMapper->SetInputData(this->HCircle);
  this->HCircleActor = vtkActor2D::New();
  this->HCircleActor->SetMapper(this->HCircleMapper);
  this->HCircleActor->VisibilityOff();
  this->HCircleActor->SetProperty(this->SelectedProperty);

  // Translation axes
  this->XAxis = vtkLeaderActor2D::New();
  this->XAxis->GetPositionCoordinate()->SetCoordinateSystemToDisplay();
  this->XAxis->GetPosition2Coordinate()->SetCoordinateSystemToDisplay();
  this->XAxis->SetArrowStyleToFilled();
  this->XAxis->SetProperty(this->Property);
  this->XAxis->SetMaximumArrowSize(12);

  this->YAxis = vtkLeaderActor2D::New();
  this->YAxis->GetPositionCoordinate()->SetCoordinateSystemToDisplay();
  this->YAxis->GetPosition2Coordinate()->SetCoordinateSystemToDisplay();
  this->YAxis->SetArrowStyleToFilled();
  this->YAxis->SetProperty(this->Property);
  this->YAxis->SetMaximumArrowSize(12);

  this->HXAxis = vtkLeaderActor2D::New();
  this->HXAxis->GetPositionCoordinate()->SetCoordinateSystemToDisplay();
  this->HXAxis->GetPosition2Coordinate()->SetCoordinateSystemToDisplay();
  this->HXAxis->SetArrowStyleToFilled();
  this->HXAxis->SetProperty(this->SelectedProperty);
  this->HXAxis->SetMaximumArrowSize(12);
  this->HXAxis->VisibilityOff();

  this->HYAxis = vtkLeaderActor2D::New();
  this->HYAxis->GetPositionCoordinate()->SetCoordinateSystemToDisplay();
  this->HYAxis->GetPosition2Coordinate()->SetCoordinateSystemToDisplay();
  this->HYAxis->SetArrowStyleToFilled();
  this->HYAxis->SetProperty(this->SelectedProperty);
  this->HYAxis->SetMaximumArrowSize(12);
  this->HYAxis->VisibilityOff();

  // Transformation matrix
  this->CurrentTransform = vtkTransform::New();
  this->TotalTransform = vtkTransform::New();
  this->TempTransform = vtkTransform::New();

  this->CurrentTranslation[0] = 0.0;
  this->CurrentTranslation[1] = 0.0;
  this->CurrentTranslation[2] = 0.0;
  this->CurrentAngle = 0.0;
  this->CurrentScale[0] = 1.0;
  this->CurrentScale[1] = 1.0;
  this->CurrentShear[0] = 0.0;
  this->CurrentShear[1] = 0.0;
}

//----------------------------------------------------------------------
vtkAffineRepresentation2D::~vtkAffineRepresentation2D()
{
  this->Property->Delete();
  this->SelectedProperty->Delete();
  this->TextProperty->Delete();

  this->TextMapper->Delete();
  this->TextActor->Delete();

  this->BoxPoints->Delete();
  this->BoxCellArray->Delete();
  this->Box->Delete();
  this->BoxMapper->Delete();
  this->BoxActor->Delete();

  this->HBoxPoints->Delete();
  this->HBoxCellArray->Delete();
  this->HBox->Delete();
  this->HBoxMapper->Delete();
  this->HBoxActor->Delete();

  this->CirclePoints->Delete();
  this->CircleCellArray->Delete();
  this->Circle->Delete();
  this->CircleMapper->Delete();
  this->CircleActor->Delete();

  this->HCirclePoints->Delete();
  this->HCircleCellArray->Delete();
  this->HCircle->Delete();
  this->HCircleMapper->Delete();
  this->HCircleActor->Delete();

  this->XAxis->Delete();
  this->YAxis->Delete();
  this->HXAxis->Delete();
  this->HYAxis->Delete();

  this->CurrentTransform->Delete();
  this->TotalTransform->Delete();
  this->TempTransform->Delete();
}

//-------------------------------------------------------------------------
void vtkAffineRepresentation2D::GetTransform(vtkTransform *t)
{
  this->CurrentTransform->Identity();
  this->CurrentTransform->Translate(this->Origin[0],this->Origin[1],this->Origin[2]);
  if ( this->InteractionState != vtkAffineRepresentation::MoveOrigin &&
       this->InteractionState != vtkAffineRepresentation::MoveOriginX &&
       this->InteractionState != vtkAffineRepresentation::MoveOriginY )
    {
    this->CurrentTransform->Translate(this->CurrentTranslation[0],
                                      this->CurrentTranslation[1],
                                      this->CurrentTranslation[2]);
    }

  this->ApplyShear();
  this->CurrentTransform->RotateZ( vtkMath::DegreesFromRadians( this->CurrentAngle ) );
  this->CurrentTransform->Scale(this->CurrentScale[0], this->CurrentScale[1], 1.0);
  this->CurrentTransform->Translate(-this->Origin[0],-this->Origin[1],-this->Origin[2]);

  t->DeepCopy(this->CurrentTransform);
  t->Concatenate(this->TotalTransform);
}

//-------------------------------------------------------------------------
void vtkAffineRepresentation2D::PlaceWidget(double bounds[6])
{
  this->Origin[0] = (bounds[1] + bounds[0]) / 2.0;
  this->Origin[1] = (bounds[3] + bounds[2]) / 2.0;
  this->Origin[2] = (bounds[5] + bounds[4]) / 2.0;

  this->TotalTransform->Identity();
}

//-------------------------------------------------------------------------
void vtkAffineRepresentation2D::SetOrigin(double ox, double oy, double oz)
{
  if ( this->Origin[0] != ox || this->Origin[1] != oy ||
       this->Origin[2] != oz )
    {
    this->Origin[0] = ox;
    this->Origin[1] = oy;
    this->Origin[2] = oz;

    this->BuildRepresentation();
    this->Modified();
    }
}

//-------------------------------------------------------------------------
int vtkAffineRepresentation2D::ComputeInteractionState(int X, int Y, int modify)
{
  double p[3], tol=static_cast<double>(this->Tolerance);
  p[0] = static_cast<double>(X);
  p[1] = static_cast<double>(Y);
  p[2] = 0.0;
  this->InteractionState = vtkAffineRepresentation::Outside;

  // Box---------------------------------------------------------------
  double p1[3], p2[3], p3[3], p4[3];
  this->BoxPoints->GetPoint(0,p1); //min corner
  this->BoxPoints->GetPoint(2,p3); //max corner

  int e0 = (p[1] >= (p1[1] - tol) && p[1] <= (p1[1] + tol));
  int e1 = (p[0] >= (p3[0] - tol) && p[0] <= (p3[0] + tol));
  int e2 = (p[1] >= (p3[1] - tol) && p[1] <= (p3[1] + tol));
  int e3 = (p[0] >= (p1[0] - tol) && p[0] <= (p1[0] + tol));

  // Points
  if ( e0 && e1 )
    {
    this->InteractionState = vtkAffineRepresentation::ScaleSE;
    }
  else if ( e1 && e2 )
    {
    this->InteractionState = vtkAffineRepresentation::ScaleNE;
    }
  else if ( e2 && e3 )
    {
    this->InteractionState = vtkAffineRepresentation::ScaleNW;
    }
  else if ( e3 && e0 )
    {
    this->InteractionState = vtkAffineRepresentation::ScaleSW;
    }

  // Edges
  else if ( e0 )
    {
    if ( ! modify )
      {
      this->InteractionState = vtkAffineRepresentation::ScaleSEdge;
      }
    else
      {
      this->InteractionState = vtkAffineRepresentation::ShearSEdge;
      }
    }
  else if ( e1 )
    {
    if ( ! modify )
      {
      this->InteractionState = vtkAffineRepresentation::ScaleEEdge;
      }
    else
      {
      this->InteractionState = vtkAffineRepresentation::ShearEEdge;
      }
    }
  else if ( e2 )
    {
    if ( ! modify )
      {
      this->InteractionState = vtkAffineRepresentation::ScaleNEdge;
      }
    else
      {
      this->InteractionState = vtkAffineRepresentation::ShearNEdge;
      }
    }
  else if ( e3 )
    {
    if ( ! modify )
      {
      this->InteractionState = vtkAffineRepresentation::ScaleWEdge;
      }
    else
      {
      this->InteractionState = vtkAffineRepresentation::ShearWEdge;
      }
    }

  // Return if necessary
  if ( this->InteractionState != vtkAffineRepresentation::Outside )
    {
    return this->InteractionState;
    }

  // Circle---------------------------------------------------------------
  double radius = sqrt((p[0]-this->DisplayOrigin[0])*(p[0]-this->DisplayOrigin[0]) +
                       (p[1]-this->DisplayOrigin[1])*(p[1]-this->DisplayOrigin[1]));
  if ( radius >= (this->CurrentRadius - tol) &&
       radius <= (this->CurrentRadius + tol) )
    {
    this->InteractionState = vtkAffineRepresentation::Rotate;
    return this->InteractionState;
    }

  // Translation Arrows----------------------------------------------------
  this->XAxis->GetPositionCoordinate()->GetValue(p1);
  this->XAxis->GetPosition2Coordinate()->GetValue(p2);
  this->YAxis->GetPositionCoordinate()->GetValue(p3);
  this->YAxis->GetPosition2Coordinate()->GetValue(p4);

  e0 = (p[0] >= (p1[0] - tol) && p[0] <= (p2[0] + tol));
  e1 = (p[1] >= (p1[1] - tol) && p[1] <= (p1[1] + tol));
  e2 = (p[1] >= (p3[1] - tol) && p[1] <= (p4[1] + tol));
  e3 = (p[0] >= (p3[0] - tol) && p[0] <= (p3[0] + tol));

  if ( e0 && e1 && e2 && e3 )
    {
    if ( ! modify )
      {
      this->InteractionState = vtkAffineRepresentation::Translate;
      }
    else
      {
      this->InteractionState = vtkAffineRepresentation::MoveOrigin;
      }
    }
  else if ( e0 && e1 )
    {
    if ( ! modify )
      {
      this->InteractionState = vtkAffineRepresentation::TranslateX;
      }
    else
      {
      this->InteractionState = vtkAffineRepresentation::MoveOriginX;
      }
    }
  else if ( e2 && e3 )
    {
    if ( ! modify )
      {
      this->InteractionState = vtkAffineRepresentation::TranslateY;
      }
    else
      {
      this->InteractionState = vtkAffineRepresentation::MoveOriginY;
      }
    }

  return this->InteractionState;
}

//----------------------------------------------------------------------
// Record the current event position, and the rectilinear wipe position.
void vtkAffineRepresentation2D::StartWidgetInteraction(double startEventPos[2])
{
  // Initialize bookeeping variables
  this->StartEventPosition[0] = startEventPos[0];
  this->StartEventPosition[1] = startEventPos[1];
  this->StartEventPosition[2] = 0.0;
  vtkInteractorObserver::ComputeDisplayToWorld(this->Renderer,
                                               startEventPos[0], startEventPos[1], 0.0,
                                               this->StartWorldPosition);

  this->StartAngle = VTK_FLOAT_MAX;

  this->WidgetInteraction(startEventPos);
}


//----------------------------------------------------------------------
// Based on the displacement vector (computed in display coordinates) and
// the cursor state (which corresponds to which part of the widget has been
// selected), the widget points are modified.
// First construct a local coordinate system based on the display coordinates
// of the widget.
void vtkAffineRepresentation2D::WidgetInteraction(double eventPos[2])
{
  // Dispatch to the correct method
  switch (this->InteractionState)
    {
    case vtkAffineRepresentation::ShearWEdge: case vtkAffineRepresentation::ShearEEdge:
    case vtkAffineRepresentation::ShearNEdge: case vtkAffineRepresentation::ShearSEdge:
      this->Shear(eventPos);
      break;

    case vtkAffineRepresentation::ScaleNE: case vtkAffineRepresentation::ScaleSW:
    case vtkAffineRepresentation::ScaleNW: case vtkAffineRepresentation::ScaleSE:
    case vtkAffineRepresentation::ScaleNEdge: case vtkAffineRepresentation::ScaleSEdge:
    case vtkAffineRepresentation::ScaleWEdge: case vtkAffineRepresentation::ScaleEEdge:
      this->Scale(eventPos);
      break;

    case vtkAffineRepresentation::Rotate:
      this->Rotate(eventPos);
      break;

    case vtkAffineRepresentation::TranslateX: case vtkAffineRepresentation::TranslateY:
    case vtkAffineRepresentation::Translate:
    case vtkAffineRepresentation::MoveOriginX: case vtkAffineRepresentation::MoveOriginY:
    case vtkAffineRepresentation::MoveOrigin:
      this->Translate(eventPos);
      break;
    }

  // Book keeping
  this->LastEventPosition[0] = eventPos[0];
  this->LastEventPosition[1] = eventPos[1];

  this->Modified();
}

//----------------------------------------------------------------------
void vtkAffineRepresentation2D::EndWidgetInteraction(double vtkNotUsed(eventPos) [2])
{
  // Have to play games here because of the "pipelined" nature of the
  // transformations.
  this->GetTransform(this->TempTransform);
  this->TotalTransform->SetMatrix(this->TempTransform->GetMatrix());

  // Adjust the origin as necessary
  this->Origin[0] += this->CurrentTranslation[0];
  this->Origin[1] += this->CurrentTranslation[1];
  this->Origin[2] += this->CurrentTranslation[2];

  // Reset the current transformations
  this->CurrentTranslation[0] = 0.0;
  this->CurrentTranslation[1] = 0.0;
  this->CurrentTranslation[2] = 0.0;

  this->CurrentAngle = 0.0;

  this->CurrentScale[0] = 1.0;
  this->CurrentScale[1] = 1.0;

  this->CurrentShear[0] = 0.0;
  this->CurrentShear[1] = 0.0;
}

//----------------------------------------------------------------------
// Translate everything
void vtkAffineRepresentation2D::Translate(double eventPos[2])
{
  double x1[3], x2[3], y1[3], y2[3], dpos[3];
  dpos[0] = dpos[1] = dpos[2] = 0.0;

  this->XAxis->GetPositionCoordinate()->GetValue(x1);
  this->XAxis->GetPosition2Coordinate()->GetValue(x2);
  this->YAxis->GetPositionCoordinate()->GetValue(y1);
  this->YAxis->GetPosition2Coordinate()->GetValue(y2);

  switch (this->InteractionState)
    {
    case vtkAffineRepresentation::TranslateX:
    case vtkAffineRepresentation::MoveOriginX:
      dpos[0] = eventPos[0] - this->StartEventPosition[0];
      break;

    case vtkAffineRepresentation::TranslateY:
    case vtkAffineRepresentation::MoveOriginY:
      dpos[1] = eventPos[1] - this->StartEventPosition[1];
      break;

    case vtkAffineRepresentation::Translate:
    case vtkAffineRepresentation::MoveOrigin:
      dpos[0] = eventPos[0] - this->StartEventPosition[0];
      dpos[1] = eventPos[1] - this->StartEventPosition[1];
      break;
    }

  x1[0] += dpos[0]; x2[0] += dpos[0];
  y1[0] += dpos[0]; y2[0] += dpos[0];
  x1[1] += dpos[1]; x2[1] += dpos[1];
  y1[1] += dpos[1]; y2[1] += dpos[1];

  this->HXAxis->GetPositionCoordinate()->SetValue(x1);
  this->HXAxis->GetPosition2Coordinate()->SetValue(x2);
  this->HYAxis->GetPositionCoordinate()->SetValue(y1);
  this->HYAxis->GetPosition2Coordinate()->SetValue(y2);

  // Update the transform
  double wxyz[4];
  vtkInteractorObserver::ComputeDisplayToWorld(this->Renderer,
                                               this->StartEventPosition[0]+dpos[0],
                                               this->StartEventPosition[1]+dpos[1], 0.0,
                                               wxyz);

  this->CurrentTranslation[0] = wxyz[0]-this->StartWorldPosition[0];
  this->CurrentTranslation[1] = wxyz[1]-this->StartWorldPosition[1];
  this->CurrentTranslation[2] = wxyz[2]-this->StartWorldPosition[2];

  // Draw the text if necessary
  if ( this->DisplayText )
    {
    char str[256];
    sprintf(str,"(%0.2g, %0.2g)", this->CurrentTranslation[0], this->CurrentTranslation[1]);
    this->UpdateText(str,eventPos);
    }
}

//----------------------------------------------------------------------
void vtkAffineRepresentation2D::Scale(double eventPos[2])
{
  // Determine the relative motion
  double d[3];
  d[0] = eventPos[0] - this->StartEventPosition[0];
  d[1] = eventPos[1] - this->StartEventPosition[1];

  double x0[3], x1[3], x2[3], x3[3];
  double p0[3], p1[3], p2[3], p3[3];
  this->BoxPoints->GetPoint(0,x0);
  this->BoxPoints->GetPoint(1,x1);
  this->BoxPoints->GetPoint(2,x2);
  this->BoxPoints->GetPoint(3,x3);

  double xChange=0.0, yChange=0.0;
  switch (this->InteractionState)
    {
    case vtkAffineRepresentation::ScaleEEdge:
      xChange = 1.0;
      break;

    case vtkAffineRepresentation::ScaleWEdge:
      xChange = -1.0;
      break;

    case vtkAffineRepresentation::ScaleNEdge:
      yChange = 1.0;
      break;

    case vtkAffineRepresentation::ScaleSEdge:
      yChange = -1.0;
      break;

    case vtkAffineRepresentation::ScaleNE:
      xChange = 1.0;
      yChange = 1.0;
      break;

    case vtkAffineRepresentation::ScaleSW:
      xChange = -1.0;
      yChange = -1.0;
      break;

    case vtkAffineRepresentation::ScaleNW:
      xChange = -1.0;
      yChange =  1.0;
      break;

    case vtkAffineRepresentation::ScaleSE:
      xChange =  1.0;
      yChange = -1.0;
      break;
    }

  p0[0] = x0[0] - xChange*d[0];
  p1[0] = x1[0] + xChange*d[0];
  p2[0] = x2[0] + xChange*d[0];
  p3[0] = x3[0] - xChange*d[0];

  p0[1] = x0[1] - yChange*d[1];
  p1[1] = x1[1] - yChange*d[1];
  p2[1] = x2[1] + yChange*d[1];
  p3[1] = x3[1] + yChange*d[1];

  p0[2] = x0[2];
  p1[2] = x1[2];
  p2[2] = x2[2];
  p3[2] = x3[2];

  this->HBoxPoints->SetPoint(0,p0);
  this->HBoxPoints->SetPoint(1,p1);
  this->HBoxPoints->SetPoint(2,p2);
  this->HBoxPoints->SetPoint(3,p3);
  this->HBoxPoints->Modified();

  this->CurrentScale[0] = (p1[0]-p0[0]) / (x1[0]-x0[0]);
  this->CurrentScale[1] = (p2[1]-p1[1]) / (x2[1]-x1[1]);

  if ( this->DisplayText )
    {
    char str[256];
    sprintf(str,"(%0.2g, %0.2g)", this->CurrentScale[0], this->CurrentScale[1]);
    this->UpdateText(str,eventPos);
    }
}

//----------------------------------------------------------------------
void vtkAffineRepresentation2D::Rotate(double eventPos[2])
{
  double deltaAngle;
  // Compute the initial selection angle, and then the change in angle between
  // the starting point and subsequent points. The angle is constrained so that
  // it is in the range (-Pi < deltaAngle <= Pi).
  if ( this->StartAngle >= VTK_FLOAT_MAX )
    {
    double delX = this->StartEventPosition[0] - this->DisplayOrigin[0];
    double delY = this->StartEventPosition[1] - this->DisplayOrigin[1];
    this->StartAngle = atan2( delY, delX );
    deltaAngle = 0.0;
    }
  else
    {
    double delEX = eventPos[0] - this->DisplayOrigin[0];
    double delEY = eventPos[1] - this->DisplayOrigin[1];
    double angle2 = atan2(delEY,delEX);
    // Compute difference in angle
    deltaAngle = angle2 - this->StartAngle;
    if ( fabs(deltaAngle) > vtkMath::Pi() ) //angle always less than Pi
      {
      if ( deltaAngle > 0 )
        {
        deltaAngle = -2.0*vtkMath::Pi() + deltaAngle;
        }
      else
        {
        deltaAngle =  2.0*vtkMath::Pi() + deltaAngle;
        }
      }
    }

  // Update the angle
  this->CurrentAngle = deltaAngle;

  // Create the arc
  vtkIdType pid;
  this->HCirclePoints->Reset();
  this->HCircleCellArray->Reset();
  this->HCircleCellArray->InsertNextCell(0);
  double p[3]; p[2] = 0.0;
  double theta, delTheta = 2.0 * vtkMath::Pi() / VTK_CIRCLE_RESOLUTION;
  int numDivs = static_cast<int>(fabs(deltaAngle)/delTheta) + 1;
  delTheta = deltaAngle / numDivs;
  for ( int i=0;  i <= numDivs; i++ )
    {
    theta = this->StartAngle + i*delTheta;
    p[0] = this->DisplayOrigin[0] + this->CurrentRadius * cos(theta);
    p[1] = this->DisplayOrigin[1] + this->CurrentRadius * sin(theta);
    pid = this->HCirclePoints->InsertNextPoint(p);
    this->HCircleCellArray->InsertCellPoint(pid);
    }
  pid = this->HCirclePoints->InsertNextPoint(this->DisplayOrigin);
  this->HCircleCellArray->InsertCellPoint(pid);
  this->HCircleCellArray->InsertCellPoint(0);
  this->HCircleCellArray->UpdateCellCount(this->HCirclePoints->GetNumberOfPoints()+1);
  this->HCirclePoints->Modified();

  if ( this->DisplayText )
    {
    char str[256];
    double angle = vtkMath::DegreesFromRadians( deltaAngle );
    sprintf(str,"(%1.1f)", angle);
    this->UpdateText(str,eventPos);
    }
}

//----------------------------------------------------------------------
// Fiddle with matrix to apply shear
void vtkAffineRepresentation2D::ApplyShear()
{
}


//----------------------------------------------------------------------
void vtkAffineRepresentation2D::Shear(double eventPos[2])
{
  // Determine the relative motion
  double d[3];
  d[0] = eventPos[0] - this->StartEventPosition[0];
  d[1] = eventPos[1] - this->StartEventPosition[1];

  double x0[3], x1[3], x2[3], x3[3];
  double p0[3], p1[3], p2[3], p3[3];
  this->BoxPoints->GetPoint(0,x0);
  this->BoxPoints->GetPoint(1,x1);
  this->BoxPoints->GetPoint(2,x2);
  this->BoxPoints->GetPoint(3,x3);

  double xChange=0.0, yChange=0.0;
  switch (this->InteractionState)
    {
    case vtkAffineRepresentation::ShearSEdge:
      xChange = 1.0;
      break;

    case vtkAffineRepresentation::ShearNEdge:
      xChange = -1.0;
      break;

    case vtkAffineRepresentation::ShearEEdge:
      yChange = 1.0;
      break;

    case vtkAffineRepresentation::ShearWEdge:
      yChange = -1.0;
      break;
    }

  p0[0] = x0[0] + xChange*d[0];
  p1[0] = x1[0] + xChange*d[0];
  p2[0] = x2[0] - xChange*d[0];
  p3[0] = x3[0] - xChange*d[0];

  p0[1] = x0[1] - yChange*d[1];
  p1[1] = x1[1] + yChange*d[1];
  p2[1] = x2[1] + yChange*d[1];
  p3[1] = x3[1] - yChange*d[1];

  p0[2] = x0[2];
  p1[2] = x1[2];
  p2[2] = x2[2];
  p3[2] = x3[2];

  this->HBoxPoints->SetPoint(0,p0);
  this->HBoxPoints->SetPoint(1,p1);
  this->HBoxPoints->SetPoint(2,p2);
  this->HBoxPoints->SetPoint(3,p3);
  this->HBoxPoints->Modified();

  // Update the current shear
  double sx = (x2[1] - x1[1]) / 2.0;
  double sy = ((p0[0]-x0[0]) + (p0[1]-x0[1]));
  double angle = vtkMath::DegreesFromRadians( atan2(sy,sx) );
  if ( this->InteractionState == vtkAffineRepresentation::ShearNEdge ||
       this->InteractionState == vtkAffineRepresentation::ShearSEdge )
    {
    this->CurrentShear[0] = angle;
    }
  else
    {
    this->CurrentShear[1] = angle;
    }

  // Display text if requested
  if ( this->DisplayText )
    {
    char str[256];
    sprintf(str,"(%0.2g)", angle);
    this->UpdateText(str,eventPos);
    }
}

//----------------------------------------------------------------------
void vtkAffineRepresentation2D::Highlight(int highlight)
{
  if ( highlight ) //enable appropriate highlight actor
    {
    // Make the text visible
    if ( this->DisplayText )
      {
      this->TextActor->VisibilityOn();
      }

    // The existing widget is set translucent
    this->Opacity = this->Property->GetOpacity();
    this->Property->SetOpacity(0.33);
    this->SelectedOpacity = this->SelectedProperty->GetOpacity();
    this->SelectedProperty->SetOpacity(1.0);

    switch (this->InteractionState)
      {
      case vtkAffineRepresentation::ShearWEdge: case vtkAffineRepresentation::ShearEEdge:
      case vtkAffineRepresentation::ShearNEdge: case vtkAffineRepresentation::ShearSEdge:
      case vtkAffineRepresentation::ScaleNE: case vtkAffineRepresentation::ScaleSW:
      case vtkAffineRepresentation::ScaleNW: case vtkAffineRepresentation::ScaleSE:
      case vtkAffineRepresentation::ScaleNEdge: case vtkAffineRepresentation::ScaleSEdge:
      case vtkAffineRepresentation::ScaleWEdge: case vtkAffineRepresentation::ScaleEEdge:
        this->HBoxActor->VisibilityOn();
        break;

      case vtkAffineRepresentation::Rotate:
        this->HCircleActor->VisibilityOn();
        break;

      case vtkAffineRepresentation::TranslateX: case vtkAffineRepresentation::TranslateY:
      case vtkAffineRepresentation::Translate:
      case vtkAffineRepresentation::MoveOriginX: case vtkAffineRepresentation::MoveOriginY:
      case vtkAffineRepresentation::MoveOrigin:
        this->HXAxis->VisibilityOn();
        this->HYAxis->VisibilityOn();
        break;
      }
    }

  else // turn off highlight actor
    {
    this->TextActor->VisibilityOff();
    this->Property->SetOpacity(this->Opacity);
    this->SelectedProperty->SetOpacity(this->SelectedOpacity);
    this->HBoxActor->VisibilityOff();
    this->HCircleActor->VisibilityOff();
    this->HXAxis->VisibilityOff();
    this->HYAxis->VisibilityOff();
    }
}

//----------------------------------------------------------------------
void vtkAffineRepresentation2D::CreateDefaultProperties()
{
  this->Property = vtkProperty2D::New();
  this->Property->SetColor(0.0,1.0,0.0);
  this->Property->SetLineWidth(0.5);

  this->SelectedProperty = vtkProperty2D::New();
  this->SelectedProperty->SetColor(1.0,0.0,0.0);
  this->SelectedProperty->SetLineWidth(1.0);

  this->TextProperty = vtkTextProperty::New();
  this->TextProperty->SetFontSize(12);
  this->TextProperty->SetColor(1.0,0.0,0.0);
  this->TextProperty->SetBold(1);
  this->TextProperty->SetFontFamilyToArial();
  this->TextProperty->SetJustificationToLeft();
  this->TextProperty->SetVerticalJustificationToBottom();
}

//----------------------------------------------------------------------
void vtkAffineRepresentation2D::UpdateText(const char *text, double eventPos[2])
{
  this->TextMapper->SetInput(text);
  this->TextActor->SetPosition(eventPos[0]+7, eventPos[1]+7);
}


//----------------------------------------------------------------------
void vtkAffineRepresentation2D::BuildRepresentation()
{
  if ( this->GetMTime() > this->BuildTime ||
       (this->Renderer && this->Renderer->GetVTKWindow() &&
        this->Renderer->GetVTKWindow()->GetMTime() > this->BuildTime) )
    {
    // Determine where the origin is on the display
    vtkInteractorObserver::ComputeWorldToDisplay(this->Renderer, this->Origin[0], this->Origin[1],
                                                 this->Origin[2], this->DisplayOrigin);

    // draw the box
    this->CurrentWidth = this->BoxWidth;
    this->CurrentWidth /= 2.0;
    double p1[3], p2[3],p3[3], p4[3];
    p1[0] = this->DisplayOrigin[0] - this->CurrentWidth;
    p1[1] = this->DisplayOrigin[1] - this->CurrentWidth;
    p1[2] = 0.0;
    p2[0] = this->DisplayOrigin[0] + this->CurrentWidth;
    p2[1] = this->DisplayOrigin[1] - this->CurrentWidth;
    p2[2] = 0.0;
    p3[0] = this->DisplayOrigin[0] + this->CurrentWidth;
    p3[1] = this->DisplayOrigin[1] + this->CurrentWidth;
    p3[2] = 0.0;
    p4[0] = this->DisplayOrigin[0] - this->CurrentWidth;
    p4[1] = this->DisplayOrigin[1] + this->CurrentWidth;
    p4[2] = 0.0;
    this->BoxPoints->SetPoint(0,p1);
    this->BoxPoints->SetPoint(1,p2);
    this->BoxPoints->SetPoint(2,p3);
    this->BoxPoints->SetPoint(3,p4);
    this->BoxPoints->Modified();

    // draw the circle
    int i;
    double theta, delTheta = 2.0 * vtkMath::Pi() / VTK_CIRCLE_RESOLUTION;
    this->CurrentRadius = this->CurrentWidth * 0.75;
    this->CircleCellArray->InsertNextCell(VTK_CIRCLE_RESOLUTION+1);
    for (i=0; i<VTK_CIRCLE_RESOLUTION; i++)
      {
      theta = i * delTheta;
      p1[0] = this->DisplayOrigin[0] + this->CurrentRadius * cos(theta);
      p1[1] = this->DisplayOrigin[1] + this->CurrentRadius * sin(theta);
      this->CirclePoints->SetPoint(i,p1);
      this->CircleCellArray->InsertCellPoint(i);
      }
    this->CircleCellArray->InsertCellPoint(0);

    // draw the translation axes
    this->CurrentAxesWidth = this->CurrentWidth * this->AxesWidth/this->BoxWidth;
    p1[0] = this->DisplayOrigin[0] - this->CurrentAxesWidth;
    p1[1] = this->DisplayOrigin[1];
    this->XAxis->GetPositionCoordinate()->SetValue(p1);
    p2[0] = this->DisplayOrigin[0] + this->CurrentAxesWidth;
    p2[1] = this->DisplayOrigin[1];
    this->XAxis->GetPosition2Coordinate()->SetValue(p2);

    p1[0] = this->DisplayOrigin[0];
    p1[1] = this->DisplayOrigin[1] - this->CurrentAxesWidth;;
    this->YAxis->GetPositionCoordinate()->SetValue(p1);
    p2[0] = this->DisplayOrigin[0];
    p2[1] = this->DisplayOrigin[1] + this->CurrentAxesWidth;
    this->YAxis->GetPosition2Coordinate()->SetValue(p2);

    this->BuildTime.Modified();
    }
}

//----------------------------------------------------------------------
void vtkAffineRepresentation2D::ShallowCopy(vtkProp *prop)
{
  vtkAffineRepresentation2D *rep =
    vtkAffineRepresentation2D::SafeDownCast(prop);
  if ( rep )
    {
    this->SetProperty(rep->GetProperty());
    this->SetSelectedProperty(rep->GetSelectedProperty());
    this->SetTextProperty(rep->GetTextProperty());
    this->BoxActor->SetProperty(this->Property);
    this->HBoxActor->SetProperty(this->SelectedProperty);
    this->CircleActor->SetProperty(this->Property);
    this->HCircleActor->SetProperty(this->SelectedProperty);
    this->XAxis->SetProperty(this->Property);
    this->YAxis->SetProperty(this->Property);
    this->HXAxis->SetProperty(this->SelectedProperty);
    this->HYAxis->SetProperty(this->SelectedProperty);
    }
  this->Superclass::ShallowCopy(prop);
}

//----------------------------------------------------------------------
void vtkAffineRepresentation2D::GetActors2D(vtkPropCollection *pc)
{
  this->BoxActor->GetActors2D(pc);
  this->HBoxActor->GetActors2D(pc);
  this->CircleActor->GetActors2D(pc);
  this->HCircleActor->GetActors2D(pc);
  this->XAxis->GetActors2D(pc);
  this->YAxis->GetActors2D(pc);
  this->HXAxis->GetActors2D(pc);
  this->HYAxis->GetActors2D(pc);
}

//----------------------------------------------------------------------
void vtkAffineRepresentation2D::ReleaseGraphicsResources(vtkWindow *win)
{
  this->TextActor->ReleaseGraphicsResources(win);
  this->BoxActor->ReleaseGraphicsResources(win);
  this->HBoxActor->ReleaseGraphicsResources(win);
  this->CircleActor->ReleaseGraphicsResources(win);
  this->HCircleActor->ReleaseGraphicsResources(win);
  this->XAxis->ReleaseGraphicsResources(win);
  this->YAxis->ReleaseGraphicsResources(win);
  this->HXAxis->ReleaseGraphicsResources(win);
  this->HYAxis->ReleaseGraphicsResources(win);
}

//----------------------------------------------------------------------
int vtkAffineRepresentation2D::RenderOverlay(vtkViewport *viewport)
{
  this->BuildRepresentation();

  int count = 0;
  if ( this->TextActor->GetVisibility() )
    {
    count += this->TextActor->RenderOverlay(viewport);
    }

  count += this->BoxActor->RenderOverlay(viewport);
  if ( this->HBoxActor->GetVisibility() )
    {
    count += this->HBoxActor->RenderOverlay(viewport);
    }

  count += this->CircleActor->RenderOverlay(viewport);
  if ( this->HCircleActor->GetVisibility() )
    {
    count += this->HCircleActor->RenderOverlay(viewport);
    }

  count += this->XAxis->RenderOverlay(viewport);
  count += this->YAxis->RenderOverlay(viewport);
  if ( this->HXAxis->GetVisibility() )
    {
    count += this->HXAxis->RenderOverlay(viewport);
    }
  if ( this->HYAxis->GetVisibility() )
    {
    count += this->HYAxis->RenderOverlay(viewport);
    }

  return count;
}


//----------------------------------------------------------------------
void vtkAffineRepresentation2D::PrintSelf(ostream& os, vtkIndent indent)
{
  //Superclass typedef defined in vtkTypeMacro() found in vtkSetGet.h
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Display Text: " << (this->DisplayText ? "On\n" : "Off\n");

  os << indent << "Origin: (" << this->Origin[0] << ","
     << this->Origin[1] << "," << this->Origin[2] << ")\n";
  os << indent << "Box Width: " << this->BoxWidth << "\n";
  os << indent << "Circle Width: " << this->CircleWidth << "\n";
  os << indent << "Axes Width: " << this->AxesWidth << "\n";

  if ( this->TextProperty )
    {
    os << indent << "Text Property:\n";
    this->TextProperty->PrintSelf(os,indent.GetNextIndent());
    }
  else
    {
    os << indent << "Property: (none)\n";
    }

  if ( this->Property )
    {
    os << indent << "Property:\n";
    this->Property->PrintSelf(os,indent.GetNextIndent());
    }
  else
    {
    os << indent << "Property: (none)\n";
    }

  if ( this->SelectedProperty )
    {
    os << indent << "Selected Property:\n";
    this->SelectedProperty->PrintSelf(os,indent.GetNextIndent());
    }
  else
    {
    os << indent << "Selected Property: (none)\n";
    }

  if ( this->TextProperty )
    {
    os << indent << "Text Property:\n";
    this->TextProperty->PrintSelf(os,indent.GetNextIndent());
    }
  else
    {
    os << indent << "Text Property: (none)\n";
    }

}
