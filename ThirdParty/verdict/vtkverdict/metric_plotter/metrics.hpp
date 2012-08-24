
#ifndef PLOTTER_METRICS_HPP
#define PLOTTER_METRICS_HPP

#include "verdict.h"
#include <qobject.h>
#include <qtimer.h>
#include <math.h>

class QMouseEvent;

#define NUM_POINTS 20000
#define NUM_Z_PLANES 20


class Metric : public QObject
{
  Q_OBJECT

public:
  Metric( VerdictFunction fun, int xrange, int yrange) 
    : func(fun), xRange(xrange), yRange(yrange) {}
  virtual ~Metric(){}
  struct metric_funcs
  {
    const char* name;
    VerdictFunction func;
  };
  struct element_types
  {
    const char* name;
    const metric_funcs* functions;
  };

  static const element_types ElementTypes[];
  static const metric_funcs tri_metric_funcs[];
  static const metric_funcs quad_metric_funcs[];
  static const metric_funcs tet_metric_funcs[];
  static void set_color_factor(int val) { colorFactor = val; }
  static double curr_metric_val( ) { return currMetricVal; }
  virtual void mouseEvent(QMouseEvent* e, int xmax, int ymax, bool flag = false) = 0;
  
  virtual void generate_plot() = 0;
  virtual void draw(int,int) = 0;
  virtual float setZVal(int){ return 0.; }
  void set_metric(VerdictFunction fun) { func = fun; }

  virtual double x_range_min() = 0;
  virtual double x_range_max() = 0;
  virtual double y_range_min() = 0;
  virtual double y_range_max() = 0;
  virtual double z_range_min() { return -1; }
  virtual double z_range_max() { return 1; }

protected:
  static int colorFactor;
  VerdictFunction func;
  static double currMetricVal;

  double xRange;
  double yRange;

signals:

  void current_val_changed( );
  void redraw();
  
};
  

class Metric2D : public Metric
{
public:
  Metric2D(VerdictFunction fun, int x_range, int y_range)
  : Metric(fun, x_range, y_range), drawingList(0),
    currX(0), currY(0){}
  virtual ~Metric2D(){}

protected:
  int drawingList;
  double currX;
  double currY;
};


class Metric3D : public Metric
{

  Q_OBJECT
  
public:
  Metric3D(VerdictFunction fun, int x_range, int y_range)
    : Metric(fun, x_range, y_range),
      drawingListBase(0), currZPlane(0), currX(0), currY(0), 
      zPlane(0), animator(0) {}

  virtual ~Metric3D(){}

  double z_range_min() { return -xRange; }
  double z_range_max() { return xRange; }
  
  float setZVal(int val)
  {
    currZPlane = val; 
    emit redraw();
    return (float)val/(float)NUM_Z_PLANES * 2 * xRange - xRange;
  }

public slots:
  void animateStep() 
  { 
    currZPlane++; 
    if(currZPlane >= NUM_Z_PLANES) 
      currZPlane = 0;
    emit redraw();
  }

protected:
  int drawingListBase;
  int currZPlane;
  double currX;
  double currY;
  double zPlane;
  QTimer* animator;
};


#endif

