/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCompositeDataIterator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCompositeDataIterator.h"

vtkCxxRevisionMacro(vtkCompositeDataIterator, "1.3");

//----------------------------------------------------------------------------
vtkCompositeDataIterator::vtkCompositeDataIterator()
{
  this->VisitOnlyLeaves = 1;
}

//----------------------------------------------------------------------------
vtkCompositeDataIterator::~vtkCompositeDataIterator()
{
}

//----------------------------------------------------------------------------
void vtkCompositeDataIterator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "VisitOnlyLeaves: " 
     << (this->VisitOnlyLeaves?"(on)":"(off)")
     << endl;
}

