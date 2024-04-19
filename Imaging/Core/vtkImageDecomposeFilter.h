// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkImageDecomposeFilter
 * @brief   Filters that execute axes in series.
 *
 * This superclass molds the vtkImageIterateFilter superclass so
 * it iterates over the axes.  The filter uses dimensionality to
 * determine how many axes to execute (starting from x).
 * The filter also provides convenience methods for permuting information
 * retrieved from input, output and vtkImageData.
 */

#ifndef vtkImageDecomposeFilter_h
#define vtkImageDecomposeFilter_h

#include "vtkImageIterateFilter.h"
#include "vtkImagingCoreModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class VTKIMAGINGCORE_EXPORT vtkImageDecomposeFilter : public vtkImageIterateFilter
{
public:
  ///@{
  /**
   * Construct an instance of vtkImageDecomposeFilter filter with default
   * dimensionality 3.
   */
  vtkTypeMacro(vtkImageDecomposeFilter, vtkImageIterateFilter);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  ///@{
  /**
   * Dimensionality is the number of axes which are considered during
   * execution. To process images dimensionality would be set to 2.
   */
  void SetDimensionality(int dim);
  vtkGetMacro(Dimensionality, int);
  ///@}

  ///@{
  /**
   * Private methods kept public for template execute functions.
   */
  void PermuteIncrements(vtkIdType* increments, vtkIdType& inc0, vtkIdType& inc1, vtkIdType& inc2);
  void PermuteExtent(int* extent, int& min0, int& max0, int& min1, int& max1, int& min2, int& max2);
  ///@}

protected:
  vtkImageDecomposeFilter();
  ~vtkImageDecomposeFilter() override = default;

  int Dimensionality;

private:
  vtkImageDecomposeFilter(const vtkImageDecomposeFilter&) = delete;
  void operator=(const vtkImageDecomposeFilter&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
