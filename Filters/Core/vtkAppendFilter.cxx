/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAppendFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkAppendFilter.h"

#include "vtkBoundingBox.h"
#include "vtkCell.h"
#include "vtkCellData.h"
#include "vtkDataSetCollection.h"
#include "vtkExecutive.h"
#include "vtkIncrementalOctreePointLocator.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkUnstructuredGrid.h"

#include <string>

vtkStandardNewMacro(vtkAppendFilter);

//----------------------------------------------------------------------------
vtkAppendFilter::vtkAppendFilter()
{
  this->InputList = nullptr;
  this->MergePoints = 0;
  this->OutputPointsPrecision = DEFAULT_PRECISION;
  this->Tolerance = 0.0;
  this->ToleranceIsAbsolute = true;
}

//----------------------------------------------------------------------------
vtkAppendFilter::~vtkAppendFilter()
{
  if (this->InputList != nullptr)
  {
    this->InputList->Delete();
    this->InputList = nullptr;
  }
}

//----------------------------------------------------------------------------
vtkDataSet* vtkAppendFilter::GetInput(int idx)
{
  if (idx >= this->GetNumberOfInputConnections(0) || idx < 0)
  {
    return nullptr;
  }

  return vtkDataSet::SafeDownCast(this->GetExecutive()->GetInputData(0, idx));
}

//----------------------------------------------------------------------------
// Remove a dataset from the list of data to append.
void vtkAppendFilter::RemoveInputData(vtkDataSet* ds)
{
  if (!ds)
  {
    return;
  }
  int numCons = this->GetNumberOfInputConnections(0);
  for (int i = 0; i < numCons; i++)
  {
    if (this->GetInput(i) == ds)
    {
      this->RemoveInputConnection(0, this->GetInputConnection(0, i));
    }
  }
}

//----------------------------------------------------------------------------
vtkDataSetCollection* vtkAppendFilter::GetInputList()
{
  if (this->InputList)
  {
    this->InputList->Delete();
  }
  this->InputList = vtkDataSetCollection::New();

  for (int idx = 0; idx < this->GetNumberOfInputConnections(0); ++idx)
  {
    if (this->GetInput(idx))
    {
      this->InputList->AddItem(this->GetInput(idx));
    }
  }

  return this->InputList;
}

//----------------------------------------------------------------------------
// Append data sets into single unstructured grid
int vtkAppendFilter::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  bool reallyMergePoints = false;
  if (this->MergePoints == 1 && inputVector[0]->GetNumberOfInformationObjects() > 0)
  {
    reallyMergePoints = true;

    // ensure that none of the inputs has ghost-cells.
    // (originally the code was checking for ghost cells only on 1st input,
    // that's not sufficient).
    for (int cc = 0; cc < inputVector[0]->GetNumberOfInformationObjects(); cc++)
    {
      vtkDataSet* tempData = vtkDataSet::GetData(inputVector[0], cc);
      if (tempData->HasAnyGhostCells())
      {
        vtkDebugMacro(<< "Ghost cells present, so points will not be merged");
        reallyMergePoints = false;
        break;
      }
    }
  }

  // get the output info object
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the output
  vtkUnstructuredGrid* output =
    vtkUnstructuredGrid::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkDebugMacro(<< "Appending data together");

  // Loop over all data sets, checking to see what data is common to
  // all inputs. Note that data is common if 1) it is the same attribute
  // type (scalar, vector, etc.), 2) it is the same native type (int,
  // float, etc.), and 3) if a data array in a field, if it has the same name.
  vtkIdType totalNumPts = 0;
  vtkIdType totalNumCells = 0;
  // If we only have a single dataset and it's an unstructured grid
  // we can just shallow copy that and exit quickly.
  int numDataSets = 0;
  vtkUnstructuredGrid* inputUG = nullptr;

  vtkSmartPointer<vtkDataSetCollection> inputs;
  inputs.TakeReference(this->GetNonEmptyInputs(inputVector));

  vtkCollectionSimpleIterator iter;
  inputs->InitTraversal(iter);
  vtkDataSet* dataSet = nullptr;
  while ((dataSet = inputs->GetNextDataSet(iter)))
  {
    totalNumPts += dataSet->GetNumberOfPoints();
    totalNumCells += dataSet->GetNumberOfCells();
    numDataSets++;
    inputUG = vtkUnstructuredGrid::SafeDownCast(dataSet);
  }

  if (totalNumPts < 1)
  {
    vtkDebugMacro(<< "No data to append!");
    return 1;
  }

  if (numDataSets == 1 && inputUG != nullptr)
  {
    vtkDebugMacro(
      << "Only a single unstructured grid in the composite dataset and we can shallow copy.");
    output->ShallowCopy(inputUG);
    return 1;
  }

  // Now we can allocate memory
  output->Allocate(totalNumCells);

  vtkSmartPointer<vtkPoints> newPts = vtkSmartPointer<vtkPoints>::New();

  // set precision for the points in the output
  if (this->OutputPointsPrecision == vtkAlgorithm::DEFAULT_PRECISION)
  {
    // take the precision of the first pointset
    int datatype = VTK_FLOAT;
    const int numInputs = inputVector[0]->GetNumberOfInformationObjects();
    for (int inputIndex = 0; inputIndex < numInputs; ++inputIndex)
    {
      vtkInformation* inInfo = inputVector[0]->GetInformationObject(inputIndex);
      vtkPointSet* ps = nullptr;
      if (inInfo)
      {
        ps = vtkPointSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
      }
      if (ps != nullptr && ps->GetNumberOfPoints() > 0)
      {
        datatype = ps->GetPoints()->GetDataType();
        break;
      }
    }
    newPts->SetDataType(datatype);
  }
  else if (this->OutputPointsPrecision == vtkAlgorithm::SINGLE_PRECISION)
  {
    newPts->SetDataType(VTK_FLOAT);
  }
  else if (this->OutputPointsPrecision == vtkAlgorithm::DOUBLE_PRECISION)
  {
    newPts->SetDataType(VTK_DOUBLE);
  }

  // If we aren't merging points, we need to allocate the points here.
  if (!reallyMergePoints)
  {
    newPts->SetNumberOfPoints(totalNumPts);
  }

  vtkSmartPointer<vtkIdList> ptIds = vtkSmartPointer<vtkIdList>::New();
  ptIds->Allocate(VTK_CELL_SIZE);
  vtkSmartPointer<vtkIdList> newPtIds = vtkSmartPointer<vtkIdList>::New();
  newPtIds->Allocate(VTK_CELL_SIZE);

  vtkIdType twentieth = (totalNumPts + totalNumCells) / 20 + 1;

  // For optionally merging duplicate points
  vtkIdType* globalIndices = new vtkIdType[totalNumPts];
  vtkSmartPointer<vtkIncrementalOctreePointLocator> ptInserter;
  if (reallyMergePoints)
  {
    vtkBoundingBox outputBB;

    inputs->InitTraversal(iter);
    while ((dataSet = inputs->GetNextDataSet(iter)))
    {

      // Union of bounding boxes
      double localBox[6];
      dataSet->GetBounds(localBox);
      outputBB.AddBounds(localBox);
    }

    double outputBounds[6];
    outputBB.GetBounds(outputBounds);

    ptInserter = vtkSmartPointer<vtkIncrementalOctreePointLocator>::New();
    if (this->ToleranceIsAbsolute)
    {
      ptInserter->SetTolerance(this->Tolerance);
    }
    else
    {
      ptInserter->SetTolerance(this->Tolerance * outputBB.GetDiagonalLength());
    }

    ptInserter->InitPointInsertion(newPts, outputBounds);
  }

  // append the blocks / pieces in terms of the geometry and topology
  vtkIdType count = 0;
  vtkIdType ptOffset = 0;
  float decimal = 0.0;
  inputs->InitTraversal(iter);
  int abort = 0;
  while (!abort && (dataSet = inputs->GetNextDataSet(iter)))
  {
    vtkIdType dataSetNumPts = dataSet->GetNumberOfPoints();
    vtkIdType dataSetNumCells = dataSet->GetNumberOfCells();

    // copy points
    for (vtkIdType ptId = 0; ptId < dataSetNumPts && !abort; ++ptId)
    {
      if (reallyMergePoints)
      {
        vtkIdType globalPtId = 0;
        ptInserter->InsertUniquePoint(dataSet->GetPoint(ptId), globalPtId);
        globalIndices[ptId + ptOffset] = globalPtId;
        // The point inserter puts the point into newPts, so we don't have to do that here.
      }
      else
      {
        globalIndices[ptId + ptOffset] = ptId + ptOffset;
        newPts->SetPoint(ptId + ptOffset, dataSet->GetPoint(ptId));
      }

      // Update progress
      count++;
      if (!(count % twentieth))
      {
        decimal += 0.05;
        this->UpdateProgress(decimal);
        abort = this->GetAbortExecute();
      }
    }

    // copy cell
    vtkUnstructuredGrid* ug = vtkUnstructuredGrid::SafeDownCast(dataSet);
    for (vtkIdType cellId = 0; cellId < dataSetNumCells && !abort; ++cellId)
    {
      newPtIds->Reset();
      if (ug && dataSet->GetCellType(cellId) == VTK_POLYHEDRON)
      {
        vtkIdType nfaces;
        const vtkIdType* facePtIds;
        ug->GetFaceStream(cellId, nfaces, facePtIds);
        for (vtkIdType id = 0; id < nfaces; ++id)
        {
          vtkIdType nPoints = facePtIds[0];
          newPtIds->InsertNextId(nPoints);
          for (vtkIdType j = 1; j <= nPoints; ++j)
          {
            newPtIds->InsertNextId(globalIndices[facePtIds[j] + ptOffset]);
          }
          facePtIds += nPoints + 1;
        }
        output->InsertNextCell(VTK_POLYHEDRON, nfaces, newPtIds->GetPointer(0));
      }
      else
      {
        dataSet->GetCellPoints(cellId, ptIds);
        for (vtkIdType id = 0; id < ptIds->GetNumberOfIds(); ++id)
        {
          newPtIds->InsertId(id, globalIndices[ptIds->GetId(id) + ptOffset]);
        }
        output->InsertNextCell(dataSet->GetCellType(cellId), newPtIds);
      }

      // Update progress
      count++;
      if (!(count % twentieth))
      {
        decimal += 0.05;
        this->UpdateProgress(decimal);
        abort = this->GetAbortExecute();
      }
    }
    ptOffset += dataSetNumPts;
  }

  // this filter can copy global ids except for global point ids when merging
  // points (see paraview/paraview#18666).
  // Note, not copying global ids is the default behavior.
  if (reallyMergePoints == false)
  {
    output->GetPointData()->CopyAllOn(vtkDataSetAttributes::COPYTUPLE);
  }
  output->GetCellData()->CopyAllOn(vtkDataSetAttributes::COPYTUPLE);

  // Now copy the array data
  this->AppendArrays(
    vtkDataObject::POINT, inputVector, globalIndices, output, newPts->GetNumberOfPoints());
  this->UpdateProgress(0.75);
  this->AppendArrays(vtkDataObject::CELL, inputVector, nullptr, output, output->GetNumberOfCells());
  this->UpdateProgress(1.0);

  // Update ourselves and release memory
  output->SetPoints(newPts);
  output->Squeeze();

  delete[] globalIndices;

  return 1;
}

//----------------------------------------------------------------------------
vtkDataSetCollection* vtkAppendFilter::GetNonEmptyInputs(vtkInformationVector** inputVector)
{
  vtkDataSetCollection* collection = vtkDataSetCollection::New();
  int numInputs = inputVector[0]->GetNumberOfInformationObjects();
  for (int inputIndex = 0; inputIndex < numInputs; ++inputIndex)
  {
    vtkInformation* inInfo = inputVector[0]->GetInformationObject(inputIndex);
    vtkDataSet* dataSet = nullptr;
    if (inInfo)
    {
      dataSet = vtkDataSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
    }
    if (dataSet != nullptr)
    {
      if (dataSet->GetNumberOfPoints() <= 0 && dataSet->GetNumberOfCells() <= 0)
      {
        continue; // no input, just skip
      }
      collection->AddItem(dataSet);
    }
  }

  return collection;
}

//----------------------------------------------------------------------------
void vtkAppendFilter::AppendArrays(int attributesType, vtkInformationVector** inputVector,
  vtkIdType* globalIds, vtkUnstructuredGrid* output, vtkIdType totalNumberOfElements)
{
  // Check if attributesType is supported
  if (attributesType != vtkDataObject::POINT && attributesType != vtkDataObject::CELL)
  {
    vtkErrorMacro(<< "Unhandled attributes type " << attributesType << ", must be either "
                  << "vtkDataObject::POINT or vtkDataObject::CELL");
    return;
  }

  vtkDataSetAttributes::FieldList fieldList;
  auto inputs = vtkSmartPointer<vtkDataSetCollection>::Take(this->GetNonEmptyInputs(inputVector));
  vtkCollectionSimpleIterator iter;
  vtkDataSet* dataSet = nullptr;
  for (dataSet = nullptr, inputs->InitTraversal(iter); (dataSet = inputs->GetNextDataSet(iter));)
  {
    if (auto inputData = dataSet->GetAttributes(attributesType))
    {
      fieldList.IntersectFieldList(inputData);
    }
  }

  vtkDataSetAttributes* outputData = output->GetAttributes(attributesType);
  outputData->CopyAllocate(fieldList, totalNumberOfElements);

  // copy arrays.
  int inputIndex;
  vtkIdType offset = 0;
  for (inputIndex = 0, dataSet = nullptr, inputs->InitTraversal(iter);
       (dataSet = inputs->GetNextDataSet(iter));)
  {
    if (auto inputData = dataSet->GetAttributes(attributesType))
    {
      const auto numberOfInputTuples = inputData->GetNumberOfTuples();
      if (globalIds != nullptr)
      {
        for (vtkIdType id = 0; id < numberOfInputTuples; ++id)
        {
          fieldList.CopyData(inputIndex, inputData, id, outputData, globalIds[offset + id]);
        }
      }
      else
      {
        fieldList.CopyData(inputIndex, inputData, 0, numberOfInputTuples, outputData, offset);
      }
      offset += numberOfInputTuples;
      ++inputIndex;
    }
  }
}

//----------------------------------------------------------------------------
int vtkAppendFilter::RequestUpdateExtent(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* vtkNotUsed(outputVector))
{
  int numInputConnections = this->GetNumberOfInputConnections(0);

  // Let downstream request a subset of connection 0, for connections >= 1
  // send their WHOLE_EXTENT as UPDATE_EXTENT.
  for (int idx = 1; idx < numInputConnections; ++idx)
  {
    vtkInformation* inputInfo = inputVector[0]->GetInformationObject(idx);
    if (inputInfo->Has(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()))
    {
      int ext[6];
      inputInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), ext);
      inputInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), ext, 6);
    }
  }

  return 1;
}

//----------------------------------------------------------------------------
int vtkAppendFilter::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  info->Set(vtkAlgorithm::INPUT_IS_REPEATABLE(), 1);
  return 1;
}

//----------------------------------------------------------------------------
void vtkAppendFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "MergePoints:" << (this->MergePoints ? "On" : "Off") << "\n";
  os << indent << "OutputPointsPrecision: " << this->OutputPointsPrecision << "\n";
  os << indent << "Tolerance: " << this->Tolerance << "\n";
}
