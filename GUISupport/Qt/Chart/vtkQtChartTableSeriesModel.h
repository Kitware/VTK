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


/// \class vtkQtChartTableSeriesModel
/// \brief
///   The vtkQtChartTableSeriesModel class is a chart series model
///   proxy for a QAbstractItemModel table.
class VTKQTCHART_EXPORT vtkQtChartTableSeriesModel :
  public vtkQtChartSeriesModel
{
  Q_OBJECT

public:
  /// \brief
  ///   Creates a table series model.
  /// \param model The item model to display.
  /// \param parent The parent object.
  vtkQtChartTableSeriesModel(QAbstractItemModel *model, QObject *parent=0);
  virtual ~vtkQtChartTableSeriesModel() {}

  /// \brief
  ///   Gets the item model.
  /// \return
  ///   A pointer to the item model.
  QAbstractItemModel *getItemModel() const {return this->Model;}

  /// \brief
  ///   Sets the item model.
  /// \param model The new item model.
  void setItemModel(QAbstractItemModel *model);

  /// \brief
  ///   Gets whether or not columns are series.
  /// \return
  ///   True if columns are series.
  bool getColumnsAsSeries() const;

  /// \brief
  ///   Sets whether or not columns are series.
  /// \param columnsAsSeries True if columns are series.
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
  /// \brief
  ///   Called when the item model is about to insert rows.
  /// \param index The parent model index.
  /// \param first The first index of the insertion range.
  /// \param last The last index of the insertion range.
  void rowsAboutToBeInserted(const QModelIndex &index, int first, int last);

  /// \brief
  ///   Called when the item model has inserted rows.
  /// \param index The parent model index.
  /// \param first The first index of the insertion range.
  /// \param last The last index of the insertion range.
  void rowsInserted(const QModelIndex &index, int first, int last);

  /// \brief
  ///   Called when the item model is about to insert columns.
  /// \param index The parent model index.
  /// \param first The first index of the insertion range.
  /// \param last The last index of the insertion range.
  void columnsAboutToBeInserted(const QModelIndex &index, int first, int last);

  /// \brief
  ///   Called when the item model has inserted columns.
  /// \param index The parent model index.
  /// \param first The first index of the insertion range.
  /// \param last The last index of the insertion range.
  void columnsInserted(const QModelIndex &index, int first, int last);

  /// \brief
  ///   Called when the item model is about to remove rows.
  /// \param index The parent model index.
  /// \param first The first index of the removal range.
  /// \param last The last index of the removal range.
  void rowsAboutToBeRemoved(const QModelIndex &index, int first, int last);

  /// \brief
  ///   Called when the item model has removed rows.
  /// \param index The parent model index.
  /// \param first The first index of the removal range.
  /// \param last The last index of the removal range.
  void rowsRemoved(const QModelIndex &index, int first, int last);

  /// \brief
  ///   Called when the item model is about to remove columns.
  /// \param index The parent model index.
  /// \param first The first index of the removal range.
  /// \param last The last index of the removal range.
  void columnsAboutToBeRemoved(const QModelIndex &index, int first, int last);

  /// \brief
  ///   Called when the item model has removed columns.
  /// \param index The parent model index.
  /// \param first The first index of the removal range.
  /// \param last The last index of the removal range.
  void columnsRemoved(const QModelIndex &index, int first, int last);

protected:
  QAbstractItemModel *Model;         ///< Stores the item model.
  vtkQtChartSeriesModelRange *Range; ///< Stores the series ranges.
  bool ColumnsAsSeries;              ///< True if columns are series.

private:
  vtkQtChartTableSeriesModel(const vtkQtChartTableSeriesModel &);
  vtkQtChartTableSeriesModel &operator=(const vtkQtChartTableSeriesModel &);
};

#endif

