/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAxisActor.cxx
  Thanks:    Kathleen Bonnell, B Division, Lawrence Livermore Nat'l Laboratory

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#include "vtkAxisActor.h"

#include "vtkAxisFollower.h"
#include "vtkCamera.h"
#include "vtkCellArray.h"
#include "vtkCoordinate.h"
#include "vtkFollower.h"
#include "vtkTextRenderer.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkProperty2D.h"
#include "vtkStringArray.h"
#include "vtkTextActor.h"
#include "vtkTextProperty.h"
#include "vtkVectorText.h"
#include "vtkViewport.h"

vtkStandardNewMacro(vtkAxisActor);
vtkCxxSetObjectMacro(vtkAxisActor, Camera, vtkCamera);
vtkCxxSetObjectMacro(vtkAxisActor,LabelTextProperty,vtkTextProperty);
vtkCxxSetObjectMacro(vtkAxisActor,TitleTextProperty,vtkTextProperty);

// ****************************************************************
// Instantiate this object.
// ****************************************************************

vtkAxisActor::vtkAxisActor()
{
  this->Point1Coordinate = vtkCoordinate::New();
  this->Point1Coordinate->SetCoordinateSystemToWorld();
  this->Point1Coordinate->SetValue(0.0, 0.0, 0.0);

  this->Point2Coordinate = vtkCoordinate::New();
  this->Point2Coordinate->SetCoordinateSystemToWorld();
  this->Point2Coordinate->SetValue(0.75, 0.0, 0.0);

  this->Camera = NULL;
  this->Title = NULL;
  this->MinorTicksVisible = 1;
  this->MajorTickSize = 1.0;
  this->MinorTickSize = 0.5;
  this->TickLocation = VTK_TICKS_INSIDE;
  this->Range[0] = 0.0;
  this->Range[1] = 1.0;

  this->Bounds[0] = this->Bounds[2] = this->Bounds[4] = -1;
  this->Bounds[1] = this->Bounds[3] = this->Bounds[5] = 1;

  this->LabelFormat = new char[8];
  sprintf(this->LabelFormat, "%s", "%-#6.3g");

  this->TitleTextProperty = vtkTextProperty::New();
  this->TitleTextProperty->SetColor(0.,0.,0.);
  this->TitleTextProperty->SetFontFamilyToArial();

  this->TitleVector = vtkVectorText::New();
  this->TitleMapper = vtkPolyDataMapper::New();
  this->TitleMapper->SetInputConnection(
    this->TitleVector->GetOutputPort());
  this->TitleActor = vtkAxisFollower::New();
  this->TitleActor->SetMapper(this->TitleMapper);
  this->TitleActor->SetEnableDistanceLOD(0);
  this->TitleActor2D = vtkTextActor::New();

  this->NumberOfLabelsBuilt = 0;
  this->LabelVectors = NULL;
  this->LabelMappers = NULL;
  this->LabelActors = NULL;
  this->LabelActors2D = NULL;

  this->LabelTextProperty = vtkTextProperty::New();
  this->LabelTextProperty->SetColor(0.,0.,0.);
  this->LabelTextProperty->SetFontFamilyToArial();

  this->AxisLines = vtkPolyData::New();
  this->AxisLinesMapper = vtkPolyDataMapper::New();
  this->AxisLinesMapper->SetInputData(this->AxisLines);
  this->AxisLinesActor = vtkActor::New();
  this->AxisLinesActor->SetMapper(this->AxisLinesMapper);
  this->Gridlines = vtkPolyData::New();
  this->GridlinesMapper = vtkPolyDataMapper::New();
  this->GridlinesMapper->SetInputData(this->Gridlines);
  this->GridlinesActor = vtkActor::New();
  this->GridlinesActor->SetMapper(this->GridlinesMapper);
  this->InnerGridlines = vtkPolyData::New();
  this->InnerGridlinesMapper = vtkPolyDataMapper::New();
  this->InnerGridlinesMapper->SetInputData(this->InnerGridlines);
  this->InnerGridlinesActor = vtkActor::New();
  this->InnerGridlinesActor->SetMapper(this->InnerGridlinesMapper);
  this->Gridpolys = vtkPolyData::New();
  this->GridpolysMapper = vtkPolyDataMapper::New();
  this->GridpolysMapper->SetInputData(this->Gridpolys);
  this->GridpolysActor = vtkActor::New();
  this->GridpolysActor->SetMapper(this->GridpolysMapper);

  this->AxisVisibility = 1;
  this->TickVisibility = 1;
  this->LabelVisibility = 1;
  this->TitleVisibility = 1;

  this->DrawGridlines = 0;
  this->DrawGridlinesOnly = 0;
  this->GridlineXLength = 1.;
  this->GridlineYLength = 1.;
  this->GridlineZLength = 1.;

  this->DrawInnerGridlines = 0;

  this->DrawGridpolys = 0;

  this->AxisType = VTK_AXIS_TYPE_X;
  //
  // AxisPosition denotes which of the four possibilities in relation
  // to the bounding box.  An x-Type axis with min min, means the x-axis
  // at minimum y and minimum z values of the bbox.
  //
  this->AxisPosition = VTK_AXIS_POS_MINMIN;

  this->LastLabelStart = 100000;

  this->LastAxisPosition = -1;
  this->LastTickLocation = -1;
  this->LastTickVisibility = -1;
  this->LastDrawGridlines = -1;
  this->LastDrawInnerGridlines = -1;
  this->LastDrawGridpolys = -1;
  this->LastMinorTicksVisible = -1;
  this->LastRange[0] = -1.0;
  this->LastRange[1] = -1.0;

  this->MinorTickPts = vtkPoints::New();
  this->MajorTickPts = vtkPoints::New();
  this->GridlinePts  = vtkPoints::New();
  this->InnerGridlinePts  = vtkPoints::New();
  this->GridpolyPts  = vtkPoints::New();

  this->AxisHasZeroLength = false;

  this->MinorStart = 0.;
  //this->MajorStart = 0.;
  for(int i=0;i<3;i++)
    {
    this->MajorStart[i] = 0.;
    }
  this->DeltaMinor = 1.;
  //this->DeltaMajor = 1.;
  for(int i=0;i<3;i++)
    {
    this->DeltaMajor[i] = 1.;
    }

  this->MinorRangeStart = 0.;
  this->MajorRangeStart = 0.;
  this->DeltaRangeMinor = 1.;
  this->DeltaRangeMajor = 1.;

  this->CalculateTitleOffset = 1;
  this->CalculateLabelOffset = 1;

  // Instance variables specific to 2D mode
  this->Use2DMode = 0;
  this->SaveTitlePosition = 0;
  this->TitleConstantPosition[0] = this->TitleConstantPosition[1] = 0.;
  this->VerticalOffsetXTitle2D = -40.;
  this->HorizontalOffsetYTitle2D = -50.;
  this->LastMinDisplayCoordinate[0] = 0;
  this->LastMinDisplayCoordinate[1] = 0;
  this->LastMinDisplayCoordinate[2] = 0;
  this->LastMaxDisplayCoordinate[0] = 0;
  this->LastMaxDisplayCoordinate[1] = 0;
  this->LastMaxDisplayCoordinate[2] = 0;

   // 0: All locations
  this->DrawGridlinesLocation = this->LastDrawGridlinesLocation = 0;

  // reset the base
  for(int i=0;i<3;i++)
    {
    this->AxisBaseForX[i] = this->AxisBaseForY[i] = this->AxisBaseForZ[i] = 0.0;
    }
  this->AxisBaseForX[0] = this->AxisBaseForY[1] = this->AxisBaseForZ[2] = 1.0;
  this->AxisOnOrigin = 0;
}

// ****************************************************************
vtkAxisActor::~vtkAxisActor()
{
  this->SetCamera(NULL);

  if (this->Point1Coordinate)
    {
    this->Point1Coordinate->Delete();
    this->Point1Coordinate = NULL;
    }

  if (this->Point2Coordinate)
    {
    this->Point2Coordinate->Delete();
    this->Point2Coordinate = NULL;
    }

  if (this->LabelFormat)
    {
    delete [] this->LabelFormat;
    this->LabelFormat = NULL;
    }

  if (this->TitleVector)
    {
    this->TitleVector->Delete();
    this->TitleVector = NULL;
    }
  if (this->TitleMapper)
    {
    this->TitleMapper->Delete();
    this->TitleMapper = NULL;
    }
  if (this->TitleActor)
    {
    this->TitleActor->Delete();
    this->TitleActor = NULL;
    }
  if (this->TitleActor2D)
    {
    this->TitleActor2D->Delete();
    this->TitleActor2D = NULL;
    }

  if (this->Title)
    {
    delete [] this->Title;
    this->Title = NULL;
    }

  if (this->TitleTextProperty)
    {
    this->TitleTextProperty->Delete();
    this->TitleTextProperty = NULL;
    }

  if (this->LabelMappers != NULL)
    {
    for (int i=0; i < this->NumberOfLabelsBuilt; i++)
      {
      this->LabelVectors[i]->Delete();
      this->LabelMappers[i]->Delete();
      this->LabelActors[i]->Delete();
      this->LabelActors2D[i]->Delete();
      }
    this->NumberOfLabelsBuilt = 0;
    delete [] this->LabelVectors;
    delete [] this->LabelMappers;
    delete [] this->LabelActors;
    delete [] this->LabelActors2D;
    this->LabelVectors = NULL;
    this->LabelMappers = NULL;
    this->LabelActors = NULL;
    this->LabelActors2D = NULL;
    }
  if (this->LabelTextProperty)
    {
    this->LabelTextProperty->Delete();
    this->LabelTextProperty = NULL;
    }

  if (this->AxisLines)
    {
    this->AxisLines->Delete();
    this->AxisLines = NULL;
    }
  if (this->AxisLinesMapper)
    {
    this->AxisLinesMapper->Delete();
    this->AxisLinesMapper = NULL;
    }
  if (this->AxisLinesActor)
    {
    this->AxisLinesActor->Delete();
    this->AxisLinesActor = NULL;
    }

  if (this->Gridlines)
    {
    this->Gridlines->Delete();
    this->Gridlines = NULL;
    }
  if (this->GridlinesMapper)
    {
    this->GridlinesMapper->Delete();
    this->GridlinesMapper = NULL;
    }
  if (this->GridlinesActor)
    {
    this->GridlinesActor->Delete();
    this->GridlinesActor = NULL;
    }

  if (this->InnerGridlines)
    {
    this->InnerGridlines->Delete();
    this->InnerGridlines = NULL;
    }
  if (this->InnerGridlinesMapper)
    {
    this->InnerGridlinesMapper->Delete();
    this->InnerGridlinesMapper = NULL;
    }
  if (this->InnerGridlinesActor)
    {
    this->InnerGridlinesActor->Delete();
    this->InnerGridlinesActor = NULL;
    }

  if (this->Gridpolys)
    {
    this->Gridpolys->Delete();
    this->Gridpolys = NULL;
    }
  if (this->GridpolysMapper)
    {
    this->GridpolysMapper->Delete();
    this->GridpolysMapper = NULL;
    }
  if (this->GridpolysActor)
    {
    this->GridpolysActor->Delete();
    this->GridpolysActor = NULL;
    }

  if (this->MinorTickPts)
    {
    this->MinorTickPts ->Delete();
    this->MinorTickPts = NULL;
    }
  if (this->MajorTickPts)
    {
    this->MajorTickPts->Delete();
    this->MajorTickPts = NULL;
    }
  if (this->GridlinePts)
    {
    this->GridlinePts->Delete();
    this->GridlinePts = NULL;
    }
  if (this->InnerGridlinePts)
    {
    this->InnerGridlinePts->Delete();
    this->InnerGridlinePts = NULL;
    }
  if (this->GridpolyPts)
    {
    this->GridpolyPts->Delete();
    this->GridpolyPts = NULL;
    }
}

// ****************************************************************
void vtkAxisActor::ReleaseGraphicsResources(vtkWindow *win)
{
  this->TitleActor->ReleaseGraphicsResources(win);
  this->TitleActor2D->ReleaseGraphicsResources(win);
  for (int i=0; i < this->NumberOfLabelsBuilt; i++)
    {
    this->LabelActors[i]->ReleaseGraphicsResources(win);
    this->LabelActors2D[i]->ReleaseGraphicsResources(win);
    }
  this->AxisLinesActor->ReleaseGraphicsResources(win);
  this->GridlinesActor->ReleaseGraphicsResources(win);
  this->InnerGridlinesActor->ReleaseGraphicsResources(win);
  this->GridpolysActor->ReleaseGraphicsResources(win);
}

// ****************************************************************
int vtkAxisActor::RenderOpaqueGeometry(vtkViewport *viewport)
{
  int i, renderedSomething=0;

  this->BuildAxis(viewport, false);

  // Everything is built, just have to render

  if (!this->AxisHasZeroLength)
    {
    if(this->DrawGridlinesOnly && this->DrawGridlines)
      {
      // Exit !!!!
      return this->GridlinesActor->RenderOpaqueGeometry(viewport);
      }
    if (this->Title != NULL && this->Title[0] != 0 && this->TitleVisibility)
      {
      if (this->Use2DMode == 0)
        {
        renderedSomething += this->TitleActor->RenderOpaqueGeometry(viewport);
        }
      else
        {
        renderedSomething += this->TitleActor2D->RenderOpaqueGeometry(viewport);
        }
      }
    if (this->AxisVisibility || this->TickVisibility)
      {
      renderedSomething += this->AxisLinesActor->RenderOpaqueGeometry(viewport);
      }
    if(this->DrawGridlines)
      {
      renderedSomething += this->GridlinesActor->RenderOpaqueGeometry(viewport);
      }
    if(this->DrawInnerGridlines)
      {
      renderedSomething += this->InnerGridlinesActor->RenderOpaqueGeometry(viewport);
      }
    if (this->LabelVisibility)
      {
      for (i=0; i<this->NumberOfLabelsBuilt; i++)
        {
        if (this->Use2DMode == 0)
          {
          renderedSomething +=
            this->LabelActors[i]->RenderOpaqueGeometry(viewport);
          }
        else
          {
          renderedSomething +=
            this->LabelActors2D[i]->RenderOpaqueGeometry(viewport);
          }
        }
      }
    }

  return renderedSomething;
}

// ****************************************************************
// Build the translucent poly actors and render.
// ****************************************************************
int vtkAxisActor::RenderTranslucentGeometry(vtkViewport *viewport)
{
  return this->RenderTranslucentPolygonalGeometry(viewport);
}

// ****************************************************************
// Build the translucent poly actors and render.
// ****************************************************************
int vtkAxisActor::RenderTranslucentPolygonalGeometry(vtkViewport *viewport)
{

  int renderedSomething=0;

  this->BuildAxis(viewport, false);

  // Everything is built, just have to render

  if (!this->AxisHasZeroLength && !this->DrawGridlinesOnly)
    {
    if(this->DrawGridpolys)
      {
      renderedSomething += this->GridpolysActor->RenderTranslucentPolygonalGeometry(viewport);
      }
    }

  return renderedSomething;
}

// ****************************************************************
// Render the 2d annotations.
// ****************************************************************
int vtkAxisActor::RenderOverlay(vtkViewport *viewport)
{
  int i, renderedSomething=0;

  // Everything is built, just have to render
  if (!this->AxisHasZeroLength && !this->DrawGridlinesOnly)
    {
    if( this->Use2DMode == 1 )
      {
      renderedSomething += this->TitleActor2D->RenderOverlay(viewport);
      }
    if (this->LabelVisibility)
      {
      for (i=0; i<this->NumberOfLabelsBuilt; i++)
        {
        if (this->Use2DMode == 1)
          {
          renderedSomething += this->LabelActors2D[i]->RenderOverlay(viewport);
          }
        }
      }
    }

  return renderedSomething;
}

// **************************************************************************
int vtkAxisActor::HasTranslucentPolygonalGeometry()
{
  if (this->Visibility && !this->AxisHasZeroLength)
    {

    if (this->TitleVisibility)
      {
      if (this->Use2DMode)
        {
        if (this->TitleActor2D->HasTranslucentPolygonalGeometry())
          {
          return 1;
          }
        }
      else
        {
        if (this->TitleActor->HasTranslucentPolygonalGeometry())
          {
          return 1;
          }
        }
      }

    if (this->LabelVisibility)
      {
      if (this->Use2DMode)
        {
        for (int i = 0; i < this->NumberOfLabelsBuilt; ++i)
          {
          if (this->LabelActors2D[i]->HasTranslucentPolygonalGeometry())
            {
            return 1;
            } // end if
          } // end for
        } // end 2D
      else
        {
        for (int i = 0; i < this->NumberOfLabelsBuilt; ++i)
          {
          if (this->LabelActors[i]->HasTranslucentPolygonalGeometry())
            {
            return 1;
            } // end if
          } // end for
        } // end 3D
      } // end label vis

    if (this->AxisLinesActor->HasTranslucentPolygonalGeometry())
      {
      return 1;
      }

    if (this->DrawGridlines &&
        this->GridlinesActor->HasTranslucentPolygonalGeometry())
      {
      return 1;
      }

    if (this->DrawInnerGridlines &&
        this->InnerGridlinesActor->HasTranslucentPolygonalGeometry())
      {
      return 1;
      }

    if (this->DrawGridpolys &&
        this-GridpolysActor->HasTranslucentPolygonalGeometry())
      {
      return 1;
      }

    return this->Superclass::HasTranslucentPolygonalGeometry();

    } // end this vis

  return 0;
}

// **************************************************************************
// Perform some initialization, determine which Axis type we are
// **************************************************************************
void vtkAxisActor::BuildAxis(vtkViewport *viewport, bool force)
{
  // We'll do our computation in world coordinates. First determine the
  // location of the endpoints.
  double *x, p1[3], p2[3];
  x = this->Point1Coordinate->GetValue();
  p1[0] = x[0]; p1[1] = x[1]; p1[2] = x[2];
  x = this->Point2Coordinate->GetValue();
  p2[0] = x[0]; p2[1] = x[1]; p2[2] = x[2];

  //
  //  Test for axis of zero length.
  //
  if (p1[0] == p2[0] && p1[1] == p2[1] && p1[2] == p2[2])
    {
    vtkDebugMacro(<<"Axis has zero length, not building.");
    this->AxisHasZeroLength = true;
    return;
    }
  this->AxisHasZeroLength = false;

  if (!force && this->GetMTime() < this->BuildTime.GetMTime() &&
      viewport->GetMTime() < this->BuildTime.GetMTime())
    {
    return; //already built
    }

  vtkDebugMacro(<<"Rebuilding axis");

  if (force || this->GetProperty()->GetMTime() > this->BuildTime.GetMTime())
    {
      //this->AxisLinesActor->SetProperty(this->GetProperty());
    this->TitleActor->SetProperty(this->GetProperty());
    this->TitleActor->GetProperty()->SetColor(this->TitleTextProperty->GetColor());
    }

  //
  // Generate the axis and tick marks.
  //
  bool ticksRebuilt;
  ticksRebuilt = this->BuildTickPoints(p1, p2, force);

  bool tickVisChanged = this->TickVisibilityChanged();

  if (force || ticksRebuilt || tickVisChanged || this->LastDrawGridlinesLocation != this->DrawGridlinesLocation)
    {
    this->LastDrawGridlinesLocation = this->DrawGridlinesLocation;
    this->SetAxisPointsAndLines();
    }

  // If the ticks have been rebuilt it is more than likely
  // that the labels should follow...
  this->BuildLabels(viewport, force || ticksRebuilt);
  if (this->Use2DMode == 1)
    {
    this->BuildLabels2D(viewport, force || ticksRebuilt);
    }

  if (this->Title != NULL && this->Title[0] != 0)
    {
    this->BuildTitle(force || ticksRebuilt);
    if( this->Use2DMode == 1 )
      {
      this->BuildTitle2D(viewport, force || ticksRebuilt);
      }
    }

  this->LastAxisPosition = this->AxisPosition;

  this->LastRange[0] = this->Range[0];
  this->LastRange[1] = this->Range[1];
  this->BuildTime.Modified();
}

// ****************************************************************
//  Set label values and properties.
// ****************************************************************
void
vtkAxisActor::BuildLabels(vtkViewport *viewport, bool force)
{
  if (!force && !this->LabelVisibility)
    {
    return;
    }

  for (int i = 0; i < this->NumberOfLabelsBuilt; i++)
    {
    this->LabelActors[i]->SetCamera(this->Camera);
    this->LabelActors[i]->GetProperty()->SetColor(this->LabelTextProperty->GetColor());
    this->LabelActors[i]->SetOrientation(0., 0., this->LabelTextProperty->GetOrientation());

    if(!this->GetCalculateLabelOffset())
      {
      this->LabelActors[i]->SetAutoCenter(1);
      }
    }

  if (force || this->BuildTime.GetMTime() <  this->BoundsTime.GetMTime() ||
      this->AxisPosition != this->LastAxisPosition ||
      this->LastRange[0] != this->Range[0] ||
      this->LastRange[1] != this->Range[1])
    {
    this->SetLabelPositions(viewport, force);
    }
}

int vtkAxisActorMultiplierTable1[4] = { -1, -1, 1,  1};
int vtkAxisActorMultiplierTable2[4] = { -1,  1, 1, -1};

// *******************************************************************
// Determine and set scale factor and position for labels.
// *******************************************************************
void vtkAxisActor::SetLabelPositions(vtkViewport *viewport, bool force)
{
  if (!force && (!this->LabelVisibility || this->NumberOfLabelsBuilt == 0))
    {
    return;
    }

  double bounds[6], center[3], tick[3], pos[3], scale[3];
  int i = 0;
  int xmult = 0;
  int ymult = 0;

  switch (this->AxisType)
    {
    case VTK_AXIS_TYPE_X :
      xmult = 0;
      ymult = vtkAxisActorMultiplierTable1[this->AxisPosition];
      break;
    case VTK_AXIS_TYPE_Y :
      xmult = vtkAxisActorMultiplierTable1[this->AxisPosition];
      ymult = 0;
      break;
    case VTK_AXIS_TYPE_Z :
      xmult = vtkAxisActorMultiplierTable1[this->AxisPosition];
      ymult = vtkAxisActorMultiplierTable2[this->AxisPosition];
      break;
    }

  int ptIdx;
  //
  // xadjust & yadjust are used for positioning the label correctly
  // depending upon the 'orientation' of the axis as determined
  // by its position in view space (via transformed bounds).
  //
  double displayBounds[6] = { 0., 0., 0., 0., 0., 0.};
  this->TransformBounds(viewport, displayBounds);
  double xadjust = (displayBounds[0] > displayBounds[1] ? -1 : 1);
  double yadjust = (displayBounds[2] > displayBounds[3] ? -1 : 1);

  for (i=0; i < this->NumberOfLabelsBuilt &&
    i < this->MajorTickPts->GetNumberOfPoints(); i++)
    {
    ptIdx = 4*i + 1;
    this->MajorTickPts->GetPoint(ptIdx, tick);

    this->LabelActors[i]->GetMapper()->GetBounds(bounds);
    this->LabelActors[i]->GetScale(scale);

    if(this->CalculateLabelOffset)
      {
      double halfWidth  = (bounds[1] - bounds[0]) * 0.5 * scale[0];
      double halfHeight = (bounds[3] - bounds[2]) * 0.5 * scale[1];

      center[0] = tick[0] + xmult * (halfWidth  + this->MinorTickSize);
      center[1] = tick[1] + ymult * (halfHeight + this->MinorTickSize);
      center[2] = tick[2];

      pos[0] = (center[0] - xadjust *halfWidth);
      pos[1] = (center[1] - yadjust *halfHeight);
      pos[2] =  center[2];
      }
    else
      {
      center[0] = tick[0] ;
      center[1] = tick[1];
      center[2] = tick[2];

      pos[0] = center[0];
      pos[1] = center[1];
      pos[2] = center[2];
      }

    this->LabelActors[i]->SetPosition(pos[0], pos[1], pos[2]);
    }
}

// *******************************************************************
//  Set 2D label values and properties.
// *******************************************************************
void
vtkAxisActor::BuildLabels2D(vtkViewport *viewport, bool force)
{
  if (!force && (!this->LabelVisibility || this->NumberOfLabelsBuilt == 0))
    return;

  for (int i = 0; i < this->NumberOfLabelsBuilt; i++)
    {
    this->LabelActors2D[i]->GetProperty()->SetColor(this->LabelTextProperty->GetColor());
    this->LabelActors2D[i]->GetProperty()->SetOpacity(1);
    this->LabelActors2D[i]->GetTextProperty()->SetFontSize(14);
    this->LabelActors2D[i]->GetTextProperty()->SetVerticalJustificationToBottom();
    this->LabelActors2D[i]->GetTextProperty()->SetJustificationToLeft();
    }

  this->NeedBuild2D = this->BoundsDisplayCoordinateChanged(viewport);
  if (force || this->NeedBuild2D)
    {
    this->SetLabelPositions2D(viewport, force);
    }
}


// *******************************************************************
// Determine and set scale factor and position for 2D labels.
// *******************************************************************
void
vtkAxisActor::SetLabelPositions2D(vtkViewport *viewport, bool force)
{
  if (!force && (!this->LabelVisibility || this->NumberOfLabelsBuilt == 0) )
    return;

  int xmult = 0;
  int ymult = 0;
  double xcoeff = 0.;
  double ycoeff = 0.;

  // we are in 2D mode, so no Z axis
  switch (this->AxisType)
    {
    case VTK_AXIS_TYPE_X :
      xmult = 0;
      ymult = vtkAxisActorMultiplierTable1[this->AxisPosition];
      xcoeff = 0.5;
      ycoeff = 1.0;
      break;
    case VTK_AXIS_TYPE_Y :
      xmult = vtkAxisActorMultiplierTable1[this->AxisPosition];
      ymult = 0;
      xcoeff = 1.0;
      ycoeff = 0.5;
      break;
    }


  int ptIdx;
  //
  // xadjust & yadjust are used for positioning the label correctly
  // depending upon the 'orientation' of the axis as determined
  // by its position in view space (via transformed bounds).
  //
  double displayBounds[6] = { 0., 0., 0., 0., 0., 0.};
  this->TransformBounds(viewport, displayBounds);
  double xadjust = (displayBounds[0] > displayBounds[1] ? -1 : 1);
  double yadjust = (displayBounds[2] > displayBounds[3] ? -1 : 1);
  double transpos[3] = {0., 0., 0.};
  double center[3], tick[3], pos[2];

  vtkTextRenderer *tren = vtkTextRenderer::GetInstance();
  if (!tren)
    {
    vtkErrorMacro(<< "Unable to obtain the vtkTextRenderer instance!");
    return;
    }

  for (int i = 0; i < this->NumberOfLabelsBuilt; i++)
    {
    ptIdx = 4*i + 1;

    this->MajorTickPts->GetPoint(ptIdx, tick);

    center[0] = tick[0] + xmult * this->MinorTickSize;
    center[1] = tick[1] + ymult * this->MinorTickSize;
    center[2] = tick[2];

    viewport->SetWorldPoint(center[0], center[1], center[2], 1.0);
    viewport->WorldToDisplay();
    viewport->GetDisplayPoint(transpos);

    int bbox[4];
    if (!tren->GetBoundingBox(this->LabelActors2D[i]->GetTextProperty(),
                              this->LabelActors2D[i]->GetInput(), bbox))
      {
      vtkErrorMacro(<< "Unable to calculate bounding box for label "
                    << this->LabelActors2D[i]->GetInput());
      continue;
      }

    double width  = (bbox[1]-bbox[0]);
    double height = (bbox[3]-bbox[2]);

    pos[0] = (transpos[0] - xadjust*width*xcoeff);
    pos[1] = (transpos[1] - yadjust*height*ycoeff);
    this->LabelActors2D[i]->SetPosition( pos[0], pos[1] );
    }
}


// **********************************************************************
//  Determines scale and position for the Title.  Currently,
//  title can only be centered with respect to its axis.
// **********************************************************************
void vtkAxisActor::BuildTitle(bool force)
{
  this->NeedBuild2D = false;
  if (!force && !this->TitleVisibility)
    {
    return;
    }
  double labBounds[6], titleBounds[6], center[3], pos[3], scale[3];
  double labHeight, maxHeight = 0, labWidth, maxWidth = 0;
  double halfTitleWidth, halfTitleHeight;

  double *p1 = this->Point1Coordinate->GetValue();
  double *p2 = this->Point2Coordinate->GetValue();
  int xmult = 0;
  int ymult = 0;

  if (!force && this->LabelBuildTime.GetMTime() < this->BuildTime.GetMTime() &&
      this->BoundsTime.GetMTime() < this->BuildTime.GetMTime() &&
      this->AxisPosition == this->LastAxisPosition &&
      this->TitleTextTime.GetMTime() < this->BuildTime.GetMTime())
    {
    return;
    }

  this->NeedBuild2D = true;
  switch (this->AxisType)
    {
    case VTK_AXIS_TYPE_X :
      xmult = 0;
      ymult = vtkAxisActorMultiplierTable1[this->AxisPosition];
      break;
    case VTK_AXIS_TYPE_Y :
      xmult = vtkAxisActorMultiplierTable1[this->AxisPosition];
      ymult = 0;
      break;
    case VTK_AXIS_TYPE_Z :
      xmult = vtkAxisActorMultiplierTable1[this->AxisPosition];
      ymult = vtkAxisActorMultiplierTable2[this->AxisPosition];
      break;
    }
  //
  //  Title should be in relation to labels (if any)
  //  so find out information about them.
  //
  for (int i = 0; i < this->NumberOfLabelsBuilt; i++)
    {
    this->LabelActors[i]->GetMapper()->GetBounds(labBounds);
    this->LabelActors[i]->GetScale(scale);
    labWidth = (labBounds[1] - labBounds[0])*scale[0];
    maxWidth = (labWidth > maxWidth ? labWidth : maxWidth);
    labHeight = (labBounds[3] - labBounds[2])*scale[1];
    maxHeight = (labHeight > maxHeight ? labHeight : maxHeight);
    }

  this->TitleVector->SetText(this->Title);

  this->TitleActor->GetProperty()->SetColor(this->TitleTextProperty->GetColor());
  this->TitleActor->SetCamera(this->Camera);
  this->TitleActor->SetPosition(p2[0], p2[1], p2[2]);
  this->TitleActor->GetMapper()->GetBounds(titleBounds);
  this->TitleActor->GetScale(scale);
  if(!this->GetCalculateTitleOffset())
    {
    this->TitleActor->SetAutoCenter(1);
    }

  center[0] = p1[0] + (p2[0] - p1[0]) / 2.0;
  center[1] = p1[1] + (p2[1] - p1[1]) / 2.0;
  center[2] = p1[2] + (p2[2] - p1[2]) / 2.0;

  if(this->CalculateTitleOffset)
    {
    halfTitleWidth  = (titleBounds[1] - titleBounds[0]) * 0.5 * scale[0];
    halfTitleHeight = (titleBounds[3] - titleBounds[2]) * 0.5 * scale[1];
    center[0] += xmult * (halfTitleWidth + maxWidth);
    center[1] += ymult * (halfTitleHeight + 2*maxHeight);
    }

  pos[0] = center[0];
  pos[1] = center[1];
  pos[2] = center[2];

  this->TitleActor->SetPosition(pos[0], pos[1], pos[2]);
}

// **********************************************************************
//  Determines scale and position for the 2D Title.  Currently,
//  title can only be centered with respect to its axis.
// **********************************************************************
void
vtkAxisActor::BuildTitle2D(vtkViewport *viewport, bool force)
{
  if (!this->NeedBuild2D && !force && !this->TitleVisibility)
    {
    return;
    }

  // for textactor instead of follower
  this->TitleActor2D->SetInput( this->TitleVector->GetText() );
  this->TitleActor2D->GetProperty()->SetColor( this->TitleTextProperty->GetColor() );
  this->TitleActor2D->GetProperty()->SetOpacity(1);
  this->TitleActor2D->GetTextProperty()->SetFontSize(18);
  this->TitleActor2D->GetTextProperty()->SetVerticalJustificationToCentered();
  this->TitleActor2D->GetTextProperty()->SetJustificationToCentered();

  if (this->AxisType == VTK_AXIS_TYPE_Y)
    {
    if (strlen(this->TitleActor2D->GetInput()) > 2)
      {
      // warning : orientation have to be set on vtkTextActor and not on the vtkTextActor's vtkTextProperty
      // otherwise there is a strange effect (first letter is not align with the others)
      this->TitleActor2D->SetOrientation(90);
      }
    else
      {
      // if in the previous rendering, the orientation was set.
      this->TitleActor2D->SetOrientation(0);
      }
    }

  // stuff for 2D axis with TextActor
  double transpos[3];
  double* pos = this->TitleActor->GetPosition();
  viewport->SetWorldPoint(pos[0], pos[1],  pos[2], 1.0);
  viewport->WorldToDisplay();
  viewport->GetDisplayPoint(transpos);
  if (this->AxisType == VTK_AXIS_TYPE_X)
    {
    transpos[1] += this->VerticalOffsetXTitle2D;
    }
  else if (this->AxisType == VTK_AXIS_TYPE_Y)
    {
    transpos[0] += this->HorizontalOffsetYTitle2D;
    }
  if (transpos[1] < 10.) transpos[1] = 10.;
  if (transpos[0] < 10.) transpos[0] = 10.;
  if (this->SaveTitlePosition == 0)
    {
    this->TitleActor2D->SetPosition(transpos[0], transpos[1]);
    }
  else
    {
    if (this->SaveTitlePosition == 1)
      {
      TitleConstantPosition[0] = transpos[0];
      TitleConstantPosition[1] = transpos[1];
      this->SaveTitlePosition = 2;
      }
    this->TitleActor2D->SetPosition(TitleConstantPosition[0], TitleConstantPosition[1]);
    }
}

//
//  Transform the bounding box to display coordinates.  Used
//  in determining orientation of the axis.
//
void vtkAxisActor::TransformBounds(vtkViewport *viewport, double bnds[6])
{
  double minPt[3], maxPt[3], transMinPt[3], transMaxPt[3];
  minPt[0] = this->Bounds[0];
  minPt[1] = this->Bounds[2];
  minPt[2] = this->Bounds[4];
  maxPt[0] = this->Bounds[1];
  maxPt[1] = this->Bounds[3];
  maxPt[2] = this->Bounds[5];

  viewport->SetWorldPoint(minPt[0], minPt[1], minPt[2], 1.0);
  viewport->WorldToDisplay();
  viewport->GetDisplayPoint(transMinPt);
  viewport->SetWorldPoint(maxPt[0], maxPt[1], maxPt[2], 1.0);
  viewport->WorldToDisplay();
  viewport->GetDisplayPoint(transMaxPt);

  bnds[0] = transMinPt[0];
  bnds[2] = transMinPt[1];
  bnds[4] = transMinPt[2];
  bnds[1] = transMaxPt[0];
  bnds[3] = transMaxPt[1];
  bnds[5] = transMaxPt[2];
}

inline double ffix(double value)
{
  int ivalue = static_cast<int>(value);
  return static_cast<double>(ivalue);
}

inline double fsign(double value, double sign)
{
  value = fabs(value);
  if (sign < 0.)
    {
    value *= -1.;
    }
  return value;
}

// ****************************************************************
void vtkAxisActor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Title: " << (this->Title ? this->Title : "(none)") << "\n";
  os << indent << "Number Of Labels Built: "
     << this->NumberOfLabelsBuilt << "\n";
  os << indent << "Range: ("
     << this->Range[0] << ", "
     << this->Range[1] << ")\n";

  os << indent << "Label Format: " << this->LabelFormat << "\n";

  os << indent << "Axis Visibility: "
     << (this->AxisVisibility ? "On\n" : "Off\n");

  os << indent << "Tick Visibility: "
     << (this->TickVisibility ? "On\n" : "Off\n");

  os << indent << "Label Visibility: "
     << (this->LabelVisibility ? "On\n" : "Off\n");

  os << indent << "Title Visibility: "
     << (this->TitleVisibility ? "On\n" : "Off\n");

  os << indent << "Point1 Coordinate: " << this->Point1Coordinate << "\n";
  this->Point1Coordinate->PrintSelf(os, indent.GetNextIndent());

  os << indent << "Point2 Coordinate: " << this->Point2Coordinate << "\n";
  this->Point2Coordinate->PrintSelf(os, indent.GetNextIndent());

  os << indent << "AxisType: ";
  switch (this->AxisType)
    {
    case VTK_AXIS_TYPE_X:
      os << "X Axis" << endl;
      break;
    case VTK_AXIS_TYPE_Y:
      os << "Y Axis" << endl;
      break;
    case VTK_AXIS_TYPE_Z:
      os << "Z Axis" << endl;
      break;
    default:
      // shouldn't get here
      ;
    }

  os << indent << "DeltaMajor: "
     << this->DeltaMajor[0] << ","
     << this->DeltaMajor[1] << ","
     << this->DeltaMajor[2] << endl;
  os << indent << "DeltaMinor: " << this->DeltaMinor << endl;
  os << indent << "DeltaRangeMajor: " << this->DeltaRangeMajor << endl;
  os << indent << "DeltaRangeMinor: " << this->DeltaRangeMinor << endl;
  os << indent << "MajorRangeStart: " << this->MajorRangeStart << endl;
  os << indent << "MinorRangeStart: " << this->MinorRangeStart << endl;

  os << indent << "MinorTicksVisible: " << this->MinorTicksVisible << endl;

  os << indent << "TitleActor: ";
  if(this->TitleActor)
    {
    os << indent << "TitleActor: (" << this->TitleActor << ")\n";
    }
  else
    {
    os << "(none)" << endl;
    }

  os << indent << "Camera: ";
  if (this->Camera)
    {
    this->Camera->PrintSelf(os, indent);
    }
  else
    {
    os << "(none)" << endl;
    }

  os << indent << "MajorTickSize: " << this->MajorTickSize << endl;
  os << indent << "MinorTickSize: " << this->MinorTickSize << endl;

  os << indent << "DrawGridlines: " << this->DrawGridlines << endl;

  os << indent << "MajorStart: "
     << this->MajorStart[0] << ","
     << this->MajorStart[1] << ","
     << this->MajorStart[2] << endl;

  os << indent << "AxisPosition: " << this->AxisPosition << endl;

  os << indent << "GridlineXLength: " << this->GridlineXLength << endl;
  os << indent << "GridlineYLength: " << this->GridlineYLength << endl;
  os << indent << "GridlineZLength: " << this->GridlineZLength << endl;

  os << indent << "DrawInnerGridpolys: " << this->DrawGridpolys << endl;
  os << indent << "DrawInnerGridlines: " << this->DrawInnerGridlines << endl;

  os << indent << "TickLocation: " << this->TickLocation << endl;

  os << indent << "CalculateLabelOffset: " << this->CalculateLabelOffset << std::endl;
  os << indent << "CalculateTitleOffset: " << this->CalculateTitleOffset << std::endl;

  os << indent << "LabelTextProperty: " << this->LabelTextProperty << endl;
  os << indent << "TitleTextProperty: " << this->TitleTextProperty << endl;

  os << indent << "Use2DMode: " << this->Use2DMode << endl;
  os << indent << "SaveTitlePosition: " << this->SaveTitlePosition << endl;
  os << indent << "VerticalOffsetXTitle2D" << this->VerticalOffsetXTitle2D << endl;
  os << indent << "HorizontalOffsetYTitle2D" << this->HorizontalOffsetYTitle2D << endl;
  os << indent << "LastMinDisplayCoordinates: ("
     << this->LastMinDisplayCoordinate[0] << ", "
     << this->LastMinDisplayCoordinate[1] << ", "
     << this->LastMinDisplayCoordinate[2] << ")" << endl;
  os << indent << "LastMaxDisplayCoordinates: ("
     << this->LastMaxDisplayCoordinate[0] << ", "
     << this->LastMaxDisplayCoordinate[1] << ", "
     << this->LastMaxDisplayCoordinate[2] << ")" << endl;
  }

// **************************************************************************
// Sets text string for label vectors.  Allocates memory if necessary.
// **************************************************************************
void vtkAxisActor::SetLabels(vtkStringArray *labels)
{
  //
  // If the number of labels has changed, re-allocate the correct
  // amount of memory.
  //
  int i, numLabels = labels->GetNumberOfValues();
  if (this->NumberOfLabelsBuilt != numLabels)
    {
    if (this->LabelMappers != NULL)
      {
      for (i = 0; i < this->NumberOfLabelsBuilt; i++)
        {
        this->LabelVectors[i]->Delete();
        this->LabelMappers[i]->Delete();
        this->LabelActors[i]->Delete();
        this->LabelActors2D[i]->Delete();
        }
      delete [] this->LabelVectors;
      delete [] this->LabelMappers;
      delete [] this->LabelActors;
      delete [] this->LabelActors2D;
      }

    this->LabelVectors = new vtkVectorText * [numLabels];
    this->LabelMappers = new vtkPolyDataMapper * [numLabels];
    this->LabelActors  = new vtkAxisFollower * [numLabels];
    this->LabelActors2D = new vtkTextActor * [numLabels];

    for (i = 0; i < numLabels; i++)
      {
      this->LabelVectors[i] = vtkVectorText::New();
      this->LabelMappers[i] = vtkPolyDataMapper::New();
      this->LabelMappers[i]->SetInputConnection(
        this->LabelVectors[i]->GetOutputPort());
      this->LabelActors[i] = vtkAxisFollower::New();
      this->LabelActors[i]->SetMapper(this->LabelMappers[i]);
      this->LabelActors[i]->SetEnableDistanceLOD(0);
      this->LabelActors[i]->GetProperty()->SetAmbient(1.);
      this->LabelActors[i]->GetProperty()->SetDiffuse(0.);
      this->LabelActors[i]->GetProperty()->SetColor(this->LabelTextProperty->GetColor());
      this->LabelActors2D[i] = vtkTextActor::New();
      }
    }

  //
  // Set the label vector text.
  //
  for (i = 0; i < numLabels; i++)
    {
    this->LabelVectors[i]->SetText(labels->GetValue(i).c_str());
    this->LabelActors2D[i]->SetInput(this->LabelVectors[i]->GetText());
    }
  this->NumberOfLabelsBuilt = numLabels;
  this->LabelBuildTime.Modified();
}

// **************************************************************************
// Creates Poly data (lines) from tickmarks (minor/major), gridlines, and axis.
// **************************************************************************
void vtkAxisActor::SetAxisPointsAndLines()
{
  vtkPoints *pts = vtkPoints::New();
  vtkCellArray *lines = vtkCellArray::New();
  vtkCellArray *gridlines = vtkCellArray::New();
  vtkCellArray *innerGridlines = vtkCellArray::New();
  vtkCellArray *polys = vtkCellArray::New();
  this->AxisLines->SetPoints(pts);
  this->AxisLines->SetLines(lines);
  this->Gridlines->SetPoints(this->GridlinePts);
  this->Gridlines->SetLines(gridlines);
  this->InnerGridlines->SetPoints(this->InnerGridlinePts);
  this->InnerGridlines->SetLines(innerGridlines);
  this->Gridpolys->SetPoints(this->GridpolyPts);
  this->Gridpolys->SetPolys(polys);
  pts->Delete();
  lines->Delete();
  gridlines->Delete();
  innerGridlines->Delete();
  polys->Delete();
  int i, numMinorTickPts, numGridlines, numInnerGridlines, numMajorTickPts, numGridpolys, numLines;
  vtkIdType ptIds[2];
  vtkIdType polyPtIds[4];

  if (this->TickVisibility)
    {
    if (this->MinorTicksVisible)
      {
      // In 2D mode, the minorTickPts for yz portion or xz portion have been removed.
      numMinorTickPts = this->MinorTickPts->GetNumberOfPoints();
      for (i = 0; i < numMinorTickPts; i++)
        {
        pts->InsertNextPoint(this->MinorTickPts->GetPoint(i));
        }
      }
    numMajorTickPts = this->MajorTickPts->GetNumberOfPoints();
    if (this->Use2DMode == 0)
      {
      for (i = 0; i < numMajorTickPts; i++)
        {
        pts->InsertNextPoint(this->MajorTickPts->GetPoint(i));
        }
      }
    else
      {
      // In 2D mode, we don't need the pts for the xz portion or yz portion of the major tickmarks
      // majorTickPts not modified because all points are used for labels' positions.
      for (i = 0; i < numMajorTickPts; i+=4)
        {
        pts->InsertNextPoint(this->MajorTickPts->GetPoint(i));
        pts->InsertNextPoint(this->MajorTickPts->GetPoint(i+1));
        }
      }
    }

  // create lines
  numLines = pts->GetNumberOfPoints() / 2;
  for (i=0; i < numLines; i++)
    {
    ptIds[0] = 2*i;
    ptIds[1] = 2*i + 1;
    lines->InsertNextCell(2, ptIds);
    }

  if (this->AxisVisibility)
    {
    //first axis point
    ptIds[0] = pts->InsertNextPoint(this->Point1Coordinate->GetValue());
    //last axis point
    ptIds[1] = pts->InsertNextPoint(this->Point2Coordinate->GetValue());
    lines->InsertNextCell(2, ptIds);
    }
  // create grid lines
  if (this->DrawGridlines && this->AxisOnOrigin == 0)
    {
    numGridlines = this->GridlinePts->GetNumberOfPoints()/2;
    int start =
        (this->DrawGridlinesLocation == 0 || this->DrawGridlinesLocation == 1)
        ? 0 : 1;
    int increment = (this->DrawGridlinesLocation == 0) ? 1 : 2;
    for (i = start; i < numGridlines; i+=increment)
      {
      ptIds[0] = 2*i;
      ptIds[1] = 2*i + 1;
      gridlines->InsertNextCell(2, ptIds);
      }
    }

  // create inner grid lines
  if (this->DrawInnerGridlines && this->AxisOnOrigin == 0)
    {
    numInnerGridlines = this->InnerGridlinePts->GetNumberOfPoints()/2;
    for (i=0; i < numInnerGridlines; i++)
      {
      ptIds[0] = 2*i;
      ptIds[1] = 2*i + 1;
      innerGridlines->InsertNextCell(2, ptIds);
      }
    }

  // create polys (grid polys)
  if (this->DrawGridpolys && this->AxisOnOrigin == 0)
    {
    numGridpolys = this->GridpolyPts->GetNumberOfPoints()/4;
    for (i = 0; i < numGridpolys; i++)
      {
      polyPtIds[0] = 4*i;
      polyPtIds[1] = 4*i + 1;
      polyPtIds[2] = 4*i + 2;
      polyPtIds[3] = 4*i + 3;
      polys->InsertNextCell(4,polyPtIds);
      }
    }
}

// *********************************************************************
// Returns true if any tick vis attribute has changed since last check.
// *********************************************************************
bool vtkAxisActor::TickVisibilityChanged()
{
  bool retVal = (this->TickVisibility != this->LastTickVisibility) ||
                (this->DrawGridlines != this->LastDrawGridlines)   ||
                (this->MinorTicksVisible != this->LastMinorTicksVisible);

  this->LastTickVisibility = this->TickVisibility;
  this->LastDrawGridlines = this->DrawGridlines;
  this->LastMinorTicksVisible = this->MinorTicksVisible;

  return retVal;
}

// *********************************************************************
// Set the bounds for this actor to use.  Sets timestamp BoundsModified.
// *********************************************************************
void
vtkAxisActor::SetBounds(double b[6])
{
  if ((this->Bounds[0] != b[0]) ||
      (this->Bounds[1] != b[1]) ||
      (this->Bounds[2] != b[2]) ||
      (this->Bounds[3] != b[3]) ||
      (this->Bounds[4] != b[4]) ||
      (this->Bounds[5] != b[5]) )
    {
    for (int i = 0; i < 6; i++)
      {
      this->Bounds[i] = b[i];
      }
    this->BoundsTime.Modified();
    }
}

// *********************************************************************
// Retrieves the bounds of this actor.
// *********************************************************************
void vtkAxisActor::
SetBounds(double xmin, double xmax, double ymin, double ymax, double zmin, double zmax)
{
  if ((this->Bounds[0] != xmin) ||
      (this->Bounds[1] != xmax) ||
      (this->Bounds[2] != ymin) ||
      (this->Bounds[3] != ymax) ||
      (this->Bounds[4] != zmin) ||
      (this->Bounds[5] != zmax) )
    {
    this->Bounds[0] = xmin;
    this->Bounds[1] = xmax;
    this->Bounds[2] = ymin;
    this->Bounds[3] = ymax;
    this->Bounds[4] = zmin;
    this->Bounds[5] = zmax;

    this->BoundsTime.Modified();
    }
}

// *********************************************************************
// Retrieves the bounds of this actor.
// *********************************************************************
double* vtkAxisActor::GetBounds()
{
  return this->Bounds;
}

// *********************************************************************
// Retrieves the bounds of this actor.
// *********************************************************************

void vtkAxisActor::GetBounds(double b[6])
{
  for (int i = 0; i < 6; i++)
    {
    b[i] = this->Bounds[i];
    }
}

// *********************************************************************
// Method:  vtkAxisActor::ComputeMaxLabelLength
// *********************************************************************
double vtkAxisActor::ComputeMaxLabelLength(const double vtkNotUsed(center)[3])
{
  double bounds[6];
  double xsize, ysize;
  vtkProperty *newProp = this->NewLabelProperty();
  double maxXSize = 0;
  double maxYSize = 0;
  for (int i = 0; i < this->NumberOfLabelsBuilt; i++)
    {
    this->LabelActors[i]->SetCamera(this->Camera);
    this->LabelActors[i]->SetProperty(newProp);
    this->LabelActors[i]->GetMapper()->GetBounds(bounds);
    this->LabelActors[i]->GetProperty()->SetColor(this->LabelTextProperty->GetColor());
    xsize = bounds[1] - bounds[0];
    ysize = bounds[3] - bounds[2];
    maxXSize = (xsize > maxXSize ? xsize : maxXSize);
    maxYSize = (ysize > maxYSize ? ysize : maxYSize);
    }
  newProp->Delete();
  return sqrt(maxXSize*maxXSize + maxYSize*maxYSize);
}

// *********************************************************************
// Method:  vtkAxisActor::ComputeTitleLength
// *********************************************************************
double vtkAxisActor::ComputeTitleLength(const double vtkNotUsed(center)[3])
{
  double bounds[6];
  double xsize, ysize;
  double length;

  this->TitleVector->SetText(this->Title);
  this->TitleActor->SetCamera(this->Camera);
  vtkProperty * newProp = this->NewTitleProperty();
  this->TitleActor->SetProperty(newProp);
  newProp->Delete();
  this->TitleActor->GetMapper()->GetBounds(bounds);
  this->TitleActor->GetProperty()->SetColor(this->TitleTextProperty->GetColor());
  xsize = bounds[1] - bounds[0];
  ysize = bounds[3] - bounds[2];
  length = sqrt(xsize*xsize + ysize*ysize);

  return length;
}

// *********************************************************************
void vtkAxisActor::SetLabelScale(const double s)
{
  for (int i=0; i < this->NumberOfLabelsBuilt; i++)
    {
    this->LabelActors[i]->SetScale(s);
    }
}

// *********************************************************************
void vtkAxisActor::SetTitleScale(const double s)
{
  this->TitleActor->SetScale(s);
}

// *********************************************************************
void vtkAxisActor::SetTitle(const char *t)
{
  if (this->Title == NULL && t == NULL)
    {
    return;
    }
  if (this->Title && (!strcmp(this->Title, t)))
    {
    return;
    }
  if (this->Title)
    {
    delete [] this->Title;
    }
  if (t)
    {
    this->Title = new char[strlen(t)+1];
    strcpy(this->Title, t);
    }
  else
    {
    this->Title = NULL;
    }
  this->TitleTextTime.Modified();
  this->Modified();
}

// ****************************************************************************
void vtkAxisActor::SetAxisLinesProperty(vtkProperty *prop)
{
  this->AxisLinesActor->SetProperty(prop);
  this->Modified();
}

// ****************************************************************************
vtkProperty* vtkAxisActor::GetAxisLinesProperty()
{
  return this->AxisLinesActor->GetProperty();
}

// ****************************************************************************
void vtkAxisActor::SetGridlinesProperty(vtkProperty *prop)
{
  this->GridlinesActor->SetProperty(prop);
  this->Modified();
}

// ****************************************************************************
vtkProperty* vtkAxisActor::GetGridlinesProperty()
{
  return this->GridlinesActor->GetProperty();
}

// ****************************************************************************
void vtkAxisActor::SetInnerGridlinesProperty(vtkProperty *prop)
{
  this->InnerGridlinesActor->SetProperty(prop);
  this->Modified();
}

// ****************************************************************************
vtkProperty* vtkAxisActor::GetInnerGridlinesProperty()
{
  return this->InnerGridlinesActor->GetProperty();
}

// ****************************************************************************
void vtkAxisActor::SetGridpolysProperty(vtkProperty *prop)
{
  this->GridpolysActor->SetProperty(prop);
  this->Modified();
}

// ****************************************************************************
vtkProperty* vtkAxisActor::GetGridpolysProperty()
{
  return this->GridpolysActor->GetProperty();
}

// ****************************************************************************
vtkProperty* vtkAxisActor::NewTitleProperty()
{
  vtkProperty *newProp = vtkProperty::New();
  newProp->DeepCopy(this->GetProperty());
  newProp->SetColor(this->TitleTextProperty->GetColor());
  // We pass the opacity in the line offset.
  //newProp->SetOpacity(this->TitleTextProperty->GetLineOffset());
  return newProp;
}

// ****************************************************************************
vtkProperty* vtkAxisActor::NewLabelProperty()
{
  vtkProperty *newProp = vtkProperty::New();
  newProp->DeepCopy(this->GetProperty());
  newProp->SetColor(this->LabelTextProperty->GetColor());
  // We pass the opacity in the line offset.
  //newProp->SetOpacity(this->LabelTextProperty->GetLineOffset());
  return newProp;
}


// ****************************************************************************
double vtkAxisActor::GetDeltaMajor(int axis){
  if(axis>=0 && axis<=2)
    {
    return (this->DeltaMajor[axis]);
    }
  return 0;
}

void vtkAxisActor::SetDeltaMajor(int axis, double value){
  if(axis>=0 && axis<=2)
    {
    this->DeltaMajor[axis] = value;
    }
}

// ****************************************************************************
double vtkAxisActor::GetMajorStart(int axis){
  if(axis>=0 && axis<=2)
    {
    return (this->MajorStart[axis]);
    }
  return 0;
}

// ****************************************************************************
void vtkAxisActor::SetMajorStart(int axis, double value){
  if(axis>=0 && axis<=2)
    {
    this->MajorStart[axis] = value;
    }
}

// ****************************************************************************
bool vtkAxisActor::BoundsDisplayCoordinateChanged(vtkViewport *viewport)
{
  double transMinPt[3], transMaxPt[3];
  viewport->SetWorldPoint(this->Bounds[0], this->Bounds[2], this->Bounds[4], 1.0);
  viewport->WorldToDisplay();
  viewport->GetDisplayPoint(transMinPt);
  viewport->SetWorldPoint(this->Bounds[1], this->Bounds[3], this->Bounds[5], 1.0);
  viewport->WorldToDisplay();
  viewport->GetDisplayPoint(transMaxPt);

  if( this->LastMinDisplayCoordinate[0] != transMinPt[0]
      || this->LastMinDisplayCoordinate[1] != transMinPt[1]
      || this->LastMinDisplayCoordinate[2] != transMinPt[2]
      || this->LastMaxDisplayCoordinate[0] != transMaxPt[0]
      || this->LastMaxDisplayCoordinate[1] != transMaxPt[1]
      || this->LastMaxDisplayCoordinate[2] != transMaxPt[2] )
    {
    int i = 0;
    for( i=0 ; i<3 ; ++i )
      {
      this->LastMinDisplayCoordinate[i] = transMinPt[i];
      this->LastMaxDisplayCoordinate[i] = transMaxPt[i];
      }
    return true;
    }

  return false;
}
//---------------------------------------------------------------------------
// endpoint-related methods
vtkCoordinate* vtkAxisActor::GetPoint1Coordinate()
{
  vtkDebugMacro(<< this->GetClassName() << " (" << this
                << "): returning Point1 Coordinate address "
                << this->Point1Coordinate );
  return this->Point1Coordinate;
}

//---------------------------------------------------------------------------
vtkCoordinate* vtkAxisActor::GetPoint2Coordinate()
{
  vtkDebugMacro(<< this->GetClassName() << " (" << this
                << "): returning Point2 Coordinate address "
                << this->Point2Coordinate );
  return this->Point2Coordinate;
}

//---------------------------------------------------------------------------
void vtkAxisActor::SetPoint1(double x, double y, double z)
{
  this->Point1Coordinate->SetValue(x, y, z);
}

//---------------------------------------------------------------------------
void vtkAxisActor::SetPoint2(double x, double y, double z)
{
  this->Point2Coordinate->SetValue(x, y, z);
}

//---------------------------------------------------------------------------
double* vtkAxisActor::GetPoint1()
{
  return this->Point1Coordinate->GetValue();
}

//---------------------------------------------------------------------------
double* vtkAxisActor::GetPoint2()
{
  return this->Point2Coordinate->GetValue();
}
// **************************************************************************
// Creates points for ticks (minor, major, gridlines) in correct position
// for a generic axis.
// **************************************************************************
bool vtkAxisActor::BuildTickPoints(double p1[3], double p2[3], bool force)
{
  // Prevent any unwanted computation
  if (!force && (this->AxisPosition == this->LastAxisPosition) &&
      (this->TickLocation == this->LastTickLocation ) &&
      (this->BoundsTime.GetMTime() < this->BuildTime.GetMTime()) &&
      (this->Point1Coordinate->GetMTime() < this->BuildTickPointsTime.GetMTime()) &&
      (this->Point2Coordinate->GetMTime() < this->BuildTickPointsTime.GetMTime()) &&
      (this->Range[0] == this->LastRange[0]) &&
      (this->Range[1] == this->LastRange[1]))
    {
    return false;
    }

  // Local tmp vars
  double uPointInside[3], vPointInside[3], uPointOutside[3], vPointOutside[3];
  double gridPointClosest[3], gridPointFarest[3], gridPointU[3], gridPointV[3];
  double innerGridPointClosestU[3], innerGridPointClosestV[3];
  double innerGridPointFarestU[3], innerGridPointFarestV[3];
  double deltaVector[3];
  double axisLength, axisShift, rangeScale, nbIterationAsDouble;
  int nbTicks, i, nbIteration, uIndex, vIndex;
  uIndex = vIndex = 0;
  bool hasOrthogonalVectorBase =
      (this->AxisBaseForX[0] == 1 && this->AxisBaseForX[1] == 0 && this->AxisBaseForX[2] == 0
       && this->AxisBaseForY[0] == 0 && this->AxisBaseForY[1] == 1 && this->AxisBaseForY[2] == 0
       && this->AxisBaseForZ[0] == 0 && this->AxisBaseForZ[1] == 0 && this->AxisBaseForZ[2] == 1);

  // Reset previous objects
  this->MinorTickPts->Reset();
  this->MajorTickPts->Reset();
  this->GridlinePts->Reset();
  this->InnerGridlinePts->Reset();
  this->GridpolyPts->Reset();

  // As we assume that the Axis is not necessery alined to the absolute X/Y/Z
  // axis, we will convert the absolut XYZ information to relative one
  // using a base composed as follow (axis, u, v)
  double uGridLength, vGridLength;
  uGridLength = vGridLength = 0;
  double *axisVector, *uVector, *vVector;
  axisVector = uVector = vVector = NULL;
  int uMult = vtkAxisActorMultiplierTable1[this->AxisPosition];
  int vMult = vtkAxisActorMultiplierTable2[this->AxisPosition];

  switch(this->AxisType)
    {
  case VTK_AXIS_TYPE_X:
    uGridLength = this->GridlineYLength;
    vGridLength = this->GridlineZLength;
    axisVector = this->AxisBaseForX;
    uVector = this->AxisBaseForY;
    vVector = this->AxisBaseForZ;
    uIndex = 1; vIndex = 2;
    break;
  case VTK_AXIS_TYPE_Y:
    uGridLength = this->GridlineXLength;
    vGridLength = this->GridlineZLength;
    uVector = this->AxisBaseForX;
    axisVector = this->AxisBaseForY;
    vVector = this->AxisBaseForZ;
    uIndex = 0; vIndex = 2;
    break;
  case VTK_AXIS_TYPE_Z:
    uGridLength = this->GridlineXLength;
    vGridLength = this->GridlineYLength;
    uVector = this->AxisBaseForX;
    vVector = this->AxisBaseForY;
    axisVector = this->AxisBaseForZ;
    uIndex = 0; vIndex = 1;
    break;
    }

  // **************************************************************************
  // Build Minor Ticks
  // **************************************************************************
  {
  // - Initialize all points to be on the axis
  for(i=0;i<3;i++)
    {
    uPointInside[i] = vPointInside[i] = uPointOutside[i] = vPointOutside[i] = p1[i];
    deltaVector[i] = (p2[i] - p1[i]);
    }
  axisLength = vtkMath::Norm(deltaVector);
  rangeScale = axisLength/(this->Range[1] - this->Range[0]);

  // - Reduce the deltaVector to correspond to a tick step
  vtkMath::Normalize(deltaVector);
  for(i=0;i<3;i++)
    {
    deltaVector[i] *= this->DeltaMinor;
    }

  // - Move outside points if needed (Axis -> Outside)
  if (this->TickLocation == VTK_TICKS_OUTSIDE || this->TickLocation == VTK_TICKS_BOTH)
    {
    for(i=0;i<3;i++)
      {
      uPointOutside[i] += uVector[i] * uMult * this->MinorTickSize;
      vPointOutside[i] += vVector[i] * vMult * this->MinorTickSize;
      }
    }

  // - Move inside points if needed (Axis -> Inside)
  if (this->TickLocation == VTK_TICKS_INSIDE || this->TickLocation == VTK_TICKS_BOTH)
    {
    for(i=0;i<3;i++)
      {
      uPointInside[i] -= uVector[i] * uMult * this->MinorTickSize;
      vPointInside[i] -= vVector[i] * vMult * this->MinorTickSize;
      }
    }

  // - Add the initial shift if any
  axisShift = (this->MinorRangeStart - this->Range[0])*rangeScale;
  for(i=0;i<3;i++)
    {
    uPointInside[i] += axisVector[i] * axisShift;
    vPointInside[i] += axisVector[i] * axisShift;
    uPointOutside[i] += axisVector[i] * axisShift;
    vPointOutside[i] += axisVector[i] * axisShift;
    }

  // - Insert tick points along the axis using the deltaVector
  nbIterationAsDouble = axisLength / vtkMath::Norm(deltaVector);
  nbIteration = vtkMath::Floor(nbIterationAsDouble+2*DBL_EPSILON);
  nbIteration = (nbIteration < VTK_MAX_TICKS) ? nbIteration : VTK_MAX_TICKS;
  for (nbTicks = 0; nbTicks < nbIteration; nbTicks++)
    {
    // axis/u side
    this->MinorTickPts->InsertNextPoint(uPointInside);
    this->MinorTickPts->InsertNextPoint(uPointOutside);
    vtkMath::Add(deltaVector, uPointInside, uPointInside);
    vtkMath::Add(deltaVector, uPointOutside, uPointOutside);
    if( this->Use2DMode == 0 )
      {
      // axis/v side
      this->MinorTickPts->InsertNextPoint(vPointInside);
      this->MinorTickPts->InsertNextPoint(vPointOutside);
      vtkMath::Add(deltaVector, vPointInside, vPointInside);
      vtkMath::Add(deltaVector, vPointOutside, vPointOutside);
      }
    }
  }
  // **************************************************************************
  // Build Gridline + GridPoly points + InnerGrid (Only for Orthonormal base)
  // **************************************************************************
  {
  // - Initialize all points to be on the axis
  for(i=0;i<3;i++)
    {
    gridPointClosest[i] = gridPointFarest[i] = gridPointU[i] = gridPointV[i] = p1[i];
    deltaVector[i] = (p2[i] - p1[i]);
    }

  // - Reduce the deltaVector to correspond to a major tick step
  vtkMath::Normalize(deltaVector);
  for(i=0;i<3;i++)
    {
    deltaVector[i] *= this->DeltaMajor[this->AxisType];
    }

  // - Move base points
  for(i=0;i<3;i++)
    {
    gridPointU[i] -= uVector[i] * uMult * uGridLength;
    gridPointV[i] -= vVector[i] * vMult * vGridLength;
    gridPointFarest[i] -= uVector[i] * uMult * uGridLength + vVector[i] * vMult * vGridLength;
    }

  // - Add the initial shift if any
  axisShift = (this->MajorRangeStart - this->Range[0])*rangeScale;
  for(i=0;i<3;i++)
    {
    gridPointU[i] += axisVector[i] * axisShift;
    gridPointV[i] += axisVector[i] * axisShift;
    gridPointFarest[i] += axisVector[i] * axisShift;
    gridPointClosest[i] += axisVector[i] * axisShift;
    }

  // - Insert Gridlines points along the axis using the DeltaMajor vector
  nbIterationAsDouble = (axisLength - axisShift) / vtkMath::Norm(deltaVector);
  nbIteration = vtkMath::Floor(nbIterationAsDouble+2*FLT_EPSILON) + 1;
  nbIteration = (nbIteration < VTK_MAX_TICKS) ? nbIteration : VTK_MAX_TICKS;
  for (nbTicks = 0; nbTicks < nbIteration; nbTicks++)
    {
    // Closest U
    this->GridlinePts->InsertNextPoint(gridPointClosest);
    this->GridlinePts->InsertNextPoint(gridPointU);

    // Farest U
    this->GridlinePts->InsertNextPoint(gridPointFarest);
    this->GridlinePts->InsertNextPoint(gridPointU);

    // Closest V
    this->GridlinePts->InsertNextPoint(gridPointClosest);
    this->GridlinePts->InsertNextPoint(gridPointV);

    // Farest V
    this->GridlinePts->InsertNextPoint(gridPointFarest);
    this->GridlinePts->InsertNextPoint(gridPointV);

    // PolyPoints
    this->GridpolyPts->InsertNextPoint(gridPointClosest);
    this->GridpolyPts->InsertNextPoint(gridPointU);
    this->GridpolyPts->InsertNextPoint(gridPointFarest);
    this->GridpolyPts->InsertNextPoint(gridPointV);

    // Move forward along the axis
    for(i=0;i<3;i++)
      {
      gridPointClosest[i] += deltaVector[i];
      gridPointU[i] += deltaVector[i];
      gridPointFarest[i] += deltaVector[i];
      gridPointV[i] += deltaVector[i];
      }
    }

  // - Insert InnerGridLines points

  // We can only handle inner grid line with orthonormal base, otherwise
  // we would need to change the API of AxisActor which we don't want for
  // backward compatibility.
  if(hasOrthogonalVectorBase)
    {
    double axis, u, v;
    axis = this->MajorStart[this->AxisType];
    innerGridPointClosestU[vIndex] = this->GetBounds()[vIndex*2];
    innerGridPointFarestU[vIndex] = this->GetBounds()[vIndex*2+1];
    innerGridPointClosestV[uIndex] = this->GetBounds()[uIndex*2];
    innerGridPointFarestV[uIndex] = this->GetBounds()[uIndex*2+1];
    while (axis <= p2[this->AxisType])
        {
        innerGridPointClosestU[this->AxisType]
            = innerGridPointClosestV[this->AxisType]
            = innerGridPointFarestU[this->AxisType]
            = innerGridPointFarestV[this->AxisType]
            = axis;

        // u lines
        u = this->MajorStart[uIndex];
        while (u <= p2[uIndex] && this->DeltaMajor[uIndex] > 0)
          {
          innerGridPointClosestU[uIndex]
              = innerGridPointFarestU[uIndex]
              = u;
          this->InnerGridlinePts->InsertNextPoint(innerGridPointClosestU);
          this->InnerGridlinePts->InsertNextPoint(innerGridPointFarestU);
          u += this->DeltaMajor[uIndex];
          }

        // v lines
        v = this->MajorStart[vIndex];
        while (v <= p2[vIndex] && this->DeltaMajor[vIndex] > 0)
          {
          innerGridPointClosestV[vIndex]
              = innerGridPointFarestV[vIndex]
              = v;
          this->InnerGridlinePts->InsertNextPoint(innerGridPointClosestV);
          this->InnerGridlinePts->InsertNextPoint(innerGridPointFarestV);
          v += this->DeltaMajor[vIndex];
          }

        axis += this->DeltaMajor[this->AxisType];
        }
    }
  }
  // **************************************************************************
  // Build Major ticks
  // **************************************************************************
  {
  // Delta vector is already initialized with the Major tick scale
  // - Initialize all points to be on the axis
  for(i=0;i<3;i++)
    {
    uPointInside[i] = vPointInside[i] = uPointOutside[i] = vPointOutside[i] = p1[i];
    }

  // - Move outside points if needed (Axis -> Outside)
  if (this->TickLocation == VTK_TICKS_OUTSIDE || this->TickLocation == VTK_TICKS_BOTH)
    {
    for(i=0;i<3;i++)
      {
      uPointOutside[i] += uVector[i] * uMult * this->MajorTickSize;
      vPointOutside[i] += vVector[i] * vMult * this->MajorTickSize;
      }
    }

  // - Move inside points if needed (Axis -> Inside)
  if (this->TickLocation == VTK_TICKS_INSIDE || this->TickLocation == VTK_TICKS_BOTH)
    {
    for(i=0;i<3;i++)
      {
      uPointInside[i] -= uVector[i] * uMult * this->MajorTickSize;
      vPointInside[i] -= vVector[i] * vMult * this->MajorTickSize;
      }
    }

  // - Add the initial shift if any
  for(i=0;i<3;i++)
    {
    uPointInside[i] += axisVector[i] * axisShift;
    vPointInside[i] += axisVector[i] * axisShift;
    uPointOutside[i] += axisVector[i] * axisShift;
    vPointOutside[i] += axisVector[i] * axisShift;
    }

  // - Insert tick points along the axis using the deltaVector
  for (nbTicks = 0; nbTicks < nbIteration; nbTicks++)
    {
    // axis/u side
    this->MajorTickPts->InsertNextPoint(uPointInside);
    this->MajorTickPts->InsertNextPoint(uPointOutside);
    vtkMath::Add(deltaVector, uPointInside, uPointInside);
    vtkMath::Add(deltaVector, uPointOutside, uPointOutside);

    // axis/v side
    this->MajorTickPts->InsertNextPoint(vPointInside);
    this->MajorTickPts->InsertNextPoint(vPointOutside);
    vtkMath::Add(deltaVector, vPointInside, vPointInside);
    vtkMath::Add(deltaVector, vPointOutside, vPointOutside);
    }
  }

  this->BuildTickPointsTime.Modified();
  this->LastTickLocation = this->TickLocation;
  return true;
}
