/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCameraRepresentation.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCameraRepresentation.h"
#include "vtkCameraInterpolator.h"
#include "vtkCallbackCommand.h"
#include "vtkObjectFactory.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkCamera.h"
#include "vtkPoints.h"
#include "vtkCellArray.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper2D.h"
#include "vtkProperty2D.h"
#include "vtkActor2D.h"
#include "vtkTransform.h"
#include "vtkTransformPolyDataFilter.h"


vtkStandardNewMacro(vtkCameraRepresentation);

vtkCxxSetObjectMacro(vtkCameraRepresentation, Camera, vtkCamera);
vtkCxxSetObjectMacro(vtkCameraRepresentation, Interpolator, vtkCameraInterpolator);

//-------------------------------------------------------------------------
vtkCameraRepresentation::vtkCameraRepresentation()
{
  this->Camera = NULL;
  this->Interpolator = vtkCameraInterpolator::New();
  this->NumberOfFrames = 24;

  // Set up the
  double size[2];
  this->GetSize(size);
  this->Position2Coordinate->SetValue(0.04*size[0], 0.04*size[1]);
  this->ProportionalResize = 1;
  this->Moving = 1;
  this->SetShowBorder(vtkBorderRepresentation::BORDER_ON);

  // Create the geometry in canonical coordinates
  this->Points = vtkPoints::New();
  this->Points->SetDataTypeToDouble();
  this->Points->SetNumberOfPoints(25);
  this->Points->SetPoint(0, 0.0, 0.0, 0.0);
  this->Points->SetPoint(1, 6.0, 0.0, 0.0);
  this->Points->SetPoint(2, 6.0, 2.0, 0.0);
  this->Points->SetPoint(3, 0.0, 2.0, 0.0);
  this->Points->SetPoint(4, 0.375, 0.25, 0.0);
  this->Points->SetPoint(5, 1.0, 0.25, 0.0);
  this->Points->SetPoint(6, 1.0, 1.75, 0.0);
  this->Points->SetPoint(7, 0.375, 1.75, 0.0);
  this->Points->SetPoint(8, 1.0, 0.875, 0.0);
  this->Points->SetPoint(9, 1.25, 0.75, 0.0);
  this->Points->SetPoint(10, 1.5, 0.75, 0.0);
  this->Points->SetPoint(11, 1.5, 1.25, 0.0);
  this->Points->SetPoint(12, 1.25, 1.25, 0.0);
  this->Points->SetPoint(13, 1.0, 1.125, 0.0);
  this->Points->SetPoint(14, 2.5, 0.5, 0.0);
  this->Points->SetPoint(15, 3.5, 1.0, 0.0);
  this->Points->SetPoint(16, 2.5, 1.5, 0.0);
  this->Points->SetPoint(17, 4.625, 0.375, 0.0);
  this->Points->SetPoint(18, 5.625, 0.375, 0.0);
  this->Points->SetPoint(19, 5.75, 0.5, 0.0);
  this->Points->SetPoint(20, 5.75, 1.5, 0.0);
  this->Points->SetPoint(21, 5.625, 1.625, 0.0);
  this->Points->SetPoint(22, 4.625, 1.625, 0.0);
  this->Points->SetPoint(23, 4.5, 1.5, 0.0);
  this->Points->SetPoint(24, 4.5, 0.5, 0.0);

  vtkCellArray *cells = vtkCellArray::New();
  cells->InsertNextCell(4); //camera body
  cells->InsertCellPoint(4);
  cells->InsertCellPoint(5);
  cells->InsertCellPoint(6);
  cells->InsertCellPoint(7);
  cells->InsertNextCell(6); //camera lens
  cells->InsertCellPoint(8);
  cells->InsertCellPoint(9);
  cells->InsertCellPoint(10);
  cells->InsertCellPoint(11);
  cells->InsertCellPoint(12);
  cells->InsertCellPoint(13);
  cells->InsertNextCell(3); //play button
  cells->InsertCellPoint(14);
  cells->InsertCellPoint(15);
  cells->InsertCellPoint(16);
  cells->InsertNextCell(4); //part of delete button
  cells->InsertCellPoint(17);
  cells->InsertCellPoint(20);
  cells->InsertCellPoint(21);
  cells->InsertCellPoint(24);
  cells->InsertNextCell(4); //part of delete button
  cells->InsertCellPoint(18);
  cells->InsertCellPoint(19);
  cells->InsertCellPoint(22);
  cells->InsertCellPoint(23);
  this->PolyData = vtkPolyData::New();
  this->PolyData->SetPoints(this->Points);
  this->PolyData->SetPolys(cells);
  cells->Delete();

  this->TransformFilter = vtkTransformPolyDataFilter::New();
  this->TransformFilter->SetTransform(this->BWTransform);
  this->TransformFilter->SetInputData(this->PolyData);

  this->Mapper = vtkPolyDataMapper2D::New();
  this->Mapper->SetInputConnection(
    this->TransformFilter->GetOutputPort());
  this->Property = vtkProperty2D::New();
  this->Actor = vtkActor2D::New();
  this->Actor->SetMapper(this->Mapper);
  this->Actor->SetProperty(this->Property);
}

//-------------------------------------------------------------------------
vtkCameraRepresentation::~vtkCameraRepresentation()
{
  this->SetCamera(0);
  this->SetInterpolator(0);

  this->Points->Delete();
  this->TransformFilter->Delete();
  this->PolyData->Delete();
  this->Mapper->Delete();
  this->Property->Delete();
  this->Actor->Delete();
}

//-------------------------------------------------------------------------
void vtkCameraRepresentation::BuildRepresentation()
{
  // Note that the transform is updated by the superclass
  this->Superclass::BuildRepresentation();
}

//-------------------------------------------------------------------------
void vtkCameraRepresentation::AddCameraToPath()
{
  if ( ! this->Camera )
    {
    return;
    }
  if ( ! this->Interpolator )
    {
    this->Interpolator = vtkCameraInterpolator::New();
    }
  this->CurrentTime = static_cast<double>(
    this->Interpolator->GetNumberOfCameras());
  this->Interpolator->AddCamera(this->CurrentTime,this->Camera);
}


//-------------------------------------------------------------------------
void vtkCameraRepresentation::AnimatePath(vtkRenderWindowInteractor *rwi)
{
  vtkCameraInterpolator *camInt = this->Interpolator;

  if ( ! camInt || ! rwi )
    {
    return;
    }

  int numCameras = camInt->GetNumberOfCameras();
  if ( numCameras <= 0 )
    {
    return;
    }
  double delT = static_cast<double>(numCameras - 1) / this->NumberOfFrames;

  double t=0.0;
  for (int i=0; i < this->NumberOfFrames; i++, t+=delT)
    {
    camInt->InterpolateCamera(t,this->Camera);
    rwi->Render();
    }
}

//-------------------------------------------------------------------------
void vtkCameraRepresentation::InitializePath()
{
  if ( ! this->Interpolator )
    {
    return;
    }
  this->Interpolator->Initialize();
  this->CurrentTime = 0.0;
}

//-------------------------------------------------------------------------
void vtkCameraRepresentation::GetActors2D(vtkPropCollection *pc)
{
  pc->AddItem(this->Actor);
  this->Superclass::GetActors2D(pc);
}

//-------------------------------------------------------------------------
void vtkCameraRepresentation::ReleaseGraphicsResources(vtkWindow *w)
{
  this->Actor->ReleaseGraphicsResources(w);
  this->Superclass::ReleaseGraphicsResources(w);
}

//-------------------------------------------------------------------------
int vtkCameraRepresentation::RenderOverlay(vtkViewport *w)
{
  int count = this->Superclass::RenderOverlay(w);
  count += this->Actor->RenderOverlay(w);
  return count;
}

//-------------------------------------------------------------------------
int vtkCameraRepresentation::RenderOpaqueGeometry(vtkViewport *w)
{
  int count = this->Superclass::RenderOpaqueGeometry(w);
  count += this->Actor->RenderOpaqueGeometry(w);
  return count;
}

//-------------------------------------------------------------------------
int vtkCameraRepresentation::RenderTranslucentPolygonalGeometry(vtkViewport *w)
{
  int count = this->Superclass::RenderTranslucentPolygonalGeometry(w);
  count += this->Actor->RenderTranslucentPolygonalGeometry(w);
  return count;
}

//-----------------------------------------------------------------------------
// Description:
// Does this prop have some translucent polygonal geometry?
int vtkCameraRepresentation::HasTranslucentPolygonalGeometry()
{
  int result = this->Superclass::HasTranslucentPolygonalGeometry();
  result |= this->Actor->HasTranslucentPolygonalGeometry();
  return result;
}

//-------------------------------------------------------------------------
void vtkCameraRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  if ( this->Property )
    {
    os << indent << "Property:\n";
    this->Property->PrintSelf(os,indent.GetNextIndent());
    }
  else
    {
    os << indent << "Property: (none)\n";
    }

  os << indent << "Camera Interpolator: " << this->Interpolator << "\n";
  os << indent << "Camera: " << this->Camera << "\n";
  os << indent << "Number of Frames: " << this->NumberOfFrames << "\n";

}
