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
#include <QScopedPointer> // for QScopedPointer.

#include "QVTKInteractor.h"        // needed for QVTKInteractor
#include "vtkGUISupportQtModule.h" // for export macro
#include "vtkNew.h"                // needed for vtkNew
#include "vtkSmartPointer.h"       // needed for vtkSmartPointer

class QVTKInteractor;
class QVTKInteractorAdapter;
class QVTKRenderWindowAdapter;
class vtkGenericOpenGLRenderWindow;

class VTKGUISUPPORTQT_EXPORT QVTKOpenGLNativeWidget : public QOpenGLWidget
{
  Q_OBJECT
  typedef QOpenGLWidget Superclass;

public:
  QVTKOpenGLNativeWidget(QWidget* parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());
  QVTKOpenGLNativeWidget(vtkGenericOpenGLRenderWindow* window, QWidget* parent = nullptr,
    Qt::WindowFlags f = Qt::WindowFlags());
  ~QVTKOpenGLNativeWidget() override;

  //@{
  /**
   * Set a render window to use. It a render window was already set, it will be
   * finalized and all of its OpenGL resource released. If the \c win is
   * non-null and it has no interactor set, then a QVTKInteractor instance will
   * be created as set on the render window as the interactor.
   */
  void setRenderWindow(vtkGenericOpenGLRenderWindow* win);
  void setRenderWindow(vtkRenderWindow* win);
  //@}

  /**
   * Returns the render window that is being shown in this widget.
   */
  vtkRenderWindow* renderWindow() const;

  /**
   * Get the QVTKInteractor that was either created by default or set by the user.
   */
  QVTKInteractor* interactor() const;

  /**
   * @copydoc QVTKRenderWindowAdapter::defaultFormat(bool)
   */
  static QSurfaceFormat defaultFormat(bool stereo_capable = false);

  //@{
  /**
   * Enable or disable support for HiDPI displays. When enabled, this enabled
   * DPI scaling i.e. `vtkWindow::SetDPI` will be called with a DPI value scaled
   * by the device pixel ratio every time the widget is resized. The unscaled
   * DPI value can be specified by using `setUnscaledDPI`.
   */
  void setEnableHiDPI(bool enable);
  bool enableHiDPI() const { return this->EnableHiDPI; }
  //@}

  //@{
  /**
   * Set/Get unscaled DPI value. Defaults to 72, which is also the default value
   * in vtkWindow.
   */
  void setUnscaledDPI(int);
  int unscaledDPI() const { return this->UnscaledDPI; }
  //@}

  //@{
  /**
   * Set/get the default cursor to use for this widget.
   */
  void setDefaultCursor(const QCursor& cursor);
  const QCursor& defaultCursor() const { return this->DefaultCursor; }
  //@}

  //@{
  /**
   * @deprecated in VTK 8.3
   */
  VTK_LEGACY(void SetRenderWindow(vtkGenericOpenGLRenderWindow* win));
  VTK_LEGACY(void SetRenderWindow(vtkRenderWindow* win));
  //@}

  //@{
  /**
   * These methods have be deprecated to fix naming style. Since
   * QVTKOpenGLNativeWidget is QObject subclass, we follow Qt naming conventions
   * rather than VTK's.
   */
  VTK_LEGACY(vtkRenderWindow* GetRenderWindow());
  VTK_LEGACY(QVTKInteractor* GetInteractor());
  //@}

  /**
   * @deprecated in VTK 8.3
   * QVTKInteractorAdapter is an internal helper. Hence the API was removed.
   */
  VTK_LEGACY(QVTKInteractorAdapter* GetInteractorAdapter());

  /**
   * @deprecated in VTK 8.3. Simply use `QWidget::setCursor` API to change
   * cursor.
   */
  VTK_LEGACY(void setQVTKCursor(const QCursor& cursor));

  /**
   * @deprecated in VTK 8.3. Use `setDefaultCursor` instead.
   */
  VTK_LEGACY(void setDefaultQVTKCursor(const QCursor& cursor));

protected slots:
  /**
   * Called as a response to `QOpenGLContext::aboutToBeDestroyed`. This may be
   * called anytime during the widget lifecycle. We need to release any OpenGL
   * resources allocated in VTK work in this method.
   */
  virtual void cleanupContext();

  void updateSize();

protected:
  bool event(QEvent* evt) override;
  void initializeGL() override;
  void paintGL() override;

protected:
  vtkSmartPointer<vtkGenericOpenGLRenderWindow> RenderWindow;
  QScopedPointer<QVTKRenderWindowAdapter> RenderWindowAdapter;

private:
  Q_DISABLE_COPY(QVTKOpenGLNativeWidget);

  bool EnableHiDPI;
  int UnscaledDPI;
  QCursor DefaultCursor;
};

#endif
