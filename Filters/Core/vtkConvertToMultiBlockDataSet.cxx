/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkConvertToMultiBlockDataSet.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkConvertToMultiBlockDataSet.h"

#include "vtkCompositeDataIterator.h"
#include "vtkInformation.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkSmartPointer.h"

vtkObjectFactoryNewMacro(vtkConvertToMultiBlockDataSet);
//----------------------------------------------------------------------------
vtkConvertToMultiBlockDataSet::vtkConvertToMultiBlockDataSet() = default;

//----------------------------------------------------------------------------
vtkConvertToMultiBlockDataSet::~vtkConvertToMultiBlockDataSet() = default;

//----------------------------------------------------------------------------
int vtkConvertToMultiBlockDataSet::FillInputPortInformation(int /*port*/, vtkInformation* info)
{
  // now add our info
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkCompositeDataSet");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkGraph");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkTable");
  return 1;
}

//----------------------------------------------------------------------------
int vtkConvertToMultiBlockDataSet::RequestData(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  auto input = vtkDataObject::GetData(inputVector[0], 0);
  auto output = vtkMultiBlockDataSet::GetData(outputVector, 0);
  return this->Execute(input, output) ? 1 : 0;
}

//----------------------------------------------------------------------------
bool vtkConvertToMultiBlockDataSet::Execute(vtkDataObject* input, vtkMultiBlockDataSet* output)
{
  if (auto inputCD = vtkCompositeDataSet::SafeDownCast(input))
  {
    output->CopyStructure(inputCD);
    auto iter = vtk::TakeSmartPointer(inputCD->NewIterator());
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
      output->SetDataSet(iter, iter->GetCurrentDataObject());
      if (iter->HasCurrentMetaData())
      {
        output->GetMetaData(iter)->Copy(iter->GetCurrentMetaData());
      }
    }
  }
  else
  {
    output->SetNumberOfBlocks(1);
    output->SetBlock(0, input);
  }
  return true;
}

//----------------------------------------------------------------------------
void vtkConvertToMultiBlockDataSet::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
