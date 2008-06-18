/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartGraphicsItemTypes.h

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

/// \file vtkQtChartGraphicsItemTypes.h
/// \date June 18, 2008

#ifndef _vtkQtChartGraphicsItemTypes_h
#define _vtkQtChartGraphicsItemTypes_h

#include <QGraphicsItem> // needed for variable

enum vtkQtChartGraphicsItemTypes
{
  vtkQtChart_AxisType = QGraphicsItem::UserType + 1,
  vtkQtChart_AxisItemType,
  vtkQtChart_AxisLayerType,
  vtkQtChart_BarChartType,
  vtkQtChart_BarChartItemType,
  vtkQtChart_ContentsAreaType,
  vtkQtChart_GridLayerType,
  vtkQtChart_GridLayerItemType,
  vtkQtChart_LayerType,
  vtkQtChart_LineChartType,
  vtkQtChart_LineChartItemType,
  vtkQtChart_PointMarkerType,
  vtkQtChart_PolylineItemType,
  vtkQtChart_SeriesLayerType,
  vtkQtChart_StackedChartType,
  vtkQtChart_StatisticalBoxChartType,
  vtkQtChart_StatisticalBoxChartItemType
};

#endif
