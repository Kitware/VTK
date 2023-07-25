// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPointDensityFilter
 * @brief   produce density field from input point cloud
 *
 * vtkPointDensityFilter is a filter that generates a density field on a
 * volume from a point cloud. Basically the density is computed as number of
 * points in a local neighborhood per unit volume; or optionally, the number
 * of points in a local neighborhood surrounding each voxel. The local
 * neighborhood is specified as a radius around each sample position (i.e.,
 * each voxel) which can be of fixed value; or the radius can be relative to
 * the voxel size. The density computation may be further weighted by a
 * scalar value which is simply multiplied by each point's presumed density
 * of 1.0.
 *
 * To use this filter, specify an input of type vtkPointSet (i.e., has an
 * explicit representation of points). Optionally a scalar weighting function
 * can be provided (part of the input to the filter). Then specify how the
 * local spherical neighborhood is to be defined, either by a fixed radius or
 * a radius relative to the voxel size. Finally, specify how the density is
 * specified, either as a points/volume, or as number of points. (The
 * weighting scalar array will affect both of these results if provided and
 * enabled.)
 *
 * An optional capability of the filter is to compute the gradients of the
 * resulting density function (a 3-component vector), which also includes the
 * gradient magnitude (single component scalar) and classification (regions
 * of zero function, a scalar with single unsigned char value per voxel).

 * @warning
 * A point locator is used to speed up searches. By default a fast
 * vtkStaticPointLocator is used; however the user may specify an alternative
 * locator. In some situations adaptive locators may run faster depending on
 * the relative variation in point cloud density.
 *
 * @warning
 * Note that the volume calculation can be affected by the boundary. The
 * local spherical neighborhood around a "near volume boundary" voxel may
 * extend beyond the volume extent, meaning that density computation may be
 * reduced. To counter this effect, the volume may be increased in size
 * and/or resolution so that the point cloud fits well within the volume.
 *
 * @warning
 * While this class is very similar to many other of VTK's the point
 * splatting and interpolation classes, the algorithm density computation is
 * specialized to generate the density computation over a volume, does not
 * require (scalar weighting) data attributes to run, and does not require
 * multiple inputs. As an interesting side note: using the
 * vtkPointInterpolation class with a vtkLinearKernel, a (scalar) weighting
 * point attribute, a point cloud source, and an input volume produces the
 * same result as this filter does (assuming that the input volume is the
 * same).
 *
 * @warning
 * This class has been threaded with vtkSMPTools. Using TBB or other
 * non-sequential type (set in the CMake variable
 * VTK_SMP_IMPLEMENTATION_TYPE) may improve performance significantly.
 *
 * @sa
 * vtkCheckerboardSplatter vtkShepardMethod vtkGaussianSplatter
 * vtkPointInterpolator vtkSPHInterpolator
*/

#ifndef vtkPointDensityFilter_h
#define vtkPointDensityFilter_h

#include "vtkFiltersPointsModule.h" // For export macro
#include "vtkImageAlgorithm.h"

#define VTK_DENSITY_ESTIMATE_FIXED_RADIUS 0
#define VTK_DENSITY_ESTIMATE_RELATIVE_RADIUS 1

#define VTK_DENSITY_FORM_VOLUME_NORM 0
#define VTK_DENSITY_FORM_NPTS 1

VTK_ABI_NAMESPACE_BEGIN
class vtkAbstractPointLocator;

class VTKFILTERSPOINTS_EXPORT vtkPointDensityFilter : public vtkImageAlgorithm
{
public:
  ///@{
  /**
   * Standard methods for instantiating, obtaining type information, and
   * printing information.
   */
  static vtkPointDensityFilter* New();
  vtkTypeMacro(vtkPointDensityFilter, vtkImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  ///@{
  /**
   * Set / get the dimensions of the sampling volume. Higher values generally
   * produce better results but may be much slower. Note however that too
   * high a resolution can generate excessive noise; too low and data can be
   * excessively smoothed.
   */
  void SetSampleDimensions(int i, int j, int k);
  void SetSampleDimensions(int dim[3]);
  vtkGetVectorMacro(SampleDimensions, int, 3);
  ///@}

  ///@{
  /**
   * Set / get the (xmin,xmax, ymin,ymax, zmin,zmax) bounding box in which
   * the sampling is performed. If any of the (min,max) bounds values are
   * min >= max, then the bounds will be computed automatically from the input
   * data. Otherwise, the user-specified bounds will be used.
   */
  vtkSetVector6Macro(ModelBounds, double);
  vtkGetVectorMacro(ModelBounds, double, 6);
  ///@}

  ///@{
  /**
   * Set / get the relative amount to pad the model bounds if automatic
   * computation is performed. The padding is the fraction to scale
   * the model bounds in each of the x-y-z directions. By default the
   * padding is 0.10 (i.e., 10% larger in each direction).
   */
  vtkSetClampMacro(AdjustDistance, double, -1.0, 1.0);
  vtkGetMacro(AdjustDistance, double);
  ///@}

  ///@{
  /**
   * Specify the method to estimate point density. The density can be
   * calculated using a fixed sphere radius; or a sphere radius that is
   * relative to voxel size.
   */
  vtkSetClampMacro(
    DensityEstimate, int, VTK_DENSITY_ESTIMATE_FIXED_RADIUS, VTK_DENSITY_ESTIMATE_RELATIVE_RADIUS);
  vtkGetMacro(DensityEstimate, int);
  void SetDensityEstimateToFixedRadius()
  {
    this->SetDensityEstimate(VTK_DENSITY_ESTIMATE_FIXED_RADIUS);
  }
  void SetDensityEstimateToRelativeRadius()
  {
    this->SetDensityEstimate(VTK_DENSITY_ESTIMATE_RELATIVE_RADIUS);
  }
  const char* GetDensityEstimateAsString();
  ///@}

  ///@{
  /**
   * Specify the form by which the density is expressed. Either the density is
   * expressed as (number of points/local sphere volume), or as simply the
   * (number of points) within the local sphere.
   */
  vtkSetClampMacro(DensityForm, int, VTK_DENSITY_FORM_VOLUME_NORM, VTK_DENSITY_FORM_NPTS);
  vtkGetMacro(DensityForm, int);
  void SetDensityFormToVolumeNormalized() { this->SetDensityForm(VTK_DENSITY_FORM_VOLUME_NORM); }
  void SetDensityFormToNumberOfPoints() { this->SetDensityForm(VTK_DENSITY_FORM_NPTS); }
  const char* GetDensityFormAsString();
  ///@}

  ///@{
  /**
   * Set / get the radius variable defining the local sphere used to estimate
   * the density function. The Radius is used when the density estimate is
   ^ set to a fixed radius (i.e., the radius doesn't change).
   */
  vtkSetClampMacro(Radius, double, 0.0, VTK_DOUBLE_MAX);
  vtkGetMacro(Radius, double);
  ///@}

  ///@{
  /**
   * Set / get the relative radius factor defining the local sphere used to
   * estimate the density function. The relative sphere radius is equal to
   * the diagonal length of a voxel times the radius factor. The RelativeRadius
   * is used when the density estimate is set to relative radius (i.e.,
   * relative to voxel size).
   */
  vtkSetClampMacro(RelativeRadius, double, 0.0, VTK_DOUBLE_MAX);
  vtkGetMacro(RelativeRadius, double);
  ///@}

  ///@{
  /**
   * Turn on/off the weighting of point density by a scalar array. By default
   * scalar weighting is off.
   */
  vtkSetMacro(ScalarWeighting, bool);
  vtkGetMacro(ScalarWeighting, bool);
  vtkBooleanMacro(ScalarWeighting, bool);
  ///@}

  ///@{
  /**
   * Turn on/off the generation of the gradient vector, gradient magnitude
   * scalar, and function classification scalar. By default this is off. Note
   * that this will increase execution time and the size of the output. (The
   * names of these point data arrays are: "Gradient", "Gradient Magnitude",
   * and "Classification".)
   */
  vtkSetMacro(ComputeGradient, bool);
  vtkGetMacro(ComputeGradient, bool);
  vtkBooleanMacro(ComputeGradient, bool);
  ///@}

  ///@{
  /**
   * Specify a point locator. By default a vtkStaticPointLocator is
   * used. The locator performs efficient searches to locate near a
   * specified interpolation position.
   */
  void SetLocator(vtkAbstractPointLocator* locator);
  vtkGetObjectMacro(Locator, vtkAbstractPointLocator);
  ///@}

  /**
   * This enum is used to classify the behavior of the function gradient. Regions
   * where all density values used in the calculation of the gradient are zero
   * are referred to as ZERO regions. Otherwise NON_ZERO. This can be used to
   * differentiate between regions where data is available and where it is not.
   */
  enum FunctionClass
  {
    ZERO = 0,
    NON_ZERO = 1
  };

protected:
  vtkPointDensityFilter();
  ~vtkPointDensityFilter() override;

  int SampleDimensions[3];          // dimensions of volume over which to estimate density
  double ModelBounds[6];            // bounding box of splatting dimensions
  double AdjustDistance;            // how much to pad the model bounds if automatically computed
  double Origin[3], Spacing[3];     // output geometry
  int DensityEstimate;              // how to compute the density
  int DensityForm;                  // how to represent density value
  double RelativeRadius;            // Radius factor for estimating density
  double Radius;                    // Actually radius used
  bool ScalarWeighting;             // Are point densities weighted or not?
  bool ComputeGradient;             // Compute the gradient vector and magnitude
  vtkAbstractPointLocator* Locator; // accelerate point searches

  int FillInputPortInformation(int port, vtkInformation* info) override;
  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  void ComputeModelBounds(vtkDataSet* input, vtkImageData* output, vtkInformation* outInfo);

private:
  vtkPointDensityFilter(const vtkPointDensityFilter&) = delete;
  void operator=(const vtkPointDensityFilter&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
