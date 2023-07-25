// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPointSmoothingFilter
 * @brief   adjust point positions to form a pleasing, packed arrangement
 *
 *
 * vtkPointSmoothingFilter modifies the coordinates of the input points of a
 * vtkPointSet by adjusting their position to create a smooth distribution
 * (and thereby form a pleasing packing of the points). Smoothing is
 * performed by considering the effects of neighboring points on one
 * another. Smoothing in its simplest form (geometric) is simply a variant of
 * Laplacian smoothing where each point moves towards the average position of
 * its neighboring points. Next, uniform smoothing uses a cubic cutoff
 * function to produce repulsive forces between close points and attractive
 * forces that are a little further away. Smoothing can be further controlled
 * either by a scalar field, by a tensor field, or a frame field (the user
 * can specify the nature of the smoothing operation). If controlled by a
 * scalar field, then each input point is assumed to be surrounded by a
 * isotropic sphere scaled by the scalar field; if controlled by a tensor
 * field, then each input point is assumed to be surrounded by an
 * anisotropic, oriented ellipsoid aligned to the the tensor eigenvectors and
 * scaled by the determinate of the tensor. A frame field also assumes a
 * surrounding, ellipsoidal shape except that the inversion of the ellipsoid
 * tensor is already performed. Typical usage of this filter is to perform a
 * smoothing (also referred to as packing) operation (i.e., first execute
 * this filter) and then combine it with a glyph filter (e.g., vtkTensorGlyph
 * or vtkGlyph3D) to visualize the packed points.
 *
 * Smoothing depends on a local neighborhood of nearby points. In general,
 * the larger the neighborhood size, the greater the reduction in high
 * frequency information. (The memory and/or computational requirements of
 * the algorithm may also significantly increase.) The PackingRadius (and
 * PackingFactor) controls what points are considered close. The
 * PackingRadius can be computed automatically, or specified by the user.
 * (The product of PackingRadius*PackingFactor is referred to as the scaling
 * factor alpha in the paper cited below. This provides a convenient way to
 * combine automatic PackingRadius computation based on average between
 * particle neighborhoods, and then adjust it with the PackingFactor.)
 *
 * Any vtkPointSet type can be provided as input, and the output will contain
 * the same number of new points each of which is adjusted to a new position.
 *
 * Note that the algorithm requires the use of a spatial point locator. The
 * point locator is used to build a local neighborhood of the points
 * surrounding each point. It is also used to perform interpolation as the
 * point positions are adjusted.
 *
 * The algorithm incrementally adjusts the point positions through an
 * iterative process. Basically points are moved due to the influence of
 * neighboring points. Iterations continue until the specified number of
 * iterations is reached, or convergence occurs. Convergence occurs when the
 * maximum displacement of any point is less than the convergence value. As
 * points move, both the local connectivity and data attributes associated
 * with each point must be updated. Rather than performing these expensive
 * operations after every iteration, a number of sub-iterations Si can be
 * specified. If Si > 1, then the neighborhood and attribute value updates
 * occur only every Si'th iteration. Using sub-iterations can improve
 * performance significantly.
 *
 * @warning
 * Geometric smoothing defines a one-sided attractive force between
 * particles. Thus particles tend to clump together, and the entire set of
 * points (with enough iterations and appropriate PackingRadius) can converge
 * to a single position. This can be mitigated by turning on point
 * constraints, which limit the movement of "boundary" points.
 *
 * @warning
 * This class has been loosely inspired by the paper by Kindlmann and Westin
 * "Diffusion Tensor Visualization with Glyph Packing". However, several
 * computational shortcuts, and generalizations have been used for performance
 * and utility reasons.
 *
 * @warning
 * This class has been threaded with vtkSMPTools. Using TBB or other
 * non-sequential type (set in the CMake variable
 * VTK_SMP_IMPLEMENTATION_TYPE) may improve performance significantly.
 *
 * @sa
 * vtkTensorWidget vtkTensorGlyph vtkSmoothPolyDataFilter vtkGlyph3D
 */

#ifndef vtkPointSmoothingFilter_h
#define vtkPointSmoothingFilter_h

#include "vtkFiltersPointsModule.h" // For export macro
#include "vtkPointSetAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkAbstractPointLocator;
class vtkDataArray;
class vtkPlane;

class VTKFILTERSPOINTS_EXPORT vtkPointSmoothingFilter : public vtkPointSetAlgorithm
{
public:
  ///@{
  /**
   * Standard methods for instantiation, obtaining type information, and
   * printing information.
   */
  static vtkPointSmoothingFilter* New();
  vtkTypeMacro(vtkPointSmoothingFilter, vtkPointSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  ///@{
  /**
   * Specify the neighborhood size. This controls the number of surrounding
   * points that can affect a point to be smoothed.
   */
  vtkSetClampMacro(NeighborhoodSize, int, 4, 128);
  vtkGetMacro(NeighborhoodSize, int);
  ///@}

  /**
   * Specify how smoothing is to be controlled.
   */
  enum
  {
    DEFAULT_SMOOTHING = 0,
    GEOMETRIC_SMOOTHING,
    UNIFORM_SMOOTHING,
    SCALAR_SMOOTHING,
    TENSOR_SMOOTHING,
    FRAME_FIELD_SMOOTHING
  };

  ///@{
  /**
   * Control how smoothing is to be performed. By default, if a point frame
   * field is available then frame field smoothing will be performed; then if
   * point tensors are available then anisotropic tensor smoothing will be
   * used; the next choice is to use isotropic scalar smoothing; and finally
   * if no frame field, tensors, or scalars are available, uniform smoothing
   * will be used. If both scalars, tensors, and /or a frame field are
   * present, the user can specify which to use; or to use uniform or
   * geometric smoothing.
   */
  vtkSetClampMacro(SmoothingMode, int, DEFAULT_SMOOTHING, FRAME_FIELD_SMOOTHING);
  vtkGetMacro(SmoothingMode, int);
  void SetSmoothingModeToDefault() { this->SetSmoothingMode(DEFAULT_SMOOTHING); }
  void SetSmoothingModeToGeometric() { this->SetSmoothingMode(GEOMETRIC_SMOOTHING); }
  void SetSmoothingModeToUniform() { this->SetSmoothingMode(UNIFORM_SMOOTHING); }
  void SetSmoothingModeToScalars() { this->SetSmoothingMode(SCALAR_SMOOTHING); }
  void SetSmoothingModeToTensors() { this->SetSmoothingMode(TENSOR_SMOOTHING); }
  void SetSmoothingModeToFrameField() { this->SetSmoothingMode(FRAME_FIELD_SMOOTHING); }
  ///@}

  ///@{
  /**
   * Specify the name of the frame field to use for smoothing. This
   * information is only necessary if a frame field smoothing is enabled.
   */
  virtual void SetFrameFieldArray(vtkDataArray*);
  vtkGetObjectMacro(FrameFieldArray, vtkDataArray);
  ///@}

  ///@{
  /**
   * Specify the number of smoothing iterations.
   */
  vtkSetClampMacro(NumberOfIterations, int, 0, VTK_INT_MAX);
  vtkGetMacro(NumberOfIterations, int);
  ///@}

  ///@{
  /**
   * Specify the number of smoothing subiterations. This specifies the
   * frequency of connectivity and data attribute updates.
   */
  vtkSetClampMacro(NumberOfSubIterations, int, 1, VTK_INT_MAX);
  vtkGetMacro(NumberOfSubIterations, int);
  ///@}

  ///@{
  /**
   * Specify the maximum smoothing step size for each smoothing iteration. This
   * step size limits the the distance over which a point can move in each
   * iteration.  As in all iterative methods, the stability of the process is
   * sensitive to this parameter. In general, small step size and large
   * numbers of iterations are more stable than a larger step size and a
   * smaller numbers of iterations.
   */
  vtkSetClampMacro(MaximumStepSize, double, 0.0, VTK_DOUBLE_MAX);
  vtkGetMacro(MaximumStepSize, double);
  ///@}

  ///@{
  /**
   * Specify a convergence criterion for the iteration
   * process. Smaller numbers result in more smoothing iterations.
   */
  vtkSetClampMacro(Convergence, double, 0.0, 1.0);
  vtkGetMacro(Convergence, double);
  ///@}

  ///@{
  /**
   * Enable or disable constraints on points. Point constraints are used to
   * prevent points from moving, or to move only on a plane. This can prevent
   * shrinking or growing point clouds. If enabled, a local topological
   * analysis is performed to determine whether a point should be marked
   * "Fixed" i.e., never moves; "Plane", the point only moves on a plane; or
   * "Unconstrained", the point can move freely. If all points in the
   * neighborhood surrounding a point are in the cone defined by FixedAngle,
   * then the point is classified "Fixed." If all points in the neighborhood
   * surrounding a point are in the cone defined by BoundaryAngle, then the
   * point is classified "Plane." (The angles are expressed in degrees.)
   */
  vtkSetMacro(EnableConstraints, bool);
  vtkGetMacro(EnableConstraints, bool);
  vtkBooleanMacro(EnableConstraints, bool);
  vtkSetClampMacro(FixedAngle, double, 0, 90);
  vtkGetMacro(FixedAngle, double);
  vtkSetClampMacro(BoundaryAngle, double, 0, 120);
  vtkGetMacro(BoundaryAngle, double);
  ///@}

  ///@{
  /**
   * If point constraints are enabled, an output scalar indicating the
   * classification of points can be generated.
   */
  vtkSetMacro(GenerateConstraintScalars, bool);
  vtkGetMacro(GenerateConstraintScalars, bool);
  vtkBooleanMacro(GenerateConstraintScalars, bool);
  ///@}

  ///@{
  /**
   * If point constraints are enabled, an output vector indicating the
   * average normal at each point can be generated.
   */
  vtkSetMacro(GenerateConstraintNormals, bool);
  vtkGetMacro(GenerateConstraintNormals, bool);
  vtkBooleanMacro(GenerateConstraintNormals, bool);
  ///@}

  ///@{
  /**
   * Enable / disable the computation of a packing radius. By default,
   * a packing radius is computed as one half of the average distance
   * between neighboring points. (Point neighbors are defined by the
   * neighborhood size.)
   */
  vtkSetMacro(ComputePackingRadius, bool);
  vtkGetMacro(ComputePackingRadius, bool);
  vtkBooleanMacro(ComputePackingRadius, bool);
  ///@}

  ///@{
  /**
   * Specify the packing radius R. This only takes effect if
   * ComputePackingRadius is off. Note that the for two points separated by
   * radius r, a repulsive force is generated when 0<=r<=R, and a repulsive
   * force when R<=r<=(1+AttractionFactor*R). By default, the PackingRadius
   * is automatically computed, but when ComputePackingRadius is off, then
   * manually setting the PackingRadius is allowed. Note that the
   * PackingRadius is updated after the algorithm runs (useful to examine the
   * computed packing radius).
   */
  vtkSetClampMacro(PackingRadius, double, 0.0, VTK_DOUBLE_MAX);
  vtkGetMacro(PackingRadius, double);
  ///@}

  ///@{
  /**
   * Specify the packing factor. Larger numbers tend to loosen the overall
   * packing of points. Note however that if the point density in a region is
   * high, then the packing factor may have little effect (due to mutual
   * inter-particle constraints). The default value is 1.0. (Note that a
   * characteristic inter-particle radius R is computed at the onset of the
   * algorithm (or can be manually specified). Within 0<=r<=R*PackingFactor a
   * repulsive force is generated.)
   */
  vtkSetClampMacro(PackingFactor, double, 0.1, 10.0);
  vtkGetMacro(PackingFactor, double);
  ///@}

  ///@{
  /**
   * Control the relative distance of inter-particle attraction. A value of
   * 1.0 means that the radius of the attraction region is the same as the
   * radius of repulsion. By default, a value of 0.5 is used (e.g., in the
   * region 0<=r<=R a repulsive force is generated, while in R<r<=R*1.5 an
   * attractive force is generated).
   */
  vtkSetClampMacro(AttractionFactor, double, 0.1, 10.0);
  vtkGetMacro(AttractionFactor, double);
  ///@}

  /**
   * Specify how point motion is to be constrained.
   */
  enum
  {
    UNCONSTRAINED_MOTION = 0,
    PLANE_MOTION
  };

  ///@{
  /**
   * Specify how to constrain the motion of points. By default, point motion is
   * unconstrained. Points can also be constrained to a plane. If constrained to
   * a plane, then an instance of vtkPlane must be specified.
   */
  vtkSetMacro(MotionConstraint, int);
  vtkGetMacro(MotionConstraint, int);
  void SetMotionConstraintToUnconstrained() { this->SetMotionConstraint(UNCONSTRAINED_MOTION); }
  void SetMotionConstraintToPlane() { this->SetMotionConstraint(PLANE_MOTION); }
  ///@}

  ///@{
  /**
   * Specify the plane to which point motion is constrained. Only required if
   * MotionConstraint is set to UNCONSTRAINED_MOTION.
   */
  void SetPlane(vtkPlane*);
  vtkGetObjectMacro(Plane, vtkPlane);
  ///@}

  ///@{
  /**
   * Specify a point locator. By default a vtkStaticPointLocator is
   * used. The locator performs efficient searches to locate points
   * around a sample point.
   */
  void SetLocator(vtkAbstractPointLocator* locator);
  vtkGetObjectMacro(Locator, vtkAbstractPointLocator);
  ///@}

protected:
  vtkPointSmoothingFilter();
  ~vtkPointSmoothingFilter() override;

  // Control the smoothing
  int NeighborhoodSize;
  int SmoothingMode;
  int NumberOfIterations;
  int NumberOfSubIterations;
  double MaximumStepSize;
  double Convergence;
  vtkDataArray* FrameFieldArray;

  // Support the algorithm
  vtkAbstractPointLocator* Locator;

  // Constraints
  bool EnableConstraints;
  double FixedAngle;
  double BoundaryAngle;
  bool GenerateConstraintScalars;
  bool GenerateConstraintNormals;

  // Packing radius and related
  bool ComputePackingRadius;
  double PackingRadius;
  double PackingFactor;
  double AttractionFactor;

  // Motion constraints
  int MotionConstraint;
  vtkPlane* Plane;

  // Pipeline support
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int FillInputPortInformation(int port, vtkInformation* info) override;

private:
  vtkPointSmoothingFilter(const vtkPointSmoothingFilter&) = delete;
  void operator=(const vtkPointSmoothingFilter&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
