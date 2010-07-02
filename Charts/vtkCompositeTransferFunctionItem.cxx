/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCompositeTransferFunctionItem.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkImageData.h"
#include "vtkPiecewiseFunction.h"
#include "vtkCompositeTransferFunctionItem.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints2D.h"

#include <cassert>

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkCompositeTransferFunctionItem);

//-----------------------------------------------------------------------------
vtkCompositeTransferFunctionItem::vtkCompositeTransferFunctionItem()
{
  this->OpacityFunction = 0;
  this->MaskAboveCurve = false;
}

//-----------------------------------------------------------------------------
vtkCompositeTransferFunctionItem::~vtkCompositeTransferFunctionItem()
{
  if (this->OpacityFunction)
    {
    this->OpacityFunction->Delete();
    this->OpacityFunction = 0;
    }
}

//-----------------------------------------------------------------------------
void vtkCompositeTransferFunctionItem::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "CompositeTransferFunction: ";
  if (this->OpacityFunction)
    {
    os << endl;
    this->OpacityFunction->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "(none)" << endl;
    }
}

//-----------------------------------------------------------------------------
void vtkCompositeTransferFunctionItem::SetOpacityFunction(vtkPiecewiseFunction* opacity)
{
  vtkSetObjectBodyMacro(OpacityFunction, vtkPiecewiseFunction, opacity);
}

//-----------------------------------------------------------------------------
void vtkCompositeTransferFunctionItem::SetMaskAboveCurve(bool mask)
{
  if (mask == this->MaskAboveCurve)
    {
    return;
    }
  if (mask == false)
    {
    this->Shape->SetNumberOfPoints(4);
    this->Shape->SetPoint(0, 0.f, 0.f);
    this->Shape->SetPoint(1, 100.f, 0.f);
    this->Shape->SetPoint(2, 100.f, 100.f);
    this->Shape->SetPoint(3, 0.f, 100.f);
    }
  this->MaskAboveCurve = mask;
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkCompositeTransferFunctionItem::ComputeTexture()
{
  this->Superclass::ComputeTexture();
  // TODO: get the dimension from superclass
  const int dimension = 256;
  double values[256];
  // TBD: it doesn't makes much sense here
  double range[2];
  this->OpacityFunction->GetRange(range);
  if (range[0] == range[1])
    {
    vtkWarningMacro(<< "The piecewise function seems empty");
    return;
    }
  this->OpacityFunction->GetTable(range[0], range[1], dimension,  values);
  unsigned char* ptr =
    reinterpret_cast<unsigned char*>(this->Texture->GetScalarPointer(0,0,0));
  if (MaskAboveCurve)
    {
    this->Shape->SetNumberOfPoints(dimension+2);
    this->Shape->SetPoint(0, 0.f, 0.f);
    this->Shape->SetPoint(dimension + 1, 100.f, 0.f);
    for (int i = 0; i < dimension; ++i)
      {
      ptr[3] = static_cast<unsigned char>(values[i] * this->Opacity * 255);
      assert(values[i] <= 1. && values[i] >= 0.);
      this->Shape->SetPoint(i+1,
                            static_cast<float>(i) * 100.f / dimension,
                            values[i] * 100.f);
      ptr+=4;
      }
    }
  else
    {
    for (int i = 0; i < dimension; ++i)
      {
      ptr[3] = static_cast<unsigned char>(values[i] * this->Opacity * 255);
      assert(values[i] <= 1. && values[i] >= 0.);
      ptr+=4;
      }
    }
}
