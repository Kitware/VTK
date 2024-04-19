// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/*----------------------------------------------------------------------------
  This test was written by Menno Deij - van Rijswijk (MARIN).
----------------------------------------------------------------------------*/

#include "vtkCell.h" // for cell types
#include "vtkCellIterator.h"
#include "vtkDataArray.h"
#include "vtkIdList.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkMappedUnstructuredGridGenerator.h"
#include "vtkNew.h"
#include "vtkPoints.h"
#include "vtkTestUtilities.h"
#include "vtkUnstructuredGrid.h"
#include "vtkXMLUnstructuredGridReader.h"
#include "vtkXMLUnstructuredGridWriter.h"
#include "vtksys/FStream.hxx"

#include <algorithm>
#include <fstream>
#include <string>

bool compareFiles(const std::string& p1, const std::string& p2)
{
  vtksys::ifstream f1(p1.c_str(), std::ios::binary | std::ios::ate);
  vtksys::ifstream f2(p2.c_str(), std::ios::binary | std::ios::ate);

  if (f1.fail() || f2.fail())
  {
    return false; // file problem
  }

  if (f1.tellg() != f2.tellg())
  {
    return false; // size mismatch
  }

  // seek back to beginning and use equal to compare contents
  f1.seekg(0, vtksys::ifstream::beg);
  f2.seekg(0, vtksys::ifstream::beg);
  return equal(std::istreambuf_iterator<char>(f1.rdbuf()), std::istreambuf_iterator<char>(),
    std::istreambuf_iterator<char>(f2.rdbuf()));
}

int TestXMLMappedUnstructuredGridIO(int argc, char* argv[])
{
  vtkUnstructuredGrid* ug;
  vtkMappedUnstructuredGridGenerator::GenerateUnstructuredGrid(&ug);
  // for testing, we write in appended, ascii and binary mode and request that
  // the files are ** binary ** equal.
  //
  // first, find a file we can write to

  char* tempDir =
    vtkTestUtilities::GetArgOrEnvOrDefault("-T", argc, argv, "VTK_TEMP_DIR", "Testing/Temporary");
  std::string dir(tempDir);
  if (dir.empty())
  {
    cerr << "Could not determine temporary directory." << endl;
    return EXIT_FAILURE;
  }

  std::string f1 = dir + "/test_ug_input.vtu";
  std::string f2 = dir + "/test_mapped_input.vtu";

  vtkNew<vtkXMLUnstructuredGridWriter> w;
  w->SetInputData(ug);
  w->SetFileName(f1.c_str());

  w->Update();
  if (ug->GetPoints()->GetData()->GetInformation()->Has(vtkDataArray::L2_NORM_RANGE()))
  {
    // for the normal unstructured grid the L2_NORM_RANGE is added. This
    // makes file comparison impossible. therefore, after the first Update()
    // remove the L2_NORM_RANGE information key and write the file again.
    ug->GetPoints()->GetData()->GetInformation()->Remove(vtkDataArray::L2_NORM_RANGE());
  }
  w->Update();
  ug->Delete();

  // create a mapped grid which basically takes the original grid
  // and uses it to map to.
  vtkUnstructuredGridBase* mg;
  vtkMappedUnstructuredGridGenerator::GenerateMappedUnstructuredGrid(&mg);

  vtkNew<vtkXMLUnstructuredGridWriter> w2;
  w2->SetInputData(mg);
  w2->SetFileName(f2.c_str());
  w2->Update();
  mg->Delete();

  // compare the files in appended, then ascii, then binary mode.
  bool same = compareFiles(f1, f2);
  if (!same)
  {
    std::cerr << "Error comparing files in appended mode.\n";
    return EXIT_FAILURE;
  }
  w->SetDataModeToAscii();
  w2->SetDataModeToAscii();
  w->Update();
  w2->Update();

  same = compareFiles(f1, f2);
  if (!same)
  {
    std::cerr << "Error comparing files in ascii mode.\n";
    return EXIT_FAILURE;
  }
  w->SetDataModeToBinary();
  w2->SetDataModeToBinary();
  w->Update();
  w2->Update();

  same = compareFiles(f1, f2);
  if (!same)
  {
    std::cerr << "Error comparing files in binary mode.\n";
    return EXIT_FAILURE;
  }

  // clean up after ourselves: remove written files and free temp dir name
  remove(f1.c_str());
  remove(f2.c_str());

  delete[] tempDir;

  return 0;
}
