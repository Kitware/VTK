/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPartitionedDataSetCollectionToMultiBlockDataSet.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPartitionedDataSetCollectionToMultiBlockDataSet.h"

#include "vtkDataSet.h"
#include "vtkInformation.h"
#include "vtkLogger.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiPieceDataSet.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPartitionedDataSetCollection.h"

vtkObjectFactoryNewMacro(vtkPartitionedDataSetCollectionToMultiBlockDataSet);
//----------------------------------------------------------------------------
vtkPartitionedDataSetCollectionToMultiBlockDataSet::
  vtkPartitionedDataSetCollectionToMultiBlockDataSet() = default;

//----------------------------------------------------------------------------
vtkPartitionedDataSetCollectionToMultiBlockDataSet::
  ~vtkPartitionedDataSetCollectionToMultiBlockDataSet() = default;

//------------------------------------------------------------------------------
int vtkPartitionedDataSetCollectionToMultiBlockDataSet::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  // now add our info
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPartitionedDataSetCollection");
  return 1;
}

//----------------------------------------------------------------------------
int vtkPartitionedDataSetCollectionToMultiBlockDataSet::RequestData(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  auto input = vtkPartitionedDataSetCollection::GetData(inputVector[0], 0);
  auto output = vtkMultiBlockDataSet::GetData(outputVector, 0);
  return this->Execute(input, output) ? 1 : 0;
}

//----------------------------------------------------------------------------
bool vtkPartitionedDataSetCollectionToMultiBlockDataSet::Execute(
  vtkPartitionedDataSetCollection* input, vtkMultiBlockDataSet* output)
{
  // Just copy the hierarchical structure.
  output->SetNumberOfBlocks(input->GetNumberOfPartitionedDataSets());
  for (unsigned int cc = 0, max = input->GetNumberOfPartitionedDataSets(); cc < max; ++cc)
  {
    vtkNew<vtkMultiPieceDataSet> mp;
    mp->ShallowCopy(input->GetPartitionedDataSet(cc));
    output->SetBlock(cc, mp);
    if (input->HasMetaData(cc))
    {
      output->GetMetaData(cc)->Copy(input->GetMetaData(cc));
    }
  }
  return true;
}

//----------------------------------------------------------------------------
void vtkPartitionedDataSetCollectionToMultiBlockDataSet::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
