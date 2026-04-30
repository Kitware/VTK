// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPolyhedronContour
 * @brief   Isosurface extraction for arbitrary polyhedra using polygon tracing
 *
 * Implementation of the polygon-tracing algorithm for isosurface extraction:
 *
 *   J. López, A. Esteban, J. Hernández, P. Gómez, R. Zamora, C. Zanzi, F. Faura,
 *   "A new isosurface extraction method on arbitrary grids",
 *   Journal of Computational Physics, Volume 444, 2021, 110579.
 *   https://doi.org/10.1016/j.jcp.2021.110579
 *
 * Reference implementation (isoap library):
 *   https://doi.org/10.17632/4rcf98s74c.1
 *
 * The algorithm works by:
 * 1. Classifying vertices as inside (scalar > isovalue) or outside
 * 2. Finding iso-vertices where edges cross the isosurface
 * 3. Tracing closed polygons through the iso-vertices using face adjacency
 *
 * Key insight: Each iso-vertex has a "key face" where the edge transitions
 * from outside→inside. Polygon tracing simply walks to the next iso-vertex
 * on each key face until returning to the start.
 */

#ifndef vtkPolyhedronContour_h
#define vtkPolyhedronContour_h

#include "vtkCommonDataModelModule.h"     // For export macro
#include "vtkStaticEdgeLocatorTemplate.h" // For vtkStaticEdgeLocatorTemplate
#include "vtkType.h"                      // For vtkIdType

#include <array>         // For array
#include <cstdint>       // For int64_t
#include <unordered_map> // For unordered_map
#include <vector>        // For vector

VTK_ABI_NAMESPACE_BEGIN

class vtkCellArray;
class vtkCellData;
class vtkDataArray;
class vtkIncrementalPointLocator;
class vtkPointData;
class vtkPolyhedron;

/**
 * Isosurface extraction using López polygon tracing algorithm
 */
class VTKCOMMONDATAMODEL_EXPORT vtkPolyhedronContour
{
public:
  vtkPolyhedronContour() = default;
  ~vtkPolyhedronContour() = default;

  enum class CellClassification
  {
    AllInside,
    AllOutside,
    Intersected
  };

  /**
   * Execute isosurface extraction on a polyhedron cell.
   *
   * @param cell The polyhedron cell
   * @param scalars Scalar values at cell vertices (local indexing)
   * @param isoValue The isosurface value
   * @param locator Point locator for output points
   * @param inPd Input point data for interpolation
   * @param outPd Output point data
   * @return Cell classification (AllInside, AllOutside, or Intersected)
   */
  CellClassification Execute(vtkPolyhedron* cell, vtkDataArray* scalars, double isoValue,
    vtkIncrementalPointLocator* locator, vtkPointData* inPd, vtkPointData* outPd);

  /**
   * Get the number of resulting iso-polygons
   */
  int GetNumberOfIsoPolygons() const { return static_cast<int>(this->Result.IsoPolygons.size()); }

  /**
   * Get iso-polygon vertex count
   */
  int GetIsoPolygonSize(int polyIdx) const
  {
    return static_cast<int>(this->Result.IsoPolygons[polyIdx].size());
  }

  /**
   * Get iso-polygon vertex (output point ID)
   */
  vtkIdType GetIsoPolygonVertex(int polyIdx, int vertIdx) const
  {
    return this->Result.IsoVertices[this->Result.IsoPolygons[polyIdx][vertIdx]].OutputPointId;
  }

  /**
   * Output contour polygons to cell array (triangulates if needed)
   */
  void OutputContours(vtkCellArray* polys, vtkCellData* inCd, vtkIdType cellId, vtkCellData* outCd);

  /**
   * Output clipped polyhedron faces to connectivity array.
   * If outFaces and outFaceLocs are provided, faces are written directly into
   * those arrays (bypassing the embedded face-stream format). The caller is
   * responsible for ensuring outFaces and outFaceLocs are initialized.
   * If null, falls back to the embedded polyhedron face-stream format.
   */
  void OutputClip(vtkPolyhedron* cell, vtkCellArray* connectivity,
    vtkIncrementalPointLocator* locator, vtkPointData* inPd, vtkPointData* outPd, vtkCellData* inCd,
    vtkIdType cellId, vtkCellData* outCd, int insideOut, vtkCellArray* outFaces = nullptr,
    vtkCellArray* outFaceLocs = nullptr);

  //
  // Bulk API for integration with threaded filters (vtkContour3DLinearGrid,
  // vtkTableBasedClipDataSet). Operates on raw point coordinates and face
  // streams without instantiating vtkPolyhedron objects.
  //
  // - Contour: single-call ContourCell helper below.
  // - Clip: two-pass CountClip / EmitClip, where CountClip produces the edge
  //   intersection list fed into a shared vtkStaticEdgeLocatorTemplate before
  //   EmitClip writes final geometry with deduplicated iso-vertex IDs.
  //============================================================================

  /**
   * ContourCell for a single polyhedron.
   *
   * @param numPointIds                 Number of unique points in this cell
   * @param pointIds                    Global point IDs for this cell, [numPointIds]
   * @param polyhedronFaces             Polyhedron faces
   * @param scalars                     Scalar array (indexed by global point ID)
   * @param isoValue                    Clip value
   * @param generateTriangles           If true, output.PolyConn is triangle soup.
   * @param polygonsSize                [out] Polygon vertex counts (num vertices per polygon, size
   *                                    NPolys). Make sure to clear it before adding.
   * @param intersectedEdges            [out] Intersected Edges defined as (globalPtId0,
   *                                    globalPtId1, t). Make sure to clear it before adding.
   */
  static void ContourCell(vtkIdType numPointIds, const vtkIdType* pointIds,
    vtkCellArray* polyhedronFaces, vtkDataArray* scalars, double isoValue, bool generateTriangles,
    std::vector<vtkIdType>& polygonsSize,
    std::vector<EdgeTuple<vtkIdType, double>>& intersectedEdges);

private:
  //============================================================================
  // Data Structures
  //============================================================================

  /**
   * Represents a point where an edge crosses the isosurface.
   * Each iso-vertex lies on exactly one edge, shared by exactly two faces.
   */
  struct IsoVertex
  {
    vtkIdType OutsideVertex;        ///< Edge endpoint with scalar <= isoValue
    vtkIdType InsideVertex;         ///< Edge endpoint with scalar > isoValue
    int KeyFace;                    ///< Face where edge goes outside→inside (used for tracing)
    int OtherFace;                  ///< Face where edge goes inside→outside
    double Weight;                  ///< Interpolation parameter t
    std::array<double, 3> Position; ///< Interpolated 3D coordinates
    vtkIdType OutputPointId;        ///< Point ID in output mesh
  };

  struct TraceResult
  {
    /// Classification of each vertex: 0 = outside (scalar <= isoValue), 1 = inside
    std::vector<int> VertexTags;

    /// All iso-vertices created during extraction
    std::vector<IsoVertex> IsoVertices;

    /// For each face: ordered list of iso-vertex indices on that face
    std::vector<std::vector<int>> FaceIsoVertices;

    /// For each face: maps iso-vertex index → position in FaceIsoVertices[face]
    /// Used during polygon tracing to find the previous iso-vertex
    std::vector<std::vector<int>> IsoVertexPositionInFace;

    /// Each iso-polygon is a list of iso-vertex indices forming a closed loop.
    /// Traced around inside regions using KeyFace adjacency.
    std::vector<std::vector<int>> IsoPolygons;

    /// Edge to Iso vertex
    std::unordered_map<int64_t, int> EdgeToIsoVert;

    // Assigne vertices
    std::vector<bool> Assigned;

    void Clear(vtkIdType nFaces)
    {
      this->VertexTags.clear();
      this->IsoVertices.clear();
      this->FaceIsoVertices.resize(nFaces);
      for (vtkIdType i = 0; i < nFaces; ++i)
      {
        this->FaceIsoVertices[i].clear();
      }
      this->IsoVertexPositionInFace.resize(nFaces);
      for (vtkIdType i = 0; i < nFaces; ++i)
      {
        this->IsoVertexPositionInFace[i].clear();
      }
      this->IsoPolygons.clear();
      this->EdgeToIsoVert.clear();
      this->Assigned.clear();
    }
  };

  TraceResult Result;

  //============================================================================
  // Shared algorithm core (static — used by both the instance path and the
  // bulk static API).
  //============================================================================

  /**
   * Step 1: Classify vertices as inside (tag=1) or outside (tag=0).
   *
   * On-surface vertices (s == isoValue) are classified as "inside" (tag=1) so
   * that edges between on-surface and strictly-below vertices generate
   * iso-vertices at t=1. This lets ambiguous cells (with only on-surface +
   * below-iso vertices, or only on-surface + above-iso vertices) produce a
   * valid cap face rather than being dropped.
   *
   * Writes result.VertexTags. Returns counts via out-params.
   */
  static void ClassifyVertices(TraceResult& result, const std::vector<double>& localScalars,
    double isoValue, int& numInside, int& numOutside);

  /**
   * Step 2: Walk the face stream and create iso-vertices for each edge that
   * crosses the isosurface. Populates result.IsoVertices (with OutsideVertex,
   * InsideVertex, KeyFace, OtherFace, Weight) and result.FaceIsoVertices /
   * result.IsoVertexPositionInFace. Position and OutputPointId are NOT set
   * here — callers that need 3D coordinates must fill them separately.
   */
  static void FindIsoVertices(TraceResult& result, vtkIdType nFaces,
    const vtkIdType* localFaceStream, const std::vector<double>& localScalars, double isoValue);

  /**
   * Step 3: Trace closed iso-polygons through the iso-vertex graph using
   * KeyFace adjacency (with OtherFace reverse-walk fallback). Populates
   * result.IsoPolygons.
   */
  static void TraceIsoPolygons(TraceResult& result);

  /**
   * Run the trace (find iso-vertices + trace iso-polygons), assuming the cell
   * has already been classified and is known to intersect the iso-surface.
   *
   * Preconditions the caller MUST satisfy:
   *   - result.Clear(nFaces) has been called
   *   - ClassifyVerticesStatic has populated result.VertexTags
   *   - The cell straddles the iso-value (numInside > 0 && numOutside > 0)
   *   - localFaceStream has been built
   *
   * Postcondition:
   *   - result.IsoPolygons holds the traced polygons (possibly empty if the
   *     cell straddled the iso-value but no closed polygons were produced).
   *
   * Splitting classify out of this function lets the instance API skip the
   * face-stream build entirely for AllInside / AllOutside cells.
   */
  static void RunLopezTraceInto(TraceResult& result, vtkIdType nFaces,
    const vtkIdType* localFaceStream, const std::vector<double>& localScalars, double isoValue);

  //============================================================================
  // Instance-only helpers
  //============================================================================

  /**
   * After the trace has produced iso-vertices (with Weight set), interpolate
   * 3D positions, register them with the locator, and interpolate point data.
   * Fills IsoVertex::Position and IsoVertex::OutputPointId.
   */
  void InterpolatePositionsAndInsertPoints(vtkPolyhedron* cell, vtkIncrementalPointLocator* locator,
    vtkPointData* inPd, vtkPointData* outPd);

  //============================================================================
  // Internal Data structured for ContourCell/CountClip/EmitClip
  //============================================================================

  // Per-cell scratch for the bulk Contour/clip API. Held in a
  // thread_local so allocations amortize across cells processed by the
  // same worker thread. Capacities grow to the largest cell seen and are
  // retained thereafter.
  struct PolyhedronWorkspace
  {
    std::vector<double> LocalScalars;
    std::vector<vtkIdType> LocalFaceStream; // face-stream remap: global ID -> local 0..N-1
    std::unordered_map<vtkIdType, int> GlobalToLocal;
    TraceResult Trace; // RunLopezTrace output reused
  };
};

VTK_ABI_NAMESPACE_END
#endif // vtkPolyhedronContour_h
