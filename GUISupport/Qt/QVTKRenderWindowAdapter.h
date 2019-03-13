/*=========================================================================

  Program:   Visualization Toolkit
  Module:    QVTKRenderWindowAdapter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef QVTKRenderWindowAdapter_h
#define QVTKRenderWindowAdapter_h

/**
 * @class QVTKRenderWindowAdapter
 * @brief Helper to manage Qt context and other OpenGL components
 *
 * QVTKRenderWindowAdapter is an internal class that is used by
 * QVTKOpenGLNativeWidget and QVTKOpenGLWindow to manage the rendering using
 * vtkGenericOpenGLRenderWindow within a OpenGL context created via Qt.
 *
 * QVTKRenderWindowAdapter is expected to be recreated anytime the context
 * changes. In the constructor, QVTKRenderWindowAdapter will mark the
 * vtkGenericOpenGLRenderWindow ready for rendering and call OpenGL context
 * initialization API (vtkOpenGLRenderWindow::OpenGLInitContext).
 *
 * By observing events on vtkGenericOpenGLRenderWindow, QVTKRenderWindowAdapter
 * can then support rendering to an internally created FBO via VTK's rendering
 * calls. Making sure that the rendering results are shown on the screen is
 * handled by QVTKOpenGLWindow or QVTKOpenGLNativeWidget.
 */

#include "vtkGUISupportQtModule.h" // For export macro
#include <QObject>

#include <QCursor>        // for ivar
#include <QScopedPointer> // for ivar

class QOpenGLContext;
class QSurfaceFormat;
class QWidget;
class QWindow;
class vtkGenericOpenGLRenderWindow;
class vtkObject;

class VTKGUISUPPORTQT_EXPORT QVTKRenderWindowAdapter : public QObject
{
  Q_OBJECT;
  using Superclass = QObject;

public:
  //@{
  /**
   * Constructor that makes vtkGenericOpenGLRenderWindow ready for subsequent
   * render requests i.e. calls
   * `vtkGenericOpenGLRenderWindow::SetReadyForRendering(true)`. This also calls
   * `vtkOpenGLRenderWindow::OpenGLInitContext` to ensure that the OpenGL
   * context is ready for VTK rendering.
   */
  QVTKRenderWindowAdapter(
    QOpenGLContext* cntxt, vtkGenericOpenGLRenderWindow* window, QWindow* parent);
  QVTKRenderWindowAdapter(
    QOpenGLContext* cntxt, vtkGenericOpenGLRenderWindow* window, QWidget* parent);
  //@}

  ~QVTKRenderWindowAdapter() override;

  /**
   * Returns a QSurfaceFormat suitable for surfaces that intend to be used for
   * VTK rendering.
   *
   * If your applications plans on using `QVTKOpenGLNativeWidget`, then this
   * format (or similar) must be set as the default format on QSurfaceFormat
   * before any widgets are created.
   *
   * Note this returns a QSurfaceFormat required to support the OpenGL rendering
   * capabilities in a vtkRenderWindow. Whether those features, e.g. multi
   * sampling, is actually used for rendering is determined by values
   * specified on the vtkRenderWindow instance itself through appropriate API.
   *
   * Passing `stereo_capable=true` is same as calling `QSurfaceFormat::setStereo(true)`.
   * This is necessary if you want to use quad-buffer based stereo in your
   * application.
   *
   * Refer to Qt docs for QOpenGLWidget and QOpenGLWindow for appropriate
   * locations in your application where to the format may be provided e.g.
   * either on the instance of QOpenGLWindow or QOpenGLWidget subclasses or as
   * default format for the application using `QSurfaceFormat::setDefaultFormat()`.
   */
  static QSurfaceFormat defaultFormat(bool stereo_capable = false);

  /**
   * Get the context to use for rendering.
   */
  QOpenGLContext* context() const;

  /**
   * Call this method in `paintGL` to request a render. This may trigger a
   * `vtkRenderWindow::Render` if this class determines the buffers may be
   * obsolete.
   */
  void paint();

  /**
   * Call this method to resize the render window. This simply calls
   * `vtkRenderWindow::SetSize` taking device pixel ratio into consideration.
   * This doesn't cause a render or resize of the FBO. That happens on a
   * subsequent render request.
   *
   * Besides widget resize, this method should also be called in cases when the
   * devicePixelRatio for the parent window (or widget) changes. This is
   * necessary since the internal FBO's pixel size is computed by scaling the
   * `widget` and `height` provided by the window's (or widget's) `devicePixelRatio`.
   */
  void resize(int width, int height);

  //@{
  /**
   * Convenience methods to blit the results rendered in the internal FBO to a
   * target
   */
  bool blit(
    unsigned int targetId, int targetAttachement, const QRect& targetRect, bool left = true);
  bool blitLeftEye(unsigned int targetId, int targetAttachement, const QRect& targetRect)
  {
    return this->blit(targetId, targetAttachement, targetRect, true);
  }
  bool blitRightEye(unsigned int targetId, int targetAttachement, const QRect& targetRect)
  {
    return this->blit(targetId, targetAttachement, targetRect, false);
  }
  //@}

  /**
   * Process the event and return true if the event have been processed
   * successfully.
   */
  bool handleEvent(QEvent* evt);

  //@{
  /**
   * Get/set the default cursor.
   */
  void setDefaultCursor(const QCursor& cursor) { this->DefaultCursor = cursor; }
  const QCursor& defaultCursor() const { return this->DefaultCursor; }
  //@}

  //@{
  /**
   * Enable/disable DPI scaling. When enabled call to `resize` (which must
   * happen any time the `devicePixelRatio`, in addition to the size may
   * change), will result in updating the DPI on the
   * vtkGenericOpenGLRenderWindow as well. The DPI change only happens in
   * `resize` to enable applications to temporarily change DPI on the
   * vtkGenericOpenGLRenderWindow and request an explicit render seamlessly. In
   * such a case, it's the applications responsibility to restore DPI value or
   * the changed value will linger until the next `resize` happens.
   */
  void setEnableHiDPI(bool value);
  //@}

  //@{
  /**
   * Set the unscaled DPI to use when scaling DPI. It defaults to 72, which is
   * same as the hard-coded default in `vtkWindow`.
   */
  void setUnscaledDPI(int value);
  //@}

private slots:
  void contextAboutToBeDestroyed();

private:
  QVTKRenderWindowAdapter(
    QOpenGLContext* cntxt, vtkGenericOpenGLRenderWindow* window, QObject* widgetOrWindow);
  Q_DISABLE_COPY(QVTKRenderWindowAdapter);

  class QVTKInternals;
  QScopedPointer<QVTKInternals> Internals;

  QCursor DefaultCursor;
};

#endif
