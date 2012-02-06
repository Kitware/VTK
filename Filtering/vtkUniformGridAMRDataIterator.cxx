/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHierarchicalBoxDataIterator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkHierarchicalBoxDataIterator.h"

#include "vtkCompositeDataSetInternals.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkHierarchicalBoxDataIterator);
//----------------------------------------------------------------------------
vtkHierarchicalBoxDataIterator::vtkHierarchicalBoxDataIterator()
{
}

//----------------------------------------------------------------------------
vtkHierarchicalBoxDataIterator::~vtkHierarchicalBoxDataIterator()
{
}

//----------------------------------------------------------------------------
unsigned int vtkHierarchicalBoxDataIterator::GetCurrentLevel()
{
  if (this->IsDoneWithTraversal())
    {
    vtkErrorMacro("IsDoneWithTraversal is true.");
    return 0;
    }

  vtkCompositeDataSetIndex index = this->Superclass::GetCurrentIndex();
  return index[0];
}

//----------------------------------------------------------------------------
unsigned int vtkHierarchicalBoxDataIterator::GetCurrentIndex()
{
  if (this->IsDoneWithTraversal())
    {
    vtkErrorMacro("IsDoneWithTraversal is true.");
    return 0;
    }

  vtkCompositeDataSetIndex index = this->Superclass::GetCurrentIndex();
  if (index.size()==2)
    {
    return index[1];
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkHierarchicalBoxDataIterator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

