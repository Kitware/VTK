
#include <qapplication.h>

#include "verdict.h"
#include "metrics_plotter.h" 

int main(int argc, char** argv)
{
  QApplication app(argc, argv);

  PlotterForm plotter(0, "Metrics Plotter");

  app.setMainWidget(&plotter);
  plotter.show();
  return app.exec();
}

