// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkUseDoublePoints_h
#define vtkUseDoublePoints_h

#include "vtkIOCesium3DTilesModule.h" // For export macro
#include "vtkObject.h"

VTK_ABI_NAMESPACE_BEGIN

class VTKIOCESIUM3DTILES_EXPORT vtkUseDoublePoints : public vtkObject
{
public:
  vtkTypeMacro(vtkUseDoublePoints, vtkObject);

  static vtkUseDoublePoints* New();
  void PrintSelf(ostream& os, vtkIndent indent) override;
  void RegisterFactory();
  void UnRegisterFactory();

protected:
  vtkUseDoublePoints();
  ~vtkUseDoublePoints() override;

private:
  class Implementation;
  Implementation* Impl;
  bool Registered;

  vtkUseDoublePoints(const vtkUseDoublePoints&) = delete;
  void operator=(const vtkUseDoublePoints&) = delete;
};

VTK_ABI_NAMESPACE_END

#endif
