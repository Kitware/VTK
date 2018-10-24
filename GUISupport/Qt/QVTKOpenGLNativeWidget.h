/*=========================================================================

  Program:   Visualization Toolkit
  Module:    QVTKOpenGLNativeWidget.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class QVTKOpenGLNativeWidget
 * @brief QOpenGLWidget subclass to house a vtkGenericOpenGLRenderWindow in a Qt
 * application.
 *
 * QVTKOpenGLNativeWidget extends QOpenGLWidget to make it work with a
 * vtkGenericOpenGLRenderWindow. This is akin to QVTKWidget except it uses Qt to create and
 * manage the OpenGL context using QOpenGLWidget (added in Qt 5.4).
 *
 * While QVTKOpenGLNativeWidget is intended to be a replacement for QVTKWidget when
 * using Qt 5, there are a few difference between QVTKOpenGLNativeWidget and
 * QVTKWidget.
 *
 * Unlike QVTKWidget, QVTKOpenGLNativeWidget only works with vtkGenericOpenGLRenderWindow.
 * This is necessary since QOpenGLWidget wants to take over the window management as
 * well as the OpenGL context creation. Getting that to work reliably with
 * vtkXRenderWindow or vtkWin32RenderWindow (and other platform specific
 * vtkRenderWindow subclasses) was tricky and fraught with issues.
 *
 * Since QVTKOpenGLNativeWidget uses QOpenGLWidget to create the OpenGL context,
 * it uses QSurfaceFormat (set using `QOpenGLWidget::setFormat` or
 * `QSurfaceFormat::setDefaultFormat`) to create appropriate window and context.
 * You can use `QVTKOpenGLNativeWidget::copyToFormat` to obtain a QSurfaceFormat
 * appropriate for a vtkRenderWindow.
 *
 * A typical usage for QVTKOpenGLNativeWidget is as follows:
 * @code{.cpp}
 *
 *  // before initializing QApplication, set the default surface format.
 *  QSurfaceFormat::setDefaultFormat(QVTKOpenGLNativeWidget::defaultFormat());
 *
 *  vtkNew<vtkGenericOpenGLRenderWindow> window;
 *  QPointer<QVTKOpenGLNativeWidget> widget = new QVTKOpenGLNativeWidget(...);
 *  widget->SetRenderWindow(window.Get());
 *
 *  // If using any of the standard view e.g. vtkContextView, then
 *  // you can do the following.
 *  vtkNew<vtkContextView> view;
 *  view->SetRenderWindow(window.Get());
 *
 *  // You can continue to use `window` as a regular vtkRenderWindow
 *  // including adding renderers, actors etc.
 *
 * @endcode
 *
 * @section OpenGLContext OpenGL Context
 *
 * In QOpenGLWidget (superclass for QVTKOpenGLNativeWidget), all rendering happens in a
 * framebuffer object. Thus, care must be taken in the rendering code to never
 * directly re-bind the default framebuffer i.e. ID 0.
 *
 * QVTKOpenGLNativeWidget creates an internal QOpenGLFramebufferObject, independent of the
 * one created by superclass, for vtkRenderWindow to do the rendering in. This
 * explicit double-buffering is useful in avoiding temporary back-buffer only
 * renders done in VTK (e.g. when making selections) from destroying the results
 * composed on screen.
 *
 * @section RenderAndPaint Handling Render and Paint.
 *
 * QWidget subclasses (including `QOpenGLWidget` and `QVTKOpenGLNativeWidget`) display
 * their contents on the screen in `QWidget::paint` in response to a paint event.
 * `QOpenGLWidget` subclasses are expected to do OpenGL rendering in
 * `QOpenGLWidget::paintGL`. QWidget can receive paint events for various
 * reasons including widget getting focus/losing focus, some other widget on
 * the UI e.g. QProgressBar in status bar updating, etc.
 *
 * In VTK applications, any time the vtkRenderWindow needs to be updated to
 * render a new result, one call `vtkRenderWindow::Render` on it.
 * vtkRenderWindowInteractor set on the render window ensures that as
 * interactions happen that affect the rendered result, it calls `Render` on the
 * render window.
 *
 * Since paint in Qt can be called more often then needed, we avoid potentially
 * expensive `vtkRenderWindow::Render` calls each time that happens. Instead,
 * QVTKOpenGLNativeWidget relies on the VTK application calling
 * `vtkRenderWindow::Render` on the render window when it needs to update the
 * rendering. `paintGL` simply passes on the result rendered by the most render
 * vtkRenderWindow::Render to Qt windowing system for composing on-screen.
 *
 * There may still be occasions when we may have to render in `paint` for
 * example if the window was resized or Qt had to recreate the OpenGL context.
 * In those cases, `QVTKOpenGLNativeWidget::paintGL` can request a render by calling
 * `QVTKOpenGLNativeWidget::renderVTK`.
 *
 * @section Caveats
 * QVTKOpenGLNativeWidget only supports **OpenGL2** rendering backend.
 * QVTKOpenGLNativeWidget does not support stereo,
 * please use QVTKOpenGLWidget if you need support for stereo rendering
 *
 * QVTKOpenGLNativeWidget is targeted for Qt version 5.5 and above.
 *
 */
#ifndef QVTKOpenGLNativeWidget_h
#define QVTKOpenGLNativeWidget_h

#include <QOpenGLWidget>

#include "QVTKInteractor.h"        // needed for QVTKInteractor
#include "vtkGUISupportQtModule.h" // for export macro
#include "vtkNew.h"                // needed for vtkNew
#include "vtkSmartPointer.h"       // needed for vtkSmartPointer

class QOpenGLDebugLogger;
class QOpenGLFramebufferObject;
class QVTKInteractor;
class QVTKInteractorAdapter;
class QVTKOpenGLNativeWidgetObserver;
class vtkGenericOpenGLRenderWindow;

class VTKGUISUPPORTQT_EXPORT QVTKOpenGLNativeWidget : public QOpenGLWidget
{
  Q_OBJECT
  typedef QOpenGLWidget Superclass;
public:
  QVTKOpenGLNativeWidget(QWidget* parent = Q_NULLPTR, Qt::WindowFlags f = Qt::WindowFlags());
  ~QVTKOpenGLNativeWidget() override;

  //@{
  /**
   * Get/Set the currently used vtkGenericOpenGLRenderWindow.
   * GetRenderWindow() creates and returns a new vtkGenericOpenGLRenderWindow
   * if it is not already provided.
   */
  void SetRenderWindow(vtkGenericOpenGLRenderWindow* win);
  void SetRenderWindow(vtkRenderWindow* win);
  virtual vtkRenderWindow* GetRenderWindow();
  //@}

  /**
   * Get the QEvent to VTK events translator.
   */
  virtual QVTKInteractorAdapter* GetInteractorAdapter() { return this->InteractorAdapter; }

  /**
   * Get the QVTKInteractor that was either created by default or set by the user.
   */
  virtual QVTKInteractor* GetInteractor();

  /**
   * Sets up vtkRenderWindow ivars using QSurfaceFormat.
   */
  static void copyFromFormat(const QSurfaceFormat& format, vtkRenderWindow* win);

  /**
   * Using the vtkRenderWindow, setup QSurfaceFormat.
   */
  static void copyToFormat(vtkRenderWindow* win, QSurfaceFormat& format);

  /**
   * Returns a typical QSurfaceFormat suitable for most applications using
   * QVTKOpenGLNativeWidget. Note that this is not the QSurfaceFormat that gets used
   * if none is specified. That is set using `QSurfaceFormat::setDefaultFormat`.
   */
  static QSurfaceFormat defaultFormat();

  /**
   * Enable or disable support for HiDPI displays.
   */
  virtual void setEnableHiDPI(bool enable);
  virtual bool enableHiDPI() { return this->EnableHiDPI; }

  /**
   * Set the cursor on this widget.
   */
  void setQVTKCursor(const QCursor &cursor);

signals:
  /**
   * This signal will be emitted whenever a mouse event occurs within the QVTK window.
   */
  void mouseEvent(QMouseEvent* event);

protected slots:
  /**
   * Called as a response to `QOpenGLContext::aboutToBeDestroyed`. This may be
   * called anytime during the widget lifecycle. We need to release any OpenGL
   * resources allocated in VTK work in this method.
   */
  virtual void cleanupContext();

private slots:
  /**
   * recreates the FBO used for VTK rendering.
   */
  void recreateFBO();

  /**
   * called before the render window starts to render. We ensure that this->FBO
   * is bound and ready to use.
   */
  void startEventCallback();

  /**
   * callback for changing the cursor. Called when vtkGenericOpenGLRenderWindow
   * fires the CursorChangedEvent.
   */
  void cursorChangedCallback(vtkObject* caller, unsigned long vtk_event,
    void* client_data, void* call_data);

protected:
  bool event(QEvent* evt) Q_DECL_OVERRIDE;
  void initializeGL() Q_DECL_OVERRIDE;
  void resizeGL(int w, int h) Q_DECL_OVERRIDE;
  void paintGL() Q_DECL_OVERRIDE;

  void mousePressEvent(QMouseEvent* event) Q_DECL_OVERRIDE;
  void mouseMoveEvent(QMouseEvent* event) Q_DECL_OVERRIDE;
  void mouseReleaseEvent(QMouseEvent* event) Q_DECL_OVERRIDE;
  void mouseDoubleClickEvent(QMouseEvent* event) Q_DECL_OVERRIDE;

  /**
   * This method is called to indicate that vtkRenderWindow needs to reinitialize
   * itself before the next render (done in QVTKOpenGLNativeWidget::paintGL).
   * This is needed when the context gets recreated
   * or the default FrameBufferObject gets recreated, for example.
   */
  void requireRenderWindowInitialization();

  /**
   * This method may be called in `paintGL` to request VTK to do a render i.e.
   * trigger render on the render window via its interactor.
   *
   * It will return true if render (or an equivalent action) was performed to
   * update the frame buffer made available to VTK for rendering with latest
   * rendering.
   *
   * Default implementation never returns false. However, subclasses can return
   * false to indicate to QVTKOpenGLNativeWidget that it cannot generate a reasonable
   * image to be displayed in QVTKOpenGLNativeWidget. In which case, the `paintGL`
   * call will return leaving the `defaultFramebufferObject` untouched.
   *
   * Since by default `QOpenGLWidget::UpdateBehavior` is set to
   * QOpenGLWidget::PartialUpdate, this means whatever was rendered in the frame
   * buffer in most recent successful call will be preserved, unless the widget
   * was forced to recreate the FBO as a result of resize or screen change.
   *
   * @sa Section @ref RenderAndPaint.
   */
  virtual bool renderVTK();

protected:
  vtkSmartPointer<vtkGenericOpenGLRenderWindow> RenderWindow;
  QVTKInteractorAdapter* InteractorAdapter;

  bool EnableHiDPI;
  int OriginalDPI;

  static const double DevicePixelRatioTolerance;

private:
  Q_DISABLE_COPY(QVTKOpenGLNativeWidget);

  /**
   * Called when vtkCommand::WindowFrameEvent is fired by the
   * vtkGenericOpenGLRenderWindow.
   */
  void windowFrameEventCallback();

  QOpenGLFramebufferObject* FBO;
  bool InPaintGL;
  bool DoVTKRenderInPaintGL;
  vtkNew<QVTKOpenGLNativeWidgetObserver> Observer;
  friend class QVTKOpenGLNativeWidgetObserver;
  QOpenGLDebugLogger* Logger;
};

#endif
