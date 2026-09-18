// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkDGTranscribeUnstructuredCells
 * @brief   Transcribe unstructured-grid cells as vtkDGCell subclasses.
 *
 * This class transcribes the 8 linear VTK cell types plus
 * VTK_QUADRATIC_HEXAHEDRON (as an order-2 serendipity hexahedron whose
 * mid-edge nodes are permuted into the Exodus/IOSS HEX20 order used by the
 * DG basis functions). Point-data arrays become continuous (CG) HGRAD
 * cell-attributes of the same order as the shape attribute.
 *
 * When linear and higher-order cells of the same shape appear in one
 * partition, all of them are transcribed at order 1 (corner nodes only)
 * and a warning is emitted, since each vtkDGCell holds a single fixed-width
 * connectivity array.
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
  void AddPointAttributes(
    TranscribeQuery* query, vtkDGCell* dgCell, vtkStringToken basis, int order);

private:
  vtkDGTranscribeUnstructuredCells(const vtkDGTranscribeUnstructuredCells&) = delete;
  void operator=(const vtkDGTranscribeUnstructuredCells&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkDGTranscribeUnstructuredCells_h
