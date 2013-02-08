/*=========================================================================

  Program:   Visualization Toolkit
  Module:    ObjectFactory.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkTimePointUtility.h"

#include "vtkSmartPointer.h"
#include "vtkMath.h"

#include <sstream>
#include <string>

static int TestSpecialDates();
static void TestBadFormatWarning();
static void TestPrintSelf();

int TestTimePointUtility(int, char *[])
{
  int testResult = EXIT_SUCCESS;

  // Test some randomw dates
  for (int n = 0; n < 10000; ++n)
    {
    int y = static_cast<int>(vtkMath::Random(1.0, 2020.0));
    int d = static_cast<int>(vtkMath::Random(1.0, 27.0));
    int m = static_cast<int>(vtkMath::Random(1.0, 12.0));

    // There is no year 0
    if (y == 0)
      {
      continue;
      }

    // The dates October 5, 1582 thru October 14, 1582 do not exist
    if (y == 1582)
      {
      if (m == 10)
        {
        if (d > 4 && d < 15)
          {
          continue;
          }
        }
      }

    // Compute time points and their ISO representations
    vtkTypeUInt64 tp1 = vtkTimePointUtility::DateToTimePoint(y, m, d);
    const char *iso0 = vtkTimePointUtility::TimePointToISO8601(tp1, 0);
    const char *iso1 = vtkTimePointUtility::TimePointToISO8601(tp1, 1);
    const char *iso2 = vtkTimePointUtility::TimePointToISO8601(tp1, 2);
    const char *iso3 = vtkTimePointUtility::TimePointToISO8601(tp1, 3);
    const char *iso4 = vtkTimePointUtility::TimePointToISO8601(tp1, 4);

    // Test to see if the converted data
    // [YYYY]-[MM]-[DD]T[hh]:[mm]:[ss]
    // can be parsed
    bool ok;
    std::string iso0copy(iso0);
    tp1 = vtkTimePointUtility::ISO8601ToTimePoint(iso0copy.c_str(), &ok);
    if (!ok)
      {
      std::cout << m << "/" << d << "/" << y << std::endl;
      std::cout << "FAILED to convert " << iso0copy
                << " to a time point. Return value: " << tp1
                << std::endl;
      testResult = EXIT_FAILURE;
      }

    tp1 = vtkTimePointUtility::ISO8601ToTimePoint(iso1, &ok);
    if (!ok)
      {
      std::cout << "FAILED to convert " << iso1
        << " to a time point. Return value: " << tp1
        << std::endl;
      testResult = EXIT_FAILURE;
      }
    tp1 = vtkTimePointUtility::ISO8601ToTimePoint(iso2, &ok);
    if (!ok)
      {
      std::cout << "FAILED to convert " << iso2
        << " to a time point. Return value: " << tp1
        << std::endl;
      testResult = EXIT_FAILURE;
      }
    tp1 = vtkTimePointUtility::ISO8601ToTimePoint(iso3, &ok);
    if (!ok)
      {
      std::cout << "FAILED to convert " << iso3
        << " to a time point. Return value: " << tp1
        << std::endl;
      testResult = EXIT_FAILURE;
      }
    tp1 = vtkTimePointUtility::ISO8601ToTimePoint(iso4, &ok);
    if (!ok)
      {
      std::cout << "FAILED to convert " << iso4
        << " to a time point. Return value: " << tp1
        << std::endl;
      testResult = EXIT_FAILURE;
      }

    // Test for invalid ISO dates
    iso0copy[4] = 'X';
    tp1 = vtkTimePointUtility::ISO8601ToTimePoint(iso0copy.c_str(), &ok);
    if (ok)
      {
      std::cout << "Should have FAILED to convert " << iso0copy
        << " to a time point. Return value: " << tp1
        << std::endl;
      testResult = EXIT_FAILURE;
      }

    iso0copy[4] = '-';
    iso0copy[10] = 'X';
    tp1 = vtkTimePointUtility::ISO8601ToTimePoint(iso0copy.c_str(), &ok);
    if (ok)
      {
      std::cout << "Should have FAILED to convert " << iso0copy
        << " to a time point. Return value: " << tp1
        << std::endl;
      testResult = EXIT_FAILURE;
      }

    iso0copy[10] = 'T';
    iso0copy[13] = 'X';
    tp1 = vtkTimePointUtility::ISO8601ToTimePoint(iso0copy.c_str(), &ok);
    if (ok)
      {
      std::cout << "Should have FAILED to convert " << iso0copy
        << " to a time point. Return value: " << tp1
        << std::endl;
      testResult = EXIT_FAILURE;
      }

    iso0copy[13] = ':';
    iso0copy[19] = 'X';
    tp1 = vtkTimePointUtility::ISO8601ToTimePoint(iso0copy.c_str(), &ok);
    if (ok)
      {
      std::cout << "Should have FAILED to convert " << iso0copy
        << " to a time point. Return value: " << tp1
        << std::endl;
      testResult = EXIT_FAILURE;
      }

    iso0copy[19] = '.';
    iso0copy[0] = 'X';
    tp1 = vtkTimePointUtility::ISO8601ToTimePoint(iso0copy.c_str(), &ok);
    if (ok)
      {
      std::cout << "Should have FAILED to convert " << iso0copy
        << " to a time point. Return value: " << tp1
        << std::endl;
      testResult = EXIT_FAILURE;
      }

    // Test to see if the converted data
    // [YYYY]-[MM]-[DD]
    // can be parsed

    std::string iso2copy(iso2);

    iso2copy[4] = 'X';
    tp1 = vtkTimePointUtility::ISO8601ToTimePoint(iso2copy.c_str(), &ok);
    if (ok)
      {
      std::cout << "Should have FAILED to convert " << iso2copy
        << " to a time point. Return value: " << tp1
        << std::endl;
      testResult = EXIT_FAILURE;
      }

    iso2copy[4] = '-';
    iso2copy[5] = 'X';
    tp1 = vtkTimePointUtility::ISO8601ToTimePoint(iso2copy.c_str(), &ok);
    if (ok)
      {
      std::cout << "Should have FAILED to convert " << iso2copy
        << " to a time point. Return value: " << tp1
        << std::endl;
      testResult = EXIT_FAILURE;
      }

    // Test to see if the converted data
    // [hh]:[mm]:[ss].[SSS]
    // can be parsed

    std::string iso3copy(iso3);

    iso3copy[2] = 'X';
    tp1 = vtkTimePointUtility::ISO8601ToTimePoint(iso3copy.c_str(), &ok);
    if (ok)
      {
      std::cout << "Should have FAILED to convert " << iso3copy
        << " to a time point. Return value: " << tp1
        << std::endl;
      testResult = EXIT_FAILURE;
      }

    iso3copy[2] = ':';
    iso3copy[8] = 'X';
    tp1 = vtkTimePointUtility::ISO8601ToTimePoint(iso3copy.c_str(), &ok);
    if (ok)
      {
      std::cout << "Should have FAILED to convert " << iso3copy
        << " to a time point. Return value: " << tp1
        << std::endl;
      testResult = EXIT_FAILURE;
      }

    iso3copy[8] = '.';
    iso3copy[7] = 'X';
    tp1 = vtkTimePointUtility::ISO8601ToTimePoint(iso3copy.c_str(), &ok);
    if (ok)
      {
      std::cout << "Should have FAILED to convert " << iso3copy
        << " to a time point. Return value: " << tp1
        << std::endl;
      testResult = EXIT_FAILURE;
      }

    int year, month, day;
    tp1 = vtkTimePointUtility::ISO8601ToTimePoint(iso0, &ok);
    vtkTimePointUtility::GetDate(tp1, year, month, day);

    // Test to see if individual components are correct
    int year2, month2, day2;
    year2 = vtkTimePointUtility::GetYear(tp1);
    if (year != year2)
      {
      std::cout << "GetYear() returned " << year2
                << ", but expected "<< year << std::endl;
      testResult = EXIT_FAILURE;
      }
    month2 = vtkTimePointUtility::GetMonth(tp1);
    if (month != month2)
      {
      std::cout << "GetMonth() returned " << month2
                << ", but expected "<< month << std::endl;
      testResult = EXIT_FAILURE;
      }
    day2 = vtkTimePointUtility::GetDay(tp1);
    if (day != day2)
      {
      std::cout << "GetDay() returned " << day2
                << ", but expected "<< day << std::endl;
      testResult = EXIT_FAILURE;
      }

    int hour, minute, second, millis;
    vtkTimePointUtility::GetTime(tp1, hour, minute, second, millis);

    int hour2, minute2, second2, millis2;
    hour2 = vtkTimePointUtility::GetHour(tp1);
    if (hour != hour2)
      {
      std::cout << "GetHour() returned " << hour2
                << ", but expected "<< hour << std::endl;
      testResult = EXIT_FAILURE;
      }
    minute2 = vtkTimePointUtility::GetMinute(tp1);
    if (minute != minute2)
      {
      std::cout << "GetMinute() returned "
                << minute2 << ", but expected "<< minute << std::endl;
      testResult = EXIT_FAILURE;
      }
    second2 = vtkTimePointUtility::GetSecond(tp1);
    if (second != second2)
      {
      std::cout << "GetSecond() returned "
                << second2 << ", but expected "<< second << std::endl;
      testResult = EXIT_FAILURE;
      }
    millis2 = vtkTimePointUtility::GetMillisecond(tp1);
    if (millis != millis2)
      {
      std::cout << "GetMillisecond() returned "
                << millis2 << ", but expected "<< millis << std::endl;
      testResult = EXIT_FAILURE;
      }

    delete []iso0;
    delete []iso1;
    delete []iso2;
    delete []iso3;
    delete []iso4;
    }

  if (TestSpecialDates() != EXIT_SUCCESS)
    {
    testResult = EXIT_FAILURE;
    }

  TestBadFormatWarning();

  TestPrintSelf();

  return testResult;
}

void TestPrintSelf()
{
  vtkSmartPointer<vtkTimePointUtility> tpu =
    vtkSmartPointer<vtkTimePointUtility>::New();
  std::cout << "Verify PrintSelf...";
  std::ostringstream pout;
  std::cout << "PASSED" << std::endl;
  tpu->Print(pout);
}

void TestBadFormatWarning()
{
  vtkTypeUInt64 tp2 = vtkTimePointUtility::DateTimeToTimePoint
    (1946, 11, 8, 0, 0, 0, 0);

  std::cout << "******* Expected warning starts *****" << std::endl;
  vtkSmartPointer<vtkTimePointUtility> tpu =
    vtkSmartPointer<vtkTimePointUtility>::New();
  const char *iso5 = tpu->TimePointToISO8601(tp2, 10);
  std::cout << "******* Expected warning ends *****" << std::endl;

  delete []iso5;
}

int TestSpecialDates()
{
  int testResult = EXIT_SUCCESS;

  // Test some special dates
  // The expected values were computed here:
  // http://bowie.gsfc.nasa.gov/time/julian.html
  vtkTypeUInt64 expectedJD;

  expectedJD = 2432133;
  vtkTypeUInt64 tp1 =
    vtkTimePointUtility::DateTimeToTimePoint(1946, 11, 8, 0, 0, 0, 0);
  vtkTypeUInt64 jd1 = tp1 / vtkTimePointUtility::MILLIS_PER_DAY;
  const char *iso6 = vtkTimePointUtility::TimePointToISO8601(tp1, 0);
  std::cout << "Julian Day for this historic date 11/8/1946 is: "
            << jd1
            << " and its representation is "
            << iso6
            << std::endl;
  delete []iso6;

  if (jd1 != expectedJD)
    {
    std::cout << "ERROR: The computed Julian Day is " << jd1
              << " but the expected day is: " << expectedJD << std::endl;
    testResult = EXIT_FAILURE;
    }

  expectedJD = 2299160;
  vtkTypeUInt64 tp2 =
    vtkTimePointUtility::DateTimeToTimePoint(1582, 10, 4, 0, 0, 0, 0);
  vtkTypeUInt64 jd2 = tp2 / vtkTimePointUtility::MILLIS_PER_DAY;

  const char *iso7 = vtkTimePointUtility::TimePointToISO8601(tp2, 0);
  std::cout << "Julian Day for this historic date 10/4/1582 is: "
            << jd2
            << " and its representation is "
            << iso7
            << std::endl;
  delete []iso7;

  if (jd2 != expectedJD)
    {
    std::cout << "ERROR: The computed Julian Day is " << jd2
              << " but the expected day is: " << expectedJD << std::endl;
    testResult = EXIT_FAILURE;
    }

  expectedJD = 2299161;
  vtkTypeUInt64 tp3 =
    vtkTimePointUtility::DateTimeToTimePoint(1582, 10, 15, 0, 0, 0, 0);
  vtkTypeUInt64 jd3 = tp3 / vtkTimePointUtility::MILLIS_PER_DAY;

  const char *iso8 = vtkTimePointUtility::TimePointToISO8601(tp3, 0);
  std::cout << "Julian Day for this historic date 10/15/1582 is: "
            << jd3
            << " and its representation is "
            << iso8
            << std::endl;
  delete []iso8;
  if (jd3 != expectedJD)
    {
    std::cout << "ERROR: The computed Julian Day is " << jd3
              << " but the expected day is: " << expectedJD << std::endl;
    testResult = EXIT_FAILURE;
    }

  expectedJD = 0;
  vtkTypeUInt64 tp4 =
    vtkTimePointUtility::DateTimeToTimePoint(-4713, 1, 1, 0, 0, 0, 0);
  vtkTypeUInt64 jd4 = tp4 / vtkTimePointUtility::MILLIS_PER_DAY;

  std::cout << "Julian Day for the beginning of time 1/1/4713 BC is: "
            << tp4
            << std::endl;
  if (jd4 != expectedJD)
    {
    std::cout << "ERROR: The computed Julian Day is " << jd4
              << " but the expected day is: " << expectedJD << std::endl;
    testResult = EXIT_FAILURE;
    }

  return testResult;
}
