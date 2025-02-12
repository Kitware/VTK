// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
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
 * QVTKOpenGLStereoWidget instead. However, developers are encouraged to check
 * Qt documentation for `QWidget::createWindowContainer` idiosyncrasies.
 * Using QVTKOpenGLNativeWidget instead is generally a better choice for causes
 * where you want to embed VTK rendering results in a QWidget. QVTKOpenGLWindow
 * or QVTKOpenGLStereoWidget is still preferred for applications that want to support
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
 * @sa QVTKOpenGLStereoWidget QVTKOpenGLNativeWidget
 */
#ifndef QVTKOpenGLWindow_h
#define QVTKOpenGLWindow_h

#include <QOpenGLWindow>
#include <QScopedPointer> // for QScopedPointer.

#include "QVTKInteractor.h"        // needed for QVTKInteractor
#include "vtkGUISupportQtModule.h" // for export macro
#include "vtkNew.h"                // needed for vtkNew
#include "vtkSmartPointer.h"       // needed for vtkSmartPointer

VTK_ABI_NAMESPACE_BEGIN
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

  ///@{
  /**
   * Set a render window to use. It a render window was already set, it will be
   * finalized and all of its OpenGL resource released. If the \c win is
   * non-null and it has no interactor set, then a QVTKInteractor instance will
   * be created as set on the render window as the interactor.
   */
  void setRenderWindow(vtkGenericOpenGLRenderWindow* win);
  void setRenderWindow(vtkRenderWindow* win);
  ///@}

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

  ///@{
  /**
   * Enable or disable support for touch event processing. When enabled, this widget
   * will process Qt::TouchBegin/TouchUpdate/TouchEnd event, otherwise, these events
   * will be ignored. For some vtk widgets like vtkDistanceWidget, if this option is
   * enabled, it will received leftButtonPressed/leftButtonRelease twice for one touch,
   * this breaks its designed logics. Default is true.
   */
  void setEnableTouchEventProcessing(bool enable);
  bool enableTouchEventProcessing() const { return this->EnableTouchEventProcessing; }
  ///@}

  ///@{
  /**
   * Enable or disable support for HiDPI displays. When enabled, this enabled
   * DPI scaling i.e. `vtkWindow::SetDPI` will be called with a DPI value scaled
   * by the device pixel ratio every time the widget is resized. The unscaled
   * DPI value can be specified by using `setUnscaledDPI`.
   */
  void setEnableHiDPI(bool enable);
  bool enableHiDPI() const { return this->EnableHiDPI; }
  ///@}

  ///@{
  /**
   * Set/Get unscaled DPI value. Defaults to 72, which is also the default value
   * in vtkWindow.
   */
  void setUnscaledDPI(int);
  int unscaledDPI() const { return this->UnscaledDPI; }
  ///@}

  ///@{
  /**
   * Set/Get a custom device pixel ratio to use to map Qt sizes to VTK (or
   * OpenGL) sizes. Thus, when the QWidget is resized, it called
   * `vtkRenderWindow::SetSize` on the internal vtkRenderWindow after
   * multiplying the QWidget's size by this scale factor.
   *
   * By default, this is set to 0. Which means that `devicePixelRatio` obtained
   * from Qt will be used. Set this to a number greater than 0 to override this
   * behaviour and use the custom scale factor instead.
   *
   * `effectiveDevicePixelRatio` can be used to obtain the device-pixel-ratio
   * that will be used given the value for customDevicePixelRatio.
   */
  void setCustomDevicePixelRatio(double cdpr);
  double customDevicePixelRatio() const { return this->CustomDevicePixelRatio; }
  double effectiveDevicePixelRatio() const;
  ///@}

  ///@{
  /**
   * Set/get the default cursor to use for this widget.
   */
  void setDefaultCursor(const QCursor& cursor);
  const QCursor& defaultCursor() const { return this->DefaultCursor; }
  ///@}

Q_SIGNALS:
  /**
   * Signal emitted when any event has been receive, with the corresponding
   * event as argument.
   */
  void windowEvent(QEvent* e);

protected Q_SLOTS:
  /**
   * Called as a response to `QOpenGLContext::aboutToBeDestroyed`. This may be
   * called anytime during the widget lifecycle. We need to release any OpenGL
   * resources allocated in VTK work in this method.
   */
  void cleanupContext();

  void updateSize();

  /**
   * QVTKOpenGLStereoWidget is given friendship so it can call `cleanupContext` in its
   * destructor to ensure that OpenGL state is proporly cleaned up before the
   * widget goes away.
   */
  friend class QVTKOpenGLStereoWidget;

protected: // NOLINT(readability-redundant-access-specifiers)
  bool event(QEvent* evt) override;
  void initializeGL() override;
  void paintGL() override;
  void resizeGL(int w, int h) override;

  vtkSmartPointer<vtkGenericOpenGLRenderWindow> RenderWindow;
  QScopedPointer<QVTKRenderWindowAdapter> RenderWindowAdapter;

private:
  Q_DISABLE_COPY(QVTKOpenGLWindow);
  bool EnableTouchEventProcessing = true;
  bool EnableHiDPI;
  int UnscaledDPI;
  double CustomDevicePixelRatio;
  QCursor DefaultCursor;
};

VTK_ABI_NAMESPACE_END
#endif
