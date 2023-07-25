// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkTestNewVar
 * @brief   Tests instantiations of the vtkNew class template.
 */

#ifndef vtkTestNewVar_h
#define vtkTestNewVar_h

#include "vtkNew.h"
#include "vtkObject.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkPoints2D;
VTK_ABI_NAMESPACE_END

class vtkTestNewVar : public vtkObject
{
public:
  static vtkTestNewVar* New();

  vtkTypeMacro(vtkTestNewVar, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Get the reference count for the points object.
   */
  vtkIdType GetPointsRefCount();

  /**
   * This is just for testing - return the points as a vtkObject so that it can
   * be assigned to a vtkSmartPointer without including the vtkPoints2D header
   * and defeating part of the point of the test.
   */
  vtkObject* GetPoints();

  /**
   * This is just for testing - return the points as a vtkObject so that it can
   * be assigned to a vtkSmartPointer without including the vtkPoints2D header
   * and defeating part of the point of the test.
   * Using implicit conversion to raw pointer.
   */
  vtkObject* GetPoints2();

protected:
  vtkTestNewVar();
  ~vtkTestNewVar() override;

  vtkNew<vtkPoints2D> Points;

private:
  vtkTestNewVar(const vtkTestNewVar&) = delete;
  void operator=(const vtkTestNewVar&) = delete;
};

#endif
