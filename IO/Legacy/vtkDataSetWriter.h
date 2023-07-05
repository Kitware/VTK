// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkDataSetWriter
 * @brief   write any type of vtk dataset to file
 *
 * vtkDataSetWriter is an abstract class for mapper objects that write their
 * data to disk (or into a communications port). The input to this object is
 * a dataset of any type.
 */

#ifndef vtkDataSetWriter_h
#define vtkDataSetWriter_h

#include "vtkDataWriter.h"
#include "vtkIOLegacyModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class VTKIOLEGACY_EXPORT vtkDataSetWriter : public vtkDataWriter
{
public:
  static vtkDataSetWriter* New();
  vtkTypeMacro(vtkDataSetWriter, vtkDataWriter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Get the input to this writer.
   */
  vtkDataSet* GetInput();
  vtkDataSet* GetInput(int port);
  ///@}

protected:
  vtkDataSetWriter() = default;
  ~vtkDataSetWriter() override = default;

  void WriteData() override;

  int FillInputPortInformation(int port, vtkInformation* info) override;

private:
  vtkDataSetWriter(const vtkDataSetWriter&) = delete;
  void operator=(const vtkDataSetWriter&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
