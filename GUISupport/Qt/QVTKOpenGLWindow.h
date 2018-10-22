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
#ifndef QVTKOpenGLWindow_H
#define QVTKOpenGLWindow_H

#include "vtkGUISupportQtModule.h" // For export macro
#include "vtkSmartPointer.h" // Required for smart pointer internal ivars.
#include <QOpenGLWindow>

class QOffscreenSurface;
// Forward VTK class declarations
class QVTKInteractorAdapter;
class vtkEventQtSlotConnect;
class vtkGenericOpenGLRenderWindow;
class vtkObject;
class vtkRenderWindow;

/**
 * @class QVTKOpenGLWindow
 * @brief display a vtkGenericOpenGLRenderWindow in a Qt QOpenGLWindow.
 *
 * QVTKOpenGLWindow provides a way to display VTK data in a Qt OpenGL context.
 *
 * QVTKOpenGLWindow extends QOpenGLWindow to make it work with a
 * vtkGenericOpenGLRenderWindow. This is akin to QVTKOpenGLNativeWidget except the
 * OpenGL context is created and managed by QOpenGLWindow (added in Qt 5.4).
 *
 * While QVTKOpenGLWindow is intended to be a replacement for
 * QVTKOpenGLNativeWidget, there are a few difference between both.
 * Unlike QVTKOpenGLNativeWidget, QVTKOpenGLWindow can target multiple framebuffers
 * and thus allows for stereo-capable applications.
 * QVTKOpenGLWindow API was adapted from deprecated QVTKWidget2.
 *
 * Since QVTKOpenGLWindow uses QOpenGLWindow to create the OpenGL context,
 * it uses QSurfaceFormat (set using `QOpenGLWidget::setFormat` or
 * `QSurfaceFormat::setDefaultFormat`)to create appropriate window and context.
 *
 * Since QOpenGLWindow (superclass for QVTKOpenGLWindow) is not a subclass
 * of QWidget, a typical usage for QVTKOpenGLWindow is as follows:
 * @code{.cpp}
 *
 *  vtkNew<vtkGenericOpenGLRenderWindow> window;
 *  QPointer<QVTKOpenGLWindow> qWindow = new QVTKOpenGLWindow(window);
 *
 *  // You can continue to use `window` as a regular vtkRenderWindow
 *  // including adding renderers, actors etc.
 *
 *  // Get QVTKOpenGLWindow `widget` instance as a QWidget instance and add it
 *  // to the parent widget.
 *  QWidget* container =
 *    QWidget::createWindowContainer(qWindow, parent);
 *
 *  // Set flag to propagate Mouse events to the parent
 *  container->setAttribute(Qt::WA_TransparentForMouseEvents);
 *
 * @endcode
 *
 * QVTKOpenGLWidget provides a wrapper around QVTKOpenGLWindow to manipulate a
 * QVTKOpenGLWindow object as a QWidget instance, it is recommended to use
 * QVTKOpenGLWidget instead of using this class directly.
 *
 * QVTKOpenGLWindow is targeted for Qt version 5.9 and above.
 *
 * @sa QVTKOpenGLWidget QVTKOpenGLNativeWidget QVTKWidget2
 */
class VTKGUISUPPORTQT_EXPORT QVTKOpenGLWindow : public QOpenGLWindow
{
  Q_OBJECT
  typedef QOpenGLWindow Superclass;
public:
  QVTKOpenGLWindow();

  QVTKOpenGLWindow(vtkGenericOpenGLRenderWindow* w,
    QOpenGLContext *shareContext = QOpenGLContext::currentContext(),
    UpdateBehavior updateBehavior = NoPartialUpdate,
    QWindow *parent = Q_NULLPTR);

  QVTKOpenGLWindow(QOpenGLContext *shareContext,
    UpdateBehavior updateBehavior = NoPartialUpdate,
    QWindow *parent = Q_NULLPTR);

  ~QVTKOpenGLWindow();

  /**
   * Set the VTK render window used for rendering.
   */
  virtual void SetRenderWindow(vtkGenericOpenGLRenderWindow*);

  /**
   * Get the VTK render window used for rendering.
   */
  virtual vtkGenericOpenGLRenderWindow* GetRenderWindow();

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
   * Get the QEvent to VTK events translator.
   */
  virtual QVTKInteractorAdapter* GetInteractorAdapter();

  /**
   * Enable or disable support for HiDPI displays.
   */
  virtual void setEnableHiDPI(bool enable);

  /**
   * Process a QEvent and send it to the internal render window interactor
   * returns whether the event was recognized and processed
   */
  bool ProcessEvent(QEvent* e);

signals:
  /**
   * Signal emitted when a QMouseEvent has been receive, with the corresponding
   * event as argument.
   */
  void windowEvent(QEvent* e);

public slots:
  /**
   * slot to make this vtk render window current
   */
  virtual void MakeCurrent();

  /**
   * slot called when vtk wants to know if the context is current
   */
  virtual void IsCurrent(vtkObject* caller, unsigned long vtk_event,
    void* client_data, void* call_data);

  /**
   * slot called when vtk wants to frame the window
   */
  virtual void Frame();

  /**
   * slot called when vtk wants to start the render
   */
  virtual void Start();

  /**
   * slot called when vtk wants to end the render
   */
  virtual void End() {}

  /**
   * slot called when vtk wants to know if a window is direct
   */
  virtual void IsDirect(vtkObject* caller, unsigned long vtk_event,
    void* client_data, void* call_data);

  /**
   * slot called when vtk wants to know if a window supports OpenGL
   */
  virtual void SupportsOpenGL(vtkObject* caller, unsigned long vtk_event,
    void* client_data, void* call_data);

  /**
   * slot called when the stereo type of the render window changed
   */
  virtual void UpdateStereoType(vtkObject* caller, unsigned long vtk_event,
    void* client_data, void* call_data);

  /*
   * slot to change the cursor
   */
  virtual void ChangeCursor(vtkObject* caller, unsigned long vtk_event,
    void* client_data, void* call_data);

  /**
   * slot to process events coming from the widget containing this window
   */
  virtual void widgetEvent(QEvent* e);

  /**
   * Returns true if the internal QOpenGLWindow's is valid, i.e. if OpenGL
   * resources, like the context, have been successfully initialized.
   */
  virtual bool isValid();

  /**
   * Returns true if the internal OpenGL contect is the actual current
   * OpenGL context, false otherwise.
   */
  virtual bool isCurrent();

private slots:
  /**
   * slot called when vtk resized the render window
   */
  virtual void ResizeToVTKWindow();

protected:
  /**
   * Override event handler
   */
  virtual void initializeGL();
  virtual void paintGL();
  virtual void moveEvent(QMoveEvent* event);
  virtual bool event(QEvent* e);
  virtual void mousePressEvent(QMouseEvent* event);
  virtual void mouseMoveEvent(QMouseEvent* event);
  virtual void mouseReleaseEvent(QMouseEvent* event);
  virtual void mouseDoubleClickEvent(QMouseEvent* event);
  virtual void keyPressEvent(QKeyEvent* event);
  virtual void keyReleaseEvent(QKeyEvent* event);
  virtual void enterEvent(QEvent*);
  virtual void leaveEvent(QEvent*);
  virtual void wheelEvent(QWheelEvent*);

private:
  /**
   * internal VTK render window
   */
  vtkSmartPointer<vtkGenericOpenGLRenderWindow> RenderWindow;

  bool EnableHiDPI;
  int OriginalDPI;

  /**
   * interaction binding
   */
  QVTKInteractorAdapter* IrenAdapter;
  vtkSmartPointer<vtkEventQtSlotConnect> EventSlotConnector;

  QOffscreenSurface* OffscreenSurface;
};

#endif // QVTKOpenGLWindow_H
