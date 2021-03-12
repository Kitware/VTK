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
#include "vtkNew.h" // For vtkNew

#include "vtkGUISupportQtQuickModule.h" // for export macro

// Forward declarations
class QQuickVTKRenderWindow;
class vtkRenderer;

class VTKGUISUPPORTQTQUICK_EXPORT QQuickVTKRenderItem
  : public QQuickItem
  , protected QOpenGLFunctions
{
  Q_OBJECT
  typedef QQuickItem Superclass;

  Q_PROPERTY(QQuickVTKRenderWindow* renderWindow READ renderWindow WRITE setRenderWindow)

public:
  QQuickVTKRenderItem(QQuickItem* parent = 0);
  ~QQuickVTKRenderItem();

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

  /**
   * Set the viewport for this item
   */
  virtual void setViewport(const QRectF& rect);

  // Event handlers
  void geometryChanged(const QRectF& newGeometry, const QRectF& oldGeometry) override;
  void mousePressEvent(QMouseEvent* event) override;
  void mouseMoveEvent(QMouseEvent* event) override;
  void mouseReleaseEvent(QMouseEvent* event) override;
  void wheelEvent(QWheelEvent*) override;
  void hoverMoveEvent(QHoverEvent* event) override;

  void keyPressEvent(QKeyEvent* event) override;
  void keyReleaseEvent(QKeyEvent* event) override;

  void focusInEvent(QFocusEvent*) override;
  void focusOutEvent(QFocusEvent*) override;

  bool event(QEvent* e) override;

private:
  QQuickVTKRenderItem(const QQuickVTKRenderItem&) = delete;
  void operator=(const QQuickVTKRenderItem) = delete;
};

#endif // QQuickVTKRenderItem_h
