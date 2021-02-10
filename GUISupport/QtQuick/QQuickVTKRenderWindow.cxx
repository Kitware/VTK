/*=========================================================================

  Program:   Visualization Toolkit
  Module:    QQuickVTKRenderWindow.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "QQuickVTKRenderWindow.h"

// Qt includes
#include <QQuickWindow>

//-------------------------------------------------------------------------------------------------
QQuickVTKRenderWindow::QQuickVTKRenderWindow(QQuickItem* parent)
  : Superclass(parent)
{
  // Accept mouse events
  setAcceptHoverEvents(true);
  setAcceptedMouseButtons(Qt::AllButtons);

  connect(
    this, SIGNAL(windowChanged(QQuickWindow*)), this, SLOT(handleWindowChanged(QQuickWindow*)));
}

//-------------------------------------------------------------------------------------------------
QQuickVTKRenderWindow::~QQuickVTKRenderWindow() {}

//-------------------------------------------------------------------------------------------------
void QQuickVTKRenderWindow::sync() {}

//-------------------------------------------------------------------------------------------------
void QQuickVTKRenderWindow::paint() {}

//-------------------------------------------------------------------------------------------------
void QQuickVTKRenderWindow::cleanup() {}

//-------------------------------------------------------------------------------------------------
void QQuickVTKRenderWindow::handleWindowChanged(QQuickWindow* w)
{
  if (window())
  {
    QObject::disconnect(
      window(), &QQuickWindow::beforeSynchronizing, this, &QQuickVTKRenderWindow::sync);
    QObject::disconnect(
      window(), &QQuickWindow::beforeRendering, this, &QQuickVTKRenderWindow::paint);
    QObject::disconnect(
      window(), &QQuickWindow::sceneGraphInvalidated, this, &QQuickVTKRenderWindow::cleanup);
  }
  if (w)
  {
    QObject::connect(w, &QQuickWindow::beforeSynchronizing, this, &QQuickVTKRenderWindow::sync,
      Qt::DirectConnection);
    QObject::connect(
      w, &QQuickWindow::beforeRendering, this, &QQuickVTKRenderWindow::paint, Qt::DirectConnection);
    QObject::connect(w, &QQuickWindow::sceneGraphInvalidated, this, &QQuickVTKRenderWindow::cleanup,
      Qt::DirectConnection);
    // Do not clear the scenegraph before the QML rendering
    // to preserve the VTK render
    w->setClearBeforeRendering(false);
    // This allows the cleanup method to be called on the render thread
    w->setPersistentSceneGraph(false);
  }
}
