/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLookupTableItem.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkCallbackCommand.h"
#include "vtkImageData.h"
#include "vtkLookupTable.h"
#include "vtkLookupTableItem.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints2D.h"

#include <cassert>

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkLookupTableItem);

//-----------------------------------------------------------------------------
vtkLookupTableItem::vtkLookupTableItem()
{
  this->Interpolate = false;
  this->LookupTable = 0;
}

//-----------------------------------------------------------------------------
vtkLookupTableItem::~vtkLookupTableItem()
{
  if (this->LookupTable)
    {
    this->LookupTable->Delete();
    this->LookupTable = 0;
    }
}

//-----------------------------------------------------------------------------
void vtkLookupTableItem::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "LookupTable: ";
  if (this->LookupTable)
    {
    os << endl;
    this->LookupTable->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "(none)" << endl;
    }
}

//-----------------------------------------------------------------------------
void vtkLookupTableItem::ComputeBounds(double* bounds)
{
  this->Superclass::ComputeBounds(bounds);
  if (this->LookupTable)
    {
    double* range = this->LookupTable->GetRange();
    bounds[0] = range[0];
    bounds[1] = range[1];
    }
}

//-----------------------------------------------------------------------------
void vtkLookupTableItem::SetLookupTable(vtkLookupTable* t)
{
  if (t == this->LookupTable)
    {
    return;
    }
  if (this->LookupTable)
    {
    this->LookupTable->RemoveObserver(this->Callback);
    }
  vtkSetObjectBodyMacro(LookupTable, vtkLookupTable, t);
  if (t)
    {
    t->AddObserver(vtkCommand::ModifiedEvent, this->Callback);
    }
  this->ScalarsToColorsModified(this->LookupTable, vtkCommand::ModifiedEvent, 0);
}

//-----------------------------------------------------------------------------
void vtkLookupTableItem::ComputeTexture()
{
  double bounds[4];
  this->GetBounds(bounds);
  if (bounds[0] == bounds[1]
      || !this->LookupTable)
    {
    return;
    }
  if (this->Texture == 0)
    {
    this->Texture = vtkImageData::New();
    }
  // Could depend of the screen resolution
  const int dimension = 256;
  double values[256];
  // Texture 1D
  this->Texture->SetExtent(0, dimension - 1,
                           0,0,
                           0,0);
  this->Texture->SetNumberOfScalarComponents(4);
  this->Texture->SetScalarTypeToUnsignedChar();
  this->Texture->AllocateScalars();
  // TODO: Support log scale ?
  for (int i = 0; i < dimension; ++i)
    {
    values[i] = bounds[0] + i * (bounds[1] - bounds[0]) / (dimension - 1);
    }
  unsigned char* ptr =
    reinterpret_cast<unsigned char*>(this->Texture->GetScalarPointer(0,0,0));
  this->LookupTable->MapScalarsThroughTable2(
    values, ptr, VTK_DOUBLE, dimension, 1, 4);
  if (this->Opacity != 1.)
    {
    for (int i = 0; i < dimension; ++i)
      {
      ptr[3] = static_cast<unsigned char>(this->Opacity * ptr[3]);
      ptr+=4;
      }
    }
  return;
}
