// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkEqualizerFilter_h
#define vtkEqualizerFilter_h

#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkTableAlgorithm.h"

#include <string> // for std::string

/**
 * @class vtkEqualizerFilter
 * @brief implements an algorithm for digital signal processing
 *
 * The vtkEqualizerFilter implements an algorithm that selectively corrects the signal amplitude
 * depending on the frequency characteristics.
 */

VTK_ABI_NAMESPACE_BEGIN
class VTKFILTERSGENERAL_EXPORT vtkEqualizerFilter : public vtkTableAlgorithm
{
public:
  static vtkEqualizerFilter* New();
  vtkTypeMacro(vtkEqualizerFilter, vtkTableAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Set / Get the sampling frequency of the original signal in Hz
   * Default value is: 1000
   */
  vtkSetMacro(SamplingFrequency, int);
  vtkGetMacro(SamplingFrequency, int);
  ///@}

  ///@{
  /**
   * Set / Get a flag to process all columns of the table.
   * If set to true, all columns of the table will be used. The "SetArray()" method will have no
   * effect.
   * Default value is: false
   */
  vtkSetMacro(AllColumns, bool);
  vtkGetMacro(AllColumns, bool);
  ///@}

  ///@{
  /**
   * Set / Get the name of the column from which the data array is taken
   */
  vtkSetStdStringFromCharMacro(Array);
  vtkGetCharFromStdStringMacro(Array);
  ///@}

  /**
   * Set / Get anchor points in the following format
   * "P1x,P1y;P2x,P2y; ... PNx,PNy;"
   * Default value is an empty string
   */
  void SetPoints(const std::string& points);
  std::string GetPoints() const;

  ///@{
  /**
   * Set / Get the spectrum gain in dB
   * Default value is: 0
   */
  vtkSetMacro(SpectrumGain, int);
  vtkGetMacro(SpectrumGain, int);
  ///@}

protected:
  vtkEqualizerFilter();
  ~vtkEqualizerFilter() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  void ProcessColumn(
    vtkDataArray* array, vtkTable* spectrumTable, vtkTable* resultTable, vtkTable* normalizedTable);

  vtkEqualizerFilter(const vtkEqualizerFilter&) = delete;
  void operator=(const vtkEqualizerFilter&) = delete;

  int SamplingFrequency = 1000; // Hz
  bool AllColumns = false;
  std::string Array;
  int SpectrumGain = 0; // dB

  class vtkInternal;
  vtkInternal* Internal;
};

VTK_ABI_NAMESPACE_END
#endif
