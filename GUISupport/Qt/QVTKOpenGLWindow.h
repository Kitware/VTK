/*=========================================================================

  Program:   Visualization Toolkit
  Module:    QVTKOpenGLWindow.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class QVTKOpenGLWindow
 * @brief display a vtkGenericOpenGLRenderWindow in a Qt QOpenGLWindow.
 *
 * QVTKOpenGLWindow is one of the mechanisms for displaying VTK rendering
 * results in a Qt application. QVTKOpenGLWindow extends QOpenGLWindow to
 * display the rendering results of a vtkGenericOpenGLRenderWindow.
 *
 * Since QVTKOpenGLWindow is based on QOpenGLWindow it is intended for
 * rendering in a top-level window. QVTKOpenGLWindow can be embedded in a
 * another QWidget using `QWidget::createWindowContainer` or by using
 * QVTKOpenGLWidget instead. However, developers are encouraged to check
 * Qt documentation for `QWidget::createWindowContainer` idiosyncrasies.
 * Using QVTKOpenGLNativeWidget instead is generally a better choice for causes
 * where you want to embed VTK rendering results in a QWidget. QVTKOpenGLWindow
 * or QVTKOpenGLWidget is still preferred for applications that want to support
 * quad-buffer based stereo rendering.
 *
 * To request a specific configuration for the context, use
 * `QWindow::setFormat()` like for any other QWindow. This allows, among others,
 * requesting a given OpenGL version and profile. Use
 * `QOpenGLWindow::defaultFormat()` to obtain a QSurfaceFormat with appropriate
 * OpenGL version configuration. To enable quad-buffer stereo, you'll need to
 * call `QSurfaceFormat::setStereo(true)`.
 *
 * VTK Rendering features like multi-sampling, double buffering etc.
 * are enabled/disabled by directly setting the corresponding attributes on
 * vtkGenericOpenGLRenderWindow and not when specifying the OpenGL context
 * format in `setFormat`. If not specified, then `QSurfaceFormat::defaultFormat`
 * will be used.
 *
 * @note QVTKOpenGLWindow requires Qt version 5.9 and above.
 * @sa QVTKOpenGLWidget QVTKOpenGLNativeWidget
 */
#ifndef QVTKOpenGLWindow_h
#define QVTKOpenGLWindow_h

#include <QOpenGLWindow>
#include <QScopedPointer> // for QScopedPointer.

#include "QVTKInteractor.h"        // needed for QVTKInteractor
#include "vtkGUISupportQtModule.h" // for export macro
#include "vtkNew.h"                // needed for vtkNew
#include "vtkSmartPointer.h"       // needed for vtkSmartPointer

class QVTKInteractor;
class QVTKInteractorAdapter;
class QVTKRenderWindowAdapter;
class vtkGenericOpenGLRenderWindow;

class VTKGUISUPPORTQT_EXPORT QVTKOpenGLWindow : public QOpenGLWindow
{
  Q_OBJECT
  typedef QOpenGLWindow Superclass;

public:
  QVTKOpenGLWindow(
    QOpenGLWindow::UpdateBehavior updateBehavior = NoPartialUpdate, QWindow* parent = nullptr);
  QVTKOpenGLWindow(QOpenGLContext* shareContext,
    QOpenGLWindow::UpdateBehavior updateBehavior = NoPartialUpdate, QWindow* parent = nullptr);
  QVTKOpenGLWindow(vtkGenericOpenGLRenderWindow* renderWindow,
    QOpenGLWindow::UpdateBehavior updateBehavior = NoPartialUpdate, QWindow* parent = nullptr);
  QVTKOpenGLWindow(vtkGenericOpenGLRenderWindow* renderWindow, QOpenGLContext* shareContext,
    QOpenGLWindow::UpdateBehavior updateBehavior = NoPartialUpdate, QWindow* parent = nullptr);
  ~QVTKOpenGLWindow() override;

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
   * @deprecated in VTK 8.3. Use `setRenderWindow` instead.
   */
  VTK_LEGACY(void SetRenderWindow(vtkGenericOpenGLRenderWindow* win));
  VTK_LEGACY(void SetRenderWindow(vtkRenderWindow* win));
  //@}

  //@{
  /**
   * These methods have be deprecated to fix naming style. Since
   * QVTKOpenGLWindow is QObject subclass, we follow Qt naming conventions
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

signals:
  /**
   * Signal emitted when any event has been receive, with the corresponding
   * event as argument.
   */
  void windowEvent(QEvent* e);

protected slots:
  /**
   * Called as a response to `QOpenGLContext::aboutToBeDestroyed`. This may be
   * called anytime during the widget lifecycle. We need to release any OpenGL
   * resources allocated in VTK work in this method.
   */
  void cleanupContext();

  void updateSize();

  /**
   * QVTKOpenGLWidget is given friendship so it can call `cleanupContext` in its
   * destructor to ensure that OpenGL state is proporly cleaned up before the
   * widget goes away.
   */
  friend class QVTKOpenGLWidget;

protected:
  bool event(QEvent* evt) override;
  void initializeGL() override;
  void paintGL() override;
  void resizeGL(int w, int h) override;

protected:
  vtkSmartPointer<vtkGenericOpenGLRenderWindow> RenderWindow;
  QScopedPointer<QVTKRenderWindowAdapter> RenderWindowAdapter;

private:
  Q_DISABLE_COPY(QVTKOpenGLWindow);
  bool EnableHiDPI;
  int UnscaledDPI;
  QCursor DefaultCursor;
};

#endif
