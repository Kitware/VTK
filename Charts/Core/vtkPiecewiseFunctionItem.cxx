/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPiecewiseFunctionItem.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkBrush.h"
#include "vtkCallbackCommand.h"
#include "vtkContext2D.h"
#include "vtkImageData.h"
#include "vtkPiecewiseFunction.h"
#include "vtkPiecewiseFunctionItem.h"
#include "vtkObjectFactory.h"
#include "vtkPen.h"
#include "vtkPointData.h"
#include "vtkPoints2D.h"

#include <cassert>

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPiecewiseFunctionItem);

//-----------------------------------------------------------------------------
vtkPiecewiseFunctionItem::vtkPiecewiseFunctionItem()
{
  this->PolyLinePen->SetLineType(vtkPen::SOLID_LINE);
  this->PiecewiseFunction = 0;
  this->SetColor(1., 1., 1.);
}

//-----------------------------------------------------------------------------
vtkPiecewiseFunctionItem::~vtkPiecewiseFunctionItem()
{
  if (this->PiecewiseFunction)
  {
    this->PiecewiseFunction->RemoveObserver(this->Callback);
    this->PiecewiseFunction->Delete();
    this->PiecewiseFunction = 0;
  }
}

//-----------------------------------------------------------------------------
void vtkPiecewiseFunctionItem::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "PiecewiseFunction: ";
  if (this->PiecewiseFunction)
  {
    os << endl;
    this->PiecewiseFunction->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << "(none)" << endl;
  }
}

//-----------------------------------------------------------------------------
void vtkPiecewiseFunctionItem::ComputeBounds(double* bounds)
{
  this->Superclass::ComputeBounds(bounds);
  if (this->PiecewiseFunction)
  {
    double* range = this->PiecewiseFunction->GetRange();
    bounds[0] = range[0];
    bounds[1] = range[1];
  }
}

//-----------------------------------------------------------------------------
void vtkPiecewiseFunctionItem::SetPiecewiseFunction(vtkPiecewiseFunction* t)
{
  if (t == this->PiecewiseFunction)
  {
    return;
  }
  if (this->PiecewiseFunction)
  {
    this->PiecewiseFunction->RemoveObserver(this->Callback);
  }
  vtkSetObjectBodyMacro(PiecewiseFunction, vtkPiecewiseFunction, t);
  if (t)
  {
    t->AddObserver(vtkCommand::ModifiedEvent, this->Callback);
  }
  this->ScalarsToColorsModified(this->PiecewiseFunction, vtkCommand::ModifiedEvent, 0);
}

//-----------------------------------------------------------------------------
void vtkPiecewiseFunctionItem::ComputeTexture()
{
  double bounds[4];
  this->GetBounds(bounds);
   if (bounds[0] == bounds[1]
       || !this->PiecewiseFunction)
   {
    return;
   }
  if (this->Texture == 0)
  {
    this->Texture = vtkImageData::New();
  }

  const int dimension = this->GetTextureWidth();
  double* values = new double[dimension];
  // should depends on the true size on screen
  this->Texture->SetExtent(0, dimension-1,
                           0, 0,
                           0, 0);
  this->Texture->AllocateScalars(VTK_UNSIGNED_CHAR, 4);

  this->PiecewiseFunction->GetTable(bounds[0], bounds[1], dimension,  values);
  unsigned char* ptr =
    reinterpret_cast<unsigned char*>(this->Texture->GetScalarPointer(0,0,0));
  if (this->MaskAboveCurve || this->PolyLinePen->GetLineType() != vtkPen::NO_PEN)
  {
    this->Shape->SetNumberOfPoints(dimension);
    double step = (bounds[1] - bounds[0]) / dimension;
    for (int i = 0; i < dimension; ++i)
    {
      this->Pen->GetColor(ptr);
      ptr[3] = static_cast<unsigned char>(values[i] * this->Opacity * 255 + 0.5);
      assert(values[i] <= 1. && values[i] >= 0.);
      this->Shape->SetPoint(i, bounds[0] + step * i, values[i]);
      ptr+=4;
    }
    this->Shape->Modified();
  }
  else
  {
    for (int i = 0; i < dimension; ++i)
    {
      this->Pen->GetColor(ptr);
      ptr[3] = static_cast<unsigned char>(values[i] * this->Opacity * 255 + 0.5);
      assert(values[i] <= 1. && values[i] >= 0.);
      ptr+=4;
    }
  }
  delete[] values;
  return;
}
