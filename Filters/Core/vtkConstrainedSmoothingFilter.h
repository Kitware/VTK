// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkConstrainedSmoothingFilter
 * @brief   adjust point positions using constrained smoothing
 *
 * vtkConstrainedSmoothingFilter is a filter that adjusts point coordinates
 * using a modified Laplacian smoothing approach. The effect is to "relax"
 * or "smooth" the mesh, making the cells better shaped and the points more
 * evenly distributed. Note that this filter operates on any vtkPointSet and
 * derived classes. Cell topology is never modified; note however if the
 * constraints are too lax, cells may self-intersect or otherwise be deformed
 * in unfavorable ways.
 *
 * A central concept of this filter is the point smoothing stencil. A
 * smoothing stencil for a point pi is the list of points pj which connect to
 * pi via an edge. To smooth the point pi, pi is moved towards the average
 * position of pj multiplied by the relaxation factor, and limited by the
 * constraint distance. This process is repeated either until convergence
 * occurs, or the maximum number of iterations is reached. Note that
 * smoothing stencils may be specified; or if not provided, the stencils are
 * computed from the input cells connected edges (using vtkExtractEdges with
 * UseAllPoints enabled).
 *
 * To constrain the motion of the points, either set the filter's constraint
 * distance or constraint box, or provide an input point data array (of type
 * vtkDoubleArray) named "SmoothingConstraints." The filter's constraint
 * distance (or constraint box if selected) defines a local sphere (or box)
 * centered on each point to restrict point motion and is applied to all
 * points; whereas the smoothing data array may have different constraint
 * values per point. If provided by the user, by default the smoothing data
 * array takes precedence over the filter's constraint distance and
 * constraint box.
 *
 * @warning
 * The smoothing process reduces high frequency information in the geometry
 * of the mesh. With excessive smoothing important details may be lost, and
 * the surface may shrink towards the centroid. The constraints on point
 * movement help significantly in preventing shrinkage from happening.
 *
 * @warning
 * This filter is used internally by the filters vtkSurfaceNets2D and
 * vtkSurfaceNets3D. vtkConstrainedSmoothingFilter is used by these filters
 * to smooth the extracted surface net, with the constraint distance and
 * constraint box set in relation to a volume voxel.
 *
 * @sa
 * vtkWindowedSincPolyDataFilter vtkSmoothPolyDataFilter
 * vtkAttributeSmoothingFilter vtkExtractEdges vtkSurfaceNets2D
 * vtkSurfaceNets3D
 */

#ifndef vtkConstrainedSmoothingFilter_h
#define vtkConstrainedSmoothingFilter_h

#include "vtkCellArray.h"         // For point smoothing stencils
#include "vtkFiltersCoreModule.h" // For export macro
#include "vtkPointSetAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKFILTERSCORE_EXPORT vtkConstrainedSmoothingFilter : public vtkPointSetAlgorithm
{
public:
  ///@{
  /**
   * Standard methods for instantiation, to obtain type information, and
   * print the state of a class instance.
   */
  static vtkConstrainedSmoothingFilter* New();
  vtkTypeMacro(vtkConstrainedSmoothingFilter, vtkPointSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@{

  ///@{
  /**
   * Specify a convergence criterion for the iteration process. Smaller
   * numbers result in more smoothing iterations. Convergence occurs
   * when, for the current iteration, the maximum distance any point moves
   * is less than or equal to Convergence. The default value is 0.
   */
  vtkSetClampMacro(Convergence, double, 0.0, 1.0);
  vtkGetMacro(Convergence, double);
  ///@}

  ///@{
  /**
   * Specify the maximum number of iterations for smoothing. The number
   * of iterations may be less if the smoothing process converges. The
   * default value is 10.
   */
  vtkSetClampMacro(NumberOfIterations, int, 0, VTK_INT_MAX);
  vtkGetMacro(NumberOfIterations, int);
  ///@}

  ///@{
  /**
   * Specify the relaxation factor for smoothing. As in all iterative
   * methods, the stability of the process is sensitive to this parameter. In
   * general, small relaxation factors and large numbers of iterations are
   * more stable than larger relaxation factors and smaller numbers of
   * iterations. The default value is 0.01.
   */
  vtkSetMacro(RelaxationFactor, double);
  vtkGetMacro(RelaxationFactor, double);
  ///@}

  enum ConstraintStrategyType
  {
    DEFAULT = 0,
    CONSTRAINT_DISTANCE = 1,
    CONSTRAINT_BOX = 2,
    CONSTRAINT_ARRAY = 3
  };

  ///@{
  /**
   * Indicate how to apply constraints. By default, a constraint array takes
   * precedence over the filter's constraint distance or constraint box, but
   * if not available then the constraint distance is used. If a
   * CONSTRAINT_ARRAY strategy is specified, and no constraint array is
   * available from the point data, then the points are unconstrained. If the
   * strategy is set to CONSTRAINT_DISTANCE, then a constraint sphere defined
   * by ConstraintDistance is used; while setting the strategy to
   * CONSTRAINT_BOX an axis-aligned x-y-z box is used to constrain point
   * motion (using constraint distance is slightly faster than using a
   * constraint box). Note that is also possible to turn off constraints
   * completely by simply specifying a very large constraint distance. The
   * default constraint strategy is DEFAULT.
   */
  vtkSetClampMacro(ConstraintStrategy, int, DEFAULT, CONSTRAINT_ARRAY);
  vtkGetMacro(ConstraintStrategy, int);
  void SetConstraintStrategyToDefault() { this->SetConstraintStrategy(DEFAULT); }
  void SetConstraintStrategyToConstraintDistance()
  {
    this->SetConstraintStrategy(CONSTRAINT_DISTANCE);
  }
  void SetConstraintStrategyToConstraintBox() { this->SetConstraintStrategy(CONSTRAINT_BOX); }
  void SetConstraintStrategyToConstraintArray() { this->SetConstraintStrategy(CONSTRAINT_ARRAY); }
  ///@}

  ///@{
  /**
   * Specify a constraint distance for point motion (this defines a a local
   * constraint sphere which is placed around each point to restrict its
   * motion). By default, if a point data array constraint distance (named
   * "SmoothingConstraints") is provided in the input point data, then the
   * array takes precedence. By default, the constraint distance is
   * 0.001. Setting the constraint strategy to CONSTRAINT_DISTANCE forces the
   * box to be used.
   */
  vtkSetClampMacro(ConstraintDistance, double, 0.0, VTK_FLOAT_MAX);
  vtkGetMacro(ConstraintDistance, double);
  ///@}

  ///@{
  /**
   * Specify a constraint box for point motion. By default, if a point data
   * array constraint distance (named "SmoothingConstraints") is provided in
   * the input point data, then the array takes precedence. By default, the
   * constraint box is (1,1,1). Setting the constraint strategy to
   * CONSTRAINT_BOX forces the box to be used.
   */
  vtkSetVector3Macro(ConstraintBox, double);
  vtkGetVectorMacro(ConstraintBox, double, 3);
  ///@}

  ///@{
  /**
   * Set / get the point smoothing stencils. Here we are repurposing a cell
   * array to define stencils. Basically what's happening is that each point
   * is treated a "cell" connected to a list of point ids (i.e., the
   * "stencil") that defines the smoothing edge connections. By default, no
   * smoothing stencils are defined.
   */
  vtkSetSmartPointerMacro(SmoothingStencils, vtkCellArray);
  vtkGetSmartPointerMacro(SmoothingStencils, vtkCellArray);
  ///@}

  ///@{
  /**
   * Turn on/off the generation of scalar distance values. By default, the
   * generation of error scalars is disabled.
   */
  vtkSetMacro(GenerateErrorScalars, bool);
  vtkGetMacro(GenerateErrorScalars, bool);
  vtkBooleanMacro(GenerateErrorScalars, bool);
  ///@}

  ///@{
  /**
   * Turn on/off the generation of error vectors. By default, the generation
   * of error vectors is disabled.
   */
  vtkSetMacro(GenerateErrorVectors, bool);
  vtkGetMacro(GenerateErrorVectors, bool);
  vtkBooleanMacro(GenerateErrorVectors, bool);
  ///@}

  ///@{
  /**
   * Set/get the desired precision for the output types. See the documentation
   * for the vtkAlgorithm::DesiredOutputPrecision enum for an explanation of
   * the available precision settings.
   */
  vtkSetMacro(OutputPointsPrecision, int);
  vtkGetMacro(OutputPointsPrecision, int);
  ///@}

protected:
  vtkConstrainedSmoothingFilter();
  ~vtkConstrainedSmoothingFilter() override = default;

  double Convergence;
  int NumberOfIterations;
  double RelaxationFactor;

  int ConstraintStrategy;
  double ConstraintDistance;
  double ConstraintBox[3];
  vtkSmartPointer<vtkCellArray> SmoothingStencils;

  bool GenerateErrorScalars;
  bool GenerateErrorVectors;
  int OutputPointsPrecision;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkConstrainedSmoothingFilter(const vtkConstrainedSmoothingFilter&) = delete;
  void operator=(const vtkConstrainedSmoothingFilter&) = delete;
};
VTK_ABI_NAMESPACE_END

#endif
