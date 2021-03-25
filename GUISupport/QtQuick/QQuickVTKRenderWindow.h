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
 * @brief QQuickItem subclass that manages the vtkRenderWindow and, in turn, the OpenGL context of
 * the QML application
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
class QQuickVTKInteractorAdapter;
class QQuickWindow;
class QWheelEvent;
class vtkGenericOpenGLRenderWindow;
class vtkImageData;
class vtkRenderWindow;
class vtkWindowToImageFilter;

class VTKGUISUPPORTQTQUICK_EXPORT QQuickVTKRenderWindow
  : public QQuickItem
  , protected QOpenGLFunctions
{
  Q_OBJECT
  typedef QQuickItem Superclass;

public:
  QQuickVTKRenderWindow(QQuickItem* parent = nullptr);
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

  //@{
  /**
   * Capture a screenshot of the window
   *
   * \param[in] Viewport area to capture.
   * \returns Image data containing the window capture.
   * \note This triggers a scenegraph update to capture the render window view.
   */
  virtual vtkSmartPointer<vtkImageData> captureScreenshot();
  virtual vtkSmartPointer<vtkImageData> captureScreenshot(double* viewport);
  //@}

  /**
   * Get whether the render window is initialized
   * Used internally to determine if the OpenGL context, QQuickWindow, children items and viewports
   * have been initialized.
   */
  virtual bool isInitialized() const;

public Q_SLOTS:
  virtual void sync();
  virtual void paint();
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

protected:
  QPointer<QQuickVTKInteractorAdapter> m_interactorAdapter;
  vtkSmartPointer<vtkGenericOpenGLRenderWindow> m_renderWindow;
  bool m_initialized = false;

  // Screenshot stuff
  bool m_screenshotScheduled = false;
  vtkNew<vtkWindowToImageFilter> m_screenshotFilter;

  // Event handlers
  void geometryChanged(const QRectF& newGeometry, const QRectF& oldGeometry) override;

private:
  QQuickVTKRenderWindow(const QQuickVTKRenderWindow&) = delete;
  void operator=(const QQuickVTKRenderWindow) = delete;
};

#endif // QQuickVTKRenderWindow_h
