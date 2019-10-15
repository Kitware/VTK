/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHyperTreeGridGhostCellsGenerator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkHyperTreeGridGhostCellsGenerator.h"

#include "vtkBitArray.h"
#include "vtkCellData.h"
#include "vtkCommunicator.h"
#include "vtkHyperTree.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridNonOrientedCursor.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkUniformHyperTreeGrid.h"
#include "vtkUnsignedCharArray.h"

#include <cmath>
#include <map>
#include <unordered_map>
#include <utility>
#include <vector>

vtkStandardNewMacro(vtkHyperTreeGridGhostCellsGenerator);

struct vtkHyperTreeGridGhostCellsGenerator::vtkInternals
{
  // Controller only has MPI processes which have cells
  vtkMultiProcessController* Controller;
};

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

static const int HTGGCG_SIZE_EXCHANGE_TAG = 5098;
static const int HTGGCG_DATA_EXCHANGE_TAG = 5099;
static const int HTGGCG_DATA2_EXCHANGE_TAG = 5100;
}

//-----------------------------------------------------------------------------
vtkHyperTreeGridGhostCellsGenerator::vtkHyperTreeGridGhostCellsGenerator()
{
  this->AppropriateOutput = true;
  this->Internals = new vtkInternals();
  this->Internals->Controller = vtkMultiProcessController::GetGlobalController();
}

//-----------------------------------------------------------------------------
vtkHyperTreeGridGhostCellsGenerator::~vtkHyperTreeGridGhostCellsGenerator()
{
  delete this->Internals;
}

//----------------------------------------------------------------------------
void vtkHyperTreeGridGhostCellsGenerator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
int vtkHyperTreeGridGhostCellsGenerator::FillOutputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkHyperTreeGrid");
  return 1;
}

//-----------------------------------------------------------------------------
int vtkHyperTreeGridGhostCellsGenerator::ProcessTrees(
  vtkHyperTreeGrid* input, vtkDataObject* outputDO)
{
  // Downcast output data object to hyper tree grid
  vtkHyperTreeGrid* output = vtkHyperTreeGrid::SafeDownCast(outputDO);
  if (!output)
  {
    vtkErrorMacro("Incorrect type of output: " << outputDO->GetClassName());
    return 0;
  }

  // We only need the structure of the input with no data in it
  output->Initialize();

  // local handle on the controller
  vtkMultiProcessController* controller = this->Internals->Controller;

  int processId = controller->GetLocalProcessId();

  int numberOfProcesses = controller->GetNumberOfProcesses();
  if (numberOfProcesses == 1)
  {
    output->DeepCopy(input);
    return 1;
  }
  else
  {
    output->CopyEmptyStructure(input);
  }

  // Link HyperTrees
  vtkHyperTreeGrid::vtkHyperTreeGridIterator inHTs;
  input->InitializeTreeIterator(inHTs);
  vtkIdType inTreeIndex;

  // To keep track of the number of nodes in the htg
  vtkIdType numberOfValues = 0;

  vtkNew<vtkHyperTreeGridNonOrientedCursor> outCursor, inCursor;

  vtkBitArray *outputMask = input->HasMask() ? vtkBitArray::New() : nullptr,
              *inputMask = input->HasMask() ? input->GetMask() : nullptr;

  // First, we copy the input htg into the output
  // We do it "by hand" to fill gaps if they exist
  while (inHTs.GetNextTree(inTreeIndex))
  {
    input->InitializeNonOrientedCursor(inCursor, inTreeIndex);
    output->InitializeNonOrientedCursor(outCursor, inTreeIndex, true);
    outCursor->SetGlobalIndexStart(numberOfValues);
    this->CopyInputTreeToOutput(
      inCursor, outCursor, input->GetPointData(), output->GetPointData(), inputMask, outputMask);
    numberOfValues += outCursor->GetTree()->GetNumberOfVertices();
  }

  // Handling receive and send buffer.
  // The structure is as follows:
  // sendBuffer[id] or recvBuffer[id] == process id of neighbor with who to communicate buffer
  // sendBuffer[id][jd] or recvBuffer[id][jd] tells which tree index is being sent.
  typedef std::map<unsigned int, SendBuffer> SendTreeBufferMap;
  typedef std::map<unsigned int, SendTreeBufferMap> SendProcessBufferMap;
  typedef std::map<unsigned int, RecvBuffer> RecvTreeBufferMap;
  typedef std::map<unsigned int, RecvTreeBufferMap> RecvProcessBufferMap;

  SendProcessBufferMap sendBuffer;
  RecvProcessBufferMap recvBuffer;

  enum FlagType
  {
    NOT_TREATED,
    INITIALIZE_TREE,
    INITIALIZE_FIELD
  };
  std::unordered_map<unsigned, FlagType> flags;

  // Broadcast hyper tree locations to everyone
  unsigned cellDims[3];
  input->GetCellDims(cellDims);
  vtkIdType nbHTs = cellDims[0] * cellDims[1] * cellDims[2];

  std::vector<int> broadcastHyperTreesMapToProcesses(nbHTs + numberOfProcesses, -1),
    hyperTreesMapToProcesses(nbHTs + numberOfProcesses);
  input->InitializeTreeIterator(inHTs);
  while (inHTs.GetNextTree(inTreeIndex))
  {
    input->InitializeNonOrientedCursor(inCursor, inTreeIndex);
    if (inCursor->HasTree())
    {
      broadcastHyperTreesMapToProcesses[inTreeIndex] = processId;
    }
  }
  broadcastHyperTreesMapToProcesses[nbHTs + processId] = input->HasMask();
  controller->AllReduce(broadcastHyperTreesMapToProcesses.data(), hyperTreesMapToProcesses.data(),
    nbHTs + numberOfProcesses, vtkCommunicator::MAX_OP);

  assert(input->GetDimension() > 1);

  // Determining who are my neighbors
  unsigned i, j, k = 0;
  input->InitializeTreeIterator(inHTs);
  switch (input->GetDimension())
  {
    case 2:
    {
      while (inHTs.GetNextTree(inTreeIndex))
      {
        input->GetLevelZeroCoordinatesFromIndex(inTreeIndex, i, j, k);
        // Avoiding over / under flowing the grid
        for (int rj = ((j > 0) ? -1 : 0); rj < (((j + 1) < cellDims[1]) ? 2 : 1); ++rj)
        {
          for (int ri = ((i > 0) ? -1 : 0); ri < (((i + 1) < cellDims[0]) ? 2 : 1); ++ri)
          {
            int neighbor = (i + ri) * cellDims[1] + j + rj;
            int id = hyperTreesMapToProcesses[neighbor];
            if (id >= 0 && id != processId)
            {
              // Construction a neighborhood mask to extract the interface in ExtractInterface later
              // on Same encoding as vtkHyperTreeGrid::GetChildMask
              sendBuffer[id][inTreeIndex].mask |= 1
                << (8 * sizeof(int) - 1 - (ri + 1 + (rj + 1) * 3));
              // Not receiving anything from this guy since we will send him stuff
              recvBuffer[id][neighbor].count = 0;
              // Process not treated yet, yielding the flag
              flags[id] = NOT_TREATED;
            }
          }
        }
      }
      break;
    }
    case 3:
    {
      while (inHTs.GetNextTree(inTreeIndex))
      {
        input->GetLevelZeroCoordinatesFromIndex(inTreeIndex, i, j, k);
        // Avoiding over / under flowing the grid
        for (int rk = ((k > 0) ? -1 : 0); rk < (((k + 1) < cellDims[2]) ? 2 : 1); ++rk)
        {
          for (int rj = ((j > 0) ? -1 : 0); rj < (((j + 1) < cellDims[1]) ? 2 : 1); ++rj)
          {
            for (int ri = ((i > 0) ? -1 : 0); ri < (((i + 1) < cellDims[0]) ? 2 : 1); ++ri)
            {
              int neighbor = ((k + rk) * cellDims[1] + j + rj) * cellDims[0] + i + ri;
              int id = hyperTreesMapToProcesses[neighbor];
              if (id >= 0 && id != processId)
              {
                // Construction a neighborhood mask to extract the interface in ExtractInterface
                // later on Same encoding as vtkHyperTreeGrid::GetChildMask
                sendBuffer[id][inTreeIndex].mask |= 1
                  << (8 * sizeof(int) - 1 - (ri + 1 + (rj + 1) * 3 + (rk + 1) * 9));
                // Not receiving anything from this guy since we will send him stuff
                recvBuffer[id][neighbor].count = 0;
                // Process not treated yet, yielding the flag
                flags[id] = NOT_TREATED;
              }
            }
          }
        }
      }
      break;
    }
  }

  // Exchanging size with my neighbors
  for (int id = 0; id < numberOfProcesses; ++id)
  {
    if (id != processId)
    {
      auto sendIt = sendBuffer.find(id);
      if (sendIt != sendBuffer.end())
      {
        SendTreeBufferMap& sendTreeMap = sendIt->second;
        std::vector<vtkIdType> counts(sendTreeMap.size());
        int cpt = 0;
        for (auto&& sendTreeBufferPair : sendTreeMap)
        {
          vtkIdType treeId = sendTreeBufferPair.first;
          auto&& sendTreeBuffer = sendTreeBufferPair.second;
          input->InitializeNonOrientedCursor(inCursor, treeId);
          // Extracting the tree interface with its neighbors
          sendTreeBuffer.count = 0;
          vtkHyperTree* tree = inCursor->GetTree();
          if (tree)
          {
            // We store the isParent profile along the interface to know when to subdivide later
            // indices store the indices in the input of the nodes on the interface
            vtkIdType nbVertices = tree->GetNumberOfVertices();
            sendTreeBuffer.indices.resize(nbVertices);
            ExtractInterface(inCursor, sendTreeBuffer.isParent, sendTreeBuffer.indices, input,
              sendTreeBuffer.mask, sendTreeBuffer.count);
          }
          // Telling my neighbors how much data I will send later
          counts[cpt++] = sendTreeBuffer.count;
        }
        controller->Send(counts.data(), cpt, id, HTGGCG_SIZE_EXCHANGE_TAG);
      }
    }
    else
    {
      // Receiving size info from my neighbors
      for (auto&& recvTreeMapPair : recvBuffer)
      {
        unsigned process = recvTreeMapPair.first;
        auto&& recvTreeMap = recvTreeMapPair.second;
        std::vector<vtkIdType> counts(recvTreeMap.size());
        controller->Receive(counts.data(), static_cast<vtkIdType>(recvTreeMap.size()), process,
          HTGGCG_SIZE_EXCHANGE_TAG);
        int cpt = 0;
        for (auto&& recvBufferPair : recvTreeMap)
        {
          recvBufferPair.second.count = counts[cpt++];
        }
      }
    }
  }

  // Synchronizing
  controller->Barrier();

  // Sending masks and parent state of each node
  for (int id = 0; id < numberOfProcesses; ++id)
  {
    if (id != processId)
    {
      auto sendIt = sendBuffer.find(id);
      if (sendIt != sendBuffer.end())
      {
        SendTreeBufferMap& sendTreeMap = sendIt->second;
        std::vector<unsigned char> buf;
        // Accumulated length
        vtkIdType len = 0;
        for (auto&& sendTreeBufferPair : sendTreeMap)
        {
          auto&& sendTreeBuffer = sendTreeBufferPair.second;
          if (sendTreeBuffer.count)
          {
            // We send the bits packed in unsigned char
            vtkIdType dlen = sendTreeBuffer.count / sizeof(unsigned char) + 1;
            if (input->HasMask())
            {
              dlen *= 2;
            }
            buf.resize(len + dlen);
            memcpy(buf.data() + len, sendTreeBuffer.isParent->GetPointer(0),
              sendTreeBuffer.count / sizeof(unsigned char) + 1);
            if (input->HasMask())
            {
              unsigned char* mask = buf.data() + len + dlen / 2;
              for (vtkIdType ii = 0; ii < dlen / 2; ++ii)
              {
                mask[ii] = 0;
              }
              vtkBitArray* bmask = input->GetMask();
              // Filling the mask with bits at the appropriate location
              for (vtkIdType m = 0; m < sendTreeBuffer.count; ++m)
              {
                *mask |= static_cast<unsigned char>(bmask->GetValue(sendTreeBuffer.indices[m]))
                  << (sizeof(unsigned char) - 1 - (m % sizeof(unsigned char)));
                // Incrementing the pointer when unsigned char overflows
                mask += !((m + 1) % sizeof(unsigned char));
              }
            }
            len += dlen;
          }
        }
        this->Internals->Controller->Send(buf.data(), len, id, HTGGCG_DATA_EXCHANGE_TAG);
      }
    }
    else
    {
      // Receiving masks
      for (auto&& recvTreeMapPair : recvBuffer)
      {
        unsigned process = recvTreeMapPair.first;
        auto&& recvTreeMap = recvTreeMapPair.second;

        // If we have not dealt with process yet,
        // we prepare for receiving with appropriate length
        if (flags[process] == NOT_TREATED)
        {
          vtkIdType len = 0;
          for (auto&& recvTreeBufferPair : recvTreeMap)
          {
            auto&& recvTreeBuffer = recvTreeBufferPair.second;
            if (recvTreeBuffer.count != 0)
            {
              // bit message is packed in unsigned char, getting the correct length of the message
              len += recvTreeBuffer.count / sizeof(unsigned char) + 1;
              if (input->HasMask())
              {
                len += recvTreeBuffer.count / sizeof(unsigned char) + 1;
              }
            }
          }
          std::vector<unsigned char> buf(len);

          this->Internals->Controller->Receive(buf.data(), len, process, HTGGCG_DATA_EXCHANGE_TAG);

          vtkIdType cpt = 0;
          // Distributing receive data among my trees, i.e. creating my ghost trees with this data
          // Remember: we only have the nodes / leaves at the inverface with our neighbor
          for (auto&& recvTreeBufferPair : recvTreeMap)
          {
            vtkIdType treeId = recvTreeBufferPair.first;
            auto&& recvTreeBuffer = recvTreeBufferPair.second;
            if (recvTreeBuffer.count != 0)
            {
              output->InitializeNonOrientedCursor(outCursor, treeId, true);
              vtkNew<vtkBitArray> isParent;

              // Stealing ownership of buf in isParent to have vtkBitArray interface
              isParent->SetArray(buf.data() + cpt,
                input->HasMask() ? 2 * recvTreeBuffer.count : recvTreeBuffer.count, 1);

              recvTreeBuffer.offset = numberOfValues;
              recvTreeBuffer.indices.resize(recvTreeBuffer.count);

              outCursor->SetGlobalIndexStart(numberOfValues);

              if (!outputMask && hyperTreesMapToProcesses[nbHTs + process])
              {
                outputMask = vtkBitArray::New();
                outputMask->Resize(numberOfValues);
                for (vtkIdType ii = 0; ii < numberOfValues; ++ii)
                {
                  outputMask->SetValue(ii, 0);
                }
              }

              numberOfValues +=
                this->CreateGhostTree(outCursor, isParent, recvTreeBuffer.indices.data());

              // TODO Bug potentiel si localement on n'a pas de masque... mais le voisin si :D
              if (hyperTreesMapToProcesses[nbHTs + process])
              {
                vtkNew<vtkBitArray> mask;
                // Stealing ownership of buf for mask handling to have vtkBitArray interface

                mask->SetArray(buf.data() + cpt + recvTreeBuffer.count / sizeof(unsigned char) + 1,
                  recvTreeBuffer.count, 1);

                for (vtkIdType m = 0; m < recvTreeBuffer.count; ++m)
                {
                  outputMask->InsertValue(recvTreeBuffer.indices[m], mask->GetValue(m));
                }
                cpt += 2 * (recvTreeBuffer.count / sizeof(unsigned char) + 1);
              }
              else
              {
                if (outputMask)
                {
                  for (vtkIdType m = 0; m < recvTreeBuffer.count; ++m)
                  {
                    outputMask->InsertValue(recvTreeBuffer.indices[m], 0);
                  }
                }
                cpt += recvTreeBuffer.count / sizeof(unsigned char) + 1;
              }
            }
          }
          flags[process] = INITIALIZE_TREE;
        }
      }
    }
  }

  // Synchronizing
  this->Internals->Controller->Barrier();

  // We now send the data store on each node
  for (int id = 0; id < numberOfProcesses; ++id)
  {
    if (id != processId)
    {
      auto sendIt = sendBuffer.find(id);
      if (sendIt != sendBuffer.end())
      {
        SendTreeBufferMap& sendTreeMap = sendIt->second;
        std::vector<double> buf;
        vtkIdType len = 0;
        for (auto&& sendTreeBufferPair : sendTreeMap)
        {
          auto&& sendTreeBuffer = sendTreeBufferPair.second;
          if (sendTreeBuffer.count)
          {
            vtkPointData* pd = input->GetPointData();
            int nbArray = pd->GetNumberOfArrays();
            len += sendTreeBuffer.count * nbArray;
            buf.resize(len);
            double* arr = buf.data() + len - sendTreeBuffer.count * nbArray;
            for (int iArray = 0; iArray < nbArray; ++iArray)
            {
              vtkDataArray* inArray = pd->GetArray(iArray);
              for (vtkIdType m = 0; m < sendTreeBuffer.count; ++m)
              {
                arr[iArray * sendTreeBuffer.count + m] =
                  inArray->GetTuple1(sendTreeBuffer.indices[m]);
              }
            }
          }
        }
        this->Internals->Controller->Send(buf.data(), len, id, HTGGCG_DATA2_EXCHANGE_TAG);
      }
    }
    else
    {
      // We receive the data
      for (auto&& recvTreeMapPair : recvBuffer)
      {
        unsigned process = recvTreeMapPair.first;
        auto&& recvTreeMap = recvTreeMapPair.second;
        if (flags[process] == INITIALIZE_TREE)
        {
          unsigned long len = 0;
          for (auto&& recvTreeBufferPair : recvTreeMap)
          {
            vtkPointData* pd = output->GetPointData();
            int nbArray = pd->GetNumberOfArrays();
            len += recvTreeBufferPair.second.count * nbArray;
          }
          std::vector<double> buf(len);

          this->Internals->Controller->Receive(buf.data(), len, process, HTGGCG_DATA2_EXCHANGE_TAG);
          vtkIdType cpt = 0;

          for (auto&& recvTreeBufferPair : recvTreeMap)
          {
            auto&& recvTreeBuffer = recvTreeBufferPair.second;
            vtkPointData* pd = output->GetPointData();
            int nbArray = pd->GetNumberOfArrays();
            double* arr = buf.data() + cpt;

            for (int d = 0; d < nbArray; ++d)
            {
              vtkDataArray* outArray = pd->GetArray(d);
              for (vtkIdType m = 0; m < recvTreeBuffer.count; ++m)
              {
                outArray->InsertTuple1(
                  recvTreeBuffer.indices[m], arr[d * recvTreeBuffer.count + m]);
              }
            }
            cpt += recvTreeBuffer.count * nbArray;
          }
          flags[process] = INITIALIZE_FIELD;
        }
      }
    }
  }

  this->Internals->Controller->Barrier();
  {
    vtkNew<vtkUnsignedCharArray> scalars;
    scalars->SetNumberOfComponents(1);
    scalars->SetName(vtkDataSetAttributes::GhostArrayName());
    scalars->SetNumberOfTuples(numberOfValues);
    for (vtkIdType ii = 0; ii < input->GetNumberOfVertices(); ++ii)
    {
      scalars->InsertValue(ii, 0);
    }
    for (vtkIdType ii = input->GetNumberOfVertices(); ii < numberOfValues; ++ii)
    {
      scalars->InsertValue(ii, 1);
    }
    output->GetPointData()->AddArray(scalars);
    output->SetMask(outputMask);
  }

  this->UpdateProgress(1.);
  return 1;
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGridGhostCellsGenerator::ExtractInterface(
  vtkHyperTreeGridNonOrientedCursor* inCursor, vtkBitArray* isParent,
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
        this->ExtractInterface(inCursor, isParent, indices, grid, newMask, pos);
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

//-----------------------------------------------------------------------------
vtkIdType vtkHyperTreeGridGhostCellsGenerator::CreateGhostTree(
  vtkHyperTreeGridNonOrientedCursor* outCursor, vtkBitArray* isParent, vtkIdType* indices,
  vtkIdType&& pos)
{
  indices[pos] = outCursor->GetGlobalNodeIndex();
  if (isParent->GetValue(pos++))
  {
    outCursor->SubdivideLeaf();
    for (int ichild = 0; ichild < outCursor->GetNumberOfChildren(); ++ichild)
    {
      outCursor->ToChild(ichild);
      this->CreateGhostTree(outCursor, isParent, indices, std::forward<vtkIdType&&>(pos));
      outCursor->ToParent();
    }
  }
  return pos;
}

//-----------------------------------------------------------------------------
void vtkHyperTreeGridGhostCellsGenerator::CopyInputTreeToOutput(
  vtkHyperTreeGridNonOrientedCursor* inCursor, vtkHyperTreeGridNonOrientedCursor* outCursor,
  vtkPointData* inPointData, vtkPointData* outPointData, vtkBitArray* inMask, vtkBitArray* outMask)
{
  vtkIdType outIdx = outCursor->GetGlobalNodeIndex(), inIdx = inCursor->GetGlobalNodeIndex();
  outPointData->InsertTuple(outIdx, inIdx, inPointData);
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
      this->CopyInputTreeToOutput(inCursor, outCursor, inPointData, outPointData, inMask, outMask);
      outCursor->ToParent();
      inCursor->ToParent();
    }
  }
}
