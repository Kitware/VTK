// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2007 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
// QT includes
#include <QApplication>
#include <QSurfaceFormat>

#include "CustomLinkView.h"
#include "QVTKOpenGLNativeWidget.h"

extern int qInitResources_icons();

int main(int argc, char** argv)
{
  // needed to ensure appropriate OpenGL context is created for VTK rendering.
  QSurfaceFormat::setDefaultFormat(QVTKOpenGLNativeWidget::defaultFormat());

  // QT Stuff
  QApplication app(argc, argv);

  QApplication::setStyle("fusion");

  qInitResources_icons();

  CustomLinkView myCustomLinkView;
  myCustomLinkView.show();

  return app.exec();
}
