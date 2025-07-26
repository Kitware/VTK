// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkLegacyStatisticalModelWriter
 * @brief   Write a statistical model to a file
 *
 * vtkLegacyStatisticalModelWriter is a source object that writes ASCII or binary
 * statistical model files in VTK format. See text for format details.
 * @warning
 * Binary files written on one system may not be readable on other systems.
 */

#ifndef vtkLegacyStatisticalModelWriter_h
#define vtkLegacyStatisticalModelWriter_h

#include "vtkDataWriter.h"
#include "vtkIOLegacyModule.h" // For export macro
#include "vtkNew.h"            // For ivar

VTK_ABI_NAMESPACE_BEGIN
class vtkStatisticalModel;

class VTKIOLEGACY_EXPORT vtkLegacyStatisticalModelWriter : public vtkDataWriter
{
public:
  static vtkLegacyStatisticalModelWriter* New();
  vtkTypeMacro(vtkLegacyStatisticalModelWriter, vtkDataWriter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Get the input to this writer.
   */
  vtkStatisticalModel* GetInput();
  vtkStatisticalModel* GetInput(int port);
  ///@}

protected:
  vtkLegacyStatisticalModelWriter() = default;
  ~vtkLegacyStatisticalModelWriter() override = default;

  void WriteData() override;

  int FillInputPortInformation(int port, vtkInformation* info) override;

private:
  vtkLegacyStatisticalModelWriter(const vtkLegacyStatisticalModelWriter&) = delete;
  void operator=(const vtkLegacyStatisticalModelWriter&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
