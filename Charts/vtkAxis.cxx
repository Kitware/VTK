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

#include "vtkObjectFactory.h"

#include "math.h"

//-----------------------------------------------------------------------------
vtkCxxRevisionMacro(vtkAxis, "1.14");

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
  this->TickLabelSize = 12;
  this->Minimum = 0.0;
  this->Maximum = 6.66;
  this->TitleSize = 15;
  this->Title = NULL;
  this->LogScale = false;
  this->ShowGrid = true;
  this->TickPositions = vtkFloatArray::New();
  this->TickLabels = vtkStringArray::New();
}

//-----------------------------------------------------------------------------
vtkAxis::~vtkAxis()
{
  this->SetTitle(NULL);
  this->TickPositions->Delete();
  this->TickPositions = NULL;
  this->TickLabels->Delete();
  this->TickLabels = NULL;
}

//-----------------------------------------------------------------------------
void vtkAxis::Update()
{
  this->TickPositions->SetNumberOfTuples(0);
  this->TickLabels->SetNumberOfTuples(0);
  // Calculate where the first tick mark should be drawn
  float tick = ceil(this->Minimum / this->TickInterval) * this->TickInterval;

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
    char string[20];
    if (this->LogScale)
      {
      sprintf(string, "%#6.3g", pow(double(10.0), double(tick)));
      }
    else
      {
      sprintf(string, "%#6.3g", tick);
      }
    this->TickLabels->InsertNextValue(string);

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

  painter->GetPen()->SetWidth(1.0);
  // Draw this axis
  painter->DrawLine(this->Point1[0], this->Point1[1],
                    this->Point2[0], this->Point2[1]);
  vtkTextProperty *prop = painter->GetTextProp();

  // Draw the axis title if there is one
  if (this->Title)
    {
    int x = 0;
    int y = 0;
    prop->SetFontSize(this->TitleSize);
    prop->SetColor(0.0, 0.0, 0.0);
    prop->SetJustificationToCentered();
    // Draw the axis label
    if (this->Point1[0] == this->Point2[0]) // x1 == x2, therefore vertical
      {
      // Draw the axis label
      x = static_cast<int>(this->Point1[0] - 45);
      y = static_cast<int>(this->Point1[1] + this->Point2[1]) / 2;
      prop->SetOrientation(90.0);
      prop->SetVerticalJustificationToBottom();
      }
    else
      {
      x = static_cast<int>(this->Point1[0] + this->Point2[0]) / 2;
      y = static_cast<int>(this->Point1[1] - 30);
      prop->SetOrientation(0.0);
      prop->SetVerticalJustificationToTop();
      }
    painter->DrawString(x, y, this->Title);
    }

  // Now draw the tick marks
  prop->SetFontSize(this->TickLabelSize);
  prop->SetOrientation(0.0);

  float *tickPos = this->TickPositions->GetPointer(0);
  vtkStdString *tickLabel = this->TickLabels->GetPointer(0);
  vtkIdType numMarks = this->TickPositions->GetNumberOfTuples();

  if (this->Point1[0] == this->Point2[0]) // x1 == x2, therefore vertical
    {
    prop->SetJustificationToRight();
    prop->SetVerticalJustificationToCentered();

    // Draw the tick marks and labels
    for (vtkIdType i = 0; i < numMarks; ++i)
      {
      painter->DrawLine(this->Point1[0] - 5, tickPos[i],
                        this->Point1[0],     tickPos[i]);
      painter->DrawString(this->Point1[0] - 7, tickPos[i], tickLabel[i]);
      }
    }
  else // Default to horizontal orientation
    {
    prop->SetJustificationToCentered();
    prop->SetVerticalJustificationToTop();

    // Draw the tick marks and labels
    for (vtkIdType i = 0; i < numMarks; ++i)
      {
      painter->DrawLine(tickPos[i], this->Point1[1] - 5,
                        tickPos[i], this->Point1[1]);
      painter->DrawString(tickPos[i], int(this->Point1[1] - 7), tickLabel[i]);
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
    vtkWarningMacro(<< "Minimum and maximum values are equal - invalid.");
    return 0.0f;
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
