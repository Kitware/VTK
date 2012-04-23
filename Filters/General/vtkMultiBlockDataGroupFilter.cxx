/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMultiBlockDataGroupFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkMultiBlockDataGroupFilter.h"

#include "vtkCellData.h"
#include "vtkDataSet.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkOnePieceExtentTranslator.h"
#include "vtkStreamingDemandDrivenPipeline.h"

vtkStandardNewMacro(vtkMultiBlockDataGroupFilter);
//-----------------------------------------------------------------------------
vtkMultiBlockDataGroupFilter::vtkMultiBlockDataGroupFilter()
{
}

//-----------------------------------------------------------------------------
vtkMultiBlockDataGroupFilter::~vtkMultiBlockDataGroupFilter()
{
}

//-----------------------------------------------------------------------------
int vtkMultiBlockDataGroupFilter::RequestInformation(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **,
  vtkInformationVector *outputVector)
{
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // Setup ExtentTranslator so that all downstream piece requests are
  // converted to whole extent update requests, as need by the histogram filter.
  if (strcmp(
      vtkStreamingDemandDrivenPipeline::GetExtentTranslator(outInfo)
        ->GetClassName(), "vtkOnePieceExtentTranslator") != 0)
    {
    vtkExtentTranslator* et = vtkOnePieceExtentTranslator::New();
    vtkStreamingDemandDrivenPipeline::SetExtentTranslator(outInfo, et);
    et->Delete();
    }
  return 1;
}

//-----------------------------------------------------------------------------
int vtkMultiBlockDataGroupFilter::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  vtkInformation* info = outputVector->GetInformationObject(0);
  vtkMultiBlockDataSet* output = vtkMultiBlockDataSet::SafeDownCast(
    info->Get(vtkDataObject::DATA_OBJECT()));
  if (!output)
    {
    return 0;
    }

  /*
  unsigned int updatePiece = static_cast<unsigned int>(
    info->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()));
  unsigned int updateNumPieces =  static_cast<unsigned int>(
    info->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES()));
  */

  int numInputs = inputVector[0]->GetNumberOfInformationObjects();
  output->SetNumberOfBlocks(numInputs);
  for (int idx = 0; idx < numInputs; ++idx)
    {
    /*
    // This can be a vtkMultiPieceDataSet if we ever support it.
    vtkMultiBlockDataSet* block = vtkMultiBlockDataSet::New();
    block->SetNumberOfBlocks(updateNumPieces);
    block->Delete();
    */

    vtkDataObject* input = 0;
    vtkInformation* inInfo = inputVector[0]->GetInformationObject(idx);
    if (inInfo)
      {
      input = inInfo->Get(vtkDataObject::DATA_OBJECT());
      }
    if (input)
      {
      vtkDataObject* dsCopy = input->NewInstance();
      dsCopy->ShallowCopy(input);
      output->SetBlock(idx, dsCopy);
      dsCopy->Delete();
      }
    else
      {
      output->SetBlock(idx, 0);
      }
    }

  if (output->GetNumberOfBlocks() == 1 &&
    output->GetBlock(0) && output->GetBlock(0)->IsA("vtkMultiBlockDataSet"))
    {
    vtkMultiBlockDataSet* block = vtkMultiBlockDataSet::SafeDownCast(
      output->GetBlock(0));
    block->Register(this);
    output->ShallowCopy(block);
    block->UnRegister(this);
    }

  return 1;
}

//-----------------------------------------------------------------------------
void vtkMultiBlockDataGroupFilter::AddInputData(vtkDataObject* input)
{
  this->AddInputData(0, input);
}

//-----------------------------------------------------------------------------
void vtkMultiBlockDataGroupFilter::AddInputData(int index, vtkDataObject* input)
{
  this->AddInputDataInternal(index, input);
}

//-----------------------------------------------------------------------------
int vtkMultiBlockDataGroupFilter::FillInputPortInformation(
  int, vtkInformation *info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataObject");
  info->Set(vtkAlgorithm::INPUT_IS_REPEATABLE(), 1);
  info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  return 1;
}

//-----------------------------------------------------------------------------
void vtkMultiBlockDataGroupFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
