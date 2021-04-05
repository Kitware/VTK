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
#include "vtkCommunicator.h"
#include "vtkDIYExplicitAssigner.h"
#include "vtkDIYUtilities.h"
#include "vtkDataSetAttributes.h"
#include "vtkFieldData.h"
#include "vtkIdList.h"
#include "vtkLogger.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkUnsignedCharArray.h"

#include <limits>
#include <numeric>
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

//============================================================================
template <>
struct vtkDIYGhostUtilities::DataSetTypeToBlockTypeConverter<vtkStructuredGrid>
{
  typedef StructuredGridBlock BlockType;
};

//----------------------------------------------------------------------------
template <class DataSetT>
void vtkDIYGhostUtilities::GhostBuffersHelper<DataSetT>::Enqueue(
  const diy::Master::ProxyWithLink& cp, const diy::BlockID& blockId, DataSetT* input,
  BlockType* block)
{
  vtkSmartPointer<vtkIdList> cellIds =
    vtkDIYGhostUtilities::ComputeInputInterfaceCellIds(block, blockId.gid, input);
  GhostBuffersType::EnqueueCellBuffers(cp, blockId, input, cellIds);

  vtkSmartPointer<vtkIdList> pointIds =
    vtkDIYGhostUtilities::ComputeInputInterfacePointIds(block, blockId.gid, input);
  GhostBuffersType::EnqueuePointBuffers(cp, blockId, input, pointIds);
}

//----------------------------------------------------------------------------
template <class DataSetT>
void vtkDIYGhostUtilities::ImplicitGeometryGhosts<DataSetT>::EnqueuePointBuffers(
  const diy::Master::ProxyWithLink& cp, const diy::BlockID& blockId, DataSetT* input,
  vtkIdList* pointIds)
{
  vtkNew<vtkPointData> pointData;
  vtkPointData* inputPointData = input->GetPointData();
  pointData->CopyStructure(inputPointData);
  inputPointData->GetField(pointIds, pointData);

  cp.enqueue<vtkFieldData*>(blockId, pointData);
}

//----------------------------------------------------------------------------
template <class DataSetT>
void vtkDIYGhostUtilities::ImplicitGeometryGhosts<DataSetT>::EnqueueCellBuffers(
  const diy::Master::ProxyWithLink& cp, const diy::BlockID& blockId, DataSetT* input,
  vtkIdList* cellIds)
{
  vtkNew<vtkCellData> cellData;
  vtkCellData* inputCellData = input->GetCellData();
  cellData->CopyStructure(inputCellData);
  inputCellData->GetField(cellIds, cellData);

  cp.enqueue<vtkFieldData*>(blockId, cellData);
}

//----------------------------------------------------------------------------
template <class DataSetT>
void vtkDIYGhostUtilities::ExplicitPointGeometryGhosts<DataSetT>::EnqueuePointBuffers(
  const diy::Master::ProxyWithLink& cp, const diy::BlockID& blockId, DataSetT* input,
  vtkIdList* pointIds)
{
  ExplicitPointGeometryGhosts::Superclass::EnqueuePointBuffers(cp, blockId, input, pointIds);

  vtkDataArray* inputPoints = input->GetPoints()->GetData();
  vtkSmartPointer<vtkDataArray> points(
    vtkSmartPointer<vtkDataArray>::Take(inputPoints->NewInstance()));
  points->SetNumberOfComponents(3);
  points->SetNumberOfTuples(pointIds->GetNumberOfIds());
  inputPoints->GetTuples(pointIds, points);

  cp.enqueue<vtkDataArray*>(blockId, points);
}

//----------------------------------------------------------------------------
template <class DataSetT>
void vtkDIYGhostUtilities::GhostBuffersHelper<DataSetT>::Dequeue(
  const diy::Master::ProxyWithLink& cp, int gid, GhostBuffersType& ghostBuffers)
{
  ghostBuffers.DequeueCellBuffers(cp, gid);
  ghostBuffers.DequeuePointBuffers(cp, gid);
}

//----------------------------------------------------------------------------
template <class DataSetT>
void vtkDIYGhostUtilities::ImplicitGeometryGhosts<DataSetT>::DequeuePointBuffers(
  const diy::Master::ProxyWithLink& cp, int gid)
{
  vtkFieldData* pointData = nullptr;
  cp.dequeue<vtkFieldData*>(gid, pointData);
  this->PointData = vtkSmartPointer<vtkFieldData>::Take(pointData);
}

//----------------------------------------------------------------------------
template <class DataSetT>
void vtkDIYGhostUtilities::ImplicitGeometryGhosts<DataSetT>::DequeueCellBuffers(
  const diy::Master::ProxyWithLink& cp, int gid)
{
  vtkFieldData* cellData = nullptr;
  cp.dequeue<vtkFieldData*>(gid, cellData);
  this->CellData = vtkSmartPointer<vtkFieldData>::Take(cellData);
}

//----------------------------------------------------------------------------
template <class DataSetT>
void vtkDIYGhostUtilities::ExplicitPointGeometryGhosts<DataSetT>::DequeuePointBuffers(
  const diy::Master::ProxyWithLink& cp, int gid)
{
  this->Superclass::DequeuePointBuffers(cp, gid);
  vtkDataArray* points = nullptr;
  cp.dequeue<vtkDataArray*>(gid, points);
  if (points)
  {
    this->Points->SetData(points);
    points->FastDelete();
  }
}

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
      vtkDIYGhostUtilities::GhostBuffersHelper<DataSetT>::Enqueue(cp, blockId, input, block);
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
        vtkDIYGhostUtilities::GhostBuffersHelper<DataSetT>::Dequeue(
          cp, gid, block->GhostBuffers[gid]);
      }
    }
  });
}

//----------------------------------------------------------------------------
template <class DataSetT>
void vtkDIYGhostUtilities::GhostBuffersHelper<DataSetT>::FillReceivedGhosts(
  BlockType* block, int gid, DataSetT* output)
{
  GhostBuffersType& ghostBuffers = block->GhostBuffers.at(gid);

  vtkSmartPointer<vtkIdList> pointIds =
    vtkDIYGhostUtilities::ComputeOutputInterfacePointIds(block, gid, output);
  ghostBuffers.FillReceivedGhostPoints(block, output, pointIds);

  vtkSmartPointer<vtkIdList> cellIds =
    vtkDIYGhostUtilities::ComputeOutputInterfaceCellIds(block, gid, output);
  ghostBuffers.FillReceivedGhostCells(block, output, cellIds);
}

//----------------------------------------------------------------------------
template <class DataSetT>
void vtkDIYGhostUtilities::ImplicitGeometryGhosts<DataSetT>::FillReceivedGhostPoints(
  BlockType* block, DataSetT* output, vtkIdList* pointIds) const
{
  vtkUnsignedCharArray* ghostPointArray = block->GhostPointArray;

  for (vtkIdType i = 0; i < pointIds->GetNumberOfIds(); ++i)
  {
    vtkIdType pointId = pointIds->GetId(i);
    ghostPointArray->SetValue(pointId, vtkDataSetAttributes::PointGhostTypes::DUPLICATEPOINT);
  }

  if (!this->PointData)
  {
    return;
  }

  vtkPointData* outputPointData = output->GetPointData();

  vtkNew<vtkIdList> sourcePointIds;
  sourcePointIds->SetNumberOfIds(this->PointData->GetNumberOfTuples());
  std::iota(sourcePointIds->begin(), sourcePointIds->end(), 0);

  for (int arrayId = 0; arrayId < this->PointData->GetNumberOfArrays(); ++arrayId)
  {
    vtkAbstractArray* array = this->PointData->GetArray(arrayId);
    if (strcmp(array->GetName(), vtkDataSetAttributes::GhostArrayName()))
    {
      vtkAbstractArray* outputArray = outputPointData->GetAbstractArray(array->GetName());
      if (outputArray)
      {
        outputArray->InsertTuples(pointIds, sourcePointIds, array);
      }
    }
  }
}

//----------------------------------------------------------------------------
template <class DataSetT>
void vtkDIYGhostUtilities::ExplicitPointGeometryGhosts<DataSetT>::FillReceivedGhostPoints(
  BlockType* block, DataSetT* output, vtkIdList* pointIds) const
{
  this->Superclass::FillReceivedGhostPoints(block, output, pointIds);

  if (!this->Points)
  {
    return;
  }

  vtkPoints* outputPoints = output->GetPoints();

  for (vtkIdType i = 0; i < pointIds->GetNumberOfIds(); ++i)
  {
    vtkIdType pointId = pointIds->GetId(i);
    outputPoints->SetPoint(pointId, this->Points->GetPoint(i));
  }
}

//----------------------------------------------------------------------------
template <class DataSetT>
void vtkDIYGhostUtilities::ImplicitGeometryGhosts<DataSetT>::FillReceivedGhostCells(
  BlockType* block, DataSetT* output, vtkIdList* cellIds) const
{
  vtkUnsignedCharArray* ghostCellArray = block->GhostCellArray;

  for (vtkIdType i = 0; i < cellIds->GetNumberOfIds(); ++i)
  {
    vtkIdType cellId = cellIds->GetId(i);
    ghostCellArray->SetValue(cellId, vtkDataSetAttributes::CellGhostTypes::DUPLICATECELL);
  }

  if (!this->CellData)
  {
    return;
  }

  vtkCellData* outputCellData = output->GetCellData();

  vtkNew<vtkIdList> sourceCellIds;
  sourceCellIds->SetNumberOfIds(this->CellData->GetNumberOfTuples());
  std::iota(sourceCellIds->begin(), sourceCellIds->end(), 0);

  for (int arrayId = 0; arrayId < this->CellData->GetNumberOfArrays(); ++arrayId)
  {
    vtkAbstractArray* array = this->CellData->GetArray(arrayId);
    if (strcmp(array->GetName(), vtkDataSetAttributes::GhostArrayName()))
    {
      vtkAbstractArray* outputArray = outputCellData->GetAbstractArray(array->GetName());
      if (outputArray)
      {
        outputArray->InsertTuples(cellIds, sourceCellIds, array);
      }
    }
  }
}

//----------------------------------------------------------------------------
template <class DataSetT>
void vtkDIYGhostUtilities::AddGhostArrays(diy::Master& master, std::vector<DataSetT*>& outputs)
{
  using BlockType = typename DataSetTypeToBlockTypeConverter<DataSetT>::BlockType;

  for (int localId = 0; localId < static_cast<int>(outputs.size()); ++localId)
  {
    vtkDIYGhostUtilities::GhostBuffersHelper<DataSetT>::AddGhostArrays(
      master.block<BlockType>(localId), outputs[localId]);
  }
}

//----------------------------------------------------------------------------
template <class DataSetT>
void vtkDIYGhostUtilities::GhostBuffersHelper<DataSetT>::AddGhostArrays(
  BlockType* block, DataSetT* output)
{
  output->GetPointData()->AddArray(block->GhostPointArray);
  output->GetCellData()->AddArray(block->GhostCellArray);
}

//----------------------------------------------------------------------------
template <class DataSetT>
void vtkDIYGhostUtilities::InitializeGhostArrays(
  diy::Master& master, std::vector<DataSetT*>& outputs)
{
  using BlockType = typename DataSetTypeToBlockTypeConverter<DataSetT>::BlockType;

  for (int localId = 0; localId < static_cast<int>(outputs.size()); ++localId)
  {
    vtkDIYGhostUtilities::GhostBuffersHelper<DataSetT>::InitializeGhostArrays(
      master.block<BlockType>(localId), outputs[localId]);
  }
}

//----------------------------------------------------------------------------
template <class DataSetT>
void vtkDIYGhostUtilities::GhostBuffersHelper<DataSetT>::InitializeGhostArrays(
  BlockType* block, DataSetT* output)
{
  vtkDIYGhostUtilities::InitializeGhostCellArray(block, output);
  vtkDIYGhostUtilities::InitializeGhostPointArray(block, output);
}

//----------------------------------------------------------------------------
template <class DataSetT>
void vtkDIYGhostUtilities::InitializeGhostCellArray(
  typename DataSetTypeToBlockTypeConverter<DataSetT>::BlockType* block, DataSetT* output)
{
  vtkSmartPointer<vtkUnsignedCharArray>& ghostCellArray = block->GhostCellArray;
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
}

//----------------------------------------------------------------------------
template <class DataSetT>
void vtkDIYGhostUtilities::InitializeGhostPointArray(
  typename DataSetTypeToBlockTypeConverter<DataSetT>::BlockType* block, DataSetT* output)
{
  vtkSmartPointer<vtkUnsignedCharArray>& ghostPointArray = block->GhostPointArray;
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

//----------------------------------------------------------------------------
template <class DataSetT>
void vtkDIYGhostUtilities::FillReceivedGhosts(
  const diy::Master& master, std::vector<DataSetT*>& outputs)
{
  using BlockType = typename DataSetTypeToBlockTypeConverter<DataSetT>::BlockType;

  for (int localId = 0; localId < static_cast<int>(outputs.size()); ++localId)
  {
    DataSetT* output = outputs[localId];
    BlockType* block = master.block<BlockType>(localId);

    for (auto& item : block->GhostBuffers)
    {
      vtkDIYGhostUtilities::GhostBuffersHelper<DataSetT>::FillReceivedGhosts(
        block, item.first, output);
    }
  }
}

//----------------------------------------------------------------------------
/**
 * Main pipeline generating ghosts.
 */
template <class DataSetT>
int vtkDIYGhostUtilities::GenerateGhostCells(std::vector<DataSetT*>& inputs,
  std::vector<DataSetT*>& outputs, int outputGhostLevels, vtkMultiProcessController* controller)
{
  static_assert((std::is_base_of<vtkImageData, DataSetT>::value ||
                  std::is_base_of<vtkRectilinearGrid, DataSetT>::value ||
                  std::is_base_of<vtkStructuredGrid, DataSetT>::value),
    "Input data set type is not supported.");

  using BlockType = typename DataSetTypeToBlockTypeConverter<DataSetT>::BlockType;

  const int size = static_cast<int>(inputs.size());

  if (size != static_cast<int>(outputs.size()))
  {
    vtkLog(ERROR, "inputs and outputs have different sizes for " << inputs[0]->GetClassName());
    return 0;
  }

  std::string logMessage = size
    ? std::string("Generating ghosts for ") + std::string(outputs[0]->GetClassName())
    : std::string("No ghosts to generate for empty rank");
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

  if (!size)
  {
    // In such instance, we can just terminate. We are empty an finished communicating with other
    // ranks.
    vtkLogEndScope(logMessage.c_str());
    return 1;
  }

  vtkLogStartScope(TRACE, "Decomposing master");
  diy::RegularDecomposer<diy::DiscreteBounds> decomposer(
    /*dim*/ 1, diy::interval(0, assigner.nblocks() - 1), assigner.nblocks());
  decomposer.decompose(comm.rank(), assigner, master);
  vtkLogEndScope("Decomposing master");

  vtkLogStartScope(TRACE, "Setup block self information.");
  vtkDIYGhostUtilities::SetupBlockSelfInformation(master, inputs);
  vtkLogEndScope("Setup block self information.");

  vtkLogStartScope(TRACE, "Exchanging block structures");
  vtkDIYGhostUtilities::ExchangeBlockStructures(master, assigner, inputs);
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

  vtkLogStartScope(TRACE, "Initializing ghost arrays in outputs");
  vtkDIYGhostUtilities::InitializeGhostArrays(master, outputs);
  vtkLogEndScope("Initializing ghost arrays in outputs");

  vtkLogStartScope(TRACE, "Filling local ghosts with received data from other blocks");
  vtkDIYGhostUtilities::FillGhostArrays(master, outputs);
  vtkLogEndScope("Filling local ghosts with received data from other blocks");

  vtkLogStartScope(TRACE, "Adding ghost arrays to point and / or cell data");
  vtkDIYGhostUtilities::AddGhostArrays(master, outputs);
  vtkLogEndScope("Adding ghost arrays to point and / or cell data");

  vtkLogEndScope(logMessage.c_str());

  return 1;
}

#endif
