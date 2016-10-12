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
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkSmartPointer.h"
#include "vtkSMPTools.h"
#include "vtkSMPMergePoints.h"

namespace
{

struct vtkMergePointsData
{
  vtkPolyData* Output;
  vtkSMPMergePoints* Locator;

  vtkMergePointsData(vtkPolyData* output, vtkSMPMergePoints* locator) :
    Output(output), Locator(locator)
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

void MergePoints(std::vector<vtkMergePointsData>& data,
                 std::vector<vtkIdList*>& idMaps,
                 vtkPolyData* outPolyData)
{
  // This merges points in parallel/

  std::vector<vtkMergePointsData>::iterator itr = data.begin();
  std::vector<vtkMergePointsData>::iterator begin = itr;
  std::vector<vtkMergePointsData>::iterator end = data.end();
  vtkPoints* outPts = (*itr).Output->GetPoints();

  // Prepare output points
  vtkIdType numPts = 0;
  while(itr != end)
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
    for (int i=0; i<numArrays; i++)
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
    vtkSMPTools::For(0, nonEmptyBuckets.size(), mergePoints);
    //mergePoints.operator()(0, nonEmptyBuckets.size());

    // Fixup output sizes.
    mergePoints.Merger->FixSizeOfPointArray();
    for (int i=0; i<numArrays; i++)
    {
      mergePoints.OutputPointData->GetArray(i)->SetNumberOfTuples(mergePoints.Merger->GetMaxId()+1);
    }
  }
  outPolyData->SetPoints(mergePoints.Merger->GetPoints());
  outPolyData->GetPointData()->ShallowCopy(mergePoints.OutputPointData);
}

class vtkParallelMergeCells
{
public:
  vtkIdList* Offsets;
  vtkCellArray* InCellArray;
  vtkIdTypeArray* OutCellArray;
  vtkIdType OutputOffset;
  vtkIdList* IdMap;

  void operator()(vtkIdType begin, vtkIdType end)
  {
    vtkIdType noffsets = this->Offsets->GetNumberOfIds();
    vtkIdList* offsets = this->Offsets;
    vtkIdTypeArray* outCellArray = this->OutCellArray;
    vtkCellArray* inCellArray = this->InCellArray;
    vtkIdType outputOffset = this->OutputOffset;
    vtkIdList* map = this->IdMap;

    vtkNew<vtkIdList> cellIds;
    for (vtkIdType i=begin; i<end; i++)
    {
      // Note that there may be multiple cells starting at
      // this offset. So we find the next offset and insert
      // all cells between here and there.
      vtkIdType nextOffset;
      if (i == noffsets - 1) // This needs to be the end of the array always, not the loop counter's end
      {
        nextOffset = this->InCellArray->GetNumberOfConnectivityEntries();
      }
      else
      {
        nextOffset = offsets->GetId(i+1);
      }
      // Process all cells between the given offset and the next.
      vtkIdType cellOffset = offsets->GetId(i);
      while (cellOffset < nextOffset)
      {
        inCellArray->GetCell(cellOffset, cellIds.GetPointer());
        vtkIdType nids = cellIds->GetNumberOfIds();
        // Insert the cells - first number of points and ids
        outCellArray->SetValue(outputOffset + cellOffset, nids);
        cellOffset++;
        for (int j=0; j<nids; j++)
        {
          // Now insert each id. First map it through the map generated by the merging
          // of the points
          outCellArray->SetValue(outputOffset + cellOffset, map->GetId(cellIds->GetId(j)));
          cellOffset++;
        }
      }
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

    for (vtkIdType i=begin; i<end; i++)
    {
      outputCellData->SetTuple(offset + i, i, inputCellData);
    }
  }
};

struct vtkMergeCellsData
{
  vtkPolyData* Output;
  vtkIdList* CellOffsets;
  vtkCellArray* OutCellArray;

  vtkMergeCellsData(vtkPolyData* output, vtkIdList* celloffsets, vtkCellArray* cellarray) :
    Output(output), CellOffsets(celloffsets), OutCellArray(cellarray)
  {
  }
};

void MergeCells(std::vector<vtkMergeCellsData>& data,
                const std::vector<vtkIdList*>& idMaps,
                vtkIdType numCells,
                vtkIdType cellDataOffset,
                vtkCellArray* outCells)
{
  std::vector<vtkMergeCellsData>::iterator begin = data.begin();
  std::vector<vtkMergeCellsData>::iterator itr;
  std::vector<vtkMergeCellsData>::iterator second = begin;
  std::vector<vtkMergeCellsData>::iterator end = data.end();
  ++second;

  std::vector<vtkIdList*>::const_iterator mapIter = idMaps.begin();

  vtkIdTypeArray* outCellsArray = outCells->GetData();

  vtkIdType outCellsOffset = 0;

  // Prepare output
  vtkCellArray* firstCells = (*begin).OutCellArray;
  vtkIdTypeArray* firstCellsArray = firstCells->GetData();
  outCellsOffset += firstCells->GetNumberOfConnectivityEntries();
  memcpy(outCellsArray->GetVoidPointer(0),
         firstCellsArray->GetVoidPointer(0),
         outCellsOffset*sizeof(vtkIdType));

  vtkParallelMergeCells mergeCells;
  mergeCells.OutCellArray = outCellsArray;

  // The first locator is what we will use to accumulate all others
  // So all iteration starts from second dataset.
  vtkNew<vtkIdList> cellIds;
  for (itr = second; itr != end; ++itr, ++mapIter)
  {
    mergeCells.Offsets = (*itr).CellOffsets;
    mergeCells.InCellArray = (*itr).OutCellArray;
    mergeCells.OutputOffset = outCellsOffset;
    mergeCells.IdMap = *mapIter;

    // First, we merge the cell arrays. This also adjust point ids.
    vtkSMPTools::For(0,  mergeCells.Offsets->GetNumberOfIds(), mergeCells);

    outCellsOffset += (*itr).OutCellArray->GetNumberOfConnectivityEntries();
  }

  outCellsArray->SetNumberOfTuples(outCellsOffset);
  outCells->SetNumberOfCells(numCells);

  outCellsOffset = cellDataOffset + (*begin).OutCellArray->GetNumberOfCells();

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

      vtkSMPTools::For(0,  cells->GetNumberOfCells(), cellCopier);
      //cellCopier.operator()(0, polys->GetNumberOfCells());

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
  while(itr != end)
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
  while(itr != end)
  {
    vertSize += (*itr).Input->GetVerts()->GetNumberOfConnectivityEntries();
    lineSize += (*itr).Input->GetLines()->GetNumberOfConnectivityEntries();
    polySize += (*itr).Input->GetPolys()->GetNumberOfConnectivityEntries();
    numVerts += (*itr).Input->GetVerts()->GetNumberOfCells();
    numLines += (*itr).Input->GetLines()->GetNumberOfCells();
    numPolys += (*itr).Input->GetPolys()->GetNumberOfCells();
    ++itr;
  }

  vtkIdType numOutCells = numVerts + numLines + numPolys;

  vtkCellData* outCellData = (*begin).Input->GetCellData();
  int numCellArrays = outCellData->GetNumberOfArrays();
  for (int i=0; i<numCellArrays; i++)
  {
    outCellData->GetArray(i)->Resize(numOutCells);
    outCellData->GetArray(i)->SetNumberOfTuples(numOutCells);
  }

  // Now merge each cell type. Because vtkPolyData stores each
  // cell type separately, we need to merge them separately.

  if (vertSize > 0)
  {
    vtkNew<vtkCellArray> outVerts;
    outVerts->Allocate(vertSize);

    itr = begin;
    while(itr != end)
    {
    mcData.push_back(vtkMergeCellsData((*itr).Input, (*itr).VertOffsets, (*itr).Input->GetVerts()));
    ++itr;
    }
    MergeCells(mcData, idMaps, numVerts, 0, outVerts.GetPointer());

    outPolyData->SetVerts(outVerts.GetPointer());

    mcData.clear();
  }

  if (lineSize > 0)
  {
    vtkNew<vtkCellArray> outLines;
    outLines->Allocate(lineSize);

    itr = begin;
    while(itr != end)
    {
    mcData.push_back(vtkMergeCellsData((*itr).Input, (*itr).LineOffsets, (*itr).Input->GetLines()));
    ++itr;
    }
    MergeCells(mcData, idMaps, numLines, vertSize, outLines.GetPointer());

    outPolyData->SetLines(outLines.GetPointer());

    mcData.clear();
  }

  if (polySize > 0)
  {
    vtkNew<vtkCellArray> outPolys;
    outPolys->Allocate(polySize);

    itr = begin;
    while(itr != end)
    {
      mcData.push_back(vtkMergeCellsData((*itr).Input, (*itr).PolyOffsets, (*itr).Input->GetPolys()));
      ++itr;
    }
    MergeCells(mcData, idMaps, numPolys, vertSize + lineSize, outPolys.GetPointer());

    outPolyData->SetPolys(outPolys.GetPointer());
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
