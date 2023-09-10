// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkDGTranscribeUnstructuredCells
 * @brief   Transcribe unstructured-grid cells as vtkDGCell subclasses.
 *
 * This class currently only outputs linear geometry but can properly
 * model higher-order polynomial cell-attributes.
 */
#ifndef vtkDGTranscribeUnstructuredCells_h
#define vtkDGTranscribeUnstructuredCells_h

#include "vtkCellGridResponder.h"
#include "vtkFiltersCellGridModule.h"      // For export macro
#include "vtkNew.h"                        // for ivar
#include "vtkUnstructuredGridToCellGrid.h" // for query template-parameter

VTK_ABI_NAMESPACE_BEGIN

class vtkDGCell;

class VTKFILTERSCELLGRID_EXPORT vtkDGTranscribeUnstructuredCells
  : public vtkCellGridResponder<vtkUnstructuredGridToCellGrid::TranscribeQuery>
{
public:
  using TranscribeQuery = vtkUnstructuredGridToCellGrid::TranscribeQuery;
  static vtkDGTranscribeUnstructuredCells* New();
  vtkTypeMacro(vtkDGTranscribeUnstructuredCells,
    vtkCellGridResponder<vtkUnstructuredGridToCellGrid::TranscribeQuery>);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  bool ClaimMatchingCells(TranscribeQuery* query, vtkDGCell* cellType);
  bool TranscribeMatchingCells(TranscribeQuery* query, vtkDGCell* cellType);
  bool Query(
    TranscribeQuery* query, vtkCellMetadata* cellType, vtkCellGridResponders* caches) override;

protected:
  vtkDGTranscribeUnstructuredCells() = default;
  ~vtkDGTranscribeUnstructuredCells() override = default;

  void AddCellAttributes(TranscribeQuery* query, vtkDGCell* dgCell);
  void AddPointAttributes(TranscribeQuery* query, vtkDGCell* dgCell);

private:
  vtkDGTranscribeUnstructuredCells(const vtkDGTranscribeUnstructuredCells&) = delete;
  void operator=(const vtkDGTranscribeUnstructuredCells&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkDGTranscribeUnstructuredCells_h
