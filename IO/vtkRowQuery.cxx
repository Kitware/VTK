/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRowQuery.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/
#include "vtkRowQuery.h"

#include "vtkObjectFactory.h"
#include "vtkVariantArray.h"

vtkCxxRevisionMacro(vtkRowQuery, "1.2");

vtkRowQuery::vtkRowQuery()
{
}

vtkRowQuery::~vtkRowQuery()
{
}

void vtkRowQuery::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

int vtkRowQuery::GetFieldIndex(char* name)
{
  int index;
  bool found = false;
  for (index = 0; index < this->GetNumberOfFields(); index++)
    {
    if (!strcmp(name, this->GetFieldName(index)))
      {
      found = true;
      break;
      }
    }
  if (found)
    {
    return index;
    }
  return -1;
}


bool vtkRowQuery::NextRow(vtkVariantArray* rowArray)
{
  if (!this->NextRow())
    {
    return false;
    }
  rowArray->Reset();
  for (int col = 0; col < this->GetNumberOfFields(); col++)
    {
    rowArray->InsertNextValue(this->DataValue(col));
    }
  return true;
}

