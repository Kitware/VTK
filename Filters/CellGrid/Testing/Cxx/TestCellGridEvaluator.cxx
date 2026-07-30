// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkCellGrid.h"
#include "vtkCellGridEvaluator.h"
#include "vtkCellGridReader.h"
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

#include <algorithm>
#include <iostream>

namespace
{

double tupleDiffMag(const std::vector<double>& aa, const std::vector<double>& bb, bool& ok)
{
  double diffMag = 0.;
  std::size_t nn = aa.size();
  if (bb.size() < nn)
  {
    nn = bb.size();
    std::cerr << "ERROR: Tuples of different size! (" << aa.size() << " vs " << bb.size() << "\n";
    ok = false;
  }
  for (std::size_t ii = 0; ii < nn; ++ii)
  {
    double compDiff = aa[ii] - bb[ii];
    diffMag += compDiff * compDiff;
  }
  diffMag = std::sqrt(diffMag);
  return diffMag;
}

bool LoadAndEvaluate(const char* filename, const std::vector<std::array<double, 3>>& testPoints,
  const std::vector<bool>& expectedClassifications, const std::string& attributeName,
  const std::vector<std::vector<double>>& expectedValues)
{
  if (!filename)
  {
    return false;
  }
  bool ok = true;

  vtkNew<vtkDoubleArray> coords;
  coords->SetNumberOfComponents(3);
  coords->SetNumberOfTuples(static_cast<vtkIdType>(testPoints.size()));
  vtkIdType ii = 0;
  for (const auto& testPoint : testPoints)
  {
    coords->SetTuple(ii++, testPoint.data());
  }

  vtkNew<vtkCellGridReader> reader;
  reader->SetFileName(filename);
  reader->Update();
  delete[] filename;

  auto* grid = vtkCellGrid::SafeDownCast(reader->GetOutputDataObject(0));
  vtkNew<vtkCellGridEvaluator> evaluator;
  // evaluator->SetCellAttribute(grid->GetShapeAttribute());
  auto cellAtt =
    grid->GetCellAttributeByName(attributeName); // TODO: Try one of every field type we support
  evaluator->SetCellAttribute(cellAtt);
  // Some test points lie exactly on boundaries shared between neighboring cells;
  // use a small tolerance so they classify into every cell that contains them.
  evaluator->SetClassifierTolerance(1e-8);
  // evaluator->ClassifyPoints(coords); // TODO: Test in each mode
  evaluator->InterpolatePoints(coords);
  grid->Query(evaluator);

  auto* cellTypes = evaluator->GetClassifierCellTypes();
  auto* cellOffsets = evaluator->GetClassifierCellOffsets();
  auto* pointIDs = evaluator->GetClassifierPointIDs();
  auto* cellIndices = evaluator->GetClassifierCellIndices();
  auto* pointParams = evaluator->GetClassifierPointParameters();
  auto* values = evaluator->GetInterpolatedValues();
  vtkNew<vtkTable> dumpTable;
  dumpTable->AddColumn(pointIDs);
  dumpTable->AddColumn(cellIndices);
  dumpTable->AddColumn(pointParams);
  dumpTable->AddColumn(values);
  std::cout << "-----\n";
  std::set<vtkIdType> pointsInside;
  std::vector<double> tuple;
  tuple.resize(values->GetNumberOfComponents());
  // The evaluator does not preserve input-point order in its output (points
  // are grouped by containing cell), so compare the interpolated values as a
  // sorted multiset rather than row by row.
  std::vector<std::vector<double>> actualValues;
  for (vtkIdType jj = 0; jj < cellOffsets->GetNumberOfTuples() - 1; ++jj)
  {
    vtkIdType startRow = cellOffsets->GetValue(jj);
    vtkIdType endRow = cellOffsets->GetValue(jj + 1);
    std::cout << "Cell type " << vtkStringToken(cellTypes->GetValue(jj)).Data() << "  rows ["
              << startRow << ", " << endRow << "[\n";
    for (ii = startRow; ii < endRow; ++ii)
    {
      pointsInside.insert(pointIDs->GetValue(ii));
      values->GetTuple(ii, tuple.data());
      actualValues.push_back(tuple);
    }
  }
  dumpTable->Dump(/* column width */ 24);
  if (actualValues.size() != expectedValues.size())
  {
    std::cerr << "ERROR: Expected " << expectedValues.size() << " classified rows, got "
              << actualValues.size() << ".\n";
    ok = false;
  }
  else
  {
    auto sortedExpected = expectedValues;
    std::sort(sortedExpected.begin(), sortedExpected.end());
    std::sort(actualValues.begin(), actualValues.end());
    for (std::size_t vv = 0; vv < actualValues.size(); ++vv)
    {
      double err = tupleDiffMag(actualValues[vv], sortedExpected[vv], ok);
      if (err > 1e-5)
      {
        std::cerr << "ERROR: Value " << vv << " expected to be " << sortedExpected[vv][0] << " got "
                  << actualValues[vv][0] << "\n";
        ok = false;
      }
    }
  }
  for (std::size_t jj = 0; jj < expectedClassifications.size(); ++jj)
  {
    bool isInside = (pointsInside.find(static_cast<vtkIdType>(jj)) != pointsInside.end());
    if (isInside != expectedClassifications[jj])
    {
      std::cerr << "ERROR: Point " << jj << " was expected to be "
                << (expectedClassifications[jj] ? "inside" : "outside") << " but was not.\n";
      ok = false;
    }
  }
  return ok;
}

} // anonymous namespace

int TestCellGridEvaluator(int argc, char* argv[])
{
  if (!LoadAndEvaluate(vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/dgHexahedra.dg", 0),
        { { 0., 0., 0. }, { 1., 0., 0. }, { 1.5, 0.5, 0.5 }, { 2.5, 0.5, 0.5 }, { 0.3, 0.3, 0.3 } },
        { false, true, true, false, true }, "scalar1", { { 3 }, { 0 }, { 1.5 }, { 1.10967 } }))
  {
    return EXIT_FAILURE;
  }

  if (!LoadAndEvaluate(vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/dgTetrahedra.dg", 0),
        { { 0.5, 0.5, 0. }, { 1., 0., 0. }, { 0.8, 0.8, 0.05 }, { 2.5, 0.5, 0.5 },
          { 0.5, 0.4, 0.1 } },
        { true, true, true, false, true }, "scalar1",
        { { 2.5 }, { 0.5 }, { 3 }, { 0 }, { 0.25 }, { 2 } }))
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
