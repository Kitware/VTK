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


/// \class vtkQtChartSeriesModelRange
/// \brief
///   The vtkQtChartSeriesModelRange class stores the series ranges
///   for a series model.
class VTKQTCHART_EXPORT vtkQtChartSeriesModelRange : public QObject
{
  Q_OBJECT

public:
  /// \brief
  ///   Creates a chart series model range.
  /// \param model The series model to use.
  vtkQtChartSeriesModelRange(vtkQtChartSeriesModel *model);
  ~vtkQtChartSeriesModelRange() {}

  /// \brief
  ///   Initializes the series ranges.
  /// \param xShared True if the series share the same x-axis array.
  void initializeRanges(bool xShared=false);

  /// \brief
  ///   Gets whether or not the series share the same x-axis array.
  /// \return
  ///   True if the series share the same x-axis array.
  bool isXRangeShared() const {return this->XRangeShared;}

  /// \brief
  ///   Gets the value range for a series component.
  /// \param series The series index.
  /// \param component The component index.
  /// \return
  ///   The value range for a series component.
  QList<QVariant> getSeriesRange(int series, int component) const;

private slots:
  /// Recalculates the series ranges.
  void resetSeries();

  /// \brief
  ///   Adds series ranges to the list.
  ///
  /// The range for each series is calculated when it is added.
  ///
  /// \param first The first series index.
  /// \param last The last series index.
  void insertSeries(int first, int last);

  /// \brief
  ///   Removes series ranges from the list.
  /// \param first The first series index.
  /// \param last The last series index.
  void removeSeries(int first, int last);

private:
  /// \brief
  ///   Calculates the range for the given series component.
  /// \param series The series index.
  /// \param component The component index.
  /// \return
  ///   The value range for a series component.
  QList<QVariant> computeSeriesRange(int series, int component);

private:
  QList<QList<QVariant> > Range[2]; ///< Stores the series ranges.
  vtkQtChartSeriesModel *Model;     ///< Stores the series model.
  bool XRangeShared; ///< True if the series share the same x-axis array.
};

#endif
