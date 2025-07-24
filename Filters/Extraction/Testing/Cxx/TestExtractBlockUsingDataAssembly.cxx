// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkDataAssembly.h"
#include "vtkDoubleArray.h"
#include "vtkExtractBlockUsingDataAssembly.h"
#include "vtkFieldData.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkNonOverlappingAMR.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkPolyData.h"
#include "vtkUniformGrid.h"

namespace
{
bool TestPDC()
{
  vtkNew<vtkPartitionedDataSetCollection> pdc;
  pdc->Initialize();
  for (int cc = 0; cc < 6; ++cc)
  {
    vtkNew<vtkPartitionedDataSet> pd;
    pdc->SetPartitionedDataSet(cc, pd);
  }

  vtkNew<vtkDoubleArray> da;
  da->SetName("SomeArray");
  pdc->GetFieldData()->AddArray(da);

  vtkNew<vtkDataAssembly> assembly;
  const auto base = assembly->AddNodes({ "blocks", "faces" });
  const auto blocks = assembly->AddNodes({ "b0", "b1" }, base[0]);
  const auto faces = assembly->AddNodes({ "f0", "f1" }, base[1]);
  assembly->AddDataSetIndices(blocks[0], { 0 });
  assembly->AddDataSetIndices(blocks[1], { 1, 2 });
  assembly->AddDataSetIndices(faces[1], { 3, 4 });
  assembly->AddDataSetIndices(base[1], { 5 });
  pdc->SetDataAssembly(assembly);

  vtkNew<vtkExtractBlockUsingDataAssembly> extractor;
  extractor->SetInputDataObject(pdc);
  extractor->SetAssemblyName("Assembly");
  extractor->AddSelector("//b0");
  extractor->AddSelector("//faces");
  extractor->Update();

  auto output = vtkPartitionedDataSetCollection::SafeDownCast(extractor->GetOutputDataObject(0));
  if (output->GetNumberOfPartitionedDataSets() != 4)
  {
    vtkLogF(ERROR, "Incorrect partitioned-datasets, expected=%d, got=%d!", 4,
      static_cast<int>(output->GetNumberOfPartitionedDataSets()));
    return false;
  }

  if (output->GetPartitionedDataSet(0) != pdc->GetPartitionedDataSet(0) ||
    output->GetPartitionedDataSet(1) != pdc->GetPartitionedDataSet(3) ||
    output->GetPartitionedDataSet(2) != pdc->GetPartitionedDataSet(4) ||
    output->GetPartitionedDataSet(3) != pdc->GetPartitionedDataSet(5))
  {
    vtkLogF(ERROR, "Incorrect blocks extracted!");
    return false;
  }

  if (!output->GetFieldData()->GetArray("SomeArray"))
  {
    vtkLogF(ERROR, "Missing field data arrays!");
    return false;
  }

  return true;
}

bool TestAMR()
{
  vtkNew<vtkNonOverlappingAMR> amr;

  // Create and populate the Non Overlapping AMR dataset.
  // The dataset should look like
  // Level 0
  //   uniform grid
  // Level 1
  //   uniform grid
  //   uniform grid
  //   empty node
  std::vector<unsigned int> blocksPerLevel{ 1, 3 };
  amr->Initialize(blocksPerLevel);

  double origin[3] = { 0.0, 0.0, 0.0 };
  double spacing[3] = { 1.0, 1.0, 1.0 };
  int dims[3] = { 11, 11, 6 };

  vtkNew<vtkUniformGrid> ug1;
  // Geometry
  ug1->SetOrigin(origin);
  ug1->SetSpacing(spacing);
  ug1->SetDimensions(dims);

  amr->SetDataSet(0, 0, ug1);

  double origin2[3] = { 0.0, 0.0, 5.0 };
  double spacing2[3] = { 1.0, 0.5, 1.0 };

  vtkNew<vtkUniformGrid> ug2;
  // Geometry
  ug2->SetOrigin(origin2);
  ug2->SetSpacing(spacing2);
  ug2->SetDimensions(dims);

  amr->SetDataSet(1, 0, ug2);

  double origin3[3] = { 0.0, 5.0, 5.0 };

  vtkNew<vtkUniformGrid> ug3;
  // Geometry
  ug3->SetOrigin(origin3);
  ug3->SetSpacing(spacing2);
  ug3->SetDimensions(dims);

  amr->SetDataSet(1, 1, ug3);

  vtkNew<vtkExtractBlockUsingDataAssembly> extractor;
  extractor->SetInputDataObject(amr);
  extractor->SetAssemblyName("Hierarchy");
  extractor->AddSelector("/Root/Level1");
  extractor->Update();

  auto output = vtkPartitionedDataSetCollection::SafeDownCast(extractor->GetOutputDataObject(0));
  if (output->GetNumberOfPartitions(0) != 3)
  {
    vtkLogF(ERROR, "Incorrect AMR extractions number of blocks, expected=%d, got=%d!", 3,
      output->GetNumberOfPartitions(0));
    return false;
  }
  if (output->GetNumberOfCells() != 1000)
  {
    vtkLogF(ERROR, "Incorrect AMR extractions number of cells, expected=%d, got=%d!", 1000,
      static_cast<int>(output->GetNumberOfCells()));
    return false;
  }

  return true;
}
}

int TestExtractBlockUsingDataAssembly(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  bool success = true;
  success &= ::TestPDC();
  success &= ::TestAMR();
  return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
