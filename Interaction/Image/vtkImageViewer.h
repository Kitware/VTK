// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkImageViewer
 * @brief   Display a 2d image.
 *
 * vtkImageViewer is a convenience class for displaying a 2d image.  It
 * packages up the functionality found in vtkRenderWindow, vtkRenderer,
 * vtkActor2D and vtkImageMapper into a single easy to use class.  Behind the
 * scenes these four classes are actually used to to provide the required
 * functionality. vtkImageViewer is simply a wrapper around them.
 *
 * @sa
 * vtkRenderWindow vtkRenderer vtkImageMapper vtkActor2D
 */

#ifndef vtkImageViewer_h
#define vtkImageViewer_h

#include "vtkInteractionImageModule.h" // For export macro
#include "vtkObject.h"

#include "vtkImageMapper.h"  // For all the inline methods
#include "vtkRenderWindow.h" // For all the inline methods

VTK_ABI_NAMESPACE_BEGIN
class vtkInteractorStyleImage;

class VTKINTERACTIONIMAGE_EXPORT vtkImageViewer : public vtkObject
{
public:
  static vtkImageViewer* New();

  vtkTypeMacro(vtkImageViewer, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Get name of rendering window
   */
  char* GetWindowName() { return this->RenderWindow->GetWindowName(); }

  /**
   * Render the resulting image.
   */
  virtual void Render();

  ///@{
  /**
   * Set/Get the input to the viewer.
   */
  void SetInputData(vtkImageData* in) { this->ImageMapper->SetInputData(in); }
  vtkImageData* GetInput() { return this->ImageMapper->GetInput(); }
  virtual void SetInputConnection(vtkAlgorithmOutput* input)
  {
    this->ImageMapper->SetInputConnection(input);
  }
  ///@}

  ///@{
  /**
   * What is the possible Min/ Max z slices available.
   */
  int GetWholeZMin() { return this->ImageMapper->GetWholeZMin(); }
  int GetWholeZMax() { return this->ImageMapper->GetWholeZMax(); }
  ///@}

  ///@{
  /**
   * Set/Get the current Z Slice to display
   */
  int GetZSlice() { return this->ImageMapper->GetZSlice(); }
  void SetZSlice(int s) { this->ImageMapper->SetZSlice(s); }
  ///@}

  ///@{
  /**
   * Sets window/level for mapping pixels to colors.
   */
  double GetColorWindow() { return this->ImageMapper->GetColorWindow(); }
  double GetColorLevel() { return this->ImageMapper->GetColorLevel(); }
  void SetColorWindow(double s) { this->ImageMapper->SetColorWindow(s); }
  void SetColorLevel(double s) { this->ImageMapper->SetColorLevel(s); }
  ///@}

  ///@{
  /**
   * These are here for using a tk window.
   */
  void SetDisplayId(void* a) { this->RenderWindow->SetDisplayId(a); }
  void SetWindowId(void* a) { this->RenderWindow->SetWindowId(a); }
  void SetParentId(void* a) { this->RenderWindow->SetParentId(a); }
  ///@}

  ///@{
  /**
   * Get the position (x and y) of the rendering window in
   * screen coordinates (in pixels).
   */
  int* GetPosition() VTK_SIZEHINT(2) { return this->RenderWindow->GetPosition(); }

  /**
   * Set the position (x and y) of the rendering window in
   * screen coordinates (in pixels). This resizes the operating
   * system's view/window and redraws it.
   */
  void SetPosition(int x, int y) { this->RenderWindow->SetPosition(x, y); }
  virtual void SetPosition(int a[2]);
  ///@}

  ///@{
  /**
   * Get the size (width and height) of the rendering window in
   * screen coordinates (in pixels).
   */
  int* GetSize() VTK_SIZEHINT(2) { return this->RenderWindow->GetSize(); }

  /**
   * Set the size (width and height) of the rendering window in
   * screen coordinates (in pixels). This resizes the operating
   * system's view/window and redraws it.
   *
   * If the size has changed, this method will fire
   * vtkCommand::WindowResizeEvent.
   */
  void SetSize(int width, int height) { this->RenderWindow->SetSize(width, height); }
  virtual void SetSize(int a[2]);
  ///@}

  ///@{
  /**
   * Get the internal objects
   */
  vtkGetObjectMacro(RenderWindow, vtkRenderWindow);
  void SetRenderWindow(vtkRenderWindow* renWin);
  vtkGetObjectMacro(Renderer, vtkRenderer);
  vtkGetObjectMacro(ImageMapper, vtkImageMapper);
  vtkGetObjectMacro(Actor2D, vtkActor2D);
  ///@}

  /**
   * Create and attach an interactor for this window
   */
  void SetupInteractor(vtkRenderWindowInteractor*);

  ///@{
  /**
   * Create a window in memory instead of on the screen. This may not
   * be supported for every type of window and on some windows you may
   * need to invoke this prior to the first render.
   */
  void SetOffScreenRendering(vtkTypeBool);
  vtkTypeBool GetOffScreenRendering();
  void OffScreenRenderingOn();
  void OffScreenRenderingOff();
  ///@}

protected:
  vtkImageViewer();
  ~vtkImageViewer() override;

  vtkRenderWindow* RenderWindow;
  vtkRenderer* Renderer;
  vtkImageMapper* ImageMapper;
  vtkActor2D* Actor2D;
  int FirstRender;
  vtkRenderWindowInteractor* Interactor;
  vtkInteractorStyleImage* InteractorStyle;

  friend class vtkImageViewerCallback;
  vtkAlgorithm* GetInputAlgorithm();

private:
  vtkImageViewer(const vtkImageViewer&) = delete;
  void operator=(const vtkImageViewer&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
