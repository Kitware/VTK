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

#include "QQuickVTKRenderWindow.h"
#include "vtkVersion.h"

// Qt includes
#include <QQmlEngine>

//-------------------------------------------------------------------------------------------------
void QQmlVTKPlugin::registerTypes(const char* uri)
{
  Q_ASSERT(uri == QLatin1String("VTK"));

  int major = vtkVersion::GetVTKMajorVersion();
  int minor = vtkVersion::GetVTKMinorVersion();

  // Register QML metatypes
  qmlRegisterType<QQuickVTKRenderWindow>(uri, major, minor, "VTKRenderWindow");
}

//-------------------------------------------------------------------------------------------------
void QQmlVTKPlugin::initializeEngine(QQmlEngine* engine, const char* uri)
{
  Q_ASSERT(uri == QLatin1String("VTK"));

  QObject::connect(
    engine, &QQmlEngine::destroyed, this, &QQmlVTKPlugin::cleanup, Qt::DirectConnection);
}

//-------------------------------------------------------------------------------------------------
void QQmlVTKPlugin::cleanup() {}
