// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkTableWriter
 * @brief   write vtkTable to a file
 *
 * vtkTableWriter is a sink object that writes ASCII or binary
 * vtkTable data files in vtk format. See text for format details.
 * @warning
 * Binary files written on one system may not be readable on other systems.
 */

#ifndef vtkTableWriter_h
#define vtkTableWriter_h

#include "vtkDataWriter.h"
#include "vtkIOLegacyModule.h" // For export macro
VTK_ABI_NAMESPACE_BEGIN
class vtkTable;

class VTKIOLEGACY_EXPORT vtkTableWriter : public vtkDataWriter
{
public:
  static vtkTableWriter* New();
  vtkTypeMacro(vtkTableWriter, vtkDataWriter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Get the input to this writer.
   */
  vtkTable* GetInput();
  vtkTable* GetInput(int port);
  ///@}

protected:
  vtkTableWriter() = default;
  ~vtkTableWriter() override = default;

  void WriteData() override;

  int FillInputPortInformation(int port, vtkInformation* info) override;

private:
  vtkTableWriter(const vtkTableWriter&) = delete;
  void operator=(const vtkTableWriter&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
