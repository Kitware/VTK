// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
/**
 * @class   vtkRemoveIsolatedVertices
 * @brief   remove vertices of a vtkGraph with
 *    degree zero.
 *
 *
 */

#ifndef vtkRemoveIsolatedVertices_h
#define vtkRemoveIsolatedVertices_h

#include "vtkGraphAlgorithm.h"
#include "vtkInfovisCoreModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkDataSet;

class VTKINFOVISCORE_EXPORT vtkRemoveIsolatedVertices : public vtkGraphAlgorithm
{
public:
  static vtkRemoveIsolatedVertices* New();
  vtkTypeMacro(vtkRemoveIsolatedVertices, vtkGraphAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkRemoveIsolatedVertices();
  ~vtkRemoveIsolatedVertices() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkRemoveIsolatedVertices(const vtkRemoveIsolatedVertices&) = delete;
  void operator=(const vtkRemoveIsolatedVertices&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
