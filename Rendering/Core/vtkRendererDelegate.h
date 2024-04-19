// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkRendererDelegate
 * @brief   Render the props of a vtkRenderer
 *
 * vtkRendererDelegate is an abstract class with a pure virtual method Render.
 * This method replaces the Render method of vtkRenderer to allow custom
 * rendering from an external project. A RendererDelegate is connected to
 * a vtkRenderer with method SetDelegate(). An external project just
 * has to provide a concrete implementation of vtkRendererDelegate.
 *
 * @sa
 * vtkRenderer
 */

#ifndef vtkRendererDelegate_h
#define vtkRendererDelegate_h

#include "vtkObject.h"
#include "vtkRenderingCoreModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkRenderer;

class VTKRENDERINGCORE_EXPORT vtkRendererDelegate : public vtkObject
{
public:
  vtkTypeMacro(vtkRendererDelegate, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Render the props of vtkRenderer if Used is on.
   */
  virtual void Render(vtkRenderer* r) = 0;

  ///@{
  /**
   * Tells if the delegate has to be used by the renderer or not.
   * Initial value is off.
   */
  vtkSetMacro(Used, bool);
  vtkGetMacro(Used, bool);
  vtkBooleanMacro(Used, bool);
  ///@}

protected:
  vtkRendererDelegate();
  ~vtkRendererDelegate() override;

  bool Used;

private:
  vtkRendererDelegate(const vtkRendererDelegate&) = delete;
  void operator=(const vtkRendererDelegate&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
