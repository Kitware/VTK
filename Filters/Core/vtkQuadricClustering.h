// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkQuadricClustering
 * @brief   reduce the number of triangles in a mesh
 *
 * vtkQuadricClustering is a filter to reduce the number of triangles in a
 * triangle mesh, forming a good approximation to the original geometry.  The
 * input to vtkQuadricClustering is a vtkPolyData object, and all types of
 * polygonal data are handled.
 *
 * The algorithm used is the one described by Peter Lindstrom in his Siggraph
 * 2000 paper, "Out-of-Core Simplification of Large Polygonal Models."  The
 * general approach of the algorithm is to cluster vertices in a uniform
 * binning of space, accumulating the quadric of each triangle (pushed out to
 * the triangles vertices) within each bin, and then determining an optimal
 * position for a single vertex in a bin by using the accumulated quadric. In
 * more detail, the algorithm first gets the bounds of the input poly data.
 * It then breaks this bounding volume into a user-specified number of
 * spatial bins.  It then reads each triangle from the input and hashes its
 * vertices into these bins.  (If this is the first time a bin has been
 * visited, initialize its quadric to the 0 matrix.) The algorithm computes
 * the error quadric for this triangle and adds it to the existing quadric of
 * the bin in which each vertex is contained. Then, if 2 or more vertices of
 * the triangle fall in the same bin, the triangle is discarded.  If the
 * triangle is not discarded, it adds the triangle to the list of output
 * triangles as a list of vertex identifiers.  (There is one vertex id per
 * bin.)  After all the triangles have been read, the representative vertex
 * for each bin is computed (an optimal location is found) using the quadric
 * for that bin.  This determines the spatial location of the vertices of
 * each of the triangles in the output.
 *
 * To use this filter, specify the divisions defining the spatial subdivision
 * in the x, y, and z directions. You must also specify an input vtkPolyData.
 * Then choose to either 1) use the original points that minimize the quadric
 * error to produce the output triangles or 2) compute an optimal position in
 * each bin to produce the output triangles (recommended and default behavior).
 *
 * This filter can take multiple inputs.  To do this, the user must explicitly
 * call StartAppend, Append (once for each input), and EndAppend.  StartAppend
 * sets up the data structure to hold the quadric matrices.  Append processes
 * each triangle in the input poly data it was called on, hashes its vertices
 * to the appropriate bins, determines whether to keep this triangle, and
 * updates the appropriate quadric matrices.  EndAppend determines the spatial
 * location of each of the representative vertices for the visited bins. While
 * this approach does not fit into the visualization architecture and requires
 * manual control, it has the advantage that extremely large data can be
 * processed in pieces and appended to the filter piece-by-piece.
 *
 * @warning
 * This filter can drastically affect topology, i.e., topology is not
 * preserved.
 *
 * @warning
 * The filter handles input triangle strips and arbitrary polygons. Arbitrary
 * polygons are assumed convex: during insertion they are triangulated using
 * a fan of triangles from the first point in the polygons. If the polygon is
 * concave, this can produce bad results. In this case, use vtkTriangleFilter
 * to triangulate the polygons first.
 *
 * @warning
 * The filter also treats polylines and vertices.
 *
 * @warning
 * Note that for certain types of geometry (e.g., a mostly 2D plane with
 * jitter in the normal direction), the decimator can perform badly. In this
 * situation, set the number of bins in the normal direction to one.
 *
 * @warning
 * vtkBinnedDecimation produces similar results with significant speedup
 * and reduced memory consumption.
 *
 * @sa
 * vtkQuadricDecimation vtkDecimatePro vtkDecimate vtkQuadricLODActor
 * vtkBinnedDecimation
 */

#ifndef vtkQuadricClustering_h
#define vtkQuadricClustering_h

#include "vtkFiltersCoreModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkCellArray;
class vtkFeatureEdges;
class vtkPoints;
class vtkQuadricClusteringCellSet;

class VTKFILTERSCORE_EXPORT vtkQuadricClustering : public vtkPolyDataAlgorithm
{
public:
  ///@{
  /**
   * Standard instantiation, type and print methods.
   */
  static vtkQuadricClustering* New();
  vtkTypeMacro(vtkQuadricClustering, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  ///@{
  /**
   * Set/Get the number of divisions along each axis for the spatial bins.
   * The number of spatial bins is NumberOfXDivisions*NumberOfYDivisions*
   * NumberOfZDivisions.  The filter may choose to ignore large numbers of
   * divisions if the input has few points and AutoAdjustNumberOfDivisions
   * is enabled.
   */
  void SetNumberOfXDivisions(int num);
  void SetNumberOfYDivisions(int num);
  void SetNumberOfZDivisions(int num);
  vtkGetMacro(NumberOfXDivisions, int);
  vtkGetMacro(NumberOfYDivisions, int);
  vtkGetMacro(NumberOfZDivisions, int);
  void SetNumberOfDivisions(int div[3]) { this->SetNumberOfDivisions(div[0], div[1], div[2]); }
  void SetNumberOfDivisions(int div0, int div1, int div2);
  int* GetNumberOfDivisions() VTK_SIZEHINT(3);
  void GetNumberOfDivisions(int div[3]);
  ///@}

  ///@{
  /**
   * Enable automatic adjustment of number of divisions. If off, the number
   * of divisions specified by the user is always used (as long as it is valid).
   * The default is On
   */
  vtkSetMacro(AutoAdjustNumberOfDivisions, vtkTypeBool);
  vtkGetMacro(AutoAdjustNumberOfDivisions, vtkTypeBool);
  vtkBooleanMacro(AutoAdjustNumberOfDivisions, vtkTypeBool);
  ///@}

  ///@{
  /**
   * This is an alternative way to set up the bins.  If you are trying to match
   * boundaries between pieces, then you should use these methods rather than
   * SetNumberOfDivisions. To use these methods, specify the origin and spacing
   * of the spatial binning.
   */
  void SetDivisionOrigin(double x, double y, double z);
  void SetDivisionOrigin(double o[3]) { this->SetDivisionOrigin(o[0], o[1], o[2]); }
  vtkGetVector3Macro(DivisionOrigin, double);
  void SetDivisionSpacing(double x, double y, double z);
  void SetDivisionSpacing(double s[3]) { this->SetDivisionSpacing(s[0], s[1], s[2]); }
  vtkGetVector3Macro(DivisionSpacing, double);
  ///@}

  ///@{
  /**
   * Normally the point that minimizes the quadric error function is used as
   * the output of the bin.  When this flag is on, the bin point is forced to
   * be one of the points from the input (the one with the smallest
   * error). This option does not work (i.e., input points cannot be used)
   * when the append methods (StartAppend(), Append(), EndAppend()) are being
   * called directly.
   */
  vtkSetMacro(UseInputPoints, vtkTypeBool);
  vtkGetMacro(UseInputPoints, vtkTypeBool);
  vtkBooleanMacro(UseInputPoints, vtkTypeBool);
  ///@}

  ///@{
  /**
   * By default, this flag is off.  When "UseFeatureEdges" is on, then
   * quadrics are computed for boundary edges/feature edges.  They influence
   * the quadrics (position of points), but not the mesh.  Which features to
   * use can be controlled by the filter "FeatureEdges".
   */
  vtkSetMacro(UseFeatureEdges, vtkTypeBool);
  vtkGetMacro(UseFeatureEdges, vtkTypeBool);
  vtkBooleanMacro(UseFeatureEdges, vtkTypeBool);
  vtkFeatureEdges* GetFeatureEdges() { return this->FeatureEdges; }
  ///@}

  ///@{
  /**
   * By default, this flag is off.  It only has an effect when
   * "UseFeatureEdges" is also on.  When "UseFeaturePoints" is on, then
   * quadrics are computed for boundary / feature points used in the boundary
   * / feature edges.  They influence the quadrics (position of points), but
   * not the mesh.
   */
  vtkSetMacro(UseFeaturePoints, vtkTypeBool);
  vtkGetMacro(UseFeaturePoints, vtkTypeBool);
  vtkBooleanMacro(UseFeaturePoints, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Set/Get the angle to use in determining whether a point on a boundary /
   * feature edge is a feature point.
   */
  vtkSetClampMacro(FeaturePointsAngle, double, 0.0, 180.0);
  vtkGetMacro(FeaturePointsAngle, double);
  ///@}

  ///@{
  /**
   * When this flag is on (and it is on by default), then triangles that are
   * completely contained in a bin are added to the bin quadrics.  When the
   * the flag is off the filter operates faster, but the surface may not be
   * as well behaved.
   */
  vtkSetMacro(UseInternalTriangles, vtkTypeBool);
  vtkGetMacro(UseInternalTriangles, vtkTypeBool);
  vtkBooleanMacro(UseInternalTriangles, vtkTypeBool);
  ///@}

  ///@{
  /**
   * These methods provide an alternative way of executing the filter.
   * PolyData can be added to the result in pieces (append).
   * In this mode, the user must specify the bounds of the entire model
   * as an argument to the "StartAppend" method.
   */
  void StartAppend(double* bounds);
  void StartAppend(double x0, double x1, double y0, double y1, double z0, double z1)
  {
    double b[6];
    b[0] = x0;
    b[1] = x1;
    b[2] = y0;
    b[3] = y1;
    b[4] = z0;
    b[5] = z1;
    this->StartAppend(b);
  }
  void Append(vtkPolyData* piece);
  void EndAppend();
  ///@}

  ///@{
  /**
   * This flag makes the filter copy cell data from input to output
   * (the best it can).  It uses input cells that trigger the addition
   * of output cells (no averaging).  This is off by default, and does
   * not work when append is being called explicitly (non-pipeline usage).
   */
  vtkSetMacro(CopyCellData, vtkTypeBool);
  vtkGetMacro(CopyCellData, vtkTypeBool);
  vtkBooleanMacro(CopyCellData, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Specify a boolean indicating whether to remove duplicate cells
   * (i.e. triangles).  This is a little slower, and takes more memory, but
   * in some cases can reduce the number of cells produced by an order of
   * magnitude. By default, this flag is true.
   */
  vtkSetMacro(PreventDuplicateCells, vtkTypeBool);
  vtkGetMacro(PreventDuplicateCells, vtkTypeBool);
  vtkBooleanMacro(PreventDuplicateCells, vtkTypeBool);
  ///@}

protected:
  vtkQuadricClustering();
  ~vtkQuadricClustering() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int FillInputPortInformation(int, vtkInformation*) override;

  /**
   * Given a point, determine what bin it falls into.
   */
  vtkIdType HashPoint(double point[3]);

  /**
   * Determine the representative point for this bin.
   */
  void ComputeRepresentativePoint(double quadric[9], vtkIdType binId, double point[3]);

  ///@{
  /**
   * Add triangles to the quadric array.  If geometry flag is on then
   * triangles are added to the output.
   */
  void AddPolygons(vtkCellArray* polys, vtkPoints* points, int geometryFlag, vtkPolyData* input,
    vtkPolyData* output);
  void AddStrips(vtkCellArray* strips, vtkPoints* points, int geometryFlag, vtkPolyData* input,
    vtkPolyData* output);
  void AddTriangle(vtkIdType* binIds, double* pt0, double* pt1, double* pt2, int geometeryFlag,
    vtkPolyData* input, vtkPolyData* output);
  ///@}

  ///@{
  /**
   * Add edges to the quadric array.  If geometry flag is on then
   * edges are added to the output.
   */
  void AddEdges(vtkCellArray* edges, vtkPoints* points, int geometryFlag, vtkPolyData* input,
    vtkPolyData* output);
  void AddEdge(vtkIdType* binIds, double* pt0, double* pt1, int geometeryFlag, vtkPolyData* input,
    vtkPolyData* output);
  ///@}

  ///@{
  /**
   * Add vertices to the quadric array.  If geometry flag is on then
   * vertices are added to the output.
   */
  void AddVertices(vtkCellArray* verts, vtkPoints* points, int geometryFlag, vtkPolyData* input,
    vtkPolyData* output);
  void AddVertex(
    vtkIdType binId, double* pt, int geometryFlag, vtkPolyData* input, vtkPolyData* output);
  ///@}

  /**
   * Initialize the quadric matrix to 0's.
   */
  void InitializeQuadric(double quadric[9]);

  /**
   * Add this quadric to the quadric already associated with this bin.
   */
  void AddQuadric(vtkIdType binId, double quadric[9]);

  /**
   * Find the feature points of a given set of edges.
   * The points returned are (1) those used by only one edge, (2) those
   * used by > 2 edges, and (3) those where the angle between 2 edges
   * using this point is < angle.
   */
  void FindFeaturePoints(vtkCellArray* edges, vtkPoints* edgePts, double angle);

  ///@{
  /**
   * This method will rep[lace the quadric generated points with the
   * input points with the lowest error.
   */
  void EndAppendUsingPoints(vtkPolyData* input, vtkPolyData* output);
  vtkTypeBool UseInputPoints;
  ///@}

  /**
   * This method sets the vertices of the output.
   * It duplicates the structure of the input cells (but decimiated).
   */
  void EndAppendVertexGeometry(vtkPolyData* input, vtkPolyData* output);

  // Unfinished option to handle boundary edges differently.
  void AppendFeatureQuadrics(vtkPolyData* pd, vtkPolyData* output);
  vtkTypeBool UseFeatureEdges;
  vtkTypeBool UseFeaturePoints;
  vtkTypeBool UseInternalTriangles;

  int NumberOfXDivisions;
  int NumberOfYDivisions;
  int NumberOfZDivisions;

  // Set this to eliminate duplicate cells
  vtkTypeBool PreventDuplicateCells;
  vtkQuadricClusteringCellSet* CellSet; // PIMPLd stl set for tracking inserted cells
  vtkIdType NumberOfBins;

  // Used internally.
  // can be smaller than user values when input numb er of points is small.
  int NumberOfDivisions[3];

  // Since there are two ways of specifying the grid, we have this flag
  // to indicate which the user has set.  When this flag is on,
  // the bin sizes are computed from the DivisionOrigin and DivisionSpacing.
  vtkTypeBool ComputeNumberOfDivisions;

  double DivisionOrigin[3];
  double DivisionSpacing[3];
  vtkTypeBool AutoAdjustNumberOfDivisions;

  double Bounds[6];
  double XBinSize;
  double YBinSize;
  double ZBinSize;
  double XBinStep; // replace some divisions with multiplication
  double YBinStep;
  double ZBinStep;
  vtkIdType SliceSize; // eliminate one multiplication

  struct PointQuadric
  {
    PointQuadric()
      : VertexId(-1)
      , Dimension(255)
    {
    }

    vtkIdType VertexId;
    // Dimension is supposed to be a flag representing the dimension of the
    // cells contributing to the quadric.  Lines: 1, Triangles: 2 (and points
    // 0 in the future?)
    unsigned char Dimension;
    double Quadric[9];
  };

  PointQuadric* QuadricArray;
  vtkIdType NumberOfBinsUsed;

  // Have to make these instance variables if we are going to allow
  // the algorithm to be driven by the Append methods.
  vtkCellArray* OutputTriangleArray;
  vtkCellArray* OutputLines;

  vtkFeatureEdges* FeatureEdges;
  vtkPoints* FeaturePoints;
  double FeaturePointsAngle;

  vtkTypeBool CopyCellData;
  int InCellCount;
  int OutCellCount;

private:
  vtkQuadricClustering(const vtkQuadricClustering&) = delete;
  void operator=(const vtkQuadricClustering&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
