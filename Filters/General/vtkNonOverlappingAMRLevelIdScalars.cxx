/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkNonOverlappingAMRLevelIdScalars.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkNonOverlappingAMRLevelIdScalars.h"

#include "vtkCellData.h"
#include "vtkUniformGridAMR.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkUniformGrid.h"
#include "vtkUnsignedCharArray.h"
#include "vtkNonOverlappingAMR.h"

#include <cassert>

vtkStandardNewMacro(vtkNonOverlappingAMRLevelIdScalars);
//----------------------------------------------------------------------------
vtkNonOverlappingAMRLevelIdScalars::vtkNonOverlappingAMRLevelIdScalars()
{
}

//----------------------------------------------------------------------------
vtkNonOverlappingAMRLevelIdScalars::~vtkNonOverlappingAMRLevelIdScalars()
{
}

//----------------------------------------------------------------------------
void vtkNonOverlappingAMRLevelIdScalars::AddColorLevels(
    vtkUniformGridAMR *input, vtkUniformGridAMR *output)
{
  assert( "pre: input should not be NULL" && (input != NULL) );
  assert( "pre: output should not be NULL" && (output != NULL)  );

  unsigned int numLevels = input->GetNumberOfLevels();
  output->SetNumberOfLevels(numLevels);

  for (unsigned int levelIdx=0; levelIdx<numLevels; levelIdx++)
    {
    unsigned int numDS = input->GetNumberOfDataSets(levelIdx);
    output->SetNumberOfDataSets(levelIdx, numDS);

    // Copy level metadata.
    if (input->HasLevelMetaData(levelIdx))
      {
      output->GetLevelMetaData(levelIdx)->
          Copy(input->GetLevelMetaData(levelIdx));
      }

    for (unsigned int cc=0; cc < numDS; cc++)
      {
      vtkUniformGrid* ds = input->GetDataSet(levelIdx,cc);
      if(ds != NULL)
        {
        vtkUniformGrid* copy = this->ColorLevel(ds, levelIdx);
        output->SetDataSet(levelIdx,cc,copy);
        copy->Delete();
        }

      // Copy meta data for each dataset within a level.
      if (input->HasMetaData(levelIdx, cc))
        {
        output->GetMetaData(levelIdx, cc)->
            Copy(input->GetMetaData(levelIdx, cc));
        }
      }
    }

}

//----------------------------------------------------------------------------
// Map ids into attribute data
int vtkNonOverlappingAMRLevelIdScalars::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkUniformGridAMR *input = vtkUniformGridAMR::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  if (!input)
    {
    return 0;
    }

  vtkInformation* info = outputVector->GetInformationObject(0);
  vtkUniformGridAMR *output = vtkUniformGridAMR::SafeDownCast(
    info->Get(vtkDataObject::DATA_OBJECT()));
  if (!output)
    {
    return 0;
    }

  this->AddColorLevels(input, output);

  return 1;
}

//----------------------------------------------------------------------------
vtkUniformGrid* vtkNonOverlappingAMRLevelIdScalars::ColorLevel(
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
void vtkNonOverlappingAMRLevelIdScalars::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
