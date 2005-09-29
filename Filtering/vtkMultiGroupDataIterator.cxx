/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMultiGroupDataIterator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkMultiGroupDataIterator.h"

#include "vtkMultiGroupDataSet.h"
#include "vtkMultiGroupDataSetInternal.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkMultiGroupDataIterator, "1.1");
vtkStandardNewMacro(vtkMultiGroupDataIterator);

class vtkMultiGroupDataIteratorInternal
{
public:
  // We store two iterators.
  // DSIterator (DataSets) iterators over the groups
  // LDSIteator (GroupDataSets) iterates over the nodes in the current group
  vtkMultiGroupDataSetInternal::GroupDataSetsIterator LDSIterator;
  vtkMultiGroupDataSetInternal::DataSetsIterator DSIterator;
};

//----------------------------------------------------------------------------
vtkMultiGroupDataIterator::vtkMultiGroupDataIterator()
{
  this->DataSet = 0;
  this->Internal = new vtkMultiGroupDataIteratorInternal;
}

//----------------------------------------------------------------------------
vtkMultiGroupDataIterator::~vtkMultiGroupDataIterator()
{
  this->SetDataSet(0);
  delete this->Internal;
}

//----------------------------------------------------------------------------
void vtkMultiGroupDataIterator::SetDataSet(vtkMultiGroupDataSet* dataset)
{
  if (this->DataSet != dataset)
    {
    if (this->DataSet) 
      { 
      this->DataSet->UnRegister(this); 
      }
    this->DataSet = dataset;
    if (this->DataSet) 
      { 
      this->DataSet->Register(this); 
      this->GoToFirstItem();
      }
    this->Modified();
    }   
}

//----------------------------------------------------------------------------
void vtkMultiGroupDataIterator::GoToFirstItem()
{
  if (!this->DataSet)
    {
    vtkErrorMacro("No data object has been set.");
    return;
    }
  // Initialize to the first group
  this->Internal->DSIterator = this->DataSet->Internal->DataSets.begin();
  if ( !this->DataSet->Internal->DataSets.empty() )
    {
    // Initialize to the first node in the first group
    this->Internal->LDSIterator = this->Internal->DSIterator->begin();
    if (this->Internal->LDSIterator == this->Internal->DSIterator->end())
      {
      this->GoToNextNonEmptyGroup();
      }
    // Skip nodes with NULL dataset pointers.
    if (!this->IsDoneWithTraversal() && !this->GetCurrentDataObject())
      {
      this->GoToNextItem();
      }
    }
}

//----------------------------------------------------------------------------
void vtkMultiGroupDataIterator::GoToNextNonEmptyGroup()
{
  if (!this->IsDoneWithTraversal())
    {
    while (1)
      {
      this->Internal->DSIterator++;
      if (this->IsDoneWithTraversal())
        {
        break;
        }
      this->Internal->LDSIterator = this->Internal->DSIterator->begin();
      if (this->Internal->LDSIterator != this->Internal->DSIterator->end())
        {
        break;
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkMultiGroupDataIterator::GoToNextItem()
{
  if (!this->DataSet)
    {
    vtkErrorMacro("No data object has been set.");
    return;
    }
  if (!this->IsDoneWithTraversal())
    {
    // In case the first group is empty
    if (this->Internal->LDSIterator == this->Internal->DSIterator->end())
      {
      this->GoToNextNonEmptyGroup();
      if (this->IsDoneWithTraversal())
        {
        return;
        }
      }

    this->Internal->LDSIterator++;
    if (this->Internal->LDSIterator == this->Internal->DSIterator->end())
      {
      this->GoToNextNonEmptyGroup();
      if (this->IsDoneWithTraversal())
        {
        return;
        }
      }
    // Skip nodes with NULL dataset pointers.
    if (!this->GetCurrentDataObject())
      {
      this->GoToNextItem();
      }
    }
}

//----------------------------------------------------------------------------
int vtkMultiGroupDataIterator::IsDoneWithTraversal()
{
  if (!this->DataSet)
    {
    vtkErrorMacro("No data object has been set.");
    return 1;
    }

  if ( this->DataSet->Internal->DataSets.empty() || 
       this->Internal->DSIterator == this->DataSet->Internal->DataSets.end() )
    {
    return 1;
    }
  return 0;
}

//----------------------------------------------------------------------------
vtkDataObject* vtkMultiGroupDataIterator::GetCurrentDataObject()
{
  if ( !this->DataSet || this->DataSet->Internal->DataSets.empty() )
    {
    return 0;
    }
  return this->Internal->LDSIterator->GetPointer();
}

//----------------------------------------------------------------------------
void vtkMultiGroupDataIterator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "DataSet: ";
  if (this->DataSet)
    {
    os << endl;
    this->DataSet->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "(none)" << endl;
    }
}

