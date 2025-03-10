// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkLogLookupTable
 * @brief   map scalars into colors using log (base 10) scale
 *
 * This class is an empty shell.  Use vtkLookupTable with SetScaleToLog10()
 * instead.
 *
 * @sa
 * vtkLookupTable
 */

#ifndef vtkLogLookupTable_h
#define vtkLogLookupTable_h

#include "vtkLookupTable.h"
#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkWrappingHints.h"       // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class VTKRENDERINGCORE_EXPORT VTK_MARSHALAUTO vtkLogLookupTable : public vtkLookupTable
{
public:
  static vtkLogLookupTable* New();

  vtkTypeMacro(vtkLogLookupTable, vtkLookupTable);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkLogLookupTable(int sze = 256, int ext = 256);
  ~vtkLogLookupTable() override = default;

private:
  vtkLogLookupTable(const vtkLogLookupTable&) = delete;
  void operator=(const vtkLogLookupTable&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
