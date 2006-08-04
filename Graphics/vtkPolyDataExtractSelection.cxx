/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolyDataExtractSelection.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPolyDataExtractSelection.h"

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

vtkCxxRevisionMacro(vtkPolyDataExtractSelection, "1.1");
vtkStandardNewMacro(vtkPolyDataExtractSelection);
vtkCxxSetObjectMacro(vtkPolyDataExtractSelection,
                     Selection,vtkSelection);

// Construct object with ExtractInside turned on.
vtkPolyDataExtractSelection::vtkPolyDataExtractSelection()
{
  this->Selection = 0;
}

vtkPolyDataExtractSelection::~vtkPolyDataExtractSelection()
{
  this->SetSelection(NULL);
}

// Overload standard modified time function. If implicit function is modified,
// then this object is modified as well.
unsigned long vtkPolyDataExtractSelection::GetMTime()
{
  unsigned long mTime=this->MTime.GetMTime();
  unsigned long impFuncMTime;

  if ( this->Selection != NULL )
    {
    impFuncMTime = this->Selection->GetMTime();
    mTime = ( impFuncMTime > mTime ? impFuncMTime : mTime );
    }

  return mTime;
}

int vtkPolyDataExtractSelection::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and ouptut
  vtkPolyData *input = vtkPolyData::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkPointData *pd = input->GetPointData();
  vtkCellData *cd = input->GetCellData();

  vtkPointData *outputPD = output->GetPointData();
  vtkCellData *outputCD = output->GetCellData();

  vtkDebugMacro(<< "Extracting poly data geometry");

  vtkSelection* sel = this->Selection;
  if ( ! sel )
    {
    vtkErrorMacro(<<"No selection specified");
    return 1;
    }

  if (!sel->GetProperties()->Has(vtkSelection::CONTENT_TYPE()) ||
      sel->GetProperties()->Get(vtkSelection::CONTENT_TYPE()) != vtkSelection::CELL_IDS)
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

void vtkPolyDataExtractSelection::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Selection: ";
  if (this->Selection)
    {
    this->Selection->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "(none)" << endl;
    }
}
