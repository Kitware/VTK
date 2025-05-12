// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkDGSidesResponder.h"

#include "vtkBoundingBox.h"
#include "vtkCellAttribute.h"
#include "vtkCellGrid.h"
#include "vtkCellGridBoundsQuery.h"
#include "vtkCellGridSidesCache.h"
#include "vtkDGCell.h"
#include "vtkDataSetAttributes.h"
#include "vtkIdTypeArray.h"
#include "vtkObjectFactory.h"
#include "vtkStringToken.h"
#include "vtkTypeInt64Array.h"

#include <sstream>
#include <unordered_set>

VTK_ABI_NAMESPACE_BEGIN

using namespace vtk::literals;

vtkStandardNewMacro(vtkDGSidesResponder);

bool vtkDGSidesResponder::Query(
  vtkCellGridSidesQuery* query, vtkCellMetadata* cellType, vtkCellGridResponders* caches)
{
  (void)caches;
  switch (query->GetPass())
  {
    case vtkCellGridSidesQuery::PassWork::HashSides:
      return this->HashSides(query, dynamic_cast<vtkDGCell*>(cellType));
    case vtkCellGridSidesQuery::PassWork::Summarize:
      return this->SummarizeSides(query, dynamic_cast<vtkDGCell*>(cellType));
    case vtkCellGridSidesQuery::PassWork::GenerateSideSets:
      return this->GenerateSideSets(query, dynamic_cast<vtkDGCell*>(cellType));
  }
  return false;
}

bool vtkDGSidesResponder::HashSides(vtkCellGridSidesQuery* query, vtkDGCell* cellType)
{
  auto* grid = cellType ? cellType->GetCellGrid() : nullptr;
  if (!grid)
  {
    vtkErrorMacro("Cells of type \"" << cellType->GetClassName() << "\" have no parent grid.");
    return false;
  }
  std::string cellTypeName = cellType->GetClassName();
  vtkStringToken cellTypeToken(cellTypeName);

  // We use the number of input side-specs to avoid computing sides
  // of sides we are about to insert (if any).
  std::size_t numInputSideSpecs = cellType->GetSideSpecs().size();

  auto& cellSpec = cellType->GetCellSpec();
  auto* conn = cellSpec.Connectivity;
  if (!conn || !conn->IsIntegral())
  {
    vtkErrorMacro("No connectivity or bad cell type.");
    return false;
  }
  auto* ngm = cellSpec.NodalGhostMarks;
  int nc = conn->GetNumberOfComponents();
  std::unordered_set<std::int64_t> pointIDs;
  std::vector<vtkTypeInt64> entry;
  std::vector<vtkIdType> side;
  entry.resize(nc);
  int numSideTypes = cellType->GetNumberOfSideTypes();

  auto* sideCache = query->GetSideCache();
  if (!cellSpec.Blanked)
  {
    // int minSideDim = cellType->GetDimension() - 1; // We only care about sides of dimension d-1.
    // Loop over elements, one per tuple of conn:
    for (vtkIdType ii = 0; ii < conn->GetNumberOfTuples(); ++ii)
    {
      conn->GetIntegerTuple(ii, entry.data());
      // Loop over types of side (one entry per shape) of the element:
      for (int sideType = 0; sideType < numSideTypes; ++sideType)
      {
        auto range = cellType->GetSideRangeForType(sideType);
        auto shape = cellType->GetSideShape(range.first);
        if (!vtkDGSidesResponder::ProcessSidesOfInput(query, shape, cellSpec.SourceShape))
        {
          // Skip any side types not requested.
          continue;
        }
#if 0
        // Only hash sides of dimension d-1
        if (vtkDGCell::GetShapeDimension(shape) < minSideDim)
        {
          break;
        }
#endif
        auto shapeName = vtkDGCell::GetShapeName(shape);
        // Loop over sides of the given type:
        for (int sideIdx = range.first; sideIdx < range.second; ++sideIdx)
        {
          const auto& sideConn = cellType->GetSideConnectivity(sideIdx);
          side.resize(sideConn.size());
          int jj = 0;
          for (const auto& sidePointIndex : sideConn)
          {
            side[jj++] = entry[sidePointIndex];
          }
          if (ngm)
          {
            // If we have ghost markings on nodes, use them to
            // determine whether to skip the side or not.
            // If any nodes are marked hidden, skip the side.
            // If all nodes are marked ghost, skip the side.
            // Otherwise, keep the side.
            bool skipSide = false;
            int ghostNodesThisSide = 0;
            for (const auto& node : side)
            {
              int gv = static_cast<int>(ngm->GetTuple1(node));
              if (gv & vtkDataSetAttributes::PointGhostTypes::HIDDENPOINT)
              {
                skipSide = true;
                break;
              }
              if (gv & vtkDataSetAttributes::PointGhostTypes::DUPLICATEPOINT)
              {
                ++ghostNodesThisSide;
              }
            }
            if (ghostNodesThisSide == static_cast<int>(side.size()))
            {
              skipSide = true;
            }
            if (skipSide)
            {
              // Do not emit the hashed side.
              continue;
            }
          }
          // Hash the sideIdx'th side of element ii and add it to the query's storage.
          sideCache->AddSide(cellTypeToken, ii, sideIdx, shapeName, side);
        }
      }
    }
  }

  // Process sideSpecs as well. This will allow things like extracting edges of
  // boundary faces for "Surface with edges" representations.
  for (std::size_t ii = 0; ii < numInputSideSpecs; ++ii)
  {
    const auto& sideSpec = cellType->GetSideSpecs()[ii];
    // Skip blanked sides or sides that are renderable (if skipping sides of renderable inputs).
    bool shouldOmit = (query->GetOmitSidesForRenderableInputs() &&
      vtkDGCell::GetShapeDimension(sideSpec.SourceShape) <= 2);
    if (sideSpec.Blanked || shouldOmit)
    {
      continue;
    }
    auto* sides = sideSpec.Connectivity;
    if (!sides || !sides->IsIntegral())
    {
      auto shapeName = vtkDGCell::GetShapeName(sideSpec.SourceShape);
      if (!sides)
      {
        vtkErrorMacro("No side array for " << shapeName.Data() << ".");
      }
      else
      {
        vtkErrorMacro("Side array for " << shapeName.Data() << " is \""
                                        << sideSpec.Connectivity->GetClassName()
                                        << "\", not an integral storage type. Skipping.");
      }
      continue;
    }

    std::array<vtkTypeInt64, 2> sideEntry;
    // Loop over input sides, one per tuple of sides:
    for (vtkIdType ss = 0; ss < sides->GetNumberOfTuples(); ++ss)
    {
      sides->GetIntegerTuple(ss, sideEntry.data());
      conn->GetIntegerTuple(sideEntry[0], entry.data());
      std::set<int> sidesVisited;
      // Recursively call cellType->GetSidesOfSide() and then fetch side connectivity
      // as directed by the query.
      const auto& sidesOfSide = cellType->GetSidesOfSide(sideEntry[1]);
      this->HashSidesOfSide(query, cellType, sideSpec.SourceShape, side, sidesOfSide, sideEntry[0],
        entry, sidesVisited, ngm);
    }
  }

  return true;
}

bool vtkDGSidesResponder::SummarizeSides(vtkCellGridSidesQuery* query, vtkDGCell* cellType)
{
  auto& hashes = query->GetSideCache()->GetHashes();
  auto& sides = query->GetSides();
  vtkStringToken cellTypeToken(cellType->GetClassName());

  // We avoid range-based iteration so we can erase each hash-entry we process.
  auto it = hashes.begin();
  for (auto nx = it; it != hashes.end(); it = nx)
  {
    nx = it;
    ++nx;

    if (it->second.Sides.empty())
    {
      continue;
    }

    const auto& sidesOfHash(it->second.Sides);
    const auto& firstSide(*sidesOfHash.begin());
    if (firstSide.CellType == cellTypeToken)
    {
      switch (query->GetStrategy())
      {
        case vtkCellGridSidesQuery::Winding:
          if (sidesOfHash.size() % 2 == 0)
          {
            hashes.erase(it);
            continue; // Do not output evenly-paired faces.
          }
          break;

        case vtkCellGridSidesQuery::SummaryStrategy::AnyOccurrence:
          // Always consume and output the hash.
          break;

        case vtkCellGridSidesQuery::SummaryStrategy::Boundary:
          // For 3-D cells, output lone faces but all edges/verts:
          switch (cellType->GetDimension())
          {
            case 3:
              switch (firstSide.SideShape.GetId())
              {
                default:
                case "quadrilateral"_hash:
                case "triangle"_hash:
                  if (sidesOfHash.size() % 2 == 0)
                  {
                    hashes.erase(it);
                    continue; // Do not output evenly-paired faces.
                  }
                  break;
                case "edge"_hash:
                case "vertex"_hash:
                  // Always output a single side.
                  break;
              }
              break;
            case 2:
              switch (firstSide.SideShape.GetId())
              {
                case "edge"_hash:
                  if (sidesOfHash.size() % 2 == 0)
                  {
                    hashes.erase(it);
                    continue; // Do not output evenly paired edges.
                  }
                  break;
                default:
                  // Always output vertices.
                  break;
              }
              break;
            case 1:
              if (sidesOfHash.size() % 2 == 0)
              {
                hashes.erase(it);
                continue; // Do not output evenly paired vertices.
              }
              break;
          }
      }
      sides[firstSide.CellType][firstSide.SideShape][firstSide.DOF].insert(firstSide.SideId);
      hashes.erase(it);
    }
  }

  return true;
}

bool vtkDGSidesResponder::GenerateSideSets(vtkCellGridSidesQuery* query, vtkDGCell* cellType)
{
  auto* grid = cellType ? cellType->GetCellGrid() : nullptr;
  if (!grid)
  {
    vtkErrorMacro("Cells of type \"" << cellType->GetClassName() << "\" have no parent grid.");
    return false;
  }

  vtkIdType offset = 0;
  // If we generated any side-sets, then turn off the output grid's cells
  // unless either (a) they are of dimension 2 or less AND the query is configured
  // or (b) the input's cells were already blanked.
  bool shouldBlankCells = cellType->GetDimension() > 2 || !query->GetPreserveRenderableInputs();
  auto sideSetArrays = query->GetSideSetArrays(cellType->GetClassName());
  auto& cellSpec = cellType->GetCellSpec();
  if (shouldBlankCells || cellSpec.Blanked)
  {
    cellSpec.Blanked = true;
    cellSpec.Offset = offset;
  }
  else
  {
    if (cellSpec.Blanked)
    {
      offset += cellSpec.Connectivity->GetNumberOfTuples();
    }
  }
  // std::cout
  //   << cellType->GetClassName() << " blanked? "
  //   << (cellSpec.Blanked ? "Y" : "N") << "\n";

  auto& sideSpecs = cellType->GetSideSpecs();

  // Unless we are preserving sides of sides, blank the input's original
  // side specs out. Update the offsets for all side-specs since we may
  // have blanked the cells.
  for (auto& sideSpec : sideSpecs)
  {
    if (!query->GetPreserveRenderableInputs())
    {
      sideSpec.Blanked = true;
      sideSpec.Offset = offset;
    }
    else if (!sideSpec.Blanked)
    {
      sideSpec.Offset = offset;
      offset += sideSpec.Connectivity->GetNumberOfTuples();
    }
  }

  // Sort the entries by descending shape.
  // Although there is no requirement they be arranged this way, it simplifies
  // debugging, testing, and user expectations. This can be relaxed if it ever
  // causes performance problems.
  std::map<vtkDGCell::Shape, vtkSmartPointer<vtkIdTypeArray>> orderedSideSets;
  for (const auto& sideSetArray : sideSetArrays)
  {
    auto sideShape = vtkDGCell::GetShapeEnum(sideSetArray.SideShape);
    orderedSideSets[sideShape] = sideSetArray.Sides;
  }

  // Now add new side sets as computed in the first pass.
  offset = (sideSpecs.empty() || sideSpecs.back().Blanked)
    ? offset
    : sideSpecs.back().Offset + sideSpecs.back().Connectivity->GetNumberOfTuples();
  for (auto sideSetEntry = orderedSideSets.rbegin(); sideSetEntry != orderedSideSets.rend();
       ++sideSetEntry)
  {
    std::ostringstream groupName;
    auto sideShape = sideSetEntry->first;
    auto sideArray = sideSetEntry->second;
    auto sideShapeName = vtkDGCell::GetShapeName(sideShape);
    groupName << sideShapeName.Data() << " sides of " << cellType->GetClassName();
    vtkStringToken groupToken(groupName.str());
    auto* sideGroup = grid->GetAttributes(groupToken.GetId());
    int sideType = cellType->GetSideTypeForShape(sideShape);
    sideGroup->AddArray(sideArray);
    sideGroup->SetScalars(sideArray);
    sideSpecs.emplace_back(sideArray, offset, /*blank*/ false, sideShape, sideType);
    // Store which shapes should be selected upon user picking in the side-specification
    // so it will be available during rendering/processing:
    sideSpecs.back().SelectionType =
      query->GetSelectionType() == vtkCellGridSidesQuery::SelectionMode::Input
      ? -1
      : sideSpecs.back().SideType;
    // Copy the parent cell's nodal ghost markings (if any).
    sideSpecs.back().NodalGhostMarks = cellSpec.NodalGhostMarks;
    // Properly offset any subsequent side-specs:
    offset += sideArray->GetNumberOfTuples();
  }

  return true;
}

bool vtkDGSidesResponder::ProcessSidesOfInput(
  vtkCellGridSidesQuery* query, vtkDGCell::Shape sideShape, vtkDGCell::Shape inputShape)
{
  int outputControl = query->GetOutputDimensionControl();
  int inputDim = vtkDGCell::GetShapeDimension(inputShape);
  int sideDim = vtkDGCell::GetShapeDimension(sideShape);
  if (query->GetOmitSidesForRenderableInputs() && inputDim <= 2)
  {
    return false;
  }
  switch (inputDim)
  {
    case 0: // vertices
      return false;
    case 1: // edges
      switch (sideDim)
      {
        case 0: // vertices of edges
          return outputControl & vtkCellGridSidesQuery::VerticesOfEdges;
        default:
          return false;
      }
    case 2:
      switch (sideDim)
      {
        case 0: // vertices of a surface
          return outputControl & vtkCellGridSidesQuery::VerticesOfSurfaces;
        case 1: // edges of a surface
          return outputControl & vtkCellGridSidesQuery::EdgesOfSurfaces;
        default:
          return false;
      }
    case 3:
      switch (sideDim)
      {
        case 0:
          return outputControl & vtkCellGridSidesQuery::VerticesOfVolumes;
        case 1:
          return outputControl & vtkCellGridSidesQuery::EdgesOfVolumes;
        case 2:
          return outputControl & vtkCellGridSidesQuery::SurfacesOfVolumes;
        default:
          return false;
      }
  }
  return false;
}

void vtkDGSidesResponder::HashSidesOfSide(vtkCellGridSidesQuery* query, vtkDGCell* cellType,
  vtkDGCell::Shape sourceShape, std::vector<vtkIdType>& side,
  const std::vector<vtkIdType>& sidesToHash, vtkIdType cellId,
  const std::vector<vtkTypeInt64>& entry, std::set<int>& sidesVisited, vtkDataArray* ngm)
{
  auto* sideCache = query->GetSideCache();
  vtkStringToken cellTypeToken = cellType->GetClassName();
  for (const auto& sideId : sidesToHash)
  {
    if (sidesVisited.find(sideId) != sidesVisited.end())
    {
      continue;
    }
    sidesVisited.insert(sideId);

    auto sideOfSideShape = cellType->GetSideShape(sideId);
    if (vtkDGSidesResponder::ProcessSidesOfInput(query, sideOfSideShape, sourceShape))
    {
      const auto& localSideConn = cellType->GetSideConnectivity(sideId);
      side.resize(localSideConn.size());
      int jj = 0;
      for (const auto& sidePointIndex : localSideConn)
      {
        side[jj++] = entry[sidePointIndex];
      }
      if (ngm)
      {
        // If we have ghost markings on nodes, use them to
        // determine whether to skip the side or not.
        // If any nodes are marked hidden, skip the side.
        // If all nodes are marked ghost, skip the side.
        // Otherwise, keep the side.
        bool skipSide = false;
        int ghostNodesThisSide = 0;
        for (const auto& node : side)
        {
          int gv = static_cast<int>(ngm->GetTuple1(node));
          if (gv & vtkDataSetAttributes::PointGhostTypes::HIDDENPOINT)
          {
            skipSide = true;
            break;
          }
          if (gv & vtkDataSetAttributes::PointGhostTypes::DUPLICATEPOINT)
          {
            ++ghostNodesThisSide;
          }
        }
        if (ghostNodesThisSide == static_cast<int>(side.size()))
        {
          skipSide = true;
        }
        if (skipSide)
        {
          // Do not emit the hashed side.
          continue;
        }
      }
      // Hash the sideId'th side of cellId and add it to the query's storage.
      sideCache->AddSide(
        cellTypeToken, cellId, sideId, vtkDGCell::GetShapeName(sideOfSideShape), side);
    }
    // Regardless of whether we hashed the side, compute any child sides and recurse
    const auto& childSides = cellType->GetSidesOfSide(sideId);
    if (!childSides.empty())
    {
      this->HashSidesOfSide(
        query, cellType, sideOfSideShape, side, childSides, cellId, entry, sidesVisited, ngm);
    }
  }
}

VTK_ABI_NAMESPACE_END
