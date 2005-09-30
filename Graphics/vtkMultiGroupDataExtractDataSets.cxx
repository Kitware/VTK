/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkMultiGroupDataExtractDataSets.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkMultiGroupDataExtractDataSets.h"

#include "vtkCompositeDataPipeline.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataSet.h"
#include "vtkHierarchicalBoxDataSet.h"
#include "vtkMultiGroupDataInformation.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkUniformGrid.h"

#include <vtkstd/list>

vtkCxxRevisionMacro(vtkMultiGroupDataExtractDataSets, "1.2");
vtkStandardNewMacro(vtkMultiGroupDataExtractDataSets);

struct vtkMultiGroupDataExtractDataSetsInternals
{
  typedef
  vtkstd::list<vtkMultiGroupDataExtractDataSets::DataSetNode> DataSetsTypes;
  DataSetsTypes DataSets;
};

//----------------------------------------------------------------------------
vtkMultiGroupDataExtractDataSets::vtkMultiGroupDataExtractDataSets()
{
  this->Internal = new vtkMultiGroupDataExtractDataSetsInternals;
  this->MinGroup = VTK_UNSIGNED_INT_MAX;
}

//----------------------------------------------------------------------------
vtkMultiGroupDataExtractDataSets::~vtkMultiGroupDataExtractDataSets()
{
  delete this->Internal;
}

//----------------------------------------------------------------------------
void vtkMultiGroupDataExtractDataSets::AddDataSet(
  unsigned int group, unsigned int idx)
{
  this->Internal->DataSets.push_back(DataSetNode(group, idx));
  this->MinGroup = (group < this->MinGroup) ? group : this->MinGroup;

  this->Modified();
}

//----------------------------------------------------------------------------
void vtkMultiGroupDataExtractDataSets::ClearDataSetList()
{
  this->Internal->DataSets.clear();
  this->MinGroup = VTK_UNSIGNED_INT_MAX;
  this->Modified();
}

//----------------------------------------------------------------------------
unsigned int vtkMultiGroupDataExtractDataSets::ComputeOutputGroups(
  unsigned int inputNumGroups)
{
  unsigned int numGroups = 0;
  vtkMultiGroupDataExtractDataSetsInternals::DataSetsTypes::iterator it =
    this->Internal->DataSets.begin();
  for (; it != this->Internal->DataSets.end(); it++)
    {
    DataSetNode& node = *it;
    unsigned int curNumGroups = node.Group - this->MinGroup + 1;
    if (curNumGroups > numGroups && curNumGroups <= inputNumGroups)
      {
      numGroups = curNumGroups;
      }
    }
  return numGroups;
}

//----------------------------------------------------------------------------
int vtkMultiGroupDataExtractDataSets::RequestDataObject(
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
int vtkMultiGroupDataExtractDataSets::RequestInformation(
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
  unsigned int numOutputGroups = this->ComputeOutputGroups(numInputGroups);

  vtkMultiGroupDataInformation* compInfo = 
    vtkMultiGroupDataInformation::New();
  if (numOutputGroups > 0)
    {
    compInfo->SetNumberOfGroups(numOutputGroups);
    vtkMultiGroupDataExtractDataSetsInternals::DataSetsTypes::iterator it =
      this->Internal->DataSets.begin();
    for (; it != this->Internal->DataSets.end(); it++)
      {
      DataSetNode& node = *it;
      unsigned int numInputDataSets = 
        inCompInfo->GetNumberOfDataSets(node.Group);
      if (node.DataSetId <= numInputDataSets)
        {
        if (node.DataSetId >= compInfo->GetNumberOfDataSets(
              node.Group - this->MinGroup))
          {
          compInfo->SetNumberOfDataSets(
            node.Group - this->MinGroup, node.DataSetId+1);

          if (inCompInfo->HasInformation(node.Group, node.DataSetId))
            {
            vtkInformation* outdInfo = 
              compInfo->GetInformation(node.Group - this->MinGroup,
                                       node.DataSetId);
            vtkInformation* indInfo = 
              inCompInfo->GetInformation(node.Group, node.DataSetId);
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
int vtkMultiGroupDataExtractDataSets::RequestData(
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

  unsigned int numInputGroups = input->GetNumberOfGroups();
  unsigned int numOutputGroups = this->ComputeOutputGroups(numInputGroups);

  if (numOutputGroups > 0)
    {
    output->SetNumberOfGroups(numOutputGroups);

    vtkMultiGroupDataExtractDataSetsInternals::DataSetsTypes::iterator it =
      this->Internal->DataSets.begin();
    for (; it != this->Internal->DataSets.end(); it++)
      {
      DataSetNode& node = *it;
      unsigned int numInputDataSets = input->GetNumberOfDataSets(node.Group);
      if (node.DataSetId <= numInputDataSets)
        {
        if (node.DataSetId >= output->GetNumberOfDataSets(
              node.Group - this->MinGroup))
          {
          output->SetNumberOfDataSets(node.Group - this->MinGroup,
                                      node.DataSetId+1);
          }
        vtkDataObject* dObj = 
          input->GetDataSet(node.Group, node.DataSetId);
        if (dObj)
          {
          vtkDataObject* copy = dObj->NewInstance();
          copy->ShallowCopy(dObj);

          // Remove blanking from output datasets.
          vtkUniformGrid* ug = vtkUniformGrid::SafeDownCast(copy);
          if (ug)
            {
            ug->SetCellVisibilityArray(0);
            }
          output->SetDataSet(node.Group - this->MinGroup, node.DataSetId, copy);
          copy->Delete();
          }
        }
      }

    vtkMultiGroupDataInformation* compInfo = 
      vtkMultiGroupDataInformation::SafeDownCast(
        info->Get(vtkCompositeDataPipeline::COMPOSITE_DATA_INFORMATION()));
    
    if (compInfo)
      {
      output->SetMultiGroupDataInformation(compInfo);
      }
    unsigned int numGroups = output->GetNumberOfGroups();
    
    vtkHierarchicalBoxDataSet* hbds = 
      vtkHierarchicalBoxDataSet::SafeDownCast(output);
    if (hbds)
      {
      vtkHierarchicalBoxDataSet* ihbds = 
        vtkHierarchicalBoxDataSet::SafeDownCast(input);
      for (unsigned int group=0; group<numGroups-1; group++)
        {
        hbds->SetRefinementRatio(
          group, ihbds->GetRefinementRatio(group + this->MinGroup));
        }
      hbds->GenerateVisibilityArrays();
      }

    }

  return 1;
}

//----------------------------------------------------------------------------
void vtkMultiGroupDataExtractDataSets::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
