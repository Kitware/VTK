/*=========================================================================

  Program:   Visualization Toolkit
  Module:    QQmlVTKPlugin.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// vtk includes
#include "QQmlVTKPlugin.h"

#include "QQuickVTKInteractiveWidget.h"
#include "QQuickVTKRenderItem.h"
#include "QQuickVTKRenderWindow.h"
#include "vtkVersion.h"

// Qt includes
#include <QQmlEngine>

//-------------------------------------------------------------------------------------------------
void QQmlVTKPlugin::registerTypes(const char* uri)
{
  Q_ASSERT(QString::compare(uri, "VTK") == 0);

  int major = vtkVersion::GetVTKMajorVersion();
  int minor = vtkVersion::GetVTKMinorVersion();

  // Register QML metatypes
  qmlRegisterType<QQuickVTKRenderWindow>(uri, major, minor, "VTKRenderWindow");
  qmlRegisterType<QQuickVTKRenderItem>(uri, major, minor, "VTKRenderItem");
  qmlRegisterType<QQuickVTKInteractiveWidget>(uri, major, minor, "VTKWidget");
}

//-------------------------------------------------------------------------------------------------
void QQmlVTKPlugin::initializeEngine(QQmlEngine* engine, const char* uri)
{
  Q_ASSERT(QString::compare(uri, "VTK") == 0);

  QObject::connect(
    engine, &QQmlEngine::destroyed, this, &QQmlVTKPlugin::cleanup, Qt::DirectConnection);
}

//-------------------------------------------------------------------------------------------------
void QQmlVTKPlugin::cleanup() {}
