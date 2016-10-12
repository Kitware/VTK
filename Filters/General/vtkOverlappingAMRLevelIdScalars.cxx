/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOverlappingAMRLevelIdScalars.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOverlappingAMRLevelIdScalars.h"

#include "vtkCellData.h"
#include "vtkUniformGridAMR.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkUniformGrid.h"
#include "vtkUnsignedCharArray.h"
#include "vtkOverlappingAMR.h"

#include <cassert>

vtkStandardNewMacro(vtkOverlappingAMRLevelIdScalars);
//----------------------------------------------------------------------------
vtkOverlappingAMRLevelIdScalars::vtkOverlappingAMRLevelIdScalars()
{
}

//----------------------------------------------------------------------------
vtkOverlappingAMRLevelIdScalars::~vtkOverlappingAMRLevelIdScalars()
{
}

//----------------------------------------------------------------------------
void vtkOverlappingAMRLevelIdScalars::AddColorLevels(
    vtkUniformGridAMR *input, vtkUniformGridAMR *output)
{
  assert( "pre: input should not be NULL" && (input != NULL) );
  assert( "pre: output should not be NULL" && (output != NULL)  );

  unsigned int numLevels = input->GetNumberOfLevels();
  output->CopyStructure(input);
  for (unsigned int levelIdx=0; levelIdx<numLevels; levelIdx++)
  {
    unsigned int numDS = input->GetNumberOfDataSets(levelIdx);
    for (unsigned int cc=0; cc < numDS; cc++)
    {
      vtkUniformGrid* ds = input->GetDataSet(levelIdx,cc);
      if(ds != NULL)
      {
        vtkUniformGrid* copy = this->ColorLevel(ds, levelIdx);
        output->SetDataSet(levelIdx,cc,copy);
        copy->Delete();
      }
    }
  }

}

//----------------------------------------------------------------------------
// Map ids into attribute data
int vtkOverlappingAMRLevelIdScalars::RequestData(
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
vtkUniformGrid* vtkOverlappingAMRLevelIdScalars::ColorLevel(
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
void vtkOverlappingAMRLevelIdScalars::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
