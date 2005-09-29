/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMultiGroupDataGroupIdScalars.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkMultiGroupDataGroupIdScalars.h"

#include "vtkCellData.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataSet.h"
#include "vtkMultiGroupDataSet.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkUnsignedCharArray.h"

vtkCxxRevisionMacro(vtkMultiGroupDataGroupIdScalars, "1.1");
vtkStandardNewMacro(vtkMultiGroupDataGroupIdScalars);

// Construct object with PointIds and CellIds on; and ids being generated
// as scalars.
vtkMultiGroupDataGroupIdScalars::vtkMultiGroupDataGroupIdScalars()
{
}

vtkMultiGroupDataGroupIdScalars::~vtkMultiGroupDataGroupIdScalars()
{
}

// 
// Map ids into attribute data
//
int vtkMultiGroupDataGroupIdScalars::RequestData(
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

  unsigned int numGroups = input->GetNumberOfGroups();
  output->SetNumberOfGroups(numGroups);

  for (unsigned int group=0; group<numGroups; group++)
    {
    unsigned int numDataSets = input->GetNumberOfDataSets(group);
    output->SetNumberOfDataSets(group, numDataSets);
    for (unsigned int dataSet=0; dataSet<numDataSets; dataSet++)
      {
      vtkDataSet* dObj = vtkDataSet::SafeDownCast(
        input->GetDataSet(group, dataSet));
      if (dObj)
        {
        vtkDataSet* copy = dObj->NewInstance();
        copy->ShallowCopy(dObj);
        output->SetDataSet(group, dataSet, copy);
        vtkIdType numCells = copy->GetNumberOfCells();
        vtkUnsignedCharArray* cArray = vtkUnsignedCharArray::New();
        cArray->SetNumberOfTuples(numCells);
        for (vtkIdType cellIdx=0; cellIdx<numCells; cellIdx++)
          {
          cArray->SetValue(cellIdx, group);
          }
        cArray->SetName("GroupIdScalars");
        copy->GetCellData()->AddArray(cArray);
        cArray->Delete();
        copy->Delete();
        }
      }
    }

  return 1;
}

void vtkMultiGroupDataGroupIdScalars::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
