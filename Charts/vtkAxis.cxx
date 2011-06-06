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

#include "vtkContext2D.h"
#include "vtkPen.h"
#include "vtkTextProperty.h"
#include "vtkVector.h"
#include "vtkFloatArray.h"
#include "vtkDoubleArray.h"
#include "vtkStringArray.h"
#include "vtkStdString.h"
#include "vtksys/ios/sstream"
#include "vtkObjectFactory.h"

#include <algorithm>
#include <limits>
#include "math.h"

//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkAxis);

//-----------------------------------------------------------------------------
vtkAxis::vtkAxis()
{
  this->Position = -1;
  this->Point1 = this->Position1.GetData();
  this->Point2 = this->Position2.GetData();
  this->Position1.SetY(10.0);
  this->Position2.SetY(10.0);
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
  this->MinimumLimit = std::numeric_limits<double>::max() * -1.;
  this->MaximumLimit = std::numeric_limits<double>::max();
  this->LogScale = false;
  this->GridVisible = true;
  this->LabelsVisible = true;
  this->Precision = 2;
  this->Notation = 0; // Mixed - do the right thing...
  this->Behavior = 0;
  this->Pen = vtkPen::New();
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
  this->LogScaleReasonable = false;
  this->MaxLabel[0] = this->MaxLabel[1] = 0.0;
  this->Resized = true;
  this->SetPosition(vtkAxis::LEFT);
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

void vtkAxis::SetPoint1(const vtkVector2f &pos)
{
  this->Position1 = pos;
  this->Resized = true;
  this->Modified();
}

void vtkAxis::SetPoint1(float x, float y)
{
  this->SetPoint1(vtkVector2f(x, y));
}

vtkVector2f vtkAxis::GetPosition1()
{
  return this->Position1;
}

void vtkAxis::SetPoint2(const vtkVector2f &pos)
{
  this->Position2 = pos;
  this->Resized = true;
  this->Modified();
}

void vtkAxis::SetPoint2(float x, float y)
{
  this->SetPoint2(vtkVector2f(x, y));
}

vtkVector2f vtkAxis::GetPosition2()
{
  return this->Position2;
}

//-----------------------------------------------------------------------------
void vtkAxis::Update()
{
  if (!this->Visible || this->BuildTime > this->MTime)
    {
    return;
    }

  if (this->Behavior < 2 && this->TickMarksDirty)
    {
    // Regenerate the tick marks/positions if necessary
    // Calculate where the first tick mark should be drawn
    if (this->LogScale && !this->LogScaleReasonable)
      {
      // Since the TickInterval may have changed due to moved axis we need to
      // recalculte TickInterval
      this->RecalculateTickSpacing();
      }
    else
      {
      // FIXME: We need a specific resize event, to handle position change
      // independently.
      //this->RecalculateTickSpacing();
      double first = ceil(this->Minimum / this->TickInterval)
        * this->TickInterval;
      double last = first;
      for (int i = 0; i < 500; ++i)
        {
        last += this->TickInterval;
        if (last > this->Maximum)
          {
          this->GenerateTickLabels(first, last-this->TickInterval);
          break;
          }
        }
      }
    }

  // Figure out what type of behavior we should follow
  if (this->Resized &&
      (this->Behavior == vtkAxis::AUTO || this->Behavior == vtkAxis::FIXED))
    {
    this->RecalculateTickSpacing();
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

  if (!this->Visible)
    {
    return false;
    }

  painter->ApplyPen(this->Pen);
  // Draw this axis
  painter->DrawLine(this->Point1[0], this->Point1[1],
                    this->Point2[0], this->Point2[1]);

  // Draw the axis title if there is one
  if (this->Title && !this->Title.empty())
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

  // There are four possible tick label positions, which should be set by the
  // class laying out the axes.
  if (this->Position == vtkAxis::LEFT || this->Position == vtkAxis::PARALLEL)
    {
    // Draw the tick marks and labels
    for (vtkIdType i = 0; i < numMarks; ++i)
      {
      painter->DrawLine(this->Point1[0] - 5, tickPos[i],
                        this->Point1[0],     tickPos[i]);
      if (this->LabelsVisible)
        {
        painter->DrawString(this->Point1[0] - 7, tickPos[i], tickLabel[i]);
        }
      }
    }
  else if (this->Position == vtkAxis::RIGHT)
    {
    // Draw the tick marks and labels
    for (vtkIdType i = 0; i < numMarks; ++i)
      {
      painter->DrawLine(this->Point1[0] + 5, tickPos[i],
                        this->Point1[0],     tickPos[i]);
      if (this->LabelsVisible)
        {
        painter->DrawString(this->Point1[0] + 7, tickPos[i], tickLabel[i]);
        }
      }
    }
  else if (this->Position == vtkAxis::BOTTOM)
    {
    // Draw the tick marks and labels
    for (vtkIdType i = 0; i < numMarks; ++i)
      {
      painter->DrawLine(tickPos[i], this->Point1[1] - 5,
                        tickPos[i], this->Point1[1]);
      if (this->LabelsVisible)
        {
        painter->DrawString(tickPos[i], int(this->Point1[1] - 7), tickLabel[i]);
        }
      }
    }
  else if (this->Position == vtkAxis::TOP)
    {
    // Draw the tick marks and labels
    for (vtkIdType i = 0; i < numMarks; ++i)
      {
      painter->DrawLine(tickPos[i], this->Point1[1] + 5,
                        tickPos[i], this->Point1[1]);
      if (this->LabelsVisible)
        {
        painter->DrawString(tickPos[i], int(this->Point1[1] + 7), tickLabel[i]);
        }
      }
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
  this->UsingNiceMinMax = false;
  this->TickMarksDirty = true;
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkAxis::SetMinimumLimit(double lowest)
{
  if (this->MinimumLimit == lowest)
    {
    return;
    }
  if (this->Minimum < lowest )
    {
    this->SetMinimum(lowest);
    }
  this->MinimumLimit = lowest;
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
  this->UsingNiceMinMax = false;
  this->TickMarksDirty = true;
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkAxis::SetMaximumLimit(double highest)
{
  if (this->MaximumLimit == highest)
    {
    return;
    }
  if (this->Maximum < highest )
    {
    this->SetMaximum(highest);
    }
  this->MaximumLimit = highest;
}

//-----------------------------------------------------------------------------
void vtkAxis::SetRange(double minimum, double maximum)
{
  this->SetMinimum(minimum);
  this->SetMaximum(maximum);
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
  // Calculate the min and max, set the number of ticks and the tick spacing
  this->TickInterval = this->CalculateNiceMinMax(this->Minimum, this->Maximum);
  this->UsingNiceMinMax = true;
  this->GenerateTickLabels(this->Minimum, this->Maximum);
}

//-----------------------------------------------------------------------------
void vtkAxis::RecalculateTickSpacing()
{
  // Calculate the min and max, set the number of ticks and the tick spacing,
  // discard the min and max in this case. TODO: Refactor the function called.
  if (this->Behavior < 2)
    {
    double min = this->Minimum;
    double max = this->Maximum;
    this->TickInterval = this->CalculateNiceMinMax(min, max);
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
      if (this->LogScale && !this->LogScaleReasonable)
        {
          // If logartihmic axis is enabled and log scale is not reasonable
          // then TickInterval was calculated for linear scale but transformed
          // to log value. Therefore we need another method to
          // increment/decrement min and max value.
          if (this->Minimum < this->Maximum)
          {
          while (min < this->Minimum)
            {
            min = log10(pow(10.0, min) + pow(10.0, this->TickInterval));
            }
          while (max > this->Maximum)
            {
            max = log10(pow(10.0, max) - pow(10.0, this->TickInterval));
            }
          }
        else
          {
          while (min > this->Minimum)
            {
            min = log10(pow(10.0, min) - pow(10.0, this->TickInterval));
            }
          while (max < this->Maximum)
            {
            max = log10(pow(10.0, max) + pow(10.0, this->TickInterval));
            }
          }
        this->GenerateTickLabels(min, max);
        }
      else
        {
        // Calculated tickinterval may be 0. So calculation of new minimum and
        // maximum by incrementing/decrementing using tickinterval will fail.
        if(this->TickInterval == 0.0)
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
}

//-----------------------------------------------------------------------------
vtkDoubleArray* vtkAxis::GetTickPositions()
{
  return this->TickPositions;
}

//-----------------------------------------------------------------------------
void vtkAxis::SetTickPositions(vtkDoubleArray* array)
{
  if (this->TickPositions == array)
    {
    return;
    }
  this->TickPositions = array;
  this->Behavior = 2;
  this->TickMarksDirty = false;
  this->Modified();
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
void vtkAxis::SetTickLabels(vtkStringArray* array)
{
  if (this->TickLabels == array)
    {
    return;
    }
  this->TickLabels = array;
  this->Behavior = 2;
  this->TickMarksDirty = false;
  this->Modified();
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
  vtkRectf bounds;
  for(vtkIdType i = 0; i < this->TickLabels->GetNumberOfTuples(); ++i)
    {
    painter->ApplyTextProp(this->LabelProperties);
    painter->ComputeStringBounds(this->TickLabels->GetValue(i),
                                 bounds.GetData());
    widest = bounds.GetWidth() > widest ? bounds.GetWidth() : widest;
    tallest = bounds.GetHeight() > tallest ? bounds.GetHeight() : tallest;
    }
  this->MaxLabel[0] = widest;
  this->MaxLabel[1] = tallest;

  // Then, if there is an axis label, add that in.
  vtkRectf titleBounds;
  if (this->Title && !this->Title.empty())
    {
    painter->ApplyTextProp(this->TitleProperties);
    painter->ComputeStringBounds(this->Title,
                                 titleBounds.GetData());
    }

  if (vertical)
    {
    bounds.SetWidth(widest + titleBounds.GetWidth() + 15);
    float range = this->Point1[1] < this->Point2[1] ?
          this->Point2[1] - this->Point1[1] : this->Point1[1] - this->Point2[1];
    bounds.SetHeight(range + tallest + 5);
    }
  else
    {
    float range = this->Point1[0] < this->Point2[0] ?
          this->Point2[0] - this->Point1[0] : this->Point1[0] - this->Point2[0];
    bounds.SetWidth(range + widest + 5);
    bounds.SetHeight(tallest + titleBounds.GetHeight() + 15);
    }
  return bounds;
}

//-----------------------------------------------------------------------------
void vtkAxis::GenerateTickLabels(double min, double max)
{
  // Now calculate the tick labels, and positions within the axis range
  this->TickPositions->SetNumberOfTuples(0);
  this->TickLabels->SetNumberOfTuples(0);

  // We generate a logarithmic scale when logarithmic axis is activated and the
  // order of magnitude of the axis is higher than 0.6.
  if (this->LogScale && this->LogScaleReasonable)
    {
    // We calculate the first tick mark for lowest order of magnitude.
    // and the last for the highest order of magnitude.
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
      GenerateLogScaleTickMarks(minOrder, minValue, maxValue);
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
    // Now calculate the tick labels, and positions within the axis range
    double mult = max > min ? 1.0 : -1.0;
    double range = 0.0;
    int n = 0;
    if (this->LogScale)
      {
      range = mult > 0.0 ? pow(10.0, max) - pow(10.0, min)
        : pow(10.0, min) - pow(10.0, max);
      n = vtkContext2D::FloatToInt(range / pow(10.0, this->TickInterval));
      }
    else
      {
      range = mult > 0.0 ? max - min : min - max;
      n = vtkContext2D::FloatToInt(range / this->TickInterval);
      }
    for (int i = 0; i <= n && i < 200; ++i)
      {
      double value = 0.0;
      if (this->LogScale)
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
      if (this->LogScale)
        {
        value = pow(double(10.0), double(value));
        }
      // Now create a label for the tick position
      vtksys_ios::ostringstream ostr;
      ostr.imbue(vtkstd::locale::classic());
      if (this->Notation > 0)
        {
        ostr.precision(this->Precision);
        }
      if (this->Notation == 1)
        {
        // Scientific notation
        ostr.setf(vtksys_ios::ios::scientific, vtksys_ios::ios::floatfield);
        }
      else if (this->Notation == 2)
        {
        ostr.setf(ios::fixed, ios::floatfield);
        }
      ostr << value;

      this->TickLabels->InsertNextValue(ostr.str());
      }
    }
  this->TickMarksDirty = false;
}

void vtkAxis::GenerateTickLabels()
{
  this->TickLabels->SetNumberOfTuples(0);
  for (vtkIdType i = 0; i < this->TickPositions->GetNumberOfTuples(); ++i)
    {
    double value = this->TickPositions->GetValue(i);
    // Make a tick mark label for the tick
    if (this->LogScale)
      {
      value = pow(double(10.0), double(value));
      }
    // Now create a label for the tick position
    vtksys_ios::ostringstream ostr;
    ostr.imbue(vtkstd::locale::classic());
    if (this->Notation > 0)
      {
      ostr.precision(this->Precision);
      }
    if (this->Notation == 1)
      {
      // Scientific notation
      ostr.setf(vtksys_ios::ios::scientific, vtksys_ios::ios::floatfield);
      }
    else if (this->Notation == 2)
      {
      ostr.setf(ios::fixed, ios::floatfield);
      }
    ostr << value;

    this->TickLabels->InsertNextValue(ostr.str());
    }
}

//-----------------------------------------------------------------------------
double vtkAxis::CalculateNiceMinMax(double &min, double &max)
{
  double oldmin = min;
  double oldmax = max;
  this->LogScaleReasonable = false;
  // We check if logaritmic scale seems reasonable.
  if (this->LogScale)
    {
    this->LogScaleReasonable = ((max - min) >= log10(6.0));
    }

  // If logarithmic axis is activated and a logarithmic scale seems NOT
  // reasonable we transform the min/max value.
  // Thus the following code works for logarithmic axis with linear scale too.
  if (this->LogScale && !this->LogScaleReasonable)
    {
    min = pow(double(10.0), double(min));
    max = pow(double(10.0), double(max));
    }

  // First get the order of the range of the numbers
  if (min == max)
    {
    if (fabs(min) < 1e-20 && fabs(max) < 1e-20)
      {
      min = -0.01;
      max = 0.01;
      }
    else
      {
      min *= 0.95;
      max *= 1.05;
      }
    }
  else if ((max - min) < 1.0e-20)
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
  int maxTicks = 0;
  if (this->Position == vtkAxis::LEFT || this->Position == vtkAxis::RIGHT
      || this->Position == vtkAxis::PARALLEL)
    {
    float pixelRange = this->Position2.Y() - this->Position1.Y();
    maxTicks = vtkContext2D::FloatToInt(pixelRange / 30.0f);
    }
  else
    {
    float pixelRange = this->Position2.X() - this->Position1.X();
    maxTicks = vtkContext2D::FloatToInt(pixelRange / 45.0f);
    }
  if (maxTicks == 0)
    {
    // The axes do not have a valid set of points - return
    return -1.0f;
    }
  double tickSpacing = range / maxTicks;

  int order = static_cast<int>(floor(log10(tickSpacing)));
  double normTickSpacing = tickSpacing * pow(double(10.0), -order);
  double niceTickSpacing = this->NiceNumber(normTickSpacing, true);
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

  double newRange = max - min;

  // If logarithmic axis is activated and logarithmic scale is NOT reasonable
  // we transform the min/max and tick spacing
  if (this->LogScale && !this->LogScaleReasonable)
    {
    // We need to handle value 0 for logarithmic function
    if (min < 1.0e-20)
      {
        min = pow(double(10.0), (floor(oldmin)));
      }
    if (max < 1.0e-20)
      {
        max = pow(double(10.0), (floor(oldmax)));
      }
    min = log10(min);
    max = log10(max);
    niceTickSpacing = log10(niceTickSpacing);
    }

  if (this->NumberOfTicks > 0)
    {
    return newRange / double(this->NumberOfTicks - 1);
    }
  else
    {
    return niceTickSpacing;
    }
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
  double result(0.0);
  niceValue = false;
  // We need to retrive the order of our number.
  order = static_cast<int>(floor(log10(number)));

  // We retrive the basis of our number depending on roundUp and return it as
  // result.
  number = number * pow(10.0, static_cast<double>(order*(-1)));
  result = roundUp ? ceil(number) : floor(number);

  // If result is 1.0, 2.0 or 5.0 we mark the result as "nice value".
  if (result == 1.0 || result == 2.0 || result == 5.0)
    {
    niceValue = true;
    }
  return result;
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

    // Now create a label for the tick position
    vtksys_ios::ostringstream ostr;
    ostr.imbue(vtkstd::locale::classic());
    if (this->Notation > 0)
      {
      ostr.precision(this->Precision);
      }
    if (this->Notation == 1)
      {
      // Scientific notation
      ostr.setf(vtksys_ios::ios::scientific, vtksys_ios::ios::floatfield);
      }
    else if (this->Notation == 2)
      {
      ostr.setf(ios::fixed, ios::floatfield);
      }
    ostr << value;

    if (niceTickMark)
      {
      this->TickLabels->InsertNextValue(ostr.str());
      }
    else
      {
      this->TickLabels->InsertNextValue("");
      }
    result += 1.0;
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
  os << indent << "Number of tick marks: " << this->NumberOfTicks << endl;

}
