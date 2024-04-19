// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPolyDataWriter
 * @brief   write vtk polygonal data
 *
 * vtkPolyDataWriter is a source object that writes ASCII or binary
 * polygonal data files in vtk format. See text for format details.
 * @warning
 * Binary files written on one system may not be readable on other systems.
 */

#ifndef vtkPolyDataWriter_h
#define vtkPolyDataWriter_h

#include "vtkDataWriter.h"
#include "vtkIOLegacyModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkPolyData;

class VTKIOLEGACY_EXPORT vtkPolyDataWriter : public vtkDataWriter
{
public:
  static vtkPolyDataWriter* New();
  vtkTypeMacro(vtkPolyDataWriter, vtkDataWriter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Get the input to this writer.
   */
  vtkPolyData* GetInput();
  vtkPolyData* GetInput(int port);
  ///@}

protected:
  vtkPolyDataWriter() = default;
  ~vtkPolyDataWriter() override = default;

  void WriteData() override;

  int FillInputPortInformation(int port, vtkInformation* info) override;

private:
  vtkPolyDataWriter(const vtkPolyDataWriter&) = delete;
  void operator=(const vtkPolyDataWriter&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
