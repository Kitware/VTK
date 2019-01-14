/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkClipClosedSurface.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkClipClosedSurface
 * @brief   Clip a closed surface with a plane collection
 *
 * vtkClipClosedSurface will clip a closed polydata surface with a
 * collection of clipping planes.  It will produce a new closed surface
 * by creating new polygonal faces where the input data was clipped.
 *
 * Non-manifold surfaces should not be used as input for this filter.
 * The input surface should have no open edges, and must not have any
 * edges that are shared by more than two faces.  The vtkFeatureEdges
 * filter can be used to verify that a data set satisfies these conditions.
 * In addition, the input surface should not self-intersect, meaning
 * that the faces of the surface should only touch at their edges.
 *
 * If GenerateOutline is on, this filter will generate an outline wherever
 * the clipping planes intersect the data.  The ScalarMode option
 * will add cell scalars to the output, so that the generated faces
 * can be visualized in a different color from the original surface.
 *
 * @warning
 * The triangulation of new faces is done in O(n) time for simple convex
 * inputs, but for non-convex inputs the worst-case time is O(n^2*m^2)
 * where n is the number of points and m is the number of 3D cavities.
 * The best triangulation algorithms, in contrast, are O(n log n).
 * There are also rare cases where the triangulation will fail to produce
 * a watertight output.  Turn on TriangulationErrorDisplay to be notified
 * of these failures.
 * @sa
 * vtkOutlineFilter vtkOutlineSource vtkVolumeOutlineSource
 * @par Thanks:
 * Thanks to David Gobbi for contributing this class to VTK.
*/

#ifndef vtkClipClosedSurface_h
#define vtkClipClosedSurface_h

#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

class vtkPlaneCollection;
class vtkUnsignedCharArray;
class vtkDoubleArray;
class vtkIdTypeArray;
class vtkCellArray;
class vtkPointData;
class vtkCellData;
class vtkPolygon;
class vtkIdList;
class vtkCCSEdgeLocator;

enum {
  VTK_CCS_SCALAR_MODE_NONE = 0,
  VTK_CCS_SCALAR_MODE_COLORS = 1,
  VTK_CCS_SCALAR_MODE_LABELS = 2
};

class VTKFILTERSGENERAL_EXPORT vtkClipClosedSurface : public vtkPolyDataAlgorithm
{
public:
  static vtkClipClosedSurface *New();
  vtkTypeMacro(vtkClipClosedSurface,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Set the vtkPlaneCollection that holds the clipping planes.
   */
  virtual void SetClippingPlanes(vtkPlaneCollection *planes);
  vtkGetObjectMacro(ClippingPlanes,vtkPlaneCollection);
  //@}

  //@{
  /**
   * Set the tolerance for creating new points while clipping.  If the
   * tolerance is too small, then degenerate triangles might be produced.
   * The default tolerance is 1e-6.
   */
  vtkSetMacro(Tolerance, double);
  vtkGetMacro(Tolerance, double);
  //@}

  //@{
  /**
   * Pass the point data to the output.  Point data will be interpolated
   * when new points are generated.  This is off by default.
   */
  vtkSetMacro(PassPointData, vtkTypeBool);
  vtkBooleanMacro(PassPointData, vtkTypeBool);
  vtkGetMacro(PassPointData, vtkTypeBool);
  //@}

  //@{
  /**
   * Set whether to generate an outline wherever an input face was
   * cut by a plane.  This is off by default.
   */
  vtkSetMacro(GenerateOutline, vtkTypeBool);
  vtkBooleanMacro(GenerateOutline, vtkTypeBool);
  vtkGetMacro(GenerateOutline, vtkTypeBool);
  //@}

  //@{
  /**
   * Set whether to generate polygonal faces for the output.  This is
   * on by default.  If it is off, then the output will have no polys.
   */
  vtkSetMacro(GenerateFaces, vtkTypeBool);
  vtkBooleanMacro(GenerateFaces, vtkTypeBool);
  vtkGetMacro(GenerateFaces, vtkTypeBool);
  //@}

  //@{
  /**
   * Set whether to add cell scalars, so that new faces and outlines
   * can be distinguished from original faces and lines.  The options
   * are "None", "Colors", and "Labels".  For the "Labels" option,
   * a scalar value of "0" indicates an original cell, "1" indicates
   * a new cell on a cut face, and "2" indicates a new cell on the
   * ActivePlane as set by the SetActivePlane() method.  The default
   * scalar mode is "None".
   */
  vtkSetClampMacro(ScalarMode, int,
    VTK_CCS_SCALAR_MODE_NONE, VTK_CCS_SCALAR_MODE_LABELS);
  void SetScalarModeToNone() {
    this->SetScalarMode(VTK_CCS_SCALAR_MODE_NONE); };
  void SetScalarModeToColors() {
    this->SetScalarMode(VTK_CCS_SCALAR_MODE_COLORS); };
  void SetScalarModeToLabels() {
    this->SetScalarMode(VTK_CCS_SCALAR_MODE_LABELS); };
  vtkGetMacro(ScalarMode, int);
  const char *GetScalarModeAsString();
  //@}

  //@{
  /**
   * Set the color for all cells were part of the original geometry.
   * If the input data already has color cell scalars, then those
   * values will be used and parameter will be ignored.  The default color
   * is red.  Requires SetScalarModeToColors.
   */
  vtkSetVector3Macro(BaseColor, double);
  vtkGetVector3Macro(BaseColor, double);
  //@}

  //@{
  /**
   * Set the color for any new geometry, either faces or outlines, that are
   * created as a result of the clipping. The default color is orange.
   * Requires SetScalarModeToColors.
   */
  vtkSetVector3Macro(ClipColor, double);
  vtkGetVector3Macro(ClipColor, double);
  //@}

  //@{
  /**
   * Set the active plane, so that the clipping from that plane can be
   * displayed in a different color.  Set this to -1 if there is no active
   * plane.  The default value is -1.
   */
  vtkSetMacro(ActivePlaneId, int);
  vtkGetMacro(ActivePlaneId, int);
  //@}

  //@{
  /**
   * Set the color for any new geometry produced by clipping with the
   * ActivePlane, if ActivePlaneId is set.  Default is yellow.
   * Requires SetScalarModeToColors.
   */
  vtkSetVector3Macro(ActivePlaneColor, double);
  vtkGetVector3Macro(ActivePlaneColor, double);
  //@}

  //@{
  /**
   * Generate errors when the triangulation fails.  Usually the
   * triangulation errors are too small to see, but they result in
   * a surface that is not watertight.  This option has no impact
   * on performance.
   */
  vtkSetMacro(TriangulationErrorDisplay, vtkTypeBool);
  vtkBooleanMacro(TriangulationErrorDisplay, vtkTypeBool);
  vtkGetMacro(TriangulationErrorDisplay, vtkTypeBool);
  //@}

protected:
  vtkClipClosedSurface();
  ~vtkClipClosedSurface() override;

  vtkPlaneCollection *ClippingPlanes;

  double Tolerance;

  vtkTypeBool PassPointData;
  vtkTypeBool GenerateOutline;
  vtkTypeBool GenerateFaces;
  int ActivePlaneId;
  int ScalarMode;
  double BaseColor[3];
  double ClipColor[3];
  double ActivePlaneColor[3];

  vtkTypeBool TriangulationErrorDisplay;

  vtkIdList *IdList;

  int ComputePipelineMTime(
    vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector, int requestFromOutputPort,
    vtkMTimeType* mtime) override;

  int RequestData(
    vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  /**
   * Method for clipping lines and copying the scalar data.
   */
  void ClipLines(
    vtkPoints *points, vtkDoubleArray *pointScalars,
    vtkPointData *pointData, vtkCCSEdgeLocator *edgeLocator,
    vtkCellArray *inputCells, vtkCellArray *outputLines,
    vtkCellData *inCellData, vtkCellData *outLineData);

  /**
   * Clip and contour polys in one step, in order to guarantee
   * that the contour lines exactly match the new free edges of
   * the clipped polygons.  This exact correspondence is necessary
   * in order to guarantee that the surface remains closed.
   */
  void ClipAndContourPolys(
    vtkPoints *points, vtkDoubleArray *pointScalars, vtkPointData *pointData,
    vtkCCSEdgeLocator *edgeLocator, int triangulate,
    vtkCellArray *inputCells, vtkCellArray *outputPolys,
    vtkCellArray *outputLines, vtkCellData *inPolyData,
    vtkCellData *outPolyData, vtkCellData *outLineData);

  /**
   * A helper function for interpolating a new point along an edge.  It
   * stores the index of the interpolated point in "i", and returns 1 if
   * a new point was added to the points.  The values i0, i1, v0, v1 are
   * the edge enpoints and scalar values, respectively.
   */
  static int InterpolateEdge(
    vtkPoints *points, vtkPointData *pointData,
    vtkCCSEdgeLocator *edgeLocator, double tol,
    vtkIdType i0, vtkIdType i1, double v0, double v1, vtkIdType &i);

  /**
   * A robust method for triangulating a polygon.  It cleans up the polygon
   * and then applies the ear-cut method that is implemented in vtkPolygon.
   * A zero return value indicates that triangulation failed.
   */
  int TriangulatePolygon(
    vtkIdList *polygon, vtkPoints *points, vtkCellArray *triangles);

  /**
   * Given some closed contour lines, create a triangle mesh that
   * fills those lines.  The input lines must be single-segment lines,
   * not polylines.  The input lines do not have to be in order.
   * Only lines from firstLine to will be used.  Specify the normal
   * of the clip plane, which will be opposite the normals
   * of the polys that will be produced.  If outCD has scalars, then color
   * scalars will be added for each poly that is created.
   */
  void TriangulateContours(
    vtkPolyData *data, vtkIdType firstLine, vtkIdType numLines,
    vtkCellArray *outputPolys, const double normal[3]);

  /**
   * Break polylines into individual lines, copying scalar values from
   * inputScalars starting at firstLineScalar.  If inputScalars is zero,
   * then scalars will be set to color.  If scalars is zero, then no
   * scalars will be generated.
   */
  static void BreakPolylines(
    vtkCellArray *inputLines, vtkCellArray *outputLines,
    vtkUnsignedCharArray *inputScalars, vtkIdType firstLineScalar,
    vtkUnsignedCharArray *outputScalars, const unsigned char color[3]);

  /**
   * Copy polygons and their associated scalars to a new array.
   * If inputScalars is set to zero, set polyScalars to color instead.
   * If polyScalars is set to zero, don't generate scalars.
   */
  static void CopyPolygons(
    vtkCellArray *inputPolys, vtkCellArray *outputPolys,
    vtkUnsignedCharArray *inputScalars, vtkIdType firstPolyScalar,
    vtkUnsignedCharArray *outputScalars, const unsigned char color[3]);

  /**
   * Break triangle strips and add the triangles to the output. See
   * CopyPolygons for more information.
   */
  static void BreakTriangleStrips(
    vtkCellArray *inputStrips, vtkCellArray *outputPolys,
    vtkUnsignedCharArray *inputScalars, vtkIdType firstStripScalar,
    vtkUnsignedCharArray *outputScalars, const unsigned char color[3]);

  /**
   * Squeeze the points and store them in the output.  Only the points that
   * are used by the cells will be saved, and the pointIds of the cells will
   * be modified.
   */
  static void SqueezeOutputPoints(
    vtkPolyData *output, vtkPoints *points, vtkPointData *pointData,
    int outputPointDataType);

  /**
   * Take three colors as doubles, and convert to unsigned char.
   */
  static void CreateColorValues(
    const double color1[3], const double color2[3], const double color3[3],
    unsigned char colors[3][3]);

private:
  vtkClipClosedSurface(const vtkClipClosedSurface&) = delete;
  void operator=(const vtkClipClosedSurface&) = delete;
};

#endif
