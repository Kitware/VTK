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

vtkCxxRevisionMacro(vtkExtractSelectedIds, "1.6");
vtkStandardNewMacro(vtkExtractSelectedIds);

//----------------------------------------------------------------------------
vtkExtractSelectedIds::vtkExtractSelectedIds()
{
  this->SetNumberOfInputPorts(2);
}

//----------------------------------------------------------------------------
vtkExtractSelectedIds::~vtkExtractSelectedIds()
{
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
    }
  return 1;
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

  vtkIdType numIds = idArray->GetNumberOfTuples();
  if (numIds == 0)
    {
    return 1;
    }

  //try to find an array to use for ID labels
  vtkIdTypeArray *labelArray = NULL;
  if (sel->GetProperties()->Has(vtkSelection::ARRAY_NAME()))
      {
      //user chose a specific label array
      labelArray = vtkIdTypeArray::SafeDownCast(
        input->GetCellData()->GetArray(
          sel->GetProperties()->Get(vtkSelection::ARRAY_NAME())
          )
        );      
      }
  if (labelArray == NULL)
    {
    //user didn't specify an array, try to use the globalid array
    labelArray = vtkIdTypeArray::SafeDownCast(input->GetCellData()->GetGlobalIds());
    }

  vtkCell *cell;
  vtkIdList *cellPts;
  vtkIdType newCellId;
  vtkIdType numCellPts;
  
  vtkPointData *pd = input->GetPointData();
  vtkCellData *cd = input->GetCellData();
  vtkPointData *outputPD = output->GetPointData();
  vtkCellData *outputCD = output->GetCellData();
  outputPD->CopyAllocate(pd);
  outputCD->CopyAllocate(cd);
  vtkIdTypeArray *originalCellIds = NULL;
  originalCellIds = vtkIdTypeArray::New();
  originalCellIds->SetNumberOfComponents(1);
  originalCellIds->SetName("vtkOriginalCellIds");
  outputCD->AddArray(originalCellIds);
  
  vtkIdType numPts = input->GetNumberOfPoints();
  vtkIdType numCells = input->GetNumberOfCells();

  vtkPoints *newPts = vtkPoints::New();
  newPts->Allocate(numPts/4,numPts);

  output->Allocate(numCells/4); //allocate storage for geometry/topology

  vtkIdType *pointMap = new vtkIdType[numPts]; // maps old point ids into new
  vtkIdList *newCellPts = vtkIdList::New();
  newCellPts->Allocate(VTK_CELL_SIZE);

  vtkIdType updateInterval = numCells/1000 + 1;

  vtkIdType ptId, newPointId;

  //initialize all points to say not looked at
  for (ptId=0; ptId < numPts; ptId++)
    {
    pointMap[ptId] = -1;
    }
  
  // Loop over all cells to see whether they are inside.
  vtkIdType cellId;
  for (cellId=0; cellId < numCells; cellId++)
    {
    if ( ! (cellId % updateInterval) ) //manage progress reports 
      {
      this->UpdateProgress ((float)cellId / numCells);
      }

    cell = input->GetCell(cellId);
    cellPts = cell->GetPointIds();
    numCellPts = cell->GetNumberOfPoints();    
    newCellPts->Reset();

    vtkIdType id;
    int isect = 0;
    if (labelArray == NULL)
      {
      //check if this cell's offset is one of the selected ids
      for (id = 0; id < numIds; id++)
        {
        if (idArray->GetValue(id) == cellId)
          {
          isect = 1;
          break;
          }
        }
      }
    else
      {
      //check if this cell's label is one of the selected ids
      vtkIdType id2find = labelArray->GetValue(cellId);
      for (id = 0; id < numIds; id++)
        {
        if (idArray->GetValue(id) == id2find)
          {
          isect = 1;
          break;
          }
        }
      }
      
    if (isect == 1)
      {
      //intersects, put all of the points inside
      for (vtkIdType i=0; i < numCellPts; i++)
        {
        ptId = cellPts->GetId(i);
        newPointId = pointMap[ptId];
        if (newPointId < 0)
          {          
          double X[3];
          input->GetPoint(ptId, X);      
          newPointId = newPts->InsertNextPoint(X);
          outputPD->CopyData(pd,ptId,newPointId);
          pointMap[ptId] = newPointId;
          }
        newCellPts->InsertId(i,newPointId);
        }
      
      newCellId = output->InsertNextCell(cell->GetCellType(),newCellPts);
      outputCD->CopyData(cd,cellId,newCellId);
      originalCellIds->InsertNextValue(cellId);
      }
    }//for all cells
  
  output->SetPoints(newPts);
  newPts->Delete();
  originalCellIds->Delete();
  delete [] pointMap;
  newCellPts->Delete();
  output->Squeeze();

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

  vtkIdType numIds = idArray->GetNumberOfTuples();
  if (numIds == 0)
    {
    return 1;
    }

  //try to find an array to use for ID labels
  vtkIdTypeArray *labelArray = NULL;
  if (sel->GetProperties()->Has(vtkSelection::ARRAY_NAME()))
      {
      //user chose a specific label array
      labelArray = vtkIdTypeArray::SafeDownCast(
        input->GetPointData()->GetArray(
          sel->GetProperties()->Get(vtkSelection::ARRAY_NAME())
          )
        );      
      }
  if (labelArray == NULL)
    {
    //user didn't specify an array, try to use the globalid array
    labelArray = vtkIdTypeArray::SafeDownCast(input->GetPointData()->GetGlobalIds());
    }
  
  //copy the point data attribute array names, data types and widths
  vtkPoints *opoints = vtkPoints::New();
  output->SetPoints(opoints);
  opoints->Delete();
  output->Allocate(numIds);
  vtkPointData *opd = output->GetPointData();
  vtkPointData *ipd = input->GetPointData();
  opd->CopyStructure(ipd);
  opd->CopyAllOn();
  opd->CopyAllocate(ipd);
  vtkIdType outloc = 0;
  if (labelArray == NULL)
    {
    //using offset within point data as the ID
    //this is fast but doesn't work in parallel
    for (vtkIdType i = 0; i < numIds; i++)
      {
      double X[3];      
      vtkIdType id2find = idArray->GetValue(i);
      if (id2find > 0 && id2find < input->GetNumberOfPoints())
        {
        input->GetPoint(id2find, X);
        output->GetPoints()->InsertNextPoint(X);
        output->InsertNextCell(VTK_VERTEX, 1, &outloc);
        opd->CopyData(ipd, id2find, outloc);
        outloc++;
        }
      }
    }
  else
    {
    //each point has a label (such as the globalidarray) use that
    //TODO: sort both idarray and labelarray and step through them so that
    //we don't have n^2 run time
    for (vtkIdType i = 0; i < numIds; i++)
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
