/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBandFiltering.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   vtkBandFiltering
 * @brief   Band filtering for table columns
 *
 * vtkBandFiltering performs a band filtering in frequential space. It takes in input a table with
 * at least a column for a specific quantity and optionally a time array like the vtkTableFFT. The
 * output will be an table with the mean of this quantity (in the original unit or in decibels) for
 * each frequencies defined in the frequency column (in Hz).
 */

#ifndef vtkBandFiltering_h
#define vtkBandFiltering_h

#include "vtkMath.h" // for VTK_DBL_MIN
#include "vtkTableAlgorithm.h"
#include "vtkTableFFT.h" // For internal enum

#include <string> // for std::string
#include <vector> // for std::vector

class VTKFILTERSGENERAL_EXPORT vtkBandFiltering : public vtkTableAlgorithm
{
public:
  vtkTypeMacro(vtkBandFiltering, vtkTableAlgorithm);
  static vtkBandFiltering* New();
  void PrintSelf(ostream& os, vtkIndent indent) override;

  enum
  {
    OCTAVE = 0,
    THIRD_OCTAVE
  };

  ///@{
  /**
   * Specify if the filter should use octave or third octave band.
   * Default is octave band.
   */
  vtkGetMacro(BandFilteringMode, int);
  vtkSetClampMacro(BandFilteringMode, int, OCTAVE, THIRD_OCTAVE);
  ///@}

  ///@{
  /**
   * Get/set the windowing function for the FFT. See vtkTableFFT for other values.
   * Only used if ApplyFFT is true.
   * Default is Hanning.
   */
  vtkGetMacro(WindowType, int);
  vtkSetClampMacro(WindowType, int, vtkTableFFT::HANNING, vtkTableFFT::RECTANGULAR);
  ///@}

  ///@{
  /**
   * Specify the frequency sample in Hz used if the input doesn't have a time column.
   * Default is 10000.
   */
  vtkGetMacro(DefaultSamplingRate, double);
  vtkSetMacro(DefaultSamplingRate, double);
  ///@}

  ///@{
  /**
   * Specify if we want to output band filtering in dB. Note that to be able to make this
   * converstion, you need to explicitly specify the reference value that to be used.
   * Default is false.
   */
  vtkGetMacro(OutputInDecibel, bool);
  vtkSetMacro(OutputInDecibel, bool);
  ///@}

  ///@{
  /**
   * Specify if we want to apply an FFT on the input before computing the band filtering.
   * It should be set to true if you're input was an sound signal and false it's already been
   * processed by an FFT.
   * Default is true.
   */
  vtkGetMacro(ApplyFFT, bool);
  vtkSetMacro(ApplyFFT, bool);
  ///@}

  ///@{
  /**
   * Specify the reference value used to convert the input quantity to decibel.
   */
  vtkGetMacro(ReferenceValue, double);
  vtkSetMacro(ReferenceValue, double);
  ///@}

protected:
  vtkBandFiltering() = default;
  ~vtkBandFiltering() override = default;

  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  /**
   * Generate lower, center and upper band filtering from a given min/max frequency
   * range.
   */
  int GenerateOctaveBands(double fmin, double fmax, std::vector<double>& lowerBand,
    std::vector<double>& centerBand, std::vector<double>& upperBand);

private:
  vtkBandFiltering(const vtkBandFiltering&) = delete;
  void operator=(const vtkBandFiltering&) = delete;

  // FFT related parameters
  int WindowType = vtkTableFFT::RECTANGULAR;
  double DefaultSamplingRate = 10000.0;

  // Band filtering specific parameters
  int BandFilteringMode = vtkBandFiltering::OCTAVE;
  double ReferenceValue = VTK_DBL_MIN;
  bool OutputInDecibel = false;
  bool ApplyFFT = true;
};

#endif // vtkBandFiltering_h
