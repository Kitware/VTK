// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkUnstructuredGridWriter
 * @brief   write vtk unstructured grid data file
 *
 * vtkUnstructuredGridWriter is a source object that writes ASCII or binary
 * unstructured grid data files in vtk format. See text for format details.
 * @warning
 * Binary files written on one system may not be readable on other systems.
 */

#ifndef vtkUnstructuredGridWriter_h
#define vtkUnstructuredGridWriter_h

#include "vtkDataWriter.h"
#include "vtkIOLegacyModule.h" // For export macro
VTK_ABI_NAMESPACE_BEGIN
class vtkUnstructuredGridBase;

class VTKIOLEGACY_EXPORT vtkUnstructuredGridWriter : public vtkDataWriter
{
public:
  static vtkUnstructuredGridWriter* New();
  vtkTypeMacro(vtkUnstructuredGridWriter, vtkDataWriter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Get the input to this writer.
   */
  vtkUnstructuredGridBase* GetInput();
  vtkUnstructuredGridBase* GetInput(int port);
  ///@}

protected:
  vtkUnstructuredGridWriter() = default;
  ~vtkUnstructuredGridWriter() override = default;

  void WriteData() override;

  int WriteCellsAndFaces(ostream* fp, vtkUnstructuredGridBase* grid, const char* label);

  int FillInputPortInformation(int port, vtkInformation* info) override;

private:
  vtkUnstructuredGridWriter(const vtkUnstructuredGridWriter&) = delete;
  void operator=(const vtkUnstructuredGridWriter&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
