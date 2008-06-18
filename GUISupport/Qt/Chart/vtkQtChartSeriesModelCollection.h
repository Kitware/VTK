/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartSeriesModelCollection.h

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

/// \file vtkQtChartSeriesModelCollection.h
/// \date February 8, 2008

#ifndef _vtkQtChartSeriesModelCollection_h
#define _vtkQtChartSeriesModelCollection_h

#include "vtkQtChartExport.h"
#include "vtkQtChartSeriesModel.h"


/// a model class that combines the series of multiple 
/// QAbstractItemModels
class VTKQTCHART_EXPORT vtkQtChartSeriesModelCollection :
  public vtkQtChartSeriesModel
{
  Q_OBJECT

public:
  vtkQtChartSeriesModelCollection(QObject *parent=0);
  virtual ~vtkQtChartSeriesModelCollection() {}

  /// \name vtkQtChartSeriesModel Methods
  //@{
  virtual int getNumberOfSeries() const;
  virtual int getNumberOfSeriesValues(int series) const;
  virtual QVariant getSeriesName(int series) const;
  virtual QVariant getSeriesValue(int series, int index, int component) const;
  virtual QList<QVariant> getSeriesRange(int series, int component) const;
  //@}

  /// add a model containing series
  void addSeriesModel(vtkQtChartSeriesModel *model);
  /// remove a model
  void removeSeriesModel(vtkQtChartSeriesModel *model);
  /// get the number of models
  int getNumberOfSeriesModels() const;
  /// get a model from an index
  vtkQtChartSeriesModel *getSeriesModel(int index) const;

protected slots:
  void onSeriesAboutToBeInserted(int first, int last);
  void onSeriesInserted(int first, int last);
  void onSeriesAboutToBeRemoved(int first, int last);
  void onSeriesRemoved(int first, int last);

private:
  vtkQtChartSeriesModel *modelForSeries(int &series) const;
  int seriesForModel(vtkQtChartSeriesModel *model) const;

private:
  QList<vtkQtChartSeriesModel *> Models;
};

#endif

