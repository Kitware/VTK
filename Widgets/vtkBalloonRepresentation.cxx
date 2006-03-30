/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBalloonRepresentation.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkBalloonRepresentation.h"
#include "vtkRenderer.h"
#include "vtkTextProperty.h"
#include "vtkTextProperty.h"
#include "vtkProperty2D.h"
#include "vtkTextMapper.h"
#include "vtkTextActor.h"
#include "vtkPoints.h"
#include "vtkCellArray.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper2D.h"
#include "vtkActor2D.h"
#include "vtkWindow.h"
#include "vtkObjectFactory.h"
#include "vtkInteractorObserver.h"

vtkCxxRevisionMacro(vtkBalloonRepresentation, "1.3");
vtkStandardNewMacro(vtkBalloonRepresentation);

vtkCxxSetObjectMacro(vtkBalloonRepresentation, TextProperty, vtkTextProperty);
vtkCxxSetObjectMacro(vtkBalloonRepresentation, FrameProperty, vtkProperty2D);
                     
//----------------------------------------------------------------------
vtkBalloonRepresentation::vtkBalloonRepresentation()
{
  // Initially we are not visible
  this->Visibility = 0;

  // Balloon text
  this->BalloonText = NULL;
  this->Padding = 5;
  this->Offset[0] =  15.0;
  this->Offset[1] = -30.0;

  // The text actor
  this->TextMapper = vtkTextMapper::New();
  this->TextActor = vtkActor2D::New();
  this->TextActor->SetMapper(this->TextMapper);
  this->TextProperty = vtkTextProperty::New();
  this->TextProperty->SetColor(0,0,0);
  this->TextProperty->SetFontSize(14);
  this->TextProperty->BoldOn();
  this->TextMapper->SetTextProperty(this->TextProperty);
  
  // The frame
  this->FramePoints = vtkPoints::New();
  this->FramePoints->SetNumberOfPoints(4);
  this->FramePolygon = vtkCellArray::New();
  this->FramePolygon->Allocate(this->FramePolygon->EstimateSize(1,5));
  this->FramePolygon->InsertNextCell(4);
  this->FramePolygon->InsertCellPoint(0);
  this->FramePolygon->InsertCellPoint(1);
  this->FramePolygon->InsertCellPoint(2);
  this->FramePolygon->InsertCellPoint(3);
  this->FramePolyData = vtkPolyData::New();
  this->FramePolyData->SetPoints(this->FramePoints);
  this->FramePolyData->SetPolys(this->FramePolygon);
  this->FrameMapper = vtkPolyDataMapper2D::New();
  this->FrameMapper->SetInput(this->FramePolyData);
  this->FrameActor = vtkActor2D::New();
  this->FrameActor->SetMapper(this->FrameMapper);
  this->FrameProperty = vtkProperty2D::New();
  this->FrameProperty->SetColor(1,1,.882);
  this->FrameProperty->SetOpacity(0.5);
  this->FrameActor->SetProperty(this->FrameProperty);
}

//----------------------------------------------------------------------
vtkBalloonRepresentation::~vtkBalloonRepresentation()
{
  this->TextMapper->Delete();
  this->TextActor->Delete();
  this->TextProperty->Delete();
  
  // The frame
  this->FramePoints->Delete();
  this->FramePolygon->Delete();
  this->FramePolyData->Delete();
  this->FrameMapper->Delete();
  this->FrameActor->Delete();
}

//----------------------------------------------------------------------
void vtkBalloonRepresentation::StartWidgetInteraction(double e[2])
{
  this->StartEventPosition[0] = e[0];
  this->StartEventPosition[1] = e[1];
  this->VisibilityOn();
}


//----------------------------------------------------------------------
void vtkBalloonRepresentation::EndWidgetInteraction(double* vtkNotUsed(e[2]))
{
  this->VisibilityOff();
}


//----------------------------------------------------------------------
void vtkBalloonRepresentation::BuildRepresentation()
{
  if ( this->GetMTime() > this->BuildTime || 
       (this->Renderer && this->Renderer->GetVTKWindow() &&
        this->Renderer->GetVTKWindow()->GetMTime() > this->BuildTime) )
    {
    // Start by getting the size of the text, and then figuring out where to place it where it is visible
    double e[2];
    e[0] = this->StartEventPosition[0] + this->Offset[0];
    e[1] = this->StartEventPosition[1] + this->Offset[1];

    int stringSize[2];
    this->TextMapper->SetInput(this->BalloonText);
    this->TextMapper->GetSize(this->Renderer, stringSize);

    int *size = this->Renderer->GetSize();
    if ( (e[0]+stringSize[0]+2*this->Padding) > size[0] )
      {
      e[0] = size[0] - (stringSize[0]+2*this->Padding);
      }
    if ( (e[1]+stringSize[1]+2*this->Padding) > size[1] )
      {
      e[1] = size[1] - (stringSize[1]+2*this->Padding);
      }

    // Now position the text and the frame
    this->TextActor->SetPosition(e[0]+this->Padding, e[1]+this->Padding);
    this->FramePoints->SetPoint(0, e[0],e[1],0.0);
    this->FramePoints->SetPoint(1, e[0]+stringSize[0]+2*this->Padding,e[1],0.0);
    this->FramePoints->SetPoint(2, e[0]+stringSize[0]+2*this->Padding,e[1]+stringSize[1]+2*this->Padding,0.0);
    this->FramePoints->SetPoint(3, e[0],e[1]+stringSize[1]+2*this->Padding,0.0);
    
    this->BuildTime.Modified();
    }
}

//----------------------------------------------------------------------
void vtkBalloonRepresentation::ReleaseGraphicsResources(vtkWindow *w)
{
  this->TextActor->ReleaseGraphicsResources(w);
  this->FrameActor->ReleaseGraphicsResources(w);
}


//----------------------------------------------------------------------
int vtkBalloonRepresentation::RenderOverlay(vtkViewport *w)
{
  this->BuildRepresentation();
  int count = this->FrameActor->RenderOverlay(w);
  count += this->TextActor->RenderOverlay(w);
  return count;
}


//----------------------------------------------------------------------
void vtkBalloonRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  //Superclass typedef defined in vtkTypeMacro() found in vtkSetGet.h
  this->Superclass::PrintSelf(os,indent);
  
  os << indent << "Balloon Text: ";
  if ( this->BalloonText )
    {
    os << this->BalloonText << "\n";
    }
  else
    {
    os << "(none)\n";
    }
  
  os << indent << "Padding: " << this->Padding << "\n";
  os << indent << "Offset: (" << this->Offset[0] << ","
     << this->Offset[1] << ")\n";
}

