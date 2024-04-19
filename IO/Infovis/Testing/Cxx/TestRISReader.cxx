// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkRISReader.h"
#include "vtkSmartPointer.h"
#include "vtkTable.h"
#include "vtkTestUtilities.h"
#include "vtkVariant.h"

template <typename value_t>
void TestValue(const value_t& Value, const value_t& ExpectedValue,
  const std::string& ValueDescription, int& ErrorCount)
{
  if (Value == ExpectedValue)
    return;

  cerr << ValueDescription << " is [" << Value << "] - expected [" << ExpectedValue << "]" << endl;

  ++ErrorCount;
}

int TestRISReader(int argc, char* argv[])
{
  char* file = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/Infovis/eg1.ris");

  cerr << "file: " << file << endl;

  vtkSmartPointer<vtkRISReader> reader = vtkSmartPointer<vtkRISReader>::New();
  reader->SetFileName(file);
  delete[] file;

  reader->Update();
  vtkTable* const table = reader->GetOutput();

  int error_count = 0;

  // Test the size of the output table ...
  TestValue(table->GetNumberOfColumns(), vtkIdType(16), "Column count", error_count);
  TestValue(table->GetNumberOfRows(), vtkIdType(14), "Row count", error_count);

  // Test a sampling of the table columns ...
  TestValue<std::string>(table->GetColumnName(0), "TY", "Column 0", error_count);
  TestValue<std::string>(table->GetColumnName(1), "T1", "Column 1", error_count);
  TestValue<std::string>(table->GetColumnName(2), "JF", "Column 2", error_count);
  TestValue<std::string>(table->GetColumnName(13), "KW", "Column 13", error_count);
  TestValue<std::string>(table->GetColumnName(14), "N1", "Column 14", error_count);
  TestValue<std::string>(table->GetColumnName(15), "UR", "Column 15", error_count);

  // Test a sampling of the table values ...
  TestValue<std::string>(table->GetValue(0, 0).ToString(), "JOUR", "Value 0, 0", error_count);
  TestValue<std::string>(table->GetValue(0, 1).ToString(),
    "Laser frequency stabilization at 1.5 microns using ultranarrow inhomogeneous "
    "absorption profiles in Er3+:LiYF4",
    "Value 0, 1", error_count);
  TestValue<std::string>(
    table->GetValue(0, 2).ToString(), "Journal of Luminescence", "Value 0, 2", error_count);

  TestValue<std::string>(table->GetValue(13, 10).ToString(),
    "Zhou, P.;Li, X.-H.;Liang, Y.-J.;Deng, F.-G.;Zhou, H.-Y.", "value 13, 10", error_count);
  TestValue<std::string>(table->GetValue(13, 11).ToString(),
    "Key Laboratory of Beam Technology and Material Modification, Ministry of Education, Beijing "
    "Normal University, Beijing, 100875, China;Institute of Low Energy Nuclear Physics, "
    "Department of Material Science and Engineering, Beijing Normal University, Beijing, 100875, "
    "China;Beijing Radiation Center, Beijing, 100875, China",
    "value 13, 11", error_count);
  TestValue<std::string>(table->GetValue(13, 13).ToString(),
    "Decoy photons;Pure entangled states;Quantum communication;Quantum secret sharing",
    "value 13, 13", error_count);

  return error_count;
}
