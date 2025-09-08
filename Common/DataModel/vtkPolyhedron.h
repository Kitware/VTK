// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPolyhedron
 * @brief   A 3D cell defined by a set of polygonal faces
 *
 * @section Instantiation Instantiation
 *
 * vtkPolyhedron is a concrete implementation of vtkCell that represents a 3D cell
 * defined by a set of polygonal faces.
 *
 * To instantiate a vtkPolyhedron, like any vtkCell, one needs to define the
 * following structures:
 * - A list of point coordinates
 * - A list of global point IDs
 *
 * Note that the ordering of points coordinates or IDs is not important.
 * However, it MUST be consistent between the two lists.
 *
 * Unlike other kinds of cells (e.g. vtkVoxel), the topology is not directly deduced from points
 * coordinates or point IDs ordering; it must be explicitly defined by providing a list of faces
 * (see the SetFaces() method). Each face is represented as a sequence of global point Ids.
 *
 * Once point coordinates, point IDs and faces are defined, the Initialize() method should be called
 * in order to setup the internal structures and finalize the construction of the polyhedron.
 *
 * Here is an example of vtkPolyhedron instantiation:
 * @code{.cpp}
 *
 * //  9 +------+.11
 * //    |`.    | `.
 * //    |13`+--+---+ 15
 * //    |   |  |   |
 * //  8 +---+--+.10|
 * //     `. |    `.|
 * //     12`+------+ 14
 * //
 * // (Global IDs are arbitrarily chosen between 8 and 15)
 *
 * // Insert point coordinates
 * polyhedron->GetPoints()->SetNumberOfPoints(8);
 * polyhedron->GetPoints()->SetPoint(0, 0., 0., 0.); // 8
 * polyhedron->GetPoints()->SetPoint(1, 0., 1., 0.); // 9
 * polyhedron->GetPoints()->SetPoint(2, 1., 0., 0.); // 10
 * polyhedron->GetPoints()->SetPoint(3, 1., 1., 0.); // 11
 * polyhedron->GetPoints()->SetPoint(4, 0., 0., 1.); // 12
 * polyhedron->GetPoints()->SetPoint(5, 0., 1., 1.); // 13
 * polyhedron->GetPoints()->SetPoint(6, 1., 0., 1.); // 14
 * polyhedron->GetPoints()->SetPoint(7, 1., 1., 1.); // 15
 *
 * // Insert point IDs (global IDs)
 * // Note that the canonical ordering (0, 1, ..., 8) is used
 * // to correlate point Ids and coordinates
 * polyhedron->GetPointIds()->Allocate(8);
 * for (int i = 8; i < 16; ++i)
 * {
 *   polyhedron->GetPointIds()->InsertNextId(i);
 * }
 *
 * // Describe faces, indexed on global IDs
 * vtkIdType faces[31] = { 6, // Number of faces
 *                         4, 9 , 11, 10, 8 , // Number of points in the face + point IDs
 *                         4, 11, 15, 14, 10, // Faces are described counter-clockwise
 *                         4, 15, 13, 12, 14, // looking from the "outside" of the cell
 *                         4, 13, 9 , 8 , 12,
 *                         4, 14, 12, 8 , 10,
 *                         4, 13, 15, 11, 9 };
 *
 * polyhedron->SetFaces(faces);
 *
 * // Initialize the polyhedron
 * // This will build internal structures and should be done before the proper
 * // use of the cell.
 * polyhedron->Initialize();
 * @endcode
 *
 * @section Specifications Specifications
 *
 * Polyhedrons described by this class must conform to some criteria in order to avoid errors and
 * guarantee good results in terms of visualization and processing.
 *
 * These specifications are described as follows. Polyhedrons must:
 * - be watertight : the faces describing the polyhedron should define an enclosed volume
 *   i.e. define the “inside” and the “outside” of the cell
 * - have planar faces : all points defining a face should be in the same 2D plane
 * - not be self-intersecting : for example, a face of the polyhedron can’t intersect other ones
 * - not contain zero-thickness portions : adjacent faces should not overlap each other even
 * partially
 * - not contain disconnected elements : detached vertice(s), edge(s) or face(s)
 * - be simply connected : vtkPolyhedron must describe a single polyhedron
 * - not contain duplicate elements : each point index and each face description should be unique
 * - not contain “internal” or “external” faces : for each face, one side should be “inside” the
 * cell, the other side “outside”
 *
 * In a more global perspective, polyhedrons must be watertight and manifold.
 * In particular, each edge of the polyhedron must be adjacent to exactly two faces.
 * Several algorithms like contour, clip or slice will assume that each edge of the polyhedron
 * is adjacent to exactly two faces and will definitely lead to bad results (and generate numerous
 * warnings) if this criterion is not fulfilled.
 *
 * @section Limitations Limitations
 *
 * The class does not require the polyhedron to be convex. However, the support of concave
 * polyhedrons is currently limited. Concavity can lead to bad results with some filters,
 * including:
 * - Contour: the contour (surface) can be constructed outside of the cell,
 * - Triangulate: the current tetrahedralization algorithm can modify the initial
 *   shape of the polygon (created tetrahedrons can change concave portions of the shape
 *   to convex ones).
 *
 * @section OtherDetails Other details
 *
 * Interpolation functions and weights are defined / computed using the method of Mean Value
 * Coordinates (MVC). See the VTK class vtkMeanValueCoordinatesInterpolator for more information.
 *
 * @sa
 * vtkCell3D vtkConvexPointSet vtkMeanValueCoordinatesInterpolator vtkPolyhedronUtilities
 */

#ifndef vtkPolyhedron_h
#define vtkPolyhedron_h

#include "vtkCell3D.h"
#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkDeprecation.h"           // For VTK_DEPRECATED
#include "vtkNew.h"                   // For vtkNew

VTK_ABI_NAMESPACE_BEGIN
class vtkIdTypeArray;
class vtkCellArray;
class vtkTriangle;
class vtkQuad;
class vtkTetra;
class vtkPolygon;
class vtkLine;
class vtkEdgeTable;
class vtkPolyData;
class vtkCellLocator;
class vtkGenericCell;
class vtkPointLocator;
class vtkMinimalStandardRandomSequence;

class VTKCOMMONDATAMODEL_EXPORT vtkPolyhedron : public vtkCell3D
{
public:
  using vtkPointIdMap = std::map<vtkIdType, vtkIdType>;

  ///@{
  /**
   * Standard new methods.
   */
  static vtkPolyhedron* New();
  vtkTypeMacro(vtkPolyhedron, vtkCell3D);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  ///@{
  /**
   * See vtkCell3D API for description of these methods.
   * @warning These method are unimplemented in vtkPolyhedron
   */
  void GetEdgePoints(vtkIdType vtkNotUsed(edgeId), const vtkIdType*& vtkNotUsed(pts)) override
  {
    vtkWarningMacro(<< "vtkPolyhedron::GetEdgePoints Not Implemented");
  }
  vtkIdType GetFacePoints(vtkIdType vtkNotUsed(faceId), const vtkIdType*& vtkNotUsed(pts)) override
  {
    vtkWarningMacro(<< "vtkPolyhedron::GetFacePoints Not Implemented");
    return 0;
  }
  void GetEdgeToAdjacentFaces(
    vtkIdType vtkNotUsed(edgeId), const vtkIdType*& vtkNotUsed(pts)) override
  {
    vtkWarningMacro(<< "vtkPolyhedron::GetEdgeToAdjacentFaces Not Implemented");
  }
  vtkIdType GetFaceToAdjacentFaces(
    vtkIdType vtkNotUsed(faceId), const vtkIdType*& vtkNotUsed(faceIds)) override
  {
    vtkWarningMacro(<< "vtkPolyhedron::GetFaceToAdjacentFaces Not Implemented");
    return 0;
  }
  vtkIdType GetPointToIncidentEdges(
    vtkIdType vtkNotUsed(pointId), const vtkIdType*& vtkNotUsed(edgeIds)) override
  {
    vtkWarningMacro(<< "vtkPolyhedron::GetPointToIncidentEdges Not Implemented");
    return 0;
  }
  vtkIdType GetPointToIncidentFaces(vtkIdType pointId, const vtkIdType*& faceIds) override;
  vtkIdType GetPointToOneRingPoints(
    vtkIdType vtkNotUsed(pointId), const vtkIdType*& vtkNotUsed(pts)) override
  {
    vtkWarningMacro(<< "vtkPolyhedron::GetPointToOneRingPoints Not Implemented");
    return 0;
  }
  bool GetCentroid(double centroid[3]) const override;
  ///@}

  /**
   * See vtkCell3D API for description of this method.
   */
  double* GetParametricCoords() override;

  /**
   * See the vtkCell API for descriptions of these methods.
   */
  int GetCellType() override { return VTK_POLYHEDRON; }

  /**
   * This cell requires that it be initialized prior to access.
   */
  int RequiresInitialization() override { return 1; }

  /**
   * The Initialize method builds up internal structures of vtkPolyhedron.
   * @warning This method should be called after setting the point coordinates,
   * point IDs and faces of the polyhedron in order to finalize its construction.
   */
  void Initialize() override;

  ///@{
  /**
   * A polyhedron is represented internally by a set of polygonal faces.
   * These faces can be processed to explicitly determine edges.
   */
  int GetNumberOfEdges() override;
  vtkCell* GetEdge(int) override;
  int GetNumberOfFaces() override;
  vtkCell* GetFace(int faceId) override;
  ///@}

  /**
   * Satisfy the vtkCell API. This method contours the input polyhedron and outputs
   * a polygon. When the result polygon is not planar, it will be triangulated.
   * @warning The current implementation assumes water-tight and manifold polyhedron cells.
   */
  void Contour(double value, vtkDataArray* scalars, vtkIncrementalPointLocator* locator,
    vtkCellArray* verts, vtkCellArray* lines, vtkCellArray* polys, vtkPointData* inPd,
    vtkPointData* outPd, vtkCellData* inCd, vtkIdType cellId, vtkCellData* outCd) override;

  /**
   * Satisfy the vtkCell API. This method clips the input polyhedron and outputs
   * a new polyhedron. The face information of the output polyhedron is encoded
   * in the output vtkCellArray using a special format:
   * CellLength [nCellFaces, nFace0Pts, i, j, k, nFace1Pts, i, j, k, ...].
   * Use the static method vtkUnstructuredGrid::DecomposePolyhedronCellArray
   * to convert it into a standard format.
   * @warning The current implementation assumes water-tight and manifold polyhedron cells.
   */
  void Clip(double value, vtkDataArray* scalars, vtkIncrementalPointLocator* locator,
    vtkCellArray* connectivity, vtkPointData* inPd, vtkPointData* outPd, vtkCellData* inCd,
    vtkIdType cellId, vtkCellData* outCd, int insideOut) override;

  /**
   * Satisfy the vtkCell API. The subId is ignored and zero is always
   * returned. The parametric coordinates pcoords are normalized values in
   * the bounding box of the polyhedron. The weights are determined by
   * evaluating the MVC coordinates. The dist is always zero if the point x[3]
   * is inside the polyhedron; otherwise it's the distance to the surface.
   */
  int EvaluatePosition(const double x[3], double closestPoint[3], int& subId, double pcoords[3],
    double& dist2, double weights[]) override;

  /**
   * The inverse of EvaluatePosition. Note the weights should be the MVC
   * weights.
   */
  void EvaluateLocation(int& subId, const double pcoords[3], double x[3], double* weights) override;

  /**
   * Intersect the line (p1,p2) with a given tolerance tol to determine a
   * point of intersection x[3] with parametric coordinate t along the
   * line. The parametric coordinates are returned as well (subId can be
   * ignored). Returns true if the line intersects a face.
   */
  int IntersectWithLine(const double p1[3], const double p2[3], double tol, double& t, double x[3],
    double pcoords[3], int& subId) override;

  /**
   * Use vtkOrderedTriangulator to tetrahedralize the polyhedron mesh.
   * Once triangulation has been performed, the results are saved in ptIds and
   * pts. The ptIds is a vtkIdList with 4xn number of ids (n is the number of
   * result tetrahedrons). The first 4 represent the point ids of the first
   * tetrahedron, the second 4 represents the point ids of the second tetrahedron
   * and so on. The point ids represent global dataset ids.
   * The points of result tetrahedons are stored in pts. Note that there are
   * 4xm output points (m is the number of points in the original polyhedron).
   * A point may be stored multiple times when it is shared by more than one
   * tetrahedrons. The points stored in pts are ordered the same as they are
   * listed in ptIds.
   * @warning This method works well for a convex polyhedron but may return
   * wrong result in a concave case.
   */
  int TriangulateLocalIds(int index, vtkIdList* ptIds) override;

  /**
   * Triangulate each face of the polyhedron.
   * This method internally use the vtkCell::Triangulate method on each face (so the
   * triangulation method vary depending on the 2D cell type corresponding to the face).
   * @warning Can lead to bad results with non-planar faces.
   */
  int TriangulateFaces(vtkIdList* newFaces);

  /**
   * Triangulate each face of the polyhedron.
   * This method internally use the vtkCell::Triangulate method on each face (so the
   * triangulation method vary depending on the 2D cell type corresponding to the face).
   * @warning Can lead to bad results with non-planar faces.
   */
  int TriangulateFaces(vtkCellArray* newFaces);

  /**
   * Computes derivatives at the point specified by the parameter coordinate.
   * Current implementation uses all vertices and subId is not used.
   * To accelerate the speed, the future implementation can triangulate and
   * extract the local tetrahedron from subId and pcoords, then evaluate
   * derivatives on the local tetrahedron.
   */
  void Derivatives(
    int subId, const double pcoords[3], const double* values, int dim, double* derivs) override;

  /**
   * Find the boundary face closest to the point defined by the pcoords[3]
   * and subId of the cell (subId can be ignored).
   */
  int CellBoundary(int subId, const double pcoords[3], vtkIdList* pts) override;

  /**
   * Return the center of the cell in parametric coordinates. In this cell,
   * the parametric location (within its bounds) of the centroid of its points is returned.
   */
  int GetParametricCenter(double pcoords[3]) override;

  /**
   * A polyhedron is a full-fledged primary cell.
   */
  int IsPrimaryCell() VTK_FUTURE_CONST override { return 1; }

  ///@{
  /**
   * Compute the interpolation functions/derivatives
   * (aka shape functions/derivatives). Here we use the MVC calculation
   * process to compute the interpolation functions.
   */
  void InterpolateFunctions(const double x[3], double* sf) override;
  void InterpolateDerivs(const double x[3], double* derivs) override;
  ///@}

  /**
   * Satisfy the vtkCell API. Always return true, because vtkPolyhedron
   * needs explicit faces definition in order to describe the topology
   * of the cell.
   */
  int RequiresExplicitFaceRepresentation() VTK_FUTURE_CONST override { return 1; }

  /**
   * Set the faces of the polyhedron.
   * Face are expressed as sequences of <b> global point IDs </b>.
   * The SetFaces method will require a copy from internal unstructured grid layout.
   *
   * @param faces 1-dimensional array with the following structure :
   * ```
   * [ NbOfFaces,
   *   NbOfPtsFace1, face1Pt1, face1Pt2, …, face1PtNbOfPtsFace1,
   *   NbOfPtsFace2, face2Pt1, face2Pt2, …, face2PtNbOfPtsFace2,
   *   …,
   *   NbOfPtsFaceN, faceNPt1, faceNPt2, …, faceNPtNbOfPtsFaceN ]
   * ```
   * This ordering corresponds to the legacy vtkCellArray form, with in
   * addition a leading count indicating the total number of faces in
   * the list.
   */
  void SetFaces(vtkIdType* faces) override;

  /**
   * Get the faces of the polyhedron.
   * Face are expressed as sequences of <b> global point IDs </b>.
   *
   * @return A 1-dimentional array with the following structure :
   * ```
   * [ NbOfPtsFace1, face1Pt1, face1Pt2, …, face1PtNbOfPtsFace1,
   *   NbOfPtsFace2, face2Pt1, face2Pt2, …, face2PtNbOfPtsFace2,
   *   …,
   *   NbOfPtsFaceN, faceNPt1, faceNPt2, …, faceNPtNbOfPtsFaceN ]
   * ```
   * This ordering corresponds to the legacy vtkCellArray form.
   * Note that unlike the SetFaces method, the total faces number leading
   * count is missing. In order to get the number of faces, please use the
   * vtkPolyhedron::GetNumberOfFaces() method.
   */
  vtkIdType* GetFaces() override;

  /**
   * Set the faces of the polyhedron.
   * Symmetric method to <b> GetCellFaces </b>
   *
   * @param faces vtkCellArray that stores a contiguous list of polygonal faces
   *  with their corresponding global point IDs defining a polyhedron.
   */
  int SetCellFaces(vtkCellArray* faces);

  ///@{
  /**
   * Get the faces of the polyhedron.
   * Face are expressed as sequences of <b> global point IDs </b>.
   * The vtkCellArray stores the list of polygonal faces with their corresponding
   * global point IDs.
   */
  vtkCellArray* GetCellFaces();
  void GetCellFaces(vtkCellArray* faces);
  ///@}

  /**
   * A method particular to vtkPolyhedron. It determines whether a point x[3]
   * is inside the polyhedron or not (returns 1 is the point is inside, 0
   * otherwise). The tolerance is expressed in normalized space; i.e., a
   * fraction of the size of the bounding box.
   */
  int IsInside(const double x[3], double tolerance);

  /**
   * Determine whether or not a polyhedron is convex. This method is adapted
   * from Devillers et al., "Checking the Convexity of Polytopes and the
   * Planarity of Subdivisions", Computational Geometry, Volume 11, Issues
   * 3 - 4, December 1998, Pages 187 - 208.
   */
  bool IsConvex();

  /**
   * Construct polydata if no one exist, then return this->PolyData
   */
  vtkPolyData* GetPolyData();

  /**
   * Shallow copy of a polyhedron.
   */
  void ShallowCopy(vtkCell* c) override;

  /**
   * Deep copy of a polyhedron.
   */
  void DeepCopy(vtkCell* c) override;

protected:
  vtkPolyhedron();
  ~vtkPolyhedron() override;

  // Internal classes for supporting operations on this cell
  vtkNew<vtkLine> Line;
  vtkNew<vtkTriangle> Triangle;
  vtkNew<vtkQuad> Quad;
  vtkNew<vtkPolygon> Polygon;
  vtkNew<vtkTetra> Tetra;

  // Filled with the SetFaces method.
  // These faces are numbered in global id space
  vtkNew<vtkCellArray> GlobalFaces;

  // Backward compatibility
  vtkNew<vtkIdTypeArray> LegacyGlobalFaces;

  // If edges are needed. Note that the edge numbering is in canonical space.
  int EdgesGenerated = 0;           // true/false
  vtkNew<vtkEdgeTable> EdgeTable;   // keep track of all edges
  vtkNew<vtkIdTypeArray> Edges;     // edge pairs kept in this list, in canonical id space
  vtkNew<vtkIdTypeArray> EdgeFaces; // face pairs that comprise each edge, with the
                                    // same ordering as EdgeTable
  int GenerateEdges();              // method populates the edge table and edge array

  // Numerous methods needs faces to be numbered in the canonical space.
  // This method uses PointIdMap to fill the Faces member (faces described
  // with canonical IDs) from the GlobalFaces member (faces described with
  // global IDs).
  void GenerateFaces();
  vtkNew<vtkCellArray> Faces; // These are numbered in canonical id space
  int FacesGenerated = 0;     // True when Faces have been successfully constructed

  // Bounds management
  int BoundsComputed = 0;
  void ComputeBounds();
  void ComputeParametricCoordinate(const double x[3], double pc[3]);
  void ComputePositionFromParametricCoordinate(const double pc[3], double x[3]);

  VTK_DEPRECATED_IN_9_4_0("Use GeneratePointToIncidentFaces instead.")
  void GeneratePointToIncidentFacesAndValenceAtPoint() { this->GeneratePointToIncidentFaces(); }

  // Members for supporting geometric operations
  int PolyDataConstructed = 0;
  vtkNew<vtkPolyData> PolyData;
  void ConstructPolyData();
  int LocatorConstructed = 0;
  vtkNew<vtkCellLocator> CellLocator;
  void ConstructLocator();
  vtkNew<vtkIdList> CellIds;
  vtkNew<vtkGenericCell> Cell;

private:
  vtkPolyhedron(const vtkPolyhedron&) = delete;
  void operator=(const vtkPolyhedron&) = delete;

  friend class vtkPolyhedronUtilities;

  // vtkCell has the data members Points (x,y,z coordinates) and PointIds (global cell ids).
  // These data members are implicitly organized in canonical space, i.e., where the cell
  // point ids are (0,1,...,npts-1).
  // The PointIdMap is constructed during the call of the Initialize() method and maps global
  // point ids to the canonical point ids.
  vtkPointIdMap PointIdMap;

  void GeneratePointToIncidentFaces();

  // Members used in GetPointToIncidentFaces
  std::vector<std::vector<vtkIdType>> PointToIncidentFaces;

  vtkNew<vtkMinimalStandardRandomSequence> RandomSequence;
  std::atomic<bool> IsRandomSequenceSeedInitialized{ false };
};

VTK_ABI_NAMESPACE_END
#endif
