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
#include "vtkImageActor.h"
#include "vtkImageData.h"
#include "vtkCommand.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkCheckerboardRepresentation, "1.2");
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
  if ( sliderNum == vtkCheckerboardRepresentation::TopSlider )
    {
    value = this->TopRepresentation->GetValue();
    this->BottomRepresentation->SetValue(value);
    this->Checkerboard->SetNumberOfDivisions(value,numDivisions[1],1);
    }
  else if ( sliderNum == vtkCheckerboardRepresentation::RightSlider )
    {
    value = this->RightRepresentation->GetValue();
    this->LeftRepresentation->SetValue(value);
    this->Checkerboard->SetNumberOfDivisions(numDivisions[0],value,1);
    }
  else if ( sliderNum == vtkCheckerboardRepresentation::BottomSlider )
    {
    value = this->BottomRepresentation->GetValue();
    this->TopRepresentation->SetValue(value);
    this->Checkerboard->SetNumberOfDivisions(value,numDivisions[1],1);
    }
  else if ( sliderNum == vtkCheckerboardRepresentation::LeftSlider )
    {
    value = this->LeftRepresentation->GetValue();
    this->RightRepresentation->SetValue(value);
    this->Checkerboard->SetNumberOfDivisions(numDivisions[0],value,1);
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
  double o[3];
  vtkImageData *image = this->ImageActor->GetInput();
  image->Update();
  image->GetBounds(bounds);
  image->GetOrigin(o);
  if ( image->GetDataDimension() != 2 )
    {
    vtkErrorMacro(<<" requires a 2D image");
    return;
    }
  double t0 = bounds[1]-bounds[0];
  double t1 = bounds[3]-bounds[2];
  double t2 = bounds[5]-bounds[4];
  int orthoAxis = ( t0 < t1 ? (t0 < t2 ? 0 : 2) : (t1 < t2 ? 1 : 2) );
  double o0 = t0*this->CornerOffset;
  double o1 = t1*this->CornerOffset;
  double o2 = t2*this->CornerOffset;

  // Set up the initial values in the slider widgets
  int *numDivisions = this->Checkerboard->GetNumberOfDivisions();

  if ( orthoAxis == 0 ) //x-axis
    {
    this->TopRepresentation->GetPoint1Coordinate()->SetValue(o[0], o[1]+o0, o[2]+t2);
    this->TopRepresentation->GetPoint2Coordinate()->SetValue(o[0], o[1]+t1-o0, o[2]+t2);
    this->TopRepresentation->SetValue(numDivisions[1]);
    this->RightRepresentation->GetPoint1Coordinate()->SetValue(o[0], o[1]+t1, o[2]+o2);
    this->RightRepresentation->GetPoint2Coordinate()->SetValue(o[0], o[1]+t1, o[2]+t2-o2);
    this->RightRepresentation->SetValue(numDivisions[2]);
    this->BottomRepresentation->GetPoint1Coordinate()->SetValue(o[0], o[1]+o1, o[2]);
    this->BottomRepresentation->GetPoint2Coordinate()->SetValue(o[0], o[1]+t1-o1, o[2]);
    this->BottomRepresentation->SetValue(numDivisions[1]);
    this->LeftRepresentation->GetPoint1Coordinate()->SetValue(o[0], o[1], o[2]+o2);
    this->LeftRepresentation->GetPoint2Coordinate()->SetValue(o[0], o[1], o[2]+t2-o2);
    this->LeftRepresentation->SetValue(numDivisions[2]);
    }
  else if ( orthoAxis == 1 ) //y-axis
    {
    this->TopRepresentation->GetPoint1Coordinate()->SetValue(o[0]+o0, o[1], o[2]+t2);
    this->TopRepresentation->GetPoint2Coordinate()->SetValue(o[0]+t0-o0, o[1], o[2]+t2);
    this->TopRepresentation->SetValue(numDivisions[0]);
    this->RightRepresentation->GetPoint1Coordinate()->SetValue(o[0]+t0, o[1], o[2]+o2);
    this->RightRepresentation->GetPoint2Coordinate()->SetValue(o[0]+t0, o[1], o[2]+t2-o2);
    this->RightRepresentation->SetValue(numDivisions[2]);
    this->BottomRepresentation->GetPoint1Coordinate()->SetValue(o[0]+o0, o[1], o[2]);
    this->BottomRepresentation->GetPoint2Coordinate()->SetValue(o[0]+t0-o0, o[1], o[2]);
    this->BottomRepresentation->SetValue(numDivisions[0]);
    this->LeftRepresentation->GetPoint1Coordinate()->SetValue(o[0], o[1], o[2]+o2);
    this->LeftRepresentation->GetPoint2Coordinate()->SetValue(o[0], o[1], o[2]+t2-o2);
    this->LeftRepresentation->SetValue(numDivisions[2]);
    }
  else // if( orthoAxis == 2 ) //z-axis
    {
    this->TopRepresentation->GetPoint1Coordinate()->SetValue(o[0]+o0, o[1]+t1, o[2]);
    this->TopRepresentation->GetPoint2Coordinate()->SetValue(o[0]+t0-o0, o[1]+t1, o[2]);
    this->TopRepresentation->SetValue(numDivisions[0]);
    this->RightRepresentation->GetPoint1Coordinate()->SetValue(o[0]+t0, o[1]+o1, o[2]);
    this->RightRepresentation->GetPoint2Coordinate()->SetValue(o[0]+t0, o[1]+t1-o1, o[2]);
    this->RightRepresentation->SetValue(numDivisions[1]);
    this->BottomRepresentation->GetPoint1Coordinate()->SetValue(o[0]+o0, o[1], o[2]);
    this->BottomRepresentation->GetPoint2Coordinate()->SetValue(o[0]+t0-o0, o[1], o[2]);
    this->BottomRepresentation->SetValue(numDivisions[0]);
    this->LeftRepresentation->GetPoint1Coordinate()->SetValue(o[0], o[1]+o1, o[2]);
    this->LeftRepresentation->GetPoint2Coordinate()->SetValue(o[0], o[1]+t1-o1, o[2]);
    this->LeftRepresentation->SetValue(numDivisions[1]);
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

//----------------------------------------------------------------------
int vtkCheckerboardRepresentation::RenderTranslucentGeometry(vtkViewport *v)
{
  int count = this->TopRepresentation->RenderTranslucentGeometry(v);
  count += this->RightRepresentation->RenderTranslucentGeometry(v);
  count += this->BottomRepresentation->RenderTranslucentGeometry(v);
  count += this->LeftRepresentation->RenderTranslucentGeometry(v);
  return count;
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
}
