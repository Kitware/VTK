/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractSelectedLocations.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkExtractSelectedLocations.h"

#include "vtkCell.h"
#include "vtkCellData.h"
#include "vtkDoubleArray.h"
#include "vtkGenericCell.h"
#include "vtkIdList.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPointLocator.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkSelection.h"
#include "vtkSignedCharArray.h"
#include "vtkSmartPointer.h"
#include "vtkUnstructuredGrid.h"

vtkCxxRevisionMacro(vtkExtractSelectedLocations, "1.12");
vtkStandardNewMacro(vtkExtractSelectedLocations);

//----------------------------------------------------------------------------
vtkExtractSelectedLocations::vtkExtractSelectedLocations()
{
  this->SetNumberOfInputPorts(2);
}

//----------------------------------------------------------------------------
vtkExtractSelectedLocations::~vtkExtractSelectedLocations()
{
}

//----------------------------------------------------------------------------
//needed because parent class sets output type to input type
//and we sometimes want to change it to make a PolyData or UnstructuredGrid
//regardless of input type
int vtkExtractSelectedLocations::RequestDataObject(
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
    if (selInfo)
      {
      vtkSelection *sel = vtkSelection::SafeDownCast(
        selInfo->Get(vtkDataObject::DATA_OBJECT()));
      if (sel->GetProperties()->Has(vtkSelection::PRESERVE_TOPOLOGY()) &&
          sel->GetProperties()->Get(vtkSelection::PRESERVE_TOPOLOGY()) != 0)
        {
        passThrough = 1;
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
          (!passThrough && !output->IsA("vtkUnstructuredGrid"))
        )
        {
        vtkDataSet* newOutput = NULL;
        if (!passThrough)
          {
          // The mesh will be modified.
          newOutput = vtkUnstructuredGrid::New();
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
int vtkExtractSelectedLocations::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *selInfo = inputVector[1]->GetInformationObject(0);
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  
  // verify the input, selection and ouptut
  vtkDataSet *input = vtkDataSet::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  if ( ! input )
    {
    vtkErrorMacro(<<"No input specified");
    return 0;
    }

  if ( ! selInfo )
    {
    //When not given a selection, quietly select nothing.
    return 1;
    }  

  vtkSelection *sel = vtkSelection::SafeDownCast(
    selInfo->Get(vtkDataObject::DATA_OBJECT()));
  if (!sel->GetProperties()->Has(vtkSelection::CONTENT_TYPE()) ||
      sel->GetProperties()->Get(vtkSelection::CONTENT_TYPE()) != vtkSelection::LOCATIONS)
    {
    vtkErrorMacro("Missing or incompatible CONTENT_TYPE.");
    return 0;
    }

  vtkDataSet *output = vtkDataSet::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));
  
  vtkDebugMacro(<< "Extracting from dataset");
  
  int fieldType = vtkSelection::CELL;
  if (sel->GetProperties()->Has(vtkSelection::FIELD_TYPE()))
    {
    fieldType = sel->GetProperties()->Get(vtkSelection::FIELD_TYPE());
    }
  switch (fieldType)
    {
    case vtkSelection::CELL:
      return this->ExtractCells(sel, input, output);
    case vtkSelection::POINT:
      return this->ExtractPoints(sel, input, output);
    }
  return 1;
}

// Copy the points marked as "in" and build a pointmap
void vtkExtractSelectedLocationsCopyPoints(vtkDataSet* input, vtkDataSet* output, signed char* inArray, vtkIdType* pointMap)
{
  vtkPoints* newPts = vtkPoints::New();

  vtkIdType i, numPts = input->GetNumberOfPoints();
  
  vtkPointData* inPD = input->GetPointData();
  vtkPointData* outPD = output->GetPointData();
  outPD->CopyAllocate(inPD);

  vtkIdTypeArray* originalPtIds = vtkIdTypeArray::New();
  originalPtIds->SetName("vtkOriginalPointIds");
  originalPtIds->SetNumberOfComponents(1);
  
  for (i = 0; i < numPts; i++)
    {
    if (inArray[i] > 0)
      {
      pointMap[i] = newPts->InsertNextPoint(input->GetPoint(i));
      outPD->CopyData(inPD, i, pointMap[i]);
      originalPtIds->InsertNextValue(i);
      }
    else
      {
      pointMap[i] = -1;
      }
    }
  
  // outputDS must be either vtkPolyData or vtkUnstructuredGrid
  vtkPointSet::SafeDownCast(output)->SetPoints(newPts);
  newPts->Delete();

  outPD->AddArray(originalPtIds);
  originalPtIds->Delete();
}

// Copy the cells marked as "in" using the given pointmap
template <class T>
void vtkExtractSelectedLocationsCopyCells(vtkDataSet* input, T* output, signed char* inArray, vtkIdType* pointMap)
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
int vtkExtractSelectedLocations::ExtractCells(
  vtkSelection *sel,
  vtkDataSet* input,
  vtkDataSet *output)
{
  //get a hold of input data structures and allocate output data structures
  vtkDoubleArray *locArray = 
    vtkDoubleArray::SafeDownCast(sel->GetSelectionList());

  if (!locArray)
    {
    return 1;
    }

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

  vtkGenericCell* cell = vtkGenericCell::New();
  vtkIdList *idList = vtkIdList::New();
  vtkIdType numLocs = locArray->GetNumberOfTuples();

  int subId;
  double pcoords[3];
  double* weights = new double[input->GetMaxCellSize()];
 
  vtkIdType ptId, cellId, locArrayIndex;
  for (locArrayIndex = 0; locArrayIndex < numLocs; locArrayIndex++)
    {
    cellId = input->FindCell(locArray->GetTuple(locArrayIndex), NULL, cell,
                             0, 0.0, subId, pcoords, weights);
    if ((cellId >= 0) && (cellInArray->GetValue(cellId) != flag))
      {
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
      }
    }
  
  delete [] weights;
  cell->Delete();

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


  if (!passThrough)
    {
    vtkIdType *pointMap = new vtkIdType[numPts]; // maps old point ids into new
    vtkExtractSelectedLocationsCopyPoints(input, output, pointInArray->GetPointer(0), pointMap);
    this->UpdateProgress(0.75);
    if (output->GetDataObjectType() == VTK_POLY_DATA)
      {
      vtkExtractSelectedLocationsCopyCells<vtkPolyData>(input, vtkPolyData::SafeDownCast(output), cellInArray->GetPointer(0), pointMap);
      }
    else
      {
      vtkExtractSelectedLocationsCopyCells<vtkUnstructuredGrid>(input, vtkUnstructuredGrid::SafeDownCast(output), cellInArray->GetPointer(0), pointMap);
      }
    delete [] pointMap;
    this->UpdateProgress(1.0);
    }

  output->Squeeze();
  return 1;
}

//----------------------------------------------------------------------------
int vtkExtractSelectedLocations::ExtractPoints(
  vtkSelection *sel,
  vtkDataSet* input,
  vtkDataSet *output)
{
  //get a hold of input data structures and allocate output data structures
  vtkDoubleArray *locArray =
    vtkDoubleArray::SafeDownCast(sel->GetSelectionList());
  if (!locArray)
    {
    return 1;
    }

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

  int containingCells = 0;
  if (sel->GetProperties()->Has(vtkSelection::CONTAINING_CELLS()))
    {
    containingCells = sel->GetProperties()->Get(vtkSelection::CONTAINING_CELLS());
    }

  double epsilon = 0.1;
  if (sel->GetProperties()->Has(vtkSelection::EPSILON()))
    {
    epsilon = sel->GetProperties()->Get(vtkSelection::EPSILON());
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

  // Reverse the "in" flag
  flag = -flag;

  vtkPointLocator* locator = NULL;

  if (input->IsA("vtkPointSet"))
    {
    locator = vtkPointLocator::New();
    locator->SetDataSet(input);
    }

  vtkIdList *ptCells = vtkIdList::New();
  vtkIdList *cellPts = vtkIdList::New();
  vtkIdType numLocs = locArray->GetNumberOfTuples();
  double dist2;
  vtkIdType j, ptId, cellId, locArrayIndex;
  double epsSquared = epsilon*epsilon;
  for (locArrayIndex = 0; locArrayIndex < numLocs; locArrayIndex++)
    {
    if (locator != NULL)
      {
      ptId = locator->FindClosestPointWithinRadius(epsilon, locArray->GetTuple(locArrayIndex), dist2);
      }
    else
      {
      double *L = locArray->GetTuple(locArrayIndex);
      ptId = input->FindPoint(locArray->GetTuple(locArrayIndex));
      if (ptId >=0)
        {
        double *X = input->GetPoint(ptId);
        double dx = X[0]-L[0];
        dx = dx * dx;
        double dy = X[1]-L[1];
        dy = dy * dy;
        double dz = X[2]-L[2];
        dz = dz * dz;
        if (dx+dy+dz > epsSquared)
          {
          ptId = -1;
          }          
        }
      }

    if ((ptId >= 0) && (pointInArray->GetValue(ptId) != flag))
     {
      pointInArray->SetValue(ptId, flag);
      if (containingCells)
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
      }
    }

  ptCells->Delete();
  cellPts->Delete();
  if (locator)
    {
    locator->SetDataSet(NULL);
    locator->Delete();
    }

  if (!passThrough)
    {
    vtkIdType *pointMap = new vtkIdType[numPts]; // maps old point ids into new
    vtkExtractSelectedLocationsCopyPoints(input, output, pointInArray->GetPointer(0), pointMap);
    this->UpdateProgress(0.75);
    if (containingCells)
      {
      if (output->GetDataObjectType() == VTK_POLY_DATA)
        {
        vtkExtractSelectedLocationsCopyCells<vtkPolyData>(input, vtkPolyData::SafeDownCast(output), cellInArray->GetPointer(0), pointMap);
        }
      else
        {
        vtkExtractSelectedLocationsCopyCells<vtkUnstructuredGrid>(input, vtkUnstructuredGrid::SafeDownCast(output), cellInArray->GetPointer(0), pointMap);
        }
      delete [] pointMap;
      }
    else
      {
      numPts = output->GetNumberOfPoints();
      vtkUnstructuredGrid* outputUG = vtkUnstructuredGrid::SafeDownCast(output);
      outputUG->Allocate(numPts);
      for (i = 0; i < numPts; ++i)
        {
        outputUG->InsertNextCell(VTK_VERTEX, 1, &i);
        }
      }
    this->UpdateProgress(1.0);      
    }

  output->Squeeze();

  return 1;
}

//----------------------------------------------------------------------------
void vtkExtractSelectedLocations::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
int vtkExtractSelectedLocations::FillInputPortInformation(
  int port, vtkInformation* info)
{
  if (port==0)
    {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");    
    }
  else
    {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkSelection");
    info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
    }
  return 1;
}
