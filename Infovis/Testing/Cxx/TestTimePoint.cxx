/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestTimePoint.cxx

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

#include "vtkFieldData.h"
#include "vtkGraph.h"
#include "vtkPointData.h"
#include "vtkRandomGraphSource.h"
#include "vtkSmartPointer.h"
#include "vtkStringArray.h"
#include "vtkStringToTimePoint.h"
#include "vtkTable.h"
#include "vtkTimePointUtility.h"
#include "vtkTimePointToString.h"
#include "vtkUnsignedIntArray.h"
#include "vtkTypeUInt64Array.h"

#define VTK_CREATE(type,name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

int TestTimePoint(int, char*[])
{
  vtkIdType size = 100;
  int errors = 0;
  bool success = true;

  // Set a start time of December 31, 1999, 00:00:00.
  vtkTypeUInt32 dateBefore2000 = 2451544;
  vtkTypeUInt64 dateTimeBefore2000 = static_cast<vtkTypeUInt64>(dateBefore2000)*vtkTimePointUtility::MILLIS_PER_DAY;

  // Create a datetime array incrementing by hours
  VTK_CREATE(vtkTypeUInt64Array, dateTimeArray);
  for (vtkIdType i = 0; i < size; i++)
    {
    dateTimeArray->InsertNextValue(dateTimeBefore2000 + i*vtkTimePointUtility::MILLIS_PER_HOUR);
    }
  dateTimeArray->SetName("datetime");

  // Create a datetime array incrementing by days
  VTK_CREATE(vtkTypeUInt64Array, dateArray);
  for (vtkIdType i = 0; i < size; i++)
    {
    dateArray->InsertNextValue(dateTimeBefore2000 + i*static_cast<vtkTypeUInt64>(vtkTimePointUtility::MILLIS_PER_DAY));
    }
  dateArray->SetName("date");

  // Create a time array incrementing by minutes from noon
  VTK_CREATE(vtkTypeUInt64Array, timeArray);
  for (vtkIdType i = 0; i < size; i++)
    {
    timeArray->InsertNextValue(12*vtkTimePointUtility::MILLIS_PER_HOUR + i*vtkTimePointUtility::MILLIS_PER_MINUTE);
    }
  timeArray->SetName("time");

  vtkTable* tableOutput;
  vtkTable* tableOutput2;
  vtkStringArray* stringArray;
  vtkTypeUInt64Array* uint64Array;
  VTK_CREATE(vtkTimePointToString, timeToString);
  VTK_CREATE(vtkStringToTimePoint, stringToTime);

  // Create a table with datetime and date columns
  VTK_CREATE(vtkTable, table);
  table->AddColumn(dateTimeArray);
  table->AddColumn(dateArray);
  table->AddColumn(timeArray);

  cerr << "Testing vtkTimePointToString on a vtkTable with datetime array ..." << endl;
  timeToString->SetInput(table);
  timeToString->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_ROWS, "datetime");
  timeToString->SetOutputArrayName("datetime [to string]");
  timeToString->SetISO8601Format(vtkTimePointUtility::ISO8601_DATETIME_MILLIS);
  timeToString->Update();

  tableOutput = vtkTable::SafeDownCast(timeToString->GetOutput());
  stringArray = vtkStringArray::SafeDownCast(tableOutput->GetColumnByName("datetime [to string]"));
  if (!strcmp("2000-01-01T00:00:00.000", stringArray->GetValue(24).c_str()))
    {
    cerr << "... Success!" << endl;
    }
  else
    {
    cerr << "... Failed! 2000-01-01T00:00:00.000 != " << stringArray->GetValue(24) << endl;
    errors++;
    }

  cerr << "Converting string array back to a datetime ..." << endl;
  stringToTime->SetInput(tableOutput);
  stringToTime->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_ROWS, "datetime [to string]");
  stringToTime->SetOutputArrayName("datetime [to string] [to datetime]");
  stringToTime->Update();
  tableOutput2 = vtkTable::SafeDownCast(stringToTime->GetOutput());
  uint64Array = vtkTypeUInt64Array::SafeDownCast(tableOutput2->GetColumnByName("datetime [to string] [to datetime]"));
  success = true;
  for (vtkIdType i = 0; i < size; i++)
    {
    if (dateTimeArray->GetValue(i) != uint64Array->GetValue(i))
      {
      cerr << "... Failed! " << dateTimeArray->GetValue(i) << " != " << uint64Array->GetValue(i) << endl;
      success = false;
      errors++;
      break;
      }
    }
  if (success)
    {
    cerr << "... Success!" << endl;
    }

  cerr << "Testing vtkTimePointToString on a vtkTable with date array ..." << endl;
  timeToString->SetInput(table);
  timeToString->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_ROWS, "date");
  timeToString->SetISO8601Format(vtkTimePointUtility::ISO8601_DATE);
  timeToString->SetOutputArrayName("date [to string]");
  timeToString->Update();

  tableOutput = vtkTable::SafeDownCast(timeToString->GetOutput());
  stringArray = vtkStringArray::SafeDownCast(tableOutput->GetColumnByName("date [to string]"));
  if (!strcmp("2000-01-01", stringArray->GetValue(1).c_str()))
    {
    cerr << "... Success!" << endl;
    }
  else
    {
    cerr << "... Failed! 2000-01-01 != " << stringArray->GetValue(1) << endl;
    errors++;
    }

  cerr << "Converting string array back to a date ..." << endl;
  stringToTime->SetInput(tableOutput);
  stringToTime->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_ROWS, "date [to string]");
  stringToTime->SetOutputArrayName("date [to string] [to date]");
  stringToTime->Update();
  tableOutput2 = vtkTable::SafeDownCast(stringToTime->GetOutput());
  uint64Array = vtkTypeUInt64Array::SafeDownCast(tableOutput2->GetColumnByName("date [to string] [to date]"));
  success = true;
  for (vtkIdType i = 0; i < size; i++)
    {
    if (dateArray->GetValue(i) != uint64Array->GetValue(i))
      {
      cerr << "... Failed! " << dateArray->GetValue(i) << " != " << uint64Array->GetValue(i) << endl;
      success = false;
      errors++;
      break;
      }
    }
  if (success)
    {
    cerr << "... Success!" << endl;
    }

  cerr << "Testing vtkTimePointToString on a vtkTable with time array ..." << endl;
  timeToString->SetInput(table);
  timeToString->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_ROWS, "time");
  timeToString->SetISO8601Format(vtkTimePointUtility::ISO8601_TIME_MILLIS);
  timeToString->SetOutputArrayName("time [to string]");
  timeToString->Update();

  tableOutput = vtkTable::SafeDownCast(timeToString->GetOutput());
  stringArray = vtkStringArray::SafeDownCast(tableOutput->GetColumnByName("time [to string]"));
  if (!strcmp("12:30:00.000", stringArray->GetValue(30).c_str()))
    {
    cerr << "... Success!" << endl;
    }
  else
    {
    cerr << "... Failed! 12:30:00.000 != " << stringArray->GetValue(30) << endl;
    errors++;
    }

  cerr << "Converting string array back to a time ..." << endl;
  stringToTime->SetInput(tableOutput);
  stringToTime->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_ROWS, "time [to string]");
  stringToTime->SetOutputArrayName("time [to string] [to time]");
  stringToTime->Update();
  tableOutput2 = vtkTable::SafeDownCast(stringToTime->GetOutput());
  uint64Array = vtkTypeUInt64Array::SafeDownCast(tableOutput2->GetColumnByName("time [to string] [to time]"));
  success = true;
  for (vtkIdType i = 0; i < size; i++)
    {
    if (timeArray->GetValue(i) != uint64Array->GetValue(i))
      {
      cerr << "... Failed! " << timeArray->GetValue(i) << " != " << uint64Array->GetValue(i) << endl;
      success = false;
      errors++;
      break;
      }
    }
  if (success)
    {
    cerr << "... Success!" << endl;
    }

  cerr << "Testing vtkTimePointToString on a vtkGraph with datetime array, custom name ..." << endl;
  VTK_CREATE(vtkRandomGraphSource, graphSource);
  graphSource->SetNumberOfVertices(size);
  graphSource->SetStartWithTree(true);
  graphSource->Update();
  vtkGraph* graph = graphSource->GetOutput();
  graph->GetVertexData()->AddArray(dateTimeArray);

  timeToString->SetInput(graph);
  timeToString->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_VERTICES, "datetime");
  timeToString->SetOutputArrayName("datetime [to string]");
  timeToString->SetISO8601Format(vtkTimePointUtility::ISO8601_DATE);
  timeToString->Update();

  vtkGraph* outputGraph = vtkGraph::SafeDownCast(timeToString->GetOutput());
  vtkStringArray* graphStringArray = vtkStringArray::SafeDownCast(outputGraph->GetVertexData()->GetAbstractArray("datetime [to string]"));
  if (!strcmp("2000-01-01", graphStringArray->GetValue(24).c_str()))
    {
    cerr << "... Success!" << endl;
    }
  else
    {
    cerr << "... Failed! 2000-01-01 != " << graphStringArray->GetValue(24) << endl;
    errors++;
    }

  return errors;
}
