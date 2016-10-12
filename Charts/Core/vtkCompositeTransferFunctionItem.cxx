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

#include "vtkCallbackCommand.h"
#include "vtkCommand.h"
#include "vtkImageData.h"
#include "vtkPiecewiseFunction.h"
#include "vtkColorTransferFunction.h"
#include "vtkCompositeTransferFunctionItem.h"
#include "vtkObjectFactory.h"
#include "vtkPen.h"
#include "vtkPointData.h"
#include "vtkPoints2D.h"

// STD includes
#include <algorithm>
#include <cassert>

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkCompositeTransferFunctionItem);

//-----------------------------------------------------------------------------
vtkCompositeTransferFunctionItem::vtkCompositeTransferFunctionItem()
{
  this->PolyLinePen->SetLineType(vtkPen::SOLID_LINE);
  this->OpacityFunction = 0;
}

//-----------------------------------------------------------------------------
vtkCompositeTransferFunctionItem::~vtkCompositeTransferFunctionItem()
{
  if (this->OpacityFunction)
  {
    this->OpacityFunction->RemoveObserver(this->Callback);
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
void vtkCompositeTransferFunctionItem::ComputeBounds(double* bounds)
{
  this->Superclass::ComputeBounds(bounds);
  if (this->OpacityFunction)
  {
    double* opacityRange = this->OpacityFunction->GetRange();
    bounds[0] = std::min(bounds[0], opacityRange[0]);
    bounds[1] = std::max(bounds[1], opacityRange[1]);
  }
}

//-----------------------------------------------------------------------------
void vtkCompositeTransferFunctionItem::SetOpacityFunction(vtkPiecewiseFunction* opacity)
{
  if (opacity == this->OpacityFunction)
  {
    return;
  }
  if (this->OpacityFunction)
  {
    this->OpacityFunction->RemoveObserver(this->Callback);
  }
  vtkSetObjectBodyMacro(OpacityFunction, vtkPiecewiseFunction, opacity);
  if (opacity)
  {
    opacity->AddObserver(vtkCommand::ModifiedEvent, this->Callback);
  }
  this->ScalarsToColorsModified(this->OpacityFunction, vtkCommand::ModifiedEvent, 0);
}

//-----------------------------------------------------------------------------
void vtkCompositeTransferFunctionItem::ComputeTexture()
{
  this->Superclass::ComputeTexture();
  double bounds[4];
  this->GetBounds(bounds);
  if (bounds[0] == bounds[1]
      || !this->OpacityFunction)
  {
    return;
  }
  if (this->Texture == 0)
  {
    this->Texture = vtkImageData::New();
  }

  const int dimension = this->GetTextureWidth();
  double* values = new double[dimension];
  this->OpacityFunction->GetTable(bounds[0], bounds[1], dimension, values);
  unsigned char* ptr =
    reinterpret_cast<unsigned char*>(this->Texture->GetScalarPointer(0,0,0));
  // TBD: maybe the shape should be defined somewhere else...
  if (this->MaskAboveCurve || this->PolyLinePen->GetLineType() != vtkPen::SOLID_LINE)
  {
    this->Shape->SetNumberOfPoints(dimension);
    double step = (bounds[1] - bounds[0]) / dimension;
    for (int i = 0; i < dimension; ++i)
    {
      ptr[3] = static_cast<unsigned char>(values[i] * this->Opacity * 255);
      if (values[i] < 0. || values[i] > 1.)
      {
        vtkWarningMacro( << "Opacity at point " << i << " is " << values[i]
                         << " which is outside the valid range of [0,1]");
      }
      this->Shape->SetPoint(i, bounds[0] + step * i, values[i]);
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
  delete [] values;
  return;
}
