/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDIYGhostUtilities.txx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef vtkDIYGhostUtilities_txx
#define vtkDIYGhostUtilities_txx

#include "vtkDIYGhostUtilities.h"

#include "vtkCellData.h"
#include "vtkDIYExplicitAssigner.h"
#include "vtkDIYUtilities.h"
#include "vtkDataSetAttributes.h"
#include "vtkFieldData.h"
#include "vtkIdList.h"
#include "vtkLogger.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkUnsignedCharArray.h"

#include <limits>
#include <string>
#include <vector>

// clang-format off
#include "vtk_diy2.h"
#include VTK_DIY2(diy/assigner.hpp)
#include VTK_DIY2(diy/link.hpp)
#include VTK_DIY2(diy/master.hpp)
#include VTK_DIY2(diy/mpi.hpp)
#include VTK_DIY2(diy/reduce-operations.hpp)
// clang-format on

//============================================================================
template <>
struct vtkDIYGhostUtilities::DataSetTypeToBlockTypeConverter<vtkImageData>
{
  typedef ImageDataBlock BlockType;
};

//============================================================================
template <>
struct vtkDIYGhostUtilities::DataSetTypeToBlockTypeConverter<vtkRectilinearGrid>
{
  typedef RectilinearGridBlock BlockType;
};

//----------------------------------------------------------------------------
template <class DataSetT>
void vtkDIYGhostUtilities::ExchangeGhosts(diy::Master& master, std::vector<DataSetT*>& inputs)
{
  using BlockType = typename DataSetTypeToBlockTypeConverter<DataSetT>::BlockType;
  master.foreach ([&master, &inputs](BlockType* block, const diy::Master::ProxyWithLink& cp) {
    int myBlockId = cp.gid();
    int localId = master.lid(myBlockId);
    auto& input = inputs[localId];

    for (int id = 0; id < static_cast<int>(cp.link()->size()); ++id)
    {
      const diy::BlockID& blockId = cp.link()->target(id);

      vtkSmartPointer<vtkIdList> cellIds =
        vtkDIYGhostUtilities::ComputeInputInterfaceCellIds(block, blockId.gid, input);

      vtkNew<vtkCellData> cellData;
      vtkCellData* inputCellData = input->GetCellData();
      cellData->CopyStructure(inputCellData);
      inputCellData->GetField(cellIds, cellData);

      cp.enqueue<vtkFieldData*>(blockId, cellData);

      vtkSmartPointer<vtkIdList> pointIds =
        vtkDIYGhostUtilities::ComputeInputInterfacePointIds(block, blockId.gid, input);

      vtkNew<vtkPointData> pointData;
      vtkPointData* inputPointData = input->GetPointData();
      pointData->CopyStructure(inputPointData);
      inputPointData->GetField(pointIds, pointData);

      cp.enqueue<vtkFieldData*>(blockId, pointData);
    }
  });

  master.exchange();

  master.foreach ([](BlockType* block, const diy::Master::ProxyWithLink& cp) {
    std::vector<int> incoming;
    cp.incoming(incoming);
    for (const int& gid : incoming)
    {
      // we need this extra check because incoming is not empty when using only one block
      if (!cp.incoming(gid).empty())
      {
        vtkFieldData* cellData = nullptr;
        cp.dequeue<vtkFieldData*>(gid, cellData);
        block->CellDatas.emplace(gid, vtkSmartPointer<vtkFieldData>::Take(cellData));

        vtkFieldData* pointData = nullptr;
        cp.dequeue<vtkFieldData*>(gid, pointData);
        block->PointDatas.emplace(gid, vtkSmartPointer<vtkFieldData>::Take(pointData));
      }
    }
  });
}

//----------------------------------------------------------------------------
template <class DataSetT>
void vtkDIYGhostUtilities::InitializeGhostArrays(std::vector<DataSetT*>& outputs,
  std::vector<vtkSmartPointer<vtkUnsignedCharArray>>& ghostCellArrays,
  std::vector<vtkSmartPointer<vtkUnsignedCharArray>>& ghostPointArrays)
{
  ghostCellArrays.resize(outputs.size());
  ghostPointArrays.resize(outputs.size());
  for (int localId = 0; localId < static_cast<int>(outputs.size()); ++localId)
  {
    DataSetT* output = outputs[localId];
    vtkSmartPointer<vtkUnsignedCharArray>& ghostCellArray = ghostCellArrays[localId];
    vtkSmartPointer<vtkUnsignedCharArray>& ghostPointArray = ghostPointArrays[localId];

    ExtentType localExtent;
    output->GetExtent(localExtent.data());

    ghostCellArray = vtkArrayDownCast<vtkUnsignedCharArray>(
      output->GetGhostArray(vtkDataObject::FIELD_ASSOCIATION_CELLS));
    if (!ghostCellArray)
    {
      ghostCellArray = vtkSmartPointer<vtkUnsignedCharArray>::New();
      ghostCellArray->SetName(vtkDataSetAttributes::GhostArrayName());
      ghostCellArray->SetNumberOfComponents(1);
      ghostCellArray->SetNumberOfTuples(output->GetNumberOfCells());
    }
    ghostCellArray->Fill(0);

    ghostPointArray = vtkArrayDownCast<vtkUnsignedCharArray>(
      output->GetGhostArray(vtkDataObject::FIELD_ASSOCIATION_POINTS));
    if (!ghostPointArray)
    {
      ghostPointArray = vtkSmartPointer<vtkUnsignedCharArray>::New();
      ghostPointArray->SetName(vtkDataSetAttributes::GhostArrayName());
      ghostPointArray->SetNumberOfComponents(1);
      ghostPointArray->SetNumberOfTuples(output->GetNumberOfPoints());
    }
    ghostPointArray->Fill(0);
  }
}

//----------------------------------------------------------------------------
template <class DataSetT>
void vtkDIYGhostUtilities::FillDuplicateGhosts(const diy::Master& master,
  std::vector<DataSetT*>& outputs,
  std::vector<vtkSmartPointer<vtkUnsignedCharArray>>& ghostCellArrays,
  std::vector<vtkSmartPointer<vtkUnsignedCharArray>>& ghostPointArrays)
{
  using BlockType = typename DataSetTypeToBlockTypeConverter<DataSetT>::BlockType;
  for (int localId = 0; localId < static_cast<int>(outputs.size()); ++localId)
  {
    DataSetT* output = outputs[localId];
    vtkSmartPointer<vtkUnsignedCharArray>& ghostCellArray = ghostCellArrays[localId];
    vtkSmartPointer<vtkUnsignedCharArray>& ghostPointArray = ghostPointArrays[localId];

    BlockType* block = master.block<BlockType>(localId);

    BlockMapType<vtkSmartPointer<vtkFieldData>>& cellDatas = block->CellDatas;
    BlockMapType<vtkSmartPointer<vtkFieldData>>& pointDatas = block->PointDatas;
    const BlockMapType<typename BlockType::BlockStructureType>& blockStructures =
      block->BlockStructures;

    auto cellDataIt = cellDatas.begin();
    auto pointDataIt = pointDatas.begin();
    auto blockStructureIt = blockStructures.cbegin();

    vtkCellData* outputCellData = output->GetCellData();
    outputCellData->CopyFieldOff(vtkDataSetAttributes::GhostArrayName());
    vtkPointData* outputPointData = output->GetPointData();
    outputPointData->CopyFieldOff(vtkDataSetAttributes::GhostArrayName());

    for (; cellDataIt != cellDatas.end(); ++cellDataIt, ++pointDataIt, ++blockStructureIt)
    {
      vtkFieldData* cellData = cellDataIt->second;
      vtkFieldData* pointData = pointDataIt->second;
      int gid = blockStructureIt->first;

      vtkSmartPointer<vtkIdList> cellIds =
        vtkDIYGhostUtilities::ComputeOutputInterfaceCellIds(block, gid, output);

      for (vtkIdType i = 0; i < cellIds->GetNumberOfIds(); ++i)
      {
        vtkIdType cellId = cellIds->GetId(i);
        outputCellData->SetTuple(cellId, i, cellData);
        ghostCellArray->SetValue(cellId, vtkDataSetAttributes::CellGhostTypes::DUPLICATECELL);
      }

      vtkSmartPointer<vtkIdList> pointIds =
        vtkDIYGhostUtilities::ComputeOutputInterfacePointIds(block, gid, output);

      for (vtkIdType i = 0; i < pointIds->GetNumberOfIds(); ++i)
      {
        vtkIdType pointId = pointIds->GetId(i);
        outputPointData->SetTuple(pointId, i, pointData);
        ghostPointArray->SetValue(pointId, vtkDataSetAttributes::PointGhostTypes::DUPLICATEPOINT);
      }
    }
  }
}

//----------------------------------------------------------------------------
/**
 * Main pipeline generating ghosts.
 */
template <class DataSetT>
int vtkDIYGhostUtilities::GenerateGhostCells(std::vector<DataSetT*>& inputs,
  std::vector<DataSetT*>& outputs, int inputGhostLevels, int outputGhostLevels,
  vtkMultiProcessController* controller)
{
  using BlockType = typename DataSetTypeToBlockTypeConverter<DataSetT>::BlockType;

  const int size = static_cast<int>(inputs.size());
  if (!size)
  {
    return 1;
  }
  if (size != static_cast<int>(outputs.size()))
  {
    vtkLog(ERROR, "inputs and outputs have different sizes for " << inputs[0]->GetClassName());
    return 0;
  }

  std::string logMessage =
    std::string("Generating ghosts for ") + std::string(inputs[0]->GetClassName());
  vtkLogStartScope(TRACE, logMessage.c_str());

  for (int i = 0; i < size; ++i)
  {
    outputs[i]->CopyStructure(inputs[i]);
  }

  vtkLogStartScope(TRACE, "Instantiating diy communicator");
  diy::mpi::communicator comm = vtkDIYUtilities::GetCommunicator(controller);
  vtkLogEndScope("Instantiating diy communicator");

  vtkLogStartScope(TRACE, "Instantiating master");
  diy::Master master(
    comm, 1, -1, []() { return static_cast<void*>(new BlockType()); },
    [](void* b) -> void { delete static_cast<BlockType*>(b); });
  vtkLogEndScope("Instantiating master");

  vtkLogStartScope(TRACE, "Instantiating assigner");
  vtkDIYExplicitAssigner assigner(comm, size);
  vtkLogEndScope("Instantiating assigner");

  vtkLogStartScope(TRACE, "Decomposing master");
  diy::RegularDecomposer<diy::DiscreteBounds> decomposer(
    /*dim*/ 1, diy::interval(0, comm.size() - 1), comm.size());
  decomposer.decompose(comm.rank(), assigner, master);
  vtkLogEndScope("Decomposing master");

  vtkLogStartScope(TRACE, "Exchanging block structures");
  vtkDIYGhostUtilities::ExchangeBlockStructures(master, assigner, inputs, inputGhostLevels);
  vtkLogEndScope("Exchanging block structures");

  vtkLogStartScope(TRACE, "Creating link map between connected blocks");
  LinkMap linkMap = vtkDIYGhostUtilities::ComputeLinkMapAndAllocateGhosts(
    master, inputs, outputs, outputGhostLevels);
  vtkLogEndScope("Creating link map between connected blocks");

  vtkLogStartScope(TRACE, "Relinking blocks using link map");
  vtkDIYUtilities::Link<BlockType>(master, assigner, linkMap);
  vtkLogEndScope("Relinking blocks using link map");

  vtkLogStartScope(TRACE, "Exchanging ghost data between blocks");
  vtkDIYGhostUtilities::ExchangeGhosts(master, inputs);
  vtkLogEndScope("Exchanging ghost data between blocks");

  std::vector<vtkSmartPointer<vtkUnsignedCharArray>> ghostCellArrays;
  std::vector<vtkSmartPointer<vtkUnsignedCharArray>> ghostPointArrays;

  vtkLogStartScope(TRACE, "Initializing ghost arrays in outputs");
  vtkDIYGhostUtilities::InitializeGhostArrays(outputs, ghostCellArrays, ghostPointArrays);
  vtkLogEndScope("Initializing ghost arrays in outputs");

  vtkLogStartScope(TRACE, "Filling local ghosts with received data from other blocks");
  vtkDIYGhostUtilities::FillGhostArrays(master, outputs, ghostCellArrays, ghostPointArrays);
  vtkLogEndScope("Filling local ghosts with received data from other blocks");

  for (int localId = 0; localId < static_cast<int>(outputs.size()); ++localId)
  {
    DataSetT*& output = outputs[localId];
    output->GetCellData()->AddArray(ghostCellArrays[localId]);
    output->GetPointData()->AddArray(ghostPointArrays[localId]);
    output->FastDelete();
  }

  vtkLogEndScope(logMessage.c_str());

  return 1;
}

#endif
