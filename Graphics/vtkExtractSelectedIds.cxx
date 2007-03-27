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

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkCellType.h"
#include "vtkExtractCells.h"
#include "vtkSignedCharArray.h"
#include "vtkSortDataArray.h"
#include "vtkSmartPointer.h"
#include "vtkIdList.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkSelection.h"
#include "vtkUnstructuredGrid.h"

vtkCxxRevisionMacro(vtkExtractSelectedIds, "1.10");
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
//needed because parent class sets output type to input type
//and we sometimes want to change it to make an UnstructuredGrid regardless of
//input type
int vtkExtractSelectedIds::RequestDataObject(
  vtkInformation*,
  vtkInformationVector** inputVector ,
  vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if (!inInfo)
    {
    return 0;
    }
  vtkDataSet *input = vtkDataSet::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));

  if (input)
    {
    vtkInformation* selInfo = inputVector[1]->GetInformationObject(0);
    int passThrough = 0;
    int pointsOnly = 1;
    if (inInfo)
      {
      vtkSelection *sel = vtkSelection::SafeDownCast(
        selInfo->Get(vtkDataObject::DATA_OBJECT()));
      if (sel->GetProperties()->Has(vtkSelection::PRESERVE_TOPOLOGY()) &&
          sel->GetProperties()->Get(vtkSelection::PRESERVE_TOPOLOGY()) != 0)
        {
        passThrough = 1;
        }
      if (!sel->GetProperties()->Has(vtkSelection::FIELD_TYPE())
          ||
          sel->GetProperties()->Get(vtkSelection::FIELD_TYPE()) == vtkSelection::CELL
          ||
          (sel->GetProperties()->Has(vtkSelection::CONTAINING_CELLS()) &&
           sel->GetProperties()->Get(vtkSelection::CONTAINING_CELLS()) == 1))
        {
        pointsOnly = 0;
        }
      }

    for(int i=0; i < this->GetNumberOfOutputPorts(); ++i)
      {
      vtkInformation* info = outputVector->GetInformationObject(i);
      vtkDataSet *output = vtkDataSet::SafeDownCast(
        info->Get(vtkDataObject::DATA_OBJECT()));

      if (!output
          ||
          (passThrough && !output->IsA(input->GetClassName()))
          ||
          (!passThrough &&
           ((pointsOnly /*|| input->IsA("vtkPolyData")*/) && !output->IsA("vtkPolyData"))
            ||
            (!(pointsOnly || input->IsA("vtkPolyData")) && !output->IsA("vtkUnstructuredGrid"))
          )
         )
        {
        vtkDataSet* newOutput = NULL;
        if (!passThrough)
          {
          // The mesh will be modified. Polydata is still polydata, but other grids become unstructured.
          if (pointsOnly /*|| input->IsA("vtkPolyData")*/)
            {
            newOutput = vtkPolyData::New();
            } 
          else 
            {
            newOutput = vtkUnstructuredGrid::New();
            }
          }
        else
          {
          // The mesh will not be modified.
          newOutput = input->NewInstance();
          }
        newOutput->SetPipelineInformation(info);
        newOutput->Delete();
        this->GetOutputPortInformation(i)->Set(
          vtkDataObject::DATA_EXTENT_TYPE(), newOutput->GetExtentType());
        }
      }
    return 1;
    }
  return 0;
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
  vtkDataSet *output = vtkDataSet::SafeDownCast(
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
      (
        sel->GetProperties()->Get(vtkSelection::CONTENT_TYPE()) != vtkSelection::GLOBALIDS &&
        sel->GetProperties()->Get(vtkSelection::CONTENT_TYPE()) != vtkSelection::VALUES &&  
        sel->GetProperties()->Get(vtkSelection::CONTENT_TYPE()) != vtkSelection::OFFSETS
        )
    )
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

// Copy the points marked as "in" and build a pointmap
void vtkExtractSelectedIdsCopyPoints(vtkDataSet* input, vtkDataSet* output, signed char* inArray, vtkIdType* pointMap)
{
  vtkPoints* newPts = vtkPoints::New();

  vtkIdType i, numPts = input->GetNumberOfPoints();

  vtkPointData* inPD = input->GetPointData();
  vtkPointData* outPD = output->GetPointData();
  outPD->CopyAllocate(inPD);

  for (i = 0; i < numPts; i++)
    {
    if (inArray[i] > 0)
      {
      pointMap[i] = newPts->InsertNextPoint(input->GetPoint(i));
      outPD->CopyData(inPD, i, pointMap[i]);
      }
    else
      {
      pointMap[i] = -1;
      }
    }

  // outputDS must be either vtkPolyData or vtkUnstructuredGrid
  vtkPointSet::SafeDownCast(output)->SetPoints(newPts);
  newPts->Delete();
}

// Copy the cells marked as "in" using the given pointmap
template <class T>
void vtkExtractSelectedIdsCopyCells(vtkDataSet* input, T* output, signed char* inArray, vtkIdType* pointMap)
{
  vtkIdType numCells = input->GetNumberOfCells();
  output->Allocate(numCells / 4);

  vtkCellData* inCD = input->GetCellData();
  vtkCellData* outCD = output->GetCellData();
  outCD->CopyAllocate(inCD);

  vtkIdTypeArray* originalIds = vtkIdTypeArray::New();
  originalIds->SetNumberOfComponents(1);
  originalIds->SetName("vtkOriginalCellIds");

  vtkIdType i, j, newId = 0;
  vtkIdList* ptIds = vtkIdList::New();
  for (i = 0; i < numCells; i++)
    {
    if (inArray[i] > 0)
      {
      input->GetCellPoints(i, ptIds);
      for (j = 0; j < ptIds->GetNumberOfIds(); j++)
        {
        ptIds->SetId(j, pointMap[ptIds->GetId(j)]);
        }
      output->InsertNextCell(input->GetCellType(i), ptIds);
      outCD->CopyData(inCD, i, newId++);
      originalIds->InsertNextValue(i);
      }
    }

  outCD->AddArray(originalIds);
  originalIds->Delete();
  ptIds->Delete();
}

//----------------------------------------------------------------------------
int vtkExtractSelectedIds::ExtractCells(
  vtkSelection *sel,  vtkDataSet *input,
  vtkDataSet *output)
{
  int passThrough = 0;
  if (sel->GetProperties()->Has(vtkSelection::PRESERVE_TOPOLOGY()))
    {
    passThrough = sel->GetProperties()->Get(vtkSelection::PRESERVE_TOPOLOGY());
    }

  int invert = 0;
  if (sel->GetProperties()->Has(vtkSelection::INVERSE()))
    {
    invert = sel->GetProperties()->Get(vtkSelection::INVERSE());
    }

  vtkIdType i, numPts = input->GetNumberOfPoints();
  vtkSmartPointer<vtkSignedCharArray> pointInArray = vtkSmartPointer<vtkSignedCharArray>::New();
  pointInArray->SetNumberOfComponents(1);
  pointInArray->SetNumberOfTuples(numPts);
  signed char flag = invert ? 1 : -1;
  for (i=0; i < numPts; i++)
    {
    pointInArray->SetValue(i, flag);
    }

  vtkIdType numCells = input->GetNumberOfCells();
  vtkSmartPointer<vtkSignedCharArray> cellInArray = vtkSmartPointer<vtkSignedCharArray>::New();
  cellInArray->SetNumberOfComponents(1);
  cellInArray->SetNumberOfTuples(numCells);
  for (i=0; i < numCells; i++)
    {
    cellInArray->SetValue(i, flag);
    }

  if (passThrough)
    {
    output->ShallowCopy(input);
    pointInArray->SetName("vtkInsidedness");
    vtkPointData *outPD = output->GetPointData();
    outPD->AddArray(pointInArray);
    outPD->SetScalars(pointInArray);
    cellInArray->SetName("vtkInsidedness");
    vtkCellData *outCD = output->GetCellData();
    outCD->AddArray(cellInArray);
    outCD->SetScalars(cellInArray);
    }

  vtkIdTypeArray *idxArray = vtkIdTypeArray::New();
  idxArray->SetNumberOfComponents(1);
  idxArray->SetNumberOfTuples(numCells);
  for (i=0; i < numCells; i++)
    {
    idxArray->SetValue(i, i);
    }

  //decide what the IDS mean
  vtkIdTypeArray *labelArray = NULL;
  int selType = sel->GetProperties()->Get(vtkSelection::CONTENT_TYPE());
  if (selType == vtkSelection::GLOBALIDS)
    {
    labelArray = vtkIdTypeArray::SafeDownCast(
      input->GetCellData()->GetGlobalIds());
    }
  else if (selType == vtkSelection::VALUES &&
           sel->GetProperties()->Has(vtkSelection::ARRAY_NAME()))
    {
    //user chose a specific label array
    labelArray = vtkIdTypeArray::SafeDownCast(
      input->GetCellData()->GetArray(
        sel->GetProperties()->Get(vtkSelection::ARRAY_NAME())
        ));
    }    
  
  if (labelArray == NULL && selType != vtkSelection::OFFSETS)
    {
    return 1;
    }

  if (labelArray)
    {
    vtkIdTypeArray* sortedArray = vtkIdTypeArray::New();
    sortedArray->DeepCopy(labelArray);
    vtkSortDataArray::Sort(sortedArray, idxArray);
    labelArray = sortedArray;
    }
  else
    {
    //no global array, so just use the input cell index
    labelArray = idxArray;
    labelArray->Register(NULL);
    }

  // Reverse the "in" flag
  flag = -flag;

  vtkIdList *ptIds = NULL;
  char* cellCounter = NULL;
  if (invert)
    {
    ptIds = vtkIdList::New();
    cellCounter = new char[numPts];
    for (i = 0; i < numPts; ++i)
      {
      cellCounter[i] = 0;
      }
    }
  vtkIdList *idList = vtkIdList::New();
  vtkIdType numIds = 0, ptId, cellId, idArrayIndex = 0, labelArrayIndex = 0;
  vtkIdTypeArray* idArray = vtkIdTypeArray::SafeDownCast(sel->GetSelectionList());
  if (idArray)
    {
    numIds = idArray->GetNumberOfTuples();
    vtkIdTypeArray* sortedArray = vtkIdTypeArray::New();
    sortedArray->DeepCopy(idArray);
    vtkSortDataArray::SortArrayByComponent(sortedArray, 0);
    idArray = sortedArray;
    }

  // Check each cell to see if it's selected
  while (labelArrayIndex < numCells)
    {
    // Advance through the selection ids until we find
    // one that's NOT LESS THAN the current cell label.
    while ((idArrayIndex < numIds) &&
           (idArray->GetValue(idArrayIndex) < labelArray->GetValue(labelArrayIndex)))
      {
      ++idArrayIndex;
      }

    this->UpdateProgress(static_cast<double>(idArrayIndex) / (numIds * (passThrough + 1)));
    if (idArrayIndex >= numIds)
      {
      // We're out of selection ids, so we're done.
      break;
      }

    // Advance through and mark all cells with a label EQUAL TO the
    // current selection id, as well as their points.
    while ((labelArrayIndex < numCells) &&
           (idArray->GetValue(idArrayIndex) == labelArray->GetValue(labelArrayIndex)))
      {
      cellId = idxArray->GetValue(labelArrayIndex);
      cellInArray->SetValue(cellId, flag);
      input->GetCellPoints(cellId, idList);
      if (!invert)
        {
        for (i = 0; i < idList->GetNumberOfIds(); ++i)
          {
          pointInArray->SetValue(idList->GetId(i), flag);
          }
        }
      else
        {
        for (i = 0; i < idList->GetNumberOfIds(); ++i)
          {
          ptId = idList->GetId(i);
          ptIds->InsertUniqueId(ptId);
          cellCounter[ptId]++;
          }
        }
      ++labelArrayIndex;
      }
      

    // Advance through cell labels until we find
    // one that's NOT LESS THAN the current selection id.
    while ((labelArrayIndex < numCells) &&
           (labelArray->GetValue(labelArrayIndex) < idArray->GetValue(idArrayIndex)))
      {
      ++labelArrayIndex;
      }
    }

  if (idArray)
    {
    idArray->Delete();
    }

  if (invert)
    {
    for (i = 0; i < ptIds->GetNumberOfIds(); ++i)
      {
      ptId = ptIds->GetId(i);
      input->GetPointCells(ptId, idList);
      if (cellCounter[ptId] == idList->GetNumberOfIds())
        {
        pointInArray->SetValue(ptId, flag);
        }
      }
  
    ptIds->Delete();
    delete [] cellCounter;
    }

  idList->Delete();
  idxArray->Delete();
  labelArray->Delete();

  if (!passThrough)
    {
    vtkIdType *pointMap = new vtkIdType[numPts]; // maps old point ids into new
    vtkExtractSelectedIdsCopyPoints(input, output, pointInArray->GetPointer(0), pointMap);
    this->UpdateProgress(0.75);
    if (output->GetDataObjectType() == VTK_POLY_DATA)
      {
      vtkExtractSelectedIdsCopyCells<vtkPolyData>(input, vtkPolyData::SafeDownCast(output), cellInArray->GetPointer(0), pointMap);
      }
    else
      {
      vtkExtractSelectedIdsCopyCells<vtkUnstructuredGrid>(input, vtkUnstructuredGrid::SafeDownCast(output), cellInArray->GetPointer(0), pointMap);
      }
    delete [] pointMap;
    this->UpdateProgress(1.0);
    }

  output->Squeeze();

  return 1;
}

//----------------------------------------------------------------------------
int vtkExtractSelectedIds::ExtractPoints(
  vtkSelection *sel,  vtkDataSet *input,
  vtkDataSet *output)
{
  int passThrough = 0;
  if (sel->GetProperties()->Has(vtkSelection::PRESERVE_TOPOLOGY()))
    {
    passThrough = sel->GetProperties()->Get(vtkSelection::PRESERVE_TOPOLOGY());
    }

  int containingCells = 0;
  if (sel->GetProperties()->Has(vtkSelection::CONTAINING_CELLS()))
    {
    containingCells = sel->GetProperties()->Get(vtkSelection::CONTAINING_CELLS());
    }

  int invert = 0;
  if (sel->GetProperties()->Has(vtkSelection::INVERSE()))
    {
    invert = sel->GetProperties()->Get(vtkSelection::INVERSE());
    }

  vtkIdType i, numPts = input->GetNumberOfPoints();
  vtkSmartPointer<vtkSignedCharArray> pointInArray = vtkSmartPointer<vtkSignedCharArray>::New();
  pointInArray->SetNumberOfComponents(1);
  pointInArray->SetNumberOfTuples(numPts);
  signed char flag = invert ? 1 : -1;
  for (i=0; i < numPts; i++)
    {
    pointInArray->SetValue(i, flag);
    }

  vtkIdType numCells = input->GetNumberOfCells();
  vtkSmartPointer<vtkSignedCharArray> cellInArray;
  if (containingCells)
    {
    cellInArray = vtkSmartPointer<vtkSignedCharArray>::New();
    cellInArray->SetNumberOfComponents(1);
    cellInArray->SetNumberOfTuples(numCells);
    for (i=0; i < numCells; i++)
     {
     cellInArray->SetValue(i, flag);
     }
   }

 if (passThrough)
    {
    output->ShallowCopy(input);
    pointInArray->SetName("vtkInsidedness");
    vtkPointData *outPD = output->GetPointData();
    outPD->AddArray(pointInArray);
    outPD->SetScalars(pointInArray);
    if (containingCells)
      {
      cellInArray->SetName("vtkInsidedness");
      vtkCellData *outCD = output->GetCellData();
      outCD->AddArray(cellInArray);
      outCD->SetScalars(cellInArray);
      }
    }

  vtkIdTypeArray *idxArray = vtkIdTypeArray::New();
  idxArray->SetNumberOfComponents(1);
  idxArray->SetNumberOfTuples(numPts);
  for (i=0; i < numPts; i++)
    {
    idxArray->SetValue(i, i);
    }

  //decide what the IDS mean
  vtkIdTypeArray *labelArray = NULL;
  int selType = sel->GetProperties()->Get(vtkSelection::CONTENT_TYPE());
  if (selType == vtkSelection::GLOBALIDS)
    {
    labelArray = vtkIdTypeArray::SafeDownCast(
      input->GetPointData()->GetGlobalIds());
    }
  else if (selType == vtkSelection::VALUES &&
           sel->GetProperties()->Has(vtkSelection::ARRAY_NAME()))
    {
    //user chose a specific label array
    labelArray = vtkIdTypeArray::SafeDownCast(
      input->GetPointData()->GetArray(
        sel->GetProperties()->Get(vtkSelection::ARRAY_NAME())
        ));      
    }
  if (labelArray == NULL && selType != vtkSelection::OFFSETS)
    {
    return 1;
    }
  

  if (labelArray)
    {
    vtkIdTypeArray* sortedArray = vtkIdTypeArray::New();
    sortedArray->DeepCopy(labelArray);
    vtkSortDataArray::Sort(sortedArray, idxArray);
    labelArray = sortedArray;
    }
  else
    {
    //no global array, so just use the input cell index
    labelArray = idxArray;
    labelArray->Register(NULL);
    }

  // Reverse the "in" flag
  flag = -flag;

  vtkIdList *ptCells = vtkIdList::New();
  vtkIdList *cellPts = vtkIdList::New();
  vtkIdType j, numIds = 0, ptId, cellId, idArrayIndex = 0, labelArrayIndex = 0;
  vtkIdTypeArray* idArray = vtkIdTypeArray::SafeDownCast(sel->GetSelectionList());
  if (idArray)
    {
    numIds = idArray->GetNumberOfTuples();
    vtkIdTypeArray* sortedArray = vtkIdTypeArray::New();
    sortedArray->DeepCopy(idArray);
    vtkSortDataArray::SortArrayByComponent(sortedArray, 0);
    idArray = sortedArray;
    }

  // Check each point to see if it's selected
  while (labelArrayIndex < numPts)
    {
    // Advance through the selection ids until we find
    // one that's NOT LESS THAN the current point label.
    while ((idArrayIndex < numIds) &&
           (idArray->GetValue(idArrayIndex) < labelArray->GetValue(labelArrayIndex)))
      {
      ++idArrayIndex;
      }

    this->UpdateProgress(static_cast<double>(idArrayIndex) / (numIds * (passThrough + 1)));
    if (idArrayIndex >= numIds)
      {
      // We're out of selection ids, so we're done.
      break;
      }

    // Advance through and mark all points with a label EQUAL TO the
    // current selection id, as well as their cells.
    while ((labelArrayIndex < numPts) &&
           (idArray->GetValue(idArrayIndex) == labelArray->GetValue(labelArrayIndex)))
      {
      ptId = idxArray->GetValue(labelArrayIndex);
      pointInArray->SetValue(ptId, flag);
      if (containingCells)
      for (vtkIdType j = 0; j < input->GetNumberOfPoints(); j++)
        {
        input->GetPointCells(ptId, ptCells);
        for (i = 0; i < ptCells->GetNumberOfIds(); ++i)
          {
          cellId = ptCells->GetId(i);
          if (!passThrough && !invert && cellInArray->GetValue(cellId) != flag)
            {
            input->GetCellPoints(cellId, cellPts);
            for (j = 0; j < cellPts->GetNumberOfIds(); ++j)
              {
              pointInArray->SetValue(cellPts->GetId(j), flag);
              }
            }
          cellInArray->SetValue(cellId, flag);
          }
        }
      ++labelArrayIndex;
      }

    // Advance through point labels until we find
    // one that's NOT LESS THAN the current selection id.
    while ((labelArrayIndex < numPts) &&
           (labelArray->GetValue(labelArrayIndex) < idArray->GetValue(idArrayIndex)))
      {
      ++labelArrayIndex;
      }
    }

  if (idArray)
    {
    idArray->Delete();
    }

  ptCells->Delete();
  cellPts->Delete();
  idxArray->Delete();
  labelArray->Delete();

  if (!passThrough)
    {
    vtkIdType *pointMap = new vtkIdType[numPts]; // maps old point ids into new
    vtkExtractSelectedIdsCopyPoints(input, output, pointInArray->GetPointer(0), pointMap);
    this->UpdateProgress(0.75);
    if (containingCells)
      {
      if (output->GetDataObjectType() == VTK_POLY_DATA)
        {
        vtkExtractSelectedIdsCopyCells<vtkPolyData>(input, vtkPolyData::SafeDownCast(output), cellInArray->GetPointer(0), pointMap);
        }
      else
        {
        vtkExtractSelectedIdsCopyCells<vtkUnstructuredGrid>(input, vtkUnstructuredGrid::SafeDownCast(output), cellInArray->GetPointer(0), pointMap);
        }
      delete [] pointMap;
      }
    else
      {
      numPts = output->GetNumberOfPoints();
      vtkPolyData* outputPD = vtkPolyData::SafeDownCast(output);
      outputPD->Allocate(numPts);
      for (i = 0; i < numPts; ++i)
        {
        outputPD->InsertNextCell(VTK_VERTEX, 1, &i);
        }
      }
      this->UpdateProgress(1.0);

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
