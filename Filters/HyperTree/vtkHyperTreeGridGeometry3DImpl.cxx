// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkHyperTreeGridGeometry3DImpl.h"
#include "vtkBitArray.h"
#include "vtkCellArray.h"
#include "vtkDataSetAttributes.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridNonOrientedVonNeumannSuperCursor.h"
#include "vtkMathUtilities.h"
#include "vtkMergePoints.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkUnsignedCharArray.h"

#include <set>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN
namespace
{
constexpr unsigned int NUMBER_OF_POINTS = 8;
constexpr unsigned int NUMBER_OF_EDGES = 12;
constexpr unsigned int MAX_NUMBER_OF_INTERFACE_EDGES = 8;

// Point Ids for each face of a cell
constexpr unsigned int FACE_PTS_IDS[6][4] = { { 0, 1, 3, 2 }, { 0, 4, 5, 1 }, { 0, 2, 6, 4 },
  { 1, 3, 7, 5 }, { 2, 6, 7, 3 }, { 4, 5, 7, 6 } };

// Edge Ids for each face of a cell
constexpr unsigned int FACE_EDGES_IDS[6][4] = { { 1, 5, 3, 0 }, { 0, 4, 8, 2 }, { 2, 9, 6, 1 },
  { 3, 7, 10, 4 }, { 6, 11, 7, 5 }, { 8, 10, 11, 9 } };

// Point Ids for each edge of a cell
const std::vector<std::pair<unsigned int, unsigned int>> EDGE_PTS_IDS = { { 0, 1 } /*  0 */,
  { 0, 2 } /*  1 */, { 0, 4 } /* 2 */, { 1, 3 } /* 3 */, { 1, 5 } /* 4 */, { 2, 3 } /*  5 */,
  { 2, 6 } /*  6 */, { 3, 7 } /* 7 */, { 4, 5 } /* 8 */, { 4, 6 } /* 9 */, { 5, 7 } /* 10 */,
  { 6, 7 } /* 11 */ };

// Orientation axis for each edge of a cell
// 0:X, 1:Y, 2:Z
constexpr unsigned int EDGE_AXIS[12] = { 0, 1, 2, 1, 2, 0, 2, 2, 0, 1, 1, 0 };

// Flag used to indicate to treat all faces of a coarse cell
// All bytes are set to 1 : all faces should be considered
constexpr unsigned char TREAT_ALL_FACES = std::numeric_limits<unsigned char>::max();

// Neighbor ids of the VonNeumann cursor
constexpr unsigned int VON_NEUMANN_NEIGH_ID[] = { 0, 1, 2, 4, 5, 6 };

// Orientation (normal of the plane) for each face of a cell : 0:YZ, 1:XZ, 2:XY
constexpr unsigned int FACE_ORIENTATION[] = { 2, 1, 0, 0, 1, 2 };

// Indicate if an offset (cell size) should be applied for a given face
// Concerns the faces that do not share the cell origin
constexpr unsigned int FACE_OFFSET[] = { 0, 0, 0, 1, 1, 1 };

// Arbitrary default edge index
// Should be superior to 12, i.e. the number of edges of a given cell
const unsigned int VTK_DEFAULT_EDGE_INDEX = 42;
}

//------------------------------------------------------------------------------
struct vtkHyperTreeGridGeometry3DImpl::HTG3DPoint
{
  double Coords[3] = { 0., 0., 0. };
  bool IsValid = false;
  vtkIdType Id = -1;
  bool HasInterfaceA = false;
  bool HasInterfaceB = false;
  double DistanceToInterfaceA = 0.;
  double DistanceToInterfaceB = 0.;
};

//------------------------------------------------------------------------------
vtkHyperTreeGridGeometry3DImpl::vtkHyperTreeGridGeometry3DImpl(bool mergePoints,
  vtkHyperTreeGrid* input, vtkPoints* outPoints, vtkCellArray* outCells,
  vtkDataSetAttributes* inCellDataAttributes, vtkDataSetAttributes* outCellDataAttributes,
  bool passThroughCellIds, const std::string& originalCellIdArrayName)
  : vtkHyperTreeGridGeometryImpl(input, outPoints, outCells, inCellDataAttributes,
      outCellDataAttributes, passThroughCellIds, originalCellIdArrayName)
{
  if (mergePoints)
  {
    this->Locator = vtkSmartPointer<vtkMergePoints>::New();
    this->Locator->InitPointInsertion(outPoints, input->GetBounds());
  }
  this->BranchFactor = static_cast<int>(this->Input->GetBranchFactor());
  this->InPureMaskArray = this->Input->GetPureMask();
}

//----------------------------------------------------------------------------------------------
vtkHyperTreeGridGeometry3DImpl::~vtkHyperTreeGridGeometry3DImpl() = default;

//----------------------------------------------------------------------------------------------
void vtkHyperTreeGridGeometry3DImpl::GenerateGeometry()
{
  vtkHyperTreeGrid::vtkHyperTreeGridIterator it;
  this->Input->InitializeTreeIterator(it);

  vtkIdType hyperTreeId;
  vtkNew<vtkHyperTreeGridNonOrientedVonNeumannSuperCursor> cursor;

  // Recursively process all HyperTrees
  while (it.GetNextTree(hyperTreeId))
  {
    this->Input->InitializeNonOrientedVonNeumannSuperCursor(cursor, hyperTreeId);
    this->RecursivelyProcessTree(cursor, ::TREAT_ALL_FACES);
  }
}

//----------------------------------------------------------------------------------------------
void vtkHyperTreeGridGeometry3DImpl::RecursivelyProcessTree(
  vtkHyperTreeGridNonOrientedVonNeumannSuperCursor* cursor,
  unsigned char coarseCellFacesToBeTreated)
{
  vtkIdType cellId = cursor->GetGlobalNodeIndex();

  // For a given cell, we can generate faces if the cell is a leaf or if the cell is masked
  if (cursor->IsLeaf() || this->IsMaskedOrGhost(cellId))
  {
    // Improvement: coarseCellFacesToBeTreated can also be used there (not used for now)
    this->GenerateCellSurface(cursor, coarseCellFacesToBeTreated, cellId);
    return;
  }

  // Case of a pure, non-masked coarse cell (optimisation)
  if (this->InPureMaskArray && this->InPureMaskArray->GetValue(cellId) == 0)
  {
    // All child cells are in the same material, so we can only treat
    // the ones at the border of the coarse cell
    std::set<int> childList;
    std::vector<unsigned char> childCellFacesToBeTreated(cursor->GetNumberOfChildren(), 0);

    for (unsigned int f = 0; f < 3; ++f) // dimension
    {
      for (unsigned int o = 0; o < 2; ++o) // left, center, right
      {
        int neighborIdx = (2 * o - 1) * (f + 1);
        if ((coarseCellFacesToBeTreated & (1 << (3 + neighborIdx))))
        {
          bool isValidN = cursor->HasTree(3 + neighborIdx);
          vtkIdType neighboringCellId = 0;
          if (isValidN)
          {
            neighboringCellId = cursor->GetGlobalNodeIndex(3 + neighborIdx);
          }
          if (!isValidN || this->InPureMaskArray->GetValue(neighboringCellId))
          {
            // If the neighboring cells do not exist or are not pure,
            // we have border children
            int iMin = (f == 0 && o == 1) ? this->BranchFactor - 1 : 0;
            int iMax = (f == 0 && o == 0) ? 1 : this->BranchFactor;
            int jMin = (f == 1 && o == 1) ? this->BranchFactor - 1 : 0;
            int jMax = (f == 1 && o == 0) ? 1 : this->BranchFactor;
            int kMin = (f == 2 && o == 1) ? this->BranchFactor - 1 : 0;
            int kMax = (f == 2 && o == 0) ? 1 : this->BranchFactor;
            for (int i = iMin; i < iMax; ++i)
            {
              for (int j = jMin; j < jMax; ++j)
              {
                for (int k = kMin; k < kMax; ++k)
                {
                  unsigned int ichild = i + this->BranchFactor * (j + this->BranchFactor * k);

                  // We can request a border child cell more than one time,
                  // once for each of it's "exposed" face
                  childList.insert(ichild);
                  childCellFacesToBeTreated[ichild] |= (1 << (3 + neighborIdx));
                } // k
              }   // j
            }     // i
          }       // if ...
        }
      } // o
    }   // f
    for (std::set<int>::iterator it = childList.begin(); it != childList.end(); ++it)
    {
      cursor->ToChild(*it);
      this->RecursivelyProcessTree(cursor, childCellFacesToBeTreated[*it]);
      cursor->ToParent();
    } // ichild
    return;
  }

  // Search for any child cell that is not present in the coarse cell material
  for (unsigned int ichild = 0; ichild < cursor->GetNumberOfChildren(); ++ichild)
  {
    cursor->ToChild(ichild);
    this->RecursivelyProcessTree(cursor, ::TREAT_ALL_FACES);
    cursor->ToParent();
  }
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridGeometry3DImpl::GenerateCellSurface(
  vtkHyperTreeGridNonOrientedVonNeumannSuperCursor* cursor,
  unsigned char vtkNotUsed(coarseCellFacesToBeTreated), vtkIdType cellId)
{
  // Determine if the current cell contains an interface and
  // fill the related member variables accordingly
  this->ProbeForCellInterface(cellId);

  // Retrieve info about the current cell
  unsigned level = cursor->GetLevel();
  bool masked = cursor->IsMasked();
  const double* cellOrigin = cursor->GetOrigin();
  const double* cellSize = cursor->GetSize();

  std::vector<HTG3DPoint> cellPoints;
  cellPoints.resize(NUMBER_OF_POINTS);
  std::vector<std::pair<HTG3DPoint, HTG3DPoint>> edgePoints;
  edgePoints.resize(NUMBER_OF_EDGES + MAX_NUMBER_OF_INTERFACE_EDGES);

  std::map<unsigned int, std::pair<HTG3DPoint*, unsigned int>> internalFaceA;
  std::map<unsigned int, std::pair<HTG3DPoint*, unsigned int>> internalFaceB;

  // Iterate over all neighboring cells using the Von Neumann neighborhood
  for (unsigned int faceId = 0; faceId < 6; ++faceId)
  {
    const unsigned int& neighborId = ::VON_NEUMANN_NEIGH_ID[faceId];
    const unsigned int& faceOrientation = ::FACE_ORIENTATION[faceId];
    const unsigned int& faceOffset = ::FACE_OFFSET[faceId];

    // Retrieve cursor to neighbor across face
    // Retrieve tree, leaf flag, and mask of neighbor cursor
    bool leafN = false;
    vtkIdType neighborCellId = 0;
    unsigned int levelN = 0;
    vtkHyperTree* treeN = cursor->GetInformation(neighborId, levelN, leafN, neighborCellId);
    int maskedN = cursor->IsMasked(neighborId);
    bool hasInterfaceCellN = this->GetHasInterface(cursor->GetGlobalNodeIndex(neighborId));

    // We generate a face if one of the following conditions are fulfilled:
    // - The current cell is unmasked, and the neighboring cell is masked
    // - The current cell is unmasked, and has no neighbouring cell
    // - The current cell is unmasked, and has an interface
    // - The current cell is unmasked, and has the neighboring cell has an interface
    // - The current cell is masked, and has a neighbor that is a non-masked leaf of lower level
    // This ensures that faces between unmasked and masked cells will be generated once and only
    // once.
    if ((!masked && (!treeN || maskedN || this->HasInterfaceOnThisCell || hasInterfaceCellN)) ||
      (masked && treeN && leafN && levelN < level && !maskedN))
    {
      // Generate face with corresponding normal and offset
      // Here we differentiate the case where the current cell is masked.
      // In that case, we must copy the data from the neighboring cell to the created face,
      // and not from the current cell.
      this->GenerateOneCellFace(cellPoints, edgePoints, faceId, (masked ? neighborCellId : cellId),
        cellOrigin, cellSize, faceOffset, faceOrientation, internalFaceA, internalFaceB);
    }
  }

  auto createInterfacePoints =
    [&](std::map<unsigned int, std::pair<HTG3DPoint*, unsigned int>>& interface) {
      if (!interface.empty() && interface.size() >= 3)
      {
        std::vector<vtkIdType> newOutputPointIds;
        unsigned int firstEdge = interface.begin()->first;
        if (firstEdge == VTK_DEFAULT_EDGE_INDEX)
        {
          vtkWarningWithObjectMacro(nullptr, "Uninitialized edge encountered");
          return;
        }
        else
        {
          HTG3DPoint* pt = interface[firstEdge].first;
          newOutputPointIds.emplace_back(pt->Id);
          unsigned int next = interface[firstEdge].second;
          while (next != firstEdge && next != VTK_DEFAULT_EDGE_INDEX)
          {
            // XXX: think adding an "emergency" breaking condition to
            // avoid potential infinite looping
            pt = interface[next].first;
            newOutputPointIds.emplace_back(pt->Id);
            next = interface[next].second;
          }
          if (next == VTK_DEFAULT_EDGE_INDEX)
          {
            vtkWarningWithObjectMacro(nullptr, "Uninitialized edge encountered");
            return;
          }
        }

        // XXX: We need to clarify a criterion for valid cells
        // (i.e. that can be added to the output)
        if (!newOutputPointIds.empty())
        {
          this->CreateNewCellAndCopyData(newOutputPointIds, cellId);
        }
      }
    };

  // Create interface points for interface A and B if they are defined
  createInterfacePoints(internalFaceA);
  createInterfacePoints(internalFaceB);
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridGeometry3DImpl::GenerateOneCellFace(std::vector<HTG3DPoint>& cellPoints,
  std::vector<std::pair<HTG3DPoint, HTG3DPoint>>& edgePoints, unsigned int faceId, vtkIdType cellId,
  const double* cellOrigin, const double* cellSize, unsigned int offset, unsigned int orientation,
  std::map<unsigned int, std::pair<HTG3DPoint*, unsigned int>>& internalFaceA,
  std::map<unsigned int, std::pair<HTG3DPoint*, unsigned int>>& internalFaceB)
{
  double pt[3] = { 0., 0., 0. };

  // We compute the current cell points coordinates only if we didn't do it before
  // (i.e. if IsValid == false for a given point)
  // XXX: The code below can be reworked. We can think about computing all
  // cell points once and for all before calling this function and remove all
  // the logic below.
  HTG3DPoint* currentPt = &cellPoints[::FACE_PTS_IDS[faceId][0]];
  if (currentPt->IsValid)
  {
    currentPt = &cellPoints[::FACE_PTS_IDS[faceId][1]];
    if (currentPt->IsValid)
    {
      currentPt = &cellPoints[::FACE_PTS_IDS[faceId][2]];
      if (currentPt->IsValid)
      {
        currentPt = &cellPoints[::FACE_PTS_IDS[faceId][3]];
        if (!currentPt->IsValid)
        {
          memcpy(pt, cellOrigin, 3 * sizeof(double));
          if (offset)
          {
            pt[orientation] += cellSize[orientation];
          }
          unsigned int axis2 = (orientation + 2) % 3;
          pt[axis2] += cellSize[axis2];
          this->SetXYZ(*currentPt, pt);
        }
      }
      else
      {
        memcpy(pt, cellOrigin, 3 * sizeof(double));
        if (offset)
        {
          pt[orientation] += cellSize[orientation];
        }
        unsigned int axis1 = (orientation + 1) % 3;
        unsigned int axis2 = (orientation + 2) % 3;
        pt[axis1] += cellSize[axis1];
        pt[axis2] += cellSize[axis2];
        this->SetXYZ(*currentPt, pt);
        currentPt = &cellPoints[::FACE_PTS_IDS[faceId][3]];
        if (!currentPt->IsValid)
        {
          pt[axis1] = cellOrigin[axis1];
          this->SetXYZ(*currentPt, pt);
        }
      }
    }
    else
    {
      memcpy(pt, cellOrigin, 3 * sizeof(double));
      if (offset)
      {
        pt[orientation] += cellSize[orientation];
      }
      unsigned int axis1 = (orientation + 1) % 3;
      pt[axis1] += cellSize[axis1];
      this->SetXYZ(*currentPt, pt);
      currentPt = &cellPoints[::FACE_PTS_IDS[faceId][2]];
      if (currentPt->IsValid)
      {
        currentPt = &cellPoints[::FACE_PTS_IDS[faceId][3]];
        if (!currentPt->IsValid)
        {
          unsigned int axis2 = (orientation + 2) % 3;
          pt[axis2] += cellSize[axis2];
          this->SetXYZ(*currentPt, pt);
        }
      }
      else
      {
        unsigned int axis2 = (orientation + 2) % 3;
        pt[axis2] += cellSize[axis2];
        this->SetXYZ(*currentPt, pt);
        currentPt = &cellPoints[::FACE_PTS_IDS[faceId][3]];
        if (!currentPt->IsValid)
        {
          pt[axis1] = cellOrigin[axis1];
          this->SetXYZ(*currentPt, pt);
        }
      }
    }
  }
  else
  {
    memcpy(pt, cellOrigin, 3 * sizeof(double));
    if (offset)
    {
      pt[orientation] += cellSize[orientation];
    }
    this->SetXYZ(*currentPt, pt);
    currentPt = &cellPoints[::FACE_PTS_IDS[faceId][1]];
    if (currentPt->IsValid)
    {
      currentPt = &cellPoints[::FACE_PTS_IDS[faceId][2]];
      if (currentPt->IsValid)
      {
        currentPt = &cellPoints[::FACE_PTS_IDS[faceId][3]];
        if (!currentPt->IsValid)
        {
          unsigned int axis2 = (orientation + 2) % 3;
          pt[axis2] += cellSize[axis2];
          this->SetXYZ(*currentPt, pt);
        }
      }
      else
      {
        unsigned int axis1 = (orientation + 1) % 3;
        unsigned int axis2 = (orientation + 2) % 3;
        pt[axis1] += cellSize[axis1];
        pt[axis2] += cellSize[axis2];
        this->SetXYZ(*currentPt, pt);
        currentPt = &cellPoints[::FACE_PTS_IDS[faceId][3]];
        if (!currentPt->IsValid)
        {
          pt[axis1] = cellOrigin[axis1];
          this->SetXYZ(*currentPt, pt);
        }
      }
    }
    else
    {
      unsigned int axis1 = (orientation + 1) % 3;
      pt[axis1] += cellSize[axis1];
      this->SetXYZ(*currentPt, pt);
      currentPt = &cellPoints[::FACE_PTS_IDS[faceId][2]];
      if (currentPt->IsValid)
      {
        currentPt = &cellPoints[::FACE_PTS_IDS[faceId][3]];
        if (!currentPt->IsValid)
        {
          unsigned int axis2 = (orientation + 2) % 3;
          pt[axis2] += cellSize[axis2];
          this->SetXYZ(*currentPt, pt);
        }
      }
      else
      {
        unsigned int axis2 = (orientation + 2) % 3;
        pt[axis2] += cellSize[axis2];
        this->SetXYZ(*currentPt, pt);
        currentPt = &cellPoints[::FACE_PTS_IDS[faceId][3]];
        if (!currentPt->IsValid)
        {
          pt[axis1] = cellOrigin[axis1];
          this->SetXYZ(*currentPt, pt);
        }
      }
    }
  }

  // Storage for new face vertex IDs
  std::vector<vtkIdType> outputIndexPoints;

  unsigned int currentEdgePointA = VTK_DEFAULT_EDGE_INDEX;
  vtkIdType lastId = -1;
  unsigned int currentEdgePointB = VTK_DEFAULT_EDGE_INDEX;

  // Iterate over the edges of the current face to add face points.
  // If there is no interface, simply insert the 4 points of the face.
  // If one or two interfaces pass through the cell, also compute the
  // additional points of the interface (intersection between the interface and
  // the edges of the cell).
  for (unsigned int edgeId = 0; edgeId < 4; ++edgeId)
  {
    unsigned int faceEdgeId = ::FACE_EDGES_IDS[faceId][edgeId];
    std::pair<unsigned int, unsigned int> edgePointId = ::EDGE_PTS_IDS[faceEdgeId];
    this->ComputeEdge(cellPoints[edgePointId.first], cellPoints[edgePointId.second], edgePoints,
      ::EDGE_AXIS[faceEdgeId], faceEdgeId, internalFaceA, internalFaceB, currentEdgePointA,
      currentEdgePointB);

    // The order of points insertion is important in order to considerate all face points.
    // Regarding the way IDs are stored in FACE_PTS_IDS, FACE_EDGES_IDS and EDGE_PTS_IDS,
    // we have to retrieve the first point of the edge for the 1st and 2nd edges,
    // and the second point for the 3rd and 4th edges of the face.
    std::vector<HTG3DPoint*> points;
    if (edgeId < 2)
    {
      points.emplace_back(&cellPoints[edgePointId.first]); // first vertex quad face
      points.emplace_back(&edgePoints[faceEdgeId].first);  // first point one edge
      points.emplace_back(&edgePoints[faceEdgeId].second); // second point one edge
    }
    else
    {
      points.emplace_back(&cellPoints[edgePointId.second]); // second vertex quad face
      points.emplace_back(&edgePoints[faceEdgeId].second);  // second point one edge
      points.emplace_back(&edgePoints[faceEdgeId].first);   // first point one edge
    }
    for (auto point : points)
    {
      if (point->IsValid)
      {
        vtkIdType pointId = -1;
        if (this->IsInside(*point))
        {
          pointId = this->InsertUniquePoint(*point);
        }
        if (pointId >= 0 && pointId != lastId) // lastId is used to avoid repetitions
        {
          outputIndexPoints.emplace_back(pointId);
          lastId = pointId;
        }
      }
    }
  }

  // Insert a new face
  if (outputIndexPoints.size() > 2)
  {
    this->CreateNewCellAndCopyData(outputIndexPoints, cellId);
  }
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridGeometry3DImpl::SetInterfaceFace(unsigned int edgeId,
  std::map<unsigned int, std::pair<vtkHyperTreeGridGeometry3DImpl::HTG3DPoint*, unsigned int>>&
    internalFace,
  vtkHyperTreeGridGeometry3DImpl::HTG3DPoint* point)
{
  if (internalFace.count(edgeId) <= 0)
  {
    internalFace[edgeId].first = point;
    internalFace[edgeId].second = VTK_DEFAULT_EDGE_INDEX;
  }
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridGeometry3DImpl::CompleteLinkage(
  std::map<unsigned int, std::pair<vtkHyperTreeGridGeometry3DImpl::HTG3DPoint*, unsigned int>>&
    internalFace,
  unsigned int edgePointId1, unsigned int edgePointId2)
{
  if (edgePointId1 == VTK_DEFAULT_EDGE_INDEX || edgePointId2 == VTK_DEFAULT_EDGE_INDEX)
  {
    // XXX: this seems to happen quite often.
    // We must clarify if it is intended
    return;
  }
  if (edgePointId1 == edgePointId2)
  {
    vtkErrorWithObjectMacro(nullptr, "Edge with 2 identical points found !");
    return;
  }

  // Build link between edgePointId1 et edgePointId2
  // XXX: need to clarify naming for nextEdgePointId1 and nextEdgePointId2
  unsigned int nextEdgePointId1 = internalFace[edgePointId1].second;
  unsigned int nextEdgePointId2 = internalFace[edgePointId2].second;
  if (nextEdgePointId1 == VTK_DEFAULT_EDGE_INDEX)
  {
    if (nextEdgePointId2 == VTK_DEFAULT_EDGE_INDEX || nextEdgePointId2 != edgePointId1)
    {
      // Arbitrary choice of linking direction,
      // or nextEdgePointId2 already describes a different linking
      internalFace[edgePointId1].second = edgePointId2;
    }
  }
  else if (nextEdgePointId2 == VTK_DEFAULT_EDGE_INDEX)
  {
    // Arbitrary choice of linking direction
    internalFace[edgePointId2].second = edgePointId1;
  }
  else if (nextEdgePointId2 != edgePointId1)
  {
    // If nextEdgePointId1 is involved in a different linkage, this is also the case for
    // nextEdgePointId2 We have to invert both linkages and connect them together
    std::vector<unsigned int> chainette;
    chainette.emplace_back(edgePointId1);
    unsigned int next = internalFace[edgePointId1].second;
    while (next != VTK_DEFAULT_EDGE_INDEX)
    {
      // XXX: think adding an "emergency" breaking condition to
      // avoid potential infinite looping
      chainette.emplace_back(next);
      next = internalFace[next].second;
    }
    unsigned int current = VTK_DEFAULT_EDGE_INDEX;
    for (std::vector<unsigned int>::reverse_iterator it = chainette.rbegin();
         it != chainette.rend(); ++it)
    {
      if (current == VTK_DEFAULT_EDGE_INDEX)
      {
        current = *it;
      }
      else
      {
        next = *it;
        internalFace[current].second = next;
        current = next;
      }
    }
    if (current != edgePointId1)
    {
      vtkWarningWithObjectMacro(nullptr,
        << "Unexpected edge encountered. Expected " << edgePointId1 << ", got " << current << ".");
    }
    internalFace[current].second = edgePointId2;
  }
}

//------------------------------------------------------------------------------
bool vtkHyperTreeGridGeometry3DImpl::ComputeEdgeInterface(const HTG3DPoint& firstPoint,
  const HTG3DPoint& secondPoint, std::vector<std::pair<HTG3DPoint, HTG3DPoint>>& edgePoints,
  unsigned int edgeAxis, unsigned int edgeId,
  std::map<unsigned int, std::pair<HTG3DPoint*, unsigned int>>& internalFace,
  HTG3DPoint& pointInter, unsigned int& edgePointId, bool isInterfaceA)
{
  if (!firstPoint.IsValid)
  {
    vtkWarningWithObjectMacro(nullptr, "First edge point is invalid.");
  }
  if (!secondPoint.IsValid)
  {
    vtkWarningWithObjectMacro(nullptr, "Second edge point is invalid.");
  }

  auto firstPointDist =
    (isInterfaceA ? firstPoint.DistanceToInterfaceA : firstPoint.DistanceToInterfaceB);
  auto secondPointDist =
    (isInterfaceA ? secondPoint.DistanceToInterfaceA : secondPoint.DistanceToInterfaceB);

  if (firstPointDist == 0.)
  {
    if (secondPointDist == 0.)
    {
      // Edge case : the interface corresponds to the edge
      // XXX: need to clarify naming for iEdgePoint1 and iEdgePoint2
      unsigned int iEdgePoint1 = ::EDGE_PTS_IDS[edgeId].first + NUMBER_OF_EDGES;
      edgePoints[iEdgePoint1].first = firstPoint;
      edgePoints[iEdgePoint1].second.IsValid = false;
      this->SetInterfaceFace(iEdgePoint1, internalFace, &edgePoints[iEdgePoint1].first);

      unsigned int iEdgePoint2 = ::EDGE_PTS_IDS[edgeId].second + NUMBER_OF_EDGES;
      edgePoints[iEdgePoint2].first = secondPoint;
      edgePoints[iEdgePoint2].second.IsValid = false;
      this->SetInterfaceFace(iEdgePoint2, internalFace, &edgePoints[iEdgePoint2].first);
      this->CompleteLinkage(internalFace, iEdgePoint1, iEdgePoint2);

      return true;
    }

    // The interface point is a the first point of the cell
    pointInter = firstPoint;
    edgePointId = ::EDGE_PTS_IDS[edgeId].first + NUMBER_OF_EDGES;
  }
  else if (secondPointDist == 0.)
  {
    // The interface point is a the second point of the cell
    pointInter = secondPoint;
    edgePointId = ::EDGE_PTS_IDS[edgeId].second + NUMBER_OF_EDGES;
  }
  else if (firstPointDist * secondPointDist < 0.)
  {
    // Compute the position of the interface point on the edge,
    // between the first and the second point
    double xyz[3] = { 0., 0., 0. };
    memcpy(xyz, firstPoint.Coords, 3 * sizeof(double));
    xyz[edgeAxis] = (secondPointDist * firstPoint.Coords[edgeAxis] -
                      firstPointDist * secondPoint.Coords[edgeAxis]) /
      (secondPointDist - firstPointDist);
    this->SetIntersectXYZ(pointInter, xyz, isInterfaceA);

    if (pointInter.Coords[edgeAxis] == firstPoint.Coords[edgeAxis] ||
      pointInter.Coords[edgeAxis] == secondPoint.Coords[edgeAxis])
    {
      vtkWarningWithObjectMacro(nullptr, "Interface point coincide with an edge point");
      pointInter.IsValid = false;
    }

    edgePointId = edgeId;
  }
  return false;
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridGeometry3DImpl::ComputeEdge(const HTG3DPoint& firstPoint,
  const HTG3DPoint& secondPoint, std::vector<std::pair<HTG3DPoint, HTG3DPoint>>& edgePoints,
  unsigned int edgeAxis, unsigned int edgeId,
  std::map<unsigned int, std::pair<HTG3DPoint*, unsigned int>>& internalFaceA,
  std::map<unsigned int, std::pair<HTG3DPoint*, unsigned int>>& internalFaceB,
  unsigned int& currentEdgePointA, unsigned int& currentEdgePointB)
{
  HTG3DPoint pointA, pointB;
  unsigned int iEdgePointA = edgeId, iEdgePointB = edgeId;

  // Compute the intersection point for the first interface, if any.
  if (firstPoint.HasInterfaceA &&
    this->ComputeEdgeInterface(firstPoint, secondPoint, edgePoints, edgeAxis, edgeId, internalFaceA,
      pointA, iEdgePointA, true))
  {
    return;
  }

  // Compute the intersection point for the second interface, if any.
  if (firstPoint.HasInterfaceB &&
    this->ComputeEdgeInterface(firstPoint, secondPoint, edgePoints, edgeAxis, edgeId, internalFaceB,
      pointB, iEdgePointB, false))
  {
    return;
  }

  // If intersection points are computed, add them to the construction (linkage)
  // of the interface faces
  if (pointA.IsValid)
  {
    if (pointB.IsValid)
    {
      if (pointA.Coords[edgeAxis] < pointB.Coords[edgeAxis])
      {
        if (edgeId == iEdgePointA && edgeId == iEdgePointB)
        {
          edgePoints[edgeId].first = pointA;
          this->SetInterfaceFace(edgeId, internalFaceA, &edgePoints[edgeId].first);

          this->CompleteLinkage(internalFaceA, currentEdgePointA, edgeId);
          currentEdgePointA = edgeId;

          edgePoints[edgeId].second = pointB;
          this->SetInterfaceFace(edgeId, internalFaceB, &edgePoints[edgeId].second);

          this->CompleteLinkage(internalFaceB, currentEdgePointB, edgeId);
          currentEdgePointB = edgeId;
        }
        else
        {
          edgePoints[iEdgePointA].first = pointA;
          this->SetInterfaceFace(iEdgePointA, internalFaceA, &edgePoints[iEdgePointA].first);

          this->CompleteLinkage(internalFaceA, currentEdgePointA, iEdgePointA);
          currentEdgePointA = iEdgePointA;

          edgePoints[iEdgePointB].second = pointB;
          this->SetInterfaceFace(iEdgePointB, internalFaceB, &edgePoints[iEdgePointB].second);

          this->CompleteLinkage(internalFaceB, currentEdgePointB, iEdgePointB);
          currentEdgePointB = iEdgePointB;
        }
      }
      if (pointA.Coords[edgeAxis] > pointB.Coords[edgeAxis])
      {
        if (edgeId == iEdgePointA && edgeId == iEdgePointB)
        {
          edgePoints[edgeId].first = pointB;
          this->SetInterfaceFace(edgeId, internalFaceB, &edgePoints[edgeId].first);

          this->CompleteLinkage(internalFaceB, currentEdgePointB, edgeId);
          currentEdgePointB = edgeId;

          edgePoints[edgeId].second = pointA;
          this->SetInterfaceFace(edgeId, internalFaceA, &edgePoints[edgeId].second);

          this->CompleteLinkage(internalFaceA, currentEdgePointA, edgeId);
          currentEdgePointA = edgeId;
        }
        else
        {
          edgePoints[iEdgePointA].first = pointA;
          edgePoints[iEdgePointA].second.IsValid = false;
          this->SetInterfaceFace(iEdgePointA, internalFaceA, &edgePoints[iEdgePointA].first);

          this->CompleteLinkage(internalFaceA, currentEdgePointA, iEdgePointA);
          currentEdgePointA = iEdgePointA;

          edgePoints[iEdgePointB].second = pointB;
          edgePoints[iEdgePointB].second.IsValid = false;
          this->SetInterfaceFace(iEdgePointB, internalFaceB, &edgePoints[iEdgePointB].second);

          this->CompleteLinkage(internalFaceB, currentEdgePointB, iEdgePointB);
          currentEdgePointB = iEdgePointB;
        }
      }
    }
    else
    {
      edgePoints[iEdgePointA].first = pointA;
      this->SetInterfaceFace(iEdgePointA, internalFaceA, &edgePoints[iEdgePointA].first);

      this->CompleteLinkage(internalFaceA, currentEdgePointA, iEdgePointA);
      currentEdgePointA = iEdgePointA;
    }
  }
  else if (pointB.IsValid)
  {
    edgePoints[iEdgePointB].first = pointB;
    this->SetInterfaceFace(iEdgePointB, internalFaceB, &edgePoints[iEdgePointB].first);

    this->CompleteLinkage(internalFaceB, currentEdgePointB, iEdgePointB);
    currentEdgePointB = iEdgePointB;
  }
}

//------------------------------------------------------------------------------
bool vtkHyperTreeGridGeometry3DImpl::IsInside(const HTG3DPoint& point)
{
  if (!point.IsValid)
  {
    return false;
  }
  if (this->CellInterfaceType == -1)
  {
    if (point.HasInterfaceA)
    {
      if (point.DistanceToInterfaceA < 0)
      {
        return false;
      }
    }
    return true;
  }
  else if (this->CellInterfaceType == 0)
  {
    if (point.DistanceToInterfaceA > 0)
    {
      return false;
    }
    if (point.DistanceToInterfaceB < 0)
    {
      return false;
    }
    return true;
  }
  else if (this->CellInterfaceType == 1)
  {
    if (point.HasInterfaceB)
    {
      if (point.DistanceToInterfaceB > 0)
      {
        return false;
      }
    }
    return true;
  }
  else
  {
    // Pure cell
    return true;
  }
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridGeometry3DImpl::SetXYZ(HTG3DPoint& point, const double* coords)
{
  point.Coords[0] = coords[0];
  point.Coords[1] = coords[1];
  point.Coords[2] = coords[2];

  point.Id = -1;
  if (this->HasInterfaceOnThisCell)
  {
    if (this->CellInterfaceType != 1.)
    {
      point.HasInterfaceA = true;
      point.DistanceToInterfaceA = this->ComputeDistanceToInterfaceA(point.Coords);
    }
    if (this->CellInterfaceType != -1.)
    {
      point.HasInterfaceB = true;
      point.DistanceToInterfaceB = this->ComputeDistanceToInterfaceB(point.Coords);
    }
  }
  point.IsValid = true;
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridGeometry3DImpl::SetIntersectXYZ(
  HTG3DPoint& point, const double* coords, bool isInterfaceA)
{
  point.Coords[0] = coords[0];
  point.Coords[1] = coords[1];
  point.Coords[2] = coords[2];
  point.Id = -1;
  if (isInterfaceA)
  {
    point.HasInterfaceA = true;
    point.DistanceToInterfaceA = 0.;
    if (this->HasInterfaceOnThisCell && this->CellInterfaceType != -1.)
    {
      point.HasInterfaceB = true;
      point.DistanceToInterfaceB = this->ComputeDistanceToInterfaceB(point.Coords);
    }
    else
    {
      point.HasInterfaceB = false;
    }
  }
  else
  {
    point.HasInterfaceB = true;
    point.DistanceToInterfaceB = 0.;
    if (this->HasInterfaceOnThisCell && this->CellInterfaceType != 1.)
    {
      point.HasInterfaceA = true;
      point.DistanceToInterfaceA = this->ComputeDistanceToInterfaceA(point.Coords); // 1 is A
    }
  }
  point.IsValid = true;
}

//------------------------------------------------------------------------------
vtkIdType vtkHyperTreeGridGeometry3DImpl::InsertUniquePoint(HTG3DPoint& point)
{
  if (point.IsValid && point.Id < 0)
  {
    // Insert a point
    if (this->Locator)
    {
      this->Locator->InsertUniquePoint(point.Coords, point.Id);
    }
    else
    {
      point.Id = this->OutPoints->InsertNextPoint(point.Coords);
    }
  }
  return point.Id;
}

//----------------------------------------------------------------------------------------------
bool vtkHyperTreeGridGeometry3DImpl::GetHasInterface(vtkIdType cellId) const
{
  // Only useful in 3D, this method makes it possible to know if the neighboring cell
  // of _inputCellIndex offset is pure or describes an interface.
  // It is pure if:
  // - there is no defined interface (m_hasInterface);
  // - there is no description of the interfaces (m_inputIntercepts);
  // - there is a description of the interfaces but the mixed cell type is not 2
  //   (pure cell) (m_inputIntercepts[2]); -1 and 1 describes a case of a mixed
  //   cell of a material with a single interface, 0 a case of a mixed cell of a
  //   material with a double interface;
  // - there is no description of the normals (m_inputNormals);
  // - there is a description of the normals but not zero.
  if (cellId < 0)
  {
    return false;
  }
  if (!this->HasInterface)
  {
    return false;
  }

  bool ret =
    (this->InIntercepts && ((this->InIntercepts->GetTuple(cellId))[2] < 2) && this->InNormals);
  if (ret)
  {
    double* normal = this->InNormals->GetTuple(cellId);
    ret = !(normal[0] == 0. && normal[1] == 0. && normal[2] == 0.);
  }
  return ret;
}

VTK_ABI_NAMESPACE_END
