/*
 * Copyright 2007 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */
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
