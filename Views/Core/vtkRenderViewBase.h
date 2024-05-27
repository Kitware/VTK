// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class   vtkRenderViewBase
 * @brief   A base view containing a renderer.
 *
 *
 * vtkRenderViewBase is a view which contains a vtkRenderer.  You may add
 * vtkActors directly to the renderer.
 *
 * This class is also the parent class for any more specialized view which uses
 * a renderer.
 *
 */

#ifndef vtkRenderViewBase_h
#define vtkRenderViewBase_h

#include "vtkSmartPointer.h" // For SP ivars
#include "vtkView.h"
#include "vtkViewsCoreModule.h" // For export macro
#include "vtkWrappingHints.h"   // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkInteractorObserver;
class vtkRenderer;
class vtkRenderWindow;
class vtkRenderWindowInteractor;

class VTKVIEWSCORE_EXPORT VTK_MARSHALAUTO vtkRenderViewBase : public vtkView
{
public:
  static vtkRenderViewBase* New();
  vtkTypeMacro(vtkRenderViewBase, vtkView);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Gets the renderer for this view.
   */
  virtual vtkRenderer* GetRenderer();

  // Sets the renderer for this view.
  virtual void SetRenderer(vtkRenderer* ren);

  /**
   * Get a handle to the render window.
   */
  virtual vtkRenderWindow* GetRenderWindow();

  /**
   * Set the render window for this view. Note that this requires special
   * handling in order to do correctly - see the notes in the detailed
   * description of vtkRenderViewBase.
   */
  virtual void SetRenderWindow(vtkRenderWindow* win);

  ///@{
  /**
   * The render window interactor. Note that this requires special
   * handling in order to do correctly - see the notes in the detailed
   * description of vtkRenderViewBase.
   */
  virtual vtkRenderWindowInteractor* GetInteractor();
  virtual void SetInteractor(vtkRenderWindowInteractor*);
  ///@}

  /**
   * Updates the representations, then calls Render() on the render window
   * associated with this view.
   */
  virtual void Render();

  /**
   * Updates the representations, then calls ResetCamera() on the renderer
   * associated with this view.
   */
  virtual void ResetCamera();

  /**
   * Updates the representations, then calls ResetCameraClippingRange() on the
   * renderer associated with this view.
   */
  virtual void ResetCameraClippingRange();

protected:
  vtkRenderViewBase();
  ~vtkRenderViewBase() override;

  /**
   * Called by the view when the renderer is about to render.
   */
  virtual void PrepareForRendering();

  vtkSmartPointer<vtkRenderer> Renderer;
  vtkSmartPointer<vtkRenderWindow> RenderWindow;

private:
  vtkRenderViewBase(const vtkRenderViewBase&) = delete;
  void operator=(const vtkRenderViewBase&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
