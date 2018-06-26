/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolyhedron.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPolyhedron
 * @brief   a 3D cell defined by a set of polygonal faces
 *
 * vtkPolyhedron is a concrete implementation that represents a 3D cell
 * defined by a set of polygonal faces. The polyhedron should be watertight,
 * non-self-intersecting and manifold (each edge is used twice).
 *
 * Interpolation functions and weights are defined / computed using the
 * method of Mean Value Coordinates (MVC). See the VTK class
 * vtkMeanValueCoordinatesInterpolator for more information.
 *
 * The class does not require the polyhedron to be convex. However, the
 * polygonal faces must be planar. Non-planar polygonal faces will
 * definitely cause problems, especially in severely warped situations.
 *
 * @sa
 * vtkCell3D vtkConvecPointSet vtkMeanValueCoordinatesInterpolator
*/

#ifndef vtkPolyhedron_h
#define vtkPolyhedron_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkCell3D.h"

class vtkIdTypeArray;
class vtkCellArray;
class vtkTriangle;
class vtkQuad;
class vtkTetra;
class vtkPolygon;
class vtkLine;
class vtkPointIdMap;
class vtkIdToIdVectorMapType;
class vtkIdToIdMapType;
class vtkEdgeTable;
class vtkPolyData;
class vtkCellLocator;
class vtkGenericCell;
class vtkPointLocator;

class VTKCOMMONDATAMODEL_EXPORT vtkPolyhedron : public vtkCell3D
{
public:
  //@{
  /**
   * Standard new methods.
   */
  static vtkPolyhedron *New();
  vtkTypeMacro(vtkPolyhedron,vtkCell3D);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  //@}

  /**
   * See vtkCell3D API for description of these methods.
   */
  void GetEdgePoints(int vtkNotUsed(edgeId), int* &vtkNotUsed(pts)) override {}
  void GetFacePoints(int vtkNotUsed(faceId), int* &vtkNotUsed(pts)) override {}
  double *GetParametricCoords() override;

  /**
   * See the vtkCell API for descriptions of these methods.
   */
  int GetCellType() override {return VTK_POLYHEDRON;}

  /**
   * This cell requires that it be initialized prior to access.
   */
  int RequiresInitialization() override {return 1;}
  void Initialize() override;

  //@{
  /**
   * A polyhedron is represented internally by a set of polygonal faces.
   * These faces can be processed to explicitly determine edges.
   */
  int GetNumberOfEdges() override;
  vtkCell *GetEdge(int) override;
  int GetNumberOfFaces() override;
  vtkCell *GetFace(int faceId) override;
  //@}

  /**
   * Satisfy the vtkCell API. This method contours the input polyhedron and outputs
   * a polygon. When the result polygon is not planar, it will be triangulated.
   * The current implementation assumes water-tight polyhedron cells.
   */
  void Contour(double value, vtkDataArray *scalars,
               vtkIncrementalPointLocator *locator, vtkCellArray *verts,
               vtkCellArray *lines, vtkCellArray *polys,
               vtkPointData *inPd, vtkPointData *outPd,
               vtkCellData *inCd, vtkIdType cellId, vtkCellData *outCd) override;

  /**
   * Satisfy the vtkCell API. This method clips the input polyhedron and outputs
   * a new polyhedron. The face information of the output polyhedron is encoded
   * in the output vtkCellArray using a special format:
   * CellLength [nCellFaces, nFace0Pts, i, j, k, nFace1Pts, i, j, k, ...].
   * Use the static method vtkUnstructuredGrid::DecomposePolyhedronCellArray
   * to convert it into a standard format. Note: the algorithm assumes water-tight
   * polyhedron cells.
   */
  void Clip(double value, vtkDataArray *scalars,
            vtkIncrementalPointLocator *locator, vtkCellArray *connectivity,
            vtkPointData *inPd, vtkPointData *outPd,
            vtkCellData *inCd, vtkIdType cellId, vtkCellData *outCd,
            int insideOut) override;

  /**
   * Satisfy the vtkCell API. The subId is ignored and zero is always
   * returned. The parametric coordinates pcoords are normalized values in
   * the bounding box of the polyhedron. The weights are determined by
   * evaluating the MVC coordinates. The dist is always zero if the point x[3]
   * is inside the polyhedron; otherwise it's the distance to the surface.
   */
  int EvaluatePosition(const double x[3], double closestPoint[3],
                       int& subId, double pcoords[3],
                       double& dist2, double weights[]) override;

  /**
   * The inverse of EvaluatePosition. Note the weights should be the MVC
   * weights.
   */
  void EvaluateLocation(int& subId, const double pcoords[3], double x[3],
                        double *weights) override;

  /**
   * Intersect the line (p1,p2) with a given tolerance tol to determine a
   * point of intersection x[3] with parametric coordinate t along the
   * line. The parametric coordinates are returned as well (subId can be
   * ignored). Returns true if the line intersects a face.
   */
  int IntersectWithLine(const double p1[3], const double p2[3], double tol, double& t,
                        double x[3], double pcoords[3], int& subId) override;

  /**
   * Use vtkOrderedTriangulator to tetrahedralize the polyhedron mesh. This
   * method works well for a convex polyhedron but may return wrong result
   * in a concave case.
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
   */
  int Triangulate(int index, vtkIdList *ptIds, vtkPoints *pts) override;

  /**
   * Computes derivatives at the point specified by the parameter coordinate.
   * Current implementation uses all vertices and subId is not used.
   * To accelerate the speed, the future implementation can triangulate and
   * extract the local tetrahedron from subId and pcoords, then evaluate
   * derivatives on the local tetrahedron.
   */
  void Derivatives(int subId, const double pcoords[3], const double *values,
                   int dim, double *derivs) override;

  /**
   * Find the boundary face closest to the point defined by the pcoords[3]
   * and subId of the cell (subId can be ignored).
   */
  int CellBoundary(int subId, const double pcoords[3], vtkIdList *pts) override;

  /**
   * Return the center of the cell in parametric coordinates. In this cell,
   * the center of the bounding box is returned.
   */
  int GetParametricCenter(double pcoords[3]) override;

  /**
   * A polyhedron is a full-fledged primary cell.
   */
  int IsPrimaryCell() override {return 1;}

  //@{
  /**
   * Compute the interpolation functions/derivatives
   * (aka shape functions/derivatives). Here we use the MVC calculation
   * process to compute the interpolation functions.
   */
  void InterpolateFunctions(const double x[3], double *sf) override;
  void InterpolateDerivs(const double x[3], double *derivs) override;
  //@}

  //@{
  /**
   * Methods supporting the definition of faces. Note that the GetFaces()
   * returns a list of faces in vtkCellArray form; use the method
   * GetNumberOfFaces() to determine the number of faces in the list.
   * The SetFaces() method is also in vtkCellArray form, except that it
   * begins with a leading count indicating the total number of faces in
   * the list.
   */
  int RequiresExplicitFaceRepresentation() override {return 1;}
  void SetFaces(vtkIdType *faces) override;
  vtkIdType *GetFaces() override;
  //@}

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

protected:
  vtkPolyhedron();
  ~vtkPolyhedron() override;

  // Internal classes for supporting operations on this cell
  vtkLine        *Line;
  vtkTriangle    *Triangle;
  vtkQuad        *Quad;
  vtkPolygon     *Polygon;
  vtkTetra       *Tetra;
  vtkIdTypeArray *GlobalFaces; //these are numbered in global id space
  vtkIdTypeArray *FaceLocations;

  // vtkCell has the data members Points (x,y,z coordinates) and PointIds
  // (global cell ids corresponding to cell canonical numbering (0,1,2,....)).
  // These data members are implicitly organized in canonical space, i.e., where
  // the cell point ids are (0,1,...,npts-1). The PointIdMap maps global point id
  // back to these canonoical point ids.
  vtkPointIdMap  *PointIdMap;

  // If edges are needed. Note that the edge numbering is in
  // canonical space.
  int             EdgesGenerated; //true/false
  vtkEdgeTable   *EdgeTable; //keep track of all edges
  vtkIdTypeArray *Edges; //edge pairs kept in this list, in canonical id space
  vtkIdTypeArray *EdgeFaces; // face pairs that comprise each edge, with the
                             // same ordering as EdgeTable
  int             GenerateEdges(); //method populates the edge table and edge array

  // If faces need renumbering into canonical numbering space these members
  // are used. When initiallly loaded, the face numbering uses global dataset
  // ids. Once renumbered, they are converted to canonical space.
  vtkIdTypeArray *Faces; //these are numbered in canonical id space
  int             FacesGenerated;
  void            GenerateFaces();

  // Bounds management
  int    BoundsComputed;
  void   ComputeBounds();
  void   ComputeParametricCoordinate(const double x[3], double pc[3]);
  void   ComputePositionFromParametricCoordinate(const double pc[3], double x[3]);

  // Members for supporting geometric operations
  int             PolyDataConstructed;
  vtkPolyData    *PolyData;
  vtkCellArray   *Polys;
  vtkIdTypeArray *PolyConnectivity;
  void            ConstructPolyData();
  int             LocatorConstructed;
  vtkCellLocator *CellLocator;
  void            ConstructLocator();
  vtkIdList      *CellIds;
  vtkGenericCell *Cell;

private:
  vtkPolyhedron(const vtkPolyhedron&) = delete;
  void operator=(const vtkPolyhedron&) = delete;

};

//----------------------------------------------------------------------------
inline int vtkPolyhedron::GetParametricCenter(double pcoords[3])
{
  pcoords[0] = pcoords[1] = pcoords[2] = 0.5;
  return 0;
}

#endif
