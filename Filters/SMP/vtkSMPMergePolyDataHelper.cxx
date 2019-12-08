/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkContourGrid.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMPMergePolyDataHelper.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkDataArrayRange.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkSMPMergePoints.h"
#include "vtkSMPTools.h"
#include "vtkSmartPointer.h"

#include <algorithm>

namespace
{

struct vtkMergePointsData
{
  vtkPolyData* Output;
  vtkSMPMergePoints* Locator;

  vtkMergePointsData(vtkPolyData* output, vtkSMPMergePoints* locator)
    : Output(output)
    , Locator(locator)
  {
  }
};

class vtkParallelMergePoints
{
public:
  vtkIdType* BucketIds;
  std::vector<vtkMergePointsData>::iterator Begin;
  std::vector<vtkMergePointsData>::iterator End;
  vtkSMPMergePoints* Merger;
  vtkIdList** IdMaps;
  vtkPointData* OutputPointData;
  vtkPointData** InputPointDatas;

  void operator()(vtkIdType begin, vtkIdType end)
  {
    // All actual work is done by vtkSMPMergePoints::Merge
    std::vector<vtkMergePointsData>::iterator itr;
    vtkPointData* outPD = this->OutputPointData;

    vtkIdType counter = 0;
    for (itr = Begin; itr != End; ++itr)
    {
      vtkIdList* idMap = this->IdMaps[counter];
      vtkPointData* inPD = this->InputPointDatas[counter++];
      for (vtkIdType i = begin; i < end; i++)
      {
        vtkIdType bucketId = BucketIds[i];
        if ((*itr).Locator->GetNumberOfIdsInBucket(bucketId) > 0)
        {
          Merger->Merge((*itr).Locator, bucketId, outPD, inPD, idMap);
        }
      }
    }
  }
};

void MergePoints(
  std::vector<vtkMergePointsData>& data, std::vector<vtkIdList*>& idMaps, vtkPolyData* outPolyData)
{
  // This merges points in parallel/

  std::vector<vtkMergePointsData>::iterator itr = data.begin();
  std::vector<vtkMergePointsData>::iterator begin = itr;
  std::vector<vtkMergePointsData>::iterator end = data.end();
  vtkPoints* outPts = (*itr).Output->GetPoints();

  // Prepare output points
  vtkIdType numPts = 0;
  while (itr != end)
  {
    numPts += (*itr).Output->GetNumberOfPoints();
    ++itr;
  }
  outPts->Resize(numPts);

  // Find non-empty buckets for best load balancing. We don't
  // want to visit bunch of empty buckets.
  vtkIdType numBuckets = (*begin).Locator->GetNumberOfBuckets();
  std::vector<vtkIdType> nonEmptyBuckets;
  std::vector<bool> bucketVisited(numBuckets, false);
  nonEmptyBuckets.reserve(numBuckets);
  for (itr = begin; itr != end; ++itr)
  {
    vtkSMPMergePoints* mp = (*itr).Locator;
    for (vtkIdType i = 0; i < numBuckets; i++)
    {
      if (mp->GetNumberOfIdsInBucket(i) > 0 && !bucketVisited[i])
      {
        nonEmptyBuckets.push_back(i);
        bucketVisited[i] = true;
      }
    }
  }

  // These id maps will later be used when merging cells.
  std::vector<vtkPointData*> pds;
  itr = begin;
  ++itr;
  while (itr != end)
  {
    pds.push_back((*itr).Output->GetPointData());
    vtkIdList* idMap = vtkIdList::New();
    idMap->Allocate((*itr).Output->GetNumberOfPoints());
    idMaps.push_back(idMap);
    ++itr;
  }

  vtkParallelMergePoints mergePoints;
  mergePoints.BucketIds = &nonEmptyBuckets[0];
  mergePoints.Merger = (*begin).Locator;
  mergePoints.OutputPointData = (*begin).Output->GetPointData();
  if (!idMaps.empty())
  {
    mergePoints.Merger->InitializeMerge();
    mergePoints.IdMaps = &idMaps[0];
    // Prepare output point data
    int numArrays = mergePoints.OutputPointData->GetNumberOfArrays();
    for (int i = 0; i < numArrays; i++)
    {
      mergePoints.OutputPointData->GetArray(i)->Resize(numPts);
    }
    mergePoints.InputPointDatas = &pds[0];

    // The first locator is what we will use to accumulate all others
    // So all iteration starts from second dataset.
    std::vector<vtkMergePointsData>::iterator second = begin;
    ++second;
    mergePoints.Begin = second;
    mergePoints.End = end;
    // Actual work
    vtkSMPTools::For(0, static_cast<vtkIdType>(nonEmptyBuckets.size()), mergePoints);
    // mergePoints.operator()(0, nonEmptyBuckets.size());

    // Fixup output sizes.
    mergePoints.Merger->FixSizeOfPointArray();
    for (int i = 0; i < numArrays; i++)
    {
      mergePoints.OutputPointData->GetArray(i)->SetNumberOfTuples(
        mergePoints.Merger->GetMaxId() + 1);
    }
  }
  outPolyData->SetPoints(mergePoints.Merger->GetPoints());
  outPolyData->GetPointData()->ShallowCopy(mergePoints.OutputPointData);
}

class vtkParallelMergeCells
{
public:
  vtkIdList* CellOffsets;
  vtkIdList* ConnOffsets;
  vtkCellArray* InCellArray;
  vtkCellArray* OutCellArray;
  vtkIdType OutputCellOffset;
  vtkIdType OutputConnOffset;
  vtkIdList* IdMap;

  struct MapCellsImpl
  {
    // Call this signature:
    template <typename InCellStateT>
    void operator()(InCellStateT& inState, vtkCellArray* outCells, vtkIdType inCellOffset,
      vtkIdType inCellOffsetEnd, vtkIdType inConnOffset, vtkIdType inConnOffsetEnd,
      vtkIdType outCellOffset, vtkIdType outConnOffset, vtkIdList* map)
    {
      outCells->Visit(*this, inState, inCellOffset, inCellOffsetEnd, inConnOffset, inConnOffsetEnd,
        outCellOffset, outConnOffset, map);
    }

    // Internal signature:
    template <typename InCellStateT, typename OutCellStateT>
    void operator()(OutCellStateT& outState, InCellStateT& inState, vtkIdType inCellOffset,
      vtkIdType inCellOffsetEnd, vtkIdType inConnOffset, vtkIdType inConnOffsetEnd,
      vtkIdType outCellOffset, vtkIdType outConnOffset, vtkIdList* map)
    {
      using InIndexType = typename InCellStateT::ValueType;
      using OutIndexType = typename OutCellStateT::ValueType;

      const auto inCell =
        vtk::DataArrayValueRange<1>(inState.GetOffsets(), inCellOffset, inCellOffsetEnd + 1);
      const auto inConn =
        vtk::DataArrayValueRange<1>(inState.GetConnectivity(), inConnOffset, inConnOffsetEnd);
      auto outCell =
        vtk::DataArrayValueRange<1>(outState.GetOffsets(), outCellOffset + inCellOffset);
      auto outConn =
        vtk::DataArrayValueRange<1>(outState.GetConnectivity(), outConnOffset + inConnOffset);

      // Copy the offsets, adding outConnOffset to adjust for existing
      // connectivity entries:
      std::transform(
        inCell.cbegin(), inCell.cend(), outCell.begin(), [&](InIndexType i) -> OutIndexType {
          return static_cast<OutIndexType>(i + outConnOffset);
        });

      // Copy the connectivities, passing them through the map:
      std::transform(
        inConn.cbegin(), inConn.cend(), outConn.begin(), [&](InIndexType i) -> OutIndexType {
          return static_cast<OutIndexType>(map->GetId(static_cast<vtkIdType>(i)));
        });
    }
  };

  void operator()(vtkIdType begin, vtkIdType end)
  {
    vtkIdType noffsets = this->CellOffsets->GetNumberOfIds();
    vtkIdList* cellOffsets = this->CellOffsets;
    vtkIdList* connOffsets = this->ConnOffsets;
    vtkCellArray* outCellArray = this->OutCellArray;
    vtkCellArray* inCellArray = this->InCellArray;
    vtkIdType outputCellOffset = this->OutputCellOffset;
    vtkIdType outputConnOffset = this->OutputConnOffset;
    vtkIdList* map = this->IdMap;

    for (vtkIdType i = begin; i < end; i++)
    {
      // Note that there may be multiple cells starting at
      // this offset. So we find the next offset and insert
      // all cells between here and there.
      vtkIdType nextCellOffset;
      vtkIdType nextConnOffset;
      if (i ==
        noffsets - 1) // This needs to be the end of the array always, not the loop counter's end
      {
        nextCellOffset = this->InCellArray->GetNumberOfCells();
        nextConnOffset = this->InCellArray->GetNumberOfConnectivityIds();
      }
      else
      {
        nextCellOffset = cellOffsets->GetId(i + 1);
        nextConnOffset = connOffsets->GetId(i + 1);
      }
      // Process all cells between the given offset and the next.
      vtkIdType cellOffset = cellOffsets->GetId(i);
      vtkIdType connOffset = connOffsets->GetId(i);

      inCellArray->Visit(MapCellsImpl{}, outCellArray, cellOffset, nextCellOffset, connOffset,
        nextConnOffset, outputCellOffset, outputConnOffset, map);
    }
  }
};

class vtkParallelCellDataCopier
{
public:
  vtkDataSetAttributes* InputCellData;
  vtkDataSetAttributes* OutputCellData;
  vtkIdType Offset;

  void operator()(vtkIdType begin, vtkIdType end)
  {
    vtkDataSetAttributes* inputCellData = this->InputCellData;
    vtkDataSetAttributes* outputCellData = this->OutputCellData;
    vtkIdType offset = this->Offset;

    for (vtkIdType i = begin; i < end; i++)
    {
      outputCellData->SetTuple(offset + i, i, inputCellData);
    }
  }
};

struct vtkMergeCellsData
{
  vtkPolyData* Output;
  vtkIdList* CellOffsets;
  vtkIdList* ConnOffsets;
  vtkCellArray* OutCellArray;

  vtkMergeCellsData(
    vtkPolyData* output, vtkIdList* cellOffsets, vtkIdList* connOffsets, vtkCellArray* cellArray)
    : Output(output)
    , CellOffsets(cellOffsets)
    , ConnOffsets(connOffsets)
    , OutCellArray(cellArray)
  {
  }
};

struct CopyCellArraysToFront
{
  // call this signature:
  template <typename OutCellArraysT>
  void operator()(OutCellArraysT& out, vtkCellArray* in)
  {
    in->Visit(*this, out);
  }

  // Internal signature:
  template <typename InCellArraysT, typename OutCellArraysT>
  void operator()(InCellArraysT& in, OutCellArraysT& out)
  {
    using InIndexType = typename InCellArraysT::ValueType;
    using OutIndexType = typename OutCellArraysT::ValueType;

    const auto inCell = vtk::DataArrayValueRange<1>(in.GetOffsets());
    const auto inConn = vtk::DataArrayValueRange<1>(in.GetConnectivity());
    auto outCell = vtk::DataArrayValueRange<1>(out.GetOffsets());
    auto outConn = vtk::DataArrayValueRange<1>(out.GetConnectivity());

    auto cast = [](InIndexType i) -> OutIndexType { return static_cast<OutIndexType>(i); };

    std::transform(inCell.cbegin(), inCell.cend(), outCell.begin(), cast);
    std::transform(inConn.cbegin(), inConn.cend(), outConn.begin(), cast);
  }
};

void MergeCells(std::vector<vtkMergeCellsData>& data, const std::vector<vtkIdList*>& idMaps,
  vtkIdType cellDataOffset, vtkCellArray* outCells)
{
  std::vector<vtkMergeCellsData>::iterator begin = data.begin();
  std::vector<vtkMergeCellsData>::iterator itr;
  std::vector<vtkMergeCellsData>::iterator second = begin;
  std::vector<vtkMergeCellsData>::iterator end = data.end();
  ++second;

  std::vector<vtkIdList*>::const_iterator mapIter = idMaps.begin();

  vtkCellArray* firstCells = (*begin).OutCellArray;

  vtkIdType outCellOffset = 0;
  vtkIdType outConnOffset = 0;
  outCellOffset += firstCells->GetNumberOfCells();
  outConnOffset += firstCells->GetNumberOfConnectivityIds();

  // Prepare output. Since there's no mapping here, do a simple copy in
  // serial:
  outCells->Visit(CopyCellArraysToFront{}, firstCells);

  vtkParallelMergeCells mergeCells;
  mergeCells.OutCellArray = outCells;

  // The first locator is what we will use to accumulate all others
  // So all iteration starts from second dataset.
  for (itr = second; itr != end; ++itr, ++mapIter)
  {
    mergeCells.CellOffsets = (*itr).CellOffsets;
    mergeCells.ConnOffsets = (*itr).ConnOffsets;
    mergeCells.InCellArray = (*itr).OutCellArray;
    mergeCells.OutputCellOffset = outCellOffset;
    mergeCells.OutputConnOffset = outConnOffset;
    mergeCells.IdMap = *mapIter;

    // First, we merge the cell arrays. This also adjust point ids.
    vtkSMPTools::For(0, mergeCells.CellOffsets->GetNumberOfIds(), mergeCells);

    outCellOffset += (*itr).OutCellArray->GetNumberOfCells();
    outConnOffset += (*itr).OutCellArray->GetNumberOfConnectivityIds();
  }

  vtkIdType outCellsOffset = cellDataOffset + (*begin).OutCellArray->GetNumberOfCells();

  // Now copy cell data in parallel
  vtkParallelCellDataCopier cellCopier;
  cellCopier.OutputCellData = (*begin).Output->GetCellData();
  int numCellArrays = cellCopier.OutputCellData->GetNumberOfArrays();
  if (numCellArrays > 0)
  {
    for (itr = second; itr != end; ++itr)
    {
      cellCopier.InputCellData = (*itr).Output->GetCellData();
      cellCopier.Offset = outCellsOffset;
      vtkCellArray* cells = (*itr).OutCellArray;

      vtkSMPTools::For(0, cells->GetNumberOfCells(), cellCopier);
      // cellCopier.operator()(0, polys->GetNumberOfCells());

      outCellsOffset += (*itr).Output->GetPolys()->GetNumberOfCells();
    }
  }
}
}

vtkPolyData* vtkSMPMergePolyDataHelper::MergePolyData(std::vector<InputData>& inputs)
{
  // First merge points

  std::vector<InputData>::iterator itr = inputs.begin();
  std::vector<InputData>::iterator begin = itr;
  std::vector<InputData>::iterator end = inputs.end();

  std::vector<vtkMergePointsData> mpData;
  while (itr != end)
  {
    mpData.push_back(vtkMergePointsData((*itr).Input, (*itr).Locator));
    ++itr;
  }

  std::vector<vtkIdList*> idMaps;
  vtkPolyData* outPolyData = vtkPolyData::New();

  MergePoints(mpData, idMaps, outPolyData);

  itr = begin;
  vtkIdType vertSize = 0;
  vtkIdType lineSize = 0;
  vtkIdType polySize = 0;
  vtkIdType numVerts = 0;
  vtkIdType numLines = 0;
  vtkIdType numPolys = 0;
  std::vector<vtkMergeCellsData> mcData;
  while (itr != end)
  {
    vertSize += (*itr).Input->GetVerts()->GetNumberOfConnectivityIds();
    lineSize += (*itr).Input->GetLines()->GetNumberOfConnectivityIds();
    polySize += (*itr).Input->GetPolys()->GetNumberOfConnectivityIds();
    numVerts += (*itr).Input->GetVerts()->GetNumberOfCells();
    numLines += (*itr).Input->GetLines()->GetNumberOfCells();
    numPolys += (*itr).Input->GetPolys()->GetNumberOfCells();
    ++itr;
  }

  vtkIdType numOutCells = numVerts + numLines + numPolys;

  vtkCellData* outCellData = (*begin).Input->GetCellData();
  int numCellArrays = outCellData->GetNumberOfArrays();
  for (int i = 0; i < numCellArrays; i++)
  {
    outCellData->GetArray(i)->Resize(numOutCells);
    outCellData->GetArray(i)->SetNumberOfTuples(numOutCells);
  }

  // Now merge each cell type. Because vtkPolyData stores each
  // cell type separately, we need to merge them separately.

  if (vertSize > 0)
  {
    vtkNew<vtkCellArray> outVerts;
    outVerts->ResizeExact(numVerts, vertSize);

    itr = begin;
    while (itr != end)
    {
      mcData.push_back(vtkMergeCellsData(
        (*itr).Input, (*itr).VertCellOffsets, (*itr).VertConnOffsets, (*itr).Input->GetVerts()));
      ++itr;
    }
    MergeCells(mcData, idMaps, 0, outVerts);

    outPolyData->SetVerts(outVerts);

    mcData.clear();
  }

  if (lineSize > 0)
  {
    vtkNew<vtkCellArray> outLines;
    outLines->ResizeExact(numLines, lineSize);

    itr = begin;
    while (itr != end)
    {
      mcData.push_back(vtkMergeCellsData(
        (*itr).Input, (*itr).LineCellOffsets, (*itr).LineConnOffsets, (*itr).Input->GetLines()));
      ++itr;
    }
    MergeCells(mcData, idMaps, vertSize, outLines);

    outPolyData->SetLines(outLines);

    mcData.clear();
  }

  if (polySize > 0)
  {
    vtkNew<vtkCellArray> outPolys;
    outPolys->ResizeExact(numPolys, polySize);

    itr = begin;
    while (itr != end)
    {
      mcData.push_back(vtkMergeCellsData(
        (*itr).Input, (*itr).PolyCellOffsets, (*itr).PolyConnOffsets, (*itr).Input->GetPolys()));
      ++itr;
    }
    MergeCells(mcData, idMaps, vertSize + lineSize, outPolys);

    outPolyData->SetPolys(outPolys);
  }

  outPolyData->GetCellData()->ShallowCopy(outCellData);

  std::vector<vtkIdList*>::iterator mapIter = idMaps.begin();
  while (mapIter != idMaps.end())
  {
    (*mapIter)->Delete();
    ++mapIter;
  }

  return outPolyData;
}
