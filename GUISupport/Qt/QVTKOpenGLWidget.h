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
#ifndef QVTKOpenGLWidget_h
#define QVTKOpenGLWidget_h

#include "vtkGUISupportQtModule.h" // For export macro

#include <QWidget>

// Forward Qt class declarations
class QSurfaceFormat;
class QOpenGLContext;

//class QVTKInteractor;
class QVTKInteractorAdapter;
class QVTKOpenGLWindow;
class vtkGenericOpenGLRenderWindow;
class vtkRenderWindow;
class vtkRenderWindowInteractor;

/**
 * @class QVTKOpenGLWidget
 * @brief QWidget for displaying a vtkRenderWindow in a Qt Application.
 *
 * QVTKOpenGLWidget is a QWidget wrapper around QVTKOpenGLWindow. It holds a
 * pointer to an internal QVTKOpenGLWindow instance in order to display VTK
 * data in a Qt OpenGL context.
 *
 * It was designed to support quad buffer stereo rendering.
 *
 * A typical usage for QVTKOpenGLWidget is as follows:
 * @code{.cpp}
 *
 *  // This must be done before instantiating the app in order to setup the
 *  // right context for our widget.
 *  QSurfaceFormat::setDefaultFormat(QVTKOpenGLWidget::defaultFormat());
 *
 *  QApplication app(argc, argv);
 *
 *  vtkNew<vtkGenericOpenGLRenderWindow> window;
 *  QScopedPointer<QVTKOpenGLWidget> qvtkWidget(new QVTKOpenGLWidget(window));
 *
 *  // You can continue to use `window` as a regular vtkRenderWindow
 *  // including adding renderers, actors etc.
 *
 *  // Show the widget and start the app.
 *  qvtkWidget->show();
 *  return app.exec();
 *
 * @endcode
 *
 * External calls requesting the render window to render might not be safe.
 * Please make sure that QVTKOpenGLWidget::isValid() returns true before
 * making explicit call to vtkGenericOpenGLRenderWindow::Render().
 * An alternative is to call update() on the widget instance to trigger a
 * render once the context gets validated;
 *
 * QVTKOpenGLWidget is compatible with Qt version 5.6 and above,
 * but it is mainly tested on Qt 5.9 and above.
 *
 * Due to Qt limitations, QVTKOpenGLWidget does not support being a
 * native widget.
 * But native widget are sometimes mandatory, for example within
 * QScrollArea and QMDIArea, so the QVTKOpenGLNativeWidget should be
 * used when in needs of VTK rendering in the context of Qt native widget.
 *
 * If a QVTKOpenGLWidget is used in a QScrollArea or in a QMDIArea, it
 * will force it to be native and this is *NOT* supported.
 *
 * @sa QVTKOpenGLWindow QVTKOpenGLNativeWidget
 */
class VTKGUISUPPORTQT_EXPORT QVTKOpenGLWidget : public QWidget
{
  Q_OBJECT
  typedef QWidget Superclass;
public:
  QVTKOpenGLWidget(QWidget* parent = Q_NULLPTR,
    Qt::WindowFlags f = Qt::WindowFlags());
  QVTKOpenGLWidget(QOpenGLContext *shareContext, QWidget* parent = Q_NULLPTR,
    Qt::WindowFlags f = Qt::WindowFlags());
  QVTKOpenGLWidget(vtkGenericOpenGLRenderWindow* w,
    QWidget* parent = Q_NULLPTR, Qt::WindowFlags f = Qt::WindowFlags());
  QVTKOpenGLWidget(vtkGenericOpenGLRenderWindow* w, QOpenGLContext *shareContext,
    QWidget* parent = Q_NULLPTR, Qt::WindowFlags f = Qt::WindowFlags());
  ~QVTKOpenGLWidget() override;

  /**
   * Set the VTK render window for the internal QVTKOpenGLWindow.
   */
  void SetRenderWindow(vtkGenericOpenGLRenderWindow* win);
  void SetRenderWindow(vtkRenderWindow* win);

  /**
   * Get the VTK render window from the internal QVTKOpenGLWindow.
   */
  virtual vtkRenderWindow* GetRenderWindow();

  /**
   * Get the VTK render window interactor from the internal QVTKOpenGLWindow.
   */
  virtual vtkRenderWindowInteractor* GetInteractor();

  /**
   * Get the QEvent to VTK events translator.
   */
  virtual QVTKInteractorAdapter* GetInteractorAdapter();

  /**
   * Returns a typical QSurfaceFormat suitable for most applications using
   * QVTKOpenGLWidget. Note that this is not the QSurfaceFormat that gets used
   * if none is specified. That is set using `QSurfaceFormat::setDefaultFormat`.
   */
  static QSurfaceFormat defaultFormat();

  /**
   * Set the QSurfaceFormat used to create the OpenGL context.
   */
  void setFormat(const QSurfaceFormat& format);

  /**
   * Enable or disable support for HiDPI displays.
   */
  virtual void setEnableHiDPI(bool enable);
  virtual bool enableHiDPI() { return this->EnableHiDPI; }

  /**
   * Set the cursor on this widget.
   */
  void setQVTKCursor(const QCursor &cursor);

  /**
   * Returns true if the internal QOpenGLWindow's is valid, i.e. if OpenGL
   * resources, like the context, have been successfully initialized.
   */
  virtual bool isValid();

  /**
   * Forward events to the internal QVTKOpenGLWindow when events are
   * explicitly sent to the widget. This is required due to QTBUG-61836 that
   * prevents the use of the flag Qt::TransparentForMouseInput.
   * This flag indicates that the internal window let events pass through.
   * When this misbehavior gets fixed, events will be forwarded to this widget's
   * event() callback, that will then forward them back to the window.
   */
  virtual bool testingEvent(QEvent* e);

  /**
   * Expose internal QVTKOpenGLWindow::grabFramebuffer(). Renders and returns
   * a 32-bit RGB image of the framebuffer.
   */
  QImage grabFramebuffer();

signals:
  /**
   * This signal will be emitted whenever a mouse event occurs within the QVTK window.
   */
  void mouseEvent(QMouseEvent* event);

  /**
   * This signal will be emitted whenever a resize event occurs within the QVTK window.
   */
  void resized();

  /**
   * Forward events to the internal QVTK window.
   */
  void widgetEvent(QEvent* e);

private slots:
  /**
   * called as a response to `QVTKOpenGLWindow::event` to forward the signal.
   */
  void windowEvent(QEvent* event);

protected:
  virtual void resizeEvent(QResizeEvent* event) Q_DECL_OVERRIDE;
  virtual bool event(QEvent* e) Q_DECL_OVERRIDE;

  bool EnableHiDPI = true;

private:
  QVTKOpenGLWindow* qVTKOpenGLWindowInternal;
};

#endif
