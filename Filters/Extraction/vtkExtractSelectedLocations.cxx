// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// VTK_DEPRECATED_IN_9_2_0() warnings for this class.
#define VTK_DEPRECATION_LEVEL 0

#include "vtkExtractSelectedLocations.h"

#include "vtkCell.h"
#include "vtkCellData.h"
#include "vtkCellIterator.h"
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
#include "vtkSelectionNode.h"
#include "vtkSignedCharArray.h"
#include "vtkSmartPointer.h"
#include "vtkUnstructuredGrid.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkExtractSelectedLocations);

//------------------------------------------------------------------------------
vtkExtractSelectedLocations::vtkExtractSelectedLocations()
{
  this->SetNumberOfInputPorts(2);
}

//------------------------------------------------------------------------------
vtkExtractSelectedLocations::~vtkExtractSelectedLocations() = default;

//------------------------------------------------------------------------------
int vtkExtractSelectedLocations::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* selInfo = inputVector[1]->GetInformationObject(0);
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // verify the input, selection and output
  vtkDataSet* input = vtkDataSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  if (!input)
  {
    vtkErrorMacro(<< "No input specified");
    return 0;
  }

  if (!selInfo)
  {
    // When not given a selection, quietly select nothing.
    return 1;
  }

  vtkSelection* sel = vtkSelection::SafeDownCast(selInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkSelectionNode* node = nullptr;
  if (sel->GetNumberOfNodes() == 1)
  {
    node = sel->GetNode(0);
  }
  if (!node)
  {
    vtkErrorMacro("Selection must have a single node.");
    return 0;
  }
  if (node->GetContentType() != vtkSelectionNode::LOCATIONS)
  {
    vtkErrorMacro("Incompatible CONTENT_TYPE.");
    return 0;
  }

  vtkDataSet* output = vtkDataSet::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkDebugMacro(<< "Extracting from dataset");

  int fieldType = vtkSelectionNode::CELL;
  if (node->GetProperties()->Has(vtkSelectionNode::FIELD_TYPE()))
  {
    fieldType = node->GetProperties()->Get(vtkSelectionNode::FIELD_TYPE());
  }
  switch (fieldType)
  {
    case vtkSelectionNode::CELL:
      return this->ExtractCells(node, input, output);
    case vtkSelectionNode::POINT:
      return this->ExtractPoints(node, input, output);
  }
  return 1;
}

// Copy the points marked as "in" and build a pointmap
static void vtkExtractSelectedLocationsCopyPoints(
  vtkDataSet* input, vtkDataSet* output, signed char* inArray, vtkIdType* pointMap)
{
  vtkPoints* newPts = vtkPoints::New();

  vtkIdType i, numPts = input->GetNumberOfPoints();

  vtkPointData* inPD = input->GetPointData();
  vtkPointData* outPD = output->GetPointData();
  outPD->SetCopyGlobalIds(1);
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
void vtkExtractSelectedLocationsCopyCells(
  vtkDataSet* input, T* output, signed char* inArray, vtkIdType* pointMap)
{
  vtkIdType numCells = input->GetNumberOfCells();
  output->AllocateEstimate(numCells / 4, 1);

  vtkCellData* inCD = input->GetCellData();
  vtkCellData* outCD = output->GetCellData();
  outCD->SetCopyGlobalIds(1);
  outCD->CopyAllocate(inCD);

  vtkIdTypeArray* originalIds = vtkIdTypeArray::New();
  originalIds->SetNumberOfComponents(1);
  originalIds->SetName("vtkOriginalCellIds");

  int cellType;
  vtkIdType numCellPts;
  vtkIdList* pointIdList;
  vtkIdType i, j, newId = 0;
  vtkIdList* ptIds = vtkIdList::New();
  vtkIdList* faces;

  vtkSmartPointer<vtkCellIterator> cellIter =
    vtkSmartPointer<vtkCellIterator>::Take(input->NewCellIterator());
  for (i = 0, cellIter->InitTraversal(); !cellIter->IsDoneWithTraversal();
       cellIter->GoToNextCell(), ++i)
  {
    if (inArray[i] > 0)
    {
      cellType = cellIter->GetCellType();
      numCellPts = cellIter->GetNumberOfPoints();
      pointIdList = cellIter->GetPointIds();

      ptIds->Reset();
      // special handling for polyhedron cells
      if (cellType == VTK_POLYHEDRON)
      {
        faces = cellIter->GetFaces();
        for (j = 0; j < faces->GetNumberOfIds(); ++j)
        {
          ptIds->InsertNextId(faces->GetId(j));
        }
        vtkUnstructuredGrid::ConvertFaceStreamPointIds(ptIds, pointMap);
      }
      else
      {
        for (j = 0; j < numCellPts; j++)
        {
          ptIds->InsertId(j, pointMap[pointIdList->GetId(j)]);
        }
      }
      output->InsertNextCell(cellType, ptIds);
      outCD->CopyData(inCD, i, newId++);
      originalIds->InsertNextValue(i);
    }
  }

  outCD->AddArray(originalIds);
  originalIds->Delete();
  ptIds->Delete();
}

//------------------------------------------------------------------------------
int vtkExtractSelectedLocations::ExtractCells(
  vtkSelectionNode* sel, vtkDataSet* input, vtkDataSet* output)
{
  // get a hold of input data structures and allocate output data structures
  vtkDoubleArray* locArray = vtkArrayDownCast<vtkDoubleArray>(sel->GetSelectionList());

  if (!locArray)
  {
    return 1;
  }

  int passThrough = 0;
  if (this->PreserveTopology)
  {
    passThrough = 1;
  }

  int invert = 0;
  if (sel->GetProperties()->Has(vtkSelectionNode::INVERSE()))
  {
    invert = sel->GetProperties()->Get(vtkSelectionNode::INVERSE());
  }

  vtkIdType i, numPts = input->GetNumberOfPoints();
  vtkSmartPointer<vtkSignedCharArray> pointInArray = vtkSmartPointer<vtkSignedCharArray>::New();
  pointInArray->SetNumberOfComponents(1);
  pointInArray->SetNumberOfTuples(numPts);
  signed char flag = invert ? 1 : -1;
  for (i = 0; i < numPts; i++)
  {
    pointInArray->SetValue(i, flag);
  }

  vtkIdType numCells = input->GetNumberOfCells();
  vtkSmartPointer<vtkSignedCharArray> cellInArray = vtkSmartPointer<vtkSignedCharArray>::New();
  cellInArray->SetNumberOfComponents(1);
  cellInArray->SetNumberOfTuples(numCells);
  for (i = 0; i < numCells; i++)
  {
    cellInArray->SetValue(i, flag);
  }

  if (passThrough)
  {
    output->ShallowCopy(input);
    pointInArray->SetName("vtkInsidedness");
    vtkPointData* outPD = output->GetPointData();
    outPD->AddArray(pointInArray);
    outPD->SetScalars(pointInArray);
    cellInArray->SetName("vtkInsidedness");
    vtkCellData* outCD = output->GetCellData();
    outCD->AddArray(cellInArray);
    outCD->SetScalars(cellInArray);
  }

  // Reverse the "in" flag
  flag = -flag;

  vtkIdList* ptIds = nullptr;
  char* cellCounter = nullptr;
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
  vtkIdList* idList = vtkIdList::New();
  vtkIdType numLocs = locArray->GetNumberOfTuples();

  int subId;
  double pcoords[3];
  double* weights = new double[input->GetMaxCellSize()];

  vtkIdType ptId, cellId, locArrayIndex;
  vtkIdType checkAbortInterval = std::min(numLocs / 10 + 1, (vtkIdType)1000);
  for (locArrayIndex = 0; locArrayIndex < numLocs; locArrayIndex++)
  {
    if (locArrayIndex % checkAbortInterval == 0 && this->CheckAbort())
    {
      break;
    }
    cellId = input->FindCell(
      locArray->GetTuple(locArrayIndex), nullptr, cell, 0, 0.0, subId, pcoords, weights);
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

  delete[] weights;
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
    delete[] cellCounter;
  }

  idList->Delete();

  if (!this->CheckAbort() && !passThrough)
  {
    vtkIdType* pointMap = new vtkIdType[numPts]; // maps old point ids into new
    vtkExtractSelectedLocationsCopyPoints(input, output, pointInArray->GetPointer(0), pointMap);
    this->UpdateProgress(0.75);
    if (output->GetDataObjectType() == VTK_POLY_DATA)
    {
      vtkExtractSelectedLocationsCopyCells<vtkPolyData>(
        input, vtkPolyData::SafeDownCast(output), cellInArray->GetPointer(0), pointMap);
    }
    else
    {
      vtkExtractSelectedLocationsCopyCells<vtkUnstructuredGrid>(
        input, vtkUnstructuredGrid::SafeDownCast(output), cellInArray->GetPointer(0), pointMap);
    }
    delete[] pointMap;
    this->UpdateProgress(1.0);
  }

  output->Squeeze();
  return 1;
}

//------------------------------------------------------------------------------
int vtkExtractSelectedLocations::ExtractPoints(
  vtkSelectionNode* sel, vtkDataSet* input, vtkDataSet* output)
{
  // get a hold of input data structures and allocate output data structures
  vtkDoubleArray* locArray = vtkArrayDownCast<vtkDoubleArray>(sel->GetSelectionList());
  if (!locArray)
  {
    return 1;
  }

  int passThrough = 0;
  if (this->PreserveTopology)
  {
    passThrough = 1;
  }

  int invert = 0;
  if (sel->GetProperties()->Has(vtkSelectionNode::INVERSE()))
  {
    invert = sel->GetProperties()->Get(vtkSelectionNode::INVERSE());
  }

  int containingCells = 0;
  if (sel->GetProperties()->Has(vtkSelectionNode::CONTAINING_CELLS()))
  {
    containingCells = sel->GetProperties()->Get(vtkSelectionNode::CONTAINING_CELLS());
  }

  double epsilon = 0.1;
  if (sel->GetProperties()->Has(vtkSelectionNode::EPSILON()))
  {
    epsilon = sel->GetProperties()->Get(vtkSelectionNode::EPSILON());
  }

  vtkIdType i, numPts = input->GetNumberOfPoints();
  vtkSmartPointer<vtkSignedCharArray> pointInArray = vtkSmartPointer<vtkSignedCharArray>::New();
  pointInArray->SetNumberOfComponents(1);
  pointInArray->SetNumberOfTuples(numPts);
  signed char flag = invert ? 1 : -1;
  for (i = 0; i < numPts; i++)
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
    for (i = 0; i < numCells; i++)
    {
      cellInArray->SetValue(i, flag);
    }
  }

  if (passThrough)
  {
    output->ShallowCopy(input);
    pointInArray->SetName("vtkInsidedness");
    vtkPointData* outPD = output->GetPointData();
    outPD->AddArray(pointInArray);
    outPD->SetScalars(pointInArray);
    if (containingCells)
    {
      cellInArray->SetName("vtkInsidedness");
      vtkCellData* outCD = output->GetCellData();
      outCD->AddArray(cellInArray);
      outCD->SetScalars(cellInArray);
    }
  }

  // Reverse the "in" flag
  flag = -flag;

  vtkPointLocator* locator = nullptr;

  if (input->IsA("vtkPointSet"))
  {
    locator = vtkPointLocator::New();
    locator->SetDataSet(input);
  }

  vtkIdList* ptCells = vtkIdList::New();
  vtkIdList* cellPts = vtkIdList::New();
  vtkIdType numLocs = locArray->GetNumberOfTuples();
  double dist2;
  vtkIdType j, ptId, cellId, locArrayIndex;
  double epsSquared = epsilon * epsilon;
  if (numPts > 0)
  {
    vtkIdType checkAbortInterval = std::min(numLocs / 10 + 1, (vtkIdType)1000);
    for (locArrayIndex = 0; locArrayIndex < numLocs; locArrayIndex++)
    {
      if (locArrayIndex % checkAbortInterval == 0 && this->CheckAbort())
      {
        break;
      }
      if (locator != nullptr)
      {
        ptId =
          locator->FindClosestPointWithinRadius(epsilon, locArray->GetTuple(locArrayIndex), dist2);
      }
      else
      {
        double* L = locArray->GetTuple(locArrayIndex);
        ptId = input->FindPoint(locArray->GetTuple(locArrayIndex));
        if (ptId >= 0)
        {
          double* X = input->GetPoint(ptId);
          double dx = X[0] - L[0];
          dx = dx * dx;
          double dy = X[1] - L[1];
          dy = dy * dy;
          double dz = X[2] - L[2];
          dz = dz * dz;
          if (dx + dy + dz > epsSquared)
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
  }
  else
  {
    ptId = -1;
    this->CheckAbort();
  }

  ptCells->Delete();
  cellPts->Delete();
  if (locator)
  {
    locator->SetDataSet(nullptr);
    locator->Delete();
  }

  if (!this->CheckAbort() && !passThrough)
  {
    vtkIdType* pointMap = new vtkIdType[numPts]; // maps old point ids into new
    vtkExtractSelectedLocationsCopyPoints(input, output, pointInArray->GetPointer(0), pointMap);
    this->UpdateProgress(0.75);
    if (containingCells)
    {
      if (output->GetDataObjectType() == VTK_POLY_DATA)
      {
        vtkExtractSelectedLocationsCopyCells<vtkPolyData>(
          input, vtkPolyData::SafeDownCast(output), cellInArray->GetPointer(0), pointMap);
      }
      else
      {
        vtkExtractSelectedLocationsCopyCells<vtkUnstructuredGrid>(
          input, vtkUnstructuredGrid::SafeDownCast(output), cellInArray->GetPointer(0), pointMap);
      }
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
    delete[] pointMap;
    this->UpdateProgress(1.0);
  }

  output->Squeeze();

  return 1;
}

//------------------------------------------------------------------------------
void vtkExtractSelectedLocations::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
