/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkHierarchicalDataExtractLevel.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkHierarchicalDataExtractLevel.h"

#include "vtkCompositeDataPipeline.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataSet.h"
#include "vtkHierarchicalBoxDataSet.h"
#include "vtkHierarchicalDataInformation.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkHierarchicalDataExtractLevel, "1.1");
vtkStandardNewMacro(vtkHierarchicalDataExtractLevel);

// Construct object with PointIds and CellIds on; and ids being generated
// as scalars.
vtkHierarchicalDataExtractLevel::vtkHierarchicalDataExtractLevel()
{
  this->MinLevel = 0;
  this->MaxLevel = 0;

  this->InputLevels[0] = 0;
  this->InputLevels[1] = 0;
}

vtkHierarchicalDataExtractLevel::~vtkHierarchicalDataExtractLevel()
{
}

//----------------------------------------------------------------------------
int vtkHierarchicalDataExtractLevel::ProcessRequest(
  vtkInformation* request, 
  vtkInformationVector** inputVector, 
  vtkInformationVector* outputVector)
{
  // create the output
  if(request->Has(vtkDemandDrivenPipeline::REQUEST_DATA_OBJECT()))
    {
    return this->RequestDataObject(request, inputVector, outputVector);
    }
  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
int vtkHierarchicalDataExtractLevel::RequestDataObject(
  vtkInformation*, 
  vtkInformationVector** inputVector , 
  vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if (!inInfo)
    {
    return 0;
    }
  vtkCompositeDataSet *input = vtkCompositeDataSet::SafeDownCast(
    inInfo->Get(vtkCompositeDataSet::COMPOSITE_DATA_SET()));
  
  if (input)
    {
    vtkInformation* info = outputVector->GetInformationObject(0);
    vtkCompositeDataSet *output = vtkCompositeDataSet::SafeDownCast(
      info->Get(vtkCompositeDataSet::COMPOSITE_DATA_SET()));
    
    if (!output || !output->IsA(input->GetClassName())) 
      {
      output = input->NewInstance();
      output->SetPipelineInformation(info);
      output->Delete();
      }
    return 1;
    }
  return 0;
}

//----------------------------------------------------------------------------
int vtkHierarchicalDataExtractLevel::RequestInformation(
  vtkInformation*,
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkHierarchicalDataInformation* inCompInfo = 
    vtkHierarchicalDataInformation::SafeDownCast(
      inInfo->Get(vtkCompositeDataPipeline::COMPOSITE_DATA_INFORMATION()));
  if (!inCompInfo)
    {
    vtkErrorMacro("Expected information not found. "
                  "Cannot provide information.");
    return 0;
    }

  unsigned int numInputLevels = inCompInfo->GetNumberOfLevels();
  this->InputLevels[1] = numInputLevels-1;

  unsigned int numLevels;
  vtkHierarchicalDataInformation* compInfo = 
    vtkHierarchicalDataInformation::New();
  if (this->MaxLevel >= numInputLevels)
    {
    numLevels = numInputLevels;
    }
  else
    {
    numLevels = this->MaxLevel+1;
    }
  compInfo->SetNumberOfLevels(numLevels);

  for (unsigned int i=0; i<numLevels; i++)
    {
    if (i < this->MinLevel || i > this->MaxLevel)
      {
      compInfo->SetNumberOfDataSets(i, 0);
      }
    else
      {
      compInfo->SetNumberOfDataSets(i, inCompInfo->GetNumberOfDataSets(i));
      }
    unsigned int numDataSets = compInfo->GetNumberOfDataSets(i);
    for (unsigned int j=0; j<numDataSets; j++)
      {
      vtkInformation* outInfo = compInfo->GetInformation(i, j);
      vtkInformation* inInfo = inCompInfo->GetInformation(i, j);
      outInfo->Copy(inInfo);
      }
    }

  vtkInformation* info = outputVector->GetInformationObject(0);
  info->Set(
    vtkCompositeDataPipeline::COMPOSITE_DATA_INFORMATION(), compInfo);
  compInfo->Delete();

  return 1;
}

//----------------------------------------------------------------------------
int vtkHierarchicalDataExtractLevel::SetUpdateBlocks(
  vtkInformation*, 
  vtkInformationVector** inputVector, 
  vtkInformationVector* outputVector)
{
  vtkInformation* info = outputVector->GetInformationObject(0);

  vtkHierarchicalDataInformation* updateInfo = 
    vtkHierarchicalDataInformation::New();
  info->Set(
    vtkCompositeDataPipeline::UPDATE_BLOCKS(), updateInfo);

  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);

  vtkHierarchicalDataInformation* inUpdateInfo = 
    vtkHierarchicalDataInformation::SafeDownCast(
      inInfo->Get(vtkCompositeDataPipeline::UPDATE_BLOCKS()));

  updateInfo->SetNumberOfLevels(this->MaxLevel+1);
  unsigned int numLevels = updateInfo->GetNumberOfLevels();
  for (unsigned int j=0; j<numLevels; j++)
    {
    if (j < this->MinLevel || j > this->MaxLevel)
      {
      updateInfo->SetNumberOfDataSets(j, 0);
      continue;
      }
    updateInfo->SetNumberOfDataSets(j, inUpdateInfo->GetNumberOfDataSets(j));
    unsigned int numBlocks = updateInfo->GetNumberOfDataSets(j);
    for (unsigned int i=0; i<numBlocks; i++)
      {
      vtkInformation* inBInfo = inUpdateInfo->GetInformation(j, i);
      if (inBInfo->Get(vtkCompositeDataPipeline::MARKED_FOR_UPDATE()))
        {
        vtkInformation* info = updateInfo->GetInformation(j, i);
        info->Set(vtkCompositeDataPipeline::MARKED_FOR_UPDATE(), 1);
        }
      }
    }
  updateInfo->Delete();
  return 1;
}

//----------------------------------------------------------------------------
int vtkHierarchicalDataExtractLevel::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkHierarchicalDataSet *input = vtkHierarchicalDataSet::SafeDownCast(
    inInfo->Get(vtkCompositeDataSet::COMPOSITE_DATA_SET()));
  if (!input) {return 0;}

  vtkInformation* info = outputVector->GetInformationObject(0);
  vtkHierarchicalDataSet *output = vtkHierarchicalDataSet::SafeDownCast(
    info->Get(vtkCompositeDataSet::COMPOSITE_DATA_SET()));
  if (!output) {return 0;}

  output->SetNumberOfLevels(this->MaxLevel+1);
  unsigned int numLevels = output->GetNumberOfLevels();

  for (unsigned int level=0; level<numLevels; level++)
    {
    if (level < this->MinLevel || level > this->MaxLevel)
      {
      output->SetNumberOfDataSets(level, 0);
      continue;
      }
    unsigned int numDataSets = input->GetNumberOfDataSets(level);
    output->SetNumberOfDataSets(level, numDataSets);
    for (unsigned int dataSet=0; dataSet<numDataSets; dataSet++)
      {
      vtkDataSet* dObj = vtkDataSet::SafeDownCast(
        input->GetDataSet(level, dataSet));
      if (dObj)
        {
        vtkDataSet* copy = dObj->NewInstance();
        copy->ShallowCopy(dObj);
        output->SetDataSet(level, dataSet, copy);
        copy->Delete();
        }
      }
    }
  vtkHierarchicalDataInformation* compInfo = 
    vtkHierarchicalDataInformation::SafeDownCast(
      info->Get(vtkCompositeDataPipeline::COMPOSITE_DATA_INFORMATION()));

  output->SetHierarchicalDataInformation(compInfo);

  vtkHierarchicalBoxDataSet* hbds = 
    vtkHierarchicalBoxDataSet::SafeDownCast(output);
  if (hbds)
    {
    vtkHierarchicalBoxDataSet* ihbds = 
      vtkHierarchicalBoxDataSet::SafeDownCast(input);
    for (unsigned int level=0; level<numLevels-1; level++)
      {
      hbds->SetRefinementRatio(level, ihbds->GetRefinementRatio(level));
      }
    hbds->GenerateVisibilityArrays();
    }

  return 1;
}

//----------------------------------------------------------------------------
void vtkHierarchicalDataExtractLevel::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
