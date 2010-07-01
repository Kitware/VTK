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
#include "vtkContext2D.h"
#include "vtkImageData.h"
#include "vtkPiecewiseFunction.h"
#include "vtkPiecewiseFunctionItem.h"
#include "vtkObjectFactory.h"
#include "vtkPen.h"
#include "vtkPointData.h"

#include <cassert>

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPiecewiseFunctionItem);

//-----------------------------------------------------------------------------
vtkPiecewiseFunctionItem::vtkPiecewiseFunctionItem()
{
  this->PiecewiseFunction = 0;
  this->Color[0] = 255; this->Color[1] = 255; this->Color[2] = 255;
}

//-----------------------------------------------------------------------------
vtkPiecewiseFunctionItem::~vtkPiecewiseFunctionItem()
{
  if (this->PiecewiseFunction)
    {
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
void vtkPiecewiseFunctionItem::SetPiecewiseFunction(vtkPiecewiseFunction* t)
{
  vtkSetObjectBodyMacro(PiecewiseFunction, vtkPiecewiseFunction, t);
}

//-----------------------------------------------------------------------------
void vtkPiecewiseFunctionItem::ComputeTexture()
{

  if (this->Texture == 0)
    {
    this->Texture = vtkImageData::New();
    }
  const int dimension = 256;
  double values[256];
  // should depends on the true size on screen
  this->Texture->SetExtent(0, dimension-1,
                           0, 0,
                           0, 0);
  this->Texture->SetNumberOfScalarComponents(4);
  this->Texture->SetScalarTypeToUnsignedChar();
  this->Texture->AllocateScalars();
  double range[2];
  this->PiecewiseFunction->GetRange(range);
   if (range[0] == range[1])
    {
    vtkWarningMacro(<< "The piecewise function seems empty");
    return;
    }
  this->PiecewiseFunction->GetTable(range[0], range[1], dimension,  values);
  unsigned char* ptr =
    reinterpret_cast<unsigned char*>(this->Texture->GetScalarPointer(0,0,0));
  unsigned char color[3] = {255, 255, 255};
  for (int i = 0; i < dimension; ++i)
    {
    ptr[0] = this->Color[0];
    ptr[1] = this->Color[1];
    ptr[2] = this->Color[2];
    ptr[3] = static_cast<unsigned char>(values[i] * this->Opacity * 255);
    ptr+=4;
    }
}
