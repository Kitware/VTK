/*=========================================================================

  Program:   Visualization Toolkit
  Module:    QQuickVTKRenderItem.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class QQuickVTKRenderItem
 * @brief [QQuickItem] subclass to render a VTK scene in a QtQuick/QML application.
 *
 * QQuickVTKRenderItem extends [QQuickItem] so that a VTK visualization pipeline can be rendererd
 * within the rect of the item.
 *
 * This item is exported to the QML layer via the QQmlVTKPlugin under the module VTK. It is
 * registered as a type \b VTKRenderItem. The QQuickVTKRenderItem manages a vtkRenderer internally
 * that is rendered as a viewport inside the render window provided by QQuickVTKRenderWindow.
 *
 * Typical usage for QQuickVTKRenderItem in a Qml application is as follows:
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
 *  }
 * @endcode
 *
 * The corresponding C++ code that sets up the VTK pipeline would look like the following:
 *
 * @code
 *   int main(int argc, char* argv[])
 *   {
 *     QQuickVTKRenderWindow::setupGraphicsBackend();
 *     QGuiApplication app(argc, argv);
 *
 *     QQmlApplicaQtionEngine engine;
 *     engine.load(QUrl("qrc:///<qmlfile>.qml"));
 *
 *     QObject* topLevel = engine.rootObjects().value(0);
 *     QQuickWindow* window = qobject_cast<QQuickWindow*>(topLevel);
 *
 *     window->show();
 *
 *     // Fetch the QQuick window using the standard object name set up in the constructor
 *     QQuickVTKRenderItem* qquickvtkItem = topLevel->findChild<QQuickVTKRenderItem*>("ConeView");
 *
 *     // Create a cone pipeline and add it to the view
 *     vtkNew<vtkActor> actor;
 *     vtkNew<vtkPolyDataMapper> mapper;
 *     vtkNew<vtkConeSource> cone;
 *     mapper->SetInputConnection(cone->GetOutputPort());
 *     actor->SetMapper(mapper);
 *     qquickvtkItem->renderer()->AddActor(actor);
 *     qquickvtkItem->renderer()->ResetCamera();
 *     qquickvtkItem->renderer()->SetBackground(0.5, 0.5, 0.7);
 *     qquickvtkItem->renderer()->SetBackground2(0.7, 0.7, 0.7);
 *     qquickvtkItem->renderer()->SetGradientBackground(true);
 *     qquickvtkItem->update();
 *     app.exec();
 *   }
 * @endcode
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
 * ## Interactive vtk widgets
 *
 * QQuickVTKRenderItem also supports interactive vtk widgets with QtQuick's threaded render loop via
 * the QQuickVTKInteractiveWidget class.
 *
 * [QQuickItem]: https://doc.qt.io/qt-5/qquickitem.html
 * [QQuickWindow]: https://doc.qt.io/qt-5/qquickwindow.html
 */

#ifndef QQuickVTKRenderItem_h
#define QQuickVTKRenderItem_h

// Qt includes
#include <QOpenGLFunctions> // For QOpenGLFunctions
#include <QQuickItem>

// vtk includes
#include "QQuickVTKRenderWindow.h" // For QQuickVTKRenderWindow
#include "vtkNew.h"                // For vtkNew
#include "vtkRenderer.h"           // For vtkRenderer

#include "vtkGUISupportQtQuickModule.h" // for export macro

// Forward declarations
class QHoverEvent;
class QKeyEvent;
class QMouseEvent;
class QQuickVTKInteractiveWidget;
class vtkImageData;

class VTKGUISUPPORTQTQUICK_EXPORT QQuickVTKRenderItem
  : public QQuickItem
  , protected QOpenGLFunctions
{
  Q_OBJECT
  typedef QQuickItem Superclass;

  Q_PROPERTY(QQuickVTKRenderWindow* renderWindow READ renderWindow WRITE setRenderWindow)

public:
  QQuickVTKRenderItem(QQuickItem* parent = nullptr);
  ~QQuickVTKRenderItem() = default;

  ///@{
  /**
   * Set/Get the render window for the item
   */
  QQuickVTKRenderWindow* renderWindow() const;
  virtual void setRenderWindow(QQuickVTKRenderWindow* w);
  ///@}

  /**
   * Get access to the renderer
   */
  vtkRenderer* renderer() const;

  /**
   * Capture a screenshot of the view
   *
   * \returns Image data containing the view capture.
   * \sa QQuickVTKRenderWindow::captureScreenshot
   */
  virtual vtkSmartPointer<vtkImageData> captureScreenshot();

  ///@{
  /**
   * Add/Remove widgets to/from the view
   */
  virtual void addWidget(QQuickVTKInteractiveWidget* w);
  virtual void removeWidget(QQuickVTKInteractiveWidget* w);
  ///@}

  ///@{
  /**
   * Get/Remove widgets from the view by their object name
   */
  virtual QQuickVTKInteractiveWidget* widgetByName(QString name) const;
  virtual void removeWidgetByName(QString name);
  ///@}

public Q_SLOTS:
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
   * Initialize the graphics resources required for this render item.
   *
   * This method is called on the QtQuick render thread at the beforeRenderPassRecording stage of
   * the scenegraph render loop.
   */
  virtual void init();

  /**
   * This is the function called on the QtQuick render thread right before the scenegraph is
   * rendered. This is the stage where all the vtk rendering is performed. Applications would rarely
   * need to override this method.
   *
   * \note This method is called at the beforeRendering stage of the QtQuick scenegraph. All the
   * QtQuick element rendering is stacked visually above the vtk rendering.
   */
  virtual void paint();

  /**
   * This is the function called on the QtQuick render thread when the scenegraph is invalidated.
   * This is where all graphics resources allocated by vtk are released.
   */
  virtual void cleanup();

protected Q_SLOTS:
  virtual void handleWindowChanged(QQuickWindow* w);

protected:
  // Helper members
  QQuickVTKRenderWindow* m_renderWindow = nullptr;
  vtkNew<vtkRenderer> m_renderer;

  QVector<QQuickVTKInteractiveWidget*> m_widgets;

  /**
   * Set the viewport for this item
   */
  virtual void setViewport(const QRectF& rect);

  // Event handlers
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
  void geometryChanged(const QRectF& newGeometry, const QRectF& oldGeometry) override;
#else
  void geometryChange(const QRectF& newGeometry, const QRectF& oldGeometry) override;
#endif
  bool event(QEvent* ev) override;

private:
  QQuickVTKRenderItem(const QQuickVTKRenderItem&) = delete;
  void operator=(const QQuickVTKRenderItem) = delete;
};

#endif // QQuickVTKRenderItem_h
