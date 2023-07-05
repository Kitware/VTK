// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkStructuredPointsWriter
 * @brief   write vtk structured points data file
 *
 * vtkStructuredPointsWriter is a source object that writes ASCII or binary
 * structured points data in vtk file format. See text for format details.
 * @warning
 * Binary files written on one system may not be readable on other systems.
 */

#ifndef vtkStructuredPointsWriter_h
#define vtkStructuredPointsWriter_h

#include "vtkDataWriter.h"
#include "vtkIOLegacyModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkImageData;

class VTKIOLEGACY_EXPORT vtkStructuredPointsWriter : public vtkDataWriter
{
public:
  static vtkStructuredPointsWriter* New();
  vtkTypeMacro(vtkStructuredPointsWriter, vtkDataWriter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Get the input to this writer.
   */
  vtkImageData* GetInput();
  vtkImageData* GetInput(int port);
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
  vtkStructuredPointsWriter()
    : WriteExtent(false)
  {
  }
  ~vtkStructuredPointsWriter() override = default;

  void WriteData() override;

  int FillInputPortInformation(int port, vtkInformation* info) override;

  bool WriteExtent;

private:
  vtkStructuredPointsWriter(const vtkStructuredPointsWriter&) = delete;
  void operator=(const vtkStructuredPointsWriter&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
