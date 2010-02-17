/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBarMark.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkBarMark.h"

#include "vtkBrush.h"
#include "vtkContext2D.h"
#include "vtkMarkUtil.h"
#include "vtkObjectFactory.h"
#include "vtkPen.h"

//-----------------------------------------------------------------------------
vtkCxxRevisionMacro(vtkBarMark, "1.2");
vtkStandardNewMacro(vtkBarMark);

//-----------------------------------------------------------------------------
vtkBarMark::vtkBarMark()
{
  this->SetFillColor(vtkMarkUtil::DefaultSeriesColorFromParent);
  this->SetLineWidth(1);
  this->SetLineColor(vtkColor(0.0, 0.0, 0.0, 1.0));
}

//-----------------------------------------------------------------------------
vtkBarMark::~vtkBarMark()
{
}

//-----------------------------------------------------------------------------
bool vtkBarMark::Hit(const vtkContextMouseEvent &)
{
  return false;
}

unsigned char ConvertColor(double d)
{
  return static_cast<unsigned char>(255*d);
}

//-----------------------------------------------------------------------------
bool vtkBarMark::Paint(vtkContext2D *painter)
{
  double* left = this->Left.GetArray(this);
  double* bottom = this->Bottom.GetArray(this);
  double* width = this->Width.GetArray(this);
  double* height = this->Height.GetArray(this);
  vtkColor* fillColor = this->FillColor.GetArray(this);
  vtkColor* lineColor = this->LineColor.GetArray(this);
  double* lineWidth = this->LineWidth.GetArray(this);
  vtkIdType numChildren = this->Data.GetData(this).GetNumberOfChildren();
  for (vtkIdType i = 0; i < numChildren; ++i)
    {
    painter->GetBrush()->SetColor(ConvertColor(fillColor[i].Red),
                                  ConvertColor(fillColor[i].Green),
                                  ConvertColor(fillColor[i].Blue),
                                  ConvertColor(fillColor[i].Alpha));
    painter->GetPen()->SetColor(ConvertColor(lineColor[i].Red),
                                ConvertColor(lineColor[i].Green),
                                ConvertColor(lineColor[i].Blue),
                                ConvertColor(lineColor[i].Alpha));
    if (lineWidth[i] > 0.0)
      {
      painter->GetPen()->SetWidth(lineWidth[i]);
      }
    else
      {
      painter->GetPen()->SetOpacity(0);
      }
    painter->DrawRect(left[i], bottom[i], width[i], height[i]);
    }
  return true;
}

//-----------------------------------------------------------------------------
void vtkBarMark::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
