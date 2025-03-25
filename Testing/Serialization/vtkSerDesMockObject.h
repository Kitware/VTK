// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class   vtkSerDesMock
 * @brief   A mock interface for testing the SerDes infrastructure
 *
 */

#ifndef vtkSerDesMockObject_h
#define vtkSerDesMockObject_h

#include "vtkObject.h"

#include "vtkTestingSerializationModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN

class VTKTESTINGSERIALIZATION_EXPORT VTK_MARSHALAUTO vtkSerDesMockObject : public vtkObject
{
public:
  /**
   * Standard object factory instantiation method.
   */
  static vtkSerDesMockObject* New();
  void PrintSelf(ostream& os, vtkIndent indent) override;

  vtkTypeMacro(vtkSerDesMockObject, vtkObject);

  vtkSetMacro(Tag, vtkTypeUInt32);
  vtkGetMacro(Tag, vtkTypeUInt32);

protected:
  vtkSerDesMockObject();
  ~vtkSerDesMockObject() override;

private:
  vtkSerDesMockObject(const vtkSerDesMockObject&) = delete;
  void operator=(const vtkSerDesMockObject&) = delete;

  vtkTypeUInt32 Tag = 0;
};
VTK_ABI_NAMESPACE_END
#endif
