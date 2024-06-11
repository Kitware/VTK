// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkAxisActor2D.h"

#include "vtkCellArray.h"
#include "vtkMath.h"
#include "vtkMathUtilities.h"
#include "vtkNumberToString.h"
#include "vtkObjectFactory.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper2D.h"
#include "vtkProperty2D.h"
#include "vtkTextMapper.h"
#include "vtkTextProperty.h"
#include "vtkViewport.h"
#include "vtkWindow.h"

#include <algorithm>
#include <cmath>
#include <limits>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkAxisActor2D);

vtkCxxSetObjectMacro(vtkAxisActor2D, LabelTextProperty, vtkTextProperty);
vtkCxxSetObjectMacro(vtkAxisActor2D, TitleTextProperty, vtkTextProperty);

namespace legacy
{
// this is a helper function that computes some useful functions
// for an axis. It returns the number of ticks
static int vtkAxisActor2DComputeTicks(double sRange[2], double& interval, double& root)
{
  // first we try assuming the first value is reasonable
  int numTicks;
  double range = fabs(sRange[1] - sRange[0]);
  int rootPower = static_cast<int>(floor(log10(range) - 1));
  root = pow(10.0, rootPower);
  // val will be between 10 and 100 inclusive of 10 but not 100
  double val = range / root;
  // first we check for an exact match
  for (numTicks = 5; numTicks < 9; ++numTicks)
  {
    if (fabs(val / (numTicks - 1.0) - floor(val / (numTicks - 1.0))) < .0001)
    {
      interval = val * root / (numTicks - 1.0);
      return numTicks;
    }
  }

  // if there isn't an exact match find a reasonable value
  int newIntScale = 10;
  if (val > 10)
  {
    newIntScale = 12;
  }
  if (val > 12)
  {
    newIntScale = 15;
  }
  if (val > 15)
  {
    newIntScale = 18;
  }
  if (val > 18)
  {
    newIntScale = 20;
  }
  if (val > 20)
  {
    newIntScale = 25;
  }
  if (val > 25)
  {
    newIntScale = 30;
  }
  if (val > 30)
  {
    newIntScale = 40;
  }
  if (val > 40)
  {
    newIntScale = 50;
  }
  if (val > 50)
  {
    newIntScale = 60;
  }
  if (val > 60)
  {
    newIntScale = 70;
  }
  if (val > 70)
  {
    newIntScale = 80;
  }
  if (val > 80)
  {
    newIntScale = 90;
  }
  if (val > 90)
  {
    newIntScale = 100;
  }

  // how many ticks should we have
  switch (newIntScale)
  {
    case 12:
    case 20:
    case 40:
    case 80:
      numTicks = 5;
      break;
    case 18:
    case 30:
    case 60:
    case 90:
      numTicks = 7;
      break;
    case 10:
    case 15:
    case 25:
    case 50:
    case 100:
      numTicks = 6;
      break;
    case 70:
      numTicks = 8;
      break;
  }

  interval = newIntScale * root / (numTicks - 1.0);
  return numTicks;
}

void ComputeRange(double inRange[2], double outRange[2], int& numTicks, double& interval)
{
  // Handle the range
  double sRange[2];
  if (inRange[0] < inRange[1])
  {
    sRange[0] = inRange[0];
    sRange[1] = inRange[1];
  }
  else if (inRange[0] > inRange[1])
  {
    sRange[1] = inRange[0];
    sRange[0] = inRange[1];
  }
  else // they're equal, so perturb them by 1 percent
  {
    double perturb = 100.;
    if (inRange[0] == 0.0)
    { // if they are both zero, then just perturb about zero
      sRange[0] = -1 / perturb;
      sRange[1] = 1 / perturb;
    }
    else
    {
      sRange[0] = inRange[0] - inRange[0] / perturb;
      sRange[1] = inRange[0] + inRange[0] / perturb;
    }
  }

  double root;
  numTicks = vtkAxisActor2DComputeTicks(sRange, interval, root);

  // is the starting point reasonable?
  if (fabs(sRange[0] / root - floor(sRange[0] / root)) < 0.01)
  {
    outRange[0] = sRange[0];
    outRange[1] = outRange[0] + (numTicks - 1.0) * interval;
  }
  else
  {
    // OK the starting point is not a good number, so we must widen the range
    // First see if the current range will handle moving the start point
    outRange[0] = floor(sRange[0] / root) * root;
    if (outRange[0] + (numTicks - 1.0) * interval <= sRange[1])
    {
      outRange[1] = outRange[0] + (numTicks - 1.0) * interval;
    }
    else
    {
      // Finally in this case we must switch to a larger range to
      // have reasonable starting and ending values
      sRange[0] = outRange[0];
      numTicks = vtkAxisActor2DComputeTicks(sRange, interval, root);
      outRange[1] = outRange[0] + (numTicks - 1.0) * interval;
    }
  }

  // Adjust if necessary
  if (inRange[0] > inRange[1])
  {
    sRange[0] = outRange[1];
    outRange[1] = outRange[0];
    outRange[0] = sRange[0];
    interval = -interval;
  }
}
}

namespace details
{
constexpr int MAX_FONT_SIZE = 1000;

constexpr std::array<int, 13> ACCEPTABLE_LABELS = { 10, 12, 15, 18, 20, 25, 30, 40, 50, 60, 80, 90,
  100 };

/**
 * Compute an interval that split range depending on targetedNumTicks,
 * while beeing "rounded" for nice display. Actually, we snap the labels
 * to be one of the following, multiply by a power of ten:
 * [10, 12, 15, 18, 20, 25, 30, 40, 50, 60, 80, 90]
 * (see ACCEPTABLE_LABELS)
 *
 * Return the computed number of ticks, that may differ from the target.
 */
int SnapTicksToRoundValues(double range[2], int targetedNumTicks, double& interval)
{
  double delta = std::fabs(range[1] - range[0]);
  double roughInterval = delta / targetedNumTicks;

  // get order of magnitude of the range
  int rootPower = static_cast<int>(std::floor(std::log10(roughInterval) - 1));
  double root = std::pow(10.0, rootPower);

  // roundedInterval will be between 10 and 100 inclusive of 10 but not 100
  // and has 2 significant digits
  int roundedInterval = roughInterval / root;

  auto resulting =
    std::lower_bound(ACCEPTABLE_LABELS.begin(), ACCEPTABLE_LABELS.end(), roundedInterval);
  if (resulting != ACCEPTABLE_LABELS.end())
  {
    roundedInterval = *resulting;
  }

  // scale back rounded interval to actual range
  interval = roundedInterval * root;
  int numTicks = (delta / interval) + 1;

  return numTicks;
}

/**
 * Update Range, so outRange can be split into `numberOfTick`
 * rounded values, including bounds.
 *
 * @see SnapTicksToRoundValues
 */
void AdjustAndSplitRange(double inRange[2], int inNumTicks, double outRange[2], int& numberOfTicks)
{
  // Handle the range
  double sRange[2];
  if (inRange[0] < inRange[1])
  {
    sRange[0] = inRange[0];
    sRange[1] = inRange[1];
  }
  else if (inRange[0] > inRange[1])
  {
    sRange[1] = inRange[0];
    sRange[0] = inRange[1];
  }
  else // they're equal, so perturb them by 1 percent
  {
    double perturb = 100.;
    if (inRange[0] == 0.0)
    { // if they are both zero, then just perturb about zero
      sRange[0] = -1 / perturb;
      sRange[1] = 1 / perturb;
    }
    else
    {
      sRange[0] = inRange[0] - inRange[0] / perturb;
      sRange[1] = inRange[0] + inRange[0] / perturb;
    }
  }

  double interval;
  numberOfTicks = details::SnapTicksToRoundValues(sRange, inNumTicks, interval);

  // round range to start on a multiple of Interval.
  outRange[0] = static_cast<int>(sRange[0] / interval) * interval;
  outRange[1] = outRange[0] + (numberOfTicks - 1) * interval;

  // Adjust if necessary
  if (inRange[0] > inRange[1])
  {
    sRange[0] = outRange[1];
    outRange[1] = outRange[0];
    outRange[0] = sRange[0];
    interval = -interval;
  }
}

};

//------------------------------------------------------------------------------
// Instantiate this object.
vtkAxisActor2D::vtkAxisActor2D()
{
  this->PositionCoordinate->SetCoordinateSystemToNormalizedViewport();
  this->PositionCoordinate->SetValue(0.0, 0.0);

  this->Position2Coordinate->SetCoordinateSystemToNormalizedViewport();
  this->Position2Coordinate->SetValue(0.75, 0.0);
  this->Position2Coordinate->SetReferenceCoordinate(nullptr);

  this->Title = nullptr;
  this->LabelTextProperty = vtkTextProperty::New();
  this->LabelTextProperty->SetBold(1);
  this->LabelTextProperty->SetItalic(1);
  this->LabelTextProperty->SetShadow(1);
  this->LabelTextProperty->SetFontFamilyToArial();

  this->TitleTextProperty = vtkTextProperty::New();
  this->TitleTextProperty->ShallowCopy(this->LabelTextProperty);

  this->LabelFormat = new char[8];
  snprintf(this->LabelFormat, 8, "%s", "%-#6.3g");

  this->TitleMapper = vtkTextMapper::New();
  this->TitleActor = vtkActor2D::New();
  this->TitleActor->SetMapper(this->TitleMapper);

  // To avoid deleting/rebuilding create once up front
  this->LabelMappers = new vtkTextMapper*[VTK_MAX_LABELS];
  this->LabelActors = new vtkActor2D*[VTK_MAX_LABELS];
  for (int i = 0; i < VTK_MAX_LABELS; i++)
  {
    this->LabelMappers[i] = vtkTextMapper::New();
    this->LabelActors[i] = vtkActor2D::New();
    this->LabelActors[i]->SetMapper(this->LabelMappers[i]);
  }

  this->AxisMapper->SetInputData(this->Axis);
  this->AxisActor->SetMapper(this->AxisMapper);
}

//------------------------------------------------------------------------------
vtkAxisActor2D::~vtkAxisActor2D()
{
  delete[] this->LabelFormat;
  this->LabelFormat = nullptr;

  this->TitleMapper->Delete();
  this->TitleActor->Delete();

  delete[] this->Title;
  this->Title = nullptr;

  if (this->LabelMappers != nullptr)
  {
    for (int i = 0; i < VTK_MAX_LABELS; i++)
    {
      this->LabelMappers[i]->Delete();
      this->LabelActors[i]->Delete();
    }
    delete[] this->LabelMappers;
    delete[] this->LabelActors;
  }

  this->SetLabelTextProperty(nullptr);
  this->SetTitleTextProperty(nullptr);
}

//------------------------------------------------------------------------------
int vtkAxisActor2D::UpdateGeometryAndRenderOpaqueGeometry(vtkViewport* viewport, bool render)
{
  this->BuildAxis(viewport);
  if (render)
  {
    return this->RenderOpaqueGeometry(viewport);
  }
  return 0;
}

//------------------------------------------------------------------------------
// Build the axis, ticks, title, and labels and render.

int vtkAxisActor2D::RenderOpaqueGeometry(vtkViewport* viewport)
{
  int i, renderedSomething = 0;

  this->BuildAxis(viewport);

  // Everything is built, just have to render
  if (this->Title != nullptr && this->Title[0] != 0 && this->TitleVisibility)
  {
    renderedSomething += this->TitleActor->RenderOpaqueGeometry(viewport);
  }

  if (this->AxisVisibility || this->TickVisibility)
  {
    renderedSomething += this->AxisActor->RenderOpaqueGeometry(viewport);
  }

  if (this->LabelVisibility)
  {
    for (i = 0; i < this->NumberOfLabelsBuilt; i++)
    {
      renderedSomething += this->LabelActors[i]->RenderOpaqueGeometry(viewport);
    }
  }

  return renderedSomething;
}

//------------------------------------------------------------------------------
// Render the axis, ticks, title, and labels.

int vtkAxisActor2D::RenderOverlay(vtkViewport* viewport)
{
  int i, renderedSomething = 0;

  this->BuildAxis(viewport);

  // Everything is built, just have to render
  if (this->Title != nullptr && this->Title[0] != 0 && this->TitleVisibility)
  {
    renderedSomething += this->TitleActor->RenderOverlay(viewport);
  }

  if (this->AxisVisibility || this->TickVisibility)
  {
    renderedSomething += this->AxisActor->RenderOverlay(viewport);
  }

  if (this->LabelVisibility)
  {
    for (i = 0; i < this->NumberOfLabelsBuilt; i++)
    {
      renderedSomething += this->LabelActors[i]->RenderOverlay(viewport);
    }
  }

  return renderedSomething;
}

//------------------------------------------------------------------------------
// Description:
// Does this prop have some translucent polygonal geometry?
vtkTypeBool vtkAxisActor2D::HasTranslucentPolygonalGeometry()
{
  return 0;
}

//------------------------------------------------------------------------------
// Release any graphics resources that are being consumed by this actor.
// The parameter window could be used to determine which graphic
// resources to release.
void vtkAxisActor2D::ReleaseGraphicsResources(vtkWindow* win)
{
  this->TitleActor->ReleaseGraphicsResources(win);
  for (int i = 0; i < VTK_MAX_LABELS; i++)
  {
    this->LabelActors[i]->ReleaseGraphicsResources(win);
  }
  this->AxisActor->ReleaseGraphicsResources(win);
}

//------------------------------------------------------------------------------
void vtkAxisActor2D::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  if (this->TitleTextProperty)
  {
    os << indent << "Title Text Property:\n";
    this->TitleTextProperty->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "Title Text Property: (none)\n";
  }

  if (this->LabelTextProperty)
  {
    os << indent << "Label Text Property:\n";
    this->LabelTextProperty->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "Label Text Property: (none)\n";
  }

  os << indent << "Title: " << (this->Title ? this->Title : "(none)") << "\n";
  os << indent << "Ruler Mode: " << (this->RulerMode ? "On" : "Off") << "\n";
  os << indent << "Ruler Distance: " << this->GetRulerDistance() << "\n";
  os << indent << "Number Of Labels: " << this->NumberOfLabels << "\n";
  os << indent << "Number Of Labels Built: " << this->NumberOfLabelsBuilt << "\n";
  os << indent << "Range: (" << this->Range[0] << ", " << this->Range[1] << ")\n";

  os << indent << "Label value notation: " << this->GetNotation() << "\n";
  os << indent << "Label value precision: " << this->GetPrecision() << "\n";
  os << indent << "Label Format: " << this->LabelFormat << "\n";
  os << indent << "Font Factor: " << this->FontFactor << "\n";
  os << indent << "Label Factor: " << this->LabelFactor << "\n";
  os << indent << "Tick Length: " << this->TickLength << "\n";
  os << indent << "Tick Offset: " << this->TickOffset << "\n";

  os << indent << "Adjust Labels: " << (this->AdjustLabels ? "On\n" : "Off\n");
  os << indent << "Snap Labels To Grid: " << (this->SnapLabelsToGrid ? "On\n" : "Off\n");

  os << indent << "Axis Visibility: " << (this->AxisVisibility ? "On\n" : "Off\n");
  os << indent << "Tick Visibility: " << (this->TickVisibility ? "On\n" : "Off\n");
  os << indent << "Label Visibility: " << (this->LabelVisibility ? "On\n" : "Off\n");
  os << indent << "Title Visibility: " << (this->TitleVisibility ? "On\n" : "Off\n");

  os << indent << "MinorTickLength: " << this->MinorTickLength << endl;
  os << indent << "NumberOfMinorTicks: " << this->NumberOfMinorTicks << endl;
  os << indent << "TitlePosition: " << this->TitlePosition << endl;

  os << indent
     << "Size Font Relative To Axis: " << (this->SizeFontRelativeToAxis ? "On\n" : "Off\n");
}

//------------------------------------------------------------------------------
bool vtkAxisActor2D::PositionsChangedOrViewportResized(vtkViewport* viewport)
{
  // Check to see whether we have to rebuild everything
  // Viewport change may not require rebuild
  int* currentPosition = this->PositionCoordinate->GetComputedViewportValue(viewport);
  int* currentPosition2 = this->Position2Coordinate->GetComputedViewportValue(viewport);
  bool positionsHaveChanged = (currentPosition[0] != this->LastPosition[0] ||
    currentPosition[1] != this->LastPosition[1] || currentPosition2[0] != this->LastPosition2[0] ||
    currentPosition2[1] != this->LastPosition2[1]);

  // See whether fonts have to be rebuilt (font size depends on viewport size)
  int* size = viewport->GetSize();
  bool viewportSizeHasChanged = (this->LastSize[0] != size[0] || this->LastSize[1] != size[1]);

  return positionsHaveChanged || viewportSizeHasChanged;
}

//------------------------------------------------------------------------------
bool vtkAxisActor2D::ShouldRebuild(vtkViewport* viewport)
{
  if (this->TitleVisibility && !this->TitleTextProperty)
  {
    vtkErrorMacro(<< "Need title text property to render axis actor");
    return false;
  }

  if (this->LabelVisibility && !this->LabelTextProperty)
  {
    vtkErrorMacro(<< "Need label text property to render axis actor");
    return false;
  }

  if (!viewport->GetVTKWindow())
  {
    return false;
  }

  bool recentBuild = viewport->GetMTime() < this->BuildTime &&
    viewport->GetVTKWindow()->GetMTime() < this->BuildTime && this->GetMTime() < this->BuildTime &&
    (!this->LabelVisibility || this->LabelTextProperty->GetMTime() < this->BuildTime) &&
    (!this->TitleVisibility || this->TitleTextProperty->GetMTime() < this->BuildTime);

  if (!this->PositionsChangedOrViewportResized(viewport) && recentBuild)
  {
    return false;
  }

  return true;
}

//------------------------------------------------------------------------------
double vtkAxisActor2D::GetViewportAxisLength(vtkViewport* viewport)
{
  int* x = this->PositionCoordinate->GetComputedViewportValue(viewport);
  double p1[3];
  p1[0] = x[0];
  p1[1] = x[1];
  p1[2] = 0.0;

  x = this->Position2Coordinate->GetComputedViewportValue(viewport);
  double p2[3];
  p2[0] = x[0];
  p2[1] = x[1];
  p2[2] = 0.0;

  double axis[3];
  vtkMath::Subtract(p2, p1, axis);
  return vtkMath::Norm(axis);
}

//------------------------------------------------------------------------------
double vtkAxisActor2D::GetViewportRulerDistance(vtkViewport* viewport)
{
  double wp1[3], wp2[3], wp21[3];
  this->PositionCoordinate->GetValue(wp1);
  this->Position2Coordinate->GetValue(wp2);
  vtkMath::Subtract(wp2, wp1, wp21);

  const double worldLength = vtkMath::Norm(wp21);
  // Tick distance was computed in world coordinates, convert to viewport
  // coordinates.
  double length = this->GetViewportAxisLength(viewport);
  const double worldToLocalRatio = (worldLength <= 0.0 ? 0.0 : length / worldLength);
  return this->RulerDistance * worldToLocalRatio;
}

//------------------------------------------------------------------------------
double vtkAxisActor2D::GetAxisAngle(vtkViewport* viewport)
{
  int* p1 = this->PositionCoordinate->GetComputedViewportValue(viewport);
  int* p2 = this->Position2Coordinate->GetComputedViewportValue(viewport);

  double deltaX = p2[0] - p1[0];
  double deltaY = p2[1] - p1[1];

  return (deltaX == 0. && deltaY == 0.) ? 0. : std::atan2(deltaY, deltaX);
}

//------------------------------------------------------------------------------
void vtkAxisActor2D::UpdateTicksValueAndPosition(vtkViewport* viewport)
{
  // viewport distances
  const double viewportAxisLength = this->GetViewportAxisLength(viewport);
  const double viewportRulerDistance = this->GetViewportRulerDistance(viewport);

  // normalized on axis size.
  const double majorLengthRatio = this->RulerMode ? viewportRulerDistance / viewportAxisLength
                                                  : 1. / (this->AdjustedNumberOfLabels - 1);
  const double minorLengthRatio = majorLengthRatio / (this->NumberOfMinorTicks + 1);

  // values (in `Range` unit)
  const double majorDelta = (this->AdjustedRange[1] - this->AdjustedRange[0]) * majorLengthRatio;
  const double minorDelta = (this->AdjustedRange[1] - this->AdjustedRange[0]) * minorLengthRatio;

  // factor for Range to Axis normalized value conversion.
  const double scale = 1. / (this->Range[1] - this->Range[0]);

  this->TickValues.clear();
  this->NormalizedTickPositions.clear();

  const double minValue = std::min(this->Range[0], this->Range[1]);
  const double maxValue = std::max(this->Range[0], this->Range[1]);

  int startingTick = this->SkipFirstTick ? 1 : 0;
  for (int major = startingTick; major < this->AdjustedNumberOfLabels; major++)
  {
    double value = this->AdjustedRange[0] + major * majorDelta;
    double position = (value - this->Range[0]) * scale;

    if (position < 0 || value < minValue || position > 1 || value > maxValue)
    {
      continue;
    }

    this->NormalizedTickPositions.push_back(position);
    this->TickValues.push_back(value);

    for (int minor = 1; minor <= this->NumberOfMinorTicks; minor++)
    {
      value = value + minor * minorDelta;
      position = (value - this->Range[0]) * scale;
      if (position > 1)
      {
        continue;
      }
      this->NormalizedTickPositions.push_back(position);
    }
  }

  this->NumberOfLabelsBuilt = static_cast<int>(this->TickValues.size());
}

//------------------------------------------------------------------------------
void vtkAxisActor2D::BuildTicksPolyData(vtkViewport* viewport)
{
  this->Axis->Initialize();

  vtkNew<vtkPoints> pts;
  vtkNew<vtkCellArray> lines;
  this->Axis->SetPoints(pts);
  this->Axis->SetLines(lines);

  // Generate the axis and tick marks.
  // We'll do our computation in viewport coordinates. First determine the
  // location of the endpoints.
  int* x = this->PositionCoordinate->GetComputedViewportValue(viewport);
  double axisStart[3];
  axisStart[0] = x[0];
  axisStart[1] = x[1];
  axisStart[2] = 0.0;

  x = this->Position2Coordinate->GetComputedViewportValue(viewport);
  double axisEnd[3];
  axisEnd[0] = x[0];
  axisEnd[1] = x[1];
  axisEnd[2] = 0.0;

  // axis extremity
  vtkIdType axisPoints[2];
  axisPoints[0] = pts->InsertNextPoint(axisStart);
  // Generate point along axis (as well as tick points)
  double theta = this->GetAxisAngle(viewport);

  double normalizedAxis[3];
  vtkMath::Subtract(axisEnd, axisStart, normalizedAxis);
  double axisLength = vtkMath::Normalize(normalizedAxis);

  int totalNumberOfTicks = static_cast<int>(this->NormalizedTickPositions.size());

  this->TicksStartPos->SetNumberOfPoints(totalNumberOfTicks);

  for (int tick = 0; tick < totalNumberOfTicks; tick++)
  {
    int tickLength = 0;
    if (tick % (this->NumberOfMinorTicks + 1) == 0)
    {
      tickLength = this->TickLength;
    }
    else
    {
      tickLength = this->MinorTickLength;
    }

    double tickPos[3];
    tickPos[2] = 0;
    tickPos[0] =
      axisStart[0] + this->NormalizedTickPositions[tick] * normalizedAxis[0] * axisLength;
    tickPos[1] =
      axisStart[1] + this->NormalizedTickPositions[tick] * normalizedAxis[1] * axisLength;
    this->TicksStartPos->SetPoint(tick, tickPos);

    vtkIdType tickPoints[2];
    tickPoints[0] = pts->InsertNextPoint(tickPos);

    tickPos[0] = tickPos[0] + tickLength * std::sin(theta);
    tickPos[1] = tickPos[1] - tickLength * std::cos(theta);
    tickPoints[1] = pts->InsertNextPoint(tickPos);

    if (this->TickVisibility)
    {
      lines->InsertNextCell(2, tickPoints);
    }
  }

  // last point
  axisPoints[1] = pts->InsertNextPoint(axisEnd);

  // Add the axis if requested
  if (this->AxisVisibility)
  {
    lines->InsertNextCell(2, axisPoints);
  }
}

//------------------------------------------------------------------------------
void vtkAxisActor2D::BuildLabels(vtkViewport* viewport)
{
  // Update the labels text. Do it only if the range has been adjusted,
  // i.e. if we think that new labels must be created.
  // WARNING: if LabelFormat has changed, they should be recreated too
  // but at this point the check on LabelFormat is "included" in
  // UpdateAdjustedRange(), which is the function that update
  // AdjustedRangeBuildTime or not.
  vtkMTimeType labeltime = this->AdjustedRangeBuildTime;
  int nbOfLabels = static_cast<int>(this->TickValues.size());
  if (this->NumberOfLabelsBuilt != nbOfLabels)
  {
    vtkErrorMacro("Inconsistent number of labels. Got " << nbOfLabels << " values but expects "
                                                        << this->NumberOfLabelsBuilt);
  }

  if (nbOfLabels < 1)
  {
    return;
  }

  if (this->AdjustedRangeBuildTime > this->BuildTime)
  {
    for (int i = 0; i < nbOfLabels; i++)
    {
      double val = this->TickValues[i];
      if (this->GetNotation() == 0)
      {
        // Use default legend notation : don't use vtkNumberToString
        // for the default setting in order to ensure retrocompatibility
        char string[512];
        snprintf(string, sizeof(string), this->LabelFormat, val);
        this->LabelMappers[i]->SetInput(string);
      }
      else
      {
        vtkNumberToString converter;
        converter.SetNotation(this->GetNotation());
        converter.SetPrecision(this->GetPrecision());
        std::string formattedString = converter.Convert(val);
        this->LabelMappers[i]->SetInput(formattedString.c_str());
      }
    }

    // Check if the label text has changed
    if (this->LabelMappers[this->NumberOfLabelsBuilt - 1]->GetMTime() > labeltime)
    {
      labeltime = this->LabelMappers[this->NumberOfLabelsBuilt - 1]->GetMTime();
    }
  }

  // Copy prop and text prop eventually
  for (int i = 0; i < this->NumberOfLabelsBuilt; i++)
  {
    if (this->LabelTextProperty->GetMTime() > this->BuildTime ||
      this->AdjustedRangeBuildTime > this->BuildTime)
    {
      // Shallow copy here so that the size of the label prop is not
      // affected by the automatic adjustment of its text mapper's
      // size (i.e. its mapper's text property is identical except
      // for the font size which will be modified later). This
      // allows text actors to share the same text property, and in
      // that case specifically allows the title and label text prop
      // to be the same.
      this->LabelMappers[i]->GetTextProperty()->ShallowCopy(this->LabelTextProperty);
    }
  }

  int* size = viewport->GetSize();

  double *xp1, *xp2, len = 0.0;
  if (this->SizeFontRelativeToAxis)
  {
    xp1 = this->PositionCoordinate->GetComputedDoubleViewportValue(viewport);
    xp2 = this->Position2Coordinate->GetComputedDoubleViewportValue(viewport);
    len = std::sqrt((xp2[0] - xp1[0]) * (xp2[0] - xp1[0]) + (xp2[1] - xp1[1]) * (xp2[1] - xp1[1]));
  }

  // Resize the mappers if needed (i.e. viewport has changed, than
  // font size should be changed, or label text property has changed,
  // or some of the labels have changed (got bigger for example)

  if (this->PositionsChangedOrViewportResized(viewport) ||
    this->LabelTextProperty->GetMTime() > this->BuildTime || labeltime > this->BuildTime)
  {
    if (!this->UseFontSizeFromProperty)
    {
      if (!this->SizeFontRelativeToAxis)
      {
        vtkTextMapper::SetMultipleRelativeFontSize(viewport, this->LabelMappers,
          this->NumberOfLabelsBuilt, size, this->LastMaxLabelSize,
          0.015 * this->FontFactor * this->LabelFactor);
      }
      else
      {
        int minFontSize = details::MAX_FONT_SIZE;
        int fontSize;
        int minLabel = 0;
        for (int i = 0; i < this->NumberOfLabelsBuilt; i++)
        {
          fontSize = this->LabelMappers[i]->SetConstrainedFontSize(viewport,
            static_cast<int>((1.0 / this->NumberOfLabelsBuilt) * len), static_cast<int>(0.2 * len));
          if (fontSize < minFontSize)
          {
            minFontSize = fontSize;
            minLabel = i;
          }
        }
        for (int i = 0; i < this->NumberOfLabelsBuilt; i++)
        {
          this->LabelMappers[i]->GetTextProperty()->SetFontSize(minFontSize);
        }
        this->LabelMappers[minLabel]->GetSize(viewport, this->LastMaxLabelSize);
      }
    }
    else
    {
      this->LabelMappers[0]->GetSize(viewport, this->LastMaxLabelSize);
    }
  }

  vtkPoints* pts = this->Axis->GetPoints();
  // Position the mappers
  for (int i = 0; i < this->NumberOfLabelsBuilt; i++)
  {
    double xTick[3];
    vtkIdType tickId = (this->NumberOfMinorTicks + 1) * i;
    // first point in the list is the Axis start, not a tick point.
    vtkIdType startPointId = tickId * 2 + 1;
    vtkIdType endPointId = startPointId + 1;
    pts->GetPoint(endPointId, xTick);
    double theta = this->GetAxisAngle(viewport);
    double textAngle = this->LabelTextProperty->GetOrientation();
    textAngle = vtkMath::RadiansFromDegrees(textAngle);

    vtkAxisActor2D::SetOffsetPosition(xTick, theta, this->LastMaxLabelSize[0],
      this->LastMaxLabelSize[1], this->TickOffset, this->LabelActors[i]);
  }
}

//------------------------------------------------------------------------------
void vtkAxisActor2D::SetTitleFontSize(vtkViewport* viewport, int stringSize[2])
{
  // the mapper returns the global bounding box. Artificially set orientation to 0
  // in this scope to get the local bounding box in `stringSize`
  double originalAngle = this->TitleMapper->GetTextProperty()->GetOrientation();
  this->TitleMapper->GetTextProperty()->SetOrientation(0);

  if (this->PositionsChangedOrViewportResized(viewport) ||
    this->TitleTextProperty->GetMTime() > this->BuildTime)
  {
    if (!this->UseFontSizeFromProperty)
    {
      if (!this->SizeFontRelativeToAxis)
      {
        int* size = viewport->GetSize();
        vtkTextMapper::SetRelativeFontSize(
          this->TitleMapper, viewport, size, stringSize, 0.015 * this->FontFactor);
      }
      else
      {

        double *xp1, *xp2, len = 0.0;
        if (this->SizeFontRelativeToAxis)
        {
          xp1 = this->PositionCoordinate->GetComputedDoubleViewportValue(viewport);
          xp2 = this->Position2Coordinate->GetComputedDoubleViewportValue(viewport);
          len = std::sqrt(
            (xp2[0] - xp1[0]) * (xp2[0] - xp1[0]) + (xp2[1] - xp1[1]) * (xp2[1] - xp1[1]));
        }

        this->TitleMapper->SetConstrainedFontSize(
          viewport, static_cast<int>(0.33 * len), static_cast<int>(0.2 * len));
        this->TitleMapper->GetSize(viewport, stringSize);
      }
    }
    else
    {
      this->TitleMapper->GetSize(viewport, stringSize);
    }
  }
  else
  {
    this->TitleMapper->GetSize(viewport, stringSize);
  }

  // Restore the orientation.
  this->TitleMapper->GetTextProperty()->SetOrientation(originalAngle);
}

//------------------------------------------------------------------------------
void vtkAxisActor2D::BuildTitle(vtkViewport* viewport)
{
  this->TitleMapper->SetInput(this->Title);

  if (this->TitleTextProperty->GetMTime() > this->BuildTime)
  {
    // Shallow copy here so that the size of the title prop is not
    // affected by the automatic adjustment of its text mapper's
    // size (i.e. its mapper's text property is identical except for
    // the font size which will be modified later). This allows text
    // actors to share the same text property, and in that case
    // specifically allows the title and label text prop to be the same.
    this->TitleMapper->GetTextProperty()->ShallowCopy(this->TitleTextProperty);
  }

  int stringSize[2];
  this->SetTitleFontSize(viewport, stringSize);

  int* x1 = this->PositionCoordinate->GetComputedViewportValue(viewport);
  int* x2 = this->Position2Coordinate->GetComputedViewportValue(viewport);
  double tickPosition[3];
  tickPosition[0] = x1[0] + (x2[0] - x1[0]) * this->TitlePosition;
  tickPosition[1] = x1[1] + (x2[1] - x1[1]) * this->TitlePosition;

  double textOrientation = this->TitleTextProperty->GetOrientation();
  double theta = vtkMath::RadiansFromDegrees(textOrientation);

  double offset = this->TickLength + this->TickOffset;
  if (this->LabelVisibility)
  {
    offset += vtkAxisActor2D::ComputeStringOffset(
      this->LastMaxLabelSize[0], this->LastMaxLabelSize[1], theta);
  }

  int textPos[2];
  this->ShiftPosition(
    tickPosition, theta, stringSize[0], stringSize[1], static_cast<int>(offset), textPos);

  this->TitleActor->SetPosition(textPos[0], textPos[1]);
}

//------------------------------------------------------------------------------
void vtkAxisActor2D::UpdateCachedInformations(vtkViewport* viewport)
{
  int* x = this->PositionCoordinate->GetComputedViewportValue(viewport);
  this->LastPosition[0] = x[0];
  this->LastPosition[1] = x[1];
  x = this->Position2Coordinate->GetComputedViewportValue(viewport);
  this->LastPosition2[0] = x[0];
  this->LastPosition2[1] = x[1];

  int* size = viewport->GetSize();
  this->LastSize[0] = size[0];
  this->LastSize[1] = size[1];
}

//------------------------------------------------------------------------------
void vtkAxisActor2D::BuildAxis(vtkViewport* viewport)
{
  if (!this->ShouldRebuild(viewport))
  {
    return;
  }

  vtkDebugMacro(<< "Rebuilding axis");

  this->AxisActor->SetProperty(this->GetProperty());

  this->UpdateAdjustedRange();

  this->UpdateTicksValueAndPosition(viewport);

  this->BuildTicksPolyData(viewport);

  if (this->LabelVisibility)
  {
    this->BuildLabels(viewport);
  }

  if (this->Title != nullptr && this->Title[0] != 0 && this->TitleVisibility)
  {
    this->BuildTitle(viewport);
  }

  this->UpdateCachedInformations(viewport);

  this->BuildTime.Modified();
}

//------------------------------------------------------------------------------
vtkPoints* vtkAxisActor2D::GetTickPositions()
{
  return this->TicksStartPos;
}

//------------------------------------------------------------------------------
void vtkAxisActor2D::UpdateAdjustedRange()
{
  // Try not to update/adjust the range to often, do not update it
  // if the object has not been modified.
  // Nevertheless, try the following optimization: there is no need to
  // update the range if the position coordinate of this actor have
  // changed. But since vtkActor2D::GetMTime() includes the check for
  // both Position and Position2 coordinates, we will have to bypass
  // it.

  // NOLINTNEXTLINE(bugprone-parent-virtual-call)
  if (this->vtkActor2D::Superclass::GetMTime() <= this->AdjustedRangeBuildTime)
  {
    return;
  }

  if (this->SnapLabelsToGrid)
  {
    details::AdjustAndSplitRange(
      this->Range, this->NumberOfLabels, this->AdjustedRange, this->AdjustedNumberOfLabels);
  }
  else if (this->AdjustLabels)
  {
    double interval;
    vtkAxisActor2D::ComputeRange(this->Range, this->AdjustedRange, this->NumberOfLabels,
      this->AdjustedNumberOfLabels, interval);
  }
  else
  {
    this->AdjustedNumberOfLabels = this->NumberOfLabels;
    this->AdjustedRange[0] = this->Range[0];
    this->AdjustedRange[1] = this->Range[1];
  }

  if (this->RulerMode)
  {
    double wp1[3], wp2[3], wp21[3];
    this->PositionCoordinate->GetValue(wp1);
    this->Position2Coordinate->GetValue(wp2);
    vtkMath::Subtract(wp2, wp1, wp21);
    const double worldLength = vtkMath::Norm(wp21);
    this->AdjustedNumberOfLabels = worldLength / this->RulerDistance;
    if (vtkMathUtilities::FuzzyCompare<double>(
          this->AdjustedNumberOfLabels * this->RulerDistance, worldLength))
    {
      this->AdjustedNumberOfLabels += 1;
    }
    this->AdjustedNumberOfLabels += 2;
  }

  if (this->AdjustedNumberOfLabels < 1)
  {
    vtkWarningMacro(
      "Axis expects to have at least 1 label. Will use 1 instead of the computed number "
      << this->AdjustedNumberOfLabels);
    this->AdjustedNumberOfLabels = 1;
  }

  this->AdjustedNumberOfLabels = std::min<int>(this->AdjustedNumberOfLabels, VTK_MAX_LABELS);

  this->AdjustedRangeBuildTime.Modified();
}

//------------------------------------------------------------------------------
void vtkAxisActor2D::ComputeRange(double inRange[2], double outRange[2], int vtkNotUsed(inNumTicks),
  int& numTicks, double& interval)
{
  legacy::ComputeRange(inRange, outRange, numTicks, interval);
}

//------------------------------------------------------------------------------
// Position text with respect to a point (xTick) where the angle of the line
// from the point to the center of the text is given by theta. The offset
// is the spacing between ticks and labels.
void vtkAxisActor2D::SetOffsetPosition(
  double xTick[3], double theta, int stringWidth, int stringHeight, int offset, vtkActor2D* actor)
{
  double x, y, center[2];
  int pos[2];

  x = stringWidth / 2.0 + offset;
  y = stringHeight / 2.0 + offset;

  center[0] = xTick[0] + x * std::sin(theta);
  center[1] = xTick[1] - y * std::cos(theta);

  pos[0] = static_cast<int>(center[0] - stringWidth / 2.0);
  pos[1] = static_cast<int>(center[1] - stringHeight / 2.0);

  actor->SetPosition(pos[0], pos[1]);
}

//------------------------------------------------------------------------------
void vtkAxisActor2D::ShiftPosition(
  double xTick[3], double textAngle, int stringWidth, int stringHeight, int offset, int finalPos[2])
{
  // Text Horizontal: center text
  finalPos[0] = xTick[0] - stringWidth / 2 * std::cos(textAngle);
  finalPos[1] = xTick[1] - stringWidth / 2 * std::sin(textAngle);

  // Text Vertical: put text "under" axes
  finalPos[0] += stringHeight * std::sin(textAngle);
  finalPos[1] -= stringHeight * std::cos(textAngle);

  // Axis Vertical: add extra offset
  finalPos[0] += offset * std::sin(textAngle);
  finalPos[1] -= offset * std::cos(textAngle);
}

//------------------------------------------------------------------------------
double vtkAxisActor2D::ComputeStringOffset(double width, double height, double theta)
{
  double f1 = height * std::cos(theta);
  double f2 = width * std::sin(theta);
  return (1.2 * std::sqrt(f1 * f1 + f2 * f2));
}

//------------------------------------------------------------------------------
void vtkAxisActor2D::ShallowCopy(vtkProp* prop)
{
  vtkAxisActor2D* a = vtkAxisActor2D::SafeDownCast(prop);
  if (a != nullptr)
  {
    this->SetRange(a->GetRange());
    this->SetNumberOfLabels(a->GetNumberOfLabels());
    this->SetLabelFormat(a->GetLabelFormat());
    this->SetAdjustLabels(a->GetAdjustLabels());
    this->SetSnapLabelsToGrid(a->GetSnapLabelsToGrid());
    this->SetTitle(a->GetTitle());
    this->SetTickLength(a->GetTickLength());
    this->SetTickOffset(a->GetTickOffset());
    this->SetAxisVisibility(a->GetAxisVisibility());
    this->SetTickVisibility(a->GetTickVisibility());
    this->SetLabelVisibility(a->GetLabelVisibility());
    this->SetTitleVisibility(a->GetTitleVisibility());
    this->SetFontFactor(a->GetFontFactor());
    this->SetLabelFactor(a->GetLabelFactor());
    this->SetLabelTextProperty(a->GetLabelTextProperty());
    this->SetTitleTextProperty(a->GetTitleTextProperty());
  }

  // Now do superclass
  this->vtkActor2D::ShallowCopy(prop);
}
VTK_ABI_NAMESPACE_END
