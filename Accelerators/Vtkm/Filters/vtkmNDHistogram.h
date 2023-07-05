// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Kitware, Inc.
// SPDX-FileCopyrightText: Copyright 2012 Sandia Corporation.
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
/**
 * @class vtkmNDHistogram
 * @brief generate a n dimensional histogram field from input fields
 *
 * vtkmNDhistogram is a filter that generate a n dimensional histogram field from
 * some input fields.
 * This filter takes a data set and with target fields and bins defined,
 * it would generate a N-Dims histogram from input fields. The input fields should
 * have the same number of values.
 * The result is stored in a field named as "Frequency". This field contains all
 * the frequencies of the N-Dims histogram in sparse representation.
 * That being said, the result field does not store 0 frequency bins. Meanwhile
 * all input fields now would have the same length and store bin ids instead.
 * E.g. (FieldA[i], FieldB[i], FieldC[i], Frequency[i]) is a bin in the histogram.
 * The first three numbers are binIDs for FieldA, FieldB and FieldC. Frequency[i] stores
 * the frequency for this bin (FieldA[i], FieldB[i], FieldC[i]).
 */

#ifndef vtkmNDHistogram_h
#define vtkmNDHistogram_h

#include <string>  // for std::string
#include <utility> // for std::pair
#include <vector>  // for std::vector

#include "vtkAcceleratorsVTKmFiltersModule.h" // required for correct export
#include "vtkArrayDataAlgorithm.h"
#include "vtkmlib/vtkmInitializer.h" // Need for initializing vtk-m

VTK_ABI_NAMESPACE_BEGIN
class VTKACCELERATORSVTKMFILTERS_EXPORT vtkmNDHistogram : public vtkArrayDataAlgorithm
{
public:
  vtkTypeMacro(vtkmNDHistogram, vtkArrayDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  void AddFieldAndBin(const std::string& fieldName, const vtkIdType& numberOfBins);

  double GetBinDelta(size_t fieldIndex);
  std::pair<double, double> GetDataRange(size_t fieldIndex);

  /**
   * @brief GetFieldIndexFromFieldName
   * @param fieldName
   * @return the index of the fieldName. If it's not in the FieldNames list, a -1
   * would be returned.
   */
  int GetFieldIndexFromFieldName(const std::string& fieldName);

  static vtkmNDHistogram* New();

protected:
  vtkmNDHistogram();
  ~vtkmNDHistogram() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int FillInputPortInformation(int port, vtkInformation* info) override;

private:
  vtkmNDHistogram(const vtkmNDHistogram&) = delete;
  void operator=(const vtkmNDHistogram&) = delete;
  std::vector<std::string> FieldNames;
  std::vector<vtkIdType> NumberOfBins;
  std::vector<double> BinDeltas;
  std::vector<std::pair<double, double>> DataRanges;
  vtkmInitializer Initializer;
};

VTK_ABI_NAMESPACE_END
#endif // vtkmNDHistogram_h
