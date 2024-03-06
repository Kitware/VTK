// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkHyperTreeGridGhostCellsGenerator.h"

#include "vtkBitArray.h"
#include "vtkCellData.h"
#include "vtkCommunicator.h"
#include "vtkHyperTree.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridNonOrientedCursor.h"
#include "vtkHyperTreeGridOrientedCursor.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkUniformHyperTreeGrid.h"
#include "vtkUnsignedCharArray.h"

#include <cmath>
#include <map>
#include <unordered_map>
#include <utility>
#include <vector>

namespace
{

template <typename MapType>
typename MapType::iterator ProbeFind(
  vtkMultiProcessController* controller, int tag, MapType& recvMap)
{
  int processBuff = -1;
  auto targetRecv = recvMap.end();
  if (controller->Probe(vtkMultiProcessController::ANY_SOURCE, tag, &processBuff) != 1)
  {
    vtkErrorWithObjectMacro(nullptr, "Probe failed on reception of tag " << tag);
    return targetRecv;
  }
  if (processBuff < 0)
  {
    vtkErrorWithObjectMacro(
      nullptr, "Probe returned erroneous process ID " << processBuff << "reception of tag " << tag);
    return targetRecv;
  }
  targetRecv = recvMap.find(processBuff);
  if (targetRecv == recvMap.end())
  {
    vtkErrorWithObjectMacro(nullptr,
      "Receiving unexpected communication from " << processBuff << " process on tag " << tag
                                                 << ".");
    return targetRecv;
  }
  return targetRecv;
}

}

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkHyperTreeGridGhostCellsGenerator);

namespace
{
struct SendBuffer
{
  SendBuffer()
    : count(0)
    , mask(0)
    , isParent(vtkBitArray::New())
  {
  }
  ~SendBuffer() { isParent->Delete(); }
  vtkIdType count;                // len buffer
  unsigned int mask;              // ghost mask
  std::vector<vtkIdType> indices; // indices for selected cells
  vtkBitArray* isParent;          // decomposition amr tree
};

struct RecvBuffer
{
  RecvBuffer()
    : count(0)
    , offset(0)
  {
  }
  vtkIdType count;  // len buffer
  vtkIdType offset; // offset in field vector
  std::vector<vtkIdType> indices;
};

typedef std::map<unsigned int, SendBuffer> SendTreeBufferMap;
typedef std::map<unsigned int, SendTreeBufferMap> SendProcessBufferMap;
typedef std::map<unsigned int, RecvBuffer> RecvTreeBufferMap;
typedef std::map<unsigned int, RecvTreeBufferMap> RecvProcessBufferMap;

enum FlagType
{
  NOT_TREATED,
  INITIALIZE_TREE,
  INITIALIZE_FIELD
};

typedef std::unordered_map<unsigned int, FlagType> FlagMap;

/**
 * Structured initialized by ProcessTrees, used to pass the execution objects to internal
 * functions.
 */
struct HTGGContextContainer
{
  HTGGContextContainer(vtkMultiProcessController* controller, vtkHyperTreeGrid* inputHTG,
    vtkHyperTreeGrid* outputHTG, vtkBitArray* outputMask, int nbHTs, vtkIdType totalVertices)
    : controller(controller)
    , inputHTG(inputHTG)
    , outputHTG(outputHTG)
    , outputMask(outputMask)
    , totalVertices(totalVertices)
    , nbHTs(nbHTs)
  {
    hyperTreesMapToProcesses.resize(nbHTs + controller->GetNumberOfProcesses());
  }

  // Handling receive and send buffer.
  // The structure is as follows:
  // sendBuffer[id] or recvBuffer[id] == process id of neighbor with who to communicate buffer
  // sendBuffer[id][jd] or recvBuffer[id][jd] tells which tree index is being sent.
  SendProcessBufferMap sendBuffer;
  RecvProcessBufferMap recvBuffer;

  FlagMap flags;

  std::vector<int> hyperTreesMapToProcesses;
  vtkMultiProcessController* controller = nullptr;
  vtkHyperTreeGrid* inputHTG = nullptr;
  vtkHyperTreeGrid* outputHTG = nullptr;
  vtkBitArray* outputMask = nullptr;
  vtkIdType totalVertices = 0;

  int nbHTs = 0;
};

const int HTGGCG_SIZE_EXCHANGE_TAG = 5098;
const int HTGGCG_DATA_EXCHANGE_TAG = 5099;
const int HTGGCG_DATA2_EXCHANGE_TAG = 5100;

/**
 * ProcessTrees subroutine creating the output ghost array and adding it to the output HTG.
 *
 * @param input Input HTG
 * @param totalVertices The number of vertices in the HTG including ghost cells.
 */
void AppendGhostArray(HTGGContextContainer& ctx)
{
  vtkNew<vtkUnsignedCharArray> scalars;
  scalars->SetNumberOfComponents(1);
  scalars->SetName(vtkDataSetAttributes::GhostArrayName());
  scalars->SetNumberOfTuples(ctx.totalVertices);
  for (vtkIdType ii = 0; ii < ctx.inputHTG->GetNumberOfCells(); ++ii)
  {
    scalars->InsertValue(ii, 0);
  }
  for (vtkIdType ii = ctx.inputHTG->GetNumberOfCells(); ii < ctx.totalVertices; ++ii)
  {
    scalars->InsertValue(ii, 1);
  }
  ctx.outputHTG->GetCellData()->AddArray(scalars);
}

/**
 * Recursively copy the input tree (cell data and mask information)
 * pointed by the cursor to the output.
 * Fills memory gaps if present.
 */
void CopyInputTreeToOutput(vtkHyperTreeGridNonOrientedCursor* inCursor,
  vtkHyperTreeGridNonOrientedCursor* outCursor, vtkCellData* inCellData, vtkCellData* outCellData,
  vtkBitArray* inMask, vtkBitArray* outMask)
{
  vtkIdType outIdx = outCursor->GetGlobalNodeIndex(), inIdx = inCursor->GetGlobalNodeIndex();
  outCellData->InsertTuple(outIdx, inIdx, inCellData);
  if (inMask)
  {
    outMask->InsertTuple1(outIdx, inMask->GetValue(inIdx));
  }
  if (!inCursor->IsLeaf())
  {
    outCursor->SubdivideLeaf();
    for (int ichild = 0; ichild < inCursor->GetNumberOfChildren(); ++ichild)
    {
      outCursor->ToChild(ichild);
      inCursor->ToChild(ichild);
      ::CopyInputTreeToOutput(inCursor, outCursor, inCellData, outCellData, inMask, outMask);
      outCursor->ToParent();
      inCursor->ToParent();
    }
  }
}

/**
 * ProcessTree subroutine copying the input tree to the output (cell and mask data information)
 * We do it "by hand" to fill gaps if they exist.
 *
 * Return the number of vertices in output trree
 */
vtkIdType CopyInputHyperTreeToOutput(
  vtkHyperTreeGrid* input, vtkHyperTreeGrid* output, vtkBitArray* outputMask)
{
  vtkBitArray* inputMask = input->HasMask() ? input->GetMask() : nullptr;
  vtkNew<vtkHyperTreeGridNonOrientedCursor> outCursor, inCursor;
  vtkHyperTreeGrid::vtkHyperTreeGridIterator inputIterator;
  vtkIdType inTreeIndex = 0, totalVertices = 0;
  input->InitializeTreeIterator(inputIterator);

  while (inputIterator.GetNextTree(inTreeIndex))
  {
    input->InitializeNonOrientedCursor(inCursor, inTreeIndex);
    output->InitializeNonOrientedCursor(outCursor, inTreeIndex, true);
    outCursor->SetGlobalIndexStart(totalVertices);
    ::CopyInputTreeToOutput(
      inCursor, outCursor, input->GetCellData(), output->GetCellData(), inputMask, outputMask);
    totalVertices += outCursor->GetTree()->GetNumberOfVertices();
  }

  return totalVertices;
}

//------------------------------------------------------------------------------
vtkIdType CreateGhostTree(vtkHyperTreeGridNonOrientedCursor* outCursor, vtkBitArray* isParent,
  vtkIdType* indices, vtkIdType&& pos = 0)
{
  indices[pos] = outCursor->GetGlobalNodeIndex();
  if (isParent->GetValue(pos++))
  {
    outCursor->SubdivideLeaf();
    for (int ichild = 0; ichild < outCursor->GetNumberOfChildren(); ++ichild)
    {
      outCursor->ToChild(ichild);
      ::CreateGhostTree(outCursor, isParent, indices, std::forward<vtkIdType&&>(pos));
      outCursor->ToParent();
    }
  }
  return pos;
}

//------------------------------------------------------------------------------
void ExtractInterface(vtkHyperTreeGridNonOrientedCursor* inCursor, vtkBitArray* isParent,
  std::vector<vtkIdType>& indices, vtkHyperTreeGrid* grid, unsigned int mask, vtkIdType& pos)
{
  isParent->InsertTuple1(pos, !inCursor->IsLeaf());
  indices[pos] = inCursor->GetGlobalNodeIndex();
  ++pos;
  if (!inCursor->IsLeaf())
  {
    for (int ichild = 0; ichild < inCursor->GetNumberOfChildren(); ++ichild)
    {
      inCursor->ToChild(ichild);
      unsigned int newMask = mask & grid->GetChildMask(ichild);
      if (newMask)
      {
        ::ExtractInterface(inCursor, isParent, indices, grid, newMask, pos);
      }
      else
      {
        isParent->InsertTuple1(pos, 0);
        indices[pos] = inCursor->GetGlobalNodeIndex();
        ++pos;
      }
      inCursor->ToParent();
    }
  }
}

//------------------------------------------------------------------------------
int GetNumberOfCellValues(vtkHyperTreeGrid* output)
{
  int totalCellSize = 0;
  vtkCellData* pd = output->GetCellData();
  int nbArray = pd->GetNumberOfArrays();
  for (int arrayId = 0; arrayId < nbArray; arrayId++)
  {
    vtkDataArray* outArray = pd->GetArray(arrayId);
    totalCellSize += outArray->GetNumberOfComponents();
  }
  return totalCellSize;
}

/**
 * ProcessTree subroutine performing an MPI AllReduce operation,
 * filling a vector where `v[i]` is the rank of the process owning tree indexed `i`.
 *
 */
void BroadcastTreeLocations(HTGGContextContainer& ctx)
{

  unsigned cellDims[3];
  ctx.inputHTG->GetCellDims(cellDims);
  vtkIdType nbHTs = cellDims[0] * cellDims[1] * cellDims[2];

  int numberOfProcesses = ctx.controller->GetNumberOfProcesses();
  int processId = ctx.controller->GetLocalProcessId();

  std::vector<int> broadcastHyperTreesMapToProcesses(nbHTs + numberOfProcesses, -1);

  vtkNew<vtkHyperTreeGridNonOrientedCursor> inCursor;
  vtkHyperTreeGrid::vtkHyperTreeGridIterator inputIterator;
  vtkIdType inTreeIndex = 0;
  ctx.inputHTG->InitializeTreeIterator(inputIterator);
  while (inputIterator.GetNextTree(inTreeIndex))
  {
    ctx.inputHTG->InitializeNonOrientedCursor(inCursor, inTreeIndex);
    if (inCursor->HasTree())
    {
      broadcastHyperTreesMapToProcesses[inTreeIndex] = processId;
    }
  }
  broadcastHyperTreesMapToProcesses[nbHTs + processId] = ctx.inputHTG->HasMask();
  ctx.controller->AllReduce(broadcastHyperTreesMapToProcesses.data(),
    ctx.hyperTreesMapToProcesses.data(), nbHTs + numberOfProcesses, vtkCommunicator::MAX_OP);

  assert(ctx.inputHTG->GetDimension() > 1);
}

void DetermineNeighbors(HTGGContextContainer& ctx)
{
  unsigned cellDims[3];
  ctx.inputHTG->GetCellDims(cellDims);
  vtkNew<vtkHyperTreeGridOrientedCursor> inOrientedCursor;
  vtkHyperTreeGrid::vtkHyperTreeGridIterator inputIterator;
  vtkIdType inTreeIndex = 0;
  unsigned int i, j, k = 0;
  ctx.inputHTG->InitializeTreeIterator(inputIterator);
  switch (ctx.inputHTG->GetDimension())
  {
    case 2:
    {
      while (inputIterator.GetNextTree(inTreeIndex))
      {
        ctx.inputHTG->InitializeOrientedCursor(inOrientedCursor, inTreeIndex);
        if (inOrientedCursor->IsMasked())
        {
          continue;
        }
        ctx.inputHTG->GetLevelZeroCoordinatesFromIndex(inTreeIndex, i, j, k);
        // Avoiding over / under flowing the grid
        for (int rj = ((j > 0) ? -1 : 0); rj < (((j + 1) < cellDims[1]) ? 2 : 1); ++rj)
        {
          for (int ri = ((i > 0) ? -1 : 0); ri < (((i + 1) < cellDims[0]) ? 2 : 1); ++ri)
          {
            vtkIdType neighbor = -1;
            ctx.inputHTG->GetIndexFromLevelZeroCoordinates(neighbor, i + ri, j + rj, 0);
            int id = ctx.hyperTreesMapToProcesses[neighbor];
            if (id >= 0 && id != ctx.controller->GetLocalProcessId())
            {
              // Build a neighborhood mask to extract the interface in
              // ExtractInterface later on.
              // Same encoding as vtkHyperTreeGrid::GetChildMask
              ctx.sendBuffer[id][inTreeIndex].mask |= 1
                << (8 * sizeof(int) - 1 - (ri + 1 + (rj + 1) * 3));
              // Not receiving anything from this guy since we will send him stuff
              ctx.recvBuffer[id][neighbor].count = 0;
              // Process not treated yet, yielding the flag
              ctx.flags[id] = NOT_TREATED;
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
        ctx.inputHTG->InitializeOrientedCursor(inOrientedCursor, inTreeIndex);
        if (inOrientedCursor->IsMasked())
        {
          continue;
        }
        ctx.inputHTG->GetLevelZeroCoordinatesFromIndex(inTreeIndex, i, j, k);
        // Avoiding over / under flowing the grid
        for (int rk = ((k > 0) ? -1 : 0); rk < (((k + 1) < cellDims[2]) ? 2 : 1); ++rk)
        {
          for (int rj = ((j > 0) ? -1 : 0); rj < (((j + 1) < cellDims[1]) ? 2 : 1); ++rj)
          {
            for (int ri = ((i > 0) ? -1 : 0); ri < (((i + 1) < cellDims[0]) ? 2 : 1); ++ri)
            {
              vtkIdType neighbor = -1;
              ctx.inputHTG->GetIndexFromLevelZeroCoordinates(neighbor, i + ri, j + rj, k + rk);
              int id = ctx.hyperTreesMapToProcesses[neighbor];
              if (id >= 0 && id != ctx.controller->GetLocalProcessId())
              {
                // Build a neighborhood mask to extract the interface in
                // ExtractInterface later on.
                // Same encoding as vtkHyperTreeGrid::GetChildMask
                ctx.sendBuffer[id][inTreeIndex].mask |= 1
                  << (8 * sizeof(int) - 1 - (ri + 1 + (rj + 1) * 3 + (rk + 1) * 9));
                // Not receiving anything from this guy since we will send him stuff
                ctx.recvBuffer[id][neighbor].count = 0;
                // Process not treated yet, yielding the flag
                ctx.flags[id] = NOT_TREATED;
              }
            }
          }
        }
      }
      break;
    }
  }
}

int ExchangeSizes(HTGGContextContainer& ctx)
{
  int numberOfProcesses = ctx.controller->GetNumberOfProcesses();
  int processId = ctx.controller->GetLocalProcessId();
  for (int id = 0; id < numberOfProcesses; ++id)
  {
    if (id != processId)
    {
      auto sendIt = ctx.sendBuffer.find(id);
      if (sendIt != ctx.sendBuffer.end())
      {
        SendTreeBufferMap& sendTreeMap = sendIt->second;
        std::vector<vtkIdType> counts(sendTreeMap.size());
        int cpt = 0;
        {
          vtkNew<vtkHyperTreeGridNonOrientedCursor> inCursor;

          for (auto&& sendTreeBufferPair : sendTreeMap)
          {
            vtkIdType treeId = sendTreeBufferPair.first;
            auto&& sendTreeBuffer = sendTreeBufferPair.second;
            ctx.inputHTG->InitializeNonOrientedCursor(inCursor, treeId);
            // Extracting the tree interface with its neighbors
            sendTreeBuffer.count = 0;
            vtkHyperTree* tree = inCursor->GetTree();
            if (tree)
            {
              // We store the isParent profile along the interface to know when to subdivide later
              // indices store the indices in the input of the nodes on the interface
              vtkIdType nbVertices = tree->GetNumberOfVertices();
              sendTreeBuffer.indices.resize(nbVertices);
              ::ExtractInterface(inCursor, sendTreeBuffer.isParent, sendTreeBuffer.indices,
                ctx.inputHTG, sendTreeBuffer.mask, sendTreeBuffer.count);
            }
            // Telling my neighbors how much data I will send later
            counts[cpt++] = sendTreeBuffer.count;
          }
        }
        vtkDebugWithObjectMacro(nullptr, "Send: data size to " << id);
        ctx.controller->Send(counts.data(), cpt, id, HTGGCG_SIZE_EXCHANGE_TAG);
      }
    }
    else
    {
      // Receiving size info from my neighbors
      std::size_t iRecv = 0;
      for (auto itRecvBuffer = ctx.recvBuffer.begin(); itRecvBuffer != ctx.recvBuffer.end();
           ++itRecvBuffer)
      {
        auto targetRecvBuffer = itRecvBuffer;
        if (ctx.controller->CanProbe())
        {
          targetRecvBuffer = ::ProbeFind(ctx.controller, HTGGCG_SIZE_EXCHANGE_TAG, ctx.recvBuffer);
          if (targetRecvBuffer == ctx.recvBuffer.end())
          {
            vtkErrorWithObjectMacro(nullptr,
              "Reception probe on process " << processId << " failed on " << iRecv
                                            << "th iteration.");
            return 0;
          }
        }
        int process = targetRecvBuffer->first;
        auto&& recvTreeMap = targetRecvBuffer->second;
        std::vector<vtkIdType> counts(recvTreeMap.size());
        vtkDebugWithObjectMacro(nullptr, "Receive: data size from " << process);
        ctx.controller->Receive(counts.data(), static_cast<vtkIdType>(recvTreeMap.size()), process,
          HTGGCG_SIZE_EXCHANGE_TAG);
        int cpt = 0;
        for (auto&& recvBufferPair : recvTreeMap)
        {
          recvBufferPair.second.count = counts[cpt++];
        }
        iRecv++;
      }
    }
  }
  return 1;
}

int ExchangeMasks(HTGGContextContainer& ctx)
{
  int numberOfProcesses = ctx.controller->GetNumberOfProcesses();
  int processId = ctx.controller->GetLocalProcessId();
  vtkDebugWithObjectMacro(nullptr, "Send masks and parent state of each node");
  for (int id = 0; id < numberOfProcesses; ++id)
  {
    if (id != processId)
    {
      auto sendIt = ctx.sendBuffer.find(id);
      if (sendIt != ctx.sendBuffer.end())
      {
        SendTreeBufferMap& sendTreeMap = sendIt->second;
        std::vector<unsigned char> buf;
        // Accumulated length
        vtkIdType totalLen = 0;
        for (auto&& sendTreeBufferPair : sendTreeMap)
        {
          auto&& sendTreeBuffer = sendTreeBufferPair.second;
          if (sendTreeBuffer.count)
          {
            // We send the bits packed in unsigned char
            vtkIdType currentLen = sendTreeBuffer.count / sizeof(unsigned char) + 1;
            if (ctx.inputHTG->HasMask())
            {
              currentLen *= 2;
            }
            buf.resize(totalLen + currentLen);
            memcpy(buf.data() + totalLen, sendTreeBuffer.isParent->GetPointer(0),
              sendTreeBuffer.count / sizeof(unsigned char) + 1);
            if (ctx.inputHTG->HasMask())
            {
              unsigned char* mask = buf.data() + totalLen + currentLen / 2;
              for (vtkIdType ii = 0; ii < currentLen / 2; ++ii)
              {
                mask[ii] = 0;
              }
              vtkBitArray* bmask = ctx.inputHTG->GetMask();
              // Filling the mask with bits at the appropriate location
              for (vtkIdType m = 0; m < sendTreeBuffer.count; ++m)
              {
                *mask |= static_cast<unsigned char>(bmask->GetValue(sendTreeBuffer.indices[m]))
                  << (sizeof(unsigned char) - 1 - (m % sizeof(unsigned char)));
                // Incrementing the pointer when unsigned char overflows
                mask += !((m + 1) % sizeof(unsigned char));
              }
            }
            totalLen += currentLen;
          }
        }
        vtkDebugWithObjectMacro(nullptr, "Send mask data from " << processId << " to " << id);
        ctx.controller->Send(buf.data(), totalLen, id, HTGGCG_DATA_EXCHANGE_TAG);
      }
    }
    else
    {
      // Receiving masks
      std::size_t iRecv = 0;
      for (auto itRecvBuffer = ctx.recvBuffer.begin(); itRecvBuffer != ctx.recvBuffer.end();
           ++itRecvBuffer)
      {
        auto targetRecvBuffer = itRecvBuffer;
        if (ctx.controller->CanProbe())
        {
          targetRecvBuffer = ::ProbeFind(ctx.controller, HTGGCG_DATA_EXCHANGE_TAG, ctx.recvBuffer);
          if (targetRecvBuffer == ctx.recvBuffer.end())
          {
            vtkErrorWithObjectMacro(nullptr,
              "Reception probe on process " << processId << " failed on " << iRecv
                                            << "th iteration.");
            return 0;
          }
        }
        int process = targetRecvBuffer->first;
        auto&& recvTreeMap = targetRecvBuffer->second;

        // If we have not dealt with process yet,
        // we prepare for receiving with appropriate length
        if (ctx.flags[process] == NOT_TREATED)
        {
          vtkIdType len = 0;
          for (auto&& recvTreeBufferPair : recvTreeMap)
          {
            auto&& recvTreeBuffer = recvTreeBufferPair.second;
            if (recvTreeBuffer.count != 0)
            {
              // bit message is packed in unsigned char, getting the correct length of the message
              len += recvTreeBuffer.count / sizeof(unsigned char) + 1;
              if (ctx.inputHTG->HasMask())
              {
                len += recvTreeBuffer.count / sizeof(unsigned char) + 1;
              }
            }
          }
          std::vector<unsigned char> buf(len);

          vtkDebugWithObjectMacro(nullptr, "Receive mask data from " << process);
          ctx.controller->Receive(buf.data(), len, process, HTGGCG_DATA_EXCHANGE_TAG);

          vtkIdType cpt = 0;
          vtkNew<vtkHyperTreeGridNonOrientedCursor> outCursor;
          // Distributing receive data among my trees, i.e. creating my ghost trees with this data
          // Remember: we only have the nodes / leaves at the inverface with our neighbor
          for (auto&& recvTreeBufferPair : recvTreeMap)
          {
            vtkIdType treeId = recvTreeBufferPair.first;
            auto&& recvTreeBuffer = recvTreeBufferPair.second;
            if (recvTreeBuffer.count != 0)
            {
              ctx.outputHTG->InitializeNonOrientedCursor(outCursor, treeId, true);
              vtkNew<vtkBitArray> isParent;

              // Stealing ownership of buf in isParent to have vtkBitArray interface
              isParent->SetArray(buf.data() + cpt,
                ctx.inputHTG->HasMask() ? 2 * recvTreeBuffer.count : recvTreeBuffer.count, 1);

              recvTreeBuffer.offset = ctx.totalVertices;
              recvTreeBuffer.indices.resize(recvTreeBuffer.count);

              outCursor->SetGlobalIndexStart(ctx.totalVertices);

              if (!ctx.outputMask && ctx.hyperTreesMapToProcesses[ctx.nbHTs + process])
              {
                ctx.outputMask = vtkBitArray::New();
                ctx.outputMask->Resize(ctx.totalVertices);
                for (vtkIdType ii = 0; ii < ctx.totalVertices; ++ii)
                {
                  ctx.outputMask->SetValue(ii, 0);
                }
              }

              ctx.totalVertices +=
                ::CreateGhostTree(outCursor, isParent, recvTreeBuffer.indices.data());

              if (ctx.hyperTreesMapToProcesses[ctx.nbHTs + process])
              {
                vtkNew<vtkBitArray> mask;
                // Stealing ownership of buf for mask handling to have vtkBitArray interface

                mask->SetArray(buf.data() + cpt + recvTreeBuffer.count / sizeof(unsigned char) + 1,
                  recvTreeBuffer.count, 1);

                for (vtkIdType m = 0; m < recvTreeBuffer.count; ++m)
                {
                  ctx.outputMask->InsertValue(recvTreeBuffer.indices[m], mask->GetValue(m));
                }
                cpt += 2 * (recvTreeBuffer.count / sizeof(unsigned char) + 1);
              }
              else
              {
                if (ctx.outputMask)
                {
                  for (vtkIdType m = 0; m < recvTreeBuffer.count; ++m)
                  {
                    ctx.outputMask->InsertValue(recvTreeBuffer.indices[m], 0);
                  }
                }
                cpt += recvTreeBuffer.count / sizeof(unsigned char) + 1;
              }
            }
          }
          ctx.flags[process] = INITIALIZE_TREE;
        }
        iRecv++;
      }
    }
  }
  return 1;
}

int ExchangeCellData(HTGGContextContainer& ctx)
{
  // Pre-compute the total number of values associated to each cell
  int totalCellSize = ::GetNumberOfCellValues(ctx.outputHTG);
  vtkDebugWithObjectMacro(nullptr, "Computed total cell data size: " << totalCellSize);

  int numberOfProcesses = ctx.controller->GetNumberOfProcesses();
  int processId = ctx.controller->GetLocalProcessId();

  vtkCellData* pd = ctx.outputHTG->GetCellData();
  int nbArray = pd->GetNumberOfArrays();
  vtkDebugWithObjectMacro(nullptr, "Send data stored on each node");
  for (int id = 0; id < numberOfProcesses; ++id)
  {
    if (id != processId)
    {
      vtkDebugWithObjectMacro(nullptr, "Begin sending to process " << id);
      auto sendIt = ctx.sendBuffer.find(id);
      if (sendIt != ctx.sendBuffer.end())
      {
        SendTreeBufferMap& sendTreeMap = sendIt->second;
        std::vector<double> buf;
        vtkIdType totalLength = 0;
        vtkIdType writeOffset = 0;

        for (auto&& sendTreeBufferPair : sendTreeMap)
        {
          auto&& sendTreeBuffer = sendTreeBufferPair.second;
          if (sendTreeBuffer.count)
          {
            vtkDebugWithObjectMacro(nullptr,
              "New sendTreeBufferPair with " << sendTreeBuffer.count << " elements for process "
                                             << id);
            totalLength += totalCellSize * sendTreeBuffer.count;
            buf.resize(totalLength);

            // Fill send buffer with array data
            for (int arrayId = 0; arrayId < nbArray; ++arrayId)
            {
              vtkDataArray* inArray = pd->GetArray(arrayId);
              for (vtkIdType tupleId = 0; tupleId < sendTreeBuffer.count; ++tupleId)
              {
                for (int compId = 0; compId < inArray->GetNumberOfComponents(); compId++)
                {
                  buf[writeOffset++] =
                    inArray->GetComponent(sendTreeBuffer.indices[tupleId], compId);
                }
              }
            }
          }
        }
        vtkDebugWithObjectMacro(nullptr, "Send arrays data to " << id);
        ctx.controller->Send(buf.data(), totalLength, id, HTGGCG_DATA2_EXCHANGE_TAG);
        vtkDebugWithObjectMacro(nullptr, "Done sending array data to " << id);
      }
    }
    else
    {
      vtkDebugWithObjectMacro(nullptr, "Receiving cell data from the other processes");

      std::size_t iRecv = 0;
      for (auto itRecvBuffer = ctx.recvBuffer.begin(); itRecvBuffer != ctx.recvBuffer.end();
           ++itRecvBuffer)
      {
        auto targetRecvBuffer = itRecvBuffer;
        if (ctx.controller->CanProbe())
        {
          targetRecvBuffer = ::ProbeFind(ctx.controller, HTGGCG_DATA2_EXCHANGE_TAG, ctx.recvBuffer);
          if (targetRecvBuffer == ctx.recvBuffer.end())
          {
            vtkErrorWithObjectMacro(nullptr,
              "Reception probe on process " << processId << " failed on " << iRecv
                                            << "th iteration.");
            return 0;
          }
        }
        int process = targetRecvBuffer->first;
        vtkDebugWithObjectMacro(nullptr, "Begin receiving data from process " << process);

        auto&& recvTreeMap = targetRecvBuffer->second;
        if (ctx.flags[process] == INITIALIZE_TREE)
        {
          // Compute total length to be received
          unsigned long totalLength = 0;
          for (auto&& recvTreeBufferPair : recvTreeMap)
          {
            totalLength += totalCellSize * recvTreeBufferPair.second.count;
          }

          std::vector<double> buf(totalLength);
          ctx.controller->Receive(buf.data(), totalLength, process, HTGGCG_DATA2_EXCHANGE_TAG);

          // Fill output arrays using data received
          vtkIdType readOffset = 0;
          for (auto&& recvTreeBufferPair : recvTreeMap)
          {
            auto&& recvTreeBuffer = recvTreeBufferPair.second;

            for (int arrayId = 0; arrayId < nbArray; ++arrayId)
            {
              vtkDataArray* outArray = pd->GetArray(arrayId);

              for (vtkIdType tupleId = 0; tupleId < recvTreeBuffer.count; ++tupleId)
              {
                for (int compIdx = 0; compIdx < outArray->GetNumberOfComponents(); compIdx++)
                {
                  outArray->InsertComponent(
                    recvTreeBuffer.indices[tupleId], compIdx, buf[readOffset++]);
                }
              }
            }
          }
          ctx.flags[process] = INITIALIZE_FIELD;
        }
        iRecv++;
      }
    }
  }
  return 1;
}

}

//------------------------------------------------------------------------------
vtkHyperTreeGridGhostCellsGenerator::vtkHyperTreeGridGhostCellsGenerator()
{
  this->AppropriateOutput = true;
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridGhostCellsGenerator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
int vtkHyperTreeGridGhostCellsGenerator::FillOutputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkHyperTreeGrid");
  return 1;
}

//------------------------------------------------------------------------------
int vtkHyperTreeGridGhostCellsGenerator::ProcessTrees(
  vtkHyperTreeGrid* input, vtkDataObject* outputDO)
{
  vtkMultiProcessController* controller = vtkMultiProcessController::GetGlobalController();
  int numberOfProcesses = controller->GetNumberOfProcesses();

  vtkHyperTreeGrid* output = vtkHyperTreeGrid::SafeDownCast(outputDO);
  if (!output)
  {
    vtkErrorMacro("Incorrect type of output: " << outputDO->GetClassName());
    return 0;
  }

  vtkDebugMacro(<< "Start processing trees: copy input structure");
  output->Initialize();
  if (numberOfProcesses == 1)
  {
    // No ghost cells to generate if we have a single process, pass through
    output->ShallowCopy(input);
    return 1;
  }
  else
  {
    output->CopyEmptyStructure(input);
    output->GetCellData()->CopyStructure(input->GetCellData());
  }

  vtkBitArray* outputMask = input->HasMask() ? vtkBitArray::New() : nullptr;
  vtkIdType totalVertices = ::CopyInputHyperTreeToOutput(input, output, outputMask);
  this->UpdateProgress(0.1);

  unsigned int cellDims[3];
  input->GetCellDims(cellDims);
  int nbHTs = cellDims[0] * cellDims[1] * cellDims[2];

  HTGGContextContainer context{ controller, input, output, outputMask, nbHTs, totalVertices };

  // Create a vector containing the processId of each consecutive tree in the HTG.
  vtkDebugMacro("Broadcast tree locations");
  ::BroadcastTreeLocations(context);
  this->UpdateProgress(0.2);

  vtkDebugMacro("Determine neighbors");
  ::DetermineNeighbors(context);
  this->UpdateProgress(0.3);

  vtkDebugMacro("Exchange sizes with neighbors");
  if (::ExchangeSizes(context) == 0)
  {
    vtkErrorMacro("Failure during size exchange, aborting.");
    return 0;
  }
  controller->Barrier();
  this->UpdateProgress(0.4);

  vtkDebugMacro("Exchange masks with neighbors");
  if (::ExchangeMasks(context) == 0)
  {
    vtkErrorMacro("Failure during mask exchange, aborting.");
    return 0;
  }
  controller->Barrier();
  this->UpdateProgress(0.6);

  vtkDebugMacro("Exchange cell data with neighbors");
  if (::ExchangeCellData(context) == 0)
  {
    vtkErrorMacro("Failure during cell data exchange, aborting.");

    return 0;
  }
  controller->Barrier();
  this->UpdateProgress(0.8);

  vtkDebugMacro("Create ghost array and set output mask");
  ::AppendGhostArray(context);
  output->SetMask(outputMask);

  this->UpdateProgress(1.);
  return 1;
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridGhostCellsGenerator::ExtractInterface(
  vtkHyperTreeGridNonOrientedCursor* inCursor, vtkBitArray* isParent,
  std::vector<vtkIdType>& indices, vtkHyperTreeGrid* grid, unsigned int mask, vtkIdType& pos)
{
  ::ExtractInterface(inCursor, isParent, indices, grid, mask, pos);
}

//------------------------------------------------------------------------------
vtkIdType vtkHyperTreeGridGhostCellsGenerator::CreateGhostTree(
  vtkHyperTreeGridNonOrientedCursor* outCursor, vtkBitArray* isParent, vtkIdType* indices,
  vtkIdType&& pos)
{
  // Implementation moved to anonymous namespace
  return ::CreateGhostTree(outCursor, isParent, indices, std::forward<vtkIdType&&>(pos));
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridGhostCellsGenerator::CopyInputTreeToOutput(
  vtkHyperTreeGridNonOrientedCursor* inCursor, vtkHyperTreeGridNonOrientedCursor* outCursor,
  vtkCellData* inCellData, vtkCellData* outCellData, vtkBitArray* inMask, vtkBitArray* outMask)
{
  // Implementation moved to anonymous namespace
  ::CopyInputTreeToOutput(inCursor, outCursor, inCellData, outCellData, inMask, outMask);
}

VTK_ABI_NAMESPACE_END
