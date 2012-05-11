

#include "metrics.hpp"
#include <qgl.h>

class Plotter2D : public QGLWidget
{
public:
  Plotter2D( QWidget* parent, const char* name)
    : QGLWidget(parent,name), metric(0) {}
  ~Plotter2D()
  {
    if(metric)
      delete metric;
  }

  void set_metric(Metric* met) 
  { 
    if(metric)
      delete metric;
    metric = met; 
    resizeGL(width(), height());
    metric->generate_plot(); 
    connect(metric, SIGNAL(current_val_changed()), this, SLOT(updateGL()));
    connect(metric, SIGNAL(redraw()), this, SLOT(updateGL()));
    updateGL(); 
  }
  Metric* get_metric() { return metric; }

protected:
  
  void initializeGL()
  {
    qglClearColor(black);
  }

  void resizeGL(int w, int h)
  {
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    if(metric)
      glOrtho(metric->x_range_min(), metric->x_range_max(), 
        metric->y_range_min(), metric->y_range_max(),
	metric->z_range_min(), metric->z_range_max());
    glViewport(0,0,w,h);
    glMatrixMode(GL_MODELVIEW);
  }

  void paintGL()
  {
    glClear(GL_COLOR_BUFFER_BIT);
    if(metric)
      metric->draw(width(), height());
  }    

  void mouseMoveEvent (QMouseEvent* e)
  {
    e->accept();
    if(metric)
      metric->mouseEvent(e, width(), height());
  }
  void mousePressEvent (QMouseEvent* e)
  {
    e->accept();
    if(metric)
      metric->mouseEvent(e, width(), height(), true);
  }
  void mouseReleaseEvent (QMouseEvent* e)
  {
    e->accept();
    if(metric)
      metric->mouseEvent(e, width(), height());
  }

private:
  Metric* metric;

};


