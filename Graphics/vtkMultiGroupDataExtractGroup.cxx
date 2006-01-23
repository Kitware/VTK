/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkMultiGroupDataExtractGroup.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkMultiGroupDataExtractGroup.h"

#include "vtkCompositeDataPipeline.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataSet.h"
#include "vtkHierarchicalBoxDataSet.h"
#include "vtkMultiGroupDataInformation.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkUniformGrid.h"

vtkCxxRevisionMacro(vtkMultiGroupDataExtractGroup, "1.5");
vtkStandardNewMacro(vtkMultiGroupDataExtractGroup);

//----------------------------------------------------------------------------
vtkMultiGroupDataExtractGroup::vtkMultiGroupDataExtractGroup()
{
  this->MinGroup = 0;
  this->MaxGroup = 0;

}

//----------------------------------------------------------------------------
vtkMultiGroupDataExtractGroup::~vtkMultiGroupDataExtractGroup()
{
}

//----------------------------------------------------------------------------
int vtkMultiGroupDataExtractGroup::RequestDataObject(
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
int vtkMultiGroupDataExtractGroup::RequestInformation(
  vtkInformation*,
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkMultiGroupDataInformation* inCompInfo = 
    vtkMultiGroupDataInformation::SafeDownCast(
      inInfo->Get(vtkCompositeDataPipeline::COMPOSITE_DATA_INFORMATION()));
  if (!inCompInfo)
    {
    vtkDebugMacro("Expected information not found. "
                  "Cannot provide information.");
    return 1;
    }

  unsigned int numInputGroups = inCompInfo->GetNumberOfGroups();

  unsigned int numGroups;
  vtkMultiGroupDataInformation* compInfo = 
    vtkMultiGroupDataInformation::New();
  if (this->MaxGroup >= numInputGroups)
    {
    numGroups = numInputGroups;
    }
  else
    {
    numGroups = this->MaxGroup+1;
    }
  if (numGroups < this->MinGroup)
    {
    numGroups = this->MinGroup;
    }
  compInfo->SetNumberOfGroups(numGroups-this->MinGroup);

  for (unsigned int i=0; i<numGroups; i++)
    {
    if (i < this->MinGroup)
      {
      continue;
      }
    else if (i > this->MaxGroup)
      {
      compInfo->SetNumberOfDataSets(i-this->MinGroup, 0);
      }
    else
      {
      compInfo->SetNumberOfDataSets(i-this->MinGroup,
                                    inCompInfo->GetNumberOfDataSets(i));
      }
    unsigned int numDataSets = compInfo->GetNumberOfDataSets(i-this->MinGroup);
    for (unsigned int j=0; j<numDataSets; j++)
      {
      if (inCompInfo->HasInformation(i, j))
        {
        vtkInformation* outdInfo = compInfo->GetInformation(i-this->MinGroup, j);
        vtkInformation* indInfo = inCompInfo->GetInformation(i-this->MinGroup, j);
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
int vtkMultiGroupDataExtractGroup::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkMultiGroupDataSet *input = vtkMultiGroupDataSet::SafeDownCast(
    inInfo->Get(vtkCompositeDataSet::COMPOSITE_DATA_SET()));
  if (!input) {return 0;}

  vtkInformation* info = outputVector->GetInformationObject(0);
  vtkMultiGroupDataSet *output = vtkMultiGroupDataSet::SafeDownCast(
    info->Get(vtkCompositeDataSet::COMPOSITE_DATA_SET()));
  if (!output) {return 0;}

  output->SetNumberOfGroups(this->MaxGroup-this->MinGroup+1);
  unsigned int numGroups = output->GetNumberOfGroups();
  unsigned int numInputGroups = input->GetNumberOfGroups();

  float progress = 0;
  int counter = 0;
  float delProg = 1.0/numInputGroups;
  for (unsigned int group=0; group<numInputGroups; group++)
    {
    if (group < this->MinGroup)
      {
      continue;
      }
    else if (group > this->MaxGroup)
      {
      output->SetNumberOfDataSets(group-this->MinGroup, 0);
      continue;
      }
    unsigned int numDataSets = input->GetNumberOfDataSets(group);
    output->SetNumberOfDataSets(group-this->MinGroup, numDataSets);
    float delProg2 = delProg / numDataSets;
    for (unsigned int dataSet=0; dataSet<numDataSets; dataSet++)
      {
      progress += delProg2;
      if ( counter % 100 == 0 )
        {
        this->UpdateProgress(progress);
        }
      vtkDataSet* dObj = vtkDataSet::SafeDownCast(
        input->GetDataSet(group, dataSet));
      if (dObj)
        {
        vtkDataSet* copy = dObj->NewInstance();
        copy->ShallowCopy(dObj);
        output->SetDataSet(group-this->MinGroup, dataSet, copy);
        copy->Delete();
        }
      }
    }
  vtkMultiGroupDataInformation* compInfo = 
    vtkMultiGroupDataInformation::SafeDownCast(
      info->Get(vtkCompositeDataPipeline::COMPOSITE_DATA_INFORMATION()));

  output->SetMultiGroupDataInformation(compInfo);

  vtkHierarchicalBoxDataSet* hbds = 
    vtkHierarchicalBoxDataSet::SafeDownCast(output);
  if (hbds)
    {
    vtkHierarchicalBoxDataSet* ihbds = 
      vtkHierarchicalBoxDataSet::SafeDownCast(input);
    for (unsigned int group=0; group<numInputGroups-1; group++)
      {
      if (group < this->MinGroup)
        {
        continue;
        }
      hbds->SetRefinementRatio(group-this->MinGroup,
                               ihbds->GetRefinementRatio(group));
      }
    }

  // Last group should not be blanked (uniform grid only)
  unsigned int numDataSets = output->GetNumberOfDataSets(numGroups-1);
  for (unsigned int dataSet=0; dataSet<numDataSets; dataSet++)
    {
    vtkUniformGrid* ug = vtkUniformGrid::SafeDownCast(
      output->GetDataSet(numGroups-1, dataSet));
    if (ug)
      {
      ug->SetCellVisibilityArray(0);
      }
    }

  return 1;
}

//----------------------------------------------------------------------------
void vtkMultiGroupDataExtractGroup::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "MinGroup:" << this->MinGroup << endl;
  os << indent << "MaxGroup:" << this->MaxGroup << endl;
}
