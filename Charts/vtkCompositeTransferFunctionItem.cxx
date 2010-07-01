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

#include <cassert>

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkCompositeTransferFunctionItem);

//-----------------------------------------------------------------------------
vtkCompositeTransferFunctionItem::vtkCompositeTransferFunctionItem()
{
  this->OpacityFunction = 0;
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
  for (int i = 0; i < dimension; ++i)
    {
    ptr[3] = static_cast<unsigned char>(values[i] * this->Opacity * 255);
    ptr+=4;
    }
}
