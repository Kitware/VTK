/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHierarchicalDataLevelFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkHierarchicalDataLevelFilter.h"

#include "vtkCellData.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataSet.h"
#include "vtkHierarchicalDataSet.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkUnsignedCharArray.h"

vtkCxxRevisionMacro(vtkHierarchicalDataLevelFilter, "1.1");
vtkStandardNewMacro(vtkHierarchicalDataLevelFilter);

// Construct object with PointIds and CellIds on; and ids being generated
// as scalars.
vtkHierarchicalDataLevelFilter::vtkHierarchicalDataLevelFilter()
{
}

vtkHierarchicalDataLevelFilter::~vtkHierarchicalDataLevelFilter()
{
}

// 
// Map ids into attribute data
//
int vtkHierarchicalDataLevelFilter::RequestData(
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

  unsigned int numLevels = input->GetNumberOfLevels();
  output->SetNumberOfLevels(numLevels);

  for (unsigned int level=0; level<numLevels; level++)
    {
    unsigned int numDataSets = input->GetNumberOfDataSets(level);
    output->SetNumberOfDataSets(level, numDataSets);
    for (unsigned int dataSet=0; dataSet<numDataSets; dataSet++)
      {
      vtkDataSet* dObj = vtkDataSet::SafeDownCast(
        input->GetDataSet(level, dataSet));
      if (dObj)
        {
        vtkDataSet* copy = dObj->NewInstance();
        copy->ShallowCopy(dObj);
        output->SetDataSet(level, dataSet, copy);
        vtkIdType numCells = copy->GetNumberOfCells();
        vtkUnsignedCharArray* cArray = vtkUnsignedCharArray::New();
        cArray->SetNumberOfTuples(numCells);
        for (vtkIdType cellIdx=0; cellIdx<numCells; cellIdx++)
          {
          cArray->SetValue(cellIdx, level);
          }
        cArray->SetName("LevelScalars");
        copy->GetCellData()->AddArray(cArray);
        cArray->Delete();
        copy->Delete();
        }
      }
    }

  return 1;
}

void vtkHierarchicalDataLevelFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
