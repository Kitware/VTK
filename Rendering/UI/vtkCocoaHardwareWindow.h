/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCocoaHardwareWindow.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkCocoaHardwareWindow
 * @brief represents a window in the Cocoa framework
 *
 */

#ifndef vtkCocoaHardwareWindow_h
#define vtkCocoaHardwareWindow_h

// vtk includes
#include "vtkHardwareWindow.h"
#include "vtkRenderingUIModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
// Forward declarations

class VTKRENDERINGUI_EXPORT vtkCocoaHardwareWindow : public vtkHardwareWindow
{
public:
  /**
   * Instantiate the class.
   */
  static vtkCocoaHardwareWindow* New();

  ///@{
  /**
   * Standard methods for the VTK class.
   */
  vtkTypeMacro(vtkCocoaHardwareWindow, vtkHardwareWindow);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  void Create() override;
  void Destroy() override;

  ///@{
  /**
   * Hide or Show the mouse cursor, it is nice to be able to hide the
   * default cursor if you want VTK to display a 3D cursor instead.
   * Set cursor position in window (note that (0,0) is the lower left
   * corner).
   */
  void HideCursor() override;
  void ShowCursor() override;
  void SetCursorPosition(int x, int y) override;
  ///@}

  /**
   * Change the shape of the cursor.
   */
  void SetCurrentCursor(int) override;

  /**
   * Get the ViewCreated flag. It is 1 if this object created an instance
   * of NSView, 0 otherwise.
   */
  virtual vtkTypeBool GetViewCreated();

  /**
   * Get the WindowCreated flag. It is 1 if this object created an instance
   * of NSWindow, 0 otherwise.
   */
  virtual vtkTypeBool GetWindowCreated();

  /**
   * Sets the NSWindow* associated with this vtkWindow.
   * This class' default behaviour, that is, if you never call
   * SetWindowId()/SetRootWindow() is to create an NSWindow and a
   * vtkCocoaGLView (NSView subclass) which are used together to draw
   * all vtk stuff into. If you already have an NSWindow and NSView and
   * you want this class to use them you must call both SetRootWindow()
   * and SetWindowId(), respectively, early on (before WindowInitialize()
   * is executed). In the case of Java, you should call only SetWindowId().
   */
  virtual void SetRootWindow(void*);

  /**
   * Returns the NSWindow* associated with this vtkWindow.
   */
  virtual void* GetRootWindow();

  /**
   * Sets the NSView* associated with this vtkWindow.
   * This class' default behaviour, that is, if you never call
   * SetWindowId()/SetRootWindow() is to create an NSWindow and a
   * vtkCocoaGLView (NSView subclass) which are used together to draw all
   * vtk stuff into. If you already have an NSWindow and NSView and you
   * want this class to use them you must call both SetRootWindow()
   * and SetWindowId(), respectively, early on (before WindowInitialize()
   * is executed). In the case of Java, you should call only SetWindowId().
   */
  void SetWindowId(void*) override;

  ///@{
  /**
   * Returns the NSView* associated with this vtkWindow.
   */
  virtual void* GetWindowId();
  void* GetGenericWindowId() override { return this->GetWindowId(); }
  ///@}

  /**
   * Returns the metal layer associated with this window's view.
   */
  virtual void* GetViewLayer();

  /**
   * Set the NSView* for the vtkWindow to be parented within.  The
   * Position and Size of the RenderWindow will set the rectangle of the
   * NSView that the vtkWindow will create within this parent.
   * If you set the WindowId, then this ParentId will be ignored.
   */
  void SetParentId(void* nsview) override;

  ///@{
  /**
   * Get the parent NSView* for this vtkWindow.  This method will
   * return "NULL" if the parent was not set with SetParentId() or
   * SetParentInfo().
   */
  virtual void* GetParentId();
  void* GetGenericParentId() override { return this->GetParentId(); }
  ///@}

  ///@{
  /**
   * Set the size (width and height) of the rendering window in
   * screen coordinates (in pixels). This resizes the operating
   * system's view/window and redraws it.
   *
   * If the size has changed, this method will fire
   * vtkCommand::WindowResizeEvent.
   */
  void SetSize(int width, int height) override;
  void SetSize(int a[2]) override { this->SetSize(a[0], a[1]); }
  ///@}

  /**
   * Get the size (width and height) of the rendering window in
   * screen coordinates (in pixels).
   */
  int* GetSize() VTK_SIZEHINT(2) override;

  ///@{
  /**
   * Set the position (x and y) of the rendering window in
   * screen coordinates (in pixels). This resizes the operating
   * system's view/window and redraws it.
   */
  void SetPosition(int x, int y) override;
  void SetPosition(int a[2]) override { this->SetPosition(a[0], a[1]); }
  ///@}

  /**
   * Set the name of the window. This appears at the top of the window
   * normally.
   */
  void SetWindowName(const char*) override;

  /**
   * Set this vtkWindow's window id to a pre-existing window.
   * The parameter is an ASCII string of a decimal number representing
   * a pointer to the window.
   */
  void SetWindowInfo(const char*) override;

  /**
   * See the documentation for SetParentId().  This method allows the ParentId
   * to be set as an ASCII string of a decimal number that is the memory
   * address of the parent NSView.
   */
  void SetParentInfo(const char*) override;

protected:
  vtkCocoaHardwareWindow();
  ~vtkCocoaHardwareWindow();

  // Helper members

private:
  // Important: this class cannot contain Objective-C instance
  // variables for 2 reasons:
  // 1) C++ files include this header
  // 2) because of garbage collection (the GC scanner does not scan objects create by C++'s new)
  // Instead, use the CocoaManager dictionary to keep a collection
  // of what would otherwise be Objective-C instance variables.
  void* CocoaManager; // Really an NSMutableDictionary*

  vtkTypeBool CursorHidden;
  vtkTypeBool WindowCreated;
  vtkTypeBool ViewCreated;

  vtkCocoaHardwareWindow(const vtkCocoaHardwareWindow&) = delete;
  void operator=(const vtkCocoaHardwareWindow) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkCocoaHardwareWindow_h
