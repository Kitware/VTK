// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkResolveHangingNodes.h"

#include "vtkAbstractPointLocator.h"
#include "vtkAffineArray.h"
#include "vtkCartesianGrid.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataArrayRange.h"
#include "vtkDataObjectTypes.h"
#include "vtkDataSetAttributesFieldList.h"
#include "vtkDoubleArray.h"
#include "vtkFieldData.h"
#include "vtkHexahedron.h"
#include "vtkIdList.h"
#include "vtkIdTypeArray.h"
#include "vtkImplicitArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkNew.h"
#include "vtkNonOverlappingAMR.h"
#include "vtkObjectFactory.h"
#include "vtkOverlappingAMR.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkPointData.h"
#include "vtkPointSet.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkPolygon.h"
#include "vtkQuad.h"
#include "vtkSMPThreadLocal.h"
#include "vtkSMPThreadLocalObject.h"
#include "vtkSMPTools.h"
#include "vtkSmartPointer.h"
#include "vtkStaticElementLocator.h"
#include "vtkStaticPointLocator.h"
#include "vtkTetra.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"
#include "vtkUnstructuredGridSides.h"
#include "vtkVector.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <bitset>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <limits>
#include <map>
#include <memory>
#include <numeric>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN
vtkObjectFactoryNewMacro(vtkResolveHangingNodes);

namespace
{
//------------------------------------------------------------------------------
// The type of a 2D cell from its number of points.
constexpr unsigned char GetPolygonType(vtkIdType numPts)
{
  if (numPts == 3)
  {
    return VTK_TRIANGLE;
  }
  return numPts == 4 ? VTK_QUAD : VTK_POLYGON;
}

//------------------------------------------------------------------------------
// Decomposition of a quad with hanging points on its edges into triangles and quads: a 2D cell,
// or a face of a hexahedron, whose triangles and quads are then coned into tetrahedra and
// pyramids (see EmitOutputCells). The local indices 0..3 are the corners of the quad, in its order,
// and 4..7 the hanging points of its edges 0..3, the edge k going from the corner k to the corner
// k + 1. The triangles and quads have the orientation of the quad.
//
// A face is shared by 2 cells, which see it rotated and mirrored, possibly on different processes
// with different point ids. So the decomposition of a quad only depends on which of its edges are
// split: the quad is rotated to a canonical case (see GetCase), whose decomposition is invariant by
// the rotations and mirrors that keep its split edges. The canonical cases are 0, 1, 3, 5, 7
// and 15. A single hanging point gives the fan of 3 triangles from it, as the 3D templates without
// new point do (see EmitOutputCells).
class QuadSplitCases
{
public:
  struct QuadCase
  {
    std::uint8_t NumCells;
    // Connectivity layout: [nPts, i0, i1..., nPts, i0, i1..., -1]
    std::array<std::int8_t, 24> Indices;
  };

  // The case of a quad whose split edges are `splitEdges`, with the local indices of the quad: the
  // first rotation of the quad that is a canonical case. Several rotations can be canonical (cases
  // 5 and 15), and they give the same decomposition.
  static QuadCase GetCase(const std::bitset<4>& splitEdges)
  {
    int rotation = 0;
    QuadCase splitCase{};
    for (; rotation < 4; ++rotation)
    {
      std::bitset<4> canonicalSplitEdges;
      for (int k = 0; k < 4; ++k)
      {
        canonicalSplitEdges[k] = splitEdges[(k + rotation) % 4];
      }
      splitCase = Cases[canonicalSplitEdges.to_ulong()];
      if (splitCase.NumCells > 0)
      {
        break;
      }
    }
    assert(splitCase.NumCells > 0);

    // The local index i of the canonical quad is the local index (i + rotation) % 4 of the quad
    // for a corner, and the same for the hanging point of an edge.
    std::int8_t* index = splitCase.Indices.data();
    for (int c = 0; c < splitCase.NumCells; ++c)
    {
      const int numCellPts = *index++;
      for (int k = 0; k < numCellPts; ++k, ++index)
      {
        *index = static_cast<std::int8_t>(
          *index < 4 ? (*index + rotation) % 4 : 4 + (*index - 4 + rotation) % 4);
      }
    }
    return splitCase;
  }

private:
  // clang-format off
  static constexpr QuadCase Cases[16] = {
    // Case 0 (0000): no split edge
    { 1, {{ 4, 0, 1, 2, 3, -1 }} },

    // Case 1 (0001): edge 0 split, the fan from M0
    { 3, {{ 3, 4, 1, 2,  3, 4, 2, 3,  3, 4, 3, 0, -1 }} },

    // Case 2 (0010): unused, case 1 rotated
    { 0, {{ -1 }} },

    // Case 3 (0011): edges 0 and 1 split, symmetric across the diagonal (1, 3)
    { 3, {{ 3, 4, 1, 5,  4, 0, 4, 5, 2,  3, 0, 2, 3, -1 }} },

    // Case 4 (0100): unused, case 1 rotated
    { 0, {{ -1 }} },

    // Case 5 (0101): edges 0 and 2 split
    { 2, {{ 4, 0, 4, 6, 3,  4, 4, 1, 2, 6, -1 }} },

    // Case 6 (0110): unused, case 3 rotated
    { 0, {{ -1 }} },

    // Case 7 (0111): edges 0, 1 and 2 split, symmetric across the axis through M1
    { 4, {{ 3, 4, 1, 5,  3, 5, 2, 6,  3, 4, 5, 6,  4, 0, 4, 6, 3, -1 }} },

    // Case 8 (1000): unused, case 1 rotated
    { 0, {{ -1 }} },

    // Case 9 (1001): unused, case 3 rotated
    { 0, {{ -1 }} },

    // Case 10 (1010): unused, case 5 rotated
    { 0, {{ -1 }} },

    // Case 11 (1011): unused, case 7 rotated
    { 0, {{ -1 }} },

    // Case 12 (1100): unused, case 3 rotated
    { 0, {{ -1 }} },

    // Case 13 (1101): unused, case 7 rotated
    { 0, {{ -1 }} },

    // Case 14 (1110): unused, case 7 rotated
    { 0, {{ -1 }} },

    // Case 15 (1111): all the edges split
    { 5, {{ 3, 0, 4, 7,  3, 4, 1, 5,  3, 5, 2, 6,  3, 6, 3, 7,  4, 4, 5, 6, 7, -1 }} },
  };
  // clang-format on
};

//------------------------------------------------------------------------------
using FaceEdgesTable = std::array<std::array<std::uint8_t, vtkHexahedron::MaximumFaceSize>,
  vtkHexahedron::NumberOfFaces>;

// For each face of a hexahedron, its 4 edges: the edge k goes from the corner k to the corner
// k + 1.
FaceEdgesTable BuildFaceEdgesTable()
{
  FaceEdgesTable table{};
  for (int f = 0; f < vtkHexahedron::NumberOfFaces; ++f)
  {
    const vtkIdType* facePts = vtkHexahedron::GetFaceArray(f);
    for (int k = 0; k < vtkHexahedron::MaximumFaceSize; ++k)
    {
      const vtkIdType p0 = facePts[k];
      const vtkIdType p1 = facePts[(k + 1) % vtkHexahedron::MaximumFaceSize];

      int foundEdge = -1;
      for (int e = 0; e < vtkHexahedron::NumberOfEdges; ++e)
      {
        const vtkIdType* edgePts = vtkHexahedron::GetEdgeArray(e);
        if ((edgePts[0] == p0 && edgePts[1] == p1) || (edgePts[0] == p1 && edgePts[1] == p0))
        {
          foundEdge = e;
          break;
        }
      }
      table[f][k] = static_cast<std::uint8_t>(foundEdge);
    }
  }
  return table;
}

//------------------------------------------------------------------------------
const FaceEdgesTable FaceEdges = BuildFaceEdgesTable();

//------------------------------------------------------------------------------
struct FaceCanonicalMap
{
  std::array<int, 4> P; // The corners of the face
  std::array<int, 4> E; // Its edges, the edge k going from P[k] to P[k + 1] (sub-primitives 0..11)
  std::array<int, 4> Opp; // The corner linked to P[k] by an edge that is not an edge of the face,
                          // the apex of the pyramid on the quarter of the split face at P[k]
};

//------------------------------------------------------------------------------
using FaceToHexTable = std::array<FaceCanonicalMap, vtkHexahedron::NumberOfFaces>;

//------------------------------------------------------------------------------
// The corners, edges and opposite corners of the faces of a hexahedron.
FaceToHexTable BuildFaceToHexTable()
{
  FaceToHexTable table{};
  for (int f = 0; f < vtkHexahedron::NumberOfFaces; ++f)
  {
    FaceCanonicalMap& map = table[f];
    const vtkIdType* facePts = vtkHexahedron::GetFaceArray(f);

    // The corners and the edges
    for (int k = 0; k < 4; ++k)
    {
      map.P[k] = static_cast<int>(facePts[k]);
      map.E[k] = FaceEdges[f][k];
    }

    // The opposite corners, at the other end of the edge leaving each corner that is not an edge of
    // the face
    for (int k = 0; k < 4; ++k)
    {
      for (int e = 0; e < vtkHexahedron::NumberOfEdges; ++e)
      {
        const vtkIdType* edgePts = vtkHexahedron::GetEdgeArray(e);
        int other = -1;
        if (edgePts[0] == map.P[k])
        {
          other = static_cast<int>(edgePts[1]);
        }
        else if (edgePts[1] == map.P[k])
        {
          other = static_cast<int>(edgePts[0]);
        }
        if (other >= 0 && std::find(map.P.begin(), map.P.end(), other) == map.P.end())
        {
          map.Opp[k] = other;
          break;
        }
      }
    }
  }
  return table;
}

//------------------------------------------------------------------------------
const FaceToHexTable FaceToHex = BuildFaceToHexTable();

//------------------------------------------------------------------------------
// For each edge, the 4 faces that do not contain it: the bases of the pyramids with the hanging
// point of the edge as apex.
using EdgeToHexTable = std::array<std::array<int, 4>, vtkHexahedron::NumberOfEdges>;

//------------------------------------------------------------------------------
// The EdgeToHex table, from the faces of vtkHexahedron.
EdgeToHexTable BuildEdgeToHexTable()
{
  EdgeToHexTable table{};
  for (int e = 0; e < vtkHexahedron::NumberOfEdges; ++e)
  {
    int next = 0;
    for (int f = 0; f < vtkHexahedron::NumberOfFaces; ++f)
    {
      if (std::find(FaceEdges[f].begin(), FaceEdges[f].end(), e) == FaceEdges[f].end())
      {
        table[e][next++] = f;
      }
    }
  }
  return table;
}

//------------------------------------------------------------------------------
const EdgeToHexTable EdgeToHex = BuildEdgeToHexTable();

//------------------------------------------------------------------------------
// A candidate cell: a quad (Dim = 2) or hexahedron (Dim = 3) that may have hanging
// points. Its sub-primitives are its 4 edges in 2D, and its 12 edges then its 6 faces in 3D. It
// keeps which of them are searched for hanging points and which are split, with their hanging
// point, and decomposes the resolved cell.
template <int Dim>
class CandidateCellInfoT
{
public:
  static constexpr int NumPoints = (Dim == 2) ? 4 /*quad*/ : 8 /*hex*/;
  static constexpr int NumEdges = (Dim == 2) ? 4 /*quad*/ : 12 /*hex*/;
  static constexpr int NumSubPrimitives = (Dim == 2) ? 4 /*edges*/ : 12 /*edges*/ + 6 /*faces*/;
  // The sides (edges in 2D, faces in 3D) are the sub-primitives [FirstSide, NumSubPrimitives).
  static constexpr int FirstSide = (Dim == 2) ? 0 : NumEdges;
  // The number of points of a side: a line (2D) or a quad (3D).
  static constexpr int NumSidePoints = (Dim == 2) ? 2 : 4;
  // A side as a tuple of its sorted point ids, for the locators: an edge (2D) or a quad (3D).
  using SideTuple = std::conditional_t<Dim == 2, EdgeTuple<vtkIdType>, QuadTuple<vtkIdType>>;
  static constexpr unsigned char CellType = (Dim == 2) ? VTK_QUAD : VTK_HEXAHEDRON;
  static constexpr const char* MeasureName = (Dim == 2) ? "Area" : "Volume";
  static constexpr const char* InverseMeasureName = (Dim == 2) ? "InverseArea" : "InverseVolume";

  using SubPrimitivesBitset = std::bitset<NumSubPrimitives>;

  CandidateCellInfoT() { this->MidPoints.fill(-1); }

  void SetOriginalCellId(vtkIdType cellId) { this->OriginalCellId = cellId; }
  vtkIdType GetOriginalCellId() const { return this->OriginalCellId; }

  // The sub-primitives on which hanging points are searched:
  // - the sides (from FirstSide) without a neighbor, see FindSidesToSearch, and once the local
  //   points are inserted, those of the true boundary, see RestrictCandidateCells;
  // - in 3D, the edges of these faces, see FindEdgesToSearch.
  //   Testing whether an edge has a neighbor does not work in 3D: the edge of a coarse cell can be
  //   shared by other coarse cells and still have a hanging point, from finer cells on the other
  //   side of a face or of the edge. All the cells sharing such an edge search it, so they all
  //   find its hanging point.
  // The sub-primitives that are split are not searched anymore.
  SubPrimitivesBitset GetSubPrimitivesToSearch() const { return this->SubPrimitivesToSearch; }
  void SetSubPrimitivesToSearch(const SubPrimitivesBitset& bitset)
  {
    this->SubPrimitivesToSearch = bitset;
  }
  void SetSubPrimitiveToSearch(int subIdx) { this->SubPrimitivesToSearch[subIdx] = true; }
  void ResetSubPrimitiveToSearch(int subIdx) { this->SubPrimitivesToSearch[subIdx] = false; }
  bool IsSubPrimitiveToSearch(int subIdx) const { return this->SubPrimitivesToSearch[subIdx]; }

  // The sides to search only, i.e., without the edges to search in 3D.
  SubPrimitivesBitset GetSidesToSearch() const;
  bool HasAnySidesToSearch() const { return this->GetSidesToSearch().any(); }

  // Clear the edges to search (3D), keeping the sides to search.
  void ResetEdgesToSearch() { this->SubPrimitivesToSearch = this->GetSidesToSearch(); }

  // The sub-primitives on which a point (local or remote) is inserted.
  void SetSplitSubPrimitive(int subIdx) { this->SplitSubPrimitives[subIdx] = true; }
  bool HasAnySplitSubPrimitives() const { return this->SplitSubPrimitives.any(); }
  bool IsSubPrimitiveSplit(int subIdx) const { return this->SplitSubPrimitives[subIdx]; }
  SubPrimitivesBitset GetSplitSubPrimitives() const { return this->SplitSubPrimitives; }

  // The point inserted on a sub-primitive, -1 if none.
  void SetMidPoint(int subIdx, vtkIdType ptId) { this->MidPoints[subIdx] = ptId; }
  vtkIdType GetMidPoint(int subIdx) const { return this->MidPoints[subIdx]; }
  bool HasMidPoint(int subIdx) const { return this->MidPoints[subIdx] != -1; }
  const std::array<vtkIdType, NumSubPrimitives>& GetMidPoints() const { return this->MidPoints; }

  // The point added at the center of the cell (Category B, see EmitOutputCells), -1 if none.
  void SetCentroidPoint(vtkIdType ptId) { this->CentroidPoint = ptId; }
  vtkIdType GetCentroidPoint() const { return this->CentroidPoint; }

  // The split case of the quad (2D, faceIdx is ignored) or of the face faceIdx (3D), see
  // QuadSplitCases.
  QuadSplitCases::QuadCase GetQuadSplitCase(int faceIdx) const;

  // The points of the local indices 0..7 of the quad (2D) or of the face faceIdx (3D) of the cell
  // whose points are `pts`: its corners, then the hanging points of its edges, see QuadSplitCases.
  std::array<vtkIdType, 8> GetQuadPoints(const vtkIdType* pts, int faceIdx = 0) const;

  // The number of split faces (sub-primitives 12..17 in 3D).
  int GetNumSplitFaces() const;

  // The number of split edges (sub-primitives 0..3 in 2D, 0..11 in 3D).
  int GetNumSplitEdges() const;

  // The number of split edges of the 3D face faceIdx (0..5).
  int GetNumSplitEdgesOfFace(int faceIdx) const;

  // The side of the NumSidePoints point ids `sidePts`.
  static SideTuple MakeSide(const vtkIdType* sidePts);

  // Write at `finerSides` the finer sides of the split sides of this cell, whose points are `pts`
  // (see RestrictCandidateCells): the halves of the split edges (2D) or the quarters of the
  // split faces (3D), i.e., NumSidePoints per split side.
  void FillFinerSides(const vtkIdType* pts, SideTuple* finerSides) const;

  // The number of split sides: edges in 2D, faces in 3D.
  int GetNumSplitSides() const
  {
    if constexpr (Dim == 2)
    {
      return this->GetNumSplitEdges();
    }
    else
    {
      return this->GetNumSplitFaces();
    }
  }

  // Whether some sub-primitives to search are not split yet.
  bool HasAnyUnsplitSubPrimitivesToSearch() const
  {
    return (this->SubPrimitivesToSearch & ~this->SplitSubPrimitives).any();
  }

  // The index (0..5) of the split face if exactly 1 face is split, with its 4 edges and no other
  // edge (Category A1), else -1.
  int GetSingleSplitFaceIndex() const;

  // The index (0..11) of the split edge if exactly 1 edge and no face is split (Category A2), else
  // -1.
  int GetSingleSplitEdgeIndex() const;

  // Whether the decomposition of the cell needs a point at its center (Category B).
  template <bool Decompose = true>
  bool HasCentroidPoint() const;

  // The number of cells that replace this cell, which must be split (see CommitResolution):
  // - without Decompose, 1 (the polygon or polyhedron);
  // - in 2D, read from QuadSplitCases;
  // - in 3D, 5 pyramids and 4 tetrahedra for a single split face (Category A1), 4 pyramids for a
  //   single split edge (Category A2), otherwise (Category B), for each face, 4 pyramids if its
  //   center is split, otherwise a tetrahedron or a pyramid per triangle or quad of QuadSplitCases.
  template <bool Decompose = true>
  vtkIdType GetOutputCellCount() const;

  // Write the number of points of each of the numCells (GetOutputCellCount) cells that replace
  // this cell in `sizesPtr`.
  template <bool Decompose>
  void FillOutputCellSizes(vtkIdType* sizesPtr, vtkIdType numCells) const;

  // Emit the numCells (GetOutputCellCount) cells that replace this cell: their connectivity at
  // connPtr + connOffsetsPtr[k], and their types at typesPtr[k]. Without Decompose in 3D, the faces
  // of the polyhedron are written at facesConnPtr + facesOffsetsPtr[f].
  template <bool Decompose>
  void EmitOutputCells(const vtkIdType* pts, vtkIdType numCells, vtkIdType* connPtr,
    const vtkIdType* connOffsetsPtr, unsigned char* typesPtr, vtkIdType* facesConnPtr,
    const vtkIdType* facesOffsetsPtr) const;

  // The number of faces of the polyhedron (3D only): 1 per unsplit face, 4 per split face.
  vtkIdType GetPolyhedronFaceCount() const;

  // Write the number of points of each face of the polyhedron in `faceSizesPtr`.
  void FillPolyhedronFaceSizes(vtkIdType* faceSizesPtr) const;

protected:
  // The number of points of the polygon (2D) or polyhedron (3D): the corners and 1 hanging point
  // per split edge or face.
  vtkIdType GetPolyPointCount() const;

  // Emit the sub-cell subIdx: write its cell type and its connectivity in the output arrays.
  static void EmitSingleSubCell(vtkIdType subIdx, unsigned char cellType, const vtkIdType* pts,
    vtkIdType numPts, vtkIdType* connPtr, const vtkIdType* connOffsetsPtr, unsigned char* typesPtr);

  template <std::size_t NPts>
  static void EmitSingleSubCell(vtkIdType subIdx, unsigned char cellType,
    const std::array<vtkIdType, NPts>& pts, vtkIdType* connPtr, const vtkIdType* connOffsetsPtr,
    unsigned char* typesPtr)
  {
    EmitSingleSubCell(subIdx, cellType, pts.data(), static_cast<vtkIdType>(NPts), connPtr,
      connOffsetsPtr, typesPtr);
  }

private:
  // Sub-primitives of a hexahedron: edges in bits 0..11, faces in bits 12..17.
  static constexpr std::bitset<18> EdgeMask3D{ 0b000000111111111111ULL };
  static constexpr std::bitset<18> FaceMask3D{ 0b111111000000000000ULL };

  vtkIdType OriginalCellId = -1;
  vtkIdType CentroidPoint = -1;
  std::array<vtkIdType, NumSubPrimitives> MidPoints;
  SubPrimitivesBitset SubPrimitivesToSearch{};
  SubPrimitivesBitset SplitSubPrimitives{};
};

} // namespace

//------------------------------------------------------------------------------
void vtkResolveHangingNodes::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Tolerance: " << this->Tolerance << endl;
  os << indent << "NumberOfLevels: " << this->NumberOfLevels << endl;
  os << indent << "Decompose: " << (this->Decompose ? "On" : "Off") << endl;
  os << indent << "ComputeInverseMeasure: " << (this->ComputeInverseMeasure ? "On" : "Off") << endl;
  os << indent << "PassThroughCellIds: " << (this->PassThroughCellIds ? "On" : "Off") << endl;
  os << indent << "OriginalCellIdsName: " << this->OriginalCellIdsName << endl;
}

//------------------------------------------------------------------------------
int vtkResolveHangingNodes::FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Remove(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE());
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPolyData");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkUnstructuredGrid");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkCartesianGrid");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPartitionedDataSet");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPartitionedDataSetCollection");
  return 1;
}

//------------------------------------------------------------------------------
int vtkResolveHangingNodes::RequestDataObject(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkDataObject* input = vtkDataObject::GetData(inputVector[0]);
  if (!input)
  {
    return 0;
  }
  // The output has the type of the input, but the cells of a vtkCartesianGrid are resolved in a
  // vtkUnstructuredGrid, and the vtkCartesianGrid blocks of a vtkNonOverlappingAMR in a
  // vtkPartitionedDataSetCollection of them.
  int outputType;
  if (vtkOverlappingAMR::SafeDownCast(input))
  {
    vtkErrorMacro("vtkOverlappingAMR is not supported, since its levels overlap.");
    return 0;
  }
  else if (vtkPolyData::SafeDownCast(input))
  {
    outputType = VTK_POLY_DATA;
  }
  else if (vtkUnstructuredGrid::SafeDownCast(input) || vtkCartesianGrid::SafeDownCast(input))
  {
    outputType = VTK_UNSTRUCTURED_GRID;
  }
  else if (vtkPartitionedDataSet::SafeDownCast(input))
  {
    outputType = VTK_PARTITIONED_DATA_SET;
  }
  else if (vtkPartitionedDataSetCollection::SafeDownCast(input))
  {
    outputType = VTK_PARTITIONED_DATA_SET_COLLECTION;
  }
  else
  {
    vtkErrorMacro("Unsupported input type " << input->GetClassName() << ".");
    return 0;
  }
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkDataObject* output = vtkDataObject::GetData(outInfo);
  if (!output || output->GetDataObjectType() != outputType)
  {
    outInfo->Set(vtkDataObject::DATA_OBJECT(),
      vtk::TakeSmartPointer(vtkDataObjectTypes::NewDataObject(outputType)));
  }
  return 1;
}

namespace
{
//------------------------------------------------------------------------------
// A locator of edges, given by their point ids.
using EdgeLocator = vtkStaticElementLocator<vtkIdType, EdgeTuple<vtkIdType>>;

//------------------------------------------------------------------------------
// 3D only: the unique edges to search of the candidate cells, a locator of the edges of their faces
// to search, whose elements are stored in `Edges`. Moving it keeps the locator valid, since the
// buffer of `Edges` moves with it. An edge is shared by several candidate cells, so
// FindPointsOnSubPrimitives searches each of them once, instead of once per cell.
struct EdgesToSearch
{
  std::vector<EdgeTuple<vtkIdType>> Edges;
  EdgeLocator Locator;
};

//------------------------------------------------------------------------------
// Set the sides to search of the cells of `input` in `subPrimitivesToSearch`, the bits of the
// sub-primitives to search of every cell: the sides that no other cell has (edges in 2D, faces in
// 3D), found by matching the sides of all the cells with vtkUnstructuredGridSides. It takes a
// vtkUnstructuredGrid: a vtkPolyData of quads is given as one, sharing its points and polygons.
template <int Dim, typename TId, class DataSetT>
void FindSidesToSearch(DataSetT* input, std::atomic<std::uint32_t>* subPrimitivesToSearch)
{
  vtkSmartPointer<vtkUnstructuredGrid> grid;
  if constexpr (std::is_same_v<DataSetT, vtkPolyData>)
  {
    grid = vtkSmartPointer<vtkUnstructuredGrid>::New();
    grid->SetPoints(input->GetPoints());
    grid->SetCells(VTK_QUAD, input->GetPolys());
  }
  else
  {
    grid = input;
  }

  using Sides = vtkUnstructuredGridSides<TId>;
  vtkNew<Sides> sides;
  sides->SetDataSet(grid);
  sides->SetOutputDimensionControl(
    Dim == 2 ? Sides::SideFlags::EdgesOfSurfaces : Sides::SideFlags::SurfacesOfVolumes);
  sides->SetStrategy(Sides::SummaryStrategy::Boundary);
  sides->DeterministicSideOrderOff();
  sides->BuildReducedSides();
  vtkSMPTools::For(0, static_cast<vtkIdType>(sides->GetNumberOfKeys()),
    [&](vtkIdType begin, vtkIdType end)
    {
      for (vtkIdType id = begin; id < end; ++id)
      {
        const auto keyId = static_cast<TId>(id);
        const TId* cellIds = sides->GetCellIdsOfKey(keyId);
        const auto* sideIds = sides->GetSideIdsOfKey(keyId);
        const vtkIdType numSides = sides->GetNumberOfSidesOfKey(keyId);
        for (vtkIdType k = 0; k < numSides; ++k)
        {
          // The unmatched sides of a cell are in different keys, so they can be set at the same
          // time. memory_order_relaxed is safe, the bits are only combined.
          subPrimitivesToSearch[cellIds[k]].fetch_or(
            1u << static_cast<unsigned>(CandidateCellInfoT<Dim>::FirstSide + sideIds[k]),
            std::memory_order_relaxed);
        }
      }
    });
}

//------------------------------------------------------------------------------
// The `npts` points `pts` of the cell `cellId` of `input`, a quad or a hexahedron, as
// vtkDataSet::GetCellPoints(). The cells of a vtkCartesianGrid are pixels or voxels, whose points
// are written in `ptIds` in the order of the points of a quad or a hexahedron: all the cells of the
// filter are then quads or hexahedra, without converting a vtkCartesianGrid. The cells of a
// vtkPolyData are only polygons, read from its polygons: their ids are the cell ids, without the
// cell map of vtkPolyData::BuildCells(), which would not be built in parallel safely.
template <class DataSetT>
void GetCellPoints(
  DataSetT* input, vtkIdType cellId, vtkIdType& npts, vtkIdType const*& pts, vtkIdList* ptIds)
{
  if constexpr (std::is_base_of_v<vtkCartesianGrid, DataSetT>)
  {
    input->GetCellPoints(cellId, ptIds);
    npts = ptIds->GetNumberOfIds();
    vtkIdType* ids = ptIds->GetPointer(0);
    // The points 2 and 3 of a pixel, and also 6 and 7 of a voxel, are swapped.
    std::swap(ids[2], ids[3]);
    if (npts == 8)
    {
      std::swap(ids[6], ids[7]);
    }
    pts = ids;
  }
  else if constexpr (std::is_same_v<DataSetT, vtkPolyData>)
  {
    input->GetPolys()->GetCellAtId(cellId, npts, pts, ptIds);
  }
  else
  {
    input->GetCellPoints(cellId, npts, pts, ptIds);
  }
}

//------------------------------------------------------------------------------
// 3D only: build `edgesToSearch` with the edges of the faces to search of `numCells` hexahedra, the
// cell `getCellId(i)` and its faces f for which `isFaceToSearch(i, f)` is true, for i in
// [0, numCells). The faces of a cell share edges: the edges of all of them are gathered first, so
// that each edge of the cell is stored once.
template <class DataSetT, class CellIdFunctor, class IsFaceToSearchFunctor>
void BuildEdgeLocatorOfFacesToSearch(DataSetT* input, vtkIdType numCells, CellIdFunctor getCellId,
  IsFaceToSearchFunctor isFaceToSearch, EdgesToSearch& edgesToSearch)
{
  std::vector<EdgeTuple<vtkIdType>>& edges = edgesToSearch.Edges;
  // The edges of the faces to search of the cell getCellId(i), each once.
  auto getEdgesOfFacesToSearch = [&](vtkIdType i)
  {
    std::bitset<vtkHexahedron::NumberOfEdges> cellEdges;
    for (int f = 0; f < vtkHexahedron::NumberOfFaces; ++f)
    {
      if (isFaceToSearch(i, f))
      {
        for (const auto e : FaceEdges[f])
        {
          cellEdges.set(e);
        }
      }
    }
    return cellEdges;
  };

  // ===========================================================================
  // 1. Count the edges of the faces to search of every cell
  // ===========================================================================
  std::vector<vtkIdType> offsets(numCells + 1, 0);
  vtkSMPTools::For(0, numCells,
    [&](vtkIdType begin, vtkIdType end)
    {
      for (vtkIdType i = begin; i < end; ++i)
      {
        offsets[i] = static_cast<vtkIdType>(getEdgesOfFacesToSearch(i).count());
      }
    });
  // The last entry is 0, so that the scan sets it to the number of edges.
  const vtkIdType numEdges =
    vtkSMPTools::ExclusiveScan(offsets.begin(), offsets.end(), static_cast<vtkIdType>(0));

  // ===========================================================================
  // 2. Build a locator of these edges
  // ===========================================================================
  edges.resize(numEdges);
  vtkSMPThreadLocalObject<vtkIdList> tlCell;
  vtkSMPTools::For(0, numCells,
    [&](vtkIdType begin, vtkIdType end)
    {
      vtkIdList* tmpCell = tlCell.Local();
      vtkIdType npts;
      const vtkIdType* cell = nullptr;
      for (vtkIdType i = begin; i < end; ++i)
      {
        if (offsets[i + 1] == offsets[i])
        {
          continue;
        }
        const auto cellEdges = getEdgesOfFacesToSearch(i);
        GetCellPoints(input, getCellId(i), npts, cell, tmpCell);
        vtkIdType next = offsets[i];
        for (int e = 0; e < vtkHexahedron::NumberOfEdges; ++e)
        {
          if (cellEdges[e])
          {
            const vtkIdType* edge = vtkHexahedron::GetEdgeArray(e);
            edges[next++] = EdgeTuple<vtkIdType>(cell[edge[0]], cell[edge[1]]);
          }
        }
      }
    });
  std::vector<vtkIdType>().swap(offsets); // No longer needed
  edgesToSearch.Locator.BuildLocator(numEdges, edges.data());
}

//------------------------------------------------------------------------------
// 3D only: call `setEdgeToSearch(i, e)` for the edges e of the cell `getCellId(i)`, for i in
// [0, numCells), that are in `locator`, the edges of the faces to search. Such an edge can have a
// hanging point even if it has neighbors, see CandidateCellInfoT::SubPrimitivesToSearch.
template <class DataSetT, class CellIdFunctor, class EdgeFunctor>
void FindEdgesToSearch(DataSetT* input, vtkIdType numCells, CellIdFunctor getCellId,
  const EdgeLocator& locator, EdgeFunctor setEdgeToSearch)
{
  if (locator.GetNumberOfElements() == 0)
  {
    return;
  }
  vtkSMPThreadLocalObject<vtkIdList> tlCell;
  vtkSMPTools::For(0, numCells,
    [&](vtkIdType begin, vtkIdType end)
    {
      vtkIdList* tmpCell = tlCell.Local();
      vtkIdType npts;
      const vtkIdType* cell = nullptr;
      for (vtkIdType i = begin; i < end; ++i)
      {
        GetCellPoints(input, getCellId(i), npts, cell, tmpCell);
        assert(npts >= 3);
        for (int e = 0; e < vtkHexahedron::NumberOfEdges; ++e)
        {
          const vtkIdType* edge = vtkHexahedron::GetEdgeArray(e);
          if (locator.IsInsertedElement(cell[edge[0]], cell[edge[1]]) >= 0)
          {
            setEdgeToSearch(i, e);
          }
        }
      }
    });
}

//------------------------------------------------------------------------------
// The candidate cells of `input`, a vtkUnstructuredGrid or a vtkPolyData, with their original cell
// ids and their sub-primitives to search, i.e., their unmatched sides (see FindSidesToSearch), and
// in 3D, their edges that are edges of unmatched faces (see FindEdgesToSearch). The candidate cells
// are:
// - the cells with a side (an edge in 2D, a face in 3D) without a neighbor;
// - in 3D, also the cells with an edge of an unmatched face, which can have a hanging point even if
//   all the faces of the cell are matched (see step 2).
// Returns the candidate cells, and in 3D, their edges to search, see EdgesToSearch (empty in 2D).
template <int Dim, class DataSetT>
std::pair<std::vector<CandidateCellInfoT<Dim>>, EdgesToSearch> FindUnstructuredCandidateCells(
  DataSetT* input)
{
  const vtkIdType numCells = input->GetNumberOfCells();
  EdgesToSearch edgesToSearch;
  // The sub-primitives to search of every cell, as the bits of a word, so that FindSidesToSearch
  // sets them in parallel: the cells with some are the candidates.
  static_assert(CandidateCellInfoT<Dim>::NumSubPrimitives <= 32,
    "the sub-primitives to search of a cell must fit in 32 bits");
  std::unique_ptr<std::atomic<std::uint32_t>[]> subPrimitivesToSearch(
    new std::atomic<std::uint32_t>[numCells]());

  // ===========================================================================
  // 1. Find the sides to search, the sides without a neighbor
  // ===========================================================================
  // In 3D, an edge is shared by up to 4 cells, so having a neighbor across an edge does not tell
  // whether the edge has a hanging point: only the faces are matched here, the edges in step 2. The
  // sides are matched with 32-bit point and cell ids when they fit, to halve their memory, as in
  // vtkGeometryFilter, and vtkUnstructuredGridSides chooses the ids of the sides from their number.
  if (input->GetNumberOfPoints() <= std::numeric_limits<vtkTypeInt32>::max() &&
    numCells <= std::numeric_limits<vtkTypeInt32>::max())
  {
    FindSidesToSearch<Dim, vtkTypeInt32>(input, subPrimitivesToSearch.get());
  }
  else
  {
    FindSidesToSearch<Dim, vtkIdType>(input, subPrimitivesToSearch.get());
  }

  // ===========================================================================
  // 2. 3D: find the edges to search of all the cells, the edges of the unmatched faces
  // ===========================================================================
  // A cell can touch finer cells only along one of its edges, with all its faces shared with other
  // cells, so it has no unmatched face. For example, a coarse cell next to 2 coarse cells, across 2
  // of its faces, whose common edge borders a block of finer cells: the hanging point at the middle
  // of that edge is not found through its faces. But going around the edge, from the cell to the
  // finer cells, a coarse cell has a face toward finer cells (or no neighbor) that contains the
  // edge: the edge is an edge of an unmatched face, so the cell is a candidate too.
  if constexpr (Dim == 3)
  {
    // A locator of the edges of the unmatched faces, the faces to search found in step 1.
    BuildEdgeLocatorOfFacesToSearch(
      input, numCells, [](vtkIdType i) -> vtkIdType { return i; },
      [&subPrimitivesToSearch](vtkIdType i, int f)
      {
        const std::uint32_t bits = subPrimitivesToSearch[i].load(std::memory_order_relaxed);
        return ((bits >> static_cast<std::uint32_t>(vtkHexahedron::NumberOfEdges + f)) & 1u) != 0;
      },
      edgesToSearch);
    // Every edge of a cell that is in the locator is to search, even if the cell has no unmatched
    // face: the cell becomes a candidate.
    FindEdgesToSearch(
      input, numCells, [](vtkIdType i) -> vtkIdType { return i; }, edgesToSearch.Locator,
      [&subPrimitivesToSearch](vtkIdType i, int e)
      {
        subPrimitivesToSearch[i].fetch_or(
          1u << static_cast<std::uint32_t>(e), std::memory_order_relaxed);
      });
  }

  // ===========================================================================
  // 3. Count the candidate cells, the cells with sub-primitives to search
  // ===========================================================================
  vtkNew<vtkIdTypeArray> offsets;
  offsets->SetNumberOfValues(numCells + 1);
  offsets->SetValue(numCells, 0);
  vtkSMPTools::For(0, numCells,
    [&](vtkIdType begin, vtkIdType end)
    {
      for (vtkIdType i = begin; i < end; ++i)
      {
        offsets->SetValue(i, subPrimitivesToSearch[i].load(std::memory_order_relaxed) != 0 ? 1 : 0);
      }
    });

  const vtkIdType numCellsFound = vtkSMPTools::ExclusiveScan(
    offsets->GetPointer(0), offsets->GetPointer(numCells), static_cast<vtkIdType>(0));
  offsets->SetValue(numCells, numCellsFound);

  // ===========================================================================
  // 4. Copy the candidate cells and their sub-primitives to search into `candidatesInfo`
  // ===========================================================================
  using SubPrimitivesBitset = typename CandidateCellInfoT<Dim>::SubPrimitivesBitset;
  std::vector<CandidateCellInfoT<Dim>> candidatesInfo(numCellsFound);
  vtkSMPTools::For(0, numCells,
    [&](vtkIdType begin, vtkIdType end)
    {
      for (vtkIdType i = begin; i < end; ++i)
      {
        if (offsets->GetValue(i + 1) != offsets->GetValue(i))
        {
          const vtkIdType index = offsets->GetValue(i);
          candidatesInfo[index].SetOriginalCellId(i);
          candidatesInfo[index].SetSubPrimitivesToSearch(
            SubPrimitivesBitset(subPrimitivesToSearch[i].load(std::memory_order_relaxed)));
        }
      }
    });
  return { std::move(candidatesInfo), std::move(edgesToSearch) };
}

//------------------------------------------------------------------------------
// The candidate cells of the vtkCartesianGrid `grid`, whose cells are read as quads or hexahedra
// (see GetCellPoints): the cells with a side on the boundary of its extent, which are their sides
// to search. A vtkCartesianGrid is conforming, so it has no other unmatched side, and in 3D, the
// cells with an edge of these sides are the same cells: their edges to search are set from the
// true boundary, see RestrictCandidateCells.
template <int Dim>
std::vector<CandidateCellInfoT<Dim>> FindCartesianCandidateCells(vtkCartesianGrid* grid)
{
  // The numbers of cells along the axes with more than one point, i.e., the axes of the cells
  int dims[3];
  grid->GetDimensions(dims);
  int cellDims[3] = { 1, 1, 1 };
  int numAxes = 0;
  for (int axis = 0; axis < 3; ++axis)
  {
    if (dims[axis] > 1)
    {
      cellDims[numAxes++] = dims[axis] - 1;
    }
  }
  assert(numAxes == Dim);

  // Whether the row of cells along i at (j, k) is on the j or k boundaries of the extent: all its
  // cells are then on the boundary, otherwise only the first and the last ones.
  auto isBoundaryRow = [&cellDims](int j, int k)
  { return j == 0 || j == cellDims[1] - 1 || (Dim == 3 && (k == 0 || k == cellDims[2] - 1)); };

  // Set `info` to the cell (i, j, k), with its sides on the boundary of the extent as sides to
  // search. The points of the cells are those of a quad or a hexahedron, see GetCellPoints: the
  // edges 0 to 3 of a quad are on the j min, i max, j max and i min boundaries, and the faces 0 to
  // 5 of a hexahedron on the i min, i max, j min, j max, k min and k max ones.
  auto setCandidateCell = [&cellDims](CandidateCellInfoT<Dim>& info, int i, int j, int k)
  {
    bool isOnBoundary[2 * Dim];
    if constexpr (Dim == 2)
    {
      isOnBoundary[0] = j == 0;
      isOnBoundary[1] = i == cellDims[0] - 1;
      isOnBoundary[2] = j == cellDims[1] - 1;
      isOnBoundary[3] = i == 0;
    }
    else
    {
      isOnBoundary[0] = i == 0;
      isOnBoundary[1] = i == cellDims[0] - 1;
      isOnBoundary[2] = j == 0;
      isOnBoundary[3] = j == cellDims[1] - 1;
      isOnBoundary[4] = k == 0;
      isOnBoundary[5] = k == cellDims[2] - 1;
    }
    typename CandidateCellInfoT<Dim>::SubPrimitivesBitset sides;
    for (int side = 0; side < 2 * Dim; ++side)
    {
      sides[CandidateCellInfoT<Dim>::FirstSide + side] = isOnBoundary[side];
    }
    info.SetOriginalCellId(i + (j + static_cast<vtkIdType>(k) * cellDims[1]) * cellDims[0]);
    info.SetSubPrimitivesToSearch(sides);
  };

  // The first candidate cell of every row of cells along i, the row (j, k) being j + k * nj: all
  // the cells of the rows on the j or k boundaries, and the first and last cells of the others
  const vtkIdType numRows = static_cast<vtkIdType>(cellDims[1]) * cellDims[2];
  const vtkIdType numInnerRowCandidates = std::min(cellDims[0], 2);
  std::vector<vtkIdType> rowOffsets(numRows + 1);
  rowOffsets[0] = 0;
  for (vtkIdType row = 0; row < numRows; ++row)
  {
    const int j = static_cast<int>(row % cellDims[1]);
    const int k = static_cast<int>(row / cellDims[1]);
    rowOffsets[row + 1] =
      rowOffsets[row] + (isBoundaryRow(j, k) ? cellDims[0] : numInnerRowCandidates);
  }

  std::vector<CandidateCellInfoT<Dim>> candidatesInfo(rowOffsets[numRows]);
  vtkSMPTools::For(0, numRows,
    [&](vtkIdType begin, vtkIdType end)
    {
      for (vtkIdType row = begin; row < end; ++row)
      {
        const int j = static_cast<int>(row % cellDims[1]);
        const int k = static_cast<int>(row / cellDims[1]);
        CandidateCellInfoT<Dim>* info = candidatesInfo.data() + rowOffsets[row];
        if (isBoundaryRow(j, k))
        {
          for (int i = 0; i < cellDims[0]; ++i)
          {
            setCandidateCell(info[i], i, j, k);
          }
        }
        else
        {
          setCandidateCell(info[0], 0, j, k);
          if (cellDims[0] > 1)
          {
            setCandidateCell(info[1], cellDims[0] - 1, j, k);
          }
        }
      }
    });
  return candidatesInfo;
}

//------------------------------------------------------------------------------
// The candidate cells of `input`, a vtkUnstructuredGrid or a vtkPolyData (see
// FindUnstructuredCandidateCells), or a vtkCartesianGrid (see FindCartesianCandidateCells), and in
// 3D, their edges to search, see EdgesToSearch (empty in 2D, and for a vtkCartesianGrid).
template <int Dim, class DataSetT>
std::pair<std::vector<CandidateCellInfoT<Dim>>, EdgesToSearch> FindCandidateCells(DataSetT* input)
{
  if constexpr (!std::is_base_of_v<vtkCartesianGrid, DataSetT>)
  {
    return FindUnstructuredCandidateCells<Dim>(input);
  }
  else
  {
    return { FindCartesianCandidateCells<Dim>(input), EdgesToSearch() };
  }
}

//------------------------------------------------------------------------------
// The area (2D) or volume (3D) of a cell.
template <int Dim>
double ComputeMeasure(vtkPoints* points, vtkIdType npts, const vtkIdType* cellPts)
{
  if constexpr (Dim == 2)
  {
    double normal[3];
    return vtkPolygon::ComputeArea(points, npts, cellPts, normal);
  }
  else // Dim == 3
  {
    std::array<vtkVector3d, 8> p;
    const int numPtsToFetch = std::min(static_cast<int>(npts), 8);
    for (int i = 0; i < numPtsToFetch; ++i)
    {
      points->GetPoint(cellPts[i], p[i].GetData());
    }

    // Unsigned volume of the tetrahedron of the points a, b, c and d of the cell
    auto computeTetVolume = [&p](int a, int b, int c, int d) -> double
    {
      return std::abs(
        vtkTetra::ComputeVolume(p[a].GetData(), p[b].GetData(), p[c].GetData(), p[d].GetData()));
    };

    // Hexahedron, or polyhedron whose 8 first points are the corners of the hexahedron: 5
    // tetrahedra
    if (npts >= 8)
    {
      return computeTetVolume(0, 1, 2, 5) + computeTetVolume(0, 2, 3, 7) +
        computeTetVolume(0, 4, 5, 7) + computeTetVolume(2, 5, 6, 7) + computeTetVolume(0, 2, 5, 7);
    }

    switch (npts)
    {
      case 5: // Pyramid: 2-tet decomposition
        return computeTetVolume(0, 1, 2, 4) + computeTetVolume(0, 2, 3, 4);

      case 4: // Tetrahedron
        return computeTetVolume(0, 1, 2, 3);

      default:
        return 0.0;
    }
  }
}

//------------------------------------------------------------------------------
// The (inverse) measures, areas (2D) or volumes (3D), of the cells `cellsIds`.
template <int Dim, bool Inverse = false, class DataSetT, class CellIdsT>
vtkSmartPointer<vtkDoubleArray> ComputeMeasures(DataSetT* grid, CellIdsT cellsIds)
{
  vtkPoints* points = grid->GetPoints();
  vtkNew<vtkDoubleArray> measures;
  measures->SetNumberOfValues(cellsIds->GetNumberOfValues());
  measures->SetName(
    Inverse ? CandidateCellInfoT<Dim>::InverseMeasureName : CandidateCellInfoT<Dim>::MeasureName);

  vtkSMPThreadLocalObject<vtkIdList> tlCell;
  vtkSMPTools::For(0, cellsIds->GetNumberOfValues(),
    [&](vtkIdType begin, vtkIdType end)
    {
      vtkIdList* tmpCell = tlCell.Local();
      vtkIdType npts;
      const vtkIdType* cell = nullptr;
      for (vtkIdType i = begin; i < end; ++i)
      {
        GetCellPoints(grid, cellsIds->GetValue(i), npts, cell, tmpCell);
        assert(npts >= 3);
        double val = ComputeMeasure<Dim>(points, npts, cell);
        if constexpr (Inverse)
        {
          val = (val > 0.0) ? (1.0 / val) : 0.0;
        }
        measures->SetValue(i, val);
      }
    });
  return measures;
}

//------------------------------------------------------------------------------
// Number of levels from the smallest and largest measures of the cells of dimension `dimension`:
// the measures of consecutive levels differ by a factor of 2^dimension.
int ComputeNumberOfLevels(int dimension, const double measureRange[2])
{
  if (dimension > 0 && measureRange[0] > 0.0 && measureRange[1] >= measureRange[0])
  {
    return static_cast<int>(std::lround(std::log2(measureRange[1] / measureRange[0]) / dimension)) +
      1;
  }
  return 0;
}

//------------------------------------------------------------------------------
// Whether every area/volume of `candidateMeasures` is `minMeasure` times a power of 4 (2D) or
// 8 (3D), within a small tolerance.
template <int Dim>
bool CellMeasuresMatchLevels(const vtkDoubleArray* candidateMeasures, double minMeasure)
{
  vtkSMPThreadLocal<int> tlConsistent(1);
  vtkSMPTools::For(0, candidateMeasures->GetNumberOfValues(),
    [&](vtkIdType begin, vtkIdType end)
    {
      int& consistent = tlConsistent.Local();
      for (vtkIdType i = begin; i < end && consistent; ++i)
      {
        const double level = (1.0 / Dim) * std::log2(candidateMeasures->GetValue(i) / minMeasure);
        if (std::abs(level - std::round(level)) > 1e-3)
        {
          consistent = 0;
          return;
        }
      }
    });
  for (int consistent : tlConsistent)
  {
    if (!consistent)
    {
      return false;
    }
  }
  return true;
}

//------------------------------------------------------------------------------
// Write at `ptIds` the point ids of the sub-primitive subIdx of the cell whose points are
// `cellPts`, and return their number: 2 for an edge, 4 for a face (3D), at most
// CandidateCellInfoT<Dim>::NumSidePoints.
template <int Dim>
int GetSubPrimitivePointIds(const vtkIdType* cellPts, int subIdx, vtkIdType* ptIds)
{
  if constexpr (Dim == 2)
  {
    const vtkIdType* edge = vtkQuad::GetEdgeArray(subIdx);
    ptIds[0] = cellPts[edge[0]];
    ptIds[1] = cellPts[edge[1]];
    return 2;
  }
  else
  {
    if (subIdx < vtkHexahedron::NumberOfEdges) // Edges (0..11)
    {
      const vtkIdType* edge = vtkHexahedron::GetEdgeArray(subIdx);
      ptIds[0] = cellPts[edge[0]];
      ptIds[1] = cellPts[edge[1]];
      return 2;
    }
    else // Faces (12..17)
    {
      const int faceIdx = subIdx - vtkHexahedron::NumberOfEdges;
      const vtkIdType* face = vtkHexahedron::GetFaceArray(faceIdx);
      ptIds[0] = cellPts[face[0]];
      ptIds[1] = cellPts[face[1]];
      ptIds[2] = cellPts[face[2]];
      ptIds[3] = cellPts[face[3]];
      return 4;
    }
  }
}

//------------------------------------------------------------------------------
// Whether a flag of SelectIds is set. An atomic flag is read with a relaxed load, since the
// flags are set before, and are not used for synchronization.
bool IsFlagSet(unsigned char flag)
{
  return flag != 0;
}

bool IsFlagSet(const std::atomic<unsigned char>& flag)
{
  return flag.load(std::memory_order_relaxed) != 0;
}

//------------------------------------------------------------------------------
// Fill `ids` with the selected indices, in increasing order, and return their number. `flags` has
// num entries, nonzero for a selected index, and is not modified. They can be atomics, for flags
// that concurrent threads may set, e.g., the marks of the points shared by several cells. The flags
// are counted per block, then every block writes its indices, so that no array of num offsets is
// needed.
template <typename TFlag>
vtkIdType SelectIds(const TFlag* flags, vtkIdType num, vtkIdList* ids)
{
  constexpr vtkIdType blockSize = 8192;
  const vtkIdType numBlocks = (num + blockSize - 1) / blockSize;
  // The last entry is 0, so that the scan sets it to the number of selected indices.
  std::vector<vtkIdType> blockOffsets(numBlocks + 1, 0);
  vtkSMPTools::For(0, numBlocks,
    [&](vtkIdType begin, vtkIdType end)
    {
      for (vtkIdType b = begin; b < end; ++b)
      {
        const vtkIdType last = std::min((b + 1) * blockSize, num);
        vtkIdType count = 0;
        for (vtkIdType i = b * blockSize; i < last; ++i)
        {
          count += IsFlagSet(flags[i]) ? 1 : 0;
        }
        blockOffsets[b] = count;
      }
    });
  const vtkIdType numSelected =
    vtkSMPTools::ExclusiveScan(blockOffsets.begin(), blockOffsets.end(), static_cast<vtkIdType>(0));

  ids->SetNumberOfIds(numSelected);
  vtkIdType* idsPointer = ids->GetPointer(0);
  vtkSMPTools::For(0, numBlocks,
    [&](vtkIdType begin, vtkIdType end)
    {
      for (vtkIdType b = begin; b < end; ++b)
      {
        const vtkIdType last = std::min((b + 1) * blockSize, num);
        vtkIdType next = blockOffsets[b];
        for (vtkIdType i = b * blockSize; i < last; ++i)
        {
          if (IsFlagSet(flags[i]))
          {
            idsPointer[next++] = i;
          }
        }
      }
    });
  return numSelected;
}

//------------------------------------------------------------------------------
// The points of `source` listed in `ids`, with their point data, global ids included, as a
// vtkPolyData without cells.
vtkSmartPointer<vtkPolyData> ExtractPoints(vtkDataSet* source, vtkIdList* ids)
{
  vtkNew<vtkPoints> points;
  points->SetDataType(source->GetPoints()->GetDataType());
  source->GetPoints()->GetPoints(ids, points);

  vtkNew<vtkPolyData> result;
  vtkPointData* pointData = result->GetPointData();
  pointData->CopyGlobalIdsOn();
  pointData->CopyAllocate(source->GetPointData(), ids->GetNumberOfIds());
  pointData->CopyData(source->GetPointData(), ids, static_cast<vtkIdType>(0));
  result->SetPoints(points);
  return result;
}

//------------------------------------------------------------------------------
// The boundary of the candidate cells: the points of their sides to search, with their point data,
// as a vtkPolyData, with these sides as lines (2D) or quads (3D) if `onlyPoints` is false. If
// `originalPointIds` is not nullptr, it is filled with the input id of every extracted point.
// Returns nullptr if there is none.
template <int Dim, class DataSetT>
vtkSmartPointer<vtkPolyData> ExtractBoundary(DataSetT* input,
  const std::vector<CandidateCellInfoT<Dim>>& candidatesInfo, vtkIdTypeArray* originalPointIds,
  bool onlyPoints)
{
  if (candidatesInfo.empty())
  {
    return nullptr;
  }
  vtkPoints* points = input->GetPoints();
  const vtkIdType numPoints = points->GetNumberOfPoints();

  // Mark the points of the sides to search. The marks are atomics, since several cells can mark
  // the same point.
  std::unique_ptr<std::atomic<unsigned char>[]> onBoundary(
    new std::atomic<unsigned char>[numPoints]());
  vtkSMPThreadLocalObject<vtkIdList> tlCell;
  vtkSMPTools::For(0, static_cast<vtkIdType>(candidatesInfo.size()),
    [&](vtkIdType begin, vtkIdType end)
    {
      vtkIdList* tmpCell = tlCell.Local();
      vtkIdType npts;
      const vtkIdType* cell = nullptr;
      for (vtkIdType i = begin; i < end; ++i)
      {
        const auto& info = candidatesInfo[i];
        if (!info.HasAnySidesToSearch())
        {
          continue;
        }
        GetCellPoints(input, info.GetOriginalCellId(), npts, cell, tmpCell);
        assert(npts == CandidateCellInfoT<Dim>::NumPoints);
        for (int j = CandidateCellInfoT<Dim>::FirstSide;
             j < CandidateCellInfoT<Dim>::NumSubPrimitives; ++j)
        {
          if (info.IsSubPrimitiveToSearch(j))
          {
            vtkIdType boundaryPts[CandidateCellInfoT<Dim>::NumSidePoints];
            const int numBoundaryPts = GetSubPrimitivePointIds<Dim>(cell, j, boundaryPts);
            for (int k = 0; k < numBoundaryPts; ++k)
            {
              // memory_order_relaxed is safe, the atomics are not used for synchronization.
              onBoundary[boundaryPts[k]].store(1, std::memory_order_relaxed);
            }
          }
        }
      }
    });

  // The marked points, in increasing order.
  vtkNew<vtkIdList> ids;
  if (SelectIds(onBoundary.get(), numPoints, ids) == 0)
  {
    return nullptr;
  }
  onBoundary.reset(); // No longer needed

  auto result = ExtractPoints(input, ids);

  if (!onlyPoints)
  {
    // The boundary elements become cells of the result, defined over the extracted points: the
    // new id of an extracted point is its index in `ids`, set in `newIds` for these points only,
    // since the others are not read. They are the sides to search (edges in 2D, faces in 3D), so
    // all the cells have NumSidePoints points.
    std::unique_ptr<vtkIdType[]> newIds(new vtkIdType[numPoints]);
    vtkSMPTools::For(0, ids->GetNumberOfIds(),
      [&](vtkIdType begin, vtkIdType end)
      {
        for (vtkIdType k = begin; k < end; ++k)
        {
          newIds[ids->GetId(k)] = k;
        }
      });
    // The ids are only kept for originalPointIds
    if (!originalPointIds)
    {
      ids->Initialize(); // No longer needed
    }
    constexpr vtkIdType elementSize = CandidateCellInfoT<Dim>::NumSidePoints;
    const vtkIdType numCandidates = static_cast<vtkIdType>(candidatesInfo.size());
    std::vector<vtkIdType> elementOffsets(numCandidates + 1, 0);
    vtkSMPTools::For(0, numCandidates,
      [&](vtkIdType begin, vtkIdType end)
      {
        for (vtkIdType i = begin; i < end; ++i)
        {
          elementOffsets[i] = static_cast<vtkIdType>(candidatesInfo[i].GetSidesToSearch().count());
        }
      });
    // The last entry is 0, so that the scan sets it to the number of elements.
    const vtkIdType numElements = vtkSMPTools::ExclusiveScan(
      elementOffsets.begin(), elementOffsets.end(), static_cast<vtkIdType>(0));

    vtkNew<vtkIdTypeArray> connectivity;
    connectivity->SetNumberOfValues(numElements * elementSize);
    vtkSMPTools::For(0, numCandidates,
      [&](vtkIdType begin, vtkIdType end)
      {
        vtkIdList* tmpCell = tlCell.Local();
        vtkIdType npts;
        const vtkIdType* cell = nullptr;
        for (vtkIdType i = begin; i < end; ++i)
        {
          const auto& info = candidatesInfo[i];
          if (!info.HasAnySidesToSearch())
          {
            continue;
          }
          GetCellPoints(input, info.GetOriginalCellId(), npts, cell, tmpCell);
          assert(npts == CandidateCellInfoT<Dim>::NumPoints);
          vtkIdType next = elementOffsets[i] * elementSize;
          for (int j = CandidateCellInfoT<Dim>::FirstSide;
               j < CandidateCellInfoT<Dim>::NumSubPrimitives; ++j)
          {
            if (info.IsSubPrimitiveToSearch(j))
            {
              vtkIdType boundaryPts[CandidateCellInfoT<Dim>::NumSidePoints];
              [[maybe_unused]] const int numBoundaryPts =
                GetSubPrimitivePointIds<Dim>(cell, j, boundaryPts);
              assert(numBoundaryPts == elementSize);
              for (vtkIdType k = 0; k < elementSize; ++k)
              {
                connectivity->SetValue(next++, newIds[boundaryPts[k]]);
              }
            }
          }
        }
      });
    vtkNew<vtkCellArray> elements;
    elements->SetData(elementSize, connectivity);
    if constexpr (Dim == 2)
    {
      result->SetLines(elements);
    }
    else
    {
      result->SetPolys(elements);
    }
  }

  // The input ids of the extracted points, if requested
  if (originalPointIds)
  {
    const vtkIdType numIds = ids->GetNumberOfIds();
    originalPointIds->SetArray(
      ids->Release(), numIds, /*save*/ 0, vtkAbstractArray::VTK_DATA_ARRAY_DELETE);
  }

  return result;
}

//------------------------------------------------------------------------------
// Fill `pointIdsOnEdges` with the point ids of `searchPoints` that lie on the edge from point `a`
// to point `b`, strictly between them. `locator` is a locator of `searchPoints`. A point is on the
// edge if it is within `tolerance` times the length of the edge from it. Points coincident with
// the end points are excluded.
void FindPointsOnEdge(vtkAbstractPointLocator* locator, vtkPoints* searchPoints,
  const vtkVector3d& a, const vtkVector3d& b, double tolerance, vtkIdList* candidates,
  std::vector<vtkIdType>& pointIdsOnEdges)
{
  pointIdsOnEdges.clear();

  const vtkVector3d ab = b - a;
  const double len2 = ab.SquaredNorm();
  if (len2 == 0.0)
  {
    return; // degenerate edge
  }
  const double len = std::sqrt(len2);
  // Distance under which a point is considered to be on the edge.
  const double tol = tolerance * len;
  const vtkVector3d mid = (a + b) * 0.5;

  // Get candidate points within half the edge length plus tolerance from its midpoint.
  locator->FindPointsWithinRadius(0.5 * len + tol, mid.GetData(), candidates);

  // Keep candidates that are on the edge, strictly between a and b.
  for (vtkIdType k = 0; k < candidates->GetNumberOfIds(); ++k)
  {
    const vtkIdType id = candidates->GetId(k);
    vtkVector3d p;
    searchPoints->GetPoint(id, p.GetData());
    // Project p on the edge line to compute scalar parameter t.
    const double t = (p - a).Dot(ab) / len2;
    // Reject points beyond or coincident with the end points.
    if (t * len <= tol || (1.0 - t) * len <= tol)
    {
      continue;
    }
    // `foot` is the projection of p on the edge; p is on the edge if it is close to foot.
    const vtkVector3d foot = a + ab * t;
    if ((p - foot).SquaredNorm() <= tol * tol)
    {
      pointIdsOnEdges.push_back(id);
    }
  }
}

//------------------------------------------------------------------------------
// Fill `pointIdsOnFaces` with the point ids of `searchPoints` that lie strictly inside the quad
// face defined by (v0, v1, v2, v3). Points on boundary edges are excluded because they are
// detected separately by edge sub-primitives. `locator` is a locator of `searchPoints`.
void FindPointsOnFace(vtkAbstractPointLocator* locator, vtkPoints* searchPoints,
  const vtkVector3d& v0, const vtkVector3d& v1, const vtkVector3d& v2, const vtkVector3d& v3,
  double tolerance, vtkIdList* candidates, std::vector<vtkIdType>& pointIdsOnFaces)
{
  pointIdsOnFaces.clear();

  const vtkVector3d center = (v0 + v1 + v2 + v3) * 0.25;
  const double diag1 = (v2 - v0).Norm();
  const double diag2 = (v3 - v1).Norm();
  const double maxDiag = std::max(diag1, diag2);
  if (maxDiag == 0.0)
  {
    return; // degenerate face
  }

  const double tol = tolerance * maxDiag;
  const double radius = 0.5 * maxDiag + tol;

  // Quad normal
  vtkVector3d n = (v1 - v0).Cross(v3 - v0);
  const double nLen = n.Norm();
  if (nLen == 0.0)
  {
    return; // degenerate quad
  }
  n = n * (1.0 / nLen);
  // The plane of v0, v1 and v3 is the plane of the face, since the faces of the cells are planar.
  assert(std::abs((v2 - v0).Dot(n)) <= tol && "the face is not planar");

  locator->FindPointsWithinRadius(radius, center.GetData(), candidates);

  const vtkVector3d quad[4] = { v0, v1, v2, v3 };
  for (vtkIdType k = 0; k < candidates->GetNumberOfIds(); ++k)
  {
    const vtkIdType id = candidates->GetId(k);
    vtkVector3d p;
    searchPoints->GetPoint(id, p.GetData());

    // 1. Check distance to quad plane
    const double distToPlane = std::abs((p - v0).Dot(n));
    if (distToPlane > tol)
    {
      continue;
    }

    // 2. Project p onto quad plane
    const vtkVector3d proj = p - n * (p - v0).Dot(n);

    // 3. Verify proj is strictly inside the 4 edge half-planes
    bool inside = true;
    for (int e = 0; e < 4; ++e)
    {
      const vtkVector3d& a = quad[e];
      const vtkVector3d& b = quad[(e + 1) % 4];
      const vtkVector3d ab = b - a;
      const double abLen = ab.Norm();
      if (abLen == 0.0)
      {
        inside = false;
        break;
      }

      const vtkVector3d edgeNormal = ab.Cross(n) * (1.0 / abLen);
      const double distToEdgePlane = (proj - a).Dot(edgeNormal);

      if (distToEdgePlane >= -tolerance * abLen)
      {
        inside = false;
        break;
      }
    }

    if (inside)
    {
      pointIdsOnFaces.push_back(id);
    }
  }
}

//------------------------------------------------------------------------------
// The points of `searchPoints`, found with `locator`, that lie on the edge or face whose numPts
// point ids are `ptIds`, with the coordinates of `cellPoints`, see FindPointsOnEdge and
// FindPointsOnFace. The points of the edge or face are first put in an order that only depends on
// their ids, so that the cells sharing it make bitwise identical floating-point queries.
void FindPointsOnSubPrimitive(vtkAbstractPointLocator* locator, vtkPoints* cellPoints,
  vtkPoints* searchPoints, const vtkIdType* ptIds, int numPts, double tolerance,
  vtkIdList* candidates, std::vector<vtkIdType>& pointIdsOnSubPrimitive)
{
  if (numPts == 2)
  {
    const vtkIdType id0 = ptIds[0];
    const vtkIdType id1 = ptIds[1];

    // The end point with the smallest id first
    const bool swapped = (id0 > id1);
    const vtkIdType canonId0 = swapped ? id1 : id0;
    const vtkIdType canonId1 = swapped ? id0 : id1;

    vtkVector3d posA, posB;
    cellPoints->GetPoint(canonId0, posA.GetData());
    cellPoints->GetPoint(canonId1, posB.GetData());

    FindPointsOnEdge(
      locator, searchPoints, posA, posB, tolerance, candidates, pointIdsOnSubPrimitive);
  }
  else if (numPts == 4)
  {
    // Start at the corner with the smallest id
    int minIdx = 0;
    for (int i = 1; i < 4; ++i)
    {
      if (ptIds[i] < ptIds[minIdx])
      {
        minIdx = i;
      }
    }
    // Then go toward its neighbor with the smallest id
    const int prevIdx = (minIdx + 3) % 4;
    const int nextIdx = (minIdx + 1) % 4;
    const bool reverseWinding = (ptIds[nextIdx] > ptIds[prevIdx]);
    // The coordinates in this order
    vtkVector3d canonicalPts[4];
    for (int i = 0; i < 4; ++i)
    {
      const int offset = reverseWinding ? (minIdx + 4 - i) % 4 : (minIdx + i) % 4;
      cellPoints->GetPoint(ptIds[offset], canonicalPts[i].GetData());
    }

    FindPointsOnFace(locator, searchPoints, canonicalPts[0], canonicalPts[1], canonicalPts[2],
      canonicalPts[3], tolerance, candidates, pointIdsOnSubPrimitive);
  }
  else
  {
    pointIdsOnSubPrimitive.clear();
  }
}

//------------------------------------------------------------------------------
// Set the hanging points of the candidate cells: for every sub-primitive to search that is not
// split yet, the point of `searchPoints`, found with `locator`, that lies on it, see
// FindPointsOnSubPrimitive. The id of the point locatorId of `searchPoints` in the output is
// `pointIdOffset + pointIdMapping[locatorId]`: the input points have an offset of 0, the remote
// points the number of input points. In the remote pass, the sub-primitives split by a local point
// are not searched again, so that a remote copy of a local point is never used instead of it.
// Under 2:1 refinement, a sub-primitive has at most one hanging point: several candidates are
// copies of the same point, received from several processes, and the one with the smallest id is
// used, see below. In 3D, the edges to search are shared by several cells: they are searched once
// each, as the unique edges of `edgesToSearch`, see EdgesToSearch.
template <int Dim, class DataSetT, class PointIdsT>
void FindPointsOnSubPrimitives(DataSetT* input, vtkAbstractPointLocator* locator,
  vtkPoints* searchPoints, vtkIdType pointIdOffset, double tolerance,
  std::vector<CandidateCellInfoT<Dim>>& candidatesInfo, PointIdsT pointIdMapping,
  const EdgeLocator& edgesToSearch)
{
  vtkPoints* points = input->GetPoints();
  vtkSMPThreadLocalObject<vtkIdList> tlCell;
  vtkSMPThreadLocalObject<vtkIdList> tlCandidates;
  vtkSMPThreadLocal<std::vector<vtkIdType>> tlOnSub;

  // ===========================================================================
  // 1. 3D: the point of every edge to search, -1 if none
  // ===========================================================================
  // In the remote pass, the edges split by a local point are searched too, but their point is not
  // used, see step 2.
  const vtkIdType numEdges = edgesToSearch.GetNumberOfElements();
  std::unique_ptr<vtkIdType[]> edgePointIds(new vtkIdType[numEdges]);
  vtkSMPTools::For(0, numEdges,
    [&](vtkIdType begin, vtkIdType end)
    {
      vtkIdList* candidates = tlCandidates.Local();
      std::vector<vtkIdType>& pointIdsOnSub = tlOnSub.Local();
      for (vtkIdType k = begin; k < end; ++k)
      {
        const EdgeTuple<vtkIdType>& edge = edgesToSearch.GetElement(k);
        const vtkIdType edgePtIds[2] = { edge.V0, edge.V1 };
        FindPointsOnSubPrimitive(
          locator, points, searchPoints, edgePtIds, 2, tolerance, candidates, pointIdsOnSub);
        if (pointIdsOnSub.empty())
        {
          edgePointIds[k] = -1;
          continue;
        }
        // The points found are copies of the same point, received from several processes: the one
        // with the smallest id is picked, which does not depend on the search, and all the cells of
        // the edge take it in step 2, so that they share the point.
        const vtkIdType locatorId = *std::min_element(pointIdsOnSub.begin(), pointIdsOnSub.end());
        edgePointIds[k] = pointIdOffset + pointIdMapping->GetValue(locatorId);
      }
    });

  // ===========================================================================
  // 2. Set the points of the sub-primitives to search of every candidate cell
  // ===========================================================================
  // The edges (3D) take the point found in step 1, and the sides are searched now: a side to
  // search has no neighbor, so it is in a single candidate cell.
  vtkSMPTools::For(0, static_cast<vtkIdType>(candidatesInfo.size()),
    [&](vtkIdType begin, vtkIdType end)
    {
      vtkIdList* tmpCell = tlCell.Local();
      vtkIdList* candidates = tlCandidates.Local();
      std::vector<vtkIdType>& pointIdsOnSub = tlOnSub.Local();
      vtkIdType npts;
      const vtkIdType* cell = nullptr;

      for (vtkIdType i = begin; i < end; ++i)
      {
        auto& info = candidatesInfo[i];
        if (!info.HasAnyUnsplitSubPrimitivesToSearch())
        {
          continue;
        }
        GetCellPoints(input, info.GetOriginalCellId(), npts, cell, tmpCell);
        assert(npts == CandidateCellInfoT<Dim>::NumPoints);

        for (int j = 0; j < CandidateCellInfoT<Dim>::NumSubPrimitives; ++j)
        {
          if (!info.IsSubPrimitiveToSearch(j) || info.IsSubPrimitiveSplit(j))
          {
            continue;
          }

          vtkIdType subPtIds[CandidateCellInfoT<Dim>::NumSidePoints];
          const int numSubPts = GetSubPrimitivePointIds<Dim>(cell, j, subPtIds);

          vtkIdType pointId = -1;
          if (j < CandidateCellInfoT<Dim>::FirstSide)
          {
            // An edge (3D), which is in `edgesToSearch`, see FindEdgesToSearch
            const vtkIdType edgeId = edgesToSearch.IsInsertedElement(subPtIds[0], subPtIds[1]);
            pointId = edgeId >= 0 ? edgePointIds[edgeId] : -1;
          }
          else
          {
            FindPointsOnSubPrimitive(locator, points, searchPoints, subPtIds, numSubPts, tolerance,
              candidates, pointIdsOnSub);
            if (!pointIdsOnSub.empty())
            {
              // The copy with the smallest id, as for the edges in step 1
              const vtkIdType locatorId =
                *std::min_element(pointIdsOnSub.begin(), pointIdsOnSub.end());
              pointId = pointIdOffset + pointIdMapping->GetValue(locatorId);
            }
          }

          if (pointId >= 0)
          {
            info.SetMidPoint(j, pointId);
            info.SetSplitSubPrimitive(j);
          }
        }
      }
    });
}

//------------------------------------------------------------------------------
// Restrict the sub-primitives to search of the candidate cells to the true boundary, once the
// local points are inserted: its sides, and in 3D, the edges to search from them.
//
// A side of a coarse cell that is split, i.e., has a hanging point, is tiled by the sides of its
// finer neighbors: 2 edges per split edge (2D), the halves from its end points to its hanging
// point, or 4 quads per split face (3D), one per corner of the face. These are the "finer sides".
// A side to search that is split locally, or that is one of the finer sides of another cell, is
// not on the true boundary, so it is cleared, except a split face with unsplit edges (3D): some of
// its finer neighbors are in other partitions. The others have no neighbor in this process. In 3D,
// `edgesToSearch` is set to the edges to search of the true boundary, see EdgesToSearch.
template <int Dim, class DataSetT>
void RestrictCandidateCells(DataSetT* input, std::vector<CandidateCellInfoT<Dim>>& candidatesInfo,
  EdgesToSearch& edgesToSearch)
{
  const vtkIdType numCells = static_cast<vtkIdType>(candidatesInfo.size());
  if (numCells == 0)
  {
    return;
  }

  // Shared thread-local scratch object reused across sequential parallel loops
  vtkSMPThreadLocalObject<vtkIdList> tlCell;

  // ===========================================================================
  // 1. Count the finer sides of the split sides of the candidates
  // ===========================================================================
  std::vector<vtkIdType> finerSideOffsets(numCells + 1, 0);
  vtkSMPTools::For(0, numCells,
    [&](vtkIdType begin, vtkIdType end)
    {
      for (vtkIdType i = begin; i < end; ++i)
      {
        // A split side has one finer side per corner: an edge half per end point, or a quad per
        // corner of the face.
        finerSideOffsets[i] =
          CandidateCellInfoT<Dim>::NumSidePoints * candidatesInfo[i].GetNumSplitSides();
      }
    });
  // The last entry is 0, so that the scan sets it to the number of finer sides.
  const vtkIdType numFinerSides = vtkSMPTools::ExclusiveScan(
    finerSideOffsets.begin(), finerSideOffsets.end(), static_cast<vtkIdType>(0));

  // ===========================================================================
  // 2. Build a locator of the finer sides
  // ===========================================================================
  // The finer sides and their locator are only used until step 3: this scope releases them before
  // step 4.
  {
    using SideTuple = typename CandidateCellInfoT<Dim>::SideTuple;
    std::vector<SideTuple> finerSides(numFinerSides);
    vtkSMPTools::For(0, numCells,
      [&](vtkIdType begin, vtkIdType end)
      {
        vtkIdList* tmpCell = tlCell.Local();
        vtkIdType npts;
        const vtkIdType* cellPoints = nullptr;
        for (vtkIdType i = begin; i < end; ++i)
        {
          if (finerSideOffsets[i + 1] == finerSideOffsets[i])
          {
            continue;
          }
          const auto& info = candidatesInfo[i];
          GetCellPoints(input, info.GetOriginalCellId(), npts, cellPoints, tmpCell);
          assert(npts == CandidateCellInfoT<Dim>::NumPoints);
          info.FillFinerSides(cellPoints, finerSides.data() + finerSideOffsets[i]);
        }
      });
    std::vector<vtkIdType>().swap(finerSideOffsets); // No longer needed

    vtkStaticElementLocator<vtkIdType, SideTuple> finerSideLocator;
    finerSideLocator.BuildLocator(numFinerSides, finerSides.data());

    // ===========================================================================
    // 3. Unmark the sides to search that are fully split, or that are finer sides of another cell
    // ===========================================================================
    vtkSMPTools::For(0, numCells,
      [&](vtkIdType begin, vtkIdType end)
      {
        vtkIdList* tmpCell = tlCell.Local();
        vtkIdType npts;
        const vtkIdType* points = nullptr;

        for (vtkIdType i = begin; i < end; ++i)
        {
          auto& info = candidatesInfo[i];
          GetCellPoints(input, info.GetOriginalCellId(), npts, points, tmpCell);
          assert(npts == CandidateCellInfoT<Dim>::NumPoints);

          // 3D: the edges to search of the local faces are stale, they are set again from the true
          // boundary below. In 2D, the edges are the sides, cleared selectively in the loop below.
          if constexpr (Dim == 3)
          {
            info.ResetEdgesToSearch();
          }

          for (int j = CandidateCellInfoT<Dim>::FirstSide;
               j < CandidateCellInfoT<Dim>::NumSubPrimitives; ++j)
          {
            if (!info.IsSubPrimitiveToSearch(j))
            {
              continue;
            }

            // A side split locally. In 3D, the 4 finer neighbors of a split face may be in several
            // partitions: the face stays on the true boundary until its 4 edges are split, since
            // the hanging points of the others are remote, and only found on the edges to search.
            if (info.IsSubPrimitiveSplit(j))
            {
              if (Dim == 2 ||
                info.GetNumSplitEdgesOfFace(j - CandidateCellInfoT<Dim>::FirstSide) == 4)
              {
                info.ResetSubPrimitiveToSearch(j);
              }
              continue;
            }

            // A finer side of a split side of another cell
            vtkIdType sidePts[CandidateCellInfoT<Dim>::NumSidePoints];
            GetSubPrimitivePointIds<Dim>(points, j, sidePts);
            const auto side = CandidateCellInfoT<Dim>::MakeSide(sidePts);
            if (finerSideLocator.IsInsertedElement(side) >= 0)
            {
              info.ResetSubPrimitiveToSearch(j);
            }
          }
        }
      });
  } // The finer sides and their locator, no longer needed

  // ===========================================================================
  // 4. 3D: set the edges to search of the true boundary, with a locator of the edges of its faces
  // ===========================================================================
  // The edges to search of the true boundary replace the local ones: the remote points can only
  // lie on the edges of the true boundary. The cells with such edges are already candidates, since
  // the true boundary is a part of the unmatched faces.
  if constexpr (Dim == 3)
  {
    // The cells are the candidates, the input cell of the candidate i being its original cell.
    auto getCellId = [&candidatesInfo](vtkIdType i)
    { return candidatesInfo[i].GetOriginalCellId(); };
    // A locator of the edges of the faces of the true boundary, the faces still to search after
    // step 3.
    BuildEdgeLocatorOfFacesToSearch(
      input, numCells, getCellId, [&candidatesInfo](vtkIdType i, int f)
      { return candidatesInfo[i].IsSubPrimitiveToSearch(vtkHexahedron::NumberOfEdges + f); },
      edgesToSearch);
    // Every edge of a candidate that is in the locator is to search again.
    FindEdgesToSearch(input, numCells, getCellId, edgesToSearch.Locator,
      [&candidatesInfo](vtkIdType i, int e) { candidatesInfo[i].SetSubPrimitiveToSearch(e); });
  }
}

} // namespace

//------------------------------------------------------------------------------
vtkBoundingBox vtkResolveHangingNodes::GetBoundaryBounds(vtkPolyData* boundary, double tolerance)
{
  vtkBoundingBox bounds;
  if (boundary && boundary->GetNumberOfPoints() > 0)
  {
    bounds.AddBounds(boundary->GetBounds());
    bounds.Inflate(tolerance * bounds.GetDiagonalLength());
  }
  return bounds;
}

//------------------------------------------------------------------------------
void vtkResolveHangingNodes::BoundaryBins::Initialize(const vtkBoundingBox& bounds)
{
  double binBounds[6];
  bounds.ComputeDivisions(TargetNumberOfBins, binBounds, this->Divisions);
  this->Bounds.SetBounds(binBounds);
  this->Occupied.reset();
}

//------------------------------------------------------------------------------
vtkIdType vtkResolveHangingNodes::BoundaryBins::GetNumberOfBins() const
{
  return static_cast<vtkIdType>(this->Divisions[0]) * this->Divisions[1] * this->Divisions[2];
}

//------------------------------------------------------------------------------
void vtkResolveHangingNodes::BoundaryBins::GetBinIndices(const double* x, int ijk[3]) const
{
  const double* minPoint = this->Bounds.GetMinPoint();
  for (int axis = 0; axis < 3; ++axis)
  {
    const double length = this->Bounds.GetLength(axis);
    ijk[axis] = length > 0.0
      ? static_cast<int>(std::floor((x[axis] - minPoint[axis]) / length * this->Divisions[axis]))
      : 0;
    ijk[axis] = std::min(std::max(ijk[axis], 0), this->Divisions[axis] - 1);
  }
}

//------------------------------------------------------------------------------
vtkIdType vtkResolveHangingNodes::BoundaryBins::GetBinIndex(const int ijk[3]) const
{
  return ijk[0] +
    static_cast<vtkIdType>(this->Divisions[0]) *
    (ijk[1] + static_cast<vtkIdType>(this->Divisions[1]) * ijk[2]);
}

//------------------------------------------------------------------------------
void vtkResolveHangingNodes::BoundaryBins::SetBinOccupied(vtkIdType binId)
{
  this->Occupied.set(binId);
}

//------------------------------------------------------------------------------
vtkIdType vtkResolveHangingNodes::BoundaryBins::GetBinIndex(const double* x) const
{
  int ijk[3];
  this->GetBinIndices(x, ijk);
  return this->GetBinIndex(ijk);
}

//------------------------------------------------------------------------------
bool vtkResolveHangingNodes::BoundaryBins::IsBinOccupied(vtkIdType binId) const
{
  return this->Occupied[binId];
}

//------------------------------------------------------------------------------
bool vtkResolveHangingNodes::BoundaryBins::IsInOccupiedBin(const double* x) const
{
  return this->Bounds.ContainsPoint(x) && this->IsBinOccupied(this->GetBinIndex(x));
}

//------------------------------------------------------------------------------
// The points that lie on an element are within `tolerance` times its size from it, see
// FindPointsOnEdge.
void vtkResolveHangingNodes::BinBoundaryElements(
  BoundaryBins& bins, vtkPolyData* boundary, double tolerance)
{
  vtkCellArray* elements = nullptr;
  if (boundary)
  {
    elements = boundary->GetNumberOfLines() > 0 ? boundary->GetLines() : boundary->GetPolys();
  }
  if (!elements || elements->GetNumberOfCells() == 0)
  {
    bins.Occupied.set();
    return;
  }

  // Several elements can overlap the same bin, so the bins are marked with atomics, then packed.
  // The packing is serial: there are at most TargetNumberOfBins bins.
  const vtkIdType numBins = bins.GetNumberOfBins();
  std::unique_ptr<std::atomic<unsigned char>[]> marks(new std::atomic<unsigned char>[numBins]());
  vtkPoints* points = boundary->GetPoints();
  vtkSMPThreadLocalObject<vtkIdList> tlElement;
  vtkSMPTools::For(0, elements->GetNumberOfCells(),
    [&](vtkIdType begin, vtkIdType end)
    {
      vtkIdList* tmpElement = tlElement.Local();
      vtkIdType npts;
      const vtkIdType* pts = nullptr;
      double p[3];
      for (vtkIdType cellId = begin; cellId < end; ++cellId)
      {
        elements->GetCellAtId(cellId, npts, pts, tmpElement);
        vtkBoundingBox elementBounds;
        for (vtkIdType k = 0; k < npts; ++k)
        {
          points->GetPoint(pts[k], p);
          elementBounds.AddPoint(p);
        }
        elementBounds.Inflate(tolerance * elementBounds.GetDiagonalLength());
        int ijkMin[3], ijkMax[3];
        bins.GetBinIndices(elementBounds.GetMinPoint(), ijkMin);
        bins.GetBinIndices(elementBounds.GetMaxPoint(), ijkMax);
        int ijk[3];
        for (ijk[2] = ijkMin[2]; ijk[2] <= ijkMax[2]; ++ijk[2])
        {
          for (ijk[1] = ijkMin[1]; ijk[1] <= ijkMax[1]; ++ijk[1])
          {
            for (ijk[0] = ijkMin[0]; ijk[0] <= ijkMax[0]; ++ijk[0])
            {
              // memory_order_relaxed is safe, the atomics are not used for synchronization.
              marks[bins.GetBinIndex(ijk)].store(1, std::memory_order_relaxed);
            }
          }
        }
      }
    });
  for (vtkIdType binId = 0; binId < numBins; ++binId)
  {
    if (marks[binId].load(std::memory_order_relaxed))
    {
      bins.SetBinOccupied(binId);
    }
  }
}

//------------------------------------------------------------------------------
vtkSmartPointer<vtkPolyData> vtkResolveHangingNodes::ExtractPointsInBins(
  vtkPolyData* source, const BoundaryBins& bins)
{
  // Select the points in the occupied bins, then list them.
  const vtkIdType numPoints = source->GetNumberOfPoints();
  std::vector<unsigned char> selected(numPoints);
  vtkSMPTools::For(0, numPoints,
    [&](vtkIdType begin, vtkIdType end)
    {
      double p[3];
      for (vtkIdType id = begin; id < end; ++id)
      {
        source->GetPoint(id, p);
        selected[id] = bins.IsInOccupiedBin(p) ? 1 : 0;
      }
    });
  vtkNew<vtkIdList> ids;
  SelectIds(selected.data(), numPoints, ids);
  std::vector<unsigned char>().swap(selected); // No longer needed
  return ExtractPoints(source, ids);
}

namespace
{
//------------------------------------------------------------------------------
// Whether `a` and `b` have the same arrays (names and numbers of components) in the same order, so
// that tuples can be copied between them. The types of the arrays may differ if both are numeric,
// since the copy converts them: a serialization changes some types, e.g., the XML writer and
// reader of vtkDIYUtilities turn char into signed char, and long into long long.
bool HaveSameDataArrays(vtkFieldData* a, vtkFieldData* b)
{
  if (a->GetNumberOfArrays() != b->GetNumberOfArrays())
  {
    return false;
  }
  for (int i = 0; i < a->GetNumberOfArrays(); ++i)
  {
    vtkAbstractArray* x = a->GetAbstractArray(i);
    vtkAbstractArray* y = b->GetAbstractArray(i);
    if (!x || !y ||
      (x->GetDataType() != y->GetDataType() &&
        (!vtkArrayDownCast<vtkDataArray>(x) || !vtkArrayDownCast<vtkDataArray>(y))) ||
      x->GetNumberOfComponents() != y->GetNumberOfComponents() ||
      std::string(x->GetName() ? x->GetName() : "") !=
        std::string(y->GetName() ? y->GetName() : ""))
    {
      return false;
    }
  }
  return true;
}

//------------------------------------------------------------------------------
// The points of `pointSets`, with their point data, appended in a vtkPolyData without cells, or
// nullptr if they have none. The points and the point data have the data type and the arrays of
// `reference`, which the point sets must have too (see HaveSameDataArrays).
vtkSmartPointer<vtkPolyData> AppendPoints(
  const std::vector<vtkPointSet*>& pointSets, vtkPointSet* reference)
{
  vtkIdType numPoints = 0;
  for (vtkPointSet* pointSet : pointSets)
  {
    numPoints += pointSet->GetNumberOfPoints();
  }
  if (numPoints == 0)
  {
    return nullptr;
  }

  vtkNew<vtkPoints> points;
  points->SetDataType(reference->GetPoints()->GetDataType());
  points->SetNumberOfPoints(numPoints);
  vtkNew<vtkPolyData> result;
  vtkPointData* pointData = result->GetPointData();
  pointData->CopyGlobalIdsOn();
  pointData->CopyAllocate(reference->GetPointData(), numPoints);
  pointData->SetNumberOfTuples(numPoints);
  vtkIdType next = 0;
  for (vtkPointSet* pointSet : pointSets)
  {
    const vtkIdType numSetPoints = pointSet->GetNumberOfPoints();
    points->InsertPoints(next, numSetPoints, 0, pointSet->GetPoints());
    pointData->CopyData(pointSet->GetPointData(), next, numSetPoints, 0);
    next += numSetPoints;
  }
  result->SetPoints(points);
  return result;
}
} // namespace

//------------------------------------------------------------------------------
vtkSmartPointer<vtkPolyData> vtkResolveHangingNodes::MergeReceivedPoints(
  const std::map<int, vtkSmartPointer<vtkPolyData>>& received, vtkPolyData* reference) const
{
  if (!reference)
  {
    return nullptr;
  }
  std::vector<vtkPointSet*> pointSets;
  for (const auto& [partitionId, pointSet] : received)
  {
    if (!pointSet || pointSet->GetNumberOfPoints() == 0)
    {
      continue;
    }
    if (!HaveSameDataArrays(reference->GetPointData(), pointSet->GetPointData()))
    {
      vtkWarningMacro("The points received from partition "
        << partitionId
        << " have different point data arrays than the local ones, they are ignored.");
      continue;
    }
    pointSets.push_back(pointSet);
  }
  // The points are only appended: a point shared by several partitions is received several times,
  // but the filter uses only one of the copies that lie on an edge or face (the one with the
  // smallest id).
  return AppendPoints(pointSets, reference);
}

//------------------------------------------------------------------------------
// The exchange between the partitions of this process, with the steps of vtkPResolveHangingNodes,
// which uses diy between the partitions of all the processes: the bounds, the neighbors, the bins
// of the partitions that have neighbors, then the points.
std::vector<vtkSmartPointer<vtkPolyData>> vtkResolveHangingNodes::GetRemotePoints(
  const std::vector<vtkPolyData*>& boundaries, const std::vector<int>& partitionedDataSetIds)
{
  const std::size_t numPartitions = boundaries.size();
  // The bounds of the boundary of every partition, invalid without boundary
  std::vector<vtkBoundingBox> bounds(numPartitions);
  for (std::size_t p = 0; p < numPartitions; ++p)
  {
    bounds[p] = vtkResolveHangingNodes::GetBoundaryBounds(boundaries[p], this->Tolerance);
  }

  // The neighbors of every partition, the other partitions whose bounds overlap its own. A
  // partition whose partitionedDataSetIds is -1 has all of them as neighbors. Otherwise, it is the
  // index of its vtkPartitionedDataSet, e.g., an AMR level, whose partitions are conforming with
  // it: only the partitions of another vtkPartitionedDataSet are neighbors. Invalid bounds overlap
  // none.
  std::vector<std::vector<std::size_t>> neighbors(numPartitions);
  for (std::size_t p = 0; p < numPartitions; ++p)
  {
    for (std::size_t q = 0; q < numPartitions; ++q)
    {
      if (q != p &&
        (partitionedDataSetIds[p] < 0 || partitionedDataSetIds[q] != partitionedDataSetIds[p]) &&
        bounds[p].Intersects(bounds[q]))
      {
        neighbors[p].push_back(q);
      }
    }
  }

  // The bins that the boundary of every partition occupies, only needed with neighbors
  std::vector<BoundaryBins> bins(numPartitions);
  for (std::size_t p = 0; p < numPartitions; ++p)
  {
    if (!neighbors[p].empty())
    {
      bins[p].Initialize(bounds[p]);
      vtkResolveHangingNodes::BinBoundaryElements(bins[p], boundaries[p], this->Tolerance);
    }
  }

  // The points of the neighbors of every partition that are in its occupied bins
  std::vector<vtkSmartPointer<vtkPolyData>> remotePoints(numPartitions);
  for (std::size_t p = 0; p < numPartitions; ++p)
  {
    std::map<int, vtkSmartPointer<vtkPolyData>> received;
    for (std::size_t q : neighbors[p])
    {
      received.emplace(
        static_cast<int>(q), vtkResolveHangingNodes::ExtractPointsInBins(boundaries[q], bins[p]));
    }
    remotePoints[p] = this->MergeReceivedPoints(received, boundaries[p]);
  }
  return remotePoints;
}

namespace
{
//------------------------------------------------------------------------------
// Add the inverse measure of the cells of `output` as cell data.
template <int Dim, class OutputT>
void AddInverseMeasures(OutputT* output)
{
  vtkNew<vtkAffineArray<vtkIdType>> outputCells;
  outputCells->ConstructBackend(1, 0);
  outputCells->SetNumberOfValues(output->GetNumberOfCells());
  output->GetCellData()->AddArray(ComputeMeasures<Dim, true>(output, outputCells.GetPointer()));
}

//------------------------------------------------------------------------------
// Renumber the remote points used as hanging points, from `basePointOffset` in their order, and
// return their ids among the remote points.
template <int Dim>
vtkSmartPointer<vtkIdList> CommitRemotePointIds(vtkPointSet* remote, vtkIdType basePointOffset,
  std::vector<CandidateCellInfoT<Dim>>& candidatesInfo)
{
  vtkNew<vtkIdList> usedRemoteIds;
  if (!remote)
  {
    return usedRemoteIds;
  }
  const vtkIdType numRemotePoints = remote->GetNumberOfPoints();

  // Step 1: Mark referenced remote points
  std::unique_ptr<std::atomic<unsigned char>[]> used(
    new std::atomic<unsigned char>[numRemotePoints]());

  vtkSMPTools::For(0, static_cast<vtkIdType>(candidatesInfo.size()),
    [&](vtkIdType begin, vtkIdType end)
    {
      for (vtkIdType i = begin; i < end; ++i)
      {
        for (const vtkIdType& id : candidatesInfo[i].GetMidPoints())
        {
          if (id >= basePointOffset)
          {
            used[id - basePointOffset].store(1, std::memory_order_relaxed);
          }
        }
      }
    });

  // Step 2: the referenced remote points, in their order
  if (SelectIds(used.get(), numRemotePoints, usedRemoteIds) == 0)
  {
    return usedRemoteIds;
  }
  used.reset(); // No longer needed

  // Step 3: the index of every referenced remote point among them, set for these points only,
  // since the others are not read
  std::unique_ptr<vtkIdType[]> newIds(new vtkIdType[numRemotePoints]);
  vtkSMPTools::For(0, usedRemoteIds->GetNumberOfIds(),
    [&](vtkIdType begin, vtkIdType end)
    {
      for (vtkIdType k = begin; k < end; ++k)
      {
        newIds[usedRemoteIds->GetId(k)] = k;
      }
    });

  // Step 4: the remote hanging points get their output ids
  vtkSMPTools::For(0, static_cast<vtkIdType>(candidatesInfo.size()),
    [&](vtkIdType begin, vtkIdType end)
    {
      for (vtkIdType i = begin; i < end; ++i)
      {
        for (int j = 0; j < CandidateCellInfoT<Dim>::NumSubPrimitives; ++j)
        {
          // Only the split sub-primitives have a hanging point
          if (candidatesInfo[i].IsSubPrimitiveSplit(j))
          {
            const vtkIdType currentId = candidatesInfo[i].GetMidPoint(j);
            if (currentId >= basePointOffset) // A remote point
            {
              const vtkIdType remoteIdx = currentId - basePointOffset;
              candidatesInfo[i].SetMidPoint(j, basePointOffset + newIds[remoteIdx]);
            }
          }
        }
      }
    });

  return usedRemoteIds;
}

//------------------------------------------------------------------------------
// Assign the ids of the center points of the Category B cells, from `basePointOffset`, and return
// the indices of these cells in `candidatesInfo`. Only used with Decompose.
template <int Dim>
vtkSmartPointer<vtkIdList> CommitCentroidPointIds(
  vtkIdType basePointOffset, std::vector<CandidateCellInfoT<Dim>>& candidatesInfo)
{
  vtkNew<vtkIdList> candidateIdsWithCentroids;
  if constexpr (Dim == 3)
  {
    const vtkIdType numCells = static_cast<vtkIdType>(candidatesInfo.size());
    if (numCells == 0)
    {
      return candidateIdsWithCentroids;
    }

    // Step 1: flag the Category B cells
    std::vector<unsigned char> flags(numCells);
    vtkSMPTools::For(0, numCells,
      [&](vtkIdType begin, vtkIdType end)
      {
        for (vtkIdType i = begin; i < end; ++i)
        {
          flags[i] = candidatesInfo[i].HasCentroidPoint() ? 1 : 0;
        }
      });

    // Step 2: their indices
    if (SelectIds(flags.data(), numCells, candidateIdsWithCentroids) == 0)
    {
      return candidateIdsWithCentroids;
    }

    // Step 3: the ids of their center points
    vtkSMPTools::For(0, candidateIdsWithCentroids->GetNumberOfIds(),
      [&](vtkIdType begin, vtkIdType end)
      {
        for (vtkIdType i = begin; i < end; ++i)
        {
          const vtkIdType candidateCellId = candidateIdsWithCentroids->GetId(i);
          candidatesInfo[candidateCellId].SetCentroidPoint(basePointOffset + i);
        }
      });
  }
  return candidateIdsWithCentroids;
}

//==============================================================================
// CandidateCellInfoT: decomposition of the resolved cells
//==============================================================================

//------------------------------------------------------------------------------
template <int Dim>
QuadSplitCases::QuadCase CandidateCellInfoT<Dim>::GetQuadSplitCase(int faceIdx) const
{
  std::bitset<4> splitEdges;
  for (int k = 0; k < 4; ++k)
  {
    if constexpr (Dim == 2)
    {
      // The quad is the cell, whose edge k goes from its corner k to its corner k + 1.
      splitEdges[k] = this->SplitSubPrimitives[k];
    }
    else
    {
      splitEdges[k] = this->SplitSubPrimitives[FaceEdges[faceIdx][k]];
    }
  }
  return QuadSplitCases::GetCase(splitEdges);
}

//------------------------------------------------------------------------------
template <int Dim>
std::array<vtkIdType, 8> CandidateCellInfoT<Dim>::GetQuadPoints(
  const vtkIdType* pts, int faceIdx) const
{
  std::array<vtkIdType, 8> quadPts;
  if constexpr (Dim == 2)
  {
    // The quad is the cell.
    for (int k = 0; k < 4; ++k)
    {
      quadPts[k] = pts[k];
      quadPts[4 + k] = this->MidPoints[k];
    }
  }
  else
  {
    const vtkIdType* quadCorners = vtkHexahedron::GetFaceArray(faceIdx);
    for (int k = 0; k < 4; ++k)
    {
      quadPts[k] = pts[quadCorners[k]];
      quadPts[4 + k] = this->MidPoints[FaceEdges[faceIdx][k]];
    }
  }
  return quadPts;
}

//------------------------------------------------------------------------------
// The number of split faces, 0 in 2D.
template <int Dim>
int CandidateCellInfoT<Dim>::GetNumSplitFaces() const
{
  if constexpr (Dim == 2)
  {
    return 0;
  }
  else
  {
    return static_cast<int>((this->SplitSubPrimitives & FaceMask3D).count());
  }
}

//------------------------------------------------------------------------------
// The number of split edges.
template <int Dim>
int CandidateCellInfoT<Dim>::GetNumSplitEdges() const
{
  if constexpr (Dim == 2)
  {
    return static_cast<int>(this->SplitSubPrimitives.count());
  }
  else
  {
    return static_cast<int>((this->SplitSubPrimitives & EdgeMask3D).count());
  }
}

//------------------------------------------------------------------------------
// The number of split edges of the 3D face faceIdx, 0 in 2D.
template <int Dim>
int CandidateCellInfoT<Dim>::GetNumSplitEdgesOfFace([[maybe_unused]] int faceIdx) const
{
  int count = 0;
  if constexpr (Dim == 3)
  {
    assert(faceIdx >= 0 && faceIdx < vtkHexahedron::NumberOfFaces);
    for (const auto edge : FaceEdges[faceIdx])
    {
      count += this->SplitSubPrimitives[edge] ? 1 : 0;
    }
  }
  return count;
}

//------------------------------------------------------------------------------
template <int Dim>
typename CandidateCellInfoT<Dim>::SideTuple CandidateCellInfoT<Dim>::MakeSide(
  const vtkIdType* sidePts)
{
  if constexpr (Dim == 2)
  {
    return SideTuple(sidePts[0], sidePts[1]);
  }
  else
  {
    return SideTuple(sidePts[0], sidePts[1], sidePts[2], sidePts[3]);
  }
}

//------------------------------------------------------------------------------
// A split edge gives its 2 halves, from its end points to its hanging point. A split face gives
// its 4 quarters, one per corner Vk: (Vk, Mk, C, MkPrev), where C is the hanging point at the
// center of the face, and Mk and MkPrev the hanging points of the edges of the face after and
// before Vk (or the corners after and before Vk if these edges are not split, for invalid
// inputs).
template <int Dim>
void CandidateCellInfoT<Dim>::FillFinerSides(const vtkIdType* pts, SideTuple* finerSides) const
{
  vtkIdType next = 0;
  if constexpr (Dim == 2)
  {
    for (int j = 0; j < NumEdges; ++j)
    {
      if (this->IsSubPrimitiveSplit(j))
      {
        const vtkIdType* edge = vtkQuad::GetEdgeArray(j);
        const vtkIdType mid = this->GetMidPoint(j);
        finerSides[next++] = SideTuple(pts[edge[0]], mid);
        finerSides[next++] = SideTuple(mid, pts[edge[1]]);
      }
    }
  }
  else
  {
    for (int f = 0; f < vtkHexahedron::NumberOfFaces; ++f)
    {
      const int faceSubPrimIdx = vtkHexahedron::NumberOfEdges + f;
      if (!this->IsSubPrimitiveSplit(faceSubPrimIdx))
      {
        continue;
      }
      const vtkIdType* face = vtkHexahedron::GetFaceArray(f);
      const auto& faceEdgeIndices = FaceEdges[f];
      const vtkIdType C = this->GetMidPoint(faceSubPrimIdx);
      for (int k = 0; k < 4; ++k)
      {
        const vtkIdType Vk = pts[face[k]];
        const vtkIdType VkNext = pts[face[(k + 1) % 4]];
        const int eCurr = faceEdgeIndices[k];
        const int ePrev = faceEdgeIndices[(k + 3) % 4];
        const vtkIdType Mk = this->IsSubPrimitiveSplit(eCurr) ? this->GetMidPoint(eCurr) : VkNext;
        const vtkIdType MkPrev = this->IsSubPrimitiveSplit(ePrev) ? this->GetMidPoint(ePrev) : Vk;
        finerSides[next++] = SideTuple(Vk, Mk, C, MkPrev);
      }
    }
  }
  assert(next == NumSidePoints * this->GetNumSplitSides());
}

//------------------------------------------------------------------------------
// The split face of Category A1, a coarse cell next to finer cells across one face, or -1.
template <int Dim>
int CandidateCellInfoT<Dim>::GetSingleSplitFaceIndex() const
{
  if constexpr (Dim == 3)
  {
    if (this->GetNumSplitFaces() == 1 && this->GetNumSplitEdges() == 4)
    {
      for (int f = 0; f < vtkHexahedron::NumberOfFaces; ++f)
      {
        if (this->SplitSubPrimitives[vtkHexahedron::NumberOfEdges + f])
        {
          return this->GetNumSplitEdgesOfFace(f) == 4 ? f : -1;
        }
      }
    }
  }
  return -1;
}

//------------------------------------------------------------------------------
// The split edge of Category A2, a coarse cell next to finer cells only along one edge, or -1.
template <int Dim>
int CandidateCellInfoT<Dim>::GetSingleSplitEdgeIndex() const
{
  if constexpr (Dim == 3)
  {
    if (this->GetNumSplitEdges() == 1 && this->GetNumSplitFaces() == 0)
    {
      for (int e = 0; e < vtkHexahedron::NumberOfEdges; ++e)
      {
        if (this->SplitSubPrimitives[e])
        {
          return e;
        }
      }
    }
  }
  return -1;
}

//------------------------------------------------------------------------------
// The sides to search: all the sub-primitives to search in 2D, the faces to search in 3D.
template <int Dim>
typename CandidateCellInfoT<Dim>::SubPrimitivesBitset CandidateCellInfoT<Dim>::GetSidesToSearch()
  const
{
  if constexpr (Dim == 2)
  {
    return this->SubPrimitivesToSearch;
  }
  else
  {
    return this->SubPrimitivesToSearch & FaceMask3D;
  }
}

//------------------------------------------------------------------------------
// Whether the decomposition of a 3D cell (with Decompose) needs a point at its center: the cell is
// split, but is neither in Category A1 nor A2.
template <int Dim>
template <bool Decompose>
bool CandidateCellInfoT<Dim>::HasCentroidPoint() const
{
  if constexpr (Dim == 3 && Decompose)
  {
    if (this->HasAnySplitSubPrimitives())
    {
      const bool isCategoryA1 = this->GetSingleSplitFaceIndex() >= 0;
      const bool isCategoryA2 = this->GetSingleSplitEdgeIndex() >= 0;

      return !isCategoryA1 && !isCategoryA2;
    }
  }
  return false;
}

//------------------------------------------------------------------------------
// The number of cells that replace a candidate cell, see the declaration.
template <int Dim>
template <bool Decompose>
vtkIdType CandidateCellInfoT<Dim>::GetOutputCellCount() const
{
  if constexpr (!Decompose)
  {
    return 1;
  }
  else if constexpr (Dim == 2)
  {
    // 2D: the triangles and quads of QuadSplitCases
    return this->GetQuadSplitCase(0).NumCells;
  }
  else // Dim == 3
  {
    assert(this->HasAnySplitSubPrimitives());

    // Category A1: 5 pyramids and 4 tetrahedra
    if (this->GetSingleSplitFaceIndex() >= 0)
    {
      return 9;
    }

    // Category A2: 4 pyramids
    if (this->GetSingleSplitEdgeIndex() >= 0)
    {
      return 4;
    }

    // Category B: the faces coned to the center
    vtkIdType totalSubCells = 0;
    for (int f = 0; f < vtkHexahedron::NumberOfFaces; ++f)
    {
      if (this->IsSubPrimitiveSplit(vtkHexahedron::NumberOfEdges + f))
      {
        totalSubCells += 4; // Face center split -> 4 pyramids on its quarters
      }
      else
      {
        totalSubCells += this->GetQuadSplitCase(f).NumCells;
      }
    }
    return totalSubCells;
  }
}

//------------------------------------------------------------------------------
// The corners and the hanging points.
template <int Dim>
vtkIdType CandidateCellInfoT<Dim>::GetPolyPointCount() const
{
  if constexpr (Dim == 2)
  {
    return static_cast<vtkIdType>(4 + this->GetNumSplitEdges());
  }
  else // Dim == 3
  {
    return static_cast<vtkIdType>(8 + this->GetNumSplitEdges() + this->GetNumSplitFaces());
  }
}

//------------------------------------------------------------------------------
// An unsplit face is 1 face, a split face its 4 quarters.
template <int Dim>
vtkIdType CandidateCellInfoT<Dim>::GetPolyhedronFaceCount() const
{
  if constexpr (Dim == 3)
  {
    return static_cast<vtkIdType>(6 + (3 * this->GetNumSplitFaces()));
  }
  else
  {
    return 0;
  }
}

//------------------------------------------------------------------------------
// The polygon or polyhedron without Decompose. Otherwise, 3 or 4 points per triangle or quad (2D),
// and 5 per pyramid and 4 per tetrahedron (3D).
template <int Dim>
template <bool Decompose>
void CandidateCellInfoT<Dim>::FillOutputCellSizes(
  vtkIdType* sizesPtr, [[maybe_unused]] vtkIdType numCells) const
{
  if constexpr (!Decompose)
  {
    assert(numCells == 1);
    sizesPtr[0] = this->GetPolyPointCount();
  }
  else if constexpr (Dim == 2)
  {
    const QuadSplitCases::QuadCase splitCase = this->GetQuadSplitCase(0);
    const auto* indexPtr = splitCase.Indices.data();
    for (vtkIdType j = 0; j < numCells; ++j)
    {
      const auto cellSize = *indexPtr;
      sizesPtr[j] = cellSize;
      indexPtr += cellSize + 1;
    }
  }
  else // Dim == 3
  {
    assert(this->HasAnySplitSubPrimitives());

    // Category A1: 5 pyramids, then 4 tetrahedra
    if (this->GetSingleSplitFaceIndex() >= 0)
    {
      std::fill_n(sizesPtr, 5, 5);
      std::fill_n(sizesPtr + 5, 4, 4);
      return;
    }

    // Category A2: 4 pyramids
    if (this->GetSingleSplitEdgeIndex() >= 0)
    {
      std::fill_n(sizesPtr, 4, 5);
      return;
    }

    // Category B
    vtkIdType writeIdx = 0;
    for (int f = 0; f < vtkHexahedron::NumberOfFaces; ++f)
    {
      if (this->IsSubPrimitiveSplit(vtkHexahedron::NumberOfEdges + f))
      {
        // The 4 pyramids on the quarters of a split face
        for (int k = 0; k < 4; ++k)
        {
          sizesPtr[writeIdx++] = 5;
        }
      }
      else
      {
        // The triangles and quads of the face coned to the center: tetrahedra and pyramids
        const QuadSplitCases::QuadCase splitCase = this->GetQuadSplitCase(f);
        const auto* indexPtr = splitCase.Indices.data();
        for (int k = 0; k < splitCase.NumCells; ++k)
        {
          const auto baseSize = *indexPtr;
          sizesPtr[writeIdx++] = baseSize + 1;
          indexPtr += baseSize + 1;
        }
      }
    }
    assert(writeIdx == numCells);
  }
}

//------------------------------------------------------------------------------
// The quarters of a split face have 4 points, the other faces their corners and hanging points.
template <int Dim>
void CandidateCellInfoT<Dim>::FillPolyhedronFaceSizes(vtkIdType* faceSizesPtr) const
{
  vtkIdType faceIdx = 0;
  for (int f = 0; f < vtkHexahedron::NumberOfFaces; ++f)
  {
    const int faceSubPrimIdx = vtkHexahedron::NumberOfEdges + f;
    if (this->IsSubPrimitiveSplit(faceSubPrimIdx))
    {
      faceSizesPtr[faceIdx++] = 4;
      faceSizesPtr[faceIdx++] = 4;
      faceSizesPtr[faceIdx++] = 4;
      faceSizesPtr[faceIdx++] = 4;
    }
    else
    {
      faceSizesPtr[faceIdx++] = 4 + this->GetNumSplitEdgesOfFace(f);
    }
  }
}

//------------------------------------------------------------------------------
// See the declaration.
template <int Dim>
void CandidateCellInfoT<Dim>::EmitSingleSubCell(vtkIdType subIdx, unsigned char cellType,
  const vtkIdType* pts, vtkIdType numPts, vtkIdType* connPtr, const vtkIdType* connOffsetsPtr,
  unsigned char* typesPtr)
{
  typesPtr[subIdx] = cellType;
  std::copy_n(pts, numPts, connPtr + connOffsetsPtr[subIdx]);
}

//------------------------------------------------------------------------------
// Emit the cells that replace a candidate cell, see the declaration. Without Decompose, a polygon
// (2D) or polyhedron (3D) with the hanging points. Otherwise, the cell is decomposed:
// - 2D: into the triangles and quads of QuadSplitCases;
// - 3D: each face is split as the cell on its other side splits it, into its 4 quarters if its
//   center is split (the faces of the finer neighbors), otherwise by QuadSplitCases. The cell is
//   then filled by pulling (coning) a point over the faces that do not contain it (see the pulling
//   triangulations in J. A. De Loera, J. Rambau, F. Santos, "Triangulations: Structures for
//   Algorithms and Applications", Springer, 2010): its hanging point in Category A2, its center in
//   Category B, as in S. J. Owen, S. Saigal, "Formation of pyramid elements for hexahedra to
//   tetrahedra transitions", Computer Methods in Applied Mechanics and Engineering 190(34),
//   pp. 4505-4518, 2001. Category A1 adds no point either, see below. The faces of the hexahedron
//   are outward, so they are reversed to get the bases of the pyramids and tetrahedra, whose
//   normal points toward the apex.
template <int Dim>
template <bool Decompose>
void CandidateCellInfoT<Dim>::EmitOutputCells(const vtkIdType* pts,
  [[maybe_unused]] vtkIdType numCells, vtkIdType* connPtr, const vtkIdType* connOffsetsPtr,
  unsigned char* typesPtr, [[maybe_unused]] vtkIdType* facesConnPtr,
  [[maybe_unused]] const vtkIdType* facesOffsetsPtr) const
{
  if constexpr (!Decompose)
  {
    assert(numCells == 1);
    vtkIdType connWriteIdx = connOffsetsPtr[0];
    if constexpr (Dim == 2)
    {
      typesPtr[0] = GetPolygonType(this->GetPolyPointCount());
      for (int j = 0; j < NumPoints; ++j)
      {
        connPtr[connWriteIdx++] = pts[j];
        if (this->HasMidPoint(j))
        {
          connPtr[connWriteIdx++] = this->GetMidPoint(j);
        }
      }
    }
    else // Dim == 3
    {
      assert(this->HasAnySplitSubPrimitives());
      typesPtr[0] = VTK_POLYHEDRON;
      for (vtkIdType j = 0; j < NumPoints; ++j)
      {
        connPtr[connWriteIdx++] = pts[j];
      }
      for (int j = 0; j < NumSubPrimitives; ++j)
      {
        if (this->HasMidPoint(j))
        {
          connPtr[connWriteIdx++] = this->GetMidPoint(j);
        }
      }

      // The faces: the 4 quarters of a split face, otherwise the face with the hanging points of
      // its split edges.
      vtkIdType faceIdx = 0;
      for (int f = 0; f < vtkHexahedron::NumberOfFaces; ++f)
      {
        const vtkIdType* faceCorners = vtkHexahedron::GetFaceArray(f);
        const auto& faceEdges = FaceEdges[f];
        if (this->IsSubPrimitiveSplit(vtkHexahedron::NumberOfEdges + f))
        {
          const vtkIdType C = this->MidPoints[vtkHexahedron::NumberOfEdges + f];
          std::array<vtkIdType, 4> V;
          std::array<vtkIdType, 4> M;
          for (int k = 0; k < 4; ++k)
          {
            V[k] = pts[faceCorners[k]];
            // The edges of a face with a split center are split too, the corner is only a
            // fallback for invalid inputs.
            M[k] = this->SplitSubPrimitives[faceEdges[k]] ? this->MidPoints[faceEdges[k]] : V[k];
          }
          const std::array<vtkIdType, 4> quarters[4] = {
            { V[0], M[0], C, M[3] },
            { M[0], V[1], M[1], C },
            { C, M[1], V[2], M[2] },
            { M[3], C, M[2], V[3] },
          };
          for (const auto& quarter : quarters)
          {
            std::copy(quarter.begin(), quarter.end(), facesConnPtr + facesOffsetsPtr[faceIdx++]);
          }
        }
        else
        {
          vtkIdType writeIdx = facesOffsetsPtr[faceIdx++];
          for (int k = 0; k < 4; ++k)
          {
            facesConnPtr[writeIdx++] = pts[faceCorners[k]];
            if (this->SplitSubPrimitives[faceEdges[k]])
            {
              facesConnPtr[writeIdx++] = this->MidPoints[faceEdges[k]];
            }
          }
        }
      }
    }
  }
  else if constexpr (Dim == 2)
  {
    const QuadSplitCases::QuadCase splitCase = this->GetQuadSplitCase(0);
    const std::array<vtkIdType, 8> localPts = this->GetQuadPoints(pts);

    const auto* indexPtr = splitCase.Indices.data();
    std::array<vtkIdType, 4> subPts{};

    for (vtkIdType j = 0; j < numCells; ++j)
    {
      const int ptsInSubCell = *indexPtr++;
      for (int p = 0; p < ptsInSubCell; ++p)
      {
        subPts[p] = localPts[*indexPtr++];
      }

      EmitSingleSubCell(j, GetPolygonType(ptsInSubCell), subPts.data(), ptsInSubCell, connPtr,
        connOffsetsPtr, typesPtr);
    }
  }
  else // Dim == 3
  {
    assert(this->HasAnySplitSubPrimitives());
    vtkIdType subCellIdx = 0;

    // Category A1, a single split face f, with center C and hanging points M[k] on its edges: 5
    // pyramids and 4 tetrahedra. The quarters of f are coned to the opposite corners O[k] of their
    // corners V[k], the opposite face to C, and a tetrahedron fills the space along each side face,
    // which is then the fan of 3 triangles from M[k]. It is not a single pulling: pulling C would
    // split the quarters of f into triangles, and pulling a corner would split the faces that
    // contain it by diagonals. It is valid because:
    // - the cells tile the hexahedron: they do not overlap, and for the unit cube their volumes are
    //   4 x 1/12 + 1/3 + 4 x 1/12 = 1;
    // - every face is split as by its neighbor: the quarters of f are the faces of the finer
    //   neighbors, the opposite face is whole, and each side face is the fan from M[k].
    // TestResolveHangingNodes3DUG checks both, with the cell validator for the orientations.
    const int splitFaceIdx = this->GetSingleSplitFaceIndex();
    if (splitFaceIdx >= 0)
    {
      const auto& F = FaceToHex[splitFaceIdx];
      const vtkIdType C = this->MidPoints[vtkHexahedron::NumberOfEdges + splitFaceIdx];
      std::array<vtkIdType, 4> V, M, O;
      for (int k = 0; k < 4; ++k)
      {
        V[k] = pts[F.P[k]];
        M[k] = this->MidPoints[F.E[k]];
        O[k] = pts[F.Opp[k]];
      }

      // The opposite face, coned to C: (O[0], O[1], O[2], O[3]) has the orientation of f, so its
      // normal already points toward C.
      EmitSingleSubCell<5>(subCellIdx++, VTK_PYRAMID, { O[0], O[1], O[2], O[3], C }, connPtr,
        connOffsetsPtr, typesPtr);

      // The quarters (V0, M0, C, M3), (M0, V1, M1, C), (C, M1, V2, M2) and (M3, C, M2, V3) of f,
      // reversed, coned to O[k]
      EmitSingleSubCell<5>(subCellIdx++, VTK_PYRAMID, { V[0], M[3], C, M[0], O[0] }, connPtr,
        connOffsetsPtr, typesPtr);
      EmitSingleSubCell<5>(subCellIdx++, VTK_PYRAMID, { M[0], C, M[1], V[1], O[1] }, connPtr,
        connOffsetsPtr, typesPtr);
      EmitSingleSubCell<5>(subCellIdx++, VTK_PYRAMID, { C, M[2], V[2], M[1], O[2] }, connPtr,
        connOffsetsPtr, typesPtr);
      EmitSingleSubCell<5>(subCellIdx++, VTK_PYRAMID, { M[3], V[3], M[2], C, O[3] }, connPtr,
        connOffsetsPtr, typesPtr);

      // The tetrahedra along the side faces
      for (int k = 0; k < 4; ++k)
      {
        EmitSingleSubCell<4>(subCellIdx++, VTK_TETRA, { M[k], O[(k + 1) % 4], O[k], C }, connPtr,
          connOffsetsPtr, typesPtr);
      }
      assert(subCellIdx == numCells);
      return;
    }

    // Category A2, a single split edge with hanging point M: pulling M, the 4 faces that do not
    // contain the edge (EdgeToHex) are coned to M, and the 2 faces that contain it are then the
    // fans of 3 triangles from M.
    const int splitEdgeIdx = this->GetSingleSplitEdgeIndex();
    if (splitEdgeIdx >= 0)
    {
      const vtkIdType mid = this->MidPoints[splitEdgeIdx];
      for (const int f : EdgeToHex[splitEdgeIdx])
      {
        const vtkIdType* facePts = vtkHexahedron::GetFaceArray(f);
        EmitSingleSubCell<5>(subCellIdx++, VTK_PYRAMID,
          { pts[facePts[0]], pts[facePts[3]], pts[facePts[2]], pts[facePts[1]], mid }, connPtr,
          connOffsetsPtr, typesPtr);
      }
      assert(subCellIdx == numCells);
      return;
    }

    // Category B: the faces are coned to the center point of the cell
    const vtkIdType apex = this->CentroidPoint;
    assert(apex >= 0);

    for (int f = 0; f < vtkHexahedron::NumberOfFaces; ++f)
    {
      const int faceSubPrimIdx = vtkHexahedron::NumberOfEdges + f;
      if (this->IsSubPrimitiveSplit(faceSubPrimIdx))
      {
        const vtkIdType* facePts = vtkHexahedron::GetFaceArray(f);
        const auto& faceEdges = FaceEdges[f];
        const vtkIdType C = this->MidPoints[faceSubPrimIdx];
        std::array<vtkIdType, 4> V;
        std::array<vtkIdType, 4> M;
        for (int k = 0; k < 4; ++k)
        {
          V[k] = pts[facePts[k]];
          // The edges of a face with a split center are split too, the corner is only a fallback
          // for invalid inputs.
          M[k] = this->SplitSubPrimitives[faceEdges[k]] ? this->MidPoints[faceEdges[k]] : V[k];
        }
        // The quarters of the face, reversed, as in Category A1
        EmitSingleSubCell<5>(subCellIdx++, VTK_PYRAMID, { V[0], M[3], C, M[0], apex }, connPtr,
          connOffsetsPtr, typesPtr);
        EmitSingleSubCell<5>(subCellIdx++, VTK_PYRAMID, { M[0], C, M[1], V[1], apex }, connPtr,
          connOffsetsPtr, typesPtr);
        EmitSingleSubCell<5>(subCellIdx++, VTK_PYRAMID, { C, M[2], V[2], M[1], apex }, connPtr,
          connOffsetsPtr, typesPtr);
        EmitSingleSubCell<5>(subCellIdx++, VTK_PYRAMID, { M[3], V[3], M[2], C, apex }, connPtr,
          connOffsetsPtr, typesPtr);
      }
      else
      {
        // The triangles and quads of QuadSplitCases, reversed
        const QuadSplitCases::QuadCase splitCase = this->GetQuadSplitCase(f);
        const std::array<vtkIdType, 8> facePts = this->GetQuadPoints(pts, f);
        const auto* indexPtr = splitCase.Indices.data();
        for (int k = 0; k < splitCase.NumCells; ++k)
        {
          const int numBasePts = *indexPtr++;
          std::array<vtkIdType, 4> base{};
          for (int p = 0; p < numBasePts; ++p)
          {
            base[p] = facePts[*indexPtr++];
          }
          if (numBasePts == 3)
          {
            EmitSingleSubCell<4>(subCellIdx++, VTK_TETRA, { base[0], base[2], base[1], apex },
              connPtr, connOffsetsPtr, typesPtr);
          }
          else
          {
            EmitSingleSubCell<5>(subCellIdx++, VTK_PYRAMID,
              { base[0], base[3], base[2], base[1], apex }, connPtr, connOffsetsPtr, typesPtr);
          }
        }
      }
    }
    assert(subCellIdx == numCells);
  }
}

//------------------------------------------------------------------------------
// The type of the output of an input of type DataSetT: a vtkPolyData of quads gives a vtkPolyData
// of polygons, the others a vtkUnstructuredGrid.
template <class DataSetT>
using ResolvedDataSetT =
  std::conditional_t<std::is_same_v<DataSetT, vtkPolyData>, vtkPolyData, vtkUnstructuredGrid>;

//------------------------------------------------------------------------------
// Add to the cell data of `output`, whose cells are those of its input in the same order, the
// cell array `name` of the ids of their input cells, i.e., their own ids, without memory.
void AddOriginalCellIds(vtkDataSet* output, const char* name)
{
  vtkNew<vtkAffineArray<vtkIdType>> originalCellIds;
  originalCellIds->ConstructBackend(1, 0);
  originalCellIds->SetNumberOfValues(output->GetNumberOfCells());
  originalCellIds->SetName(name);
  output->GetCellData()->AddArray(originalCellIds);
}

//------------------------------------------------------------------------------
// Set the cells and cell data of `output`: the cells of `input`, where every cell of
// `candidatesInfo` (the split candidate cells) is replaced, at the same place, by its
// decomposition if Decompose is true, otherwise by a polygon or polyhedron. Unless
// `originalCellIdsName` is null, the cell array of that name gets the input cell of every output
// cell.
template <int Dim, bool Decompose, class DataSetT>
void CommitResolvedCells(DataSetT* input, ResolvedDataSetT<DataSetT>* output,
  const std::vector<CandidateCellInfoT<Dim>>& candidatesInfo, const char* originalCellIdsName)
{
  const vtkIdType numInputCells = input->GetNumberOfCells();
  const vtkIdType numReplaced = static_cast<vtkIdType>(candidatesInfo.size());

  // Step 1: mark the replaced cells
  std::vector<char> replaced(numInputCells, 0);
  vtkSMPTools::For(0, numReplaced,
    [&](vtkIdType begin, vtkIdType end)
    {
      for (vtkIdType i = begin; i < end; ++i)
      {
        replaced[candidatesInfo[i].GetOriginalCellId()] = 1;
      }
    });

  // Step 2: the offsets of the output cells of every input cell
  vtkNew<vtkIdTypeArray> inputToOutputOffsets;
  inputToOutputOffsets->SetNumberOfValues(numInputCells + 1);
  vtkIdType numOutputCells = 0;

  // The other cells give a single output cell, and the candidate cells give
  // GetOutputCellCount() cells each.
  inputToOutputOffsets->FillValue(1);
  vtkSMPTools::For(0, numReplaced,
    [&](vtkIdType begin, vtkIdType end)
    {
      for (vtkIdType i = begin; i < end; ++i)
      {
        inputToOutputOffsets->SetValue(candidatesInfo[i].GetOriginalCellId(),
          candidatesInfo[i].template GetOutputCellCount<Decompose>());
      }
    });
  numOutputCells = vtkSMPTools::ExclusiveScan(inputToOutputOffsets->GetPointer(0),
    inputToOutputOffsets->GetPointer(numInputCells), static_cast<vtkIdType>(0));
  inputToOutputOffsets->SetValue(numInputCells, numOutputCells);
  auto inToOutOffsets = vtk::DataArrayValueRange<1, vtkIdType>(inputToOutputOffsets);

  // Step 3: the number of points of the output cells, and their number of faces (polyhedra)
  vtkNew<vtkIdTypeArray> outCellSizeOffsets;
  outCellSizeOffsets->SetNumberOfValues(numOutputCells + 1);

  vtkNew<vtkCellArray> faceLocations;
  vtkNew<vtkCellArray> faces;
  vtkNew<vtkIdTypeArray> faceLocsCellSizeOffsets;

  if constexpr (Dim == 3 && !Decompose)
  {
    // Allocate array for the number of faces per output cell (unmodified cells = 0 faces).
    faceLocsCellSizeOffsets->SetNumberOfValues(numOutputCells + 1);
    faceLocsCellSizeOffsets->FillValue(0);
  }

  // The other cells keep their NumPoints points
  vtkSMPTools::For(0, numInputCells,
    [&](vtkIdType begin, vtkIdType end)
    {
      for (vtkIdType c = begin; c < end; ++c)
      {
        if (!replaced[c])
        {
          outCellSizeOffsets->SetValue(inToOutOffsets[c], CandidateCellInfoT<Dim>::NumPoints);
        }
      }
    });

  // The replaced cells
  vtkSMPTools::For(0, numReplaced,
    [&](vtkIdType begin, vtkIdType end)
    {
      for (vtkIdType i = begin; i < end; ++i)
      {
        const auto& cellInfo = candidatesInfo[i];
        const vtkIdType c = cellInfo.GetOriginalCellId();
        const vtkIdType outCellStart = inToOutOffsets[c];
        const vtkIdType outCellCount = inToOutOffsets[c + 1] - outCellStart;
        cellInfo.template FillOutputCellSizes<Decompose>(
          outCellSizeOffsets->GetPointer(outCellStart), outCellCount);

        if constexpr (Dim == 3 && !Decompose)
        {
          // An unsplit face is 1 face, a split face its 4 quarters
          faceLocsCellSizeOffsets->SetValue(c, cellInfo.GetPolyhedronFaceCount());
        }
      }
    });

  // The connectivity offsets of the output cells
  const vtkIdType totalConn = vtkSMPTools::ExclusiveScan(outCellSizeOffsets->GetPointer(0),
    outCellSizeOffsets->GetPointer(numOutputCells), static_cast<vtkIdType>(0));
  outCellSizeOffsets->SetValue(numOutputCells, totalConn);

  // Step 4: the faces of the polyhedra (3D without Decompose only)
  vtkNew<vtkIdTypeArray> facesCellSizeOffsets;
  vtkIdType* facesConnPtr = nullptr;

  if constexpr (Dim == 3 && !Decompose)
  {
    // The offsets of the faces of the output cells
    const vtkIdType totalFaces = vtkSMPTools::ExclusiveScan(faceLocsCellSizeOffsets->GetPointer(0),
      faceLocsCellSizeOffsets->GetPointer(numOutputCells), static_cast<vtkIdType>(0));
    faceLocsCellSizeOffsets->SetValue(numOutputCells, totalFaces);

    facesCellSizeOffsets->SetNumberOfValues(totalFaces + 1);

    // The number of points of each face
    vtkSMPTools::For(0, numReplaced,
      [&](vtkIdType begin, vtkIdType end)
      {
        for (vtkIdType i = begin; i < end; ++i)
        {
          const auto& cellInfo = candidatesInfo[i];
          const vtkIdType c = cellInfo.GetOriginalCellId();
          const vtkIdType startFaceId = faceLocsCellSizeOffsets->GetValue(c);
          cellInfo.FillPolyhedronFaceSizes(facesCellSizeOffsets->GetPointer(startFaceId));
        }
      });

    // The connectivity offsets of the faces
    const vtkIdType totalFaceConn = vtkSMPTools::ExclusiveScan(facesCellSizeOffsets->GetPointer(0),
      facesCellSizeOffsets->GetPointer(totalFaces), static_cast<vtkIdType>(0));
    facesCellSizeOffsets->SetValue(totalFaces, totalFaceConn);

    // The faces of every polyhedron are consecutive, so the face ids are 0..totalFaces-1
    vtkNew<vtkIdTypeArray> faceLocsConn;
    faceLocsConn->SetNumberOfValues(totalFaces);

    vtkSMPTools::For(0, totalFaces, [&](vtkIdType begin, vtkIdType end)
      { std::iota(faceLocsConn->GetPointer(begin), faceLocsConn->GetPointer(end), begin); });

    // The connectivity of the faces, filled in Step 7
    vtkNew<vtkIdTypeArray> facesConn;
    facesConn->SetNumberOfValues(totalFaceConn);
    facesConnPtr = facesConn->GetPointer(0);

    faceLocations->SetData(faceLocsCellSizeOffsets, faceLocsConn);
    faces->SetData(facesCellSizeOffsets, facesConn);
  }

  // Step 5: allocate the connectivity and the cell types
  vtkNew<vtkIdTypeArray> connectivity;
  connectivity->SetNumberOfValues(totalConn);
  vtkIdType* connPtr = connectivity->GetPointer(0);

  vtkNew<vtkUnsignedCharArray> cellTypes;
  cellTypes->SetNumberOfValues(numOutputCells);
  unsigned char* typesPtr = cellTypes->GetPointer(0);

  // Step 6: copy the other cells
  vtkSMPThreadLocalObject<vtkIdList> tlCell;
  vtkSMPTools::For(0, numInputCells,
    [&](vtkIdType begin, vtkIdType end)
    {
      vtkIdList* tmpCell = tlCell.Local();
      vtkIdType npts;
      const vtkIdType* cell = nullptr;

      for (vtkIdType c = begin; c < end; ++c)
      {
        if (!replaced[c])
        {
          const vtkIdType outCellId = inToOutOffsets[c];
          const vtkIdType connWriteIdx = outCellSizeOffsets->GetValue(outCellId);

          GetCellPoints(input, c, npts, cell, tmpCell);
          assert(npts == CandidateCellInfoT<Dim>::NumPoints);
          std::copy_n(cell, CandidateCellInfoT<Dim>::NumPoints, connPtr + connWriteIdx);
          typesPtr[outCellId] = CandidateCellInfoT<Dim>::CellType;
        }
      }
    });
  std::vector<char>().swap(replaced); // No longer needed

  // Step 7: emit the replaced cells
  vtkSMPTools::For(0, numReplaced,
    [&](vtkIdType begin, vtkIdType end)
    {
      vtkIdList* tmpCell = tlCell.Local();
      vtkIdType npts;
      const vtkIdType* cell = nullptr;

      for (vtkIdType i = begin; i < end; ++i)
      {
        const auto& cellInfo = candidatesInfo[i];
        const vtkIdType c = cellInfo.GetOriginalCellId();
        const vtkIdType outCellStart = inToOutOffsets[c];

        GetCellPoints(input, c, npts, cell, tmpCell);
        assert(npts == CandidateCellInfoT<Dim>::NumPoints);

        // The faces of the polyhedron (3D without Decompose only)
        const vtkIdType* facesOffsetsPtr = nullptr;
        if constexpr (Dim == 3 && !Decompose)
        {
          facesOffsetsPtr = facesCellSizeOffsets->GetPointer(faceLocsCellSizeOffsets->GetValue(c));
        }

        const vtkIdType outCellCount = inToOutOffsets[c + 1] - outCellStart;
        cellInfo.template EmitOutputCells<Decompose>(cell, outCellCount, connPtr,
          outCellSizeOffsets->GetPointer(outCellStart), typesPtr + outCellStart, facesConnPtr,
          facesOffsetsPtr);
      }
    });

  // Step 8: set the cells of the output. A vtkPolyData has only polygons, which do not need their
  // types.
  vtkNew<vtkCellArray> newCells;
  newCells->SetData(outCellSizeOffsets, connectivity);
  if constexpr (std::is_same_v<DataSetT, vtkPolyData>)
  {
    output->SetPolys(newCells);
  }
  else if constexpr (Dim == 3 && !Decompose)
  {
    vtkNew<vtkPolyhedronCellArray> polyhedra;
    polyhedra->SetData(faces, faceLocations);
    output->SetPolyhedralCells(cellTypes, newCells, polyhedra);
  }
  else
  {
    output->SetPolyhedralCells(cellTypes, newCells, nullptr);
  }

  // Step 9: the cell data. Without Decompose, the cells keep their ids, so the cell data of the
  // input is passed. Otherwise, every output cell copies the cell data of its input cell, all at
  // once.
  vtkCellData* inCD = input->GetCellData();
  vtkCellData* outCD = output->GetCellData();
  if constexpr (!Decompose)
  {
    outCD->PassData(inCD);
    if (originalCellIdsName)
    {
      AddOriginalCellIds(output, originalCellIdsName);
    }
  }
  else
  {
    // The input cell of every output cell
    vtkNew<vtkIdList> inputCellIds;
    inputCellIds->SetNumberOfIds(numOutputCells);
    vtkIdType* inputCellIdsPtr = inputCellIds->GetPointer(0);
    vtkSMPTools::For(0, numInputCells,
      [&](vtkIdType begin, vtkIdType end)
      {
        for (vtkIdType c = begin; c < end; ++c)
        {
          std::fill(
            inputCellIdsPtr + inToOutOffsets[c], inputCellIdsPtr + inToOutOffsets[c + 1], c);
        }
      });
    inputToOutputOffsets->Initialize(); // No longer needed
    outCD->CopyAllocate(inCD, numOutputCells);
    outCD->CopyData(inCD, inputCellIds, static_cast<vtkIdType>(0));
    if (originalCellIdsName)
    {
      // The input cells, without copy
      vtkNew<vtkIdTypeArray> originalCellIds;
      originalCellIds->SetName(originalCellIdsName);
      originalCellIds->SetArray(
        inputCellIds->Release(), numOutputCells, 0, vtkIdTypeArray::VTK_DATA_ARRAY_DELETE);
      outCD->AddArray(originalCellIds);
    }
  }
}

//------------------------------------------------------------------------------
// Set the points and point data of `output`: the input points, then the remote points
// `usedRemoteIds`, then the center points of the Category B cells `centroidCellIds`, at the
// average of the points of their cell (corners and hanging points), with the average point data
// for the arrays that are interpolated and null values for the others, and with point global ids,
// the global ids from `firstCentroidGlobalId`.
template <int Dim, class DataSetT>
void CommitResolvedPoints(DataSetT* input, vtkPointSet* remotePoints, vtkIdList* usedRemoteIds,
  vtkIdList* centroidCellIds, const std::vector<CandidateCellInfoT<Dim>>& candidatesInfo,
  vtkIdType firstCentroidGlobalId, vtkPointSet* output)
{
  const vtkIdType numLocalPoints = input->GetNumberOfPoints();
  const vtkIdType numRemotePoints = usedRemoteIds->GetNumberOfIds();
  const vtkIdType numCentroidPoints = centroidCellIds->GetNumberOfIds();
  const vtkIdType totalPoints = numLocalPoints + numRemotePoints + numCentroidPoints;

  // Without remote or center points, the points are the input points
  if (numLocalPoints == totalPoints)
  {
    output->SetPoints(input->GetPoints());
    output->GetPointData()->PassData(input->GetPointData());
    return;
  }

  vtkPoints* inputPoints = input->GetPoints();
  vtkNew<vtkPoints> resolvedPoints;
  resolvedPoints->SetDataType(inputPoints->GetDataType());
  resolvedPoints->SetNumberOfPoints(totalPoints);

  vtkPointData* inPD = input->GetPointData();
  vtkPointData* outPD = output->GetPointData();
  // The remote points keep the global ids of the points they copy, and the center points get new
  // ones. The output has the arrays of the input in the same order, since the center points are
  // interpolated from it (see Pass 3).
  outPD->CopyGlobalIdsOn();
  outPD->CopyAllocate(inPD, totalPoints);
  outPD->SetNumberOfTuples(totalPoints);

  // Pass 1: the input points
  if (numLocalPoints > 0 && inputPoints)
  {
    resolvedPoints->InsertPoints(0, numLocalPoints, 0, inputPoints);
    outPD->CopyData(inPD, 0, numLocalPoints, 0);
  }

  // Pass 2: the remote points used as hanging points
  if (numRemotePoints > 0 && remotePoints)
  {
    vtkPoints* remotePts = remotePoints->GetPoints();
    vtkPointData* remotePD = remotePoints->GetPointData();

    resolvedPoints->GetData()->InsertTuplesStartingAt(
      numLocalPoints, usedRemoteIds, remotePts->GetData());
    outPD->CopyData(remotePD, usedRemoteIds, numLocalPoints);
  }

  // Pass 3: the center points
  if (numCentroidPoints > 0 && inputPoints)
  {
    // The new global ids of the center points, see ResolvePartitions
    vtkDataArray* globalIds = outPD->GetGlobalIds();
    assert(!globalIds || firstCentroidGlobalId >= 0);

    const vtkIdType centroidOffset = numLocalPoints + numRemotePoints;

    // The point data of the center points is interpolated in `centroidPD`, which has the arrays
    // that InterpolateAllocate would select, as in other filters, e.g., not the pedigree ids. The
    // output is the source: it has the arrays of the input, in the same order.
    vtkDataSetAttributesFieldList fieldList;
    fieldList.InitializeFieldList(inPD);
    vtkNew<vtkPointData> centroidPD;
    fieldList.CopyAllocate(centroidPD, vtkDataSetAttributes::INTERPOLATE, numCentroidPoints, 0);
    centroidPD->SetNumberOfTuples(numCentroidPoints);

    vtkSMPThreadLocalObject<vtkIdList> tlCellPts;
    vtkSMPThreadLocalObject<vtkIdList> tlAllCellPointIds;
    vtkSMPThreadLocal<std::vector<double>> tlWeights;

    vtkSMPTools::For(0, numCentroidPoints,
      [&](vtkIdType begin, vtkIdType end)
      {
        vtkIdType npts;
        const vtkIdType* cell = nullptr;
        vtkIdList* tempCell = tlCellPts.Local();
        vtkIdList* allCellPointIds = tlAllCellPointIds.Local();
        std::vector<double>& weights = tlWeights.Local();

        for (vtkIdType i = begin; i < end; ++i)
        {
          const vtkIdType candidateCellId = centroidCellIds->GetId(i);
          const auto& candidateCell = candidatesInfo[candidateCellId];
          const vtkIdType outputPointId = centroidOffset + i;

          GetCellPoints(input, candidateCell.GetOriginalCellId(), npts, cell, tempCell);
          assert(npts == CandidateCellInfoT<Dim>::NumPoints);

          const vtkIdType numMidPoints = candidateCell.GetSplitSubPrimitives().count();
          const vtkIdType numAllPoints = npts + numMidPoints;
          allCellPointIds->SetNumberOfIds(numAllPoints);
          weights.assign(numAllPoints, 1.0 / static_cast<double>(numAllPoints));

          // The corners
          for (vtkIdType j = 0; j < npts; ++j)
          {
            allCellPointIds->SetId(j, cell[j]);
          }

          // The hanging points
          vtkIdType midIdx = 0;
          for (int s = 0; s < CandidateCellInfoT<Dim>::NumSubPrimitives; ++s)
          {
            if (candidateCell.IsSubPrimitiveSplit(s))
            {
              allCellPointIds->SetId(npts + midIdx, candidateCell.GetMidPoint(s));
              ++midIdx;
            }
          }

          vtkVector3d centroid(0.0, 0.0, 0.0);
          for (vtkIdType j = 0; j < numAllPoints; ++j)
          {
            vtkVector3d p;
            resolvedPoints->GetPoint(allCellPointIds->GetId(j), p.GetData());
            centroid = centroid + p;
          }
          centroid = centroid * (1.0 / static_cast<double>(numAllPoints));
          resolvedPoints->SetPoint(outputPointId, centroid.GetData());

          fieldList.InterpolatePoint(0, outPD, allCellPointIds, weights.data(), centroidPD, i);
          if (globalIds)
          {
            globalIds->SetComponent(
              outputPointId, 0, static_cast<double>(firstCentroidGlobalId + i));
          }
        }
      });

    // The arrays that are interpolated get the tuples of `centroidPD`, and the other numeric arrays
    // null values, but the global ids, which the center points get new values of.
    std::vector<vtkAbstractArray*> interpolated;
    fieldList.TransformData(0, outPD, centroidPD,
      [&](vtkAbstractArray* outArray, vtkAbstractArray* centroidArray)
      {
        outArray->InsertTuples(centroidOffset, numCentroidPoints, 0, centroidArray);
        interpolated.push_back(outArray);
      });
    for (int a = 0; a < outPD->GetNumberOfArrays(); ++a)
    {
      vtkDataArray* array = outPD->GetArray(a);
      if (!array || array == globalIds ||
        std::find(interpolated.begin(), interpolated.end(), array) != interpolated.end())
      {
        continue;
      }
      const std::vector<double> nullTuple(array->GetNumberOfComponents(), 0.0);
      vtkSMPTools::For(centroidOffset, centroidOffset + numCentroidPoints,
        [&](vtkIdType begin, vtkIdType end)
        {
          for (vtkIdType pointId = begin; pointId < end; ++pointId)
          {
            array->SetTuple(pointId, nullTuple.data());
          }
        });
    }
  }

  output->SetPoints(resolvedPoints);
}

//------------------------------------------------------------------------------
// Set `output` to `input` unchanged, sharing its points, cells, point, cell and field data. The
// cells of a vtkCartesianGrid become quads or hexahedra in the same order, see GetCellPoints.
// vtkExtractCells is not used for a vtkCartesianGrid: it keeps pixels and voxels, while the
// resolved outputs have quads or hexahedra, it copies the points instead of sharing them, and it
// writes explicit cell types and offsets, where a constant type and a fixed cell size suffice.
template <int Dim, class DataSetT>
void CopyInput(DataSetT* input, ResolvedDataSetT<DataSetT>* output)
{
  if constexpr (!std::is_base_of_v<vtkCartesianGrid, DataSetT>)
  {
    output->ShallowCopy(input);
  }
  else
  {
    output->Initialize();
    output->SetPoints(input->GetPoints());
    output->GetPointData()->ShallowCopy(input->GetPointData());
    output->GetCellData()->ShallowCopy(input->GetCellData());
    output->GetFieldData()->ShallowCopy(input->GetFieldData());
    const vtkIdType numCells = input->GetNumberOfCells();
    if (numCells == 0)
    {
      return;
    }

    constexpr int cellSize = CandidateCellInfoT<Dim>::NumPoints;
    vtkNew<vtkIdTypeArray> connectivity;
    connectivity->SetNumberOfValues(numCells * cellSize);
    vtkIdType* outPoints = connectivity->GetPointer(0);
    vtkSMPThreadLocalObject<vtkIdList> tlCell;
    vtkSMPTools::For(0, numCells,
      [&](vtkIdType begin, vtkIdType end)
      {
        vtkIdList* tmpCell = tlCell.Local();
        vtkIdType npts;
        const vtkIdType* cell = nullptr;
        for (vtkIdType cellId = begin; cellId < end; ++cellId)
        {
          GetCellPoints(input, cellId, npts, cell, tmpCell);
          assert(npts >= 3);
          std::copy_n(cell, cellSize, outPoints + cellId * cellSize);
        }
      });
    vtkNew<vtkCellArray> cells;
    cells->SetData(cellSize, connectivity);
    output->SetCells(CandidateCellInfoT<Dim>::CellType, cells);
  }
}

//------------------------------------------------------------------------------
// Build the output from the candidate cells and the points inserted in them, see the steps below.
// `candidatesInfo` is released once the split candidate cells are copied. Unless
// `originalCellIdsName` is null, the cell array of that name gets the input cell of every output
// cell. With point global ids, the center points get the global ids from `firstCentroidGlobalId`.
template <int Dim, class DataSetT>
void CommitResolution(DataSetT* input, vtkPointSet* remotePoints,
  std::vector<CandidateCellInfoT<Dim>>& candidatesInfo, ResolvedDataSetT<DataSetT>* output,
  bool decompose, bool computeInverseMeasure, const char* originalCellIdsName,
  vtkIdType firstCentroidGlobalId)
{
  if (!input || !output)
  {
    return;
  }

  // Step 1: the split candidate cells. Only they are replaced, the others would give their own
  // cell back.
  const vtkIdType numCandidates = static_cast<vtkIdType>(candidatesInfo.size());
  std::vector<unsigned char> isSplit(numCandidates);
  vtkSMPTools::For(0, numCandidates,
    [&](vtkIdType begin, vtkIdType end)
    {
      for (vtkIdType i = begin; i < end; ++i)
      {
        isSplit[i] = candidatesInfo[i].HasAnySplitSubPrimitives() ? 1 : 0;
      }
    });
  vtkNew<vtkIdList> splitIds;
  const vtkIdType numSplit = SelectIds(isSplit.data(), numCandidates, splitIds);
  std::vector<unsigned char>().swap(isSplit); // No longer needed

  // Their infos, in the order of the candidate cells
  std::vector<CandidateCellInfoT<Dim>> splitCandidatesInfo(numSplit);
  vtkSMPTools::For(0, numSplit,
    [&](vtkIdType begin, vtkIdType end)
    {
      for (vtkIdType k = begin; k < end; ++k)
      {
        splitCandidatesInfo[k] = candidatesInfo[splitIds->GetId(k)];
      }
    });
  std::vector<CandidateCellInfoT<Dim>>().swap(candidatesInfo); // No longer needed
  splitIds->Initialize();                                      // No longer needed

  // No cell is split: the output is the input
  if (numSplit == 0)
  {
    CopyInput<Dim>(input, output);
    if (computeInverseMeasure)
    {
      AddInverseMeasures<Dim>(output);
    }
    if (originalCellIdsName)
    {
      AddOriginalCellIds(output, originalCellIdsName);
    }
    return;
  }

  // The output is built from the input: its field data, then its cells and points, whose data is
  // copied into arrays of the output, not shared with the input. Shared arrays would be resized in
  // place, and kept if they are not copied, e.g., the global ids.
  output->GetFieldData()->PassData(input->GetFieldData());

  // Step 2: the output ids of the remote hanging points
  const vtkIdType numLocalPoints = input->GetNumberOfPoints();
  vtkSmartPointer<vtkIdList> usedRemoteIds =
    CommitRemotePointIds<Dim>(remotePoints, numLocalPoints, splitCandidatesInfo);
  const vtkIdType numRemotePoints = usedRemoteIds->GetNumberOfIds();

  // Step 3: the cells
  // With Decompose, the center points of the Category B cells get their ids first.
  vtkSmartPointer<vtkIdList> centroidCellIds;
  if (decompose)
  {
    centroidCellIds =
      CommitCentroidPointIds<Dim>(numLocalPoints + numRemotePoints, splitCandidatesInfo);
    CommitResolvedCells<Dim, true>(input, output, splitCandidatesInfo, originalCellIdsName);
  }
  else
  {
    centroidCellIds = vtkSmartPointer<vtkIdList>::New();
    CommitResolvedCells<Dim, false>(input, output, splitCandidatesInfo, originalCellIdsName);
  }

  // Step 4: the points and point data
  CommitResolvedPoints<Dim>(input, remotePoints, usedRemoteIds, centroidCellIds,
    splitCandidatesInfo, firstCentroidGlobalId, output);
  std::vector<CandidateCellInfoT<Dim>>().swap(splitCandidatesInfo); // No longer needed
  usedRemoteIds = nullptr;                                          // No longer needed
  centroidCellIds = nullptr;                                        // No longer needed

  // Step 5: the inverse measures
  if (computeInverseMeasure)
  {
    AddInverseMeasures<Dim>(output);
  }
}

//------------------------------------------------------------------------------
// A partition of the data, a partition of a vtkPartitionedDataSet, and its state between the phases
// of vtkResolveHangingNodes::ResolvePartitions, which resolves all the partitions together.
template <int Dim>
struct Partition
{
  // The input, a vtkPolyData, a vtkUnstructuredGrid or a vtkCartesianGrid, whose cells are read as
  // quads or hexahedra, see GetCellPoints.
  vtkDataSet* Input = nullptr;
  // The output, of type ResolvedDataSetT of the input.
  vtkPointSet* Output = nullptr;
  // Whether the output is set, or the resolution failed: the next phases skip the partition.
  bool IsDone = false;
  // The candidate cells, and in 3D, their edges to search, see FindCandidateCells.
  std::vector<CandidateCellInfoT<Dim>> CandidatesInfo;
  EdgesToSearch Edges;
  // The measures of the candidate cells, checked once their range over all the partitions is
  // known.
  vtkSmartPointer<vtkDoubleArray> CandidateMeasures;
  // The true boundary, for GetRemotePoints, once the local points are inserted.
  vtkSmartPointer<vtkPolyData> TrueBoundary;
  // The points of the other partitions that may lie on the true boundary, see GetRemotePoints, and
  // once inserted, those whose point data match the one of the input.
  vtkSmartPointer<vtkPolyData> RemotePoints;
  // The global id of the first center point of the partition, -1 without point global ids or
  // center points, see ResolvePartitions.
  vtkIdType FirstCentroidGlobalId = -1;
};

//------------------------------------------------------------------------------
// Implicit array backend of the original cell ids of the candidate cells `CandidatesInfo`, without
// allocation.
template <typename CandidateInfoT>
struct CandidateCellOriginalIdBackend
{
  const std::vector<CandidateInfoT>& CandidatesInfo;

  explicit CandidateCellOriginalIdBackend(const std::vector<CandidateInfoT>& infos)
    : CandidatesInfo(infos)
  {
  }
  vtkIdType operator()(vtkIdType idx) const
  {
    return this->CandidatesInfo[idx].GetOriginalCellId();
  }
};

//------------------------------------------------------------------------------
// Steps 1 and 2 of vtkResolveHangingNodes::ResolvePartitions: the candidate cells of `partition`,
// whose input is `input`, in 3D their edges to search, and their measures. Without cells, the
// output is the input, and the partition is done.
template <int Dim, class DataSetT>
void FindPartitionCandidates(DataSetT* input, Partition<Dim>& partition)
{
  if (input->GetNumberOfCells() == 0 || input->GetPoints() == nullptr)
  {
    CopyInput<Dim>(input, ResolvedDataSetT<DataSetT>::SafeDownCast(partition.Output));
    partition.IsDone = true;
    return;
  }

  // Step 1: find the candidate cells, and in 3D, the edges they search
  std::tie(partition.CandidatesInfo, partition.Edges) = FindCandidateCells<Dim>(input);

  // Step 2: the measures of the candidate cells
  // The ids of the candidate cells, without copy
  using CandidateCellIdsBackend = CandidateCellOriginalIdBackend<CandidateCellInfoT<Dim>>;
  vtkNew<vtkImplicitArray<CandidateCellIdsBackend>> candidateCells;
  candidateCells->ConstructBackend(partition.CandidatesInfo);
  candidateCells->SetNumberOfValues(static_cast<vtkIdType>(partition.CandidatesInfo.size()));
  partition.CandidateMeasures = ComputeMeasures<Dim>(input, candidateCells.GetPointer());
}

//------------------------------------------------------------------------------
// Add to `output`, the output of a partition without cells, the empty cell arrays that the outputs
// of the other partitions get: the inverse measures, named after `dimension`, the dimension of the
// cells of all the processes, 0 if none has cells, and unless `originalCellIdsName` is null, the
// input cells.
void AddEmptyCellArrays(
  vtkPointSet* output, int dimension, bool computeInverseMeasure, const char* originalCellIdsName)
{
  if (dimension == 0 || output->GetNumberOfCells() > 0)
  {
    return;
  }
  if (computeInverseMeasure)
  {
    vtkNew<vtkDoubleArray> inverseMeasures;
    inverseMeasures->SetName(dimension == 3 ? CandidateCellInfoT<3>::InverseMeasureName
                                            : CandidateCellInfoT<2>::InverseMeasureName);
    output->GetCellData()->AddArray(inverseMeasures);
  }
  if (originalCellIdsName)
  {
    AddOriginalCellIds(output, originalCellIdsName);
  }
}

//------------------------------------------------------------------------------
// Steps 3 to 6 of vtkResolveHangingNodes::ResolvePartitions: the local resolution of `partition`,
// whose input is `input`, unless it is a vtkCartesianGrid, which is conforming, and if `exchange`,
// i.e., if the partitions exchange the points of their true boundary (see ResolvePartitions), its
// true boundary: the sides to search that local points do not fully split, for GetRemotePoints.
// Returns false, reported by `self`, if the input is not a valid mesh: the partition is then done.
template <int Dim, class DataSetT>
bool ResolvePartitionLocally(vtkResolveHangingNodes* self, DataSetT* input,
  Partition<Dim>& partition, double tolerance, bool exchange)
{
  std::vector<CandidateCellInfoT<Dim>>& candidatesInfo = partition.CandidatesInfo;
  if constexpr (!std::is_base_of_v<vtkCartesianGrid, DataSetT>)
  {
    // Step 3: the points of the sides to search, which include all the hanging points of the
    // local hanging nodes
    vtkNew<vtkIdTypeArray> potentialLocalBoundaryPointMapping;
    vtkSmartPointer<vtkPolyData> potentialLocalBoundaryPoints =
      ExtractBoundary<Dim>(input, candidatesInfo, potentialLocalBoundaryPointMapping.GetPointer(),
        /*onlyPoints*/ true);

    if (!candidatesInfo.empty() &&
      (!potentialLocalBoundaryPoints || potentialLocalBoundaryPoints->GetNumberOfPoints() == 0))
    {
      vtkErrorWithObjectMacro(self,
        "No potential local boundary points were found, but there are candidate cells. "
        "The input is probably not a valid mesh.");
      partition.IsDone = true;
      std::vector<CandidateCellInfoT<Dim>>().swap(candidatesInfo); // No longer needed
      partition.Edges = EdgesToSearch();                           // No longer needed
      return false;
    }

    // Step 4: insert these points in the candidate cells
    if (potentialLocalBoundaryPoints)
    {
      vtkNew<vtkStaticPointLocator> locator;
      locator->SetDataSet(potentialLocalBoundaryPoints);
      locator->BuildLocator();
      FindPointsOnSubPrimitives<Dim>(input, locator, potentialLocalBoundaryPoints->GetPoints(), 0,
        tolerance, candidatesInfo, potentialLocalBoundaryPointMapping.GetPointer(),
        partition.Edges.Locator);
    }
  }
  partition.Edges = EdgesToSearch(); // No longer needed

  if (exchange)
  {
    // Step 5: restrict the sides to search to the true boundary, the other unmatched sides
    // being resolved locally, and in 3D, the edges to search to those of the true boundary
    RestrictCandidateCells<Dim>(input, candidatesInfo, partition.Edges);

    // Step 6: the true boundary, with its sides, for GetRemotePoints
    partition.TrueBoundary =
      ExtractBoundary<Dim>(input, candidatesInfo, nullptr, /*onlyPoints*/ false);
  }
  return true;
}

//------------------------------------------------------------------------------
// Step 8 of vtkResolveHangingNodes::ResolvePartitions: the remote points of `partition`, whose
// input is `input`, inserted in its candidate cells. The remote points whose point data differ from
// the one of the input are ignored, with a warning reported by `self`.
template <int Dim, class DataSetT>
void InsertRemotePoints(
  vtkResolveHangingNodes* self, DataSetT* input, Partition<Dim>& partition, double tolerance)
{
  // The remote points, unless there is no candidate cell to insert them in, or their point data
  // differ from the one of the partition.
  vtkSmartPointer<vtkPolyData> remotePoints = partition.RemotePoints;
  partition.RemotePoints = nullptr;
  if (!remotePoints || !remotePoints->GetPoints() || remotePoints->GetNumberOfPoints() == 0 ||
    partition.CandidatesInfo.empty())
  {
    // No remote point to insert
    remotePoints = nullptr;
  }
  else if (!HaveSameDataArrays(input->GetPointData(), remotePoints->GetPointData()))
  {
    vtkWarningWithObjectMacro(self,
      "The point data of the remote points does not have the same arrays as the point data of the "
      "input. The remote points are ignored.");
    // No remote point to insert
    remotePoints = nullptr;
  }

  if (remotePoints)
  {
    // The remote points are searched by their own ids.
    vtkNew<vtkAffineArray<vtkIdType>> allRemotePoints;
    allRemotePoints->ConstructBackend(1, 0);
    allRemotePoints->SetNumberOfValues(remotePoints->GetNumberOfPoints());
    vtkNew<vtkStaticPointLocator> locator;
    locator->SetDataSet(remotePoints);
    locator->BuildLocator();
    FindPointsOnSubPrimitives<Dim>(input, locator, remotePoints->GetPoints(),
      input->GetNumberOfPoints(), tolerance, partition.CandidatesInfo, allRemotePoints.GetPointer(),
      partition.Edges.Locator);
  }
  partition.Edges = EdgesToSearch(); // No longer needed

  partition.RemotePoints = remotePoints;
}

//------------------------------------------------------------------------------
// The number of center points of the candidate cells `candidatesInfo` with Decompose: one
// per Category B cell, see CommitCentroidPointIds.
template <int Dim>
vtkIdType CountCentroidPoints(const std::vector<CandidateCellInfoT<Dim>>& candidatesInfo)
{
  return static_cast<vtkIdType>(std::count_if(candidatesInfo.begin(), candidatesInfo.end(),
    [](const CandidateCellInfoT<Dim>& info) { return info.HasCentroidPoint(); }));
}

//------------------------------------------------------------------------------
// Step 10 of vtkResolveHangingNodes::ResolvePartitions: the output of `partition`, whose input
// is `input`, once its remote points are inserted. Unless `originalCellIdsName` is null, the cell
// array of that name gets the input cell of every output cell.
template <int Dim, class DataSetT>
void CommitPartition(DataSetT* input, Partition<Dim>& partition, bool decompose,
  bool computeInverseMeasure, const char* originalCellIdsName)
{
  vtkSmartPointer<vtkPolyData> remotePoints = partition.RemotePoints;
  partition.RemotePoints = nullptr;
  CommitResolution<Dim>(input, remotePoints, partition.CandidatesInfo,
    ResolvedDataSetT<DataSetT>::SafeDownCast(partition.Output), decompose, computeInverseMeasure,
    originalCellIdsName, partition.FirstCentroidGlobalId);
}

} // namespace

//------------------------------------------------------------------------------
template <int Dim>
int vtkResolveHangingNodes::ResolvePartitions(
  const std::vector<vtkSmartPointer<vtkDataSet>>& inputs,
  const std::vector<vtkSmartPointer<vtkPointSet>>& outputs,
  const std::vector<int>& partitionedDataSetIds)
{
  // All the processes must take part in the parallel calls (GetGlobalMeasureRange,
  // GetRemotePoints), even those without cells: each is made once, for all the partitions, and
  // GetRemotePoints is skipped by all of them only when no process has candidate cells.
  const bool useRemotePoints = this->UseRemotePoints();
  int result = 1;
  std::vector<Partition<Dim>> partitions(inputs.size());

  // ===========================================================================
  // Phase 1: the candidate cells of every partition, and the range of their measures
  // ===========================================================================
  double measureRange[2] = { VTK_DOUBLE_MAX, VTK_DOUBLE_MIN };
  // The dimension of the cells, 0 without cells, see GetGlobalMeasureRange
  int dimension = 0;
  std::size_t numPartitionsWithCells = 0;
  for (std::size_t p = 0; p < partitions.size(); ++p)
  {
    Partition<Dim>& partition = partitions[p];
    partition.Input = inputs[p];
    partition.Output = outputs[p];

    // Steps 1 and 2: the candidate cells, and the range of their measures
    if (auto polyData = vtkPolyData::SafeDownCast(partition.Input))
    {
      FindPartitionCandidates<Dim>(polyData, partition);
    }
    else if (auto cartesianGrid = vtkCartesianGrid::SafeDownCast(partition.Input))
    {
      FindPartitionCandidates<Dim>(cartesianGrid, partition);
    }
    else
    {
      FindPartitionCandidates<Dim>(vtkUnstructuredGrid::SafeDownCast(partition.Input), partition);
    }
    if (partition.IsDone)
    {
      continue;
    }
    dimension = Dim;
    ++numPartitionsWithCells;
    if (!partition.CandidatesInfo.empty())
    {
      double range[2];
      partition.CandidateMeasures->GetRange(range);
      measureRange[0] = std::min(measureRange[0], range[0]);
      measureRange[1] = std::max(measureRange[1], range[1]);
    }
  }

  // The range over all the processes
  this->GetGlobalMeasureRange(measureRange, dimension);
  // The largest number of levels of the vtkPartitionedDataSet of a collection, see RequestData
  this->NumberOfLevels =
    std::max(this->NumberOfLevels, ComputeNumberOfLevels(dimension, measureRange));

  // The partitions without cells are done, their outputs are their inputs, with the cell arrays of
  // the outputs of the others, which only `dimension`, reduced over all the processes, names
  // consistently: a process without cells resolves its partitions as 2D ones.
  const char* originalCellIdsName =
    this->PassThroughCellIds ? this->OriginalCellIdsName.c_str() : nullptr;
  for (Partition<Dim>& partition : partitions)
  {
    if (partition.IsDone)
    {
      AddEmptyCellArrays(
        partition.Output, dimension, this->ComputeInverseMeasure, originalCellIdsName);
    }
  }
  bool measuresMatchLevels = true;
  for (Partition<Dim>& partition : partitions)
  {
    if (measuresMatchLevels && !partition.IsDone && !partition.CandidatesInfo.empty() &&
      measureRange[0] > 0.0 &&
      !CellMeasuresMatchLevels<Dim>(partition.CandidateMeasures, measureRange[0]))
    {
      measuresMatchLevels = false;
    }
    partition.CandidateMeasures = nullptr; // No longer needed
  }
  if (!measuresMatchLevels)
  {
    vtkWarningMacro("Some cell measures are not the smallest measure times a power of "
      << (Dim == 2 ? "4" : "8")
      << ", i.e. the cell sizes are not consistent with a refinement ratio of 2 in each "
         "direction. The number of levels may be wrong.");
  }

  // Whether some process has candidate cells. measureRange is global, so all the processes agree.
  // Otherwise, the phases 2 and 3 are skipped, and the outputs are the inputs (phase 4).
  const bool hasCandidates = measureRange[0] != VTK_DOUBLE_MAX;
  // Whether the partitions exchange the points of their true boundary (phase 3): the hanging nodes
  // between partitions are only resolved with the points of the other partitions. It needs
  // candidate cells, and several partitions, here (numPartitionsWithCells > 1) or on other
  // processes (useRemotePoints). Without it, a single partition is fully resolved by its own
  // points (steps 3 and 4), so its true boundary is not restricted nor extracted (steps 5 and 6).
  // GetRemotePoints is a parallel call, so all the processes must agree: with several processes,
  // useRemotePoints is true on all of them, and numPartitionsWithCells, which is local, does not
  // matter.
  const bool exchange = hasCandidates && (useRemotePoints || numPartitionsWithCells > 1);

  // ===========================================================================
  // Phase 2: the local resolution of every partition, and its true boundary
  // ===========================================================================
  for (Partition<Dim>& partition : partitions)
  {
    if (partition.IsDone || !hasCandidates)
    {
      continue;
    }
    // Steps 3 to 6. A failed partition is done, but the parallel calls are still made below, for
    // the other partitions and processes.
    bool resolved;
    if (auto polyData = vtkPolyData::SafeDownCast(partition.Input))
    {
      resolved = ResolvePartitionLocally<Dim>(this, polyData, partition, this->Tolerance, exchange);
    }
    else if (auto cartesianGrid = vtkCartesianGrid::SafeDownCast(partition.Input))
    {
      resolved =
        ResolvePartitionLocally<Dim>(this, cartesianGrid, partition, this->Tolerance, exchange);
    }
    else
    {
      resolved = ResolvePartitionLocally<Dim>(this,
        vtkUnstructuredGrid::SafeDownCast(partition.Input), partition, this->Tolerance, exchange);
    }
    if (!resolved)
    {
      result = 0;
    }
  }

  // ===========================================================================
  // Phase 3: the points of the other partitions, here or on other processes, that may lie on the
  // true boundary of every partition
  // ===========================================================================
  if (exchange)
  {
    // Step 7: the remote points, from all the other partitions, or with a partitionedDataSetIds
    // that is not -1, only from the partitions of the other vtkPartitionedDataSet, see
    // GetRemotePoints
    std::vector<vtkPolyData*> boundaries(partitions.size());
    for (std::size_t p = 0; p < partitions.size(); ++p)
    {
      boundaries[p] = partitions[p].TrueBoundary;
    }
    std::vector<vtkSmartPointer<vtkPolyData>> remotePoints =
      this->GetRemotePoints(boundaries, partitionedDataSetIds);
    for (std::size_t p = 0; p < partitions.size() && p < remotePoints.size(); ++p)
    {
      partitions[p].RemotePoints = remotePoints[p];
    }
    for (Partition<Dim>& partition : partitions)
    {
      partition.TrueBoundary = nullptr; // No longer needed
    }
  }

  // ===========================================================================
  // Phase 4: the remote points inserted in every partition, the global ids of the center points,
  // and the outputs
  // ===========================================================================
  for (Partition<Dim>& partition : partitions)
  {
    if (partition.IsDone)
    {
      continue;
    }
    // Step 8
    if (auto polyData = vtkPolyData::SafeDownCast(partition.Input))
    {
      InsertRemotePoints<Dim>(this, polyData, partition, this->Tolerance);
    }
    else if (auto cartesianGrid = vtkCartesianGrid::SafeDownCast(partition.Input))
    {
      InsertRemotePoints<Dim>(this, cartesianGrid, partition, this->Tolerance);
    }
    else
    {
      InsertRemotePoints<Dim>(
        this, vtkUnstructuredGrid::SafeDownCast(partition.Input), partition, this->Tolerance);
    }
  }

  // Step 9: with Decompose, the center points of the Category B cells of the partitions
  // with point global ids get new global ids, after the largest one of the whole input and of the
  // center points of the vtkPartitionedDataSet resolved before (see MaxPointGlobalId), in the
  // order of the processes and of their partitions. All the processes take part, see
  // GetCentroidGlobalIdsOffset.
  if (hasCandidates && this->Decompose)
  {
    vtkIdType maxGlobalId = this->MaxPointGlobalId;
    vtkIdType numCentroidPoints = 0;
    std::vector<vtkIdType> partitionNumCentroidPoints(partitions.size(), 0);
    for (std::size_t p = 0; p < partitions.size(); ++p)
    {
      vtkDataArray* globalIds = partitions[p].Input->GetPointData()->GetGlobalIds();
      if (globalIds && globalIds->GetNumberOfTuples() > 0 && !partitions[p].IsDone)
      {
        partitionNumCentroidPoints[p] = CountCentroidPoints<Dim>(partitions[p].CandidatesInfo);
        numCentroidPoints += partitionNumCentroidPoints[p];
      }
    }
    vtkIdType offset = 0;
    vtkIdType totalNumCentroidPoints = 0;
    this->GetCentroidGlobalIdsOffset(
      maxGlobalId, numCentroidPoints, offset, totalNumCentroidPoints);
    this->MaxPointGlobalId = maxGlobalId + totalNumCentroidPoints;
    vtkIdType nextGlobalId = maxGlobalId + 1 + offset;
    for (std::size_t p = 0; p < partitions.size(); ++p)
    {
      if (partitionNumCentroidPoints[p] > 0)
      {
        partitions[p].FirstCentroidGlobalId = nextGlobalId;
        nextGlobalId += partitionNumCentroidPoints[p];
      }
    }
  }

  // Step 10: the outputs
  for (Partition<Dim>& partition : partitions)
  {
    if (partition.IsDone)
    {
      continue;
    }
    if (auto polyData = vtkPolyData::SafeDownCast(partition.Input))
    {
      CommitPartition<Dim>(
        polyData, partition, this->Decompose, this->ComputeInverseMeasure, originalCellIdsName);
    }
    else if (auto cartesianGrid = vtkCartesianGrid::SafeDownCast(partition.Input))
    {
      CommitPartition<Dim>(cartesianGrid, partition, this->Decompose, this->ComputeInverseMeasure,
        originalCellIdsName);
    }
    else
    {
      CommitPartition<Dim>(vtkUnstructuredGrid::SafeDownCast(partition.Input), partition,
        this->Decompose, this->ComputeInverseMeasure, originalCellIdsName);
    }
  }
  return result;
}

namespace
{
//------------------------------------------------------------------------------
// The dimension of the cells of the partition `input`: 2 for quads or pixels, 3 for hexahedra or
// voxels, 0 without cells, or -1, reported by `self`, if `input` is null or is not a vtkPolyData of
// quads, a vtkUnstructuredGrid of quads or hexahedra, or a 2D or 3D vtkCartesianGrid without
// blanking.
int GetCellDimension(vtkResolveHangingNodes* self, vtkDataSet* input)
{
  auto inputPolyData = vtkPolyData::SafeDownCast(input);
  auto inputGrid = vtkUnstructuredGrid::SafeDownCast(input);
  auto inputCartesianGrid = vtkCartesianGrid::SafeDownCast(input);
  if (!inputPolyData && !inputGrid && !inputCartesianGrid)
  {
    vtkErrorWithObjectMacro(
      self, "The input dataset must be vtkPolyData, vtkUnstructuredGrid or vtkCartesianGrid.");
    return -1;
  }
  if (input->GetNumberOfCells() == 0)
  {
    return 0;
  }
  if (inputCartesianGrid)
  {
    // The candidate cells of a vtkCartesianGrid are its boundary cells, which a blanked cell or
    // point would change, see FindCartesianCandidateCells.
    if (inputCartesianGrid->HasAnyBlankCells() || inputCartesianGrid->HasAnyBlankPoints())
    {
      vtkErrorWithObjectMacro(self, "The input vtkCartesianGrid must not have blanking.");
      return -1;
    }
    const int dimension = inputCartesianGrid->GetDataDimension();
    if (dimension != 2 && dimension != 3)
    {
      vtkErrorWithObjectMacro(self, "The input vtkCartesianGrid must be 2D or 3D.");
      return -1;
    }
    return dimension;
  }
  if (inputPolyData)
  {
    // The cell ids of vtkPolyData count the vertices, lines and strips before the polygons.
    if (inputPolyData->GetNumberOfVerts() > 0 || inputPolyData->GetNumberOfLines() > 0 ||
      inputPolyData->GetNumberOfStrips() > 0 || inputPolyData->GetPolys()->IsHomogeneous() != 4)
    {
      vtkErrorWithObjectMacro(self, "The input vtkPolyData must have only quads.");
      return -1;
    }
    return 2;
  }
  const int cellType = inputGrid->IsHomogeneous() ? inputGrid->GetCellType(0) : VTK_EMPTY_CELL;
  if (cellType != VTK_QUAD && cellType != VTK_HEXAHEDRON)
  {
    vtkErrorWithObjectMacro(
      self, "The input vtkUnstructuredGrid must have only quads or only hexahedra.");
    return -1;
  }
  return cellType == VTK_HEXAHEDRON ? 3 : 2;
}

//------------------------------------------------------------------------------
// Check the partitions `inputs`, null if they are not vtkDataSet, whose results are written in
// `outputs`, null if they are not vtkPointSet, with `isValid` telling whether all the partitions
// are valid: they have only quads or only hexahedra, as the others. An invalid partition is
// replaced in `inputs` and `outputs` by empty ones, and is resolved as such: its output stays
// empty, as the pipeline or AddPartitions created it. Returns the dimension of the cells. The
// partitions are not converted: a vtkPolyData gives a vtkPolyData, and a vtkCartesianGrid, whose
// cells are read as quads or hexahedra (see GetCellPoints), a vtkUnstructuredGrid. Without cells,
// the output is empty anyway, and the 2D path is used: it makes the same parallel calls as the 3D
// one, and the number of levels uses the dimension of the other processes (see
// GetGlobalMeasureRange).
int CheckInputs(vtkResolveHangingNodes* self, std::vector<vtkSmartPointer<vtkDataSet>>& inputs,
  std::vector<vtkSmartPointer<vtkPointSet>>& outputs, bool& isValid)
{
  const std::size_t numPartitions = inputs.size();
  isValid = true;
  int dimension = 0;
  for (std::size_t p = 0; p < numPartitions; ++p)
  {
    int partitionDimension = GetCellDimension(self, inputs[p]);
    if (partitionDimension > 0 && dimension > 0 && partitionDimension != dimension)
    {
      vtkErrorWithObjectMacro(self, "The partitions must all have quads, or all hexahedra.");
      partitionDimension = -1;
    }
    else if (partitionDimension > 0)
    {
      dimension = partitionDimension;
    }
    if (partitionDimension < 0)
    {
      // The other partitions and processes still expect this one in the parallel calls: run with
      // no cell.
      isValid = false;
      inputs[p] = vtkSmartPointer<vtkUnstructuredGrid>::New();
      outputs[p] = vtkSmartPointer<vtkUnstructuredGrid>::New();
    }
  }
  return dimension;
}

} // namespace

//------------------------------------------------------------------------------
int vtkResolveHangingNodes::CheckAndResolvePartitions(
  std::vector<vtkSmartPointer<vtkDataSet>> inputs,
  std::vector<vtkSmartPointer<vtkPointSet>> outputs, const std::vector<int>& partitionedDataSetIds)
{
  bool isValid;
  const int dimension = CheckInputs(this, inputs, outputs, isValid);
  const int result = dimension == 3
    ? this->ResolvePartitions<3>(inputs, outputs, partitionedDataSetIds)
    : this->ResolvePartitions<2>(inputs, outputs, partitionedDataSetIds);
  return isValid ? result : 0;
}

namespace
{
//------------------------------------------------------------------------------
// Append to `inputs` the partitions of `inputPDS`, which may be null, null if they are not
// vtkDataSet, to `outputs` the partitions of `outputPDS` that receive their results, null if they
// are not vtkPointSet: empty datasets of the same types, but vtkUnstructuredGrid for
// vtkCartesianGrid, and to `partitionedDataSetIds` their `partitionedDataSetId`: -1 to resolve them
// with all the other partitions, or the index of `inputPDS` to resolve them only with the
// partitions of other vtkPartitionedDataSet, see vtkResolveHangingNodes::GetRemotePoints().
void AddPartitions(vtkPartitionedDataSet* inputPDS, vtkPartitionedDataSet* outputPDS,
  int partitionedDataSetId, std::vector<vtkSmartPointer<vtkDataSet>>& inputs,
  std::vector<vtkSmartPointer<vtkPointSet>>& outputs, std::vector<int>& partitionedDataSetIds)
{
  for (unsigned int p = 0; inputPDS && p < inputPDS->GetNumberOfPartitions(); ++p)
  {
    if (auto partition = inputPDS->GetPartitionAsDataObject(p))
    {
      if (vtkCartesianGrid::SafeDownCast(partition))
      {
        outputPDS->SetPartition(p, vtkSmartPointer<vtkUnstructuredGrid>::New());
      }
      else
      {
        outputPDS->SetPartition(p, vtk::TakeSmartPointer(partition->NewInstance()));
      }
      inputs.emplace_back(vtkDataSet::SafeDownCast(partition));
      outputs.emplace_back(vtkPointSet::SafeDownCast(outputPDS->GetPartitionAsDataObject(p)));
      partitionedDataSetIds.push_back(partitionedDataSetId);
    }
  }
}
} // namespace

//------------------------------------------------------------------------------
int vtkResolveHangingNodes::ResolvePartitionedDataSet(
  vtkPartitionedDataSet* inputPDS, vtkPartitionedDataSet* outputPDS)
{
  std::vector<vtkSmartPointer<vtkDataSet>> inputs;
  std::vector<vtkSmartPointer<vtkPointSet>> outputs;
  std::vector<int> partitionedDataSetIds;
  // -1: every partition is resolved with all the other partitions of `inputPDS`, here and on the
  // other processes, since they may have hanging nodes between them.
  AddPartitions(inputPDS, outputPDS, -1, inputs, outputs, partitionedDataSetIds);
  return this->CheckAndResolvePartitions(inputs, outputs, partitionedDataSetIds);
}

namespace
{
//------------------------------------------------------------------------------
// The largest point global id of the datasets of `input`, -1 without point global ids.
vtkIdType GetMaxPointGlobalId(vtkDataObject* input)
{
  vtkIdType maxGlobalId = -1;
  for (vtkDataSet* dataSet : vtkCompositeDataSet::GetDataSets(input))
  {
    vtkDataArray* globalIds = dataSet->GetPointData()->GetGlobalIds();
    if (globalIds && globalIds->GetNumberOfTuples() > 0)
    {
      double range[2];
      globalIds->GetRange(range, 0);
      maxGlobalId = std::max(maxGlobalId, static_cast<vtkIdType>(range[1]));
    }
  }
  return maxGlobalId;
}
} // namespace

//------------------------------------------------------------------------------
int vtkResolveHangingNodes::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkDataObject* inputDO = vtkDataObject::GetData(inputVector[0]);
  vtkDataObject* outputDO = vtkDataObject::GetData(outputVector);
  if (!inputDO || !outputDO)
  {
    return 0;
  }
  // The number of levels is the largest one of the vtkPartitionedDataSet that are resolved, and the
  // center points follow the largest point global id of the whole input, see ResolvePartitions.
  this->NumberOfLevels = 0;
  this->MaxPointGlobalId = GetMaxPointGlobalId(inputDO);

  // The output has the structure of the input, with empty partitions to receive the results. The
  // blocks of all the levels of a vtkNonOverlappingAMR are resolved together, but a level is
  // conforming: a block only receives the points of the blocks of other levels.
  if (auto inputAMR = vtkNonOverlappingAMR::SafeDownCast(inputDO))
  {
    auto outputPDC = vtkPartitionedDataSetCollection::SafeDownCast(outputDO);
    outputPDC->CopyStructure(inputAMR);
    std::vector<vtkSmartPointer<vtkDataSet>> inputs;
    std::vector<vtkSmartPointer<vtkPointSet>> outputs;
    std::vector<int> partitionedDataSetIds;
    for (unsigned int level = 0; level < inputAMR->GetNumberOfPartitionedDataSets(); ++level)
    {
      // The level: a block is only resolved with the blocks of the other levels.
      AddPartitions(inputAMR->GetPartitionedDataSet(level), outputPDC->GetPartitionedDataSet(level),
        static_cast<int>(level), inputs, outputs, partitionedDataSetIds);
    }
    return this->CheckAndResolvePartitions(inputs, outputs, partitionedDataSetIds);
  }
  // The partitions of a vtkPartitionedDataSet are resolved together, and every
  // vtkPartitionedDataSet of a collection on its own.
  if (auto inputPDS = vtkPartitionedDataSet::SafeDownCast(inputDO))
  {
    auto outputPDS = vtkPartitionedDataSet::SafeDownCast(outputDO);
    outputPDS->CopyStructure(inputPDS);
    return this->ResolvePartitionedDataSet(inputPDS, outputPDS);
  }
  if (auto inputPDC = vtkPartitionedDataSetCollection::SafeDownCast(inputDO))
  {
    auto outputPDC = vtkPartitionedDataSetCollection::SafeDownCast(outputDO);
    outputPDC->CopyStructure(inputPDC);
    int result = 1;
    for (unsigned int i = 0; i < inputPDC->GetNumberOfPartitionedDataSets(); ++i)
    {
      if (!this->ResolvePartitionedDataSet(
            inputPDC->GetPartitionedDataSet(i), outputPDC->GetPartitionedDataSet(i)))
      {
        result = 0;
      }
    }
    return result;
  }
  if (auto inputDS = vtkDataSet::SafeDownCast(inputDO))
  {
    // A single partition
    return this->CheckAndResolvePartitions(
      { inputDS }, { vtkPointSet::SafeDownCast(outputDO) }, { -1 });
  }
  vtkErrorMacro("Unsupported input type " << inputDO->GetClassName() << ".");
  return 0;
}

VTK_ABI_NAMESPACE_END
