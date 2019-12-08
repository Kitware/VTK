/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExplicitStructuredGridToUnstructuredGrid.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkExplicitStructuredGridToUnstructuredGrid.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkDataSetAttributes.h"
#include "vtkExplicitStructuredGrid.h"
#include "vtkIdList.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkUnstructuredGrid.h"

vtkStandardNewMacro(vtkExplicitStructuredGridToUnstructuredGrid);

//----------------------------------------------------------------------------
int vtkExplicitStructuredGridToUnstructuredGrid::RequestData(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // Retrieve input and output
  vtkExplicitStructuredGrid* input = vtkExplicitStructuredGrid::GetData(inputVector[0], 0);
  vtkUnstructuredGrid* output = vtkUnstructuredGrid::GetData(outputVector, 0);

  // Copy field data
  output->GetFieldData()->ShallowCopy(input->GetFieldData());

  // Copy input point data to output
  vtkDataSetAttributes* inPointData = static_cast<vtkDataSetAttributes*>(input->GetPointData());
  vtkDataSetAttributes* outPointData = static_cast<vtkDataSetAttributes*>(output->GetPointData());
  if (outPointData && inPointData)
  {
    outPointData->DeepCopy(inPointData);
  }

  output->SetPoints(input->GetPoints());

  // Initialize output cell data
  vtkDataSetAttributes* inCellData = static_cast<vtkDataSetAttributes*>(input->GetCellData());
  vtkDataSetAttributes* outCellData = static_cast<vtkDataSetAttributes*>(output->GetCellData());
  outCellData->CopyAllocate(inCellData);

  vtkIdType nbCells = input->GetNumberOfCells();

  // CellArray which links the new cells ids with the old ones
  vtkNew<vtkIdTypeArray> originalCellIds;
  originalCellIds->SetName("vtkOriginalCellIds");
  originalCellIds->SetNumberOfComponents(1);
  originalCellIds->Allocate(nbCells);

  vtkNew<vtkIntArray> iArray;
  iArray->SetName("BLOCK_I");
  iArray->SetNumberOfComponents(1);
  iArray->Allocate(nbCells);

  vtkNew<vtkIntArray> jArray;
  jArray->SetName("BLOCK_J");
  jArray->SetNumberOfComponents(1);
  jArray->Allocate(nbCells);

  vtkNew<vtkIntArray> kArray;
  kArray->SetName("BLOCK_K");
  kArray->SetNumberOfComponents(1);
  kArray->Allocate(nbCells);

  vtkNew<vtkCellArray> cells;
  cells->AllocateEstimate(nbCells, 8);
  int i, j, k;
  for (vtkIdType cellId = 0; cellId < nbCells; cellId++)
  {
    if (input->IsCellVisible(cellId))
    {
      vtkNew<vtkIdList> ptIds;
      input->GetCellPoints(cellId, ptIds.GetPointer());
      vtkIdType newCellId = cells->InsertNextCell(ptIds.GetPointer());
      outCellData->CopyData(inCellData, cellId, newCellId);
      originalCellIds->InsertValue(newCellId, cellId);
      input->ComputeCellStructuredCoords(cellId, i, j, k);
      iArray->InsertValue(newCellId, i);
      jArray->InsertValue(newCellId, j);
      kArray->InsertValue(newCellId, k);
    }
  }
  originalCellIds->Squeeze();
  iArray->Squeeze();
  jArray->Squeeze();
  kArray->Squeeze();
  output->SetCells(VTK_HEXAHEDRON, cells.GetPointer());
  outCellData->AddArray(originalCellIds.GetPointer());
  outCellData->AddArray(iArray.GetPointer());
  outCellData->AddArray(jArray.GetPointer());
  outCellData->AddArray(kArray.GetPointer());

  this->UpdateProgress(1.);

  return 1;
}

//----------------------------------------------------------------------------
int vtkExplicitStructuredGridToUnstructuredGrid::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkExplicitStructuredGrid");
  return 1;
}
