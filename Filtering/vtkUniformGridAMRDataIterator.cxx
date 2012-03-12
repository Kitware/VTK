/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkUniformGridAMRDataIterator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkUniformGridAMRDataIterator.h"

#include "vtkCompositeDataSetInternals.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkUniformGridAMRDataIterator);
//----------------------------------------------------------------------------
vtkUniformGridAMRDataIterator::vtkUniformGridAMRDataIterator()
{
}

//----------------------------------------------------------------------------
vtkUniformGridAMRDataIterator::~vtkUniformGridAMRDataIterator()
{
}

//----------------------------------------------------------------------------
unsigned int vtkUniformGridAMRDataIterator::GetCurrentLevel()
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
unsigned int vtkUniformGridAMRDataIterator::GetCurrentIndex()
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
void vtkUniformGridAMRDataIterator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

