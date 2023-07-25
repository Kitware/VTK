// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSimplePointsWriter
 * @brief   write a file of xyz coordinates
 *
 * vtkSimplePointsWriter writes a simple file of xyz coordinates
 *
 * @sa
 * vtkSimplePointsReader
 */

#ifndef vtkSimplePointsWriter_h
#define vtkSimplePointsWriter_h

#include "vtkDataSetWriter.h"
#include "vtkIOLegacyModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class VTKIOLEGACY_EXPORT vtkSimplePointsWriter : public vtkDataSetWriter
{
public:
  static vtkSimplePointsWriter* New();
  vtkTypeMacro(vtkSimplePointsWriter, vtkDataSetWriter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  vtkGetMacro(DecimalPrecision, int);
  vtkSetMacro(DecimalPrecision, int);

protected:
  vtkSimplePointsWriter();
  ~vtkSimplePointsWriter() override = default;

  void WriteData() override;

  int DecimalPrecision;

private:
  vtkSimplePointsWriter(const vtkSimplePointsWriter&) = delete;
  void operator=(const vtkSimplePointsWriter&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
