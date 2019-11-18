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
#include "vtkCompositeDataSet.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkCompositeDataIterator::vtkCompositeDataIterator()
{
  this->Reverse = 0;
  this->SkipEmptyNodes = 1;
  this->DataSet = nullptr;
}

//----------------------------------------------------------------------------
vtkCompositeDataIterator::~vtkCompositeDataIterator()
{
  this->SetDataSet(nullptr);
}

//----------------------------------------------------------------------------
void vtkCompositeDataIterator::SetDataSet(vtkCompositeDataSet* ds)
{
  vtkSetObjectBodyMacro(DataSet, vtkCompositeDataSet, ds);
  if (ds)
  {
    this->GoToFirstItem();
  }
}

//----------------------------------------------------------------------------
void vtkCompositeDataIterator::InitTraversal()
{
  this->SetReverse(0);
  this->GoToFirstItem();
}

//----------------------------------------------------------------------------
void vtkCompositeDataIterator::InitReverseTraversal()
{
  this->SetReverse(1);
  this->GoToFirstItem();
}

//----------------------------------------------------------------------------
void vtkCompositeDataIterator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Reverse: " << (this->Reverse ? "On" : "Off") << endl;
  os << indent << "SkipEmptyNodes: " << (this->SkipEmptyNodes ? "On" : "Off") << endl;
}
