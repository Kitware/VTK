// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov

#include "vtkQtTimePointUtility.h"

#include "vtkObjectFactory.h"

VTK_ABI_NAMESPACE_BEGIN
void vtkQtTimePointUtility::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

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
  vtkTypeUInt64 timePoint = QDateToTimePoint(time.date()) + QTimeToTimePoint(time.time());
  return timePoint;
}

vtkTypeUInt64 vtkQtTimePointUtility::QDateToTimePoint(QDate date)
{
  vtkTypeUInt64 timePoint = static_cast<vtkTypeUInt64>(date.toJulianDay()) * 86400000;
  return timePoint;
}

vtkTypeUInt64 vtkQtTimePointUtility::QTimeToTimePoint(QTime time)
{
  vtkTypeUInt64 timePoint =
    +time.hour() * 3600000 + time.minute() * 60000 + time.second() * 1000 + time.msec();
  return timePoint;
}
VTK_ABI_NAMESPACE_END
