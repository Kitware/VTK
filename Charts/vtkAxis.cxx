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

//-----------------------------------------------------------------------------
vtkCxxRevisionMacro(vtkAxis, "1.3");

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
  this->Minimum = 0.0;
  this->Maximum = 6.66;
  this->Label = NULL;
}

//-----------------------------------------------------------------------------
vtkAxis::~vtkAxis()
{
  this->SetLabel(NULL);
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

  // Draw the axis label
  if (this->Point1[0] == this->Point2[0]) // x1 == x2, therefore vertical
    {
    // Draw the axis label
    int x = static_cast<int>(this->Point1[0] - 45);
    int y = static_cast<int>(this->Point1[1] + this->Point2[1]) / 2;
    vtkTextProperty *prop = painter->GetTextProp();
    prop->SetFontSize(15);
    prop->SetFontFamilyAsString("Arial");
    prop->SetColor(0.0, 0.0, 0.0);
    prop->SetOrientation(90.0);
    prop->SetJustificationToCentered();
    prop->SetVerticalJustificationToBottom();
    painter->DrawText(x, y, this->Label);

    // Now draw the tick marks
    float spacing = (this->Point2[1] - this->Point1[1]) /
                    float(this->NumberOfTicks - 1);
    prop->SetOrientation(0.0);
    prop->SetJustificationToRight();
    prop->SetVerticalJustificationToCentered();
    float labelSpacing = (this->Maximum - this->Minimum) /
                         float(this->NumberOfTicks -1);
    for (int i = 0; i < this->NumberOfTicks; ++i)
      {
      int yTick = static_cast<int>(this->Point1[1] + float(i)*spacing);
      painter->DrawLine(this->Point1[0] - 5, yTick, this->Point1[0], yTick);

      // Draw the tick label
      char string[20];
      sprintf(string, "%-#6.3g", this->Minimum + i*labelSpacing);
      painter->DrawText(this->Point1[0] - 5, yTick, string);
      }
    }
  else // Default to horizontal orientation
    {
    int x = static_cast<int>(this->Point1[0] + this->Point2[0]) / 2;
    int y = static_cast<int>(this->Point1[1] - 30);
    vtkTextProperty *prop = painter->GetTextProp();
    prop->SetFontSize(15);
    prop->SetFontFamilyAsString("Arial");
    prop->SetColor(0.0, 0.0, 0.0);
    prop->SetJustificationToCentered();
    prop->SetVerticalJustificationToTop();
    painter->DrawText(x, y, "X Axis");

    // Now draw the tick marks
    float spacing = (this->Point2[0] - this->Point1[0]) /
                    float(this->NumberOfTicks - 1);
    prop->SetJustificationToCentered();
    prop->SetVerticalJustificationToTop();
    float labelSpacing = (this->Maximum - this->Minimum) /
                         float(this->NumberOfTicks -1);
    for (int i = 0; i < this->NumberOfTicks; ++i)
      {
      int xTick = static_cast<int>(this->Point1[1] + float(i)*spacing);
      painter->DrawLine(xTick, this->Point1[1] - 5, xTick, this->Point1[1]);

      char string[20];
      sprintf(string, "%-#6.3g", this->Minimum + i*labelSpacing);
      painter->DrawText(xTick, int(this->Point1[1] - 5), string);
      }
    }
  return true;
}

//-----------------------------------------------------------------------------
void vtkAxis::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Axis label: \"" << *this->Label << "\"" << endl;
  os << indent << "Minimum point: " << this->Point1[0] << ", "
     << this->Point1[1] << endl;
  os << indent << "Maximum point: " << this->Point2[0] << ", "
     << this->Point2[1] << endl;
  os << indent << "Range: " << this->Minimum << " - " << this->Maximum << endl;
  os << indent << "Number of tick marks: " << this->NumberOfTicks << endl;

}
