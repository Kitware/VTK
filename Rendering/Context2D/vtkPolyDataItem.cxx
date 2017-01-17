/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolyDataItem.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkAbstractMapper.h"
#include "vtkContext2D.h"
#include "vtkPolyData.h"
#include "vtkObjectFactory.h"
#include "vtkPolyDataItem.h"


vtkStandardNewMacro(vtkPolyDataItem);

vtkCxxSetObjectMacro(vtkPolyDataItem, PolyData, vtkPolyData);

vtkCxxSetObjectMacro(vtkPolyDataItem, MappedColors, vtkUnsignedCharArray);

//-----------------------------------------------------------------------------
vtkPolyDataItem::vtkPolyDataItem()
: PolyData(NULL)
, MappedColors(NULL)
, ScalarMode(VTK_SCALAR_MODE_USE_POINT_DATA)
{
  this->Position[0] = this->Position[1] = 0;
}

//-----------------------------------------------------------------------------
vtkPolyDataItem::~vtkPolyDataItem()
{
  this->SetPolyData(NULL);
  this->SetMappedColors(NULL);
}

//-----------------------------------------------------------------------------
bool vtkPolyDataItem::Paint(vtkContext2D* painter)
{
  if (this->PolyData && this->MappedColors)
  {
    // Draw the PolyData in the bottom left corner of the item.
    painter->DrawPolyData(this->Position[0], this->Position[1], this->PolyData,
      this->MappedColors, this->ScalarMode);
  }

  return true;
}

//-----------------------------------------------------------------------------
void vtkPolyDataItem::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os, indent);
}
