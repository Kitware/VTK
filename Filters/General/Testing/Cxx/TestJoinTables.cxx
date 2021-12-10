/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestJoinTables.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkJoinTables.h"

#include "vtkCellArray.h"
#include "vtkExecutive.h"
#include "vtkFloatArray.h"
#include "vtkIntArray.h"
#include "vtkNew.h"
#include "vtkPassArrays.h"
#include "vtkStringArray.h"
#include "vtkTable.h"
#include "vtkTestErrorObserver.h"
#include "vtkVariantArray.h"

#include <iostream>
#include <list>
#include <string>

namespace utils
{

//------------------------------------------------------------------------------
bool AreEqual(vtkAbstractArray* c1, vtkAbstractArray* c2)
{
  if (std::string(c1->GetName()) != std::string(c2->GetName()))
  {
    return false;
  }

  if (c1->GetNumberOfValues() != c2->GetNumberOfValues())
  {
    return false;
  }
  for (int val = 0; val < c1->GetNumberOfValues(); val++)
  {
    auto v1 = c1->GetVariantValue(val);
    auto v2 = c2->GetVariantValue(val);
    if (!(v1 == v2))
    {
      // By convention, v1 and v2 can still be Nan and Nan
      if (v1.IsNumeric() && v2.IsNumeric() && vtkMath::IsNan(v1.ToDouble()) &&
        vtkMath::IsNan(v2.ToDouble()))
      {
        continue;
      }
      else
      {
        return false;
      }
    }
  }
  return true;
}

//------------------------------------------------------------------------------
bool AreEqual(vtkTable* t1, vtkTable* t2)
{
  if (t1->GetNumberOfColumns() != t2->GetNumberOfColumns())
  {
    return false;
  }

  for (int col = 0; col < t1->GetNumberOfColumns(); col++)
  {
    auto c1 = t1->GetColumn(col);
    auto c2 = t2->GetColumn(col);
    if (!utils::AreEqual(c1, c2))
    {
      return false;
    }
  }
  return true;
}

//------------------------------------------------------------------------------
void InitInputTables(vtkTable* tableLeft, vtkTable* tableRight)
{
  // Create columns
  vtkNew<vtkIntArray> col0;
  col0->SetName("KEYL");
  col0->SetNumberOfTuples(4);

  vtkNew<vtkIntArray> col1;
  col1->SetName("KEYR");
  col1->SetNumberOfTuples(4);

  vtkNew<vtkIntArray> col2;
  col2->SetName("A");
  col2->SetNumberOfTuples(4);

  vtkNew<vtkFloatArray> col3;
  col3->SetName("B");
  col3->SetNumberOfTuples(4);

  vtkNew<vtkIntArray> col4;
  col4->SetName("C");
  col4->SetNumberOfTuples(4);

  vtkNew<vtkIntArray> col5;
  col5->SetName("D");
  col5->SetNumberOfTuples(4);

  // Populate these columns with data
  for (int cc = 0; cc < 4; ++cc)
  {
    col0->SetValue(cc, 2 * cc);
    col1->SetValue(cc, 4 * cc);
  }

  for (int cc = 0; cc < 4; ++cc)
  {
    col2->SetValue(cc, 10 * cc);
    col3->SetValue(cc, 100.0f * cc);
    col4->SetValue(cc, 1000 * cc);
    col5->SetValue(cc, 10000 * cc);
  }

  vtkNew<vtkStringArray> col6;
  col6->SetName("NamesL");
  col6->InsertNextValue("Alex");
  col6->InsertNextValue("Bert");
  col6->InsertNextValue("Cory");
  col6->InsertNextValue("Dave");

  vtkNew<vtkStringArray> col7;
  col7->SetName("NamesR");
  col7->InsertNextValue("Cory");
  col7->InsertNextValue("Dave");
  col7->InsertNextValue("Elly");
  col7->InsertNextValue("Fran");

  // Fill the tables
  tableLeft->AddColumn(col0);
  tableLeft->AddColumn(col2);
  tableLeft->AddColumn(col3);
  tableLeft->AddColumn(col6);

  tableRight->AddColumn(col1);
  tableRight->AddColumn(col4);
  tableRight->AddColumn(col5);
  tableRight->AddColumn(col7);
}

//------------------------------------------------------------------------------
vtkNew<vtkIntArray> NewIntColumn(std::string name, const std::list<int>& valueList)
{
  vtkNew<vtkIntArray> col;
  col->SetName(name.c_str());
  for (std::list<int>::const_iterator it = valueList.begin(); it != valueList.end(); it++)
  {
    col->InsertNextValue(*it);
  }
  return col;
}

//------------------------------------------------------------------------------
vtkNew<vtkStringArray> NewStrColumn(std::string name, const std::list<std::string>& valueList)
{
  vtkNew<vtkStringArray> col;
  col->SetName(name.c_str());
  for (std::list<std::string>::const_iterator it = valueList.begin(); it != valueList.end(); it++)
  {
    col->InsertNextValue(*it);
  }
  return col;
}
}

//------------------------------------------------------------------------------
bool TestIntersection()
{
  // Test INTERSECTION JoinMode
  auto col0 = utils::NewIntColumn(std::string("KEYL"), std::list<int>({ 0, 4 }));
  auto col1 = utils::NewIntColumn(std::string("A"), std::list<int>({ 0, 20 }));
  auto col2 = utils::NewIntColumn(std::string("B"), std::list<int>({ 0, 200 }));
  auto col3 =
    utils::NewStrColumn(std::string("NamesL"), std::list<std::string>({ "Alex", "Cory" }));
  auto col4 = utils::NewIntColumn(std::string("C"), std::list<int>({ 0, 1000 }));
  auto col5 = utils::NewIntColumn(std::string("D"), std::list<int>({ 0, 10000 }));
  auto col6 =
    utils::NewStrColumn(std::string("NamesR"), std::list<std::string>({ "Cory", "Dave" }));

  vtkNew<vtkTable> tableLeft;
  vtkNew<vtkTable> tableRight;
  utils::InitInputTables(tableLeft, tableRight);

  vtkNew<vtkTable> expectedResult;
  expectedResult->AddColumn(col0);
  expectedResult->AddColumn(col1);
  expectedResult->AddColumn(col2);
  expectedResult->AddColumn(col3);
  expectedResult->AddColumn(col4);
  expectedResult->AddColumn(col5);
  expectedResult->AddColumn(col6);

  // Instantiate a join filter with the desired parameters
  vtkNew<vtkJoinTables> join_filter;
  join_filter->SetInputData(tableLeft);
  join_filter->SetSourceData(tableRight);
  join_filter->SetMode(0);
  join_filter->SetLeftKey("KEYL");
  join_filter->SetRightKey("KEYR");
  join_filter->Update();
  auto result = join_filter->GetOutput();

  if (utils::AreEqual(result, expectedResult))
  {
    return true;
  }
  else
  {
    std::cerr << "[TestJoinTables] Test for JoinMode = INTERSECTION has failed." << std::endl;
    return false;
  }
}

//------------------------------------------------------------------------------
bool TestUnion()
{
  // Test UNION JoinMode
  auto col0 = utils::NewIntColumn(std::string("KEYL"), std::list<int>({ 0, 2, 4, 6, 8, 12 }));
  auto col1 = utils::NewIntColumn(std::string("A"), std::list<int>({ 0, 10, 20, 30, 0, 0 }));
  auto col2 = utils::NewIntColumn(std::string("B"), std::list<int>({ 0, 100, 200, 300, 0, 0 }));
  auto col3 = utils::NewStrColumn(
    std::string("NamesL"), std::list<std::string>({ "Alex", "Bert", "Cory", "Dave", "", "" }));
  auto col4 = utils::NewIntColumn(std::string("C"), std::list<int>({ 0, 0, 1000, 0, 2000, 3000 }));
  auto col5 =
    utils::NewIntColumn(std::string("D"), std::list<int>({ 0, 0, 10000, 0, 20000, 30000 }));
  auto col6 = utils::NewStrColumn(
    std::string("NamesR"), std::list<std::string>({ "Cory", "", "Dave", "", "Elly", "Fran" }));

  vtkNew<vtkTable> tableLeft;
  vtkNew<vtkTable> tableRight;
  utils::InitInputTables(tableLeft, tableRight);

  vtkNew<vtkTable> expectedResult;
  expectedResult->AddColumn(col0);
  expectedResult->AddColumn(col1);
  expectedResult->AddColumn(col2);
  expectedResult->AddColumn(col3);
  expectedResult->AddColumn(col4);
  expectedResult->AddColumn(col5);
  expectedResult->AddColumn(col6);

  // Instantiate a join filter with the desired parameters
  vtkNew<vtkJoinTables> join_filter;
  join_filter->SetInputData(0, tableLeft);
  join_filter->SetInputData(1, tableRight);
  join_filter->SetMode(1);
  join_filter->SetLeftKey("KEYL");
  join_filter->SetRightKey("KEYR");
  join_filter->Update();
  auto result = join_filter->GetOutput();

  if (utils::AreEqual(result, expectedResult))
  {
    return true;
  }
  else
  {
    std::cerr << "[TestJoinTables] Test for JoinMode = UNION has failed." << std::endl;
    return false;
  }
}

//------------------------------------------------------------------------------
bool TestLeft()
{
  // Test LEFT JoinMode
  auto col0 = utils::NewIntColumn(std::string("KEYL"), std::list<int>({ 0, 2, 4, 6 }));
  auto col1 = utils::NewIntColumn(std::string("A"), std::list<int>({ 0, 10, 20, 30 }));
  auto col2 = utils::NewIntColumn(std::string("B"), std::list<int>({ 0, 100, 200, 300 }));
  auto col3 = utils::NewStrColumn(
    std::string("NamesL"), std::list<std::string>({ "Alex", "Bert", "Cory", "Dave" }));
  auto col4 = utils::NewIntColumn(std::string("C"), std::list<int>({ 0, 0, 1000, 0 }));
  auto col5 = utils::NewIntColumn(std::string("D"), std::list<int>({ 0, 0, 10000, 0 }));
  auto col6 =
    utils::NewStrColumn(std::string("NamesR"), std::list<std::string>({ "Cory", "", "Dave", "" }));

  vtkNew<vtkTable> tableLeft;
  vtkNew<vtkTable> tableRight;
  utils::InitInputTables(tableLeft, tableRight);

  vtkNew<vtkTable> expectedResult;
  expectedResult->AddColumn(col0);
  expectedResult->AddColumn(col1);
  expectedResult->AddColumn(col2);
  expectedResult->AddColumn(col3);
  expectedResult->AddColumn(col4);
  expectedResult->AddColumn(col5);
  expectedResult->AddColumn(col6);

  // Instantiate a join filter with the desired parameters
  vtkNew<vtkJoinTables> join_filter;
  join_filter->SetInputData(0, tableLeft);
  join_filter->SetInputData(1, tableRight);
  join_filter->SetMode(2);
  join_filter->SetLeftKey("KEYL");
  join_filter->SetRightKey("KEYR");
  join_filter->Update();
  auto result = join_filter->GetOutput();

  if (utils::AreEqual(result, expectedResult))
  {
    return true;
  }
  else
  {
    std::cerr << "[TestJoinTables] Test for JoinMode = LEFT has failed." << std::endl;
    return false;
  }
}

//------------------------------------------------------------------------------
bool TestRight()
{
  // Test RIGHT JoinMode
  auto col0 = utils::NewIntColumn(std::string("KEYL"), std::list<int>({ 0, 4, 8, 12 }));
  auto col1 = utils::NewIntColumn(std::string("A"), std::list<int>({ 0, 20, 0, 0 }));
  auto col2 = utils::NewIntColumn(std::string("B"), std::list<int>({ 0, 200, 0, 0 }));
  auto col3 =
    utils::NewStrColumn(std::string("NamesL"), std::list<std::string>({ "Alex", "Cory", "", "" }));
  auto col4 = utils::NewIntColumn(std::string("C"), std::list<int>({ 0, 1000, 2000, 3000 }));

  auto col5 = utils::NewIntColumn(std::string("D"), std::list<int>({ 0, 10000, 20000, 30000 }));
  auto col6 = utils::NewStrColumn(
    std::string("NamesR"), std::list<std::string>({ "Cory", "Dave", "Elly", "Fran" }));

  vtkNew<vtkTable> tableLeft;
  vtkNew<vtkTable> tableRight;
  utils::InitInputTables(tableLeft, tableRight);

  vtkNew<vtkTable> expectedResult;
  expectedResult->AddColumn(col0);
  expectedResult->AddColumn(col1);
  expectedResult->AddColumn(col2);
  expectedResult->AddColumn(col3);
  expectedResult->AddColumn(col4);
  expectedResult->AddColumn(col5);
  expectedResult->AddColumn(col6);

  // Instantiate a join filter with the desired parameters
  vtkNew<vtkJoinTables> join_filter;
  join_filter->SetInputData(0, tableLeft);
  join_filter->SetInputData(1, tableRight);
  join_filter->SetMode(3);
  join_filter->SetLeftKey("KEYL");
  join_filter->SetRightKey("KEYR");
  join_filter->Update();
  auto result = join_filter->GetOutput();

  if (utils::AreEqual(result, expectedResult))
  {
    return true;
  }
  else
  {
    std::cerr << "[TestJoinTables] Test for JoinMode = RIGHT has failed." << std::endl;
    return false;
  }
}

//------------------------------------------------------------------------------
bool TestSuffixes()
{
  // Test that suffixes are appended when there are duplicate column names
  auto col0 = utils::NewIntColumn(std::string("KEYL"), std::list<int>({ 0, 1 }));
  auto col1 = utils::NewIntColumn(std::string("A"), std::list<int>({ 10, 20 }));
  auto col2 = utils::NewIntColumn(std::string("B"), std::list<int>({ 30, 40 }));
  auto col3 = utils::NewIntColumn(std::string("KEYR"), std::list<int>({ 0, 1 }));
  auto col4 = utils::NewIntColumn(std::string("A"), std::list<int>({ 15, 25 }));
  auto col5 = utils::NewIntColumn(std::string("C"), std::list<int>({ 50, 60 }));
  auto col6 = utils::NewIntColumn(std::string("A_0"), std::list<int>({ 10, 20 }));
  auto col7 = utils::NewIntColumn(std::string("A_1"), std::list<int>({ 15, 25 }));

  vtkNew<vtkTable> tableLeft;
  vtkNew<vtkTable> tableRight;
  vtkNew<vtkTable> expectedResult;

  tableLeft->AddColumn(col0);
  tableLeft->AddColumn(col1);
  tableLeft->AddColumn(col2);
  tableRight->AddColumn(col3);
  tableRight->AddColumn(col4);
  tableRight->AddColumn(col5);
  expectedResult->AddColumn(col0);
  expectedResult->AddColumn(col6);
  expectedResult->AddColumn(col2);
  expectedResult->AddColumn(col7);
  expectedResult->AddColumn(col5);

  // Instantiate a join filter with the desired parameters
  vtkNew<vtkJoinTables> join_filter;
  join_filter->SetInputData(0, tableLeft);
  join_filter->SetInputData(1, tableRight);
  join_filter->SetMode(0);
  join_filter->SetLeftKey("KEYL");
  join_filter->SetRightKey("KEYR");
  join_filter->Update();
  auto result = join_filter->GetOutput();

  if (utils::AreEqual(result, expectedResult))
  {
    return true;
  }
  else
  {
    std::cerr << "[TestJoinTables] Test for suffixes has failed." << std::endl;
    return false;
  }
}

//------------------------------------------------------------------------------
bool TestReplacementValue()
{
  // Test that unknown numerical data is replaced by ReplacementValue
  auto col0 = utils::NewIntColumn(std::string("KEYL"), std::list<int>({ 0, 2, 4, 6 }));
  auto col1 = utils::NewIntColumn(std::string("A"), std::list<int>({ 0, 10, 20, 30 }));
  auto col2 = utils::NewIntColumn(std::string("B"), std::list<int>({ 0, 100, 200, 300 }));
  auto col3 = utils::NewStrColumn(
    std::string("NamesL"), std::list<std::string>({ "Alex", "Bert", "Cory", "Dave" }));
  auto col4 = utils::NewIntColumn(std::string("C"), std::list<int>({ 0, 9999, 1000, 9999 }));
  auto col5 = utils::NewIntColumn(std::string("D"), std::list<int>({ 0, 9999, 10000, 9999 }));
  auto col6 =
    utils::NewStrColumn(std::string("NamesR"), std::list<std::string>({ "Cory", "", "Dave", "" }));

  vtkNew<vtkTable> tableLeft;
  vtkNew<vtkTable> tableRight;
  utils::InitInputTables(tableLeft, tableRight);

  vtkNew<vtkTable> expectedResult;
  expectedResult->AddColumn(col0);
  expectedResult->AddColumn(col1);
  expectedResult->AddColumn(col2);
  expectedResult->AddColumn(col3);
  expectedResult->AddColumn(col4);
  expectedResult->AddColumn(col5);
  expectedResult->AddColumn(col6);

  // Instantiate a join filter with the desired parameters
  vtkNew<vtkJoinTables> join_filter;
  join_filter->SetInputData(0, tableLeft);
  join_filter->SetInputData(1, tableRight);
  join_filter->SetMode(2);
  join_filter->SetReplacementValue(9999);
  join_filter->SetLeftKey("KEYL");
  join_filter->SetRightKey("KEYR");
  join_filter->Update();
  auto result = join_filter->GetOutput();

  if (utils::AreEqual(result, expectedResult))
  {
    return true;
  }
  else
  {
    std::cerr << "[TestJoinTables] Test for JoinMode = LEFT has failed." << std::endl;
    return false;
  }
}

//------------------------------------------------------------------------------
bool TestEmptyTableInput()
{
  // Ensure that the filter behaves correctly when one of the input tables is empty
  vtkNew<vtkTable> tableLeft;
  vtkNew<vtkTable> tableRight;
  vtkNew<vtkTable> tableEmpty;
  vtkNew<vtkTable> expectedResult; // Output should also be empty
  utils::InitInputTables(tableLeft, tableRight);

  vtkNew<vtkJoinTables> join_filter;
  join_filter->SetInputData(0, tableLeft);
  join_filter->SetInputData(1, tableEmpty);
  join_filter->SetMode(0);
  join_filter->SetLeftKey("KEYL");
  join_filter->SetRightKey("KEYR");
  join_filter->Update();
  auto result = join_filter->GetOutput();

  if (utils::AreEqual(result, expectedResult))
  {
    return true;
  }
  else
  {
    std::cerr << "[TestJoinTables] Test for empty input table has failed." << std::endl;
    return false;
  }
}

//------------------------------------------------------------------------------
bool TestKeyDuplicate()
{
  // Ensure that the filter doesn't accept columns containing duplicate values
  vtkNew<vtkTable> tableLeft;
  vtkNew<vtkTable> tableRight;
  utils::InitInputTables(tableLeft, tableRight);

  vtkNew<vtkIntArray> c;
  c->SetName("contains_duplicates");
  c->InsertNextValue(11);
  c->InsertNextValue(22);
  c->InsertNextValue(33);
  c->InsertNextValue(33);
  tableLeft->AddColumn(c);

  vtkNew<vtkJoinTables> join_filter;
  auto errorObserver = vtkSmartPointer<vtkTest::ErrorObserver>::New();
  auto errorObserver1 = vtkSmartPointer<vtkTest::ErrorObserver>::New();
  join_filter->AddObserver(vtkCommand::ErrorEvent, errorObserver);
  join_filter->GetExecutive()->AddObserver(vtkCommand::ErrorEvent, errorObserver1);
  join_filter->SetInputData(0, tableLeft);
  join_filter->SetInputData(1, tableRight);
  join_filter->SetMode(0);
  join_filter->SetLeftKey("contains_duplicates");
  join_filter->SetRightKey("KEYR");
  join_filter->Update();
  errorObserver->CheckErrorMessage(
    "The key columns must not contain duplicate values"); // Will throw an error and fail the test
                                                          // if not found.

  return true;
}

//------------------------------------------------------------------------------
bool TestInvalidKeyName()
{
  // Ensure that an unknown column name can't be provided
  vtkNew<vtkTable> tableLeft;
  vtkNew<vtkTable> tableRight;
  utils::InitInputTables(tableLeft, tableRight);

  vtkNew<vtkJoinTables> join_filter;
  auto errorObserver = vtkSmartPointer<vtkTest::ErrorObserver>::New();
  auto errorObserver1 = vtkSmartPointer<vtkTest::ErrorObserver>::New();
  join_filter->AddObserver(vtkCommand::ErrorEvent, errorObserver);
  join_filter->GetExecutive()->AddObserver(vtkCommand::ErrorEvent, errorObserver1);
  join_filter->SetInputData(0, tableLeft);
  join_filter->SetInputData(1, tableRight);
  join_filter->SetMode(0);
  join_filter->SetLeftKey("KEYL");
  join_filter->SetRightKey("unknown_column_name");
  join_filter->Update();
  errorObserver->CheckErrorMessage("key is invalid"); // Will throw an error and fail the test
                                                      // if not found.
  return true;
}

//------------------------------------------------------------------------------
bool TestUnmatchedKeyTypes()
{
  // Ensure that the filter doesn't perform a comparison on different types
  vtkNew<vtkTable> tableLeft;
  vtkNew<vtkTable> tableRight;
  utils::InitInputTables(tableLeft, tableRight);

  vtkNew<vtkJoinTables> join_filter;
  auto errorObserver = vtkSmartPointer<vtkTest::ErrorObserver>::New();
  auto errorObserver1 = vtkSmartPointer<vtkTest::ErrorObserver>::New();
  join_filter->AddObserver(vtkCommand::ErrorEvent, errorObserver);
  join_filter->GetExecutive()->AddObserver(vtkCommand::ErrorEvent, errorObserver1);
  join_filter->SetInputData(0, tableLeft);
  join_filter->SetInputData(1, tableRight);
  join_filter->SetMode(0);
  join_filter->SetLeftKey("KEYL");    // <vtkIntArray>
  join_filter->SetRightKey("NamesR"); // <vtkStringArray>
  join_filter->Update();
  errorObserver->CheckErrorMessage(
    "Key columns data types do not match"); // Will throw an error and fail the test
                                            // if not found.
  return true;
}

//------------------------------------------------------------------------------
bool TestSetKeyAfterError()
{
  // Test SetKey after an error
  auto col0 = utils::NewIntColumn(std::string("KEYL"), std::list<int>({ 0, 4 }));
  auto col1 = utils::NewIntColumn(std::string("A"), std::list<int>({ 0, 20 }));
  auto col2 = utils::NewIntColumn(std::string("B"), std::list<int>({ 0, 200 }));
  auto col3 =
    utils::NewStrColumn(std::string("NamesL"), std::list<std::string>({ "Alex", "Cory" }));
  auto col4 = utils::NewIntColumn(std::string("C"), std::list<int>({ 0, 1000 }));
  auto col5 = utils::NewIntColumn(std::string("D"), std::list<int>({ 0, 10000 }));
  auto col6 =
    utils::NewStrColumn(std::string("NamesR"), std::list<std::string>({ "Cory", "Dave" }));

  vtkNew<vtkTable> tableLeft;
  vtkNew<vtkTable> tableRight;
  utils::InitInputTables(tableLeft, tableRight);

  vtkNew<vtkTable> expectedResult;
  expectedResult->AddColumn(col0);
  expectedResult->AddColumn(col1);
  expectedResult->AddColumn(col2);
  expectedResult->AddColumn(col3);
  expectedResult->AddColumn(col4);
  expectedResult->AddColumn(col5);
  expectedResult->AddColumn(col6);

  // Instantiate a join filter with the desired parameters
  vtkNew<vtkJoinTables> join_filter;
  join_filter->SetInputData(tableLeft);
  join_filter->SetSourceData(tableRight);
  join_filter->SetMode(0);
  join_filter->SetLeftKey("KEYL");
  join_filter->SetRightKey("NamesR");
  auto errorObserver = vtkSmartPointer<vtkTest::ErrorObserver>::New();
  auto errorObserver1 = vtkSmartPointer<vtkTest::ErrorObserver>::New();
  join_filter->AddObserver(vtkCommand::ErrorEvent, errorObserver);
  join_filter->GetExecutive()->AddObserver(vtkCommand::ErrorEvent, errorObserver1);

  join_filter->Update(); // This should yield an error stating that data types do not match
  errorObserver->CheckErrorMessage(
    "Key columns data types do not match"); // Will throw an error and fail the test
                                            // if not found.

  join_filter->SetRightKey("KEYR");
  join_filter->Update(); // This should now work
  auto result = join_filter->GetOutput();
  if (utils::AreEqual(result, expectedResult))
  {
    return true;
  }
  else
  {
    std::cerr << "[TestJoinTables] Test for SetKey after error has failed." << std::endl;
    return false;
  }
}

//------------------------------------------------------------------------------
int TestJoinTables(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  // Main test function
  if (TestIntersection() && TestUnion() && TestLeft() && TestRight() && TestSuffixes() &&
    TestReplacementValue() && TestEmptyTableInput() && TestKeyDuplicate() && TestInvalidKeyName() &&
    TestUnmatchedKeyTypes() && TestSetKeyAfterError())
  {
    return EXIT_SUCCESS;
  }
  else
  {
    return EXIT_FAILURE;
  }
}
