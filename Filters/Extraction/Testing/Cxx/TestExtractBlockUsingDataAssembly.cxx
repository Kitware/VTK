/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestExtractBlockUsingDataAssembly.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include <vtkDataAssembly.h>
#include <vtkDoubleArray.h>
#include <vtkExtractBlockUsingDataAssembly.h>
#include <vtkFieldData.h>
#include <vtkLogger.h>
#include <vtkNew.h>
#include <vtkPartitionedDataSet.h>
#include <vtkPartitionedDataSetCollection.h>
#include <vtkPolyData.h>

int TestExtractBlockUsingDataAssembly(int, char*[])
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
    return EXIT_FAILURE;
  }

  if (output->GetPartitionedDataSet(0) != pdc->GetPartitionedDataSet(0) ||
    output->GetPartitionedDataSet(1) != pdc->GetPartitionedDataSet(3) ||
    output->GetPartitionedDataSet(2) != pdc->GetPartitionedDataSet(4) ||
    output->GetPartitionedDataSet(3) != pdc->GetPartitionedDataSet(5))
  {
    vtkLogF(ERROR, "Incorrect blocks extracted!");
    return EXIT_FAILURE;
  }

  if (!output->GetFieldData()->GetArray("SomeArray"))
  {
    vtkLogF(ERROR, "Missing field data arrays!");
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
