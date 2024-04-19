// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkMultiCorrelativeStatisticsAssessFunctor_h
#define vtkMultiCorrelativeStatisticsAssessFunctor_h

#include "vtkStatisticsAlgorithm.h"

#include <vector>

VTK_ABI_NAMESPACE_BEGIN
class vtkDataArray;
class vtkTable;

#define VTK_MULTICORRELATIVE_KEYCOLUMN1 "Column1"
#define VTK_MULTICORRELATIVE_KEYCOLUMN2 "Column2"
#define VTK_MULTICORRELATIVE_ENTRIESCOL "Entries"
#define VTK_MULTICORRELATIVE_AVERAGECOL "Mean"
#define VTK_MULTICORRELATIVE_COLUMNAMES "Column"

class vtkMultiCorrelativeAssessFunctor : public vtkStatisticsAlgorithm::AssessFunctor
{
public:
  static vtkMultiCorrelativeAssessFunctor* New();

  vtkMultiCorrelativeAssessFunctor() = default;
  ~vtkMultiCorrelativeAssessFunctor() override = default;
  virtual bool Initialize(vtkTable* inData, vtkTable* reqModel, bool cholesky = true);

  void operator()(vtkDoubleArray* result, vtkIdType row) override;

  vtkIdType GetNumberOfColumns() { return static_cast<vtkIdType>(this->Columns.size()); }
  vtkDataArray* GetColumn(vtkIdType colIdx) { return this->Columns[colIdx]; }

  std::vector<vtkDataArray*> Columns; // Source of data
  double* Center;             // Offset per column (usu. to re-center the data about the mean)
  std::vector<double> Factor; // Weights per column
  // double Normalization; // Scale factor for the volume under a multivariate Gaussian used to
  // normalize the CDF
  std::vector<double> Tuple; // Place to store product of detrended input tuple and Cholesky inverse
  std::vector<double> EmptyTuple; // Used to quickly initialize Tuple for each datum
};

VTK_ABI_NAMESPACE_END
#endif // vtkMultiCorrelativeStatisticsAssessFunctor_h
// VTK-HeaderTest-Exclude: vtkMultiCorrelativeStatisticsAssessFunctor.h
