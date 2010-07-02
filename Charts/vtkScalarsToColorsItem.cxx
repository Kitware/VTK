/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkScalarsToColorsItem.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkBrush.h"
#include "vtkContext2D.h"
#include "vtkImageData.h"
#include "vtkScalarsToColorsItem.h"
#include "vtkObjectFactory.h"
#include "vtkPen.h"
#include "vtkPoints2D.h"

#include <cassert>

//-----------------------------------------------------------------------------
vtkScalarsToColorsItem::vtkScalarsToColorsItem()
{
  this->Texture = 0;
  this->Interpolate = true;
  this->Shape = vtkPoints2D::New();
  this->Shape->SetDataTypeToFloat();
  this->Shape->SetNumberOfPoints(4);
  this->Shape->SetPoint(0, 0.f, 0.f);
  this->Shape->SetPoint(1, 100.f, 0.f);
  this->Shape->SetPoint(2, 100.f, 100.f);
  this->Shape->SetPoint(3, 0.f, 100.f);
}

//-----------------------------------------------------------------------------
vtkScalarsToColorsItem::~vtkScalarsToColorsItem()
{
  if (this->Texture)
    {
    this->Texture->Delete();
    this->Texture = 0;
    }
  if (this->Shape)
    {
    this->Shape->Delete();
    this->Shape = 0;
    }
}

//-----------------------------------------------------------------------------
void vtkScalarsToColorsItem::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Interpolate: " << this->Interpolate << endl;
}

//-----------------------------------------------------------------------------
bool vtkScalarsToColorsItem::Paint(vtkContext2D* painter)
{
  if (this->Texture == 0 ||
      this->Texture->GetMTime() < this->GetMTime())
    {
    this->ComputeTexture();
    }
  painter->GetPen()->SetLineType(vtkPen::NO_PEN);
  painter->GetBrush()->SetColorF(1., 1., 1., 1.);
  painter->GetBrush()->SetTexture(this->Texture);
  painter->GetBrush()->SetTextureProperties(
    (this->Interpolate ? vtkBrush::Nearest : vtkBrush::Linear) |
    vtkBrush::Stretch);
  painter->DrawPolygon(this->Shape);
}
