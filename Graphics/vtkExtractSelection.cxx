/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractSelection.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkExtractSelection.h"

#include "vtkDataSet.h"
#include "vtkExtractCells.h"
#include "vtkIdList.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkSelection.h"
#include "vtkUnstructuredGrid.h"

vtkCxxRevisionMacro(vtkExtractSelection, "1.2");
vtkStandardNewMacro(vtkExtractSelection);
vtkCxxSetObjectMacro(vtkExtractSelection,
                     Selection,vtkSelection);

//----------------------------------------------------------------------------
vtkExtractSelection::vtkExtractSelection()
{
  this->Selection = 0;
  this->ExtractFilter = vtkExtractCells::New();
}

//----------------------------------------------------------------------------
vtkExtractSelection::~vtkExtractSelection()
{
  this->SetSelection(NULL);
  this->ExtractFilter->Delete();
}

//----------------------------------------------------------------------------
// Overload standard modified time function. If selection is modified,
// then this object is modified as well.
unsigned long vtkExtractSelection::GetMTime()
{
  unsigned long mTime=this->MTime.GetMTime();
  unsigned long selTime;

  if ( this->Selection != NULL )
    {
    selTime = this->Selection->GetMTime();
    mTime = ( selTime > mTime ? selTime : mTime );
    }

  return mTime;
}

//----------------------------------------------------------------------------
int vtkExtractSelection::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and ouptut
  vtkDataSet *input = vtkDataSet::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkUnstructuredGrid *output = vtkUnstructuredGrid::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkDebugMacro(<< "Extracting from dataset");

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

  vtkIdList* ids = vtkIdList::New();
  vtkIdType* idsPtr = ids->WritePointer(0, numCells);

  memcpy(idsPtr, idArray->GetPointer(0), numCells*sizeof(vtkIdType));

  this->ExtractFilter->SetCellList(ids);
  ids->Delete();

  vtkDataSet* inputCopy = input->NewInstance();
  inputCopy->ShallowCopy(input);
  this->ExtractFilter->SetInput(inputCopy);
  inputCopy->Delete();

  this->ExtractFilter->Update();

  vtkUnstructuredGrid* ecOutput = vtkUnstructuredGrid::SafeDownCast(
    this->ExtractFilter->GetOutputDataObject(0));
  output->ShallowCopy(ecOutput);
  ecOutput->Initialize();

  return 1;
}

//----------------------------------------------------------------------------
void vtkExtractSelection::PrintSelf(ostream& os, vtkIndent indent)
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

//----------------------------------------------------------------------------
int vtkExtractSelection::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  return 1;
}
