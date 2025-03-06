// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkInteractorStyleSwitchBase
 * @brief   dummy interface class.
 *
 * The class vtkInteractorStyleSwitchBase is here to allow the
 * vtkRenderWindowInteractor to instantiate a default interactor style and
 * preserve backward compatible behavior when the object factory is overridden
 * and vtkInteractorStyleSwitch is returned.
 *
 * @sa
 * vtkInteractorStyleSwitchBase vtkRenderWindowInteractor
 */

#ifndef vtkInteractorStyleSwitchBase_h
#define vtkInteractorStyleSwitchBase_h

#include "vtkInteractorStyle.h"
#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkWrappingHints.h"       // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class VTKRENDERINGCORE_EXPORT VTK_MARSHALAUTO vtkInteractorStyleSwitchBase
  : public vtkInteractorStyle
{
public:
  static vtkInteractorStyleSwitchBase* New();
  vtkTypeMacro(vtkInteractorStyleSwitchBase, vtkInteractorStyle);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  VTK_MARSHALEXCLUDE(VTK_MARSHAL_EXCLUDE_REASON_IS_REDUNDANT)
  vtkRenderWindowInteractor* GetInteractor() override;

protected:
  vtkInteractorStyleSwitchBase();
  ~vtkInteractorStyleSwitchBase() override;

private:
  vtkInteractorStyleSwitchBase(const vtkInteractorStyleSwitchBase&) = delete;
  void operator=(const vtkInteractorStyleSwitchBase&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
