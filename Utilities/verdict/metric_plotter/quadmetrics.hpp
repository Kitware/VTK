
#ifndef PLOTTER_QUAD_METRICS_HPP
#define PLOTTER_QUAD_METRICS_HPP

#include "metrics.hpp"
#include <qgl.h>

class Metric3DQuad : public Metric3D
{
public:
  Metric3DQuad( VerdictFunction func)
    : Metric3D(func, 4,4) {}

  ~Metric3DQuad(){ glDeleteLists(drawingListBase,NUM_Z_PLANES); }

  double x_range_min() { return 0; }
  double x_range_max() { return xRange; }
  double y_range_min() { return 0; }
  double y_range_max() { return yRange; }

  void draw(int /*xwin*/, int /*ywin*/);

  void mouseEvent( QMouseEvent* e, int xmax, int ymax, bool );
  
  void generate_plot();

};


class Metric2DQuad : public Metric2D
{
public:
  Metric2DQuad( VerdictFunction func)
    : Metric2D(func, 4,4){}
  ~Metric2DQuad(){ glDeleteLists(drawingList, 1); }

  double x_range_min() { return 0; }
  double x_range_max() { return xRange; }
  double y_range_min() { return 0; }
  double y_range_max() { return yRange; }
    
  void draw(int /*xwin*/, int /*ywin*/);

  void mouseEvent( QMouseEvent* e, int xmax, int ymax, bool );
  
  void generate_plot();
};

#endif


