/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartSeriesSelection.h

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

/// \file vtkQtChartSeriesSelection.h
/// \date March 14, 2008

#ifndef _vtkQtChartSeriesSelection_h
#define _vtkQtChartSeriesSelection_h

#include "vtkQtChartExport.h"
#include <QMap> // needed for return type
#include "vtkQtChartIndexRangeList.h" // needed for return type.

class vtkQtChartSeriesSelectionInternal;


/// \class vtkQtChartSeriesSelection
/// \brief
///   The vtkQtChartSeriesSelection class is used for series and point
///   selection.
class VTKQTCHART_EXPORT vtkQtChartSeriesSelection
{
public:
  enum SelectionType
    {
    NoSelection = 0, ///< No selection is made.
    SeriesSelection, ///< The selection contains series indexes.
    PointSelection   ///< The selection contains point indexes.
    };

public:
  vtkQtChartSeriesSelection();
  vtkQtChartSeriesSelection(const vtkQtChartSeriesSelection &other);
  ~vtkQtChartSeriesSelection();

  vtkQtChartSeriesSelection &operator=(const vtkQtChartSeriesSelection &other);

  /// \brief
  ///   Gets whether or not the selection is empty.
  /// \return
  ///   True if the selection is empty.
  bool isEmpty() const;

  /// \brief
  ///   Gets the selection type.
  /// \return
  ///   The selection type.
  SelectionType getType() const;

  /// Clears the selection lists.
  bool clear();

  /// \name Series Selection Methods
  //@{
  /// \brief
  ///   Gets the list of selected series ranges.
  /// \return
  ///   A reference to the list of selected series ranges.
  const vtkQtChartIndexRangeList &getSeries() const;

  /// \brief
  ///   Sets the list of selected series ranges.
  /// \param series The new list of selected series ranges.
  /// \return
  ///   True if the selection was modified.
  bool setSeries(const vtkQtChartIndexRangeList &series);

  /// \brief
  ///   Sets the list of selected series ranges.
  /// \param first The first series index in the range.
  /// \param last The last series index in the range.
  /// \return
  ///   True if the selection was modified.
  bool setSeries(int first, int last);

  /// \brief
  ///   Adds the list of series ranges to the selection.
  /// \param series The list of selected series ranges to add.
  /// \return
  ///   True if the selection was modified.
  bool addSeries(const vtkQtChartIndexRangeList &series);

  /// \brief
  ///   Adds the series index range to the selection.
  /// \param first The first series index in the range.
  /// \param last The last series index in the range.
  /// \return
  ///   True if the selection was modified.
  bool addSeries(int first, int last);

  /// \brief
  ///   Subtracts the list of series ranges from the selection.
  /// \param series The list of selected series ranges to subtract.
  /// \return
  ///   True if the selection was modified.
  bool subtractSeries(const vtkQtChartIndexRangeList &series);

  /// \brief
  ///   Subtracts the series index range from the selection.
  /// \param first The first series index in the range.
  /// \param last The last series index in the range.
  /// \return
  ///   True if the selection was modified.
  bool subtractSeries(int first, int last);

  /// \brief
  ///   Selects unique series from the given list and the selection.
  /// \param series The list of series ranges.
  /// \return
  ///   True if the selection was modified.
  bool xorSeries(const vtkQtChartIndexRangeList &series);

  /// \brief
  ///   Selects unique series from the given range and the selection.
  /// \param first The first series index in the range.
  /// \param last The last series index in the range.
  /// \return
  ///   True if the selection was modified.
  bool xorSeries(int first, int last);

  /// \brief
  ///   Trims the selected series to the given bounds.
  /// \param minimum The minimum series index.
  /// \param maximum The maximum series index.
  void limitSeries(int minimum, int maximum);

  /// \brief
  ///   Adds the offset to all the series greater than or equal to
  ///   the given series.
  /// \param first The starting series index.
  /// \param offset The offset to add to the indexes.
  /// \return
  ///   True if the selection was modified.
  bool offsetSeries(int first, int offset);
  //@}

  /// \name Point Selection Methods
  //@{
  /// \brief
  ///   Gets the list of selected point ranges.
  /// \return
  ///   A reference to the list of selected point ranges.
  const QMap<int, vtkQtChartIndexRangeList> &getPoints() const;

  /// \brief
  ///   Sets the list of selected point ranges.
  /// \param points The new list of selected point ranges.
  /// \return
  ///   True if the selection was modified.
  bool setPoints(const QMap<int, vtkQtChartIndexRangeList> &points);

  /// \brief
  ///   Sets the list of selected point ranges.
  /// \param series The series index.
  /// \param indexes The list of point ranges.
  /// \return
  ///   True if the selection was modified.
  bool setPoints(int series, const vtkQtChartIndexRangeList &indexes);

  /// \brief
  ///   Adds the list of point ranges to the selection.
  /// \param points The list of selected point ranges to add.
  /// \return
  ///   True if the selection was modified.
  bool addPoints(const QMap<int, vtkQtChartIndexRangeList> &points);

  /// \brief
  ///   Adds the list of point ranges to the selection.
  /// \param series The series index.
  /// \param indexes The list of point ranges.
  /// \return
  ///   True if the selection was modified.
  bool addPoints(int series, const vtkQtChartIndexRangeList &indexes);

  /// \brief
  ///   Subtracts the list of point ranges from the selection.
  /// \param points The list of selected point ranges to subtract.
  /// \return
  ///   True if the selection was modified.
  bool subtractPoints(const QMap<int, vtkQtChartIndexRangeList> &points);

  /// \brief
  ///   Subtracts the list of point ranges from the selection.
  /// \param series The series index.
  /// \param indexes The list of point ranges.
  /// \return
  ///   True if the selection was modified.
  bool subtractPoints(int series, const vtkQtChartIndexRangeList &indexes);

  /// \brief
  ///   Subtracts all the selected points in the given series from
  ///   the selection.
  /// \param first The first series in the range.
  /// \param last The last series in the range.
  /// \return
  ///   True if the selection was modified.
  bool subtractPoints(int first, int last);

  /// \brief
  ///   Selects unique points from the given list and the selection.
  /// \param points The list of point ranges.
  /// \return
  ///   True if the selection was modified.
  bool xorPoints(const QMap<int, vtkQtChartIndexRangeList> &points);

  /// \brief
  ///   Selects unique points from the given list and the selection.
  /// \param series The series index.
  /// \param indexes The list of point ranges.
  /// \return
  ///   True if the selection was modified.
  bool xorPoints(int series, const vtkQtChartIndexRangeList &indexes);

  /// \brief
  ///   Trims the selected point indexes for the given series.
  /// \param series The series index.
  /// \param minimum The minimum point index.
  /// \param maximum The maximum point index.
  void limitPoints(int series, int minimum, int maximum);
  //@}

private:
  vtkQtChartSeriesSelectionInternal *Internal; ///< Stores the selection lists.
};

#endif
