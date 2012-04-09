/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTimePointUtility.cxx

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

#include "vtkTimePointUtility.h"

#include "vtkObjectFactory.h"
#include "vtkStdString.h"

#include "vtksys/ios/sstream"

#if defined (__BORLANDC__) && (__BORLANDC__ >= 0x0550)
#include <ctype.h> // for isdigit
#endif

#include <locale> // C++ locale


const int vtkTimePointUtility::MILLIS_PER_SECOND  =     1000;
const int vtkTimePointUtility::MILLIS_PER_MINUTE  =    60000;
const int vtkTimePointUtility::MILLIS_PER_HOUR    =  3600000;
const int vtkTimePointUtility::MILLIS_PER_DAY     = 86400000;
const int vtkTimePointUtility::SECONDS_PER_MINUTE =       60;
const int vtkTimePointUtility::SECONDS_PER_HOUR   =     3600;
const int vtkTimePointUtility::SECONDS_PER_DAY    =    86400;
const int vtkTimePointUtility::MINUTES_PER_HOUR   =       60;
const int vtkTimePointUtility::MINUTES_PER_DAY    =     1440;
const int vtkTimePointUtility::HOURS_PER_DAY      =       24;

vtkStandardNewMacro(vtkTimePointUtility);

void vtkTimePointUtility::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

vtkTypeUInt64 vtkTimePointUtility::DateToTimePoint(
  int year, int month, int day)
{
  if (year < 0)
    {
    ++year;
    }

  vtkTypeUInt64 julianDay;
  if (year > 1582 || (year == 1582 && (month > 10 || (month == 10 && day >= 15))))
    {
    // Gregorian calendar starting from October 15, 1582
    // Algorithm from Henry F. Fliegel and Thomas C. Van Flandern
    julianDay = (1461 * (year + 4800 + (month - 14) / 12)) / 4
       + (367 * (month - 2 - 12 * ((month - 14) / 12))) / 12
       - (3 * ((year + 4900 + (month - 14) / 12) / 100)) / 4
       + day - 32075;
    }
  else if (year < 1582 || (year == 1582 && (month < 10 || (month == 10 && day <= 4))))
    {
    // Julian calendar until October 4, 1582
    // Algorithm from Frequently Asked Questions about Calendars by Claus Toendering
    int a = (14 - month) / 12;
    julianDay = (153 * (month + (12 * a) - 3) + 2) / 5
       + (1461 * (year + 4800 - a)) / 4
       + day - 32083;
    }
  else
    {
    // the day following October 4, 1582 is October 15, 1582
    julianDay = 0;
    }
  return julianDay * MILLIS_PER_DAY;
}

vtkTypeUInt64 vtkTimePointUtility::TimeToTimePoint(
  int hour, int minute, int second, int millis)
{
  return MILLIS_PER_HOUR*hour +
    MILLIS_PER_MINUTE*minute +
    MILLIS_PER_SECOND*second + millis;
}

vtkTypeUInt64 vtkTimePointUtility::DateTimeToTimePoint(
  int year, int month, int day,
  int hour, int minute, int second, int millis)
{
  return DateToTimePoint(year, month, day) +
    TimeToTimePoint(hour, minute, second, millis);
}

void vtkTimePointUtility::GetDate(vtkTypeUInt64 time,
  int& year, int& month, int& day)
{
  int y, m, d;
  int julianDay = static_cast<int>(time / MILLIS_PER_DAY);

  if (julianDay >= 2299161)
    {
    // Gregorian calendar starting from October 15, 1582
    // This algorithm is from Henry F. Fliegel and Thomas C. Van Flandern
    int ell, n, i, j;
    ell = julianDay + 68569;
    n = (4 * ell) / 146097;
    ell = ell - (146097 * n + 3) / 4;
    i = (4000 * (ell + 1)) / 1461001;
    ell = ell - (1461 * i) / 4 + 31;
    j = (80 * ell) / 2447;
    d = ell - (2447 * j) / 80;
    ell = j / 11;
    m = j + 2 - (12 * ell);
    y = 100 * (n - 49) + i + ell;
    }
  else
    {
    // Julian calendar until October 4, 1582
    // Algorithm from Frequently Asked Questions about Calendars by Claus Toendering
    julianDay += 32082;
    int dd = (4 * julianDay + 3) / 1461;
    int ee = julianDay - (1461 * dd) / 4;
    int mm = ((5 * ee) + 2) / 153;
    d = ee - (153 * mm + 2) / 5 + 1;
    m = mm + 3 - 12 * (mm / 10);
    y = dd - 4800 + (mm / 10);
    if (y <= 0)
      {
      --y;
      }
    }

  year = y;
  month = m;
  day = d;
}

void vtkTimePointUtility::GetTime(vtkTypeUInt64 time,
  int& hour, int& minute, int& second, int& millis)
{
  hour = static_cast<int>(time % MILLIS_PER_DAY) / MILLIS_PER_HOUR;
  minute = static_cast<int>(time % MILLIS_PER_HOUR) / MILLIS_PER_MINUTE;
  second = static_cast<int>(time % MILLIS_PER_MINUTE) / MILLIS_PER_SECOND;
  millis = static_cast<int>(time % MILLIS_PER_SECOND);
}

void vtkTimePointUtility::GetDateTime(vtkTypeUInt64 time,
  int& year, int& month, int& day,
  int& hour, int& minute, int& second, int& millis)
{
  GetDate(time, year, month, day);
  GetTime(time, hour, minute, second, millis);
}

int vtkTimePointUtility::GetYear(vtkTypeUInt64 time)
{
  int year, month, day;
  GetDate(time, year, month, day);
  return year;
}

int vtkTimePointUtility::GetMonth(vtkTypeUInt64 time)
{
  int year, month, day;
  GetDate(time, year, month, day);
  return month;
}

int vtkTimePointUtility::GetDay(vtkTypeUInt64 time)
{
  int year, month, day;
  GetDate(time, year, month, day);
  return day;
}

int vtkTimePointUtility::GetHour(vtkTypeUInt64 time)
{
  return static_cast<int>(time % MILLIS_PER_DAY) / MILLIS_PER_HOUR;
}

int vtkTimePointUtility::GetMinute(vtkTypeUInt64 time)
{
  return static_cast<int>(time % MILLIS_PER_HOUR) / MILLIS_PER_MINUTE;
}

int vtkTimePointUtility::GetSecond(vtkTypeUInt64 time)
{
  return static_cast<int>(time % MILLIS_PER_MINUTE) / MILLIS_PER_SECOND;
}

int vtkTimePointUtility::GetMillisecond(vtkTypeUInt64 time)
{
  return static_cast<int>(time % MILLIS_PER_SECOND);
}

vtkTypeUInt64 vtkTimePointUtility::ISO8601ToTimePoint(const char* cstr, bool* ok)
{
  bool formatValid = true;
  vtkTypeUInt64 value = 0;

  vtkStdString str(cstr);

  if (str.length() == 19 || str.length() == 23)
    {
    // Format is [YYYY]-[MM]-[DD]T[hh]:[mm]:[ss]
    // Index:     0123 4 56 7 89 0 12 3 45 6 78
    // -OR-
    // Format is [YYYY]-[MM]-[DD]T[hh]:[mm]:[ss].[SSS]
    // Index:     0123 4 56 7 89 0 12 3 45 6 78 9 012
    for (vtkStdString::size_type c = 0; c < str.length(); c++)
      {
      if (c == 4 || c == 7)
        {
        if (str.at(c) != '-')
          {
          formatValid = false;
          break;
          }
        }
      else if (c == 10)
        {
        if (str.at(c) != 'T' && str.at(c) != ' ')
          {
          formatValid = false;
          break;
          }
        }
      else if (c == 13 || c == 16)
        {
        if (str.at(c) != ':')
          {
          formatValid = false;
          break;
          }
        }
      else if (c == 19)
        {
        if (str.at(c) != '.')
          {
          formatValid = false;
          break;
          }
        }
      else if (!isdigit(str.at(c)))
        {
        formatValid = false;
        break;
        }
      }
    if (formatValid)
      {
      int year = atoi(str.substr(0, 4).c_str());
      int month = atoi(str.substr(5, 2).c_str());
      int day = atoi(str.substr(8, 2).c_str());
      int hour = atoi(str.substr(11, 2).c_str());
      int minute = atoi(str.substr(14, 2).c_str());
      int second = atoi(str.substr(17, 2).c_str());
      int msec = 0;
      if (str.length() == 23)
        {
        msec = atoi(str.substr(20, 3).c_str());
        }
      value = DateTimeToTimePoint(year, month, day, hour, minute, second, msec);
      }
    }
  else if (str.length() == 10)
    {
    // Format is [YYYY]-[MM]-[DD]
    // Index:     0123 4 56 7 89
    for (vtkStdString::size_type c = 0; c < str.length(); c++)
      {
      if (c == 4 || c == 7)
        {
        if (str.at(c) != '-')
          {
          formatValid = false;
          break;
          }
        }
      else if (!isdigit(str.at(c)))
        {
        formatValid = false;
        break;
        }
      }
    if (formatValid)
      {
      int year = atoi(str.substr(0, 4).c_str());
      int month = atoi(str.substr(5, 2).c_str());
      int day = atoi(str.substr(8, 2).c_str());
      value = DateToTimePoint(year, month, day);
      }
    }
  else if (str.length() == 8 || str.length() == 12)
    {
    // Format is [hh]:[mm]:[ss]
    // Index:     01 2 34 5 67
    // -OR-
    // Format is [hh]:[mm]:[ss].[SSS]
    // Index:     01 2 34 5 67 8 901
    for (vtkStdString::size_type c = 0; c < str.length(); c++)
      {
      if (c == 2 || c == 5)
        {
        if (str.at(c) != ':')
          {
          formatValid = false;
          break;
          }
        }
      else if (c == 8)
        {
        if (str.at(c) != '.')
          {
          formatValid = false;
          break;
          }
        }
      else if (!isdigit(str.at(c)))
        {
        formatValid = false;
        break;
        }
      }
    if (formatValid)
      {
      int hour = atoi(str.substr(0, 2).c_str());
      int minute = atoi(str.substr(3, 2).c_str());
      int second = atoi(str.substr(6, 2).c_str());
      int msec = 0;
      if (str.length() == 12)
        {
        msec = atoi(str.substr(9, 3).c_str());
        }
      value = TimeToTimePoint(hour, minute, second, msec);
      }
    }
  else
    {
    formatValid = false;
    }

  if (ok != NULL)
    {
    *ok = formatValid;
    }
  return value;
}

const char* vtkTimePointUtility::TimePointToISO8601(vtkTypeUInt64 time, int format)
{
  int year, month, day, hour, minute, second, msec;
  GetDateTime(time, year, month, day, hour, minute, second, msec);

  vtksys_ios::ostringstream oss;
  oss.imbue(std::locale::classic());
  oss.fill('0');
  if (format == ISO8601_DATETIME)
    {
    oss.width(4);
    oss << year << '-';
    oss.width(2);
    oss << month << '-';
    oss.width(2);
    oss << day << 'T';
    oss.width(2);
    oss << hour << ':';
    oss.width(2);
    oss << minute << ':';
    oss.width(2);
    oss << second;
    }
  else if (format == ISO8601_DATETIME_MILLIS)
    {
    oss.width(4);
    oss << year << '-';
    oss.width(2);
    oss << month << '-';
    oss.width(2);
    oss << day << 'T';
    oss.width(2);
    oss << hour << ':';
    oss.width(2);
    oss << minute << ':';
    oss.width(2);
    oss << second << '.';
    oss.width(3);
    oss << msec;
    }
  else if (format == ISO8601_DATE)
    {
    oss.width(4);
    oss << year << '-';
    oss.width(2);
    oss << month << '-';
    oss.width(2);
    oss << day;
    }
  else if (format == ISO8601_TIME)
    {
    oss.width(2);
    oss << hour << ':';
    oss.width(2);
    oss << minute << ':';
    oss.width(2);
    oss << second;
    }
  else if (format == ISO8601_TIME_MILLIS)
    {
    oss.width(2);
    oss << hour << ':';
    oss.width(2);
    oss << minute << ':';
    oss.width(2);
    oss << second << ".";
    oss.width(3);
    oss << msec;
    }
  else
    {
    vtkGenericWarningMacro(<< "Format undefined.");
    return 0;
    }
  char* copy = new char[25];
  strcpy(copy, oss.str().c_str());
  return copy;
}
