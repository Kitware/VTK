/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLineMark.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkLineMark.h"

#include "vtkBrush.h"
#include "vtkContext2D.h"
#include "vtkMarkUtil.h"
#include "vtkObjectFactory.h"
#include "vtkPen.h"

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkLineMark);

//-----------------------------------------------------------------------------
vtkLineMark::vtkLineMark()
{
  this->SetLineColor(vtkMarkUtil::DefaultSeriesColorFromParent);
  this->SetLineWidth(2);
}

//-----------------------------------------------------------------------------
vtkLineMark::~vtkLineMark()
{
}

//-----------------------------------------------------------------------------
bool vtkLineMark::Hit(const vtkContextMouseEvent &)
{
  return false;
}

//-----------------------------------------------------------------------------
bool vtkLineMark::Paint(vtkContext2D *painter)
{
  double* left = this->Left.GetArray(this);
  double* bottom = this->Bottom.GetArray(this);
  vtkColor* lineColor = this->LineColor.GetArray(this);
  double* lineWidth = this->LineWidth.GetArray(this);

  vtkIdType numChildren = this->Data.GetData(this).GetNumberOfChildren();
  if (numChildren > 0)
    {
    painter->GetPen()->SetWidth(lineWidth[0]);
    painter->GetPen()->SetColor(
      static_cast<unsigned char>(255*lineColor[0].Red),
      static_cast<unsigned char>(255*lineColor[0].Green),
      static_cast<unsigned char>(255*lineColor[0].Blue),
      static_cast<unsigned char>(255*lineColor[0].Alpha));
    }
  for (vtkIdType i = 1; i < numChildren; ++i)
    {
    painter->DrawLine(left[i-1], bottom[i-1], left[i], bottom[i]);
    }
  return true;
}

//-----------------------------------------------------------------------------
void vtkLineMark::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
