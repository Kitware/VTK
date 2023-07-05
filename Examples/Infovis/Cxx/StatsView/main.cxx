// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2007 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
// QT includes
#include "StatsView.h"
#include <QApplication>

extern int qInitResources_icons();

int main(int argc, char** argv)
{

  // QT Stuff
  QApplication app(argc, argv);

  QApplication::setStyle("fusion");

  qInitResources_icons();

  StatsView myStatsView;
  myStatsView.show();

  return app.exec();
}
