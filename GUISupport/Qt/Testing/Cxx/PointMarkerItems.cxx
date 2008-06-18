/*=========================================================================

  Program:   Visualization Toolkit
  Module:    PointMarkerItems.cxx

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
#include <QGraphicsRectItem>
#include "vtkQtPointMarker.h"
#include <QSizeF>

int PointMarkerItems(int argc, char* argv[])
{
  QTestApp app(argc, argv);

  QGraphicsScene scene;
  scene.setSceneRect(0,0,400,400);

  QGraphicsRectItem* boundary = new QGraphicsRectItem(0,0,100,100);
  scene.addItem(boundary);


  QSizeF size(10,10);
  QPolygonF points;
  points.append(QPointF(0.0, 0.0));
  vtkQtPointMarker* cross = new vtkQtPointMarker(size,
    vtkQtPointMarker::Cross);
  cross->setPen(QPen(QBrush(QColor("red")), 1));
  cross->setPoints(points);
  scene.addItem(cross);


  vtkQtPointMarker* plus = new vtkQtPointMarker(size, vtkQtPointMarker::Plus);
  plus->moveBy(50,50);
  plus->setPen(QPen(QBrush(QColor("green")), 2));
  plus->setPoints(points);
  scene.addItem(plus);


  vtkQtPointMarker* square = new vtkQtPointMarker(size,
    vtkQtPointMarker::Square);
  square->moveBy(100,0);
  square->setPen(QPen(QBrush(QColor("blue")), 3));
  square->setPoints(points);
  scene.addItem(square);


  vtkQtPointMarker* circle = new vtkQtPointMarker(size,
    vtkQtPointMarker::Circle);
  circle->moveBy(100,100);
  circle->setPen(QPen(QBrush(QColor("yellow")), 4));
  circle->setPoints(points);
  scene.addItem(circle);


  vtkQtPointMarker* diamond = new vtkQtPointMarker(size,
    vtkQtPointMarker::Diamond);
  diamond->moveBy(0,100);
  diamond->setPen(QPen(QBrush(QColor("purple")), 5));
  diamond->setPoints(points);
  scene.addItem(diamond);


  QGraphicsView view(&scene);
  view.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view.setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

  view.resize(400,400);
  view.show();

  int status = app.exec();

  return status; 
}

