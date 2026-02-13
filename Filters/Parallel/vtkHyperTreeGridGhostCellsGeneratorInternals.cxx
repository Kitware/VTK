// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkHyperTreeGridGhostCellsGeneratorInternals.h"

#include "vtkArrayDispatch.h"
#include "vtkCellData.h"
#include "vtkCommunicator.h"
#include "vtkCompositeArray.h"
#include "vtkDataArray.h"
#include "vtkHyperTreeGrid.h"
#include "vtkMath.h"
#include "vtkMultiProcessController.h"
#include "vtkSetGet.h"
#include "vtkSmartPointer.h"
#include "vtkUnsignedCharArray.h"

#include <cassert>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN
namespace
{

using CellDataArray = vtkHyperTreeGridGhostCellsGeneratorInternals::CellDataArray;
using CellDataAttributes = vtkHyperTreeGridGhostCellsGeneratorInternals::CellDataAttributes;

/**
 * build the output celldata with composite array for each input cell data
 */
struct AddIndexedArrayWorker
{
  template <typename ArrayType>
  void operator()(ArrayType* inputArray, CellDataArray& cdHandler, vtkCellData* outputCD) const
  {
    using ValueType = vtk::GetAPIType<ArrayType>;
    std::vector<vtkDataArray*> arrayList = { cdHandler.InternalArray, cdHandler.GhostCDBuffer };

    vtkSmartPointer<vtkCompositeArray<ValueType>> compositeArr =
      vtk::ConcatenateDataArrays<ValueType>(arrayList);
    compositeArr->SetName(inputArray->GetName());
    cdHandler.GhostCDBuffer->UnRegister(inputArray); // transfer ownership to composite

    // Replace existing array
    outputCD->AddArray(compositeArr);
  }
};

/**
 * Subroutine to compute the number of values attached to a single cell in the output HTG.
 */
int GetNumberOfCellValues(vtkCellData* cellData)
{
  int totalCellSize = 0;
  int nbArray = cellData->GetNumberOfArrays();
  for (int arrayId = 0; arrayId < nbArray; arrayId++)
  {
    vtkDataArray* outArray = cellData->GetArray(arrayId);
    totalCellSize += outArray->GetNumberOfComponents();
  }
  return totalCellSize;
}

/**
 * Creates a ghost tree in the output. It is built in mirror with
 * vtkHyperTreeGridGhostCellsGenerator::ExtractInterface.
 *
 * @param outCursor Cursor on the output tree that will create the hyper tree.
 * @param isParent Input vtkBitArray produced by a neighbor process to tell if the current node is
 * a leaf or not.
 * @param isMasked Optional input vtkBitArray produced by a neighbor process to tell if the current
 * node is a masked or not.
 * @param indices Output array mapping the created nodes to their position in the output data
 * arrays.
 * @param pos Parameter which should be left untouched, it is used to keep track of the number of
 * inserted data.
 */
vtkIdType CreateGhostTree(vtkHyperTreeGridNonOrientedCursor* outCursor, vtkBitArray* isParent,
  vtkBitArray* isMasked, vtkBitArray* outputMask, vtkIdType* indices, vtkIdType& pos)
{
  indices[pos] = outCursor->GetGlobalNodeIndex();

  if (outputMask)
  {
    outputMask->InsertValue(indices[pos], isMasked->GetValue(pos));
    if (isMasked->GetValue(pos))
    {
      pos++;
      return pos;
    }
  }
  if (isParent->GetValue(pos++))
  {
    outCursor->SubdivideLeaf();
    for (int ichild = 0; ichild < outCursor->GetNumberOfChildren(); ++ichild)
    {
      outCursor->ToChild(ichild);
      ::CreateGhostTree(outCursor, isParent, isMasked, outputMask, indices, pos);
      outCursor->ToParent();
    }
  }
  return pos;
}

/**
 * Reads the input interface with neighbor processes.
 * This method is built in mirror with vtkHyperTreeGridGhostCellsGenerator::CreateGhostTree
 *
 * @param inCursor Cursor on the current tree to read from the input
 * @param isParent A bit array being produced by this filter,
 * telling if the corresponding node is parent or not. A node is
 * a parent if it is not a leaf. The map of the tracking is stored in indices.
 * For example, if the data array of the input is called inArray,
 * isParent->GetValue(m) equals one if inArray->GetTuple1(indices[m]) is not a leaf.
 * @param isMasked A bit array filed by this filter. isMasked->GetValue(m) is set to 1 if the
 * corresponding cell is masked, and 0 otherwise.
 * @param indices An array produced by this filter mapping the nodes of the interface with their
 * location in the input data array.
 * @param grid Input vtkHyperTreeGrid used to have the neighborhood profile. This neighborhood
 * profile is tested with the mask parameter to know whether to descend or not in the current
 * hyper tree.
 * @param mask Input parameter which should be shaped as vtkHyperTreeGrid::GetChildMask() of the
 * input. This parameter is used to only descend on the interface with the other processes.
 * @param pos This parameter will be equal to the number of nodes in the hyper tree to send to the
 * other processes.
 */
void ExtractInterface(vtkHyperTreeGridNonOrientedCursor* inCursor, vtkBitArray* isParent,
  vtkBitArray* isMasked, std::vector<vtkIdType>& indices, vtkHyperTreeGrid* grid, unsigned int mask,
  vtkIdType& pos)
{
  isParent->InsertTuple1(pos, !inCursor->IsLeaf());
  isMasked->InsertTuple1(pos, inCursor->IsMasked());
  indices[pos++] = inCursor->GetGlobalNodeIndex();

  if (!inCursor->IsLeaf() && !inCursor->IsMasked())
  {
    for (int ichild = 0; ichild < inCursor->GetNumberOfChildren(); ++ichild)
    {
      inCursor->ToChild(ichild);
      unsigned int newMask = mask & grid->GetChildMask(ichild);
      if (newMask)
      {
        ::ExtractInterface(inCursor, isParent, isMasked, indices, grid, newMask, pos);
      }
      else
      {
        isParent->InsertTuple1(pos, 0);
        isMasked->InsertTuple1(pos, inCursor->IsMasked());
        indices[pos++] = inCursor->GetGlobalNodeIndex();
      }
      inCursor->ToParent();
    }
  }
}
}

//------------------------------------------------------------------------------
vtkHyperTreeGridGhostCellsGeneratorInternals::vtkHyperTreeGridGhostCellsGeneratorInternals(
  vtkMultiProcessController* controller, vtkHyperTreeGrid* inputHTG, vtkHyperTreeGrid* outputHTG)
  : Controller(controller)
  , InputHTG(inputHTG)
  , OutputHTG(outputHTG)
{
  unsigned int cellDims[3];
  this->InputHTG->GetCellDims(cellDims);
  this->HyperTreesMapToProcesses.resize(cellDims[0] * cellDims[1] * cellDims[2]);
  this->NumberOfVertices = inputHTG->GetNumberOfElements(vtkHyperTreeGrid::CELL);
  this->InitialNumberOfVertices = this->NumberOfVertices;

  if (inputHTG->HasMask())
  {
    this->OutputMask.TakeReference(vtkBitArray::New());
    this->OutputMask->DeepCopy(inputHTG->GetMask());
  }
  outputHTG->ShallowCopy(inputHTG);
  outputHTG->SetMask(nullptr); // externally handled
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridGhostCellsGeneratorInternals::InitializeCellData()
{
  int nbArrays = this->InputHTG->GetCellData()->GetNumberOfArrays();
  auto nbCells = this->InputHTG->GetNumberOfCells();
  // estimate boundary size from # cells:
  // in 2D: root square of nb cells
  // in 3D: pow (2/3)
  vtkIdType alloc =
    std::pow(nbCells, (this->InputHTG->GetDimension() - 1.0) / this->InputHTG->GetDimension());

  for (int iA = 0; iA < nbArrays; iA++)
  {
    auto da = vtkDataArray::SafeDownCast(this->InputHTG->GetCellData()->GetAbstractArray(iA));
    if (!da || !da->GetName()) // Name are required here
    {
      continue;
    }
    if (this->ImplicitCD.count(da->GetName()) == 0)
    {
      CellDataArray cdHandler;
      cdHandler.InternalArray = da;
      cdHandler.GhostCDBuffer = da->NewInstance();
      cdHandler.GhostCDBuffer->SetNumberOfComponents(da->GetNumberOfComponents());
      cdHandler.GhostCDBuffer->SetNumberOfTuples(0);
      cdHandler.GhostCDBuffer->Allocate(alloc);
      this->ImplicitCD.emplace(da->GetName(), cdHandler);
    }
  }

  // Also set the structure to the output cell data for later use
  this->OutputHTG->GetCellData()->CopyStructure(this->InputHTG->GetCellData());
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridGhostCellsGeneratorInternals::BroadcastTreeLocations()
{
  unsigned cellDims[3];
  this->InputHTG->GetCellDims(cellDims);
  vtkIdType nbHTs = cellDims[0] * cellDims[1] * cellDims[2];

  int processId = this->Controller->GetLocalProcessId();

  std::vector<int> broadcastHyperTreesMapToProcesses(nbHTs, -1);

  vtkNew<vtkHyperTreeGridNonOrientedCursor> inCursor;
  vtkHyperTreeGrid::vtkHyperTreeGridIterator inputIterator;
  vtkIdType inTreeIndex = 0;
  this->InputHTG->InitializeTreeIterator(inputIterator);
  while (inputIterator.GetNextTree(inTreeIndex))
  {
    this->InputHTG->InitializeNonOrientedCursor(inCursor, inTreeIndex);
    if (inCursor->HasTree())
    {
      broadcastHyperTreesMapToProcesses[inTreeIndex] = processId;
    }
  }
  this->Controller->AllReduce(broadcastHyperTreesMapToProcesses.data(),
    this->HyperTreesMapToProcesses.data(), nbHTs, vtkCommunicator::MAX_OP);

  assert(this->InputHTG->GetDimension() > 1);
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridGhostCellsGeneratorInternals::DetermineNeighbors()
{
  unsigned cellDims[3];
  this->InputHTG->GetCellDims(cellDims);
  vtkNew<vtkHyperTreeGridOrientedCursor> inOrientedCursor;
  vtkHyperTreeGrid::vtkHyperTreeGridIterator inputIterator;
  vtkIdType inTreeIndex = 0;
  unsigned int i, j, k = 0;
  int thisProcessId = this->Controller->GetLocalProcessId();
  this->InputHTG->InitializeTreeIterator(inputIterator);
  switch (this->InputHTG->GetDimension())
  {
    case 2:
    {
      while (inputIterator.GetNextTree(inTreeIndex))
      {
        this->InputHTG->InitializeOrientedCursor(inOrientedCursor, inTreeIndex);
        this->InputHTG->GetLevelZeroCoordinatesFromIndex(inTreeIndex, i, j, k);
        // Avoiding over / under flowing the grid
        for (int rj = ((j > 0) ? -1 : 0); rj < (((j + 1) < cellDims[1]) ? 2 : 1); ++rj)
        {
          for (int ri = ((i > 0) ? -1 : 0); ri < (((i + 1) < cellDims[0]) ? 2 : 1); ++ri)
          {
            vtkIdType neighTreeId = -1;
            this->InputHTG->GetIndexFromLevelZeroCoordinates(neighTreeId, i + ri, j + rj, 0);
            int neighProcessId = this->HyperTreesMapToProcesses[neighTreeId];
            if (neighProcessId >= 0 && neighProcessId != thisProcessId)
            {
              // Build a neighborhood mask to extract the interface in
              // ExtractInterface later on.
              // Same encoding as vtkHyperTreeGrid::GetChildMask
              this->SendBuffer[neighProcessId][inTreeIndex].mask |= 1
                << (8 * sizeof(int) - 1 - (ri + 1 + (rj + 1) * 3));
              // Not receiving anything from this guy since we will send him stuff
              this->RecvBuffer[neighProcessId][neighTreeId].count = 0;
              // Process not treated yet, yielding the flag
              this->Flags[neighProcessId] = NOT_TREATED;
            }
          }
        }
      }
      break;
    }
    case 3:
    {
      while (inputIterator.GetNextTree(inTreeIndex))
      {
        this->InputHTG->InitializeOrientedCursor(inOrientedCursor, inTreeIndex);
        this->InputHTG->GetLevelZeroCoordinatesFromIndex(inTreeIndex, i, j, k);
        // Avoiding over / under flowing the grid
        for (int rk = ((k > 0) ? -1 : 0); rk < (((k + 1) < cellDims[2]) ? 2 : 1); ++rk)
        {
          for (int rj = ((j > 0) ? -1 : 0); rj < (((j + 1) < cellDims[1]) ? 2 : 1); ++rj)
          {
            for (int ri = ((i > 0) ? -1 : 0); ri < (((i + 1) < cellDims[0]) ? 2 : 1); ++ri)
            {
              vtkIdType neighbor = -1;
              this->InputHTG->GetIndexFromLevelZeroCoordinates(neighbor, i + ri, j + rj, k + rk);
              int id = this->HyperTreesMapToProcesses[neighbor];
              if (id >= 0 && id != thisProcessId)
              {
                // Build a neighborhood mask to extract the interface in
                // ExtractInterface later on.
                // Same encoding as vtkHyperTreeGrid::GetChildMask
                this->SendBuffer[id][inTreeIndex].mask |= 1
                  << (8 * sizeof(int) - 1 - (ri + 1 + (rj + 1) * 3 + (rk + 1) * 9));
                // Not receiving anything from this guy since we will send him stuff
                this->RecvBuffer[id][neighbor].count = 0;
                // Process not treated yet, yielding the flag
                this->Flags[id] = NOT_TREATED;
              }
            }
          }
        }
      }
      break;
    }
  }
}

//------------------------------------------------------------------------------
int vtkHyperTreeGridGhostCellsGeneratorInternals::ExchangeSizes()
{
  int numberOfProcesses = this->Controller->GetNumberOfProcesses();

  std::vector<vtkIdType> treeSizeSendData;
  std::vector<int> treeSizeSendCount(numberOfProcesses, 0);
  std::vector<int> treeSizeSendOffset(numberOfProcesses, 0);

  std::vector<vtkIdType> treeSizeRecvData;
  std::vector<int> treeSizeRecvCount(numberOfProcesses, 0);
  std::vector<int> treeSizeRecvOffset(numberOfProcesses, 0);

  int currentDataBufferOffset = 0;
  for (int id = 0; id < numberOfProcesses; ++id)
  {
    if (id >= 1)
    {
      treeSizeSendOffset[id] = treeSizeSendOffset[id - 1] + treeSizeSendCount[id - 1];
    }
    auto sendIt = this->SendBuffer.find(id);
    if (sendIt != this->SendBuffer.end())
    {
      SendTreeBufferMap& sendTreeMap = sendIt->second;
      treeSizeSendCount[id] = static_cast<int>(sendTreeMap.size());

      treeSizeSendData.resize(treeSizeSendOffset[id] + treeSizeSendCount[id]);

      vtkNew<vtkHyperTreeGridNonOrientedCursor> inCursor;
      for (auto&& sendTreeBufferPair : sendTreeMap)
      {
        vtkIdType treeId = sendTreeBufferPair.first;
        auto&& sendTreeBuffer = sendTreeBufferPair.second;
        this->InputHTG->InitializeNonOrientedCursor(inCursor, treeId);
        // Extracting the tree interface with its neighbors
        sendTreeBuffer.count = 0;
        vtkHyperTree* tree = inCursor->GetTree();
        if (tree)
        {
          // We store the isParent profile along the interface to know when to subdivide later
          // indices store the indices in the input of the nodes on the interface
          vtkIdType nbVertices = tree->GetNumberOfVertices();
          sendTreeBuffer.indices.resize(nbVertices);
          ::ExtractInterface(inCursor, sendTreeBuffer.isParent, sendTreeBuffer.isMasked,
            sendTreeBuffer.indices, this->InputHTG, sendTreeBuffer.mask, sendTreeBuffer.count);
        }
        treeSizeSendData[currentDataBufferOffset++] = sendTreeBuffer.count;
      }
    }
    // Receiving size info from my neighbors
    auto recvIt = this->RecvBuffer.find(id);
    if (id >= 1)
    {
      treeSizeRecvOffset[id] = treeSizeRecvOffset[id - 1] + treeSizeRecvCount[id - 1];
    }
    if (recvIt == this->RecvBuffer.end())
    {
      continue;
    }
    RecvTreeBufferMap& recvTreeMap = recvIt->second;

    treeSizeRecvCount[id] = static_cast<int>(recvTreeMap.size());
    treeSizeRecvData.resize(treeSizeRecvOffset[id] + treeSizeRecvCount[id]);
  }

  this->Controller->GetCommunicator()->AllToAllVVoidArray(treeSizeSendData.data(),
    treeSizeSendCount.data(), treeSizeSendOffset.data(), treeSizeRecvData.data(),
    treeSizeRecvCount.data(), treeSizeRecvOffset.data(), VTK_ID_TYPE);

  // Store counts in receive buffer
  currentDataBufferOffset = 0;
  for (int id = 0; id < numberOfProcesses; ++id)
  {
    auto recvIt = this->RecvBuffer.find(id);
    if (recvIt == this->RecvBuffer.end())
    {
      continue;
    }

    for (auto&& RecvBufferPair : recvIt->second)
    {
      RecvBufferPair.second.count = treeSizeRecvData[currentDataBufferOffset++];
    }
  }
  return 1;
}

//------------------------------------------------------------------------------
int vtkHyperTreeGridGhostCellsGeneratorInternals::ExchangeTreeDecomposition()
{
  constexpr vtkIdType BITS_IN_UCHAR = 8;
  int numberOfProcesses = this->Controller->GetNumberOfProcesses();

  std::vector<unsigned char> cellDataSendData;
  std::vector<int> cellDataSendCount(numberOfProcesses, 0);
  std::vector<int> cellDataSendOffset(numberOfProcesses, 0);

  std::vector<unsigned char> cellDataRecvData;
  std::vector<int> cellDataRecvCount(numberOfProcesses, 0);
  std::vector<int> cellDataRecvOffset(numberOfProcesses, 0);

  // Data size is doubled when we need to transfer isMasked bit array.
  // We store isParent and isMasked bit arrays in the sent buffer contiguously.
  vtkIdType maskFactor = this->InputHTG->HasMask() ? 2 : 1;
  vtkDebugWithObjectMacro(nullptr, "Mask factor: " << maskFactor);

  int currentDataBufferOffset = 0;
  for (int id = 0; id < numberOfProcesses; ++id)
  {
    if (id >= 1)
    {
      cellDataSendOffset[id] = cellDataSendOffset[id - 1] + cellDataSendCount[id - 1];
    }
    auto sendIt = this->SendBuffer.find(id);
    if (sendIt != this->SendBuffer.end())
    {
      SendTreeBufferMap& sendTreeMap = sendIt->second;
      // Accumulated length
      vtkIdType currentProcessCount = 0;
      for (auto&& sendTreeBufferPair : sendTreeMap)
      {
        auto&& sendTreeBuffer = sendTreeBufferPair.second;
        if (sendTreeBuffer.count)
        {
          // We send the bits packed in unsigned char
          vtkIdType currentTreeCount = sendTreeBuffer.count / BITS_IN_UCHAR + 1;

          cellDataSendData.resize(cellDataSendData.size() + maskFactor * currentTreeCount);
          memcpy(cellDataSendData.data() + currentDataBufferOffset,
            sendTreeBuffer.isParent->GetPointer(0), currentTreeCount);
          if (this->InputHTG->HasMask())
          {
            memcpy(cellDataSendData.data() + currentDataBufferOffset + currentTreeCount,
              sendTreeBuffer.isMasked->GetPointer(0), currentTreeCount);
          }

          currentProcessCount += currentTreeCount * maskFactor;
          currentDataBufferOffset += currentTreeCount * maskFactor;
        }
      }
      cellDataSendCount[id] = currentProcessCount;
    }

    // Receiving masks info from my neighbors
    auto recvIt = this->RecvBuffer.find(id);
    if (id >= 1)
    {
      cellDataRecvOffset[id] = cellDataRecvOffset[id - 1] + cellDataRecvCount[id - 1];
    }
    if (recvIt == this->RecvBuffer.end())
    {
      continue;
    }

    RecvTreeBufferMap& recvTreeMap = recvIt->second;

    // If we have not dealt with process yet,
    // we prepare for receiving with appropriate length
    if (this->Flags[id] != NOT_TREATED)
    {
      continue;
    }
    int recvBufferLength = 0;
    for (auto&& recvTreeBufferPair : recvTreeMap)
    {
      auto&& recvTreeBuffer = recvTreeBufferPair.second;
      if (recvTreeBuffer.count != 0)
      {
        // bit message is packed in unsigned char, getting the correct length of the message
        recvBufferLength += recvTreeBuffer.count / BITS_IN_UCHAR + 1;
      }
    }
    recvBufferLength *= maskFactor; // isParent + potentially isMasked data

    cellDataRecvCount[id] = recvBufferLength;
    cellDataRecvData.resize(cellDataRecvOffset[id] + cellDataRecvCount[id]);
  }

  this->Controller->GetCommunicator()->AllToAllVVoidArray(cellDataSendData.data(),
    cellDataSendCount.data(), cellDataSendOffset.data(), cellDataRecvData.data(),
    cellDataRecvCount.data(), cellDataRecvOffset.data(), VTK_UNSIGNED_CHAR);

  currentDataBufferOffset = 0;
  for (int id = 0; id < numberOfProcesses; ++id)
  {
    if (this->Flags[id] != NOT_TREATED)
    {
      continue;
    }
    auto recvIt = this->RecvBuffer.find(id);
    if (recvIt == this->RecvBuffer.end())
    {
      continue;
    }
    // Distributing receive data among my trees, i.e. creating my ghost trees with this data
    // Remember: we only have the nodes / leaves at the inverface with our neighbor
    vtkNew<vtkHyperTreeGridNonOrientedCursor> outCursor;
    for (auto&& RecvBufferPair : recvIt->second)
    {
      vtkIdType treeId = RecvBufferPair.first;
      auto&& recvTreeBuffer = RecvBufferPair.second;
      if (recvTreeBuffer.count != 0)
      {
        this->OutputHTG->InitializeNonOrientedCursor(outCursor, treeId, true);

        // Stealing ownership of buf in isParent/isMasked to have vtkBitArray interface
        vtkNew<vtkBitArray> isParent;
        isParent->SetArray(
          cellDataRecvData.data() + currentDataBufferOffset, recvTreeBuffer.count, 1);
        vtkSmartPointer<vtkBitArray> isMasked = nullptr;
        if (this->InputHTG->HasMask())
        {
          isMasked = vtkSmartPointer<vtkBitArray>::New();
          isMasked->SetArray(cellDataRecvData.data() + currentDataBufferOffset +
              recvTreeBuffer.count / BITS_IN_UCHAR + 1,
            recvTreeBuffer.count, 1);
        }

        recvTreeBuffer.offset = this->NumberOfVertices;
        recvTreeBuffer.indices.resize(recvTreeBuffer.count);

        outCursor->SetGlobalIndexStart(this->NumberOfVertices);

        vtkIdType pos = 0;
        this->NumberOfVertices += ::CreateGhostTree(
          outCursor, isParent, isMasked, this->OutputMask, recvTreeBuffer.indices.data(), pos);

        currentDataBufferOffset += (recvTreeBuffer.count / BITS_IN_UCHAR + 1) * maskFactor;
      }
    }
    this->Flags[id] = INITIALIZE_TREE;
  }
  return 1;
}

//------------------------------------------------------------------------------
int vtkHyperTreeGridGhostCellsGeneratorInternals::ExchangeCellData()
{
  int numberOfProcesses = this->Controller->GetNumberOfProcesses();

  std::vector<double> cellDataSendData;
  std::vector<int> cellDataSendCount(numberOfProcesses, 0);
  std::vector<int> cellDataSendOffset(numberOfProcesses, 0);

  std::vector<double> cellDataRecvData;
  std::vector<int> cellDataRecvCount(numberOfProcesses, 0);
  std::vector<int> cellDataRecvOffset(numberOfProcesses, 0);

  vtkCellData* cellData = this->InputHTG->GetCellData();

  int currentDataBufferOffset = 0;
  for (int id = 0; id < numberOfProcesses; ++id)
  {
    if (id >= 1)
    {
      cellDataSendOffset[id] = cellDataSendOffset[id - 1] + cellDataSendCount[id - 1];
    }
    auto sendIt = this->SendBuffer.find(id);
    if (sendIt != this->SendBuffer.end())
    {
      SendTreeBufferMap& sendTreeMap = sendIt->second;

      vtkIdType currentProcessCount = 0;
      for (auto&& sendTreeBufferPair : sendTreeMap)
      {
        auto&& sendTreeBuffer = sendTreeBufferPair.second;
        if (sendTreeBuffer.count)
        {
          currentProcessCount += sendTreeBuffer.count * ::GetNumberOfCellValues(cellData);
          cellDataSendData.resize(
            cellDataSendData.size() + sendTreeBuffer.count * ::GetNumberOfCellValues(cellData));

          // Fill send buffer with array data
          for (int arrayId = 0; arrayId < cellData->GetNumberOfArrays(); ++arrayId)
          {
            vtkDataArray* inArray = cellData->GetArray(arrayId);
            for (vtkIdType tupleId = 0; tupleId < sendTreeBuffer.count; ++tupleId)
            {
              for (int compId = 0; compId < inArray->GetNumberOfComponents(); compId++)
              {
                cellDataSendData[currentDataBufferOffset++] =
                  inArray->GetComponent(sendTreeBuffer.indices[tupleId], compId);
              }
            }
          }
        }
      }
      cellDataSendCount[id] = currentProcessCount;
    }

    // Receiving cell data info from my neighbors
    auto recvIt = this->RecvBuffer.find(id);
    if (id >= 1)
    {
      cellDataRecvOffset[id] = cellDataRecvOffset[id - 1] + cellDataRecvCount[id - 1];
    }
    if (recvIt == this->RecvBuffer.end())
    {
      continue;
    }

    RecvTreeBufferMap& recvTreeMap = recvIt->second;

    if (this->Flags[id] != INITIALIZE_TREE)
    {
      continue;
    }
    // Compute total length to be received
    int recvBufferLength = 0;
    for (auto&& recvTreeBufferPair : recvTreeMap)
    {
      recvBufferLength += recvTreeBufferPair.second.count * ::GetNumberOfCellValues(cellData);
    }
    cellDataRecvCount[id] = recvBufferLength;
    cellDataRecvData.resize(cellDataRecvOffset[id] + cellDataRecvCount[id]);
  }

  this->Controller->GetCommunicator()->AllToAllVVoidArray(cellDataSendData.data(),
    cellDataSendCount.data(), cellDataSendOffset.data(), cellDataRecvData.data(),
    cellDataRecvCount.data(), cellDataRecvOffset.data(), VTK_DOUBLE);

  currentDataBufferOffset = 0;
  for (int id = 0; id < numberOfProcesses; ++id)
  {
    if (this->Flags[id] != INITIALIZE_TREE)
    {
      continue;
    }
    auto recvIt = this->RecvBuffer.find(id);
    if (recvIt == this->RecvBuffer.end())
    {
      continue;
    }

    // Fill ImplicitCD using data received
    for (auto&& RecvBufferPair : recvIt->second)
    {
      auto&& recvTreeBuffer = RecvBufferPair.second;
      for (int arrayId = 0; arrayId < cellData->GetNumberOfArrays(); ++arrayId)
      {
        std::string arrName = cellData->GetArrayName(arrayId);
        vtkDataArray* outArray = this->ImplicitCD.at(arrName).GhostCDBuffer;
        assert(outArray);
        vtkIdType offset = this->ImplicitCD.at(arrName).InternalArray->GetNumberOfTuples();
        for (vtkIdType tupleId = 0; tupleId < recvTreeBuffer.count; ++tupleId)
        {
          for (int compIdx = 0; compIdx < outArray->GetNumberOfComponents(); compIdx++)
          {
            vtkIdType implicitComponent = recvTreeBuffer.indices[tupleId] - offset;
            assert(implicitComponent >= 0);
            outArray->InsertComponent(
              implicitComponent, compIdx, cellDataRecvData[currentDataBufferOffset++]);
          }
        }
      }
    }
    this->Flags[id] = INITIALIZE_FIELD;
  }

  return 1;
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridGhostCellsGeneratorInternals::FinalizeCellData()
{
  using SupportedTypes = vtkTypeList::Append<vtkArrayDispatch::AllTypes, std::string>::Result;
  using Dispatcher = vtkArrayDispatch::DispatchByValueType<SupportedTypes>;

  AddIndexedArrayWorker worker;
  vtkCellData* outputCD = this->OutputHTG->GetCellData();

  int nbArrays = outputCD->GetNumberOfArrays();
  for (int iA = 0; iA < nbArrays; iA++)
  {
    auto da = vtkDataArray::SafeDownCast(outputCD->GetAbstractArray(iA));
    if (!da)
    {
      continue;
    }

    if (!Dispatcher::Execute(da, worker, this->ImplicitCD[da->GetName()], outputCD))
    {
      worker(da, this->ImplicitCD[da->GetName()], outputCD); // fallback
    }
  }

  // Adding the ghost array
  vtkDebugWithObjectMacro(nullptr,
    "Adding ghost array: ghost from id " << this->InitialNumberOfVertices << " to "
                                         << this->NumberOfVertices);

  vtkNew<vtkUnsignedCharArray> scalars;
  scalars->SetNumberOfComponents(1);
  scalars->SetName(vtkDataSetAttributes::GhostArrayName());
  scalars->SetNumberOfTuples(this->NumberOfVertices);

  for (vtkIdType ii = 0; ii < this->InitialNumberOfVertices; ++ii)
  {
    scalars->InsertValue(ii, 0);
  }
  for (vtkIdType ii = this->InitialNumberOfVertices; ii < this->NumberOfVertices; ++ii)
  {
    scalars->InsertValue(ii, 1);
  }
  this->OutputHTG->GetCellData()->AddArray(scalars);
  this->OutputHTG->SetMask(this->OutputMask);
}

VTK_ABI_NAMESPACE_END
