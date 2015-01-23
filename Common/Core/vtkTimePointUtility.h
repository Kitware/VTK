/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTimePointUtility.h

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
// .NAME vtkTimePointUtility - performs common time operations
// .SECTION Description
//
// vtkTimePointUtility is provides methods to perform common time operations.

#ifndef vtkTimePointUtility_h
#define vtkTimePointUtility_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkObject.h"

class VTKCOMMONCORE_EXPORT vtkTimePointUtility : public vtkObject
{
public:
  static vtkTimePointUtility *New();
  vtkTypeMacro(vtkTimePointUtility,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Return the time point for 12:00am on a specified day.
  static vtkTypeUInt64 DateToTimePoint(
    int year, int month, int day);

  // Description:
  // Return the time point for a time of day (the number of milliseconds from 12:00am.
  // The hour should be from 0-23.
  static vtkTypeUInt64 TimeToTimePoint(
    int hour, int minute, int second, int millis = 0);

  // Description:
  // Return the time point for a date and time.
  static vtkTypeUInt64 DateTimeToTimePoint(
    int year, int month, int day,
    int hour, int minute, int sec, int millis = 0);

  // Description:
  // Retrieve the year, month, and day of a time point.
  // Everything but the first argument are output parameters.
  static void GetDate(vtkTypeUInt64 time,
    int& year, int& month, int& day);

  // Description:
  // Retrieve the hour, minute, second, and milliseconds of a time point.
  // Everything but the first argument are output parameters.
  static void GetTime(vtkTypeUInt64 time,
    int& hour, int& minute, int& second, int& millis);

  // Description:
  // Retrieve the date and time of a time point.
  // Everything but the first argument are output parameters.
  static void GetDateTime(vtkTypeUInt64 time,
    int& year, int& month, int& day,
    int& hour, int& minute, int& second, int& millis);

  // Description:
  // Retrieve the year from a time point.
  static int GetYear(vtkTypeUInt64 time);

  // Description:
  // Retrieve the month from a time point.
  static int GetMonth(vtkTypeUInt64 time);

  // Description:
  // Retrieve the day of the month from a time point.
  static int GetDay(vtkTypeUInt64 time);

  // Description:
  // Retrieve the hour of the day from the time point.
  static int GetHour(vtkTypeUInt64 time);

  // Description:
  // Retrieve the number of minutes from the start of the last hour.
  static int GetMinute(vtkTypeUInt64 time);

  // Description:
  // Retrieve the number of seconds from the start of the last minute.
  static int GetSecond(vtkTypeUInt64 time);

  // Description:
  // Retrieve the milliseconds from the start of the last second.
  static int GetMillisecond(vtkTypeUInt64 time);

  //BTX
  enum {
    ISO8601_DATETIME_MILLIS = 0,
    ISO8601_DATETIME = 1,
    ISO8601_DATE = 2,
    ISO8601_TIME_MILLIS = 3,
    ISO8601_TIME = 4
    };

  static const int MILLIS_PER_SECOND;
  static const int MILLIS_PER_MINUTE;
  static const int MILLIS_PER_HOUR;
  static const int MILLIS_PER_DAY;
  static const int SECONDS_PER_MINUTE;
  static const int SECONDS_PER_HOUR;
  static const int SECONDS_PER_DAY;
  static const int MINUTES_PER_HOUR;
  static const int MINUTES_PER_DAY;
  static const int HOURS_PER_DAY;

  // Description:
  // Converts a ISO8601 string into a VTK timepoint.
  // The string must follow one of the ISO8601 formats described
  // in ToISO8601.  To check for a valid format, pass a bool* as
  // the second argument.  The value will be set to true if the
  // string was parsed successfully, false otherwise.
  static vtkTypeUInt64 ISO8601ToTimePoint(const char* str, bool* ok = NULL);
  //ETX

  // Description:
  // Converts a VTK timepoint into one of the following ISO8601
  // formats.  The default format is ISO8601_DATETIME_MILLIS.
  //
  // <PRE>
  // Type                      Format / Example
  // 0 ISO8601_DATETIME_MILLIS [YYYY]-[MM]-[DD]T[hh]:[mm]:[ss].[SSS]
  //                           2006-01-02T03:04:05.678
  // 1 ISO8601_DATETIME        [YYYY]-[MM]-[DD]T[hh]:[mm]:[ss]
  //                           2006-01-02T03:04:05
  // 2 ISO8601_DATE            [YYYY]-[MM]-[DD]
  //                           2006-01-02
  // 3 ISO8601_TIME_MILLIS     [hh]:[mm]:[ss].[SSS]
  //                           03:04:05.678
  // 4 ISO8601_TIME            [hh]:[mm]:[ss]
  //                           03:04:05
  // </PRE>
  static const char* TimePointToISO8601(
    vtkTypeUInt64, int format = ISO8601_DATETIME_MILLIS);

protected:
  vtkTimePointUtility() {}
  ~vtkTimePointUtility() {}

private:
  vtkTimePointUtility(const vtkTimePointUtility&);  // Not implemented.
  void operator=(const vtkTimePointUtility&);  // Not implemented.
};

#endif
