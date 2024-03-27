// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPolyDataNormals
 * @brief   compute normals for polygonal mesh
 *
 * vtkPolyDataNormals is a filter that computes point and/or cell normals
 * for a polygonal mesh. The user specifies if they would like the point
 * and/or cell normals to be computed by setting the ComputeCellNormals
 * and ComputePointNormals flags.
 *
 * The computed normals (a vtkFloatArray) are set to be the active normals
 * (using SetNormals()) of the PointData and/or the CellData (respectively)
 * of the output PolyData. The name of these arrays is "Normals", so they
 * can be retrieved either with
 * vtkArrayDownCast<vtkFloatArray>(output->GetPointData()->GetNormals())
 * or with
 * vtkArrayDownCast<vtkFloatArray>(output->GetPointData()->GetArray("Normals"))
 *
 * The filter can reorder polygons to ensure consistent
 * orientation across polygon neighbors. Sharp edges can be split and points
 * duplicated with separate normals to give crisp (rendered) surface definition.
 * It is also possible to globally flip the normal orientation.
 *
 * The algorithm works by determining normals for each polygon and then
 * averaging them at shared points. When sharp edges are present, the edges
 * are split and new points generated to prevent blurry edges (due to
 * Gouraud shading).
 *
 * @warning
 * Normals are computed only for polygons and triangle strips. Normals are
 * not computed for lines or vertices.
 *
 * @warning
 * Triangle strips are broken up into triangle polygons. You may want to
 * restrip the triangles.
 *
 * @warning
 * This class has been threaded with vtkSMPTools. Using TBB or other
 * non-sequential type (set in the CMake variable
 * VTK_SMP_IMPLEMENTATION_TYPE) may improve performance significantly.
 *
 * @sa
 * For high-performance rendering, you could use vtkTriangleMeshPointNormals
 * if you know that you have a triangle mesh which does not require splitting
 * nor consistency check on the cell orientations.
 *
 * @sa
 * vtkOrientPolyData, vtkSplitSharpEdgesPolyData, vtkTriangleFilter
 */

#ifndef vtkPolyDataNormals_h
#define vtkPolyDataNormals_h

#include "vtkFiltersCoreModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkFloatArray;
class vtkIdList;
class vtkPolyData;

class VTKFILTERSCORE_EXPORT vtkPolyDataNormals : public vtkPolyDataAlgorithm
{
public:
  vtkTypeMacro(vtkPolyDataNormals, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Construct with feature angle=30, splitting and consistency turned on,
   * flipNormals turned off, and non-manifold traversal turned on.
   * ComputePointNormals is on and ComputeCellNormals is off.
   */
  static vtkPolyDataNormals* New();

  ///@{
  /**
   * Specify the angle that defines a sharp edge. If the difference in
   * angle across neighboring polygons is greater than this value, the
   * shared edge is considered "sharp".
   *
   * The default value is 30 degrees.
   */
  vtkSetClampMacro(FeatureAngle, double, 0.0, 180.0);
  vtkGetMacro(FeatureAngle, double);
  ///@}

  ///@{
  /**
   * Turn on/off the splitting of sharp edges.
   *
   * Splitting is performed only if ComputePointNormals is on.
   *
   * The default value is true.
   */
  vtkSetMacro(Splitting, vtkTypeBool);
  vtkGetMacro(Splitting, vtkTypeBool);
  vtkBooleanMacro(Splitting, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Turn on/off the enforcement of consistent polygon ordering.
   *
   * The default value is true.
   */
  vtkSetMacro(Consistency, vtkTypeBool);
  vtkGetMacro(Consistency, vtkTypeBool);
  vtkBooleanMacro(Consistency, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Turn on/off the automatic determination of correct normal
   * orientation. NOTE: This assumes a completely closed surface
   * (i.e. no boundary edges) and no non-manifold edges. If these
   * constraints do not hold, all bets are off. This option adds some
   * computational complexity, and is useful if you don't want to have
   * to inspect the rendered image to determine whether to turn on the
   * FlipNormals flag. However, this flag can work with the FlipNormals
   * flag, and if both are set, all the normals in the output will
   * point "inward".
   *
   * The default value is false.
   */
  vtkSetMacro(AutoOrientNormals, vtkTypeBool);
  vtkGetMacro(AutoOrientNormals, vtkTypeBool);
  vtkBooleanMacro(AutoOrientNormals, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Turn on/off the computation of point normals.
   *
   * The default value is true.
   */
  vtkSetMacro(ComputePointNormals, vtkTypeBool);
  vtkGetMacro(ComputePointNormals, vtkTypeBool);
  vtkBooleanMacro(ComputePointNormals, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Turn on/off the computation of cell normals.
   *
   * The default value is false.
   */
  vtkSetMacro(ComputeCellNormals, vtkTypeBool);
  vtkGetMacro(ComputeCellNormals, vtkTypeBool);
  vtkBooleanMacro(ComputeCellNormals, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Turn on/off the global flipping of normal orientation. Flipping
   * reverves the meaning of front and back for Frontface and Backface
   * culling in vtkProperty.  Flipping modifies both the normal
   * direction and the order of a cell's points.
   *
   * The default value is false.
   */
  vtkSetMacro(FlipNormals, vtkTypeBool);
  vtkGetMacro(FlipNormals, vtkTypeBool);
  vtkBooleanMacro(FlipNormals, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Turn on/off traversal across non-manifold edges. This will prevent
   * problems where the consistency of polygonal ordering is corrupted due
   * to topological loops.
   *
   * The default value is true.
   */
  vtkSetMacro(NonManifoldTraversal, vtkTypeBool);
  vtkGetMacro(NonManifoldTraversal, vtkTypeBool);
  vtkBooleanMacro(NonManifoldTraversal, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Set/get the desired precision for the output types. See the documentation
   * for the vtkAlgorithm::DesiredOutputPrecision enum for an explanation of
   * the available precision settings.
   *
   * The default value is vtkAlgorithm::DEFAULT_PRECISION.
   */
  vtkSetClampMacro(OutputPointsPrecision, int, SINGLE_PRECISION, DEFAULT_PRECISION);
  vtkGetMacro(OutputPointsPrecision, int);
  ///@}

  static vtkSmartPointer<vtkFloatArray> GetCellNormals(vtkPolyData* data);
  static vtkSmartPointer<vtkFloatArray> GetPointNormals(
    vtkPolyData* data, vtkFloatArray* cellNormals, double flipDirection = 1.0);

protected:
  vtkPolyDataNormals();
  ~vtkPolyDataNormals() override = default;

  // Usual data generation method
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  double FeatureAngle;
  vtkTypeBool Splitting;
  vtkTypeBool Consistency;
  vtkTypeBool FlipNormals;
  vtkTypeBool AutoOrientNormals;
  vtkTypeBool NonManifoldTraversal;
  vtkTypeBool ComputePointNormals;
  vtkTypeBool ComputeCellNormals;
  int OutputPointsPrecision;

private:
  vtkPolyDataNormals(const vtkPolyDataNormals&) = delete;
  void operator=(const vtkPolyDataNormals&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
