// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkAppendFilter.h"

#include "vtkCellData.h"
#include "vtkDataSetCollection.h"
#include "vtkIdTypeArray.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkRectilinearGrid.h"
#include "vtkSMPThreadLocalObject.h"
#include "vtkSMPTools.h"
#include "vtkSmartPointer.h"
#include "vtkStaticCleanUnstructuredGrid.h"
#include "vtkStaticPointLocator.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStructuredCellArray.h"
#include "vtkStructuredGrid.h"
#include "vtkUnstructuredGrid.h"

#include <numeric>
#include <string>
#include <unordered_map>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkAppendFilter);

//------------------------------------------------------------------------------
vtkAppendFilter::vtkAppendFilter()
{
  this->InputList = nullptr;
  this->MergePoints = 0;
  this->OutputPointsPrecision = DEFAULT_PRECISION;
  this->Tolerance = 0.0;
  this->ToleranceIsAbsolute = true;
}

//------------------------------------------------------------------------------
vtkAppendFilter::~vtkAppendFilter()
{
  if (this->InputList != nullptr)
  {
    this->InputList->Delete();
    this->InputList = nullptr;
  }
}

//------------------------------------------------------------------------------
vtkDataSet* vtkAppendFilter::GetInput(int idx)
{
  if (idx >= this->GetNumberOfInputConnections(0) || idx < 0)
  {
    return nullptr;
  }

  return vtkDataSet::SafeDownCast(this->GetExecutive()->GetInputData(0, idx));
}

//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
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

namespace
{
struct AppendCellArray
{
  template <typename CellStateT>
  void operator()(CellStateT& state, vtkIdTypeArray* outputOffsets,
    vtkIdTypeArray* outputConnectivity, vtkIdType cellOffset, vtkIdType cellConnectivityOffset,
    std::vector<vtkIdType>& globalIndices, vtkIdType pointOffset)
  {
    using ValueType = typename CellStateT::ValueType;
    auto inputOffsets = state.GetOffsets();
    auto inputConnectivity = state.GetConnectivity();
    auto numberOfCells = inputOffsets->GetNumberOfValues() - 1;
    auto numberOfConnectivityIds = inputConnectivity->GetNumberOfValues();

    // Copy the offsets and transform them using the cellConnectivityOffset
    std::transform(inputOffsets->GetPointer(0), inputOffsets->GetPointer(numberOfCells),
      outputOffsets->GetPointer(cellOffset),
      [&](ValueType offset) { return static_cast<ValueType>(offset + cellConnectivityOffset); });
    if (!globalIndices.empty())
    {
      // Copy the connectivity and transform them using the pointOffset and globalIndices
      std::transform(inputConnectivity->GetPointer(0),
        inputConnectivity->GetPointer(numberOfConnectivityIds),
        outputConnectivity->GetPointer(cellConnectivityOffset),
        [&](ValueType ptId) { return static_cast<vtkIdType>(globalIndices[ptId + pointOffset]); });
    }
    else
    {
      // Copy the connectivity and transform them using the pointOffset
      std::transform(inputConnectivity->GetPointer(0),
        inputConnectivity->GetPointer(numberOfConnectivityIds),
        outputConnectivity->GetPointer(cellConnectivityOffset),
        [&](ValueType ptId) { return static_cast<vtkIdType>(ptId + pointOffset); });
    }
  }
};
}

//------------------------------------------------------------------------------
// Append data sets into single unstructured grid
int vtkAppendFilter::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the output info object
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the output
  auto output = vtkUnstructuredGrid::GetData(outInfo);

  vtkDebugMacro(<< "Appending data together");

  auto datasets = this->GetNonEmptyInputs(inputVector);

  // Loop over all data sets, checking to see what data is common to
  // all inputs. Note that data is common if 1) it is the same attribute
  // type (scalar, vector, etc.), 2) it is the same native type (int,
  // float, etc.), and 3) if a data array in a field, if it has the same name.
  vtkIdType totalNumberOfPoints = 0;
  vtkIdType totalNumberOfCells = 0;
  vtkIdType totalNumberOfCellConnectivity = 0;
  vtkIdType totalNumberOfFaces = 0;
  vtkIdType totalNumberOfFaceConnectivity = 0;

  std::vector<vtkIdType> pointOffsets(datasets.size());
  std::vector<vtkIdType> cellOffsets(datasets.size());
  std::vector<vtkIdType> cellConnectivityOffsets(datasets.size());
  std::vector<vtkIdType> faceOffsets(datasets.size());
  std::vector<vtkIdType> faceConnectivityOffsets(datasets.size());

  // If we only have a single dataset and it's an unstructured grid
  // we can just shallow copy that and exit quickly.
  vtkUnstructuredGrid* inputUG = nullptr;
  for (size_t idx = 0; idx < datasets.size(); ++idx)
  {
    const auto& dataset = datasets[idx];

    pointOffsets[idx] = totalNumberOfPoints;
    cellOffsets[idx] = totalNumberOfCells;
    cellConnectivityOffsets[idx] = totalNumberOfCellConnectivity;
    faceOffsets[idx] = totalNumberOfFaces;
    faceConnectivityOffsets[idx] = totalNumberOfFaceConnectivity;

    totalNumberOfPoints += dataset->GetNumberOfPoints();
    totalNumberOfCells += dataset->GetNumberOfCells();

    inputUG = vtkUnstructuredGrid::SafeDownCast(dataset);
    if (inputUG)
    {
      totalNumberOfCellConnectivity += inputUG->GetCells()->GetNumberOfConnectivityIds();
      if (inputUG->GetPolyhedronFaces() && inputUG->GetPolyhedronFaceLocations())
      {
        totalNumberOfFaces += inputUG->GetPolyhedronFaces()->GetNumberOfCells();
        totalNumberOfFaceConnectivity +=
          inputUG->GetPolyhedronFaces()->GetNumberOfConnectivityIds();
      }
    }
    else if (auto polyData = vtkPolyData::SafeDownCast(dataset))
    {
      totalNumberOfCellConnectivity += polyData->GetVerts()->GetNumberOfConnectivityIds();
      totalNumberOfCellConnectivity += polyData->GetLines()->GetNumberOfConnectivityIds();
      totalNumberOfCellConnectivity += polyData->GetPolys()->GetNumberOfConnectivityIds();
      totalNumberOfCellConnectivity += polyData->GetStrips()->GetNumberOfConnectivityIds();
    }
    else if (auto grid = vtkStructuredGrid::SafeDownCast(dataset))
    {
      totalNumberOfCellConnectivity += grid->GetCells()->GetNumberOfConnectivityIds();
    }
    else if (auto rectGrid = vtkRectilinearGrid::SafeDownCast(dataset))
    {
      totalNumberOfCellConnectivity += rectGrid->GetCells()->GetNumberOfConnectivityIds();
    }
    else if (auto imageData = vtkImageData::SafeDownCast(dataset))
    {
      totalNumberOfCellConnectivity += imageData->GetCells()->GetNumberOfConnectivityIds();
    }
    else
    {
      for (vtkIdType i = 0; i < dataset->GetNumberOfCells(); ++i)
      {
        totalNumberOfCellConnectivity += dataset->GetCellSize(i);
      }
    }
  }

  if (totalNumberOfPoints < 1)
  {
    vtkDebugMacro(<< "No data to append!");
    return 1;
  }

  if (datasets.size() == 1 && inputUG != nullptr)
  {
    vtkDebugMacro(
      << "Only a single unstructured grid in the composite dataset and we can shallow copy.");
    output->ShallowCopy(inputUG);
    return 1;
  }

  vtkNew<vtkPoints> newPoints;
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
    newPoints->SetDataType(datatype);
  }
  else if (this->OutputPointsPrecision == vtkAlgorithm::SINGLE_PRECISION)
  {
    newPoints->SetDataType(VTK_FLOAT);
  }
  else if (this->OutputPointsPrecision == vtkAlgorithm::DOUBLE_PRECISION)
  {
    newPoints->SetDataType(VTK_DOUBLE);
  }

  // We look if we really can merge points.
  // Additionally to having this->MergePoints set to true,
  // points can be merge if there are not input cells cells OR if global point ids are
  // available in the inputs.
  const bool haveGlobalIdsArray =
    vtkIdTypeArray::SafeDownCast(datasets.front()->GetPointData()->GetGlobalIds());

  bool reallyMergePoints = false;
  if (this->MergePoints == 1 && inputVector[0]->GetNumberOfInformationObjects() > 0)
  {
    reallyMergePoints = true;

    // If global point ids are present, we merge points sharing same global id
    if (!haveGlobalIdsArray)
    {
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
  }

  // append points
  newPoints->SetNumberOfPoints(totalNumberOfPoints);
  vtkSMPTools::For(0, static_cast<vtkIdType>(datasets.size()),
    [&](vtkIdType begin, vtkIdType end)
    {
      for (vtkIdType idx = begin; idx < end; ++idx)
      {
        const auto& dataset = datasets[idx];
        // copy points
        newPoints->GetData()->InsertTuples(
          pointOffsets[idx], dataset->GetNumberOfPoints(), 0, dataset->GetPoints()->GetData());
      }
    });
  this->UpdateProgress(0.15);
  if (this->CheckAbort())
  {
    return 1;
  }

  // this filter can copy global ids except for global point ids when merging
  // points (see paraview/paraview#18666).
  // Note, not copying global ids is the default behavior.
  // Since paraview/paraview#19961, global point ids can be used for the merging
  // decision. In this case, they can be merged.
  auto outputPD = output->GetPointData();
  if (!reallyMergePoints || haveGlobalIdsArray)
  {
    outputPD->CopyAllOn(vtkDataSetAttributes::COPYTUPLE);
  }

  // append point data.
  vtkAppendFilter::AppendArrays(vtkDataObject::POINT, inputVector, output, totalNumberOfPoints);
  this->UpdateProgress(0.30);
  if (this->CheckAbort())
  {
    return 1;
  }
  std::vector<vtkIdType> globalIndices;
  if (reallyMergePoints) // merge points
  {
    globalIndices.resize(static_cast<size_t>(totalNumberOfPoints));
    std::vector<vtkIdType> mergeMap(static_cast<size_t>(totalNumberOfPoints), -1);
    vtkIdType totalMergedPoints = 0;
    if (haveGlobalIdsArray) // merge points with global ids
    {
      std::unordered_map<vtkIdType, vtkIdType> addedPointsMap;
      for (size_t idx = 0; idx < datasets.size(); ++idx)
      {
        auto& dataset = datasets[idx];
        const auto& pointOffset = pointOffsets[idx];
        auto globalIdsArray = vtkIdTypeArray::SafeDownCast(dataset->GetPointData()->GetGlobalIds());
        auto ghostArray = dataset->GetPointData()->GetGhostArray();
        const vtkIdType numberOfPoints = dataset->GetNumberOfPoints();
        std::vector<vtkIdType> ghostPoints;
        ghostPoints.reserve(0.1 * numberOfPoints); // reserve 10% of the points for ghost ones
        for (vtkIdType ptId = 0; ptId < numberOfPoints; ++ptId)
        {
          const vtkIdType globalId = globalIdsArray->GetValue(ptId);
          auto it = addedPointsMap.find(globalId);
          if (it == addedPointsMap.end())
          {
            // skip ghost points to get the point data from the non-ghost ones
            if (ghostArray && ghostArray->GetValue(ptId) > 0)
            {
              // store ghost point and process it later
              ghostPoints.push_back(ptId);
            }
            else
            {
              globalIndices[pointOffset + ptId] = totalMergedPoints;
              mergeMap[pointOffset + ptId] = totalMergedPoints;
              addedPointsMap[globalId] = totalMergedPoints++;
            }
          }
          else
          {
            // point already added, update the global index
            globalIndices[pointOffset + ptId] = it->second;
          }
        }
        // now update the ghost points, if any, this is done to copy the point data from
        // the non-ghost points (if any is present).
        for (const auto& ptId : ghostPoints)
        {
          const vtkIdType globalId = globalIdsArray->GetValue(ptId);
          auto it = addedPointsMap.find(globalId);
          if (it == addedPointsMap.end())
          {
            globalIndices[pointOffset + ptId] = totalMergedPoints;
            mergeMap[pointOffset + ptId] = totalMergedPoints;
            addedPointsMap[globalId] = totalMergedPoints++;
          }
          else
          {
            // point already added, update the global index
            globalIndices[pointOffset + ptId] = it->second;
          }
        }
      }
    }
    else // merge points with locator
    {
      // create a temporary dataset to merge points
      vtkNew<vtkPointSet> tempDataSet;
      tempDataSet->SetPoints(newPoints);

      // Create a locator to merge points
      vtkNew<vtkStaticPointLocator> locator;
      locator->SetDataSet(tempDataSet);
      locator->BuildLocator();

      const double tolerance =
        this->ToleranceIsAbsolute ? this->Tolerance : this->Tolerance * tempDataSet->GetLength();
      // merge points, See vtkStaticCleanUnstructuredGrid
      locator->MergePoints(tolerance, mergeMap.data());
      // Build the map from old points to new points.
      totalMergedPoints = vtkStaticCleanUnstructuredGrid::BuildPointMap(
        totalNumberOfPoints, globalIndices.data(), nullptr, mergeMap);
    }

    // copy points and point data to the output
    vtkNew<vtkPoints> mergedNewPoints;
    mergedNewPoints->SetDataType(newPoints->GetDataType());
    mergedNewPoints->SetNumberOfPoints(totalMergedPoints);
    vtkNew<vtkPointData> mergedNewPD;
    if (haveGlobalIdsArray)
    {
      mergedNewPD->CopyAllOn(vtkDataSetAttributes::COPYTUPLE);
    }
    mergedNewPD->CopyAllocate(outputPD, totalMergedPoints);
    mergedNewPD->SetNumberOfTuples(totalMergedPoints);
    // 1) When a locator is used, the global indices are ordered properly thanks to MergePoints,
    //    and they are used to copy the points and point data.
    // 2) When global ids are used, we need to mark the points whose point data will NOT be copied.
    //    That's why a mergeMap is used instead of globalIndices.
    vtkIdType* pointMap = haveGlobalIdsArray ? mergeMap.data() : globalIndices.data();
    vtkStaticCleanUnstructuredGrid::CopyPoints(
      newPoints, outputPD, mergedNewPoints, mergedNewPD, pointMap);

    newPoints->ShallowCopy(mergedNewPoints);
    outputPD->ShallowCopy(mergedNewPD);
  }
  else // no merging, just append
  {
    // global indices not needed
  }
  output->SetPoints(newPoints);
  this->UpdateProgress(0.50);
  if (this->CheckAbort())
  {
    return 1;
  }

  // create cells;
  const bool havePolyhedronFaces = totalNumberOfFaces > 0 && totalNumberOfFaceConnectivity > 0;
  vtkNew<vtkUnsignedCharArray> cellTypesArray;
  cellTypesArray->SetNumberOfValues(totalNumberOfCells);
  vtkNew<vtkIdTypeArray> offsetsArray;
  offsetsArray->SetNumberOfValues(totalNumberOfCells + 1);
  offsetsArray->SetValue(totalNumberOfCells, totalNumberOfCellConnectivity);
  vtkNew<vtkIdTypeArray> connectivityArray;
  connectivityArray->SetNumberOfValues(totalNumberOfCellConnectivity);
  vtkNew<vtkIdTypeArray> faceOffsetsArray;
  vtkNew<vtkIdTypeArray> faceConnectivityArray;
  vtkNew<vtkIdTypeArray> faceLocationsOffsetsArray;
  vtkNew<vtkIdTypeArray> faceLocationsConnectivityArray;
  if (havePolyhedronFaces)
  {
    faceOffsetsArray->SetNumberOfValues(totalNumberOfFaces + 1);
    faceOffsetsArray->SetValue(totalNumberOfFaces, totalNumberOfFaceConnectivity);
    faceConnectivityArray->SetNumberOfValues(totalNumberOfFaceConnectivity);
    faceLocationsOffsetsArray->SetNumberOfValues(totalNumberOfCells + 1);
    faceLocationsOffsetsArray->SetValue(totalNumberOfCells, totalNumberOfFaces);
    faceLocationsConnectivityArray->SetNumberOfValues(totalNumberOfFaces);
  }
  vtkSMPThreadLocalObject<vtkIdList> tlPointIdsList, tlNewPointIdsList;
  vtkSMPTools::For(0, static_cast<vtkIdType>(datasets.size()),
    [&](vtkIdType begin, vtkIdType end)
    {
      for (vtkIdType idx = begin; idx < end; ++idx)
      {
        const auto& dataset = datasets[idx];
        const auto& pointOffset = pointOffsets[idx];
        auto cellOffset = cellOffsets[idx];
        auto cellConnectivityOffset = cellConnectivityOffsets[idx];
        const auto& faceOffset = faceOffsets[idx];
        const auto& faceConnectivityOffset = faceConnectivityOffsets[idx];
        const auto numberOfCells = dataset->GetNumberOfCells();

        if (auto ug = vtkUnstructuredGrid::SafeDownCast(dataset)) // vtkUnstructuredGrid
        {
          // copy cell types
          auto cellTypes = ug->GetCellTypesArray();
          std::copy_n(
            cellTypes->GetPointer(0), numberOfCells, cellTypesArray->GetPointer(cellOffset));
          // copy cells
          ug->GetCells()->Visit(AppendCellArray{}, offsetsArray, connectivityArray, cellOffset,
            cellConnectivityOffset, globalIndices, pointOffset);
          if (havePolyhedronFaces)
          {
            if (ug->GetPolyhedronFaces() && ug->GetPolyhedronFaceLocations()) // handle polyhedrons
            {
              // copy polyhedron faces
              ug->GetPolyhedronFaces()->Visit(AppendCellArray{}, faceOffsetsArray,
                faceConnectivityArray, faceOffset, faceConnectivityOffset, globalIndices,
                pointOffset);
              // copy polyhedron face locations
              std::vector<vtkIdType> dummy;
              ug->GetPolyhedronFaceLocations()->Visit(AppendCellArray{}, faceLocationsOffsetsArray,
                faceLocationsConnectivityArray, cellOffset, faceOffset, dummy, faceOffset);
            }
            else
            {
              // fill location to avoid incorrect offsets
              std::fill_n(
                faceLocationsOffsetsArray->GetPointer(cellOffset), numberOfCells, faceOffset);
            }
          }
        }
        else if (auto pd = vtkPolyData::SafeDownCast(dataset)) // vtkPolyData
        {
          // copy cell types
          for (vtkIdType i = 0; i < numberOfCells; i++)
          {
            cellTypesArray->SetValue(
              cellOffset + i, static_cast<unsigned char>(pd->GetCellType(i)));
          }
          if (havePolyhedronFaces)
          {
            // fill location to avoid incorrect offsets
            std::fill_n(
              faceLocationsOffsetsArray->GetPointer(cellOffset), numberOfCells, faceOffset);
          }
          // copy cells
          if (auto numVerts = pd->GetVerts()->GetNumberOfCells())
          {
            pd->GetVerts()->Visit(AppendCellArray{}, offsetsArray, connectivityArray, cellOffset,
              cellConnectivityOffset, globalIndices, pointOffset);
            cellOffset += numVerts;
            cellConnectivityOffset += pd->GetVerts()->GetNumberOfConnectivityIds();
          }
          if (auto numLines = pd->GetLines()->GetNumberOfCells())
          {
            pd->GetLines()->Visit(AppendCellArray{}, offsetsArray, connectivityArray, cellOffset,
              cellConnectivityOffset, globalIndices, pointOffset);
            cellOffset += numLines;
            cellConnectivityOffset += pd->GetLines()->GetNumberOfConnectivityIds();
          }
          if (auto numPolys = pd->GetPolys()->GetNumberOfCells())
          {
            pd->GetPolys()->Visit(AppendCellArray{}, offsetsArray, connectivityArray, cellOffset,
              cellConnectivityOffset, globalIndices, pointOffset);
            cellOffset += numPolys;
            cellConnectivityOffset += pd->GetPolys()->GetNumberOfConnectivityIds();
          }
          if (auto numStrips = pd->GetStrips()->GetNumberOfCells())
          {
            pd->GetStrips()->Visit(AppendCellArray{}, offsetsArray, connectivityArray, cellOffset,
              cellConnectivityOffset, globalIndices, pointOffset);
            cellOffset += numStrips;
            cellConnectivityOffset += pd->GetStrips()->GetNumberOfConnectivityIds();
          }
        }
        else // vtkDataSet
        {
          if (havePolyhedronFaces)
          {
            // fill location to avoid incorrect offsets
            std::fill_n(
              faceLocationsOffsetsArray->GetPointer(cellOffset), numberOfCells, faceOffset);
          }
          auto pointIdsList = tlPointIdsList.Local();
          auto newPointIdsList = tlNewPointIdsList.Local();
          vtkIdType numPointIds;
          const vtkIdType* pointIds;
          for (vtkIdType cellId = 0; cellId < numberOfCells; ++cellId)
          {
            // get cell point Ids
            dataset->GetCellPoints(cellId, numPointIds, pointIds, pointIdsList);
            // renumber point Ids
            newPointIdsList->SetNumberOfIds(numPointIds);
            if (!globalIndices.empty())
            {
              std::transform(pointIds, pointIds + numPointIds, newPointIdsList->GetPointer(0),
                [&](vtkIdType ptId) { return globalIndices[ptId + pointOffset]; });
            }
            else
            {
              std::transform(pointIds, pointIds + numPointIds, newPointIdsList->GetPointer(0),
                [&](vtkIdType ptId) { return ptId + pointOffset; });
            }
            // set cell type
            auto cellType = static_cast<unsigned char>(dataset->GetCellType(cellId));
            cellTypesArray->SetValue(cellOffset, cellType);
            // set offset
            offsetsArray->SetValue(cellOffset, cellConnectivityOffset);
            // set connectivity
            std::copy_n(newPointIdsList->GetPointer(0), numPointIds,
              connectivityArray->GetPointer(cellConnectivityOffset));
            // increment offsets
            cellOffset++;
            cellConnectivityOffset += numPointIds;
          }
        }
      }
    });
  vtkNew<vtkCellArray> newCells;
  newCells->SetData(offsetsArray, connectivityArray);
  if (!havePolyhedronFaces)
  {
    output->SetPolyhedralCells(cellTypesArray, newCells, nullptr, nullptr);
  }
  else
  {
    vtkNew<vtkCellArray> newFaces;
    newFaces->SetData(faceOffsetsArray, faceConnectivityArray);
    vtkNew<vtkCellArray> newFaceLocations;
    newFaceLocations->SetData(faceLocationsOffsetsArray, faceLocationsConnectivityArray);
    output->SetPolyhedralCells(cellTypesArray, newCells, newFaceLocations, newFaces);
  }
  this->UpdateProgress(0.75);
  if (this->CheckAbort())
  {
    return 1;
  }

  // Since cells are not merged, this filter can easily pass all field arrays, including global ids.
  auto outputCD = output->GetCellData();
  outputCD->CopyAllOn(vtkDataSetAttributes::COPYTUPLE);
  vtkAppendFilter::AppendArrays(vtkDataObject::CELL, inputVector, output, totalNumberOfCells);
  this->UpdateProgress(1.00);

  // Release memory
  output->Squeeze();

  return 1;
}

//------------------------------------------------------------------------------
std::vector<vtkSmartPointer<vtkDataSet>> vtkAppendFilter::GetNonEmptyInputs(
  vtkInformationVector** inputVector)
{
  std::vector<vtkSmartPointer<vtkDataSet>> collection;
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
      collection.emplace_back(dataSet);
    }
  }

  return collection;
}

//------------------------------------------------------------------------------
void vtkAppendFilter::AppendArrays(int attributesType, vtkInformationVector** inputVector,
  vtkUnstructuredGrid* output, vtkIdType totalNumberOfElements)
{
  // Check if attributesType is supported
  if (attributesType != vtkDataObject::POINT && attributesType != vtkDataObject::CELL)
  {
    vtkErrorMacro(<< "Unhandled attributes type " << attributesType << ", must be either "
                  << "vtkDataObject::POINT or vtkDataObject::CELL");
    return;
  }

  vtkDataSetAttributes::FieldList fieldList;
  auto datasets = this->GetNonEmptyInputs(inputVector);
  std::vector<vtkIdType> offsets;
  vtkIdType prevOffset = 0;
  for (const auto& dataset : datasets)
  {
    if (auto inputData = dataset->GetAttributes(attributesType))
    {
      fieldList.IntersectFieldList(inputData);
      offsets.push_back(prevOffset);
      prevOffset += inputData->GetNumberOfTuples();
    }
    else
    {
      offsets.emplace_back(prevOffset);
    }
  }

  vtkDataSetAttributes* outputData = output->GetAttributes(attributesType);
  outputData->CopyAllocate(fieldList, totalNumberOfElements);
  outputData->SetNumberOfTuples(totalNumberOfElements);
  // copy arrays.
  vtkSMPTools::For(0, static_cast<vtkIdType>(datasets.size()),
    [&](vtkIdType begin, vtkIdType end)
    {
      for (vtkIdType idx = begin; idx < end; ++idx)
      {
        auto& dataSet = datasets[idx];
        const auto& offset = offsets[idx];
        auto inputData = dataSet->GetAttributes(attributesType);
        const auto numberOfInputTuples = inputData->GetNumberOfTuples();
        fieldList.CopyData(idx, inputData, 0, numberOfInputTuples, outputData, offset);
      }
    });
}

//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
int vtkAppendFilter::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  info->Set(vtkAlgorithm::INPUT_IS_REPEATABLE(), 1);
  return 1;
}

//------------------------------------------------------------------------------
void vtkAppendFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "MergePoints:" << (this->MergePoints ? "On" : "Off") << "\n";
  os << indent << "OutputPointsPrecision: " << this->OutputPointsPrecision << "\n";
  os << indent << "Tolerance: " << this->Tolerance << "\n";
}
VTK_ABI_NAMESPACE_END
