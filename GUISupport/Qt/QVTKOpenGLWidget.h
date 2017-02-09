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
 * @section OpenGL Context
 *
 * In QOpenGLWidget (supclass for QVTKOpenGLWidget), all rendering happens in a
 * framebuffer object. Thus, care must be taken in the rendering code to never
 * directly re-bind the framebuffer with ID 0.
 *
 * @section Handling Render and Paint.
 *
 * In VTK, rendering is requested calling `vtkRenderWindow::Render`, while with
 * Qt, a widget is updated in an **paint** event. Since QVTKOpenGLWidget renders
 * to a framebuffer, any OpenGL calls don't direcly update the image shown on the
 * screen. Instead, when QVTKOpenGLWidget::paintGL returns, Qt composes the
 * rendered image with the widget stack and displays on screen. This poses a
 * challenge when VTK (or the VTK application) triggers a render outside the
 * paint event by calling `vtkRenderWindow::Render`, since the rendering results
 * won't show on screen until Qt composes the rendered image. To handle that,
 * QVTKOpenGLWidget listens to the vtkCommand::WindowFrameEvent fired by
 * vtkGenericOpenGLRenderWindow when vtkRenderWindow::Frame is called. That
 * typically happens at the end of a render to swap the front and back buffers
 * (for example). If QVTKOpenGLWidget receives that event outside of a paintGL
 * call, then it requests Qt to update the widget by calling `QWidget::update`.
 * Note, when `paintGL` subsequently gets called, QVTKOpenGLWidget leaves the
 * framebuffer untouched i.e. doesn't call vtkRenderWindow::Render again.
 * If vtkRenderWindow::SwapBuffers if off, then the `QWidget::update` is not
 * called. The rationale is that the changes were done in back buffer, most
 * likely and hence are not intended to be shown onscreen.
 *
 * @section Caveats
 * QVTKOpenGLWidget only supports **OpenGL2** rendering backend. Thus is not available
 * if VTK is built with **VTK_RENDERING_BACKEND** set to **OpenGL**.
 */
#ifndef QVTKOpenGLWidget_h
#define QVTKOpenGLWidget_h

#include <QOpenGLWidget>

#include "QVTKInteractor.h"        // needed for QVTKInteractor
#include "vtkGUISupportQtModule.h" // for export macro
#include "vtkNew.h"                // needed for vtkNew
#include "vtkSmartPointer.h"       // needed for vtkSmartPointer
#include "vtkTextureObject.h"      // needed for vtkTextureObject.
#include <QTimer>                  // needed for QTimer.

class QVTKInteractor;
class QVTKInteractorAdapter;
class vtkGenericOpenGLRenderWindow;
class QVTKOpenGLWidgetObserver;

class VTKGUISUPPORTQT_EXPORT QVTKOpenGLWidget : public QOpenGLWidget
{
  Q_OBJECT
  Q_PROPERTY(bool automaticImageCacheEnabled READ isAutomaticImageCacheEnabled WRITE
      setAutomaticImageCacheEnabled)
  Q_PROPERTY(double maxRenderRateForImageCache READ maxRenderRateForImageCache WRITE
      setMaxRenderRateForImageCache)
  Q_PROPERTY(
    bool deferRenderInPaintEvent READ deferRenderInPaintEvent WRITE setDeferRenderInPaintEvent)

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

  //@{
  /**
   * Enables/disables automatic image caching.  If disabled (the default),
   * QOpenGLWidget will not call saveImageToCache() on its own.
   */
  virtual void setAutomaticImageCacheEnabled(bool flag);
  virtual bool isAutomaticImageCacheEnabled() const { return this->AutomaticImageCacheEnabled; }
  //@}

  //@{
  /**
   * If automatic image caching is enabled, then the image will be cached
   * after every render with a DesiredUpdateRate that is less than
   * this parameter.  By default, the vtkRenderWindowInteractor will
   * change the desired render rate depending on the user's
   * interactions. (See vtkRenderWindow::DesiredUpdateRate,
   * vtkRenderWindowInteractor::DesiredUpdateRate and
   * vtkRenderWindowInteractor::StillUpdateRate for more details.)
   */
  virtual void setMaxRenderRateForImageCache(double rate);
  virtual double maxRenderRateForImageCache() const { return this->MaxRenderRateForImageCache; }
  //@}

  //@{
  /**
   * When set to true (default is false), paintEvent() will never directly trigger
   * a render on the vtkRenderWindow (via vtkRenderWindowInteractor::Render()).
   * Instead, it starts a timer that then triggers the render on idle. This, in
   * general is a good strategy for cases where Render may take a while with
   * applications wanting to report progress and consequently trigger paint
   * events on other widgets like progress bars, etc.
   * There is one caveat: when paintEvent() is called using a redirected paint device,
   * then this flag is ignored and the paintEvent() will trigger
   * vtkRenderWindowInteractor::Render(), if needed.
   */
  virtual void setDeferRenderInPaintEvent(bool val);
  virtual bool deferRenderInPaintEvent() const { return this->DeferRenderInPaintEvent; }
  //@}

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

public slots:
  /**
   * This will mark the cached image as dirty.  This slot is automatically
   * invoked whenever the render window has a render event or the widget is
   * resized.  Your application should invoke this slot whenever the image in
   * the render window is changed by some other means.  If the image goes
   * from clean to dirty, the cachedImageDirty() signal is emitted.
   */
  void markCachedImageAsDirty();

  /**
   * If the cached image is dirty, it is updated with the current image in the render window
   * and the cachedImageClean() signal is emitted.
   */
  void saveImageToCache();

signals:
  /**
   * This signal will be emitted whenever a mouse event occurs within the QVTK window.
   */
  void mouseEvent(QMouseEvent* event);

  /**
   * This signal will be emitted whenever the cached image goes from clean to dirty.
   */
  void cachedImageDirty();

  /**
   * This signal will be emitted whenever the cached image is refreshed.
   */
  void cachedImageClean();

protected slots:
  /**
   * Request to defer a render call i.e. start the mDeferedRenderTimer. When the
   * timer times out, it will call doDeferredRender() to do the actual rendering.
   */
  virtual void deferRender();

  /**
   * Called when the mDeferedRenderTimer times out to do the rendering.
   */
  virtual void doDeferredRender();

  bool event(QEvent* evt) Q_DECL_OVERRIDE;

  /**
   * Called as a response to `QOpenGLContext::aboutToBeDestroyed`. This may be
   * called anytime during the widget lifecycle. We need to release any OpenGL
   * resources allocated in VTK work in this method.
   */
  virtual void cleanupContext();

  /**
   * Slot connected to `QOpenGLWidget::resized`. The `resized`
   * signal is fired when `QOpenGLWidget` recreates the FBO used for
   * rendering. Since this means that the `vtkOpenGLRenderWindow::DefaultFrameBufferId`
   * may have changed, we need to tell vtkOpenGLRenderWindow to reinitialize itself
   * the next time `QVTKOpenGLWidget::paintGL` gets called.
   */
  virtual void defaultFrameBufferObjectChanged();

protected:
  void initializeGL() Q_DECL_OVERRIDE;
  void resizeGL(int w, int h) Q_DECL_OVERRIDE;
  void paintGL() Q_DECL_OVERRIDE;
  void moveEvent(QMoveEvent* event) Q_DECL_OVERRIDE;

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
   * This method is called in paintGL() to render the image cache on to the device.
   * return false, if cache couldn't be used for painting. In that case, the
   * paintGL() method will continue with the default painting code.
   */
  virtual bool paintCachedImage();

protected:
  vtkSmartPointer<vtkTextureObject> CachedTexture;
  bool AutomaticImageCacheEnabled;
  bool MaxRenderRateForImageCache;
  bool DeferRenderInPaintEvent;

  vtkSmartPointer<vtkGenericOpenGLRenderWindow> RenderWindow;
  QVTKInteractorAdapter* InteractorAdaptor;

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

  QTimer DeferedRenderTimer;
  bool InPaintGL;
  bool NeedToReinitializeWindow;
  bool SkipRenderInPaintGL;
  vtkNew<QVTKOpenGLWidgetObserver> Observer;
  friend class QVTKOpenGLWidgetObserver;
};

#endif
