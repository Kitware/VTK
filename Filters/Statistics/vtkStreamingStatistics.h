// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2010 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
/**
 * @class   vtkStreamingStatistics
 * @brief   A class for using the statistics filters
 * in a streaming mode.
 *
 *
 * A class for using the statistics filters in a streaming mode or perhaps
 * an "online, incremental, push" mode.
 *
 * @par Thanks:
 * Thanks to the Universe for unfolding in a way that allowed this class
 * to be implemented, also Godzilla for not crushing my computer.
 */

#ifndef vtkStreamingStatistics_h
#define vtkStreamingStatistics_h

#include "vtkFiltersStatisticsModule.h" // For export macro
#include "vtkTableAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkDataObjectCollection;
class vtkMultiBlockDataSet;
class vtkStatisticsAlgorithm;
class vtkTable;

class VTKFILTERSSTATISTICS_EXPORT vtkStreamingStatistics : public vtkTableAlgorithm
{
public:
  vtkTypeMacro(vtkStreamingStatistics, vtkTableAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkStreamingStatistics* New();

  /**
   * enumeration values to specify input port types
   */
  enum InputPorts
  {
    INPUT_DATA = 0,       //!< Port 0 is for learn data
    LEARN_PARAMETERS = 1, //!< Port 1 is for learn parameters (initial guesses, etc.)
    INPUT_MODEL = 2       //!< Port 2 is for a priori models
  };

  /**
   * enumeration values to specify output port types
   */
  enum OutputIndices
  {
    OUTPUT_DATA = 0,  //!< Output 0 mirrors the input data, plus optional assessment columns
    OUTPUT_MODEL = 1, //!< Output 1 contains any generated model
    OUTPUT_TEST = 2   //!< Output 2 contains result of statistical test(s)
  };

  virtual void SetStatisticsAlgorithm(vtkStatisticsAlgorithm*);

protected:
  vtkStreamingStatistics();
  ~vtkStreamingStatistics() override;

  int FillInputPortInformation(int port, vtkInformation* info) override;
  int FillOutputPortInformation(int port, vtkInformation* info) override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkStreamingStatistics(const vtkStreamingStatistics&) = delete;
  void operator=(const vtkStreamingStatistics&) = delete;

  // Internal statistics algorithm to care for and feed
  vtkStatisticsAlgorithm* StatisticsAlgorithm;

  // Internal model that gets aggregated
  vtkMultiBlockDataSet* InternalModel;
};

VTK_ABI_NAMESPACE_END
#endif
