/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMergeFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkMergeFilter.h"

#include "vtkCellData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkRectilinearGrid.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStructuredGrid.h"
#include "vtkStructuredPoints.h"
#include "vtkUnstructuredGrid.h"

vtkStandardNewMacro(vtkMergeFilter);

class vtkFieldNode
{
public:
  vtkFieldNode(const char* name, vtkDataSet* ptr = nullptr)
  {
    size_t length = strlen(name);
    if (length > 0)
    {
      this->Name = new char[length + 1];
      strcpy(this->Name, name);
    }
    else
    {
      this->Name = nullptr;
    }
    this->Ptr = ptr;
    this->Next = nullptr;
  }
  ~vtkFieldNode() { delete[] this->Name; }

  const char* GetName() { return Name; }
  vtkDataSet* Ptr;
  vtkFieldNode* Next;

private:
  vtkFieldNode(const vtkFieldNode&) = delete;
  void operator=(const vtkFieldNode&) = delete;
  char* Name;
};

class vtkFieldList
{
public:
  vtkFieldList()
  {
    this->First = nullptr;
    this->Last = nullptr;
  }
  ~vtkFieldList()
  {
    vtkFieldNode* node = this->First;
    vtkFieldNode* next;
    while (node)
    {
      next = node->Next;
      delete node;
      node = next;
    }
  }

  void Add(const char* name, vtkDataSet* ptr)
  {
    vtkFieldNode* newNode = new vtkFieldNode(name, ptr);
    if (!this->First)
    {
      this->First = newNode;
      this->Last = newNode;
    }
    else
    {
      this->Last->Next = newNode;
      this->Last = newNode;
    }
  }

  friend class vtkFieldListIterator;

private:
  vtkFieldNode* First;
  vtkFieldNode* Last;
};

class vtkFieldListIterator
{
public:
  vtkFieldListIterator(vtkFieldList* list)
  {
    this->List = list;
    this->Position = nullptr;
  }
  void Begin() { this->Position = this->List->First; }
  void Next()
  {
    if (this->Position)
    {
      this->Position = this->Position->Next;
    }
  }
  int End() { return this->Position ? 0 : 1; }
  vtkFieldNode* Get() { return this->Position; }

private:
  vtkFieldNode* Position;
  vtkFieldList* List;
};

//------------------------------------------------------------------------------

// Create object with no input or output.
vtkMergeFilter::vtkMergeFilter()
{
  this->FieldList = new vtkFieldList;
  this->SetNumberOfInputPorts(6);
}

vtkMergeFilter::~vtkMergeFilter()
{
  delete this->FieldList;
}

vtkDataSet* vtkMergeFilter::GetGeometry()
{
  if (this->GetNumberOfInputConnections(0) < 1)
  {
    return nullptr;
  }
  return vtkDataSet::SafeDownCast(this->GetExecutive()->GetInputData(0, 0));
}

void vtkMergeFilter::SetScalarsData(vtkDataSet* input)
{
  this->SetInputData(1, input);
}
vtkDataSet* vtkMergeFilter::GetScalars()
{
  if (this->GetNumberOfInputConnections(1) < 1)
  {
    return nullptr;
  }
  return vtkDataSet::SafeDownCast(this->GetExecutive()->GetInputData(1, 0));
}

void vtkMergeFilter::SetVectorsData(vtkDataSet* input)
{
  this->SetInputData(2, input);
}
vtkDataSet* vtkMergeFilter::GetVectors()
{
  if (this->GetNumberOfInputConnections(2) < 1)
  {
    return nullptr;
  }
  return vtkDataSet::SafeDownCast(this->GetExecutive()->GetInputData(2, 0));
}

void vtkMergeFilter::SetNormalsData(vtkDataSet* input)
{
  this->SetInputData(3, input);
}
vtkDataSet* vtkMergeFilter::GetNormals()
{
  if (this->GetNumberOfInputConnections(3) < 1)
  {
    return nullptr;
  }
  return vtkDataSet::SafeDownCast(this->GetExecutive()->GetInputData(3, 0));
}

void vtkMergeFilter::SetTCoordsData(vtkDataSet* input)
{
  this->SetInputData(4, input);
}
vtkDataSet* vtkMergeFilter::GetTCoords()
{
  if (this->GetNumberOfInputConnections(4) < 1)
  {
    return nullptr;
  }
  return vtkDataSet::SafeDownCast(this->GetExecutive()->GetInputData(4, 0));
}

void vtkMergeFilter::SetTensorsData(vtkDataSet* input)
{
  this->SetInputData(5, input);
}
vtkDataSet* vtkMergeFilter::GetTensors()
{
  if (this->GetNumberOfInputConnections(5) < 1)
  {
    return nullptr;
  }
  return vtkDataSet::SafeDownCast(this->GetExecutive()->GetInputData(5, 0));
}

void vtkMergeFilter::AddField(const char* name, vtkDataSet* input)
{
  this->FieldList->Add(name, input);
}

int vtkMergeFilter::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkInformation* scalarsInfo = inputVector[1]->GetInformationObject(0);
  vtkInformation* vectorsInfo = inputVector[2]->GetInformationObject(0);
  vtkInformation* normalsInfo = inputVector[3]->GetInformationObject(0);
  vtkInformation* tCoordsInfo = inputVector[4]->GetInformationObject(0);
  vtkInformation* tensorsInfo = inputVector[5]->GetInformationObject(0);

  // get the input and output
  vtkDataSet* input = vtkDataSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkDataSet* output = vtkDataSet::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkDataSet* scalarsData = nullptr;
  vtkDataSet* vectorsData = nullptr;
  vtkDataSet* normalsData = nullptr;
  vtkDataSet* tCoordsData = nullptr;
  vtkDataSet* tensorsData = nullptr;
  if (scalarsInfo)
  {
    scalarsData = vtkDataSet::SafeDownCast(scalarsInfo->Get(vtkDataObject::DATA_OBJECT()));
  }
  if (vectorsInfo)
  {
    vectorsData = vtkDataSet::SafeDownCast(vectorsInfo->Get(vtkDataObject::DATA_OBJECT()));
  }
  if (normalsInfo)
  {
    normalsData = vtkDataSet::SafeDownCast(normalsInfo->Get(vtkDataObject::DATA_OBJECT()));
  }
  if (tCoordsInfo)
  {
    tCoordsData = vtkDataSet::SafeDownCast(tCoordsInfo->Get(vtkDataObject::DATA_OBJECT()));
  }
  if (tensorsInfo)
  {
    tensorsData = vtkDataSet::SafeDownCast(tensorsInfo->Get(vtkDataObject::DATA_OBJECT()));
  }

  vtkIdType numPts, numScalars = 0, numVectors = 0, numNormals = 0, numTCoords = 0;
  vtkIdType numTensors = 0;
  vtkIdType numCells, numCellScalars = 0, numCellVectors = 0, numCellNormals = 0;
  vtkIdType numCellTCoords = 0, numCellTensors = 0;
  vtkPointData* pd;
  vtkDataArray* scalars = nullptr;
  vtkDataArray* vectors = nullptr;
  vtkDataArray* normals = nullptr;
  vtkDataArray* tcoords = nullptr;
  vtkDataArray* tensors = nullptr;
  vtkCellData* cd;
  vtkDataArray* cellScalars = nullptr;
  vtkDataArray* cellVectors = nullptr;
  vtkDataArray* cellNormals = nullptr;
  vtkDataArray* cellTCoords = nullptr;
  vtkDataArray* cellTensors = nullptr;
  vtkPointData* outputPD = output->GetPointData();
  vtkCellData* outputCD = output->GetCellData();

  vtkDebugMacro(<< "Merging data!");

  // geometry needs to be copied
  output->CopyStructure(input);
  if ((numPts = input->GetNumberOfPoints()) < 1)
  {
    vtkWarningMacro(<< "Nothing to merge!");
  }
  numCells = input->GetNumberOfCells();

  if (scalarsData)
  {
    pd = scalarsData->GetPointData();
    scalars = pd->GetScalars();
    if (scalars != nullptr)
    {
      numScalars = scalars->GetNumberOfTuples();
    }
    cd = scalarsData->GetCellData();
    cellScalars = cd->GetScalars();
    if (cellScalars != nullptr)
    {
      numCellScalars = cellScalars->GetNumberOfTuples();
    }
  }

  if (vectorsData)
  {
    pd = vectorsData->GetPointData();
    vectors = pd->GetVectors();
    if (vectors != nullptr)
    {
      numVectors = vectors->GetNumberOfTuples();
    }
    cd = vectorsData->GetCellData();
    cellVectors = cd->GetVectors();
    if (cellVectors != nullptr)
    {
      numCellVectors = cellVectors->GetNumberOfTuples();
    }
  }

  if (normalsData)
  {
    pd = normalsData->GetPointData();
    normals = pd->GetNormals();
    if (normals != nullptr)
    {
      numNormals = normals->GetNumberOfTuples();
    }
    cd = normalsData->GetCellData();
    cellNormals = cd->GetNormals();
    if (cellNormals != nullptr)
    {
      numCellNormals = cellNormals->GetNumberOfTuples();
    }
  }

  if (tCoordsData)
  {
    pd = tCoordsData->GetPointData();
    tcoords = pd->GetTCoords();
    if (tcoords != nullptr)
    {
      numTCoords = tcoords->GetNumberOfTuples();
    }
    cd = tCoordsData->GetCellData();
    cellTCoords = cd->GetTCoords();
    if (cellTCoords != nullptr)
    {
      numCellTCoords = cellTCoords->GetNumberOfTuples();
    }
  }

  if (tensorsData)
  {
    pd = tensorsData->GetPointData();
    tensors = pd->GetTensors();
    if (tensors != nullptr)
    {
      numTensors = tensors->GetNumberOfTuples();
    }
    cd = tensorsData->GetCellData();
    cellTensors = cd->GetTensors();
    if (cellTensors != nullptr)
    {
      numCellTensors = cellTensors->GetNumberOfTuples();
    }
  }

  // merge data only if it is consistent
  if (numPts == numScalars)
  {
    outputPD->SetScalars(scalars);
  }
  else
  {
    vtkWarningMacro("Scalars for point data cannot be merged because the number of points in the "
                    "input geometry do not match the number of point scalars "
      << numPts << " != " << numScalars);
  }
  if (numCells == numCellScalars)
  {
    outputCD->SetScalars(cellScalars);
  }
  else
  {
    vtkWarningMacro("Scalars for cell data cannot be merged because the number of cells in the "
                    "input geometry do not match the number of cell scalars "
      << numCells << " != " << numCellScalars);
  }

  if (numPts == numVectors)
  {
    outputPD->SetVectors(vectors);
  }
  else
  {
    vtkWarningMacro("Vectors for point data cannot be merged because the number of points in the "
                    "input geometry do not match the number of point vectors "
      << numPts << " != " << numVectors);
  }
  if (numCells == numCellVectors)
  {
    outputCD->SetVectors(cellVectors);
  }
  else
  {
    vtkWarningMacro("Vectors for cell data cannot be merged because the number of cells in the "
                    "input geometry do not match the number of cell vectors "
      << numCells << " != " << numCellVectors);
  }

  if (numPts == numNormals)
  {
    outputPD->SetNormals(normals);
  }
  else
  {
    vtkWarningMacro("Normals for point data cannot be merged because the number of points in the "
                    "input geometry do not match the number of point normals "
      << numPts << " != " << numNormals);
  }
  if (numCells == numCellNormals)
  {
    outputCD->SetNormals(cellNormals);
  }
  else
  {
    vtkWarningMacro("Normals for cell data cannot be merged because the number of cells in the "
                    "input geometry do not match the number of cell normals "
      << numCells << " != " << numCellNormals);
  }

  if (numPts == numTCoords)
  {
    outputPD->SetTCoords(tcoords);
  }
  else
  {
    vtkWarningMacro("TCoords for point data cannot be merged because the number of points in the "
                    "input geometry do not match the number of point tcoords "
      << numPts << " != " << numTCoords);
  }
  if (numCells == numCellTCoords)
  {
    outputCD->SetTCoords(cellTCoords);
  }
  else
  {
    vtkWarningMacro("TCoords for cell data cannot be merged because the number of cells in the "
                    "input geometry do not match the number of cell tcoords "
      << numCells << " != " << numCellTCoords);
  }

  if (numPts == numTensors)
  {
    outputPD->SetTensors(tensors);
  }
  else
  {
    vtkWarningMacro("Tensors for point data cannot be merged because the number of points in the "
                    "input geometry do not match the number of point tensors "
      << numPts << " != " << numTensors);
  }

  if (numCells == numCellTensors)
  {
    outputCD->SetTensors(cellTensors);
  }
  else
  {
    vtkWarningMacro("Tensors for cell data cannot be merged because the number of cells in the "
                    "input geometry do not match the number of cell tcoords "
      << numCells << " != " << numTCoords);
  }

  vtkFieldListIterator it(this->FieldList);
  vtkDataArray* da;
  const char* name;
  vtkIdType num;
  for (it.Begin(); !it.End(); it.Next())
  {
    pd = it.Get()->Ptr->GetPointData();
    cd = it.Get()->Ptr->GetCellData();
    name = it.Get()->GetName();
    if ((da = pd->GetArray(name)))
    {
      num = da->GetNumberOfTuples();
      if (num == numPts)
      {
        outputPD->AddArray(da);
      }
    }
    if ((da = cd->GetArray(name)))
    {
      num = da->GetNumberOfTuples();
      if (num == numCells)
      {
        outputCD->AddArray(da);
      }
    }
  }

  return 1;
}

//----------------------------------------------------------------------------
//  Trick:  Abstract data types that may or may not be the same type
// (structured/unstructured), but the points/cells match up.
// Output/Geometry may be structured while ScalarInput may be
// unstructured (but really have same triangulation/topology as geometry).
// Just request all the input. Always generate all of the output (todo).
int vtkMergeFilter::RequestUpdateExtent(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* vtkNotUsed(outputVector))
{
  vtkInformation* inputInfo;
  int idx;

  for (idx = 0; idx < 6; ++idx)
  {
    inputInfo = inputVector[idx]->GetInformationObject(0);
    if (inputInfo)
    {
      inputInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), 0);
      inputInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(), 1);
      inputInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(), 0);
      inputInfo->Set(vtkStreamingDemandDrivenPipeline::EXACT_EXTENT(), 1);
    }
  }
  return 1;
}

int vtkMergeFilter::FillInputPortInformation(int port, vtkInformation* info)
{
  int retval = this->Superclass::FillInputPortInformation(port, info);
  if (port > 0)
  {
    info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  }
  return retval;
}

void vtkMergeFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
