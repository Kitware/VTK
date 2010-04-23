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
#include "vtkFloatArray.h"
#include "vtkDoubleArray.h"
#include "vtkStringArray.h"
#include "vtkStdString.h"
#include "vtksys/ios/sstream"

#include "vtkObjectFactory.h"

#include "math.h"

//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkAxis);

//-----------------------------------------------------------------------------
vtkAxis::vtkAxis()
{
  this->Position = vtkAxis::LEFT;
  this->Point1[0] = 0.0;
  this->Point1[1] = 0.0;
  this->Point2[0] = 10.0;
  this->Point2[1] = 10.0;
  this->TickInterval = 1.0;
  this->NumberOfTicks = 6;
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
  this->Title = NULL;
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
}

//-----------------------------------------------------------------------------
vtkAxis::~vtkAxis()
{
  this->SetTitle(NULL);
  this->TitleProperties->Delete();
  this->LabelProperties->Delete();
  this->Pen->Delete();
  this->GridPen->Delete();
}

//-----------------------------------------------------------------------------
void vtkAxis::Update()
{
  if (!this->Visible || this->BuildTime > this->MTime)
    {
    return;
    }

  // Figure out what type of behavior we should follow
  if (this->Behavior == 1)
    {
    this->RecalculateTickSpacing();
    }

  if (this->Behavior < 2 && this->TickMarksDirty)
    {
    // Regenerate the tick marks/positions if necessary
    // Calculate where the first tick mark should be drawn
    double first = ceil(this->Minimum / this->TickInterval) * this->TickInterval;
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
    vtkWarningMacro("The number of tick positions is not the same as the number "
                    << "of tick labels - error.");
    this->TickScenePositions->SetNumberOfTuples(0);
    return;
    }

  vtkIdType n = this->TickPositions->GetNumberOfTuples();
  this->TickScenePositions->SetNumberOfTuples(n);
  for (vtkIdType i = 0; i < n; ++i)
    {
    int iPos = static_cast<int>(origin +
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
  vtkTextProperty *prop = painter->GetTextProp();

  // Draw the axis title if there is one
  if (this->Title && this->Title[0])
    {
    int x = 0;
    int y = 0;
    painter->ApplyTextProp(this->TitleProperties);

    // Draw the axis label
    if (this->Position == vtkAxis::LEFT)
      {
      // Draw the axis label
      x = static_cast<int>(this->Point1[0] - 35);
      y = static_cast<int>(this->Point1[1] + this->Point2[1]) / 2;
      prop->SetOrientation(90.0);
      prop->SetVerticalJustificationToBottom();
      }
    else if (this->Position == vtkAxis::RIGHT)
      {
      // Draw the axis label
      x = static_cast<int>(this->Point1[0] + 45);
      y = static_cast<int>(this->Point1[1] + this->Point2[1]) / 2;
      prop->SetOrientation(90.0);
      prop->SetVerticalJustificationToTop();
      }
    else if (this->Position == vtkAxis::BOTTOM)
      {
      x = static_cast<int>(this->Point1[0] + this->Point2[0]) / 2;
      y = static_cast<int>(this->Point1[1] - 30);
      prop->SetOrientation(0.0);
      prop->SetVerticalJustificationToTop();
      }
    else if (this->Position == vtkAxis::TOP)
      {
      x = static_cast<int>(this->Point1[0] + this->Point2[0]) / 2;
      y = static_cast<int>(this->Point1[1] + 30);
      prop->SetOrientation(0.0);
      prop->SetVerticalJustificationToBottom();
      }
    else if (this->Position == vtkAxis::PARALLEL)
      {
      x = static_cast<int>(this->Point1[0]);
      y = static_cast<int>(this->Point1[1] - 10);
      prop->SetOrientation(0.0);
      prop->SetVerticalJustificationToTop();
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
    prop->SetJustificationToRight();
    prop->SetVerticalJustificationToCentered();

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
    prop->SetJustificationToLeft();
    prop->SetVerticalJustificationToCentered();

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
    prop->SetJustificationToCentered();
    prop->SetVerticalJustificationToTop();

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
    prop->SetJustificationToCentered();
    prop->SetVerticalJustificationToBottom();

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
void vtkAxis::SetMaximum(double maximum)
{
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
void vtkAxis::SetRange(double minimum, double maximum)
{
  this->SetMinimum(minimum);
  this->SetMaximum(maximum);
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
    double min, max;
    this->TickInterval = this->CalculateNiceMinMax(min, max);
    if (this->UsingNiceMinMax)
      {
      this->GenerateTickLabels(this->Minimum, this->Maximum);
      }
    else
      {
      while (min < this->Minimum)
        {
        min += this->TickInterval;
        }
      while (max > this->Maximum)
        {
        max -= this->TickInterval;
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
void vtkAxis::GenerateTickLabels(double min, double max)
{
  // Now calculate the tick labels, and positions within the axis range
  this->TickPositions->SetNumberOfTuples(0);
  this->TickLabels->SetNumberOfTuples(0);
  int n = static_cast<int>((max - min) / this->TickInterval);
  for (int i = 0; i <= n && i < 200; ++i)
    {
    double value = min + double(i) * this->TickInterval;
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
  this->TickMarksDirty = false;
}

//-----------------------------------------------------------------------------
double vtkAxis::CalculateNiceMinMax(double &min, double &max)
{
  // First get the order of the range of the numbers
  if (this->Maximum == this->Minimum)
    {
    this->Minimum *= 0.95;
    this->Maximum *= 1.05;
    }
  else if ((this->Maximum - this->Minimum) < 1.0e-20)
    {
    this->Minimum *= 0.95;
    this->Maximum *= 1.05;
    }

  double range = this->Maximum - this->Minimum;
  bool isNegative = false;
  if (range < 0.0f)
    {
    isNegative = true;
    range *= -1.0f;
    }

  // Calculate an upper limit on the number of tick marks - at least 30 pixels
  // should be between each tick mark.
  float pixelRange = this->Point1[0] == this->Point2[0] ?
                     this->Point2[1] - this->Point1[1] :
                     this->Point2[0] - this->Point1[0];
  int maxTicks = static_cast<int>(pixelRange / 50.0f);
  if (maxTicks == 0)
    {
    // The axes do not have a valid set of points - return
    return 0.0f;
    }
  double tickSpacing = range / maxTicks;

  int order = static_cast<int>(floor(log10(tickSpacing)));
  double normTickSpacing = tickSpacing * pow(10.0f, -order);
  double niceTickSpacing = this->NiceNumber(normTickSpacing, true);
  niceTickSpacing *= pow(10.0f, order);

  if (isNegative)
    {
    min = ceil(this->Minimum / niceTickSpacing) * niceTickSpacing;
    max = floor(this->Maximum / niceTickSpacing) * niceTickSpacing;
    }
  else
    {
    min = floor(this->Minimum / niceTickSpacing) * niceTickSpacing;
    max = ceil(this->Maximum / niceTickSpacing) * niceTickSpacing;
    }

  float newRange = max - min;
  this->NumberOfTicks = static_cast<int>(floor(newRange / niceTickSpacing)) + 1;

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
