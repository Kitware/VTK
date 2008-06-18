/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtLineChartSeriesOptions.h

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

/// \file vtkQtLineChartSeriesOptions.h
/// \date February 15, 2008

#ifndef _vtkQtLineChartSeriesOptions_h
#define _vtkQtLineChartSeriesOptions_h

#include "vtkQtChartExport.h"
#include "vtkQtChartSeriesOptions.h"
#include "vtkQtChartLayer.h"  // needed for enum
#include "vtkQtPointMarker.h" // needed for enum

class QSizeF;


class VTKQTCHART_EXPORT vtkQtLineChartSeriesOptions :
  public vtkQtChartSeriesOptions
{
  Q_OBJECT

public:
  vtkQtLineChartSeriesOptions(QObject *parent=0);
  virtual ~vtkQtLineChartSeriesOptions();

  virtual void setStyle(int style, vtkQtChartStyleGenerator *generator);

  /// get which axes corner (e.g. bottom-left), this series is plotted against
  vtkQtChartLayer::AxesCorner getAxesCorner() const;
  void setAxesCorner(vtkQtChartLayer::AxesCorner axes);

  bool arePointsVisible() const {return this->ShowPoints;}
  void setPointsVisible(bool visible);

  vtkQtPointMarker::MarkerStyle getMarkerStyle() const;
  void setMarkerStyle(vtkQtPointMarker::MarkerStyle style);

  const QSizeF &getMarkerSize() const;
  void setMarkerSize(const QSizeF &size);

signals:
  void axesCornerChanged(int corner, int previous);
  void pointVisibilityChanged(bool visible);
  void pointMarkerChanged();

protected:
  vtkQtChartLayer::AxesCorner AxesCorner;
  vtkQtPointMarker::MarkerStyle PointStyle;
  QSizeF *PointSize;
  bool ShowPoints;
};

#endif

