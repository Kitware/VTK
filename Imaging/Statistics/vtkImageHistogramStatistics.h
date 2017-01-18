/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageHistogramStatistics.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkImageHistogramStatistics
 * @brief   Compute statistics for an image
 *
 * vtkImageHistogramStatistics computes statistics such as mean, median, and
 * standard deviation.  These statistics are computed from the histogram
 * of the image, rather than from the image itself, because this is more
 * efficient than computing the statistics while traversing the pixels.
 * If the input image is of type float or double, then the precision of
 * the Mean, Median, and StandardDeviation will depend on the number of
 * histogram bins.  By default, 65536 bins are used for float data, giving
 * at least 16 bits of precision.
 * @par Thanks:
 * Thanks to David Gobbi at the Seaman Family MR Centre and Dept. of Clinical
 * Neurosciences, Foothills Medical Centre, Calgary, for providing this class.
*/

#ifndef vtkImageHistogramStatistics_h
#define vtkImageHistogramStatistics_h

#include "vtkImagingStatisticsModule.h" // For export macro
#include "vtkImageHistogram.h"

class vtkImageStencilData;
class vtkIdTypeArray;

class VTKIMAGINGSTATISTICS_EXPORT vtkImageHistogramStatistics : public vtkImageHistogram
{
public:
  static vtkImageHistogramStatistics *New();
  vtkTypeMacro(vtkImageHistogramStatistics,vtkImageHistogram);

  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Get the minimum value present in the image.  This value is computed
   * when Update() is called.
   */
  double GetMinimum() { return this->Minimum; }

  /**
   * Get the maximum value present in the image.  This value is computed
   * when Update() is called.
   */
  double GetMaximum() { return this->Maximum; }

  /**
   * Get the mean value of the image.  This value is computed when Update()
   * is called.
   */
  double GetMean() { return this->Mean; }

  /**
   * Get the median value.  This is computed when Update() is called.
   */
  double GetMedian() { return this->Median; }

  /**
   * Get the standard deviation of the values in the image.  This is
   * computed when Update() is called.
   */
  double GetStandardDeviation() { return this->StandardDeviation; }

  //@{
  /**
   * Set the percentiles to use for automatic view range computation.
   * This allows one to compute a range that does not include outliers
   * that are significantly darker or significantly brighter than the
   * rest of the pixels in the image.  The default is to use the first
   * percentile and the 99th percentile.
   */
  vtkSetVector2Macro(AutoRangePercentiles, double);
  vtkGetVector2Macro(AutoRangePercentiles, double);
  //@}

  //@{
  /**
   * Set lower and upper expansion factors to apply to the auto range
   * that was computed from the AutoRangePercentiles.  Any outliers that
   * are within this expanded range will be included, even if they are
   * beyond the percentile.  This allows inclusion of values that are
   * just slightly outside of the percentile, while rejecting values
   * that are far beyond the percentile.  The default is to expand the
   * range by a factor of 0.1 at each end.  The range will never be
   * expanded beyond the Minimum or Maximum pixel values.
   */
  vtkSetVector2Macro(AutoRangeExpansionFactors, double);
  vtkGetVector2Macro(AutoRangeExpansionFactors, double);
  //@}

  //@{
  /**
   * Get an automatically computed view range for the image, for use
   * with the lookup table or image property that is used when viewing
   * the image.  The use of this range will avoid situations where an
   * image looks too dark because a few pixels happen to be much brighter
   * than the rest.
   */
  vtkGetVector2Macro(AutoRange, double);
  //@}

protected:
  vtkImageHistogramStatistics();
  ~vtkImageHistogramStatistics() VTK_OVERRIDE;

  int RequestData(vtkInformation *,
                          vtkInformationVector **,
                          vtkInformationVector *) VTK_OVERRIDE;

  double Minimum;
  double Maximum;
  double Mean;
  double StandardDeviation;
  double Median;

  double AutoRange[2];
  double AutoRangePercentiles[2];
  double AutoRangeExpansionFactors[2];

private:
  vtkImageHistogramStatistics(const vtkImageHistogramStatistics&) VTK_DELETE_FUNCTION;
  void operator=(const vtkImageHistogramStatistics&) VTK_DELETE_FUNCTION;
};

#endif
