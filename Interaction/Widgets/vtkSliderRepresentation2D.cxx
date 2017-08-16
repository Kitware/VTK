/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSliderRepresentation2D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSliderRepresentation2D.h"
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

vtkStandardNewMacro(vtkSliderRepresentation2D);

//----------------------------------------------------------------------
vtkSliderRepresentation2D::vtkSliderRepresentation2D()
{
  // The coordinates defining the slider
  this->Point1Coordinate = vtkCoordinate::New();
  this->Point1Coordinate->SetCoordinateSystemToWorld();
  this->Point1Coordinate->SetValue(-1.0,0.0,0.0);

  this->Point2Coordinate = vtkCoordinate::New();
  this->Point2Coordinate->SetCoordinateSystemToWorld();
  this->Point2Coordinate->SetValue(1.0,0.0,0.0);

  // Default configuration
  this->LabelHeight = 0.025;
  this->TitleHeight = 0.030;

  this->SliderLength = 0.01;
  this->SliderWidth = 0.02;
  this->EndCapLength = 0.005;
  this->TubeWidth = 0.01;

  // The points and the transformation for the points. There are a total
  // of 18 points: 4 for each of slider, tube and caps, and two extra points
  // for the title and label text.
  this->XForm = vtkTransform::New();
  this->Points = vtkPoints::New();
  this->Points->SetNumberOfPoints(18);

  // The slider
  this->SliderCells = vtkCellArray::New();
  this->SliderCells->Allocate(this->SliderCells->EstimateSize(1,4));
  this->SliderCells->InsertNextCell(4);
  this->SliderCells->InsertCellPoint(8);
  this->SliderCells->InsertCellPoint(9);
  this->SliderCells->InsertCellPoint(10);
  this->SliderCells->InsertCellPoint(11);
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

  // The tube (the slider moves along the tube)
  this->TubeCells = vtkCellArray::New();
  this->TubeCells->Allocate(this->TubeCells->EstimateSize(1,4));
  this->TubeCells->InsertNextCell(4);
  this->TubeCells->InsertCellPoint(4);
  this->TubeCells->InsertCellPoint(5);
  this->TubeCells->InsertCellPoint(6);
  this->TubeCells->InsertCellPoint(7);
  this->Tube = vtkPolyData::New();
  this->Tube->SetPoints(this->Points);
  this->Tube->SetPolys(this->TubeCells);

  this->TubeXForm = vtkTransformPolyDataFilter::New();
  this->TubeXForm->SetInputData(this->Tube);
  this->TubeXForm->SetTransform(XForm);

  this->TubeMapper = vtkPolyDataMapper2D::New();
  this->TubeMapper->SetInputConnection(
    this->TubeXForm->GetOutputPort());

  this->TubeProperty = vtkProperty2D::New();
  this->TubeProperty->SetColor(1,1,1);

  this->TubeActor = vtkActor2D::New();
  this->TubeActor->SetMapper(this->TubeMapper);
  this->TubeActor->SetProperty(this->TubeProperty);

  this->SelectedProperty = vtkProperty2D::New();
  this->SelectedProperty->SetColor(1.0000, 0.4118, 0.7059); //hot pink

  // The two caps
  this->CapCells = vtkCellArray::New();
  this->CapCells->Allocate(this->CapCells->EstimateSize(2,4));
  this->CapCells->InsertNextCell(4);
  this->CapCells->InsertCellPoint(0);
  this->CapCells->InsertCellPoint(1);
  this->CapCells->InsertCellPoint(2);
  this->CapCells->InsertCellPoint(3);
  this->CapCells->InsertNextCell(4);
  this->CapCells->InsertCellPoint(12);
  this->CapCells->InsertCellPoint(13);
  this->CapCells->InsertCellPoint(14);
  this->CapCells->InsertCellPoint(15);
  this->Cap = vtkPolyData::New();
  this->Cap->SetPoints(this->Points);
  this->Cap->SetPolys(this->CapCells);

  this->CapXForm = vtkTransformPolyDataFilter::New();
  this->CapXForm->SetInputData(this->Cap);
  this->CapXForm->SetTransform(XForm);

  this->CapMapper = vtkPolyDataMapper2D::New();
  this->CapMapper->SetInputConnection(
    this->CapXForm->GetOutputPort());

  this->CapProperty = vtkProperty2D::New();
  this->CapProperty->SetColor(1,1,1);

  this->CapActor = vtkActor2D::New();
  this->CapActor->SetMapper(this->CapMapper);
  this->CapActor->SetProperty(this->CapProperty);

  // Labels and text
  this->ShowSliderLabel = 1;

  this->LabelProperty = vtkTextProperty::New();
  this->LabelProperty->SetBold(1);
  this->LabelProperty->SetShadow(1);
  this->LabelProperty->SetFontFamilyToArial();
  this->LabelProperty->SetJustificationToCentered();
  this->LabelProperty->SetVerticalJustificationToCentered();
  this->LabelMapper = vtkTextMapper::New();
  this->LabelMapper->SetInput("");
  this->LabelMapper->SetTextProperty(this->LabelProperty);
  this->LabelActor = vtkActor2D::New();
  this->LabelActor->SetMapper(this->LabelMapper);

  this->TitleProperty = vtkTextProperty::New();
  this->TitleProperty->SetBold(1);
  this->TitleProperty->SetShadow(1);
  this->TitleProperty->SetFontFamilyToArial();
  this->TitleProperty->SetJustificationToCentered();
  this->TitleProperty->SetVerticalJustificationToCentered();
  this->TitleMapper = vtkTextMapper::New();
  this->TitleMapper->SetInput("");
  this->TitleMapper->SetTextProperty(this->TitleProperty);
  this->TitleActor = vtkActor2D::New();
  this->TitleActor->SetMapper(this->TitleMapper);
}

//----------------------------------------------------------------------
vtkSliderRepresentation2D::~vtkSliderRepresentation2D()
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

  this->TubeCells->Delete();
  this->Tube->Delete();
  this->TubeXForm->Delete();
  this->TubeMapper->Delete();
  this->TubeActor->Delete();
  this->TubeProperty->Delete();

  this->CapCells->Delete();
  this->Cap->Delete();
  this->CapXForm->Delete();
  this->CapMapper->Delete();
  this->CapActor->Delete();
  this->CapProperty->Delete();

  this->SelectedProperty->Delete();

  this->LabelProperty->Delete();
  this->LabelMapper->Delete();
  this->LabelActor->Delete();

  this->TitleProperty->Delete();
  this->TitleMapper->Delete();
  this->TitleActor->Delete();
}

//----------------------------------------------------------------------
void vtkSliderRepresentation2D::SetTitleText(const char* label)
{
  this->TitleMapper->SetInput(label);
  if ( this->TitleMapper->GetMTime() > this->GetMTime() )
  {
    this->Modified();
  }
}

//----------------------------------------------------------------------
const char* vtkSliderRepresentation2D::GetTitleText()
{
  return this->TitleMapper->GetInput();
}

//----------------------------------------------------------------------
vtkCoordinate *vtkSliderRepresentation2D::GetPoint1Coordinate()
{
  return this->Point1Coordinate;
}

//----------------------------------------------------------------------
void vtkSliderRepresentation2D::StartWidgetInteraction(double eventPos[2])
{
  // Compute which polygon the pick is in (if any).
  int subId;
  double event[3], pcoords[3], closest[3], weights[4], dist2;
  event[0] = eventPos[0] - this->Renderer->GetOrigin()[0];
  event[1] = eventPos[1] - this->Renderer->GetOrigin()[1];
  event[2] = 0.0;

  vtkCell *sliderCell = this->SliderXForm->GetOutput()->GetCell(0);
  if ( sliderCell->EvaluatePosition(event,closest,subId,pcoords,dist2,weights) > 0 )
  {
    this->InteractionState = vtkSliderRepresentation::Slider;
    return;
  }

  vtkCell *tubeCell = this->TubeXForm->GetOutput()->GetCell(0);
  if ( tubeCell->EvaluatePosition(event,closest,subId,pcoords,dist2,weights) > 0 )
  {
    this->InteractionState = vtkSliderRepresentation::Tube;
    this->ComputePickPosition(eventPos);
    return;
  }

  vtkCell *leftCapCell = this->CapXForm->GetOutput()->GetCell(0);
  if ( leftCapCell->EvaluatePosition(event,closest,subId,pcoords,dist2,weights) > 0 )
  {
    this->InteractionState = vtkSliderRepresentation::LeftCap;
    this->PickedT = 0.0;
    return;
  }

  vtkCell *rightCapCell = this->CapXForm->GetOutput()->GetCell(1);
  if ( rightCapCell->EvaluatePosition(event,closest,subId,pcoords,dist2,weights) > 0 )
  {
    this->InteractionState = vtkSliderRepresentation::RightCap;
    this->PickedT = 1.0;
    return;
  }

  this->InteractionState = vtkSliderRepresentation::Outside;
}

//----------------------------------------------------------------------
void vtkSliderRepresentation2D::WidgetInteraction(double eventPos[2])
{
  double t = this->ComputePickPosition(eventPos);
  this->SetValue(this->MinimumValue + t*(this->MaximumValue-this->MinimumValue));
  this->BuildRepresentation();
}

//----------------------------------------------------------------------
vtkCoordinate *vtkSliderRepresentation2D::GetPoint2Coordinate()
{
  return this->Point2Coordinate;
}

//----------------------------------------------------------------------
void vtkSliderRepresentation2D::PlaceWidget(double *vtkNotUsed(bds[6]))
{
  // Position the handles at the end of the lines
  this->BuildRepresentation();
}

//----------------------------------------------------------------------
double vtkSliderRepresentation2D::ComputePickPosition(double eventPos[2])
{
  double p4[3], p5[3], p6[3], p7[3];
  double x1[3], x2[3];

  // Project the event position onto the tube axis.
  this->SliderXForm->GetOutput()->GetPoints()->GetPoint(4,p4);
  this->SliderXForm->GetOutput()->GetPoints()->GetPoint(5,p5);
  this->SliderXForm->GetOutput()->GetPoints()->GetPoint(6,p6);
  this->SliderXForm->GetOutput()->GetPoints()->GetPoint(7,p7);

  x1[0] = (p4[0] + p7[0])/2.0;
  x1[1] = (p4[1] + p7[1])/2.0;
  x1[2] = (p4[2] + p7[2])/2.0;

  x2[0] = (p5[0] + p6[0])/2.0;
  x2[1] = (p5[1] + p6[1])/2.0;
  x2[2] = (p5[2] + p6[2])/2.0;

  double event[3], closestPoint[3];
  event[0] = eventPos[0] - this->Renderer->GetOrigin()[0];
  event[1] = eventPos[1] - this->Renderer->GetOrigin()[1];
  event[2] = 0.0;

  // Intersect geometry. Don't forget to scale the pick because the tube
  // geometry is longer than the sliding region (due to the thickness of the
  // slider).
  vtkLine::DistanceToLine(event,x1,x2,this->PickedT,closestPoint);
  double scale = (2.0*this->X - 2.0*this->EndCapLength) /
    (2.0*this->X - 2.0*this->EndCapLength - this->SliderLength);
  this->PickedT = 0.5 + (this->PickedT - 0.5)*scale;
  this->PickedT = ( this->PickedT < 0 ? 0.0 :
                    (this->PickedT > 1.0 ? 1.0 : this->PickedT) );

  return this->PickedT;
}

//----------------------------------------------------------------------
void vtkSliderRepresentation2D::Highlight(int highlight)
{
  if ( highlight )
  {
    this->SliderActor->SetProperty(this->SelectedProperty);
  }
  else
  {
    this->SliderActor->SetProperty(this->SliderProperty);
  }
}


//----------------------------------------------------------------------
void vtkSliderRepresentation2D::BuildRepresentation()
{
  if ( this->GetMTime() > this->BuildTime ||
       (this->Renderer && this->Renderer->GetVTKWindow() &&
        this->Renderer->GetVTKWindow()->GetMTime() > this->BuildTime) )
  {
    int *size = this->Renderer->GetSize();
    if (0 == size[0] || 0 == size[1])
    {
      // Renderer has no size yet: wait until the next
      // BuildRepresentation...
      return;
    }

    double t = (this->Value-this->MinimumValue) / (this->MaximumValue-this->MinimumValue);

    // Setup the geometry of the widget (canonical along the x-axis).
    // Later we will transform the widget into place. We take into account the length of
    // the widget here.
    int *p1 = this->Point1Coordinate->GetComputedDisplayValue(this->Renderer);
    int *p2 = this->Point2Coordinate->GetComputedDisplayValue(this->Renderer);
    double delX = static_cast<double>(p2[0]-p1[0]);
    double delY = static_cast<double>(p2[1]-p1[1]);
    double length = sqrt ( delX*delX + delY*delY );
    length = (length <= 0.0 ? 1.0 : length);
    this->X = 0.5 * (length/size[0]);
    double theta = atan2(delY,delX);

    // Generate the points
    double x[6], y[6];
    x[0] = -this->X;
    x[1] = -this->X + this->EndCapLength;
    x[2] = x[1] + t*(2.0*X - 2.0*this->EndCapLength - this->SliderLength);
    x[3] = x[2] + this->SliderLength;
    x[4] = this->X - this->EndCapLength;
    x[5] = this->X;

    y[0] = -0.5*this->EndCapWidth;
    y[1] = -0.5*this->SliderWidth;
    y[2] = -0.5*this->TubeWidth;
    y[3] =  0.5*this->TubeWidth;
    y[4] =  0.5*this->SliderWidth;
    y[5] =  0.5*this->EndCapWidth;

    this->Points->SetPoint(0, x[0],y[0],0.0);
    this->Points->SetPoint(1, x[1],y[0],0.0);
    this->Points->SetPoint(2, x[1],y[5],0.0);
    this->Points->SetPoint(3, x[0],y[5],0.0);
    this->Points->SetPoint(4, x[1],y[2],0.0);
    this->Points->SetPoint(5, x[4],y[2],0.0);
    this->Points->SetPoint(6, x[4],y[3],0.0);
    this->Points->SetPoint(7, x[1],y[3],0.0);
    this->Points->SetPoint(8, x[2],y[1],0.0);
    this->Points->SetPoint(9, x[3],y[1],0.0);
    this->Points->SetPoint(10, x[3],y[4],0.0);
    this->Points->SetPoint(11, x[2],y[4],0.0);
    this->Points->SetPoint(12, x[4],y[0],0.0);
    this->Points->SetPoint(13, x[5],y[0],0.0);
    this->Points->SetPoint(14, x[5],y[5],0.0);
    this->Points->SetPoint(15, x[4],y[5],0.0);

    // Specify the location of the text. Because the slider can rotate
    // we have to take into account the text height and width.
    int titleSize[2];
    double textSize[2];
    double maxY = (this->SliderWidth > this->TubeWidth ?
                   (this->SliderWidth > this->EndCapWidth ? this->SliderWidth : this->EndCapWidth) :
                   (this->TubeWidth > this->EndCapWidth ? this->TubeWidth : this->EndCapWidth) );

    if ( ! this->ShowSliderLabel )
    {
      this->LabelActor->VisibilityOff();
    }
    else
    {
      this->LabelActor->VisibilityOn();
      int labelSize[2];
      char label[256];
      snprintf(label, sizeof(label), this->LabelFormat, this->Value);
      this->LabelMapper->SetInput(label);
      this->LabelProperty->SetFontSize(static_cast<int>(this->LabelHeight*size[1]));
      this->LabelMapper->GetSize(this->Renderer, labelSize);
      textSize[0] = static_cast<double>(labelSize[0])/static_cast<double>(size[0]);
      textSize[1] = static_cast<double>(labelSize[1])/static_cast<double>(size[1]);
      double radius = maxY/2.0 + textSize[1]*cos(theta) + textSize[0]*sin(theta);
      this->Points->SetPoint(16, (x[2]+x[3])/2.0, radius, 0.0); //label
    }

    this->TitleProperty->SetFontSize(static_cast<int>(this->TitleHeight*size[1]));
    this->TitleMapper->GetSize(this->Renderer, titleSize);
    textSize[0] = static_cast<double>(titleSize[0])/static_cast<double>(size[0]);
    textSize[1] = static_cast<double>(titleSize[1])/static_cast<double>(size[1]);
    double radius = maxY/2.0 + textSize[1]*cos(theta) + textSize[0]*sin(theta);
    this->Points->SetPoint(17, 0.0,-radius,0.0); //title

    // Begin transforming the slider
    double sx = static_cast<double>(size[0]);
    double sy = static_cast<double>(size[1]);

    double tx = static_cast<double>((p1[0]+p2[0])/2.0);
    double ty = static_cast<double>((p1[1]+p2[1])/2.0);

    this->XForm->Identity();
    this->XForm->Translate(tx,ty,0.0);
    this->XForm->Scale(sx,sy,1.0);
    this->XForm->RotateZ( vtkMath::DegreesFromRadians( theta ) );

    // The transform has done the work of finding the center point for the text.
    // Put the title and label at these points.
    double p16[3], p17[3];
    this->SliderXForm->Update(); //want to get the points that were transformed
    this->SliderXForm->GetOutput()->GetPoints()->GetPoint(16,p16);
    this->SliderXForm->GetOutput()->GetPoints()->GetPoint(17,p17);
    this->LabelActor->SetPosition(p16[0],p16[1]);
    this->TitleActor->SetPosition(p17[0],p17[1]);

    this->BuildTime.Modified();
  }
}

//----------------------------------------------------------------------
void vtkSliderRepresentation2D::GetActors2D(vtkPropCollection *pc)
{
  pc->AddItem(this->SliderActor);
  pc->AddItem(this->TubeActor);
  pc->AddItem(this->CapActor);
  pc->AddItem(this->LabelActor);
  pc->AddItem(this->TitleActor);
}

//----------------------------------------------------------------------
void vtkSliderRepresentation2D::ReleaseGraphicsResources(vtkWindow *w)
{
  this->SliderActor->ReleaseGraphicsResources(w);
  this->TubeActor->ReleaseGraphicsResources(w);
  this->CapActor->ReleaseGraphicsResources(w);
  this->LabelActor->ReleaseGraphicsResources(w);
  this->TitleActor->ReleaseGraphicsResources(w);
}

//----------------------------------------------------------------------
int vtkSliderRepresentation2D::RenderOpaqueGeometry(vtkViewport *viewport)
{
  this->BuildRepresentation();
  int count = this->TubeActor->RenderOpaqueGeometry(viewport);
  count += this->SliderActor->RenderOpaqueGeometry(viewport);
  count += this->CapActor->RenderOpaqueGeometry(viewport);
  count += this->LabelActor->RenderOpaqueGeometry(viewport);
  count += this->TitleActor->RenderOpaqueGeometry(viewport);
  return count;
}

//----------------------------------------------------------------------
int vtkSliderRepresentation2D::RenderOverlay(vtkViewport *viewport)
{
  this->BuildRepresentation();
  int count = this->TubeActor->RenderOverlay(viewport);
  count += this->SliderActor->RenderOverlay(viewport);
  count += this->CapActor->RenderOverlay(viewport);
  count += this->LabelActor->RenderOverlay(viewport);
  count += this->TitleActor->RenderOverlay(viewport);
  return count;
}


//----------------------------------------------------------------------
void vtkSliderRepresentation2D::PrintSelf(ostream& os, vtkIndent indent)
{
  //Superclass typedef defined in vtkTypeMacro() found in vtkSetGet.h
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Label Text: " << (this->LabelMapper->GetInput() ?
                                     this->LabelMapper->GetInput() :
                                     "(none)") << "\n";
  os << indent << "Title Text: " << (this->TitleMapper->GetInput() ?
                                     this->TitleMapper->GetInput() :
                                     "(none)") << "\n";

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

  if ( this->CapProperty )
  {
    os << indent << "CapProperty:\n";
    this->CapProperty->PrintSelf(os,indent.GetNextIndent());
  }
  else
  {
    os << indent << "CapProperty: (none)\n";
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

  if ( this->TitleProperty )
  {
    os << indent << "TitleProperty:\n";
    this->TitleProperty->PrintSelf(os,indent.GetNextIndent());
  }
  else
  {
    os << indent << "TitleProperty: (none)\n";
  }
}
