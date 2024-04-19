// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkScaledTextActor
 * @brief   create text that will scale as needed
 *
 * vtkScaledTextActor is deprecated. New code should use vtkTextActor with
 * the Scaled = true option.
 *
 * @sa
 * vtkTextActor vtkActor2D vtkTextMapper
 */

#ifndef vtkScaledTextActor_h
#define vtkScaledTextActor_h

#include "vtkRenderingFreeTypeModule.h" // For export macro
#include "vtkTextActor.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKRENDERINGFREETYPE_EXPORT vtkScaledTextActor : public vtkTextActor
{
public:
  vtkTypeMacro(vtkScaledTextActor, vtkTextActor);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Instantiate object with a rectangle in normaled view coordinates
   * of (0.2,0.85, 0.8, 0.95).
   */
  static vtkScaledTextActor* New();

protected:
  vtkScaledTextActor();

private:
  vtkScaledTextActor(const vtkScaledTextActor&) = delete;
  void operator=(const vtkScaledTextActor&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
