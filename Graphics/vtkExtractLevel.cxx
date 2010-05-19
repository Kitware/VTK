/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractLevel.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkExtractLevel.h"

#include "vtkHierarchicalBoxDataSet.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkUniformGrid.h"
#include "vtkAMRBox.h"

#include <vtkstd/set>

class vtkExtractLevel::vtkSet : public vtkstd::set<unsigned int>
{
};

vtkStandardNewMacro(vtkExtractLevel);
//----------------------------------------------------------------------------
vtkExtractLevel::vtkExtractLevel()
{
  this->Levels = new vtkExtractLevel::vtkSet();
}

//----------------------------------------------------------------------------
vtkExtractLevel::~vtkExtractLevel()
{
  delete this->Levels;
}

//----------------------------------------------------------------------------
void vtkExtractLevel::AddLevel(unsigned int level)
{
  this->Levels->insert(level);
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkExtractLevel::RemoveLevel(unsigned int level)
{
  this->Levels->erase(level);
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkExtractLevel::RemoveAllLevels()
{
  this->Levels->clear();
  this->Modified();
}

//----------------------------------------------------------------------------
int vtkExtractLevel::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkHierarchicalBoxDataSet *input = vtkHierarchicalBoxDataSet::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  if (!input) {return 0;}

  vtkInformation* info = outputVector->GetInformationObject(0);
  vtkHierarchicalBoxDataSet *output = vtkHierarchicalBoxDataSet::SafeDownCast(
    info->Get(vtkDataObject::DATA_OBJECT()));
  if (!output) {return 0;}

  unsigned int numLevels = input->GetNumberOfLevels();
  output->SetNumberOfLevels(numLevels);

  // copy level meta data.
  for (unsigned int cc=0; cc < numLevels; cc++)
    {
    if (input->HasLevelMetaData(cc))
      {
      output->GetLevelMetaData(cc)->Copy(input->GetLevelMetaData(cc));
      }
    }


  unsigned int last_level = 0;
  vtkExtractLevel::vtkSet::iterator iter;
  for (iter = this->Levels->begin(); iter != this->Levels->end(); ++iter)
    {
    unsigned int level = (*iter);
    last_level = level;
    unsigned int numDataSets = input->GetNumberOfDataSets(level);
    output->SetNumberOfDataSets(level, numDataSets);
    for (unsigned int kk = 0; kk < numDataSets; kk++)
      {
      // Copy meta data.
      if (input->HasMetaData(level, kk))
        {
        vtkInformation* copy = output->GetMetaData(level, kk);
        copy->Copy(input->GetMetaData(level, kk));
        }

      // Copy data object.
      vtkAMRBox box;
      vtkUniformGrid* data = input->GetDataSet(level, kk, box);
      if (data)
        {
        vtkUniformGrid* copy = data->NewInstance();
        copy->ShallowCopy(data);
        output->SetDataSet(level, kk, box, copy);
        copy->Delete();
        }
      else
        {
        output->SetDataSet(level, kk, box, NULL);
        }
      }
    }

  // Last levelfshould not be blanked (uniform grid only)
  unsigned int numDataSets = output->GetNumberOfDataSets(last_level);
  for (unsigned int nn=0; nn < numDataSets; nn++)
    {
    vtkAMRBox temp;
    vtkUniformGrid* ug = vtkUniformGrid::SafeDownCast(
      output->GetDataSet(numLevels-1, nn, temp));
    if (ug)
      {
      ug->SetCellVisibilityArray(0);
      }
    }

  return 1;
}

//----------------------------------------------------------------------------
void vtkExtractLevel::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

