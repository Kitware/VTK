/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCenteredSliderRepresentation.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

#include "vtkCenteredSliderRepresentation.h"
#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkActor2D.h"
#include "vtkPolyDataMapper2D.h"
#include "vtkPoints.h"
#include "vtkCellArray.h"
#include "vtkProperty2D.h"
#include "vtkRenderer.h"
#include "vtkMath.h"
#include "vtkLine.h"
#include "vtkEvent.h"
#include "vtkInteractorObserver.h"
#include "vtkWindow.h"
#include "vtkPolyData.h"
#include "vtkTextProperty.h"
#include "vtkTextMapper.h"
#include "vtkTextActor.h"
#include "vtkTransform.h"
#include "vtkTransformPolyDataFilter.h"
#include "vtkSmartPointer.h"
#include "vtkUnsignedCharArray.h"
#include "vtkPointData.h"

vtkStandardNewMacro(vtkCenteredSliderRepresentation);

//----------------------------------------------------------------------
vtkCenteredSliderRepresentation::vtkCenteredSliderRepresentation()
{
  // The coordinates defining the slider
  this->Point1Coordinate = vtkCoordinate::New();
  this->Point1Coordinate->SetCoordinateSystemToNormalizedViewport();
  this->Point1Coordinate->SetValue(0.95,0.8,0.0);

  this->Point2Coordinate = vtkCoordinate::New();
  this->Point2Coordinate->SetCoordinateSystemToNormalizedViewport();
  this->Point2Coordinate->SetValue(0.99,0.98,0.0);

  // Default configuration
  this->ButtonSize = 0.08;
  this->TubeSize = 0.85; // includes buttons
  this->ArcCount = 31;
  this->ArcStart = 1.0 - this->TubeSize + this->ButtonSize;
  this->ArcEnd = 1.0 - this->ButtonSize;

  // The points and the transformation for the points.
  this->XForm = vtkTransform::New();
  this->Points = vtkPoints::New();
  this->Points->SetNumberOfPoints(2*this->ArcCount + 12);

  this->TubeCells = 0;
  this->Tube = 0;
  this->BuildTube();

  this->TubeXForm = vtkTransformPolyDataFilter::New();
  this->TubeXForm->SetInputData(this->Tube);
  this->TubeXForm->SetTransform(this->XForm);

  this->TubeMapper = vtkPolyDataMapper2D::New();
  this->TubeMapper->SetInputConnection(
    this->TubeXForm->GetOutputPort());

  this->TubeProperty = vtkProperty2D::New();
  this->TubeProperty->SetOpacity(0.6);

  this->TubeActor = vtkActor2D::New();
  this->TubeActor->SetMapper(this->TubeMapper);
  this->TubeActor->SetProperty(this->TubeProperty);

  this->SelectedProperty = vtkProperty2D::New();
  this->SelectedProperty->SetOpacity(1.0);

  // The slider
  this->SliderCells = vtkCellArray::New();
  this->SliderCells->InsertNextCell(4);
  this->SliderCells->InsertCellPoint(this->ArcCount*2+8);
  this->SliderCells->InsertCellPoint(this->ArcCount*2+9);
  this->SliderCells->InsertCellPoint(this->ArcCount*2+10);
  this->SliderCells->InsertCellPoint(this->ArcCount*2+11);
  this->Slider = vtkPolyData::New();
  this->Slider->SetPoints(this->Points);
  this->Slider->SetPolys(this->SliderCells);

  this->SliderXForm = vtkTransformPolyDataFilter::New();
  this->SliderXForm->SetInputData(this->Slider);
  this->SliderXForm->SetTransform(XForm);

  this->SliderMapper = vtkPolyDataMapper2D::New();
  this->SliderMapper->SetInputConnection(
    this->SliderXForm->GetOutputPort());

  this->SliderProperty = vtkProperty2D::New();
  this->SliderProperty->SetColor(1,1,1);

  this->SliderActor = vtkActor2D::New();
  this->SliderActor->SetMapper(this->SliderMapper);
  this->SliderActor->SetProperty(this->SliderProperty);

  this->LabelProperty = vtkTextProperty::New();
  this->LabelProperty->SetFontFamilyToArial();
  this->LabelProperty->SetJustificationToCentered();
  this->LabelActor = vtkTextActor::New();
  this->LabelActor->SetTextProperty(this->LabelProperty);
  this->LabelActor->SetInput("");
  this->LabelActor->GetPositionCoordinate()->
    SetCoordinateSystemToViewport();

  this->Value = 0;
  this->PickedT = 0.5;
  this->HighlightState = 0;
}

void vtkCenteredSliderRepresentation::BuildTube()
{
  // The tube (the slider moves along the tube)

  // the top cap
  if (this->TubeCells)
    {
    this->TubeCells->Delete();
    this->TubeCells = 0;
    }
  this->TubeCells = vtkCellArray::New();
  this->TubeCells->InsertNextCell(5);
  this->TubeCells->InsertCellPoint(0);
  this->TubeCells->InsertCellPoint(1);
  this->TubeCells->InsertCellPoint(this->ArcCount+4+1);
  this->TubeCells->InsertCellPoint(this->ArcCount+4);
  this->TubeCells->InsertCellPoint(0);

  // the bottom cap
  this->TubeCells->InsertNextCell(5);
  this->TubeCells->InsertCellPoint(this->ArcCount+2);
  this->TubeCells->InsertCellPoint(this->ArcCount+3);
  this->TubeCells->InsertCellPoint(2*this->ArcCount+4+3);
  this->TubeCells->InsertCellPoint(2*this->ArcCount+4+2);
  this->TubeCells->InsertCellPoint(this->ArcCount+2);

  for (int i = 0; i < this->ArcCount; i += 2)
    {
    this->TubeCells->InsertNextCell(4);
    this->TubeCells->InsertCellPoint(i+1);
    this->TubeCells->InsertCellPoint(i+2);
    this->TubeCells->InsertCellPoint(this->ArcCount+i+6);
    this->TubeCells->InsertCellPoint(this->ArcCount+i+5);
    }

  vtkSmartPointer<vtkUnsignedCharArray> colors =
    vtkSmartPointer<vtkUnsignedCharArray>::New();
  colors->SetNumberOfComponents(4);
  colors->SetNumberOfTuples(this->ArcCount*2+12);

  if (this->Tube)
    {
    this->Tube->Delete();
    this->Tube = 0;
    }
  this->Tube = vtkPolyData::New();
  this->Tube->SetPoints(this->Points);
  this->Tube->GetPointData()->SetScalars(colors);
  this->Tube->SetPolys(this->TubeCells);

  unsigned char col[4];
  col[0] = 255; col[1] = 255; col[2] = 255; col[3] = 200;

  // build tube points
  this->Points->SetPoint(0, 0.0, 1.0, 0.0);
  this->Points->SetPoint(1, 0.0, this->ArcEnd, 0.0);
  this->Points->SetPoint(this->ArcCount+2, 0.0, this->ArcStart, 0.0);
  this->Points->SetPoint(this->ArcCount+3, 0.0, 1.0 - this->TubeSize, 0.0);
  colors->SetTupleValue(0,col);
  colors->SetTupleValue(1,col);
  colors->SetTupleValue(this->ArcCount+2,col);
  colors->SetTupleValue(this->ArcCount+3,col);

  this->Points->SetPoint(this->ArcCount+4, 1.0,1.0, 0.0);
  this->Points->SetPoint(this->ArcCount+5, 1.0, this->ArcEnd, 0.0);
  this->Points->SetPoint(2*this->ArcCount+6, 1.0, this->ArcStart, 0.0);
  this->Points->SetPoint(2*this->ArcCount+7, 1.0, 1.0 - this->TubeSize, 0.0);
  colors->SetTupleValue(this->ArcCount+4,col);
  colors->SetTupleValue(this->ArcCount+5,col);
  colors->SetTupleValue(2*this->ArcCount+6,col);
  colors->SetTupleValue(2*this->ArcCount+7,col);

  // and the arc
  double midPoint = this->ArcCount/2.0;
  double halfArcLength = (this->ArcEnd - this->ArcStart)/2.0;
  for (int i = 0; i < this->ArcCount; ++i)
    {
    double factor = fabs((i - midPoint)/midPoint);
    factor = pow(factor,1.4);
    double sign = 1;
    if (i < midPoint)
      {
      sign = -1;
      }
    this->Points->SetPoint
      (i + 2, 0.3,
       (1.0 - this->TubeSize/2.0) - halfArcLength*factor*sign, 0.0);
    this->Points->SetPoint
      (i + this->ArcCount + 6, 0.7,
       (1.0 - this->TubeSize/2.0) - halfArcLength*factor*sign, 0.0);
    col[3] = static_cast<unsigned char>(255*factor);
    colors->SetTupleValue(i+2,col);
    colors->SetTupleValue(i+this->ArcCount+6,col);
    }

  // last four points are the slider
  this->Points->SetPoint(this->ArcCount*2+8, 0.0,
                         (this->ArcStart + this->ArcEnd)/2.0 + 0.025, 0.0);
  this->Points->SetPoint(this->ArcCount*2+9, 0.0,
                         (this->ArcStart + this->ArcEnd)/2.0 - 0.025, 0.0);
  this->Points->SetPoint(this->ArcCount*2+10, 1.0,
                         (this->ArcStart + this->ArcEnd)/2.0 - 0.025, 0.0);
  this->Points->SetPoint(this->ArcCount*2+11, 1.0,
                         (this->ArcStart + this->ArcEnd)/2.0 + 0.025, 0.0);
  col[0] = 255; col[1] = 255; col[2] = 255; col[3] = 255;
  colors->SetTupleValue(this->ArcCount*2+8,col);
  colors->SetTupleValue(this->ArcCount*2+9,col);
  colors->SetTupleValue(this->ArcCount*2+10,col);
  colors->SetTupleValue(this->ArcCount*2+11,col);
}

//----------------------------------------------------------------------
vtkCenteredSliderRepresentation::~vtkCenteredSliderRepresentation()
{
  this->Point1Coordinate->Delete();
  this->Point2Coordinate->Delete();

  this->XForm->Delete();
  this->Points->Delete();

  this->SliderCells->Delete();
  this->Slider->Delete();
  this->SliderXForm->Delete();
  this->SliderMapper->Delete();
  this->SliderActor->Delete();
  this->SliderProperty->Delete();

  this->Tube->Delete();
  this->TubeCells->Delete();
  this->TubeXForm->Delete();
  this->TubeMapper->Delete();
  this->TubeActor->Delete();
  this->TubeProperty->Delete();

  this->SelectedProperty->Delete();

  this->LabelProperty->Delete();
  this->LabelActor->Delete();
}

//----------------------------------------------------------------------
vtkCoordinate *vtkCenteredSliderRepresentation::GetPoint1Coordinate()
{
  return this->Point1Coordinate;
}

void vtkCenteredSliderRepresentation::StartWidgetInteraction(double eventPos[2])
{
  this->ComputeInteractionState(static_cast<int>(eventPos[0]),
                                static_cast<int>(eventPos[1]));
}

//----------------------------------------------------------------------
int vtkCenteredSliderRepresentation::ComputeInteractionState(int x, int y,
                                                             int /*modify*/)
{
  // where is the pick
  int *p1 = this->Point1Coordinate->GetComputedViewportValue(this->Renderer);
  int *p2 = this->Point2Coordinate->GetComputedViewportValue(this->Renderer);

  // convert the eventPos into parametric coordinates
  double pcoord[2];
  if (!(p2[0] - p1[0]) || !(p2[1] - p1[1]))
    {
    this->InteractionState = vtkSliderRepresentation::Outside;
    return this->InteractionState;
    }

  pcoord[0] = (static_cast<double>(x) - p1[0])/(p2[0] - p1[0]);
  pcoord[1] = (static_cast<double>(y) - p1[1])/(p2[1] - p1[1]);

  if ( pcoord[0] < 0 || pcoord[0] > 1.0)
    {
    this->InteractionState = vtkSliderRepresentation::Outside;
    return this->InteractionState;
    }

  // if it is on the slider...
  if ( fabs(pcoord[1] - (1.0 - 0.5*this->TubeSize)) < 0.1)
    {
    this->InteractionState = vtkSliderRepresentation::Slider;
    return this->InteractionState;
    }

  // if on the tube
  if ( pcoord[1] >= this->ArcStart && pcoord[1] <= this->ArcEnd)
    {
    this->InteractionState = vtkSliderRepresentation::Tube;
    this->ComputePickPosition(x,y);
    return this->InteractionState;
    }

  // on the bottom aka left cap
  if ( pcoord[1] >= 1.0 - this->TubeSize &&
       pcoord[1] <= 1.0 - this->TubeSize + this->ArcStart)
    {
    this->InteractionState = vtkSliderRepresentation::LeftCap;
    return this->InteractionState;
    }

  // on the top aka right cap
  if ( pcoord[1] >= this->ArcEnd && pcoord[1] <= 1.0)
    {
    this->InteractionState = vtkSliderRepresentation::RightCap;
    return this->InteractionState;
    }

  this->InteractionState = vtkSliderRepresentation::Outside;
  return this->InteractionState;
}

//----------------------------------------------------------------------
void vtkCenteredSliderRepresentation::WidgetInteraction(double eventPos[2])
{
  double t = this->ComputePickPosition(eventPos[0], eventPos[1]);
  this->SetValue(this->MinimumValue +
                 t*(this->MaximumValue-this->MinimumValue));
  this->BuildRepresentation();
}

//----------------------------------------------------------------------
vtkCoordinate *vtkCenteredSliderRepresentation::GetPoint2Coordinate()
{
  return this->Point2Coordinate;
}

//----------------------------------------------------------------------
void vtkCenteredSliderRepresentation::PlaceWidget(double *vtkNotUsed(bds[6]))
{
  // Position the handles at the end of the lines
  this->BuildRepresentation();
}

//----------------------------------------------------------------------
double vtkCenteredSliderRepresentation
::ComputePickPosition(double /* x */, double y)
{
  // where is the pick
  int *p1 = this->Point1Coordinate->GetComputedViewportValue(this->Renderer);
  int *p2 = this->Point2Coordinate->GetComputedViewportValue(this->Renderer);

  // convert the eventPos into parametric coordinates
  this->PickedT = (y - p1[1])/(p2[1] - p1[1]);
  this->PickedT = (this->PickedT - this->ArcStart)/
    (this->ArcEnd - this->ArcStart);


  this->PickedT = ( this->PickedT < 0 ? 0.0 :
                    (this->PickedT > 1.0 ? 1.0 : this->PickedT) );

  return this->PickedT;
}

//----------------------------------------------------------------------
void vtkCenteredSliderRepresentation::Highlight(int highlight)
{
  if ( highlight )
    {
    this->SliderActor->SetProperty(this->SelectedProperty);
    }
  else
    {
    this->SliderActor->SetProperty(this->SliderProperty);
    }
  this->HighlightState = highlight;
}


//----------------------------------------------------------------------
void vtkCenteredSliderRepresentation::BuildRepresentation()
{
  if ( this->GetMTime() <= this->BuildTime &&
       (!this->Renderer || !this->Renderer->GetVTKWindow() ||
        this->Renderer->GetVTKWindow()->GetMTime() <= this->BuildTime) )
    {
    return;
    }

  int *size = this->Renderer->GetSize();
  if (0 == size[0] || 0 == size[1])
    {
    // Renderer has no size yet: wait until the next
    // BuildRepresentation...
    return;
    }

  this->XForm->Identity();

  // scale and position and rotate the polydata
  int *p1 = this->Point1Coordinate->GetComputedViewportValue(this->Renderer);
  int *p2 = this->Point2Coordinate->GetComputedViewportValue(this->Renderer);

  double xsize = p2[0] - p1[0];
  double ysize = p2[1] - p1[1];

  this->XForm->Translate(p1[0], p1[1], 0.0);
  this->XForm->Scale(xsize,ysize,1.0);

  // adjust the slider position
  double t = (this->Value-this->MinimumValue) /
    (this->MaximumValue-this->MinimumValue);
  double pos = this->ArcStart + t*(this->ArcEnd - this->ArcStart);
  this->Points->SetPoint(this->ArcCount*2+8, 0.0,
                         pos - 0.025, 0.0);
  this->Points->SetPoint(this->ArcCount*2+9, 0.0,
                         pos + 0.025, 0.0);
  this->Points->SetPoint(this->ArcCount*2+10, 1.0,
                         pos + 0.025, 0.0);
  this->Points->SetPoint(this->ArcCount*2+11, 1.0,
                         pos - 0.025, 0.0);


  this->LabelActor->SetPosition(p1[0]+xsize*0.5,p1[1]);
  this->LabelProperty->SetFontSize(static_cast<int>(xsize*0.8));

  this->BuildTime.Modified();
}

//----------------------------------------------------------------------
void vtkCenteredSliderRepresentation::GetActors(vtkPropCollection *pc)
{
  pc->AddItem(this->TubeActor);
  pc->AddItem(this->SliderActor);
  pc->AddItem(this->LabelActor);
}

//----------------------------------------------------------------------
void vtkCenteredSliderRepresentation::ReleaseGraphicsResources(vtkWindow *w)
{
  this->TubeActor->ReleaseGraphicsResources(w);
  this->LabelActor->ReleaseGraphicsResources(w);
  this->SliderActor->ReleaseGraphicsResources(w);
}

//----------------------------------------------------------------------
int vtkCenteredSliderRepresentation::RenderOpaqueGeometry(vtkViewport *viewport)
{
  this->BuildRepresentation();
  int count = this->TubeActor->RenderOpaqueGeometry(viewport);
  count += this->SliderActor->RenderOpaqueGeometry(viewport);
  if (this->HighlightState && strlen(this->LabelActor->GetInput()))
    {
    count += this->LabelActor->RenderOpaqueGeometry(viewport);
    }
  return count;
}

//----------------------------------------------------------------------
int vtkCenteredSliderRepresentation::RenderOverlay(vtkViewport *viewport)
{
  this->BuildRepresentation();
  int count = this->TubeActor->RenderOverlay(viewport);
  count += this->SliderActor->RenderOverlay(viewport);
  if (this->HighlightState && strlen(this->LabelActor->GetInput()))
    {
    count += this->LabelActor->RenderOverlay(viewport);
    }
  return count;
}

//----------------------------------------------------------------------
void vtkCenteredSliderRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  //Superclass typedef defined in vtkTypeMacro() found in vtkSetGet.h
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Point1 Coordinate: " << this->Point1Coordinate << "\n";
  this->Point1Coordinate->PrintSelf(os, indent.GetNextIndent());

  os << indent << "Point2 Coordinate: " << this->Point2Coordinate << "\n";
  this->Point2Coordinate->PrintSelf(os, indent.GetNextIndent());

  if ( this->SliderProperty )
    {
    os << indent << "Slider Property:\n";
    this->SliderProperty->PrintSelf(os,indent.GetNextIndent());
    }
  else
    {
    os << indent << "Slider Property: (none)\n";
    }

  if ( this->SelectedProperty )
    {
    os << indent << "SelectedProperty:\n";
    this->SelectedProperty->PrintSelf(os,indent.GetNextIndent());
    }
  else
    {
    os << indent << "SelectedProperty: (none)\n";
    }

  if ( this->TubeProperty )
    {
    os << indent << "TubeProperty:\n";
    this->TubeProperty->PrintSelf(os,indent.GetNextIndent());
    }
  else
    {
    os << indent << "TubeProperty: (none)\n";
    }

  if ( this->SelectedProperty )
    {
    os << indent << "SelectedProperty:\n";
    this->SelectedProperty->PrintSelf(os,indent.GetNextIndent());
    }
  else
    {
    os << indent << "SelectedProperty: (none)\n";
    }

  if ( this->LabelProperty )
    {
    os << indent << "LabelProperty:\n";
    this->LabelProperty->PrintSelf(os,indent.GetNextIndent());
    }
  else
    {
    os << indent << "LabelProperty: (none)\n";
    }

}


//----------------------------------------------------------------------
void vtkCenteredSliderRepresentation::SetTitleText(const char* label)
{
  this->LabelActor->SetInput(label);
  if ( this->LabelActor->GetMTime() > this->GetMTime() )
    {
    this->Modified();
    }
}

//----------------------------------------------------------------------
const char* vtkCenteredSliderRepresentation::GetTitleText()
{
  return this->LabelActor->GetInput();
}
