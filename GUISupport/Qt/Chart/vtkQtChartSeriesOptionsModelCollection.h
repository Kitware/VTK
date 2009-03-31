/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartSeriesOptionsModelCollection.h

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

#ifndef __vtkQtChartSeriesOptionsModelCollection_h
#define __vtkQtChartSeriesOptionsModelCollection_h

#include "vtkQtChartSeriesOptionsModel.h"

/// \class vtkQtChartSeriesOptionsModelCollection
/// \brief
///   The vtkQtChartSeriesOptionsModelCollection class is used to combine
///   chart series options models.
///
/// The collection maps the overall series index to the model
/// specific series index. This is analogous to vtkQtChartSeriesModelCollection
/// except that it keeps track of vtkQtChartSeriesOptionsModel instead of
/// vtkQtChartSeriesModel.

class VTKQTCHART_EXPORT vtkQtChartSeriesOptionsModelCollection :
  public vtkQtChartSeriesOptionsModel
{
  Q_OBJECT
public:
  typedef vtkQtChartSeriesOptionsModel Superclass;

  /// \brief
  ///   Creates a chart series options model.
  /// \param param The parent object.
  vtkQtChartSeriesOptionsModelCollection(QObject* parent=0);
  virtual ~vtkQtChartSeriesOptionsModelCollection();

  /// \name vtkQtChartSeriesOptionsModel Methods
  //@{
  /// \brief
  ///   Gets the number of options.
  virtual int getNumberOfOptions() const;

  /// \brief
  ///   Gets the options for a particular series.
  /// \param series The series index
  /// \return
  ///   The options for the series at the given index.
  virtual vtkQtChartSeriesOptions* getOptions(int series) const;

  /// \brief
  ///   Gets the index for the given series options.
  /// \param options The series options object.
  /// \return
  ///   The index for the given series options.
  virtual int getOptionsIndex(vtkQtChartSeriesOptions *options) const;
  //@}

  /// \brief
  ///   Adds a series model to the collection.
  /// \param model The series model to add.
  void addSeriesOptionsModel(vtkQtChartSeriesOptionsModel *model);

  /// \brief
  ///   Removes a series model from the collection.
  /// \param model The series model to remove.
  void removeSeriesOptionsModel(vtkQtChartSeriesOptionsModel* model);

  /// \brief
  ///   Gets the number of series models in the collection.
  /// \return
  ///   The number of series models in the collection.
  int getNumberOfSeriesOptionsModels() const;

  /// \brief
  ///   Gets the series model at the specified index.
  /// \param index The series model index.
  /// \return
  ///   A pointer to the series model.
  vtkQtChartSeriesOptionsModel* getSeriesOptionsModel(int index) const;

  /// \brief
  ///   Maps an index from a series model to an index in the collection.
  /// \param model The series model, must be a member of the model collection
  /// \param index A series index from the given series model
  /// \return
  ///   A series index in the series model collection.
  ///   Returns 0 if model is not in collection.
  int mapSeriesIndexToCollectionIndex(
    vtkQtChartSeriesOptionsModel* model, int index) const;

public slots:
  /// \brief
  ///   Resets the model.
  virtual void reset();

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
  void onOptionsAboutToBeInserted(int first, int last);

  /// \brief
  ///   Called when a series is inserted into a model.
  /// \param first The first model series index.
  /// \param last The last model series index.
  void onOptionsInserted(int first, int last);

  /// \brief
  ///   Called when a series is about to be removed from a model.
  /// \param first The first model series index.
  /// \param last The last model series index.
  void onOptionsAboutToBeRemoved(int first, int last);

  /// \brief
  ///   Called when a series is removed from a model.
  /// \param first The first model series index.
  /// \param last The last model series index.
  void onOptionsRemoved(int first, int last);

private:
  /// \brief
  ///   Gets the series model for the given index.
  /// \param series The collection series index. Used to return the
  ///   model series index.
  /// \return
  ///   A pointer to the series model.
  vtkQtChartSeriesOptionsModel *modelForSeries(int &series) const;

  /// \brief
  ///   Gets the first series in the given model.
  /// \param model The series model.
  /// \return
  ///   The first series in the given model.
  int seriesForModel(vtkQtChartSeriesOptionsModel *model) const;

private:
  QList<vtkQtChartSeriesOptionsModel*> Models; ///< Stores the models.

private:
  vtkQtChartSeriesOptionsModelCollection(const
    vtkQtChartSeriesOptionsModelCollection&);
  void operator=(
    const vtkQtChartSeriesOptionsModelCollection&);
};

#endif


