/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkConstrainedSmoothingFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkConstrainedSmoothingFilter
 * @brief   adjust point positions using constrained smoothing
 *
 * vtkConstrainedSmoothingFilter is a filter that adjusts point coordinates
 * using a modified Laplacian smoothing approach. The effect is to "relax"
 * the mesh, making the cells better shaped and the points more evenly
 * distributed. Note that this filter operates on any vtkPointSet and derived
 * classes. Cell topology is never modified; note however if the constraints
 * are too lax, cells may self-intersect or otherwise be deformed in
 * unfavorable ways.
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
 * distance, or provide an input point data array (of type vtkDoubleArray)
 * named "SmoothingConstraints." The filter's constraint distance is applied
 * to all points; whereas the smoothing data array may have different
 * constraint values per point. If provided by the user, by default the
 * smoothing data array takes precedence over the filter's constraint
 * distance.
 *
 * @warning
 * The smoothing process reduces high frequency information in the geometry
 * of the mesh. With excessive smoothing important details may be lost, and
 * the surface may shrink towards the centroid. The constraints on point
 * movement help significantly in preventing shrinkage from happening.
 *
 * @sa
 * vtkWindowedSincPolyDataFilter vtkSmoothPolyDataFilter
 * vtkExtractEdges
 */

#ifndef vtkConstrainedSmoothingFilter_h
#define vtkConstrainedSmoothingFilter_h

#include "vtkCellArray.h"         // For point smoothing stencils
#include "vtkFiltersCoreModule.h" // For export macro
#include "vtkPointSetAlgorithm.h"

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
    CONSTRAINT_ARRAY = 2
  };

  ///@{
  /**
   * Indicate how to apply constraints. By default, a constraint array takes
   * precedence over the filter's constraint distance, but if not available
   * then the constraint distance is used. If a CONSTRAINT_ARRAY strategy is
   * specified, and no constraint array is available from the point data,
   * then no constraints are provided. Note that is also possible to turn off
   * constraints completely by simply specifying a very large constraint
   * distance. The default constraint strategy is DEFAULT.
   */
  vtkSetClampMacro(ConstraintStrategy, int, DEFAULT, CONSTRAINT_ARRAY);
  vtkGetMacro(ConstraintStrategy, int);
  void SetConstraintStrategyToDefault() { this->SetConstraintStrategy(DEFAULT); }
  void SetConstraintStrategyToConstraintDistance()
  {
    this->SetConstraintStrategy(CONSTRAINT_DISTANCE);
  }
  void SetConstraintStrategyToConstraintArray() { this->SetConstraintStrategy(CONSTRAINT_ARRAY); }
  ///@}

  ///@{
  /**
   * Specify a constraint distance for point motion. By default, if a point
   * data array constraint distance (named "SmoothingConstraints") is provided
   * in the input point data, then the array takes precedence. By default, the
   * constraint distance is 0.001.
   */
  vtkSetClampMacro(ConstraintDistance, double, 0.0, VTK_FLOAT_MAX);
  vtkGetMacro(ConstraintDistance, double);
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
  vtkSmartPointer<vtkCellArray> SmoothingStencils;

  bool GenerateErrorScalars;
  bool GenerateErrorVectors;
  int OutputPointsPrecision;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkConstrainedSmoothingFilter(const vtkConstrainedSmoothingFilter&) = delete;
  void operator=(const vtkConstrainedSmoothingFilter&) = delete;
};

#endif
