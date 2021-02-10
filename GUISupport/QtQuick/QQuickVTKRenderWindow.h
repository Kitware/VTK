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
 * @brief QQuickItem subclass to render a VTK scene ina QtQuick application.
 *
 */

#ifndef QQuickVTKRenderWindow_h
#define QQuickVTKRenderWindow_h

// Qt includes
#include <QQuickItem>

#include "vtkGUISupportQtQuickModule.h" // for export macro
#include <QOpenGLFunctions>             // for QOpenGLFunctions

// Forward declarations
class QQuickWindow;

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

public Q_SLOTS:
  virtual void sync();
  virtual void paint();
  virtual void cleanup();

protected Q_SLOTS:
  virtual void handleWindowChanged(QQuickWindow* w);

private:
  Q_DISABLE_COPY(QQuickVTKRenderWindow)
};

#endif // QQuickVTKRenderWindow_h
