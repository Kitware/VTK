/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartSeriesModelRange.h

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

/// \file vtkQtChartSeriesModelRange.h
/// \date February 19, 2008

#ifndef _vtkQtChartSeriesModelRange_h
#define _vtkQtChartSeriesModelRange_h

#include "vtkQtChartExport.h"
#include <QObject>
#include <QList>    // Needed for return type.
#include <QVariant> // Needed for return type.

class vtkQtChartSeriesModel;


class VTKQTCHART_EXPORT vtkQtChartSeriesModelRange : public QObject
{
  Q_OBJECT

public:
  vtkQtChartSeriesModelRange(vtkQtChartSeriesModel *model);
  ~vtkQtChartSeriesModelRange() {}

  void initializeRanges(bool xShared=false);

  bool isXRangeShared() const {return this->XRangeShared;}

  QList<QVariant> getSeriesRange(int series, int component) const;

private slots:
  void resetSeries();
  void insertSeries(int first, int last);
  void removeSeries(int first, int last);

private:
  QList<QVariant> computeSeriesRange(int series, int component);

private:
  QList<QList<QVariant> > Range[2];
  vtkQtChartSeriesModel *Model;
  bool XRangeShared;
};

#endif
