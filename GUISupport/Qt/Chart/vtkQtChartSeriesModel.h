/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartSeriesModel.h

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

/// \file vtkQtChartSeriesModel.h
/// \date February 8, 2008

#ifndef _vtkQtChartSeriesModel_h
#define _vtkQtChartSeriesModel_h

#include "vtkQtChartExport.h"
#include <QVariant> // Needed for return type.
#include <QList>    // Needed for return type.

/// a model class that combines the series of multiple 
/// QAbstractItemModels
class VTKQTCHART_EXPORT vtkQtChartSeriesModel : public QObject
{
  Q_OBJECT

public:
  vtkQtChartSeriesModel(QObject *parent=0);
  virtual ~vtkQtChartSeriesModel() {}

  /// get the number of series from all the models combined 
  virtual int getNumberOfSeries() const = 0;
  /// get the number of values in a series
  virtual int getNumberOfSeriesValues(int series) const = 0;
  /// get the name of a series
  virtual QVariant getSeriesName(int series) const = 0;

  virtual QVariant getSeriesValue(int series, int index,
      int component) const = 0;

  virtual QList<QVariant> getSeriesRange(int series, int component) const = 0;

signals:
  void modelAboutToBeReset();
  void modelReset();

  void seriesAboutToBeInserted(int first, int last);
  void seriesInserted(int first, int last);
  
  void seriesAboutToBeRemoved(int first, int last);
  void seriesRemoved(int first, int last);
};

#endif
