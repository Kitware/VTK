/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartScene.h

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

/// \file vtkQtChartScene.h
/// \date March 12, 2008

#ifndef _vtkQtChartScene_h
#define _vtkQtChartScene_h

#include "vtkQtChartExport.h"
#include <QGraphicsScene>


class VTKQTCHART_EXPORT vtkQtChartScene : public QGraphicsScene
{
public:
  vtkQtChartScene(QObject *parent=0);
  virtual ~vtkQtChartScene() {}

protected:
  virtual void drawItems(QPainter *painter, int numItems,
      QGraphicsItem **items, const QStyleOptionGraphicsItem *options,
      QWidget *widget=0);
};

#endif
