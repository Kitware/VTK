// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkDGSummaryInformationResponder.h"

#include "vtkArrayDispatch.h"
#include "vtkBatch.h"
#include "vtkCellAttribute.h"
#include "vtkCellGrid.h"
#include "vtkCellGridSummaryInformationQuery.h"
#include "vtkCellMetadata.h"
#include "vtkDataArray.h"
#include "vtkDataArrayRange.h"
#include "vtkDataSetAttributes.h"
#include "vtkObjectFactory.h"
#include "vtkSMPTools.h"
#include "vtkStringToken.h"

#include <atomic>

VTK_ABI_NAMESPACE_BEGIN

using namespace vtk::literals;

vtkStandardNewMacro(vtkDGSummaryInformationResponder);

namespace
{
constexpr unsigned char MASKED_POINT_VALUE =
  vtkDataSetAttributes::DUPLICATEPOINT | vtkDataSetAttributes::HIDDENPOINT;
struct PointBatchData
{
  vtkIdType PointsOffset;

  PointBatchData()
    : PointsOffset(0)
  {
  }
  ~PointBatchData() = default;
  PointBatchData& operator+=(const PointBatchData& other)
  {
    this->PointsOffset += other.PointsOffset;
    return *this;
  }
  PointBatchData operator+(const PointBatchData& other) const
  {
    PointBatchData result = *this;
    result += other;
    return result;
  }
};
using PointBatch = vtkBatch<PointBatchData>;
using PointBatches = vtkBatches<PointBatchData>;

template <typename TConnectivityArray, typename TGhostsArray>
struct ComputeDegreesOfFreedom
{
  TConnectivityArray* Connectivity;
  TGhostsArray* Ghosts;

  PointBatches Batches;

  std::unique_ptr<std::atomic<unsigned char>[]> PtUses;
  vtkIdType PtUsagesSize;
  vtkIdType DOFCount;

  ComputeDegreesOfFreedom(TConnectivityArray* conn, TGhostsArray* ghosts)
    : Connectivity(conn)
    , Ghosts(ghosts)
    , PtUsagesSize(0)
    , DOFCount(0)
  {
    // create a shallow-copy to do fast GetRange on only 1 component, instead of all individually.
    auto connectivityCopy = vtk::TakeSmartPointer(this->Connectivity->NewInstance());
    connectivityCopy->ShallowCopy(this->Connectivity);
    connectivityCopy->SetNumberOfComponents(1);
    if constexpr (std::is_same_v<TConnectivityArray, vtkDataArray>)
    {
      double range[2];
      connectivityCopy->GetRange(range);
      this->PtUsagesSize = static_cast<vtkIdType>(range[1]) + 1;
    }
    else
    {
      using T = vtk::GetAPIType<TConnectivityArray>;
      T range[2];
      connectivityCopy->GetValueRange(range);
      this->PtUsagesSize = static_cast<vtkIdType>(range[1]) + 1;
    }
    this->PtUses.reset(new std::atomic<unsigned char>[this->PtUsagesSize]());
    this->Batches.Initialize(this->Connectivity->GetNumberOfValues());
  }

  void Initialize() {}

  void operator()(vtkIdType beginBatchId, vtkIdType endBatchId)
  {
    auto connectivity = vtk::DataArrayValueRange(this->Connectivity);
    if (this->Ghosts)
    {
      auto pointGhosts = vtk::DataArrayValueRange<1, unsigned char>(this->Ghosts);
      for (vtkIdType batchId = beginBatchId; batchId < endBatchId; ++batchId)
      {
        PointBatch& batch = this->Batches[batchId];
        auto& batchNumberOfPoints = batch.Data.PointsOffset;
        for (vtkIdType i = batch.BeginId; i < batch.EndId; ++i)
        {
          const auto pointId = static_cast<vtkIdType>(connectivity[i]);
          if (pointGhosts[pointId] & MASKED_POINT_VALUE)
          {
            continue;
          }
          if (this->PtUses[pointId].fetch_or(1, std::memory_order_relaxed) == 0)
          {
            ++batchNumberOfPoints;
          }
        }
      }
    }
    else
    {
      for (vtkIdType batchId = beginBatchId; batchId < endBatchId; ++batchId)
      {
        PointBatch& batch = this->Batches[batchId];
        auto& batchNumberOfPoints = batch.Data.PointsOffset;
        for (vtkIdType i = batch.BeginId; i < batch.EndId; ++i)
        {
          const auto pointId = static_cast<vtkIdType>(connectivity[i]);
          if (this->PtUses[pointId].fetch_or(1, std::memory_order_relaxed) == 0)
          {
            ++batchNumberOfPoints;
          }
        }
      }
    }
  }

  void Reduce()
  {
    const auto globalSum = this->Batches.BuildOffsetsAndGetGlobalSum();
    this->DOFCount = globalSum.PointsOffset;
  }
};

struct ComputeDegreesOfFreedomWorker
{
  template <typename TConnectivityArray, typename TGhostsArray>
  void operator()(TConnectivityArray* conn, TGhostsArray* ghosts, vtkIdType& dofCount)
  {
    ComputeDegreesOfFreedom<TConnectivityArray, TGhostsArray> functor(conn, ghosts);
    vtkSMPTools::For(0, functor.Batches.GetNumberOfBatches(), functor);
    dofCount = functor.DOFCount;
  }
};
}

bool vtkDGSummaryInformationResponder::Query(vtkCellGridSummaryInformationQuery* query,
  vtkCellMetadata* cellType, vtkCellGridResponders* caches)
{
  (void)caches;

  auto* grid = cellType->GetCellGrid();
  if (!grid)
  {
    return false;
  }

  // The responder is invoked once per cell type (e.g. vtkDGHex, vtkDGTet).
  // Tokenize the class name to look up per-cell-type info on each attribute.
  vtkStringToken cellTypeName(cellType->GetClassName());

  const char* filterName = query->GetAttributeName();

  for (const auto& att : grid->GetCellAttributeList())
  {
    // If a name filter is set, skip attributes that do not match.
    if (filterName && att->GetName().Data() != filterName)
    {
      continue;
    }

    auto info = att->GetCellTypeInfo(cellTypeName);
    if (info.ArraysByRole.empty())
    {
      // Attribute is not defined for this cell type.
      continue;
    }

    vtkCellGridSummaryInformationQuery::SummaryInformation summaryInfo;
    // --- Polynomial order ---
    summaryInfo.OrderRange[0] = info.Order;
    summaryInfo.OrderRange[1] = info.Order;

    // --- Degrees of freedom ---
    summaryInfo.DOFCount = 0;

    if (info.DOFSharing.IsValid())
    {
      // Continuous / shared DOF field: DOF points are shared between cells and
      // indexed by a connectivity array. Count unique point IDs, then subtract
      // ghost IDs so DOFs belonging to a neighbouring partition are not double-counted.

      auto connIt = info.ArraysByRole.find("connectivity"_token);
      auto ghostIt = info.ArraysByRole.find("ghost"_token);
      using IntegerArrays = vtkArrayDispatch::FilterArraysByValueType<vtkArrayDispatch::AOSArrays,
        vtkArrayDispatch::Integrals>::Result;

      ComputeDegreesOfFreedomWorker worker;
      if (connIt != info.ArraysByRole.end() && ghostIt != info.ArraysByRole.end())
      {
        auto* conn = vtkDataArray::SafeDownCast(connIt->second);
        auto* ghost = vtkDataArray::SafeDownCast(ghostIt->second);

        if (!vtkArrayDispatch::Dispatch2ByArray<IntegerArrays, IntegerArrays>::Execute(
              conn, ghost, worker, summaryInfo.DOFCount))
        {
          worker(conn, ghost, summaryInfo.DOFCount);
        }
      }
      else if (connIt != info.ArraysByRole.end())
      {
        auto* conn = vtkDataArray::SafeDownCast(connIt->second);
        if (!vtkArrayDispatch::DispatchByArray<IntegerArrays>::Execute(
              conn, worker, static_cast<vtkDataArray*>(nullptr), summaryInfo.DOFCount))
        {
          worker(conn, static_cast<vtkDataArray*>(nullptr), summaryInfo.DOFCount);
        }
      }
      else
      {
        vtkErrorMacro(<< "DOF sharing is valid but no connectivity array found for attribute "
                      << att->GetName().Data() << " on cell type " << cellTypeName.Data());
        return false;
      }
    }
    else
    {
      // Discontinuous field: each cell owns its private DOF values;
      // the total is the size of the values array.
      auto valIt = info.ArraysByRole.find("values"_token);
      if (valIt != info.ArraysByRole.end() && valIt->second)
      {
        summaryInfo.DOFCount = valIt->second->GetNumberOfValues();
      }
    }

    query->AddSummaryInformation(att.GetPointer(), summaryInfo);
  }

  return true;
}

VTK_ABI_NAMESPACE_END
