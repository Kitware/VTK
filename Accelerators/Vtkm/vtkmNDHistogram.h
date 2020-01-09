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

#include "vtkAcceleratorsVTKmModule.h" // required for correct export
#include "vtkArrayDataAlgorithm.h"
#include <utility>
#include <vector>

class VTKACCELERATORSVTKM_EXPORT vtkmNDHistogram : public vtkArrayDataAlgorithm
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
  std::vector<std::pair<double, double> > DataRanges;
};

#endif // vtkmNDHistogram_h

// VTK-HeaderTest-Exclude: vtkmNDHistogram.h
