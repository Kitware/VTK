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
#include "vtkContext2D.h"
#include "vtkImageData.h"
#include "vtkColorTransferFunction.h"
#include "vtkColorTransferFunctionItem.h"
#include "vtkObjectFactory.h"
#include "vtkPen.h"
#include "vtkPointData.h"

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
void vtkColorTransferFunctionItem::SetColorTransferFunction(vtkColorTransferFunction* t)
{
  vtkSetObjectBodyMacro(ColorTransferFunction, vtkColorTransferFunction, t);
}

//-----------------------------------------------------------------------------
void vtkColorTransferFunctionItem::ComputeTexture()
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
  this->ColorTransferFunction->GetRange(range);
  if (range[0] == range[1])
    {
    vtkWarningMacro(<< "The color transfer function seems empty");
    return;
    }
  unsigned char* ptr =
    reinterpret_cast<unsigned char*>(this->Texture->GetScalarPointer(0,0,0));
  for (int i = 0; i < dimension; ++i)
    {
    values[i] = range[0] + i * (range[1] - range[0]) / dimension;
    ptr[3] = this->Opacity * 255;
    ptr+=4;
    }
  this->ColorTransferFunction->MapScalarsThroughTable2(
    values,
    reinterpret_cast<unsigned char*>(this->Texture->GetScalarPointer(0,0,0)),
    VTK_DOUBLE,dimension,1,4);

}
