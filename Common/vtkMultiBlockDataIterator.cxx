/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMultiBlockDataIterator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkMultiBlockDataIterator.h"

#include "vtkMultiBlockDataSet.h"
#include "vtkMultiBlockDataSetInternal.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkMultiBlockDataIterator, "1.2");
vtkStandardNewMacro(vtkMultiBlockDataIterator);

// Note that this class is dependent on the implementation
// of the data structure in vtkMultiBlockDataSet. Changes
// to vtkMultiBlockDataSet might require changes to this class.
class vtkMultiBlockDataIteratorInternal
{
public:
  vtkstd::vector< vtkSmartPointer<vtkDataObject> >::iterator Iterator;
};

//----------------------------------------------------------------------------
vtkMultiBlockDataIterator::vtkMultiBlockDataIterator()
{
  this->DataSet = 0;
  this->Internal = new vtkMultiBlockDataIteratorInternal;
}

//----------------------------------------------------------------------------
vtkMultiBlockDataIterator::~vtkMultiBlockDataIterator()
{
  this->SetDataSet(0);
  delete this->Internal;
}

//----------------------------------------------------------------------------
void vtkMultiBlockDataIterator::SetDataSet(vtkMultiBlockDataSet* dataset)
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
void vtkMultiBlockDataIterator::GoToFirstItem()
{
  if (!this->DataSet)
    {
    vtkErrorMacro("No data object has been set.");
    return;
    }
  // Simply use the STL iterator from the vector
  this->Internal->Iterator = this->DataSet->Internal->DataSets.begin();
}

//----------------------------------------------------------------------------
void vtkMultiBlockDataIterator::GoToNextItem()
{
  if (!this->DataSet)
    {
    vtkErrorMacro("No data object has been set.");
    return;
    }
  if (!this->IsDoneWithTraversal())
    {
    this->Internal->Iterator++;
    }
}

//----------------------------------------------------------------------------
int vtkMultiBlockDataIterator::IsDoneWithTraversal()
{
  if (!this->DataSet)
    {
    vtkErrorMacro("No data object has been set.");
    return 1;
    }
  if ( this->Internal->Iterator == this->DataSet->Internal->DataSets.end() )
    {
    return 1;
    }
  else
    {
    return 0;
    }
}

//----------------------------------------------------------------------------
vtkDataObject* vtkMultiBlockDataIterator::GetCurrentDataObject()
{
  if (!this->DataSet)
    {
    vtkErrorMacro("No data object has been set.");
    return 0;
    }
  return this->Internal->Iterator->GetPointer();
}

//----------------------------------------------------------------------------
void vtkMultiBlockDataIterator::PrintSelf(ostream& os, vtkIndent indent)
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

