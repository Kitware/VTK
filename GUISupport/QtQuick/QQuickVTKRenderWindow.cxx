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
void QQuickVTKRenderWindow::handleWindowChanged(QQuickWindow* w) {}
