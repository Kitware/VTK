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
#include "vtkMultiGroupDataSet.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"

vtkCxxRevisionMacro(vtkMultiBlockMergeFilter, "1.3");
vtkStandardNewMacro(vtkMultiBlockMergeFilter);

vtkMultiBlockMergeFilter::vtkMultiBlockMergeFilter()
{
}

vtkMultiBlockMergeFilter::~vtkMultiBlockMergeFilter()
{
}

int vtkMultiBlockMergeFilter::RequestData(
  vtkInformation *request,
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  vtkInformation* info = outputVector->GetInformationObject(0);
  vtkMultiGroupDataSet *output = vtkMultiGroupDataSet::SafeDownCast(
    info->Get(vtkDataObject::DATA_OBJECT()));
  if (!output) {return 0;}

  int numInputs = inputVector[0]->GetNumberOfInformationObjects();

  //determine if we have a parallel partitioned mb data set to merge
  int allMBs = 1;
  for (int idx = 0; idx < numInputs; ++idx)
    {
    vtkDataObject* input = 0;
    vtkInformation* inInfo = inputVector[0]->GetInformationObject(idx);
    if (inInfo)
      {
      input = inInfo->Get(vtkDataObject::DATA_OBJECT());
      }
    if (input && !input->IsA("vtkMultiBlockDataSet"))
      {
      allMBs = 0;
      break;
      }
    }

  if (!allMBs)
    {
    //we do not, let the superclass append all of the inputs
    //cerr << "reverting to superclass" << endl;
    return this->Superclass::RequestData(request,inputVector,outputVector);
    }

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
        output->ShallowCopy(vtkMultiGroupDataSet::SafeDownCast(input));
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

void vtkMultiBlockMergeFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
