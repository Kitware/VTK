// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class   vtkContextItem
 * @brief   base class for items that are part of a vtkContextScene.
 *
 *
 * Derive from this class to create custom items that can be added to a
 * vtkContextScene.
 */

#ifndef vtkContextItem_h
#define vtkContextItem_h

#include "vtkAbstractContextItem.h"
#include "vtkRenderingContext2DModule.h" // For export macro
#include "vtkWrappingHints.h"            // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkContextTransform;

class VTKRENDERINGCONTEXT2D_EXPORT VTK_MARSHALAUTO vtkContextItem : public vtkAbstractContextItem
{
public:
  vtkTypeMacro(vtkContextItem, vtkAbstractContextItem);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Get the opacity of the item.
   */
  vtkGetMacro(Opacity, double);
  ///@}

  ///@{
  /**
   * Set the opacity of the item.
   * 1.0 by default.
   */
  vtkSetMacro(Opacity, double);
  ///@}

  /**
   * Set the transform of the item.
   */
  virtual void SetTransform(vtkContextTransform*);

protected:
  vtkContextItem() = default;
  ~vtkContextItem() override;

  double Opacity = 1.0;
  vtkContextTransform* Transform = nullptr;

private:
  vtkContextItem(const vtkContextItem&) = delete;
  void operator=(const vtkContextItem&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkContextItem_h
