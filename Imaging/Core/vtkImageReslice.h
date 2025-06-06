// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkImageReslice
 * @brief   Reslices a volume along a new set of axes.
 *
 * vtkImageReslice is the swiss-army-knife of image geometry filters:
 * It can permute, rotate, flip, scale, resample, deform, and pad image
 * data in any combination with reasonably high efficiency.  Simple
 * operations such as permutation, resampling and padding are done
 * with similar efficiently to the specialized vtkImagePermute,
 * vtkImageResample, and vtkImagePad filters.  There are a number of
 * tasks that vtkImageReslice is well suited for:
 * <p>1) Application of transformations (either linear or nonlinear) to
 * an image. It is sometimes convenient to use vtkImageChangeInformation
 * to center the image first, so that scales and rotations occur around
 * the center rather than around the lower-left corner of the image.
 * <p>2) Resampling of one data set to match the voxel sampling of
 * a second data set via the SetInformationInput() method, e.g. for
 * the purpose of comparing two images or combining two images.
 * A transformation, either linear or nonlinear, can be applied
 * at the same time via the SetResliceTransform method if the two
 * images are not in the same coordinate space.
 * <p>3) Extraction of slices from an image volume. The most convenient
 * way to do this is to use SetOutputDirection() to specify the
 * orientation of the output slices. The columns of the direction
 * matrix specify the x, y, and z axes for the output volume or slice,
 * and SetOutputOrigin() can be used to specify the position. You can
 * use these methods together with SetResliceTransform() in order
 * to extract slices in a certain orientation while simultaneously
 * applying a transformation to the coordinate system.
 * @warning
 * This filter is very inefficient if the output X dimension is 1.
 * @sa
 * vtkAbstractImageInterpolator vtkAbstractTransform vtkImageResliceToColors
 */

#ifndef vtkImageReslice_h
#define vtkImageReslice_h

#include "vtkImagingCoreModule.h" // For export macro
#include "vtkThreadedImageAlgorithm.h"
#include "vtkWrappingHints.h"

// interpolation mode constants
#define VTK_RESLICE_NEAREST VTK_NEAREST_INTERPOLATION
#define VTK_RESLICE_LINEAR VTK_LINEAR_INTERPOLATION
#define VTK_RESLICE_CUBIC VTK_CUBIC_INTERPOLATION

VTK_ABI_NAMESPACE_BEGIN
class vtkImageData;
class vtkAbstractTransform;
class vtkMatrix4x4;
class vtkImageStencilData;
class vtkScalarsToColors;
class vtkAbstractImageInterpolator;

class VTKIMAGINGCORE_EXPORT VTK_MARSHALAUTO vtkImageReslice : public vtkThreadedImageAlgorithm
{
public:
  static vtkImageReslice* New();
  vtkTypeMacro(vtkImageReslice, vtkThreadedImageAlgorithm);

  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * This method is used to set up the axes for the output voxels.
   * The output Spacing, Origin, and Extent specify the locations
   * of the voxels within the coordinate system defined by the axes.
   * The ResliceAxes are used most often to permute the data, e.g.
   * to extract ZY or XZ slices of a volume as 2D XY images.
   * <p>The first column of the matrix specifies the x-axis
   * vector (the fourth element must be set to zero), the second
   * column specifies the y-axis, and the third column the
   * z-axis.  The fourth column is the origin of the
   * axes (the fourth element must be set to one).
   * <p>An alternative to SetResliceAxes() is to use
   * SetResliceAxesDirectionCosines() to set the directions of the
   * axes and SetResliceAxesOrigin() to set the origin of the axes.
   */
  virtual void SetResliceAxes(vtkMatrix4x4*);
  vtkGetObjectMacro(ResliceAxes, vtkMatrix4x4);
  ///@}

  ///@{
  /**
   * Specify the direction cosines for the ResliceAxes (i.e. the
   * first three elements of each of the first three columns of
   * the ResliceAxes matrix).  This will modify the current
   * ResliceAxes matrix, or create a new matrix if none exists.
   */
  void SetResliceAxesDirectionCosines(double x0, double x1, double x2, double y0, double y1,
    double y2, double z0, double z1, double z2);
  void SetResliceAxesDirectionCosines(const double x[3], const double y[3], const double z[3])
  {
    this->SetResliceAxesDirectionCosines(x[0], x[1], x[2], y[0], y[1], y[2], z[0], z[1], z[2]);
  }
  void SetResliceAxesDirectionCosines(const double xyz[9])
  {
    this->SetResliceAxesDirectionCosines(
      xyz[0], xyz[1], xyz[2], xyz[3], xyz[4], xyz[5], xyz[6], xyz[7], xyz[8]);
  }
  void GetResliceAxesDirectionCosines(double x[3], double y[3], double z[3]);
  void GetResliceAxesDirectionCosines(double xyz[9])
  {
    this->GetResliceAxesDirectionCosines(&xyz[0], &xyz[3], &xyz[6]);
  }
  double* GetResliceAxesDirectionCosines() VTK_SIZEHINT(9)
  {
    this->GetResliceAxesDirectionCosines(this->ResliceAxesDirectionCosines);
    return this->ResliceAxesDirectionCosines;
  }
  ///@}

  ///@{
  /**
   * Specify the origin for the ResliceAxes (i.e. the first three
   * elements of the final column of the ResliceAxes matrix).
   * This will modify the current ResliceAxes matrix, or create
   * new matrix if none exists.
   */
  void SetResliceAxesOrigin(double x, double y, double z);
  void SetResliceAxesOrigin(const double xyz[3])
  {
    this->SetResliceAxesOrigin(xyz[0], xyz[1], xyz[2]);
  }
  void GetResliceAxesOrigin(double xyz[3]);
  double* GetResliceAxesOrigin() VTK_SIZEHINT(3)
  {
    this->GetResliceAxesOrigin(this->ResliceAxesOrigin);
    return this->ResliceAxesOrigin;
  }
  ///@}

  ///@{
  /**
   * Set a transform to be applied to the resampling grid that has
   * been defined via the ResliceAxes and the output Origin, Spacing
   * and Extent.  Note that applying a transform to the resampling
   * grid (which lies in the output coordinate system) is
   * equivalent to applying the inverse of that transform to
   * the input volume.  Nonlinear transforms such as vtkGridTransform
   * and vtkThinPlateSplineTransform can be used here.
   */
  virtual void SetResliceTransform(vtkAbstractTransform*);
  vtkGetObjectMacro(ResliceTransform, vtkAbstractTransform);
  ///@}

  ///@{
  /**
   * Set a vtkImageData from which the default Spacing, Origin,
   * and WholeExtent of the output will be copied.  The spacing,
   * origin, and extent will be permuted according to the
   * ResliceAxes.  Any values set via SetOutputSpacing,
   * SetOutputOrigin, and SetOutputExtent will override these
   * values.  By default, the Spacing, Origin, and WholeExtent
   * of the Input are used.
   */
  virtual void SetInformationInput(vtkImageData*);
  vtkGetObjectMacro(InformationInput, vtkImageData);
  ///@}

  ///@{
  /**
   * Specify whether to transform the spacing, origin and extent
   * of the Input (or the InformationInput) according to the
   * direction cosines and origin of the ResliceAxes before applying
   * them as the default output spacing, origin and extent
   * (default: On).
   */
  vtkSetMacro(TransformInputSampling, vtkTypeBool);
  vtkBooleanMacro(TransformInputSampling, vtkTypeBool);
  vtkGetMacro(TransformInputSampling, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Turn this on if you want to guarantee that the extent of the
   * output will be large enough to ensure that none of the
   * data will be cropped (default: Off).
   */
  vtkSetMacro(AutoCropOutput, vtkTypeBool);
  vtkBooleanMacro(AutoCropOutput, vtkTypeBool);
  vtkGetMacro(AutoCropOutput, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Turn on wrap-pad feature (default: Off).
   */
  vtkSetMacro(Wrap, vtkTypeBool);
  vtkGetMacro(Wrap, vtkTypeBool);
  vtkBooleanMacro(Wrap, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Turn on mirror-pad feature (default: Off).
   * This will override the wrap-pad.
   */
  vtkSetMacro(Mirror, vtkTypeBool);
  vtkGetMacro(Mirror, vtkTypeBool);
  vtkBooleanMacro(Mirror, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Extend the apparent input border by a half voxel (default: On).
   * This changes how interpolation is handled at the borders of the
   * input image: if the center of an output voxel is beyond the edge
   * of the input image, but is within a half voxel width of the edge
   * (using the input voxel width), then the value of the output voxel
   * is calculated as if the input's edge voxels were duplicated past
   * the edges of the input.
   * This has no effect if Mirror or Wrap are on.
   */
  vtkSetMacro(Border, vtkTypeBool);
  vtkGetMacro(Border, vtkTypeBool);
  vtkBooleanMacro(Border, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Set the border thickness for BorderOn() (default: 0.5).
   * See SetBorder() for more information.
   */
  ///@{
  vtkSetMacro(BorderThickness, double);
  vtkGetMacro(BorderThickness, double);
  ///@}

  /**
   * Set interpolation mode (default: nearest neighbor).
   */
  vtkSetClampMacro(InterpolationMode, int, VTK_RESLICE_NEAREST, VTK_RESLICE_CUBIC);
  vtkGetMacro(InterpolationMode, int);
  void SetInterpolationModeToNearestNeighbor() { this->SetInterpolationMode(VTK_RESLICE_NEAREST); }
  void SetInterpolationModeToLinear() { this->SetInterpolationMode(VTK_RESLICE_LINEAR); }
  void SetInterpolationModeToCubic() { this->SetInterpolationMode(VTK_RESLICE_CUBIC); }
  virtual const char* GetInterpolationModeAsString();
  ///@}

  ///@{
  /**
   * Set the interpolator to use.  The default interpolator
   * supports the Nearest, Linear, and Cubic interpolation modes.
   */
  virtual void SetInterpolator(vtkAbstractImageInterpolator* sampler);
  virtual vtkAbstractImageInterpolator* GetInterpolator();
  ///@}

  ///@{
  /**
   * Set the slab mode, for generating thick slices. The default is Mean.
   * If SetSlabNumberOfSlices(N) is called with N greater than one, then
   * each output slice will actually be a composite of N slices.  This method
   * specifies the compositing mode to be used.
   */
  vtkSetClampMacro(SlabMode, int, VTK_IMAGE_SLAB_MIN, VTK_IMAGE_SLAB_SUM);
  vtkGetMacro(SlabMode, int);
  void SetSlabModeToMin() { this->SetSlabMode(VTK_IMAGE_SLAB_MIN); }
  void SetSlabModeToMax() { this->SetSlabMode(VTK_IMAGE_SLAB_MAX); }
  void SetSlabModeToMean() { this->SetSlabMode(VTK_IMAGE_SLAB_MEAN); }
  void SetSlabModeToSum() { this->SetSlabMode(VTK_IMAGE_SLAB_SUM); }
  virtual const char* GetSlabModeAsString();
  ///@}

  ///@{
  /**
   * Set the number of slices that will be combined to create the slab.
   */
  vtkSetMacro(SlabNumberOfSlices, int);
  vtkGetMacro(SlabNumberOfSlices, int);
  ///@}

  ///@{
  /**
   * Use trapezoid integration for slab computation.  All this does is
   * weigh the first and last slices by half when doing sum and mean.
   * It is off by default.
   */
  vtkSetMacro(SlabTrapezoidIntegration, vtkTypeBool);
  vtkBooleanMacro(SlabTrapezoidIntegration, vtkTypeBool);
  vtkGetMacro(SlabTrapezoidIntegration, vtkTypeBool);
  ///@}

  ///@{
  /**
   * The slab spacing as a fraction of the output slice spacing.
   * When one of the various slab modes is chosen, each output slice is
   * produced by generating several "temporary" output slices and then
   * combining them according to the slab mode.  By default, the spacing
   * between these temporary slices is the Z component of the OutputSpacing.
   * This method sets the spacing between these temporary slices to be a
   * fraction of the output spacing.
   */
  vtkSetMacro(SlabSliceSpacingFraction, double);
  vtkGetMacro(SlabSliceSpacingFraction, double);
  ///@}

  ///@{
  /**
   * Turn on and off optimizations (default on, they should only be
   * turned off for testing purposes).
   */
  vtkSetMacro(Optimization, vtkTypeBool);
  vtkGetMacro(Optimization, vtkTypeBool);
  vtkBooleanMacro(Optimization, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Set a value to add to all the output voxels.
   * After a sample value has been interpolated from the input image, the
   * equation u = (v + ScalarShift)*ScalarScale will be applied to it before
   * it is written to the output image.  The result will always be clamped to
   * the limits of the output data type.
   */
  vtkSetMacro(ScalarShift, double);
  vtkGetMacro(ScalarShift, double);
  ///@}

  ///@{
  /**
   * Set multiplication factor to apply to all the output voxels.
   * After a sample value has been interpolated from the input image, the
   * equation u = (v + ScalarShift)*ScalarScale will be applied to it before
   * it is written to the output image.  The result will always be clamped to
   * the limits of the output data type.
   */
  vtkSetMacro(ScalarScale, double);
  vtkGetMacro(ScalarScale, double);
  ///@}

  ///@{
  /**
   * Set the scalar type of the output to be different from the input.
   * The default value is -1, which means that the input scalar type will be
   * used to set the output scalar type.  Otherwise, this must be set to one
   * of the following types: VTK_CHAR, VTK_SIGNED_CHAR, VTK_UNSIGNED_CHAR,
   * VTK_SHORT, VTK_UNSIGNED_SHORT, VTK_INT, VTK_UNSIGNED_INT, VTK_FLOAT,
   * or VTK_DOUBLE.  Other types are not permitted.  If the output type
   * is an integer type, the output will be rounded and clamped to the
   * limits of the type.
   */
  vtkSetMacro(OutputScalarType, int);
  vtkGetMacro(OutputScalarType, int);
  ///@}

  ///@{
  /**
   * Set the background color (for multi-component images).
   */
  vtkSetVector4Macro(BackgroundColor, double);
  vtkGetVector4Macro(BackgroundColor, double);
  ///@}

  ///@{
  /**
   * Set background grey level (for single-component images).
   */
  void SetBackgroundLevel(double v) { this->SetBackgroundColor(v, v, v, v); }
  double GetBackgroundLevel() { return this->GetBackgroundColor()[0]; }
  ///@}

  ///@{
  /**
   * Set the voxel spacing for the output data.  The default output
   * spacing is the input spacing permuted through the ResliceAxes.
   */
  virtual void SetOutputSpacing(double x, double y, double z);
  virtual void SetOutputSpacing(const double a[3]) { this->SetOutputSpacing(a[0], a[1], a[2]); }
  vtkGetVector3Macro(OutputSpacing, double);
  void SetOutputSpacingToDefault();
  ///@}

  ///@{
  /**
   * Set the direction for the output data.  By default, the direction of
   * the input data is passed to the output.  But if SetOutputDirection()
   * is used, then the image will be resliced according to the new output
   * direction.  Unlike SetResliceAxes(), this does not change the physical
   * coordinate system for the image.  Instead, it changes the orientation
   * of the sampling grid while maintaining the same physical coordinate
   * system.
   */
  virtual void SetOutputDirection(double xx, double xy, double xz, double yx, double yy, double yz,
    double zx, double zy, double zz);
  virtual void SetOutputDirection(const double a[9])
  {
    this->SetOutputDirection(a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7], a[8]);
  }
  vtkGetVector3Macro(OutputDirection, double);
  void SetOutputDirectionToDefault();
  ///@}

  ///@{
  /**
   * Set the origin for the output data.  The default output origin
   * is the input origin permuted through the ResliceAxes.
   */
  virtual void SetOutputOrigin(double x, double y, double z);
  virtual void SetOutputOrigin(const double a[3]) { this->SetOutputOrigin(a[0], a[1], a[2]); }
  vtkGetVector3Macro(OutputOrigin, double);
  void SetOutputOriginToDefault();
  ///@}

  ///@{
  /**
   * Set the extent for the output data.  The default output extent
   * is the input extent permuted through the ResliceAxes.
   */
  virtual void SetOutputExtent(int a, int b, int c, int d, int e, int f);
  virtual void SetOutputExtent(const int a[6])
  {
    this->SetOutputExtent(a[0], a[1], a[2], a[3], a[4], a[5]);
  }
  vtkGetVector6Macro(OutputExtent, int);
  void SetOutputExtentToDefault();
  ///@}

  ///@{
  /**
   * Force the dimensionality of the output to either 1, 2,
   * 3 or 0 (default: 3).  If the dimensionality is 2D, then
   * the Z extent of the output is forced to (0,0) and the Z
   * origin of the output is forced to 0.0 (i.e. the output
   * extent is confined to the xy plane).  If the dimensionality
   * is 1D, the output extent is confined to the x axis.
   * For 0D, the output extent consists of a single voxel at
   * (0,0,0).
   */
  vtkSetMacro(OutputDimensionality, int);
  vtkGetMacro(OutputDimensionality, int);
  ///@}

  /**
   * When determining the modified time of the filter,
   * this check the modified time of the transform and matrix.
   */
  vtkMTimeType GetMTime() override;

  /**
   * Report object referenced by instances of this class.
   */
  void ReportReferences(vtkGarbageCollector*) override;

  ///@{
  /**
   * Convenient methods for switching between nearest-neighbor and linear
   * interpolation.
   * InterpolateOn() is equivalent to SetInterpolationModeToLinear() and
   * InterpolateOff() is equivalent to SetInterpolationModeToNearestNeighbor()
   * You should not use these methods if you use the SetInterpolationMode
   * methods.
   */
  void SetInterpolate(int t)
  {
    if (t && !this->GetInterpolate())
    {
      this->SetInterpolationModeToLinear();
    }
    else if (!t && this->GetInterpolate())
    {
      this->SetInterpolationModeToNearestNeighbor();
    }
  }
  void InterpolateOn() { this->SetInterpolate(1); }
  void InterpolateOff() { this->SetInterpolate(0); }
  int GetInterpolate() { return (this->GetInterpolationMode() != VTK_RESLICE_NEAREST); }
  ///@}

  ///@{
  /**
   * Use a stencil to limit the calculations to a specific region of
   * the output.  Portions of the output that are 'outside' the stencil
   * will be cleared to the background color.
   */
  void SetStencilData(vtkImageStencilData* stencil);
  vtkImageStencilData* GetStencil();
  ///@}

  ///@{
  /**
   * Generate an output stencil that defines which pixels were
   * interpolated and which pixels were out-of-bounds of the input.
   */
  vtkSetMacro(GenerateStencilOutput, vtkTypeBool);
  vtkGetMacro(GenerateStencilOutput, vtkTypeBool);
  vtkBooleanMacro(GenerateStencilOutput, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Get the output stencil.
   */
  vtkAlgorithmOutput* GetStencilOutputPort() { return this->GetOutputPort(1); }
  vtkImageStencilData* GetStencilOutput();
  void SetStencilOutput(vtkImageStencilData* stencil);
  ///@}

protected:
  vtkImageReslice();
  ~vtkImageReslice() override;

  vtkMatrix4x4* ResliceAxes;
  double ResliceAxesDirectionCosines[9];
  double ResliceAxesOrigin[3];
  vtkAbstractTransform* ResliceTransform;
  vtkAbstractImageInterpolator* Interpolator;
  vtkImageData* InformationInput;
  vtkTypeBool Wrap;
  vtkTypeBool Mirror;
  vtkTypeBool Border;
  int InterpolationMode;
  vtkTypeBool Optimization;
  int SlabMode;
  int SlabNumberOfSlices;
  vtkTypeBool SlabTrapezoidIntegration;
  double SlabSliceSpacingFraction;
  double ScalarShift;
  double ScalarScale;
  double BorderThickness;
  double BackgroundColor[4];
  double OutputDirection[9];
  double OutputOrigin[3];
  double OutputSpacing[3];
  int OutputExtent[6];
  int OutputScalarType;
  int OutputDimensionality;
  vtkTypeBool TransformInputSampling;
  vtkTypeBool AutoCropOutput;
  int HitInputExtent;
  int UsePermuteExecute;
  int ComputeOutputSpacing;
  bool PassDirectionToOutput;
  int ComputeOutputOrigin;
  int ComputeOutputExtent;
  vtkTypeBool GenerateStencilOutput;

  vtkMatrix4x4* IndexMatrix;
  vtkAbstractTransform* OptimizedTransform;

  /**
   * This should be set to 1 by derived classes that override the
   * ConvertScalars method.
   */
  vtkTypeBool HasConvertScalars;

  /**
   * This should be overridden by derived classes that operate on
   * the interpolated data before it is placed in the output.
   */
  virtual int ConvertScalarInfo(int& scalarType, int& numComponents);

  /**
   * This should be overridden by derived classes that operate on
   * the interpolated data before it is placed in the output.
   * The input data will usually be double or float (since the
   * interpolation routines use floating-point) but it could be
   * of any type.  This method will be called from multiple threads,
   * so it must be thread-safe in derived classes.
   */
  virtual void ConvertScalars(void* inPtr, void* outPtr, int inputType, int inputNumComponents,
    int count, int idX, int idY, int idZ, int threadId);

  void ConvertScalarsBase(void* inPtr, void* outPtr, int inputType, int inputNumComponents,
    int count, int idX, int idY, int idZ, int threadId)
  {
    this->ConvertScalars(
      inPtr, outPtr, inputType, inputNumComponents, count, idX, idY, idZ, threadId);
  }

  /**
   * For derived classes, this should be called at the very end of
   * RequestInformation() to ensure that various bit and pieces of
   * state information are correctly set prior to execution.
   */
  int RequestInformationBase(vtkInformationVector**, vtkInformationVector*);

  void GetAutoCroppedOutputBounds(
    vtkInformation* inInfo, const double outDirection[9], double bounds[6]);
  void AllocateOutputData(vtkImageData* output, vtkInformation* outInfo, int* uExtent) override;
  vtkImageData* AllocateOutputData(vtkDataObject*, vtkInformation*) override;
  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestUpdateExtent(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  void ThreadedRequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector, vtkImageData*** inData, vtkImageData** outData, int ext[6],
    int id) override;
  int FillInputPortInformation(int port, vtkInformation* info) override;
  int FillOutputPortInformation(int port, vtkInformation* info) override;

  vtkMatrix4x4* GetIndexMatrix(vtkInformation* inInfo, vtkInformation* outInfo);
  vtkAbstractTransform* GetOptimizedTransform() { return this->OptimizedTransform; }

private:
  vtkImageReslice(const vtkImageReslice&) = delete;
  void operator=(const vtkImageReslice&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
