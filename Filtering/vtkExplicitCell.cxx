/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExplicitCell.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkExplicitCell.h"

#include "vtkCellArray.h"
#include "vtkObjectFactory.h"
#include "vtkDataSet.h"

vtkCxxSetObjectMacro(vtkExplicitCell,DataSet,vtkDataSet);

//----------------------------------------------------------------------------
vtkExplicitCell::vtkExplicitCell()
{
  this->CellId = -1;
  this->DataSet = NULL;
}

//----------------------------------------------------------------------------
void vtkExplicitCell::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Cell Id: " << this->CellId << "\n";
  os << indent << "Data Set: " << this->DataSet << "\n";
}
