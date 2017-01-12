/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkProgressBarRepresentation.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

    This software is distributed WITHOUT ANY WARRANTY; without even
    the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
    PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkProgressBarRepresentation.h"

#include "vtkActor2D.h"
#include "vtkCellArray.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper2D.h"
#include "vtkPropCollection.h"
#include "vtkProperty2D.h"
#include "vtkTransform.h"
#include "vtkTransformPolyDataFilter.h"
#include "vtkUnsignedCharArray.h"

const double PROGRESS_BAR_WIDTH = 12;
const double PROGRESS_BAR_HEIGHT = 2;

vtkStandardNewMacro(vtkProgressBarRepresentation);

vtkProgressBarRepresentation::vtkProgressBarRepresentation()
{
  this->ProgressRate = 0;
  this->ProgressBarColor[0] = 0;
  this->ProgressBarColor[1] = 1;
  this->ProgressBarColor[2] = 0;
  this->BackgroundColor[0] = 1;
  this->BackgroundColor[1] = 1;
  this->BackgroundColor[2] = 1;
  this->DrawBackground = true;

  // Set up the geometry
  double size[2];
  this->GetSize(size);
  this->Position2Coordinate->SetValue(0.04*size[0], 0.04*size[1]);
  this->ProportionalResizeOff();
  this->Moving = 1;
  this->SetShowBorder(vtkBorderRepresentation::BORDER_ACTIVE);

  // Create the geometry in canonical coordinates
  this->Points = vtkPoints::New();
  this->Points->SetDataTypeToDouble();
  this->Points->SetNumberOfPoints(8);
  this->Points->SetPoint(0, 0.2, 0.2, 0.0);
  this->Points->SetPoint(1, 0.2, PROGRESS_BAR_HEIGHT, 0.0);
  this->Points->SetPoint(2, PROGRESS_BAR_WIDTH,
    PROGRESS_BAR_HEIGHT, 0.0);
  this->Points->SetPoint(3, PROGRESS_BAR_WIDTH, 0.2, 0.0);
  double progressPoint = 0.2 +
    (this->ProgressRate * (PROGRESS_BAR_WIDTH - 0.2));
  this->Points->SetPoint(4, 0.2, 0.2, 0.0);
  this->Points->SetPoint(5, 0.2, PROGRESS_BAR_HEIGHT, 0.0);
  this->Points->SetPoint(6, progressPoint, PROGRESS_BAR_HEIGHT, 0.0);
  this->Points->SetPoint(7, progressPoint, 0.2, 0.0);

  // Frame
  vtkNew<vtkCellArray> lines;
  vtkIdType linesIds[5] = {0, 1, 2, 3, 0};
  lines->InsertNextCell(5, linesIds);

  // Progress bar
  vtkNew<vtkCellArray> polys;
  vtkIdType polysIds[4] = {4, 5, 6, 7};
  polys->InsertNextCell(4, polysIds);

  vtkNew<vtkPolyData> polydata;
  polydata->SetPoints(this->Points);
  polydata->SetLines(lines.Get());
  polydata->SetPolys(polys.Get());

  // Create cell data to color cells
  this->ProgressBarData = vtkUnsignedCharArray::New();
  this->ProgressBarData->SetName("Color");
  this->ProgressBarData->SetNumberOfComponents(3);
  this->ProgressBarData->SetNumberOfTuples(8);
  polydata->GetPointData()->SetScalars(this->ProgressBarData);

  // Add a transform to position progress bar
  // and a mapper and actor
  vtkNew<vtkTransformPolyDataFilter> transformFilter;
  transformFilter->SetTransform(this->BWTransform);
  transformFilter->SetInputData(polydata.Get());
  vtkNew<vtkPolyDataMapper2D> mapper;
  mapper->SetInputConnection(
    transformFilter->GetOutputPort());
  this->Property = vtkProperty2D::New();
  this->Actor = vtkActor2D::New();
  this->Actor->SetMapper(mapper.Get());
  this->Actor->SetProperty(this->Property);

  // Background cell
  vtkNew<vtkCellArray> background;
  background->InsertNextCell(4, linesIds);

  // Background polydata
  vtkNew<vtkPolyData> backgroundPolydata;
  backgroundPolydata->SetPoints(this->Points);
  backgroundPolydata->SetPolys(background.Get());

  // first four points of ProgressBarData are the background
  // so we use the same array (which is good as we are using the
  // same points and there are 8 of them so we need 8 colors
  // anyhow even though our cells only use the first 4
  backgroundPolydata->GetPointData()->SetScalars(this->ProgressBarData);

  // Add transform, mapper and actor
  vtkNew<vtkTransformPolyDataFilter> backgroundTransformFilter;
  backgroundTransformFilter->SetTransform(this->BWTransform);
  backgroundTransformFilter->SetInputData(backgroundPolydata.Get());
  vtkNew<vtkPolyDataMapper2D> backgroundMapper;
  backgroundMapper->SetInputConnection(
    backgroundTransformFilter->GetOutputPort());
  this->BackgroundActor = vtkActor2D::New();
  this->BackgroundActor->SetMapper(backgroundMapper.Get());
}

//-------------------------------------------------------------------------
vtkProgressBarRepresentation::~vtkProgressBarRepresentation()
{
  this->Points->Delete();
  this->ProgressBarData->Delete();
  this->Property->Delete();
  this->Actor->Delete();
  this->BackgroundActor->Delete();
}

//-------------------------------------------------------------------------
void vtkProgressBarRepresentation::BuildRepresentation()
{
  // Reposition progress bar points
  double progressPoint = 0.2 +
    (this->ProgressRate * (PROGRESS_BAR_WIDTH - 0.2));
  this->Points->SetPoint(6, progressPoint, PROGRESS_BAR_HEIGHT, 0.0);
  this->Points->SetPoint(7, progressPoint, 0.2, 0.0);
  this->Points->Modified();

  // Set color
  double backgroundColor[3] = { this->BackgroundColor[0] * 255,
    this->BackgroundColor[1] * 255, this->BackgroundColor[2] * 255};
  double progressBarColor[3] = {this->ProgressBarColor[0] * 255,
    this->ProgressBarColor[1] * 255, this->ProgressBarColor[2] * 255};
  for (int i = 0; i < 4; i++)
  {
    this->ProgressBarData->SetTuple(i, backgroundColor);
    this->ProgressBarData->SetTuple(i+4, progressBarColor);
  }

  // Note that the transform is updated by the superclass
  this->Superclass::BuildRepresentation();
}

//-------------------------------------------------------------------------
void vtkProgressBarRepresentation::GetSize(double size[2])
{
  size[0] = PROGRESS_BAR_WIDTH + 0.2;
  size[1] = PROGRESS_BAR_HEIGHT + 0.2;
}

//-------------------------------------------------------------------------
void vtkProgressBarRepresentation::GetActors2D(vtkPropCollection *pc)
{
  if (this->DrawBackground)
  {
    pc->AddItem(this->BackgroundActor);
  }
  pc->AddItem(this->Actor);
  this->Superclass::GetActors2D(pc);
}

//-------------------------------------------------------------------------
void vtkProgressBarRepresentation::ReleaseGraphicsResources(vtkWindow *w)
{
  if (this->DrawBackground)
  {
    this->BackgroundActor->ReleaseGraphicsResources(w);
  }
  this->Actor->ReleaseGraphicsResources(w);
  this->Superclass::ReleaseGraphicsResources(w);
}

//-------------------------------------------------------------------------
int vtkProgressBarRepresentation::RenderOverlay(vtkViewport *w)
{
  int count = this->Superclass::RenderOverlay(w);
  if (this->DrawBackground)
  {
    count += this->BackgroundActor->RenderOverlay(w);
  }
  count += this->Actor->RenderOverlay(w);
  return count;
}

//-------------------------------------------------------------------------
int vtkProgressBarRepresentation::RenderOpaqueGeometry(vtkViewport *w)
{
  int count = this->Superclass::RenderOpaqueGeometry(w);
  if (this->DrawBackground)
  {
    count += this->BackgroundActor->RenderOpaqueGeometry(w);
  }
  count += this->Actor->RenderOpaqueGeometry(w);
  return count;
}

//-----------------------------------------------------------------------------
int vtkProgressBarRepresentation::RenderTranslucentPolygonalGeometry(
  vtkViewport *w)
{
  int count = this->Superclass::RenderTranslucentPolygonalGeometry(w);
  if (this->DrawBackground)
  {
    count += this->BackgroundActor->RenderTranslucentPolygonalGeometry(w);
  }
  count += this->Actor->RenderTranslucentPolygonalGeometry(w);
  return count;
}

//-----------------------------------------------------------------------------
int vtkProgressBarRepresentation::HasTranslucentPolygonalGeometry()
{
  int result = this->Superclass::HasTranslucentPolygonalGeometry();
  if (this->DrawBackground)
  {
    result |= this->BackgroundActor->HasTranslucentPolygonalGeometry();
  }
  result |= this->Actor->HasTranslucentPolygonalGeometry();
  return result;
}

//-------------------------------------------------------------------------
void vtkProgressBarRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  if (this->Property)
  {
    os << indent << "Property:\n";
    this->Property->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "Property: (none)\n";
  }
  os << indent << "ProgressRate: " << this->ProgressRate << "\n";
  os << indent << "ProgressBarColor: " << this->ProgressBarColor[0] << " "
    << this->ProgressBarColor[1] << " " <<this->ProgressBarColor[2] << "\n";
  os << indent << "DrawBackground: " << this->DrawBackground << "\n";
  os << indent << "BackgroundColor: " << this->BackgroundColor[0] << " "
    << this->BackgroundColor[1] << " " <<this->BackgroundColor[2] << "\n";
}
