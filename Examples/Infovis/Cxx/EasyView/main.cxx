/*
 * Copyright 2007 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */
// QT includes
#include <QApplication>
#if QT_VERSION < 0x050000
#include <QCleanlooksStyle>
#endif
#include "EasyView.h"

extern int qInitResources_icons();

int main( int argc, char** argv )
{

  // QT Stuff
  QApplication app( argc, argv );

#if QT_VERSION >= 0x050000
  QApplication::setStyle("fusion");
#else
  QApplication::setStyle(new QCleanlooksStyle);
#endif

  qInitResources_icons();

  EasyView myEasyView;
  myEasyView.show();

  return app.exec();
}
