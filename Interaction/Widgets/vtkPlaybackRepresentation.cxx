/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPlaybackRepresentation.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPlaybackRepresentation.h"
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


vtkStandardNewMacro(vtkPlaybackRepresentation);

vtkPlaybackRepresentation::vtkPlaybackRepresentation()
{
  // Set up the geometry
  double size[2];
  this->GetSize(size);
  this->Position2Coordinate->SetValue(0.04*size[0], 0.04*size[1]);
  this->ProportionalResize = 1;
  this->Moving = 1;
  this->SetShowBorder(vtkBorderRepresentation::BORDER_ON);

  // Create the geometry in canonical coordinates
  this->Points = vtkPoints::New();
  this->Points->SetDataTypeToDouble();
  this->Points->SetNumberOfPoints(43);
  this->Points->SetPoint(0, 0.3, 0.2, 0.0);
  this->Points->SetPoint(1, 0.3, 1.8, 0.0);
  this->Points->SetPoint(2, 0.3, 1.0, 0.0);
  this->Points->SetPoint(3, 1.0, 0.2, 0.0);
  this->Points->SetPoint(4, 1.0, 1.8, 0.0);
  this->Points->SetPoint(5, 1.0, 1.0, 0.0);
  this->Points->SetPoint(6, 1.7, 0.2, 0.0);
  this->Points->SetPoint(7, 1.7, 1.8, 0.0);
  this->Points->SetPoint(8, 2.3, 0.3, 0.0);
  this->Points->SetPoint(9, 2.5, 0.3, 0.0);
  this->Points->SetPoint(10, 2.5, 0.5, 0.0);
  this->Points->SetPoint(11, 2.3, 0.5, 0.0);
  this->Points->SetPoint(12, 2.2, 1.0, 0.0);
  this->Points->SetPoint(13, 3.0, 0.2, 0.0);
  this->Points->SetPoint(14, 3.0, 1.8, 0.0);
  this->Points->SetPoint(15, 3.0, 1.0, 0.0);
  this->Points->SetPoint(16, 3.8, 0.2, 0.0);
  this->Points->SetPoint(17, 3.8, 1.8, 0.0);
  this->Points->SetPoint(18, 4.5, 0.3, 0.0);
  this->Points->SetPoint(19, 5.7, 0.3, 0.0);
  this->Points->SetPoint(20, 5.7, 1.7, 0.0);
  this->Points->SetPoint(21, 4.5, 1.7, 0.0);
  this->Points->SetPoint(22, 6.5, 0.3, 0.0);
  this->Points->SetPoint(23, 7.7, 1.0, 0.0);
  this->Points->SetPoint(24, 6.5, 1.7, 0.0);
  this->Points->SetPoint(25, 8.2, 0.2, 0.0);
  this->Points->SetPoint(26, 9.0, 1.0, 0.0);
  this->Points->SetPoint(27, 8.2, 1.8, 0.0);
  this->Points->SetPoint(28, 9.0, 0.2, 0.0);
  this->Points->SetPoint(29, 9.8, 1.0, 0.0);
  this->Points->SetPoint(30, 9.0, 1.8, 0.0);
  this->Points->SetPoint(31, 9.7, 0.3, 0.0);
  this->Points->SetPoint(32, 9.9, 0.3, 0.0);
  this->Points->SetPoint(33, 9.9, 0.5, 0.0);
  this->Points->SetPoint(34, 9.7, 0.5, 0.0);
  this->Points->SetPoint(35, 10.3, 0.2, 0.0);
  this->Points->SetPoint(36, 11.0, 1.0, 0.0);
  this->Points->SetPoint(37, 10.3, 1.8, 0.0);
  this->Points->SetPoint(38, 11.0, 0.2, 0.0);
  this->Points->SetPoint(39, 11.7, 1.0, 0.0);
  this->Points->SetPoint(40, 11.0, 1.8, 0.0);
  this->Points->SetPoint(41, 11.7, 0.2, 0.0);
  this->Points->SetPoint(42, 11.7, 1.8, 0.0);

  vtkCellArray *lines = vtkCellArray::New();
  lines->InsertNextCell(2); //left jump
  lines->InsertCellPoint(0);
  lines->InsertCellPoint(1);
  lines->InsertNextCell(5); //left frame
  lines->InsertCellPoint(8);
  lines->InsertCellPoint(9);
  lines->InsertCellPoint(10);
  lines->InsertCellPoint(11);
  lines->InsertCellPoint(8);
  lines->InsertNextCell(5); //right frame
  lines->InsertCellPoint(31);
  lines->InsertCellPoint(32);
  lines->InsertCellPoint(33);
  lines->InsertCellPoint(34);
  lines->InsertCellPoint(31);
  lines->InsertNextCell(2); //right jump
  lines->InsertCellPoint(41);
  lines->InsertCellPoint(42);

  vtkCellArray *polys = vtkCellArray::New();
  polys->InsertNextCell(3); //left jump
  polys->InsertCellPoint(2);
  polys->InsertCellPoint(3);
  polys->InsertCellPoint(4);
  polys->InsertNextCell(3);
  polys->InsertCellPoint(5);
  polys->InsertCellPoint(6);
  polys->InsertCellPoint(7);
  polys->InsertNextCell(3); //left frame
  polys->InsertCellPoint(12);
  polys->InsertCellPoint(13);
  polys->InsertCellPoint(14);
  polys->InsertNextCell(3);
  polys->InsertCellPoint(15);
  polys->InsertCellPoint(16);
  polys->InsertCellPoint(17);
  polys->InsertNextCell(4); //stop
  polys->InsertCellPoint(18);
  polys->InsertCellPoint(19);
  polys->InsertCellPoint(20);
  polys->InsertCellPoint(21);
  polys->InsertNextCell(3); //play
  polys->InsertCellPoint(22);
  polys->InsertCellPoint(23);
  polys->InsertCellPoint(24);
  polys->InsertNextCell(3); //right frame
  polys->InsertCellPoint(25);
  polys->InsertCellPoint(26);
  polys->InsertCellPoint(27);
  polys->InsertNextCell(3);
  polys->InsertCellPoint(28);
  polys->InsertCellPoint(29);
  polys->InsertCellPoint(30);
  polys->InsertNextCell(3); //right jump
  polys->InsertCellPoint(35);
  polys->InsertCellPoint(36);
  polys->InsertCellPoint(37);
  polys->InsertNextCell(3);
  polys->InsertCellPoint(38);
  polys->InsertCellPoint(39);
  polys->InsertCellPoint(40);

  this->PolyData = vtkPolyData::New();
  this->PolyData->SetPoints(this->Points);
  this->PolyData->SetLines(lines);
  this->PolyData->SetPolys(polys);
  lines->Delete();
  polys->Delete();

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
vtkPlaybackRepresentation::~vtkPlaybackRepresentation()
{
  this->Points->Delete();
  this->TransformFilter->Delete();
  this->PolyData->Delete();
  this->Mapper->Delete();
  this->Property->Delete();
  this->Actor->Delete();
}

//-------------------------------------------------------------------------
void vtkPlaybackRepresentation::BuildRepresentation()
{
  // Note that the transform is updated by the superclass
  this->Superclass::BuildRepresentation();
}

//-------------------------------------------------------------------------
void vtkPlaybackRepresentation::GetActors2D(vtkPropCollection *pc)
{
  pc->AddItem(this->Actor);
  this->Superclass::GetActors2D(pc);
}

//-------------------------------------------------------------------------
void vtkPlaybackRepresentation::ReleaseGraphicsResources(vtkWindow *w)
{
  this->Actor->ReleaseGraphicsResources(w);
  this->Superclass::ReleaseGraphicsResources(w);
}

//-------------------------------------------------------------------------
int vtkPlaybackRepresentation::RenderOverlay(vtkViewport *w)
{
  int count = this->Superclass::RenderOverlay(w);
  count += this->Actor->RenderOverlay(w);
  return count;
}

//-------------------------------------------------------------------------
int vtkPlaybackRepresentation::RenderOpaqueGeometry(vtkViewport *w)
{
  int count = this->Superclass::RenderOpaqueGeometry(w);
  count += this->Actor->RenderOpaqueGeometry(w);
  return count;
}

//-----------------------------------------------------------------------------
int vtkPlaybackRepresentation::RenderTranslucentPolygonalGeometry(
  vtkViewport *w)
{
  int count = this->Superclass::RenderTranslucentPolygonalGeometry(w);
  count += this->Actor->RenderTranslucentPolygonalGeometry(w);
  return count;
}

//-----------------------------------------------------------------------------
// Description:
// Does this prop have some translucent polygonal geometry?
int vtkPlaybackRepresentation::HasTranslucentPolygonalGeometry()
{
  int result = this->Superclass::HasTranslucentPolygonalGeometry();
  result |= this->Actor->HasTranslucentPolygonalGeometry();
  return result;
}

//-------------------------------------------------------------------------
void vtkPlaybackRepresentation::PrintSelf(ostream& os, vtkIndent indent)
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

}
