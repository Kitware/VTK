// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkCellData.h"
#include "vtkGenerateIds.h"
#include "vtkHyperTreeGrid.h"
#include "vtkIdTypeArray.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkRandomHyperTreeGridSource.h"
#include "vtkTestUtilities.h"
#include "vtkTesting.h"
#include "vtkXMLHyperTreeGridReader.h"

#include "vtkXMLHyperTreeGridWriter.h"

#include <string>

//------------------------------------------------------------------------------
/**
 * Ensure that cell ids array is present and has been filled correctly
 */
bool TestIdArray(vtkCellData* cellData, vtkIdType expectedSize)
{
  auto idArray = vtkIdTypeArray::SafeDownCast(cellData->GetAbstractArray("vtkCellIds"));
  if (!idArray)
  {
    std::cerr << "Unable to retrieve the cell IDs array." << endl;
    return false;
  }

  if (idArray->GetNumberOfTuples() != expectedSize)
  {
    std::cerr << "Wrong number of tuples in the generated cell IDs array."
              << "Expected " << expectedSize << ", got " << idArray->GetNumberOfTuples() << endl;
    return false;
  }

  if (idArray->GetNumberOfComponents() != 1)
  {
    std::cerr << "Wrong number of tuples in the generated cell IDs array."
              << "Expected 1, got " << idArray->GetNumberOfComponents() << endl;
    return false;
  }

  for (vtkIdType id = 0; id < idArray->GetNumberOfTuples(); id++)
  {
    if (idArray->GetValue(id) != id)
    {
      std::cerr << "Wrong cell ID at index " << id << "."
                << "Expected" << id << ", got " << idArray->GetValue(id) << endl;
      return false;
    }
  }

  return true;
}

//------------------------------------------------------------------------------
int TestGenerateIdsHTG(int argc, char* argv[])
{
  vtkNew<vtkTesting> testHelper;
  testHelper->AddArguments(argc, argv);
  if (!testHelper->IsFlagSpecified("-D"))
  {
    std::cerr << "Error: -D /path/to/data was not specified.";
    return EXIT_FAILURE;
  }
  std::string dataRoot = testHelper->GetDataRoot();

  vtkNew<vtkRandomHyperTreeGridSource> htgSource;
  htgSource->SetSeed(42);
  htgSource->SetMaxDepth(3);
  htgSource->SetDimensions(3, 3, 3);
  htgSource->SetSplitFraction(0.5);

  vtkNew<vtkGenerateIds> generateIds;
  generateIds->SetInputConnection(htgSource->GetOutputPort());
  generateIds->SetCellIds(true);
  generateIds->Update();
  auto data = generateIds->GetOutput();

  vtkHyperTreeGrid* outputHTG = vtkHyperTreeGrid::SafeDownCast(data);
  if (!outputHTG)
  {
    std::cerr << "Unable to retrieve output HTG." << std::endl;
    return EXIT_FAILURE;
  }

  vtkCellData* outputCellData = outputHTG->GetCellData();
  if (!outputCellData)
  {
    std::cerr << "Unable to retrieve output cell data." << std::endl;
    return EXIT_FAILURE;
  }

  // Test ID array
  if (!TestIdArray(outputCellData, outputHTG->GetNumberOfCells()))
  {
    return EXIT_FAILURE;
  }

  // Never hurt to do regression test on the whole dataset
  const std::string baselinePath = dataRoot + "/Data/HTG/generate_ids.htg";
  vtkNew<vtkXMLHyperTreeGridReader> reader;
  reader->SetFileName(baselinePath.c_str());
  reader->Update();
  auto expectedData = reader->GetOutput();

  if (!vtkTestUtilities::CompareDataObjects(data, expectedData))
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
