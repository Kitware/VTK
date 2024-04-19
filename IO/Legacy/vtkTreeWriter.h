// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkTreeWriter
 * @brief   write vtkTree data to a file
 *
 * vtkTreeWriter is a sink object that writes ASCII or binary
 * vtkTree data files in vtk format. See text for format details.
 * @warning
 * Binary files written on one system may not be readable on other systems.
 */

#ifndef vtkTreeWriter_h
#define vtkTreeWriter_h

#include "vtkDataWriter.h"
#include "vtkIOLegacyModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkTree;

class VTKIOLEGACY_EXPORT vtkTreeWriter : public vtkDataWriter
{
public:
  static vtkTreeWriter* New();
  vtkTypeMacro(vtkTreeWriter, vtkDataWriter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Get the input to this writer.
   */
  vtkTree* GetInput();
  vtkTree* GetInput(int port);
  ///@}

protected:
  vtkTreeWriter() = default;
  ~vtkTreeWriter() override = default;

  void WriteData() override;

  int FillInputPortInformation(int port, vtkInformation* info) override;

private:
  vtkTreeWriter(const vtkTreeWriter&) = delete;
  void operator=(const vtkTreeWriter&) = delete;

  void WriteEdges(ostream& Stream, vtkTree* Tree);
};

VTK_ABI_NAMESPACE_END
#endif
