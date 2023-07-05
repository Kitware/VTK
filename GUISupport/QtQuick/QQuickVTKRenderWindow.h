// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class QQuickVTKRenderWindow
 * @brief [QQuickItem] subclass that manages the vtkRenderWindow and, in
 * turn, the OpenGL context of the QML application
 *
 * QQuickVTKRenderWindow extends [QQuickItem] in a way that allows for VTK to get a handle to, and
 * draw inside of the QtQuick scenegraph, using OpenGL draw calls.
 *
 * This item is exported to the QML layer via the QQmlVTKPlugin under the module VTK. It is
 * registered as a type \b VTKRenderWindow. Since, this class is intended to manage an OpenGL
 * context in the window, a single instance would be needed for most QML applications.
 *
 * Typical usage for QQuickVTKRenderWindow in a Qml application is as follows:
 *
 * @code
 *  // import related modules
 *  import QtQuick 2.15
 *  import QtQuick.Controls 2.15
 *  import QtQuick.Window 2.15
 *
 *  // import the VTK module
 *  import VTK 9.0
 *
 *  // window containing the application
 *  ApplicationWindow {
 *    // title of the application
 *    title: qsTr("VTK QtQuick App")
 *    width: 400
 *    height: 400
 *    color: palette.window
 *
 *    SystemPalette {
 *      id: palette
 *      colorGroup: SystemPalette.Active
 *    }
 *
 *    // Instantiate the vtk render window
 *    VTKRenderWindow {
 *      id: vtkwindow
 *      width: 400
 *      height: 400
 *    }
 *
 *    // add one or more vtk render items
 *    VTKRenderItem {
 *      objectName: "ConeView"
 *      x: 200
 *      y: 200
 *      width: 200
 *      height: 200
 *      // Provide the handle to the render window
 *      renderWindow: vtkwindow
 *    }
 *    VTKRenderItem {
 *      objectName: "VolumeView"
 *      x: 0
 *      y: 0
 *      width: 200
 *      height: 200
 *      // Provide the handle to the render window
 *      renderWindow: vtkwindow
 *    }
 *  }
 * @endcode
 *
 * To ensure that the graphics backend set up by QtQuick matches that expected by VTK, use the
 * method QQuickVTKRenderWindow::setupGraphicsBackend() before a QApplication/QGuiApplication is
 * instantiated in the main method of the application.
 *
 * @code
 * int main(int argc, char* argv[])
 * {
 *   // Setup the graphics backend
 *   QQuickVTKRenderWindow::setupGraphicsBackend();
 *   QGuiApplication app(argc, argv);
 *   ...
 *   return EXIT_SUCCESS;
 * }
 * @endcode
 *
 * The VTK pipeline can be then set up for each \b VTKRenderItem in the C++ code.
 *
 * ## QtQuick scenegraph and threaded render loop
 *
 * QtQuick/QML scenegraph rendering is done via private API inside the [QQuickWindow] class. For
 * details on QtQuick's render loop, see [QtQuick Scenegraph Rendering](
 * https://doc.qt.io/qt-6/qtquick-visualcanvas-scenegraph.html#scene-graph-and-rendering).
 * Qt automatically decides between a threaded and basic render loop for most applications.
 * QQuickVTKRenderWindow and QQuickVTKRenderItem support both these variants of the QtQuick render
 * loop.
 *
 * When the scenegraph render loop is threaded, i.e. there is a dedicated rendering thread, vtk
 * sticks to doing all rendering on this render thread. This means that all the vtk classes,
 * pipelines etc. can be set up on the main thread but vtkRenderWindow::Render should only be
 * invoked on the render thread. Care must be taken not to call Render on the main thread because
 * the OpenGL context would not be valid on the main thread.
 *
 * [QQuickItem]: https://doc.qt.io/qt-5/qquickitem.html
 * [QQuickWindow]: https://doc.qt.io/qt-5/qquickwindow.html
 */

#ifndef QQuickVTKRenderWindow_h
#define QQuickVTKRenderWindow_h

// vtk includes
#include "vtkDeprecation.h"
#include "vtkSmartPointer.h" // For vtkSmartPointer

// Qt includes
#include <QOpenGLFunctions> // For QOpenGLFunctions
#include <QPointer>         // For QPointer
#include <QQuickItem>

#include "vtkGUISupportQtQuickModule.h" // for export macro

// Qt Forward declarations
class QEvent;
class QQuickWindow;
class QWheelEvent;

VTK_ABI_NAMESPACE_BEGIN

// VTK Forward declarations
class QQuickVTKInteractorAdapter;
class vtkGenericOpenGLRenderWindow;
class vtkImageData;
class vtkRenderWindow;
class vtkRenderer;
class vtkWindowToImageFilter;

class VTK_DEPRECATED_IN_9_3_0(
  "Use QQuickVTKItem instead") VTKGUISUPPORTQTQUICK_EXPORT QQuickVTKRenderWindow
  : public QQuickItem
  , protected QOpenGLFunctions
{
  Q_OBJECT
  typedef QQuickItem Superclass;

public:
  /**
   * Constructor
   * Creates a QQuickVTKRenderWindow with:
   *  - a vtkGenericOpenGLRenderWindow to manage the OpenGL context
   *  - an interactor adapter to forward Qt events to vtk's interactor
   */
  QQuickVTKRenderWindow(QQuickItem* parent = nullptr);

  /**
   * Destructor
   */
  ~QQuickVTKRenderWindow() override;

  /**
   * Set up the graphics surface format and api.
   *
   * This method sets the graphics API to OpenGLRhi and sets up the surface format for intermixed
   * VTK and QtQuick rendering.
   * Use this method before instantiating a QApplication/QGuiApplication in a QtQuick/QML app with
   * a VTK render view like QQuickVTKRenderItem.
   */
  static void setupGraphicsBackend();

  ///@{
  /**
   * Set/Get the vtkRenderWindow for the view.
   * Note that this render window should be of type vtkGenericOpenGLRenderWindow. This is necessary
   * since that would allow vtk's opengl draw calls to work seamlessly inside the QtQuick created
   * scenegraph and OpenGL context.
   *
   * By default, a vtkGenericOpenGLRenderWindow is created and set on this item at construction
   * time.
   */
  virtual void setRenderWindow(vtkRenderWindow* renWin);
  virtual void setRenderWindow(vtkGenericOpenGLRenderWindow* renWin);
  vtkRenderWindow* renderWindow() const;
  ///@}

  /**
   * Map a Qt item rect to viewport coordinates
   */
  virtual void mapToViewport(const QRectF& rect, double viewport[4]);

  /**
   * Get access to the interactor adapter
   */
  QPointer<QQuickVTKInteractorAdapter> interactorAdapter() const;

  ///@{
  /**
   * Capture a screenshot of the window
   *
   * \param viewport area to capture.
   * \returns Image data containing the window capture.
   * \note This triggers a scenegraph update to capture the render window view.
   */
  virtual vtkSmartPointer<vtkImageData> captureScreenshot();
  virtual vtkSmartPointer<vtkImageData> captureScreenshot(double* viewport);
  ///@}

  /**
   * Get whether the render window is initialized
   * Used internally to determine if the OpenGL context, QQuickWindow, children items and viewports
   * have been initialized.
   */
  virtual bool isInitialized() const;

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  /**
   * This is the function called on the QtQuick render thread before the scenegraph state
   * is synchronized. This is where most of the pipeline updates, camera manipulations, etc. and
   * other pre-render steps can be performed.
   *
   * \note At the time of this method execution, the GUI thread is blocked. Hence, it is safe to
   * perform state synchronization between the GUI elements and the VTK classes here.
   */
  virtual void sync();

  /**
   * Initialize the VTK render window for OpenGL based on the context created by QtQuick
   *
   * \note This method is called at the beforeRenderPassRecording stage of the QtQuick scenegraph.
   * All the QtQuick element rendering is stacked visually above the vtk rendering.
   */
  virtual void init();

  /**
   * This is the function called on the QtQuick render thread right before the scenegraph is
   * rendered. This is the stage where all the vtk rendering is performed. Applications would rarely
   * need to override this method.
   *
   * \note This method is called at the beforeRenderPassRecording stage of the QtQuick scenegraph.
   * All the QtQuick element rendering is stacked visually above the vtk rendering.
   */
  virtual void paint();

  /**
   * This is the function called on the QtQuick render thread when the scenegraph is invalidated.
   * This is where all graphics resources allocated by vtk are released.
   */
  virtual void cleanup();

  /**
   * Convenience method that schedules a scenegraph update and waits for the update.
   * \sa render()
   */
  virtual void renderNow();

  /**
   * Schedule a scenegraph update
   *
   * \note Since this schedules a scenegraph update, it does not guarantee that the scene will be
   * updated after this call.
   * \sa renderNow()
   */
  virtual void render();

protected Q_SLOTS:
  virtual void handleWindowChanged(QQuickWindow* w);

protected: // NOLINT(readability-redundant-access-specifiers)
  QPointer<QQuickVTKInteractorAdapter> m_interactorAdapter;
  vtkSmartPointer<vtkGenericOpenGLRenderWindow> m_renderWindow;
  bool m_initialized = false;

  // Screenshot stuff
  bool m_screenshotScheduled = false;
  vtkNew<vtkWindowToImageFilter> m_screenshotFilter;
  vtkNew<vtkRenderer> m_dummyRenderer;

  // Event handlers
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
  void geometryChanged(const QRectF& newGeometry, const QRectF& oldGeometry) override;
#else
  void geometryChange(const QRectF& newGeometry, const QRectF& oldGeometry) override;
#endif

  /**
   * Check the scenegraph backend and graphics API being used.
   */
  bool checkGraphicsBackend();

private:
  QQuickVTKRenderWindow(const QQuickVTKRenderWindow&) = delete;
  void operator=(const QQuickVTKRenderWindow) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // QQuickVTKRenderWindow_h
