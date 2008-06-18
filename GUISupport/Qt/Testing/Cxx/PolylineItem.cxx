/*=========================================================================

  Program:   Visualization Toolkit
  Module:    PolylineItem.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

#include "QTestApp.h"

#include <QGraphicsScene>
#include <QGraphicsView>
#include <QPolygonF>
#include "vtkQtPolylineItem.h"
#include "math.h"

int PolylineItem(int argc, char* argv[])
{
  QTestApp app(argc, argv);

  QGraphicsScene scene;
  vtkQtPolylineItem* item = new vtkQtPolylineItem;
  scene.addItem(item);

  QPolygonF polyline;
  for(int i=0; i<2000; i++)
    {
    polyline << QPointF(i,sin(i/100.0));
    }
  item->setPolyline(polyline);
  item->setPen(QPen(QBrush(QColor("blue")), 0, Qt::DashDotDotLine));
  
  QGraphicsView view(&scene);
  view.fitInView(item);
  view.show();

  int status = app.exec();

  return status; 
}

