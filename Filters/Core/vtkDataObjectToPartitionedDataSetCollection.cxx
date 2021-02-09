/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataObjectToPartitionedDataSetCollection.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkDataObjectToPartitionedDataSetCollection.h"

#include "vtkDataAssembly.h"
#include "vtkDataAssemblyUtilities.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkLogger.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiPieceDataSet.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkUniformGrid.h"
#include "vtkUniformGridAMR.h"

#include <cassert>
#include <functional>
#include <vector>

vtkStandardNewMacro(vtkDataObjectToPartitionedDataSetCollection);
//----------------------------------------------------------------------------
vtkDataObjectToPartitionedDataSetCollection::vtkDataObjectToPartitionedDataSetCollection() =
  default;

//----------------------------------------------------------------------------
vtkDataObjectToPartitionedDataSetCollection::~vtkDataObjectToPartitionedDataSetCollection() =
  default;

//----------------------------------------------------------------------------
int vtkDataObjectToPartitionedDataSetCollection::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkDataObject");
  return 1;
}

//----------------------------------------------------------------------------
int vtkDataObjectToPartitionedDataSetCollection::RequestData(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  auto inputDO = vtkDataObject::GetData(inputVector[0], 0);
  auto output = vtkPartitionedDataSetCollection::GetData(outputVector, 0);

  if (auto pdc = vtkPartitionedDataSetCollection::SafeDownCast(inputDO))
  {
    // nothing to do, input is already a vtkPartitionedDataSetCollection.
    output->ShallowCopy(pdc);
    return 1;
  }
  else if (auto pd = vtkPartitionedDataSet::SafeDownCast(inputDO))
  {
    output->SetPartitionedDataSet(0, pd);
    return 1;
  }
  else if (vtkCompositeDataSet::SafeDownCast(inputDO) == nullptr)
  {
    output->SetPartition(0, 0, inputDO);
    return 1;
  }

  vtkNew<vtkDataAssembly> assembly;
  if (vtkDataAssemblyUtilities::GenerateHierarchy(
        vtkCompositeDataSet::SafeDownCast(inputDO), assembly, output))
  {
    output->SetDataAssembly(assembly);
    return 1;
  }

  output->Initialize();
  return 0;
}

//----------------------------------------------------------------------------
void vtkDataObjectToPartitionedDataSetCollection::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
