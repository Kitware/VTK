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


class VTKQTCHART_EXPORT vtkQtChartSeriesDomainGroup
{
public:
  vtkQtChartSeriesDomainGroup(bool sortSeries=false);
  virtual ~vtkQtChartSeriesDomainGroup() {}

  int getNumberOfGroups() const;
  int getNumberOfSeries(int group) const;
  QList<int> getGroup(int group) const;

  int findGroup(int series) const;

  virtual void prepareInsert(int seriesFirst, int seriesLast);
  virtual void insertSeries(int series, int group);

  virtual int removeSeries(int series);
  virtual void finishRemoval(int seriesFirst=-1, int seriesLast=-1);

  virtual void clear();

protected:
  virtual void insertGroup(int group);
  virtual void removeGroup(int group);

private:
  QList<QList<int> > Groups;
  bool SortSeries;
};

#endif
