/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataValue.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkDataValue.h"

#include "vtkMark.h"
#include "vtkPanelMark.h"

//-----------------------------------------------------------------------------
vtkDataElement vtkDataValue::GetData(vtkMark* m)
{
  if (this->Function)
    {
    vtkMark* p = m->GetParent();
    vtkDataElement d = p->GetData().GetData(p).GetChild(p->GetIndex());
    return this->Function(m, d);
    }
  return this->Constant;
}
