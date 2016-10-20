/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCell.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkCell
 * @brief   abstract class to specify cell behavior
 *
 * vtkCell is an abstract class that specifies the interfaces for data cells.
 * Data cells are simple topological elements like points, lines, polygons,
 * and tetrahedra of which visualization datasets are composed. In some
 * cases visualization datasets may explicitly represent cells (e.g.,
 * vtkPolyData, vtkUnstructuredGrid), and in some cases, the datasets are
 * implicitly composed of cells (e.g., vtkStructuredPoints).
 *
 * @warning
 * The \#define VTK_CELL_SIZE is a parameter used to construct cells and provide
 * a general guideline for controlling object execution. This parameter is
 * not a hard boundary: you can create cells with more points.
 *
 * @sa
 * vtkHexahedron vtkLine vtkPixel vtkPolyLine vtkPolyVertex
 * vtkPolygon vtkQuad vtkTetra vtkTriangle
 * vtkTriangleStrip vtkVertex vtkVoxel vtkWedge vtkPyramid
*/

#ifndef vtkCell_h
#define vtkCell_h

#define VTK_CELL_SIZE 512
#define VTK_TOL 1.e-05 // Tolerance for geometric calculation

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkObject.h"

#include "vtkIdList.h" // Needed for inline methods
#include "vtkCellType.h" // Needed to define cell types

class vtkCellArray;
class vtkCellData;
class vtkDataArray;
class vtkPointData;
class vtkIncrementalPointLocator;
class vtkPoints;

class VTKCOMMONDATAMODEL_EXPORT vtkCell : public vtkObject
{
public:
  vtkTypeMacro(vtkCell,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Initialize cell from outside with point ids and point
   * coordinates specified.
   */
  void Initialize(int npts, vtkIdType *pts, vtkPoints *p);

  /**
   * Initialize the cell with point coordinates specified. Note that this
   * simplified version of Initialize() assumes that the point ids are simply
   * the indices into the supplied points array. Make sure that the ordering
   * of the points is consistent with the definition of the cell.
   */
  void Initialize(int npts, vtkPoints *p);

  /**
   * Copy this cell by reference counting the internal data structures.
   * This is safe if you want a "read-only" copy. If you modify the cell
   * you might wish to use DeepCopy().
   */
  virtual void ShallowCopy(vtkCell *c);

  /**
   * Copy this cell by completely copying internal data structures. This is
   * slower but safer than ShallowCopy().
   */
  virtual void DeepCopy(vtkCell *c);

  /**
   * Return the type of cell.
   */
  virtual int GetCellType() = 0;

  /**
   * Return the topological dimensional of the cell (0,1,2, or 3).
   */
  virtual int GetCellDimension() = 0;

  /**
   * Non-linear cells require special treatment beyond the usual cell type
   * and connectivity list information.  Most cells in VTK are implicit
   * cells.
   */
  virtual int IsLinear() {return 1;}

  /**
   * Some cells require initialization prior to access. For example, they
   * may have to triangulate themselves or set up internal data structures.
   */
  virtual int RequiresInitialization() {return 0;}
  virtual void Initialize() {}

  /**
   * Explicit cells require additional representational information
   * beyond the usual cell type and connectivity list information.
   * Most cells in VTK are implicit cells.
   */
  virtual int IsExplicitCell() {return 0;}

  /**
   * Determine whether the cell requires explicit face representation, and
   * methods for setting and getting the faces (see vtkPolyhedron for example
   * usage of these methods).
   */
  virtual int RequiresExplicitFaceRepresentation() {return 0;}
  virtual void SetFaces(vtkIdType *vtkNotUsed(faces)) {}
  virtual vtkIdType *GetFaces() {return NULL;}

  /**
   * Get the point coordinates for the cell.
   */
  vtkPoints *GetPoints() {return this->Points;}

  /**
   * Return the number of points in the cell.
   */
  vtkIdType GetNumberOfPoints() {return this->PointIds->GetNumberOfIds();}

  /**
   * Return the number of edges in the cell.
   */
  virtual int GetNumberOfEdges() = 0;

  /**
   * Return the number of faces in the cell.
   */
  virtual int GetNumberOfFaces() = 0;

  /**
   * Return the list of point ids defining the cell.
   */
  vtkIdList *GetPointIds() {return this->PointIds;}

  /**
   * For cell point i, return the actual point id.
   */
  vtkIdType GetPointId(int ptId) {return this->PointIds->GetId(ptId);}

  /**
   * Return the edge cell from the edgeId of the cell.
   */
  virtual vtkCell *GetEdge(int edgeId) = 0;

  /**
   * Return the face cell from the faceId of the cell.
   */
  virtual vtkCell *GetFace(int faceId) = 0;

  /**
   * Given parametric coordinates of a point, return the closest cell
   * boundary, and whether the point is inside or outside of the cell. The
   * cell boundary is defined by a list of points (pts) that specify a face
   * (3D cell), edge (2D cell), or vertex (1D cell). If the return value of
   * the method is != 0, then the point is inside the cell.
   */
  virtual int CellBoundary(int subId, double pcoords[3], vtkIdList *pts) = 0;

  /**
   * Given a point x[3] return inside(=1), outside(=0) cell, or (-1)
   * computational problem encountered; evaluate
   * parametric coordinates, sub-cell id (!=0 only if cell is composite),
   * distance squared of point x[3] to cell (in particular, the sub-cell
   * indicated), closest point on cell to x[3] (unless closestPoint is null,
   * in which case, the closest point and dist2 are not found), and
   * interpolation weights in cell. (The number of weights is equal to the
   * number of points defining the cell). Note: on rare occasions a -1 is
   * returned from the method. This means that numerical error has occurred
   * and all data returned from this method should be ignored. Also,
   * inside/outside is determine parametrically. That is, a point is inside
   * if it satisfies parametric limits. This can cause problems for cells of
   * topological dimension 2 or less, since a point in 3D can project onto
   * the cell within parametric limits but be "far" from the cell.  Thus the
   * value dist2 may be checked to determine true in/out.
   */
  virtual int EvaluatePosition(double x[3], double* closestPoint,
                               int& subId, double pcoords[3],
                               double& dist2, double *weights) = 0;

  /**
   * Determine global coordinate (x[3]) from subId and parametric coordinates.
   * Also returns interpolation weights. (The number of weights is equal to
   * the number of points in the cell.)
   */
  virtual void EvaluateLocation(int& subId, double pcoords[3],
                                double x[3], double *weights) = 0;

  /**
   * Generate contouring primitives. The scalar list cellScalars are
   * scalar values at each cell point. The point locator is essentially a
   * points list that merges points as they are inserted (i.e., prevents
   * duplicates). Contouring primitives can be vertices, lines, or
   * polygons. It is possible to interpolate point data along the edge
   * by providing input and output point data - if outPd is NULL, then
   * no interpolation is performed. Also, if the output cell data is
   * non-NULL, the cell data from the contoured cell is passed to the
   * generated contouring primitives. (Note: the CopyAllocate() method
   * must be invoked on both the output cell and point data. The
   * cellId refers to the cell from which the cell data is copied.)
   */
  virtual void Contour(double value, vtkDataArray *cellScalars,
                       vtkIncrementalPointLocator *locator, vtkCellArray *verts,
                       vtkCellArray *lines, vtkCellArray *polys,
                       vtkPointData *inPd, vtkPointData *outPd,
                       vtkCellData *inCd, vtkIdType cellId,
                       vtkCellData *outCd) = 0;

  /**
   * Cut (or clip) the cell based on the input cellScalars and the
   * specified value. The output of the clip operation will be one or
   * more cells of the same topological dimension as the original cell.
   * The flag insideOut controls what part of the cell is considered inside -
   * normally cell points whose scalar value is greater than "value" are
   * considered inside. If insideOut is on, this is reversed. Also, if the
   * output cell data is non-NULL, the cell data from the clipped cell is
   * passed to the generated contouring primitives. (Note: the CopyAllocate()
   * method must be invoked on both the output cell and point data. The
   * cellId refers to the cell from which the cell data is copied.)
   */
  virtual void Clip(double value, vtkDataArray *cellScalars,
                    vtkIncrementalPointLocator *locator, vtkCellArray *connectivity,
                    vtkPointData *inPd, vtkPointData *outPd,
                    vtkCellData *inCd, vtkIdType cellId, vtkCellData *outCd,
                    int insideOut) = 0;

  /**
   * Intersect with a ray. Return parametric coordinates (both line and cell)
   * and global intersection coordinates, given ray definition p1[3], p2[3] and tolerance tol.
   * The method returns non-zero value if intersection occurs. A parametric distance t
   * between 0 and 1 along the ray representing the intersection point, the point coordinates
   * x[3] in data coordinates and also pcoords[3] in parametric coordinates. subId is the index
   * within the cell if a composed cell like a triangle strip.
   */
  virtual int IntersectWithLine(double p1[3], double p2[3],
                                double tol, double& t, double x[3],
                                double pcoords[3], int& subId) = 0;

  /**
   * Generate simplices of proper dimension. If cell is 3D, tetrahedron are
   * generated; if 2D triangles; if 1D lines; if 0D points. The form of the
   * output is a sequence of points, each n+1 points (where n is topological
   * cell dimension) defining a simplex. The index is a parameter that controls
   * which triangulation to use (if more than one is possible). If numerical
   * degeneracy encountered, 0 is returned, otherwise 1 is returned.
   * This method does not insert new points: all the points that define the
   * simplices are the points that define the cell.
   */
  virtual int Triangulate(int index, vtkIdList *ptIds, vtkPoints *pts) = 0;

  /**
   * Compute derivatives given cell subId and parametric coordinates. The
   * values array is a series of data value(s) at the cell points. There is a
   * one-to-one correspondence between cell point and data value(s). Dim is
   * the number of data values per cell point. Derivs are derivatives in the
   * x-y-z coordinate directions for each data value. Thus, if computing
   * derivatives for a scalar function in a hexahedron, dim=1, 8 values are
   * supplied, and 3 deriv values are returned (i.e., derivatives in x-y-z
   * directions). On the other hand, if computing derivatives of velocity
   * (vx,vy,vz) dim=3, 24 values are supplied ((vx,vy,vz)1, (vx,vy,vz)2,
   * ....()8), and 9 deriv values are returned
   * ((d(vx)/dx),(d(vx)/dy),(d(vx)/dz), (d(vy)/dx),(d(vy)/dy), (d(vy)/dz),
   * (d(vz)/dx),(d(vz)/dy),(d(vz)/dz)).
   */
  virtual void Derivatives(int subId, double pcoords[3], double *values,
                           int dim, double *derivs) = 0;


  /**
   * Compute cell bounding box (xmin,xmax,ymin,ymax,zmin,zmax). Copy result
   * into user provided array.
   */
  void GetBounds(double bounds[6]);


  /**
   * Compute cell bounding box (xmin,xmax,ymin,ymax,zmin,zmax). Return pointer
   * to array of six double values.
   */
  double *GetBounds();


  /**
   * Compute Length squared of cell (i.e., bounding box diagonal squared).
   */
  double GetLength2();


  /**
   * Return center of the cell in parametric coordinates.  Note that the
   * parametric center is not always located at (0.5,0.5,0.5). The return
   * value is the subId that the center is in (if a composite cell). If you
   * want the center in x-y-z space, invoke the EvaluateLocation() method.
   */
  virtual int GetParametricCenter(double pcoords[3]);


  /**
   * Return the distance of the parametric coordinate provided to the
   * cell. If inside the cell, a distance of zero is returned. This is
   * used during picking to get the correct cell picked. (The tolerance
   * will occasionally allow cells to be picked who are not really
   * intersected "inside" the cell.)
   */
  virtual double GetParametricDistance(double pcoords[3]);


  /**
   * Return whether this cell type has a fixed topology or whether the
   * topology varies depending on the data (e.g., vtkConvexPointSet).
   * This compares to composite cells that are typically composed of
   * primary cells (e.g., a triangle strip composite cell is made up of
   * triangle primary cells).
   */
  virtual int IsPrimaryCell() {return 1;}


  /**
   * Return a contiguous array of parametric coordinates of the points
   * defining this cell. In other words, (px,py,pz, px,py,pz, etc..)  The
   * coordinates are ordered consistent with the definition of the point
   * ordering for the cell. This method returns a non-NULL pointer when
   * the cell is a primary type (i.e., IsPrimaryCell() is true). Note that
   * 3D parametric coordinates are returned no matter what the topological
   * dimension of the cell.
   */
  virtual double *GetParametricCoords();

  /**
   * Compute the interpolation functions/derivatives
   * (aka shape functions/derivatives)
   * No-ops at this level. Typically overridden in subclasses.
   */
  virtual void InterpolateFunctions(double vtkNotUsed(pcoords)[3], double* vtkNotUsed(weight))
  {
  }
  virtual void InterpolateDerivs(double vtkNotUsed(pcoords)[3], double* vtkNotUsed(derivs))
  {
  }

  // left public for quick computational access
  vtkPoints *Points;
  vtkIdList *PointIds;

protected:
  vtkCell();
  ~vtkCell() VTK_OVERRIDE;

  double Bounds[6];

private:
  vtkCell(const vtkCell&) VTK_DELETE_FUNCTION;
  void operator=(const vtkCell&) VTK_DELETE_FUNCTION;
};

#endif
