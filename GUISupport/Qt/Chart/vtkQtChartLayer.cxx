/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartLayer.cxx

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

/// \file vtkQtChartLayer.cxx
/// \date February 1, 2008

#ifdef _MSC_VER
// Disable warnings that Qt headers give.
#pragma warning(disable:4127)
#endif

#include "vtkQtChartLayer.h"

#include "vtkQtChartAxis.h"
#include "vtkQtChartAxisDomain.h"
#include "vtkQtChartLayerDomain.h"
#include "vtkQtChartArea.h"
#include <QPainter>


vtkQtChartLayer::vtkQtChartLayer()
  : QObject(), QGraphicsItem()
{
  this->ChartArea = 0;
}

void vtkQtChartLayer::getLayerDomain(vtkQtChartLayerDomain &) const
{
}

bool vtkQtChartLayer::drawItemFilter(QGraphicsItem *, QPainter *)
{
  return false;
}

bool vtkQtChartLayer::getHelpText(const QPointF &, QString &)
{
  return false;
}


