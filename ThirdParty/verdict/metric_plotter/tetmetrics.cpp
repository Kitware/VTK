
#include "tetmetrics.hpp"
#include <math.h>

void Metric3DTet::draw(int /*xwin*/, int /*ywin*/)
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

void Metric3DTet::mouseEvent( QMouseEvent *e, int xmax, int ymax , bool flag )
{
  if(e->button() == RightButton && flag  )
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
    int ywin = ymax - e->y();
    int xwin = e->x();
    double nodes[4][3] = { {-0.5,0,0.866025}, {0.5,0,0.866025}, 
      {0,0,-0.866025}, {999,999,999} };
      
      nodes[3][2] = (double)currZPlane/(double)NUM_Z_PLANES * 2 * xRange - xRange;
      nodes[3][0] = 2*xRange*(double)xwin/(double)xmax - xRange;
      nodes[3][1] = yRange*(double)ywin/(double)ymax;
      currX = nodes[3][0];
      currY = nodes[3][1];
      // calculate metrics
      currMetricVal = (*func)(4, nodes);
      // emit value changed
      emit current_val_changed();
  }
} 
  
void Metric3DTet::generate_plot()
{
  // create a drawing list and reuse old ones if necessary
  if(!drawingListBase)
    drawingListBase = glGenLists(NUM_Z_PLANES);
  double nodes[4][3] = { {-0.5,0,0.866025}, {0.5,0,0.866025}, 
      {0,0,-0.866025}, {999,999,999} };
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
          nodes[3][0] = (double)j/(double)hscan * 2 * xRange - xRange;
          // calculate metric
          double val = (*func)(4, nodes);
          // set color based on value of metric
          glColor3f( (colorFactor-val)*(colorFactor-val), 
              val*val,2*(colorFactor-val)*val);
          // draw point
          glBegin(GL_POINTS);
          glVertex3d(nodes[3][0], nodes[3][1], nodes[3][2]);
          glEnd();
        }
      }
    }
    glEndList();
  }
}



