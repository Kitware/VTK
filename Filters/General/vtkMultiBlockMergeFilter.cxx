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
#include "vtkDataSet.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"

vtkStandardNewMacro(vtkMultiBlockMergeFilter);
//-----------------------------------------------------------------------------
vtkMultiBlockMergeFilter::vtkMultiBlockMergeFilter()
{
}

//-----------------------------------------------------------------------------
vtkMultiBlockMergeFilter::~vtkMultiBlockMergeFilter()
{
}

//-----------------------------------------------------------------------------
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
  unsigned int usNInputs = static_cast<unsigned int>(numInputs);

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
        if (!this->Merge(usNInputs, idx, output, input))
        {
          return 0;
        }
        /*
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
          */
      }
    }
  }

  return !first;
}

//-----------------------------------------------------------------------------
int vtkMultiBlockMergeFilter::IsMultiPiece(vtkMultiBlockDataSet* mb)
{
  unsigned int numBlocks = mb->GetNumberOfBlocks();
  for (unsigned int cc=0; cc < numBlocks; cc++)
  {
    vtkDataObject* block = mb->GetBlock(cc);
    if (block && !block->IsA("vtkDataSet"))
    {
      return 0;
    }
  }
  return 1;
}

//-----------------------------------------------------------------------------
int vtkMultiBlockMergeFilter::Merge(unsigned int numPieces, unsigned int pieceNo,
  vtkMultiBlockDataSet* output,
  vtkMultiBlockDataSet* input)
{
  if (!input && !output)
  {
    return 1;
  }

  if (!input || !output)
  {
    vtkErrorMacro("Case not handled");
    return 0;
  }

  unsigned int numInBlocks = input->GetNumberOfBlocks();
  unsigned int numOutBlocks = output->GetNumberOfBlocks();

  // Current limitation of this filter is that all blocks must either be
  // vtkMultiBlockDataSet or vtkDataSet not a mixture of the two.
  // a vtkMultiBlockDataSet with all child blocks as vtkDataSet is a multipiece
  // dataset. This filter merges pieces together.
  int mpInput = this->IsMultiPiece(input);
  int mpOutput = this->IsMultiPiece(output);

  if (!mpInput && !mpOutput && (numInBlocks == numOutBlocks))
  {
    for (unsigned int cc=0; cc < numInBlocks; cc++)
    {
      if (!this->Merge(numPieces, pieceNo,
          vtkMultiBlockDataSet::SafeDownCast(output->GetBlock(cc)),
          vtkMultiBlockDataSet::SafeDownCast(input->GetBlock(cc))))
      {
        return 0;
      }
    }
    return 1;
  }
  else if (mpInput && mpOutput)
  {
    output->SetNumberOfBlocks(numPieces);
    unsigned int inIndex = 0;
    //inputs are allowed to have either 1 or N datasets in each group
    if (numInBlocks == numPieces)
    {
      inIndex = pieceNo;
    }
    else if (numInBlocks != 1)
    {
      vtkErrorMacro("Case not currently handled.");
      return 0;
    }
    output->SetBlock(pieceNo, vtkDataSet::SafeDownCast(input->GetBlock(inIndex)));
    return 1;
  }

  vtkErrorMacro("Case not currently handled.");
  return 0;
}


//-----------------------------------------------------------------------------
void vtkMultiBlockMergeFilter::AddInputData(vtkDataObject* input)
{
  this->AddInputData(0, input);
}

//-----------------------------------------------------------------------------
void vtkMultiBlockMergeFilter::AddInputData(int index, vtkDataObject* input)
{
  this->AddInputDataInternal(index, input);
}

//-----------------------------------------------------------------------------
int vtkMultiBlockMergeFilter::FillInputPortInformation(
  int, vtkInformation *info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkMultiBlockDataSet");
  info->Set(vtkAlgorithm::INPUT_IS_REPEATABLE(), 1);
  info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  return 1;
}

//-----------------------------------------------------------------------------
void vtkMultiBlockMergeFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
