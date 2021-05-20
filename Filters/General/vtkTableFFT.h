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
 * It can perform the FFT per block in order to resample the input and have a
 * smaller output, and also offer a interface for windowing the input signal.
 */

#ifndef vtkTableFFT_h
#define vtkTableFFT_h

#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkSmartPointer.h"         // For internal method.
#include "vtkTableAlgorithm.h"

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

  //@{
  /**
   * Specify if the output should be normalized.
   *
   * Default is false
   */
  vtkGetMacro(Normalize, bool);
  vtkSetMacro(Normalize, bool);
  vtkBooleanMacro(Normalize, bool);
  //@}

  //@{
  /**
   * Specify if the input should be split in multiple blocks to compute
   * an average fft across all blocks.
   *
   * Default is false
   */
  vtkGetMacro(AverageFft, bool);
  virtual void SetAverageFft(bool);
  vtkBooleanMacro(AverageFft, bool);
  //@}

  //@{
  /**
   * Specify if the filter should use the optimized discrete fourier transform for
   * real values. This will cause output columns to have from n to ((n / 2) + 1) rows.
   *
   * Default is false
   */
  vtkGetMacro(OptimizeForRealInput, bool);
  vtkSetMacro(OptimizeForRealInput, bool);
  vtkBooleanMacro(OptimizeForRealInput, bool);
  //@}

  //@{
  /**
   * Specify if the filter should create a frequency column based on a column
   * named "time" (not case sensitive). An evenly-spaced time array is expected.
   *
   * Default is false
   */
  vtkGetMacro(CreateFrequencyColumn, bool);
  vtkSetMacro(CreateFrequencyColumn, bool);
  vtkBooleanMacro(CreateFrequencyColumn, bool);
  //@}

  //@{
  /**
   * Only used if @c AverageFft is true
   *
   * Specify the number of blocks to use when computing the average fft over
   * the whole input sample array. If NumberOfBlock == 1, no average is done
   * and we only compute the fft on the first @c BlockSize samples of the input data.
   *
   * This parameter is ignored if @c BlockSize is superior
   * to the number of samples of the input array.
   *
   * Default is 2.
   */
  vtkGetMacro(NumberOfBlock, int);
  vtkSetMacro(NumberOfBlock, int);
  //@}

  //@{
  /**
   * Only used if @c AverageFft is true
   *
   * Specify the number of samples to use for each block. This should be a power of 2.
   * If not, the closest power of two will be used anyway.
   *
   * Default is 1024
   */
  vtkGetMacro(BlockSize, int);
  virtual void SetBlockSize(int);
  //@}

  //@{
  /**
   * Specify the windowing function to apply on the input.
   * If @c AverageFft is true the windowing function will be
   * applied per block and not on the whole input
   *
   * Default is RECTANGULAR (does nothing).
   */
  vtkGetMacro(WindowingFunction, int);
  virtual void SetWindowingFunction(int);
  //@}

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

  struct vtkInternal;
  vtkInternal* Internals;
};

#endif // vtkTableFFT_h
