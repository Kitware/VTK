// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkMaskPointsFilter
 * @brief   extract points within an image/volume mask
 *
 * vtkMaskPointsFilter extracts points that are inside an image mask. The
 * image mask is a second input to the filter. Points that are inside a voxel
 * marked "inside" are copied to the output. The image mask can be generated
 * by vtkPointOccupancyFilter, with optional image processing steps performed
 * on the mask. Thus vtkPointOccupancyFilter and vtkMaskPointsFilter are
 * generally used together, with a pipeline of image processing algorithms
 * in between the two filters.
 *
 * Note also that this filter is a subclass of vtkPointCloudFilter which has
 * the ability to produce an output mask indicating which points were
 * selected for output. It also has an optional second output containing the
 * points that were masked out (i.e., outliers) during processing.
 *
 * Finally, the mask value indicating non-selection of points (i.e., the
 * empty value) may be specified. The second input, masking image, is
 * typically of type unsigned char so the empty value is of this type as
 * well.
 *
 * @warning
 * During processing, points not within the masking image/volume are
 * considered outside and never extracted.
 *
 * @warning
 * This class has been threaded with vtkSMPTools. Using TBB or other
 * non-sequential type (set in the CMake variable
 * VTK_SMP_IMPLEMENTATION_TYPE) may improve performance significantly.
 *
 * @sa
 * vtkPointOccupancyFilter vtkPointCloudFilter
 */

#ifndef vtkMaskPointsFilter_h
#define vtkMaskPointsFilter_h

#include "vtkFiltersPointsModule.h" // For export macro
#include "vtkPointCloudFilter.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkImageData;
class vtkPointSet;

class VTKFILTERSPOINTS_EXPORT vtkMaskPointsFilter : public vtkPointCloudFilter
{
public:
  ///@{
  /**
   * Standard methods for instantiating, obtaining type information, and
   * printing information.
   */
  static vtkMaskPointsFilter* New();
  vtkTypeMacro(vtkMaskPointsFilter, vtkPointCloudFilter);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  ///@{
  /**
   * Specify the masking image. It must be of type vtkImageData.
   */
  void SetMaskData(vtkDataObject* source);
  vtkDataObject* GetMask();
  ///@}

  /**
   * Specify the masking image. It is vtkImageData output from an algorithm.
   */
  void SetMaskConnection(vtkAlgorithmOutput* algOutput);

  ///@{
  /**
   * Set / get the values indicating whether a voxel is empty. By default, an
   * empty voxel is marked with a zero value. Any point inside a voxel marked
   * empty is not selected for output. All other voxels with a value that is
   * not equal to the empty value are selected for output.
   */
  vtkSetMacro(EmptyValue, unsigned char);
  vtkGetMacro(EmptyValue, unsigned char);
  ///@}

protected:
  vtkMaskPointsFilter();
  ~vtkMaskPointsFilter() override;

  unsigned char EmptyValue; // what value indicates a voxel is empty

  // All derived classes must implement this method. Note that a side effect of
  // the class is to populate the PointMap. Zero is returned if there is a failure.
  int FilterPoints(vtkPointSet* input) override;

  // Support second input
  int FillInputPortInformation(int port, vtkInformation* info) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestUpdateExtent(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  vtkImageData* Mask; // just a placeholder during execution

private:
  vtkMaskPointsFilter(const vtkMaskPointsFilter&) = delete;
  void operator=(const vtkMaskPointsFilter&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
