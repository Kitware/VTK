// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkDGEvaluator.h"

#include "vtkBatch.h"
#include "vtkBoundingBox.h"
#include "vtkCellAttribute.h"
#include "vtkCellGrid.h"
#include "vtkCellGridEvaluator.h"
#include "vtkConvexHull.h"
#include "vtkDGTet.h"
#include "vtkDataSetAttributes.h"
#include "vtkIdList.h"
#include "vtkIdTypeArray.h"
#include "vtkInterpolateCalculator.h"
#include "vtkObjectFactory.h"
#include "vtkSMPTools.h"
#include "vtkStaticPointLocator.h"
#include "vtkStringToken.h"
#include "vtkTypeInt64Array.h"
#include "vtkVector.h"

#include "vtk_eigen.h"
#include VTK_EIGEN(Eigen)

#include <iostream>

// Switch the following to #define to get debug printouts. Beware, these are in a tight loop.
#undef VTK_DBG_DGEVAL

VTK_ABI_NAMESPACE_BEGIN

using namespace vtk::literals;

namespace
{

/// Compute the centroid and circumscribed-sphere radius for a set of cell corner points.
/// Used as a fast conservative candidate filter: input points outside the sphere
/// cannot be inside the cell, so they are skipped before the more expensive Newton step.
void centerAndRadiusOfCellPoints(
  const std::vector<vtkVector3d>& cellCorners, vtkVector3d& center, double& radius)
{
  center = vtkVector3d(0., 0., 0.);
  for (const auto& corner : cellCorners)
  {
    center += corner;
  }
  center = (1.0 / static_cast<double>(cellCorners.size())) * center;
  radius = -1.0; // Initialize with an invalid value.
  for (const auto& corner : cellCorners)
  {
    double cornerRadius = (corner - center).Norm();
    radius = std::max(cornerRadius, radius);
  }
}

//-----------------------------------------------------------------------------
// Keep track of output information within each batch of cells — this
// information is eventually rolled up into offsets so that separate threads
// know where to write their data. We need to know how many classified
// (probe point, cell) pairs are produced per batch.
struct CellBatchData
{
  // In the accumulation pass (first vtkSMPTools::For in ClassifyPoints) this
  // counts the number of classified points found in the batch.
  // In the scatter pass (second vtkSMPTools::For) this is changed to the
  // starting offset into alloc.InputPoints for this batch.
  // The dual use reduces memory footprint.
  vtkIdType PointsOffset;

  CellBatchData()
    : PointsOffset(0)
  {
  }
  ~CellBatchData() = default;
  CellBatchData& operator+=(const CellBatchData& other)
  {
    this->PointsOffset += other.PointsOffset;
    return *this;
  }
  CellBatchData operator+(const CellBatchData& other) const
  {
    CellBatchData result = *this;
    result += other;
    return result;
  }
};
using CellBatch = vtkBatch<CellBatchData>;
using CellBatches = vtkBatches<CellBatchData>;

} // anonymous namespace

vtkStandardNewMacro(vtkDGEvaluator);

bool vtkDGEvaluator::Query(
  vtkCellGridEvaluator* request, vtkCellMetadata* cellType, vtkCellGridResponders* caches)
{
  switch (request->GetPhasesToPerform())
  {
    case vtkCellGridEvaluator::Phases::ClassifyAndInterpolate:
    case vtkCellGridEvaluator::Phases::Interpolate:
      if (!request->GetCellAttribute())
      {
        return false;
      }
      break;
    default:
      break;
  }

  int pass = request->GetPass();
  switch (request->GetPhasesToPerform())
  {
    case vtkCellGridEvaluator::Phases::Classify:
    case vtkCellGridEvaluator::Phases::ClassifyAndInterpolate:
      if (pass == 0)
      {
        return this->ClassifyPoints(request, cellType, caches);
      }
      else if (pass == 1 &&
        request->GetPhasesToPerform() == vtkCellGridEvaluator::Phases::ClassifyAndInterpolate)
      {
        return this->InterpolatePoints(request, cellType, caches);
      }
      break;
    case vtkCellGridEvaluator::Phases::Interpolate:
      return this->InterpolatePoints(request, cellType, caches);
    case vtkCellGridEvaluator::Phases::None:
    default:
      break;
  }
  return false;
}

bool vtkDGEvaluator::ClassifyPoints(
  vtkCellGridEvaluator* query, vtkCellMetadata* cellType, vtkCellGridResponders* caches)
{
  auto* dgCell = dynamic_cast<vtkDGCell*>(cellType);
  if (!dgCell)
  {
    return false;
  }

  auto* locator = query->GetLocator();
  auto* grid = cellType->GetCellGrid();
  auto* shape = grid ? grid->GetShapeAttribute() : nullptr;
  if (!shape)
  {
    return false;
  }

  // The shape attribute calculator inverts the reference-to-world mapping via
  // Newton-Raphson, giving us parametric coordinates for free during classification.
  auto calc = caches->AttributeCalculator<vtkInterpolateCalculator>(
    dgCell, shape, dgCell->GetAttributeTags(shape, true));
  if (!calc)
  {
    return false;
  }

  auto cellTypeInfo = shape->GetCellTypeInfo(cellType->GetClassName());
  auto& arrays = cellTypeInfo.ArraysByRole;
  auto it = arrays.find("values"_hash);
  vtkDataArray* coords = nullptr;
  if (it != arrays.end())
  {
    coords = vtkDataArray::SafeDownCast(it->second);
  }
  it = arrays.find("connectivity"_hash);
  vtkDataArray* conn = nullptr;
  if (it != arrays.end())
  {
    conn = vtkDataArray::SafeDownCast(it->second);
  }
  if (!conn || !coords)
  {
    return false;
  }
  vtkIdType numCells = conn->GetNumberOfTuples();
  int numPointIds = conn->GetNumberOfComponents();

  // Per-cell storage for classified points found during the accumulation pass.
  // Each inner vector holds (inputPointId, parametricCoords) pairs for one cell.
  // Indexed by cellId so the scatter pass can write results in deterministic order.
  std::vector<std::vector<std::pair<vtkIdType, std::array<double, 3>>>> cellPoints(numCells);

  auto& alloc = query->GetAllocationsForCellType(cellType->GetClassName());
  auto* inputPoints = query->GetInputPoints();
  const int dim = dgCell->GetDimension();

  // Parametric center of the cell shape — used as Newton-Raphson starting guess.
  // Computed once per ClassifyPoints call since all cells share the same dgCell type.
  const vtkVector3d rstCenter = dgCell->GetParametricCenterOfSide(-1);

  // Accumulation pass: for each cell, find candidate probe points via the
  // circumscribed-sphere filter, then run Newton-Raphson to classify them.
  // Successful classifications are stored in cellPoints[cellId] and counted
  // in batch.Data.PointsOffset so the scatter pass knows where to write.
  CellBatches batches;
  batches.Initialize(numCells, /*batchSize=*/50);
  vtkSMPTools::For(0, batches.GetNumberOfBatches(),
    [&](vtkIdType beginBatchId, vtkIdType endBatchId)
    {
      vtkNew<vtkIdList> testPointIDs;
      std::vector<vtkTypeInt64> cellConn(numPointIds);
      std::vector<vtkVector3d> cellCorners(numPointIds);
      std::vector<double> xyz(3, 0.0);
      std::vector<double> jacobian(9, 0.0);
      std::vector<vtkConvexHull::Plane> hullPlanes; // per-thread; capacity preserved across cells
      vtkVector3d center;
      double radius;

      for (vtkIdType batchId = beginBatchId; batchId < endBatchId; ++batchId)
      {
        CellBatch& batch = batches[batchId];
        auto& batchNumberOfPoints = batch.Data.PointsOffset;
        for (vtkIdType cellId = batch.BeginId; cellId < batch.EndId; ++cellId)
        {
          auto& pointsInsideCell = cellPoints[cellId];
          // Get corner point IDs and coordinates.
          conn->GetIntegerTuple(cellId, cellConn.data());
          for (int jj = 0; jj < numPointIds; ++jj)
          {
            coords->GetTuple(cellConn[jj], cellCorners[jj].GetData());
          }

          // Use the circumscribed sphere as a conservative candidate filter: any input
          // point outside this sphere cannot be inside the cell.
          centerAndRadiusOfCellPoints(cellCorners, center, radius);
          locator->FindPointsWithinRadius(radius, center.GetData(), testPointIDs);

          // Build convex hull once per cell; used to reject sphere-filtered
          // candidates before the more expensive EvaluatePosition step.
          vtkConvexHull::ComputeConvexHull(cellCorners.data(), numPointIds, dim, hullPlanes);

          vtkVector3d testPoint, rst;
          for (const auto& testPointID : *testPointIDs)
          {
            inputPoints->GetTuple(testPointID, testPoint.GetData());
            // skip points outside the convex hull of the cell's points
            if (!vtkConvexHull::IsPointInside(testPoint, hullPlanes))
            {
              continue;
            }
            // Classify by inverting the reference-to-world mapping. Parametric coordinates
            // are stored on success, eliminating a separate EvaluatePositions pass.
            rst = rstCenter;
            if (EvaluatePosition(calc, dgCell, cellId, testPoint, xyz, jacobian, rst))
            {
              pointsInsideCell.emplace_back(
                testPointID, std::array<double, 3>{ rst[0], rst[1], rst[2] });
              ++batchNumberOfPoints;
            }
          }
        }
      }
    });

  // Trim batches with 0 classified points in-place to skip them in the scatter pass.
  batches.TrimBatches([](const CellBatch& batch) { return batch.Data.PointsOffset == 0; });
  // Convert per-batch point counts to prefix-sum offsets and get the total output size.
  const auto globalSum = batches.BuildOffsetsAndGetGlobalSum();
  const vtkIdType numOutputPoints = globalSum.PointsOffset;

  // Pre-size the output array so the scatter pass can write directly by index.
  alloc.InputPoints.resize(numOutputPoints);

  // Scatter pass: write classified points from cellPoints into alloc.InputPoints
  // at the pre-computed batch offset. Each batch writes to a disjoint slice of
  // the output array, so this pass is fully parallel with no contention.
  vtkSMPTools::For(0, batches.GetNumberOfBatches(),
    [&](vtkIdType beginBatchId, vtkIdType endBatchId)
    {
      for (vtkIdType batchId = beginBatchId; batchId < endBatchId; ++batchId)
      {
        CellBatch& batch = batches[batchId];
        auto batchPointsOffset = batch.Data.PointsOffset;
        for (vtkIdType cellId = batch.BeginId; cellId < batch.EndId; ++cellId)
        {
          auto& pointsInsideCell = cellPoints[cellId];
          for (const auto& [inputPointID, paramCoords] : pointsInsideCell)
          {
            auto& inputPoint = alloc.InputPoints[batchPointsOffset++];
            inputPoint.InputPointId = inputPointID;
            inputPoint.CellId = cellId;
            inputPoint.ParametricCoords = paramCoords;
          }
        }
      }
    });

  return true;
}

/// Invert the reference-to-world mapping for \a testPoint in cell \a cellId via Newton-Raphson.
///
/// \a rst must be set to the initial guess on entry (typically the parametric center of the cell
/// shape). On convergence it holds the parametric coordinates and the function returns true only
/// if those coordinates satisfy \a dgCell->GetSignedParametricDistance() ≤ 0. Returns false if
/// Newton did not converge within 20 iterations or if the converged point lies outside the
/// reference element.
///
/// \a xyz (size ≥ 3) and \a jacobian (size ≥ 9) are caller-supplied working buffers that are
/// reused across calls to amortize allocation cost.
bool vtkDGEvaluator::EvaluatePosition(vtkInterpolateCalculator* calc, vtkDGCell* dgCell,
  vtkIdType cellId, const vtkVector3d& testPoint, std::vector<double>& xyz,
  std::vector<double>& jacobian, vtkVector3d& rst)
{
  for (int iter = 0; iter < 20; ++iter)
  {
    calc->Evaluate(cellId, rst, xyz);
#ifdef VTK_DBG_DGEVAL
    std::cout << "  " << rst << " → " << xyz[0] << " " << xyz[1] << " " << xyz[2] << "\n";
#endif
    vtkVector3d delta(xyz[0] - testPoint[0], xyz[1] - testPoint[1], xyz[2] - testPoint[2]);
    const double paramDist = dgCell->GetSignedParametricDistance(rst);
    const bool isInside = paramDist <= 0.0;
    if (!isInside && (delta.Norm() / paramDist) < 1e-3)
    {
      // Newton diverged;
      return false;
    }
    if (isInside && delta.Norm() < 1e-7)
    {
      // Newton converged; accept only if the parametric point is inside the reference element.
      return true;
    }
    calc->EvaluateDerivative(cellId, rst, jacobian);
    Eigen::Map<Eigen::Matrix<double, 3, 3, Eigen::RowMajor>> map(jacobian.data());
    Eigen::Vector3d edelt(delta[0], delta[1], delta[2]);
    // partialPivLu is sufficient and cheaper than HouseholderQR for the
    // guaranteed 3×3 Jacobian arising from a DG reference-element mapping.
    const Eigen::Vector3d xx = map.partialPivLu().solve(edelt);
#ifdef VTK_DBG_DGEVAL
    std::cout << "AA\n" << map << "\nbb\n" << edelt << "\n => xx\n" << xx << "\n--\n";
#endif
    rst[0] -= xx[0];
    rst[1] -= xx[1];
    rst[2] -= xx[2];
#ifdef VTK_DBG_DGEVAL
    std::cout << "      delta " << edelt[0] << " " << edelt[1] << " " << edelt[2] << "\n";
#endif
  }
  return false; // Newton did not converge within the iteration budget.
}

bool vtkDGEvaluator::InterpolatePoints(
  vtkCellGridEvaluator* query, vtkCellMetadata* cellType, vtkCellGridResponders* caches)
{
  auto* dgCell = dynamic_cast<vtkDGCell*>(cellType);
  if (!dgCell)
  {
    return false;
  }
  auto attribute = query->GetCellAttribute();
  // Get a calculator initialized to work on the attribute we wish to interpolate:
  auto calc = caches->AttributeCalculator<vtkInterpolateCalculator>(
    dgCell, attribute, dgCell->GetAttributeTags(attribute, true));
  if (!calc)
  {
    return false;
  }

  // We must have cell IDs and parametric coordinates to interpolate attributes.
  auto cellIds = query->GetClassifierCellIndices();
  auto pointParams = query->GetClassifierPointParameters();
  auto values = query->GetInterpolatedValues();
  auto& alloc = query->GetAllocationsForCellType(dgCell->GetClassName());
  vtkIdType outputPointId = alloc.Offset; // The start of the output points we will interpolate
  vtkIdType numberOfPoints = alloc.GetNumberOfOutputPoints(); // The number of points to process.

  const vtkIdType end = outputPointId + numberOfPoints;
  vtkSMPTools::For(outputPointId, end,
    [&](vtkIdType bid, vtkIdType eid)
    {
      vtkVector3d rst;
      std::vector<double> value;
      value.resize(attribute->GetNumberOfComponents());
      for (vtkIdType ii = bid; ii < eid; ++ii)
      {
        const vtkTypeUInt64 cellId = cellIds->GetValue(ii);
        pointParams->GetTuple(ii, rst.GetData());
        calc->Evaluate(cellId, rst, value);
        values->SetTuple(ii, value.data());
      }
    });

  return true;
}

VTK_ABI_NAMESPACE_END
