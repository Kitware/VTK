// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2009 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-NVIDIA-USGov

/**
 * @class   vtkTableFFT
 * @brief   FFT for table columns
 *
 * vtkTableFFT performs the Fast Fourier Transform on the columns of a table.
 * It can perform the FFT per block : this performs something close to the
 * Welch method but it uses raw FFTs instead of periodograms. This allows to
 * reduce the impact of noise as well as speeding up the filter when the input
 * signal is too big.
 *
 * It is also possible to apply a window on the input signal. If performing
 * the FFT per block then the window will be applied to each block instead.
 *
 * The filter will look for a "Time" array (case insensitive) to determine the
 * sampling frequency. "Time" array is considered to have the same frequency
 * all along. If no "Time" array is found then the filter use the default frequency
 * value.
 *
 * This filter will not apply the FFT on any arrays which names begin with 'vtk'.
 *
 * This filter will consider arrays with 2 components as arrays of complex numbers,
 * the first column representing the real part and the second the imaginary part.
 */

#ifndef vtkTableFFT_h
#define vtkTableFFT_h

#include "vtkFFT.h"                  // For vtkFFT::Scaling
#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkSmartPointer.h"         // For internal method.
#include "vtkTableAlgorithm.h"

#include <memory> // For unique_ptr

VTK_ABI_NAMESPACE_BEGIN

class VTKFILTERSGENERAL_EXPORT vtkTableFFT : public vtkTableAlgorithm
{
public:
  vtkTypeMacro(vtkTableFFT, vtkTableAlgorithm);
  static vtkTableFFT* New();
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Specify if the filter should create a frequency column based on a column
   * named "time" (not case sensitive). An evenly-spaced time array is expected.
   *
   * @see vtkTableFFT::SetDefaultSampleRate(double)
   *
   * Default is false
   */
  vtkGetMacro(CreateFrequencyColumn, bool);
  vtkSetMacro(CreateFrequencyColumn, bool);
  vtkBooleanMacro(CreateFrequencyColumn, bool);
  ///@}

  ///@{
  /**
   * If the "Time" column is not found then this value will be used.
   * Expressed in Hz.
   *
   * Default is 10'000 (Hz)
   */
  vtkGetMacro(DefaultSampleRate, double);
  vtkSetMacro(DefaultSampleRate, double);
  ///@}

  /**
   * Enum allowing to choose the windowing function to apply on the input signal.
   */
  enum : int
  {
    HANNING = 0,
    BARTLETT,
    SINE,
    BLACKMAN,
    RECTANGULAR,

    MAX_WINDOWING_FUNCTION
  };

  ///@{
  /**
   * Specify the windowing function to apply on the input.
   * If @c AverageFft is true the windowing function will be
   * applied per block and not on the whole input.
   *
   * Default is RECTANGULAR (does nothing)
   */
  vtkGetMacro(WindowingFunction, int);
  virtual void SetWindowingFunction(int);
  ///@}

  ///@{
  /**
   * Specify if the filter should use the optimized discrete fourier transform for
   * real values and return a onesided spectrum : this will cause output columns to
   * have from n to ((n / 2) + 1) values.
   * If ReturnOnesided is true but the input contains columns with 2 components
   * (aka complex data) or started with `vtk`, these columns will be ignored.
   *
   * Default is false
   */
  vtkGetMacro(ReturnOnesided, bool);
  vtkSetMacro(ReturnOnesided, bool);
  vtkBooleanMacro(ReturnOnesided, bool);
  ///@}

  ///@{
  /**
   * Specify if filter should use the Welch / periodogram method. If true the
   * input should be split in multiple segment to compute an average fft across
   * all segments / blocks.
   *
   * Note that in this case, complex data and array with name started with "vtk"
   * will be ignored.
   *
   * @see vtkTableFFT::SetBlockSize(int)
   * @see vtkTableFFT::SetBlockOverlap(int)
   *
   * Default is false
   */
  vtkGetMacro(AverageFft, bool);
  virtual void SetAverageFft(bool);
  vtkBooleanMacro(AverageFft, bool);

  ///@{
  /**
   * Specify if the output should be normalized so that Parseval's theorem is
   * respected. If enabled output will be scaled according to the number of samples
   * and the window energy. Else the raw FFT will be returned as is. Only used if
   * AverageFft is false.
   *
   * @see vtkTableFFT::SetAverageFft(bool)
   *
   * Default is false
   */
  vtkGetMacro(Normalize, bool);
  vtkSetMacro(Normalize, bool);
  vtkBooleanMacro(Normalize, bool);
  ///@}

  ///@}

  ///@{
  /**
   * Specify the number of samples to use for each block / segment in the Welch
   * method. Only used if AverageFft is true
   *
   * @see vtkTableFFT::SetAverageFft(bool)
   *
   * Default is 1024
   */
  vtkGetMacro(BlockSize, int);
  virtual void SetBlockSize(int);
  ///@}

  ///@{
  /**
   * Specify the number of samples which will overlap between each block / segment.
   * If value is not in a valid range (ie < 0 or >= BlockSize) then the
   * value BlockSize / 2 will be used. Only used if AverageFft is true.
   *
   * @see vtkTableFFT::SetAverageFft(bool)
   * @see vtkTableFFT::SetBlockSize(int)
   *
   * Default is -1
   */
  vtkGetMacro(BlockOverlap, int);
  vtkSetMacro(BlockOverlap, int);
  ///@}

  ///@{
  /**
   * Set what scaling should be used when applying the Welch method. It uses vtkFFT::Scaling
   * enum as values.
   *
   * @see vtkFFT::Scaling
   *
   * Default is vtkFFT::Scaling::Density (aka 0)
   */
  vtkGetMacro(ScalingMethod, int);
  vtkSetClampMacro(ScalingMethod, int, vtkFFT::Scaling::Density, vtkFFT::Scaling::Spectrum);
  ///@}

  ///@{
  /**
   * Remove trend on each segment before applying the FFT. This is a constant
   * detrend where the mean of the signal is subtracted to the signal.
   * Only used if AverageFft is true.
   *
   * @see vtkTableFFT::SetAverageFft(bool)
   *
   * Default is false.
   */
  vtkGetMacro(Detrend, bool);
  vtkSetMacro(Detrend, bool);
  vtkBooleanMacro(Detrend, bool);
  ///@}

protected:
  vtkTableFFT();
  ~vtkTableFFT() override;

  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  /**
   * Initialize the internal state before performing the actual fft.
   * This checks that the given parameters are coherent with the input and
   * tries to extract time information from a column.
   */
  void Initialize(vtkTable* input);

  /**
   * Perform the FFT on the given data array.
   */
  vtkSmartPointer<vtkDataArray> DoFFT(vtkDataArray* input);

private:
  vtkTableFFT(const vtkTableFFT&) = delete;
  void operator=(const vtkTableFFT&) = delete;

  // Common
  bool CreateFrequencyColumn = false;
  double DefaultSampleRate = 1e4;
  bool ReturnOnesided = false;
  bool AverageFft = false;
  int WindowingFunction = RECTANGULAR;
  // Direct method
  bool Normalize = false;
  // Welch method
  int BlockSize = 1024;
  int BlockOverlap = -1;
  bool Detrend = false;
  int ScalingMethod = 0;

  struct vtkInternal;
  std::unique_ptr<vtkInternal> Internals;
};

VTK_ABI_NAMESPACE_END
#endif // vtkTableFFT_h
