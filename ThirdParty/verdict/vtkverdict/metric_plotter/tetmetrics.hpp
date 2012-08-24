
#ifndef PLOTTER_TET_METRICS_HPP
#define PLOTTER_TET_METRICS_HPP

#include "metrics.hpp"
#include <qgl.h>

class Metric3DTet : public Metric3D
{
public:
  Metric3DTet( VerdictFunction func)
    : Metric3D(func, 4, 4) {}

  ~Metric3DTet()
  {
    if(drawingListBase) 
      glDeleteLists(drawingListBase,NUM_Z_PLANES); 
  }

  double x_range_min() { return -xRange; }
  double x_range_max() { return xRange; }
  double y_range_min() { return 0; }
  double y_range_max() { return yRange; }

  void draw(int /*xwin*/, int /*ywin*/);

  void mouseEvent( QMouseEvent *e, int xmax, int ymax , bool flag );
  
  void generate_plot();

};

#endif


