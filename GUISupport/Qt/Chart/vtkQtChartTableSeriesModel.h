/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartTableSeriesModel.h

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

/// \file vtkQtChartTableSeriesModel.h
/// \date February 11, 2008

#ifndef _vtkQtChartTableSeriesModel_h
#define _vtkQtChartTableSeriesModel_h

#include "vtkQtChartExport.h"
#include "vtkQtChartSeriesModel.h"

class vtkQtChartSeriesModelRange;
class QAbstractItemModel;
class QModelIndex;

/// a model class that combines the series of multiple 
/// QAbstractItemModels
class VTKQTCHART_EXPORT vtkQtChartTableSeriesModel : 
  public vtkQtChartSeriesModel
{
  Q_OBJECT

public:
  vtkQtChartTableSeriesModel(QAbstractItemModel* model, QObject *parent=0);
  virtual ~vtkQtChartTableSeriesModel() {}

  bool getColumnsAsSeries() const;
  void setColumnsAsSeries(bool columnsAsSeries);

  /// \name vtkQtChartSeriesModel Methods
  //@{
  virtual int getNumberOfSeries() const;
  virtual int getNumberOfSeriesValues(int series) const;
  virtual QVariant getSeriesName(int series) const;
  virtual QVariant getSeriesValue(int series, int index, int component) const;
  virtual QList<QVariant> getSeriesRange(int series, int component) const;
  //@}

protected slots:
  void rowsAboutToBeInserted(const QModelIndex& idx, int, int);
  void rowsInserted(const QModelIndex& idx, int, int);
  void columnsAboutToBeInserted(const QModelIndex& idx, int, int);
  void columnsInserted(const QModelIndex& idx, int, int);
  void rowsAboutToBeRemoved(const QModelIndex& idx, int, int);
  void rowsRemoved(const QModelIndex& idx, int, int);
  void columnsAboutToBeRemoved(const QModelIndex& idx, int, int);
  void columnsRemoved(const QModelIndex& idx, int, int);
  
protected:
  QAbstractItemModel *Model;
  vtkQtChartSeriesModelRange *Range;
  bool ColumnsAsSeries;
};

#endif

