
#include "trimetrics.hpp"
#include <math.h>

void Metric2DTri::draw(int /*xwin*/, int /*ywin*/ )
{
  // draw metric information
  glCallList(drawingList);
  // draw moused point
  glBegin(GL_POINTS);
  glPointSize(5.0);
  glColor3f(0.0,0.0,0.0);
  glVertex3f(currX, currY, 0);
  glEnd();
}

void Metric2DTri::mouseEvent(QMouseEvent *e, int xmax, int ymax, bool)
{
  // convert window coords to world coords
  int ywin = ymax - e->y();
  int xwin = e->x();
  double nodes[3][3] = { {-.5,1,0}, {-.5,0,0}, {.5,0,0}};
  nodes[0][0] = 2*xRange*(double)xwin/(double)xmax - xRange;
  nodes[0][1] = yRange*(double)ywin/(double)ymax;
  currX = nodes[0][0];
  currY = nodes[0][1];
  // calculate metric
  currMetricVal = (*func)(3, nodes);
  // emit value changed
  emit current_val_changed();
}

void Metric2DTri::generate_plot()
{
  // create a drawing list and delete old one if it exists
  if(drawingList)
    glDeleteLists(drawingList,1);
  drawingList = glGenLists(1);
  glNewList(drawingList, GL_COMPILE);
  {
    double nodes[3][3] = { {-.5,1,0}, {-.5,0,0}, {.5,0,0}};
    glPointSize(4.0);
    // coordinates can range between (-xRange, xRange) and (0, yRange)
    double hscan , vscan;
    hscan = vscan = sqrt((double)NUM_POINTS);
    // scan vertically
    for(int i=0; i<vscan; i++)
    {
      nodes[0][1] = (double)i/(double)vscan * yRange;
      // scan horizontally
      for(int j=0; j<hscan; j++)
      {
        nodes[0][0] = (double)j/(double)hscan * 2 * xRange - xRange;
        // calculate metric
        double val = (*func)(3, nodes);
        // set color based on value
        glColor3f( (colorFactor-val)*(colorFactor-val), val*val,2*(colorFactor-val)*val);
        // draw the point
        glBegin(GL_POINTS);
        glVertex3d(nodes[0][0], nodes[0][1], nodes[0][2]);
        glEnd();
      }
    }
    // draw fixed nodes  
    glPointSize(5.0);
    glColor3f(0,0,0);
    glBegin(GL_POINTS);
      glVertex3d(-.5,0,0);
      glVertex3d( .5,0,0);
    glEnd();
  }
  glEndList();
}


