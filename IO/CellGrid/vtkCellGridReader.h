// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class   vtkCellGridReader
 * @brief   Read a cell-grid file.
 *
 * vtkCellGridReader is a concrete subclass of vtkCellGridAlgorithm that reads data
 * into a vtkCellGrid instance (which is really just a collection of array groups).
 *
 * @sa
 * vtkCellGridAlgorithm
 * vtkCellGridImporter
 */

#ifndef vtkCellGridReader_h
#define vtkCellGridReader_h

#include "vtkCellGridAlgorithm.h"
#include "vtkIOCellGridModule.h" // For export macro
#include "vtkNew.h"              // For New
#include "vtkSmartPointer.h"     // For SmartPointer

// clang-format off
#include <vtk_nlohmannjson.h>        // For API.
#include VTK_NLOHMANN_JSON(json.hpp) // For API.
// clang-format on

#include <string> // For std::string
#include <vector> // For std::vector

VTK_ABI_NAMESPACE_BEGIN
class vtkCellGridIOQuery;

class VTKIOCELLGRID_EXPORT vtkCellGridReader : public vtkCellGridAlgorithm
{
public:
  static vtkCellGridReader* New();
  vtkTypeMacro(vtkCellGridReader, vtkCellGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Set/Get the name of the file from which to read data.
   */
  vtkSetFilePathMacro(FileName);
  vtkGetFilePathMacro(FileName);
  ///@}

  /// Read directly from \a jj into \a grid.
  bool FromJSON(const nlohmann::json& jj, vtkCellGrid* grid);

protected:
  vtkCellGridReader();
  ~vtkCellGridReader() override;

  char* FileName = nullptr;
  vtkNew<vtkCellGridIOQuery> Query;

  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkCellGridReader(const vtkCellGridReader&) = delete;
  void operator=(const vtkCellGridReader&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
