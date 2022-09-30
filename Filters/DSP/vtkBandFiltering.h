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
 * vtkBandFiltering performs a band filtering in frequency space. It takes as input a table with
 * at least a column for a specific quantity and an optional time array like the vtkTableFFT. The
 * output will be an table with the mean of this quantity (in the original unit or in decibels) for
 * each frequencies defined in the frequency column (in Hz).
 */

#ifndef vtkBandFiltering_h
#define vtkBandFiltering_h

#include "vtkFiltersDSPModule.h" // for export macro
#include "vtkTableAlgorithm.h"
#include "vtkTableFFT.h" // for vtkTableFFT enums

#include <string> // std::string

VTK_ABI_NAMESPACE_BEGIN
class VTKFILTERSDSP_EXPORT vtkBandFiltering : public vtkTableAlgorithm
{
public:
  vtkTypeMacro(vtkBandFiltering, vtkTableAlgorithm);
  static vtkBandFiltering* New();
  void PrintSelf(ostream& os, vtkIndent indent) override;

  enum
  {
    OCTAVE = 0,
    THIRD_OCTAVE,
    CUSTOM
  };

  ///@{
  /**
   * Specify if the filter should use octave, third or custom octave band.
   *
   * Default is OCTAVE
   *
   * @see SetOctaveSubdivision
   */
  vtkGetMacro(BandFilteringMode, int);
  vtkSetClampMacro(BandFilteringMode, int, OCTAVE, CUSTOM);
  ///@}

  ///@{
  /**
   * Get/Set the number of octave subdivision when using
   * BandFilteringMode == CUSTOM . Only odd numbers are valid.
   * When using even numbers, the number just below will be used.
   * 1 is equivalent to using the OCTAVE mode, and 3 the THIRD_OCTAVE
   * mode.
   *
   * Default is 1
   *
   * @see vtkBandFiltering::SetBandFilteringMode(int)
   */
  vtkGetMacro(OctaveSubdivision, int);
  vtkSetClampMacro(OctaveSubdivision, int, 1, VTK_INT_MAX);
  ///@}

  ///@{
  /**
   * Get/set the windowing function for the FFT. Only used if ApplyFFT is true.
   * Windowing function enum is defined in vtkTableFFT.
   *
   * Default is vtkTableFFT::HANNING.
   */
  vtkGetMacro(WindowType, int);
  vtkSetClampMacro(WindowType, int, vtkTableFFT::HANNING, vtkTableFFT::RECTANGULAR);
  ///@}

  ///@{
  /**
   * Specify the frequency sample rate in Hz.
   * If ApplyFFT is true: this will be used if the filter cannot find a time column
   * If ApplyFFT is false: this will be used if the filter cannot find a frequency column.
   *
   * @see vtkBandFiltering::SetFrequencyArrayName(std::string)
   *
   * Default is 10000.
   */
  vtkGetMacro(DefaultSamplingRate, double);
  vtkSetMacro(DefaultSamplingRate, double);
  ///@}

  ///@{
  /**
   * Specify if we want to output band filtering in dB. Reference value used is the one
   * for sound pressure, i.e. 2e-5 (Pa).
   *
   * Default is false.
   */
  vtkGetMacro(OutputInDecibel, bool);
  vtkSetMacro(OutputInDecibel, bool);
  ///@}

  ///@{
  /**
   * Specify the reference value used to convert the input quantity to decibel.
   *
   * Default is 2e-5
   */
  vtkGetMacro(ReferenceValue, double);
  vtkSetMacro(ReferenceValue, double);
  ///@}

  ///@{
  /**
   * Specify if one want to apply an FFT on the input before computing the band filtering.
   * It should be set to true if the input is a sound signal and false if it has already been
   * processed by an FFT. When taking an FFT as its input, the filter expects it to be a
   * signal of real values where its mirrored part has already been removed.
   *
   * If false then one should specify which array is the Frequency array.
   *
   * Default is true.
   *
   * @see vtkTableFFT::SetReturnOnesided(bool)
   */
  vtkGetMacro(ApplyFFT, bool);
  vtkSetMacro(ApplyFFT, bool);
  ///@}

  ///@{
  /**
   * When ApplyFFT is false, specify the frequency array to use when filtering the signals.
   * If no array with this name is found then use the specified default sample rate to create
   * a new one.
   *
   * Default is "Frequency".
   *
   * @see vtkTableFFT::SetDefaultSamplingRate(double)
   * @see vtkTableFFT::SetApplyFFT(bool)
   */
  vtkGetMacro(FrequencyArrayName, std::string);
  vtkSetMacro(FrequencyArrayName, std::string);
  ///@}

protected:
  vtkBandFiltering() = default;
  ~vtkBandFiltering() override = default;

  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  static vtkSmartPointer<vtkTable> ApplyFFTInternal(
    vtkTable* input, int window, double defaultSampleRate);

private:
  vtkBandFiltering(const vtkBandFiltering&) = delete;
  void operator=(const vtkBandFiltering&) = delete;

  int WindowType = vtkTableFFT::HANNING;
  double DefaultSamplingRate = 10000.0;
  std::string FrequencyArrayName = "Frequency";

  bool ApplyFFT = true;
  int BandFilteringMode = vtkBandFiltering::OCTAVE;
  int OctaveSubdivision = 1;
  bool OutputInDecibel = false;
  double ReferenceValue = 2e-5;
};
VTK_ABI_NAMESPACE_END

#endif // vtkBandFiltering_h
