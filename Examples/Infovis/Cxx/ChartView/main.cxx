/*-------------------------------------------------------------------------
  Copyright 2009 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

// QT includes
#include <QApplication>
#include <QCleanlooksStyle>
#include "ChartView.h"

extern int qInitResources_icons();

int main( int argc, char** argv )
{
  
  // QT Stuff
  QApplication app( argc, argv );

  QApplication::setStyle(new QCleanlooksStyle);
  
  qInitResources_icons();
  
  ChartView myChartView;
  myChartView.show();
  
  return app.exec();
}
