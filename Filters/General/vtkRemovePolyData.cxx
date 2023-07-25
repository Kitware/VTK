// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkRemovePolyData.h"

#include "vtkAlgorithmOutput.h"
#include "vtkArrayDispatch.h"
#include "vtkArrayListTemplate.h" // For processing attribute data
#include "vtkCellArray.h"
#include "vtkCellArrayIterator.h"
#include "vtkCellData.h"
#include "vtkDataArrayRange.h"
#include "vtkDataSetAttributes.h"
#include "vtkIdList.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkSMPThreadLocalObject.h"
#include "vtkSMPTools.h"
#include "vtkStaticCellLinksTemplate.h"
#include "vtkStreamingDemandDrivenPipeline.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkRemovePolyData);
vtkCxxSetObjectMacro(vtkRemovePolyData, CellIds, vtkIdTypeArray);
vtkCxxSetObjectMacro(vtkRemovePolyData, PointIds, vtkIdTypeArray);

//------------------------------------------------------------------------------
vtkRemovePolyData::vtkRemovePolyData()
{
  this->CellIds = nullptr;
  this->PointIds = nullptr;
  this->ExactMatch = false;
}

//------------------------------------------------------------------------------
vtkRemovePolyData::~vtkRemovePolyData()
{
  this->SetCellIds(nullptr);
  this->SetPointIds(nullptr);
}

//------------------------------------------------------------------------------
// Remove a dataset from the list of data to process.
void vtkRemovePolyData::RemoveInputData(vtkPolyData* ds)
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
// The core threaded algorithms follow.
namespace
{
// Map input cells to output cells. This is a global map across all verts,
// lines, polys, and strips.
typedef std::vector<vtkIdType> CellMapType;

// Mark cells for deletion which are connected to point ids
template <typename TIds>
struct MarkPointIds
{
  vtkIdType* PtIds;
  vtkStaticCellLinksTemplate<TIds>* Links;
  CellMapType* CellMap;
  vtkRemovePolyData* Filter;

  MarkPointIds(vtkIdType* ptIds, vtkStaticCellLinksTemplate<TIds>* links, CellMapType* cellMap,
    vtkRemovePolyData* filter)
    : PtIds(ptIds)
    , Links(links)
    , CellMap(cellMap)
    , Filter(filter)
  {
  }

  void operator()(vtkIdType idx, vtkIdType endIdx)
  {
    bool isFirst = vtkSMPTools::GetSingleThread();
    for (; idx < endIdx; ++idx)
    {
      if (isFirst)
      {
        this->Filter->CheckAbort();
      }
      if (this->Filter->GetAbortOutput())
      {
        break;
      }
      vtkIdType ptId = this->PtIds[idx];
      vtkIdType ncells = this->Links->GetNcells(ptId);
      auto cells = this->Links->GetCells(ptId);
      for (auto i = 0; i < ncells; ++i)
      {
        (*this->CellMap)[cells[i]] = (-1);
      }
    }
  }

  static void Execute(vtkIdTypeArray* ptIds, vtkStaticCellLinksTemplate<TIds>* links,
    CellMapType* cellMap, vtkRemovePolyData* filter)
  {
    if (ptIds == nullptr)
    {
      return;
    }

    vtkIdType numPtIds = ptIds->GetNumberOfTuples();
    MarkPointIds<TIds> markPtIds(ptIds->GetPointer(0), links, cellMap, filter);
    vtkSMPTools::For(0, numPtIds, markPtIds);
  }
};

// This function is used to mark cells which are be deleted from a cell array,
// using an input list of cell connectivities.
template <typename TIds>
struct MarkCells
{
  vtkPolyData* Input;
  vtkCellArray* Cells;
  vtkStaticCellLinksTemplate<TIds>* Links;
  vtkCellArray* RemoveCells;
  vtkIdType CellIdOffset;
  vtkTypeBool ExactMatch;
  CellMapType* CellMap;
  vtkIdType NumCells;
  vtkIdType ConnSize;

  // These are local object for supporting thread-specific operations
  vtkSMPThreadLocal<vtkSmartPointer<vtkIdList>> LinkedCells;
  vtkSMPThreadLocal<vtkSmartPointer<vtkCellArrayIterator>> CellIterator;
  vtkSMPThreadLocal<vtkSmartPointer<vtkCellArrayIterator>> RCellIterator;
  vtkRemovePolyData* Filter;

  MarkCells(vtkPolyData* input, vtkCellArray* cellArray, vtkStaticCellLinksTemplate<TIds>* links,
    vtkCellArray* rCellArray, vtkIdType offset, vtkTypeBool exactMatch, CellMapType* cellMap,
    vtkRemovePolyData* filter)
    : Input(input)
    , Cells(cellArray)
    , Links(links)
    , RemoveCells(rCellArray)
    , CellIdOffset(offset)
    , ExactMatch(exactMatch)
    , CellMap(cellMap)
    , Filter(filter)
  {
  }

  void Initialize()
  {
    this->LinkedCells.Local() = vtkSmartPointer<vtkIdList>::New();
    this->CellIterator.Local().TakeReference(this->Cells->NewIterator());
    this->RCellIterator.Local().TakeReference(this->RemoveCells->NewIterator());
  }

  // Loop over a batch of cells, and mark those that are to be deleted.
  void operator()(vtkIdType cellId, vtkIdType endCellId)
  {
    vtkIdType npts, cId;
    const vtkIdType* pts;
    vtkCellArrayIterator* cellIter = this->CellIterator.Local();
    vtkCellArrayIterator* rCellIter = this->RCellIterator.Local();
    vtkIdList* linkedCells = this->LinkedCells.Local();
    bool isFirst = vtkSMPTools::GetSingleThread();

    for (; cellId < endCellId; ++cellId)
    {
      if (isFirst)
      {
        this->Filter->CheckAbort();
      }
      if (this->Filter->GetAbortOutput())
      {
        break;
      }
      rCellIter->GetCellAtId(cellId, npts, pts);
      this->Links->GetCells(npts, pts, linkedCells);
      auto numLinkedCells = linkedCells->GetNumberOfIds();
      for (auto i = 0; i < numLinkedCells; ++i)
      {
        cId = linkedCells->GetId(i);
        if (!this->ExactMatch) // any match will do
        {
          (*this->CellMap)[cId + this->CellIdOffset] = -1;
        }
        else
        {
          vtkIdType nMatchPts;
          const vtkIdType* matchPts;
          cellIter->GetCellAtId(cId, nMatchPts, matchPts);
          if (nMatchPts == npts)
          {
            (*this->CellMap)[cId + this->CellIdOffset] = -1;
          }
        }
      }
    }
  }

  void Reduce() {}

  static void Execute(vtkPolyData* input, vtkCellArray* cellArray,
    vtkStaticCellLinksTemplate<TIds>* links, vtkIdType numRCells, vtkCellArray* rCellArray,
    vtkIdType offset, vtkTypeBool exactMatch, CellMapType* cellMap, vtkRemovePolyData* filter)
  {
    MarkCells<TIds> markCells(
      input, cellArray, links, rCellArray, offset, exactMatch, cellMap, filter);
    vtkSMPTools::For(0, numRCells, markCells);
  }
};

// Process polydata and its associated cell arrays to mark the cells to be
// deleted. We use the cell links structure to quickly find the cells
// specified for removal.
template <typename TIds>
struct MarkDeletedCells
{
  void operator()(vtkPolyData* input, vtkIdType inOffsets[5], vtkIdTypeArray* cellIds,
    vtkIdTypeArray* ptIds, int numInputs, vtkInformationVector** inputVector, bool exactMatch,
    CellMapType* cellMap, vtkRemovePolyData* filter)
  {
    vtkIdType numPts = input->GetNumberOfPoints();
    vtkIdType* cellIdsPtr = (cellIds != nullptr ? cellIds->GetPointer(0) : nullptr);

    // If cell ids are provided, mark these cells for deletion
    if (cellIdsPtr != nullptr)
    {
      vtkIdType numCellIds = cellIds->GetNumberOfTuples();
      vtkSMPTools::For(0, numCellIds, [&, cellIdsPtr, cellMap](vtkIdType idx, vtkIdType endIdx) {
        for (; idx < endIdx; ++idx)
        {
          (*cellMap)[cellIdsPtr[idx]] = (-1);
        }
      }); // end lambda
    }

    // Now process any additional polydata inputs, as well is point
    // ids. Both require the building of cell links for performance.
    vtkCellArray* cells;
    vtkIdType nCells;

    vtkCellArray* inVerts = input->GetVerts();
    vtkIdType numInVerts = inVerts->GetNumberOfCells();
    vtkCellArray* inLines = input->GetLines();
    vtkIdType numInLines = inLines->GetNumberOfCells();
    vtkCellArray* inPolys = input->GetPolys();
    vtkIdType numInPolys = inPolys->GetNumberOfCells();
    vtkCellArray* inStrips = input->GetStrips();
    vtkIdType numInStrips = inStrips->GetNumberOfCells();

    // Construct the offsets
    inOffsets[0] = 0;
    inOffsets[1] = inOffsets[0] + numInVerts;
    inOffsets[2] = inOffsets[1] + numInLines;
    inOffsets[3] = inOffsets[2] + numInPolys;
    inOffsets[4] = inOffsets[3] + numInStrips;

    // Mark cells to be deleted in each of the four cell arrays
    // of vtkPolyData. Cell links are built if necessary, and
    // are used to quickly identify the cells to delete.
    if (numInVerts > 0)
    {
      vtkStaticCellLinksTemplate<TIds> links;
      links.ThreadedBuildLinks(numPts, numInVerts, inVerts);
      MarkPointIds<TIds>::Execute(ptIds, &links, cellMap, filter);
      for (auto i = 1; i < numInputs; ++i)
      {
        cells = vtkPolyData::GetData(inputVector[0], i)->GetVerts();
        if ((nCells = cells->GetNumberOfCells()) > 0)
        {
          MarkCells<TIds>::Execute(
            input, inVerts, &links, nCells, cells, inOffsets[0], exactMatch, cellMap, filter);
        }
      }
    }

    if (numInLines > 0)
    {
      vtkStaticCellLinksTemplate<TIds> links;
      links.ThreadedBuildLinks(numPts, numInLines, inLines);
      MarkPointIds<TIds>::Execute(ptIds, &links, cellMap, filter);
      for (auto i = 1; i < numInputs; ++i)
      {
        cells = vtkPolyData::GetData(inputVector[0], i)->GetLines();
        if ((nCells = cells->GetNumberOfCells()) > 0)
        {
          MarkCells<TIds>::Execute(
            input, inLines, &links, nCells, cells, inOffsets[1], exactMatch, cellMap, filter);
        }
      }
    }

    if (numInPolys > 0)
    {
      vtkStaticCellLinksTemplate<TIds> links;
      links.ThreadedBuildLinks(numPts, numInPolys, inPolys);
      MarkPointIds<TIds>::Execute(ptIds, &links, cellMap, filter);
      for (auto i = 1; i < numInputs; ++i)
      {
        cells = vtkPolyData::GetData(inputVector[0], i)->GetPolys();
        if ((nCells = cells->GetNumberOfCells()) > 0)
        {
          MarkCells<TIds>::Execute(
            input, inPolys, &links, nCells, cells, inOffsets[2], exactMatch, cellMap, filter);
        }
      }
    }

    if (numInStrips > 0)
    {
      vtkStaticCellLinksTemplate<TIds> links;
      links.ThreadedBuildLinks(numPts, numInStrips, inStrips);
      MarkPointIds<TIds>::Execute(ptIds, &links, cellMap, filter);
      for (auto i = 1; i < numInputs; ++i)
      {
        cells = vtkPolyData::GetData(inputVector[0], i)->GetStrips();
        if ((nCells = cells->GetNumberOfCells()) > 0)
        {
          MarkCells<TIds>::Execute(
            input, inStrips, &links, nCells, cells, inOffsets[3], exactMatch, cellMap, filter);
        }
      }
    }

    // Assign output cell ids.
    vtkIdType outCellId = 0;
    for (auto cellId = 0; cellId < inOffsets[4]; ++cellId)
    {
      if ((*cellMap)[cellId] > 0) // cell has not been deleted
      {
        (*cellMap)[cellId] = outCellId++;
      }
    }

  } // operator()
};  // MarkDeletedCells

// Determine the information required to build output cell arrays,
// including the number of remaining cells in a cell array, and the
// size of the connectivity array.
struct CountCells
{
  vtkCellArray* CellArray;
  CellMapType* CellMap;
  vtkIdType CellIdOffset;
  vtkIdType NumCells;
  vtkIdType ConnSize;

  vtkSMPThreadLocal<vtkIdType> LocalNumCells;
  vtkSMPThreadLocal<vtkIdType> LocalConnSize;

  vtkSMPThreadLocal<vtkSmartPointer<vtkCellArrayIterator>> CellIterator;
  vtkRemovePolyData* Filter;

  CountCells(
    vtkCellArray* cellArray, CellMapType* cellMap, vtkIdType offset, vtkRemovePolyData* filter)
    : CellArray(cellArray)
    , CellMap(cellMap)
    , CellIdOffset(offset)
    , NumCells(0)
    , ConnSize(0)
    , Filter(filter)
  {
  }

  void Initialize()
  {
    this->LocalNumCells.Local() = 0;
    this->LocalConnSize.Local() = 0;
    this->CellIterator.Local().TakeReference(this->CellArray->NewIterator());
  }

  void operator()(vtkIdType cellId, vtkIdType endCellId)
  {
    CellMapType* cellMap = this->CellMap;
    vtkCellArrayIterator* cellIter = this->CellIterator.Local();
    vtkIdType& numCells = this->LocalNumCells.Local();
    vtkIdType& connSize = this->LocalConnSize.Local();
    vtkIdType npts;
    const vtkIdType* pts;
    bool isFirst = vtkSMPTools::GetSingleThread();

    for (; cellId < endCellId; ++cellId)
    {
      if (isFirst)
      {
        this->Filter->CheckAbort();
      }
      if (this->Filter->GetAbortOutput())
      {
        break;
      }
      vtkIdType offsetId = cellId + this->CellIdOffset;
      if ((*cellMap)[offsetId] >= 0)
      {
        ++numCells;
        cellIter->GetCellAtId(cellId, npts, pts);
        connSize += npts;
      }
    }
  }

  void Reduce()
  {
    auto lEnd = this->LocalNumCells.end();
    for (auto lItr = this->LocalNumCells.begin(); lItr != lEnd; ++lItr)
    {
      this->NumCells += *lItr;
    }
    auto lcsEnd = this->LocalConnSize.end();
    for (auto lItr = this->LocalConnSize.begin(); lItr != lcsEnd; ++lItr)
    {
      this->ConnSize += *lItr;
    }
  }

  static void Execute(vtkCellArray* ca, CellMapType* cellMap, vtkIdType offset,
    vtkIdType& numOutCells, vtkIdType& connSize, vtkRemovePolyData* filter)
  {
    CountCells countCells(ca, cellMap, offset, filter);
    vtkSMPTools::For(0, ca->GetNumberOfCells(), countCells);
    numOutCells = countCells.NumCells;
    connSize = countCells.ConnSize;
  }
}; // CountCells

// Count the number of cells, and determine connectivity size, in
// preparation for allocating and configuring output. Basically
// determine what's left after marking cells for deletion.
struct MapOutput
{
  void operator()(vtkPolyData* input, vtkIdType inOffsets[5], CellMapType* cellMap,
    vtkIdType outOffsets[5], vtkIdType connSizes[4], vtkRemovePolyData* filter)
  {
    vtkIdType numInVerts = inOffsets[1] - inOffsets[0];
    vtkIdType numInLines = inOffsets[2] - inOffsets[1];
    vtkIdType numInPolys = inOffsets[3] - inOffsets[2];
    vtkIdType numInStrips = inOffsets[4] - inOffsets[3];
    vtkIdType numOutCells = 0;
    connSizes[0] = connSizes[1] = connSizes[2] = connSizes[3] = 0;

    // Verts
    outOffsets[0] = 0;
    if (numInVerts > 0)
    {
      vtkCellArray* inVerts = input->GetVerts();
      CountCells::Execute(inVerts, cellMap, inOffsets[0], numOutCells, connSizes[0], filter);
    }
    outOffsets[1] = outOffsets[0] + numOutCells;

    // Lines
    numOutCells = 0;
    if (numInLines > 0)
    {
      vtkCellArray* inLines = input->GetLines();
      CountCells::Execute(inLines, cellMap, inOffsets[1], numOutCells, connSizes[1], filter);
    }
    outOffsets[2] = outOffsets[1] + numOutCells;

    // Polys
    numOutCells = 0;
    if (numInPolys > 0)
    {
      vtkCellArray* inPolys = input->GetPolys();
      CountCells::Execute(inPolys, cellMap, inOffsets[2], numOutCells, connSizes[2], filter);
    }
    outOffsets[3] = outOffsets[2] + numOutCells;

    // Strips
    numOutCells = 0;
    if (numInStrips > 0)
    {
      vtkCellArray* inStrips = input->GetStrips();
      CountCells::Execute(inStrips, cellMap, inOffsets[3], numOutCells, connSizes[3], filter);
    }
    outOffsets[4] = outOffsets[3] + numOutCells;
  }
};

// Build an output offset array for a cell array.
struct BuildOffsets
{
  CellMapType* CellMap;
  vtkIdType InCellsIdOffset;
  vtkIdType OutCellsIdOffset;
  vtkCellArray* InArray;
  vtkIdType NumCells;
  vtkIdType ConnSize;
  vtkIdType* Offsets;

  vtkSMPThreadLocal<vtkSmartPointer<vtkCellArrayIterator>> CellIterator;
  vtkRemovePolyData* Filter;

  BuildOffsets(CellMapType* cellMap, vtkIdType inCellOffset, vtkIdType outCellOffset,
    vtkCellArray* inArray, vtkIdType numOutCells, vtkIdType connSize, vtkIdType* offsets,
    vtkRemovePolyData* filter)
    : CellMap(cellMap)
    , InCellsIdOffset(inCellOffset)
    , OutCellsIdOffset(outCellOffset)
    , InArray(inArray)
    , NumCells(numOutCells)
    , ConnSize(connSize)
    , Offsets(offsets)
    , Filter(filter)
  {
  }

  void Initialize() { this->CellIterator.Local().TakeReference(this->InArray->NewIterator()); }

  void operator()(vtkIdType cellId, vtkIdType endCellId)
  {
    vtkCellArrayIterator* cellIter = this->CellIterator.Local();
    vtkIdType npts;
    const vtkIdType* pts;
    bool isFirst = vtkSMPTools::GetSingleThread();

    for (; cellId < endCellId; ++cellId)
    {
      if (isFirst)
      {
        this->Filter->CheckAbort();
      }
      if (this->Filter->GetAbortOutput())
      {
        break;
      }
      vtkIdType inCellId = cellId + this->InCellsIdOffset;
      vtkIdType outCellId = (*this->CellMap)[inCellId] - this->OutCellsIdOffset;
      if (outCellId >= 0)
      {
        cellIter->GetCellAtId(cellId, npts, pts);
        *(this->Offsets + outCellId) = npts;
      }
    }
  }

  void Reduce()
  {
    // Prefix sum to roll up the offsets
    vtkIdType offset = 0, npts;
    for (auto cellId = 0; cellId < this->NumCells; ++cellId)
    {
      npts = this->Offsets[cellId];
      this->Offsets[cellId] = offset;
      offset += npts;
    }
    this->Offsets[this->NumCells] = this->ConnSize;
  }
};

// Build an output connectivity array for a cell array. Also copies cell data
// from input to output.
struct BuildConnectivity
{
  CellMapType* CellMap;
  vtkIdType InCellsIdOffset;
  vtkIdType OutCellsIdOffset;
  vtkCellArray* InArray;
  vtkIdType* Offsets;
  vtkIdType* Conn;
  ArrayList* Arrays;

  vtkSMPThreadLocal<vtkSmartPointer<vtkCellArrayIterator>> CellIterator;
  vtkRemovePolyData* Filter;

  BuildConnectivity(CellMapType* cellMap, vtkIdType inCellsIdOffset, vtkIdType outCellsIdOffset,
    vtkCellArray* inArray, vtkIdType* offsets, vtkIdType* conn, ArrayList* arrays,
    vtkRemovePolyData* filter)
    : CellMap(cellMap)
    , InCellsIdOffset(inCellsIdOffset)
    , OutCellsIdOffset(outCellsIdOffset)
    , InArray(inArray)
    , Offsets(offsets)
    , Conn(conn)
    , Arrays(arrays)
    , Filter(filter)
  {
  }

  void Initialize() { this->CellIterator.Local().TakeReference(this->InArray->NewIterator()); }

  void operator()(vtkIdType cellId, vtkIdType endCellId)
  {
    vtkCellArrayIterator* cellIter = this->CellIterator.Local();
    vtkIdType* connPtr;
    vtkIdType npts;
    const vtkIdType* pts;
    bool isFirst = vtkSMPTools::GetSingleThread();

    for (; cellId < endCellId; ++cellId)
    {
      if (isFirst)
      {
        this->Filter->CheckAbort();
      }
      if (this->Filter->GetAbortOutput())
      {
        break;
      }
      vtkIdType inCellId = cellId + this->InCellsIdOffset;
      vtkIdType outCellId = (*this->CellMap)[inCellId];
      if (outCellId >= 0)
      {
        cellIter->GetCellAtId(cellId, npts, pts);
        connPtr = this->Conn + *(this->Offsets + (outCellId - this->OutCellsIdOffset));
        for (auto i = 0; i < npts; ++i)
        {
          *connPtr++ = pts[i];
        }
        this->Arrays->Copy(inCellId, outCellId);
      }
    }
  }

  void Reduce() {}
};

// Build the four cell arrays for the output vtkPolyData.
struct BuildCellArrays
{
  void operator()(CellMapType* cellMap, vtkPolyData* input, vtkIdType inOffsets[5],
    vtkPolyData* output, vtkIdType outOffsets[5], vtkIdType connSizes[4], vtkRemovePolyData* filter)
  {
    vtkIdType numInVerts = inOffsets[1] - inOffsets[0];
    vtkIdType numInLines = inOffsets[2] - inOffsets[1];
    vtkIdType numInPolys = inOffsets[3] - inOffsets[2];
    vtkIdType numInStrips = inOffsets[4] - inOffsets[3];
    vtkIdType numOutVerts = outOffsets[1] - outOffsets[0];
    vtkIdType numOutLines = outOffsets[2] - outOffsets[1];
    vtkIdType numOutPolys = outOffsets[3] - outOffsets[2];
    vtkIdType numOutStrips = outOffsets[4] - outOffsets[3];
    vtkIdType numOutCells = outOffsets[4];

    // Set up the copying of the cell data.
    vtkCellData* inCD = input->GetCellData();
    vtkCellData* outCD = output->GetCellData();
    outCD->CopyAllocate(inCD, numOutCells);
    ArrayList arrays;
    arrays.AddArrays(numOutCells, inCD, outCD);

    // Now build the polydata arrays
    // Verts
    if (numOutVerts > 0)
    {
      vtkCellArray* inVerts = input->GetVerts();
      vtkNew<vtkCellArray> outVerts;
      this->BuildArray(cellMap, numInVerts, inVerts, numOutVerts, outVerts, inOffsets[0],
        outOffsets[0], connSizes[0], &arrays, filter);
      output->SetVerts(outVerts);
    }

    // Lines
    if (numOutLines > 0)
    {
      vtkCellArray* inLines = input->GetLines();
      vtkNew<vtkCellArray> outLines;
      this->BuildArray(cellMap, numInLines, inLines, numOutLines, outLines, inOffsets[1],
        outOffsets[1], connSizes[1], &arrays, filter);
      output->SetLines(outLines);
    }

    // Polys
    if (numOutPolys > 0)
    {
      vtkCellArray* inPolys = input->GetPolys();
      vtkNew<vtkCellArray> outPolys;
      this->BuildArray(cellMap, numInPolys, inPolys, numOutPolys, outPolys, inOffsets[2],
        outOffsets[2], connSizes[2], &arrays, filter);
      output->SetPolys(outPolys);
    }

    // Strips
    if (numOutStrips > 0)
    {
      vtkCellArray* inStrips = input->GetStrips();
      vtkNew<vtkCellArray> outStrips;
      this->BuildArray(cellMap, numInStrips, inStrips, numOutStrips, outStrips, inOffsets[3],
        outOffsets[3], connSizes[3], &arrays, filter);
      output->SetStrips(outStrips);
    }
  }

  void BuildArray(CellMapType* cellMap, vtkIdType numInCells, vtkCellArray* inArray,
    vtkIdType numOutCells, vtkCellArray* outArray, vtkIdType inCellsIdOffset,
    vtkIdType outCellsIdOffset, vtkIdType connSize, ArrayList* arrays, vtkRemovePolyData* filter)
  {
    // Create the offset array, and populate it.
    vtkNew<vtkIdTypeArray> offsets;
    vtkIdType* offsetsPtr = offsets->WritePointer(0, numOutCells + 1);
    BuildOffsets buildOffsets(cellMap, inCellsIdOffset, outCellsIdOffset, inArray, numOutCells,
      connSize, offsetsPtr, filter);
    vtkSMPTools::For(0, numInCells, buildOffsets);

    // Now create the connectivity array and populate it.
    vtkNew<vtkIdTypeArray> conn;
    vtkIdType* connPtr = conn->WritePointer(0, connSize);
    BuildConnectivity buildConn(
      cellMap, inCellsIdOffset, outCellsIdOffset, inArray, offsetsPtr, connPtr, arrays, filter);
    vtkSMPTools::For(0, numInCells, buildConn);

    // Define the cell array
    outArray->SetData(offsets, conn);
  }
};

}; // anonymous

//------------------------------------------------------------------------------
// Remove cells from a polygonal data set.
int vtkRemovePolyData::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  int numInputs = inputVector[0]->GetNumberOfInformationObjects();
  vtkPolyData* input = vtkPolyData::GetData(inputVector[0], 0);
  vtkPolyData* output = vtkPolyData::GetData(outputVector, 0);

  // If there is only one input, and no deletion via point or cell ids,
  // then copy it through to the output.
  if (numInputs == 1 && this->CellIds == nullptr && this->PointIds == nullptr)
  {
    output->ShallowCopy(input);
    return 1;
  }

  // Okay we have some data to remove. Mark the cells to be deleted.
  // Make sure some cells are available.
  vtkIdType numPts = input->GetNumberOfPoints();
  vtkIdType numCells = input->GetNumberOfCells();
  if (numPts < 1 || numCells < 1)
  {
    return 1;
  }

  // The output points are the input points. This could be changed as a
  // filter option.
  output->SetPoints(input->GetPoints());
  output->GetPointData()->PassData(input->GetPointData());

  // The offsets are basically used to manage cell ids since vtkPolyData is
  // using four separate cell arrays which complicates things. The offsets
  // are used to keep track of the number of cells in the four lists
  // (verts,lines,polys,strips). We also keep track of the size of the
  // connectivity list for each of the four output cell arrays.
  vtkIdType inOffsets[5], outOffsets[5], connSizes[4];

  // Now determine the cells to be deleted. Initially, all cells are marked
  // as being retained - then as we process the inputs [1,numInputs) we mark
  // for deletion those cells that match the cells in input #0. We are going
  // to build some cell links, so we'll dispatch based on type of ids needed
  // to represent points and cells. The cellMap spans all cells contained in
  // the four separate cell arrays, so offsets into each array must be
  // maintained.
  CellMapType cellMap(numCells, 1);
  if (numPts < VTK_INT_MAX && numCells < VTK_INT_MAX)
  {
    MarkDeletedCells<int> markCells;
    markCells(input, inOffsets, this->CellIds, this->PointIds, numInputs, inputVector,
      this->ExactMatch, &cellMap, this);
  }
  else
  {
    MarkDeletedCells<vtkIdType> markCells;
    markCells(input, inOffsets, this->CellIds, this->PointIds, numInputs, inputVector,
      this->ExactMatch, &cellMap, this);
  }

  // Determine what remains after the deletion of cells, and produce a
  // mapping of input to output cells.
  MapOutput mapOutput;
  mapOutput(input, inOffsets, &cellMap, outOffsets, connSizes, this);

  // Build the output cell arrays
  BuildCellArrays buildCellArrays;
  buildCellArrays(&cellMap, input, inOffsets, output, outOffsets, connSizes, this);

  return 1;
}

//------------------------------------------------------------------------------
int vtkRemovePolyData::RequestUpdateExtent(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the output info object
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  int piece, numPieces, ghostLevel;
  int idx;

  piece = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  numPieces = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
  ghostLevel = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS());

  // make sure piece is valid
  if (piece < 0 || piece >= numPieces)
  {
    return 0;
  }

  int numInputs = this->GetNumberOfInputConnections(0);

  vtkInformation* inInfo;
  // just copy the Update extent as default behavior.
  for (idx = 0; idx < numInputs; ++idx)
  {
    inInfo = inputVector[0]->GetInformationObject(idx);
    if (inInfo)
    {
      inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), piece);
      inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(), numPieces);
      inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(), ghostLevel);
    }
  }
  // Let downstream request a subset of connection 0, for connections >= 1
  // send their WHOLE_EXTENT as UPDATE_EXTENT.
  for (idx = 1; idx < numInputs; ++idx)
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
vtkPolyData* vtkRemovePolyData::GetInput(int idx)
{
  return vtkPolyData::SafeDownCast(this->GetExecutive()->GetInputData(0, idx));
}

//------------------------------------------------------------------------------
int vtkRemovePolyData::FillInputPortInformation(int port, vtkInformation* info)
{
  if (!this->Superclass::FillInputPortInformation(port, info))
  {
    return 0;
  }
  info->Set(vtkAlgorithm::INPUT_IS_REPEATABLE(), 1);
  return 1;
}

//------------------------------------------------------------------------------
void vtkRemovePolyData::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << "Cell Ids: " << this->CellIds << endl;
  os << "Point Ids: " << this->PointIds << endl;
  os << "Exact Match: " << (this->ExactMatch ? "On" : "Off") << endl;
}
VTK_ABI_NAMESPACE_END
