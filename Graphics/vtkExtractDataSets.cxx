/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractDataSets.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkExtractDataSets.h"

#include "vtkObjectFactory.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkHierarchicalBoxDataSet.h"
#include "vtkUniformGrid.h"
#include "vtkAMRBox.h"
#include "vtkUnsignedCharArray.h"
#include <vtkstd/set>

class vtkExtractDataSets::vtkInternals
{
public:
  struct Node
    {
    unsigned int Level;
    unsigned int Index;
    
    bool operator() (const Node& n1, const Node& n2) const
      {
      if (n1.Level == n2.Level)
        {
        return (n1.Index < n2.Index);
        }
      return (n1.Level < n2.Level);
      }
    };


  typedef vtkstd::set<Node, Node> DatasetsType;
  DatasetsType Datasets;
};

vtkStandardNewMacro(vtkExtractDataSets);
//----------------------------------------------------------------------------
vtkExtractDataSets::vtkExtractDataSets()
{
  this->Internals = new vtkInternals();
}

//----------------------------------------------------------------------------
vtkExtractDataSets::~vtkExtractDataSets()
{
  delete this->Internals;
}

//----------------------------------------------------------------------------
void vtkExtractDataSets::AddDataSet(
  unsigned int level, unsigned int idx)
{
  vtkInternals::Node node;
  node.Level = level;
  node.Index = idx;
  this->Internals->Datasets.insert(node);
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkExtractDataSets::ClearDataSetList()
{
  this->Internals->Datasets.clear();
  this->Modified();
}

//----------------------------------------------------------------------------
int vtkExtractDataSets::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkHierarchicalBoxDataSet *input = vtkHierarchicalBoxDataSet::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkInformation* info = outputVector->GetInformationObject(0);
  vtkHierarchicalBoxDataSet *output = vtkHierarchicalBoxDataSet::SafeDownCast(
    info->Get(vtkDataObject::DATA_OBJECT()));

  unsigned int numLevels = input->GetNumberOfLevels();
  output->SetNumberOfLevels(numLevels);
  
  // copy levels meta-data.
  for (unsigned int cc=0; cc < numLevels; cc++)
    {
    if (input->HasLevelMetaData(cc))
      {
      output->GetLevelMetaData(cc)->Copy(input->GetLevelMetaData(cc));
      }
    }


  // Note that we need to ensure that all processess have the same tree
  // structure in the output.
  vtkInternals::DatasetsType::iterator iter;
  for (iter = this->Internals->Datasets.begin();
    iter != this->Internals->Datasets.end(); ++iter)
    {
    vtkAMRBox box;
    vtkUniformGrid* inUG = input->GetDataSet(
      iter->Level, iter->Index, box);
    unsigned int out_index = output->GetNumberOfDataSets(iter->Level);
    output->SetNumberOfDataSets(iter->Level, out_index+1);
    if (input->HasMetaData(iter->Level, iter->Index))
      {
      output->GetMetaData(iter->Level, out_index)->Copy(
        input->GetMetaData(iter->Level, iter->Index));
      }

    if (inUG)
      {
      vtkUniformGrid* clone = inUG->NewInstance();
      clone->ShallowCopy(inUG);
        
      // Remove blanking from output datasets.
      clone->SetCellVisibilityArray(0);
      output->SetDataSet(iter->Level, 
        out_index, box, clone);
      clone->Delete();
      }
    }

  // regenerate blanking.
//  output->GenerateVisibilityArrays();
  return 1;
}


//----------------------------------------------------------------------------
void vtkExtractDataSets::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

