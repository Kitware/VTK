/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageCroppingRegionsWidget.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageCroppingRegionsWidget.h"
#include "vtkObjectFactory.h"
#include "vtkActor2D.h"
#include "vtkCallbackCommand.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkImageData.h"
#include "vtkLineSource.h"
#include "vtkPolyDataMapper2D.h"
#include "vtkPolyData.h"
#include "vtkCoordinate.h"
#include "vtkProperty2D.h"
#include "vtkCamera.h"
#include "vtkVolumeMapper.h"
#include "vtkImageCroppingRegionsWidget.h"

vtkStandardNewMacro(vtkImageCroppingRegionsWidget);


//----------------------------------------------------------------------------
vtkImageCroppingRegionsWidget::vtkImageCroppingRegionsWidget()
{
  this->PlaceFactor = 1.0;

  int i;

  this->EventCallbackCommand->SetCallback(
    vtkImageCroppingRegionsWidget::ProcessEvents);

  for (i = 0; i < 4; i++)
    {
    this->LineSources[i] = vtkLineSource::New();
    this->LineActors[i] = vtkActor2D::New();
    vtkPolyDataMapper2D *pdm = vtkPolyDataMapper2D::New();
    vtkCoordinate *tcoord = vtkCoordinate::New();
    tcoord->SetCoordinateSystemToWorld();
    pdm->SetTransformCoordinate(tcoord);
    tcoord->Delete();
    this->LineActors[i]->SetMapper(pdm);
    this->LineActors[i]->GetProperty()->SetColor(1, 1, 1);
    pdm->SetInput(this->LineSources[i]->GetOutput());
    pdm->Delete();
    }

  vtkPoints *points = vtkPoints::New();
  points->Allocate(16);
  for (i = 0; i < 16; i++)
    {
    points->InsertNextPoint(0.0, 0.0, 0.0);
    }

  for (i = 0; i < 9; i++)
    {
    this->RegionPolyData[i] = vtkPolyData::New();
    this->RegionPolyData[i]->Allocate(1, 1);
    this->RegionPolyData[i]->SetPoints(points);
    }

  points->Delete();

  vtkIdType ptIds[4];

  ptIds[0] = 0; ptIds[1] = 1; ptIds[2] = 5; ptIds[3] = 4;
  this->RegionPolyData[0]->InsertNextCell(VTK_QUAD, 4, ptIds);

  ptIds[0] = 1; ptIds[1] = 2; ptIds[2] = 6; ptIds[3] = 5;
  this->RegionPolyData[1]->InsertNextCell(VTK_QUAD, 4, ptIds);

  ptIds[0] = 2; ptIds[1] = 3; ptIds[2] = 7; ptIds[3] = 6;
  this->RegionPolyData[2]->InsertNextCell(VTK_QUAD, 4, ptIds);

  ptIds[0] = 4; ptIds[1] = 5; ptIds[2] = 9; ptIds[3] = 8;
  this->RegionPolyData[3]->InsertNextCell(VTK_QUAD, 4, ptIds);

  ptIds[0] = 5; ptIds[1] = 6; ptIds[2] = 10; ptIds[3] = 9;
  this->RegionPolyData[4]->InsertNextCell(VTK_QUAD, 4, ptIds);

  ptIds[0] = 6; ptIds[1] = 7; ptIds[2] = 11; ptIds[3] = 10;
  this->RegionPolyData[5]->InsertNextCell(VTK_QUAD, 4, ptIds);

  ptIds[0] = 8; ptIds[1] = 9; ptIds[2] = 13; ptIds[3] = 12;
  this->RegionPolyData[6]->InsertNextCell(VTK_QUAD, 4, ptIds);

  ptIds[0] = 9; ptIds[1] = 10; ptIds[2] = 14; ptIds[3] = 13;
  this->RegionPolyData[7]->InsertNextCell(VTK_QUAD, 4, ptIds);

  ptIds[0] = 10; ptIds[1] = 11; ptIds[2] = 15; ptIds[3] = 14;
  this->RegionPolyData[8]->InsertNextCell(VTK_QUAD, 4, ptIds);

  for (i = 0; i < 9; i++)
    {
    vtkPolyDataMapper2D *pdm = vtkPolyDataMapper2D::New();
    vtkCoordinate *tcoord = vtkCoordinate::New();
    tcoord->SetCoordinateSystemToWorld();
    pdm->SetTransformCoordinate(tcoord);
    tcoord->Delete();

    this->RegionActors[i] = vtkActor2D::New();
    this->RegionActors[i]->SetMapper(pdm);
    this->RegionActors[i]->GetProperty()->SetColor(1, 1, 1);
    this->RegionActors[i]->GetProperty()->SetOpacity(0.0);

    pdm->SetInput(this->RegionPolyData[i]);
    pdm->Delete();
    }

  this->SliceOrientation = vtkImageCroppingRegionsWidget::SLICE_ORIENTATION_XY;
  this->Slice = 0;
  this->MouseCursorState = vtkImageCroppingRegionsWidget::NoLine;

  this->Moving = 0;
  this->CroppingRegionFlags = 0;
  this->VolumeMapper = NULL;

  for (i = 0; i < 6; i += 2)
    {
    this->InitialBounds[i] = this->PlanePositions[i] = 0;
    this->InitialBounds[i + 1] = this->PlanePositions[i + 1] = 1;
    }
}

//----------------------------------------------------------------------------
vtkImageCroppingRegionsWidget::~vtkImageCroppingRegionsWidget()
{
  int i;

  for (i = 0; i < 4; i++)
    {
    this->LineSources[i]->Delete();
    this->LineSources[i] = NULL;
    this->LineActors[i]->Delete();
    this->LineActors[i] = NULL;
    }

  for (i = 0; i < 9; i++)
    {
    this->RegionPolyData[i]->Delete();
    this->RegionPolyData[i]= NULL;
    this->RegionActors[i]->Delete();
    this->RegionActors[i] = NULL;
    }

  this->SetVolumeMapper(NULL);
}

//----------------------------------------------------------------------------
void vtkImageCroppingRegionsWidget::SetCroppingRegionFlags(int flags)
{
  if (this->CroppingRegionFlags == flags ||
      flags < 0x0 || flags > 0x7ffffff)
    {
    return;
    }

  this->CroppingRegionFlags = flags;
  this->Modified();

  this->UpdateOpacity();
}

//----------------------------------------------------------------------------
double vtkImageCroppingRegionsWidget::GetSlicePosition()
{
  if (!this->VolumeMapper || !this->VolumeMapper->GetInput())
    {
    return 0.0;
    }

  double *origin = this->VolumeMapper->GetInput()->GetOrigin();
  double *spacing = this->VolumeMapper->GetInput()->GetSpacing();

  return (double)origin[this->SliceOrientation] +
    ((double)this->Slice) * (double)spacing[this->SliceOrientation];
}

//----------------------------------------------------------------------------
void vtkImageCroppingRegionsWidget::UpdateOpacity()
{
  if (!this->VolumeMapper || !this->VolumeMapper->GetInput())
    {
    return;
    }

  static int indices[][9] = {{ 0,  9, 18,  3, 12, 21,  6, 15, 24},
                             { 1, 10, 19,  4, 13, 22,  7, 16, 25},
                             { 2, 11, 20,  5, 14, 23,  8, 17, 26},
                             { 0,  1,  2,  9, 10, 11, 18, 19, 20},
                             { 3,  4,  5, 12, 13, 14, 21, 22, 23},
                             { 6,  7,  8, 15, 16, 17, 24, 25, 26},
                             { 0,  1,  2,  3,  4,  5,  6,  7,  8},
                             { 9, 10, 11, 12, 13, 14, 15, 16, 17},
                             {18, 19, 20, 21, 22, 23, 24, 25, 26}};

  double slice_pos = this->GetSlicePosition();

  int sliceId = this->SliceOrientation * 3;
  if (slice_pos >= this->PlanePositions[this->SliceOrientation * 2] &&
      slice_pos <= this->PlanePositions[this->SliceOrientation * 2 + 1])
    {
    sliceId += 1;
    }
  else if (slice_pos > this->PlanePositions[this->SliceOrientation * 2 + 1])
    {
    sliceId += 2;
    }

  int compare = 1;
  int i;
  for (i = 0; i < 9; i++)
    {
    if ((compare << indices[sliceId][i]) & this->CroppingRegionFlags)
      {
      this->RegionActors[i]->GetProperty()->SetOpacity(0.0);
      }
    else
      {
      this->RegionActors[i]->GetProperty()->SetOpacity(0.3);
      }
    }
}

//----------------------------------------------------------------------------
void vtkImageCroppingRegionsWidget::SetPlanePositions(double xMin, double xMax,
                                                   double yMin, double yMax,
                                                   double zMin, double zMax)
{
  double positions[6];
  positions[0] = xMin;
  positions[1] = xMax;
  positions[2] = yMin;
  positions[3] = yMax;
  positions[4] = zMin;
  positions[5] = zMax;

  this->ConstrainPlanePositions(positions);

  if (this->PlanePositions[0] == positions[0] &&
      this->PlanePositions[1] == positions[1] &&
      this->PlanePositions[2] == positions[2] &&
      this->PlanePositions[3] == positions[3] &&
      this->PlanePositions[4] == positions[4] &&
      this->PlanePositions[5] == positions[5])
    {
    return;
    }

  int i;
  for (i = 0; i < 6; i++)
    {
    this->PlanePositions[i] = positions[i];
    }

  this->VolumeMapper->SetCroppingRegionPlanes(this->PlanePositions);
  this->UpdateGeometry();
}

//----------------------------------------------------------------------------
void vtkImageCroppingRegionsWidget::ConstrainPlanePositions(double positions[6])
{
  int i;
  double tmp;

  for (i = 0; i < 6; i += 2)
    {
    if (positions[i] > positions[i + 1])
      {
      tmp = positions[i];
      positions[i] = positions[i + 1];
      positions[i + 1] = tmp;
      }
    if (positions[i] < this->InitialBounds[i] ||
        positions[i] > this->InitialBounds[i + 1])
      {
      positions[i] = this->InitialBounds[i];
      }
    if (positions[i + 1] < this->InitialBounds[i] ||
        positions[i + 1] > this->InitialBounds[i + 1])
      {
      positions[i + 1] = this->InitialBounds[i + 1];
      }
    }
}

//----------------------------------------------------------------------------
void vtkImageCroppingRegionsWidget::UpdateGeometry()
{
  if (!this->VolumeMapper || !this->VolumeMapper->GetInput())
    {
    return;
    }

  // Could use any of the 9 region poly data because they share points

  vtkPoints *points = this->RegionPolyData[0]->GetPoints();

  double slice_pos = this->GetSlicePosition();
  double *plane_pos = this->PlanePositions;
  double *bounds = this->InitialBounds;

  switch (this->SliceOrientation)
    {
    case vtkImageCroppingRegionsWidget::SLICE_ORIENTATION_YZ:
      this->LineSources[0]->SetPoint1(slice_pos, plane_pos[2], bounds[4]);
      this->LineSources[0]->SetPoint2(slice_pos, plane_pos[2], bounds[5]);
      this->LineSources[1]->SetPoint1(slice_pos, plane_pos[3], bounds[4]);
      this->LineSources[1]->SetPoint2(slice_pos, plane_pos[3], bounds[5]);
      this->LineSources[2]->SetPoint1(slice_pos, bounds[2], plane_pos[4]);
      this->LineSources[2]->SetPoint2(slice_pos, bounds[3], plane_pos[4]);
      this->LineSources[3]->SetPoint1(slice_pos, bounds[2], plane_pos[5]);
      this->LineSources[3]->SetPoint2(slice_pos, bounds[3], plane_pos[5]);

      points->SetPoint(0, slice_pos, bounds[2], bounds[4]);
      points->SetPoint(1, slice_pos, plane_pos[2], bounds[4]);
      points->SetPoint(2, slice_pos, plane_pos[3], bounds[4]);
      points->SetPoint(3, slice_pos, bounds[3], bounds[4]);
      points->SetPoint(4, slice_pos, bounds[2], plane_pos[4]);
      points->SetPoint(5, slice_pos, plane_pos[2], plane_pos[4]);
      points->SetPoint(6, slice_pos, plane_pos[3], plane_pos[4]);
      points->SetPoint(7, slice_pos, bounds[3], plane_pos[4]);
      points->SetPoint(8, slice_pos, bounds[2],plane_pos[5]);
      points->SetPoint(9, slice_pos, plane_pos[2], plane_pos[5]);
      points->SetPoint(10, slice_pos, plane_pos[3], plane_pos[5]);
      points->SetPoint(11, slice_pos, bounds[3], plane_pos[5]);
      points->SetPoint(12, slice_pos, bounds[2], bounds[5]);
      points->SetPoint(13, slice_pos, plane_pos[2], bounds[5]);
      points->SetPoint(14, slice_pos, plane_pos[3], bounds[5]);
      points->SetPoint(15, slice_pos, bounds[3], bounds[5]);
      break;

    case vtkImageCroppingRegionsWidget::SLICE_ORIENTATION_XZ:
      this->LineSources[0]->SetPoint1(plane_pos[0], slice_pos, bounds[4]);
      this->LineSources[0]->SetPoint2(plane_pos[0], slice_pos, bounds[5]);
      this->LineSources[1]->SetPoint1(plane_pos[1], slice_pos, bounds[4]);
      this->LineSources[1]->SetPoint2(plane_pos[1], slice_pos, bounds[5]);
      this->LineSources[2]->SetPoint1(bounds[0], slice_pos, plane_pos[4]);
      this->LineSources[2]->SetPoint2(bounds[1], slice_pos, plane_pos[4]);
      this->LineSources[3]->SetPoint1(bounds[0], slice_pos, plane_pos[5]);
      this->LineSources[3]->SetPoint2(bounds[1], slice_pos, plane_pos[5]);

      points->SetPoint(0, bounds[0], slice_pos, bounds[4]);
      points->SetPoint(1, plane_pos[0], slice_pos, bounds[4]);
      points->SetPoint(2, plane_pos[1], slice_pos, bounds[4]);
      points->SetPoint(3, bounds[1], slice_pos, bounds[4]);
      points->SetPoint(4, bounds[0], slice_pos, plane_pos[4]);
      points->SetPoint(5, plane_pos[0], slice_pos, plane_pos[4]);
      points->SetPoint(6, plane_pos[1], slice_pos, plane_pos[4]);
      points->SetPoint(7, bounds[1], slice_pos, plane_pos[4]);
      points->SetPoint(8, bounds[0], slice_pos, plane_pos[5]);
      points->SetPoint(9, plane_pos[0], slice_pos, plane_pos[5]);
      points->SetPoint(10, plane_pos[1], slice_pos, plane_pos[5]);
      points->SetPoint(11, bounds[1], slice_pos, plane_pos[5]);
      points->SetPoint(12, bounds[0], slice_pos, bounds[5]);
      points->SetPoint(13, plane_pos[0], slice_pos, bounds[5]);
      points->SetPoint(14, plane_pos[1], slice_pos, bounds[5]);
      points->SetPoint(15, bounds[1], slice_pos, bounds[5]);
      break;

    case vtkImageCroppingRegionsWidget::SLICE_ORIENTATION_XY:
      this->LineSources[0]->SetPoint1(plane_pos[0], bounds[2], slice_pos);
      this->LineSources[0]->SetPoint2(plane_pos[0], bounds[3], slice_pos);
      this->LineSources[1]->SetPoint1(plane_pos[1], bounds[2], slice_pos);
      this->LineSources[1]->SetPoint2(plane_pos[1], bounds[3], slice_pos);
      this->LineSources[2]->SetPoint1(bounds[0], plane_pos[2], slice_pos);
      this->LineSources[2]->SetPoint2(bounds[1], plane_pos[2], slice_pos);
      this->LineSources[3]->SetPoint1(bounds[0], plane_pos[3], slice_pos);
      this->LineSources[3]->SetPoint2(bounds[1], plane_pos[3], slice_pos);

      points->SetPoint(0, bounds[0], bounds[2], slice_pos);
      points->SetPoint(1, plane_pos[0], bounds[2], slice_pos);
      points->SetPoint(2, plane_pos[1], bounds[2], slice_pos);
      points->SetPoint(3, bounds[1], bounds[2], slice_pos);
      points->SetPoint(4, bounds[0], plane_pos[2], slice_pos);
      points->SetPoint(5, plane_pos[0], plane_pos[2], slice_pos);
      points->SetPoint(6, plane_pos[1], plane_pos[2], slice_pos);
      points->SetPoint(7, bounds[1], plane_pos[2], slice_pos);
      points->SetPoint(8, bounds[0], plane_pos[3], slice_pos);
      points->SetPoint(9, plane_pos[0], plane_pos[3], slice_pos);
      points->SetPoint(10, plane_pos[1], plane_pos[3], slice_pos);
      points->SetPoint(11, bounds[1], plane_pos[3], slice_pos);
      points->SetPoint(12, bounds[0], bounds[3], slice_pos);
      points->SetPoint(13, plane_pos[0], bounds[3], slice_pos);
      points->SetPoint(14, plane_pos[1], bounds[3], slice_pos);
      points->SetPoint(15, bounds[1], bounds[3], slice_pos);
      break;
    }

  this->UpdateOpacity();
}

//----------------------------------------------------------------------------
void vtkImageCroppingRegionsWidget::SetEnabled(int enabling)
{
  if (!this->Interactor)
    {
    vtkErrorMacro(
      <<"The interactor must be set prior to enabling/disabling widget");
    return;
    }

  if (this->Enabled == enabling)
    {
    return;
    }

  int count;
  if (enabling)
    {
    this->SetCurrentRenderer(
      this->Interactor->FindPokedRenderer(
        this->Interactor->GetLastEventPosition()[0],
        this->Interactor->GetLastEventPosition()[1]));
    if (this->CurrentRenderer == NULL)
      {
      return;
      }

    this->Enabled = 1;

    // Listen for the following events

    vtkRenderWindowInteractor *i = this->Interactor;
    i->AddObserver(vtkCommand::MouseMoveEvent,
                   this->EventCallbackCommand, this->Priority);
    i->AddObserver(vtkCommand::LeftButtonPressEvent,
                   this->EventCallbackCommand, this->Priority);
    i->AddObserver(vtkCommand::LeftButtonReleaseEvent,
                   this->EventCallbackCommand, this->Priority);
    i->AddObserver(vtkCommand::MiddleButtonPressEvent,
                   this->EventCallbackCommand, this->Priority);
    i->AddObserver(vtkCommand::MiddleButtonReleaseEvent,
                   this->EventCallbackCommand, this->Priority);
    i->AddObserver(vtkCommand::RightButtonPressEvent,
                   this->EventCallbackCommand, this->Priority);
    i->AddObserver(vtkCommand::RightButtonReleaseEvent,
                   this->EventCallbackCommand, this->Priority);

    // Add the cropping regions

    this->CurrentRenderer->AddViewProp(this->LineActors[0]);
    this->CurrentRenderer->AddViewProp(this->LineActors[1]);
    this->CurrentRenderer->AddViewProp(this->LineActors[2]);
    this->CurrentRenderer->AddViewProp(this->LineActors[3]);
    for (count = 0; count < 9; count++)
      {
      this->CurrentRenderer->AddViewProp(this->RegionActors[count]);
      }

    this->InvokeEvent(vtkCommand::EnableEvent,NULL);
    }
  else
    {
    this->Enabled = 0;

    // Don't listen for events any more

    this->Interactor->RemoveObserver(this->EventCallbackCommand);

    // Turn off the cropping regions

    if (this->CurrentRenderer)
      {
      this->CurrentRenderer->RemoveActor(this->LineActors[0]);
      this->CurrentRenderer->RemoveActor(this->LineActors[1]);
      this->CurrentRenderer->RemoveActor(this->LineActors[2]);
      this->CurrentRenderer->RemoveActor(this->LineActors[3]);
      for (count = 0; count < 9; count++)
        {
        this->CurrentRenderer->RemoveActor(this->RegionActors[count]);
        }
      }

    this->InvokeEvent(vtkCommand::DisableEvent,NULL);
    }

  this->Interactor->Render();
}

//----------------------------------------------------------------------------
void vtkImageCroppingRegionsWidget::ProcessEvents(vtkObject* vtkNotUsed(object),
                                               unsigned long event,
                                               void* clientdata,
                                               void* vtkNotUsed(calldata))
{
  vtkImageCroppingRegionsWidget* self =
    reinterpret_cast<vtkImageCroppingRegionsWidget *>(clientdata);

  switch (event)
    {
    case vtkCommand::LeftButtonPressEvent:
    case vtkCommand::MiddleButtonPressEvent:
    case vtkCommand::RightButtonPressEvent:
      self->OnButtonPress();
      break;
    case vtkCommand::MouseMoveEvent:
      self->OnMouseMove();
      break;
    case vtkCommand::LeftButtonReleaseEvent:
    case vtkCommand::MiddleButtonReleaseEvent:
    case vtkCommand::RightButtonReleaseEvent:
      self->OnButtonRelease();
      break;
    }
}

//----------------------------------------------------------------------------
void vtkImageCroppingRegionsWidget::OnButtonPress()
{
  if (this->MouseCursorState == vtkImageCroppingRegionsWidget::NoLine)
    {
    return;
    }

  this->Moving = 1;
  this->EventCallbackCommand->SetAbortFlag(1);
  this->StartInteraction();
  this->InvokeEvent(vtkCommand::StartInteractionEvent, NULL);
  this->Interactor->Render();
}

//----------------------------------------------------------------------------
void vtkImageCroppingRegionsWidget::OnButtonRelease()
{
  if (this->MouseCursorState == vtkImageCroppingRegionsWidget::NoLine)
    {
    return;
    }

  this->Moving = 0;
  this->EventCallbackCommand->SetAbortFlag(1);
  this->EndInteraction();
  this->InvokeEvent(vtkCommand::EndInteractionEvent, NULL);

  this->MouseCursorState = vtkImageCroppingRegionsWidget::NoLine;
  this->SetMouseCursor(this->MouseCursorState);

  this->Interactor->Render();
}

//----------------------------------------------------------------------------
void vtkImageCroppingRegionsWidget::OnMouseMove()
{
  if (this->Moving)
    {
    switch (this->MouseCursorState)
      {
      case vtkImageCroppingRegionsWidget::MovingH1:
      case vtkImageCroppingRegionsWidget::MovingH2:
        this->MoveHorizontalLine();
        break;
      case vtkImageCroppingRegionsWidget::MovingV1:
      case vtkImageCroppingRegionsWidget::MovingV2:
        this->MoveVerticalLine();
        break;
      case vtkImageCroppingRegionsWidget::MovingH1AndV1:
      case vtkImageCroppingRegionsWidget::MovingH2AndV1:
      case vtkImageCroppingRegionsWidget::MovingH1AndV2:
      case vtkImageCroppingRegionsWidget::MovingH2AndV2:
        this->MoveIntersectingLines();
        break;
      }
    this->UpdateCursorIcon();
    this->EventCallbackCommand->SetAbortFlag(1);
    this->InvokeEvent(vtkCommand::InteractionEvent, NULL);
    }
  else
    {
    this->UpdateCursorIcon();
    }
}

//----------------------------------------------------------------------------
void vtkImageCroppingRegionsWidget::MoveHorizontalLine()
{
  double newPosition[3];
  float planes[6];
  int i;

  for (i = 0; i < 6; i++)
    {
    planes[i] = this->PlanePositions[i];
    }

  int x = this->Interactor->GetEventPosition()[0];
  int y = this->Interactor->GetEventPosition()[1];

  if (!this->ComputeWorldCoordinate(x, y, newPosition))
    {
    return;
    }

  if (this->MouseCursorState == vtkImageCroppingRegionsWidget::MovingH1)
    {
    switch (this->SliceOrientation)
      {
      case vtkImageCroppingRegionsWidget::SLICE_ORIENTATION_YZ:
      case vtkImageCroppingRegionsWidget::SLICE_ORIENTATION_XZ:
        if (newPosition[2] < planes[5])
          {
          planes[4] = newPosition[2];
          }
        break;
      case vtkImageCroppingRegionsWidget::SLICE_ORIENTATION_XY:
        if (newPosition[1] < planes[3])
          {
          planes[2] = newPosition[1];
          }
        break;
      }

    this->SetPlanePositions(planes);
    this->InvokeEvent(
      vtkImageCroppingRegionsWidget::CroppingPlanesPositionChangedEvent, planes);
    this->EventCallbackCommand->SetAbortFlag(1);
    this->Interactor->Render();
    }
  else if (this->MouseCursorState == vtkImageCroppingRegionsWidget::MovingH2)
    {
    switch (this->SliceOrientation)
      {
      case vtkImageCroppingRegionsWidget::SLICE_ORIENTATION_YZ:
      case vtkImageCroppingRegionsWidget::SLICE_ORIENTATION_XZ:
        if (newPosition[2] > planes[4])
          {
          planes[5] = newPosition[2];
          }
        break;
      case vtkImageCroppingRegionsWidget::SLICE_ORIENTATION_XY:
        if (newPosition[1] > planes[2])
          {
          planes[3] = newPosition[1];
          }
        break;
      }
    this->SetPlanePositions(planes);
    this->InvokeEvent(
      vtkImageCroppingRegionsWidget::CroppingPlanesPositionChangedEvent, planes);
    this->EventCallbackCommand->SetAbortFlag(1);
    this->Interactor->Render();
    }
}

//----------------------------------------------------------------------------
void vtkImageCroppingRegionsWidget::MoveVerticalLine()
{
  double newPosition[3];
  float planes[6];
  int i;

  for (i = 0; i < 6; i++)
    {
    planes[i] = this->PlanePositions[i];
    }

  int x = this->Interactor->GetEventPosition()[0];
  int y = this->Interactor->GetEventPosition()[1];

  if (!this->ComputeWorldCoordinate(x, y, newPosition))
    {
    return;
    }

  if (this->MouseCursorState == vtkImageCroppingRegionsWidget::MovingV1)
    {
    switch (this->SliceOrientation)
      {
      case vtkImageCroppingRegionsWidget::SLICE_ORIENTATION_YZ:
        if (newPosition[1] < planes[3])
          {
          planes[2] = newPosition[1];
          }
        break;
      case vtkImageCroppingRegionsWidget::SLICE_ORIENTATION_XZ:
      case vtkImageCroppingRegionsWidget::SLICE_ORIENTATION_XY:
        if (newPosition[0] < planes[1])
          {
          planes[0] = newPosition[0];
          }
        break;
      }
    this->SetPlanePositions(planes);
    this->InvokeEvent(
      vtkImageCroppingRegionsWidget::CroppingPlanesPositionChangedEvent, planes);
    this->EventCallbackCommand->SetAbortFlag(1);
    this->Interactor->Render();
    }
  else if (this->MouseCursorState == vtkImageCroppingRegionsWidget::MovingV2)
    {
    switch (this->SliceOrientation)
      {
      case vtkImageCroppingRegionsWidget::SLICE_ORIENTATION_YZ:
        if (newPosition[1] > planes[2])
          {
          planes[3] = newPosition[1];
          }
        break;
      case vtkImageCroppingRegionsWidget::SLICE_ORIENTATION_XZ:
      case vtkImageCroppingRegionsWidget::SLICE_ORIENTATION_XY:
        if (newPosition[0] > planes[0])
          {
          planes[1] = newPosition[0];
          }
        break;
      }

    this->SetPlanePositions(planes);
    this->InvokeEvent(
      vtkImageCroppingRegionsWidget::CroppingPlanesPositionChangedEvent, planes);
    this->EventCallbackCommand->SetAbortFlag(1);
    this->Interactor->Render();
    }
}

//----------------------------------------------------------------------------
void vtkImageCroppingRegionsWidget::MoveIntersectingLines()
{
  double newPosition[3];
  float planes[6];
  int i;

  for (i = 0; i < 6; i++)
    {
    planes[i] = this->PlanePositions[i];
    }

  int x = this->Interactor->GetEventPosition()[0];
  int y = this->Interactor->GetEventPosition()[1];

  if (!this->ComputeWorldCoordinate(x, y, newPosition))
    {
    return;
    }

  switch (this->MouseCursorState)
    {
    case vtkImageCroppingRegionsWidget::MovingH1AndV1:
      switch (this->SliceOrientation)
        {
        case vtkImageCroppingRegionsWidget::SLICE_ORIENTATION_YZ:
          if (newPosition[1] < planes[3])
            {
            planes[2] = newPosition[1];
            }
          if (newPosition[2] < planes[5])
            {
            planes[4] = newPosition[2];
            }
          break;
        case vtkImageCroppingRegionsWidget::SLICE_ORIENTATION_XZ:
          if (newPosition[0] < planes[1])
            {
            planes[0] = newPosition[0];
            }
          if (newPosition[2] < planes[5])
            {
            planes[4] = newPosition[2];
            }
          break;
        case vtkImageCroppingRegionsWidget::SLICE_ORIENTATION_XY:
          if (newPosition[0] < planes[1])
            {
            planes[0] = newPosition[0];
            }
          if (newPosition[1] < planes[3])
            {
            planes[2] = newPosition[1];
            }
          break;
        }

      this->SetPlanePositions(planes);
      this->InvokeEvent(
        vtkImageCroppingRegionsWidget::CroppingPlanesPositionChangedEvent, planes);
      this->EventCallbackCommand->SetAbortFlag(1);
      this->Interactor->Render();
      break;
    case vtkImageCroppingRegionsWidget::MovingH1AndV2:
      switch (this->SliceOrientation)
        {
        case vtkImageCroppingRegionsWidget::SLICE_ORIENTATION_YZ:
          if (newPosition[1] > planes[2])
            {
            planes[3] = newPosition[1];
            }
          if (newPosition[2] < planes[5])
            {
            planes[4] = newPosition[2];
            }
          break;
        case vtkImageCroppingRegionsWidget::SLICE_ORIENTATION_XZ:
          if (newPosition[0] > planes[0])
            {
            planes[1] = newPosition[0];
            }
          if (newPosition[2] < planes[5])
            {
            planes[4] = newPosition[2];
            }
          break;
        case vtkImageCroppingRegionsWidget::SLICE_ORIENTATION_XY:
          if (newPosition[0] > planes[0])
            {
            planes[1]  = newPosition[0];
            }
          if (newPosition[1] < planes[3])
            {
            planes[2] = newPosition[1];
            }
          break;
        }

      this->SetPlanePositions(planes);
      this->InvokeEvent(vtkImageCroppingRegionsWidget::CroppingPlanesPositionChangedEvent,
                        planes);
      this->EventCallbackCommand->SetAbortFlag(1);
      this->Interactor->Render();
      break;
    case vtkImageCroppingRegionsWidget::MovingH2AndV1:
      switch (this->SliceOrientation)
        {
        case vtkImageCroppingRegionsWidget::SLICE_ORIENTATION_YZ:
          if (newPosition[1] < planes[3])
            {
            planes[2] = newPosition[1];
            }
          if (newPosition[2] > planes[4])
            {
            planes[5] = newPosition[2];
            }
          break;
        case vtkImageCroppingRegionsWidget::SLICE_ORIENTATION_XZ:
          if (newPosition[0] < planes[1])
            {
            planes[0] = newPosition[0];
            }
          if (newPosition[2] > planes[4])
            {
            planes[5] = newPosition[2];
            }
          break;
        case vtkImageCroppingRegionsWidget::SLICE_ORIENTATION_XY:
          if (newPosition[0] < planes[1])
            {
            planes[0] = newPosition[0];
            }
          if (newPosition[1] > planes[2])
            {
            planes[3] = newPosition[1];
            }
          break;
        }

      this->SetPlanePositions(planes);
      this->InvokeEvent(vtkImageCroppingRegionsWidget::CroppingPlanesPositionChangedEvent,
                        planes);
      this->EventCallbackCommand->SetAbortFlag(1);
      this->Interactor->Render();
      break;
    case vtkImageCroppingRegionsWidget::MovingH2AndV2:
      switch (this->SliceOrientation)
        {
        case vtkImageCroppingRegionsWidget::SLICE_ORIENTATION_YZ:
          if (newPosition[1] > planes[2])
            {
            planes[3] = newPosition[1];
            }
          if (newPosition[2] > planes[4])
            {
            planes[5] = newPosition[2];
            }
          break;
        case vtkImageCroppingRegionsWidget::SLICE_ORIENTATION_XZ:
          if (newPosition[0] > planes[0])
            {
            planes[1] = newPosition[0];
            }
          if (newPosition[2] > planes[4])
            {
            planes[5] = newPosition[2];
            }
          break;
        case vtkImageCroppingRegionsWidget::SLICE_ORIENTATION_XY:
          if (newPosition[0] > planes[0])
            {
            planes[1] = newPosition[0];
            }
          if (newPosition[1] > planes[2])
            {
            planes[3] = newPosition[1];
            }
          break;
        }

      this->SetPlanePositions(planes);
      this->InvokeEvent(vtkImageCroppingRegionsWidget::CroppingPlanesPositionChangedEvent,
                        planes);
      this->EventCallbackCommand->SetAbortFlag(1);
      this->Interactor->Render();
      break;
    }
}

//----------------------------------------------------------------------------
void vtkImageCroppingRegionsWidget::UpdateCursorIcon()
{
  if (!this->Enabled)
    {
    this->Interactor->GetRenderWindow()->SetCurrentCursor(VTK_CURSOR_DEFAULT);
    return;
    }

  if (!this->CurrentRenderer || this->Moving)
    {
    return;
    }

  double slice_pos = this->GetSlicePosition();
  double *plane_pos = this->PlanePositions;
  double *bounds = this->InitialBounds;

  int x = this->Interactor->GetEventPosition()[0];
  int y = this->Interactor->GetEventPosition()[1];

  double lineX1 = 0.0;
  double lineX2 = 0.0;
  double lineY1 = 0.0;
  double lineY2 = 0.0;

  switch (this->SliceOrientation)
    {
    case vtkImageCroppingRegionsWidget::SLICE_ORIENTATION_YZ:
      this->CurrentRenderer->SetWorldPoint(
        slice_pos, plane_pos[2], bounds[4], 1);
      this->CurrentRenderer->WorldToDisplay();
      lineX1 = this->CurrentRenderer->GetDisplayPoint()[0];
      this->CurrentRenderer->SetWorldPoint(
        slice_pos, plane_pos[3], bounds[4], 1);
      this->CurrentRenderer->WorldToDisplay();
      lineX2 = this->CurrentRenderer->GetDisplayPoint()[0];

      this->CurrentRenderer->SetWorldPoint(
        slice_pos, bounds[2], plane_pos[4], 1);
      this->CurrentRenderer->WorldToDisplay();
      lineY1 = this->CurrentRenderer->GetDisplayPoint()[1];
      this->CurrentRenderer->SetWorldPoint(
        slice_pos, bounds[2], plane_pos[5], 1);
      this->CurrentRenderer->WorldToDisplay();
      lineY2 = this->CurrentRenderer->GetDisplayPoint()[1];
      break;

    case vtkImageCroppingRegionsWidget::SLICE_ORIENTATION_XZ:
      this->CurrentRenderer->SetWorldPoint(
        plane_pos[0], slice_pos, bounds[4], 1);
      this->CurrentRenderer->WorldToDisplay();
      lineX1 = this->CurrentRenderer->GetDisplayPoint()[0];
      this->CurrentRenderer->SetWorldPoint(
        plane_pos[1], slice_pos, bounds[4], 1);
      this->CurrentRenderer->WorldToDisplay();
      lineX2 = this->CurrentRenderer->GetDisplayPoint()[0];

      this->CurrentRenderer->SetWorldPoint(
        bounds[0], slice_pos, plane_pos[4], 1);
      this->CurrentRenderer->WorldToDisplay();
      lineY1 = this->CurrentRenderer->GetDisplayPoint()[1];
      this->CurrentRenderer->SetWorldPoint(
        bounds[0], slice_pos, plane_pos[5], 1);
      this->CurrentRenderer->WorldToDisplay();
      lineY2 = this->CurrentRenderer->GetDisplayPoint()[1];
      break;

    case vtkImageCroppingRegionsWidget::SLICE_ORIENTATION_XY:
      this->CurrentRenderer->SetWorldPoint(
        plane_pos[0], bounds[2], slice_pos, 1);
      this->CurrentRenderer->WorldToDisplay();
      lineX1 = this->CurrentRenderer->GetDisplayPoint()[0];
      this->CurrentRenderer->SetWorldPoint(
        plane_pos[1], bounds[2], slice_pos, 1);
      this->CurrentRenderer->WorldToDisplay();
      lineX2 = this->CurrentRenderer->GetDisplayPoint()[0];

      this->CurrentRenderer->SetWorldPoint(
        bounds[0], plane_pos[2], slice_pos, 1);
      this->CurrentRenderer->WorldToDisplay();
      lineY1 = this->CurrentRenderer->GetDisplayPoint()[1];
      this->CurrentRenderer->SetWorldPoint(
        bounds[0], plane_pos[3], slice_pos, 1);
      this->CurrentRenderer->WorldToDisplay();
      lineY2 = this->CurrentRenderer->GetDisplayPoint()[1];
      break;
    }

  double xDist1 = fabs(x - lineX1);
  double xDist2 = fabs(x - lineX2);
  double yDist1 = fabs(y - lineY1);
  double yDist2 = fabs(y - lineY2);

  int pState = this->MouseCursorState;

  if (xDist1 < 3)
    {
    if (yDist1 < 3)
      {
      this->MouseCursorState = vtkImageCroppingRegionsWidget::MovingH1AndV1;
      }
    else if (yDist2 < 3)
      {
      this->MouseCursorState = vtkImageCroppingRegionsWidget::MovingH2AndV1;
      }
    else
      {
      this->MouseCursorState = vtkImageCroppingRegionsWidget::MovingV1;
      }
    }
  else if (xDist2 < 3)
    {
    if (yDist1 < 3)
      {
      this->MouseCursorState = vtkImageCroppingRegionsWidget::MovingH1AndV2;
      }
    else if (yDist2 < 3)
      {
      this->MouseCursorState = vtkImageCroppingRegionsWidget::MovingH2AndV2;
      }
    else
      {
      this->MouseCursorState = vtkImageCroppingRegionsWidget::MovingV2;
      }
    }
  else if (yDist1 < 3)
    {
    this->MouseCursorState = vtkImageCroppingRegionsWidget::MovingH1;
    }
  else if (yDist2 < 3)
    {
    this->MouseCursorState = vtkImageCroppingRegionsWidget::MovingH2;
    }
  else
    {
    this->MouseCursorState = vtkImageCroppingRegionsWidget::NoLine;
    }

  if (pState == this->MouseCursorState)
    {
    return;
    }

  this->SetMouseCursor(this->MouseCursorState);

}

//----------------------------------------------------------------------------
void vtkImageCroppingRegionsWidget::SetMouseCursor(int state)
{
  switch (state)
    {
    case vtkImageCroppingRegionsWidget::MovingH1AndV1:
    case vtkImageCroppingRegionsWidget::MovingH2AndV1:
    case vtkImageCroppingRegionsWidget::MovingH1AndV2:
    case vtkImageCroppingRegionsWidget::MovingH2AndV2:
      this->Interactor->GetRenderWindow()->SetCurrentCursor(VTK_CURSOR_SIZEALL);
      break;
    case vtkImageCroppingRegionsWidget::MovingV1:
    case vtkImageCroppingRegionsWidget::MovingV2:
      this->Interactor->GetRenderWindow()->SetCurrentCursor(VTK_CURSOR_SIZEWE);
      break;
    case vtkImageCroppingRegionsWidget::MovingH1:
    case vtkImageCroppingRegionsWidget::MovingH2:
      this->Interactor->GetRenderWindow()->SetCurrentCursor(VTK_CURSOR_SIZENS);
      break;
    case vtkImageCroppingRegionsWidget::NoLine:
      this->Interactor->GetRenderWindow()->SetCurrentCursor(VTK_CURSOR_DEFAULT);
      break;
    }
}

//----------------------------------------------------------------------------
int vtkImageCroppingRegionsWidget::ComputeWorldCoordinate(
  int x, int y, double* coord)
{
  if (!this->CurrentRenderer)
    {
    return 0;
    }

  this->CurrentRenderer->SetWorldPoint(
    this->InitialBounds[0], this->InitialBounds[2], this->InitialBounds[4], 1.0);
  this->CurrentRenderer->WorldToDisplay();
  double *dispPoint = this->CurrentRenderer->GetDisplayPoint();

  this->CurrentRenderer->SetDisplayPoint(x, y, dispPoint[2]);
  this->CurrentRenderer->DisplayToWorld();
  double *worldPoint = this->CurrentRenderer->GetWorldPoint();
  if (worldPoint[3] != 0.0)
    {
    worldPoint[0] = (double)((double)worldPoint[0] / (double)worldPoint[3]);
    worldPoint[1] = (double)((double)worldPoint[1] / (double)worldPoint[3]);
    worldPoint[2] = (double)((double)worldPoint[2] / (double)worldPoint[3]);
    }

  coord[0] = worldPoint[0];
  coord[1] = worldPoint[1];
  coord[2] = worldPoint[2];

  int idx1 = (this->SliceOrientation + 1) % 3;
  int idx2 = (this->SliceOrientation + 2) % 3;

  if (worldPoint[idx1] < this->InitialBounds[idx1 * 2] ||
      worldPoint[idx1] > this->InitialBounds[idx1 * 2 + 1] ||
      worldPoint[idx2] < this->InitialBounds[idx2 * 2] ||
      worldPoint[idx2] > this->InitialBounds[idx2 * 2 + 1])
    {
    return 0;
    }

  return 1;
}

//----------------------------------------------------------------------------
void vtkImageCroppingRegionsWidget::SetLine1Color(double r, double g, double b)
{
  this->LineActors[0]->GetProperty()->SetColor(r, g, b);
  this->Interactor->Render();
}

//----------------------------------------------------------------------------
double* vtkImageCroppingRegionsWidget::GetLine1Color()
{
  return this->LineActors[0]->GetProperty()->GetColor();
}

//----------------------------------------------------------------------------
void vtkImageCroppingRegionsWidget::GetLine1Color(double rgb[3])
{
  this->LineActors[0]->GetProperty()->GetColor(rgb);
}

//----------------------------------------------------------------------------
void vtkImageCroppingRegionsWidget::SetLine2Color(double r, double g, double b)
{
  this->LineActors[1]->GetProperty()->SetColor(r, g, b);
  this->Interactor->Render();
}

//----------------------------------------------------------------------------
double* vtkImageCroppingRegionsWidget::GetLine2Color()
{
  return this->LineActors[1]->GetProperty()->GetColor();
}

//----------------------------------------------------------------------------
void vtkImageCroppingRegionsWidget::GetLine2Color(double rgb[3])
{
  this->LineActors[1]->GetProperty()->GetColor(rgb);
}

//----------------------------------------------------------------------------
void vtkImageCroppingRegionsWidget::SetLine3Color(double r, double g, double b)
{
  this->LineActors[2]->GetProperty()->SetColor(r, g, b);
  this->Interactor->Render();
}

//----------------------------------------------------------------------------
double* vtkImageCroppingRegionsWidget::GetLine3Color()
{
  return this->LineActors[2]->GetProperty()->GetColor();
}

//----------------------------------------------------------------------------
void vtkImageCroppingRegionsWidget::GetLine3Color(double rgb[3])
{
  this->LineActors[2]->GetProperty()->GetColor(rgb);
}

//----------------------------------------------------------------------------
void vtkImageCroppingRegionsWidget::SetLine4Color(double r, double g, double b)
{
  this->LineActors[3]->GetProperty()->SetColor(r, g, b);
  this->Interactor->Render();
}

//----------------------------------------------------------------------------
double* vtkImageCroppingRegionsWidget::GetLine4Color()
{
  return this->LineActors[3]->GetProperty()->GetColor();
}

//----------------------------------------------------------------------------
void vtkImageCroppingRegionsWidget::GetLine4Color(double rgb[3])
{
  this->LineActors[3]->GetProperty()->GetColor(rgb);
}

//----------------------------------------------------------------------------
void vtkImageCroppingRegionsWidget::SetVolumeMapper(vtkVolumeMapper *arg)
{
  if (this->VolumeMapper == arg)
    {
    return;
    }

  if (this->VolumeMapper)
    {
    this->VolumeMapper->UnRegister(this);
    }

  this->VolumeMapper = arg;

  if (this->VolumeMapper)
    {
    this->VolumeMapper->Register(this);
    }

  this->Modified();

  // Update internal objects according to the new Input

  this->UpdateAccordingToInput();
}

//----------------------------------------------------------------------------
void vtkImageCroppingRegionsWidget::PlaceWidget(double bounds[6])
{
  double center[6];
  this->AdjustBounds(bounds, this->InitialBounds, center);

  for (int i = 0; i < 3; i++)
    {
    if (this->InitialBounds[i * 2] > this->InitialBounds[i * 2 + 1])
      {
      double temp = this->InitialBounds[i * 2];
      this->InitialBounds[i * 2] = this->InitialBounds[i * 2 + 1];
      this->InitialBounds[i * 2 + 1] = temp;
      }
    }

  // Bounds have changed, let's try to place the plane at the same positions
  // and they will be constrain automatically

  this->SetPlanePositions(this->PlanePositions);
}

//----------------------------------------------------------------------------
void vtkImageCroppingRegionsWidget::UpdateAccordingToInput()
{
  vtkVolumeMapper *mapper = this->GetVolumeMapper();
  if (mapper)
    {
    this->PlaceWidget(mapper->GetBounds());
    this->SetPlanePositions(mapper->GetCroppingRegionPlanes());
    this->SetCroppingRegionFlags(mapper->GetCroppingRegionFlags());
    }
}

//----------------------------------------------------------------------------
void vtkImageCroppingRegionsWidget::SetSlice(int num)
{
  this->Slice = num;

  this->Modified();

  this->UpdateGeometry();

  if (this->Interactor)
    {
    this->Interactor->Render();
    }
}

//----------------------------------------------------------------------------
void vtkImageCroppingRegionsWidget::SetSliceOrientation(int arg)
{
  if (this->SliceOrientation == arg)
    {
    return;
    }

  this->SliceOrientation = arg;

  this->UpdateGeometry();

  if (this->Interactor)
    {
    this->Interactor->Render();
    }
}

//----------------------------------------------------------------------------
void vtkImageCroppingRegionsWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "CroppingRegionFlags: " << this->CroppingRegionFlags << endl;
  os << indent << "PlanePositions: " << endl
     << indent << "  In X: " << this->PlanePositions[0]
     << " to " << this->PlanePositions[1] << endl
     << indent << "  In Y: " << this->PlanePositions[2]
     << " to " << this->PlanePositions[3] << endl
     << indent << "  In Z: " << this->PlanePositions[4]
     << " to " << this->PlanePositions[5] << endl;
  os << indent << "Slice: " << this->Slice << endl;
  os << indent << "SliceOrientation: " << this->SliceOrientation << endl;
  os << indent << "VolumeMapper: " << this->VolumeMapper << endl;
}
