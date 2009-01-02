/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartSeriesDomainGroup.h

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

/// \file vtkQtChartSeriesDomainGroup.h
/// \date March 6, 2008

#ifndef _vtkQtChartSeriesDomainGroup_h
#define _vtkQtChartSeriesDomainGroup_h

#include "vtkQtChartExport.h"
#include <QList> // needed for return type


/// \class vtkQtChartSeriesDomainGroup
/// \brief
///   The vtkQtChartSeriesDomainGroup class is used to group together
///   series with similar domains.
class VTKQTCHART_EXPORT vtkQtChartSeriesDomainGroup
{
public:
  /// \brief
  ///   Creates a chart series domain group.
  /// \param sortSeries True if the series should be sorted when added
  ///   to a group.
  vtkQtChartSeriesDomainGroup(bool sortSeries=false);
  virtual ~vtkQtChartSeriesDomainGroup() {}

  /// \brief
  ///   Gets the number of groups.
  /// \return
  ///   The number of groups.
  int getNumberOfGroups() const;

  /// \brief
  ///   Gets the number of series in the given group.
  /// \param group The group index.
  /// \return
  ///   The number of series in the group.
  int getNumberOfSeries(int group) const;

  /// \brief
  ///   Gets the list of series in the given group.
  /// \param group The group index.
  /// \return
  ///   The list of series in the given group.
  QList<int> getGroup(int group) const;

  /// \brief
  ///   Finds the group index for the given series.
  /// \param series The series to look up.
  /// \return
  ///   The group index of the series or -1 on failure.
  int findGroup(int series) const;

  /// \brief
  ///   Updates the series indexes prior to an insert.
  /// \param seriesFirst The first index to be inserted.
  /// \param seriesLast The last index to be inserted.
  virtual void prepareInsert(int seriesFirst, int seriesLast);

  /// \brief
  ///   Inserts a new series in the specified group.
  /// \param series The new series.
  /// \param group The group index.
  virtual void insertSeries(int series, int group);

  /// Sorts the newly inserted series if sorting is enabled.
  void finishInsert();

  /// \brief
  ///   Removes a series from its group.
  /// \param series The series to remove.
  /// \return
  ///   The series group index.
  virtual int removeSeries(int series);

  /// \brief
  ///   Updates the series indexes after a removal.
  /// \param seriesFirst The first index removed.
  /// \param seriesLast The last index removed.
  virtual void finishRemoval(int seriesFirst=-1, int seriesLast=-1);

  /// Removes all the series groups.
  virtual void clear();

public:
  /// \brief
  ///   Merges two sorted lists of series indexes.
  /// \param target The list where the result will be stored.
  /// \param source The list of seires to merge.
  static void mergeSeriesLists(QList<int> &target, const QList<int> &source);

protected:
  /// \brief
  ///   Inserts a new group in the list.
  ///
  /// Subclasses can override this method to set up data structures
  /// associated with the series group.
  ///
  /// \param group The group index.
  virtual void insertGroup(int group);

  /// \brief
  ///   Removes a group from the list.
  ///
  /// Subclasses should override this method to clean up any data
  /// structures associated with the series group.
  ///
  /// \param group The group index.
  virtual void removeGroup(int group);

private:
  QList<QList<int> > Groups; ///< Stores the series groups.
  QList<QList<int> > ToSort; ///< Stores the new series groups.
  bool SortSeries;           ///< True if series are sorted.
};

#endif
