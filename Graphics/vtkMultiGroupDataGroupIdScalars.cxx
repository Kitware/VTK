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

vtkCxxRevisionMacro(vtkMultiGroupDataGroupIdScalars, "1.3");
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
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  if (!input) {return 0;}

  vtkInformation* info = outputVector->GetInformationObject(0);
  vtkMultiGroupDataSet *output = vtkMultiGroupDataSet::SafeDownCast(
    info->Get(vtkDataObject::DATA_OBJECT()));
  if (!output) {return 0;}

  unsigned int numGroups = input->GetNumberOfGroups();
  output->SetNumberOfGroups(numGroups);

  for (unsigned int group=0; group<numGroups; group++)
    {
    unsigned int numDataSets = input->GetNumberOfDataSets(group);
    output->SetNumberOfDataSets(group, numDataSets);
    for (unsigned int dataSet=0; dataSet<numDataSets; dataSet++)
      {
      vtkDataObject* dObj = input->GetDataSet(group, dataSet);
      if (dObj)
        {
        vtkDataObject* block = this->ColorBlock(dObj, group);
        if (block)
          {
          output->SetDataSet(group, dataSet, block);
          block->Delete();
          }
        }
      }
    }

  return 1;
}

vtkDataObject* vtkMultiGroupDataGroupIdScalars::ColorBlock(
  vtkDataObject* input, int group)
{
  vtkDataObject* output = 0;
  if (input->IsA("vtkMultiGroupDataSet"))
    {
    vtkMultiGroupDataSet* mbInput = 
      vtkMultiGroupDataSet::SafeDownCast(input);

    output = input->NewInstance();
    vtkMultiGroupDataSet* mbOutput =
      vtkMultiGroupDataSet::SafeDownCast(output);

    unsigned int numGroups = mbInput->GetNumberOfGroups();
    mbOutput->SetNumberOfGroups(numGroups);
    
    for (unsigned int group=0; group<numGroups; group++)
      {
      unsigned int numDataSets = mbInput->GetNumberOfDataSets(group);
      mbOutput->SetNumberOfDataSets(group, numDataSets);
      for (unsigned int dataSet=0; dataSet<numDataSets; dataSet++)
        {
        vtkDataObject* dObj = mbInput->GetDataSet(group, dataSet);
        vtkDataObject* outBlock = this->ColorBlock(dObj, group);
        if (outBlock)
          {
          mbOutput->SetDataSet(group, dataSet, outBlock);
          outBlock->Delete();
          }
        }
      }
    }
  else
    {
    vtkDataSet* ds = vtkDataSet::SafeDownCast(input);
    if (ds)
      {
      output = ds->NewInstance();
      output->ShallowCopy(ds);
      vtkDataSet* dsOutput = vtkDataSet::SafeDownCast(output);
      vtkIdType numCells = dsOutput->GetNumberOfCells();
      vtkUnsignedCharArray* cArray = vtkUnsignedCharArray::New();
      cArray->SetNumberOfTuples(numCells);
      for (vtkIdType cellIdx=0; cellIdx<numCells; cellIdx++)
        {
        cArray->SetValue(cellIdx, group);
        }
      cArray->SetName("GroupIdScalars");
      dsOutput->GetCellData()->AddArray(cArray);
      cArray->Delete();
      }
    }
  return output;
}

void vtkMultiGroupDataGroupIdScalars::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
