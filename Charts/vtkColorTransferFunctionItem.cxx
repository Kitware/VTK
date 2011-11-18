/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkColorTransferFunctionItem.cxx

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
#include "vtkColorTransferFunction.h"
#include "vtkColorTransferFunctionItem.h"
#include "vtkObjectFactory.h"
#include "vtkPen.h"
#include "vtkPointData.h"
#include "vtkPoints2D.h"

#include <cassert>

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkColorTransferFunctionItem);

//-----------------------------------------------------------------------------
vtkColorTransferFunctionItem::vtkColorTransferFunctionItem()
{
  this->ColorTransferFunction = 0;
}

//-----------------------------------------------------------------------------
vtkColorTransferFunctionItem::~vtkColorTransferFunctionItem()
{
  if (this->ColorTransferFunction)
    {
    this->ColorTransferFunction->Delete();
    this->ColorTransferFunction = 0;
    }
}

//-----------------------------------------------------------------------------
void vtkColorTransferFunctionItem::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "ColorTransferFunction: ";
  if (this->ColorTransferFunction)
    {
    os << endl;
    this->ColorTransferFunction->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "(none)" << endl;
    }
}

//-----------------------------------------------------------------------------
void vtkColorTransferFunctionItem::ComputeBounds(double* bounds)
{
  this->Superclass::ComputeBounds(bounds);
  if (this->ColorTransferFunction)
    {
    double* range = this->ColorTransferFunction->GetRange();
    bounds[0] = range[0];
    bounds[1] = range[1];
    }
}

//-----------------------------------------------------------------------------
void vtkColorTransferFunctionItem::SetColorTransferFunction(vtkColorTransferFunction* t)
{
  if (t == this->ColorTransferFunction)
    {
    return;
    }
  if (this->ColorTransferFunction)
    {
    this->ColorTransferFunction->RemoveObserver(this->Callback);
    }
  vtkSetObjectBodyMacro(ColorTransferFunction, vtkColorTransferFunction, t);
  if (t)
    {
    t->AddObserver(vtkCommand::ModifiedEvent, this->Callback);
    }
  this->ScalarsToColorsModified(t, vtkCommand::ModifiedEvent, 0);
}

//-----------------------------------------------------------------------------
void vtkColorTransferFunctionItem::ComputeTexture()
{
  double bounds[4];
  this->GetBounds(bounds);
  if (bounds[0] == bounds[1]
      || !this->ColorTransferFunction)
    {
    return;
    }
  if (this->Texture == 0)
    {
    this->Texture = vtkImageData::New();
    }

  // Could depend of the screen resolution
  const int dimension = this->GetTextureWidth();
  double* values = new double[dimension];
  // Texture 1D
  this->Texture->SetExtent(0, dimension-1,
                           0, 0,
                           0, 0);
  this->Texture->SetNumberOfScalarComponents(4);
  this->Texture->SetScalarTypeToUnsignedChar();
  this->Texture->AllocateScalars();

  for (int i = 0; i < dimension; ++i)
    {
    values[i] = bounds[0] + i * (bounds[1] - bounds[0]) / (dimension - 1);
    }
  unsigned char* ptr =
    reinterpret_cast<unsigned char*>(this->Texture->GetScalarPointer(0,0,0));
  this->ColorTransferFunction->MapScalarsThroughTable2(
    values, ptr, VTK_DOUBLE, dimension, VTK_LUMINANCE, VTK_RGBA);
  if (this->Opacity != 1.0)
    {
    for (int i = 0; i < dimension; ++i)
      {
      ptr[3] = static_cast<unsigned char>(this->Opacity * ptr[3]);
      ptr+=4;
      }
    }
  delete [] values;
  return;
}
