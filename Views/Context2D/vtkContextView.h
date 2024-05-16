// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkContextView
 * @brief   provides a view of the vtkContextScene.
 *
 *
 * This class is derived from vtkRenderViewBase and provides a view of a
 * vtkContextScene, with a default interactor style, renderer etc. It is
 * the simplest way to create a vtkRenderWindow and display a 2D scene inside
 * of it.
 *
 * By default the scene has a white background.
 */

#ifndef vtkContextView_h
#define vtkContextView_h

#include "vtkRenderViewBase.h"
#include "vtkSmartPointer.h"         // Needed for SP ivars
#include "vtkViewsContext2DModule.h" // For export macro
#include "vtkWrappingHints.h"        // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkContext2D;
class vtkContextScene;

class VTKVIEWSCONTEXT2D_EXPORT VTK_MARSHALAUTO vtkContextView : public vtkRenderViewBase
{
public:
  void PrintSelf(ostream& os, vtkIndent indent) override;
  vtkTypeMacro(vtkContextView, vtkRenderViewBase);

  static vtkContextView* New();

  /**
   * Set the vtkContext2D for the view.
   */
  VTK_MARSHALEXCLUDE(VTK_MARSHAL_EXCLUDE_REASON_IS_INTERNAL)
  virtual void SetContext(vtkContext2D* context);

  /**
   * Get the vtkContext2D for the view.
   */
  VTK_MARSHALEXCLUDE(VTK_MARSHAL_EXCLUDE_REASON_IS_INTERNAL)
  virtual vtkContext2D* GetContext();

  /**
   * Set the scene object for the view.
   */
  virtual void SetScene(vtkContextScene* scene);

  /**
   * Get the scene of the view.
   */
  virtual vtkContextScene* GetScene();

protected:
  vtkContextView();
  ~vtkContextView() override;

  vtkSmartPointer<vtkContextScene> Scene;
  vtkSmartPointer<vtkContext2D> Context;

private:
  vtkContextView(const vtkContextView&) = delete;
  void operator=(const vtkContextView&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
