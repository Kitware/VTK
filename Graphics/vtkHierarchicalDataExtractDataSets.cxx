/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkHierarchicalDataExtractDataSets.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkHierarchicalDataExtractDataSets.h"

#include "vtkCompositeDataPipeline.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataSet.h"
#include "vtkHierarchicalBoxDataSet.h"
#include "vtkHierarchicalDataInformation.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkUniformGrid.h"

#include <vtkstd/list>

vtkCxxRevisionMacro(vtkHierarchicalDataExtractDataSets, "1.1");
vtkStandardNewMacro(vtkHierarchicalDataExtractDataSets);

struct vtkHierarchicalDataExtractDataSetsInternals
{
  typedef
  vtkstd::list<vtkHierarchicalDataExtractDataSets::DataSetNode> DataSetsTypes;
  DataSetsTypes DataSets;
};

//----------------------------------------------------------------------------
vtkHierarchicalDataExtractDataSets::vtkHierarchicalDataExtractDataSets()
{
  this->Internal = new vtkHierarchicalDataExtractDataSetsInternals;
}

//----------------------------------------------------------------------------
vtkHierarchicalDataExtractDataSets::~vtkHierarchicalDataExtractDataSets()
{
  delete this->Internal;
}

//----------------------------------------------------------------------------
void vtkHierarchicalDataExtractDataSets::AddDataSet(
  unsigned int level, unsigned int idx)
{
  this->Internal->DataSets.push_back(DataSetNode(level, idx));
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkHierarchicalDataExtractDataSets::ClearDataSetList()
{
  this->Internal->DataSets.clear();
  this->Modified();
}

//----------------------------------------------------------------------------
unsigned int vtkHierarchicalDataExtractDataSets::ComputeOutputLevels(
  unsigned int inputNumLevels)
{
  unsigned int numLevels = 0;
  vtkHierarchicalDataExtractDataSetsInternals::DataSetsTypes::iterator it =
    this->Internal->DataSets.begin();
  for (; it != this->Internal->DataSets.end(); it++)
    {
    DataSetNode& node = *it;
    unsigned int curNumLevels = node.Level + 1;
    if (curNumLevels > numLevels && curNumLevels <= inputNumLevels)
      {
      numLevels = curNumLevels;
      }
    }
  return numLevels;
}

//----------------------------------------------------------------------------
int vtkHierarchicalDataExtractDataSets::RequestDataObject(
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
int vtkHierarchicalDataExtractDataSets::RequestInformation(
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
  unsigned int numOutputLevels = this->ComputeOutputLevels(numInputLevels);

  vtkHierarchicalDataInformation* compInfo = 
    vtkHierarchicalDataInformation::New();
  if (numOutputLevels > 0)
    {
    compInfo->SetNumberOfLevels(numOutputLevels);
    vtkHierarchicalDataExtractDataSetsInternals::DataSetsTypes::iterator it =
      this->Internal->DataSets.begin();
    for (; it != this->Internal->DataSets.end(); it++)
      {
      DataSetNode& node = *it;
      unsigned int numInputDataSets = inCompInfo->GetNumberOfDataSets(node.Level);
      if (node.DataSetId <= numInputDataSets)
        {
        if (node.DataSetId >= compInfo->GetNumberOfDataSets(node.Level))
          {
          compInfo->SetNumberOfDataSets(node.Level, node.DataSetId+1);

          if (inCompInfo->HasInformation(node.Level, node.DataSetId))
            {
            vtkInformation* outdInfo = 
              compInfo->GetInformation(node.Level, node.DataSetId);
            vtkInformation* indInfo = 
              inCompInfo->GetInformation(node.Level, node.DataSetId);
            outdInfo->Copy(indInfo);
            }
          }
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
int vtkHierarchicalDataExtractDataSets::RequestData(
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

  unsigned int numInputLevels = input->GetNumberOfLevels();
  unsigned int numOutputLevels = this->ComputeOutputLevels(numInputLevels);

  if (numOutputLevels > 0)
    {
    output->SetNumberOfLevels(numOutputLevels);

    vtkHierarchicalDataExtractDataSetsInternals::DataSetsTypes::iterator it =
      this->Internal->DataSets.begin();
    for (; it != this->Internal->DataSets.end(); it++)
      {
      DataSetNode& node = *it;
      unsigned int numInputDataSets = input->GetNumberOfDataSets(node.Level);
      if (node.DataSetId <= numInputDataSets)
        {
        if (node.DataSetId >= output->GetNumberOfDataSets(node.Level))
          {
          output->SetNumberOfDataSets(node.Level, node.DataSetId+1);
          }
        vtkDataObject* dObj = 
          input->GetDataSet(node.Level, node.DataSetId);
        if (dObj)
          {
          vtkDataObject* copy = dObj->NewInstance();
          copy->ShallowCopy(dObj);
          output->SetDataSet(node.Level, node.DataSetId, copy);
          // Remove blanking from output datasets.
          vtkUniformGrid* ug = vtkUniformGrid::SafeDownCast(copy);
          if (ug)
            {
            ug->SetCellVisibilityArray(0);
            }
          output->SetDataSet(node.Level, node.DataSetId, copy);
          copy->Delete();
          }
        }
      }
    }

  vtkHierarchicalDataInformation* compInfo = 
    vtkHierarchicalDataInformation::SafeDownCast(
      info->Get(vtkCompositeDataPipeline::COMPOSITE_DATA_INFORMATION()));

  output->SetHierarchicalDataInformation(compInfo);
  unsigned int numLevels = output->GetNumberOfLevels();

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
void vtkHierarchicalDataExtractDataSets::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
