
#include "quadmetrics.hpp"
#include <math.h>

void Metric3DQuad::draw(int /*xwin*/, int /*ywin*/)
{
  // draw metric information
  glCallList(drawingListBase+currZPlane);

  // draw moused point
  glBegin(GL_POINTS);
  glPointSize(5.0);
  glColor3f(0.0,0.0,0.0);
  glVertex3f(currX, currY, 0);
  glEnd();
}

void Metric3DQuad::mouseEvent( QMouseEvent* e, int xmax, int ymax, bool flag )
{
  if(e->button() == RightButton && flag)
  {
    if(!animator)
    {
      animator = new QTimer(this);
      connect(animator, SIGNAL(timeout()), this, SLOT(animateStep()));
    }
    if(animator && animator->isActive())
      animator->stop();
    else
      animator->start(100);
  }
  else
  {
    // convert window coords to world coords
    int ywin = ywin = ymax - e->y();
    double nodes[4][3] = { {0,1,0}, {0,0,0}, {1,0,0}, {.25,2,0} };
    nodes[3][2] = (double)currZPlane/(double)NUM_Z_PLANES * 2 * xRange - xRange;
    nodes[3][0] = xRange*(double)e->x()/(double)xmax;
    nodes[3][1] = yRange*(double)ywin/(double)ymax;
    currX = nodes[3][0];
    currY = nodes[3][1];
    // calculate metrics
    currMetricVal = (*func)(4, nodes);
    // emit value changed
    emit current_val_changed();
  }
} 

void Metric3DQuad::generate_plot()
{
  // create a drawing list and delete the old one if necessary
  if(!drawingListBase)
    drawingListBase = glGenLists(NUM_Z_PLANES);
    
  double nodes[4][3] = { {0,1,0}, {0,0,0}, {1,0,0}, {.25,2,0} };
  glPointSize(4.0);
  double hscan , vscan;
  hscan = vscan = sqrt((double)NUM_POINTS);

  for(int z = 0; z<NUM_Z_PLANES; z++)
  {
    nodes[3][2] = (double)z/(double)NUM_Z_PLANES * 2 * xRange - xRange;
    glNewList(drawingListBase + z, GL_COMPILE);
    {
      // coordinates can range between (0,xRange) and (0,yRange)
      // scan vertically
      for(int i=0; i<vscan; i++)
      {
        nodes[3][1] = (double)i/(double)vscan * yRange;
        // scan horizontally
        for(int j=0; j<hscan; j++)
        {
          nodes[3][0] = (double)j/(double)hscan * xRange;
          // calculate metric
          double val = (*func)(4, nodes);
          // set color based on value of metric
          glColor3f( (colorFactor-val)*(colorFactor-val), val*val,2*(colorFactor-val)*val);
          // draw point
          glBegin(GL_POINTS);
          glVertex3d(nodes[3][0], nodes[3][1], nodes[3][2]);
          glEnd();
        }
      }        
      // fixed node positions
      glPointSize(5.0);
      glColor3f(0,0,0);
      glBegin(GL_POINTS);
      glVertex3d(1,0,0);
      glVertex3d(0,0,0);
      glVertex3d(0,1,0);    
      glEnd();
    }
    glEndList();
  }
}

void Metric2DQuad::draw(int /*xwin*/, int /*ywin*/ )
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

void Metric2DQuad::mouseEvent(QMouseEvent *e, int xmax, int ymax, bool)
{
  // convert window coords to world coords
  int ywin = ymax - e->y();
  int xwin = e->x();
  double nodes[4][3] = { {0,1,0}, {0,0,0}, {1,0,0}, {.25,2,0} };
  nodes[3][0] = xRange*(double)xwin/(double)xmax;
  nodes[3][1] = yRange*(double)ywin/(double)ymax;
  currX = nodes[3][0];
  currY = nodes[3][1];
  // calculate metric
  currMetricVal = (*func)(3, nodes);
  // emit value changed
  emit current_val_changed();
}

void Metric2DQuad::generate_plot()
{
  // create a drawing list and delete old one if it exists
  if(drawingList)
    glDeleteLists(drawingList,1);
  drawingList = glGenLists(1);
  glNewList(drawingList, GL_COMPILE);
  {
    double nodes[4][3] = { {0,1,0}, {0,0,0}, {1,0,0}, {.25,2,0} };
    glPointSize(4.0);
    // coordinates can range between (-xRange, xRange) and (0, yRange)
    double hscan , vscan;
    hscan = vscan = sqrt((double)NUM_POINTS);
    // scan vertically
    for(int i=0; i<vscan; i++)
    {
      nodes[3][1] = (double)i/(double)vscan * yRange;
      // scan horizontally
      for(int j=0; j<hscan; j++)
      {
        nodes[3][0] = (double)j/(double)hscan * xRange;
        // calculate metric
        double val = (*func)(3, nodes);
        // set color based on value
        glColor3f( (colorFactor-val)*(colorFactor-val), val*val,2*(colorFactor-val)*val);
        // draw the point
        glBegin(GL_POINTS);
        glVertex3d(nodes[3][0], nodes[3][1], nodes[3][2]);
        glEnd();
      }
    }
    // fixed node positions
    glPointSize(5.0);
    glColor3f(0,0,0);
    glBegin(GL_POINTS);
    glVertex3d(1,0,0);
    glVertex3d(0,0,0);
    glVertex3d(0,1,0);    
    glEnd();
  }
  glEndList();
}




