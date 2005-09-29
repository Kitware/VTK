/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMultiGroupDataGroupFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkMultiGroupDataGroupFilter.h"

#include "vtkCellData.h"
#include "vtkDataSet.h"
#include "vtkMultiGroupDataSet.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"

vtkCxxRevisionMacro(vtkMultiGroupDataGroupFilter, "1.1");
vtkStandardNewMacro(vtkMultiGroupDataGroupFilter);

vtkMultiGroupDataGroupFilter::vtkMultiGroupDataGroupFilter()
{
}

vtkMultiGroupDataGroupFilter::~vtkMultiGroupDataGroupFilter()
{
}

int vtkMultiGroupDataGroupFilter::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  vtkInformation* info = outputVector->GetInformationObject(0);
  vtkMultiGroupDataSet *output = vtkMultiGroupDataSet::SafeDownCast(
    info->Get(vtkCompositeDataSet::COMPOSITE_DATA_SET()));
  if (!output) {return 0;}

  unsigned int updatePiece = static_cast<unsigned int>(
    info->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()));
  unsigned int updateNumPieces =  static_cast<unsigned int>(
    info->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES()));

  int numInputs = inputVector[0]->GetNumberOfInformationObjects();
  output->SetNumberOfGroups(numInputs);
  for (int idx = 0; idx < numInputs; ++idx)
    {
    output->SetNumberOfDataSets(idx, updateNumPieces);
    vtkDataSet* input = 0;
    vtkInformation* inInfo = inputVector[0]->GetInformationObject(idx);
    if (inInfo)
      {
      input = 
        vtkDataSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
      }
    if (input)
      {
      vtkDataSet* dsCopy = input->NewInstance();
      dsCopy->ShallowCopy(input);
      output->SetDataSet(idx, updatePiece, dsCopy);
      dsCopy->Delete();
      }
    }

  return 1;
}

void vtkMultiGroupDataGroupFilter::AddInput(vtkDataObject* input)
{
  this->AddInput(0, input);
}

void vtkMultiGroupDataGroupFilter::AddInput(int index, vtkDataObject* input)
{
  if(input)
    {
    this->AddInputConnection(index, input->GetProducerPort());
    }
}

int vtkMultiGroupDataGroupFilter::FillInputPortInformation(
  int, vtkInformation *info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  info->Set(vtkAlgorithm::INPUT_IS_REPEATABLE(), 1);
  info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  return 1;
}

void vtkMultiGroupDataGroupFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
