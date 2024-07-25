// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkLegacyCellGridWriter
 * @brief   Write VTK cell-grid data to a file
 *
 * vtkLegacyCellGridWriter is a source object that writes ASCII or binary
 * cell-grid data files in VTK format. See text for format details.
 * @warning
 * Binary files written on one system may not be readable on other systems.
 */

#ifndef vtkLegacyCellGridWriter_h
#define vtkLegacyCellGridWriter_h

#include "vtkCellGridWriter.h" // For ivar
#include "vtkDataWriter.h"
#include "vtkIOLegacyModule.h" // For export macro
#include "vtkNew.h"            // For ivar

VTK_ABI_NAMESPACE_BEGIN

class VTKIOLEGACY_EXPORT vtkLegacyCellGridWriter : public vtkDataWriter
{
public:
  static vtkLegacyCellGridWriter* New();
  vtkTypeMacro(vtkLegacyCellGridWriter, vtkDataWriter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Get the input to this writer.
   */
  vtkCellGrid* GetInput();
  vtkCellGrid* GetInput(int port);
  ///@}

protected:
  vtkLegacyCellGridWriter() = default;
  ~vtkLegacyCellGridWriter() override = default;

  void WriteData() override;

#if 0
  int WriteArrayGroups(ostream* fp, vtkCellGrid* grid);
  int WriteCellAttributes(ostream* fp, vtkCellGrid* grid);
  int WriteCellMetadata(ostream* fp, vtkCellGrid* grid);
#endif

  int FillInputPortInformation(int port, vtkInformation* info) override;

  vtkNew<vtkCellGridWriter> Subwriter;

private:
  vtkLegacyCellGridWriter(const vtkLegacyCellGridWriter&) = delete;
  void operator=(const vtkLegacyCellGridWriter&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
