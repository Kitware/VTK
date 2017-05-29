/*=========================================================================

  Program:   Visualization Toolkit
  Module:    QVTKOpenGLWidget.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class QVTKOpenGLWidget
 * @brief QOpenGLWidget subclass to house a vtkGenericOpenGLRenderWindow in a Qt
 * application.
 *
 * QVTKOpenGLWidget extends QOpenGLWidget to make it work with a
 * vtkGenericOpenGLRenderWindow. This is akin to QVTKWidget except it uses Qt to create and
 * manage the OpenGL context using QOpenGLWidget (added in Qt 5.4).
 *
 * While QVTKOpenGLWidget is intended to be a replacement for QVTKWidget when
 * using Qt 5, there are a few difference between QVTKOpenGLWidget and
 * QVTKWidget.
 *
 * Unlike QVTKWidget, QVTKOpenGLWidget only works with vtkGenericOpenGLRenderWindow.
 * This is necessary since QOpenGLWidget wants to take over the window management as
 * well as the OpenGL context creation. Getting that to work reliably with
 * vtkXRenderWindow or vtkWin32RenderWindow (and other platform specific
 * vtkRenderWindow subclasses) was tricky and fraught with issues.
 *
 * Since QVTKOpenGLWidget uses QOpenGLWidget to create the OpenGL context,
 * it uses QSurfaceFormat (set using `QOpenGLWidget::setFormat` or
 * `QSurfaceFormat::setDefaultFormat`) to create appropriate window and context.
 * You can use `QVTKOpenGLWidget::copyToFormat` to obtain a QSurfaceFormat
 * appropriate for a vtkRenderWindow.
 *
 * A typical usage for QVTKOpenGLWidget is as follows:
 * @code{.cpp}
 *
 *  // before initializing QApplication, set the default surface format.
 *  QSurfaceFormat::setDefaultFormat(QVTKOpenGLWidget::defaultFormat());
 *
 *  vtkNew<vtkGenericOpenGLRenderWindow> window;
 *  QPointer<QVTKOpenGLWidget> widget = new QVTKOpenGLWidget(...);
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
 * In QOpenGLWidget (superclass for QVTKOpenGLWidget), all rendering happens in a
 * framebuffer object. Thus, care must be taken in the rendering code to never
 * directly re-bind the default framebuffer i.e. ID 0.
 *
 * QVTKOpenGLWidget creates an internal QOpenGLFramebufferObject, independent of the
 * one created by superclass, for vtkRenderWindow to do the rendering in. This
 * explicit double-buffering is useful in avoiding temporary back-buffer only
 * renders done in VTK (e.g. when making selections) from destroying the results
 * composed on screen.
 *
 * @section RenderAndPaint Handling Render and Paint.
 *
 * QWidget subclasses (including `QOpenGLWidget` and `QVTKOpenGLWidget`) display
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
 * QVTKOpenGLWidget relies on the VTK application calling
 * `vtkRenderWindow::Render` on the render window when it needs to update the
 * rendering. `paintGL` simply passes on the result rendered by the most render
 * vtkRenderWindow::Render to Qt windowing system for composing on-screen.
 *
 * There may still be occasions when we may have to render in `paint` for
 * example if the window was resized or Qt had to recreate the OpenGL context.
 * In those cases, `QVTKOpenGLWidget::paintGL` can request a render by calling
 * `QVTKOpenGLWidget::renderVTK`.
 *
 * @section Caveats
 * QVTKOpenGLWidget only supports **OpenGL2** rendering backend. Thus is not available
 * if VTK is built with **VTK_RENDERING_BACKEND** set to **OpenGL**.
 *
 * QVTKOpenGLWidget is targeted for Qt version 5.5 and above.
 *
 */
#ifndef QVTKOpenGLWidget_h
#define QVTKOpenGLWidget_h

#include <QOpenGLWidget>

#include "QVTKInteractor.h"        // needed for QVTKInteractor
#include "vtkGUISupportQtModule.h" // for export macro
#include "vtkNew.h"                // needed for vtkNew
#include "vtkSmartPointer.h"       // needed for vtkSmartPointer

class QOpenGLDebugLogger;
class QOpenGLFramebufferObject;
class QVTKInteractor;
class QVTKInteractorAdapter;
class QVTKOpenGLWidgetObserver;
class vtkGenericOpenGLRenderWindow;

class VTKGUISUPPORTQT_EXPORT QVTKOpenGLWidget : public QOpenGLWidget
{
  Q_OBJECT
  typedef QOpenGLWidget Superclass;
public:
  QVTKOpenGLWidget(QWidget* parent = Q_NULLPTR, Qt::WindowFlags f = Qt::WindowFlags());
  virtual ~QVTKOpenGLWidget();

  //@{
  /**
   * Get/Set the currently used vtkGenericOpenGLRenderWindow.
   */
  void SetRenderWindow(vtkGenericOpenGLRenderWindow* win);
  void SetRenderWindow(vtkRenderWindow* win);
  virtual vtkRenderWindow* GetRenderWindow();
  //@}

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
   * QVTKOpenGLWidget. Note that this is not the QSurfaceFormat that gets used
   * if none is specified. That is set using `QSurfaceFormat::setDefaultFormat`.
   */
  static QSurfaceFormat defaultFormat();

  /**
   * Enable or disable support for HiDPI displays.
   */
  virtual void setEnableHiDPI(bool enable);

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
   * itself before the next render (done in QVTKOpenGLWidget::paintGL).
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
   * false to indicate to QVTKOpenGLWidget that it cannot generate a reasonable
   * image to be displayed in QVTKOpenGLWidget. In which case, the `paintGL`
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
  QVTKInteractorAdapter* InteractorAdaptor;

  bool EnableHiDPI;
  int OriginalDPI;

private:
  Q_DISABLE_COPY(QVTKOpenGLWidget);

  /**
   * initializes the render window during constructor.
   */
  void initializeRenderWindow(vtkGenericOpenGLRenderWindow* win);

  /**
   * Called when vtkCommand::WindowFrameEvent is fired by the
   * vtkGenericOpenGLRenderWindow.
   */
  void windowFrameEventCallback();

  QOpenGLFramebufferObject* FBO;
  bool InPaintGL;
  bool DoVTKRenderInPaintGL;
  vtkNew<QVTKOpenGLWidgetObserver> Observer;
  friend class QVTKOpenGLWidgetObserver;
  QOpenGLDebugLogger* Logger;
};

#endif
