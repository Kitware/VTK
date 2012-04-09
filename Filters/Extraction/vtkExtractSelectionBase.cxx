/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractSelectionBase.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkExtractSelectionBase.h"

#include "vtkGraph.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkTable.h"
#include "vtkUnstructuredGrid.h"

//----------------------------------------------------------------------------
vtkExtractSelectionBase::vtkExtractSelectionBase()
{
  this->PreserveTopology = 0;
  this->SetNumberOfInputPorts(2);
}

//----------------------------------------------------------------------------
vtkExtractSelectionBase::~vtkExtractSelectionBase()
{
}

//----------------------------------------------------------------------------
int vtkExtractSelectionBase::FillInputPortInformation(
  int port, vtkInformation* info)
{
  if (port==0)
    {
    // Cannot work with composite datasets.
    info->Remove(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE());
    info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
    info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkGraph");
    info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkTable");
    }
  else
    {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkSelection");
    info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
    }
  return 1;
}

//----------------------------------------------------------------------------
// Needed because parent class sets output type to input type
// and we sometimes want to change it to make an UnstructuredGrid regardless of
// input type
int vtkExtractSelectionBase::RequestDataObject(
  vtkInformation*,
  vtkInformationVector** inputVector ,
  vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if (!inInfo)
    {
    return 0;
    }

  vtkDataSet *input = vtkDataSet::GetData(inInfo);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  if (input)
    {
    int passThrough = this->PreserveTopology? 1 : 0;

    vtkDataSet *output = vtkDataSet::GetData(outInfo);
    if (!output ||
      (passThrough && !output->IsA(input->GetClassName())) ||
      (!passThrough && !output->IsA("vtkUnstructuredGrid")))
      {
      vtkDataSet* newOutput = NULL;
      if (!passThrough)
        {
        // The mesh will be modified.
        newOutput = vtkUnstructuredGrid::New();
        }
      else
        {
        // The mesh will not be modified.
        newOutput = input->NewInstance();
        }
      outInfo->Set(vtkDataObject::DATA_OBJECT(), newOutput);
      newOutput->Delete();
      }
    return 1;
    }

  vtkGraph *graphInput = vtkGraph::GetData(inInfo);
  if (graphInput)
    {
    // Accept graph input, but we don't produce the correct extracted
    // graph as output yet.
    return 1;
    }

  vtkTable *tableInput = vtkTable::GetData(inInfo);
  if (tableInput)
    {
    vtkTable *output = vtkTable::GetData(outInfo);
    if (!output)
      {
      output = vtkTable::New();
      outInfo->Set(vtkDataObject::DATA_OBJECT(), output);
      output->Delete();
      }
    return 1;
    }

  return 0;
}

//----------------------------------------------------------------------------
void vtkExtractSelectionBase::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "PreserveTopology: " << this->PreserveTopology << endl;
}

