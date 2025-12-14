// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2025 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
/**
 * @class   vtkExtractStatisticalModelTables
 * @brief   Extract all tables from statistical models.
 *
 * This class accepts a statistical model or a partitioned dataset collection
 * of statistical models as input and produces a partitioned dataset collection
 * holding the model tables as output.
 *
 * Once model tables are extracted, they can be examined in ParaView's
 * spreadsheet view but cannot be used to evaluate data any longer.
 */

#ifndef vtkExtractStatisticalModelTables_h
#define vtkExtractStatisticalModelTables_h

#include "vtkFiltersStatisticsModule.h" // For export macro
#include "vtkPartitionedDataSetCollectionAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkDataObjectCollection;
class vtkStatisticalModel;
class vtkStdString;
class vtkStringArray;
class vtkVariant;
class vtkVariantArray;
class vtkDoubleArray;
class vtkExtractStatisticalModelTablesPrivate;

class VTKFILTERSSTATISTICS_EXPORT vtkExtractStatisticalModelTables
  : public vtkPartitionedDataSetCollectionAlgorithm
{
public:
  vtkTypeMacro(vtkExtractStatisticalModelTables, vtkPartitionedDataSetCollectionAlgorithm);
  static vtkExtractStatisticalModelTables* New();
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkExtractStatisticalModelTables();
  ~vtkExtractStatisticalModelTables() override;

  int FillInputPortInformation(int port, vtkInformation* info) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkExtractStatisticalModelTables(const vtkExtractStatisticalModelTables&) = delete;
  void operator=(const vtkExtractStatisticalModelTables&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
