// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * Check that vtkMultiVariateToRGBA processes table columns of any numeric
 * type, the same type or mixed, and AOS or not, the same as double columns,
 * and that it rejects columns with several components.
 */

#include <vtkCommand.h>
#include <vtkDataArrayRange.h>
#include <vtkDataSetAttributes.h>
#include <vtkDoubleArray.h>
#include <vtkExecutive.h>
#include <vtkFloatArray.h>
#include <vtkIntArray.h>
#include <vtkMultiVariateToRGBA.h>
#include <vtkNew.h>
#include <vtkSOADataArrayTemplate.h>
#include <vtkShortArray.h>
#include <vtkSmartPointer.h>
#include <vtkTable.h>
#include <vtkTestErrorObserver.h>

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace
{
#define CHECK(condition)                                                                           \
  if (!(condition))                                                                                \
  {                                                                                                \
    std::cerr << "Line " << __LINE__ << ": check failed: " #condition << std::endl;                \
    return EXIT_FAILURE;                                                                           \
  }

constexpr vtkIdType NumberOfValues = 20;

// Returns a one component column 'name' of type 'ArrayType' with integer
// values that depend on 'seed'.
template <typename ArrayType>
vtkSmartPointer<vtkDataArray> MakeColumn(const std::string& name, int seed)
{
  auto column = vtkSmartPointer<ArrayType>::New();
  column->SetName(name.c_str());
  column->SetNumberOfTuples(NumberOfValues);
  for (vtkIdType i = 0; i < NumberOfValues; ++i)
  {
    column->SetTuple1(i, (i * (seed + 3) + seed) % 11);
  }
  return column;
}

// Returns the 'gbc' array the filter computes for a table with 'columns',
// with errors caught by 'errorObserver' if it is set.
vtkSmartPointer<vtkDataArray> ComputeGBC(const std::vector<vtkSmartPointer<vtkDataArray>>& columns,
  vtkTest::ErrorObserver* errorObserver = nullptr)
{
  vtkNew<vtkTable> table;
  for (vtkDataArray* column : columns)
  {
    table->AddColumn(column);
  }
  vtkNew<vtkMultiVariateToRGBA> filter;
  if (errorObserver)
  {
    filter->AddObserver(vtkCommand::ErrorEvent, errorObserver);
    filter->GetExecutive()->AddObserver(vtkCommand::ErrorEvent, errorObserver);
  }
  filter->SetController(nullptr);
  filter->SetStoreGBC(true);
  filter->SetInputData(table);
  filter->Update();
  auto* output = vtkTable::SafeDownCast(filter->GetOutputDataObject(0));
  return output ? output->GetRowData()->GetArray("gbc") : nullptr;
}

bool Equal(vtkDataArray* a, vtkDataArray* b)
{
  auto aValues = vtk::DataArrayValueRange(a);
  auto bValues = vtk::DataArrayValueRange(b);
  return aValues.size() == bValues.size() &&
    std::equal(aValues.begin(), aValues.end(), bValues.begin());
}
}

int TestMultiVariateToRGBATableTypes(int, char*[])
{
  vtkSmartPointer<vtkDataArray> expected = ComputeGBC({ MakeColumn<vtkDoubleArray>("a", 0),
    MakeColumn<vtkDoubleArray>("b", 1), MakeColumn<vtkDoubleArray>("c", 2) });
  CHECK(expected && expected->GetNumberOfTuples() == NumberOfValues);

  // the same type, not float or double
  vtkSmartPointer<vtkDataArray> gbc = ComputeGBC({ MakeColumn<vtkIntArray>("a", 0),
    MakeColumn<vtkIntArray>("b", 1), MakeColumn<vtkIntArray>("c", 2) });
  CHECK(gbc && Equal(gbc, expected));
  // mixed types
  gbc = ComputeGBC({ MakeColumn<vtkIntArray>("a", 0), MakeColumn<vtkFloatArray>("b", 1),
    MakeColumn<vtkShortArray>("c", 2) });
  CHECK(gbc && Equal(gbc, expected));
  // not AOS
  gbc = ComputeGBC({ MakeColumn<vtkSOADataArrayTemplate<double>>("a", 0),
    MakeColumn<vtkSOADataArrayTemplate<double>>("b", 1),
    MakeColumn<vtkSOADataArrayTemplate<double>>("c", 2) });
  CHECK(gbc && Equal(gbc, expected));

  // a column with several components is rejected, alone or with other columns
  vtkNew<vtkDoubleArray> multi;
  multi->SetName("multi");
  multi->SetNumberOfComponents(4);
  multi->SetNumberOfTuples(NumberOfValues);
  multi->Fill(1.0);
  const std::string oneComponent = "Column is not a one component numeric array: multi";
  vtkNew<vtkTest::ErrorObserver> errorObserver;
  CHECK(!ComputeGBC({ multi }, errorObserver));
  CHECK(errorObserver->HasErrorMessage(oneComponent));
  errorObserver->Clear();
  CHECK(
    !ComputeGBC({ MakeColumn<vtkDoubleArray>("a", 0), MakeColumn<vtkDoubleArray>("b", 1), multi },
      errorObserver));
  CHECK(errorObserver->HasErrorMessage(oneComponent));
  return EXIT_SUCCESS;
}
