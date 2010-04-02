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
#include "vtkStringArray.h"
#include "vtkStdString.h"
#include "vtksys/ios/sstream"

#include "vtkObjectFactory.h"

#include "math.h"

//-----------------------------------------------------------------------------
vtkCxxRevisionMacro(vtkAxis, "1.22");

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
  this->Notation = 2; // Fixed - do the right thing...
  this->Behavior = 0;
  this->Pen = vtkPen::New();
  this->Pen->SetColor(0, 0, 0);
  this->Pen->SetWidth(1.0);
  this->GridPen = vtkPen::New();
  this->GridPen->SetColor(242, 242, 242);
  this->GridPen->SetWidth(1.0);
  this->TickPositions = vtkFloatArray::New();
  this->TickLabels = vtkStringArray::New();
}

//-----------------------------------------------------------------------------
vtkAxis::~vtkAxis()
{
  this->SetTitle(NULL);
  this->TitleProperties->Delete();
  this->LabelProperties->Delete();
  this->TickPositions->Delete();
  this->TickPositions = NULL;
  this->TickLabels->Delete();
  this->TickLabels = NULL;
  this->Pen->Delete();
  this->GridPen->Delete();
}

//-----------------------------------------------------------------------------
void vtkAxis::Update()
{
  this->TickPositions->SetNumberOfTuples(0);
  this->TickLabels->SetNumberOfTuples(0);

  // Figure out what type of behavior we should follow
  if (this->Behavior == 1)
    {
    this->RecalculateTickSpacing();
    }

  // Calculate where the first tick mark should be drawn
  float tick = ceilf(this->Minimum / this->TickInterval) * this->TickInterval;

  // If this check is not used, and the first tick is at 0.0 then it has the
  // negative sign bit set. This check gets rid of the negative bit but is quite
  // possibly not the best way of doing it...
  if (tick == 0.0f)
    {
    tick = 0.0f;
    }

  float scaling = 0.0;
  float origin = 0.0;
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
  while (tick <= this->Maximum)
    {
    // Calculate the next tick mark position
    int iTick = static_cast<int>(origin + (tick - this->Minimum) * scaling);
    this->TickPositions->InsertNextValue(iTick);
    // Make a tick mark label for the tick
    float value = tick;
    if (this->LogScale)
      {
      value = pow(double(10.0), double(tick));
      }

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

    tick += this->TickInterval;
    }
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
  if (this->Title)
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

  float *tickPos = this->TickPositions->GetPointer(0);
  vtkStdString *tickLabel = this->TickLabels->GetPointer(0);
  vtkIdType numMarks = this->TickPositions->GetNumberOfTuples();

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
void vtkAxis::AutoScale()
{
  // Calculate the min and max, set the number of ticks and the tick spacing
  this->TickInterval = this->CalculateNiceMinMax(this->Minimum, this->Maximum);
}

//-----------------------------------------------------------------------------
void vtkAxis::RecalculateTickSpacing()
{
  // Calculate the min and max, set the number of ticks and the tick spacing,
  // discard the min and max in this case. TODO: Refactor the function called.
  float min, max;
  this->TickInterval = this->CalculateNiceMinMax(min, max);
}

//-----------------------------------------------------------------------------
float vtkAxis::CalculateNiceMinMax(float &min, float &max)
{
  // First get the order of the range of the numbers
  if (this->Maximum == this->Minimum)
    {
    this->Minimum *= 0.95;
    this->Maximum *= 1.05;
    }
  float range = this->Maximum - this->Minimum;
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
  float tickSpacing = range / maxTicks;

  int order = static_cast<int>(floor(log10(tickSpacing)));
  float normTickSpacing = tickSpacing * pow(10.0f, -order);
  float niceTickSpacing = this->NiceNumber(normTickSpacing, true);
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
float vtkAxis::NiceNumber(float n, bool roundUp)
{
  if (roundUp)
    {
    if (n <= 1.0f)
      {
      return 1.0f;
      }
    else if (n <= 2.0f)
      {
      return 2.0f;
      }
    else if (n <= 5.0f)
      {
      return 5.0f;
      }
    else
      {
      return 10.0f;
      }
    }
  else
    {
    if (n < 1.5f)
      {
      return 1.0f;
      }
    else if (n <= 3.0f)
      {
      return 2.0f;
      }
    else if (n <= 7.0f)
      {
      return 5.0f;
      }
    else
      {
      return 10.0f;
      }
    }
}

//-----------------------------------------------------------------------------
void vtkAxis::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Axis title: \"" << *this->Title << "\"" << endl;
  os << indent << "Minimum point: " << this->Point1[0] << ", "
     << this->Point1[1] << endl;
  os << indent << "Maximum point: " << this->Point2[0] << ", "
     << this->Point2[1] << endl;
  os << indent << "Range: " << this->Minimum << " - " << this->Maximum << endl;
  os << indent << "Number of tick marks: " << this->NumberOfTicks << endl;

}
