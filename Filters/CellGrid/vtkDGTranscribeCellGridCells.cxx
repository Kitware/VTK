// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkDGTranscribeCellGridCells.h"

#include "vtkBoundingBox.h"
#include "vtkCellArray.h"
#include "vtkCellAttribute.h"
#include "vtkCellGrid.h"
#include "vtkCellGridElevationQuery.h"
#include "vtkDGInterpolateCalculator.h"
#include "vtkDGVert.h"
#include "vtkDataSetAttributes.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkIdTypeArray.h"
#include "vtkIncrementalOctreePointLocator.h"
#include "vtkIntArray.h"
#include "vtkObjectFactory.h"
#include "vtkSMPTools.h"
#include "vtkStringToken.h"
#include "vtkTypeInt64Array.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"
#include "vtkVector.h"

#include <sstream>
#include <unordered_set>

VTK_ABI_NAMESPACE_BEGIN

using namespace vtk::literals;

namespace
{

int vtkCellTypeForDGShape(vtkDGCell::Shape shape)
{
  switch (shape)
  {
    case vtkDGCell::Shape::Vertex:
      return VTK_VERTEX;
    case vtkDGCell::Shape::Edge:
      return VTK_LINE;
    case vtkDGCell::Shape::Triangle:
      return VTK_TRIANGLE;
    case vtkDGCell::Shape::Quadrilateral:
      return VTK_QUAD;
    case vtkDGCell::Shape::Tetrahedron:
      return VTK_TETRA;
    case vtkDGCell::Shape::Hexahedron:
      return VTK_HEXAHEDRON;
    case vtkDGCell::Shape::Wedge:
      return VTK_WEDGE;
    case vtkDGCell::Shape::Pyramid:
      return VTK_PYRAMID;
    default:
      break;
  }
  return VTK_EMPTY_CELL;
}

void vtkCellInfoFromDGType(
  vtkCellGridToUnstructuredGrid::Query::OutputAllocation& alloc, vtkDGCell* dgCell)
{
  auto shape = dgCell->GetShape();
  alloc.CellType = vtkCellTypeForDGShape(shape);
  alloc.NumberOfCells = 0;
  alloc.NumberOfConnectivityEntries = 0;
  for (int ii = -1; ii < static_cast<int>(dgCell->GetNumberOfCellSources()); ++ii)
  {
    const auto& source(dgCell->GetCellSource(ii));
    if (source.Blanked)
    {
      continue;
    }

    // Fetch the range of side indices that have the shape corresponding to source.SideType:
    auto sideRange = dgCell->GetSideRangeForType(source.SideType);
    shape = dgCell->GetSideShape(sideRange.first);
    vtkIdType pointsPerSide = vtkDGCell::GetShapeCornerCount(shape);
    vtkIdType numCells = source.Connectivity->GetNumberOfTuples();
    alloc.NumberOfCells += numCells;
    alloc.NumberOfConnectivityEntries += (pointsPerSide + 1) * numCells;
  }
}

// The contributions of cell-grid corner points to
// corner points in the output unstructured grid.
// Attributes are interpolated using the cell IDs
// and parametric coordinates, then summed to the
// output points.
struct Contributions
{
  Contributions() { this->ParametricCoords->SetNumberOfComponents(3); }

  vtkIdType AddContribution(
    vtkIdType outputPointId, vtkIdType inputCellId, const std::array<double, 3>& pcoord)
  {
    vtkIdType nn = this->OutputPointIds->InsertNextValue(outputPointId);
    this->InputCellIds->InsertNextValue(inputCellId);
    this->ParametricCoords->InsertNextTuple(pcoord.data());
    return nn;
  }

  vtkNew<vtkIdTypeArray> OutputPointIds;
  vtkNew<vtkIdTypeArray> InputCellIds;
  vtkNew<vtkDoubleArray> ParametricCoords;
};

using ContributionMap = std::unordered_map<vtkStringToken, Contributions>;

class TranscribeCellGridPointCache : public vtkObject
{
public:
  vtkTypeMacro(TranscribeCellGridPointCache, vtkObject);
  void PrintSelf(std::ostream& os, vtkIndent indent) override
  {
    this->Superclass::PrintSelf(os, indent);
    os << indent << "ContributionsByType: " << this->ContributionsByType.size() << " entries\n";
  }
  static TranscribeCellGridPointCache* New();

  ContributionMap ContributionsByType;
};

Contributions& FetchPointContributionCache(
  vtkCellGridToUnstructuredGrid::Query* request, vtkDGCell* cellType, vtkCellGridResponders* caches)
{
  std::ostringstream cacheName;
  cacheName << "TranscribeCellGridPointCache_" << request;
  vtkStringToken cacheKey(cacheName.str());
  vtkStringToken cellTypeToken(cellType->GetClassName());
  auto data =
    caches->GetCacheDataAs<TranscribeCellGridPointCache>(cacheKey.GetId(), /*createIfAbsent*/ true);
  return data->ContributionsByType[cellTypeToken];
};

void FreePointContributionCache(
  vtkCellGridToUnstructuredGrid::Query* request, vtkDGCell* cellType, vtkCellGridResponders* caches)
{
  std::ostringstream cacheName;
  cacheName << "TranscribeCellGridPointCache_" << request;
  vtkStringToken cacheKey(cacheName.str());
  vtkStringToken cellTypeToken(cellType->GetClassName());
  auto data = caches->GetCacheDataAs<TranscribeCellGridPointCache>(
    cacheKey.GetId(), /*createIfAbsent*/ false);
  if (data)
  {
    data->ContributionsByType.erase(cellTypeToken);
    if (data->ContributionsByType.empty())
    {
      vtkSmartPointer<TranscribeCellGridPointCache> blank;
      caches->SetCacheData(cacheKey.GetId(), blank, /*overwrite*/ true);
    }
  }
};

} // anonymous namespace

vtkStandardNewMacro(vtkDGTranscribeCellGridCells);
vtkStandardNewMacro(TranscribeCellGridPointCache);

void vtkDGTranscribeCellGridCells::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

bool vtkDGTranscribeCellGridCells::Query(
  TranscribeQuery* request, vtkCellMetadata* cellType, vtkCellGridResponders* caches)
{
  (void)caches;

  auto* dgCell = vtkDGCell::SafeDownCast(cellType);
  if (!dgCell)
  {
    return false;
  }

  auto* grid = dgCell->GetCellGrid();
  if (!grid)
  {
    return false;
  }

  switch (request->GetPass())
  {
    case TranscribeQuery::PassType::CountOutputs:
    {
      auto& alloc(request->GetOutputAllocations());
      vtkCellInfoFromDGType(alloc[dgCell->GetClassName()], dgCell);
    }
    break;
    case TranscribeQuery::PassType::GenerateConnectivity:
      this->GenerateConnectivity(request, dgCell, caches);
      break;
    case TranscribeQuery::PassType::GeneratePointData:
      this->GeneratePointData(request, dgCell, caches);
      break;
    default:
      vtkErrorMacro("Unknown pass " << request->GetPass());
  }

  return true;
}

void vtkDGTranscribeCellGridCells::GenerateConnectivity(
  TranscribeQuery* request, vtkDGCell* cellType, vtkCellGridResponders* caches)
{
  vtkStringToken cellTypeToken = cellType->GetClassName();
  auto& alloc = request->GetOutputAllocations();
  auto ait = alloc.find(cellTypeToken);
  if (ait == alloc.end())
  {
    return;
  }
  auto& contribs = FetchPointContributionCache(request, cellType, caches);

  auto* cellArray = request->GetOutput()->GetCells();
  auto* cellTypes = request->GetOutput()->GetCellTypesArray();
  auto* locator = request->GetLocator();
  auto& pointMap = request->GetConnectivityTransform(cellTypeToken);
  auto& pointCounts = request->GetConnectivityCount();
  auto shapeAtt = request->GetInput()->GetShapeAttribute();
  auto shapeInfo = shapeAtt->GetCellTypeInfo(cellTypeToken);
  auto shapePoints = shapeInfo.GetArrayForRoleAs<vtkDataArray>("values"_token);
  auto shapeConn = shapeInfo.GetArrayForRoleAs<vtkDataArray>("connectivity"_token);
  // Insert points, add to map, and write output-cell connectivity
  // NB: We currently assume the shape attribute uses a constant (vertices) or HGRAD
  //     function space. If not, we would need to interpolate values here instead of
  //     copying from the shape attribute.
  for (int ii = -1; ii < static_cast<int>(cellType->GetSideSpecs().size()); ++ii)
  {
    auto& source(cellType->GetCellSource(ii));
    if (source.Blanked)
    {
      continue;
    }
    // source.Connectivity is either the connectivity of the cells (when
    // source.SideType < 0) or (cellId, sideIndex) 2-tuples (when
    // source.SideType >= 0). Either way, the number of tuples is the
    // number of cells corresponding to \a source:
    vtkIdType numSideTuples = source.Connectivity->GetNumberOfTuples();
    std::vector<vtkTypeUInt64> inConn;
    std::vector<vtkIdType> outConn;
    inConn.resize(shapeConn->GetNumberOfComponents());
    outConn.reserve(shapeConn->GetNumberOfComponents());
    std::array<double, 3> xx;
    std::array<vtkTypeUInt64, 2> sideTuple; // (cellId, sideIndex)
    if (source.SideType < 0)
    {
      for (vtkIdType cc = 0; cc < numSideTuples; ++cc)
      {
        outConn.clear();
        // source is the CellSpec.
        source.Connectivity->GetUnsignedTuple(cc, inConn.data());
        int pp = 0;
        for (const auto& inPointId : inConn)
        {
          vtkIdType outPointId;
          shapePoints->GetTuple(inPointId, xx.data());
          if (locator->InsertUniquePoint(xx.data(), outPointId) != 0)
          {
            pointMap[static_cast<vtkIdType>(inPointId)] = outPointId;
          }
          ++pointCounts[outPointId];
          outConn.push_back(outPointId);
          contribs.AddContribution(
            outPointId, cc + source.Offset, cellType->GetCornerParameter(pp));
          ++pp;
        }
        cellArray->InsertNextCell(outConn.size(), outConn.data());
        cellTypes->InsertNextValue(ait->second.CellType);
      }
    }
    else
    {
      // source is a SideSpec; fetch the side 2-tuple, then fetch
      // the cell's connectivity, then fetch a subset of the values
      // using the connectivity and side-connectivity.
      auto sideRange = cellType->GetSideRangeForType(source.SideType);
      auto shape = cellType->GetSideShape(sideRange.first);
      unsigned char sideShapeVTK = vtkCellTypeForDGShape(shape);
      for (vtkIdType cc = 0; cc < numSideTuples; ++cc)
      {
        outConn.clear();
        source.Connectivity->GetUnsignedTuple(cc, sideTuple.data());
        shapeConn->GetUnsignedTuple(sideTuple[0], inConn.data());
        const auto& sideConn = cellType->GetSideConnectivity(sideTuple[1]);
        for (const auto& sidePointId : sideConn)
        {
          vtkIdType inPointId = inConn[sidePointId];
          vtkIdType outPointId;
          shapePoints->GetTuple(inPointId, xx.data());
          if (locator->InsertUniquePoint(xx.data(), outPointId) != 0)
          {
            pointMap[inPointId] = outPointId;
          }
          ++pointCounts[outPointId];
          outConn.push_back(outPointId);
          contribs.AddContribution(
            outPointId, cc + source.Offset, cellType->GetCornerParameter(sidePointId));
        }
        cellArray->InsertNextCell(outConn.size(), outConn.data());
        cellTypes->InsertNextValue(sideShapeVTK);
      }
    }
  }
}

void vtkDGTranscribeCellGridCells::GeneratePointData(
  TranscribeQuery* request, vtkDGCell* cellType, vtkCellGridResponders* caches)
{
  auto& alloc = request->GetOutputAllocations();
  auto ait = alloc.find(cellType->GetClassName());
  if (ait == alloc.end())
  {
    return;
  }
  auto& contribs = FetchPointContributionCache(request, cellType, caches);
  vtkIdType nn = contribs.InputCellIds->GetNumberOfTuples();
  auto& pointWeights = request->GetConnectivityWeights();

  vtkNew<vtkDGInterpolateCalculator> interpolateProto;
  for (const auto& inCellAtt : request->GetInput()->GetCellAttributeList())
  {
    if (inCellAtt == request->GetInput()->GetShapeAttribute())
    {
      continue;
    }
    // TODO: We could handle the "constant"_token function-space differently
    //       (by creating cell-data, not point-data, arrays).
    auto* outputArray = request->GetOutputArray(inCellAtt);
    auto rawCalc = interpolateProto->PrepareForGrid(cellType, inCellAtt);
    auto dgCalc = vtkDGInterpolateCalculator::SafeDownCast(rawCalc);
    vtkNew<vtkDoubleArray> interpResult;
    int nc = inCellAtt->GetNumberOfComponents();
    interpResult->SetNumberOfComponents(nc);
    interpResult->SetNumberOfTuples(nn);
    dgCalc->Evaluate(contribs.InputCellIds, contribs.ParametricCoords, interpResult);
    vtkSMPTools::For(0, nn,
      [&](vtkIdType begin, vtkIdType end)
      {
        std::vector<double> outTuple(nc, 0.);
        std::vector<double> inTuple(nc, 0.);
        for (vtkIdType ii = begin; ii < end; ++ii)
        {
          interpResult->GetTuple(ii, inTuple.data());
          vtkIdType outputPointId = contribs.OutputPointIds->GetValue(ii);
          outputArray->GetTuple(outputPointId, outTuple.data());
          double pw = pointWeights[outputPointId];
          for (int jj = 0; jj < nc; ++jj)
          {
            outTuple[jj] += pw * inTuple[jj];
          }
          outputArray->SetTuple(outputPointId, outTuple.data());
        }
      });
  }
  FreePointContributionCache(request, cellType, caches);
}

VTK_ABI_NAMESPACE_END
