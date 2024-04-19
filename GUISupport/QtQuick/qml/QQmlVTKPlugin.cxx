// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// this class is deprecated, don't warn about deprecated classes it uses
#define VTK_DEPRECATION_LEVEL 0
// vtk includes
#include "QQmlVTKPlugin.h"

#include "QQuickVTKInteractiveWidget.h"
#include "QQuickVTKRenderItem.h"
#include "QQuickVTKRenderWindow.h"
#include "vtkVersion.h"

// Qt includes
#include <QQmlEngine>

//-------------------------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
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
VTK_ABI_NAMESPACE_END
