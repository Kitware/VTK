// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// Regression test: vtkDataReader::CharacterizeFile() (invoked via
// GetNumberOfScalarsInFile()) crashes when the legacy file contains a named
// SCALARS/FIELD/etc. attribute, see vtkDataReader::CheckFor().

#include "vtkCellData.h"
#include "vtkDataSetReader.h"
#include "vtkDataSetWriter.h"
#include "vtkFieldData.h"
#include "vtkFloatArray.h"
#include "vtkIdList.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkPoints.h"
#include "vtkTesting.h"
#include "vtkUnstructuredGrid.h"

#include <string>

int TestLegacyDataReaderCharacterizeFile(int argc, char* argv[])
{
  vtkNew<vtkTesting> testing;
  testing->AddArguments(argc, argv);

  const std::string tempDirectory = testing->GetTempDirectory();
  const std::string fileName = tempDirectory + "/TestLegacyDataReaderCharacterizeFile.vtk";

  {
    // Write a small unstructured grid with named cell scalars and field data.
    vtkNew<vtkUnstructuredGrid> grid;
    vtkNew<vtkPoints> points;
    points->InsertNextPoint(0.0, 0.0, 0.0);
    points->InsertNextPoint(1.0, 0.0, 0.0);
    points->InsertNextPoint(0.0, 1.0, 0.0);
    points->InsertNextPoint(0.0, 0.0, 1.0);
    grid->SetPoints(points);

    vtkNew<vtkIdList> ptIds;
    ptIds->InsertId(0, 0);
    ptIds->InsertId(1, 1);
    ptIds->InsertId(2, 2);
    ptIds->InsertId(3, 3);
    grid->InsertNextCell(VTK_TETRA, ptIds);

    vtkNew<vtkFloatArray> scalars;
    scalars->SetName("TestScalars");
    scalars->InsertNextValue(1.5f);
    // Use SetScalars() so the writer emits an actual "SCALARS" line.
    grid->GetCellData()->SetScalars(scalars);

    vtkNew<vtkFloatArray> fieldArray;
    fieldArray->SetName("FieldData");
    fieldArray->InsertNextValue(42.0f);
    grid->GetFieldData()->AddArray(fieldArray);

    vtkNew<vtkDataSetWriter> writer;
    writer->SetFileName(fileName.c_str());
    writer->SetInputData(grid);
    writer->Write();
  }

  {
    vtkNew<vtkDataSetReader> reader;
    reader->SetFileName(fileName.c_str());
    reader->SetReadAllScalars(1);
    reader->SetReadAllFields(1);

    // This call crashes in vtkDataReader::CheckFor() when it copies the
    // parsed attribute name into the array slot that was never allocated.
    int numScalars = reader->GetNumberOfScalarsInFile();
    vtkLog(INFO, "Number of scalars in file: " << numScalars);

    if (numScalars <= 0)
    {
      vtkLog(ERROR, "Unexpected negative/empty scalar count.");
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}
