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

#include "vtkArrayDispatch.h"
#include "vtkBoundingBox.h"
#include "vtkCellData.h"
#include "vtkCommunicator.h"
#include "vtkDIYExplicitAssigner.h"
#include "vtkDIYUtilities.h"
#include "vtkDataArrayRange.h"
#include "vtkDataSetAttributes.h"
#include "vtkDataSetSurfaceFilter.h"
#include "vtkFieldData.h"
#include "vtkIdList.h"
#include "vtkImageData.h"
#include "vtkLogger.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkRectilinearGrid.h"
#include "vtkSMPTools.h"
#include "vtkStructuredData.h"
#include "vtkStructuredGrid.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"

#include <atomic>
#include <limits>
#include <string>
#include <type_traits>
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
  static constexpr bool IsUnstructuredData = false;
};

//============================================================================
template <>
struct vtkDIYGhostUtilities::DataSetTypeToBlockTypeConverter<vtkRectilinearGrid>
{
  typedef RectilinearGridBlock BlockType;
  static constexpr bool IsUnstructuredData = false;
};

//============================================================================
template <>
struct vtkDIYGhostUtilities::DataSetTypeToBlockTypeConverter<vtkStructuredGrid>
{
  typedef StructuredGridBlock BlockType;
  static constexpr bool IsUnstructuredData = false;
};

//============================================================================
template <>
struct vtkDIYGhostUtilities::DataSetTypeToBlockTypeConverter<vtkUnstructuredGrid>
{
  typedef UnstructuredGridBlock BlockType;
  static constexpr bool IsUnstructuredData = true;
};

//============================================================================
template <>
struct vtkDIYGhostUtilities::DataSetTypeToBlockTypeConverter<vtkPolyData>
{
  typedef PolyDataBlock BlockType;
  static constexpr bool IsUnstructuredData = true;
};

namespace vtkDIYGhostUtilities_detail
{
//============================================================================
template <class ValueT, bool IsIntegerT = std::numeric_limits<ValueT>::is_integer>
struct Limits;

//============================================================================
template <class ValueT>
struct Limits<ValueT, true>
{
  static constexpr ValueT Epsilon = 0;
  static constexpr ValueT Min = 0;
};

//============================================================================
template <class ValueT>
struct Limits<ValueT, false>
{
  static constexpr ValueT Epsilon = std::numeric_limits<ValueT>::epsilon();
  static constexpr ValueT Min = std::numeric_limits<ValueT>::min();
};

//----------------------------------------------------------------------------
template <class ValueT>
ValueT ComputePrecision(ValueT val)
{
  constexpr ValueT Epsilon = Limits<ValueT>::Epsilon;
  constexpr ValueT Min = Limits<ValueT>::Min;
  return std::max<ValueT>(val * Epsilon, Min);
}

//============================================================================
template <bool CellVersionT>
struct ConvertIdToCoordinatesFromCroppedExtentToFullExtentWorker;

//============================================================================
template <>
struct ConvertIdToCoordinatesFromCroppedExtentToFullExtentWorker<true>
{
  vtkIdType operator()(vtkIdType id, const vtkDIYGhostUtilities::ExtentType& fullExtent,
    const vtkDIYGhostUtilities::ExtentType& croppedExtent)
  {
    int ijk[3];
    vtkStructuredData::ComputeCellStructuredCoordsForExtent(id, croppedExtent.data(), ijk);
    return vtkStructuredData::ComputeCellIdForExtent(fullExtent.data(), ijk);
  }
};

//============================================================================
template <>
struct ConvertIdToCoordinatesFromCroppedExtentToFullExtentWorker<false>
{
  vtkIdType operator()(vtkIdType id, const vtkDIYGhostUtilities::ExtentType& fullExtent,
    const vtkDIYGhostUtilities::ExtentType& croppedExtent)
  {
    int ijk[3];
    vtkStructuredData::ComputePointStructuredCoordsForExtent(id, croppedExtent.data(), ijk);
    return vtkStructuredData::ComputePointIdForExtent(fullExtent.data(), ijk);
  }
};

//============================================================================
template <bool CellVersionT>
struct GhostFinder
{
  using ExtentType = vtkDIYGhostUtilities::ExtentType;

  GhostFinder(vtkUnsignedCharArray* ghosts, const ExtentType& fullExtent,
    const ExtentType& croppedExtent, unsigned char ghostTarget)
    : Ghosts(ghosts)
    , FullExtent(fullExtent)
    , CroppedExtent(croppedExtent)
    , GhostTarget(ghostTarget)
    , FoundGhost(false)
  {
  }

  void operator()(vtkIdType startId, vtkIdType endId)
  {
    auto ghosts = vtk::DataArrayValueRange<1>(this->Ghosts);
    using RefType = typename decltype(ghosts)::ReferenceType;

    for (vtkIdType id = startId; id < endId; ++id)
    {
      if (this->FoundGhost)
      {
        return;
      }

      ConvertIdToCoordinatesFromCroppedExtentToFullExtentWorker<CellVersionT> worker;
      vtkIdType remappedId = worker(id, this->FullExtent, this->CroppedExtent);

      RefType ghost = ghosts[remappedId];

      if (ghost & this->GhostTarget)
      {
        this->FoundGhost = true;
      }
    }
  }

  vtkUnsignedCharArray* Ghosts;
  const ExtentType& FullExtent;
  const ExtentType& CroppedExtent;
  unsigned char GhostTarget;

  std::atomic<bool> FoundGhost;
};

//----------------------------------------------------------------------------
template <class StructuredDataSetT>
vtkSmartPointer<StructuredDataSetT> CleanGhostsIfPossibleForStructuredData(
  typename vtkDIYGhostUtilities::DataSetTypeToBlockTypeConverter<StructuredDataSetT>::BlockType*
    block,
  StructuredDataSetT* ds)
{
  using ExtentType = vtkDIYGhostUtilities::ExtentType;

  vtkSmartPointer<StructuredDataSetT> cleanedDS = vtkSmartPointer<StructuredDataSetT>::New();
  cleanedDS->ShallowCopy(ds);
  const ExtentType& extent = block->Information.Extent;
  const int* dsExtent = ds->GetExtent();
  const ExtentType fullExtent{ dsExtent[0], dsExtent[1], dsExtent[2], dsExtent[3], dsExtent[4],
    dsExtent[5] };

  if (vtkUnsignedCharArray* ghostCells = ds->GetCellGhostArray())
  {
    const vtkIdType numberOfCells = vtkStructuredData::GetNumberOfCells(extent.data());
    vtkDIYGhostUtilities_detail::GhostFinder<true> worker(
      ghostCells, fullExtent, extent, vtkDataSetAttributes::HIDDENCELL);
    vtkSMPTools::For(0, numberOfCells, worker);
    if (!worker.FoundGhost)
    {
      cleanedDS->GetCellData()->RemoveArray(ghostCells->GetName());
    }
  }
  if (vtkUnsignedCharArray* ghostPoints = ds->GetPointGhostArray())
  {
    const vtkIdType numberOfPoints = vtkStructuredData::GetNumberOfPoints(extent.data());
    vtkDIYGhostUtilities_detail::GhostFinder<false> worker(
      ghostPoints, fullExtent, extent, vtkDataSetAttributes::HIDDENPOINT);
    vtkSMPTools::For(0, numberOfPoints, worker);
    if (!worker.FoundGhost)
    {
      cleanedDS->GetPointData()->RemoveArray(ghostPoints->GetName());
    }
  }

  return cleanedDS;
}

//============================================================================
template <bool IsUnstructuredDataT>
struct CleanGhostsIfPossibleWorkerImpl;

//============================================================================
template <>
struct CleanGhostsIfPossibleWorkerImpl<true>
{
  template <class DataSetT>
  vtkSmartPointer<DataSetT> operator()(
    typename vtkDIYGhostUtilities::DataSetTypeToBlockTypeConverter<DataSetT>::BlockType*,
    DataSetT* ds)
  {
    return ds;
  }
};

//============================================================================
template <>
struct CleanGhostsIfPossibleWorkerImpl<false>
{
  template <class DataSetT>
  vtkSmartPointer<DataSetT> operator()(
    typename vtkDIYGhostUtilities::DataSetTypeToBlockTypeConverter<DataSetT>::BlockType* block,
    DataSetT* ds)
  {
    return vtkDIYGhostUtilities_detail::CleanGhostsIfPossibleForStructuredData(block, ds);
  }
};

//============================================================================
template <class DataSetT>
struct CleanGhostsIfPossibleWorker
{
  using DataSetInfo = vtkDIYGhostUtilities::DataSetTypeToBlockTypeConverter<DataSetT>;
  using BlockType = typename DataSetInfo::BlockType;
  static constexpr bool IsUnstructuredData = DataSetInfo::IsUnstructuredData;

  vtkSmartPointer<DataSetT> operator()(BlockType* block, DataSetT* ds)
  {
    return this->Impl(block, ds);
  }

  CleanGhostsIfPossibleWorkerImpl<IsUnstructuredData> Impl;
};
} // namesapce vtkDIYGhostUtilities_detail

//----------------------------------------------------------------------------
template <class DataSetT>
void vtkDIYGhostUtilities::ExchangeBoundingBoxes(
  diy::Master& master, const vtkDIYExplicitAssigner& assigner, std::vector<DataSetT*>& inputs)
{
  using BlockType = typename DataSetTypeToBlockTypeConverter<DataSetT>::BlockType;

  diy::all_to_all(
    master, assigner, [&master, &inputs](BlockType* block, const diy::ReduceProxy& srp) {
      int myBlockId = srp.gid();
      int localId = master.lid(myBlockId);
      auto& input = inputs[localId];
      if (srp.round() == 0)
      {
        const double* bounds = input->GetBounds();
        vtkBoundingBox& bb = block->BoundingBox;
        bb = vtkBoundingBox(bounds);

        vtkDIYGhostUtilities::InflateBoundingBoxIfNecessary(input, bb);

        for (int i = 0; i < srp.out_link().size(); ++i)
        {
          const diy::BlockID& blockId = srp.out_link().target(i);
          if (blockId.gid != myBlockId)
          {
            srp.enqueue(blockId, bb.GetMinPoint(), 3);
            srp.enqueue(blockId, bb.GetMaxPoint(), 3);
          }
        }
      }
      else
      {
        double minPoint[3], maxPoint[3];
        for (int i = 0; i < static_cast<int>(srp.in_link().size()); ++i)
        {
          const diy::BlockID& blockId = srp.in_link().target(i);
          if (blockId.gid != myBlockId)
          {
            srp.dequeue(blockId, minPoint, 3);
            srp.dequeue(blockId, maxPoint, 3);

            block->NeighborBoundingBoxes.emplace(blockId.gid,
              vtkBoundingBox(
                minPoint[0], maxPoint[0], minPoint[1], maxPoint[1], minPoint[2], maxPoint[2]));
          }
        }
      }
    });
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
      vtkDIYGhostUtilities::EnqueueGhosts(cp, blockId, input, block);
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
        vtkDIYGhostUtilities::DequeueGhosts(cp, gid, block->BlockStructures.at(gid));
      }
    }
  });
}

//----------------------------------------------------------------------------
template <class BlockT>
vtkDIYGhostUtilities::LinkMap vtkDIYGhostUtilities::ComputeLinkMapUsingBoundingBoxes(
  const diy::Master& master)
{
  LinkMap linkMap(master.size());

  for (int localId = 0; localId < static_cast<int>(master.size()); ++localId)
  {
    Links& links = linkMap[localId];
    BlockT* block = master.block<BlockT>(localId);
    vtkBoundingBox& localbb = block->BoundingBox;
    BlockMapType<vtkBoundingBox>& bb = block->NeighborBoundingBoxes;
    for (auto item : bb)
    {
      if (localbb.Intersects(item.second))
      {
        links.insert(item.first);
      }
    }
  }
  return linkMap;
}

//----------------------------------------------------------------------------
template <class DataSetT>
void vtkDIYGhostUtilities::CopyInputsAndAllocateGhosts(const diy::Master& master,
  std::vector<DataSetT*>& inputs, std::vector<DataSetT*>& outputs, int outputGhostLevels)
{
  using BlockType = typename DataSetTypeToBlockTypeConverter<DataSetT>::BlockType;

  for (int localId = 0; localId < static_cast<int>(inputs.size()); ++localId)
  {
    DataSetT* input = inputs[localId];
    DataSetT* output = outputs[localId];
    BlockType* block = master.block<BlockType>(localId);

    vtkUnsignedCharArray* ghostCells = input->GetCellGhostArray();

    if (outputGhostLevels == 0 && !ghostCells)
    {
      output->ShallowCopy(input);
    }
    else
    {
      vtkDIYGhostUtilities_detail::CleanGhostsIfPossibleWorker<DataSetT> cleaner;
      vtkSmartPointer<DataSetT> cleanedInput = cleaner(block, input);

      vtkDIYGhostUtilities::DeepCopyInputAndAllocateGhosts(block, cleanedInput, output);
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
    DataSetT* output = outputs[localId];
    BlockType* block = master.block<BlockType>(localId);

    output->GetPointData()->AddArray(block->GhostPointArray);

    if (block->GhostCellArray)
    {
      output->GetCellData()->AddArray(block->GhostCellArray);
    }
  }
}

//----------------------------------------------------------------------------
template <class DataSetT>
void vtkDIYGhostUtilities::InitializeGhostArrays(
  diy::Master& master, std::vector<DataSetT*>& outputs, int outputGhostLevels)
{
  using BlockType = typename DataSetTypeToBlockTypeConverter<DataSetT>::BlockType;

  for (int localId = 0; localId < static_cast<int>(outputs.size()); ++localId)
  {
    DataSetT* output = outputs[localId];
    BlockType* block = master.block<BlockType>(localId);

    if (outputGhostLevels != 0)
    {
      vtkDIYGhostUtilities::InitializeGhostCellArray(block, output);
    }

    vtkDIYGhostUtilities::InitializeGhostPointArray(block, output);
  }
}

//----------------------------------------------------------------------------
template <class DataSetT>
void vtkDIYGhostUtilities::InitializeGhostCellArray(
  typename DataSetTypeToBlockTypeConverter<DataSetT>::BlockType* block, DataSetT* output)
{
  vtkSmartPointer<vtkUnsignedCharArray>& ghostCellArray = block->GhostCellArray;
  ghostCellArray = output->GetGhostArray(vtkDataObject::FIELD_ASSOCIATION_CELLS);
  if (!ghostCellArray)
  {
    ghostCellArray = vtkSmartPointer<vtkUnsignedCharArray>::New();
    ghostCellArray->SetName(vtkDataSetAttributes::GhostArrayName());
    ghostCellArray->SetNumberOfComponents(1);
    ghostCellArray->SetNumberOfValues(output->GetNumberOfCells());
    ghostCellArray->Fill(0);
  }
}

//----------------------------------------------------------------------------
template <class DataSetT>
void vtkDIYGhostUtilities::InitializeGhostPointArray(
  typename DataSetTypeToBlockTypeConverter<DataSetT>::BlockType* block, DataSetT* output)
{
  vtkSmartPointer<vtkUnsignedCharArray>& ghostPointArray = block->GhostPointArray;
  ghostPointArray = output->GetGhostArray(vtkDataObject::FIELD_ASSOCIATION_POINTS);
  if (!ghostPointArray)
  {
    ghostPointArray = vtkSmartPointer<vtkUnsignedCharArray>::New();
    ghostPointArray->SetName(vtkDataSetAttributes::GhostArrayName());
    ghostPointArray->SetNumberOfComponents(1);
    ghostPointArray->SetNumberOfValues(output->GetNumberOfPoints());
    ghostPointArray->Fill(0);
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
                  std::is_base_of<vtkStructuredGrid, DataSetT>::value ||
                  std::is_base_of<vtkUnstructuredGrid, DataSetT>::value ||
                  std::is_base_of<vtkPolyData, DataSetT>::value),
    "Input data set type is not supported.");

  using BlockType = typename DataSetTypeToBlockTypeConverter<DataSetT>::BlockType;

  const int size = static_cast<int>(inputs.size());
  if (size != static_cast<int>(outputs.size()))
  {
    return 0;
  }

  std::string logMessage = size
    ? std::string("Generating ghosts for ") + std::string(outputs[0]->GetClassName())
    : std::string("No ghosts to generate for empty rank");
  vtkLogStartScope(TRACE, logMessage.c_str());

  vtkDIYGhostUtilities::CloneGeometricStructures(inputs, outputs);

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

  // At this step, we gather data from the inputs and store it inside the local blocks
  // so we don't have to carry extra parameters later.
  vtkLogStartScope(TRACE, "Setup block self information.");
  vtkDIYGhostUtilities::InitializeBlocks(master, inputs);
  vtkLogEndScope("Setup block self information.");

  vtkLogStartScope(TRACE, "Exchanging bounding boxes");
  vtkDIYGhostUtilities::ExchangeBoundingBoxes(master, assigner, inputs);
  vtkLogEndScope("Exchanging bounding boxes");

  // We compute a temporary link map that weeds out data sets that do not have
  // overlapping bounding boxes.
  vtkLogStartScope(TRACE, "Computing temporary link map using bounding boxes.");
  LinkMap temporaryLinkMap =
    vtkDIYGhostUtilities::ComputeLinkMapUsingBoundingBoxes<BlockType>(master);
  vtkLogEndScope("Computing temporary link map using bounding boxes.");

  // We link our blocks using the temporary link map
  vtkLogStartScope(TRACE, "Relinking blocks using temporary link map");
  vtkDIYUtilities::Link(master, assigner, temporaryLinkMap);
  vtkLogEndScope("Relinking blocks using temporary link map");

  // Here, we exchange structural information between blocks that will be used to
  // determine if blocks are actually adjacent or not.
  vtkLogStartScope(TRACE, "Exchanging block structures");
  vtkDIYGhostUtilities::ExchangeBlockStructures(master, inputs);
  vtkLogEndScope("Exchanging block structures");

  // The structural information that has been exchanged is used to compute
  // the final link map, mapping blocks that will actually exchange ghosts.
  vtkLogStartScope(TRACE, "Creating link map between connected blocks");
  LinkMap linkMap = vtkDIYGhostUtilities::ComputeLinkMap(master, inputs, outputGhostLevels);
  vtkLogEndScope("Creating link map between connected blocks");

  vtkLogStartScope(TRACE, "Relinking blocks using link map");
  vtkDIYUtilities::Link(master, assigner, linkMap);
  vtkLogEndScope("Relinking blocks using link map");

  vtkLogStartScope(TRACE, "Exchanging ghost data between blocks");
  vtkDIYGhostUtilities::ExchangeGhosts(master, inputs);
  vtkLogEndScope("Exchanging ghost data between blocks");

  vtkLogStartScope(TRACE, "Allocating ghosts in outputs");
  vtkDIYGhostUtilities::CopyInputsAndAllocateGhosts(master, inputs, outputs, outputGhostLevels);
  vtkLogEndScope("Allocating ghosts in outputs");

  vtkLogStartScope(TRACE, "Initializing ghost arrays in outputs");
  vtkDIYGhostUtilities::InitializeGhostArrays(master, outputs, outputGhostLevels);
  vtkLogEndScope("Initializing ghost arrays in outputs");

  vtkLogStartScope(TRACE, "Filling local ghosts with received data from other blocks");
  vtkDIYGhostUtilities::FillGhostArrays(master, outputs, outputGhostLevels);
  vtkLogEndScope("Filling local ghosts with received data from other blocks");

  vtkLogStartScope(TRACE, "Adding ghost arrays to point and / or cell data");
  vtkDIYGhostUtilities::AddGhostArrays(master, outputs);
  vtkLogEndScope("Adding ghost arrays to point and / or cell data");

  vtkLogEndScope(logMessage.c_str());

  return 1;
}

#endif
