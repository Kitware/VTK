/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCubeAxesActor.cxx
  Thanks:    Kathleen Bonnell, B Division, Lawrence Livermore National Lab

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
#include "vtkCubeAxesActor.h"

#include "vtkAxisActor.h"
#include "vtkAxisFollower.h"
#include "vtkBoundingBox.h"
#include "vtkCamera.h"
#include "vtkCoordinate.h"
#include "vtkFollower.h"
#include "vtkFrustumSource.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPlanes.h"
#include "vtkProp3DAxisFollower.h"
#include "vtkProperty.h"
#include "vtkStringArray.h"
#include "vtkTextProperty.h"
#include "vtkViewport.h"

#include <algorithm>
#include <sstream>
#include <string>

vtkStandardNewMacro(vtkCubeAxesActor);
vtkCxxSetObjectMacro(vtkCubeAxesActor, Camera,vtkCamera);
// *************************************************************************
// Instantiate this object.
// *************************************************************************
vtkCubeAxesActor::vtkCubeAxesActor() : vtkActor()
{
  this->Bounds[0] = -1.0; this->Bounds[1] = 1.0;
  this->Bounds[2] = -1.0; this->Bounds[3] = 1.0;
  this->Bounds[4] = -1.0; this->Bounds[5] = 1.0;
  for(int i=0; i < 6; ++i)
    {
    this->RenderedBounds[i] = this->Bounds[i];
    }

  this->OrientedBounds[0] = -1.0; this->OrientedBounds[1] = 1.0;
  this->OrientedBounds[2] = -1.0; this->OrientedBounds[3] = 1.0;
  this->OrientedBounds[4] = -1.0; this->OrientedBounds[5] = 1.0;

  // Disable oriented bounds and Axis origin
  this->UseOrientedBounds = this->UseAxisOrigin = 0;

  // Init default axis origin
  this->AxisOrigin[0] = this->AxisOrigin[1] = this->AxisOrigin[2] = 0.0;

  // Init default axis base
  this->AxisBaseForX[0] = this->AxisBaseForX[1] = this->AxisBaseForX[2] = 0;
  this->AxisBaseForY[0] = this->AxisBaseForY[1] = this->AxisBaseForY[2] = 0;
  this->AxisBaseForZ[0] = this->AxisBaseForZ[1] = this->AxisBaseForZ[2] = 0;
  this->AxisBaseForX[0] = this->AxisBaseForY[1] = this->AxisBaseForZ[2] = 1.0;

  this->RebuildAxes = true;

  this->Camera = NULL;

  this->FlyMode = VTK_FLY_CLOSEST_TRIAD;
  this->GridLineLocation = VTK_GRID_LINES_ALL;

  this->StickyAxes = 0;
  this->CenterStickyAxes = 1;

  // By default enable distance based LOD
  this->EnableDistanceLOD = 1;
  this->DistanceLODThreshold = .8;

  // By default enable view angle based LOD
  this->EnableViewAngleLOD = 1;
  this->ViewAngleLODThreshold = .2;

  // Title and label text properties
  for (int i = 0; i < 3; i++)
    {
    this->TitleTextProperty[i] = vtkTextProperty::New();
    this->TitleTextProperty[i]->SetColor(1.,1.,1.);
    this->TitleTextProperty[i]->SetFontFamilyToArial();
    this->TitleTextProperty[i]->SetFontSize(18);
    this->TitleTextProperty[i]->SetVerticalJustificationToCentered();
    this->TitleTextProperty[i]->SetJustificationToCentered();

    this->LabelTextProperty[i] = vtkTextProperty::New();
    this->LabelTextProperty[i]->SetColor(1.,1.,1.);
    this->LabelTextProperty[i]->SetFontFamilyToArial();
    this->LabelTextProperty[i]->SetFontSize(14);
    this->LabelTextProperty[i]->SetVerticalJustificationToBottom();
    this->LabelTextProperty[i]->SetJustificationToLeft();
    }

  // Axis lines
  this->XAxesLinesProperty = vtkProperty::New();
  this->YAxesLinesProperty = vtkProperty::New();
  this->ZAxesLinesProperty = vtkProperty::New();

  // Outer grid lines
  this->XAxesGridlinesProperty = vtkProperty::New();
  this->YAxesGridlinesProperty = vtkProperty::New();
  this->ZAxesGridlinesProperty = vtkProperty::New();

  // Inner grid lines
  this->XAxesInnerGridlinesProperty = vtkProperty::New();
  this->YAxesInnerGridlinesProperty = vtkProperty::New();
  this->ZAxesInnerGridlinesProperty = vtkProperty::New();
  this->XAxesInnerGridlinesProperty->SetColor(.3,.6,.1);
  this->YAxesInnerGridlinesProperty->SetColor(.3,.6,.1);
  this->ZAxesInnerGridlinesProperty->SetColor(.3,.6,.1);

  this->XAxesGridpolysProperty = vtkProperty::New();
  this->YAxesGridpolysProperty = vtkProperty::New();
  this->ZAxesGridpolysProperty = vtkProperty::New();
  this->XAxesGridpolysProperty->SetOpacity(.6);     // Default grid polys opacity
  this->YAxesGridpolysProperty->SetOpacity(.6);     // Default grid polys opacity
  this->ZAxesGridpolysProperty->SetOpacity(.6);     // Default grid polys opacity
  //this->XAxesGridpolysProperty->LightingOff();       // To be able to see the polys from high camera angles
  //this->YAxesGridpolysProperty->LightingOff();       // To be able to see the polys from high camera angles
  //this->ZAxesGridpolysProperty->LightingOff();       // To be able to see the polys from high camera angles

  this->ScreenSize  = 10.;
  this->LabelOffset = 20.;
  this->TitleOffset = 20.;

  for (int i = 0; i < NUMBER_OF_ALIGNED_AXIS; i++)
    {
    this->XAxes[i] = vtkAxisActor::New();
    this->XAxes[i]->SetTickVisibility(1);
    this->XAxes[i]->SetMinorTicksVisible(1);
    this->XAxes[i]->SetLabelVisibility(1);
    this->XAxes[i]->SetTitleVisibility(1);
    this->XAxes[i]->SetAxisTypeToX();
    this->XAxes[i]->SetAxisPosition(i);
    this->XAxes[i]->SetAxisLinesProperty(this->XAxesLinesProperty);
    this->XAxes[i]->SetGridlinesProperty(this->XAxesGridlinesProperty);
    this->XAxes[i]->SetInnerGridlinesProperty(this->XAxesInnerGridlinesProperty);
    this->XAxes[i]->SetGridpolysProperty(this->XAxesGridpolysProperty);
    this->XAxes[i]->SetLabelOffset(this->LabelOffset);
    this->XAxes[i]->SetTitleOffset(this->TitleOffset);
    this->XAxes[i]->SetScreenSize(this->ScreenSize);
    this->XAxes[i]->SetCalculateTitleOffset(0);
    this->XAxes[i]->SetCalculateLabelOffset(0);

    this->YAxes[i] = vtkAxisActor::New();
    this->YAxes[i]->SetTickVisibility(1);
    this->YAxes[i]->SetMinorTicksVisible(1);
    this->YAxes[i]->SetLabelVisibility(1);
    this->YAxes[i]->SetTitleVisibility(1);
    this->YAxes[i]->SetAxisTypeToY();
    this->YAxes[i]->SetAxisPosition(i);
    this->YAxes[i]->SetAxisLinesProperty(this->YAxesLinesProperty);
    this->YAxes[i]->SetGridlinesProperty(this->YAxesGridlinesProperty);
    this->YAxes[i]->SetInnerGridlinesProperty(this->YAxesInnerGridlinesProperty);
    this->YAxes[i]->SetGridpolysProperty(this->YAxesGridpolysProperty);
    this->YAxes[i]->SetLabelOffset(this->LabelOffset);
    this->YAxes[i]->SetTitleOffset(this->TitleOffset);
    this->YAxes[i]->SetScreenSize(this->ScreenSize);
    this->YAxes[i]->SetCalculateTitleOffset(0);
    this->YAxes[i]->SetCalculateLabelOffset(0);

    this->ZAxes[i] = vtkAxisActor::New();
    this->ZAxes[i]->SetTickVisibility(1);
    this->ZAxes[i]->SetMinorTicksVisible(1);
    this->ZAxes[i]->SetLabelVisibility(1);
    this->ZAxes[i]->SetTitleVisibility(1);
    this->ZAxes[i]->SetAxisTypeToZ();
    this->ZAxes[i]->SetAxisPosition(i);
    this->ZAxes[i]->SetAxisLinesProperty(this->ZAxesLinesProperty);
    this->ZAxes[i]->SetGridlinesProperty(this->ZAxesGridlinesProperty);
    this->ZAxes[i]->SetInnerGridlinesProperty(this->ZAxesInnerGridlinesProperty);
    this->ZAxes[i]->SetGridpolysProperty(this->ZAxesGridpolysProperty);
    this->ZAxes[i]->SetLabelOffset(this->LabelOffset);
    this->ZAxes[i]->SetTitleOffset(this->TitleOffset);
    this->ZAxes[i]->SetScreenSize(this->ScreenSize);
    this->ZAxes[i]->SetCalculateTitleOffset(0);
    this->ZAxes[i]->SetCalculateLabelOffset(0);

    // Pass information to axes followers.
    vtkAxisFollower* follower = this->XAxes[i]->GetTitleActor();
    follower->SetEnableDistanceLOD( this->EnableDistanceLOD );
    follower->SetDistanceLODThreshold( this->DistanceLODThreshold );
    follower->SetEnableViewAngleLOD( this->EnableViewAngleLOD );
    follower->SetViewAngleLODThreshold( this->ViewAngleLODThreshold );
    vtkProp3DAxisFollower* axisFollower = this->XAxes[i]->GetTitleProp3D();
    axisFollower->SetEnableDistanceLOD( this->EnableDistanceLOD );
    axisFollower->SetDistanceLODThreshold( this->DistanceLODThreshold );
    axisFollower->SetEnableViewAngleLOD( this->EnableViewAngleLOD );
    axisFollower->SetViewAngleLODThreshold( this->ViewAngleLODThreshold );

    follower = this->YAxes[i]->GetTitleActor();
    follower->SetEnableDistanceLOD( this->EnableDistanceLOD );
    follower->SetDistanceLODThreshold( this->DistanceLODThreshold );
    follower->SetEnableViewAngleLOD( this->EnableViewAngleLOD );
    follower->SetViewAngleLODThreshold( this->ViewAngleLODThreshold );
    axisFollower = this->YAxes[i]->GetTitleProp3D();
    axisFollower->SetEnableDistanceLOD( this->EnableDistanceLOD );
    axisFollower->SetDistanceLODThreshold( this->DistanceLODThreshold );
    axisFollower->SetEnableViewAngleLOD( this->EnableViewAngleLOD );
    axisFollower->SetViewAngleLODThreshold( this->ViewAngleLODThreshold );

    follower = this->ZAxes[i]->GetTitleActor();
    follower->SetEnableDistanceLOD( this->EnableDistanceLOD );
    follower->SetDistanceLODThreshold( this->DistanceLODThreshold );
    follower->SetEnableViewAngleLOD( this->EnableViewAngleLOD );
    follower->SetViewAngleLODThreshold( this->ViewAngleLODThreshold );
    axisFollower = this->ZAxes[i]->GetTitleProp3D();
    axisFollower->SetEnableDistanceLOD( this->EnableDistanceLOD );
    axisFollower->SetDistanceLODThreshold( this->DistanceLODThreshold );
    axisFollower->SetEnableViewAngleLOD( this->EnableViewAngleLOD );
    axisFollower->SetViewAngleLODThreshold( this->ViewAngleLODThreshold );
    }

  this->XTitle = new char[7];
  sprintf(this->XTitle, "%s", "X-Axis");
  this->XUnits = NULL;
  this->YTitle = new char[7];
  sprintf(this->YTitle, "%s", "Y-Axis");
  this->YUnits = NULL;
  this->ZTitle = new char[7];
  sprintf(this->ZTitle, "%s", "Z-Axis");
  this->ZUnits = NULL;

  this->ActualXLabel = 0;
  this->ActualYLabel = 0;
  this->ActualZLabel = 0;

  this->TickLocation = VTK_TICKS_INSIDE;

  this->XAxisVisibility = 1;
  this->YAxisVisibility = 1;
  this->ZAxisVisibility = 1;

  this->XAxisTickVisibility = 1;
  this->YAxisTickVisibility = 1;
  this->ZAxisTickVisibility = 1;

  this->XAxisMinorTickVisibility = 1;
  this->YAxisMinorTickVisibility = 1;
  this->ZAxisMinorTickVisibility = 1;

  this->XAxisLabelVisibility = 1;
  this->YAxisLabelVisibility = 1;
  this->ZAxisLabelVisibility = 1;

  this->DrawXGridlines = 0;
  this->DrawYGridlines = 0;
  this->DrawZGridlines = 0;

  this->DrawXInnerGridlines = 0;
  this->DrawYInnerGridlines = 0;
  this->DrawZInnerGridlines = 0;

  this->DrawXGridpolys = 0;
  this->DrawYGridpolys = 0;
  this->DrawZGridpolys = 0;

  this->XLabelFormat = new char[8];
  sprintf(this->XLabelFormat, "%s", "%-#6.3g");
  this->YLabelFormat = new char[8];
  sprintf(this->YLabelFormat, "%s", "%-#6.3g");
  this->ZLabelFormat = new char[8];
  sprintf(this->ZLabelFormat, "%s", "%-#6.3g");

  this->CornerOffset = 0.0;

  this->Inertia = 1;

  this->RenderCount = 0;

  this->InertiaLocs[0] = this->InertiaLocs[1] = this->InertiaLocs[2] = -1;

  this->RenderSomething = 0;

  this->LastUseOrientedBounds = 0;

  this->LastXPow = 0;
  this->LastYPow = 0;
  this->LastZPow = 0;

  this->UserXPow = 0;
  this->UserYPow = 0;
  this->UserZPow = 0;

  this->AutoLabelScaling = true;

  this->LastXAxisDigits = 3;
  this->LastYAxisDigits = 3;
  this->LastZAxisDigits = 3;

  this->LastXRange[0] = VTK_FLOAT_MAX;
  this->LastXRange[1] = VTK_FLOAT_MAX;
  this->LastYRange[0] = VTK_FLOAT_MAX;
  this->LastYRange[1] = VTK_FLOAT_MAX;
  this->LastZRange[0] = VTK_FLOAT_MAX;
  this->LastZRange[1] = VTK_FLOAT_MAX;

  this->LastBounds[0] = VTK_DOUBLE_MAX;
  this->LastBounds[1] = VTK_DOUBLE_MAX;
  this->LastBounds[2] = VTK_DOUBLE_MAX;
  this->LastBounds[3] = VTK_DOUBLE_MAX;
  this->LastBounds[4] = VTK_DOUBLE_MAX;
  this->LastBounds[5] = VTK_DOUBLE_MAX;

  this->LastFlyMode = -1;

  for (int i = 0; i < NUMBER_OF_ALIGNED_AXIS; i++)
    {
    this->RenderAxesX[i] = i;
    this->RenderAxesY[i] = i;
    this->RenderAxesZ[i] = i;
    }
  this->NumberOfAxesX = this->NumberOfAxesY = this->NumberOfAxesZ = 1;

  this->MustAdjustXValue = false;
  this->MustAdjustYValue = false;
  this->MustAdjustZValue = false;

  this->ForceXLabelReset = false;
  this->ForceYLabelReset = false;
  this->ForceZLabelReset = false;

  this->XAxisRange[0] = VTK_DOUBLE_MAX;
  this->XAxisRange[1] = VTK_DOUBLE_MAX;
  this->YAxisRange[0] = VTK_DOUBLE_MAX;
  this->YAxisRange[1] = VTK_DOUBLE_MAX;
  this->ZAxisRange[0] = VTK_DOUBLE_MAX;
  this->ZAxisRange[1] = VTK_DOUBLE_MAX;

  for (int i = 0; i < 3; ++i)
    {
    this->AxisLabels[i] = NULL;
    }
  this->LabelScale = -1.0;
  this->TitleScale = -1.0;
}

// *************************************************************************
void vtkCubeAxesActor::SetUseTextActor3D( int val )
{
  for( int i = 0 ; i < NUMBER_OF_ALIGNED_AXIS ; ++ i )
    {
    this->XAxes[i]->SetUseTextActor3D( val );
    this->YAxes[i]->SetUseTextActor3D( val );
    this->ZAxes[i]->SetUseTextActor3D( val );
    }
}

// *************************************************************************
int vtkCubeAxesActor::GetUseTextActor3D()
{
  // It is assumed that all axes have the same value
  return this->XAxes[0]->GetUseTextActor3D();
}

void vtkCubeAxesActor::SetUse2DMode( int val )
{
  for( int i = 0 ; i < NUMBER_OF_ALIGNED_AXIS ; ++ i )
    {
    this->XAxes[i]->SetUse2DMode( val );
    this->YAxes[i]->SetUse2DMode( val );
    this->ZAxes[i]->SetUse2DMode( val );
    }
  if( ! val )
    {
    this->SetZAxisVisibility( 1 );
    }
  else
    {
    this->SetZAxisVisibility( 0 );
    }
}

int vtkCubeAxesActor::GetUse2DMode()
{
  // It is assumed that all axes have the same value
  return this->XAxes[0]->GetUse2DMode();
}

void vtkCubeAxesActor::SetSaveTitlePosition( int val )
{
  // For 2D mode only :
  //   val = 0: no need to save position (3D axis)
  //   val = 1: positions have to be saved during the next render pass
  //   val = 2: positions are saved -> use them
  for( int i = 0 ; i < NUMBER_OF_ALIGNED_AXIS ; ++ i )
    {
    this->XAxes[i]->SetSaveTitlePosition( val );
    this->YAxes[i]->SetSaveTitlePosition( val );
    }
}

// ****************************************************************************
vtkCubeAxesActor::~vtkCubeAxesActor()
{
  this->SetCamera(NULL);

  for (int i = 0; i < NUMBER_OF_ALIGNED_AXIS; i++)
    {
    if (this->XAxes[i])
      {
      this->XAxes[i]->Delete();
      this->XAxes[i] = NULL;
      }
    if (this->YAxes[i])
      {
      this->YAxes[i]->Delete();
      this->YAxes[i] = NULL;
      }
    if (this->ZAxes[i])
      {
      this->ZAxes[i]->Delete();
      this->ZAxes[i] = NULL;
      }
    }

  if (this->XAxesLinesProperty)
    {
    this->XAxesLinesProperty->Delete();
    }
  if (this->XAxesGridlinesProperty)
    {
    this->XAxesGridlinesProperty->Delete();
    }
  if (this->XAxesInnerGridlinesProperty)
    {
    this->XAxesInnerGridlinesProperty->Delete();
    }
  if (this->XAxesGridpolysProperty)
    {
    this->XAxesGridpolysProperty->Delete();
    }
  if (this->YAxesLinesProperty)
    {
    this->YAxesLinesProperty->Delete();
    }
  if (this->YAxesGridlinesProperty)
    {
    this->YAxesGridlinesProperty->Delete();
    }
  if (this->YAxesInnerGridlinesProperty)
    {
    this->YAxesInnerGridlinesProperty->Delete();
    }
  if (this->YAxesGridpolysProperty)
    {
    this->YAxesGridpolysProperty->Delete();
    }
  if (this->ZAxesLinesProperty)
    {
    this->ZAxesLinesProperty->Delete();
    }
  if (this->ZAxesGridlinesProperty)
    {
    this->ZAxesGridlinesProperty->Delete();
    }
  if (this->ZAxesInnerGridlinesProperty)
    {
    this->ZAxesInnerGridlinesProperty->Delete();
    }
  if (this->ZAxesGridpolysProperty)
    {
    this->ZAxesGridpolysProperty->Delete();
    }

  for (int i = 0; i < 3; i++)
    {
    if(this->TitleTextProperty[i] != NULL)
      {
      this->TitleTextProperty[i]->Delete();
      }
    this->TitleTextProperty[i] = NULL;

    if(this->LabelTextProperty[i] != NULL)
      {
      this->LabelTextProperty[i]->Delete();
      }
    this->LabelTextProperty[i] = NULL;
    }

  delete [] this->XLabelFormat;
  this->XLabelFormat = NULL;

  delete [] this->YLabelFormat;
  this->YLabelFormat = NULL;

  delete [] this->ZLabelFormat;
  this->ZLabelFormat = NULL;

  delete [] this->XTitle;
  this->XTitle = NULL;

  delete [] this->YTitle;
  this->YTitle = NULL;

  delete [] this->ZTitle;
  this->ZTitle = NULL;

  delete [] this->XUnits;
  this->XUnits = NULL;

  delete [] this->YUnits;
  this->YUnits = NULL;

  delete [] this->ZUnits;
  this->ZUnits = NULL;

  delete [] this->ActualXLabel;
  this->ActualXLabel = NULL;

  delete [] this->ActualYLabel;
  this->ActualYLabel = NULL;

  delete [] this->ActualZLabel;
  this->ActualZLabel = NULL;
}

// *************************************************************************
// Project the bounding box and compute edges on the border of the bounding
// cube. Determine which parts of the edges are visible via intersection
// with the boundary of the viewport (minus borders).
// *************************************************************************
int vtkCubeAxesActor::RenderOpaqueGeometry(vtkViewport *viewport)
{
  static bool initialRender = true;
  return this->RenderGeometry(
        initialRender, viewport, true,
        &vtkAxisActor::RenderOpaqueGeometry);
}

// *************************************************************************
// Project the bounding box and compute edges on the border of the bounding
// cube. Determine which parts of the edges are visible via intersection
// with the boundary of the viewport (minus borders).
// *************************************************************************
int vtkCubeAxesActor::RenderTranslucentGeometry(vtkViewport *viewport)
{
  static bool initialRender = true;
  return this->RenderGeometry(
        initialRender, viewport, true,
        &vtkAxisActor::RenderTranslucentGeometry);
}

// *************************************************************************
// Project the bounding box and compute edges on the border of the bounding
// cube. Determine which parts of the edges are visible via intersection
// with the boundary of the viewport (minus borders).
// *************************************************************************
int vtkCubeAxesActor::RenderTranslucentPolygonalGeometry(vtkViewport *viewport)
{
  static bool initialRender = true;
  return this->RenderGeometry(
        initialRender, viewport, true,
        &vtkAxisActor::RenderTranslucentPolygonalGeometry);
}

// *************************************************************************
// RenderOverlay : render 2D annotations.
// *************************************************************************
int vtkCubeAxesActor::RenderOverlay(vtkViewport *viewport)
{
  static bool initialRender = true;
  return this->RenderGeometry(
        initialRender, viewport, false,
        &vtkAxisActor::RenderOverlay);
}

// --------------------------------------------------------------------------
int vtkCubeAxesActor::HasTranslucentPolygonalGeometry()
{
  if ((this->NumberOfAxesX > 0 &&
       this->XAxes[0]->HasTranslucentPolygonalGeometry()) ||
      (this->NumberOfAxesY > 0 &&
       this->YAxes[0]->HasTranslucentPolygonalGeometry()) ||
      (this->NumberOfAxesZ > 0 &&
       this->ZAxes[0]->HasTranslucentPolygonalGeometry()))
    {
    return 1;
    }

  return 0;
}

// --------------------------------------------------------------------------
// Do final adjustment of axes to control offset, etc.
void vtkCubeAxesActor::AdjustAxes(double bounds[6],
                                  double xCoords[NUMBER_OF_ALIGNED_AXIS][6],
                                  double yCoords[NUMBER_OF_ALIGNED_AXIS][6],
                                  double zCoords[NUMBER_OF_ALIGNED_AXIS][6],
                                  double xRange[2], double yRange[2],
                                  double zRange[2])
{
  xRange[0] = (this->XAxisRange[0] == VTK_DOUBLE_MAX ?
                                  bounds[0] : this->XAxisRange[0]);
  xRange[1] = (this->XAxisRange[1] == VTK_DOUBLE_MAX ?
                                  bounds[1] : this->XAxisRange[1]);
  yRange[0] = (this->YAxisRange[0] == VTK_DOUBLE_MAX ?
                                  bounds[2] : this->YAxisRange[0]);
  yRange[1] = (this->YAxisRange[1] == VTK_DOUBLE_MAX ?
                                  bounds[3] : this->YAxisRange[1]);
  zRange[0] = (this->ZAxisRange[0] == VTK_DOUBLE_MAX ?
                                  bounds[4] : this->ZAxisRange[0]);
  zRange[1] = (this->ZAxisRange[1] == VTK_DOUBLE_MAX ?
                                  bounds[5] : this->ZAxisRange[1]);

  if (this->StickyAxes)
    {
    // Change ranges according to transformation from original bounds to
    // viewport-constrained bounds
    double originalBounds[6];
    this->GetBounds(originalBounds);
    double range[6] = {xRange[0], xRange[1],
                       yRange[0], yRange[1],
                       zRange[0], zRange[1]};

    for (int i = 0; i < 3; ++i)
      {
      double length   = originalBounds[2*i+1] - originalBounds[2*i+0];
      double beginPercent = (bounds[2*i+0] - originalBounds[2*i+0]) / length;
      double endPercent   = (bounds[2*i+1] - originalBounds[2*i+0]) / length;
      double rangeLength = range[2*i+1] - range[2*i+0];
      double rangeStart = range[2*i+0];
      range[2*i+0] = rangeStart + rangeLength * beginPercent;
      range[2*i+1] = rangeStart + rangeLength * endPercent;
      }

    xRange[0] = range[0];
    xRange[1] = range[1];
    yRange[0] = range[2];
    yRange[1] = range[3];
    zRange[0] = range[4];
    zRange[1] = range[5];
    }

  const double xScale = (xRange[1] - xRange[0])/(bounds[1] - bounds[0]);
  const double yScale = (yRange[1] - yRange[0])/(bounds[3] - bounds[2]);
  const double zScale = (zRange[1] - zRange[0])/(bounds[5] - bounds[4]);

  // Pull back the corners if specified
  if (this->CornerOffset > 0.0)
    {
    for (int i = 0; i < NUMBER_OF_ALIGNED_AXIS; i++)
      {
      double ave;

      // x-axis
      ave = (xCoords[i][0] + xCoords[i][2]) / 2.0;
      xCoords[i][0] =
        xCoords[i][0] - this->CornerOffset * (xCoords[i][0] - ave);
      xCoords[i][2] =
        xCoords[i][2] - this->CornerOffset * (xCoords[i][2] - ave);

      ave = (xCoords[i][1] + xCoords[i][3]) / 2.0;
      xCoords[i][1] =
        xCoords[i][1] - this->CornerOffset * (xCoords[i][1] - ave);
      xCoords[i][3] =
        xCoords[i][3] - this->CornerOffset * (xCoords[i][3] - ave);

      ave = (xRange[1] + xRange[0]) / 2.0;
      xRange[0] = xRange[0] - this->CornerOffset * xScale * (xRange[0] - ave);
      xRange[1] = xRange[1] - this->CornerOffset * xScale * (xRange[1] - ave);

      // y-axis
      ave = (yCoords[i][0] + yCoords[i][2]) / 2.0;
      yCoords[i][0] =
        yCoords[i][0] - this->CornerOffset * (yCoords[i][0] - ave);
      yCoords[i][2] =
        yCoords[i][2] - this->CornerOffset * (yCoords[i][2] - ave);

      ave = (yCoords[i][1] + yCoords[i][3]) / 2.0;
      yCoords[i][1] =
        yCoords[i][1] - this->CornerOffset * (yCoords[i][1] - ave);
      yCoords[i][3] =
        yCoords[i][3] - this->CornerOffset * (yCoords[i][3] - ave);

      ave = (yRange[1] + yRange[0]) / 2.0;
      yRange[0] = yRange[0] - this->CornerOffset * yScale * (yRange[0] - ave);
      yRange[1] = yRange[1] - this->CornerOffset * yScale * (yRange[1] - ave);

      // z-axis
      ave = (zCoords[i][0] + zCoords[i][2]) / 2.0;
      zCoords[i][0] =
        zCoords[i][0] - this->CornerOffset * (zCoords[i][0] - ave);
      zCoords[i][2] =
        zCoords[i][2] - this->CornerOffset * (zCoords[i][2] - ave);

      ave = (zCoords[i][1] + zCoords[i][3]) / 2.0;
      zCoords[i][1] =
        zCoords[i][1] - this->CornerOffset * (zCoords[i][1] - ave);
      zCoords[i][3] =
        zCoords[i][3] - this->CornerOffset * (zCoords[i][3] - ave);

      ave = (zRange[1] + zRange[0]) / 2.0;
      zRange[0] = zRange[0] - this->CornerOffset * zScale * (zRange[0] - ave);
      zRange[1] = zRange[1] - this->CornerOffset * zScale * (zRange[1] - ave);
      }
    }
}

// *************************************************************************
// Screen size affects the screen offset as well.
// *************************************************************************
void vtkCubeAxesActor::SetScreenSize(double screenSize)
{
  this->ScreenSize = screenSize;
  for (int i = 0; i < NUMBER_OF_ALIGNED_AXIS; i++)
    {
    this->XAxes[i]->SetScreenSize(screenSize);
    this->YAxes[i]->SetScreenSize(screenSize);
    this->ZAxes[i]->SetScreenSize(screenSize);
    }

  this->Modified();
}

// *************************************************************************
// Offset between labels and axis.
// *************************************************************************
void vtkCubeAxesActor::SetLabelOffset(double offset)
{
  this->LabelOffset = offset;
  for (int i = 0; i < NUMBER_OF_ALIGNED_AXIS; i++)
    {
    this->XAxes[i]->SetLabelOffset(offset);
    this->YAxes[i]->SetLabelOffset(offset);
    this->ZAxes[i]->SetLabelOffset(offset);
    }

  this->Modified();
}

// *************************************************************************
// Offset between title and labels.
// *************************************************************************
void vtkCubeAxesActor::SetTitleOffset(double offset)
{
  this->TitleOffset = offset;
  for (int i = 0; i < NUMBER_OF_ALIGNED_AXIS; i++)
    {
    this->XAxes[i]->SetTitleOffset(offset);
    this->YAxes[i]->SetTitleOffset(offset);
    this->ZAxes[i]->SetTitleOffset(offset);
    }

  this->Modified();
}

// *************************************************************************
// Release any graphics resources that are being consumed by this actor.
// The parameter window could be used to determine which graphic
// resources to release.
// *************************************************************************
void vtkCubeAxesActor::ReleaseGraphicsResources(vtkWindow *win)
{
  for (int i = 0; i < NUMBER_OF_ALIGNED_AXIS; i++)
    {
    this->XAxes[i]->ReleaseGraphicsResources(win);
    this->YAxes[i]->ReleaseGraphicsResources(win);
    this->ZAxes[i]->ReleaseGraphicsResources(win);
    }
}

// ******************************************************************
void vtkCubeAxesActor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Bounds: \n";
  os << indent << "  Xmin,Xmax: (" << this->Bounds[0] << ", "
     << this->Bounds[1] << ")\n";
  os << indent << "  Ymin,Ymax: (" << this->Bounds[2] << ", "
     << this->Bounds[3] << ")\n";
  os << indent << "  Zmin,Zmax: (" << this->Bounds[4] << ", "
     << this->Bounds[5] << ")\n";


  os << indent << "XAxisRange: [" << this->XAxisRange[0] << ", "
    << this->XAxisRange[1] << "] " << endl;
  os << indent << "YAxisRange: [" << this->YAxisRange[0] << ", "
    << this->YAxisRange[1] << "] " << endl;
  os << indent << "ZAxisRange: [" << this->ZAxisRange[0] << ", "
    << this->ZAxisRange[1] << "] " << endl;

  os << indent << "ScreenSize: (" << this->ScreenSize << ")\n";

  if (this->Camera)
    {
    os << indent << "Camera:\n";
    this->Camera->PrintSelf(os,indent.GetNextIndent());
    }
  else
    {
    os << indent << "Camera: (none)\n";
    }

  os << indent << "RebuildAxes: " << this->RebuildAxes << endl;

  if (this->FlyMode == VTK_FLY_CLOSEST_TRIAD)
    {
    os << indent << "Fly Mode: CLOSEST_TRIAD\n";
    }
  else if (this->FlyMode == VTK_FLY_FURTHEST_TRIAD)
    {
    os << indent << "Fly Mode: FURTHEST_TRIAD\n";
    }
  else if (this->FlyMode == VTK_FLY_STATIC_TRIAD)
    {
    os << indent << "Fly Mode: STATIC_TRIAD\n";
    }
  else if (this->FlyMode == VTK_FLY_STATIC_EDGES)
    {
    os << indent << "Fly Mode: STATIC_EDGES\n";
    }
  else
    {
    os << indent << "Fly Mode: OUTER_EDGES\n";
    }

  os << indent << "EnableDistanceLOD: "
     << ( this->EnableDistanceLOD ? "On" : "Off" ) << endl;
  os << indent << "DistanceLODThreshold: "   << this->DistanceLODThreshold    << "\n";

  os << indent << "EnableViewAngleLOD: "
     << ( this->EnableViewAngleLOD ? "On" : "Off" ) << endl;
  os << indent << "ViewAngleLODThreshold: "   << this->ViewAngleLODThreshold    << "\n";

  os << indent << "X Axis Title: " << this->XTitle << "\n";
  os << indent << "Y Axis Title: " << this->YTitle << "\n";
  os << indent << "Z Axis Title: " << this->ZTitle << "\n";

  os << indent << "X Axis Visibility: "
     << (this->XAxisVisibility ? "On\n" : "Off\n");
  os << indent << "Y Axis Visibility: "
     << (this->YAxisVisibility ? "On\n" : "Off\n");
  os << indent << "Z Axis Visibility: "
     << (this->ZAxisVisibility ? "On\n" : "Off\n");

  os << indent << "X Axis Label Format: " << this->XLabelFormat << "\n";
  os << indent << "Y Axis Label Format: " << this->YLabelFormat << "\n";
  os << indent << "Z Axis Label Format: " << this->ZLabelFormat << "\n";
  os << indent << "Inertia: " << this->Inertia << "\n";
  os << indent << "Corner Offset: " << this->CornerOffset << "\n";

  os << indent << "XAxisTickVisibility: "
     << (this->XAxisTickVisibility ? "On" : "Off") << endl;
  os << indent << "YAxisTickVisibility: "
     << (this->YAxisTickVisibility ? "On" : "Off") << endl;
  os << indent << "ZAxisTickVisibility: "
     << (this->ZAxisTickVisibility ? "On" : "Off") << endl;

  os << indent << "XAxisMinorTickVisibility: "
     << (this->XAxisMinorTickVisibility ? "On" : "Off") << endl;
  os << indent << "YAxisMinorTickVisibility: "
     << (this->YAxisMinorTickVisibility ? "On" : "Off") << endl;
  os << indent << "ZAxisMinorTickVisibility: "
     << (this->ZAxisMinorTickVisibility ? "On" : "Off") << endl;

  os << indent << "XAxisLabelVisibility: "
     << (this->XAxisLabelVisibility ? "On" : "Off") << endl;
  os << indent << "YAxisLabelVisibility: "
     << (this->YAxisLabelVisibility ? "On" : "Off") << endl;
  os << indent << "ZAxisLabelVisibility: "
     << (this->ZAxisLabelVisibility ? "On" : "Off") << endl;

  os << indent << "XUnits: "
     << (this->XUnits ? this->XUnits : "(none)") << endl;
  os << indent << "YUnits: "
     << (this->YUnits ? this->YUnits : "(none)") << endl;
  os << indent << "ZUnits: "
     << (this->ZUnits ? this->ZUnits : "(none)") << endl;

  os << indent << "TickLocation: " << this->TickLocation << endl;

  os << indent << "DrawXGridlines: " << this->DrawXGridlines << endl;
  os << indent << "DrawYGridlines: " << this->DrawYGridlines << endl;
  os << indent << "DrawZGridlines: " << this->DrawZGridlines << endl;

  switch(this->GridLineLocation)
    {
  case VTK_GRID_LINES_ALL:
    os << indent << "GridLineLocation: VTK_GRID_LINES_ALL (0)" << endl;
    break;
  case VTK_GRID_LINES_CLOSEST:
    os << indent << "GridLineLocation: VTK_GRID_LINES_CLOSEST (1)" << endl;
    break;
  case VTK_GRID_LINES_FURTHEST:
    os << indent << "GridLineLocation: VTK_GRID_LINES_FURTHEST (2)" << endl;
    break;
    }

  os << indent << "DrawXInnerGridlines: " << this->DrawXInnerGridlines << endl;
  os << indent << "DrawYInnerGridlines: " << this->DrawYInnerGridlines << endl;
  os << indent << "DrawZInnerGridlines: " << this->DrawZInnerGridlines << endl;

  os << indent << "DrawXGridpolys: " << this->DrawXGridpolys << endl;
  os << indent << "DrawYGridpolys: " << this->DrawYGridpolys << endl;
  os << indent << "DrawZGridpolys: " << this->DrawZGridpolys << endl;


  os << indent << "UseOrientedBounds: " << this->UseOrientedBounds << endl;
  if(this->UseOrientedBounds)
    {
    os << indent << "OrientedBounds: \n";
    os << indent << "  Xmin,Xmax: (" << this->OrientedBounds[0] << ", "
       << this->OrientedBounds[1] << ")\n";
    os << indent << "  Ymin,Ymax: (" << this->OrientedBounds[2] << ", "
       << this->OrientedBounds[3] << ")\n";
    os << indent << "  Zmin,Zmax: (" << this->OrientedBounds[4] << ", "
       << this->OrientedBounds[5] << ")\n";
    }

  os << indent << "Base: \n";
  os << indent << "  For X: (" << this->AxisBaseForX[0] << ", "
     << this->AxisBaseForX[1] << ", " << this->AxisBaseForX[2] << ") \n";
  os << indent << "  For Y: (" << this->AxisBaseForY[0] << ", "
     << this->AxisBaseForY[1] << ", " << this->AxisBaseForY[2] << ") \n";
  os << indent << "  For Z: (" << this->AxisBaseForZ[0] << ", "
     << this->AxisBaseForZ[1] << ", " << this->AxisBaseForZ[2] << ") \n";

  os << indent << "UseAxisOrigin: " << this->UseAxisOrigin << endl;
  if(this->UseAxisOrigin)
    {
    os << indent << "AxisOrigin: (" << this->AxisOrigin[0] << ", "
       << this->AxisOrigin[1] << ", " << this->AxisOrigin[2] << ")" << endl;
    }
}

// --------------------------------------------------------------------------
void vtkCubeAxesActor::TransformBounds(vtkViewport *viewport,
                                       const double bounds[6],
                                       double pts[8][3])
{
  // The indices of points in the input bounding box are:
  //
  //        2-----3
  //       /|    /|
  //      / |   / |
  // +y  6--0--7--1  z-
  //     | /   | /
  //     |/    |/
  // -y  4-----5  z+
  //     -x    +x

  double x[3];

  //loop over verts of bounding box
  for (int idx = 0; idx < 8; ++idx)
    {
    vtkCubeAxesActor::GetBoundsPoint(idx, bounds, x);
    viewport->SetWorldPoint( x[0], x[1], x[2], 1. );
    viewport->WorldToDisplay();
    viewport->GetDisplayPoint( pts[idx] );
    }
}

// ***********************************************************************
//  Calculate the size (length) of major and minor ticks,
//  based on an average of the coordinate direction ranges.
//  Set the necessary Axes methods with the calculated information.
//
//  Returns:  false if tick size not recomputed, true otherwise.
// ***********************************************************************
bool vtkCubeAxesActor::ComputeTickSize(double bounds[6])
{
  bool xPropsChanged = this->LabelTextProperty[0]->GetMTime() > this->BuildTime.GetMTime();
  bool yPropsChanged = this->LabelTextProperty[1]->GetMTime() > this->BuildTime.GetMTime();
  bool zPropsChanged = this->LabelTextProperty[2]->GetMTime() > this->BuildTime.GetMTime();

  bool xRangeChanged = this->LastXRange[0] != this->XAxisRange[0] ||
                       this->LastXRange[1] != this->XAxisRange[1];

  bool yRangeChanged = this->LastYRange[0] != this->YAxisRange[0] ||
                       this->LastYRange[1] != this->YAxisRange[1];

  bool zRangeChanged = this->LastZRange[0] != this->ZAxisRange[0] ||
                       this->LastZRange[1] != this->ZAxisRange[1];

  bool boundsChanged = this->LastBounds[0] != bounds[0] ||
                       this->LastBounds[1] != bounds[1] ||
                       this->LastBounds[2] != bounds[2] ||
                       this->LastBounds[3] != bounds[3] ||
                       this->LastBounds[4] != bounds[4] ||
                       this->LastBounds[5] != bounds[5];

  if (!(xRangeChanged || yRangeChanged || zRangeChanged) &&
      !(xPropsChanged || yPropsChanged || zPropsChanged || boundsChanged))
    {
    // no need to re-compute ticksize.
    return false;
    }

  double xExt = bounds[1] - bounds[0];
  double yExt = bounds[3] - bounds[2];
  double zExt = bounds[5] - bounds[4];

  if (xRangeChanged || boundsChanged)
    {
    this->AdjustTicksComputeRange(this->XAxes, bounds[0], bounds[1]);
    this->BuildLabels(this->XAxes);
    this->UpdateLabels(this->XAxes, 0);
    }
  if (yRangeChanged || boundsChanged)
    {
    this->AdjustTicksComputeRange(this->YAxes, bounds[2], bounds[3]);
    this->BuildLabels(this->YAxes);
    this->UpdateLabels(this->YAxes, 1);
    }
  if (zRangeChanged || boundsChanged)
    {
    this->AdjustTicksComputeRange(this->ZAxes, bounds[4], bounds[5]);
    this->BuildLabels(this->ZAxes);
    this->UpdateLabels(this->ZAxes, 2);
    }

  // We give information on deltas for the inner grid lines generation
  for(int i = 0 ; i < NUMBER_OF_ALIGNED_AXIS ; i++)
    {
    for(int j = 0 ; j < 3 ; j++)
      {
      this->XAxes[i]->SetMajorStart(j,this->MajorStart[j]);
      this->XAxes[i]->SetDeltaMajor(j,this->DeltaMajor[j]);
      this->YAxes[i]->SetMajorStart(j,this->MajorStart[j]);
      this->YAxes[i]->SetDeltaMajor(j,this->DeltaMajor[j]);
      this->ZAxes[i]->SetMajorStart(j,this->MajorStart[j]);
      this->ZAxes[i]->SetDeltaMajor(j,this->DeltaMajor[j]);
      }
    }

  this->LastXRange[0] = (this->XAxisRange[0] == VTK_DOUBLE_MAX ?
                                  bounds[0] : this->XAxisRange[0]);
  this->LastXRange[1] = (this->XAxisRange[1] == VTK_DOUBLE_MAX ?
                                  bounds[1] : this->XAxisRange[1]);
  this->LastYRange[0] = (this->YAxisRange[0] == VTK_DOUBLE_MAX ?
                                  bounds[2] : this->YAxisRange[0]);
  this->LastYRange[1] = (this->YAxisRange[1] == VTK_DOUBLE_MAX ?
                                  bounds[3] : this->YAxisRange[1]);
  this->LastZRange[0] = (this->ZAxisRange[0] == VTK_DOUBLE_MAX ?
                                  bounds[4] : this->ZAxisRange[0]);
  this->LastZRange[1] = (this->ZAxisRange[1] == VTK_DOUBLE_MAX ?
                                  bounds[5] : this->ZAxisRange[1]);
  for(int i=0; i < 6; i++)
    {
    this->LastBounds[i] = bounds[i];
    }

  double major = 0.02 * (xExt + yExt + zExt) / 3.;
  double minor = 0.5 * major;
  for (int i = 0; i < NUMBER_OF_ALIGNED_AXIS; i++)
    {
    this->XAxes[i]->SetMajorTickSize(major);
    this->XAxes[i]->SetMinorTickSize(minor);

    this->YAxes[i]->SetMajorTickSize(major);
    this->YAxes[i]->SetMinorTickSize(minor);

    this->ZAxes[i]->SetMajorTickSize(major);
    this->ZAxes[i]->SetMinorTickSize(minor);

    this->XAxes[i]->SetGridlineXLength(xExt);
    this->XAxes[i]->SetGridlineYLength(yExt);
    this->XAxes[i]->SetGridlineZLength(zExt);

    this->YAxes[i]->SetGridlineXLength(xExt);
    this->YAxes[i]->SetGridlineYLength(yExt);
    this->YAxes[i]->SetGridlineZLength(zExt);

    this->ZAxes[i]->SetGridlineXLength(xExt);
    this->ZAxes[i]->SetGridlineYLength(yExt);
    this->ZAxes[i]->SetGridlineZLength(zExt);
    }
  return true;
}

// ****************************************************************************
//  Method: vtkCubeAxesActor::AdjustValues
//
//  Purpose:
//      If the range of values is too big or too small, put them in scientific
//      notation and changes the labels.
//
//  Arguments:
//      bnds     The min/max values in each coordinate direction:
//                 (min_x, max_x, min_y, max_y, min_z, max_x).
//
//  Note:       This code is partially stolen from old MeshTV code,
//              /meshtvx/toolkit/plotgrid.c, axlab[x|y].
//
// ****************************************************************************
void vtkCubeAxesActor::AdjustValues(const double xRange[2],
                                    const double yRange[2],
                                    const double zRange[2])
{
  int xPow, yPow, zPow;

  if (AutoLabelScaling)
    {
    if (this->AxisLabels[0] == NULL)
      {
      xPow = this->LabelExponent(xRange[0], xRange[1]);
      }
    else
      {
      xPow = 0;
      }
    if (this->AxisLabels[1] == NULL)
      {
      yPow = this->LabelExponent(yRange[0], yRange[1]);
      }
    else
      {
      yPow = 0;
      }
    if (this->AxisLabels[2] == NULL)
      {
      zPow = this->LabelExponent(zRange[0], zRange[1]);
      }
    else
      {
      zPow = 0;
      }
    }
  else
    {
    xPow = UserXPow;
    yPow = UserYPow;
    zPow = UserZPow;
    }

  std::string xTitle;
  if (xPow != 0)
    {
    if (!this->MustAdjustXValue || this->LastXPow != xPow)
      {
      this->ForceXLabelReset = true;
      }
    else
      {
      this->ForceXLabelReset = false;
      }
    this->MustAdjustXValue = true;

    std::ostringstream sstream;
    if (XUnits == NULL || XUnits[0] == '\0')
      {
      sstream << this->XTitle << " (x10^" << xPow << ")";
      }
    else
      {
      sstream << this->XTitle << " (x10^" << xPow << " " << XUnits << ")";
      }
    xTitle = sstream.str();
    }
  else
    {
    if (this->MustAdjustXValue)
      {
      this->Modified();
      this->ForceXLabelReset = true;
      }
    else
      {
      this->ForceXLabelReset = false;
      }
    this->MustAdjustXValue = false;

    if (XUnits == NULL || XUnits[0] == '\0')
      {
      xTitle = this->XTitle;
      }
    else
      {
      xTitle = std::string(this->XTitle) + " (" + XUnits + ")";
      }
    }

  std::string yTitle;
  if (yPow != 0)
    {
    if (!this->MustAdjustYValue || this->LastYPow != yPow)
      {
      this->ForceYLabelReset = true;
      }
    else
      {
      this->ForceYLabelReset = false;
      }
    this->MustAdjustYValue = true;

    std::ostringstream sstream;
    if (YUnits == NULL || YUnits[0] == '\0')
      {
      sstream << this->YTitle << " (x10^" << yPow << ")";
      }
    else
      {
      sstream << this->YTitle << " (x10^" << yPow << " " << YUnits << ")";
      }
    yTitle = sstream.str();
    }
  else
    {
    if (this->MustAdjustYValue)
      {
      this->Modified();
      this->ForceYLabelReset = true;
      }
    else
      {
      this->ForceYLabelReset = false;
      }
    this->MustAdjustYValue = false;
    if (YUnits == NULL || YUnits[0] == '\0')
      {
      yTitle = this->YTitle;
      }
    else
      {
      yTitle = std::string(this->YTitle) + " (" + YUnits + ")";
      }
    }

  std::string zTitle;
  if (zPow != 0)
    {
    if (!this->MustAdjustZValue || this->LastZPow != zPow)
      {
      this->ForceZLabelReset = true;
      }
    else
      {
      this->ForceZLabelReset = false;
      }
    this->MustAdjustZValue = true;

    std::ostringstream sstream;
    if (ZUnits == NULL || ZUnits[0] == '\0')
      {
      sstream << this->ZTitle << " (x10^" << zPow << ")";
      }
    else
      {
      sstream << this->ZTitle << " (x10^" << zPow << " " << ZUnits << ")";
      }
    zTitle = sstream.str();
    }
  else
    {
    if (this->MustAdjustZValue)
      {
      this->Modified();
      this->ForceZLabelReset = true;
      }
    else
      {
      this->ForceZLabelReset = false;
      }
    this->MustAdjustZValue = false;

    if (ZUnits == NULL || ZUnits[0] == '\0')
      {
      zTitle = this->ZTitle;
      }
    else
      {
      zTitle = std::string(this->ZTitle) + " (" + ZUnits + ")";
      }
    }

  this->LastXPow = xPow;
  this->LastYPow = yPow;
  this->LastZPow = zPow;

  this->SetActualXLabel(xTitle.c_str());
  this->SetActualYLabel(yTitle.c_str());
  this->SetActualZLabel(zTitle.c_str());
}

// ****************************************************************************
//  Method: vtkCubeAxesActor::AdjustRange
//
//  Purpose:
//    If the range is small, adjust the precision of the values displayed.
//
//  Arguments: ranges The minimum and maximum specified values in each
//    coordinate direction (min_x, max_x, min_y, max_y, min_z,
//    max_z). NOTE: This may not the bounds of the box in physical space
//    if the user has specified a custom axis range.
// ****************************************************************************
void vtkCubeAxesActor::AdjustRange(const double ranges[6])
{
  double xrange[2], yrange[2], zrange[2];

  xrange[0] = ranges[0];
  xrange[1] = ranges[1];
  yrange[0] = ranges[2];
  yrange[1] = ranges[3];
  zrange[0] = ranges[4];
  zrange[1] = ranges[5];

  if (this->LastXPow != 0)
    {
    xrange[0] /= pow(10., this->LastXPow);
    xrange[1] /= pow(10., this->LastXPow);
    }
  if (this->LastYPow != 0)
    {
    yrange[0] /= pow(10., this->LastYPow);
    yrange[1] /= pow(10., this->LastYPow);
    }
  if (this->LastZPow != 0)
    {
    zrange[0] /= pow(10., this->LastZPow);
    zrange[1] /= pow(10., this->LastZPow);
    }

  int xAxisDigits = this->Digits(xrange[0], xrange[1]);
  if (xAxisDigits != this->LastXAxisDigits)
    {
    char  format[16];
    sprintf(format, "%%.%df", xAxisDigits);
    this->SetXLabelFormat(format);
    this->LastXAxisDigits = xAxisDigits;
    }

  int yAxisDigits = this->Digits(yrange[0], yrange[1]);
  if (yAxisDigits != this->LastYAxisDigits)
    {
    char  format[16];
    sprintf(format, "%%.%df", yAxisDigits);
    this->SetYLabelFormat(format);
    this->LastYAxisDigits = yAxisDigits;
    }

  int zAxisDigits = this->Digits(zrange[0], zrange[1]);
  if (zAxisDigits != this->LastZAxisDigits)
    {
    char  format[16];
    sprintf(format, "%%.%df", zAxisDigits);
    this->SetZLabelFormat(format);
    this->LastZAxisDigits = zAxisDigits;
    }
}

// ****************************************************************************
//  Method: Digits
//
//  Purpose:
//      Determines the appropriate number of digits for a given range.
//
//  Arguments:
//      min    The minimum value in the range.
//      max    The maximum value in the range.
//
//  Returns:   The appropriate number of digits.
// ****************************************************************************
int vtkCubeAxesActor::Digits(double min, double max )
{
  long digitsPastDecimal;

  double range = max - min;
  double pow10 = log10(range);
  if (!vtkMath::IsFinite(pow10))
    {
    digitsPastDecimal = 0;
    }
  else
    {
    long ipow10 = static_cast<long>(floor(pow10));
    digitsPastDecimal = -ipow10;

    if (digitsPastDecimal < 0)
      {
      //
      // The range is more than 10, but not so big we need scientific
      // notation, we don't need to worry about decimals.
      //
      digitsPastDecimal = 0;
      }
    else
      {
      //
      // We want one more than the range since there is more than one
      // tick per decade.
      //
      digitsPastDecimal++;

      //
      // Anything more than 5 is just noise.  (and probably 5 is noise with
      // doubling point if the part before the decimal is big).
      //
      if (digitsPastDecimal > 5)
        {
        digitsPastDecimal = 5;
        }
      }
    }

  return (int)digitsPastDecimal;
}

// ****************************************************************************
//  Method: LabelExponent
//
//  Purpose:
//      Determines the proper exponent for the min and max values.
//
//  Arguments:
//      min     The minimum value along a certain axis.
//      max     The maximum value along a certain axis.
//
//  Note:       This code is mostly stolen from old MeshTV code,
//              /meshtvx/toolkit/plotgrid.c, axlab_format.
// ****************************************************************************

int vtkCubeAxesActor::LabelExponent(double min, double max)
{
  if (min == max)
    {
    return 0;
    }

  //
  // Determine power of 10 to scale axis labels to.
  //
  double range = (fabs(min) > fabs(max) ? fabs(min) : fabs(max));
  double pow10 = log10(range);

  //
  // Cutoffs for using scientific notation.  The following 4 variables
  // should all be static for maximum performance but were made non-static
  // to get around a compiler bug with the MIPSpro 7.2.1.3 compiler.
  //
  double eformat_cut_min = -1.5;
  double eformat_cut_max =  3.0;
  double cut_min = pow(10., eformat_cut_min);
  double cut_max = pow(10., eformat_cut_max);
  double ipow10;
  if (range < cut_min || range > cut_max)
    {
    //
    // We are going to use scientific notation and round the exponents to
    // the nearest multiple of three.
    //
    ipow10 = (floor(floor(pow10)/3.))*3;
    }
  else
    {
    ipow10 = 0;
    }

  return static_cast<int>(ipow10);
}

// *************************************************************************
//  Build the axes. Determine coordinates, position, etc.
// *************************************************************************
void vtkCubeAxesActor::BuildAxes(vtkViewport *viewport)
{
  if ((this->GetMTime() < this->BuildTime.GetMTime()) && !this->StickyAxes)
    {
    this->AutoScale(viewport);
    return;
    }

  this->SetNonDependentAttributes();

  // Reset range in case of bounds type changed
  if(this->LastUseOrientedBounds != this->UseOrientedBounds)
    {
    this->XAxisRange[0] = this->XAxisRange[1] = VTK_DOUBLE_MAX;
    this->YAxisRange[0] = this->YAxisRange[1] = VTK_DOUBLE_MAX;
    this->ZAxisRange[0] = this->ZAxisRange[1] = VTK_DOUBLE_MAX;
    this->LastUseOrientedBounds = this->UseOrientedBounds;
    }

  // determine the bounds to use (input, prop, or user-defined)
  double bounds[6];
  if(this->UseOrientedBounds != 0)
    {
    this->GetOrientedBounds(bounds);
    }
  else
    {
    if (this->StickyAxes)
      {
      this->GetViewportLimitedBounds(viewport, bounds);
      }
    else
      {
      this->GetBounds(bounds);
      }
    }

  // Setup the axes for plotting
  double xCoords[NUMBER_OF_ALIGNED_AXIS][6];
  double yCoords[NUMBER_OF_ALIGNED_AXIS][6];
  double zCoords[NUMBER_OF_ALIGNED_AXIS][6];

  // these arrays are accessed by 'location':  mm, mX, XX, or Xm.
  int mm1[4] = { 0, 0, 1, 1 };
  int mm2[4] = { 0, 1, 1, 0 };

  // Compute axes end-points
  int i;
  for (i = 0; i < NUMBER_OF_ALIGNED_AXIS; i++)
    {
    if(this->UseAxisOrigin == 0)
      {
      xCoords[i][0] = bounds[0]*this->AxisBaseForX[0] + bounds[2+mm1[i]]*this->AxisBaseForY[0] + bounds[4+mm2[i]]*this->AxisBaseForZ[0];
      xCoords[i][1] = bounds[0]*this->AxisBaseForX[1] + bounds[2+mm1[i]]*this->AxisBaseForY[1] + bounds[4+mm2[i]]*this->AxisBaseForZ[1];
      xCoords[i][2] = bounds[0]*this->AxisBaseForX[2] + bounds[2+mm1[i]]*this->AxisBaseForY[2] + bounds[4+mm2[i]]*this->AxisBaseForZ[2];
      xCoords[i][3] = bounds[1]*this->AxisBaseForX[0] + bounds[2+mm1[i]]*this->AxisBaseForY[0] + bounds[4+mm2[i]]*this->AxisBaseForZ[0];
      xCoords[i][4] = bounds[1]*this->AxisBaseForX[1] + bounds[2+mm1[i]]*this->AxisBaseForY[1] + bounds[4+mm2[i]]*this->AxisBaseForZ[1];
      xCoords[i][5] = bounds[1]*this->AxisBaseForX[2] + bounds[2+mm1[i]]*this->AxisBaseForY[2] + bounds[4+mm2[i]]*this->AxisBaseForZ[2];
      }
    else
      {
      xCoords[i][0] = bounds[0]*this->AxisBaseForX[0] + this->AxisOrigin[1]*this->AxisBaseForY[0] + this->AxisOrigin[2]*this->AxisBaseForZ[0];
      xCoords[i][1] = bounds[0]*this->AxisBaseForX[1] + this->AxisOrigin[1]*this->AxisBaseForY[1] + this->AxisOrigin[2]*this->AxisBaseForZ[1];
      xCoords[i][2] = bounds[0]*this->AxisBaseForX[2] + this->AxisOrigin[1]*this->AxisBaseForY[2] + this->AxisOrigin[2]*this->AxisBaseForZ[2];
      xCoords[i][3] = bounds[1]*this->AxisBaseForX[0] + this->AxisOrigin[1]*this->AxisBaseForY[0] + this->AxisOrigin[2]*this->AxisBaseForZ[0];
      xCoords[i][4] = bounds[1]*this->AxisBaseForX[1] + this->AxisOrigin[1]*this->AxisBaseForY[1] + this->AxisOrigin[2]*this->AxisBaseForZ[1];
      xCoords[i][5] = bounds[1]*this->AxisBaseForX[2] + this->AxisOrigin[1]*this->AxisBaseForY[2] + this->AxisOrigin[2]*this->AxisBaseForZ[2];
      }

    if(this->UseAxisOrigin == 0)
      {
      yCoords[i][0] = bounds[2]*this->AxisBaseForY[0] + bounds[0+mm1[i]]*this->AxisBaseForX[0] + bounds[4+mm2[i]]*this->AxisBaseForZ[0];
      yCoords[i][1] = bounds[2]*this->AxisBaseForY[1] + bounds[0+mm1[i]]*this->AxisBaseForX[1] + bounds[4+mm2[i]]*this->AxisBaseForZ[1];
      yCoords[i][2] = bounds[2]*this->AxisBaseForY[2] + bounds[0+mm1[i]]*this->AxisBaseForX[2] + bounds[4+mm2[i]]*this->AxisBaseForZ[2];
      yCoords[i][3] = bounds[3]*this->AxisBaseForY[0] + bounds[0+mm1[i]]*this->AxisBaseForX[0] + bounds[4+mm2[i]]*this->AxisBaseForZ[0];
      yCoords[i][4] = bounds[3]*this->AxisBaseForY[1] + bounds[0+mm1[i]]*this->AxisBaseForX[1] + bounds[4+mm2[i]]*this->AxisBaseForZ[1];
      yCoords[i][5] = bounds[3]*this->AxisBaseForY[2] + bounds[0+mm1[i]]*this->AxisBaseForX[2] + bounds[4+mm2[i]]*this->AxisBaseForZ[2];
      }
    else
      {
      yCoords[i][0] = bounds[2]*this->AxisBaseForY[0] + this->AxisOrigin[0]*this->AxisBaseForX[0] + this->AxisOrigin[2]*this->AxisBaseForZ[0];
      yCoords[i][1] = bounds[2]*this->AxisBaseForY[1] + this->AxisOrigin[0]*this->AxisBaseForX[1] + this->AxisOrigin[2]*this->AxisBaseForZ[1];
      yCoords[i][2] = bounds[2]*this->AxisBaseForY[2] + this->AxisOrigin[0]*this->AxisBaseForX[2] + this->AxisOrigin[2]*this->AxisBaseForZ[2];
      yCoords[i][3] = bounds[3]*this->AxisBaseForY[0] + this->AxisOrigin[0]*this->AxisBaseForX[0] + this->AxisOrigin[2]*this->AxisBaseForZ[0];
      yCoords[i][4] = bounds[3]*this->AxisBaseForY[1] + this->AxisOrigin[0]*this->AxisBaseForX[1] + this->AxisOrigin[2]*this->AxisBaseForZ[1];
      yCoords[i][5] = bounds[3]*this->AxisBaseForY[2] + this->AxisOrigin[0]*this->AxisBaseForX[2] + this->AxisOrigin[2]*this->AxisBaseForZ[2];
      }

    if(this->UseAxisOrigin == 0)
      {
      zCoords[i][0] = bounds[4]*this->AxisBaseForZ[0] + bounds[0+mm1[i]]*this->AxisBaseForX[0] + bounds[2+mm2[i]]*this->AxisBaseForY[0];
      zCoords[i][1] = bounds[4]*this->AxisBaseForZ[1] + bounds[0+mm1[i]]*this->AxisBaseForX[1] + bounds[2+mm2[i]]*this->AxisBaseForY[1];
      zCoords[i][2] = bounds[4]*this->AxisBaseForZ[2] + bounds[0+mm1[i]]*this->AxisBaseForX[2] + bounds[2+mm2[i]]*this->AxisBaseForY[2];
      zCoords[i][3] = bounds[5]*this->AxisBaseForZ[0] + bounds[0+mm1[i]]*this->AxisBaseForX[0] + bounds[2+mm2[i]]*this->AxisBaseForY[0];
      zCoords[i][4] = bounds[5]*this->AxisBaseForZ[1] + bounds[0+mm1[i]]*this->AxisBaseForX[1] + bounds[2+mm2[i]]*this->AxisBaseForY[1];
      zCoords[i][5] = bounds[5]*this->AxisBaseForZ[2] + bounds[0+mm1[i]]*this->AxisBaseForX[2] + bounds[2+mm2[i]]*this->AxisBaseForY[2];
      }
    else
      {
      zCoords[i][0] = bounds[4]*this->AxisBaseForZ[0] + this->AxisOrigin[0]*this->AxisBaseForX[0] + this->AxisOrigin[1]*this->AxisBaseForY[0];
      zCoords[i][1] = bounds[4]*this->AxisBaseForZ[1] + this->AxisOrigin[0]*this->AxisBaseForX[1] + this->AxisOrigin[1]*this->AxisBaseForY[1];
      zCoords[i][2] = bounds[4]*this->AxisBaseForZ[2] + this->AxisOrigin[0]*this->AxisBaseForX[2] + this->AxisOrigin[1]*this->AxisBaseForY[2];
      zCoords[i][3] = bounds[5]*this->AxisBaseForZ[0] + this->AxisOrigin[0]*this->AxisBaseForX[0] + this->AxisOrigin[1]*this->AxisBaseForY[0];
      zCoords[i][4] = bounds[5]*this->AxisBaseForZ[1] + this->AxisOrigin[0]*this->AxisBaseForX[1] + this->AxisOrigin[1]*this->AxisBaseForY[1];
      zCoords[i][5] = bounds[5]*this->AxisBaseForZ[2] + this->AxisOrigin[0]*this->AxisBaseForX[2] + this->AxisOrigin[1]*this->AxisBaseForY[2];
      }
    }

  double xRange[2], yRange[2], zRange[2];

  // this method sets the coords, offsets, and ranges if necessary.
  this->AdjustAxes(bounds, xCoords, yCoords, zCoords, xRange, yRange, zRange);

  // adjust for sci. notation if necessary
  // May set a flag for each axis specifying that label values should
  // be scaled, may change title of each axis, may change label format.
  this->AdjustValues(xRange, yRange, zRange);
  double ranges[6] = {xRange[0], xRange[1],
                      yRange[0], yRange[1],
                      zRange[0], zRange[1]};
  this->AdjustRange(ranges);

  // Prepare axes for rendering with user-definable options
  for (i = 0; i < NUMBER_OF_ALIGNED_AXIS; i++)
    {
    this->XAxes[i]->SetAxisOnOrigin(this->UseAxisOrigin);
    this->XAxes[i]->GetPoint1Coordinate()->SetValue(xCoords[i][0],
                                                    xCoords[i][1],
                                                    xCoords[i][2]);
    this->XAxes[i]->GetPoint2Coordinate()->SetValue(xCoords[i][3],
                                                    xCoords[i][4],
                                                    xCoords[i][5]);
    this->YAxes[i]->SetAxisOnOrigin(this->UseAxisOrigin);
    this->YAxes[i]->GetPoint1Coordinate()->SetValue(yCoords[i][0],
                                                    yCoords[i][1],
                                                    yCoords[i][2]);
    this->YAxes[i]->GetPoint2Coordinate()->SetValue(yCoords[i][3],
                                                    yCoords[i][4],
                                                    yCoords[i][5]);
    this->ZAxes[i]->SetAxisOnOrigin(this->UseAxisOrigin);
    this->ZAxes[i]->GetPoint1Coordinate()->SetValue(zCoords[i][0],
                                                    zCoords[i][1],
                                                    zCoords[i][2]);
    this->ZAxes[i]->GetPoint2Coordinate()->SetValue(zCoords[i][3],
                                                    zCoords[i][4],
                                                    zCoords[i][5]);

    this->XAxes[i]->SetRange(xRange[0], xRange[1]);
    this->YAxes[i]->SetRange(yRange[0], yRange[1]);
    this->ZAxes[i]->SetRange(zRange[0], zRange[1]);

    this->XAxes[i]->SetTitle(this->ActualXLabel);
    this->YAxes[i]->SetTitle(this->ActualYLabel);
    this->ZAxes[i]->SetTitle(this->ActualZLabel);
    }

  bool ticksRecomputed = this->ComputeTickSize(bounds);

  //
  // Labels are built during ComputeTickSize. if
  // ticks were not recomputed, but we need a label
  // reset, then build the labels here.
  //
  if (!ticksRecomputed)
    {
    if (this->ForceXLabelReset)
      {
      this->BuildLabels(this->XAxes);
      this->UpdateLabels(this->XAxes, 0);
      }
    if (this->ForceYLabelReset)
      {
      this->BuildLabels(this->YAxes);
      this->UpdateLabels(this->YAxes, 1);
      }
    if (this->ForceZLabelReset)
      {
      this->BuildLabels(this->ZAxes);
      this->UpdateLabels(this->ZAxes, 2);
      }
    }

  if (ticksRecomputed || this->ForceXLabelReset || this->ForceYLabelReset ||
      this->ForceZLabelReset)
    {
    // labels were re-built, need to recompute the scale.
    double center[3];

    center[0] = (bounds[1] - bounds[0]) * 0.5;
    center[1] = (bounds[3] - bounds[2]) * 0.5;
    center[2] = (bounds[5] - bounds[4]) * 0.5;

    double lenX = this->XAxes[0]->ComputeMaxLabelLength(center);
    double lenY = this->YAxes[0]->ComputeMaxLabelLength(center);
    double lenZ = this->ZAxes[0]->ComputeMaxLabelLength(center);
    double lenTitleX = this->XAxes[0]->ComputeTitleLength(center);
    double lenTitleY = this->YAxes[0]->ComputeTitleLength(center);
    double lenTitleZ = this->ZAxes[0]->ComputeTitleLength(center);
    double maxLabelLength = this->MaxOf(lenX, lenY, lenZ, 0.);
    double maxTitleLength = this->MaxOf(lenTitleX, lenTitleY, lenTitleZ, 0.);
    double bWidth  = bounds[1] - bounds[0];
    double bHeight = bounds[3] - bounds[2];

    double bLength = sqrt(bWidth*bWidth + bHeight*bHeight);

    double target = bLength *0.04;
    this->LabelScale = 1.;
    if (maxLabelLength != 0.)
      {
      this->LabelScale = target / maxLabelLength;
      }
    target = bLength *0.10;
    this->TitleScale = 1.;
    if (maxTitleLength != 0.)
      {
      this->TitleScale = target / maxTitleLength;
      }

    //
    // Allow a bit bigger title if we have units, otherwise
    // the title may be too small to read.
    //
    if (XUnits != NULL && XUnits[0] != '\0')
      {
      this->TitleScale *= 2;
      }

    for (i = 0; i < NUMBER_OF_ALIGNED_AXIS; i++)
      {
      this->XAxes[i]->SetLabelScale(this->LabelScale);
      this->YAxes[i]->SetLabelScale(this->LabelScale);
      this->ZAxes[i]->SetLabelScale(this->LabelScale);
      this->XAxes[i]->SetTitleScale(this->TitleScale);
      this->YAxes[i]->SetTitleScale(this->TitleScale);
      this->ZAxes[i]->SetTitleScale(this->TitleScale);

      // Need to build the axis again prior to calling AutoScale so
      // that labels are positioned accordingly.
      this->XAxes[i]->BuildAxis(viewport, true);
      this->YAxes[i]->BuildAxis(viewport, true);
      this->ZAxes[i]->BuildAxis(viewport, true);
      }
    }

  // Scale appropriately.
  this->AutoScale(viewport);

  this->RenderSomething = 1;
  this->BuildTime.Modified();
  this->LastFlyMode = this->FlyMode;
}

// *************************************************************************
//  Sends attributes to each vtkAxisActor.  Only sets those that are
//  not dependent upon viewport changes, and thus do not need to be set
//  very often.
// *************************************************************************
void vtkCubeAxesActor::SetNonDependentAttributes()
{
  vtkProperty *prop = this->GetProperty();
  prop->SetAmbient(1.0);
  prop->SetDiffuse(0.0);

  // Make sure our Axis Base is normalized
  vtkMath::Normalize(this->AxisBaseForX);
  vtkMath::Normalize(this->AxisBaseForY);
  vtkMath::Normalize(this->AxisBaseForZ);

  // Manage custome grid visibility location if FLY and STATIC axis
  int gridLocationBasedOnAxis = (this->GridLineLocation == VTK_GRID_LINES_ALL)
      ? VTK_GRID_LINES_ALL : VTK_GRID_LINES_CLOSEST;

  for (int i = 0; i < NUMBER_OF_ALIGNED_AXIS; i++)
    {
    this->XAxes[i]->SetAxisPosition(i);
    this->XAxes[i]->SetAxisBaseForX(this->AxisBaseForX);
    this->XAxes[i]->SetAxisBaseForY(this->AxisBaseForY);
    this->XAxes[i]->SetAxisBaseForZ(this->AxisBaseForZ);
    this->XAxes[i]->SetCamera(this->Camera);
    this->XAxes[i]->SetProperty(prop);
    this->XAxes[i]->SetTitleTextProperty(this->TitleTextProperty[0]);
    this->XAxes[i]->SetLabelTextProperty(this->LabelTextProperty[0]);
    this->XAxes[i]->SetAxisLinesProperty(this->XAxesLinesProperty);
    this->XAxes[i]->SetGridlinesProperty(this->XAxesGridlinesProperty);
    this->XAxes[i]->SetGridpolysProperty(this->XAxesGridpolysProperty);
    this->XAxes[i]->SetTickLocation(this->TickLocation);
    this->XAxes[i]->SetDrawGridlines(this->DrawXGridlines);
    this->XAxes[i]->SetDrawGridlinesLocation(gridLocationBasedOnAxis);
    this->XAxes[i]->SetDrawInnerGridlines(this->DrawXInnerGridlines);
    this->XAxes[i]->SetDrawGridpolys(this->DrawXGridpolys);
    this->XAxes[i]->SetBounds(this->Bounds);
    this->XAxes[i]->SetAxisVisibility(this->XAxisVisibility);
    this->XAxes[i]->SetLabelVisibility(this->XAxisLabelVisibility);
    this->XAxes[i]->SetTitleVisibility(this->XAxisLabelVisibility);
    this->XAxes[i]->SetTickVisibility(this->XAxisTickVisibility);
    this->XAxes[i]->SetMinorTicksVisible(this->XAxisMinorTickVisibility);

    this->YAxes[i]->SetAxisPosition(i);
    this->YAxes[i]->SetAxisBaseForX(this->AxisBaseForX);
    this->YAxes[i]->SetAxisBaseForY(this->AxisBaseForY);
    this->YAxes[i]->SetAxisBaseForZ(this->AxisBaseForZ);
    this->YAxes[i]->SetCamera(this->Camera);
    this->YAxes[i]->SetProperty(prop);
    this->YAxes[i]->SetTitleTextProperty(this->TitleTextProperty[1]);
    this->YAxes[i]->SetLabelTextProperty(this->LabelTextProperty[1]);
    this->YAxes[i]->SetAxisLinesProperty(this->YAxesLinesProperty);
    this->YAxes[i]->SetGridlinesProperty(this->YAxesGridlinesProperty);
    this->YAxes[i]->SetGridpolysProperty(this->YAxesGridpolysProperty);
    this->YAxes[i]->SetTickLocation(this->TickLocation);
    this->YAxes[i]->SetDrawGridlines(this->DrawYGridlines);
    this->YAxes[i]->SetDrawGridlinesLocation(gridLocationBasedOnAxis);
    this->YAxes[i]->SetDrawInnerGridlines(this->DrawYInnerGridlines);
    this->YAxes[i]->SetDrawGridpolys(this->DrawYGridpolys);
    this->YAxes[i]->SetBounds(this->Bounds);
    this->YAxes[i]->SetAxisVisibility(this->YAxisVisibility);
    this->YAxes[i]->SetLabelVisibility(this->YAxisLabelVisibility);
    this->YAxes[i]->SetTitleVisibility(this->YAxisLabelVisibility);
    this->YAxes[i]->SetTickVisibility(this->YAxisTickVisibility);
    this->YAxes[i]->SetMinorTicksVisible(this->YAxisMinorTickVisibility);

    this->ZAxes[i]->SetAxisPosition(i);
    this->ZAxes[i]->SetAxisBaseForX(this->AxisBaseForX);
    this->ZAxes[i]->SetAxisBaseForY(this->AxisBaseForY);
    this->ZAxes[i]->SetAxisBaseForZ(this->AxisBaseForZ);
    this->ZAxes[i]->SetCamera(this->Camera);
    this->ZAxes[i]->SetProperty(prop);
    this->ZAxes[i]->SetTitleTextProperty(this->TitleTextProperty[2]);
    this->ZAxes[i]->SetLabelTextProperty(this->LabelTextProperty[2]);
    this->ZAxes[i]->SetAxisLinesProperty(this->ZAxesLinesProperty);
    this->ZAxes[i]->SetGridlinesProperty(this->ZAxesGridlinesProperty);
    this->ZAxes[i]->SetGridpolysProperty(this->ZAxesGridpolysProperty);
    this->ZAxes[i]->SetTickLocation(this->TickLocation);
    this->ZAxes[i]->SetDrawGridlines(this->DrawZGridlines);
    this->ZAxes[i]->SetDrawGridlinesLocation(gridLocationBasedOnAxis);
    this->ZAxes[i]->SetDrawInnerGridlines(this->DrawZInnerGridlines);
    this->ZAxes[i]->SetDrawGridpolys(this->DrawZGridpolys);
    this->ZAxes[i]->SetBounds(this->Bounds);
    this->ZAxes[i]->SetAxisVisibility(this->ZAxisVisibility);
    this->ZAxes[i]->SetLabelVisibility(this->ZAxisLabelVisibility);
    this->ZAxes[i]->SetTitleVisibility(this->ZAxisLabelVisibility);
    this->ZAxes[i]->SetTickVisibility(this->ZAxisTickVisibility);
    this->ZAxes[i]->SetMinorTicksVisible(this->ZAxisMinorTickVisibility);
    }
}

// Static variable describes locations in cube, relative to the type
// of axis:  mm for an X-axis means the x-edge at min-y and min-z.
// mX for a Y-axis means the y-edge at min-x and max-z, and so on.

enum {mm = 0, mX, XX, Xm };
//
// For CLOSEST_TRIAD, and FURTHEST_TRIAD, this variable determines
// which locations in the cube each 'Major' axis should take.
//
static int vtkCubeAxesActorTriads[8][3] = {
  {mm,mm,mm}, {mm,Xm,Xm}, {Xm,mm,mX}, {Xm,Xm,XX},
  {mX,mX,mm}, {mX,XX,Xm}, {XX,mX,mX}, {XX,XX,XX}};
static int vtkCubeAxesActorConn[8][3] = {{1,2,4}, {0,3,5}, {3,0,6}, {2,1,7},
                                         {5,6,0}, {4,7,1}, {7,4,2}, {6,5,3}};

// *************************************************************************
// Determine which of the axes in each coordinate direction actually should
// be rendered.  For STATIC FlyMode, all axes are rendered.  For other
// FlyModes, either 1 or 2 per coordinate direction are rendered.
// *************************************************************************
void vtkCubeAxesActor::DetermineRenderAxes(vtkViewport *viewport)
{
  double bounds[6];
  double pts[8][3];
  int i = 0, closestIdx = -1, furtherstIdx = -1;
  int xloc = 0, yloc = 0, zloc = 0;

  // Make sure we start with only one axis by default, then we might extend it
  this->NumberOfAxesX = this->NumberOfAxesY = this->NumberOfAxesZ = 1;

  // Compute relevant axis points only if a axis/grid visibility change based
  // on the viewpoint
  if( !( this->GridLineLocation == VTK_GRID_LINES_ALL
         && ( this->FlyMode == VTK_FLY_STATIC_EDGES
              || this->FlyMode == VTK_FLY_STATIC_TRIAD)))
    {
    // determine the bounds to use (input, prop, or user-defined)
    this->GetBounds(bounds);
    this->TransformBounds(viewport, bounds, pts);
    }

  // Check closest point if needed
  if( this->GridLineLocation == VTK_GRID_LINES_CLOSEST
      || this->FlyMode == VTK_FLY_CLOSEST_TRIAD )
    {
    closestIdx = this->FindClosestAxisIndex(pts);
    }

  // Check furtherst point if needed
  if( this->GridLineLocation == VTK_GRID_LINES_FURTHEST
      || this->FlyMode == VTK_FLY_FURTHEST_TRIAD )
    {
    furtherstIdx = this->FindFurtherstAxisIndex(pts);
    }

  // Manage fast static axis visibility
  if (this->FlyMode == VTK_FLY_STATIC_EDGES || this->FlyMode == VTK_FLY_STATIC_TRIAD)
    {
    if(this->FlyMode == VTK_FLY_STATIC_EDGES)
      {
      this->NumberOfAxesX = this->NumberOfAxesY = this->NumberOfAxesZ
          = NUMBER_OF_ALIGNED_AXIS;
      }

    for (i = 0; i < this->NumberOfAxesX; i++)
      {
      this->RenderAxesX[i] = this->RenderAxesY[i] = this->RenderAxesZ[i] = i;
      }

    this->UpdateGridLineVisibility(
          (this->GridLineLocation == VTK_GRID_LINES_CLOSEST)
          ? closestIdx : furtherstIdx);
    return;
    }


  // Take into account the inertia. Process only so often.
  if (this->RenderCount++ == 0 || !(this->RenderCount % this->Inertia))
    {
    if (this->FlyMode == VTK_FLY_CLOSEST_TRIAD)
      {
      xloc = vtkCubeAxesActorTriads[closestIdx][0];
      yloc = vtkCubeAxesActorTriads[closestIdx][1];
      zloc = vtkCubeAxesActorTriads[closestIdx][2];
      }
    else if (this->FlyMode == VTK_FLY_FURTHEST_TRIAD)
      {
      xloc = vtkCubeAxesActorTriads[furtherstIdx][0];
      yloc = vtkCubeAxesActorTriads[furtherstIdx][1];
      zloc = vtkCubeAxesActorTriads[furtherstIdx][2];
      }
    else // else boundary edges fly mode
      {
      this->FindBoundaryEdge(xloc, yloc, zloc, pts);
      }

    this->InertiaLocs[0] = xloc;
    this->InertiaLocs[1] = yloc;
    this->InertiaLocs[2] = zloc;
    } // inertia
  else
    {
    // Do not change anything, use locations from last render
    xloc = this->InertiaLocs[0];
    yloc = this->InertiaLocs[1];
    zloc = this->InertiaLocs[2];
    }

  // Set axes to be rendered
  this->RenderAxesX[0] = xloc % NUMBER_OF_ALIGNED_AXIS;
  this->RenderAxesY[0] = yloc % NUMBER_OF_ALIGNED_AXIS;
  this->RenderAxesZ[0] = zloc % NUMBER_OF_ALIGNED_AXIS;

  // Manage grid visibility (can increase the number of axis to render)
  this->UpdateGridLineVisibility(
        (this->GridLineLocation == VTK_GRID_LINES_CLOSEST)
        ? closestIdx : furtherstIdx);
}

// --------------------------------------------------------------------------
double vtkCubeAxesActor::MaxOf(double a, double b)
{
  return (a > b ? a : b);
}

// --------------------------------------------------------------------------
double vtkCubeAxesActor::MaxOf(double a, double b, double c, double d)
{
  return this->MaxOf(this->MaxOf(a, b), this->MaxOf(c, d));
}

// --------------------------------------------------------------------------
inline double vtkCubeAxesActor::FFix(double value)
{
  int ivalue = static_cast<int>(value);
  return ivalue;
}

inline int vtkCubeAxesActor::FRound(double value)
{
  return value <= 0.5 ? static_cast<int>(this->FFix(value)) : static_cast<int>(this->FFix(value) + 1);
}

inline int vtkCubeAxesActor::GetNumTicks(double range, double fxt)
{
  // Find the number of integral points in the interval.
  double fnt  = range/fxt;
  fnt  = this->FFix(fnt);
  return this->FRound(fnt);
}

// --------------------------------------------------------------------------
inline double vtkCubeAxesActor::FSign(double value, double sign)
{
  value = fabs(value);
  if (sign < 0.)
    {
    value *= -1.;
    }
  return value;
}

// *******************************************************************
// Method: vtkCubeAxesActor::AdjustTicksComputeRange
//
// Purpose: Sets private members controlling the number and position
//          of ticks.
//
// Arguments:
//   inRange   The range for this axis.
// *******************************************************************

void vtkCubeAxesActor::AdjustTicksComputeRange(vtkAxisActor *axes[NUMBER_OF_ALIGNED_AXIS],
    double boundsMin, double boundsMax)
{
  double sortedRange[2], range;
  double fxt;
  double div, major, minor;
  double majorStart, minorStart;
  int numTicks;
  double *inRange = axes[0]->GetRange();
  vtkStringArray* customizedLabels = NULL;

  sortedRange[0] = inRange[0] < inRange[1] ? inRange[0] : inRange[1];
  sortedRange[1] = inRange[0] > inRange[1] ? inRange[0] : inRange[1];

  range = sortedRange[1] - sortedRange[0];

  // Find the integral points.
  double pow10 = log10(range);

  // Build in numerical tolerance
  if (pow10 != 0.)
    {
    double eps = 10.0e-10;
    pow10 = this->FSign((fabs(pow10) + eps), pow10);
    }

  // FFix move you in the wrong direction if pow10 is negative.
  if (pow10 < 0.)
    {
    pow10 = pow10 - 1.;
    }

  fxt = pow(10., this->FFix(pow10));

  numTicks = this->GetNumTicks(range, fxt);

  div = 1.;
  if (numTicks < 5)
    {
    div = 2.;
    }
  if (numTicks <= 2)
    {
    div = 5.;
    }

  // If there aren't enough major tick points in this decade, use the next
  // decade.
  major = fxt;
  if (div != 1.)
    {
    major /= div;
    }

  int axis = 0;
  switch(axes[0]->GetAxisType())
  {
  case VTK_AXIS_TYPE_X:
    axis = 0;
    break;
  case VTK_AXIS_TYPE_Y:
    axis = 1;
    break;
  case VTK_AXIS_TYPE_Z:
    axis = 2;
    break;
  }
  customizedLabels = this->AxisLabels[axis];

  if (customizedLabels == NULL)
    {
    // Figure out the first major tick locations, relative to the
    // start of the axis.
    if (sortedRange[0] <= 0.)
      {
      majorStart = major*(this->FFix(sortedRange[0]*(1./major)) + 0.);
      }
    else
      {
      majorStart = major*(this->FFix(sortedRange[0]*(1./major)) + 1.);
      }
    }
  else
    {
      // If we have custom labels, they are supposed to be uniformly distributed
      // inside the values range.
      majorStart = sortedRange[0];
      numTicks = this->GetNumTicks(range, major);
      int labelsCount = customizedLabels->GetNumberOfValues();
      if (numTicks > labelsCount)
        {
        major = range / (labelsCount - 1.);
        }
    }

  minor = major / 10.;
  // Figure out the first minor tick locations, relative to the
  // start of the axis.
  if (sortedRange[0] <= 0.)
    {
    minorStart = minor*(this->FFix(sortedRange[0]*(1./minor)) + 0.);
    }
  else
    {
    minorStart = minor*(this->FFix(sortedRange[0]*(1./minor)) + 1.);
    }

  for (int i = 0; i < NUMBER_OF_ALIGNED_AXIS; i++)
    {
    axes[i]->SetMinorRangeStart(minorStart);
    axes[i]->SetMajorRangeStart(majorStart);
    axes[i]->SetDeltaRangeMinor(minor);
    axes[i]->SetDeltaRangeMajor(major);
    }

  double t;
  t = (minorStart - sortedRange[0])/range;
  minorStart = t * boundsMax + (1-t) * boundsMin;
  t = (majorStart - sortedRange[0])/range;
  majorStart = t * boundsMax + (1-t) * boundsMin;
  const double scale = (boundsMax - boundsMin) / range;
  minor *= scale;
  major *= scale;

  // Set major start and delta for the corresponding cube axis
  switch(axes[0]->GetAxisType())
    {
    case VTK_AXIS_TYPE_X:
      this->MajorStart[0] = majorStart;
      this->DeltaMajor[0] = major;
      break;
    case VTK_AXIS_TYPE_Y:
      this->MajorStart[1] = majorStart;
      this->DeltaMajor[1] = major;
      break;
    case VTK_AXIS_TYPE_Z:
      this->MajorStart[2] = majorStart;
      this->DeltaMajor[2] = major;
      break;
    }

  // Set major and minor starts and deltas for all underlying axes
  for (int i = 0; i < NUMBER_OF_ALIGNED_AXIS; i++)
    {
    axes[i]->SetMinorStart(minorStart);
    axes[i]->SetMajorStart(axes[0]->GetAxisType(), majorStart);

    axes[i]->SetDeltaMinor(minor);
    axes[i]->SetDeltaMajor(axes[0]->GetAxisType(), major);
    }
}

// ****************************************************************
void vtkCubeAxesActor::AutoScale(vtkViewport *viewport)
{
  // Current implementation only for perspective projections.
  this->AutoScale(viewport, this->XAxes);
  this->AutoScale(viewport, this->YAxes);
  this->AutoScale(viewport, this->ZAxes);
}

// ****************************************************************
void vtkCubeAxesActor::AutoScale(vtkViewport *viewport, vtkAxisActor *axis[NUMBER_OF_ALIGNED_AXIS])
{
  for (int i = 0; i < NUMBER_OF_ALIGNED_AXIS; ++i)
    {
    double newTitleScale
      = this->AutoScale(viewport,
                        this->ScreenSize,
                        axis[i]->GetTitleActor()->GetPosition());

    axis[i]->SetTitleScale(newTitleScale);

    // Now labels.
    vtkAxisFollower** labelActors = axis[i]->GetLabelActors();

    for(int j = 0; j < axis[i]->GetNumberOfLabelsBuilt(); ++j)
      {
      double newLabelScale
        = this->AutoScale(viewport,
                          this->ScreenSize,
                          labelActors[j]->GetPosition());

      axis[i]->SetLabelScale(j, newLabelScale);
      }
    }
}

// ****************************************************************
double vtkCubeAxesActor::AutoScale(vtkViewport *viewport, double screenSize,
                                   double position[3])
{
  double factor = 1;
  if (viewport->GetSize()[1] > 0)
    {
    factor = 2.0 * screenSize
      * tan(vtkMath::RadiansFromDegrees(this->Camera->GetViewAngle()/2.0))
      / viewport->GetSize()[1];
    }

    double dist = sqrt(
          vtkMath::Distance2BetweenPoints(position,
                                          this->Camera->GetPosition()));
    double newScale = factor * dist;

    return newScale;
}

// ****************************************************************
//  Determine what the labels should be and set them in each axis.
// ****************************************************************
void vtkCubeAxesActor::BuildLabels(vtkAxisActor *axes[NUMBER_OF_ALIGNED_AXIS])
{
  char label[64];
  int labelCount = 0;
  double deltaMajor = axes[0]->GetDeltaMajor(axes[0]->GetAxisType());
  double val = axes[0]->GetMajorRangeStart();
  double p2[3], p1[3];
  axes[0]->GetPoint1Coordinate()->GetValue(p1);
  axes[0]->GetPoint2Coordinate()->GetValue(p2);
  double *range = axes[0]->GetRange();
  double axis[3] = { p2[0]-p1[0], p2[1]-p1[1], p2[2]-p1[2] };
  double axisLength = vtkMath::Norm(axis);
  double extents = range[1] - range[0];
  double rangeScale = axisLength / extents;
  double labelCountAsDouble = (axisLength - (val-range[0])*rangeScale) / deltaMajor;
  bool mustAdjustValue = 0;
  int lastPow = 0;
  int axisIndex = 0;
  vtkStringArray* customizedLabels = NULL;

  vtkStringArray *labels = vtkStringArray::New();
  const char *format = "%s";
  switch (axes[0]->GetAxisType())
    {
    case VTK_AXIS_TYPE_X:
      axisIndex = 0;
      format = this->XLabelFormat;
      mustAdjustValue = this->MustAdjustXValue;
      lastPow = this->LastXPow;
      break;
    case VTK_AXIS_TYPE_Y:
      axisIndex = 1;
      format = this->YLabelFormat;
      mustAdjustValue = this->MustAdjustYValue;
      lastPow = this->LastYPow;
      break;
    case VTK_AXIS_TYPE_Z:
      axisIndex = 2;
      format = this->ZLabelFormat;
      mustAdjustValue = this->MustAdjustZValue;
      lastPow = this->LastZPow;
      break;
    }
  customizedLabels = this->AxisLabels[axisIndex];
  // figure out how many labels we need:
  if(extents == 0 || vtkMath::IsNan(labelCountAsDouble))
    {
    labelCount = 0;
    }
  else
    {
    labelCount = vtkMath::Floor(labelCountAsDouble+2*FLT_EPSILON) + 1;
    }

  labels->SetNumberOfValues(labelCount);

  if (customizedLabels == NULL)
    {
    // Convert deltaMajor from world coord to range scale
    deltaMajor = extents * deltaMajor/axisLength;

    double scaleFactor = 1.;
    if (lastPow != 0)
      {
      scaleFactor = 1.0/pow(10., lastPow);
      }

    for (int i = 0; i < labelCount; i++)
      {
      if (fabs(val) < 0.01 && extents > 1)
        {
        // We just happened to fall at something near zero and the range is
        // large, so set it to zero to avoid ugliness.
        val = 0.;
        }
      if (mustAdjustValue)
        {
        sprintf(label, format, val*scaleFactor);
        }
      else
        {
        sprintf(label, format, val);
        }
      if (fabs(val) < 0.01)
        {
        //
        // Ensure that -0.0 is never a label
        // The maximum number of digits that we allow past the decimal is 5.
        //
        if (strcmp(label, "-0") == 0)
          {
          sprintf(label, "0");
          }
        else if (strcmp(label, "-0.0") == 0)
          {
          sprintf(label, "0.0");
          }
        else if (strcmp(label, "-0.00") == 0)
          {
          sprintf(label, "0.00");
          }
        else if (strcmp(label, "-0.000") == 0)
          {
          sprintf(label, "0.000");
          }
        else if (strcmp(label, "-0.0000") == 0)
          {
          sprintf(label, "0.0000");
          }
        else if (strcmp(label, "-0.00000") == 0)
          {
          sprintf(label, "0.00000");
          }
        }
      labels->SetValue(i, label);
      val += deltaMajor;
      }
    }
  else
    {
    if (labelCount > 0)
      {
      double delta = customizedLabels->GetNumberOfValues() / labelCount;
      for (int i = 0; i < labelCount; ++i)
        {
        labels->SetValue(i, customizedLabels->GetValue(static_cast<vtkIdType>(i * delta)));
        }
      }
    }
  for (int i = 0; i < NUMBER_OF_ALIGNED_AXIS; i++)
    {
    axes[i]->SetLabels(labels);
    }
  labels->Delete();
}

vtkStringArray* vtkCubeAxesActor::GetAxisLabels(int axis)
{
  return (axis >= 0 && axis < 3) ? this->AxisLabels[axis] : NULL;
}

void vtkCubeAxesActor::SetAxisLabels(int axis, vtkStringArray* value)
{
  if (axis >= 0 && axis < 3 && value != this->AxisLabels[axis])
    {
    vtkStringArray* previous = this->AxisLabels[axis];
    if (value != NULL)
      {
      value->Register(this);
      }
    this->AxisLabels[axis] = value;
    if (previous != NULL)
      {
      previous->UnRegister(this);
      }
    this->Modified();
    }
}

// ****************************************************************************
//  Set automatic label scaling mode, set exponents for each axis type.
// ****************************************************************************
void vtkCubeAxesActor::SetLabelScaling(bool autoscale, int upowX, int upowY,
                                       int upowZ)
{
  if (autoscale != this->AutoLabelScaling || upowX != this->UserXPow ||
      upowY != this->UserYPow || upowZ != this->UserZPow)
    {
    this->AutoLabelScaling = autoscale;
    this->UserXPow = upowX;
    this->UserYPow = upowY;
    this->UserZPow = upowZ;
    this->Modified();
    }
}

// ****************************************************************************
// Set the i-th title text property.
// ****************************************************************************

vtkTextProperty* vtkCubeAxesActor::GetTitleTextProperty(int axis)
{
  return (axis >= 0 && axis < 3) ? this->TitleTextProperty[axis] : NULL;
}

// ****************************************************************************
// Get the i-th label text property.
// ****************************************************************************

vtkTextProperty* vtkCubeAxesActor::GetLabelTextProperty(int axis)
{
  return (axis >= 0 && axis < 3) ? this->LabelTextProperty[axis] : NULL;
}

// ****************************************************************************
//  Set axes and screen size of the labels.
// ****************************************************************************
void vtkCubeAxesActor::UpdateLabels(vtkAxisActor **axis, int vtkNotUsed(index))
  {
  for (int i = 0; i < NUMBER_OF_ALIGNED_AXIS; i++)
    {
    int numberOfLabelsBuild = axis[i]->GetNumberOfLabelsBuilt();
    vtkAxisFollower **labelActors = axis[i]->GetLabelActors();
    vtkProp3DAxisFollower **labelProps = axis[i]->GetLabelProps3D();
    for(int k=0; k < numberOfLabelsBuild; ++k)
      {
      labelActors[k]->SetEnableDistanceLOD( this->EnableDistanceLOD );
      labelActors[k]->SetDistanceLODThreshold( this->DistanceLODThreshold );
      labelActors[k]->SetEnableViewAngleLOD( this->EnableViewAngleLOD );
      labelActors[k]->SetViewAngleLODThreshold( this->ViewAngleLODThreshold );
      labelProps[k]->SetEnableDistanceLOD( this->EnableDistanceLOD );
      labelProps[k]->SetDistanceLODThreshold( this->DistanceLODThreshold );
      labelProps[k]->SetEnableViewAngleLOD( this->EnableViewAngleLOD );
      labelProps[k]->SetViewAngleLODThreshold( this->ViewAngleLODThreshold );
      }
    }
  }
// ****************************************************************************
void vtkCubeAxesActor::SetXAxesLinesProperty(vtkProperty *prop)
{
  this->XAxesLinesProperty->DeepCopy(prop);
  this->Modified();
}
void vtkCubeAxesActor::SetYAxesLinesProperty(vtkProperty *prop)
{
  this->YAxesLinesProperty->DeepCopy(prop);
  this->Modified();
}
void vtkCubeAxesActor::SetZAxesLinesProperty(vtkProperty *prop)
{
  this->ZAxesLinesProperty->DeepCopy(prop);
  this->Modified();
}

// ****************************************************************************
vtkProperty* vtkCubeAxesActor::GetXAxesLinesProperty()
{
  return this->XAxesLinesProperty;
}
vtkProperty* vtkCubeAxesActor::GetYAxesLinesProperty()
{
  return this->YAxesLinesProperty;
}
vtkProperty* vtkCubeAxesActor::GetZAxesLinesProperty()
{
  return this->ZAxesLinesProperty;
}

// ****************************************************************************
void vtkCubeAxesActor::SetXAxesGridlinesProperty(vtkProperty *prop)
{
  this->XAxesGridlinesProperty->DeepCopy(prop);
  this->Modified();
}
void vtkCubeAxesActor::SetYAxesGridlinesProperty(vtkProperty *prop)
{
  this->YAxesGridlinesProperty->DeepCopy(prop);
  this->Modified();
}
void vtkCubeAxesActor::SetZAxesGridlinesProperty(vtkProperty *prop)
{
  this->ZAxesGridlinesProperty->DeepCopy(prop);
  this->Modified();
}

// ****************************************************************************
vtkProperty* vtkCubeAxesActor::GetXAxesGridlinesProperty()
{
  return this->XAxesGridlinesProperty;
}
vtkProperty* vtkCubeAxesActor::GetYAxesGridlinesProperty()
{
  return this->YAxesGridlinesProperty;
}
vtkProperty* vtkCubeAxesActor::GetZAxesGridlinesProperty()
{
  return this->ZAxesGridlinesProperty;
}

// ****************************************************************************
void vtkCubeAxesActor::SetXAxesInnerGridlinesProperty(vtkProperty *prop)
{
  this->XAxesInnerGridlinesProperty->DeepCopy(prop);
  this->Modified();
}
void vtkCubeAxesActor::SetYAxesInnerGridlinesProperty(vtkProperty *prop)
{
  this->YAxesInnerGridlinesProperty->DeepCopy(prop);
  this->Modified();
}
void vtkCubeAxesActor::SetZAxesInnerGridlinesProperty(vtkProperty *prop)
{
  this->ZAxesInnerGridlinesProperty->DeepCopy(prop);
  this->Modified();
}

// ****************************************************************************
vtkProperty* vtkCubeAxesActor::GetXAxesInnerGridlinesProperty()
{
  return this->XAxesInnerGridlinesProperty;
}
vtkProperty* vtkCubeAxesActor::GetYAxesInnerGridlinesProperty()
{
  return this->YAxesInnerGridlinesProperty;
}
vtkProperty* vtkCubeAxesActor::GetZAxesInnerGridlinesProperty()
{
  return this->ZAxesInnerGridlinesProperty;
}

// ****************************************************************************
void vtkCubeAxesActor::SetXAxesGridpolysProperty(vtkProperty *prop)
{
  this->XAxesGridpolysProperty->DeepCopy(prop);
  this->Modified();
}
void vtkCubeAxesActor::SetYAxesGridpolysProperty(vtkProperty *prop)
{
  this->YAxesGridpolysProperty->DeepCopy(prop);
  this->Modified();
}
void vtkCubeAxesActor::SetZAxesGridpolysProperty(vtkProperty *prop)
{
  this->ZAxesGridpolysProperty->DeepCopy(prop);
  this->Modified();
}

// ****************************************************************************
vtkProperty* vtkCubeAxesActor::GetXAxesGridpolysProperty()
{
  return this->XAxesGridpolysProperty;
}
vtkProperty* vtkCubeAxesActor::GetYAxesGridpolysProperty()
{
  return this->YAxesGridpolysProperty;
}
vtkProperty* vtkCubeAxesActor::GetZAxesGridpolysProperty()
{
  return this->ZAxesGridpolysProperty;
}
// --------------------------------------------------------------------------
void vtkCubeAxesActor::UpdateGridLineVisibility(int idx)
{
  if( this->GridLineLocation != VTK_GRID_LINES_ALL &&
      (this->DrawXGridlines || this->DrawYGridlines || this->DrawZGridlines) )
    {
    for(int i=0; i < NUMBER_OF_ALIGNED_AXIS; ++i)
      {
      this->XAxes[i]->SetDrawGridlines(0);
      this->YAxes[i]->SetDrawGridlines(0);
      this->ZAxes[i]->SetDrawGridlines(0);
      this->XAxes[i]->SetDrawGridlinesOnly(0);
      this->YAxes[i]->SetDrawGridlinesOnly(0);
      this->ZAxes[i]->SetDrawGridlinesOnly(0);
      }

    this->XAxes[vtkCubeAxesActorTriads[idx][0]]->SetDrawGridlines(this->DrawXGridlines);
    this->YAxes[vtkCubeAxesActorTriads[idx][1]]->SetDrawGridlines(this->DrawYGridlines);
    this->ZAxes[vtkCubeAxesActorTriads[idx][2]]->SetDrawGridlines(this->DrawZGridlines);

    // Update axis render list
    int id = 0;
    if(this->NumberOfAxesX == 1)
      {
      id = this->RenderAxesX[this->NumberOfAxesX] = vtkCubeAxesActorTriads[idx][0];
      this->XAxes[id]->SetDrawGridlinesOnly((this->RenderAxesX[0] != id) ? 1 : 0);
      this->NumberOfAxesX += (this->RenderAxesX[0] != id) ? 1 : 0;
      }
    if(this->NumberOfAxesY == 1)
      {
      id = this->RenderAxesY[this->NumberOfAxesY] = vtkCubeAxesActorTriads[idx][1];
      this->YAxes[id]->SetDrawGridlinesOnly((this->RenderAxesY[0] != id) ? 1 : 0);
      this->NumberOfAxesY += (this->RenderAxesY[0] != id) ? 1 : 0;
      }
    if(this->NumberOfAxesZ == 1)
      {
      id = this->RenderAxesZ[this->NumberOfAxesZ] = vtkCubeAxesActorTriads[idx][2];
      this->ZAxes[id]->SetDrawGridlinesOnly((this->RenderAxesZ[0] != id) ? 1 : 0);
      this->NumberOfAxesZ += (this->RenderAxesZ[0] != id) ? 1 : 0;
      }
    }
}
// --------------------------------------------------------------------------
int vtkCubeAxesActor::FindClosestAxisIndex(double pts[8][3])
{
  // Loop over points and find the closest point to the camera
  double min = VTK_FLOAT_MAX;
  int idx = 0;
  for (int i=0; i < 8; i++)
    {
    if (pts[i][2] < min)
      {
      idx = i;
      min = pts[i][2];
      }
    }
  return idx;
}

// --------------------------------------------------------------------------
int vtkCubeAxesActor::FindFurtherstAxisIndex(double pts[8][3])
{
  // Loop over points and find the furthest point from the camera
  double max = -VTK_FLOAT_MAX;
  int idx = 0;
  for (int i=0; i < 8; i++)
    {
    if (pts[i][2] > max)
      {
      idx = i;
      max = pts[i][2];
      }
    }
  return idx;
}
// --------------------------------------------------------------------------
 void vtkCubeAxesActor::FindBoundaryEdge( int &xloc, int &yloc,
                                          int &zloc, double pts[8][3])
 {
   // boundary edges fly mode
   xloc = yloc = zloc = 1;
   int i, xIdx = 0, yIdx = 0, zIdx = 0, zIdx2 = 0;
   int xAxes = 0, yAxes = 0, zAxes = 0;
   double slope = 0.0, minSlope, num, den, d2;
   double e1[3], e2[3], e3[3];
   int idx = 0;

   // Find distance to origin
   double d2Min = VTK_FLOAT_MAX;
   for (i=0; i < 8; i++)
     {
     d2 = pts[i][0]*pts[i][0] + pts[i][1]*pts[i][1];
     if (d2 < d2Min)
       {
       d2Min = d2;
       idx = i;
       }
     }

   // find minimum slope point connected to closest point and on
   // right side (in projected coordinates). This is the first edge.
   minSlope = VTK_FLOAT_MAX;
   for (xIdx=0, i=0; i<3; i++)
     {
     num = (pts[vtkCubeAxesActorConn[idx][i]][1] - pts[idx][1]);
     den = (pts[vtkCubeAxesActorConn[idx][i]][0] - pts[idx][0]);
     if (den != 0.0)
       {
       slope = num / den;
       }
     if (slope < minSlope && den > 0)
       {
       xIdx = vtkCubeAxesActorConn[idx][i];
       yIdx = vtkCubeAxesActorConn[idx][(i+1)%3];
       zIdx = vtkCubeAxesActorConn[idx][(i+2)%3];
       xAxes = i;
       minSlope = slope;
       }
     }

   // find edge (connected to closest point) on opposite side
   for ( i=0; i<3; i++)
     {
     e1[i] = (pts[xIdx][i] - pts[idx][i]);
     e2[i] = (pts[yIdx][i] - pts[idx][i]);
     e3[i] = (pts[zIdx][i] - pts[idx][i]);
     }
   vtkMath::Normalize(e1);
   vtkMath::Normalize(e2);
   vtkMath::Normalize(e3);

   if (vtkMath::Dot(e1,e2) < vtkMath::Dot(e1,e3))
     {
     yAxes = (xAxes + 1) % 3;
     }
   else
     {
     yIdx = zIdx;
     yAxes = (xAxes + 2) % 3;
     }

   // Find the final point by determining which global x-y-z axes have not
   // been represented, and then determine the point closest to the viewer.
   zAxes = (xAxes != 0 && yAxes != 0 ? 0 :
           (xAxes != 1 && yAxes != 1 ? 1 : 2));
   if (pts[vtkCubeAxesActorConn[xIdx][zAxes]][2] <
       pts[vtkCubeAxesActorConn[yIdx][zAxes]][2])
     {
     zIdx = xIdx;
     zIdx2 = vtkCubeAxesActorConn[xIdx][zAxes];
     }
   else
     {
     zIdx = yIdx;
     zIdx2 = vtkCubeAxesActorConn[yIdx][zAxes];
     }

   int mini = (idx < xIdx ? idx : xIdx);
   switch (xAxes)
     {
     case 0:
       xloc = vtkCubeAxesActorTriads[mini][0];
       break;
     case 1:
       yloc = vtkCubeAxesActorTriads[mini][1];
       break;
     case 2:
       zloc = vtkCubeAxesActorTriads[mini][2];
       break;
     }
   mini = (idx < yIdx ? idx : yIdx);
   switch (yAxes)
     {
     case 0:
       xloc = vtkCubeAxesActorTriads[mini][0];
       break;
     case 1:
       yloc =vtkCubeAxesActorTriads[mini][1];
       break;
     case 2:
       zloc = vtkCubeAxesActorTriads[mini][2];
       break;
     }
   mini = (zIdx < zIdx2 ? zIdx : zIdx2);
   switch (zAxes)
     {
     case 0:
       xloc = vtkCubeAxesActorTriads[mini][0];
       break;
     case 1:
       yloc = vtkCubeAxesActorTriads[mini][1];
       break;
     case 2:
       zloc = vtkCubeAxesActorTriads[mini][2];
       break;
     }
 }

// --------------------------------------------------------------------------
int vtkCubeAxesActor::RenderGeometry(
    bool &initialRender, vtkViewport *viewport, bool checkAxisVisibility,
    int (vtkAxisActor::*renderMethod)(vtkViewport*))
{
  int i, renderedSomething = 0;

  // Make sure axes are initialized and visibility is properly set
  if(checkAxisVisibility)
    {
    // Initialization
    if (!this->Camera)
      {
      vtkErrorMacro(<<"No camera!");
      this->RenderSomething = 0;
      return 0;
      }

    this->BuildAxes(viewport);

    if (initialRender)
      {
      for (i = 0; i < NUMBER_OF_ALIGNED_AXIS; i++)
        {
        this->XAxes[i]->BuildAxis(viewport, true);
        this->YAxes[i]->BuildAxis(viewport, true);
        this->ZAxes[i]->BuildAxis(viewport, true);
        }
      }
    initialRender = false;

    this->DetermineRenderAxes(viewport);
    }

  // Render the axes
  for (i = 0; i < this->NumberOfAxesX; i++)
    {
    renderedSomething +=
        (this->XAxes[this->RenderAxesX[i]]->*renderMethod)(viewport);
    }

  for (i = 0; i < this->NumberOfAxesY; i++)
    {
    renderedSomething +=
        (this->YAxes[this->RenderAxesY[i]]->*renderMethod)(viewport);
    }

  for (i = 0; i < this->NumberOfAxesZ; i++)
    {
    renderedSomething +=
        (this->ZAxes[this->RenderAxesZ[i]]->*renderMethod)(viewport);
    }
  return renderedSomething;
}

// --------------------------------------------------------------------------
void vtkCubeAxesActor::ComputeStickyAxesBoundingSphere(vtkViewport* viewport,
                                                       const double originalBounds[6],
                                                       double sphereCenter[3],
                                                       double & sphereRadius)
{
  double aspect[2];
  viewport->GetAspect(aspect);
  vtkPlanes* frustumPlanes = vtkPlanes::New();
  double frustumPlanesArray[24];
  this->GetCamera()->GetFrustumPlanes(aspect[0], frustumPlanesArray);
  frustumPlanes->SetFrustumPlanes(frustumPlanesArray);

  vtkFrustumSource* frustumSource = vtkFrustumSource::New();
  frustumSource->SetPlanes(frustumPlanes);
  frustumPlanes->Delete();
  frustumSource->Update();

  vtkPoints* points = frustumSource->GetOutput()->GetPoints();

  // From http://gamedev.stackexchange.com/questions/60104/largest-sphere-inside-a-frustum
  // Point indices are set up to match the second figure.
  double p0[3], p1[3], p2[3], p3[3], p4[3];
  double q0[3], q1[3], q2[3], q3[3], q4[3];
  points->GetPoint(0, p1); // left bottom near
  points->GetPoint(1, p2); // right bottom near
  points->GetPoint(2, p4); // right top near
  points->GetPoint(3, p3); // left top near

  points->GetPoint(4, q1); // left bottom far
  points->GetPoint(5, q2); // right bottom far
  points->GetPoint(6, q4); // right top far
  points->GetPoint(7, q3); // left top far

  for (int i = 0; i < 3; ++i)
    {
    p0[i] = 0.25*(p1[i] + p2[i] + p3[i] + p4[i]); // near center
    q0[i] = 0.25*(q1[i] + q2[i] + q3[i] + q4[i]); // far center
    }
  frustumSource->Delete();

  double view[3];
  vtkMath::Subtract(p0, q0, view);
  double d = vtkMath::Norm(view);

  double v0[3], v1[3];
  vtkMath::Subtract(p1, q1, v0);
  vtkMath::Subtract(q2, q1, v1);
  double l = 0.5*vtkMath::Norm(v1);
  double alpha = atan(vtkMath::Dot(v0, v1) / (d*vtkMath::Norm(v1)));
  double halfWidth = l * tan((vtkMath::Pi() - 2.0*alpha) / 4.0);

  vtkMath::Subtract(q3, q1, v1);
  l = 0.5*vtkMath::Norm(v1);
  alpha = atan(vtkMath::Dot(v0, v1) / (d*vtkMath::Norm(v1)));
  double halfHeight = l * tan((vtkMath::Pi() - 2.0*alpha) / 4.0);

  sphereRadius = std::min(halfWidth, halfHeight);

  vtkMath::Normalize(view);
  sphereCenter[0] = q0[0] + sphereRadius*view[0];
  sphereCenter[1] = q0[1] + sphereRadius*view[1];
  sphereCenter[2] = q0[2] + sphereRadius*view[2];

  // Now shift the sphere so that its center is at the same depth as
  // the original bounding box.
  double* sidePlane;
  if (viewport->GetSize()[0] < viewport->GetSize()[1])
    {
    sidePlane = frustumPlanesArray + 0; // left side
    }
  else
    {
    sidePlane = frustumPlanesArray + 8; // bottom side
    }
  double f = vtkMath::Dot(q0, sidePlane) + sidePlane[3];

  vtkBoundingBox bb(originalBounds);
  double bbCenter[3];
  bb.GetCenter(bbCenter);
  double* backPlane = frustumPlanesArray + 16;
  double g = vtkMath::Dot(bbCenter, backPlane) + backPlane[3];
  double radiusReduction = (g - sphereRadius) * ((f - sphereRadius) / sphereRadius);

  sphereRadius -= radiusReduction;

  vtkMath::Subtract(p0, q0, view);
  vtkMath::Normalize(view);

  sphereCenter[0] = q0[0] + g*view[0];
  sphereCenter[1] = q0[1] + g*view[1];
  sphereCenter[2] = q0[2] + g*view[2];

  if (this->CenterStickyAxes)
    {
    // No need to shift the sticky axes bounding box up/down or left/right.
    return;
    }

  // Now see whether we can shift the sphere toward the side of the
  // frustum closest to the new sphere center.
  double shiftDirection[3];
  double minusSide[4], plusSide[4];
  if (viewport->GetSize()[0] < viewport->GetSize()[1])
    {
    vtkMath::Subtract(q1, q3, shiftDirection); // up vector

    memcpy(minusSide, frustumPlanesArray+2*4, 4*sizeof(double)); // bottom frustum side
    memcpy(plusSide,  frustumPlanesArray+3*4, 4*sizeof(double)); // top frustum side
    }
  else if (viewport->GetSize()[0] > viewport->GetSize()[1])
    {
    vtkMath::Subtract(q1, q2, shiftDirection); // right vector

    memcpy(minusSide, frustumPlanesArray+0*4, 4*sizeof(double)); // left frustum side
    memcpy(plusSide,  frustumPlanesArray+1*4, 4*sizeof(double)); // right frustum side
    }
  else // viewport->GetSize()[0] == viewport->GetSize()[1]
    {
    // Nothing to do; sticky bounding sphere is already centered
    return;
    }

  // Shift the sphere to the size of the frustum closest to the center
  // of the original bounding box.
  vtkMath::Normalize(shiftDirection);

  double v[3], shift[3], newCenter[3];
  vtkMath::Subtract(bbCenter, sphereCenter, v);
  vtkMath::ProjectVector(v, shiftDirection, shift);
  vtkMath::Add(sphereCenter, shift, newCenter);

  // Change the sphere center to this new center. Below, we check if
  // we have gone too far toward the frustum.
  memcpy(sphereCenter, newCenter, 3*sizeof(double));

  // Shift plane by the sphere radius in towards the center of the frustum
  minusSide[3] -= sphereRadius;
  plusSide[3]  -= sphereRadius;

  // Is the newCenter outside the shifted frustum minus side?
  if (vtkMath::Dot(minusSide, newCenter) + minusSide[3] < 0.0)
    {
    // Intersection with shifted bottom side
    double t = -(vtkMath::Dot(minusSide, newCenter) + minusSide[3]) /
      (vtkMath::Dot(minusSide, shiftDirection));
    sphereCenter[0] = newCenter[0] + t*shiftDirection[0];
    sphereCenter[1] = newCenter[1] + t*shiftDirection[1];
    sphereCenter[2] = newCenter[2] + t*shiftDirection[2];
    }

  // Is the newCenter outside the shifted frustum plus side?
  if (vtkMath::Dot(plusSide, newCenter) + plusSide[3] < 0.0)
    {
    // Intersection with shifted top side
    double t = -(vtkMath::Dot(plusSide, newCenter) + plusSide[3]) /
      (vtkMath::Dot(plusSide, shiftDirection));
    sphereCenter[0] = newCenter[0] + t*shiftDirection[0];
    sphereCenter[1] = newCenter[1] + t*shiftDirection[1];
    sphereCenter[2] = newCenter[2] + t*shiftDirection[2];
    }
}

// --------------------------------------------------------------------------
void vtkCubeAxesActor::GetViewportLimitedBounds(vtkViewport* viewport,
                                                double bounds[6])
{
  double originalBounds[6];
  this->GetBounds(originalBounds);
  vtkBoundingBox originalBB(originalBounds);
  double originalCenter[3];
  originalBB.GetCenter(originalCenter);

  double sphereCenter[3];
  double sphereRadius;
  this->ComputeStickyAxesBoundingSphere(viewport, originalBounds,
                                        sphereCenter, sphereRadius);

  // Now that we have the maximal sphere that will fit in the frustum,
  // compute a cubic bounding box that fits inside it.
  vtkBoundingBox sphereBB;
  double direction[3] = {1, 1, 1};
  vtkMath::Normalize(direction);
  double pt1[3] = {sphereCenter[0] + sphereRadius*direction[0],
                   sphereCenter[1] + sphereRadius*direction[1],
                   sphereCenter[2] + sphereRadius*direction[2]};
  sphereBB.AddPoint(pt1);

  // Opposite corner
  double pt2[3] = {sphereCenter[0] - sphereRadius*direction[0],
                   sphereCenter[1] - sphereRadius*direction[1],
                   sphereCenter[2] - sphereRadius*direction[2]};
  sphereBB.AddPoint(pt2);

  // Now intersect this sphere bounding box with the original bounds
  // to get the final sticky bounds
  if (originalBB.IntersectBox(sphereBB) == 1)
    {
    originalBB.GetBounds(bounds);
    }
  else
    {
    bounds[0] = bounds[1] = bounds[2] = bounds[3] = bounds[4] = bounds[5] = 0.0;
    }
}

// --------------------------------------------------------------------------
void vtkCubeAxesActor::GetBoundsPointBits(unsigned int pointIndex,
                                          unsigned int & xBit,
                                          unsigned int & yBit,
                                          unsigned int & zBit)
{
  // Coordinate position is encoded in binary:
  // 1st bit - 0 for minimum x, 1 for maximum x
  // 2nd bit - 0 for minimum y, 1 for maximum y
  // 3rd bit - 0 for minimum z, 1 for maximum z
  unsigned int xMask = 1;
  unsigned int yMask = 2;
  unsigned int zMask = 4;

  xBit = (pointIndex & xMask) >> 0;
  yBit = (pointIndex & yMask) >> 1;
  zBit = (pointIndex & zMask) >> 2;
}

// --------------------------------------------------------------------------
void vtkCubeAxesActor::GetBoundsPoint(unsigned int pointIndex, const double bounds[6],
                                      double point[3])
{
  if (pointIndex > 7)
    {
    return;
    }

  unsigned int xBit, yBit, zBit;
  vtkCubeAxesActor::GetBoundsPointBits(pointIndex, xBit, yBit, zBit);
  point[0] = bounds[xBit + 0];
  point[1] = bounds[yBit + 2];
  point[2] = bounds[zBit + 4];
}

// --------------------------------------------------------------------------
void vtkCubeAxesActor::GetRenderedBounds(double *b)
{
  vtkBoundingBox bbox(this->GetBounds()); // Data bounds

  // Make a heuristic on the final bounds that embed test labels
  // Just inflate the box based on its max length
  bbox.Inflate(bbox.GetMaxLength());

  bbox.GetBounds(b);
}

// --------------------------------------------------------------------------
double* vtkCubeAxesActor::GetRenderedBounds()
{
  this->GetRenderedBounds(this->RenderedBounds);
  // Return our data holder
  return this->RenderedBounds;
}
