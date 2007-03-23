/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractSelectedPolyDataIds.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkExtractSelectedPolyDataIds.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkFloatArray.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkSelection.h"

vtkCxxRevisionMacro(vtkExtractSelectedPolyDataIds, "1.3");
vtkStandardNewMacro(vtkExtractSelectedPolyDataIds);

//----------------------------------------------------------------------------
vtkExtractSelectedPolyDataIds::vtkExtractSelectedPolyDataIds()
{
  this->SetNumberOfInputPorts(2);
}

//----------------------------------------------------------------------------
vtkExtractSelectedPolyDataIds::~vtkExtractSelectedPolyDataIds()
{
}


//----------------------------------------------------------------------------
int vtkExtractSelectedPolyDataIds::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *selInfo = inputVector[1]->GetInformationObject(0);
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and ouptut
  vtkPolyData *input = vtkPolyData::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkSelection *sel = vtkSelection::SafeDownCast(
    selInfo->Get(vtkDataObject::DATA_OBJECT()));
  if ( ! sel )
    {
    vtkErrorMacro(<<"No selection specified");
    return 1;
    }

  vtkPointData *pd = input->GetPointData();
  vtkCellData *cd = input->GetCellData();

  vtkPointData *outputPD = output->GetPointData();
  vtkCellData *outputCD = output->GetCellData();

  vtkDebugMacro(<< "Extracting poly data geometry");

  if (!sel->GetProperties()->Has(vtkSelection::CONTENT_TYPE()) ||
      sel->GetProperties()->Get(vtkSelection::CONTENT_TYPE()) != vtkSelection::OFFSETS ||
      !sel->GetProperties()->Has(vtkSelection::FIELD_TYPE()) ||
      sel->GetProperties()->Get(vtkSelection::FIELD_TYPE()) != vtkSelection::CELL)
    {
    return 1;
    }

  vtkIdTypeArray* idArray = 
    vtkIdTypeArray::SafeDownCast(sel->GetSelectionList());

  if (!idArray)
    {
    return 1;
    }

  vtkIdType numCells = 
    idArray->GetNumberOfComponents()*idArray->GetNumberOfTuples();

  if (numCells == 0)
    {
    return 1;
    }

  output->Allocate(numCells);
  output->SetPoints(input->GetPoints());
  outputPD->PassData(pd);

  // Now loop over all cells to see whether they are in the selection.
  // Copy if they are.

  vtkIdList* ids = vtkIdList::New();

  vtkIdType numInputCells = input->GetNumberOfCells();
  for (vtkIdType i=0; i < numCells; i++)
    {
    vtkIdType cellId = idArray->GetValue(i);
    if (cellId >= numInputCells)
      {
      continue;
      }
    input->GetCellPoints(cellId, ids);
    vtkIdType newId = output->InsertNextCell(
      input->GetCellType(cellId), ids);
    outputCD->CopyData(cd, cellId, newId);
    }
  ids->Delete();
  output->Squeeze();

  return 1;
}

//----------------------------------------------------------------------------
void vtkExtractSelectedPolyDataIds::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
int vtkExtractSelectedPolyDataIds::FillInputPortInformation(
  int port, vtkInformation* info)
{
  if (port==0)
    {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPolyData");    
    }
  else
    {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkSelection");
    }
  return 1;
}
