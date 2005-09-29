/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHierarchicalDataIterator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkHierarchicalDataIterator.h"

#include "vtkHierarchicalDataSet.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkHierarchicalDataIterator, "1.4");
vtkStandardNewMacro(vtkHierarchicalDataIterator);

//----------------------------------------------------------------------------
vtkHierarchicalDataIterator::vtkHierarchicalDataIterator()
{
}

//----------------------------------------------------------------------------
vtkHierarchicalDataIterator::~vtkHierarchicalDataIterator()
{
}

//----------------------------------------------------------------------------
vtkHierarchicalDataSet* vtkHierarchicalDataIterator::GetDataSet()
{
  return vtkHierarchicalDataSet::SafeDownCast(
    this->Superclass::GetDataSet());
}

//----------------------------------------------------------------------------
void vtkHierarchicalDataIterator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

