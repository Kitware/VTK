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
#include "vtkUniformGrid.h"

vtkCxxRevisionMacro(vtkHierarchicalDataExtractLevel, "1.7");
vtkStandardNewMacro(vtkHierarchicalDataExtractLevel);

//----------------------------------------------------------------------------
vtkHierarchicalDataExtractLevel::vtkHierarchicalDataExtractLevel()
{
  this->MinLevel = 0;
  this->MaxLevel = 0;

  this->InputLevels[0] = 0;
  this->InputLevels[1] = 0;
}

//----------------------------------------------------------------------------
vtkHierarchicalDataExtractLevel::~vtkHierarchicalDataExtractLevel()
{
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
    vtkDebugMacro("Expected information not found. "
                  "Cannot provide information.");
    return 1;
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
      if (inCompInfo->HasInformation(i, j))
        {
        vtkInformation* outdInfo = compInfo->GetInformation(i, j);
        vtkInformation* indInfo = inCompInfo->GetInformation(i, j);
        outdInfo->Copy(indInfo);
        }
      }
    }

  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  outInfo->Set(
    vtkCompositeDataPipeline::COMPOSITE_DATA_INFORMATION(), compInfo);
  compInfo->Delete();

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
    }

  // Last level should not be blanked (uniform grid only)
  unsigned int numDataSets = output->GetNumberOfDataSets(numLevels-1);
  for (unsigned int dataSet=0; dataSet<numDataSets; dataSet++)
    {
    vtkUniformGrid* ug = vtkUniformGrid::SafeDownCast(
      output->GetDataSet(numLevels-1, dataSet));
    if (ug)
      {
      ug->SetCellVisibilityArray(0);
      }
    }

  return 1;
}

//----------------------------------------------------------------------------
void vtkHierarchicalDataExtractLevel::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "MinLevel:" << this->MinLevel << endl;
  os << indent << "MaxLevel:" << this->MaxLevel << endl;
  os << indent << "InputLevels: (" << this->InputLevels[0] << "," 
                                   << this->InputLevels[1] << ")" << endl;

}
