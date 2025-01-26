// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkGaussianSplatter
 * @brief   splat points into a volume with an elliptical, Gaussian distribution
 *
 * vtkGaussianSplatter is a filter that injects input points into a
 * structured points (volume) dataset. As each point is injected, it "splats"
 * or distributes values to nearby voxels. Data is distributed using an
 * elliptical, Gaussian distribution function. The distribution function is
 * modified using scalar values (expands distribution) or normals
 * (creates ellipsoidal distribution rather than spherical).
 *
 * In general, the Gaussian distribution function f(x) around a given
 * splat point p is given by
 *
 *     f(x) = ScaleFactor * exp( ExponentFactor*((r/Radius)**2) )
 *
 * where x is the current voxel sample point; r is the distance |x-p|
 * ExponentFactor <= 0.0, and ScaleFactor can be multiplied by the scalar
 * value of the point p that is currently being splatted.
 *
 * If points normals are present (and NormalWarping is on), then the splat
 * function becomes elliptical (as compared to the spherical one described
 * by the previous equation). The Gaussian distribution function then
 * becomes:
 *
 *     f(x) = ScaleFactor *
 *               exp( ExponentFactor*( ((rxy/E)**2 + z**2)/R**2) )
 *
 * where E is a user-defined eccentricity factor that controls the elliptical
 * shape of the splat; z is the distance of the current voxel sample point
 * along normal N; and rxy is the distance of x in the direction
 * perpendicular to N.
 *
 * This class is typically used to convert point-valued distributions into
 * a volume representation. The volume is then usually iso-surfaced or
 * volume rendered to generate a visualization. It can be used to create
 * surfaces from point distributions, or to create structure (i.e.,
 * topology) when none exists.
 *
 * @warning
 * The input to this filter is any dataset type. This filter can be used
 * to resample any form of data, i.e., the input data need not be
 * unstructured.
 *
 * @warning
 * Some voxels may never receive a contribution during the splatting process.
 * The final value of these points can be specified with the "NullValue"
 * instance variable.
 *
 * @warning
 * This class has been threaded with vtkSMPTools. Using TBB or other
 * non-sequential type (set in the CMake variable
 * VTK_SMP_IMPLEMENTATION_TYPE) may improve performance significantly.
 *
 * @sa
 * vtkShepardMethod vtkCheckerboardSplatter
 */

#ifndef vtkGaussianSplatter_h
#define vtkGaussianSplatter_h

#include "vtkImageAlgorithm.h"
#include "vtkImagingHybridModule.h" // For export macro

#include <cmath> // for std::exp

#define VTK_ACCUMULATION_MODE_MIN 0
#define VTK_ACCUMULATION_MODE_MAX 1
#define VTK_ACCUMULATION_MODE_SUM 2

VTK_ABI_NAMESPACE_BEGIN
class vtkDoubleArray;
class vtkCompositeDataSet;
class vtkGaussianSplatterAlgorithm;

class VTKIMAGINGHYBRID_EXPORT vtkGaussianSplatter : public vtkImageAlgorithm
{
public:
  vtkTypeMacro(vtkGaussianSplatter, vtkImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Construct object with dimensions=(50,50,50); automatic computation of
   * bounds; a splat radius of 0.1; an exponent factor of -5; and normal and
   * scalar warping turned on.
   */
  static vtkGaussianSplatter* New();

  ///@{
  /**
   * Set / get the dimensions of the sampling structured point set. Higher
   * values produce better results but are much slower.
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
   * Set / get the radius of propagation of the splat. This value is expressed
   * as a percentage of the length of the longest side of the sampling
   * volume. Smaller numbers greatly reduce execution time.
   */
  vtkSetClampMacro(Radius, double, 0.0, 1.0);
  vtkGetMacro(Radius, double);
  ///@}

  ///@{
  /**
   * Multiply Gaussian splat distribution by this value. If ScalarWarping
   * is on, then the Scalar value will be multiplied by the ScaleFactor
   * times the Gaussian function.
   */
  vtkSetClampMacro(ScaleFactor, double, 0.0, VTK_DOUBLE_MAX);
  vtkGetMacro(ScaleFactor, double);
  ///@}

  ///@{
  /**
   * Set / get the sharpness of decay of the splats. This is the
   * exponent constant in the Gaussian equation. Normally this is
   * a negative value.
   */
  vtkSetMacro(ExponentFactor, double);
  vtkGetMacro(ExponentFactor, double);
  ///@}

  ///@{
  /**
   * Turn on/off the generation of elliptical splats. If normal warping is
   * on, then the input normals affect the distribution of the splat. This
   * boolean is used in combination with the Eccentricity ivar.
   */
  vtkSetMacro(NormalWarping, vtkTypeBool);
  vtkGetMacro(NormalWarping, vtkTypeBool);
  vtkBooleanMacro(NormalWarping, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Control the shape of elliptical splatting. Eccentricity is the ratio
   * of the major axis (aligned along normal) to the minor (axes) aligned
   * along other two axes. So Eccentricity > 1 creates needles with the
   * long axis in the direction of the normal; Eccentricity<1 creates
   * pancakes perpendicular to the normal vector.
   */
  vtkSetClampMacro(Eccentricity, double, 0.001, VTK_DOUBLE_MAX);
  vtkGetMacro(Eccentricity, double);
  ///@}

  ///@{
  /**
   * Turn on/off the scaling of splats by scalar value.
   */
  vtkSetMacro(ScalarWarping, vtkTypeBool);
  vtkGetMacro(ScalarWarping, vtkTypeBool);
  vtkBooleanMacro(ScalarWarping, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Turn on/off the capping of the outer boundary of the volume
   * to a specified cap value. This can be used to close surfaces
   * (after iso-surfacing) and create other effects.
   */
  vtkSetMacro(Capping, vtkTypeBool);
  vtkGetMacro(Capping, vtkTypeBool);
  vtkBooleanMacro(Capping, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Specify the cap value to use. (This instance variable only has effect
   * if the ivar Capping is on.)
   */
  vtkSetMacro(CapValue, double);
  vtkGetMacro(CapValue, double);
  ///@}

  ///@{
  /**
   * Specify the scalar accumulation mode. This mode expresses how scalar
   * values are combined when splats are overlapped. The Max mode acts
   * like a set union operation and is the most commonly used; the Min
   * mode acts like a set intersection, and the sum is just weird.
   */
  vtkSetClampMacro(AccumulationMode, int, VTK_ACCUMULATION_MODE_MIN, VTK_ACCUMULATION_MODE_SUM);
  vtkGetMacro(AccumulationMode, int);
  void SetAccumulationModeToMin() { this->SetAccumulationMode(VTK_ACCUMULATION_MODE_MIN); }
  void SetAccumulationModeToMax() { this->SetAccumulationMode(VTK_ACCUMULATION_MODE_MAX); }
  void SetAccumulationModeToSum() { this->SetAccumulationMode(VTK_ACCUMULATION_MODE_SUM); }
  const char* GetAccumulationModeAsString();
  ///@}

  ///@{
  /**
   * Set the Null value for output points not receiving a contribution from the
   * input points. (This is the initial value of the voxel samples.)
   */
  vtkSetMacro(NullValue, double);
  vtkGetMacro(NullValue, double);
  ///@}

  ///@{
  /**
   * Compute the size of the sample bounding box automatically from the
   * input data. This is an internal helper function.
   */
  void ComputeModelBounds(vtkDataSet* input, vtkImageData* output, vtkInformation* outInfo);
  void ComputeModelBounds(
    vtkCompositeDataSet* input, vtkImageData* output, vtkInformation* outInfo);
  ///@}

  ///@{
  /**
   * Provide access to templated helper class. Note that SamplePoint() method
   * is public here because some compilers don't handle friend functions
   * properly.
   */
  friend class vtkGaussianSplatterAlgorithm;
  double SamplePoint(double x[3]) // for compilers who can't handle this
  {
    return (this->*Sample)(x);
  }
  void SetScalar(vtkIdType idx, double dist2, double* sPtr)
  {
    double v =
      (this->*SampleFactor)(this->S) * std::exp(this->ExponentFactor * (dist2) / (this->Radius2));
    ///@}

    if (!this->Visited[idx])
    {
      this->Visited[idx] = 1;
      *sPtr = v;
    }
    else
    {
      switch (this->AccumulationMode)
      {
        case VTK_ACCUMULATION_MODE_MIN:
          if (*sPtr > v)
          {
            *sPtr = v;
          }
          break;
        case VTK_ACCUMULATION_MODE_MAX:
          if (*sPtr < v)
          {
            *sPtr = v;
          }
          break;
        case VTK_ACCUMULATION_MODE_SUM:
          *sPtr += v;
          break;
      }
    } // not first visit
  }

protected:
  vtkGaussianSplatter();
  ~vtkGaussianSplatter() override = default;

  int FillInputPortInformation(int port, vtkInformation* info) override;
  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  void Cap(vtkDoubleArray* s);

  int SampleDimensions[3];   // dimensions of volume to splat into
  double Radius;             // maximum distance splat propagates (as fraction 0->1)
  double ExponentFactor;     // scale exponent of gaussian function
  double ModelBounds[6];     // bounding box of splatting dimensions
  vtkTypeBool NormalWarping; // on/off warping of splat via normal
  double Eccentricity;       // elliptic distortion due to normals
  vtkTypeBool ScalarWarping; // on/off warping of splat via scalar
  double ScaleFactor;        // splat size influenced by scale factor
  vtkTypeBool Capping;       // Cap side of volume to close surfaces
  double CapValue;           // value to use for capping
  int AccumulationMode;      // how to combine scalar values

  double Gaussian(double x[3]);
  double EccentricGaussian(double x[3]);
  double ScalarSampling(double s) { return this->ScaleFactor * s; }
  double PositionSampling(double) { return this->ScaleFactor; }

private:
  double Radius2;
  double (vtkGaussianSplatter::*Sample)(double x[3]);
  double (vtkGaussianSplatter::*SampleFactor)(double s);
  char* Visited;
  double Eccentricity2;
  double* P;
  double* N;
  double S;
  double Origin[3];
  double Spacing[3];
  double SplatDistance[3];
  double NullValue;

  vtkGaussianSplatter(const vtkGaussianSplatter&) = delete;
  void operator=(const vtkGaussianSplatter&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
