/*=========================================================================

  Program:   Visualization Toolkit
  Module:    QQuickVTKRenderWindow.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class QQuickVTKRenderWindow
 * @brief QQuickItem subclass to render a VTK scene in a QtQuick application.
 */

#ifndef QQuickVTKRenderWindow_h
#define QQuickVTKRenderWindow_h

// vtk includes
#include "vtkSmartPointer.h" // For vtkSmartPointer

// Qt includes
#include <QOpenGLFunctions> // For QOpenGLFunctions
#include <QPointer>         // For QPointer
#include <QQuickItem>

#include "vtkGUISupportQtQuickModule.h" // for export macro

// Forward declarations
class QEvent;
class QHoverEvent;
class QKeyEvent;
class QMouseEvent;
class QQuickVTKInteractorAdapter;
class QQuickWindow;
class QWheelEvent;
class vtkGenericOpenGLRenderWindow;
class vtkRenderWindow;

/**
 * \class QQuickVTKRenderWindow
 * \brief
 */
class VTKGUISUPPORTQTQUICK_EXPORT QQuickVTKRenderWindow
  : public QQuickItem
  , protected QOpenGLFunctions
{
  Q_OBJECT
  typedef QQuickItem Superclass;

public:
  QQuickVTKRenderWindow(QQuickItem* parent = 0);
  ~QQuickVTKRenderWindow();

  //@{
  /**
   * Set/Get the render window for the item.
   */
  void setRenderWindow(vtkRenderWindow* renWin);
  void setRenderWindow(vtkGenericOpenGLRenderWindow* renWin);
  vtkRenderWindow* renderWindow() const;
  //@}

  /**
   * Map a Qt item rect to viewport coordinates
   */
  virtual void mapToViewport(const QRectF& rect, double viewport[4]);

  /**
   * Get access to the interactor adapter
   */
  QPointer<QQuickVTKInteractorAdapter> interactorAdapter() const;

public Q_SLOTS:
  virtual void sync();
  virtual void paint();
  virtual void cleanup();

protected Q_SLOTS:
  virtual void handleWindowChanged(QQuickWindow* w);

protected:
  QPointer<QQuickVTKInteractorAdapter> m_interactorAdapter;
  vtkSmartPointer<vtkGenericOpenGLRenderWindow> m_renderWindow;
  bool m_initialized = false;

  // Event handlers
  void geometryChanged(const QRectF& newGeometry, const QRectF& oldGeometry) override;
  //  void mousePressEvent(QMouseEvent* event) override;
  //  void mouseMoveEvent(QMouseEvent* event) override;
  //  void mouseReleaseEvent(QMouseEvent* event) override;
  //  void wheelEvent(QWheelEvent*) override;
  //  void hoverMoveEvent(QHoverEvent* event) override;
  //
  //  void keyPressEvent(QKeyEvent* event) override;
  //  void keyReleaseEvent(QKeyEvent* event) override;
  //
  //  void focusInEvent(QFocusEvent*) override;
  //  void focusOutEvent(QFocusEvent*) override;

  bool event(QEvent* e) override;

private:
  Q_DISABLE_COPY(QQuickVTKRenderWindow)
};

#endif // QQuickVTKRenderWindow_h
