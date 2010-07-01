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

#include "vtkImageData.h"
#include "vtkLookupTable.h"
#include "vtkLookupTableItem.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"

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
void vtkLookupTableItem::SetLookupTable(vtkLookupTable* t)
{
  vtkSetObjectBodyMacro(LookupTable, vtkLookupTable, t);
}

//-----------------------------------------------------------------------------
void vtkLookupTableItem::ComputeTexture()
{
  if (this->Texture == 0)
    {
    this->Texture = vtkImageData::New();
    }

  this->Texture->SetExtent(0, this->LookupTable->GetNumberOfTableValues() - 1,
                           0,0,
                           0,0);
  this->Texture->SetNumberOfScalarComponents(4);
  this->Texture->SetScalarTypeToUnsignedChar();
  // this->Texture will share the same scalar array than this->LookupTable
  // TODO: Support log scale
  this->Texture->GetPointData()->SetScalars(this->LookupTable->GetTable());
}
