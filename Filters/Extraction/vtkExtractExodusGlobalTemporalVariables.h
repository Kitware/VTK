// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkExtractExodusGlobalTemporalVariables
 * @brief extract global temporal arrays or suitable field data arrays
 *
 * vtkExtractExodusGlobalTemporalVariables extracts field data arrays that it
 * determines to represent temporal quantities. This determination is done as
 * follows:
 *
 * * If `AutoDetectGlobalTemporalDataArrays` is true, it checks if to see if the
 *   field data has any array with a key named "GLOBAL_TEMPORAL_VARIABLE". If
 *   found, only arrays with this key are extracted.
 * * If such an array is not found, or if `AutoDetectGlobalTemporalDataArrays` is
 *   false, then all arrays with single tuple are extracted.
 *
 * If an array has GLOBAL_TEMPORAL_VARIABLE key in its information, it means
 * that the array has multiple tuples each associated with the specific
 * timestep. This was pattern first introduced in `vtkExodusIIReader` and hence
 * the name for this class. This class was originally only intended to extract
 * such arrays. It has since been expanded to support other arrays in field
 * data.
 *
 * If the number of tuples in a GLOBAL_TEMPORAL_VARIABLE array is less than the
 * number of timesteps, we assume that we are dealing with restarted files and
 * hence update the pipeline appropriately to request the remaining tuples
 * iteratively.
 *
 * For arrays without GLOBAL_TEMPORAL_VARIABLE, we always iterate over all input
 * timesteps one at a time and accumulate the results.
 *
 * @sa vtkExodusIIReader, vtkExodusIIReader::GLOBAL_TEMPORAL_VARIABLE.
 */
#ifndef vtkExtractExodusGlobalTemporalVariables_h
#define vtkExtractExodusGlobalTemporalVariables_h

#include "vtkFiltersExtractionModule.h" // For export macro
#include "vtkTableAlgorithm.h"

#include <memory> // for std::unique_ptr

VTK_ABI_NAMESPACE_BEGIN
class VTKFILTERSEXTRACTION_EXPORT vtkExtractExodusGlobalTemporalVariables : public vtkTableAlgorithm
{
public:
  static vtkExtractExodusGlobalTemporalVariables* New();
  vtkTypeMacro(vtkExtractExodusGlobalTemporalVariables, vtkTableAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * When set to true (default) this filter will check if any of the arrays in
   * the input field data has a key named `GLOBAL_TEMPORAL_VARIABLE`. If so,
   * this filter will only extract those arrays. If no such array is found, then
   * all single-tuple arrays are extracted. Set this to false to disable this
   * auto-detection and simply extract all single-tuple arrays.
   *
   * @sa `vtkExodusIIReader::GLOBAL_TEMPORAL_VARIABLE`
   */
  vtkSetMacro(AutoDetectGlobalTemporalDataArrays, bool);
  vtkGetMacro(AutoDetectGlobalTemporalDataArrays, bool);
  vtkBooleanMacro(AutoDetectGlobalTemporalDataArrays, bool);
  ///@}

protected:
  vtkExtractExodusGlobalTemporalVariables();
  ~vtkExtractExodusGlobalTemporalVariables() override;

  int FillInputPortInformation(int port, vtkInformation* info) override;
  int RequestInformation(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;
  int RequestUpdateExtent(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;
  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  ///@{
  /**
   * These methods are used by vtkPExtractExodusGlobalTemporalVariables to
   * synchronize internal state between ranks.
   */
  void GetContinuationState(bool& continue_executing_flag, size_t& offset) const;
  void SetContinuationState(bool continue_executing_flag, size_t offset);
  ///@}

private:
  vtkExtractExodusGlobalTemporalVariables(const vtkExtractExodusGlobalTemporalVariables&) = delete;
  void operator=(const vtkExtractExodusGlobalTemporalVariables&) = delete;

  class vtkInternals;
  std::unique_ptr<vtkInternals> Internals;
  bool AutoDetectGlobalTemporalDataArrays;
};

VTK_ABI_NAMESPACE_END
#endif
