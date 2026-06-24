// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkPolyhedronContour.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkIdList.h"
#include "vtkIncrementalPointLocator.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolygon.h"
#include "vtkPolyhedron.h"

#include <algorithm>
#include <array>
#include <cstring>

VTK_ABI_NAMESPACE_BEGIN

//------------------------------------------------------------------------------
// Raw-array Lopez algorithm core
// All three steps (classify, find iso-vertices, trace polygons) are static
// helpers that operate on a TraceResult plus raw arrays. Both the instance
// API (Execute) and the bulk API (ContourCell, CountClip, EmitClip) drive
// the same shared core.
//------------------------------------------------------------------------------

namespace
{
struct PolyhedronFacesToLocalFaceStream : public vtkCellArray::DispatchUtilities
{
  template <class OffsetsT, class ConnectivityT, class MapT>
  void operator()(OffsetsT* offsets, ConnectivityT* conn, MapT& globalToLocal,
    std::vector<vtkIdType>& localFaceStream)
  {
    auto offsetsRange = GetRange(offsets);
    auto connRange = GetRange(conn);

    vtkIdType numFaces = GetNumberOfCells(offsets);

    localFaceStream.clear();
    localFaceStream.reserve(connRange.size() + numFaces);

    for (vtkIdType i = 0; i < numFaces; ++i)
    {
      vtkIdType numFaceVerts = offsetsRange[i + 1] - offsetsRange[i];
      localFaceStream.push_back(numFaceVerts);
      for (vtkIdType j = 0; j < numFaceVerts; ++j)
      {
        vtkIdType globalPtId = connRange[offsetsRange[i] + j];
        auto it = globalToLocal.find(globalPtId);
        assert(it != globalToLocal.end());
        localFaceStream.push_back(static_cast<vtkIdType>(it->second));
      }
    }
  }
};

// Build a map from global point ID to local index (0..nCellPts-1)
void BuildLocalMap(
  const vtkIdType* pointIds, vtkIdType nCellPts, std::unordered_map<vtkIdType, int>& globalToLocal)
{
  globalToLocal.clear();
  globalToLocal.reserve(static_cast<size_t>(nCellPts));
  for (vtkIdType i = 0; i < nCellPts; ++i)
  {
    globalToLocal[pointIds[i]] = static_cast<int>(i);
  }
}

// Canonicalized edge key — (min(a,b), max(a,b)) packed into 64 bits.
// Used everywhere two-endpoint edge identity is needed.
inline int64_t MakeEdgeKey(vtkIdType a, vtkIdType b)
{
  vtkIdType lo = (a < b) ? a : b;
  vtkIdType hi = (a < b) ? b : a;
  return (static_cast<int64_t>(lo) << 32) | static_cast<int64_t>(hi & 0xFFFFFFFF);
}
} // anonymous namespace

//------------------------------------------------------------------------------
// Step 1: Classify vertices.
//
// On-surface vertices (s == isoValue) are classified as "inside" (tag=1) so
// that edges between on-surface and strictly-below vertices generate
// iso-vertices at t=1. This lets ambiguous cells (with only on-surface +
// below-iso vertices, or only on-surface + above-iso vertices) produce a
// valid cap face rather than being dropped.
//------------------------------------------------------------------------------
void vtkPolyhedronContour::ClassifyVertices(TraceResult& result,
  const std::vector<double>& localScalars, double isoValue, int& numInside, int& numOutside)
{
  result.VertexTags.assign(localScalars.size(), 0);
  numInside = 0;
  numOutside = 0;
  for (size_t i = 0; i < localScalars.size(); ++i)
  {
    if (localScalars[i] >= isoValue)
    {
      result.VertexTags[i] = 1;
      ++numInside;
    }
    else
    {
      ++numOutside;
    }
  }
}

//------------------------------------------------------------------------------
// Step 2: Walk the face stream and emit iso-vertices for crossing edges.
//
// Edge identity is keyed by (min,max) of the two local vertex IDs so that
// the two faces sharing an edge see the same key. Each iso-vertex records
// its KeyFace (outside->inside transition) and OtherFace (inside->outside).
// For inconsistently-wound polyhedra both faces may see the same direction;
// in that case the second face is forced into whichever slot is still empty
// so every iso-vertex ends up with both faces filled.
//------------------------------------------------------------------------------
void vtkPolyhedronContour::FindIsoVertices(TraceResult& result, vtkIdType nFaces,
  const vtkIdType* localFaceStream, const std::vector<double>& localScalars, double isoValue)
{
  auto& faceIsoVertices = result.FaceIsoVertices;
  auto& isoVertPosInFace = result.IsoVertexPositionInFace;
  auto& edgeToIsoVert = result.EdgeToIsoVert;

  const vtkIdType* fs = localFaceStream;
  for (vtkIdType faceIdx = 0; faceIdx < nFaces; ++faceIdx)
  {
    vtkIdType numFaceVerts = *fs++;
    // Traverse each edge of the face (consecutive vertex pairs, wrapping around).
    for (vtkIdType i = 0; i < numFaceVerts; ++i)
    {
      vtkIdType loc0 = fs[i];
      vtkIdType loc1 = fs[(i + 1) % numFaceVerts];

      if (result.VertexTags[loc0] == result.VertexTags[loc1])
      {
        continue;
      }

      // Determine outside/inside endpoints.
      vtkIdType outsideLocId = (result.VertexTags[loc0] == 0) ? loc0 : loc1;
      vtkIdType insideLocId = (result.VertexTags[loc0] == 1) ? loc0 : loc1;
      // Edge type on this face:
      //   Type 1 (key):   outside->inside transition (traversing face boundary)
      //   Type 2 (other): inside->outside transition
      bool isKeyFace = (result.VertexTags[loc0] == 0);

      int64_t ek = MakeEdgeKey(outsideLocId, insideLocId);
      auto eit = edgeToIsoVert.find(ek);
      int isoIdx;

      if (eit == edgeToIsoVert.end())
      {
        // New iso-vertex
        isoIdx = static_cast<int>(result.IsoVertices.size());
        IsoVertex iv;
        iv.OutsideVertex = outsideLocId;
        iv.InsideVertex = insideLocId;
        iv.KeyFace = isKeyFace ? static_cast<int>(faceIdx) : -1;
        iv.OtherFace = isKeyFace ? -1 : static_cast<int>(faceIdx);
        iv.OutputPointId = -1;

        // Linear interpolation parameter along the (outside -> inside) edge.
        double phi0 = localScalars[outsideLocId];
        double phi1 = localScalars[insideLocId];
        double deltaScalar = phi1 - phi0;
        iv.Weight = (deltaScalar != 0.0) ? (isoValue - phi0) / deltaScalar : 0.0;

        result.IsoVertices.push_back(iv);
        edgeToIsoVert[ek] = isoIdx;
      }
      else
      {
        // Edge already visited from the other face — fill in the missing slot.
        // For consistently-wound polyhedra, one face sees outside->inside (key)
        // and the other sees inside->outside (other). For inconsistently-wound
        // faces both may see the same direction; force the second face into
        // whichever slot is still unassigned so every iso-vertex ends up with
        // both KeyFace and OtherFace populated.
        isoIdx = eit->second;
        auto& iv = result.IsoVertices[isoIdx];
        if (isKeyFace && iv.KeyFace < 0)
        {
          iv.KeyFace = static_cast<int>(faceIdx);
        }
        else if (!isKeyFace && iv.OtherFace < 0)
        {
          iv.OtherFace = static_cast<int>(faceIdx);
        }
        else if (iv.KeyFace < 0)
        {
          iv.KeyFace = static_cast<int>(faceIdx);
        }
        else if (iv.OtherFace < 0)
        {
          iv.OtherFace = static_cast<int>(faceIdx);
        }
      }

      int posInFace = static_cast<int>(faceIsoVertices[faceIdx].size());
      faceIsoVertices[faceIdx].push_back(isoIdx);

      if (isoIdx >= static_cast<int>(isoVertPosInFace[faceIdx].size()))
      {
        isoVertPosInFace[faceIdx].resize(isoIdx + 1, -1);
      }
      isoVertPosInFace[faceIdx][isoIdx] = posInFace;
    }
    fs += numFaceVerts;
  }
}

//------------------------------------------------------------------------------
// Step 3: Trace closed iso-polygons.
//
// For each unassigned iso-vertex, walk forward along its KeyFace (advancing
// +1 in the face's iso-vertex list) until the loop closes or breaks. If only
// OtherFace is available we walk backward (-1) instead, since the polygon
// ring runs in the opposite winding direction on that face. Polygons with
// fewer than 3 vertices are dropped.
//------------------------------------------------------------------------------
void vtkPolyhedronContour::TraceIsoPolygons(TraceResult& result)
{
  const int numIsoVerts = static_cast<int>(result.IsoVertices.size());
  if (numIsoVerts == 0)
  {
    return;
  }

  result.Assigned.assign(numIsoVerts, false);
  auto& assigned = result.Assigned;
  const auto& faceIsoVertices = result.FaceIsoVertices;
  const auto& isoVertPosInFace = result.IsoVertexPositionInFace;

  // Find the next iso-vertex on the polygon walk. Returns -1 if the walk
  // cannot be continued.
  auto findNext = [&](int currentVertex) -> int
  {
    int keyFace = result.IsoVertices[currentVertex].KeyFace;
    bool usingOtherFace = false;

    if (keyFace < 0 || keyFace >= static_cast<int>(isoVertPosInFace.size()))
    {
      keyFace = result.IsoVertices[currentVertex].OtherFace;
      usingOtherFace = true;
      if (keyFace < 0)
      {
        return -1;
      }
    }

    if (currentVertex >= static_cast<int>(isoVertPosInFace[keyFace].size()))
    {
      return -1;
    }
    int position = isoVertPosInFace[keyFace][currentVertex];
    if (position < 0)
    {
      return -1;
    }

    int numOnFace = static_cast<int>(faceIsoVertices[keyFace].size());
    if (numOnFace <= 0)
    {
      return -1;
    }

    // KeyFace (outside->inside): forward (+1).
    // OtherFace (inside->outside): backward (-1) because the polygon ring
    // runs in opposite winding on that face.
    int nextPosition =
      usingOtherFace ? (position - 1 + numOnFace) % numOnFace : (position + 1) % numOnFace;
    int nextVertex = faceIsoVertices[keyFace][nextPosition];

    if (nextVertex < 0 || nextVertex >= numIsoVerts)
    {
      return -1;
    }
    return nextVertex;
  };

  for (int startVertex = 0; startVertex < numIsoVerts; ++startVertex)
  {
    if (assigned[startVertex])
    {
      continue;
    }

    std::vector<int> polygon;
    int currentVertex = startVertex;
    const int maxIter = numIsoVerts + 1;
    int iter = 0;

    while (iter++ < maxIter)
    {
      polygon.push_back(currentVertex);
      assigned[currentVertex] = true;

      int nextVertex = findNext(currentVertex);
      if (nextVertex < 0)
      {
        break;
      }
      // Loop closed.
      if (nextVertex == startVertex)
      {
        break;
      }
      // Reached an already-assigned vertex. On polyhedra with faces cut
      // multiple times, the OtherFace fallback can loop back to a vertex
      // already in the current polygon; the partial polygon collected so far
      // is still geometrically valid as long as it has >= 3 vertices.
      if (assigned[nextVertex])
      {
        break;
      }

      currentVertex = nextVertex;
    }

    if (polygon.size() >= 3)
    {
      result.IsoPolygons.push_back(std::move(polygon));
    }
  }
}

//------------------------------------------------------------------------------
// Run the trace assuming the cell straddles the iso-value.
//
// Caller responsibilities (must happen BEFORE this is called):
//   1. result.Clear(nFaces)        — reset per-face scratch
//   2. ClassifyVertices(...) — populate result.VertexTags
//   3. Decide that the cell intersects the iso-surface
//      (numInside > 0 && numOutside > 0) — otherwise don't call this at all
//   4. Build localFaceStream
//
// On return, the caller checks result.IsoPolygons; if non-empty the trace
// produced output. (result.IsoVertices.empty() implies IsoPolygons.empty()
// since polygons are made of iso-vertices, so checking IsoVertices alone
// is sufficient.)
//------------------------------------------------------------------------------
void vtkPolyhedronContour::RunLopezTraceInto(TraceResult& result, vtkIdType nFaces,
  const vtkIdType* localFaceStream, const std::vector<double>& localScalars, double isoValue)
{
  FindIsoVertices(result, nFaces, localFaceStream, localScalars, isoValue);
  if (result.IsoVertices.empty())
  {
    return;
  }

  TraceIsoPolygons(result);
}

//------------------------------------------------------------------------------
// Interpolate iso-vertex positions, register them with the locator, and
// interpolate point data. Fills IsoVertex::Position and ::OutputPointId.
//------------------------------------------------------------------------------
void vtkPolyhedronContour::InterpolatePositionsAndInsertPoints(
  vtkPolyhedron* cell, vtkIncrementalPointLocator* locator, vtkPointData* inPd, vtkPointData* outPd)
{
  vtkPoints* cellPoints = cell->GetPoints();
  vtkIdList* localToGlobal = cell->GetPointIds();

  for (auto& isoVert : this->Result.IsoVertices)
  {
    double p0[3], p1[3];
    cellPoints->GetPoint(isoVert.OutsideVertex, p0);
    cellPoints->GetPoint(isoVert.InsideVertex, p1);

    // Weight was computed during FindIsoVerticesStatic; reuse it here.
    const double t = isoVert.Weight;

    isoVert.Position[0] = p0[0] + t * (p1[0] - p0[0]);
    isoVert.Position[1] = p0[1] + t * (p1[1] - p0[1]);
    isoVert.Position[2] = p0[2] + t * (p1[2] - p0[2]);

    if (locator->InsertUniquePoint(isoVert.Position.data(), isoVert.OutputPointId) < 0)
    {
      vtkGenericWarningMacro("Failed to insert iso-vertex into point locator");
    }

    // Interpolate point data.
    if (inPd && outPd)
    {
      vtkIdType globalId0 = localToGlobal->GetId(isoVert.OutsideVertex);
      vtkIdType globalId1 = localToGlobal->GetId(isoVert.InsideVertex);
      if (globalId0 >= 0 && globalId1 >= 0 && inPd->GetNumberOfArrays() > 0)
      {
        outPd->InterpolateEdge(inPd, isoVert.OutputPointId, globalId0, globalId1, t);
      }
    }
  }
}

//------------------------------------------------------------------------------
vtkPolyhedronContour::CellClassification vtkPolyhedronContour::Execute(vtkPolyhedron* cell,
  vtkDataArray* scalars, double isoValue, vtkIncrementalPointLocator* locator, vtkPointData* inPd,
  vtkPointData* outPd)
{
  if (!cell || !scalars || !locator)
  {
    return CellClassification::AllOutside;
  }

  const vtkIdType numVertices = cell->GetNumberOfPoints();
  const vtkIdType numFaces = cell->GetNumberOfFaces();
  if (numVertices == 0 || numFaces == 0)
  {
    return CellClassification::AllOutside;
  }

  // (1) Local scalars — the only input ClassifyVerticesStatic needs.
  std::vector<double> localScalars(numVertices);
  for (vtkIdType i = 0; i < numVertices; ++i)
  {
    localScalars[i] = scalars->GetTuple1(i);
  }

  // (2) Classify
  this->Result.Clear(numFaces);
  int numInside = 0, numOutside = 0;
  ClassifyVertices(this->Result, localScalars, isoValue, numInside, numOutside);
  if (numInside == 0)
  {
    this->Result.IsoPolygons.clear();
    return CellClassification::AllOutside;
  }
  if (numOutside == 0)
  {
    this->Result.IsoPolygons.clear();
    return CellClassification::AllInside;
  }

  // (3) Cell intersects — now pay for the face stream. The dispatcher is the
  // same one the bulk static API uses; cell->PointIdMap is global -> local.
  std::vector<vtkIdType> localFaceStream;
  cell->GetCellFaces()->Dispatch(
    PolyhedronFacesToLocalFaceStream{}, cell->PointIdMap, localFaceStream);

  // (4) Find iso-vertices and trace iso-polygons.
  RunLopezTraceInto(this->Result, numFaces, localFaceStream.data(), localScalars, isoValue);

  if (this->Result.IsoVertices.empty())
  {
    this->Result.IsoPolygons.clear();
    return CellClassification::AllOutside;
  }

  // (5) Place iso-vertices in 3D, register with locator, interpolate point data.
  this->InterpolatePositionsAndInsertPoints(cell, locator, inPd, outPd);

  return CellClassification::Intersected;
}

//------------------------------------------------------------------------------
void vtkPolyhedronContour::OutputContours(
  vtkCellArray* polys, vtkCellData* inCd, vtkIdType cellId, vtkCellData* outCd)
{
  if (!polys)
  {
    return;
  }

  std::vector<vtkIdType> pts;
  for (const auto& polygon : this->Result.IsoPolygons)
  {
    const int numVerts = static_cast<int>(polygon.size());
    if (numVerts < 3)
    {
      continue;
    }

    pts.resize(numVerts);
    bool valid = true;
    for (int i = 0; i < numVerts; ++i)
    {
      int isoIdx = polygon[i];
      if (isoIdx < 0 || isoIdx >= static_cast<int>(this->Result.IsoVertices.size()))
      {
        valid = false;
        break;
      }
      if (this->Result.IsoVertices[isoIdx].OutputPointId < 0)
      {
        valid = false;
        break;
      }
      pts[i] = this->Result.IsoVertices[isoIdx].OutputPointId;
    }
    if (!valid)
    {
      continue;
    }

    // Output iso-polygon directly. The López algorithm produces polygons,
    // not triangles. The calling filter (vtkContourHelper/vtkContourGrid)
    // is responsible for triangulation when GenerateTriangles is on.
    vtkIdType newCellId = polys->InsertNextCell(numVerts, pts.data());
    if (outCd)
    {
      outCd->CopyData(inCd, cellId, newCellId);
    }
  }
}

//------------------------------------------------------------------------------
void vtkPolyhedronContour::OutputClip(vtkPolyhedron* cell, vtkCellArray* connectivity,
  vtkIncrementalPointLocator* locator, vtkPointData* inPd, vtkPointData* outPd, vtkCellData* inCd,
  vtkIdType cellId, vtkCellData* outCd, int insideOut, vtkCellArray* outFaces,
  vtkCellArray* outFaceLocs)
{
  if (!cell || !connectivity || !locator)
  {
    return;
  }

  const int keepTag = insideOut ? 0 : 1; // Which vertices to keep

  vtkPoints* cellPoints = cell->GetPoints();
  auto& pointIdMap = cell->PointIdMap;
  const vtkIdType numFaces = cell->GetNumberOfFaces();

  // Map local vertex ID → output point ID (for kept original vertices)
  std::vector<vtkIdType> localToOutputId(cell->GetNumberOfPoints(), -1);

  // Per-face assembly needs random access for wraparound merge and degenerate
  // removal; completed faces are flushed via flushFace into either:
  //   - outFaces/outFaceLocs (direct path, no embedded format), or
  //   - polyhedronStream (fallback embedded face-stream format).
  std::vector<std::vector<vtkIdType>> facePieces;
  vtkIdType nFacesWritten = 0;
  std::vector<vtkIdType> faceIds; // direct path: face cell IDs inserted into outFaces

  vtkNew<vtkIdList> polyhedronStream;
  if (!outFaces)
  {
    polyhedronStream->InsertNextId(0); // placeholder for nFaces; filled in at end
  }

  const auto flushFace = [&](const std::vector<vtkIdType>& face)
  {
    if (outFaces)
    {
      faceIds.push_back(outFaces->InsertNextCell(static_cast<vtkIdType>(face.size()), face.data()));
    }
    else
    {
      polyhedronStream->InsertNextId(static_cast<vtkIdType>(face.size()));
      for (vtkIdType ptId : face)
      {
        polyhedronStream->InsertNextId(ptId);
      }
    }
    ++nFacesWritten;
  };

  std::vector<vtkIdType> newFace;

  // Build O(1) lookup from edge key to iso-vertex OutputPointId.
  std::unordered_map<int64_t, vtkIdType> edgeToIsoOutputId;
  edgeToIsoOutputId.reserve(this->Result.IsoVertices.size());
  for (const auto& iv : this->Result.IsoVertices)
  {
    edgeToIsoOutputId[MakeEdgeKey(iv.OutsideVertex, iv.InsideVertex)] = iv.OutputPointId;
  }

  for (vtkIdType faceIdx = 0; faceIdx < numFaces; ++faceIdx)
  {
    const vtkIdType* facePts = nullptr;
    vtkIdType numFaceVerts = cell->GetFacePoints(faceIdx, facePts);
    if (numFaceVerts < 3 || !facePts)
    {
      continue;
    }

    // Walk face boundary, collecting kept vertices and iso-vertices.
    // When a face is cut multiple times, the kept region may consist of
    // multiple disconnected segments. Each segment becomes a separate face
    // piece. The face boundary is circular, so a kept segment may straddle
    // the walk start point — in that case the trailing partial piece wraps
    // around and must be merged with the first flushed piece.
    newFace.clear();
    bool flushedAny = false;

    // Traverse each edge of the face (consecutive vertex pairs, wrapping around).
    for (vtkIdType i = 0; i < numFaceVerts; ++i)
    {
      vtkIdType globalId0 = facePts[i];
      vtkIdType globalId1 = facePts[(i + 1) % numFaceVerts];

      auto it0 = pointIdMap.find(globalId0);
      auto it1 = pointIdMap.find(globalId1);
      if (it0 == pointIdMap.end() || it1 == pointIdMap.end())
      {
        continue;
      }

      vtkIdType localId0 = it0->second;
      vtkIdType localId1 = it1->second;

      // Add vertex if on kept side
      if (this->Result.VertexTags[localId0] == keepTag)
      {
        if (localToOutputId[localId0] < 0)
        {
          double pt[3];
          cellPoints->GetPoint(localId0, pt);
          if (locator->InsertUniquePoint(pt, localToOutputId[localId0]) < 0)
          {
            vtkGenericWarningMacro("Failed to insert kept vertex into point locator");
          }
          if (inPd)
          {
            outPd->CopyData(inPd, globalId0, localToOutputId[localId0]);
          }
        }
        newFace.push_back(localToOutputId[localId0]);
      }

      // If edge is cut, add the iso-vertex
      if (this->Result.VertexTags[localId0] != this->Result.VertexTags[localId1])
      {
        vtkIdType outsideVert = (this->Result.VertexTags[localId0] == 0) ? localId0 : localId1;
        vtkIdType insideVert = (this->Result.VertexTags[localId0] == 1) ? localId0 : localId1;
        auto eit = edgeToIsoOutputId.find(MakeEdgeKey(outsideVert, insideVert));
        if (eit != edgeToIsoOutputId.end())
        {
          newFace.push_back(eit->second);
        }

        // If this crossing goes from kept→discarded, flush the current
        // face piece (it's complete) and start a new one.
        if (this->Result.VertexTags[localId0] == keepTag)
        {
          // Always flush on kept→discard transition. Pieces may be partial
          // if the kept segment straddles the walk start point — the
          // wraparound merge below will join them.
          facePieces.push_back(std::move(newFace));
          flushedAny = true;
          newFace.clear();
        }
      }
    }

    // Handle wraparound: if there's a trailing partial piece and we flushed
    // at least one piece, the trailing piece is the continuation of the
    // first piece (the kept segment straddles the walk start point).
    if (!newFace.empty() && flushedAny && !facePieces.empty())
    {
      // Prepend the trailing partial to the first piece from this face
      auto& first = facePieces[0];
      newFace.insert(newFace.end(), first.begin(), first.end());
      first = std::move(newFace);
      newFace.clear(); // reset moved-from vector for reuse below
    }
    else if (!newFace.empty())
    {
      facePieces.push_back(std::move(newFace));
      newFace.clear(); // reset moved-from vector for reuse below
    }

    // Remove degenerate pieces (< 3 vertices)
    while (!facePieces.empty() && facePieces.back().size() < 3)
      facePieces.pop_back();
    for (size_t pi = 0; pi < facePieces.size();)
    {
      if (facePieces[pi].size() < 3)
      {
        facePieces.erase(facePieces.begin() + static_cast<ptrdiff_t>(pi));
      }
      else
      {
        ++pi;
      }
    }

    // Flush completed pieces for this face into polyhedronStream.
    for (size_t pi = 0; pi < facePieces.size(); ++pi)
    {
      flushFace(facePieces[pi]);
    }
    facePieces.clear();
  }
  // Add cap faces. Both inside and outside clips use the same iso-polygons
  // (traced via KeyFace). For the outside clip (keepTag==0), the winding
  // is reversed so the face normal points outward.
  for (const auto& polygon : this->Result.IsoPolygons)
  {
    const int numVerts = static_cast<int>(polygon.size());
    if (numVerts < 3)
    {
      continue;
    }

    // Reuse newFace buffer for cap faces to avoid extra allocation.
    newFace.resize(numVerts);
    if (keepTag == 1)
    {
      // Inside clip: use original winding
      for (int i = 0; i < numVerts; ++i)
      {
        newFace[i] = this->Result.IsoVertices[polygon[i]].OutputPointId;
      }
    }
    else
    {
      // Outside clip: reverse winding for outward-facing normal
      for (int i = 0; i < numVerts; ++i)
      {
        newFace[i] = this->Result.IsoVertices[polygon[numVerts - 1 - i]].OutputPointId;
      }
    }

    flushFace(newFace);
  }

  // Emit the polyhedron.
  if (nFacesWritten == 0)
  {
    return;
  }

  vtkIdType newCellId;
  if (outFaces && outFaceLocs)
  {
    // Direct path: write FaceLocations and one connectivity entry.
    outFaceLocs->InsertNextCell(static_cast<vtkIdType>(faceIds.size()), faceIds.data());
    // Build connectivity using *output* point IDs (localToOutputId), not
    // raw input IDs from cell->GetPointIds(). Only include points that were
    // mapped (localToOutputId >= 0); iso-vertices are captured in faceIds.
    std::vector<vtkIdType> outputPtIds;
    outputPtIds.reserve(cell->GetNumberOfPoints());
    for (vtkIdType li = 0; li < static_cast<vtkIdType>(localToOutputId.size()); ++li)
    {
      if (localToOutputId[li] >= 0)
      {
        outputPtIds.push_back(localToOutputId[li]);
      }
    }
    // Also include iso-vertex output IDs (they are output points not in localToOutputId).
    for (const auto& iv : this->Result.IsoVertices)
    {
      if (iv.OutputPointId >= 0)
      {
        outputPtIds.push_back(iv.OutputPointId);
      }
    }
    // Deduplicate while preserving order.
    std::sort(outputPtIds.begin(), outputPtIds.end());
    outputPtIds.erase(std::unique(outputPtIds.begin(), outputPtIds.end()), outputPtIds.end());
    newCellId =
      connectivity->InsertNextCell(static_cast<vtkIdType>(outputPtIds.size()), outputPtIds.data());
  }
  else
  {
    // Fallback: embedded face-stream format.
    polyhedronStream->SetId(0, nFacesWritten);
    newCellId = connectivity->InsertNextCell(polyhedronStream);
  }

  // Only copy cell data in the fallback path. In the direct path (outFaces set),
  // the caller (e.g. vtkTableBasedClipDataSet) manages cell data externally.
  if (outCd && !outFaces)
  {
    outCd->CopyData(inCd, cellId, newCellId);
  }
}

//------------------------------------------------------------------------------
// Cell-array ContourCell: takes vtkCellArray* faces + vtkDataArray* scalars.
// Produces a flat polygon-traversal-order list of intersected edges plus a
// per-polygon vertex-count array. Designed for integration with threaded
// filters (vtkContour3DLinearGrid) that drive iso-vertex deduplication through
// their own shared edge locator.
//------------------------------------------------------------------------------
void vtkPolyhedronContour::ContourCell(vtkIdType numPointIds, const vtkIdType* pointIds,
  vtkCellArray* polyhedronFaces, vtkDataArray* points, vtkDataArray* scalars, double isoValue,
  bool generateTriangles, std::vector<vtkIdType>& polygonsSize,
  std::vector<EdgeTuple<vtkIdType, double>>& intersectedEdges)
{
  polygonsSize.clear();
  intersectedEdges.clear();

  if (numPointIds < 4 || polyhedronFaces == nullptr || scalars == nullptr)
  {
    return;
  }

  // Per-thread workspace — all per-cell scratch buffers amortize allocation
  // across cells processed by the same worker thread.
  thread_local PolyhedronWorkspace ws;

  // Build raw scalar array indexed by local index (0..numPointIds-1).
  auto& localScalars = ws.LocalScalars;
  localScalars.resize(numPointIds);
  for (vtkIdType i = 0; i < numPointIds; ++i)
  {
    localScalars[i] = scalars->GetComponent(pointIds[i], 0);
  }

  // Classify before building the face stream.
  auto& trace = ws.Trace;
  const vtkIdType nFaces = polyhedronFaces->GetNumberOfCells();
  trace.Clear(nFaces);
  int numInside = 0, numOutside = 0;
  ClassifyVertices(trace, localScalars, isoValue, numInside, numOutside);
  // skip cells that don't straddle the iso-value.
  if (numInside == 0 || numOutside == 0)
  {
    return;
  }

  // Build global->local map for remapping face stream IDs.
  auto& globalToLocal = ws.GlobalToLocal;
  BuildLocalMap(pointIds, numPointIds, globalToLocal);

  // Build raw face stream from vtkCellArray, remapping global IDs to local.
  auto& localFaceStream = ws.LocalFaceStream;
  polyhedronFaces->Dispatch(PolyhedronFacesToLocalFaceStream{}, globalToLocal, localFaceStream);

  RunLopezTraceInto(trace, nFaces, localFaceStream.data(), localScalars, isoValue);

  if (trace.IsoPolygons.empty())
  {
    return;
  }

  // Helper: push one iso-vertex (by trace-local iso-vertex index) as an edge
  // tuple in normalized (g0 < g1) form into intersectedEdges.
  auto pushEdge = [&](int isoIdx)
  {
    const auto& iv = trace.IsoVertices[isoIdx];
    vtkIdType g0 = pointIds[iv.OutsideVertex];
    vtkIdType g1 = pointIds[iv.InsideVertex];
    if (g0 < g1)
    {
      intersectedEdges.emplace_back(g0, g1, iv.Weight);
    }
    else
    {
      intersectedEdges.emplace_back(g1, g0, 1.0 - iv.Weight);
    }
  };

  // Walk iso-polygons; emit edges and polygon sizes.
  if (generateTriangles)
  {
    // Triangulate each iso-polygon with vtkPolygon's quality ear clip
    // (iso-polygons traced through arbitrary polyhedra can be non-convex, which a
    // naive fan does not handle), so the result matches the triangulation
    // vtkPolygon produces everywhere else in VTK. Triangulation is a small
    // fraction of the contour pipeline (dominated by the Lopez trace and the edge
    // locator), so the measure-ordered clip's cost is negligible.
    //
    // triPolygon/triIds are reused across this call's iso-polygons and destroyed
    // when this function returns; they are deliberately NOT thread_local, because
    // a thread_local vtkObject outlives teardown and is reported as a leak.
    auto& earCoords = ws.EarCoords; // contiguous iso-vertex coords, loop order
    vtkNew<vtkPolygon> triPolygon;
    vtkNew<vtkIdList> triIds;
    for (const auto& poly : trace.IsoPolygons)
    {
      const vtkIdType nv = static_cast<vtkIdType>(poly.size());
      if (nv < 3)
      {
        continue;
      }

      // Triangle iso-polygons need no triangulation; emit directly.
      if (nv == 3)
      {
        pushEdge(poly[0]);
        pushEdge(poly[1]);
        pushEdge(poly[2]);
        polygonsSize.push_back(3);
        continue;
      }

      // Gather iso-vertex 3D positions (linear interpolation along each
      // intersected edge) into a contiguous buffer; loop ids are 0..nv-1.
      earCoords.resize(static_cast<std::size_t>(nv));
      for (vtkIdType j = 0; j < nv; ++j)
      {
        const auto& iv = trace.IsoVertices[poly[j]];
        double p0[3], p1[3];
        points->GetTuple(pointIds[iv.OutsideVertex], p0);
        points->GetTuple(pointIds[iv.InsideVertex], p1);
        const double w = iv.Weight;
        auto& cc = earCoords[static_cast<std::size_t>(j)];
        cc[0] = p0[0] + w * (p1[0] - p0[0]);
        cc[1] = p0[1] + w * (p1[1] - p0[1]);
        cc[2] = p0[2] + w * (p1[2] - p0[2]);
      }

      // Fill the reused vtkPolygon with the iso-vertex coordinates and
      // triangulate; EarCutTriangulation emits polygon-local triangle index
      // triples into triIds.
      triPolygon->GetPoints()->SetNumberOfPoints(nv);
      triPolygon->GetPointIds()->SetNumberOfIds(nv);
      for (vtkIdType j = 0; j < nv; ++j)
      {
        triPolygon->GetPoints()->SetPoint(j, earCoords[static_cast<std::size_t>(j)].data());
        triPolygon->GetPointIds()->SetId(j, j);
      }
      triPolygon->EarCutTriangulation(triIds);
      const vtkIdType ntriIds = triIds->GetNumberOfIds();
      for (vtkIdType k = 0; k + 2 < ntriIds; k += 3)
      {
        pushEdge(poly[triIds->GetId(k)]);
        pushEdge(poly[triIds->GetId(k + 1)]);
        pushEdge(poly[triIds->GetId(k + 2)]);
        polygonsSize.push_back(3);
      }
    }
  }
  else
  {
    for (const auto& poly : trace.IsoPolygons)
    {
      const vtkIdType nv = static_cast<vtkIdType>(poly.size());
      if (nv < 3)
      {
        continue;
      }
      for (vtkIdType j = 0; j < nv; ++j)
      {
        pushEdge(poly[j]);
      }
      polygonsSize.push_back(nv);
    }
  }
}

//------------------------------------------------------------------------------
// New-style CountClip: takes vtkCellArray* faces + vtkDataArray* scalars.
// Points are not needed for counting (only scalar classification and edge
// endpoint IDs are required). A zero-filled dummy coordinate array is used
// to satisfy RunLopezTrace's signature; positions are not accessed for count.
//------------------------------------------------------------------------------
void vtkPolyhedronContour::CountClip(vtkIdType numPointIds, const vtkIdType* pointIds,
  vtkCellArray* polyhedronFaces, vtkDataArray* scalars, double isoValue, bool insideOut,
  vtkIdType& numOutputCells, vtkIdType& numOutputCellConnectivity, vtkIdType& numOutputFaces,
  vtkIdType& numOutputFacesConnectivity,
  std::vector<EdgeTuple<vtkIdType, double>>& intersectedEdges)
{
  numOutputCells = 0;
  numOutputCellConnectivity = 0;
  numOutputFaces = 0;
  numOutputFacesConnectivity = 0;
  intersectedEdges.clear();

  // Per-thread workspace — all per-cell scratch buffers amortize allocation
  // across cells processed by the same worker thread.
  thread_local PolyhedronWorkspace ws;

  // Build raw scalar array indexed by local index (0..numPointIds-1).
  auto& localScalars = ws.LocalScalars;
  localScalars.resize(numPointIds);
  for (vtkIdType i = 0; i < numPointIds; ++i)
  {
    localScalars[i] = scalars->GetComponent(pointIds[i], 0);
  }

  // Build global->local map for remapping face stream IDs.
  auto& globalToLocal = ws.GlobalToLocal;
  BuildLocalMap(pointIds, numPointIds, globalToLocal);

  // Build raw face stream from vtkCellArray, remapping global IDs to local.
  const vtkIdType nFaces = polyhedronFaces->GetNumberOfCells();
  auto& localFaceStream = ws.LocalFaceStream;
  polyhedronFaces->Dispatch(PolyhedronFacesToLocalFaceStream{}, globalToLocal, localFaceStream);

  auto& trace = ws.Trace;
  trace.Clear(nFaces);
  int numInside = 0, numOutside = 0;
  ClassifyVertices(trace, localScalars, isoValue, numInside, numOutside);
  // skip cells that don't straddle the iso-value.
  if (numInside == 0 || numOutside == 0)
  {
    return;
  }

  RunLopezTraceInto(trace, nFaces, localFaceStream.data(), localScalars, isoValue);

  const int keepTag = insideOut ? 0 : 1;

  // RunLopezTraceInto classifies on-surface vertices (s == isoValue) as tag=1,
  // so they are kept when !insideOut (keepTag=1) and dropped when insideOut
  // (keepTag=0). This matches TBC's pointsMap convention for !insideOut.
  // For insideOut=true, on-surface verts are NOT kept — the existing face
  // walker and coincidence fold in EmitClip handle that case correctly (iso-
  // vertices at t=0 at the on-surface endpoint route via the edge locator).

  vtkIdType nSurviving = 0;
  for (vtkIdType i = 0; i < numPointIds; ++i)
  {
    if (trace.VertexTags[i] == keepTag)
    {
      ++nSurviving;
    }
  }

  vtkIdType nIso = 0;
  for (const auto& iv : trace.IsoVertices)
  {
    if (trace.VertexTags[iv.OutsideVertex] != trace.VertexTags[iv.InsideVertex])
    {
      ++nIso;
    }
  }
  // A valid output polyhedron needs at least 3 vertices (degenerate flat
  // polygon is still a valid output face). Fewer indicates a truly degenerate
  // sliver — emit nothing.
  if (nSurviving + nIso < 3)
  {
    intersectedEdges.clear();
    return;
  }

  numOutputCells = 1;

  // Populate intersected edges. Skip two kinds of iso-vertices:
  //  (1) Degenerate: endpoints share a tag after re-tag.
  //  (2) Coincident: weight 0 or 1 with the coincident endpoint on-surface AND
  //      kept by TBC. Registering would allocate a second output point at the
  //      same spatial location, breaking face stitching.
  for (const auto& iv : trace.IsoVertices)
  {
    if (trace.VertexTags[iv.OutsideVertex] == trace.VertexTags[iv.InsideVertex])
    {
      continue;
    }
    const bool outsideOnSurface = (localScalars[iv.OutsideVertex] == isoValue);
    const bool insideOnSurface = (localScalars[iv.InsideVertex] == isoValue);
    const bool outsideKept = (trace.VertexTags[iv.OutsideVertex] == keepTag);
    const bool insideKept = (trace.VertexTags[iv.InsideVertex] == keepTag);
    if ((iv.Weight == 0.0 && outsideOnSurface && outsideKept) ||
      (iv.Weight == 1.0 && insideOnSurface && insideKept))
    {
      continue;
    }
    vtkIdType g0 = pointIds[iv.OutsideVertex];
    vtkIdType g1 = pointIds[iv.InsideVertex];
    if (g0 > g1)
    {
      std::swap(g0, g1);
      intersectedEdges.emplace_back(g0, g1, 1.0 - iv.Weight);
    }
    else
    {
      intersectedEdges.emplace_back(g0, g1, iv.Weight);
    }
  }
  // numOutputCellConnectivity counts ALL non-degenerate iso-vertices (nIso),
  // not just those registered with the edge locator (nIsoRegistered). Coincident
  // iso-verts (weight=0/1 at on-surface kept endpoint) skip locator registration
  // but EmitClip still pushes them to the cell connectivity via the coincidence
  // fold (folded to the kept endpoint's pointMap ID).
  numOutputCellConnectivity = nSurviving + nIso;

  // Count output faces. Mirror EmitClip's actual piece-building algorithm so
  // the two passes stay in lockstep.
  // Build the iso-vertex edge-key set so the face walker knows exactly which
  // tag-differing edges will produce an iso-vertex (a degenerate one after
  // re-tag will NOT, and CountClip must not reserve a slot for it).
  auto& liveIsoEdgeKeys = ws.LiveIsoEdgeKeys;
  liveIsoEdgeKeys.clear();
  for (const auto& iv : trace.IsoVertices)
  {
    if (trace.VertexTags[iv.OutsideVertex] != trace.VertexTags[iv.InsideVertex])
    {
      liveIsoEdgeKeys.insert(MakeEdgeKey(iv.OutsideVertex, iv.InsideVertex));
    }
  }

  // localFaceStream was already built with local IDs (0..N-1), so no further
  // remapping is needed in the face walker — stream values ARE local indices.
  auto& pieceSizes = ws.PieceSizes;
  const vtkIdType* fs = localFaceStream.data();
  for (vtkIdType faceIdx = 0; faceIdx < nFaces; ++faceIdx)
  {
    vtkIdType numFaceVerts = *fs++;
    pieceSizes.clear();
    vtkIdType currentPieceSize = 0;
    bool flushedAny = false;
    size_t firstPieceIdx = 0;

    for (vtkIdType i = 0; i < numFaceVerts; ++i)
    {
      int loc0 = static_cast<int>(fs[i]);
      int loc1 = static_cast<int>(fs[(i + 1) % numFaceVerts]);
      // Values above numPointIds indicate a stream entry we couldn't remap
      // (shouldn't happen for valid polyhedra, but defend).
      if (loc0 < 0 || loc0 >= numPointIds || loc1 < 0 || loc1 >= numPointIds)
      {
        continue;
      }

      if (trace.VertexTags[loc0] == keepTag)
      {
        ++currentPieceSize;
      }

      if (trace.VertexTags[loc0] != trace.VertexTags[loc1])
      {
        if (liveIsoEdgeKeys.count(MakeEdgeKey(loc0, loc1)))
        {
          ++currentPieceSize;
        }
        if (trace.VertexTags[loc0] == keepTag)
        {
          pieceSizes.push_back(currentPieceSize);
          if (!flushedAny)
          {
            firstPieceIdx = pieceSizes.size() - 1;
            flushedAny = true;
          }
          currentPieceSize = 0;
        }
      }
    }
    if (currentPieceSize > 0 && flushedAny && firstPieceIdx < pieceSizes.size())
    {
      pieceSizes[firstPieceIdx] += currentPieceSize;
    }
    else if (currentPieceSize > 0)
    {
      pieceSizes.push_back(currentPieceSize);
    }

    for (vtkIdType sz : pieceSizes)
    {
      if (sz >= 3)
      {
        ++numOutputFaces;
        numOutputFacesConnectivity += sz;
      }
    }
    fs += numFaceVerts;
  }
  for (const auto& poly : trace.IsoPolygons)
  {
    vtkIdType nv = static_cast<vtkIdType>(poly.size());
    if (nv < 3)
    {
      continue;
    }
    // Mirror EmitClip's cap-face validity check: a cap face is only emitted if
    // every iso-vertex has a valid output ID. A degenerate iso-vertex (endpoints
    // sharing a tag after the boundary re-tag) folds onto the kept endpoint; if
    // neither endpoint is kept (both have tag != keepTag, i.e. both were dropped
    // by TBC), the iso-vertex has no valid output ID and the whole cap face is
    // dropped. Reserving a slot for it here would leave a hole in the output
    // face arrays.
    bool capValid = true;
    for (int ivIdx : poly)
    {
      const auto& iv = trace.IsoVertices[ivIdx];
      int tOut = trace.VertexTags[iv.OutsideVertex];
      int tIn = trace.VertexTags[iv.InsideVertex];
      if (tOut == tIn)
      {
        // Degenerate: only valid if the shared-tag endpoint is kept.
        if (tOut != keepTag)
        {
          capValid = false;
          break;
        }
      }
      // Non-degenerate iso-vertices are always valid (they resolve via the
      // global edge locator in EmitClip; intersectedEdges above registered them).
    }
    if (!capValid)
    {
      continue;
    }
    ++numOutputFaces;
    numOutputFacesConnectivity += nv;
  }
}

//------------------------------------------------------------------------------
// New-style EmitClip: takes vtkCellArray* faces + vtkDataArray* scalars.
// Uses pointMap (input global ID -> output ID) and edgeLocator for iso-vertices.
//------------------------------------------------------------------------------
void vtkPolyhedronContour::EmitClip(vtkIdType numPointIds, const vtkIdType* pointIds,
  vtkCellArray* polyhedronFaces, vtkDataArray* scalars, double isoValue, bool insideOut,
  vtkDataArray* pointMap, vtkIdType numberOfKeptPoints,
  const vtkStaticEdgeLocatorTemplate<vtkIdType, double>& edgeLocator, vtkCellArray* outputCells,
  vtkCellArray* outputFaces)
{
  outputCells->Reset();
  outputFaces->Reset();

  // Per-thread workspace — see CountClip for the rationale.
  thread_local PolyhedronWorkspace ws;

  // Build local scalars indexed 0..numPointIds-1
  auto& localScalars = ws.LocalScalars;
  localScalars.resize(numPointIds);
  for (vtkIdType i = 0; i < numPointIds; ++i)
  {
    localScalars[i] = scalars->GetComponent(pointIds[i], 0);
  }

  // Build global->local map for remapping face stream IDs.
  auto& globalToLocal = ws.GlobalToLocal;
  BuildLocalMap(pointIds, numPointIds, globalToLocal);

  // Build raw face stream, remapping global IDs to local.
  const vtkIdType nFaces = polyhedronFaces->GetNumberOfCells();
  auto& localFaceStream = ws.LocalFaceStream;
  polyhedronFaces->Dispatch(PolyhedronFacesToLocalFaceStream{}, globalToLocal, localFaceStream);

  auto& trace = ws.Trace;
  trace.Clear(nFaces);
  int numInside = 0, numOutside = 0;
  ClassifyVertices(trace, localScalars, isoValue, numInside, numOutside);
  // skip cells that don't straddle the iso-value.
  if (numInside == 0 || numOutside == 0)
  {
    return;
  }

  RunLopezTraceInto(trace, nFaces, localFaceStream.data(), localScalars, isoValue);

  const int keepTag = insideOut ? 0 : 1;

  // RunLopezTraceInto classifies on-surface vertices as tag=1 — see CountClip
  // for the matching comment. The logic below (face walker, iso-vertex output
  // ID mapping, and coincidence fold) handles on-surface verts correctly
  // because they show up in the face walker as kept (for !insideOut) and the
  // (below,on) edge iso-vertices at t=1 route via the edge locator.

  // Build local->output ID map for surviving vertices (via pointMap lookup).
  // Iso-vertex output IDs come through isoToOutputId (built next).
  auto& localToOutputId = ws.LocalToOutputId;
  localToOutputId.assign(numPointIds, -1);
  for (vtkIdType i = 0; i < numPointIds; ++i)
  {
    if (trace.VertexTags[i] == keepTag)
    {
      localToOutputId[i] = static_cast<vtkIdType>(pointMap->GetComponent(pointIds[i], 0));
    }
  }

  // Map iso-vertices to output IDs. Three cases:
  //  (1) Degenerate (endpoints share a tag after re-tag): fold onto the kept endpoint.
  //  (2) Coincident (weight 0 or 1, on-surface endpoint kept by TBC):
  //      fold onto that kept endpoint — must match CountClip which skipped locator
  //      registration for this iso-vertex.
  //  (3) Normal crossing: resolve via the edge locator to (numberOfKeptPoints + edgeId).
  auto& isoToOutputId = ws.IsoToOutputId;
  isoToOutputId.assign(trace.IsoVertices.size(), -1);
  for (size_t i = 0; i < trace.IsoVertices.size(); ++i)
  {
    const auto& iv = trace.IsoVertices[i];
    if (trace.VertexTags[iv.OutsideVertex] == trace.VertexTags[iv.InsideVertex])
    {
      vtkIdType keptLocal =
        (trace.VertexTags[iv.OutsideVertex] == keepTag) ? iv.OutsideVertex : iv.InsideVertex;
      if (trace.VertexTags[keptLocal] == keepTag)
      {
        isoToOutputId[i] = localToOutputId[keptLocal];
      }
      continue;
    }
    // Coincidence check — MUST match CountClip exactly.
    const bool outsideOnSurface = (localScalars[iv.OutsideVertex] == isoValue);
    const bool insideOnSurface = (localScalars[iv.InsideVertex] == isoValue);
    const bool outsideKept = (trace.VertexTags[iv.OutsideVertex] == keepTag);
    const bool insideKept = (trace.VertexTags[iv.InsideVertex] == keepTag);
    if (iv.Weight == 0.0 && outsideOnSurface && outsideKept)
    {
      isoToOutputId[i] = localToOutputId[iv.OutsideVertex];
    }
    else if (iv.Weight == 1.0 && insideOnSurface && insideKept)
    {
      isoToOutputId[i] = localToOutputId[iv.InsideVertex];
    }
    else
    {
      vtkIdType g0 = pointIds[iv.OutsideVertex];
      vtkIdType g1 = pointIds[iv.InsideVertex];
      vtkIdType edgeId = edgeLocator.IsInsertedEdge(g0, g1);
      if (edgeId >= 0)
      {
        isoToOutputId[i] = numberOfKeptPoints + edgeId;
      }
    }
  }

  // Build edge key -> iso output ID map for face walking
  auto& edgeToOutputId = ws.EdgeToOutputId;
  edgeToOutputId.clear();
  for (size_t i = 0; i < trace.IsoVertices.size(); ++i)
  {
    const auto& iv = trace.IsoVertices[i];
    if (isoToOutputId[i] >= 0)
    {
      edgeToOutputId[MakeEdgeKey(iv.OutsideVertex, iv.InsideVertex)] = isoToOutputId[i];
    }
  }

  // Collect output point IDs for the cell connectivity (output IDs — TBC copies directly).
  // Skip degenerate iso-vertices: they fold onto a kept vertex already listed above.
  auto& cellPts = ws.CellPts;
  cellPts.clear();
  for (vtkIdType i = 0; i < numPointIds; ++i)
  {
    if (trace.VertexTags[i] == keepTag)
    {
      cellPts.push_back(localToOutputId[i]);
    }
  }
  for (size_t i = 0; i < trace.IsoVertices.size(); ++i)
  {
    const auto& iv = trace.IsoVertices[i];
    if (trace.VertexTags[iv.OutsideVertex] == trace.VertexTags[iv.InsideVertex])
    {
      continue;
    }
    if (isoToOutputId[i] >= 0)
    {
      cellPts.push_back(isoToOutputId[i]);
    }
  }
  // Match CountClip's degeneracy guard.
  if (cellPts.size() < 3)
  {
    return;
  }
  outputCells->InsertNextCell(static_cast<vtkIdType>(cellPts.size()), cellPts.data());

  // Emit clipped faces. localFaceStream holds local indices already.
  auto& pieces = ws.Pieces;
  auto& currentPiece = ws.CurrentPiece;
  const vtkIdType* fs = localFaceStream.data();
  for (vtkIdType faceIdx = 0; faceIdx < nFaces; ++faceIdx)
  {
    vtkIdType numFaceVerts = *fs++;
    // Clear + reuse inner pieces. pieces.clear() retains outer capacity;
    // inner piece buffers are kept via manual clear-each.
    for (auto& p : pieces)
    {
      p.clear();
    }
    pieces.clear();
    currentPiece.clear();
    bool flushedAny = false;
    size_t firstPieceIdx = 0;

    for (vtkIdType i = 0; i < numFaceVerts; ++i)
    {
      int loc0 = static_cast<int>(fs[i]);
      int loc1 = static_cast<int>(fs[(i + 1) % numFaceVerts]);
      if (loc0 < 0 || loc0 >= numPointIds || loc1 < 0 || loc1 >= numPointIds)
      {
        continue;
      }

      if (trace.VertexTags[loc0] == keepTag)
      {
        currentPiece.push_back(localToOutputId[loc0]);
      }

      if (trace.VertexTags[loc0] != trace.VertexTags[loc1])
      {
        vtkIdType outsideLocal = (trace.VertexTags[loc0] == 0) ? loc0 : loc1;
        vtkIdType insideLocal = (trace.VertexTags[loc0] == 1) ? loc0 : loc1;
        auto eit = edgeToOutputId.find(MakeEdgeKey(outsideLocal, insideLocal));
        if (eit != edgeToOutputId.end())
        {
          currentPiece.push_back(eit->second);
        }

        if (trace.VertexTags[loc0] == keepTag)
        {
          pieces.push_back(std::move(currentPiece));
          if (!flushedAny)
          {
            firstPieceIdx = pieces.size() - 1;
            flushedAny = true;
          }
          currentPiece.clear();
        }
      }
    }
    if (!currentPiece.empty() && flushedAny && firstPieceIdx < pieces.size())
    {
      pieces[firstPieceIdx].insert(
        pieces[firstPieceIdx].begin(), currentPiece.begin(), currentPiece.end());
    }
    else if (!currentPiece.empty())
    {
      pieces.push_back(std::move(currentPiece));
    }

    for (const auto& piece : pieces)
    {
      if (piece.size() >= 3)
      {
        outputFaces->InsertNextCell(static_cast<vtkIdType>(piece.size()), piece.data());
      }
    }

    fs += numFaceVerts;
  }

  // Cap faces from iso-polygons — write iso-vertex output IDs directly.
  auto& capFace = ws.CapFace;
  for (const auto& poly : trace.IsoPolygons)
  {
    vtkIdType nv = static_cast<vtkIdType>(poly.size());
    if (nv < 3)
    {
      continue;
    }
    capFace.resize(nv);
    if (keepTag == 1)
    {
      for (vtkIdType j = 0; j < nv; ++j)
      {
        capFace[j] = isoToOutputId[poly[j]];
      }
    }
    else
    {
      for (vtkIdType j = 0; j < nv; ++j)
      {
        capFace[j] = isoToOutputId[poly[nv - 1 - j]];
      }
    }
    bool valid = true;
    for (auto id : capFace)
    {
      if (id < 0)
      {
        valid = false;
        break;
      }
    }
    if (valid)
    {
      outputFaces->InsertNextCell(nv, capFace.data());
    }
  }
}

VTK_ABI_NAMESPACE_END
