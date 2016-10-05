/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDelaunay2D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkDelaunay2D
 * @brief   create 2D Delaunay triangulation of input points
 *
 * vtkDelaunay2D is a filter that constructs a 2D Delaunay triangulation from
 * a list of input points. These points may be represented by any dataset of
 * type vtkPointSet and subclasses. The output of the filter is a polygonal
 * dataset. Usually the output is a triangle mesh, but if a non-zero alpha
 * distance value is specified (called the "alpha" value), then only
 * triangles, edges, and vertices laying within the alpha radius are
 * output. In other words, non-zero alpha values may result in arbitrary
 * combinations of triangles, lines, and vertices. (The notion of alpha value
 * is derived from Edelsbrunner's work on "alpha shapes".) Also, it is
 * possible to generate "constrained triangulations" using this filter.
 * A constrained triangulation is one where edges and loops (i.e., polygons)
 * can be defined and the triangulation will preserve them (read on for
 * more information).
 *
 * The 2D Delaunay triangulation is defined as the triangulation that
 * satisfies the Delaunay criterion for n-dimensional simplexes (in this case
 * n=2 and the simplexes are triangles). This criterion states that a
 * circumsphere of each simplex in a triangulation contains only the n+1
 * defining points of the simplex. (See "The Visualization Toolkit" text
 * for more information.) In two dimensions, this translates into an optimal
 * triangulation. That is, the maximum interior angle of any triangle is less
 * than or equal to that of any possible triangulation.
 *
 * Delaunay triangulations are used to build topological structures
 * from unorganized (or unstructured) points. The input to this filter
 * is a list of points specified in 3D, even though the triangulation
 * is 2D. Thus the triangulation is constructed in the x-y plane, and
 * the z coordinate is ignored (although carried through to the
 * output). If you desire to triangulate in a different plane, you
 * can use the vtkTransformFilter to transform the points into and
 * out of the x-y plane or you can specify a transform to the Delaunay2D
 * directly.  In the latter case, the input points are transformed, the
 * transformed points are triangulated, and the output will use the
 * triangulated topology for the original (non-transformed) points.  This
 * avoids transforming the data back as would be required when using the
 * vtkTransformFilter method.  Specifying a transform directly also allows
 * any transform to be used: rigid, non-rigid, non-invertible, etc.
 *
 * If an input transform is used, then alpha values are applied (for the
 * most part) in the original data space.  The exception is when
 * BoundingTriangulation is on.  In this case, alpha values are applied in
 * the original data space unless a cell uses a bounding vertex.
 *
 * The Delaunay triangulation can be numerically sensitive in some cases. To
 * prevent problems, try to avoid injecting points that will result in
 * triangles with bad aspect ratios (1000:1 or greater). In practice this
 * means inserting points that are "widely dispersed", and enables smooth
 * transition of triangle sizes throughout the mesh. (You may even want to
 * add extra points to create a better point distribution.) If numerical
 * problems are present, you will see a warning message to this effect at
 * the end of the triangulation process.
 *
 * To create constrained meshes, you must define an additional
 * input. This input is an instance of vtkPolyData which contains
 * lines, polylines, and/or polygons that define constrained edges and
 * loops. Only the topology of (lines and polygons) from this second
 * input are used.  The topology is assumed to reference points in the
 * input point set (the one to be triangulated). In other words, the
 * lines and polygons use point ids from the first input point
 * set. Lines and polylines found in the input will be mesh edges in
 * the output. Polygons define a loop with inside and outside
 * regions. The inside of the polygon is determined by using the
 * right-hand-rule, i.e., looking down the z-axis a polygon should be
 * ordered counter-clockwise. Holes in a polygon should be ordered
 * clockwise. If you choose to create a constrained triangulation, the
 * final mesh may not satisfy the Delaunay criterion. (Noted: the
 * lines/polygon edges must not intersect when projected onto the 2D
 * plane.  It may not be possible to recover all edges due to not
 * enough points in the triangulation, or poorly defined edges
 * (coincident or excessively long).  The form of the lines or
 * polygons is a list of point ids that correspond to the input point
 * ids used to generate the triangulation.)
 *
 * If an input transform is used, constraints are defined in the
 * "transformed" space.  So when the right hand rule is used for a
 * polygon constraint, that operation is applied using the transformed
 * points.  Since the input transform can be any transformation (rigid
 * or non-rigid), care must be taken in constructing constraints when
 * an input transform is used.
 *
 * @warning
 * Points arranged on a regular lattice (termed degenerate cases) can be
 * triangulated in more than one way (at least according to the Delaunay
 * criterion). The choice of triangulation (as implemented by
 * this algorithm) depends on the order of the input points. The first three
 * points will form a triangle; other degenerate points will not break
 * this triangle.
 *
 * @warning
 * Points that are coincident (or nearly so) may be discarded by the algorithm.
 * This is because the Delaunay triangulation requires unique input points.
 * You can control the definition of coincidence with the "Tolerance" instance
 * variable.
 *
 * @warning
 * The output of the Delaunay triangulation is supposedly a convex hull. In
 * certain cases this implementation may not generate the convex hull. This
 * behavior can be controlled by the Offset instance variable. Offset is a
 * multiplier used to control the size of the initial triangulation. The
 * larger the offset value, the more likely you will generate a convex hull;
 * but the more likely you are to see numerical problems.
 *
 * @sa
 * vtkDelaunay3D vtkTransformFilter vtkGaussianSplatter
*/

#ifndef vtkDelaunay2D_h
#define vtkDelaunay2D_h

#include "vtkFiltersCoreModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

class vtkAbstractTransform;
class vtkCellArray;
class vtkIdList;
class vtkPointSet;

#define VTK_DELAUNAY_XY_PLANE 0
#define VTK_SET_TRANSFORM_PLANE 1
#define VTK_BEST_FITTING_PLANE 2

class VTKFILTERSCORE_EXPORT vtkDelaunay2D : public vtkPolyDataAlgorithm
{
public:
  vtkTypeMacro(vtkDelaunay2D,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Construct object with Alpha = 0.0; Tolerance = 0.001; Offset = 1.25;
   * BoundingTriangulation turned off.
   */
  static vtkDelaunay2D *New();

  /**
   * Specify the source object used to specify constrained edges and loops.
   * (This is optional.) If set, and lines/polygons are defined, a constrained
   * triangulation is created. The lines/polygons are assumed to reference
   * points in the input point set (i.e. point ids are identical in the
   * input and source).
   * Note that this method does not connect the pipeline. See SetSourceConnection
   * for connecting the pipeline.
   */
  void SetSourceData(vtkPolyData *);

  /**
   * Specify the source object used to specify constrained edges and loops.
   * (This is optional.) If set, and lines/polygons are defined, a constrained
   * triangulation is created. The lines/polygons are assumed to reference
   * points in the input point set (i.e. point ids are identical in the
   * input and source).
   * New style. This method is equivalent to SetInputConnection(1, algOutput).
   */
  void SetSourceConnection(vtkAlgorithmOutput *algOutput);

  /**
   * Get a pointer to the source object.
   */
  vtkPolyData *GetSource();

  //@{
  /**
   * Specify alpha (or distance) value to control output of this filter.
   * For a non-zero alpha value, only edges or triangles contained within
   * a sphere centered at mesh vertices will be output. Otherwise, only
   * triangles will be output.
   */
  vtkSetClampMacro(Alpha,double,0.0,VTK_DOUBLE_MAX);
  vtkGetMacro(Alpha,double);
  //@}

  //@{
  /**
   * Specify a tolerance to control discarding of closely spaced points.
   * This tolerance is specified as a fraction of the diagonal length of
   * the bounding box of the points.
   */
  vtkSetClampMacro(Tolerance,double,0.0,1.0);
  vtkGetMacro(Tolerance,double);
  //@}

  //@{
  /**
   * Specify a multiplier to control the size of the initial, bounding
   * Delaunay triangulation.
   */
  vtkSetClampMacro(Offset,double,0.75,VTK_DOUBLE_MAX);
  vtkGetMacro(Offset,double);
  //@}

  //@{
  /**
   * Boolean controls whether bounding triangulation points (and associated
   * triangles) are included in the output. (These are introduced as an
   * initial triangulation to begin the triangulation process. This feature
   * is nice for debugging output.)
   */
  vtkSetMacro(BoundingTriangulation,int);
  vtkGetMacro(BoundingTriangulation,int);
  vtkBooleanMacro(BoundingTriangulation,int);
  //@}

  //@{
  /**
   * Set / get the transform which is applied to points to generate a
   * 2D problem.  This maps a 3D dataset into a 2D dataset where
   * triangulation can be done on the XY plane.  The points are
   * transformed and triangulated.  The topology of triangulated
   * points is used as the output topology.  The output points are the
   * original (untransformed) points.  The transform can be any
   * subclass of vtkAbstractTransform (thus it does not need to be a
   * linear or invertible transform).
   */
  virtual void SetTransform(vtkAbstractTransform*);
  vtkGetObjectMacro(Transform, vtkAbstractTransform);
  //@}

  //@{
  /**
   * Define
   */
  vtkSetClampMacro(ProjectionPlaneMode,int,
                   VTK_DELAUNAY_XY_PLANE,VTK_BEST_FITTING_PLANE);
  vtkGetMacro(ProjectionPlaneMode,int);
  //@}

protected:
  vtkDelaunay2D();
  ~vtkDelaunay2D() VTK_OVERRIDE;

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *) VTK_OVERRIDE;

  vtkAbstractTransform * ComputeBestFittingPlane(vtkPointSet *input);

  double Alpha;
  double Tolerance;
  int BoundingTriangulation;
  double Offset;

  vtkAbstractTransform *Transform;

  int ProjectionPlaneMode; //selects the plane in 3D where the Delaunay triangulation will be computed.

private:
  vtkPolyData *Mesh; //the created mesh
  double *Points;    //the raw points in double precision
  void SetPoint(vtkIdType id, double *x)
  {vtkIdType idx=3*id;
    this->Points[idx] = x[0];
    this->Points[idx+1] = x[1];
    this->Points[idx+2] = x[2];
  }

  void GetPoint(vtkIdType id, double x[3])
  {double *ptr = this->Points + 3*id;
    x[0] = *ptr++;
    x[1] = *ptr++;
    x[2] = *ptr;
  }

  int NumberOfDuplicatePoints;
  int NumberOfDegeneracies;

  int *RecoverBoundary(vtkPolyData *source);
  int RecoverEdge(vtkPolyData* source, vtkIdType p1, vtkIdType p2);
  void FillPolygons(vtkCellArray *polys, int *triUse);

  int InCircle (double x[3], double x1[3], double x2[3], double x3[3]);
  vtkIdType FindTriangle(double x[3], vtkIdType ptIds[3], vtkIdType tri,
                         double tol, vtkIdType nei[3], vtkIdList *neighbors);
  void CheckEdge(vtkIdType ptId, double x[3], vtkIdType p1, vtkIdType p2,
                 vtkIdType tri, bool recursive);

  int FillInputPortInformation(int, vtkInformation*) VTK_OVERRIDE;

private:
  vtkDelaunay2D(const vtkDelaunay2D&) VTK_DELETE_FUNCTION;
  void operator=(const vtkDelaunay2D&) VTK_DELETE_FUNCTION;
};

#endif
