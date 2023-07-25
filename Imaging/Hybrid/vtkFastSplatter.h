// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkFastSplatter
 * @brief   A splatter optimized for splatting single kernels.
 *
 *
 *
 * vtkFastSplatter takes any vtkPointSet as input (of which vtkPolyData and
 * vtkUnstructuredGrid inherit).  Each point in the data set is considered to be
 * an impulse.  These impulses are convolved with a given splat image.  In other
 * words, the splat image is added to the final image at every place where there
 * is an input point.
 *
 * Note that point and cell data are thrown away.  If you want a sampling
 * of unstructured points consider vtkGaussianSplatter or vtkShepardMethod.
 *
 * Use input port 0 for the impulse data (vtkPointSet), and input port 1 for
 * the splat image (vtkImageData)
 *
 *
 * @bug
 * Any point outside of the extents of the image is thrown away, even if it is
 * close enough such that it's convolution with the splat image would overlap
 * the extents.
 *
 */

#ifndef vtkFastSplatter_h
#define vtkFastSplatter_h

#include "vtkImageAlgorithm.h"
#include "vtkImagingHybridModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class VTKIMAGINGHYBRID_EXPORT vtkFastSplatter : public vtkImageAlgorithm
{
public:
  vtkTypeMacro(vtkFastSplatter, vtkImageAlgorithm);
  static vtkFastSplatter* New();
  void PrintSelf(ostream& os, vtkIndent indent) override;

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
   * Set/get the dimensions of the output image
   */
  vtkSetVector3Macro(OutputDimensions, int);
  vtkGetVector3Macro(OutputDimensions, int);
  ///@}

  enum
  {
    NoneLimit,
    ClampLimit,
    ScaleLimit,
    FreezeScaleLimit
  };

  ///@{
  /**
   * Set/get the way voxel values will be limited.  If this is set to None (the
   * default), the output can have arbitrarily large values.  If set to clamp,
   * the output will be clamped to [MinValue,MaxValue].  If set to scale, the
   * output will be linearly scaled between MinValue and MaxValue.
   */
  vtkSetMacro(LimitMode, int);
  vtkGetMacro(LimitMode, int);
  void SetLimitModeToNone() { this->SetLimitMode(NoneLimit); }
  void SetLimitModeToClamp() { this->SetLimitMode(ClampLimit); }
  void SetLimitModeToScale() { this->SetLimitMode(ScaleLimit); }
  void SetLimitModeToFreezeScale() { this->SetLimitMode(FreezeScaleLimit); }
  ///@}

  ///@{
  /**
   * See the LimitMode method.
   */
  vtkSetMacro(MinValue, double);
  vtkGetMacro(MinValue, double);
  vtkSetMacro(MaxValue, double);
  vtkGetMacro(MaxValue, double);
  ///@}

  ///@{
  /**
   * This returns the number of points splatted (as opposed to
   * discarded for being outside the image) during the previous pass.
   */
  vtkGetMacro(NumberOfPointsSplatted, int);
  ///@}

  /**
   * Convenience function for connecting the splat algorithm source.
   * This is provided mainly for convenience using the filter with
   * ParaView, VTK users should prefer SetInputConnection(1, splat) instead.
   */
  void SetSplatConnection(vtkAlgorithmOutput*);

protected:
  vtkFastSplatter();
  ~vtkFastSplatter() override;

  double ModelBounds[6];
  int OutputDimensions[3];

  int LimitMode;
  double MinValue;
  double MaxValue;
  double FrozenScale;

  vtkImageData* Buckets;

  int FillInputPortInformation(int port, vtkInformation* info) override;
  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestUpdateExtent(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  // Used internally for converting points in world space to indices in
  // the output image.
  double Origin[3];
  double Spacing[3];

  // This is updated every time the filter executes
  int NumberOfPointsSplatted;

  // Used internally to track the data range.  When the limit mode is
  // set to FreezeScale, the data will be scaled as if this were the
  // range regardless of what it actually is.
  double LastDataMinValue;
  double LastDataMaxValue;

private:
  vtkFastSplatter(const vtkFastSplatter&) = delete;
  void operator=(const vtkFastSplatter&) = delete;
};

//-----------------------------------------------------------------------------

template <class T>
void vtkFastSplatterClamp(T* array, vtkIdType arraySize, T minValue, T maxValue)
{
  for (vtkIdType i = 0; i < arraySize; i++)
  {
    if (array[i] < minValue)
      array[i] = minValue;
    if (array[i] > maxValue)
      array[i] = maxValue;
  }
}

//-----------------------------------------------------------------------------

template <class T>
void vtkFastSplatterScale(T* array, int numComponents, vtkIdType numTuples, T minValue, T maxValue,
  double* dataMinValue, double* dataMaxValue)
{
  T* a;
  T min, max;
  *dataMinValue = 0;
  *dataMaxValue = 0;
  vtkIdType t;
  for (int c = 0; c < numComponents; c++)
  {
    // Find the min and max values in the array.
    a = array + c;
    min = max = *a;
    a += numComponents;
    for (t = 1; t < numTuples; t++, a += numComponents)
    {
      if (min > *a)
        min = *a;
      if (max < *a)
        max = *a;
    }

    // Bias everything so that 0 is really the minimum.
    if (min != 0)
    {
      for (t = 0, a = array + c; t < numTuples; t++, a += numComponents)
      {
        *a -= min;
      }
    }

    // Scale the values.
    if (max != min)
    {
      for (t = 0, a = array + c; t < numTuples; t++, a += numComponents)
      {
        *a = ((maxValue - minValue) * (*a)) / (max - min);
      }
    }

    // Bias everything again so that it lies in the correct range.
    if (minValue != 0)
    {
      for (t = 0, a = array + c; t < numTuples; t++, a += numComponents)
      {
        *a += minValue;
      }
    }
    if (c == 0)
    {
      *dataMinValue = min;
      *dataMaxValue = max;
    }
  }
}

//-----------------------------------------------------------------------------

template <class T>
void vtkFastSplatterFrozenScale(
  T* array, int numComponents, vtkIdType numTuples, T minValue, T maxValue, double min, double max)
{
  T* a;

  vtkIdType t;
  for (int c = 0; c < numComponents; c++)
  {
    // Bias everything so that 0 is really the minimum.
    if (min != 0)
    {
      for (t = 0, a = array + c; t < numTuples; t++, a += numComponents)
      {
        *a -= static_cast<T>(min);
      }
    }

    // Scale the values.
    if (max != min)
    {
      for (t = 0, a = array + c; t < numTuples; t++, a += numComponents)
      {
        *a = static_cast<T>(((maxValue - minValue) * (*a)) / (max - min));
      }
    }

    // Bias everything again so that it lies in the correct range.
    if (minValue != 0)
    {
      for (t = 0, a = array + c; t < numTuples; t++, a += numComponents)
      {
        *a += minValue;
      }
    }
  }
}

VTK_ABI_NAMESPACE_END
#endif // vtkFastSplatter_h
