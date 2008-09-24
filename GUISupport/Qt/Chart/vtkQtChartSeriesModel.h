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


/// \class vtkQtChartSeriesModel
/// \brief
///   The vtkQtChartSeriesModel class is the base class for all chart
///   series models.
class VTKQTCHART_EXPORT vtkQtChartSeriesModel : public QObject
{
  Q_OBJECT

public:
  /// \brief
  ///   Creates a chart series model.
  /// \param parent The parent object.
  vtkQtChartSeriesModel(QObject *parent=0);
  virtual ~vtkQtChartSeriesModel() {}

  /// \brief
  ///   Gets the number of series in the model.
  /// \return
  ///   The number of series in the model.
  virtual int getNumberOfSeries() const = 0;

  /// \brief
  ///   Gets the number of values in a series.
  /// \param series The series index.
  /// \return
  ///   The number of values in a series.
  virtual int getNumberOfSeriesValues(int series) const = 0;

  /// \brief
  ///   Gets the name for the given series.
  /// \param series The series index.
  /// \return
  ///   The name for the given series.
  virtual QVariant getSeriesName(int series) const = 0;

  /// \brief
  ///   Gets the series value for the given index and component.
  /// \param series The series index.
  /// \param index The index in the given series.
  /// \param component The component index.
  /// \return
  ///   The series value for the given index and component.
  virtual QVariant getSeriesValue(int series, int index,
      int component) const = 0;

  /// \brief
  ///   Gets the value range for a series component.
  /// \param series The series index.
  /// \param component The component index.
  /// \return
  ///   The value range for a series component.
  virtual QList<QVariant> getSeriesRange(int series, int component) const = 0;

signals:
  /// Emitted when the model is about to be reset.
  void modelAboutToBeReset();

  /// Emitted when the model has been reset.
  void modelReset();

  /// \brief
  ///   Emitted when series will be inserted into the model.
  /// \param first The first index to be added.
  /// \param last The last index to be added.
  void seriesAboutToBeInserted(int first, int last);

  /// \brief
  ///   Emitted when series have been inserted into the model.
  /// \param first The first index that was inserted.
  /// \param last The last index that was inserted.
  void seriesInserted(int first, int last);

  /// \brief
  ///   Emitted when series will be removed from the model.
  /// \param first The first index to be removed.
  /// \param last The last index to be removed.
  void seriesAboutToBeRemoved(int first, int last);

  /// \brief
  ///   Emitted when series have been removed from the model.
  /// \param first The first index that was removed.
  /// \param last The last index that was removed.
  void seriesRemoved(int first, int last);
};

#endif
