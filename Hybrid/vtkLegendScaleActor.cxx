/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLegendScaleActor.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkLegendScaleActor.h"
#include "vtkAxisActor2D.h"
#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkMath.h"
#include "vtkCamera.h"
#include "vtkRenderer.h"
#include "vtkWindow.h"
#include "vtkPolyData.h"
#include "vtkPoints.h"
#include "vtkCellArray.h"
#include "vtkUnsignedCharArray.h"
#include "vtkActor2D.h"
#include "vtkPolyDataMapper2D.h"
#include "vtkTextMapper.h"
#include "vtkTextProperty.h"
#include "vtkCellData.h"
#include "vtkCoordinate.h"

vtkStandardNewMacro(vtkLegendScaleActor);


//----------------------------------------------------------------------
vtkLegendScaleActor::vtkLegendScaleActor()
{
  this->LabelMode = DISTANCE;

  this->RightBorderOffset = 50;
  this->TopBorderOffset = 30;
  this->LeftBorderOffset = 50;
  this->BottomBorderOffset = 30;
  this->CornerOffsetFactor = 2.0;
  
  this->RightAxis = vtkAxisActor2D::New();
  this->RightAxis->GetPositionCoordinate()->SetCoordinateSystemToViewport();
  this->RightAxis->GetPosition2Coordinate()->SetCoordinateSystemToViewport();
  this->RightAxis->GetPositionCoordinate()->SetReferenceCoordinate(NULL);
  this->RightAxis->SetFontFactor(0.6);
  this->RightAxis->SetNumberOfLabels(5);
  this->RightAxis->AdjustLabelsOff();

  this->TopAxis = vtkAxisActor2D::New();
  this->TopAxis->GetPositionCoordinate()->SetCoordinateSystemToViewport();
  this->TopAxis->GetPosition2Coordinate()->SetCoordinateSystemToViewport();
  this->TopAxis->GetPositionCoordinate()->SetReferenceCoordinate(NULL);
  this->TopAxis->SetFontFactor(0.6);
  this->TopAxis->SetNumberOfLabels(5);
  this->TopAxis->AdjustLabelsOff();

  this->LeftAxis = vtkAxisActor2D::New();
  this->LeftAxis->GetPositionCoordinate()->SetCoordinateSystemToViewport();
  this->LeftAxis->GetPosition2Coordinate()->SetCoordinateSystemToViewport();
  this->LeftAxis->GetPositionCoordinate()->SetReferenceCoordinate(NULL);
  this->LeftAxis->SetFontFactor(0.6);
  this->LeftAxis->SetNumberOfLabels(5);
  this->LeftAxis->AdjustLabelsOff();

  this->BottomAxis = vtkAxisActor2D::New();
  this->BottomAxis->GetPositionCoordinate()->SetCoordinateSystemToViewport();
  this->BottomAxis->GetPosition2Coordinate()->SetCoordinateSystemToViewport();
  this->BottomAxis->GetPositionCoordinate()->SetReferenceCoordinate(NULL);
  this->BottomAxis->SetFontFactor(0.6);
  this->BottomAxis->SetNumberOfLabels(5);
  this->BottomAxis->AdjustLabelsOff();

  this->RightAxisVisibility = 1;
  this->TopAxisVisibility = 1;
  this->LeftAxisVisibility = 1;
  this->BottomAxisVisibility = 1;

  this->LegendVisibility = 1;
  this->Legend = vtkPolyData::New();
  this->LegendPoints = vtkPoints::New();
  this->Legend->SetPoints(this->LegendPoints);
  this->LegendMapper = vtkPolyDataMapper2D::New();
  this->LegendMapper->SetInput(this->Legend);
  this->LegendActor = vtkActor2D::New();
  this->LegendActor->SetMapper(this->LegendMapper);
  
  // Create the legend
  vtkIdType pts[4];
  this->LegendPoints->SetNumberOfPoints(10);
  vtkCellArray *legendPolys = vtkCellArray::New();
  legendPolys->Allocate(legendPolys->EstimateSize(4,4));
  pts[0] = 0; pts[1] = 1; pts[2] = 6; pts[3] = 5;
  legendPolys->InsertNextCell(4,pts);
  pts[0] = 1; pts[1] = 2; pts[2] = 7; pts[3] = 6;
  legendPolys->InsertNextCell(4,pts);
  pts[0] = 2; pts[1] = 3; pts[2] = 8; pts[3] = 7;
  legendPolys->InsertNextCell(4,pts);
  pts[0] = 3; pts[1] = 4; pts[2] = 9; pts[3] = 8;
  legendPolys->InsertNextCell(4,pts);
  this->Legend->SetPolys(legendPolys);
  legendPolys->Delete();

  // Create the cell data
  vtkUnsignedCharArray *colors = vtkUnsignedCharArray::New();
  colors->SetNumberOfComponents(3);
  colors->SetNumberOfTuples(4);
  colors->SetTuple3(0,0,0,0);
  colors->SetTuple3(1,255,255,255);
  colors->SetTuple3(2,0,0,0);
  colors->SetTuple3(3,255,255,255);
  this->Legend->GetCellData()->SetScalars(colors);
  colors->Delete();
  
  // Now the text. The first five are for the 0,1/4,1/2,3/4,1 labels.
  this->LegendTitleProperty = vtkTextProperty::New();
  this->LegendTitleProperty->SetJustificationToCentered();
  this->LegendTitleProperty->SetVerticalJustificationToBottom();
  this->LegendTitleProperty->SetBold(1);
  this->LegendTitleProperty->SetItalic(1);
  this->LegendTitleProperty->SetShadow(1);
  this->LegendTitleProperty->SetFontFamilyToArial();
  this->LegendTitleProperty->SetFontSize(10);
  this->LegendLabelProperty = vtkTextProperty::New();
  this->LegendLabelProperty->SetJustificationToCentered();
  this->LegendLabelProperty->SetVerticalJustificationToTop();
  this->LegendLabelProperty->SetBold(1);
  this->LegendLabelProperty->SetItalic(1);
  this->LegendLabelProperty->SetShadow(1);
  this->LegendLabelProperty->SetFontFamilyToArial();
  this->LegendLabelProperty->SetFontSize(8);
  for (int i=0; i<6; i++)
    {
    this->LabelMappers[i] = vtkTextMapper::New();
    this->LabelMappers[i]->SetTextProperty(this->LegendLabelProperty);
    this->LabelActors[i] = vtkActor2D::New();
    this->LabelActors[i]->SetMapper(this->LabelMappers[i]);
    }
  this->LabelMappers[5]->SetTextProperty(this->LegendTitleProperty);
  this->LabelMappers[0]->SetInput("0");
  this->LabelMappers[1]->SetInput("1/4");
  this->LabelMappers[2]->SetInput("1/2");
  this->LabelMappers[3]->SetInput("3/4");
  this->LabelMappers[4]->SetInput("1");
  
  this->Coordinate = vtkCoordinate::New();
  this->Coordinate->SetCoordinateSystemToDisplay();
}

//----------------------------------------------------------------------
vtkLegendScaleActor::~vtkLegendScaleActor()
{
  this->RightAxis->Delete();
  this->TopAxis->Delete();
  this->LeftAxis->Delete();
  this->BottomAxis->Delete();
  
  this->Legend->Delete();
  this->LegendPoints->Delete();
  this->LegendMapper->Delete();
  this->LegendActor->Delete();
  
  for (int i=0; i<6; i++)
    {
    this->LabelMappers[i]->Delete();
    this->LabelActors[i]->Delete();
    }
  this->LegendTitleProperty->Delete();
  this->LegendLabelProperty->Delete();
  this->Coordinate->Delete();
}

//----------------------------------------------------------------------
void vtkLegendScaleActor::GetActors2D(vtkPropCollection *pc)
{
  pc->AddItem(this->RightAxis);
  pc->AddItem(this->TopAxis);
  pc->AddItem(this->LeftAxis);
  pc->AddItem(this->BottomAxis);
}

//----------------------------------------------------------------------
void vtkLegendScaleActor::ReleaseGraphicsResources(vtkWindow *w)
{
  this->RightAxis->ReleaseGraphicsResources(w);
  this->TopAxis->ReleaseGraphicsResources(w);
  this->LeftAxis->ReleaseGraphicsResources(w);
  this->BottomAxis->ReleaseGraphicsResources(w);
}

//----------------------------------------------------------------------
int vtkLegendScaleActor::RenderOpaqueGeometry(vtkViewport *viewport)
{
  this->BuildRepresentation(viewport);
  
  int renderedSomething=0;
  if ( this->RightAxisVisibility )
    {
    renderedSomething = this->RightAxis->RenderOpaqueGeometry(viewport);
    }
  if ( this->TopAxisVisibility )
    {
    renderedSomething += this->TopAxis->RenderOpaqueGeometry(viewport);
    }
  if ( this->LeftAxisVisibility )
    {
    renderedSomething += this->LeftAxis->RenderOpaqueGeometry(viewport);
    }
  if ( this->BottomAxisVisibility )
    {
    renderedSomething += this->BottomAxis->RenderOpaqueGeometry(viewport);
    }
  if ( this->LegendVisibility )
    {
    renderedSomething += this->LegendActor->RenderOpaqueGeometry(viewport);
    renderedSomething += this->LabelActors[0]->RenderOpaqueGeometry(viewport);
    renderedSomething += this->LabelActors[1]->RenderOpaqueGeometry(viewport);
    renderedSomething += this->LabelActors[2]->RenderOpaqueGeometry(viewport);
    renderedSomething += this->LabelActors[3]->RenderOpaqueGeometry(viewport);
    renderedSomething += this->LabelActors[4]->RenderOpaqueGeometry(viewport);
    renderedSomething += this->LabelActors[5]->RenderOpaqueGeometry(viewport);
    }

  return renderedSomething;
}

//----------------------------------------------------------------------
int vtkLegendScaleActor::RenderOverlay(vtkViewport *viewport)
{
  int renderedSomething=0;
  if ( this->RightAxisVisibility )
    {
    renderedSomething = this->RightAxis->RenderOverlay(viewport);
    }
  if ( this->TopAxisVisibility )
    {
    renderedSomething += this->TopAxis->RenderOverlay(viewport);
    }
  if ( this->LeftAxisVisibility )
    {
    renderedSomething += this->LeftAxis->RenderOverlay(viewport);
    }
  if ( this->BottomAxisVisibility )
    {
    renderedSomething += this->BottomAxis->RenderOverlay(viewport);
    }
  if ( this->LegendVisibility )
    {
    renderedSomething += this->LegendActor->RenderOverlay(viewport);
    renderedSomething += this->LabelActors[0]->RenderOverlay(viewport);
    renderedSomething += this->LabelActors[1]->RenderOverlay(viewport);
    renderedSomething += this->LabelActors[2]->RenderOverlay(viewport);
    renderedSomething += this->LabelActors[3]->RenderOverlay(viewport);
    renderedSomething += this->LabelActors[4]->RenderOverlay(viewport);
    renderedSomething += this->LabelActors[5]->RenderOverlay(viewport);
    }

  return renderedSomething;
}

//----------------------------------------------------------------------
void vtkLegendScaleActor::BuildRepresentation(vtkViewport *viewport)
{
  if ( 1 ) //it's probably best just to rerender every time
//   if ( this->GetMTime() > this->BuildTime || 
//        (this->Renderer && this->Renderer->GetVTKWindow() &&
//         this->Renderer->GetVTKWindow()->GetMTime() > this->BuildTime) )
    {
    // Specify the locations of the axes.
    int *size = viewport->GetSize();
    
    this->RightAxis->GetPositionCoordinate()->
      SetValue(size[0]-this->RightBorderOffset,
        this->CornerOffsetFactor*this->BottomBorderOffset,0.0);
    this->RightAxis->GetPosition2Coordinate()->
      SetValue(size[0]-this->RightBorderOffset,
        size[1]-this->CornerOffsetFactor*this->TopBorderOffset,0.0);

    this->TopAxis->GetPositionCoordinate()->
      SetValue(size[0]-this->CornerOffsetFactor*this->RightBorderOffset,
        size[1]-this->TopBorderOffset,0.0);
    this->TopAxis->GetPosition2Coordinate()->
      SetValue(this->CornerOffsetFactor*this->LeftBorderOffset,
        size[1]-this->TopBorderOffset,0.0);

    this->LeftAxis->GetPositionCoordinate()->
      SetValue(this->LeftBorderOffset,
        size[1]-this->CornerOffsetFactor*this->TopBorderOffset,0.0);
    this->LeftAxis->GetPosition2Coordinate()->
      SetValue(this->LeftBorderOffset,
        this->CornerOffsetFactor*this->BottomBorderOffset,0.0);

    if ( this->LegendVisibility )
      {
      this->BottomAxis->GetPositionCoordinate()->
        SetValue(this->CornerOffsetFactor*this->LeftBorderOffset,
          2*this->BottomBorderOffset,0.0);
      this->BottomAxis->GetPosition2Coordinate()->
        SetValue(size[0]-this->CornerOffsetFactor*this->RightBorderOffset,
          2*this->BottomBorderOffset,0.0);
      }
    else
      {
      this->BottomAxis->GetPositionCoordinate()->
        SetValue(this->CornerOffsetFactor*this->LeftBorderOffset,
          this->BottomBorderOffset,0.0);
      this->BottomAxis->GetPosition2Coordinate()->
        SetValue(size[0]-this->CornerOffsetFactor*this->RightBorderOffset,
          this->BottomBorderOffset,0.0);
      }
    

    // Now specify the axis values
    if ( this->LabelMode == XY_COORDINATES )
      {
      double *xL = this->RightAxis->GetPositionCoordinate()->
        GetComputedWorldValue(viewport);
      double *xR = this->RightAxis->GetPosition2Coordinate()->
        GetComputedWorldValue(viewport);
      this->RightAxis->SetRange(xL[1],xR[1]);

      xL = this->TopAxis->GetPositionCoordinate()->
        GetComputedWorldValue(viewport);
      xR = this->TopAxis->GetPosition2Coordinate()->
        GetComputedWorldValue(viewport);
      this->TopAxis->SetRange(xL[0],xR[0]);

      xL = this->LeftAxis->GetPositionCoordinate()->
        GetComputedWorldValue(viewport);
      xR = this->LeftAxis->GetPosition2Coordinate()->
        GetComputedWorldValue(viewport);
      this->LeftAxis->SetRange(xL[1],xR[1]);

      xL = this->BottomAxis->GetPositionCoordinate()->
        GetComputedWorldValue(viewport);
      xR = this->BottomAxis->GetPosition2Coordinate()->
        GetComputedWorldValue(viewport);
      this->BottomAxis->SetRange(xL[0],xR[0]);
      }
    else //distance between points
      {
      double d;

      double *xL = this->RightAxis->GetPositionCoordinate()->
        GetComputedWorldValue(viewport);
      double *xR = this->RightAxis->GetPosition2Coordinate()->
        GetComputedWorldValue(viewport);
      d = sqrt (vtkMath::Distance2BetweenPoints(xL,xR));
      this->RightAxis->SetRange(-d/2.0,d/2.0);

      xL = this->TopAxis->GetPositionCoordinate()->
        GetComputedWorldValue(viewport);
      xR = this->TopAxis->GetPosition2Coordinate()->
        GetComputedWorldValue(viewport);
      d = sqrt (vtkMath::Distance2BetweenPoints(xL,xR));
      this->TopAxis->SetRange(d/2.0,-d/2.0);

      xL = this->LeftAxis->GetPositionCoordinate()->
        GetComputedWorldValue(viewport);
      xR = this->LeftAxis->GetPosition2Coordinate()->
        GetComputedWorldValue(viewport);
      d = sqrt (vtkMath::Distance2BetweenPoints(xL,xR));
      this->LeftAxis->SetRange(d/2.0,-d/2.0);

      xL = this->BottomAxis->GetPositionCoordinate()->
        GetComputedWorldValue(viewport);
      xR = this->BottomAxis->GetPosition2Coordinate()->
        GetComputedWorldValue(viewport);
      d = sqrt (vtkMath::Distance2BetweenPoints(xL,xR));
      this->BottomAxis->SetRange(-d/2.0,d/2.0);
      }
    
    if ( this->LegendVisibility )
      {
      // Update the position
      double x1 = 0.33333*size[0];
      double delX = x1/4.0;

      this->LegendPoints->SetPoint(0, x1,10,0);
      this->LegendPoints->SetPoint(1, x1+delX,10,0);
      this->LegendPoints->SetPoint(2, x1+2*delX,10,0);
      this->LegendPoints->SetPoint(3, x1+3*delX,10,0);
      this->LegendPoints->SetPoint(4, x1+4*delX,10,0);
      this->LegendPoints->SetPoint(5, x1,20,0);
      this->LegendPoints->SetPoint(6, x1+delX,20,0);
      this->LegendPoints->SetPoint(7, x1+2*delX,20,0);
      this->LegendPoints->SetPoint(8, x1+3*delX,20,0);
      this->LegendPoints->SetPoint(9, x1+4*delX,20,0);
      
      // Specify the position of the legend title
      this->LabelActors[5]->SetPosition(0.5*size[0],22);
      this->Coordinate->SetValue(0.33333*size[0],15,0.0);
      double *x = this->Coordinate->GetComputedWorldValue(viewport);
      double xL[3]; xL[0]=x[0];xL[1]=x[1];xL[2]=x[2];
      this->Coordinate->SetValue(0.66667*size[0],15,0.0);
      x = this->Coordinate->GetComputedWorldValue(viewport);
      double xR[3]; xR[0]=x[0];xR[1]=x[1];xR[2]=x[2];
      double len = sqrt(vtkMath::Distance2BetweenPoints(xL,xR));
      char buf[256];
      sprintf(buf,"Scale 1 : %g",len);
      this->LabelMappers[5]->SetInput(buf);
      
      // Now specify the position of the legend labels
      x = this->LegendPoints->GetPoint(0);
      this->LabelActors[0]->SetPosition(x[0],x[1]-1);
      x = this->LegendPoints->GetPoint(1);
      this->LabelActors[1]->SetPosition(x[0],x[1]-1);
      x = this->LegendPoints->GetPoint(2);
      this->LabelActors[2]->SetPosition(x[0],x[1]-1);
      x = this->LegendPoints->GetPoint(3);
      this->LabelActors[3]->SetPosition(x[0],x[1]-1);
      x = this->LegendPoints->GetPoint(4);
      this->LabelActors[4]->SetPosition(x[0],x[1]-1);
      }

    this->BuildTime.Modified();
    }
}

//----------------------------------------------------------------------
void vtkLegendScaleActor::AllAnnotationsOn()
{
  if ( this->RightAxisVisibility && this->TopAxisVisibility &&
       this->LeftAxisVisibility && this->BottomAxisVisibility &&
       this->LegendVisibility )
    {
    return;
    }

  // If here, we are modified and something gets turned on
  this->RightAxisVisibility = 1;
  this->TopAxisVisibility = 1;
  this->LeftAxisVisibility = 1;
  this->BottomAxisVisibility = 1;
  this->LegendVisibility = 1;
  this->Modified();
}

//----------------------------------------------------------------------
void vtkLegendScaleActor::AllAnnotationsOff()
{
  if ( !this->RightAxisVisibility && !this->TopAxisVisibility &&
       !this->LeftAxisVisibility && !this->BottomAxisVisibility &&
       !this->LegendVisibility )
    {
    return;
    }

  // If here, we are modified and something gets turned off
  this->RightAxisVisibility = 0;
  this->TopAxisVisibility = 0;
  this->LeftAxisVisibility = 0;
  this->BottomAxisVisibility = 0;
  this->LegendVisibility = 0;
  this->Modified();
}

//----------------------------------------------------------------------
void vtkLegendScaleActor::AllAxesOn()
{
  if ( this->RightAxisVisibility && this->TopAxisVisibility &&
       this->LeftAxisVisibility && this->BottomAxisVisibility )
    {
    return;
    }

  // If here, we are modified and something gets turned on
  this->RightAxisVisibility = 1;
  this->TopAxisVisibility = 1;
  this->LeftAxisVisibility = 1;
  this->BottomAxisVisibility = 1;
  this->Modified();
}

//----------------------------------------------------------------------
void vtkLegendScaleActor::AllAxesOff()
{
  if ( !this->RightAxisVisibility && !this->TopAxisVisibility &&
       !this->LeftAxisVisibility && !this->BottomAxisVisibility )
    {
    return;
    }

  // If here, we are modified and something gets turned off
  this->RightAxisVisibility = 0;
  this->TopAxisVisibility = 0;
  this->LeftAxisVisibility = 0;
  this->BottomAxisVisibility = 0;
  this->Modified();
}

//----------------------------------------------------------------------
void vtkLegendScaleActor::PrintSelf(ostream& os, vtkIndent indent)
{
  //Superclass typedef defined in vtkTypeMacro() found in vtkSetGet.h
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Label Mode: ";
  if ( this->LabelMode == DISTANCE )
    {
    os << "Distance\n";
    }
  else //if ( this->LabelMode == DISTANCE )
    {
    os << "XY_Coordinates\n";
    }
  
  os << indent << "Right Axis Visibility: " 
     << (this->RightAxisVisibility ? "On\n" : "Off\n");
  os << indent << "Top Axis Visibility: " 
     << (this->TopAxisVisibility ? "On\n" : "Off\n");
  os << indent << "Left Axis Visibility: " 
     << (this->LeftAxisVisibility ? "On\n" : "Off\n");
  os << indent << "Bottom Axis Visibility: " 
     << (this->BottomAxisVisibility ? "On\n" : "Off\n");
  os << indent << "Legend Visibility: " 
     << (this->LegendVisibility ? "On\n" : "Off\n");
  os << indent << "Corner Offset Factor: " << this->CornerOffsetFactor << "\n";
  
  os << indent << "Right Border Offset: " << this->RightBorderOffset << "\n";
  os << indent << "Top Border Offset: " << this->TopBorderOffset << "\n";
  os << indent << "Left Border Offset: " << this->LeftBorderOffset << "\n";
  os << indent << "Bottom Border Offset: " << this->BottomBorderOffset << "\n";

  os << indent << "Legend Title Property: ";
  if ( this->LegendTitleProperty )
    {
    os << this->LegendTitleProperty << "\n";
    }
  else
    {
    os << "(none)\n";
    }
  os << indent << "Legend Label Property: ";
  if ( this->LegendLabelProperty )
    {
    os << this->LegendLabelProperty << "\n";
    }
  else
    {
    os << "(none)\n";
    }
  
  os << indent << "Right Axis: ";
  if ( this->RightAxis )
    {
    os << this->RightAxis << "\n";
    }
  else
    {
    os << "(none)\n";
    }
  os << indent << "Top Axis: ";
  if ( this->TopAxis )
    {
    os << this->TopAxis << "\n";
    }
  else
    {
    os << "(none)\n";
    }
  os << indent << "Left Axis: ";
  if ( this->LeftAxis )
    {
    os << this->LeftAxis << "\n";
    }
  else
    {
    os << "(none)\n";
    }
  os << indent << "Bottom Axis: ";
  if ( this->BottomAxis )
    {
    os << this->BottomAxis << "\n";
    }
  else
    {
    os << "(none)\n";
    }
}
