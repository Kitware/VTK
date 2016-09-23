/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCheckerboardRepresentation.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCheckerboardRepresentation.h"
#include "vtkSliderRepresentation3D.h"
#include "vtkImageCheckerboard.h"
#include "vtkImageMapper3D.h"
#include "vtkImageActor.h"
#include "vtkImageData.h"
#include "vtkCommand.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkCheckerboardRepresentation);

vtkCxxSetObjectMacro(vtkCheckerboardRepresentation,Checkerboard,vtkImageCheckerboard);
vtkCxxSetObjectMacro(vtkCheckerboardRepresentation,ImageActor,vtkImageActor);

vtkCxxSetObjectMacro(vtkCheckerboardRepresentation,TopRepresentation,vtkSliderRepresentation3D);
vtkCxxSetObjectMacro(vtkCheckerboardRepresentation,RightRepresentation,vtkSliderRepresentation3D);
vtkCxxSetObjectMacro(vtkCheckerboardRepresentation,BottomRepresentation,vtkSliderRepresentation3D);
vtkCxxSetObjectMacro(vtkCheckerboardRepresentation,LeftRepresentation,vtkSliderRepresentation3D);


//----------------------------------------------------------------------
vtkCheckerboardRepresentation::vtkCheckerboardRepresentation()
{
  this->Checkerboard = NULL;
  this->ImageActor = NULL;

  this->TopRepresentation = vtkSliderRepresentation3D::New();
  this->TopRepresentation->ShowSliderLabelOff();
  this->TopRepresentation->SetTitleText(NULL);
  this->TopRepresentation->GetPoint1Coordinate()->SetCoordinateSystemToWorld();
  this->TopRepresentation->GetPoint2Coordinate()->SetCoordinateSystemToWorld();
  this->TopRepresentation->SetSliderLength(0.050);
  this->TopRepresentation->SetSliderWidth(0.025);
  this->TopRepresentation->SetTubeWidth(0.015);
  this->TopRepresentation->SetEndCapLength(0.0);
  this->TopRepresentation->SetMinimumValue(1);
  this->TopRepresentation->SetMaximumValue(10);
  this->TopRepresentation->SetSliderShapeToCylinder();

  this->RightRepresentation = vtkSliderRepresentation3D::New();
  this->RightRepresentation->ShowSliderLabelOff();
  this->RightRepresentation->SetTitleText(NULL);
  this->RightRepresentation->GetPoint1Coordinate()->SetCoordinateSystemToWorld();
  this->RightRepresentation->GetPoint2Coordinate()->SetCoordinateSystemToWorld();
  this->RightRepresentation->SetSliderLength(0.050);
  this->RightRepresentation->SetSliderWidth(0.025);
  this->RightRepresentation->SetTubeWidth(0.015);
  this->RightRepresentation->SetEndCapLength(0.0);
  this->RightRepresentation->SetMinimumValue(1);
  this->RightRepresentation->SetMaximumValue(10);
  this->RightRepresentation->SetSliderShapeToCylinder();

  this->BottomRepresentation = vtkSliderRepresentation3D::New();
  this->BottomRepresentation->ShowSliderLabelOff();
  this->BottomRepresentation->SetTitleText(NULL);
  this->BottomRepresentation->GetPoint1Coordinate()->SetCoordinateSystemToWorld();
  this->BottomRepresentation->GetPoint2Coordinate()->SetCoordinateSystemToWorld();
  this->BottomRepresentation->SetSliderLength(0.050);
  this->BottomRepresentation->SetSliderWidth(0.025);
  this->BottomRepresentation->SetTubeWidth(0.015);
  this->BottomRepresentation->SetEndCapLength(0.0);
  this->BottomRepresentation->SetMinimumValue(1);
  this->BottomRepresentation->SetMaximumValue(10);
  this->BottomRepresentation->SetSliderShapeToCylinder();

  this->LeftRepresentation = vtkSliderRepresentation3D::New();
  this->LeftRepresentation->ShowSliderLabelOff();
  this->LeftRepresentation->SetTitleText(NULL);
  this->LeftRepresentation->GetPoint1Coordinate()->SetCoordinateSystemToWorld();
  this->LeftRepresentation->GetPoint2Coordinate()->SetCoordinateSystemToWorld();
  this->LeftRepresentation->SetSliderLength(0.050);
  this->LeftRepresentation->SetSliderWidth(0.025);
  this->LeftRepresentation->SetTubeWidth(0.015);
  this->LeftRepresentation->SetEndCapLength(0.0);
  this->LeftRepresentation->SetMinimumValue(1);
  this->LeftRepresentation->SetMaximumValue(10);
  this->LeftRepresentation->SetSliderShapeToCylinder();

  this->CornerOffset = 0.00;
  this->OrthoAxis = 2;
}

//----------------------------------------------------------------------
vtkCheckerboardRepresentation::~vtkCheckerboardRepresentation()
{
  if ( this->Checkerboard )
  {
    this->Checkerboard->Delete();
  }
  if ( this->ImageActor )
  {
    this->ImageActor->Delete();
  }

  this->TopRepresentation->Delete();
  this->RightRepresentation->Delete();
  this->BottomRepresentation->Delete();
  this->LeftRepresentation->Delete();
}

//----------------------------------------------------------------------
void vtkCheckerboardRepresentation::SliderValueChanged(int sliderNum)
{
  int *numDivisions = this->Checkerboard->GetNumberOfDivisions();
  int value;
  int div[] = {1,1,1};

  if ( sliderNum == vtkCheckerboardRepresentation::TopSlider )
  {
    value = static_cast<int>(this->TopRepresentation->GetValue());
    this->BottomRepresentation->SetValue(this->TopRepresentation->GetValue());
    switch ( this->OrthoAxis )
    {
      case 0: div[1] = value; div[2] = numDivisions[2]; break;
      case 1: div[0] = value; div[2] = numDivisions[2]; break;
      case 2: div[0] = value; div[1] = numDivisions[1]; break;
    }

    this->Checkerboard->SetNumberOfDivisions(div);
  }
  else if ( sliderNum == vtkCheckerboardRepresentation::RightSlider )
  {
    value = static_cast<int>(this->RightRepresentation->GetValue());
    this->LeftRepresentation->SetValue(this->RightRepresentation->GetValue());
    switch ( this->OrthoAxis )
    {
      case 0: div[1] = numDivisions[1]; div[2] = value; break;
      case 1: div[0] = numDivisions[0]; div[2] = value; break;
      case 2: div[0] = numDivisions[0]; div[1] = value; break;
    }

    this->Checkerboard->SetNumberOfDivisions(div);
  }
  else if ( sliderNum == vtkCheckerboardRepresentation::BottomSlider )
  {
    value = static_cast<int>(this->BottomRepresentation->GetValue());
    this->TopRepresentation->SetValue(this->BottomRepresentation->GetValue());
    switch ( this->OrthoAxis )
    {
      case 0: div[1] = value; div[2] = numDivisions[2]; break;
      case 1: div[0] = value; div[2] = numDivisions[2]; break;
      case 2: div[0] = value; div[1] = numDivisions[1]; break;
    }

    this->Checkerboard->SetNumberOfDivisions(div);
  }
  else if ( sliderNum == vtkCheckerboardRepresentation::LeftSlider )
  {
    value = static_cast<int>(this->LeftRepresentation->GetValue());
    this->RightRepresentation->SetValue(this->LeftRepresentation->GetValue());
    switch ( this->OrthoAxis )
    {
      case 0: div[1] = numDivisions[1]; div[2] = value; break;
      case 1: div[0] = numDivisions[0]; div[2] = value; break;
      case 2: div[0] = numDivisions[0]; div[1] = value; break;
    }

    this->Checkerboard->SetNumberOfDivisions(div);
  }
}

//----------------------------------------------------------------------
void vtkCheckerboardRepresentation::BuildRepresentation()
{
  // Make sure that the checkerboard is up to date
  if ( !this->Checkerboard || !this->ImageActor )
  {
    vtkErrorMacro("requires a checkerboard and image actor");
    return;
  }

  double bounds[6];
  vtkImageData *image = this->ImageActor->GetInput();
  this->ImageActor->GetMapper()->GetInputAlgorithm()->Update();
  image->GetBounds(bounds);
  if ( image->GetDataDimension() != 2 )
  {
    vtkErrorMacro(<<" requires a 2D image");
    return;
  }
  double t0 = bounds[1]-bounds[0];
  double t1 = bounds[3]-bounds[2];
  double t2 = bounds[5]-bounds[4];
  this->OrthoAxis = ( t0 < t1 ? (t0 < t2 ? 0 : 2) : (t1 < t2 ? 1 : 2) );
  double o0 = t0*this->CornerOffset;
  double o1 = t1*this->CornerOffset;
  double o2 = t2*this->CornerOffset;

  // Set up the initial values in the slider widgets
  int *numDivisions = this->Checkerboard->GetNumberOfDivisions();

  if ( this->OrthoAxis == 0 ) //x-axis
  {
    // point1 and point2 are switched for top and bottom in case a
    // user wants to see the slider label positions as text, and
    // rotation of the text about the slider's local x-axis must be
    // set correctly.  Similar logic applies to X-Z plane case.

    this->TopRepresentation->GetPoint2Coordinate()->SetValue(bounds[0], bounds[2]+o1, bounds[5]);
    this->TopRepresentation->GetPoint1Coordinate()->SetValue(bounds[0], bounds[3]-o1, bounds[5]);
    this->TopRepresentation->SetValue(numDivisions[1]);
    this->TopRepresentation->SetRotation(90.0);

    this->RightRepresentation->GetPoint1Coordinate()->SetValue(bounds[0], bounds[3], bounds[4]+o2);
    this->RightRepresentation->GetPoint2Coordinate()->SetValue(bounds[0], bounds[3], bounds[5]-o2);
    this->RightRepresentation->SetValue(numDivisions[2]);
    this->RightRepresentation->SetRotation(0.0);

    this->BottomRepresentation->GetPoint2Coordinate()->SetValue(bounds[0], bounds[2]+o1, bounds[4]);
    this->BottomRepresentation->GetPoint1Coordinate()->SetValue(bounds[0], bounds[3]-o1, bounds[4]);
    this->BottomRepresentation->SetValue(numDivisions[1]);
    this->BottomRepresentation->SetRotation(90.0);

    this->LeftRepresentation->GetPoint1Coordinate()->SetValue(bounds[0], bounds[2], bounds[4]+o2);
    this->LeftRepresentation->GetPoint2Coordinate()->SetValue(bounds[0], bounds[2], bounds[5]-o2);
    this->LeftRepresentation->SetValue(numDivisions[2]);
    this->LeftRepresentation->SetRotation(0.0);
  }
  else if ( this->OrthoAxis == 1 ) //y-axis
  {
    this->TopRepresentation->GetPoint1Coordinate()->SetValue(bounds[0]+o0, bounds[2], bounds[5]);
    this->TopRepresentation->GetPoint2Coordinate()->SetValue(bounds[1]-o0, bounds[2], bounds[5]);
    this->TopRepresentation->SetValue(numDivisions[0]);
    this->TopRepresentation->SetRotation(90.0);

    this->RightRepresentation->GetPoint1Coordinate()->SetValue(bounds[1], bounds[2], bounds[4]+o2);
    this->RightRepresentation->GetPoint2Coordinate()->SetValue(bounds[1], bounds[2], bounds[5]-o2);
    this->RightRepresentation->SetValue(numDivisions[2]);
    this->RightRepresentation->SetRotation(90.0);

    this->BottomRepresentation->GetPoint1Coordinate()->SetValue(bounds[0]+o0, bounds[2], bounds[4]);
    this->BottomRepresentation->GetPoint2Coordinate()->SetValue(bounds[1]-o0, bounds[2], bounds[4]);
    this->BottomRepresentation->SetValue(numDivisions[0]);
    this->BottomRepresentation->SetRotation(90.0);

    this->LeftRepresentation->GetPoint1Coordinate()->SetValue(bounds[0], bounds[2], bounds[4]+o2);
    this->LeftRepresentation->GetPoint2Coordinate()->SetValue(bounds[0], bounds[2], bounds[5]-o2);
    this->LeftRepresentation->SetValue(numDivisions[2]);
    this->LeftRepresentation->SetRotation(90.0);
  }
  else // if( orthoAxis == 2 ) //z-axis
  {
    this->TopRepresentation->GetPoint1Coordinate()->SetValue(bounds[0]+o0, bounds[3], bounds[4]);
    this->TopRepresentation->GetPoint2Coordinate()->SetValue(bounds[1]-o0, bounds[3], bounds[4]);
    this->TopRepresentation->SetValue(numDivisions[0]);
    this->TopRepresentation->SetRotation(0.0);

    this->RightRepresentation->GetPoint1Coordinate()->SetValue(bounds[1], bounds[2]+o1, bounds[4]);
    this->RightRepresentation->GetPoint2Coordinate()->SetValue(bounds[1], bounds[3]-o1, bounds[4]);
    this->RightRepresentation->SetValue(numDivisions[1]);
    this->RightRepresentation->SetRotation(0.0);

    this->BottomRepresentation->GetPoint1Coordinate()->SetValue(bounds[0]+o0, bounds[2], bounds[4]);
    this->BottomRepresentation->GetPoint2Coordinate()->SetValue(bounds[1]-o0, bounds[2], bounds[4]);
    this->BottomRepresentation->SetValue(numDivisions[0]);
    this->BottomRepresentation->SetRotation(0.0);

    this->LeftRepresentation->GetPoint1Coordinate()->SetValue(bounds[0], bounds[2]+o1, bounds[4]);
    this->LeftRepresentation->GetPoint2Coordinate()->SetValue(bounds[0], bounds[3]-o1, bounds[4]);
    this->LeftRepresentation->SetValue(numDivisions[1]);
    this->LeftRepresentation->SetRotation(0.0);
  }

  this->TopRepresentation->BuildRepresentation();
  this->RightRepresentation->BuildRepresentation();
  this->BottomRepresentation->BuildRepresentation();
  this->LeftRepresentation->BuildRepresentation();
}

//----------------------------------------------------------------------
void vtkCheckerboardRepresentation::GetActors(vtkPropCollection *pc)
{
  this->TopRepresentation->GetActors(pc);
  this->RightRepresentation->GetActors(pc);
  this->BottomRepresentation->GetActors(pc);
  this->LeftRepresentation->GetActors(pc);
}

//----------------------------------------------------------------------
void vtkCheckerboardRepresentation::ReleaseGraphicsResources(vtkWindow *w)
{
  this->TopRepresentation->ReleaseGraphicsResources(w);
  this->RightRepresentation->ReleaseGraphicsResources(w);
  this->BottomRepresentation->ReleaseGraphicsResources(w);
  this->LeftRepresentation->ReleaseGraphicsResources(w);
}

//----------------------------------------------------------------------
int vtkCheckerboardRepresentation::RenderOverlay(vtkViewport *v)
{
  int count = this->TopRepresentation->RenderOverlay(v);
  count += this->RightRepresentation->RenderOverlay(v);
  count += this->BottomRepresentation->RenderOverlay(v);
  count += this->LeftRepresentation->RenderOverlay(v);
  return count;
}

//----------------------------------------------------------------------
int vtkCheckerboardRepresentation::RenderOpaqueGeometry(vtkViewport *v)
{
  int count = this->TopRepresentation->RenderOpaqueGeometry(v);
  count += this->RightRepresentation->RenderOpaqueGeometry(v);
  count += this->BottomRepresentation->RenderOpaqueGeometry(v);
  count += this->LeftRepresentation->RenderOpaqueGeometry(v);
  return count;
}

//-----------------------------------------------------------------------------
int vtkCheckerboardRepresentation::RenderTranslucentPolygonalGeometry(
  vtkViewport *v)
{
  int count = this->TopRepresentation->RenderTranslucentPolygonalGeometry(v);
  count += this->RightRepresentation->RenderTranslucentPolygonalGeometry(v);
  count += this->BottomRepresentation->RenderTranslucentPolygonalGeometry(v);
  count += this->LeftRepresentation->RenderTranslucentPolygonalGeometry(v);
  return count;
}
//-----------------------------------------------------------------------------
int vtkCheckerboardRepresentation::HasTranslucentPolygonalGeometry()
{
  int result = this->TopRepresentation->HasTranslucentPolygonalGeometry();
  result |= this->RightRepresentation->HasTranslucentPolygonalGeometry();
  result |= this->BottomRepresentation->HasTranslucentPolygonalGeometry();
  result |= this->LeftRepresentation->HasTranslucentPolygonalGeometry();
  return result;
}

//----------------------------------------------------------------------
void vtkCheckerboardRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  //Superclass typedef defined in vtkTypeMacro() found in vtkSetGet.h
  this->Superclass::PrintSelf(os,indent);

  if ( this->ImageActor )
  {
    os << indent << "Image Actor: " << this->ImageActor << "\n";
  }
  else
  {
    os << indent << "Image Actor: (none)\n";
  }

  if ( this->Checkerboard )
  {
    os << indent << "Checkerboard: " << this->Checkerboard << "\n";
  }
  else
  {
    os << indent << "Image Checkerboard: (none)\n";
  }

  os << indent << "Corner Offset: " << this->CornerOffset << "\n";

  os << indent << "Top Representation\n";
  this->TopRepresentation->PrintSelf(os,indent.GetNextIndent());

  os << indent << "Bottom Representation\n";
  this->BottomRepresentation->PrintSelf(os,indent.GetNextIndent());

  os << indent << "Right Representation\n";
  this->RightRepresentation->PrintSelf(os,indent.GetNextIndent());

  os << indent << "Left Representation\n";
  this->LeftRepresentation->PrintSelf(os,indent.GetNextIndent());

}
