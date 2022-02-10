/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBorderRepresentation.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkBorderRepresentation.h"
#include "vtkActor2D.h"
#include "vtkCellArray.h"
#include "vtkFeatureEdges.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper2D.h"
#include "vtkProperty2D.h"
#include "vtkRenderer.h"
#include "vtkTransform.h"
#include "vtkTransformPolyDataFilter.h"
#include "vtkWindow.h"

#include <algorithm>
#include <cassert>

vtkStandardNewMacro(vtkBorderRepresentation);

//------------------------------------------------------------------------------
vtkBorderRepresentation::vtkBorderRepresentation()
{
  this->InteractionState = vtkBorderRepresentation::Outside;

  // Initial positioning information
  this->Negotiated = 0;
  this->PositionCoordinate->SetCoordinateSystemToNormalizedViewport();
  this->PositionCoordinate->SetValue(0.05, 0.05);
  this->Position2Coordinate->SetCoordinateSystemToNormalizedViewport();
  this->Position2Coordinate->SetValue(0.1, 0.1); // may be updated by the subclass
  this->Position2Coordinate->SetReferenceCoordinate(this->PositionCoordinate);

  // Create the geometry in canonical coordinates
  this->BWPoints->SetDataTypeToDouble();
  this->BWPoints->SetNumberOfPoints(4);
  this->BWPoints->SetPoint(0, 0.0, 0.0, 0.0); // may be updated by the subclass
  this->BWPoints->SetPoint(1, 1.0, 0.0, 0.0);
  this->BWPoints->SetPoint(2, 1.0, 1.0, 0.0);
  this->BWPoints->SetPoint(3, 0.0, 1.0, 0.0);

  vtkNew<vtkCellArray> outline;
  outline->InsertNextCell(5);
  outline->InsertCellPoint(0);
  outline->InsertCellPoint(1);
  outline->InsertCellPoint(2);
  outline->InsertCellPoint(3);
  outline->InsertCellPoint(0);

  this->BWPolyData->SetPoints(this->BWPoints);
  this->BWPolyData->SetLines(outline);

  this->BWTransformFilter->SetTransform(this->BWTransform);
  this->BWTransformFilter->SetInputData(this->BWPolyData);

  // In order to link a different property for the border
  // and the inner polygon, we create 2 new polydata that will
  // share the points of the input poly data
  // Beware that this will break the pipeline, so we need to
  // Call update manually on BWTransformFilter

  // Edges
  this->BWMapperEdges->SetInputData(this->PolyDataEdges);
  this->BWActorEdges->SetMapper(this->BWMapperEdges);
  this->BorderProperty->SetColor(this->BorderColor);
  this->BorderProperty->SetLineWidth(this->BorderThickness);
  this->BorderProperty->SetPointSize(1.5f);
  this->BWActorEdges->SetProperty(this->BorderProperty);

  // Inner polygon
  this->BWMapperPolygon->SetInputData(this->PolyDataPolygon);
  this->BWActorPolygon->SetMapper(this->BWMapperPolygon);
  this->PolygonProperty->SetColor(this->PolygonColor);
  this->PolygonProperty->SetOpacity(this->PolygonOpacity);
  this->PolygonProperty->SetPointSize(0.f);
  this->BWActorPolygon->SetProperty(this->PolygonProperty);
}

//------------------------------------------------------------------------------
vtkBorderRepresentation::~vtkBorderRepresentation() = default;

//------------------------------------------------------------------------------
void vtkBorderRepresentation::ComputeRoundCorners()
{
  vtkCellArray* lines = this->BWPolyData->GetLines();

  // Link the pipeline manually as we need two properties for the border
  // and for the inner polygon
  this->BWTransformFilter->Update();

  // Create round corners after the transform as we do not want to scale the corners
  vtkPolyData* pd = this->BWTransformFilter->GetOutput();
  vtkNew<vtkPoints> pdPoints;
  pdPoints->DeepCopy(pd->GetPoints());

  if (lines->GetNumberOfCells() != 1 || this->CornerResolution == 0)
  {
    // All borders are not shown, we cannot compute
    // round corners
    this->PolyDataEdges->SetPoints(pdPoints);
    this->PolyDataEdges->SetLines(lines);

    this->PolyDataPolygon->SetPoints(pdPoints);
    this->PolyDataPolygon->SetPolys(lines);

    return;
  }

  // Get the bottom left corner point
  double p0[3];
  pdPoints->GetPoint(0, p0);

  // And the top right corner point
  double p1[3];
  pdPoints->GetPoint(2, p1);

  // Scale the maximum radius by radius strength
  double radius = this->CornerRadiusStrength * std::min(p1[0] - p0[0], p1[1] - p0[1]) / 2.0;

  // Add 2 points of each side of each corners to start and end the
  // curve of the round corner. With the previous 4 points, the number of
  // points is now 12
  pdPoints->SetNumberOfPoints(12);
  // Bottom-left corner
  pdPoints->SetPoint(4, p0[0], p0[1] + radius, 0.0);
  pdPoints->SetPoint(5, p0[0] + radius, p0[1], 0.0);
  // Bottom-right corner
  pdPoints->SetPoint(6, p1[0] - radius, p0[1], 0.0);
  pdPoints->SetPoint(7, p1[0], p0[1] + radius, 0.0);
  // Top-right corner
  pdPoints->SetPoint(8, p1[0], p1[1] - radius, 0.0);
  pdPoints->SetPoint(9, p1[0] - radius, p1[1], 0.0);
  // Top-left corner
  pdPoints->SetPoint(10, p0[0] + radius, p1[1], 0.0);
  pdPoints->SetPoint(11, p0[0], p1[1] - radius, 0.0);

  // Create polygon with only one cell
  vtkNew<vtkCellArray> polys;
  polys->InsertNextCell(4 * this->CornerResolution + 1);

  // Compute bottom-left corner
  this->ComputeOneRoundCorner(polys, pdPoints, radius, 5, 4, vtkMath::Pi());
  // Compute bottom-right corner
  this->ComputeOneRoundCorner(polys, pdPoints, radius, 6, 7, 3.0 * vtkMath::Pi() / 2.0);
  // Compute top-right corner
  this->ComputeOneRoundCorner(polys, pdPoints, radius, 9, 8, 0.0);
  // Compute top-left corner
  this->ComputeOneRoundCorner(polys, pdPoints, radius, 10, 11, vtkMath::Pi() / 2.0);

  // Don't forget to link the last point
  polys->InsertCellPoint(12);

  this->PolyDataEdges->SetPoints(pdPoints);
  this->PolyDataEdges->SetVerts(polys);
  this->PolyDataEdges->SetLines(polys);

  this->PolyDataPolygon->SetPoints(pdPoints);
  this->PolyDataPolygon->SetPolys(polys);
}

//------------------------------------------------------------------------------
void vtkBorderRepresentation::ComputeOneRoundCorner(vtkCellArray* polys, vtkPoints* points,
  const double radius, vtkIdType idCenterX, vtkIdType idCenterY, const double startAngle)
{
  double xPoint[3], yPoint[3];
  points->GetPoint(idCenterX, xPoint);
  points->GetPoint(idCenterY, yPoint);
  double center[2] = { xPoint[0], yPoint[1] };

  // Angle step in radians
  double angleStep = vtkMath::Pi() / (2.0 * this->CornerResolution);
  double angle = startAngle;

  for (int i = 0; i < this->CornerResolution; ++i)
  {
    double x = center[0] + radius * cos(angle);
    double y = center[1] + radius * sin(angle);
    vtkIdType id = points->InsertNextPoint(x, y, 0.0);
    polys->InsertCellPoint(id);
    angle += angleStep;
  }
}

//------------------------------------------------------------------------------
vtkMTimeType vtkBorderRepresentation::GetMTime()
{
  vtkMTimeType mTime = this->Superclass::GetMTime();
  mTime = std::max(mTime, this->PositionCoordinate->GetMTime());
  mTime = std::max(mTime, this->Position2Coordinate->GetMTime());
  mTime = std::max(mTime, this->BorderProperty->GetMTime());
  mTime = std::max(mTime, this->PolygonProperty->GetMTime());
  return mTime;
}

//------------------------------------------------------------------------------
void vtkBorderRepresentation::SetShowBorder(int border)
{
  this->SetShowVerticalBorder(border);
  this->SetShowHorizontalBorder(border);
  this->SetShowPolygonBackground(border);
  this->UpdateShowBorder();
}

//------------------------------------------------------------------------------
int vtkBorderRepresentation::GetShowBorderMinValue()
{
  return BORDER_OFF;
}

//------------------------------------------------------------------------------
int vtkBorderRepresentation::GetShowBorderMaxValue()
{
  return BORDER_ACTIVE;
}

//------------------------------------------------------------------------------
int vtkBorderRepresentation::GetShowBorder()
{
  return this->GetShowVerticalBorder() != BORDER_OFF
    ? this->GetShowVerticalBorder()
    : (this->GetShowHorizontalBorder() != BORDER_OFF ? this->GetShowHorizontalBorder()
                                                     : this->GetShowPolygonBackground());
}

//------------------------------------------------------------------------------
void vtkBorderRepresentation::SetShowPolygon(int polygon)
{
  this->SetShowPolygonBackground(polygon);
  this->UpdateShowBorder();
}

//------------------------------------------------------------------------------
int vtkBorderRepresentation::GetShowPolygon()
{
  return this->GetShowPolygonBackground();
}

//------------------------------------------------------------------------------
void vtkBorderRepresentation::StartWidgetInteraction(double eventPos[2])
{
  this->StartEventPosition[0] = eventPos[0];
  this->StartEventPosition[1] = eventPos[1];
}

//------------------------------------------------------------------------------
void vtkBorderRepresentation::WidgetInteraction(double eventPos[2])
{
  double XF = eventPos[0];
  double YF = eventPos[1];

  // convert to normalized viewport coordinates
  this->Renderer->DisplayToNormalizedDisplay(XF, YF);
  this->Renderer->NormalizedDisplayToViewport(XF, YF);
  this->Renderer->ViewportToNormalizedViewport(XF, YF);

  // there are four parameters that can be adjusted
  double* fpos1 = this->PositionCoordinate->GetValue();
  double* fpos2 = this->Position2Coordinate->GetValue();
  double par1[2];
  double par2[2];
  par1[0] = fpos1[0];
  par1[1] = fpos1[1];
  par2[0] = fpos1[0] + fpos2[0];
  par2[1] = fpos1[1] + fpos2[1];

  double delX = XF - this->StartEventPosition[0];
  double delY = YF - this->StartEventPosition[1];
  double delX2 = 0.0, delY2 = 0.0;

  // Based on the state, adjust the representation. Note that we force a
  // uniform scaling of the widget when tugging on the corner points (and
  // when proportional resize is on). This is done by finding the maximum
  // movement in the x-y directions and using this to scale the widget.
  if (this->ProportionalResize && !this->Moving)
  {
    double sx = fpos2[0] / fpos2[1];
    double sy = fpos2[1] / fpos2[0];
    if (fabs(delX) > fabs(delY))
    {
      delY = sy * delX;
      delX2 = delX;
      delY2 = -delY;
    }
    else
    {
      delX = sx * delY;
      delY2 = delY;
      delX2 = -delX;
    }
  }
  else
  {
    delX2 = delX;
    delY2 = delY;
  }

  // The previous "if" statement has taken care of the proportional resize
  // for the most part. However, tugging on edges has special behavior, which
  // is to scale the box about its center.
  switch (this->InteractionState)
  {
    case vtkBorderRepresentation::AdjustingP0:
      par1[0] = par1[0] + delX;
      par1[1] = par1[1] + delY;
      break;
    case vtkBorderRepresentation::AdjustingP1:
      par2[0] = par2[0] + delX2;
      par1[1] = par1[1] + delY2;
      break;
    case vtkBorderRepresentation::AdjustingP2:
      par2[0] = par2[0] + delX;
      par2[1] = par2[1] + delY;
      break;
    case vtkBorderRepresentation::AdjustingP3:
      par1[0] = par1[0] + delX2;
      par2[1] = par2[1] + delY2;
      break;
    case vtkBorderRepresentation::AdjustingE0:
      par1[1] = par1[1] + delY;
      if (this->ProportionalResize)
      {
        par2[1] = par2[1] - delY;
        par1[0] = par1[0] + delX;
        par2[0] = par2[0] - delX;
      }
      break;
    case vtkBorderRepresentation::AdjustingE1:
      par2[0] = par2[0] + delX;
      if (this->ProportionalResize)
      {
        par1[0] = par1[0] - delX;
        par1[1] = par1[1] - delY;
        par2[1] = par2[1] + delY;
      }
      break;
    case vtkBorderRepresentation::AdjustingE2:
      par2[1] = par2[1] + delY;
      if (this->ProportionalResize)
      {
        par1[1] = par1[1] - delY;
        par1[0] = par1[0] - delX;
        par2[0] = par2[0] + delX;
      }
      break;
    case vtkBorderRepresentation::AdjustingE3:
      par1[0] = par1[0] + delX;
      if (this->ProportionalResize)
      {
        par2[0] = par2[0] - delX;
        par1[1] = par1[1] + delY;
        par2[1] = par2[1] - delY;
      }
      break;
    case vtkBorderRepresentation::Inside:
      if (this->Moving)
      {
        par1[0] = par1[0] + delX;
        par1[1] = par1[1] + delY;
        par2[0] = par2[0] + delX;
        par2[1] = par2[1] + delY;
      }
      break;
  }

  // Enforce bounds to keep the widget on screen and bigger than minimum size
  if (!this->ProportionalResize && this->EnforceNormalizedViewportBounds)
  {
    switch (this->InteractionState)
    {
      case vtkBorderRepresentation::AdjustingP0:
        par1[0] = std::min(
          std::max(par1[0] /*+ delX*/, 0.0), par2[0] - this->MinimumNormalizedViewportSize[0]);
        par1[1] = std::min(
          std::max(par1[1] /*+ delY*/, 0.0), par2[1] - this->MinimumNormalizedViewportSize[1]);
        break;
      case vtkBorderRepresentation::AdjustingP1:
        par2[0] = std::min(
          std::max(par2[0] /*+ delX2*/, par1[0] + this->MinimumNormalizedViewportSize[0]), 1.0);
        par1[1] = std::min(
          std::max(par1[1] /*+ delY2*/, 0.0), par2[1] - this->MinimumNormalizedViewportSize[1]);
        break;
      case vtkBorderRepresentation::AdjustingP2:
        par2[0] = std::min(
          std::max(par2[0] /*+ delX*/, par1[0] + this->MinimumNormalizedViewportSize[0]), 1.0);
        par2[1] = std::min(
          std::max(par2[1] /*+ delY*/, par1[1] + this->MinimumNormalizedViewportSize[1]), 1.0);
        break;
      case vtkBorderRepresentation::AdjustingP3:
        par1[0] = std::min(
          std::max(par1[0] /*+ delX2*/, 0.0), par2[0] - this->MinimumNormalizedViewportSize[0]);
        par2[1] = std::min(
          std::max(par2[1] /*+ delY2*/, par1[1] + this->MinimumNormalizedViewportSize[1]), 1.0);
        break;
      case vtkBorderRepresentation::AdjustingE0:
        par1[1] = std::min(
          std::max(par1[1] /*+ delY*/, 0.0), par2[1] - this->MinimumNormalizedViewportSize[1]);
        break;
      case vtkBorderRepresentation::AdjustingE1:
        par2[0] = std::min(
          std::max(par2[0] /*+ delX*/, par1[0] + this->MinimumNormalizedViewportSize[0]), 1.0);
        break;
      case vtkBorderRepresentation::AdjustingE2:
        par2[1] = std::min(
          std::max(par2[1] /*+ delY*/, par1[1] + this->MinimumNormalizedViewportSize[1]), 1.0);
        break;
      case vtkBorderRepresentation::AdjustingE3:
        par1[0] = std::min(
          std::max(par1[0] /*+ delX*/, 0.0), par2[0] - this->MinimumNormalizedViewportSize[0]);
        break;
      case vtkBorderRepresentation::Inside:
        if (this->Moving)
        {
          // Keep border from moving off normalized screen
          if (par1[0] < 0.0)
          {
            double delta = -par1[0];
            par1[0] += delta;
            par2[0] += delta;
          }
          if (par1[1] < 0.0)
          {
            double delta = -par1[1];
            par1[1] += delta;
            par2[1] += delta;
          }
          if (par2[0] > 1.0)
          {
            double delta = par2[0] - 1.0;
            par1[0] -= delta;
            par2[0] -= delta;
          }
          if (par2[1] > 1.0)
          {
            double delta = par2[1] - 1.0;
            par1[1] -= delta;
            par2[1] -= delta;
          }
        }
        break;
      default:
        break;
    }
  }

  // Modify the representation
  if (par2[0] > par1[0] && par2[1] > par1[1])
  {
    this->PositionCoordinate->SetValue(par1[0], par1[1]);
    this->Position2Coordinate->SetValue(par2[0] - par1[0], par2[1] - par1[1]);
    this->StartEventPosition[0] = XF;
    this->StartEventPosition[1] = YF;
  }

  this->Modified();
  this->BuildRepresentation();
}

//------------------------------------------------------------------------------
void vtkBorderRepresentation::NegotiateLayout()
{
  double size[2];
  this->GetSize(size);

  // Update the initial border geometry
  this->BWPoints->SetPoint(0, 0.0, 0.0, 0.0); // may be updated by the subclass
  this->BWPoints->SetPoint(1, size[0], 0.0, 0.0);
  this->BWPoints->SetPoint(2, size[0], size[1], 0.0);
  this->BWPoints->SetPoint(3, 0.0, size[1], 0.0);
}

//------------------------------------------------------------------------------
int vtkBorderRepresentation::ComputeInteractionState(int X, int Y, int vtkNotUsed(modify))
{
  int* pos1 = this->PositionCoordinate->GetComputedDisplayValue(this->Renderer);
  int* pos2 = this->Position2Coordinate->GetComputedDisplayValue(this->Renderer);

  // Figure out where we are in the widget. Exclude outside case first.
  if (X < (pos1[0] - this->Tolerance) || (pos2[0] + this->Tolerance) < X ||
    Y < (pos1[1] - this->Tolerance) || (pos2[1] + this->Tolerance) < Y)
  {
    this->InteractionState = vtkBorderRepresentation::Outside;
  }

  else // we are on the boundary or inside the border
  {
    // Now check for proximinity to edges and points
    int e0 = (Y >= (pos1[1] - this->Tolerance) && Y <= (pos1[1] + this->Tolerance));
    int e1 = (X >= (pos2[0] - this->Tolerance) && X <= (pos2[0] + this->Tolerance));
    int e2 = (Y >= (pos2[1] - this->Tolerance) && Y <= (pos2[1] + this->Tolerance));
    int e3 = (X >= (pos1[0] - this->Tolerance) && X <= (pos1[0] + this->Tolerance));

    int adjustHorizontalEdges = (this->ShowHorizontalBorder != BORDER_OFF);
    int adjustVerticalEdges = (this->ShowVerticalBorder != BORDER_OFF);
    int adjustPoints = (adjustHorizontalEdges && adjustVerticalEdges);

    if (e0 && e1 && adjustPoints)
    {
      this->InteractionState = vtkBorderRepresentation::AdjustingP1;
    }
    else if (e1 && e2 && adjustPoints)
    {
      this->InteractionState = vtkBorderRepresentation::AdjustingP2;
    }
    else if (e2 && e3 && adjustPoints)
    {
      this->InteractionState = vtkBorderRepresentation::AdjustingP3;
    }
    else if (e3 && e0 && adjustPoints)
    {
      this->InteractionState = vtkBorderRepresentation::AdjustingP0;
    }

    // Edges
    else if (e0 || e1 || e2 || e3)
    {
      if (e0 && adjustHorizontalEdges)
      {
        this->InteractionState = vtkBorderRepresentation::AdjustingE0;
      }
      else if (e1 && adjustVerticalEdges)
      {
        this->InteractionState = vtkBorderRepresentation::AdjustingE1;
      }
      else if (e2 && adjustHorizontalEdges)
      {
        this->InteractionState = vtkBorderRepresentation::AdjustingE2;
      }
      else if (e3 && adjustVerticalEdges)
      {
        this->InteractionState = vtkBorderRepresentation::AdjustingE3;
      }
    }

    else // must be interior
    {
      if (this->Moving)
      {
        // FIXME: This must be wrong.  Moving is not an entry in the
        // InteractionStateType enum.  It is an ivar flag and it has no business
        // being set to InteractionState.  This just happens to work because
        // Inside happens to be 1, and this gets set when Moving is 1.
        this->InteractionState = vtkBorderRepresentation::Moving;
      }
      else
      {
        this->InteractionState = vtkBorderRepresentation::Inside;
      }
    }
  } // else inside or on border
  this->UpdateShowBorder();

  return this->InteractionState;
}

//------------------------------------------------------------------------------
void vtkBorderRepresentation::UpdateShowBorder()
{
  enum
  {
    NoBorder = 0x00,
    VerticalBorder = 0x01,
    HorizontalBorder = 0x02,
    AllBorders = VerticalBorder | HorizontalBorder
  };

  int currentBorder = NoBorder;
  switch (this->BWPolyData->GetLines()->GetNumberOfCells())
  {
    case 1:
      currentBorder = AllBorders;
      break;
    case 2:
    {
      vtkIdType npts = 0;
      const vtkIdType* pts = nullptr;
      this->BWPolyData->GetLines()->GetCellAtId(0, npts, pts);
      assert(npts == 2);
      currentBorder = (pts[0] == 0 ? HorizontalBorder : VerticalBorder);
      break;
    }
    case 0:
    default: // not supported
      currentBorder = NoBorder;
      break;
  }
  bool currentBackground = this->BWActorPolygon->GetVisibility();

  int newBorder = NoBorder;
  if (this->ShowVerticalBorder == this->ShowHorizontalBorder)
  {
    newBorder = (this->ShowVerticalBorder == BORDER_ON ||
                  (this->ShowVerticalBorder == BORDER_ACTIVE &&
                    this->InteractionState != vtkBorderRepresentation::Outside))
      ? AllBorders
      : NoBorder;
  }
  else
  {
    newBorder = newBorder |
      ((this->ShowVerticalBorder == BORDER_ON ||
         (this->ShowVerticalBorder == BORDER_ACTIVE &&
           this->InteractionState != vtkBorderRepresentation::Outside))
          ? VerticalBorder
          : NoBorder);
    newBorder = newBorder |
      ((this->ShowHorizontalBorder == BORDER_ON ||
         (this->ShowHorizontalBorder == BORDER_ACTIVE &&
           this->InteractionState != vtkBorderRepresentation::Outside))
          ? HorizontalBorder
          : NoBorder);
  }
  bool backgroundVisible = (this->ShowPolygonBackground == BORDER_ON ||
    (this->ShowPolygonBackground == BORDER_ACTIVE &&
      this->InteractionState != vtkBorderRepresentation::Outside));

  bool edgesVisible = (newBorder != NoBorder);
  if ((currentBorder != newBorder || currentBackground != backgroundVisible) &&
    (edgesVisible || backgroundVisible))
  {
    vtkNew<vtkCellArray> outline;
    switch (newBorder)
    {
      case NoBorder: // fall-through for backgrounds only
      case AllBorders:
        outline->InsertNextCell(5);
        outline->InsertCellPoint(0);
        outline->InsertCellPoint(1);
        outline->InsertCellPoint(2);
        outline->InsertCellPoint(3);
        outline->InsertCellPoint(0);
        break;
      case VerticalBorder:
        outline->InsertNextCell(2);
        outline->InsertCellPoint(1);
        outline->InsertCellPoint(2);
        outline->InsertNextCell(2);
        outline->InsertCellPoint(3);
        outline->InsertCellPoint(0);
        break;
      case HorizontalBorder:
        outline->InsertNextCell(2);
        outline->InsertCellPoint(0);
        outline->InsertCellPoint(1);
        outline->InsertNextCell(2);
        outline->InsertCellPoint(2);
        outline->InsertCellPoint(3);
        break;
      default:
        break;
    }
    this->BWPolyData->SetLines(outline);
    this->BWPolyData->Modified();
    this->Modified();
    this->ComputeRoundCorners();
  }
  this->BWActorEdges->SetVisibility(edgesVisible);
  this->BWActorPolygon->SetVisibility(backgroundVisible);
}

//------------------------------------------------------------------------------
void vtkBorderRepresentation::SetBWActorDisplayOverlay(bool enable)
{
  if (this->BWActorEdges)
  {
    this->BWActorEdges->SetVisibility(enable);
  }
  if (this->BWActorPolygon)
  {
    this->BWActorPolygon->SetVisibility(enable);
  }
}

//------------------------------------------------------------------------------
void vtkBorderRepresentation::SetBWActorDisplayOverlayEdges(bool enable)
{
  if (this->BWActorEdges)
  {
    this->BWActorEdges->SetVisibility(enable);
  }
}

//------------------------------------------------------------------------------
void vtkBorderRepresentation::SetBWActorDisplayOverlayPolygon(bool enable)
{
  if (this->BWActorPolygon)
  {
    this->BWActorPolygon->SetVisibility(enable);
  }
}

//------------------------------------------------------------------------------
void vtkBorderRepresentation::BuildRepresentation()
{
  if (this->Renderer &&
    (this->GetMTime() > this->BuildTime ||
      (this->Renderer->GetVTKWindow() &&
        this->Renderer->GetVTKWindow()->GetMTime() > this->BuildTime)))
  {
    // Negotiate with subclasses
    if (!this->Negotiated)
    {
      this->NegotiateLayout();
      this->Negotiated = 1;
    }

    // Set things up
    int* pos1 = this->PositionCoordinate->GetComputedViewportValue(this->Renderer);
    int* pos2 = this->Position2Coordinate->GetComputedViewportValue(this->Renderer);

    // If the widget's aspect ratio is to be preserved (ProportionalResizeOn),
    // then (pos1,pos2) are a bounding rectangle.
    if (this->ProportionalResize)
    {
    }

    // Now transform the canonical widget into display coordinates
    double size[2];
    this->GetSize(size);
    double tx = pos1[0];
    double ty = pos1[1];
    double sx = (pos2[0] - pos1[0]) / size[0];
    double sy = (pos2[1] - pos1[1]) / size[1];

    sx = (sx < this->MinimumSize[0] ? this->MinimumSize[0]
                                    : (sx > this->MaximumSize[0] ? this->MaximumSize[0] : sx));
    sy = (sy < this->MinimumSize[1] ? this->MinimumSize[1]
                                    : (sy > this->MaximumSize[1] ? this->MaximumSize[1] : sy));

    this->BWTransform->Identity();
    this->BWTransform->Translate(tx, ty, 0.0);
    this->BWTransform->Scale(sx, sy, 1);

    // Compute round corners after the transform has been set
    // Only if the polydata contains a unique cell (ie. all borders
    // are visible)
    this->ComputeRoundCorners();

    // Modify border properties
    this->BorderProperty->SetColor(this->BorderColor);
    this->BorderProperty->SetLineWidth(this->BorderThickness);

    // In order to fill the holes in the corners
    // We use a little trick : we render the points with
    // a point size that fill the holes
    double pointSize = this->BorderThickness;
    this->BorderProperty->SetPointSize(std::max(0.0, pointSize - 1.0));

    // And polygon properties
    this->PolygonProperty->SetColor(this->PolygonColor);
    this->PolygonProperty->SetOpacity(this->PolygonOpacity);

    this->BuildTime.Modified();
  }
}

//------------------------------------------------------------------------------
void vtkBorderRepresentation::GetActors2D(vtkPropCollection* pc)
{
  pc->AddItem(this->BWActorEdges);
  pc->AddItem(this->BWActorPolygon);
}

//------------------------------------------------------------------------------
void vtkBorderRepresentation::ReleaseGraphicsResources(vtkWindow* w)
{
  this->BWActorEdges->ReleaseGraphicsResources(w);
  this->BWActorPolygon->ReleaseGraphicsResources(w);
}

//------------------------------------------------------------------------------
int vtkBorderRepresentation::RenderOverlay(vtkViewport* w)
{
  this->BuildRepresentation();

  vtkTypeBool edgesVisible = this->BWActorEdges->GetVisibility();
  vtkTypeBool polygonVisible = this->BWActorPolygon->GetVisibility();
  if (edgesVisible && polygonVisible)
  {
    return this->BWActorEdges->RenderOverlay(w) && this->BWActorPolygon->RenderOverlay(w);
  }
  else if (edgesVisible)
  {
    return this->BWActorEdges->RenderOverlay(w);
  }
  else if (polygonVisible)
  {
    return this->BWActorPolygon->RenderOverlay(w);
  }
  return 0;
}

//------------------------------------------------------------------------------
int vtkBorderRepresentation::RenderOpaqueGeometry(vtkViewport* w)
{
  this->BuildRepresentation();

  vtkTypeBool edgesVisible = this->BWActorEdges->GetVisibility();
  vtkTypeBool polygonVisible = this->BWActorPolygon->GetVisibility();
  if (edgesVisible && polygonVisible)
  {
    return this->BWActorEdges->RenderOpaqueGeometry(w) &&
      this->BWActorPolygon->RenderOpaqueGeometry(w);
  }
  else if (edgesVisible)
  {
    return this->BWActorEdges->RenderOpaqueGeometry(w);
  }
  else if (polygonVisible)
  {
    return this->BWActorPolygon->RenderOpaqueGeometry(w);
  }
  return 0;
}

//------------------------------------------------------------------------------
int vtkBorderRepresentation::RenderTranslucentPolygonalGeometry(vtkViewport* w)
{
  this->BuildRepresentation();

  vtkTypeBool edgesVisible = this->BWActorEdges->GetVisibility();
  vtkTypeBool polygonVisible = this->BWActorPolygon->GetVisibility();
  if (edgesVisible && polygonVisible)
  {
    return this->BWActorEdges->RenderTranslucentPolygonalGeometry(w) &&
      this->BWActorPolygon->RenderTranslucentPolygonalGeometry(w);
  }
  else if (edgesVisible)
  {
    return this->BWActorEdges->RenderTranslucentPolygonalGeometry(w);
  }
  else if (polygonVisible)
  {
    return this->BWActorPolygon->RenderTranslucentPolygonalGeometry(w);
  }
  return 0;
}

//------------------------------------------------------------------------------
// Description:
// Does this prop have some translucent polygonal geometry?
vtkTypeBool vtkBorderRepresentation::HasTranslucentPolygonalGeometry()
{
  this->BuildRepresentation();

  vtkTypeBool edgesVisible = this->BWActorEdges->GetVisibility();
  vtkTypeBool polygonVisible = this->BWActorPolygon->GetVisibility();
  if (edgesVisible && polygonVisible)
  {
    return this->BWActorEdges->HasTranslucentPolygonalGeometry() &&
      this->BWActorPolygon->HasTranslucentPolygonalGeometry();
  }
  else if (edgesVisible)
  {
    return this->BWActorEdges->HasTranslucentPolygonalGeometry();
  }
  else if (polygonVisible)
  {
    return this->BWActorPolygon->HasTranslucentPolygonalGeometry();
  }
  return 0;
}

//------------------------------------------------------------------------------
void vtkBorderRepresentation::SetPolygonRGBA(double rgba[4])
{
  this->SetPolygonRGBA(rgba[0], rgba[1], rgba[2], rgba[3]);
}

//------------------------------------------------------------------------------
void vtkBorderRepresentation::SetPolygonRGBA(double r, double g, double b, double a)
{
  this->SetPolygonColor(r, g, b);
  this->SetPolygonOpacity(a);
}

//------------------------------------------------------------------------------
void vtkBorderRepresentation::GetPolygonRGBA(double rgba[4])
{
  this->GetPolygonRGBA(rgba[0], rgba[1], rgba[2], rgba[3]);
}

//------------------------------------------------------------------------------
void vtkBorderRepresentation::GetPolygonRGBA(double& r, double& g, double& b, double& a)
{
  this->GetPolygonColor(r, g, b);
  a = this->GetPolygonOpacity();
}

//------------------------------------------------------------------------------
void vtkBorderRepresentation::UpdateWindowLocation()
{
  if (this->WindowLocation != vtkBorderRepresentation::AnyLocation)
  {
    double* pos2 = this->Position2Coordinate->GetValue();
    switch (this->WindowLocation)
    {
      case vtkBorderRepresentation::LowerLeftCorner:
        this->SetPosition(0.01, 0.01);
        break;
      case vtkBorderRepresentation::LowerRightCorner:
        this->SetPosition(0.99 - pos2[0], 0.01);
        break;
      case vtkBorderRepresentation::LowerCenter:
        this->SetPosition((1 - pos2[0]) / 2.0, 0.01);
        break;
      case vtkBorderRepresentation::UpperLeftCorner:
        this->SetPosition(0.01, 0.99 - pos2[1]);
        break;
      case vtkBorderRepresentation::UpperRightCorner:
        this->SetPosition(0.99 - pos2[0], 0.99 - pos2[1]);
        break;
      case vtkBorderRepresentation::UpperCenter:
        this->SetPosition((1 - pos2[0]) / 2.0, 0.99 - pos2[1]);
        break;
      default:
        break;
    }
  }
}

//------------------------------------------------------------------------------
void vtkBorderRepresentation::SetWindowLocation(int enumLocation)
{
  if (this->WindowLocation == enumLocation)
  {
    return;
  }

  this->WindowLocation = enumLocation;

  if (this->WindowLocation != vtkBorderRepresentation::AnyLocation)
  {
    this->UpdateWindowLocation();
  }
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkBorderRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Show Vertical Border: ";
  if (this->ShowVerticalBorder == BORDER_OFF)
  {
    os << "Off" << endl;
  }
  else if (this->ShowVerticalBorder == BORDER_ON)
  {
    os << "On" << endl;
  }
  else // if ( this->ShowVerticalBorder == BORDER_ACTIVE)
  {
    os << "Active" << endl;
  }

  os << indent << "Show Horizontal Border: ";
  if (this->ShowHorizontalBorder == BORDER_OFF)
  {
    os << "Off" << endl;
  }
  else if (this->ShowHorizontalBorder == BORDER_ON)
  {
    os << "On" << endl;
  }
  else // if ( this->ShowHorizontalBorder == BORDER_ACTIVE)
  {
    os << "Active" << endl;
  }

  os << indent << "Show Polygon: ";
  if (this->ShowPolygonBackground == BORDER_OFF)
  {
    os << "Off" << endl;
  }
  else if (this->ShowPolygonBackground == BORDER_ON)
  {
    os << "On" << endl;
  }
  else // if ( this->ShowPolygonBackground == BORDER_ACTIVE)
  {
    os << "Active" << endl;
  }

  if (this->BorderProperty)
  {
    os << indent << "Border Property:" << endl;
    this->BorderProperty->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "Border Property: (none)" << endl;
  }

  if (this->PolygonProperty)
  {
    os << indent << "Polygon Property:" << endl;
    this->PolygonProperty->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "Polygon Property: (none)" << endl;
  }

  os << indent << "Enforce Normalized Viewport Bounds: "
     << (this->EnforceNormalizedViewportBounds ? "On\n" : "Off\n");
  os << indent << "Proportional Resize: " << (this->ProportionalResize ? "On" : "Off") << endl;
  os << indent << "Minimum Normalized Viewport Size: " << this->MinimumNormalizedViewportSize[0]
     << " " << this->MinimumNormalizedViewportSize[1] << endl;
  os << indent << "Minimum Size: " << this->MinimumSize[0] << " " << this->MinimumSize[1] << endl;
  os << indent << "Maximum Size: " << this->MaximumSize[0] << " " << this->MaximumSize[1] << endl;

  os << indent << "Moving: " << (this->Moving ? "On" : "Off") << endl;
  os << indent << "Tolerance: " << this->Tolerance << endl;

  os << indent << "Selection Point: (" << this->SelectionPoint[0] << "," << this->SelectionPoint[1]
     << "}" << endl;

  os << indent << "BorderColor: (" << this->BorderColor[0] << ", " << this->BorderColor[1] << ", "
     << this->BorderColor[2] << ")" << endl;
  os << indent << "BorderThickness: " << this->BorderThickness << endl;
  os << indent << "CornerRadiusStrength: " << this->CornerRadiusStrength << endl;
  os << indent << "CornerResolution: " << this->CornerResolution << endl;

  os << indent << "PolygonColor: (" << this->PolygonColor[0] << ", " << this->PolygonColor[1]
     << ", " << this->PolygonColor[2] << ")" << endl;
  os << indent << "PolygonOpacity: " << this->PolygonOpacity << endl;

  os << indent << "Window Location: ";
  switch (this->WindowLocation)
  {
    case vtkBorderRepresentation::LowerLeftCorner:
      os << "LowerLeftCorner\n";
      break;
    case vtkBorderRepresentation::LowerRightCorner:
      os << "LowerRightCorner\n";
      break;
    case vtkBorderRepresentation::LowerCenter:
      os << "LowerCenter\n";
      break;
    case vtkBorderRepresentation::UpperLeftCorner:
      os << "UpperLeftCorner\n";
      break;
    case vtkBorderRepresentation::UpperRightCorner:
      os << "UpperRightCorner\n";
      break;
    case vtkBorderRepresentation::UpperCenter:
      os << "UpperCenter\n";
      break;
    case vtkBorderRepresentation::AnyLocation:
      os << "Any Location\n";
      break;
  }
}
