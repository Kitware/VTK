// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkImageProbeFilter
 * @brief   sample image values at specified point positions
 *
 * vtkImageProbeFilter interpolates an image at specified point positions.
 * This filter has two inputs: the Input and Source. The Input geometric
 * structure is passed through the filter, and the Output point scalars
 * are interpolated from the Source image.
 *
 * This filter can be used to resample an image onto a set of arbitrarily
 * placed sample points.  For example, if you have a surface data set
 * (i.e. a vtkPolyData that has been tessellated so that its points are
 * very closely spaced), you can color the polydata from the image points.
 *
 * In general, this filter is similar to vtkProbeFilter except that the
 * Source data is always an image.  The advantages that it provides over
 * vtkProbeFilter is that it is faster, and it can take advantage of the
 * advanced interpolation methods offered by vtkAbstractImageInterpolator
 * subclasses.
 */

#ifndef vtkImageProbeFilter_h
#define vtkImageProbeFilter_h

#include "vtkDataSetAlgorithm.h"
#include "vtkImagingCoreModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkAbstractImageInterpolator;
class vtkUnsignedCharArray;
class vtkImageData;
class vtkPointData;

class VTKIMAGINGCORE_EXPORT vtkImageProbeFilter : public vtkDataSetAlgorithm
{
public:
  static vtkImageProbeFilter* New();
  vtkTypeMacro(vtkImageProbeFilter, vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Specify the data set that will be probed at the input points.
   * The Input gives the geometry (the points and cells) for the output,
   * while the Source is probed (interpolated) to generate the scalars,
   * vectors, etc. for the output points based on the point locations.
   */
  void SetSourceData(vtkDataObject* source);
  vtkDataObject* GetSource();
  ///@}

  /**
   * Specify the data set that will be probed at the input points.
   * The Input gives the geometry (the points and cells) for the output,
   * while the Source is probed (interpolated) to generate the scalars,
   * vectors, etc. for the output points based on the point locations.
   */
  void SetSourceConnection(vtkAlgorithmOutput* algOutput);

  ///@{
  /**
   * Set the interpolator to use.  If this is not set, then nearest-neighbor
   * interpolation will be used, with the tolerance of the interpolator set
   * to 0.5 (half the voxel size).
   */
  virtual void SetInterpolator(vtkAbstractImageInterpolator* interpolator);
  virtual vtkAbstractImageInterpolator* GetInterpolator() { return this->Interpolator; }
  ///@}

protected:
  vtkImageProbeFilter();
  ~vtkImageProbeFilter() override;

  int FillInputPortInformation(int port, vtkInformation* info) override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestUpdateExtent(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  /**
   * Equivalent to calling InitializeForProbing(); DoProbing().
   */
  void Probe(vtkDataSet* input, vtkImageData* source, vtkDataSet* output);

  /**
   * Initialize the interpolator and the output arrays.
   */
  virtual void InitializeForProbing(vtkDataSet* input, vtkImageData* source, vtkDataSet* output);

  /**
   * Probe appropriate points (InitializeForProbing() must be called first).
   */
  void DoProbing(vtkDataSet* input, vtkImageData* source, vtkDataSet* output);

  vtkAbstractImageInterpolator* Interpolator;
  vtkUnsignedCharArray* MaskScalars;

private:
  vtkImageProbeFilter(const vtkImageProbeFilter&) = delete;
  void operator=(const vtkImageProbeFilter&) = delete;

  class ProbePointsWorklet;
  struct ProbePointsThreadLocal;
  struct ProbePointsThreadStruct;

  /**
   * This method is called from the work threads if SMP is used, or called
   * from the main thread if SMP is not used.
   */
  void ProbePoints(vtkDataSet* input, vtkImageData* source, vtkPointData* outPD, vtkIdType startId,
    vtkIdType endId, ProbePointsThreadLocal* threadLocal);
};

VTK_ABI_NAMESPACE_END
#endif
