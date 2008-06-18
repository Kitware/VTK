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


class VTKQTCHART_EXPORT vtkQtChartSeriesSelectionItem
{
public:
  vtkQtChartSeriesSelectionItem();
  vtkQtChartSeriesSelectionItem(int series);
  vtkQtChartSeriesSelectionItem(const vtkQtChartSeriesSelectionItem &other);
  ~vtkQtChartSeriesSelectionItem() {}

  vtkQtChartSeriesSelectionItem &operator=(
      const vtkQtChartSeriesSelectionItem &other);

  int Series;
  vtkQtChartIndexRangeList Points;
};


class VTKQTCHART_EXPORT vtkQtChartSeriesSelection
{
public:
  enum SelectionType
    {
    NoSelection = 0,
    SeriesSelection,
    PointSelection
    };

public:
  vtkQtChartSeriesSelection();
  vtkQtChartSeriesSelection(const vtkQtChartSeriesSelection &other);
  ~vtkQtChartSeriesSelection() {}

  bool isEmpty() const;
  SelectionType getType() const;

  bool clear();

  /// \name Series Selection Methods
  //@{
  const vtkQtChartIndexRangeList &getSeries() const;
  bool setSeries(const vtkQtChartIndexRangeList &series);
  bool setSeries(const vtkQtChartIndexRange &series);
  bool addSeries(const vtkQtChartIndexRangeList &series);
  bool addSeries(const vtkQtChartIndexRange &series);
  bool subtractSeries(const vtkQtChartIndexRangeList &series);
  bool subtractSeries(const vtkQtChartIndexRange &series);
  bool xorSeries(const vtkQtChartIndexRangeList &series);
  bool xorSeries(const vtkQtChartIndexRange &series);
  //@}

  /// \name Point Selection Methods
  //@{
  const QList<vtkQtChartSeriesSelectionItem> &getPoints() const;
  bool setPoints(const QList<vtkQtChartSeriesSelectionItem> &points);
  bool addPoints(const QList<vtkQtChartSeriesSelectionItem> &points);
  bool subtractPoints(const QList<vtkQtChartSeriesSelectionItem> &points);
  bool subtractPoints(const vtkQtChartIndexRange &series);
  bool xorPoints(const QList<vtkQtChartSeriesSelectionItem> &points);
  //@}

  void limitSeries(int minimum, int maximum);
  QList<int> getPointSeries() const;
  void limitPoints(int series, int minimum, int maximum);

  vtkQtChartSeriesSelection &operator=(const vtkQtChartSeriesSelection &other);

private:
  bool addRanges(const vtkQtChartIndexRangeList &source,
      vtkQtChartIndexRangeList &target);
  bool subtractRanges(const vtkQtChartIndexRangeList &source,
      vtkQtChartIndexRangeList &target);
  void limitRanges(vtkQtChartIndexRangeList &list, int minimum, int maximum);

private:
  vtkQtChartIndexRangeList Series;
  QList<vtkQtChartSeriesSelectionItem> Points;
};

#endif
