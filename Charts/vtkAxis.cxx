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

#include "vtkObjectFactory.h"

#include "math.h"

//-----------------------------------------------------------------------------
vtkCxxRevisionMacro(vtkAxis, "1.6");

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkAxis);

//-----------------------------------------------------------------------------
vtkAxis::vtkAxis()
{
  this->Point1[0] = 10.0;
  this->Point1[1] = 10.0;
  this->Point2[0] = 10.0;
  this->Point2[1] = 10.0;
  this->NumberOfTicks = 6;
  this->TickLabelSize = 12;
  this->Minimum = 0.0;
  this->Maximum = 6.66;
  this->TitleSize = 15;
  this->Title = NULL;
}

//-----------------------------------------------------------------------------
vtkAxis::~vtkAxis()
{
  this->SetTitle(NULL);
}

//-----------------------------------------------------------------------------
bool vtkAxis::Paint(vtkContext2D *painter)
{
  // This is where everything should be drawn, or dispatched to other methods.
  vtkDebugMacro(<< "Paint event called in vtkAxis.");

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

  prop->SetFontSize(this->TickLabelSize);
  // Now draw the tick marks
  if (this->Point1[0] == this->Point2[0]) // x1 == x2, therefore vertical
    {
    float spacing = (this->Point2[1] - this->Point1[1]) /
                    float(this->NumberOfTicks - 1);
    prop->SetOrientation(0.0);
    prop->SetJustificationToRight();
    prop->SetVerticalJustificationToCentered();
    float labelSpacing = (this->Maximum - this->Minimum) /
                         float(this->NumberOfTicks - 1);
    for (int i = 0; i < this->NumberOfTicks; ++i)
      {
      int yTick = static_cast<int>(this->Point1[1] + float(i)*spacing);
      painter->DrawLine(this->Point1[0] - 5, yTick, this->Point1[0], yTick);

      // Draw the tick label
      char string[20];
      sprintf(string, "%-#6.3g", this->Minimum + i*labelSpacing);
      painter->DrawString(this->Point1[0] - 7, yTick, string);
      }
    }
  else // Default to horizontal orientation
    {
    float spacing = (this->Point2[0] - this->Point1[0]) /
                    float(this->NumberOfTicks - 1);
    prop->SetJustificationToCentered();
    prop->SetVerticalJustificationToTop();
    float labelSpacing = (this->Maximum - this->Minimum) /
                         float(this->NumberOfTicks - 1);
    for (int i = 0; i < this->NumberOfTicks; ++i)
      {
      int xTick = static_cast<int>(this->Point1[0] + float(i)*spacing);
      painter->DrawLine(xTick, this->Point1[1] - 5, xTick, this->Point1[1]);

      char string[20];
      sprintf(string, "%-#6.3g", this->Minimum + i*labelSpacing);
      painter->DrawString(xTick, int(this->Point1[1] - 7), string);
      }
    }
/*
  float min, max;
  min = max = 0.0f;
  this->CalculateNiceMinMax(min, max);
*/
  return true;
}

//-----------------------------------------------------------------------------
int vtkAxis::CalculateNiceMinMax(float &/*min*/, float &/*max*/)
{
  // First get the order of the range of the numbers
  if (this->Maximum == this->Minimum)
    {
    vtkWarningMacro(<< "Minimum and maximum values are equal - invalid.");
    return 0;
    }
  float range = this->Maximum - this->Minimum;
  bool isNegative = false;
  if (range < 0)
    {
    isNegative = true;
    range *= -1.0f;
    }
  int order = static_cast<int>(log10(range));
  cout << "Order of the range = " << order << endl;
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
