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


/// \class vtkQtChartSeriesModelCollection
/// \brief
///   The vtkQtChartSeriesModelCollection class is used to combine
///   chart series models.
///
/// The collection maps the overall series index to the model
/// specific series index.
class VTKQTCHART_EXPORT vtkQtChartSeriesModelCollection :
  public vtkQtChartSeriesModel
{
  Q_OBJECT

public:
  /// \brief
  ///   Creates a chart series model collection.
  /// \param parent The parent object.
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

  /// \brief
  ///   Adds a series model to the collection.
  /// \param model The series model to add.
  void addSeriesModel(vtkQtChartSeriesModel *model);

  /// \brief
  ///   Removes a series model from the collection.
  /// \param model The series model to remove.
  void removeSeriesModel(vtkQtChartSeriesModel *model);

  /// \brief
  ///   Gets the number of series models in the collection.
  /// \return
  ///   The number of series models in the collection.
  int getNumberOfSeriesModels() const;

  /// \brief
  ///   Gets the series model at the specified index.
  /// \param index The series model index.
  /// \return
  ///   A pointer to the series model.
  vtkQtChartSeriesModel *getSeriesModel(int index) const;

protected slots:
  /// \brief
  ///   Called when a series is about to be inserted into a model.
  ///
  /// This method uses the signal sender to determine which model has
  /// changed. It then maps the model series indexes to collection
  /// series indexes and re-emits the signal.
  ///
  /// \param first The first model series index.
  /// \param last The last model series index.
  void onSeriesAboutToBeInserted(int first, int last);

  /// \brief
  ///   Called when a series is inserted into a model.
  /// \param first The first model series index.
  /// \param last The last model series index.
  void onSeriesInserted(int first, int last);

  /// \brief
  ///   Called when a series is about to be removed from a model.
  /// \param first The first model series index.
  /// \param last The last model series index.
  void onSeriesAboutToBeRemoved(int first, int last);

  /// \brief
  ///   Called when a series is removed from a model.
  /// \param first The first model series index.
  /// \param last The last model series index.
  void onSeriesRemoved(int first, int last);

private:
  /// \brief
  ///   Gets the series model for the given index.
  /// \param series The collection series index. Used to return the
  ///   model series index.
  /// \return
  ///   A pointer to the series model.
  vtkQtChartSeriesModel *modelForSeries(int &series) const;

  /// \brief
  ///   Gets the first series in the given model.
  /// \param model The series model.
  /// \return
  ///   The first series in the given model.
  int seriesForModel(vtkQtChartSeriesModel *model) const;

private:
  QList<vtkQtChartSeriesModel *> Models; ///< Stores the models.

private:
  vtkQtChartSeriesModelCollection(const vtkQtChartSeriesModelCollection &);
  vtkQtChartSeriesModelCollection &operator=(
      const vtkQtChartSeriesModelCollection &);
};

#endif

