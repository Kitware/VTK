/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractSelectedIds.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkExtractSelectedIds.h"

#include "vtkDataSet.h"
#include "vtkExtractCells.h"
#include "vtkIdList.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkSelection.h"
#include "vtkUnstructuredGrid.h"
#include "vtkCellData.h"
#include "vtkPointData.h"
#include "vtkCellType.h"
#include "vtkCellArray.h"

vtkCxxRevisionMacro(vtkExtractSelectedIds, "1.3");
vtkStandardNewMacro(vtkExtractSelectedIds);

//----------------------------------------------------------------------------
vtkExtractSelectedIds::vtkExtractSelectedIds()
{
  this->SetNumberOfInputPorts(2);
  this->ExtractFilter = vtkExtractCells::New();
}

//----------------------------------------------------------------------------
vtkExtractSelectedIds::~vtkExtractSelectedIds()
{
  this->ExtractFilter->Delete();
}

//----------------------------------------------------------------------------
int vtkExtractSelectedIds::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *selInfo = inputVector[1]->GetInformationObject(0);
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the selection, input and ouptut
  vtkDataSet *input = vtkDataSet::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkUnstructuredGrid *output = vtkUnstructuredGrid::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkSelection *sel = vtkSelection::SafeDownCast(
    selInfo->Get(vtkDataObject::DATA_OBJECT()));
  if ( ! sel )
    {
    vtkErrorMacro(<<"No selection specified");
    return 1;
    }

  vtkDebugMacro(<< "Extracting from dataset");


  if (!sel->GetProperties()->Has(vtkSelection::CONTENT_TYPE())
      || 
      (sel->GetProperties()->Get(vtkSelection::CONTENT_TYPE()) != vtkSelection::IDS))
    {
    return 1;
    }
  
  int fieldType = vtkSelection::CELL;
  if (sel->GetProperties()->Has(vtkSelection::FIELD_TYPE()))
    {
    fieldType = sel->GetProperties()->Get(vtkSelection::FIELD_TYPE());
    }
  switch (fieldType)
    {
    case vtkSelection::CELL:
      return this->ExtractCells(sel, input, output);
      break;
    case vtkSelection::POINT:
      return this->ExtractPoints(sel, input, output);
    default:
      return 1;
    }
}

//----------------------------------------------------------------------------
int vtkExtractSelectedIds::ExtractCells(
  vtkSelection *sel,  vtkDataSet *input,
  vtkUnstructuredGrid *output)
{
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
int vtkExtractSelectedIds::ExtractPoints(
  vtkSelection *sel,  vtkDataSet *input,
  vtkUnstructuredGrid *output)
{
  vtkIdTypeArray* idArray = 
    vtkIdTypeArray::SafeDownCast(sel->GetSelectionList());
  if (!idArray)
    {
    return 1;
    }

  vtkIdType numPoints = 
    idArray->GetNumberOfComponents()*idArray->GetNumberOfTuples();
  if (numPoints == 0)
    {
    return 1;
    }

  //try to find an array to use for point labels
  vtkIdTypeArray *labelArray = NULL;
  if (sel->GetProperties()->Has(vtkSelection::NAME()))
      {
      //user chose a specific label array
      labelArray = vtkIdTypeArray::SafeDownCast(
        input->GetPointData()->GetArray(
          sel->GetProperties()->Get(vtkSelection::NAME())
          )
        );      
      }
  if (labelArray == NULL)
    {
    //user didn't specify an array, try to use the globalid array
    //that's what its for afterall
    labelArray = vtkIdTypeArray::SafeDownCast(input->GetPointData()->GetGlobalIds());
    }
  
  //copy the point data attribute array names, data types and widths
  vtkPoints *opoints = vtkPoints::New();
  output->SetPoints(opoints);
  opoints->Delete();
  output->Allocate(numPoints);
  vtkPointData *opd = output->GetPointData();
  vtkPointData *ipd = input->GetPointData();
  opd->CopyStructure(ipd);
  opd->CopyAllOn();
  opd->CopyAllocate(ipd);
  if (labelArray == NULL)
    {
    //using offset within point data as the ID
    //this is fast but doesn't work well in parallel
    for (vtkIdType i = 0; i < numPoints; i++)
      {
      double X[3];      
      vtkIdType id2find = idArray->GetValue(i);
      input->GetPoint(id2find, X);
      output->GetPoints()->InsertNextPoint(X);
      opd->CopyData(ipd, id2find, i);
      output->InsertNextCell(VTK_VERTEX, 1, &i);
      }
    }
  else
    {
    //each point has a label (such as the globalidarray) use that
    //TODO: sort both idarray and labelarray and step through them so that
    //we don't have n^2 run time
    vtkIdType outloc = 0;
    for (vtkIdType i = 0; i < numPoints; i++)
      {
      double X[3];      
      vtkIdType id2find = idArray->GetValue(i);
      bool found = false;
      vtkIdType idfound = 0;
      vtkIdType actualLocation = 0;
      for (vtkIdType j = 0; j < input->GetNumberOfPoints(); j++)
        {
        idfound = labelArray->GetValue(j);
        if (idfound == id2find)
          {
          actualLocation = j;
          found = true;
          break;
          }          
        }
      if (found)
        {
        input->GetPoint(actualLocation, X);
        output->GetPoints()->InsertNextPoint(X);
        output->InsertNextCell(VTK_VERTEX, 1, &outloc);
        opd->CopyData(ipd, actualLocation, outloc);
        outloc++;
        }
      }
    }
  output->Squeeze();
  return 1;
}

//----------------------------------------------------------------------------
void vtkExtractSelectedIds::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

}

//----------------------------------------------------------------------------
int vtkExtractSelectedIds::FillInputPortInformation(
  int port, vtkInformation* info)
{
  if (port==0)
    {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");    
    }
  else
    {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkSelection");
    }
  return 1;
}
