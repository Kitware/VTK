/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMultiBlockMergeFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkMultiBlockMergeFilter.h"

#include "vtkCellData.h"
#include "vtkDataObject.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"

vtkCxxRevisionMacro(vtkMultiBlockMergeFilter, "1.5");
vtkStandardNewMacro(vtkMultiBlockMergeFilter);

vtkMultiBlockMergeFilter::vtkMultiBlockMergeFilter()
{
}

vtkMultiBlockMergeFilter::~vtkMultiBlockMergeFilter()
{
}

int vtkMultiBlockMergeFilter::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  vtkInformation* info = outputVector->GetInformationObject(0);
  vtkMultiBlockDataSet *output = vtkMultiBlockDataSet::SafeDownCast(
    info->Get(vtkDataObject::DATA_OBJECT()));
  if (!output) {return 0;}

  int numInputs = inputVector[0]->GetNumberOfInformationObjects();
  if (numInputs<0)
    {
    vtkErrorMacro("Too many inputs to algorithm.")
    return 0;
    }
  unsigned int usNInputs = (unsigned int)numInputs;
  
  int first = 1;
  for (int idx = 0; idx < numInputs; ++idx)
    {
    vtkMultiBlockDataSet* input = 0;
    vtkInformation* inInfo = inputVector[0]->GetInformationObject(idx);
    if (inInfo)
      {
      input = vtkMultiBlockDataSet::SafeDownCast(
        inInfo->Get(vtkDataObject::DATA_OBJECT()));
      }
    if (input)
      {
      if (first)
        {
        //shallow copy first input to output to start off with      
        //cerr << "Copy first input" << endl;
        output->ShallowCopy(vtkMultiBlockDataSet::SafeDownCast(input));
        first = 0;
        }
      else
        {
        //merge in the inputs data sets into the proper places in the output
        for (unsigned int blk = 0; blk < input->GetNumberOfBlocks(); blk++)
          {
          //inputs are allowed to have either 1 or N datasets in each group
          unsigned int dsId = 0;
          if (input->GetNumberOfDataSets(blk) == usNInputs)
            {
            dsId = idx;
            }
          //cerr << "Copying blk " << blk << " index " << idx << endl;
          vtkDataObject* inblk = input->GetDataSet(blk, dsId);
          vtkDataObject* dsCopy = inblk->NewInstance();
          dsCopy->ShallowCopy(inblk);
          //output will always have N datasets in each group
          if (output->GetNumberOfDataSets(blk) != usNInputs)
            {
            output->SetNumberOfDataSets(blk, numInputs);
            }
          output->SetDataSet(blk, idx, dsCopy);
          dsCopy->Delete();
          }
        }
      }
    }

  return !first;
}

void vtkMultiBlockMergeFilter::AddInput(vtkDataObject* input)
{
  this->AddInput(0, input);
}

void vtkMultiBlockMergeFilter::AddInput(int index, vtkDataObject* input)
{
  if(input)
    {
    this->AddInputConnection(index, input->GetProducerPort());
    }
}

int vtkMultiBlockMergeFilter::FillInputPortInformation(
  int, vtkInformation *info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkMultiBlockDataSet");
  info->Set(vtkAlgorithm::INPUT_IS_REPEATABLE(), 1);
  info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  return 1;
}

void vtkMultiBlockMergeFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
