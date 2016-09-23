/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPeriodicFiler.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

    This software is distributed WITHOUT ANY WARRANTY; without even
    the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
    PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkPeriodicFilter.h"

#include "vtkDataObjectTreeIterator.h"
#include "vtkDataSet.h"
#include "vtkInformationVector.h"
#include "vtkInformation.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiProcessController.h"

//----------------------------------------------------------------------------
vtkPeriodicFilter::vtkPeriodicFilter()
{
  this->IterationMode = VTK_ITERATION_MODE_MAX;
  this->NumberOfPeriods = 1;
  this->ReducePeriodNumbers = false;
}

//----------------------------------------------------------------------------
vtkPeriodicFilter::~vtkPeriodicFilter()
{
}

//----------------------------------------------------------------------------
void vtkPeriodicFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  if (this->IterationMode == VTK_ITERATION_MODE_DIRECT_NB)
  {
    os << indent << "Iteration Mode: Direct Number" << endl;
    os << indent << "Number of Periods: " << this->NumberOfPeriods << endl;
  }
  else
  {
    os << indent << "Iteration Mode: Maximum" << endl;
  }
}

//----------------------------------------------------------------------------
void vtkPeriodicFilter::AddIndex(unsigned int index)
{
  this->Indices.insert(index);
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPeriodicFilter::RemoveIndex(unsigned int index)
{
  this->Indices.erase(index);
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPeriodicFilter::RemoveAllIndices()
{
  this->Indices.clear();
  this->Modified();
}

//----------------------------------------------------------------------------
int vtkPeriodicFilter::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  // now add our info
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataObject");
  return 1;
}

//----------------------------------------------------------------------------
int vtkPeriodicFilter::RequestData(vtkInformation *vtkNotUsed(request),
                                   vtkInformationVector **inputVector,
                                   vtkInformationVector *outputVector)
{
  // Recover casted dataset
  vtkDataObject* inputObject = vtkDataObject::GetData(inputVector[0], 0);
  vtkDataObjectTree *input = vtkDataObjectTree::SafeDownCast(inputObject);
  vtkDataSet* dsInput = vtkDataSet::SafeDownCast(inputObject);
  vtkMultiBlockDataSet* mb = NULL;

  vtkMultiBlockDataSet *output = vtkMultiBlockDataSet::GetData(outputVector, 0);

  if (dsInput)
  {
    mb = vtkMultiBlockDataSet::New();
    mb->SetNumberOfBlocks(1);
    mb->SetBlock(0, dsInput);
    this->AddIndex(1);
    input = mb;
  }
  else if (this->Indices.empty())
  {
    // Trivial case
    output->ShallowCopy(input);
    return 1;
  }

  this->PeriodNumbers.clear();

  output->CopyStructure(input);

  // Copy selected blocks over to the output.
  vtkDataObjectTreeIterator* iter = input->NewTreeIterator();

  // Generate leaf multipieces
  iter->VisitOnlyLeavesOn();
  iter->SkipEmptyNodesOff();
  iter->InitTraversal();
  while (!iter->IsDoneWithTraversal() && this->Indices.size() > 0)
  {
    const unsigned int index = iter->GetCurrentFlatIndex();
    if (this->Indices.find(index) != this->Indices.end())
    {
      this->CreatePeriodicDataSet(iter, output, input);
    }
    else
    {
      vtkDataObject* inputLeaf = input->GetDataSet(iter);
      if (inputLeaf)
      {
        vtkDataObject* newLeaf = inputLeaf->NewInstance();
        newLeaf->ShallowCopy(inputLeaf);
        output->SetDataSet(iter, newLeaf);
        newLeaf->Delete();
      }
    }
    iter->GoToNextItem();
  }

  // Reduce period number in case of parrallelism, and update empty multipieces
  if (this->ReducePeriodNumbers)
  {
    int* reducedPeriodNumbers = new int[this->PeriodNumbers.size()];
    vtkMultiProcessController *controller = vtkMultiProcessController::GetGlobalController();
    if (controller)
    {
      controller->AllReduce(&this->PeriodNumbers.front(), reducedPeriodNumbers,
        this->PeriodNumbers.size(), vtkCommunicator::MAX_OP);
      int i = 0;
      iter->InitTraversal();
      while (!iter->IsDoneWithTraversal() && this->Indices.size() > 0)
      {
        if (reducedPeriodNumbers[i] > this->PeriodNumbers[i])
        {
          const unsigned int index = iter->GetCurrentFlatIndex();
          if (this->Indices.find(index) != this->Indices.end())
          {
            this->SetPeriodNumber(iter, output, reducedPeriodNumbers[i]);
          }
        }
        iter->GoToNextItem();
        i++;
      }
    }
    delete [] reducedPeriodNumbers;
  }
  iter->Delete();

  if (mb != NULL)
  {
    mb->Delete();
  }
  return 1;
}
