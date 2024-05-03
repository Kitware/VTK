// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkCellData.h"
#include "vtkDataObjectTree.h"
#include "vtkDataObjectTreeIterator.h"
#include "vtkGroupDataSetsFilter.h"
#include "vtkHyperTreeGrid.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkRandomAttributeGenerator.h"
#include "vtkRandomHyperTreeGridSource.h"
#include "vtkTestUtilities.h"
#include "vtkTesting.h"
#include "vtkUnsignedCharArray.h"
#include "vtkXMLHyperTreeGridReader.h"

#include <string>

//------------------------------------------------------------------------------
/**
 * Ensure that cell ids array is present and has been filled correctly
 */
bool TestRandomUnsignedCharCellArray(vtkCellData* cellData, const char* arrayName,
  vtkIdType expectedNbOfTuples, int expectedNbOfComponents, int min, int max)
{
  auto randomArray = vtkUnsignedCharArray::SafeDownCast(cellData->GetAbstractArray(arrayName));
  if (!randomArray)
  {
    std::cerr << "Unable to retrieve the " << arrayName << " array." << endl;
    return false;
  }

  if (randomArray->GetNumberOfTuples() != expectedNbOfTuples)
  {
    std::cerr << "Wrong number of tuples in the generated " << arrayName << " array."
              << "Expected " << expectedNbOfTuples << ", got " << randomArray->GetNumberOfTuples()
              << endl;
    return false;
  }

  if (randomArray->GetNumberOfComponents() != expectedNbOfComponents)
  {
    std::cerr << "Wrong number of tuples in the generated " << arrayName << " array."
              << "Expected" << expectedNbOfComponents << ", got "
              << randomArray->GetNumberOfComponents() << endl;
    return false;
  }

  // Test values are within the given range
  for (vtkIdType id = 0; id < randomArray->GetNumberOfTuples(); id++)
  {
    if (randomArray->GetValue(id) < min || randomArray->GetValue(id) > max)
    {
      std::cerr << "Wrong random value in the " << arrayName << "array at index " << id
                << ". Expected value between " << min << " and " << max << ", got "
                << randomArray->GetValue(id) << endl;
      return false;
    }
  }

  return true;
}

//------------------------------------------------------------------------------
/**
 * Test the random attributes filter on a single HTG
 */
bool TestRandomAttributesSingleHTG(std::string& dataRoot)
{
  vtkNew<vtkRandomHyperTreeGridSource> htgSource;
  htgSource->SetSeed(42);
  htgSource->SetMaxDepth(3);
  htgSource->SetDimensions(3, 3, 3);
  htgSource->SetSplitFraction(0.5);

  vtkNew<vtkRandomAttributeGenerator> generator;
  generator->SetInputConnection(htgSource->GetOutputPort());
  generator->SetDataTypeToUnsignedChar();
  generator->SetComponentRange(0, 255);
  generator->SetGenerateCellScalars(true);
  generator->SetGenerateCellVectors(true);
  generator->Update();
  auto data = generator->GetOutput();

  vtkHyperTreeGrid* outputHTG = vtkHyperTreeGrid::SafeDownCast(data);
  if (!outputHTG)
  {
    std::cerr << "Unable to retrieve output HTG." << std::endl;
    return false;
  }

  vtkCellData* outputCellData = outputHTG->GetCellData();
  if (!outputCellData)
  {
    std::cerr << "Unable to retrieve output cell data." << std::endl;
    return false;
  }

  // Test generated random scalars and vectors
  if (!TestRandomUnsignedCharCellArray(
        outputCellData, "RandomCellScalars", outputHTG->GetNumberOfCells(), 1, 0, 255) ||
    !TestRandomUnsignedCharCellArray(
      outputCellData, "RandomCellVectors", outputHTG->GetNumberOfCells(), 3, 0, 255))
  {
    return false;
  }

  // Do regression test on the whole dataset
  const std::string baselinePath = dataRoot + "/Data/HTG/random_attributes.htg";
  vtkNew<vtkXMLHyperTreeGridReader> reader;
  reader->SetFileName(baselinePath.c_str());
  reader->Update();
  auto expectedData = reader->GetOutput();

  if (!vtkTestUtilities::CompareDataObjects(data, expectedData))
  {
    return false;
  }

  return true;
}

//------------------------------------------------------------------------------
/**
 * Test the random attributes filter on a collection containing two HTGs
 */
bool TestRandomAttributesCompositeHTG()
{
  vtkNew<vtkRandomHyperTreeGridSource> htgSource;
  htgSource->SetSeed(42);
  htgSource->SetMaxDepth(3);
  htgSource->SetDimensions(3, 3, 3);
  htgSource->SetSplitFraction(0.5);

  vtkNew<vtkRandomHyperTreeGridSource> htgSource2;
  htgSource2->SetSeed(12);
  htgSource2->SetMaxDepth(3);
  htgSource2->SetDimensions(5, 4, 3);
  htgSource2->SetSplitFraction(0.3);

  vtkNew<vtkGroupDataSetsFilter> groupFilter;
  groupFilter->SetOutputTypeToPartitionedDataSetCollection();
  groupFilter->SetInputConnection(0, htgSource->GetOutputPort());
  groupFilter->AddInputConnection(0, htgSource2->GetOutputPort());

  vtkNew<vtkRandomAttributeGenerator> generator;
  generator->SetInputConnection(groupFilter->GetOutputPort());
  generator->SetDataTypeToUnsignedChar();
  generator->SetComponentRange(0, 255);
  generator->SetGenerateCellScalars(true);
  generator->SetGenerateCellVectors(true);
  generator->Update();

  auto compositeData = vtkDataObjectTree::SafeDownCast(generator->GetOutput());
  if (!compositeData)
  {
    std::cerr << "Unable to retrieve output composite data of HTGs." << std::endl;
    return false;
  }

  vtkSmartPointer<vtkDataObjectTreeIterator> it;
  it.TakeReference(compositeData->NewTreeIterator());
  it->VisitOnlyLeavesOn();
  for (it->InitTraversal(); !it->IsDoneWithTraversal(); it->GoToNextItem())
  {
    vtkHyperTreeGrid* outputHTG = vtkHyperTreeGrid::SafeDownCast(it->GetCurrentDataObject());
    if (!outputHTG)
    {
      std::cerr << "Unable to retrieve output HTG at index " << it->GetCurrentFlatIndex()
                << std::endl;
      return false;
    }

    vtkCellData* outputCellData = outputHTG->GetCellData();
    if (!outputCellData)
    {
      std::cerr << "Unable to retrieve output cell data for HTG at index "
                << it->GetCurrentFlatIndex() << std::endl;
      return false;
    }

    // Test generated random scalars and vectors
    if (!TestRandomUnsignedCharCellArray(
          outputCellData, "RandomCellScalars", outputHTG->GetNumberOfCells(), 1, 0, 255) ||
      !TestRandomUnsignedCharCellArray(
        outputCellData, "RandomCellVectors", outputHTG->GetNumberOfCells(), 3, 0, 255))
    {
      return false;
    }
  }

  return true;
}

//------------------------------------------------------------------------------
int TestRandomAttributeGeneratorHTG(int argc, char* argv[])
{
  vtkNew<vtkTesting> testHelper;
  testHelper->AddArguments(argc, argv);
  if (!testHelper->IsFlagSpecified("-D"))
  {
    std::cerr << "Error: -D /path/to/data was not specified.";
    return EXIT_FAILURE;
  }
  std::string dataRoot = testHelper->GetDataRoot();

  if (!TestRandomAttributesSingleHTG(dataRoot))
  {
    return EXIT_FAILURE;
  }

  if (!TestRandomAttributesCompositeHTG())
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
