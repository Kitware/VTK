// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkDummy_h
#define vtkDummy_h

#include "vtkDummyModule.h" // For export macro
#include "vtkObject.h"

class VTKDUMMY_EXPORT vtkDummy : public vtkObject
{
public:
  static vtkDummy* New();
  vtkTypeMacro(vtkDummy, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  vtkDummy() = default;
  ~vtkDummy() override = default;

private:
  vtkDummy(const vtkDummy&) = delete;
  void operator=(const vtkDummy&) = delete;
};

#endif
