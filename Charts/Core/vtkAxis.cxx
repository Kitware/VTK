/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAxis.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkAxis.h"

#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkContext2D.h"
#include "vtkContextScene.h"
#include "vtkPen.h"
#include "vtkChart.h"
#include "vtkTextProperty.h"
#include "vtkVector.h"
#include "vtkFloatArray.h"
#include "vtkDoubleArray.h"
#include "vtkStringArray.h"
#include "vtkStdString.h"
#include "vtkAxisExtended.h"

#include <sstream>
#include "vtkObjectFactory.h"

#include "vtksys/RegularExpression.hxx"

#include <algorithm>
#include <cstdio>
#include <limits>
#include <cmath>

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkAxis);

//-----------------------------------------------------------------------------
vtkAxis::vtkAxis()
{
  this->Position = -1;
  this->Point1 = this->Position1.GetData();
  this->Point2 = this->Position2.GetData();
  this->Position1.Set(0.0, 10.0);
  this->Position2.Set(0.0, 10.0);
  this->TickInterval = 1.0;
  this->NumberOfTicks = -1;
  this->LabelProperties = vtkTextProperty::New();
  this->LabelProperties->SetColor(0.0, 0.0, 0.0);
  this->LabelProperties->SetFontSize(12);
  this->LabelProperties->SetFontFamilyToArial();
  this->LabelProperties->SetJustificationToCentered();
  this->TitleProperties = vtkTextProperty::New();
  this->TitleProperties->SetColor(0.0, 0.0, 0.0);
  this->TitleProperties->SetFontSize(12);
  this->TitleProperties->SetFontFamilyToArial();
  this->TitleProperties->SetBold(1);
  this->TitleProperties->SetJustificationToCentered();
  this->Minimum = 0.0;
  this->Maximum = 6.66;
  this->UnscaledMinimum = this->Minimum;
  this->UnscaledMaximum = this->Maximum;
  this->MinimumLimit = std::numeric_limits<double>::max() * -1.;
  this->MaximumLimit = std::numeric_limits<double>::max();
  this->UnscaledMinimumLimit = std::numeric_limits<double>::max() * -1.;
  this->UnscaledMaximumLimit = std::numeric_limits<double>::max();
  this->NonLogUnscaledMinLimit = this->UnscaledMinimumLimit;
  this->NonLogUnscaledMaxLimit = this->UnscaledMaximumLimit;
  this->Margins[0] = 15;
  this->Margins[1] = 5;
  this->LogScale = false;
  this->LogScaleActive = false;
  this->GridVisible = true;
  this->LabelsVisible = true;
  this->RangeLabelsVisible = false;
  this->LabelOffset = 7;
  this->TicksVisible = true;
  this->AxisVisible = true;
  this->Precision = 2;
  this->LabelFormat = "%g";
  this->RangeLabelFormat = "%g";
  this->Notation = vtkAxis::STANDARD_NOTATION;
  this->Behavior = vtkAxis::AUTO;
  this->Pen = vtkPen::New();
  this->TitleAppended = false;
  this->ScalingFactor = 1.0;
  this->Shift = 0.0;

  this->Pen->SetColor(0, 0, 0);
  this->Pen->SetWidth(1.0);
  this->GridPen = vtkPen::New();
  this->GridPen->SetColor(242, 242, 242);
  this->GridPen->SetWidth(1.0);
  this->TickPositions = vtkSmartPointer<vtkDoubleArray>::New();
  this->TickScenePositions = vtkSmartPointer<vtkFloatArray>::New();
  this->TickLabels = vtkSmartPointer<vtkStringArray>::New();
  this->UsingNiceMinMax = false;
  this->TickMarksDirty = true;
  this->MaxLabel[0] = this->MaxLabel[1] = 0.0;
  this->Resized = true;
  this->SetPosition(vtkAxis::LEFT);
  this->TickLabelAlgorithm = vtkAxis::TICK_SIMPLE;
  this->CustomTickLabels = false;
}

//-----------------------------------------------------------------------------
vtkAxis::~vtkAxis()
{
  this->TitleProperties->Delete();
  this->LabelProperties->Delete();
  this->Pen->Delete();
  this->GridPen->Delete();
}

void vtkAxis::SetPosition(int position)
{
  if (this->Position != position)
  {
    this->Position = position;
    // Draw the axis label
    switch (this->Position)
    {
      case vtkAxis::LEFT:
        this->TitleProperties->SetOrientation(90.0);
        this->TitleProperties->SetVerticalJustificationToBottom();
        this->LabelProperties->SetJustificationToRight();
        this->LabelProperties->SetVerticalJustificationToCentered();
        break;
      case vtkAxis::RIGHT:
        this->TitleProperties->SetOrientation(90.0);
        this->TitleProperties->SetVerticalJustificationToTop();
        this->LabelProperties->SetJustificationToLeft();
        this->LabelProperties->SetVerticalJustificationToCentered();
        break;
      case vtkAxis::BOTTOM:
        this->TitleProperties->SetOrientation(0.0);
        this->TitleProperties->SetVerticalJustificationToTop();
        this->LabelProperties->SetJustificationToCentered();
        this->LabelProperties->SetVerticalJustificationToTop();
        break;
      case vtkAxis::TOP:
        this->TitleProperties->SetOrientation(0.0);
        this->TitleProperties->SetVerticalJustificationToBottom();
        this->LabelProperties->SetJustificationToCentered();
        this->LabelProperties->SetVerticalJustificationToBottom();
        break;
      case vtkAxis::PARALLEL:
        this->TitleProperties->SetOrientation(0.0);
        this->TitleProperties->SetVerticalJustificationToTop();
        this->LabelProperties->SetJustificationToRight();
        this->LabelProperties->SetVerticalJustificationToCentered();
        break;
    }
  }
}

//-----------------------------------------------------------------------------
void vtkAxis::SetPoint1(const vtkVector2f &pos)
{
  if (this->Position1 != pos)
  {
    this->Position1 = pos;
    this->Resized = true;
    this->Modified();
  }
}

//-----------------------------------------------------------------------------
void vtkAxis::SetPoint1(float x, float y)
{
  this->SetPoint1(vtkVector2f(x, y));
}

//-----------------------------------------------------------------------------
vtkVector2f vtkAxis::GetPosition1()
{
  return this->Position1;
}

//-----------------------------------------------------------------------------
void vtkAxis::SetPoint2(const vtkVector2f &pos)
{
  if (this->Position2 != pos)
  {
    this->Position2 = pos;
    this->Resized = true;
    this->Modified();
  }
}

//-----------------------------------------------------------------------------
void vtkAxis::SetPoint2(float x, float y)
{
  this->SetPoint2(vtkVector2f(x, y));
}

//-----------------------------------------------------------------------------
vtkVector2f vtkAxis::GetPosition2()
{
  return this->Position2;
}

//-----------------------------------------------------------------------------
void vtkAxis::SetNumberOfTicks(int numberOfTicks)
{
  if (this->NumberOfTicks != numberOfTicks)
  {
    this->TickMarksDirty = true;
    this->Resized = true;
    this->NumberOfTicks = numberOfTicks;
    this->Modified();
  }
}

//-----------------------------------------------------------------------------
void vtkAxis::Update()
{
  if (!this->Visible || this->BuildTime > this->MTime)
  {
    return;
  }

  this->UpdateLogScaleActive(false);
  if ((this->Behavior == vtkAxis::AUTO || this->Behavior == vtkAxis::FIXED) &&
      this->TickMarksDirty)
  {
    // Regenerate the tick marks/positions if necessary
    // Calculate where the first tick mark should be drawn
    // FIXME: We need a specific resize event, to handle position change
    // independently.
    this->RecalculateTickSpacing();
    double first = ceil(this->Minimum / this->TickInterval)
      * this->TickInterval;
    double last = first;
    double interval(this->TickInterval);
    if (this->Minimum > this->Maximum)
    {
      interval *= -1.0;
    }
    for (int i = 0; i < 500; ++i)
    {
      last += interval;
      if ((interval > 0.0 && last > this->Maximum) ||
          (interval <= 0.0 && last < this->Maximum))
      {
        this->GenerateTickLabels(first, last - this->TickInterval);
        break;
      }
    }
  }

  // Figure out what type of behavior we should follow
  if (this->Resized &&
      (this->Behavior == vtkAxis::AUTO || this->Behavior == vtkAxis::FIXED))
  {
    this->RecalculateTickSpacing();
    this->Resized = false;
  }

  // Figure out the scaling and origin for the scene
  double scaling = 0.0;
  double origin = 0.0;
  if (this->Point1[0] == this->Point2[0]) // x1 == x2, therefore vertical
  {
    scaling = (this->Point2[1] - this->Point1[1]) /
              (this->Maximum - this->Minimum);
    origin = this->Point1[1];
  }
  else
  {
    scaling = (this->Point2[0] - this->Point1[0]) /
              (this->Maximum - this->Minimum);
    origin = this->Point1[0];
  }

  if (this->TickPositions->GetNumberOfTuples() !=
      this->TickLabels->GetNumberOfTuples())
  {
    // Generate the tick labels based on the tick positions
    this->GenerateTickLabels();
  }

  vtkIdType n = this->TickPositions->GetNumberOfTuples();
  this->TickScenePositions->SetNumberOfTuples(n);
  for (vtkIdType i = 0; i < n; ++i)
  {
    int iPos = vtkContext2D::FloatToInt(origin +
                                (this->TickPositions->GetValue(i) -
                                 this->Minimum) * scaling);
    this->TickScenePositions->InsertValue(i, iPos);
  }

  this->BuildTime.Modified();
}

//-----------------------------------------------------------------------------
bool vtkAxis::Paint(vtkContext2D *painter)
{
  // This is where everything should be drawn, or dispatched to other methods.
  vtkDebugMacro(<< "Paint event called in vtkAxis.");

  this->UpdateLogScaleActive(false);

  if (!this->Visible)
  {
    return false;
  }

  this->GetBoundingRect(painter);

  painter->ApplyPen(this->Pen);
  // Draw this axis
  if (this->AxisVisible)
  {
    painter->DrawLine(this->Point1[0], this->Point1[1],
                      this->Point2[0], this->Point2[1]);
  }

  // Draw the axis title if there is one
  if (!this->Title.empty())
  {
    int x = 0;
    int y = 0;
    painter->ApplyTextProp(this->TitleProperties);

    // Draw the axis label
    if (this->Position == vtkAxis::LEFT)
    {
      // Draw the axis label
      x = vtkContext2D::FloatToInt(this->Point1[0] - this->MaxLabel[0] - 10);
      y = vtkContext2D::FloatToInt(this->Point1[1] + this->Point2[1]) / 2;
    }
    else if (this->Position == vtkAxis::RIGHT)
    {
      // Draw the axis label
      x = vtkContext2D::FloatToInt(this->Point1[0] + this->MaxLabel[0] + 10);
      y = vtkContext2D::FloatToInt(this->Point1[1] + this->Point2[1]) / 2;
    }
    else if (this->Position == vtkAxis::BOTTOM)
    {
      x = vtkContext2D::FloatToInt(this->Point1[0] + this->Point2[0]) / 2;
      y = vtkContext2D::FloatToInt(this->Point1[1] - this->MaxLabel[1] - 10);
    }
    else if (this->Position == vtkAxis::TOP)
    {
      x = vtkContext2D::FloatToInt(this->Point1[0] + this->Point2[0]) / 2;
      y = vtkContext2D::FloatToInt(this->Point1[1] + this->MaxLabel[1] + 10);
    }
    else if (this->Position == vtkAxis::PARALLEL)
    {
      x = vtkContext2D::FloatToInt(this->Point1[0]);
      y = vtkContext2D::FloatToInt(this->Point1[1] - this->MaxLabel[1] - 15);
    }
    painter->DrawString(x, y, this->Title);
  }

  // Now draw the tick marks
  painter->ApplyTextProp(this->LabelProperties);

  float *tickPos = this->TickScenePositions->GetPointer(0);
  vtkStdString *tickLabel = this->TickLabels->GetPointer(0);
  vtkIdType numMarks = this->TickScenePositions->GetNumberOfTuples();

  // There are five possible tick label positions, which should be set by the
  // class laying out the axes.
  float tickLength = 5;
  float labelOffset = this->LabelOffset;
  if (this->Position == vtkAxis::LEFT || this->Position == vtkAxis::PARALLEL ||
      this->Position == vtkAxis::BOTTOM)
  {
    // The other side of the axis line.
    tickLength *= -1.0;
    labelOffset *= -1.0;
  }

  vtkVector2i tileScale(1);
  if (!this->Scene)
  {
    vtkWarningMacro("vtkAxis needs a vtkContextScene to determine window "
                    "properties. Assuming no tile scaling is set.");
  }
  else
  {
    tileScale = this->Scene->GetLogicalTileScale();
  }

  vtkRectf minLabelRect(0, 0, 0, 0);
  vtkRectf maxLabelRect(0, 0, 0, 0);
  float* minLabelBounds = minLabelRect.GetData();
  float* maxLabelBounds = maxLabelRect.GetData();

  // Optionally draw min/max labels
  if (this->RangeLabelsVisible)
  {
    vtkStdString minString = this->GenerateSprintfLabel(this->UnscaledMinimum, this->RangeLabelFormat);
    vtkStdString maxString = this->GenerateSprintfLabel(this->UnscaledMaximum, this->RangeLabelFormat);

    painter->ComputeJustifiedStringBounds(minString, minLabelBounds);
    painter->ComputeJustifiedStringBounds(maxString, maxLabelBounds);

    float minLabelShift[2] = {0, 0};
    float maxLabelShift[2] = {0, 0};

    // Compute where the string should go...
    if (this->Position == vtkAxis::LEFT || this->Position == vtkAxis::PARALLEL ||
        this->Position == vtkAxis::RIGHT)
    {
      minLabelShift[0] = this->Point1[0] + labelOffset;
      minLabelShift[1] = this->Point1[1];
      maxLabelShift[0] = this->Point2[0] + labelOffset;
      maxLabelShift[1] = this->Point2[1];
      if (this->TicksVisible)
      {
        painter->DrawLine(this->Point1[0] + tickLength, this->Point1[1],
                          this->Point1[0]             , this->Point1[1]);
        painter->DrawLine(this->Point2[0] + tickLength, this->Point2[1],
                          this->Point2[0]             , this->Point2[1]);
      }
    }
    else if (this->Position == vtkAxis::TOP || this->Position == vtkAxis::BOTTOM)
    {
      minLabelShift[0] = this->Point1[0];
      minLabelShift[1] = this->Point1[1] + labelOffset;
      maxLabelShift[0] = this->Point2[0];
      maxLabelShift[1] = this->Point2[1] + labelOffset;
      if (this->TicksVisible)
      {
        painter->DrawLine(this->Point1[0], this->Point1[1] + tickLength,
                          this->Point1[0], this->Point1[1]);
        painter->DrawLine(this->Point2[0], this->Point2[1] + tickLength,
                          this->Point2[0], this->Point2[1]             );
      }
    }

    // Now draw the labels
    painter->DrawString(minLabelShift[0], minLabelShift[1], minString);
    painter->DrawString(maxLabelShift[0], maxLabelShift[1], maxString);

    minLabelBounds[0] += minLabelShift[0];
    minLabelBounds[1] += minLabelShift[1];
    maxLabelBounds[0] += maxLabelShift[0];
    maxLabelBounds[1] += maxLabelShift[1];

    // Pad the range label bounds by a few pixels.
    float pad = 4;
    minLabelBounds[0] -= pad;
    minLabelBounds[1] -= pad;
    minLabelBounds[2] += 2*pad;
    minLabelBounds[3] += 2*pad;

    maxLabelBounds[0] -= pad;
    maxLabelBounds[1] -= pad;
    maxLabelBounds[2] += 2*pad;
    maxLabelBounds[3] += 2*pad;
  }

  // Horizontal or vertical axis.
  if (this->Position == vtkAxis::LEFT || this->Position == vtkAxis::PARALLEL ||
      this->Position == vtkAxis::RIGHT)
  {
    // Adptating tickLength and labelOffset to the tiling of the scene
    tickLength *= tileScale.GetX();
    labelOffset *= tileScale.GetX();

    // Draw the tick marks and labels
    for (vtkIdType i = 0; i < numMarks; ++i)
    {
      // Skip any tick positions that are outside of the axis range.
      if (!this->InRange(this->TickPositions->GetValue(i)))
      {
        continue;
      }

      // Don't skip if range labels aren't visible
      bool skipTick = this->RangeLabelsVisible;
      if (this->LabelsVisible)
      {
        float bounds[4];
        painter->ComputeJustifiedStringBounds(tickLabel[i], bounds);
        float pos[2] = { this->Point1[0] + labelOffset, tickPos[i] };
        bounds[0] += pos[0];
        bounds[1] += pos[1];

        vtkRectf boundsRect(bounds[0], bounds[1], bounds[2], bounds[3]);
        if (!boundsRect.IntersectsWith(minLabelRect) &&
            !boundsRect.IntersectsWith(maxLabelRect))
        {
          painter->DrawString(pos[0], pos[1], tickLabel[i]);
          skipTick = false;
        }
      }

      if (this->TicksVisible && !skipTick)
      {
        painter->DrawLine(this->Point1[0] + tickLength, tickPos[i],
                          this->Point1[0]             , tickPos[i]);
      }
    }
  }
  else if (this->Position == vtkAxis::TOP || this->Position == vtkAxis::BOTTOM)
  {

    // Adptating tickLength and labelOffset to the tiling of the scene
    tickLength *= tileScale.GetY();
    labelOffset *= tileScale.GetY();

    // Draw the tick marks and labels
    for (vtkIdType i = 0; i < numMarks; ++i)
    {
      // Skip any tick positions that are outside of the axis range.
      if (!this->InRange(this->TickPositions->GetValue(i)))
      {
        continue;
      }

      bool skipTick = this->RangeLabelsVisible;
      if (this->LabelsVisible)
      {
        float bounds[4];
        painter->ComputeJustifiedStringBounds(tickLabel[i], bounds);
        float pos[2] = { tickPos[i], this->Point1[1] + labelOffset };
        bounds[0] += pos[0];
        bounds[1] += pos[1];
        vtkRectf boundsRect(bounds[0], bounds[1], bounds[2], bounds[3]);
        if (!boundsRect.IntersectsWith(minLabelRect) &&
            !boundsRect.IntersectsWith(maxLabelRect))
        {
          painter->DrawString(pos[0], pos[1], tickLabel[i]);
          skipTick = false;
        }
      }

      if (this->TicksVisible && !skipTick)
      {
        painter->DrawLine(tickPos[i], this->Point1[1] + tickLength,
                          tickPos[i], this->Point1[1]);
      }

    }
  }
  else
  {
    vtkWarningMacro("Unknown position encountered in the paint call: "
                    << this->Position);
  }

  return true;
}

//-----------------------------------------------------------------------------
void vtkAxis::SetMinimum(double minimum)
{
  minimum = std::max(minimum, this->MinimumLimit);
  if (this->Minimum == minimum)
  {
    return;
  }
  this->Minimum = minimum;
  this->UnscaledMinimum = this->LogScaleActive ? pow(10., this->Minimum) : this->Minimum;
  this->UsingNiceMinMax = false;
  this->TickMarksDirty = true;
  this->Modified();
  this->InvokeEvent(vtkChart::UpdateRange);
}

//-----------------------------------------------------------------------------
void vtkAxis::SetUnscaledMinimum(double minimum)
{
  minimum = std::max(minimum, this->UnscaledMinimumLimit);
  if (this->UnscaledMinimum == minimum)
  {
    return;
  }
  this->UnscaledMinimum = minimum;
  this->UpdateLogScaleActive(true);
  this->UsingNiceMinMax = false;
  this->TickMarksDirty = true;
  this->Modified();
  this->InvokeEvent(vtkChart::UpdateRange);
}

//-----------------------------------------------------------------------------
void vtkAxis::SetMinimumLimit(double lowest)
{
  if (this->MinimumLimit == lowest)
  {
    return;
  }
  this->MinimumLimit = lowest;
  if (this->LogScaleActive)
  {
    if (this->UnscaledMinimum < 0)
    {
      this->UnscaledMaximumLimit = -1. * pow(10., this->MinimumLimit);
    }
    else
    {
      this->UnscaledMinimumLimit = pow(10., this->MinimumLimit);
    }
  }
  else
  {
    this->UnscaledMinimumLimit = this->MinimumLimit;
  }
  if (this->Minimum < lowest)
  {
    this->SetMinimum(lowest);
  }
}

//-----------------------------------------------------------------------------
void vtkAxis::SetUnscaledMinimumLimit(double lowest)
{
  if (this->UnscaledMinimumLimit == lowest)
  {
    return;
  }
  this->UnscaledMinimumLimit = lowest;
  this->NonLogUnscaledMinLimit = this->UnscaledMinimumLimit;
  this->MinimumLimit = this->LogScaleActive ?
    log10(this->UnscaledMinimumLimit) : this->UnscaledMinimumLimit;
  if (this->UnscaledMinimum < lowest)
  {
    this->SetUnscaledMinimum(lowest);
  }
}

//-----------------------------------------------------------------------------
void vtkAxis::SetMaximum(double maximum)
{
  maximum = std::min(maximum, this->MaximumLimit);
  if (this->Maximum == maximum)
  {
    return;
  }
  this->Maximum = maximum;
  this->UnscaledMaximum = this->LogScaleActive ? pow(10., this->Maximum) : this->Maximum;
  this->UsingNiceMinMax = false;
  this->TickMarksDirty = true;
  this->Modified();
  this->InvokeEvent(vtkChart::UpdateRange);
}

//-----------------------------------------------------------------------------
void vtkAxis::SetUnscaledMaximum(double maximum)
{
  maximum = std::min(maximum, this->UnscaledMaximumLimit);
  if (this->UnscaledMaximum == maximum)
  {
    return;
  }
  this->UnscaledMaximum = maximum;
  this->UpdateLogScaleActive(true);
  this->UsingNiceMinMax = false;
  this->TickMarksDirty = true;
  this->Modified();
  this->InvokeEvent(vtkChart::UpdateRange);
}

//-----------------------------------------------------------------------------
void vtkAxis::SetMaximumLimit(double highest)
{
  if (this->MaximumLimit == highest)
  {
    return;
  }
  this->MaximumLimit = highest;
  if (this->LogScaleActive)
  {
    if (this->UnscaledMaximum < 0)
    {
      this->UnscaledMinimumLimit = -1. * pow(10., this->MaximumLimit);
    }
    else
    {
      this->UnscaledMaximumLimit = pow(10., this->MaximumLimit);
    }
  }
  else
  {
    this->UnscaledMaximumLimit = this->MaximumLimit;
  }
  if (this->Maximum > highest)
  {
    this->SetMaximum(highest);
  }
}

//-----------------------------------------------------------------------------
void vtkAxis::SetUnscaledMaximumLimit(double highest)
{
  if (this->UnscaledMaximumLimit == highest)
  {
    return;
  }
  this->UnscaledMaximumLimit = highest;
  this->NonLogUnscaledMaxLimit = this->UnscaledMaximumLimit;
  this->MaximumLimit = this->LogScaleActive ?
    log10(this->UnscaledMaximumLimit) : this->UnscaledMaximumLimit;
  if (this->UnscaledMaximum > highest)
  {
    this->SetUnscaledMaximum(highest);
  }
}

//-----------------------------------------------------------------------------
void vtkAxis::SetRange(double minimum, double maximum)
{
  this->SetMinimum(minimum);
  this->SetMaximum(maximum);
}

//-----------------------------------------------------------------------------
void vtkAxis::SetRange(double *range)
{
  if (range)
  {
    this->SetMinimum(range[0]);
    this->SetMaximum(range[1]);
  }
}

//-----------------------------------------------------------------------------
void vtkAxis::SetUnscaledRange(double minimum, double maximum)
{
  this->SetUnscaledMinimum(minimum);
  this->SetUnscaledMaximum(maximum);
}

//-----------------------------------------------------------------------------
void vtkAxis::SetUnscaledRange(double *range)
{
  if (range)
  {
    this->SetUnscaledMinimum(range[0]);
    this->SetUnscaledMaximum(range[1]);
  }
}

//-----------------------------------------------------------------------------
void vtkAxis::GetRange(double *range)
{
  if (range)
  {
    range[0] = this->Minimum;
    range[1] = this->Maximum;
  }
}

//-----------------------------------------------------------------------------
void vtkAxis::GetUnscaledRange(double *range)
{
  if (range)
  {
    range[0] = this->UnscaledMinimum;
    range[1] = this->UnscaledMaximum;
  }
}

//-----------------------------------------------------------------------------
void vtkAxis::SetTitle(const vtkStdString &title)
{
  if (this->Title != title)
  {
    this->Title = title;
    this->Modified();
  }
}

//-----------------------------------------------------------------------------
vtkStdString vtkAxis::GetTitle()
{
  return this->Title;
}

//-----------------------------------------------------------------------------
void vtkAxis::SetPrecision(int precision)
{
  if (this->Precision == precision)
  {
    return;
  }
  this->Precision = precision;
  this->TickMarksDirty = true;
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkAxis::SetLabelFormat(const std::string &fmt)
{
  vtkDebugMacro(<< this->GetClassName() << " (" << this
                << "): setting LabelFormat to " << fmt);
  if (this->LabelFormat != fmt)
  {
    this->LabelFormat = fmt;
    this->Modified();
    this->TickMarksDirty = true;
  }
}

//-----------------------------------------------------------------------------
void vtkAxis::SetLogScale(bool logScale)
{
  if (this->LogScale == logScale)
  {
    return;
  }
  this->LogScale = logScale;
  this->UpdateLogScaleActive(false);
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkAxis::SetNotation(int notation)
{
  if (this->Notation == notation)
  {
    return;
  }
  this->Notation = notation;
  this->TickMarksDirty = true;
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkAxis::AutoScale()
{
  if (this->Behavior != vtkAxis::AUTO)
  {
    return;
  }

  this->UpdateLogScaleActive(false);
  // Calculate the min and max, set the number of ticks and the tick spacing.
  if (this->TickLabelAlgorithm == vtkAxis::TICK_SIMPLE)
  {
    double min = this->Minimum;
    double max = this->Maximum;
    this->TickInterval = this->CalculateNiceMinMax(min, max);
    this->SetRange(min, max);
  }
  this->UsingNiceMinMax = true;
  this->GenerateTickLabels(this->Minimum, this->Maximum);
}

//-----------------------------------------------------------------------------
void vtkAxis::RecalculateTickSpacing()
{
  // Calculate the min and max, set the number of ticks and the tick spacing,
  // discard the min and max in this case. TODO: Refactor the function called.
  if (this->Behavior == vtkAxis::AUTO || this->Behavior == vtkAxis::FIXED)
  {
    double min = this->Minimum;
    double max = this->Maximum;
    if (this->TickLabelAlgorithm == vtkAxis::TICK_SIMPLE)
    {
      this->TickInterval = this->CalculateNiceMinMax(min, max);
    }

    if (this->UsingNiceMinMax)
    {
      this->GenerateTickLabels(this->Minimum, this->Maximum);
    }
    else if (this->TickInterval == -1.0)
    {
      // if axis do not have a valid tickinterval - return
      return;
    }
    else
    {
      // Calculated tickinterval may be 0. So calculation of new minimum and
      // maximum by incrementing/decrementing using tickinterval will fail.
      if (this->TickInterval == 0.0)
      {
        return;
      }
      if (this->Minimum < this->Maximum)
      {
        while (min < this->Minimum)
        {
          min += this->TickInterval;
        }
        while (max > this->Maximum)
        {
          max -= this->TickInterval;
        }
      }
      else
      {
        while (min > this->Minimum)
        {
          min -= this->TickInterval;
        }
        while (max < this->Maximum)
        {
          max += this->TickInterval;
        }
      }
      this->GenerateTickLabels(min, max);
    }
  }
}

//-----------------------------------------------------------------------------
vtkDoubleArray* vtkAxis::GetTickPositions()
{
  return this->TickPositions;
}

//-----------------------------------------------------------------------------
vtkFloatArray* vtkAxis::GetTickScenePositions()
{
  return this->TickScenePositions;
}

//-----------------------------------------------------------------------------
vtkStringArray* vtkAxis::GetTickLabels()
{
  return this->TickLabels;
}

//-----------------------------------------------------------------------------
bool vtkAxis::SetCustomTickPositions(vtkDoubleArray *positions,
                                     vtkStringArray *labels)
{
  if (!positions && !labels)
  {
    this->CustomTickLabels = false;
    this->TickMarksDirty = true;
    this->TickPositions->SetNumberOfTuples(0);
    this->TickLabels->SetNumberOfTuples(0);
    this->Modified();
    return true;
  }
  else if (positions && !labels)
  {
    this->TickPositions->DeepCopy(positions);
    this->TickLabels->SetNumberOfTuples(0);
    this->CustomTickLabels = true;
    this->TickMarksDirty = false;
    this->Modified();
    return true;
  }
  else if (positions && labels)
  {
    if (positions->GetNumberOfTuples() != labels->GetNumberOfTuples())
    {
      return false;
    }
    this->TickPositions->DeepCopy(positions);
    this->TickLabels->DeepCopy(labels);
    this->CustomTickLabels = true;
    this->TickMarksDirty = false;
    this->Modified();
    return true;
  }
  else
  {
    return false;
  }
}

//-----------------------------------------------------------------------------
vtkRectf vtkAxis::GetBoundingRect(vtkContext2D* painter)
{
  bool vertical = false;
  if (this->Position == vtkAxis::LEFT || this->Position == vtkAxis::RIGHT ||
      this->Position == vtkAxis::PARALLEL)
  {
    vertical = true;
  }
  // First, calculate the widest tick label
  float widest = 0.0;
  // Second, calculate the tallest tick label
  float tallest = 0.0;
  vtkRectf bounds(0, 0, 0, 0);
  if (this->LabelsVisible)
  {
    for(vtkIdType i = 0; i < this->TickLabels->GetNumberOfTuples(); ++i)
    {
      painter->ApplyTextProp(this->LabelProperties);
      painter->ComputeStringBounds(this->TickLabels->GetValue(i),
                                   bounds.GetData());
      widest = bounds.GetWidth() > widest ? bounds.GetWidth() : widest;
      tallest = bounds.GetHeight() > tallest ? bounds.GetHeight() : tallest;
    }
  }

  if (this->RangeLabelsVisible)
  {
    // Add in the range labels
    vtkStdString minLabel = this->GenerateSprintfLabel(this->UnscaledMinimum, this->RangeLabelFormat);
    vtkStdString maxLabel = this->GenerateSprintfLabel(this->UnscaledMaximum, this->RangeLabelFormat);

    painter->ComputeStringBounds(minLabel, bounds.GetData());
    widest = bounds.GetWidth() > widest ? bounds.GetWidth() : widest;
    tallest = bounds.GetHeight() > tallest ? bounds.GetHeight() : tallest;

    painter->ComputeStringBounds(maxLabel, bounds.GetData());
    widest = bounds.GetWidth() > widest ? bounds.GetWidth() : widest;
    tallest = bounds.GetHeight() > tallest ? bounds.GetHeight() : tallest;
  }

  this->MaxLabel[0] = widest;
  this->MaxLabel[1] = tallest;

  // Then, if there is an axis label, add that in.
  vtkRectf titleBounds(0, 0, 0, 0);
  if (this->Title && !this->Title.empty())
  {
    painter->ApplyTextProp(this->TitleProperties);
    painter->ComputeStringBounds(this->Title,
                                 titleBounds.GetData());
  }

  if (vertical)
  {
    bounds.SetWidth(widest + titleBounds.GetWidth() + this->Margins[0]);
    float range = this->Point1[1] < this->Point2[1] ?
          this->Point2[1] - this->Point1[1] : this->Point1[1] - this->Point2[1];
    bounds.SetHeight(range + tallest + this->Margins[1]);
  }
  else
  {
    bounds.SetHeight(tallest + titleBounds.GetHeight() + this->Margins[0]);
    float range = this->Point1[0] < this->Point2[0] ?
          this->Point2[0] - this->Point1[0] : this->Point1[0] - this->Point2[0];
    bounds.SetWidth(range + widest + this->Margins[1]);
  }
  return bounds;
}

//-----------------------------------------------------------------------------
void vtkAxis::UpdateLogScaleActive(bool alwaysUpdateMinMaxFromUnscaled)
{
  bool needUpdate = false;
  if (this->LogScale &&
    this->UnscaledMinimum * this->UnscaledMaximum > 0.)
  {
    if (!this->LogScaleActive)
    {
      this->LogScaleActive = true;
      this->TickMarksDirty = true;
      needUpdate = true;
    }
    if (needUpdate || alwaysUpdateMinMaxFromUnscaled)
    {
      if (this->UnscaledMinimum < 0)
      { // Both unscaled min & max are negative, logs must be swapped
        this->Minimum = log10(fabs(this->UnscaledMaximum));
        this->Maximum = log10(fabs(this->UnscaledMinimum));
        if (this->UnscaledMaximumLimit >= 0)
        {
          // The limit is on the other side of 0 relative to the data...
          // move it to the same side as the data.
          // Specifically, allow scrolling equal to the width of the plot.
          this->MinimumLimit = -vtkMath::Inf();
          this->NonLogUnscaledMaxLimit = this->UnscaledMaximumLimit;
          this->UnscaledMaximumLimit = 0.;
        }
        else
        {
          this->MinimumLimit = log10(fabs(this->UnscaledMaximumLimit));
        }
        this->MaximumLimit = log10(fabs(this->UnscaledMinimumLimit));
      }
      else
      {
        this->Minimum = log10(fabs(this->UnscaledMinimum));
        this->Maximum = log10(fabs(this->UnscaledMaximum));
        if (this->UnscaledMinimumLimit <= 0)
        {
          // The limit is on the other side of 0 relative to the data...
          // move it to the same side as the data.
          // Specifically, allow scrolling equal to the width of the plot.
          this->MinimumLimit = -vtkMath::Inf();
          this->NonLogUnscaledMinLimit = this->UnscaledMinimumLimit;
          this->UnscaledMinimumLimit = 0.;
        }
        else
        {
          this->MinimumLimit = log10(fabs(this->UnscaledMinimumLimit));
        }
        this->MaximumLimit = log10(fabs(this->UnscaledMaximumLimit));
      }
      this->Modified();
    }
  }
  else
  {
    if (this->LogScaleActive)
    {
      this->LogScaleActive = false;
      this->TickMarksDirty = true;
      needUpdate = true;
    }
    if (needUpdate || alwaysUpdateMinMaxFromUnscaled)
    {
      this->Minimum = this->UnscaledMinimum;
      this->Maximum = this->UnscaledMaximum;
      this->UnscaledMinimumLimit = this->NonLogUnscaledMinLimit;
      this->UnscaledMaximumLimit = this->NonLogUnscaledMaxLimit;
      this->MinimumLimit = this->UnscaledMinimumLimit;
      this->MaximumLimit = this->UnscaledMaximumLimit;
      this->Modified();
    }
  }
}

//-----------------------------------------------------------------------------
void vtkAxis::GenerateTickLabels(double min, double max)
{
  if (this->CustomTickLabels == true)
  {
    // Never generate new tick labels if custom tick labels are being used.
    return;
  }
  // Now calculate the tick labels, and positions within the axis range
  this->TickPositions->SetNumberOfTuples(0);
  this->TickLabels->SetNumberOfTuples(0);

  // We generate a logarithmic scale when logarithmic axis is activated and the
  // order of magnitude of the axis is higher than 0.6.
  if (this->LogScaleActive)
  {
    // We calculate the first tick mark for lowest order of magnitude.
    // and the last for the highest order of magnitude.
    this->TickInterval = this->CalculateNiceMinMax(min, max);

    bool niceTickMark = false;
    int minOrder = 0;
    int maxOrder = 0;
    double minValue = LogScaleTickMark(pow(double(10.0), double(min)),
                                       true,
                                       niceTickMark,
                                       minOrder);
    double maxValue = LogScaleTickMark(pow(double(10.0), double(max)),
                                       false,
                                       niceTickMark,
                                       maxOrder);

    // We generate the tick marks for all orders of magnitude
    if (maxOrder - minOrder == 0)
    {
      this->GenerateLogSpacedLinearTicks(minOrder, min, max);
    }
    else
    {
      if (maxOrder - minOrder + 1 > 5)
      {
        GenerateLogScaleTickMarks(minOrder, minValue, 9.0, false);
        for(int i = minOrder + 1; i < maxOrder; ++i)
        {
          GenerateLogScaleTickMarks(i, 1.0, 9.0, false);
        }
        GenerateLogScaleTickMarks(maxOrder, 1.0, maxValue, false);
      }
      else
      {
        GenerateLogScaleTickMarks(minOrder, minValue, 9.0);
        for(int i = minOrder + 1; i < maxOrder; ++i)
        {
          GenerateLogScaleTickMarks(i, 1.0, 9.0);
        }
        GenerateLogScaleTickMarks(maxOrder, 1.0, maxValue);
      }
    }
  }
  else
  {
    if (this->TickLabelAlgorithm == vtkAxis::TICK_WILKINSON_EXTENDED)
    {
      // Now calculate the tick labels, and positions within the axis range
      //This gets the tick interval and max, min of labeling from the Extended
      // algorithm
      double scaling = 0.0;
      bool axisVertical = false;

      // When the axis is not initialized
      if(this->Point1[0] == 0 && this->Point2[0] == 0)
      {
        // 500 is an initial guess for the length of the axis in pixels
        scaling = 500 / (this->Maximum - this->Minimum);
      }
      else
      {
        if (this->Point1[0] == this->Point2[0]) // x1 == x2, therefore vertical
        {
          scaling = (this->Point2[1] - this->Point1[1]) /
                    (this->Maximum - this->Minimum);
          axisVertical = true;
        }
        else
        {
          scaling = (this->Point2[0] - this->Point1[0]) /
                    (this->Maximum - this->Minimum);
        }
      }

      int fontSize = this->LabelProperties->GetFontSize();
      vtkNew<vtkAxisExtended> tickPositionExtended;

      // The following parameters are required for the legibility part in the
      // optimization tickPositionExtended->SetFontSize(fontSize);
      tickPositionExtended->SetDesiredFontSize(fontSize);
      tickPositionExtended->SetPrecision(this->Precision);
      tickPositionExtended->SetIsAxisVertical(axisVertical);

      // Value 4 is hard coded for the user desired tick spacing
      vtkVector3d values =
          tickPositionExtended->GenerateExtendedTickLabels(min, max, 4,
                                                           scaling);
      min = values[0];
      max = values[1];
      this->TickInterval = values[2];

      if(min < this->Minimum)
      {
        this->Minimum = min;
        this->UnscaledMinimum = (this->LogScaleActive ? pow(10., this->Minimum) : this->Minimum);
      }
      if(max > this->Maximum)
      {
        this->Maximum = max;
        this->UnscaledMaximum = (this->LogScaleActive ? pow(10., this->Maximum) : this->Maximum);
      }

      this->Notation = tickPositionExtended->GetLabelFormat();
      this->LabelProperties->SetFontSize(tickPositionExtended->GetFontSize());
      if(tickPositionExtended->GetOrientation() == 1)
      {
        // Set this to 90 to make the labels vertical
        this->LabelProperties->SetOrientation(90);
      }
    }

    double mult = max > min ? 1.0 : -1.0;
    double range = 0.0;
    int n = 0;
    if (this->LogScaleActive)
    {
      range = mult > 0.0 ? pow(10.0, max) - pow(10.0, min)
        : pow(10.0, min) - pow(10.0, max);
      n = vtkContext2D::FloatToInt(range / pow(10.0, this->TickInterval));
    }
    else if (this->NumberOfTicks >= 0)
    {
      n = this->NumberOfTicks - 1;
    }
    else
    {
      range = mult > 0.0 ? max - min : min - max;
      n = vtkContext2D::FloatToInt(range / this->TickInterval);
    }
    for (int i = 0; i <= n && i < 200; ++i)
    {
      double value = 0.0;
      if (this->LogScaleActive)
      {
        value = log10(pow(10.0, min) + double(i) * mult
          * pow(10.0, this->TickInterval));
      }
      else
      {
        value = min + double(i) * mult * this->TickInterval;
      }
      if (this->TickInterval < 1.0)
      {
        // For small TickInterval, increase the precision of the comparison
        if (fabs(value) < (0.00000001 * this->TickInterval))
        {
          value = 0.0;
        }
      }
      else
      {
        if (fabs(value) < 0.00000001)
        {
          value = 0.0;
        }
      }
      this->TickPositions->InsertNextValue(value);
      // Make a tick mark label for the tick
      if (this->LogScaleActive)
      {
        value = pow(double(10.0), double(value));
      }
      // Now create a label for the tick position
      if (this->TickLabelAlgorithm == vtkAxis::TICK_SIMPLE)
      {
        this->TickLabels->InsertNextValue(this->GenerateSimpleLabel(value));
      }
      else
      {
        // The following call inserts a label into this->TickLabels
        this->GenerateLabelFormat(this->Notation, value);
      }
    }

  }
  this->TickMarksDirty = false;
}

//-----------------------------------------------------------------------------
void vtkAxis::GenerateTickLabels()
{
  this->TickLabels->SetNumberOfTuples(0);
  for (vtkIdType i = 0; i < this->TickPositions->GetNumberOfTuples(); ++i)
  {
    double value = this->TickPositions->GetValue(i);
    // Make a tick mark label for the tick
    if (this->LogScaleActive)
    {
      value = pow(double(10.0), double(value));
    }
    this->TickLabels->InsertNextValue(this->GenerateSimpleLabel(value));
  }
}

//-----------------------------------------------------------------------------
vtkStdString vtkAxis::GenerateSimpleLabel(double val)
{
  vtkStdString result;
  if (this->Notation == PRINTF_NOTATION)
  { // Use the C-style printf specification:
    result = this->GenerateSprintfLabel(val, this->LabelFormat);
  }
  else
  { // Use the C++ style stream format specification:
    std::ostringstream ostr;
    ostr.imbue(std::locale::classic());
    if (this->Notation != STANDARD_NOTATION)
    {
      ostr.precision(this->Precision);
      if (this->Notation == SCIENTIFIC_NOTATION)
      {
        ostr.setf(std::ios::scientific, std::ios::floatfield);
      }
      else if (this->Notation == FIXED_NOTATION)
      {
        ostr.setf(ios::fixed, ios::floatfield);
      }
    }
    ostr << val;
    result = vtkStdString(ostr.str());
  }

  // Strip out leading zeros on the exponent:
  vtksys::RegularExpression regExp("[Ee][+-]");
  if (regExp.find(result))
  {
    vtkStdString::iterator it = result.begin() + regExp.start() + 2;
    while (it != result.end() && *it == '0')
    {
      it = result.erase(it);
    }

    // If the exponent is 0, remove the e+ bit, too.
    if (it == result.end())
    {
      result.erase(regExp.start());
    }
  }

  return result;
}

//-----------------------------------------------------------------------------
// This methods generates tick labels for 8 different format notations
//   1 - Scientific 5 * 10^6
//   2 - Decimal e.g. 5000
//   3 - K e.g. 5K
//   4 - Factored K e.g. 5(K)
//   5 - M e.g. 5M
//   6 - Factored M e.g. 5(M)
//   7 - Factored Decimals e.g. 5 (thousands)
//   8 - Factored Scientific 5 (10^6)
void vtkAxis::GenerateLabelFormat(int notation, double n)
{
  std::ostringstream ostr;
  ostr.imbue(std::locale::classic());

  switch(notation)
  {
    case 1:
      ostr << n;
      ostr.precision(this->Precision);
      ostr.setf(std::ios::scientific, std::ios::floatfield);
      this->TickLabels->InsertNextValue(ostr.str());
      break;
    case 2:
      ostr << n;
      if((std::ceil(n)-std::floor(n)) != 0.0 )
      {
        ostr.precision(this->Precision);
      }
      this->TickLabels->InsertNextValue(ostr.str());
      break;
    case 3:
      ostr.setf(ios::fixed, ios::floatfield);
      ostr << n/1000.0 << "K";
      if((std::ceil(n/1000.0)-std::floor(n/1000.0)) != 0.0 )
      {
        ostr.precision(this->Precision);
      }
      this->TickLabels->InsertNextValue(ostr.str()); // minus three zeros + K
      break;
    case 4:
      ostr.setf(ios::fixed, ios::floatfield);
      ostr << n/1000.0 ;
      if((std::ceil(n/1000.0)-std::floor(n/1000.0)) != 0.0 )
      {
        ostr.precision(this->Precision);
      }
      if(!TitleAppended)
      {
        this->Title.append(" (K)");
        TitleAppended = true;
      }
      this->TickLabels->InsertNextValue(ostr.str());// minus three zeros
      break;
    case 5:
      ostr.setf(ios::fixed, ios::floatfield);
      ostr << n/1000000.0 << "M";
      if((std::ceil(n/1000000.0)-std::floor(n/1000000.0)) != 0.0 )
      {
        ostr.precision(this->Precision);
      }
      this->TickLabels->InsertNextValue(ostr.str()); // minus six zeros
      break;
    case 6:
      ostr.precision(this->Precision);
      ostr.setf(ios::fixed, ios::floatfield);
      ostr << n/1000000.0;
      if((std::ceil(n/1000000.0)-std::floor(n/1000000.0)) != 0.0 )
      {
        ostr.precision(this->Precision);
      }
      if(!TitleAppended)
      {
        this->Title.append(" (M)");
        TitleAppended = true;
      }
      this->TickLabels->InsertNextValue(ostr.str()); // minus six zeros + M
      break;
    case 7:
      ostr.precision(this->Precision);
      ostr.setf(ios::fixed, ios::floatfield);
      ostr << n/1000.0;
      if((std::ceil(n/1000.0)-std::floor(n/1000.0)) != 0.0 )
      {
        ostr.precision(this->Precision);
      }
      if(!TitleAppended)
      {
        this->Title.append(" ('000)");
        TitleAppended = true;
      }
      this->TickLabels->InsertNextValue(ostr.str());  // Three 0's get reduced
      break;
    case 8:
      ostr.precision(this->Precision);
      ostr.setf(std::ios::scientific, std::ios::floatfield);
      ostr << n/1000.0 ;
      if(!TitleAppended)
      {
        this->Title.append(" ('000)");
        TitleAppended = true;
      }
      this->TickLabels->InsertNextValue(ostr.str());
      break;
  }
}

//-----------------------------------------------------------------------------
vtkStdString vtkAxis::GenerateSprintfLabel(double value, const std::string & format)
{
  // Use the C-style printf specification:
  const int buffSize = 1024;
  char buffer[buffSize];

  // On Windows, formats with exponents have three digits by default
  // whereas on other systems, exponents have two digits. Set to two
  // digits on Windows for consistent behavior.
#if defined(_MSC_VER) && _MSC_VER < 1900
  unsigned int oldWin32ExponentFormat = _set_output_format(_TWO_DIGIT_EXPONENT);

  _snprintf(buffer, buffSize-1, format.c_str(), value);
  buffer[buffSize-1] = '\0';

  _set_output_format(oldWin32ExponentFormat);
#else
  snprintf(buffer, buffSize, format.c_str(), value);
#endif

  vtkStdString result = vtkStdString(buffer);

  return result;
}

//-----------------------------------------------------------------------------
double vtkAxis::NiceMinMax(double &min, double &max, float pixelRange,
                           float tickPixelSpacing)
{
  // First get the order of the range of the numbers
  if (min == max)
  {
    if (fabs(min) < 1e-70 && fabs(max) < 1e-70)
    {
      min = -0.0000001;
      max =  0.0000001;
    }
    else
    {
      min *= 0.95;
      max *= 1.05;
    }
  }
  else if ((max - min) < 1.0e-60)
  {
    min *= 0.95;
    max *= 1.05;
  }

  double range = max - min;
  bool isNegative = false;
  if (range < 0.0f)
  {
    isNegative = true;
    range *= -1.0f;
  }

  // Calculate an upper limit on the number of tick marks - at least 30 pixels
  // should be between each tick mark.
  int maxTicks = vtkContext2D::FloatToInt(pixelRange / tickPixelSpacing);
  if (maxTicks == 0)
  {
    // The axes do not have a valid set of points - return
    return -1.0f;
  }
  double tickSpacing = range / maxTicks;

  int order = static_cast<int>(floor(log10(tickSpacing)));
  double normTickSpacing = tickSpacing * pow(double(10.0), -order);
  double niceTickSpacing = vtkAxis::NiceNumber(normTickSpacing, true);
  niceTickSpacing *= pow(double(10.0), order);

  if (isNegative)
  {
    min = ceil(min / niceTickSpacing) * niceTickSpacing;
    max = floor(max / niceTickSpacing) * niceTickSpacing;
  }
  else
  {
    min = floor(min / niceTickSpacing) * niceTickSpacing;
    max = ceil(max / niceTickSpacing) * niceTickSpacing;
  }

  return niceTickSpacing;
}

//-----------------------------------------------------------------------------
double vtkAxis::CalculateNiceMinMax(double &min, double &max)
{
  if (this->NumberOfTicks > 0)
  {
    // An exact number of ticks was requested, use the min/max and exact number.
    min = this->Minimum;
    max = this->Maximum;
    double range = fabs(max - min);
    return range / double(this->NumberOfTicks - 1);
  }

  vtkVector2i tileScale(1);
  if (!this->Scene)
  {
    vtkWarningMacro("vtkAxis needs a vtkContextScene to determine window "
                    "properties. Assuming no tile scaling is set.");
  }
  else
  {
    tileScale = this->Scene->GetLogicalTileScale();
  }

  float pixelRange = 0;
  float tickPixelSpacing = 0;
  if (this->Position == vtkAxis::LEFT || this->Position == vtkAxis::RIGHT
      || this->Position == vtkAxis::PARALLEL)
  {
    pixelRange = this->Position2.GetY() - this->Position1.GetY();
    tickPixelSpacing = 30 * tileScale.GetX();
  }
  else
  {
    pixelRange = this->Position2.GetX() - this->Position1.GetX();
    tickPixelSpacing = 45 * tileScale.GetY();
  }

  double niceTickSpacing = 0.0;
  if (max < min)
  {
    niceTickSpacing =
      vtkAxis::NiceMinMax(max, min, pixelRange, tickPixelSpacing);
  }
  else
  {
    niceTickSpacing =
      vtkAxis::NiceMinMax(min, max, pixelRange, tickPixelSpacing);
  }

  return niceTickSpacing;
}

//-----------------------------------------------------------------------------
double vtkAxis::NiceNumber(double n, bool roundUp)
{
  if (roundUp)
  {
    if (n <= 1.0)
    {
      return 1.0;
    }
    else if (n <= 2.0)
    {
      return 2.0;
    }
    else if (n <= 5.0)
    {
      return 5.0;
    }
    else
    {
      return 10.0;
    }
  }
  else
  {
    if (n < 1.5)
    {
      return 1.0;
    }
    else if (n <= 3.0)
    {
      return 2.0;
    }
    else if (n <= 7.0)
    {
      return 5.0;
    }
    else
    {
      return 10.0;
    }
  }
}

//-----------------------------------------------------------------------------
double vtkAxis::LogScaleTickMark(double number,
                                 bool roundUp,
                                 bool &niceValue,
                                 int &order)
{
  // We need to retrive the order of our number.
  order = static_cast<int>(floor(log10(number)));

  // We retrive the basis of our number depending on roundUp and return it as
  // result.
  number = number * pow(10.0, static_cast<double>(order*(-1)));
  double result = roundUp ? ceil(number) : floor(number);

  // If result is 1.0, 2.0 or 5.0 we mark the result as "nice value".
  niceValue = false;
  if (result == 1.0 || result == 2.0 || result == 5.0)
  {
    niceValue = true;
  }
  return result;
}

//-----------------------------------------------------------------------------
void vtkAxis::GenerateLogSpacedLinearTicks(int order, double min, double max)
{
  // Log-scale axis, but zoomed in too far to show an order of magnitude in
  // the left-most digit.
  // Figure out which digit to vary and by how much.
  double linMin = pow(10., min);
  double linMax = pow(10., max);
  int varyDigit = static_cast<int>(floor(log10(linMax - linMin)));
  if (varyDigit == order)
  {
    --varyDigit;
  }
  double multiplier = pow(10.,varyDigit);
  int lo = static_cast<int>(floor(linMin / multiplier));
  int hi = static_cast<int>(ceil(linMax / multiplier));
  if (hi - lo < 2)
  {
    ++hi;
    --lo;
  }
  int incr = 1;
  int nt = hi - lo;
  if (nt > 20)
  {
    incr = nt > 10 ? 5 : 2;
  }

  for(int j = lo; j <= hi; j += incr)
  {
    // We calculate the tick mark value
    double value = j * multiplier;
    this->TickPositions->InsertNextValue(log10(value));

    // Now create a label for the tick position
    std::ostringstream ostr;
    ostr.imbue(std::locale::classic());
    if (this->Notation > 0)
    {
      ostr.precision(this->Precision);
    }
    if (this->Notation == SCIENTIFIC_NOTATION)
    {
      ostr.setf(std::ios::scientific, std::ios::floatfield);
    }
    else if (this->Notation == FIXED_NOTATION)
    {
      ostr.setf(ios::fixed, ios::floatfield);
    }
    ostr << value;

    this->TickLabels->InsertNextValue(ostr.str());
  }
}

//-----------------------------------------------------------------------------
void vtkAxis::GenerateLogScaleTickMarks(int order,
                                        double min,
                                        double max,
                                        bool detailLabels)
{
  // If the values min and max are not within limits we set defaults
  if (min < 1.0)
  {
    min = 1.0;
  }
  if (min > 9.0)
  {
    min = 1.0;
  }
  if (max < 1.0)
  {
    max = 9.0;
  }
  if (max > 9.0)
  {
    max = 9.0;
  }
  if (fabs(max-min) < 1.0)
  {
    min = 1.0;
    max = 9.0;
  }

  // Make sure we have integers
  int minimum = static_cast<int>(ceil(min));
  int maximum = static_cast<int>(floor(max));

  double result(minimum);
  for(int j = minimum; j <= maximum; ++j)
  {
    // We check if tick mark is getting an label depending on detailLabels
    bool niceTickMark = false;
    if (detailLabels)
    {
      niceTickMark = (result == 1.0 || result == 2.0 || result == 5.0);
    }
    else
    {
      niceTickMark = (result == 1.0);
    }

    // We calculate the tick mark value
    double value = result * pow(10.0, static_cast<double>(order));
    this->TickPositions->InsertNextValue(log10(value));

    if (niceTickMark)
    {
      this->TickLabels->InsertNextValue(this->GenerateSimpleLabel(value));
    }
    else
    {
      this->TickLabels->InsertNextValue("");
    }
    result += 1.0;
  }
}

//-----------------------------------------------------------------------------
inline bool vtkAxis::InRange(double value)
{
  // Figure out which way around the axes are, then see if the value is inside.
  double min(this->Minimum);
  double max(this->Maximum);
  if (min > max)
  {
    min = max;
    max = this->Minimum;
  }
  if (value < min || value > max)
  {
    return false;
  }
  else
  {
    return true;
  }
}

//-----------------------------------------------------------------------------
void vtkAxis::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  if (this->Title)
  {
    os << indent << "Axis title: \"" << *this->Title << "\"" << endl;
  }
  os << indent << "Minimum point: " << this->Point1[0] << ", "
     << this->Point1[1] << endl;
  os << indent << "Maximum point: " << this->Point2[0] << ", "
     << this->Point2[1] << endl;
  os << indent << "Range: " << this->Minimum << " - " << this->Maximum << endl;
  os << indent << "Range limits: "
    << this->MinimumLimit << " - " << this->MaximumLimit << endl;
  os << indent << "Number of tick marks: " << this->NumberOfTicks << endl;
  os << indent << "LogScale: " << (this->LogScale ? "TRUE" : "FALSE") << endl;
  os << indent << "LogScaleActive: " << (this->LogScaleActive ? "TRUE" : "FALSE") << endl;
  os << indent << "GridVisible: " << (this->GridVisible ? "TRUE" : "FALSE") << endl;
  os << indent << "LabelsVisible: " << (this->LabelsVisible ? "TRUE" : "FALSE") << endl;
  os << indent << "RangeLabelsVisible: " << (this->RangeLabelsVisible ? "TRUE" : "FALSE") << endl;
  os << indent << "TicksVisible: " << (this->TicksVisible ? "TRUE" : "FALSE") << endl;
  os << indent << "AxisVisible: " << (this->AxisVisible ? "TRUE" : "FALSE") << endl;
  os << indent << "Precision: " << this->Precision << endl;
  os << indent << "Notation: ";
  switch (this->Notation)
  {
    case STANDARD_NOTATION:
      os << "STANDARD_NOTATION";
      break;

    case SCIENTIFIC_NOTATION:
      os << "SCIENTIFIC_NOTATION";
      break;

    case FIXED_NOTATION:
      os << "FIXED_NOTATION";
      break;

    case PRINTF_NOTATION:
      os << "PRINTF_NOTATION";
      break;

    default:
      os << "<unknown>";
      break;
  }
  os << endl;
  os << indent << "LabelFormat: " << this->LabelFormat << endl;
  os << indent << "Behavior: ";
  switch (this->Behavior)
  {
    case AUTO:
      os << "AUTO";
      break;

    case FIXED:
      os << "FIXED";
      break;

    case CUSTOM:
      os << "CUSTOM";
      break;

    default:
      os << "<unknown>";
      break;
  }
  os << endl;

  os << indent << "Unscaled range: "
    << this->UnscaledMinimum << " - " << this->UnscaledMaximum << endl;
  os << indent << "Unscaled range limits: "
    << this->UnscaledMinimumLimit << " - " << this->UnscaledMaximumLimit << endl;
  os << indent << "Fallback unscaled range limits: "
    << this->NonLogUnscaledMinLimit << " - " << this->NonLogUnscaledMaxLimit << endl;
  os << indent << "ScalingFactor: " << this->ScalingFactor << endl;
  os << indent << "Shift: " << this->Shift << endl;
}
