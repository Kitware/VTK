//=============================================================================
//
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2012 Sandia Corporation.
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//=============================================================================
/**
 * @class   vtkmHistogram
 * @brief   generate a histogram out of a scalar data
 *
 * vtkmHistogram is a filter that generates a histogram out of a scalar data.
 * The histogram consists of a certain number of bins specified by the user, and
 * the user can fetch the range and bin delta after completion.
 *
 */

#ifndef vtkmHistogram_h
#define vtkmHistogram_h

#include "vtkAcceleratorsVTKmModule.h" //required for correct export
#include "vtkTableAlgorithm.h"

class vtkDoubleArray;

class VTKACCELERATORSVTKM_EXPORT vtkmHistogram : public vtkTableAlgorithm
{
public:
  vtkTypeMacro(vtkmHistogram, vtkTableAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkmHistogram* New();

  //@{
  /**
   * Specify number of bins.  Default is 10.
   */
  vtkSetMacro(NumberOfBins, size_t);
  vtkGetMacro(NumberOfBins, size_t);
  //@}

  //@{
  /**
   * Specify the range to use to generate the histogram. They are only used when
   * UseCustomBinRanges is set to true.
   */
  vtkSetVector2Macro(CustomBinRange, double);
  vtkGetVector2Macro(CustomBinRange, double);
  //@}

  //@{
  /**
   * When set to true, CustomBinRanges will  be used instead of using the full
   * range for the selected array. By default, set to false.
   */
  vtkSetMacro(UseCustomBinRanges, bool);
  vtkGetMacro(UseCustomBinRanges, bool);
  vtkBooleanMacro(UseCustomBinRanges, bool);
  //@}

  //@{
  /**
   * Get/Set if first and last bins must be centered around the min and max
   * data. This is only used when UseCustomBinRanges is set to false.
   * Default is false.
   */
  vtkSetMacro(CenterBinsAroundMinAndMax, bool);
  vtkGetMacro(CenterBinsAroundMinAndMax, bool);
  vtkBooleanMacro(CenterBinsAroundMinAndMax, bool);
  //@}

  //@{
  /**
   * Return the range used to generate the histogram.
   */
  vtkGetVectorMacro(ComputedRange, double, 2);
  //@}

  //@{
  /**
   * Return the bin delta of the computed field.
   */
  vtkGetMacro(BinDelta, double);
  //@}

protected:
  vtkmHistogram();
  ~vtkmHistogram() override;

  virtual int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int FillInputPortInformation(int port, vtkInformation* info) override;

private:
  vtkmHistogram(const vtkmHistogram&) = delete;
  void operator=(const vtkmHistogram&) = delete;

  void FillBinExtents(vtkDoubleArray* binExtents);

  size_t NumberOfBins;
  double BinDelta;
  double CustomBinRange[2];
  bool UseCustomBinRanges;
  bool CenterBinsAroundMinAndMax;
  double ComputedRange[2];
};

#endif // vtkmHistogram_h
// VTK-HeaderTest-Exclude: vtkmHistogram.h
