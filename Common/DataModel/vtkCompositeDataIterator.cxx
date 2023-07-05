// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkObjectFactory.h"

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
vtkCompositeDataIterator::vtkCompositeDataIterator()
{
  this->Reverse = 0;
  this->SkipEmptyNodes = 1;
  this->DataSet = nullptr;
}

//------------------------------------------------------------------------------
vtkCompositeDataIterator::~vtkCompositeDataIterator()
{
  this->SetDataSet(nullptr);
}

//------------------------------------------------------------------------------
void vtkCompositeDataIterator::SetDataSet(vtkCompositeDataSet* ds)
{
  vtkSetObjectBodyMacro(DataSet, vtkCompositeDataSet, ds);
  if (ds)
  {
    this->GoToFirstItem();
  }
}

//------------------------------------------------------------------------------
void vtkCompositeDataIterator::InitTraversal()
{
  this->SetReverse(0);
  this->GoToFirstItem();
}

//------------------------------------------------------------------------------
void vtkCompositeDataIterator::InitReverseTraversal()
{
  this->SetReverse(1);
  this->GoToFirstItem();
}

//------------------------------------------------------------------------------
void vtkCompositeDataIterator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Reverse: " << (this->Reverse ? "On" : "Off") << endl;
  os << indent << "SkipEmptyNodes: " << (this->SkipEmptyNodes ? "On" : "Off") << endl;
}
VTK_ABI_NAMESPACE_END
