// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkDGEvaluator.h"

#include "vtkBoundingBox.h"
#include "vtkCellAttribute.h"
#include "vtkCellGrid.h"
#include "vtkCellGridEvaluator.h"
#include "vtkDGHex.h"
#include "vtkDGTet.h"
#include "vtkDataSetAttributes.h"
#include "vtkIdList.h"
#include "vtkIdTypeArray.h"
#include "vtkInterpolateCalculator.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkStaticPointLocator.h"
#include "vtkStringToken.h"
#include "vtkTypeInt64Array.h"
#include "vtkVector.h"
#include "vtkVectorOperators.h"

#include "vtk_eigen.h"
#include VTK_EIGEN(Eigen)

#include <unordered_set>

// Switch the following to #define to get debug printouts. Beware, these are in a tight loop.
#undef VTK_DBG_DGEVAL

VTK_ABI_NAMESPACE_BEGIN

using namespace vtk::literals;

namespace
{

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
    if (cornerRadius > radius)
    {
      radius = cornerRadius;
    }
  }
}

void getSideHalfspace(vtkVector3d& origin, vtkVector3d& normal, vtkDGCell::Shape sideShape,
  const std::vector<vtkIdType>& sideConn, vtkDGCell::Shape cellShape,
  // const std::vector<std::int64_t>& cellConn,
  const std::vector<vtkVector3d>& cellCorners)
{
  (void)cellShape;
  switch (sideShape)
  {
    default:
      break;
    case vtkDGCell::Shape::Edge:
      origin = cellCorners[sideConn[1]];
      normal = origin - cellCorners[sideConn[0]];
      break;
    case vtkDGCell::Shape::Triangle:
    {
      origin = cellCorners[sideConn[1]];
      auto e0 = origin - cellCorners[sideConn[0]];
      auto e1 = cellCorners[sideConn[2]] - origin;
      normal = (e0).Cross(e1);
    }
    break;
    case vtkDGCell::Shape::Quadrilateral:
    {
      // Find three points in CCW order that are farthest from the cell center.
      // This will always give us a conservative result (no false negative
      // classifications of points as outside when they are in fact inside).

      for (int ii = 0; ii < 4; ++ii)
      {
        // Try the 3 points starting at ii.
        origin = cellCorners[sideConn[(ii + 1) % 4]];
        auto e0 = origin - cellCorners[sideConn[ii]];
        auto e1 = cellCorners[sideConn[(ii + 2) % 4]] - origin;
        normal = (e0).Cross(e1);
        // II. If the fourth point is inside the halfspace of the first three,
        //     then we are done.
        if ((cellCorners[(ii + 3) % 4] - origin).Dot(normal) < 0.)
        {
          break;
        }
        // Note that if we iterate through all permutations it is possible
        // we will not break early due to precision issues (i.e., it is
        // possible for every candidate plane to classify the extra point
        // as outside when the points are close to planar). We don't care
        // about this case as any plane should be good enough.
      }
    }
    break;
  }
  normal.Normalize();
}

} // anonymous namespace

vtkStandardNewMacro(vtkDGEvaluator);

bool vtkDGEvaluator::Query(
  vtkCellGridEvaluator* request, vtkCellMetadata* cellType, vtkCellGridResponders* caches)
{
  (void)caches;

  std::string cellTypeName = cellType->GetClassName();

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
      else if (pass == 1)
      {
        return this->EvaluatePositions(request, cellType, caches);
      }
      else if (pass == 2 && request->GetPhasesToPerform() != vtkCellGridEvaluator::Phases::Classify)
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
  vtkCellGridEvaluator* query, vtkCellMetadata* cellType, vtkCellGridResponders* vtkNotUsed(caches))
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

  auto arrays = shape->GetArraysForCellType(cellType->GetClassName());
  auto it = arrays.find("values"_hash);
  vtkDataArray* coords = nullptr;
  if (it != arrays.end())
  {
    coords = vtkDataArray::SafeDownCast(it->second);
  }
  it = arrays.find("connectivity"_hash);
  vtkTypeInt64Array* conn = nullptr;
  if (it != arrays.end())
  {
    conn = vtkTypeInt64Array::SafeDownCast(it->second);
  }
  if (!conn || !coords)
  {
    return false;
  }
  vtkIdType numCells = conn->GetNumberOfTuples();
  vtkVector3d center;
  double radius;
  vtkNew<vtkIdList> searchPoints;
  // A classifier for determining whether a point is inside or outside a DG cell.
  // We rely on the fact that the DG cells are easy to bound with related convex planes.
  // If the point is inside every boundary's half-space, the point is inside.
  // Otherwise it is outside. (We might handle "on"-surface classification later).
  //
  // For cells of dimension 0, 1, or 2, we also test that the point is not far
  // from the manifold in directions not spanned by the parametric basis.
  // TODO. Note that classification is approximate for quadrilateral faces or
  //       elements if they aren't planar.
  // TODO. Classification is *very* approximate for higher order shapes as we
  //       don't support them yet.
  int numCorners = dgCell->GetNumberOfCorners();
  int dim = dgCell->GetDimension();
  auto cellShape = dgCell->GetShape();
  std::vector<vtkTypeInt64Array::ValueType> cellConn;
  std::vector<vtkVector3d> cellCorners;
  cellConn.resize(numCorners);
  cellCorners.resize(numCorners);
  auto classifier = [&](vtkTypeUInt64 cellId, const vtkVector3d& testPoint,
                      const std::vector<vtkVector3d>& cellCornerData) {
    (void)cellId;
    // Loop over sides of dimension (dim - 1), testing its halfspace.
    int numSideTypes = dgCell->GetNumberOfSideTypes();
    for (int sideType = 0; sideType < numSideTypes; ++sideType)
    {
      auto sideIdRange = dgCell->GetSideRangeForType(sideType);
      auto sideShape = dgCell->GetSideShape(sideIdRange.first);
      int sideDim = vtkDGCell::GetShapeDimension(sideShape);
      if (sideDim < dim - 1)
      {
        // Do not process sides of lower dimensions.
        break;
      }
      for (int sideId = sideIdRange.first; sideId < sideIdRange.second; ++sideId)
      {
        auto& sideConn = dgCell->GetSideConnectivity(sideId);
        vtkVector3d origin;
        vtkVector3d normal;
        getSideHalfspace(origin, normal, sideShape, sideConn, cellShape, cellCornerData);
        // TODO: When dim < 3, we should perform additional checks to ensure
        //       that \a testPoint is close to the cell's manifold.
        //       For 2-d cells, test that the point lies in the cell's plane.
        //       For 1-d cells, test that the point lies on the cell's curve.
        switch (sideDim)
        {
          case 0:
            // TODO: Test that the point lies on/near the edge.
            // Fall through to case 2?
          case 1:
            // TODO: Test that the point lies on/near the surface of the face
            //       as well as in the halfspaces pointing out from the edge endpoints.
            // Fall through to case 2.
          case 2:
            if ((testPoint - origin).Dot(normal) > 0)
            {
              // We can fail immediately; the point must be outside.
              return false;
            }
            break;
        }
      }
    }
    return true;
  };
  auto& alloc = query->GetAllocationsForCellType(cellType->GetClassName());
  auto* inputPoints = query->GetInputPoints();
  vtkNew<vtkIdList> testPointIDs;
  vtkVector3d testPoint;
  for (vtkIdType ii = 0; ii < numCells; ++ii)
  {
    // Get corner point IDs
    conn->GetTypedTuple(ii, cellConn.data());
    // Get corner point coordinates
    for (int jj = 0; jj < numCorners; ++jj)
    {
      coords->GetTuple(cellConn[jj], cellCorners[jj].GetData());
    }
    centerAndRadiusOfCellPoints(cellCorners, center, radius);
    locator->FindPointsWithinRadius(radius, center.GetData(), testPointIDs);
    for (const auto& testPointID : *testPointIDs)
    {
      inputPoints->GetTuple(testPointID, testPoint.GetData());
      if (classifier(ii, testPoint, cellCorners))
      {
        alloc.InputPoints[testPointID].insert(ii);
        if (alloc.InputPoints.size() == static_cast<std::size_t>(inputPoints->GetNumberOfTuples()))
        {
          // We have consumed all the input points with already-processed cells.
          // NB: If a point lies in multiple cells (i.e., cells overlap or one
          // cell forms the boundary of another), then returning now will fail
          // to report all the containing cells).
          // return true;
        }
      }
    }
  }
  return true;
}

bool vtkDGEvaluator::EvaluatePositions(
  vtkCellGridEvaluator* query, vtkCellMetadata* cellType, vtkCellGridResponders* caches)
{
  auto* dgCell = dynamic_cast<vtkDGCell*>(cellType);
  if (!dgCell)
  {
    return false;
  }
  auto grid = dgCell->GetCellGrid();
  auto shape = grid ? grid->GetShapeAttribute() : nullptr;
  auto calc = caches->AttributeCalculator<vtkInterpolateCalculator>(dgCell, shape);
  if (!calc)
  {
    return false;
  }
  auto* inputPoints = query->GetInputPoints();
  auto pointIds = query->GetClassifierPointIDs();
  auto cellIds = query->GetClassifierCellIndices();
  auto pointParams = query->GetClassifierPointParameters();
  auto& alloc = query->GetAllocationsForCellType(dgCell->GetClassName());
  vtkIdType outputPointId = alloc.Offset;
  int numCorners = dgCell->GetNumberOfCorners();
  std::vector<vtkTypeInt64Array::ValueType> cellConn;
  std::vector<vtkVector3d> cellCorners;
  cellConn.resize(numCorners);
  cellCorners.resize(numCorners);
  vtkVector3d testPoint;
  for (const auto& entry : alloc.InputPoints)
  {
    inputPoints->GetTuple(entry.first, testPoint.GetData());
    vtkTypeUInt64 outputPointValue = entry.first;
    for (const auto& cellId : entry.second)
    {
      vtkTypeUInt64 outputCellValue = cellId;
      pointIds->SetTypedTuple(outputPointId, &outputPointValue);
      cellIds->SetTypedTuple(outputPointId, &outputCellValue);

      // Compute the parametric coordinates of \a testPoint by Newton iteration.
      vtkVector3d rst(0., 0., 0.);
      std::vector<double> xyz(3, 0.0);
      std::vector<double> jacobian(9, 0.0);
      bool done = false;
#ifdef VTK_DBG_DGEVAL
      std::cout << "Test point " << entry.first << " " << testPoint << " cell " << cellId << ":\n";
#endif
      // Iterate at most 20 times before giving up:
      for (int ii = 0; !done && ii < 20; ++ii)
      {
        calc->Evaluate(cellId, rst, xyz);
#ifdef VTK_DBG_DGEVAL
        std::cout << "  " << rst << " → " << xyz[0] << " " << xyz[1] << " " << xyz[2] << "\n";
#endif
        vtkVector3d delta(xyz[0] - testPoint[0], xyz[1] - testPoint[1], xyz[2] - testPoint[2]);
        if (delta.Norm() < 1e-7)
        {
          done = true;
          break;
        }
        calc->EvaluateDerivative(cellId, rst, jacobian);
        Eigen::Map<Eigen::Matrix<double, 3, 3, Eigen::RowMajor>> map(&jacobian[0]);
        Eigen::Vector3d edelt(delta[0], delta[1], delta[2]);
        Eigen::HouseholderQR<Eigen::Matrix3d> solver(map);
        auto xx = solver.solve(edelt);
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
      if (done)
      {
        // Clamp to cell boundary as needed.
        done &= dgCell->IsInside(rst);
      }
      if (done)
      {
        pointParams->SetTuple(outputPointId, rst.GetData());
#ifdef VTK_DBG_DGEVAL
        std::cout << "        success to " << outputPointId << " " << rst << "\n";
#endif
      }
      else
      {
        pointParams->SetTuple3(outputPointId, vtkMath::Nan(), vtkMath::Nan(), vtkMath::Nan());
#ifdef VTK_DBG_DGEVAL
        std::cout << "        failure to " << outputPointId << " " << rst << "\n";
#endif
      }

      ++outputPointId;
    }
  }
  return true;
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
  auto calc = caches->AttributeCalculator<vtkInterpolateCalculator>(dgCell, attribute);
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
  vtkVector3d rst;
  vtkTypeUInt64 cellId;
  std::vector<double> value;
  value.resize(attribute->GetNumberOfComponents());

  for (const vtkIdType end = outputPointId + numberOfPoints; outputPointId < end; ++outputPointId)
  {
    cellIds->GetTypedTuple(outputPointId, &cellId);
    pointParams->GetTuple(outputPointId, rst.GetData());
    calc->Evaluate(cellId, rst, value);
    values->SetTuple(outputPointId, &value[0]);
  }

  return true;
}

VTK_ABI_NAMESPACE_END
