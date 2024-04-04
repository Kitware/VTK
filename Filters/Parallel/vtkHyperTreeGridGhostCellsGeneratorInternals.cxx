// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkHyperTreeGridGhostCellsGeneratorInternals.h"

VTK_ABI_NAMESPACE_BEGIN
namespace
{

/**
 * Probe for the given tag.
 * Return the an iterator to the item in the map corresponding to the rank sending the probed tag.
 */
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
  vtkBitArray* isMasked, vtkBitArray* outputMask, vtkIdType* indices, vtkIdType&& pos = 0)
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
      ::CreateGhostTree(
        outCursor, isParent, isMasked, outputMask, indices, std::forward<vtkIdType&&>(pos));
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

const int HTGGCG_SIZE_EXCHANGE_TAG = 5098;
const int HTGGCG_DATA_EXCHANGE_TAG = 5099;
const int HTGGCG_DATA2_EXCHANGE_TAG = 5100;
}

//------------------------------------------------------------------------------
vtkHyperTreeGridGhostCellsGeneratorInternals::vtkHyperTreeGridGhostCellsGeneratorInternals(
  vtkHyperTreeGridGhostCellsGenerator* self, vtkMultiProcessController* controller,
  vtkHyperTreeGrid* inputHTG, vtkHyperTreeGrid* outputHTG, vtkBitArray* outputMask,
  vtkIdType totalVertices)
  : Self(self)
  , Controller(controller)
  , InputHTG(inputHTG)
  , OutputHTG(outputHTG)
  , OutputMask(outputMask)
  , NumberOfVertices(totalVertices)
{
  unsigned int cellDims[3];
  this->InputHTG->GetCellDims(cellDims);
  this->HyperTreesMapToProcesses.resize(cellDims[0] * cellDims[1] * cellDims[2]);
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
            vtkIdType neighbor = -1;
            this->InputHTG->GetIndexFromLevelZeroCoordinates(neighbor, i + ri, j + rj, 0);
            int id = this->HyperTreesMapToProcesses[neighbor];
            if (id >= 0 && id != this->Controller->GetLocalProcessId())
            {
              // Build a neighborhood mask to extract the interface in
              // ExtractInterface later on.
              // Same encoding as vtkHyperTreeGrid::GetChildMask
              this->SendBuffer[id][inTreeIndex].mask |= 1
                << (8 * sizeof(int) - 1 - (ri + 1 + (rj + 1) * 3));
              // Not receiving anything from this guy since we will send him stuff
              this->RecvBuffer[id][neighbor].count = 0;
              // Process not treated yet, yielding the flag
              this->Flags[id] = NOT_TREATED;
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
              if (id >= 0 && id != this->Controller->GetLocalProcessId())
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
  int processId = this->Controller->GetLocalProcessId();
  for (int id = 0; id < numberOfProcesses; ++id)
  {
    if (id != processId)
    {
      auto sendIt = this->SendBuffer.find(id);
      if (sendIt != this->SendBuffer.end())
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
            // Telling my neighbors how much data I will send later
            counts[cpt++] = sendTreeBuffer.count;
          }
        }
        vtkDebugWithObjectMacro(this->Self, "Send: data size to " << id);
        this->Controller->Send(counts.data(), cpt, id, HTGGCG_SIZE_EXCHANGE_TAG);
      }
    }
    else
    {
      // Receiving size info from my neighbors
      std::size_t iRecv = 0;
      for (auto itRecvBuffer = this->RecvBuffer.begin(); itRecvBuffer != this->RecvBuffer.end();
           ++itRecvBuffer)
      {
        auto targetRecvBuffer = itRecvBuffer;
        if (this->Controller->CanProbe())
        {
          targetRecvBuffer =
            ::ProbeFind(this->Controller, HTGGCG_SIZE_EXCHANGE_TAG, this->RecvBuffer);
          if (targetRecvBuffer == this->RecvBuffer.end())
          {
            vtkErrorWithObjectMacro(this->Self,
              "Reception probe on process " << processId << " failed on " << iRecv
                                            << "th iteration.");
            return 0;
          }
        }
        int process = targetRecvBuffer->first;
        auto&& recvTreeMap = targetRecvBuffer->second;
        std::vector<vtkIdType> counts(recvTreeMap.size());
        vtkDebugWithObjectMacro(this->Self, "Receive: data size from " << process);
        this->Controller->Receive(counts.data(), static_cast<vtkIdType>(recvTreeMap.size()),
          process, HTGGCG_SIZE_EXCHANGE_TAG);
        int cpt = 0;
        for (auto&& RecvBufferPair : recvTreeMap)
        {
          RecvBufferPair.second.count = counts[cpt++];
        }
        iRecv++;
      }
    }
  }
  return 1;
}

//------------------------------------------------------------------------------
int vtkHyperTreeGridGhostCellsGeneratorInternals::ExchangeTreeDecomposition()
{
  constexpr vtkIdType BITS_IN_UCHAR = 8;
  int numberOfProcesses = this->Controller->GetNumberOfProcesses();
  int processId = this->Controller->GetLocalProcessId();

  // Data size is doubled when we need to transfer isMasked bit array.
  // We store isParent and isMasked bit arrays in the sent buffer contiguously.
  vtkIdType maskFactor = this->InputHTG->HasMask() ? 2 : 1;
  vtkDebugWithObjectMacro(this->Self, "Mask factor: " << maskFactor);

  for (int id = 0; id < numberOfProcesses; ++id)
  {
    if (id != processId)
    {
      auto sendIt = this->SendBuffer.find(id);
      if (sendIt != this->SendBuffer.end())
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
            vtkIdType currentLen = sendTreeBuffer.count / BITS_IN_UCHAR + 1;

            buf.resize(totalLen + maskFactor * currentLen);
            memcpy(buf.data() + totalLen, sendTreeBuffer.isParent->GetPointer(0), currentLen);
            if (this->InputHTG->HasMask())
            {
              memcpy(buf.data() + totalLen + currentLen, sendTreeBuffer.isMasked->GetPointer(0),
                currentLen);
            }

            totalLen += currentLen * maskFactor;
          }
        }
        vtkDebugWithObjectMacro(this->Self, "Send mask data from " << processId << " to " << id);
        this->Controller->Send(buf.data(), totalLen, id, HTGGCG_DATA_EXCHANGE_TAG);
      }
    }
    else
    {
      // Receiving masks
      std::size_t iRecv = 0;
      for (auto itRecvBuffer = this->RecvBuffer.begin(); itRecvBuffer != this->RecvBuffer.end();
           ++itRecvBuffer)
      {
        auto targetRecvBuffer = itRecvBuffer;
        if (this->Controller->CanProbe())
        {
          targetRecvBuffer =
            ::ProbeFind(this->Controller, HTGGCG_DATA_EXCHANGE_TAG, this->RecvBuffer);
          if (targetRecvBuffer == this->RecvBuffer.end())
          {
            vtkErrorWithObjectMacro(this->Self,
              "Reception probe on process " << processId << " failed on " << iRecv
                                            << "th iteration.");
            return 0;
          }
        }
        int process = targetRecvBuffer->first;
        auto&& recvTreeMap = targetRecvBuffer->second;

        // If we have not dealt with process yet,
        // we prepare for receiving with appropriate length
        if (this->Flags[process] == NOT_TREATED)
        {
          vtkIdType bufferLength = 0;
          for (auto&& recvTreeBufferPair : recvTreeMap)
          {
            auto&& recvTreeBuffer = recvTreeBufferPair.second;
            if (recvTreeBuffer.count != 0)
            {
              // bit message is packed in unsigned char, getting the correct length of the message
              bufferLength += recvTreeBuffer.count / BITS_IN_UCHAR + 1;
            }
          }
          bufferLength *= maskFactor; // isParent + potentially isMasked data
          std::vector<unsigned char> buf(bufferLength);

          vtkDebugWithObjectMacro(this->Self, "Receive mask data from " << process);
          this->Controller->Receive(buf.data(), bufferLength, process, HTGGCG_DATA_EXCHANGE_TAG);

          // Distributing receive data among my trees, i.e. creating my ghost trees with this data
          // Remember: we only have the nodes / leaves at the inverface with our neighbor
          vtkIdType offset = 0;
          vtkNew<vtkHyperTreeGridNonOrientedCursor> outCursor;
          for (auto&& recvTreeBufferPair : recvTreeMap)
          {
            vtkIdType treeId = recvTreeBufferPair.first;
            auto&& recvTreeBuffer = recvTreeBufferPair.second;
            if (recvTreeBuffer.count != 0)
            {
              this->OutputHTG->InitializeNonOrientedCursor(outCursor, treeId, true);

              // Stealing ownership of buf in isParent/isMasked to have vtkBitArray interface
              vtkNew<vtkBitArray> isParent;
              isParent->SetArray(buf.data() + offset, recvTreeBuffer.count, 1);
              vtkSmartPointer<vtkBitArray> isMasked = nullptr;
              if (this->InputHTG->HasMask())
              {
                isMasked = vtkSmartPointer<vtkBitArray>::New();
                isMasked->SetArray(buf.data() + offset + recvTreeBuffer.count / BITS_IN_UCHAR + 1,
                  recvTreeBuffer.count, 1);
              }

              recvTreeBuffer.offset = this->NumberOfVertices;
              recvTreeBuffer.indices.resize(recvTreeBuffer.count);

              outCursor->SetGlobalIndexStart(this->NumberOfVertices);

              this->NumberOfVertices += ::CreateGhostTree(
                outCursor, isParent, isMasked, this->OutputMask, recvTreeBuffer.indices.data());

              offset += (recvTreeBuffer.count / BITS_IN_UCHAR + 1) * maskFactor;
            }
          }
          this->Flags[process] = INITIALIZE_TREE;
        }
        iRecv++;
      }
    }
  }
  return 1;
}

//------------------------------------------------------------------------------
int vtkHyperTreeGridGhostCellsGeneratorInternals::ExchangeCellData()
{
  int numberOfProcesses = this->Controller->GetNumberOfProcesses();
  int processId = this->Controller->GetLocalProcessId();

  for (int id = 0; id < numberOfProcesses; ++id)
  {
    if (id != processId)
    {
      vtkDebugWithObjectMacro(this->Self, "Begin sending cell data to process " << id);
      auto sendIt = this->SendBuffer.find(id);
      if (sendIt != this->SendBuffer.end())
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
            vtkDebugWithObjectMacro(this->Self,
              "Processing buffer with " << sendTreeBuffer.count << " elements for process " << id);
            vtkCellData* cellData = this->InputHTG->GetCellData();
            totalLength += sendTreeBuffer.count * ::GetNumberOfCellValues(cellData);
            buf.resize(totalLength);

            // Fill send buffer with array data
            for (int arrayId = 0; arrayId < cellData->GetNumberOfArrays(); ++arrayId)
            {
              vtkDataArray* inArray = cellData->GetArray(arrayId);
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
        this->Controller->Send(buf.data(), totalLength, id, HTGGCG_DATA2_EXCHANGE_TAG);
        vtkDebugWithObjectMacro(this->Self, "Done sending cell data to " << id);
      }
    }
    else
    {
      vtkDebugWithObjectMacro(this->Self, "Receiving cell data from the other processes");

      std::size_t iRecv = 0;
      for (auto itRecvBuffer = this->RecvBuffer.begin(); itRecvBuffer != this->RecvBuffer.end();
           ++itRecvBuffer)
      {
        auto targetRecvBuffer = itRecvBuffer;
        if (this->Controller->CanProbe())
        {
          targetRecvBuffer =
            ::ProbeFind(this->Controller, HTGGCG_DATA2_EXCHANGE_TAG, this->RecvBuffer);
          if (targetRecvBuffer == this->RecvBuffer.end())
          {
            vtkErrorWithObjectMacro(this->Self,
              "Reception probe on process " << processId << " failed on " << iRecv
                                            << "th iteration.");
            return 0;
          }
        }
        int process = targetRecvBuffer->first;
        vtkDebugWithObjectMacro(this->Self, "Begin receiving data from process " << process);
        auto&& recvTreeMap = targetRecvBuffer->second;
        if (this->Flags[process] == INITIALIZE_TREE)
        {
          vtkCellData* cellData = OutputHTG->GetCellData();

          // Compute total length to be received
          unsigned long totalLength = 0;
          for (auto&& recvTreeBufferPair : recvTreeMap)
          {
            totalLength += recvTreeBufferPair.second.count * ::GetNumberOfCellValues(cellData);
          }
          std::vector<double> buf(totalLength);

          Controller->Receive(buf.data(), totalLength, process, HTGGCG_DATA2_EXCHANGE_TAG);

          // Fill output arrays using data received
          vtkIdType readOffset = 0;
          for (auto&& recvTreeBufferPair : recvTreeMap)
          {
            auto&& recvTreeBuffer = recvTreeBufferPair.second;
            for (int arrayId = 0; arrayId < cellData->GetNumberOfArrays(); ++arrayId)
            {
              vtkDataArray* outArray = cellData->GetArray(arrayId);
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
          this->Flags[process] = INITIALIZE_FIELD;
        }
        iRecv++;
        vtkDebugWithObjectMacro(this->Self, "Done receiving data from process " << process);
      }
    }
  }
  return 1;
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridGhostCellsGeneratorInternals::AppendGhostArray(vtkIdType nonGhostVertices)
{
  vtkDebugWithObjectMacro(this->Self,
    "Adding ghost array: ghost from id " << nonGhostVertices << " to " << this->NumberOfVertices);

  vtkNew<vtkUnsignedCharArray> scalars;
  scalars->SetNumberOfComponents(1);
  scalars->SetName(vtkDataSetAttributes::GhostArrayName());
  scalars->SetNumberOfTuples(this->NumberOfVertices);

  for (vtkIdType ii = 0; ii < nonGhostVertices; ++ii)
  {
    scalars->InsertValue(ii, 0);
  }
  for (vtkIdType ii = nonGhostVertices; ii < this->NumberOfVertices; ++ii)
  {
    scalars->InsertValue(ii, 1);
  }
  this->OutputHTG->GetCellData()->AddArray(scalars);
}

VTK_ABI_NAMESPACE_END
