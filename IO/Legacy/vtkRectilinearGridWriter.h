// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkRectilinearGridWriter
 * @brief   write vtk rectilinear grid data file
 *
 * vtkRectilinearGridWriter is a source object that writes ASCII or binary
 * rectilinear grid data files in vtk format. See text for format details.
 *
 * @warning
 * Binary files written on one system may not be readable on other systems.
 */

#ifndef vtkRectilinearGridWriter_h
#define vtkRectilinearGridWriter_h

#include "vtkDataWriter.h"
#include "vtkIOLegacyModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkRectilinearGrid;

class VTKIOLEGACY_EXPORT vtkRectilinearGridWriter : public vtkDataWriter
{
public:
  static vtkRectilinearGridWriter* New();
  vtkTypeMacro(vtkRectilinearGridWriter, vtkDataWriter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Get the input to this writer.
   */
  vtkRectilinearGrid* GetInput();
  vtkRectilinearGrid* GetInput(int port);
  ///@}

  ///@{
  /**
   * When WriteExtent is on, vtkStructuredPointsWriter writes
   * data extent in the output file. Otherwise, it writes dimensions.
   * The only time this option is useful is when the extents do
   * not start at (0, 0, 0). This is an options to support writing
   * of older formats while still using a newer VTK.
   */
  vtkSetMacro(WriteExtent, bool);
  vtkGetMacro(WriteExtent, bool);
  vtkBooleanMacro(WriteExtent, bool);
  ///@}

protected:
  vtkRectilinearGridWriter()
    : WriteExtent(false)
  {
  }
  ~vtkRectilinearGridWriter() override = default;

  void WriteData() override;

  int FillInputPortInformation(int port, vtkInformation* info) override;

  bool WriteExtent;

private:
  vtkRectilinearGridWriter(const vtkRectilinearGridWriter&) = delete;
  void operator=(const vtkRectilinearGridWriter&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
