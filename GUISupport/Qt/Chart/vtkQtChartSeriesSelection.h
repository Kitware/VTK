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
#include <QList> // needed for typedef
#include <QPair> // needed for typedef

typedef QList<QPair<int, int> > vtkQtChartIndexRangeList;
typedef QPair<int, int> vtkQtChartIndexRange;


/// \class vtkQtChartSeriesSelectionItem
/// \brief
///   The vtkQtChartSeriesSelectionItem class stores a list of index
///   ranges.
class VTKQTCHART_EXPORT vtkQtChartSeriesSelectionItem
{
public:
  vtkQtChartSeriesSelectionItem();
  vtkQtChartSeriesSelectionItem(int series);
  vtkQtChartSeriesSelectionItem(const vtkQtChartSeriesSelectionItem &other);
  ~vtkQtChartSeriesSelectionItem() {}

  vtkQtChartSeriesSelectionItem &operator=(
      const vtkQtChartSeriesSelectionItem &other);

  int Series;                      ///< Stores the series index.
  vtkQtChartIndexRangeList Points; ///< Stores the list of ranges.
};


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
  ~vtkQtChartSeriesSelection() {}

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
  /// \param series The series index range to select.
  /// \return
  ///   True if the selection was modified.
  bool setSeries(const vtkQtChartIndexRange &series);

  /// \brief
  ///   Adds the list of series ranges to the selection.
  /// \param series The list of selected series ranges to add.
  /// \return
  ///   True if the selection was modified.
  bool addSeries(const vtkQtChartIndexRangeList &series);

  /// \brief
  ///   Adds the series index range to the selection.
  /// \param series The series index range to add.
  /// \return
  ///   True if the selection was modified.
  bool addSeries(const vtkQtChartIndexRange &series);

  /// \brief
  ///   Subtracts the list of series ranges from the selection.
  /// \param series The list of selected series ranges to subtract.
  /// \return
  ///   True if the selection was modified.
  bool subtractSeries(const vtkQtChartIndexRangeList &series);

  /// \brief
  ///   Subtracts the series index range from the selection.
  /// \param series The series index range to subtract.
  /// \return
  ///   True if the selection was modified.
  bool subtractSeries(const vtkQtChartIndexRange &series);

  /// \brief
  ///   Selects unique series from the given list and the selection.
  /// \param series The list of series ranges.
  /// \return
  ///   True if the selection was modified.
  bool xorSeries(const vtkQtChartIndexRangeList &series);

  /// \brief
  ///   Selects unique series from the given range and the selection.
  /// \param series The series index range.
  /// \return
  ///   True if the selection was modified.
  bool xorSeries(const vtkQtChartIndexRange &series);

  /// \brief
  ///   Trims the selected series to the given bounds.
  /// \param minimum The minimum series index.
  /// \param maximum The maximum series index.
  void limitSeries(int minimum, int maximum);
  //@}

  /// \name Point Selection Methods
  //@{
  /// \brief
  ///   Gets the list of selected point ranges.
  /// \return
  ///   A reference to the list of selected point ranges.
  const QList<vtkQtChartSeriesSelectionItem> &getPoints() const;

  /// \brief
  ///   Sets the list of selected point ranges.
  /// \param points The new list of selected point ranges.
  /// \return
  ///   True if the selection was modified.
  bool setPoints(const QList<vtkQtChartSeriesSelectionItem> &points);

  /// \brief
  ///   Adds the list of point ranges to the selection.
  /// \param points The list of selected point ranges to add.
  /// \return
  ///   True if the selection was modified.
  bool addPoints(const QList<vtkQtChartSeriesSelectionItem> &points);

  /// \brief
  ///   Subtracts the list of point ranges from the selection.
  /// \param points The list of selected point ranges to subtract.
  /// \return
  ///   True if the selection was modified.
  bool subtractPoints(const QList<vtkQtChartSeriesSelectionItem> &points);

  /// \brief
  ///   Subtracts all the selected points in the given series from
  ///   the selection.
  /// \param series The list of series ranges to subtract.
  /// \return
  ///   True if the selection was modified.
  bool subtractPoints(const vtkQtChartIndexRange &series);

  /// \brief
  ///   Selects unique points from the given list and the selection.
  /// \param points The list of point ranges.
  /// \return
  ///   True if the selection was modified.
  bool xorPoints(const QList<vtkQtChartSeriesSelectionItem> &points);

  /// \brief
  ///   Gets the list of series that have selected points.
  /// \return
  ///   The list of series that have selected points.
  QList<int> getPointSeries() const;

  /// \brief
  ///   Trims the selected point indexes for the given series.
  /// \param series The series index.
  /// \param minimum The minimum point index.
  /// \param maximum The maximum point index.
  void limitPoints(int series, int minimum, int maximum);
  //@}

  vtkQtChartSeriesSelection &operator=(const vtkQtChartSeriesSelection &other);

private:
  /// \brief
  ///   Adds the source ranges to the target ranges.
  /// \param source The list of index ranges to add.
  /// \param target The list of index ranges to be added to.
  /// \return
  ///   True if the target list was modified in the union.
  bool addRanges(const vtkQtChartIndexRangeList &source,
      vtkQtChartIndexRangeList &target);

  /// \brief
  ///   Subtracts the source ranges from the target ranges.
  /// \param source The list of index ranges to subtract.
  /// \param target The list of index ranges to be subtracted from.
  /// \return
  ///   True if the target list was modified in the subtraction.
  bool subtractRanges(const vtkQtChartIndexRangeList &source,
      vtkQtChartIndexRangeList &target);

  /// \brief
  ///   Trims the index ranges to be within the given bounds.
  /// \param list The index ranges to limit.
  /// \param minimum The minimum index.
  /// \param maximum The maximum index.
  void limitRanges(vtkQtChartIndexRangeList &list, int minimum, int maximum);

private:
  vtkQtChartIndexRangeList Series;             ///< Stores the selected series.
  QList<vtkQtChartSeriesSelectionItem> Points; ///< Stores the selected points.
};

#endif
