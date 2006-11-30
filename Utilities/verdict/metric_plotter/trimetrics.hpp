
#ifndef PLOTTER_TRI_METRICS_HPP
#define PLOTTER_TRI_METRICS_HPP


#include "metrics.hpp"
#include <qgl.h>

class Metric2DTri : public Metric2D
{
public:
  Metric2DTri( VerdictFunction func)
    : Metric2D(func, 4,4) {}

  ~Metric2DTri(){ glDeleteLists(drawingList,1); }

  double x_range_min() { return -xRange; }
  double x_range_max() { return xRange; }
  double y_range_min() { return 0.; }
  double y_range_max() { return yRange; }
  void draw(int /*xwin*/, int /*ywin*/ );

  void mouseEvent(QMouseEvent *e, int xmax, int ymax, bool);

  void generate_plot();

};

#endif


