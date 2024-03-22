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

struct AttInfo
{
  vtkStringToken Name;
  vtkStringToken CellType;
  vtkStringToken DOFSharing;
  vtkStringToken FunctionSpace;
  vtkStringToken Basis;
  int Order;
};

bool RoundTrip(const char* filename, const std::string& tempDir, vtkIdType numCells,
  const std::vector<AttInfo>& expectedAttributes)
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
    auto* att = cg->GetCellAttributeByName(attData.Name.Data());
    if (!att)
    {
      ok = false;
      std::cerr << "ERROR: Failed to find cell-attribute \"" << attData.Name.Data() << "\".\n";
      continue;
    }
    auto cellTypeInfo = att->GetCellTypeInfo(attData.CellType);
    if (attData.DOFSharing != cellTypeInfo.DOFSharing)
    {
      ok = false;
      std::cerr << "ERROR: Attribute " << attData.Name.Data() << " had DOF sharing "
                << cellTypeInfo.DOFSharing.Data() << " " << std::hex
                << cellTypeInfo.DOFSharing.GetId() << " vs " << attData.DOFSharing.Data() << " "
                << attData.DOFSharing.GetId() << ".\n";
    }
    if (attData.FunctionSpace != cellTypeInfo.FunctionSpace)
    {
      ok = false;
      std::cerr << "ERROR: Attribute " << attData.Name.Data() << " had function space "
                << cellTypeInfo.FunctionSpace.Data() << " " << std::hex
                << cellTypeInfo.FunctionSpace.GetId() << " vs " << attData.FunctionSpace.GetId()
                << ".\n";
    }
    if (attData.Basis != cellTypeInfo.Basis)
    {
      ok = false;
      std::cerr << "ERROR: Attribute " << attData.Name.Data() << " had basis "
                << cellTypeInfo.Basis.Data() << " " << std::hex << cellTypeInfo.Basis.GetId()
                << " vs " << attData.Basis.GetId() << ".\n";
    }
    if (attData.Order != cellTypeInfo.Order)
    {
      ok = false;
      std::cerr << "ERROR: Attribute " << attData.Name.Data() << " had order " << cellTypeInfo.Order
                << " vs " << attData.Order << ".\n";
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
  vtkStringToken invalid;
  std::string tempDir =
    vtkTestUtilities::GetArgOrEnvOrDefault("-T", argc, argv, "VTK_TEMP_DIR", "Testing/Temporary/");

  // clang-format off
  if (!RoundTrip(vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/dgHexahedra.dg", 0),
        tempDir,
        /* numCells */ 2, {
        { "shape",     "vtkDGHex", "coordinates", "HGRAD", "C", 1 },
        { "scalar0",   "vtkDGHex",  invalid,      "HGRAD", "C", 1 },
        { "scalar1",   "vtkDGHex",  invalid,      "HGRAD", "C", 1 },
        { "scalar2",   "vtkDGHex",  invalid,      "HGRAD", "C", 1 },
        { "scalar3",   "vtkDGHex", "point-data",  "HGRAD", "C", 1 },
        { "curl1",     "vtkDGHex",  invalid,      "HCURL", "I", 1 },
        { "quadratic", "vtkDGHex",  invalid,      "HGRAD", "I", 2 } }))
  {
    return EXIT_FAILURE;
  }

  if (!RoundTrip(vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/dgTetrahedra.dg", 0),
        tempDir,
        /* numCells */ 2, {
        { "shape",   "vtkDGTet", "coordinates", "HGRAD", "C", 1 },
        { "scalar0", "vtkDGTet",  invalid,      "HGRAD", "C", 1 },
        { "scalar1", "vtkDGTet",  invalid,      "HGRAD", "C", 1 },
        { "scalar2", "vtkDGTet",  invalid,      "HGRAD", "C", 1 },
        { "scalar3", "vtkDGTet", "point-data",  "HGRAD", "C", 1 } }))
  {
    return EXIT_FAILURE;
  }
  // clang-format on

  return EXIT_SUCCESS;
}
