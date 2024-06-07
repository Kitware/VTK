// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkCellGridCellSource
 * @brief   Create a cell-grid with one or more cells of the requested type.
 */

#ifndef vtkCellGridCellSource_h
#define vtkCellGridCellSource_h

#include "vtkCellGridAlgorithm.h"
#include "vtkFiltersCellGridModule.h" // For export macro.
#include "vtkNew.h"                   // For ivar.

#include <string> // For ivar.

VTK_ABI_NAMESPACE_BEGIN

class VTKFILTERSCELLGRID_EXPORT vtkCellGridCellSource : public vtkCellGridAlgorithm
{
public:
  static vtkCellGridCellSource* New();
  vtkTypeMacro(vtkCellGridCellSource, vtkCellGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /// A cell-grid query used by this filter.
  class Query : public vtkCellGridQuery
  {
  public:
    static Query* New();
    vtkTypeMacro(vtkCellGridCellSource::Query, vtkCellGridQuery);
    void PrintSelf(ostream& os, vtkIndent indent) override;

    /// Set/get the type of cell to create.
    ///
    /// Call vtkCellMetadata::CellTypes() to fetch a list
    /// of values acceptable as inputs to SetCellType().
    vtkGetCharFromStdStringMacro(CellType);
    vtkSetStdStringFromCharMacro(CellType);
    std::string GetCellTypeString() { return this->CellType; }

  protected:
    std::string CellType;
  };

  /// Set/get the type of cell to create.
  ///
  /// These methods simply forward the call to the filter's query.
  virtual void SetCellType(const char* cellType);
  const char* GetCellType() const;

protected:
  vtkCellGridCellSource();
  ~vtkCellGridCellSource() override = default;

  int RequestData(
    vtkInformation* request, vtkInformationVector** inInfo, vtkInformationVector* ouInfo) override;

  vtkNew<Query> Request;

private:
  vtkCellGridCellSource(const vtkCellGridCellSource&) = delete;
  void operator=(const vtkCellGridCellSource&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkCellGridCellSource_h
