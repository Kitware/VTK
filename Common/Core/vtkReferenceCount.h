// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkReferenceCount
 * @brief   Obsolete / empty subclass of object.
 *
 * vtkReferenceCount functionality has now been moved into vtkObject
 * @sa
 * vtkObject
 */

#ifndef vtkReferenceCount_h
#define vtkReferenceCount_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkObject.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKCOMMONCORE_EXPORT vtkReferenceCount : public vtkObject
{
public:
  static vtkReferenceCount* New();

  vtkTypeMacro(vtkReferenceCount, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkReferenceCount();
  ~vtkReferenceCount() override;

private:
  vtkReferenceCount(const vtkReferenceCount&) = delete;
  void operator=(const vtkReferenceCount&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
