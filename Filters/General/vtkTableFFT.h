// -*- c++ -*-
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTableFFT.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*-------------------------------------------------------------------------
  Copyright 2009 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

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

#include "vtkDeprecation.h"          // For VTK_DEPRECATED_IN_9_2_0
#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkSmartPointer.h"         // For internal method.
#include "vtkTableAlgorithm.h"

#include <memory> // For unique_ptr

class VTKFILTERSGENERAL_EXPORT vtkTableFFT : public vtkTableAlgorithm
{
public:
  vtkTypeMacro(vtkTableFFT, vtkTableAlgorithm);
  static vtkTableFFT* New();
  void PrintSelf(ostream& os, vtkIndent indent) override;

  enum // Windowing functions
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
   * Specify if the output should be normalized. This has 2 consequences :
   * first is that for each block the mean signal value is removed from the input
   * signal before doing the actual FFT. Second is that it will take the norm of
   * the resulting imaginary values and normalize it using Parseval's theorem.
   *
   * Default is false
   */
  vtkGetMacro(Normalize, bool);
  vtkSetMacro(Normalize, bool);
  vtkBooleanMacro(Normalize, bool);
  ///@}

  ///@{
  /**
   * Specify if the input should be split in multiple blocks to compute
   * an average fft across all blocks. It uses the Welch method except
   * that it averages raw FFTs instead of periodograms.
   *
   * @see vtkTableFFT::SetNumberOfBlock(int)
   * @see vtkTableFFT::SetBlockSize(int)
   *
   * Default is false
   */
  vtkGetMacro(AverageFft, bool);
  virtual void SetAverageFft(bool);
  vtkBooleanMacro(AverageFft, bool);
  ///@}

  ///@{
  /**
   * Specify if the filter should use the optimized discrete fourier transform for
   * real values. This will cause output columns to have from n to ((n / 2) + 1) rows.
   * If OptimizeForRealInput is On but the input contains columns with 2 components,
   * these columns will be ignored.
   *
   * Default is false
   */
  vtkGetMacro(OptimizeForRealInput, bool);
  vtkSetMacro(OptimizeForRealInput, bool);
  vtkBooleanMacro(OptimizeForRealInput, bool);
  ///@}

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

  ///@{
  /**
   * Only used if @c AverageFft is true
   *
   * Specify the number of blocks to use when computing the average fft over
   * the whole input sample array. Blocks can overlap if @c NumberOfBlock times
   * @c BlockSize is superior to the input signal size.
   *
   * This parameter is ignored if @c BlockSize is superior
   * to the number of samples of the input array.
   *
   * @see vtkTableFFT::SetAverageFft(bool)
   *
   * Default is 2
   */
  vtkGetMacro(NumberOfBlock, int);
  vtkSetMacro(NumberOfBlock, int);
  ///@}

  ///@{
  /**
   * Only used if @c AverageFft is true
   *
   * Specify the number of samples to use for each block. This should be a power of 2.
   * If not, the closest power of two will be used anyway.
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
   * Specify if output array should be prefixed by "FFT_" or not.
   * This behavior was introduced in v9.1. Default is false.
   *
   * Deprecated in v9.2.
   */
  VTK_DEPRECATED_IN_9_2_0(
    "Deprecated in favor of always keeping the output array names the same as the input.")
  vtkGetMacro(PrefixOutputArrays, bool);
  VTK_DEPRECATED_IN_9_2_0(
    "Deprecated in favor of always keeping the output array names the same as the input.")
  vtkSetMacro(PrefixOutputArrays, bool);
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

  bool Normalize = false;
  bool AverageFft = false;
  bool OptimizeForRealInput = false;
  bool CreateFrequencyColumn = false;
  int NumberOfBlock = 2;
  vtkIdType BlockSize = 1024;
  int WindowingFunction = RECTANGULAR;
  double DefaultSampleRate = 1e4;

  bool PrefixOutputArrays = false;

  struct vtkInternal;
  std::unique_ptr<vtkInternal> Internals;
};

#endif // vtkTableFFT_h
