/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLevelIdScalars.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkLevelIdScalars.h"

#include "vtkAMRBox.h"
#include "vtkCellData.h"
#include "vtkHierarchicalBoxDataSet.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkUniformGrid.h"
#include "vtkUnsignedCharArray.h"

vtkStandardNewMacro(vtkLevelIdScalars);
//----------------------------------------------------------------------------
vtkLevelIdScalars::vtkLevelIdScalars()
{
}

//----------------------------------------------------------------------------
vtkLevelIdScalars::~vtkLevelIdScalars()
{
}

//----------------------------------------------------------------------------
// Map ids into attribute data
int vtkLevelIdScalars::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkHierarchicalBoxDataSet *input = vtkHierarchicalBoxDataSet::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  if (!input) 
    {
    return 0;
    }

  vtkInformation* info = outputVector->GetInformationObject(0);
  vtkHierarchicalBoxDataSet *output = vtkHierarchicalBoxDataSet::SafeDownCast(
    info->Get(vtkDataObject::DATA_OBJECT()));
  if (!output) 
    {
    return 0;
    }

  unsigned int numLevels = input->GetNumberOfLevels();
  output->SetNumberOfLevels(numLevels);

  for (unsigned int levelIdx=0; levelIdx<numLevels; levelIdx++)
    {
    unsigned int numDS = input->GetNumberOfDataSets(levelIdx);
    output->SetNumberOfDataSets(levelIdx, numDS);

    // Copy level metadata.
    if (input->HasLevelMetaData(levelIdx))
      {
      output->GetLevelMetaData(levelIdx)->Copy(
        input->GetLevelMetaData(levelIdx));
      }

    for (unsigned int cc=0; cc < numDS; cc++)
      {
      vtkAMRBox box;
      vtkUniformGrid* ds = input->GetDataSet(levelIdx, cc, box);
      if (ds)
        {
        vtkUniformGrid* copy = this->ColorLevel(ds, levelIdx);
        output->SetDataSet(levelIdx, cc, box, copy);
        copy->Delete();
        }

      // Copy meta data for each dataset within a level.
      if (input->HasMetaData(levelIdx, cc))
        {
        output->GetMetaData(levelIdx, cc)->Copy(
          input->GetMetaData(levelIdx, cc));
        }
      }
    }

  return 1;
}

//----------------------------------------------------------------------------
vtkUniformGrid* vtkLevelIdScalars::ColorLevel(
  vtkUniformGrid* input, int group)
{
  vtkUniformGrid* output = 0;
  output = input->NewInstance();
  output->ShallowCopy(input);
  vtkDataSet* dsOutput = vtkDataSet::SafeDownCast(output);
  vtkIdType numCells = dsOutput->GetNumberOfCells();
  vtkUnsignedCharArray* cArray = vtkUnsignedCharArray::New();
  cArray->SetNumberOfTuples(numCells);
  for (vtkIdType cellIdx=0; cellIdx<numCells; cellIdx++)
    {
    cArray->SetValue(cellIdx, group);
    }
  cArray->SetName("BlockIdScalars");
  dsOutput->GetCellData()->AddArray(cArray);
  cArray->Delete();
  return output;
}


//----------------------------------------------------------------------------
void vtkLevelIdScalars::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

