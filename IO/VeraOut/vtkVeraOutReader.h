// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// .NAME vtkVeraOutReader - File reader for VERA OUT HDF5 format.

#ifndef vtkVeraOutReader_h
#define vtkVeraOutReader_h

#include "vtkIOVeraOutModule.h" // For VTKIOVERAOUT_EXPORT macro
#include <vector>               // For STL vector

// vtkCommonExecutionModel
#include "vtkRectilinearGridAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkDataArraySelection;

class VTKIOVERAOUT_EXPORT vtkVeraOutReader : public vtkRectilinearGridAlgorithm
{
public:
  static vtkVeraOutReader* New();
  vtkTypeMacro(vtkVeraOutReader, vtkRectilinearGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  vtkSetFilePathMacro(FileName);
  vtkGetFilePathMacro(FileName);

  /**
   * Get vtkDataArraySelection instance to select cell arrays to read.
   */
  vtkDataArraySelection* GetCellDataArraySelection() const;
  /**
   * Get vtkDataArraySelection instance to select field arrays to read.
   */
  vtkDataArraySelection* GetFieldDataArraySelection() const;

  /**
   * Override GetMTime because of array selector.
   */
  vtkMTimeType GetMTime() override;

protected:
  vtkVeraOutReader();
  ~vtkVeraOutReader() override;

  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  // Trigger the real data access
  int RequestData(
    vtkInformation*, vtkInformationVector**, vtkInformationVector* outVector) override;

  char* FileName;
  int NumberOfTimeSteps;
  std::vector<double> TimeSteps;

private:
  vtkVeraOutReader(const vtkVeraOutReader&) = delete;
  void operator=(const vtkVeraOutReader&) = delete;

  class Internals;
  Internals* Internal;
};

VTK_ABI_NAMESPACE_END
#endif
