/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtTimePointUtility.cxx

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

#include "vtkQtTimePointUtility.h"

#include "vtkObjectFactory.h"


QDateTime vtkQtTimePointUtility::TimePointToQDateTime(vtkTypeUInt64 time)
{
  int julianDay = time / 86400000;
  QDate qdate = QDate::fromJulianDay(julianDay);
  int hour = static_cast<int>(time % 86400000) / 3600000;
  int minute = static_cast<int>(time % 3600000) / 60000;
  int second = static_cast<int>(time % 60000) / 1000;
  int millis = static_cast<int>(time % 1000);
  QTime qtime(hour, minute, second, millis);
  QDateTime dt(qdate, qtime);
  return dt;
}

vtkTypeUInt64 vtkQtTimePointUtility::QDateTimeToTimePoint(QDateTime time)
{
  vtkTypeUInt64 timePoint = 
    QDateToTimePoint(time.date()) + QTimeToTimePoint(time.time());
  return timePoint;
}

vtkTypeUInt64 vtkQtTimePointUtility::QDateToTimePoint(QDate date)
{
  vtkTypeUInt64 timePoint = 
    static_cast<vtkTypeUInt64>(date.toJulianDay())*86400000;
  return timePoint;
}

vtkTypeUInt64 vtkQtTimePointUtility::QTimeToTimePoint(QTime time)
{
  vtkTypeUInt64 timePoint = 
    + time.hour()*3600000
    + time.minute()*60000
    + time.second()*1000
    + time.msec();
  return timePoint;
}
