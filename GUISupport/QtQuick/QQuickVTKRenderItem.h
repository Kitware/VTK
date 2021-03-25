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
 * @brief QQuickItem subclass to render a VTK scene in a QtQuick application.
 */

#ifndef QQuickVTKRenderItem_h
#define QQuickVTKRenderItem_h

// Qt includes
#include <QOpenGLFunctions> // For QOpenGLFunctions
#include <QQuickItem>

// vtk includes
#include "vtkNew.h"      // For vtkNew
#include "vtkRenderer.h" // For vtkRenderer

#include "vtkGUISupportQtQuickModule.h" // for export macro

// Forward declarations
class QHoverEvent;
class QKeyEvent;
class QMouseEvent;
class QQuickVTKInteractiveWidget;
class QQuickVTKRenderWindow;
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

  //@{
  /**
   * Set/Get the render window for the item
   */
  QQuickVTKRenderWindow* renderWindow() const;
  virtual void setRenderWindow(QQuickVTKRenderWindow* w);
  //@}

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

  //@{
  /**
   * Add/Remove widgets to/from the view
   */
  virtual void addWidget(QQuickVTKInteractiveWidget* w);
  virtual void removeWidget(QQuickVTKInteractiveWidget* w);
  //@}

  //@{
  /**
   * Get/Remove widgets from the view by their object name
   */
  virtual QQuickVTKInteractiveWidget* widgetByName(QString name) const;
  virtual void removeWidgetByName(QString name);
  //@}

public Q_SLOTS:
  virtual void sync();
  virtual void paint();
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
  void geometryChanged(const QRectF& newGeometry, const QRectF& oldGeometry) override;
  bool event(QEvent* ev) override;

private:
  QQuickVTKRenderItem(const QQuickVTKRenderItem&) = delete;
  void operator=(const QQuickVTKRenderItem) = delete;
};

#endif // QQuickVTKRenderItem_h
