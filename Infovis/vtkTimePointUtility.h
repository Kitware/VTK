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
// .NAME vtkTimePointUtility - performs common time operations
// .SECTION Description
//
// vtkTimePointUtility is provides methods to perform common time operations.

#ifndef __vtkTimePointUtility_h
#define __vtkTimePointUtility_h

#include "vtkObject.h"
#include "vtkStdString.h" // For string conversions

class VTK_INFOVIS_EXPORT vtkTimePointUtility : public vtkObject
{
public:
  static vtkTimePointUtility *New();
  vtkTypeRevisionMacro(vtkTimePointUtility,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  static vtkTypeUInt64 DateToTimePoint(
    int year, int month, int day);

  // Description:
  static vtkTypeUInt64 TimeToTimePoint(
    int hour, int minute, int second, int millis = 0);

  // Description:
  static vtkTypeUInt64 DateTimeToTimePoint(
    int year, int month, int day, 
    int hour, int minute, int sec, int millis = 0);

  // Description:
  static void GetDate(vtkTypeUInt64 time, 
    int& year, int& month, int& day);

  // Description:
  static void GetTime(vtkTypeUInt64 time, 
    int& hour, int& minute, int& second, int& millis);

  // Description:
  static void GetDateTime(vtkTypeUInt64 time, 
    int& year, int& month, int& day, 
    int& hour, int& minute, int& second, int& millis);

  // Description:
  static int GetYear(vtkTypeUInt64 time);

  // Description:
  static int GetMonth(vtkTypeUInt64 time);

  // Description:
  static int GetDay(vtkTypeUInt64 time);

  // Description:
  static int GetHour(vtkTypeUInt64 time);

  // Description:
  static int GetMinute(vtkTypeUInt64 time);

  // Description:
  static int GetSecond(vtkTypeUInt64 time);

  // Description:
  static int GetMillisecond(vtkTypeUInt64 time);

  //BTX
  enum {
    ISO8601_DATETIME_MILLIS = 0,
    ISO8601_DATETIME = 1,
    ISO8601_DATE = 2,
    ISO8601_TIME_MILLIS = 3,
    ISO8601_TIME = 4
    };

  static const int MILLIS_PER_SECOND  =     1000;
  static const int MILLIS_PER_MINUTE  =    60000;
  static const int MILLIS_PER_HOUR    =  3600000;
  static const int MILLIS_PER_DAY     = 86400000;
  static const int SECONDS_PER_MINUTE =       60;
  static const int SECONDS_PER_HOUR   =     3600;
  static const int SECONDS_PER_DAY    =    86400;
  static const int MINUTES_PER_HOUR   =       60;
  static const int MINUTES_PER_DAY    =     1440;
  static const int HOURS_PER_DAY      =       24;
  //ETX

  // Description:
  // Converts a ISO8601 string into a VTK timepoint.
  // The string must follow one of the ISO8601 formats described
  // in ToISO8601.  To check for a valid format, pass a bool* as
  // the second argument.  The value will be set to true if the
  // string was parsed successfully, false otherwise.
  static vtkTypeUInt64 ISO8601ToTimePoint(vtkStdString str, bool* ok = NULL);

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
  static vtkStdString TimePointToISO8601(
    vtkTypeUInt64, int format = ISO8601_DATETIME_MILLIS);

protected:
  vtkTimePointUtility() {};
  ~vtkTimePointUtility() {};

private:
  vtkTimePointUtility(const vtkTimePointUtility&);  // Not implemented.
  void operator=(const vtkTimePointUtility&);  // Not implemented.
};

#endif
