// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkCellGrid.h"
#include "vtkCellGridReader.h"
#include "vtkCellGridWriter.h"
#include "vtkDGHex.h"
#include "vtkDGTet.h"
#include "vtkDGTri.h"
#include "vtkDataSetAttributes.h"
#include "vtkDoubleArray.h"
#include "vtkIdTypeArray.h"
#include "vtkIntArray.h"
#include "vtkNew.h"
#include "vtkSmartPointer.h"
#include "vtkTable.h"
#include "vtkTestUtilities.h"
#include "vtkVector.h"

namespace
{

bool RoundTrip(const char* filename, const std::string& tempDir, vtkIdType numCells,
  const std::vector<std::pair<std::string, std::string>>& expectedAttributes)
{
  if (!filename)
  {
    return false;
  }
  bool ok = true;

  std::cout << "=== Start of round trip " << filename << " ===\n";
  vtkNew<vtkCellGridReader> reader;
  reader->SetFileName(filename);
  reader->Update();
  auto og = vtkCellGrid::SafeDownCast(reader->GetOutputDataObject(0));
  if (!og)
  {
    std::cerr << "ERROR: Could not read source cell-grid.\n";
    ok = false;
    return ok;
  }

  std::cout << "  === Write step ===\n";
  vtkNew<vtkCellGridWriter> writer;
  std::string tempFile = tempDir.empty() ? "test.dg" : tempDir + "/test.dg";
  writer->SetFileName(tempFile.c_str());
  writer->SetInputConnection(reader->GetOutputPort());
  writer->Write();

  std::cout << "  === Read step ===\n";
  vtkNew<vtkCellGridReader> reader2;
  reader2->SetFileName(tempFile.c_str()); // "test.dg");
  reader2->Update();

  std::cout << "  === Validation ===\n";
  auto cg = vtkCellGrid::SafeDownCast(reader2->GetOutputDataObject(0));
  if (!cg)
  {
    std::cerr << "ERROR: Could not round trip.\n";
    ok = false;
    return ok;
  }

  if (cg->GetNumberOfCells() != numCells)
  {
    std::cerr << "ERROR: Expected to have " << numCells << " cells, got " << cg->GetNumberOfCells()
              << ".\n";
    ok = false;
  }

  for (const auto& attData : expectedAttributes)
  {
    auto* att = cg->GetCellAttributeByName(attData.first);
    if (!att)
    {
      ok = false;
      std::cerr << "ERROR: Failed to find cell-attribute \"" << attData.first << "\".\n";
      continue;
    }
    if (att->GetAttributeType() != attData.second)
    {
      ok = false;
      std::cerr << "ERROR: Attribute " << attData.first << " had type "
                << att->GetAttributeType().Data() << ".\n";
    }
  }
  if (expectedAttributes.size() != cg->GetCellAttributeIds().size())
  {
    ok = false;
    std::cerr << "ERROR: Expected " << expectedAttributes.size() << " attributes, got "
              << cg->GetCellAttributeIds().size() << ".\n";
  }

  if (og->GetSchemaName() != cg->GetSchemaName() ||
    og->GetSchemaVersion() != cg->GetSchemaVersion())
  {
    ok = false;
    std::cerr << "ERROR: Schema name/version information not preserved.\n";
  }

  if (og->GetContentVersion() != cg->GetContentVersion())
  {
    ok = false;
    std::cerr << "ERROR: Content version information not preserved.\n";
  }

  delete[] filename;

  return ok;
}

} // anonymous namespace

int TestCellGridReadWrite(int argc, char* argv[])
{
  std::string tempDir =
    vtkTestUtilities::GetArgOrEnvOrDefault("-T", argc, argv, "VTK_TEMP_DIR", "Testing/Temporary/");

  if (!RoundTrip(vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/dgHexahedra.dg", 0),
        tempDir,
        /* numCells */ 2,
        { { "shape", "CG HGRAD C1" }, { "scalar0", "DG HGRAD C1" }, { "scalar1", "DG HGRAD C1" },
          { "scalar2", "DG HGRAD C1" }, { "scalar3", "CG HGRAD C1" }, { "curl1", "DG HCURL I1" },
          { "quadratic", "DG HGRAD I2" } }))
  {
    return EXIT_FAILURE;
  }

  if (!RoundTrip(vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/dgTetrahedra.dg", 0),
        tempDir,
        /* numCells */ 2,
        { { "shape", "CG HGRAD C1" }, { "scalar0", "DG HGRAD C1" }, { "scalar1", "DG HGRAD C1" },
          { "scalar2", "DG HGRAD C1" }, { "scalar3", "CG HGRAD C1" } }))
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
