// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkCellData.h"
#include "vtkDataSetAttributes.h"
#include "vtkDataSetTriangleFilter.h"
#include "vtkExplodeDataSet.h"
#include "vtkGeometryFilter.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkNew.h"
#include "vtkNumberToString.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkRandomAttributeGenerator.h"
#include "vtkTestUtilities.h"
#include "vtkUnstructuredGrid.h"
#include "vtkXMLImageDataReader.h"

namespace details
{
bool CheckOutput(vtkDataSet* input, vtkPartitionedDataSetCollection* output, unsigned int nbOfParts,
  const std::string& arrayName)
{
  if (output->GetNumberOfPartitionedDataSets() != nbOfParts)
  {
    std::cerr << "Output has " << output->GetNumberOfPartitionedDataSets() << " parts instead of "
              << nbOfParts << std::endl;
    return false;
  }

  if (output->GetNumberOfCells() != input->GetNumberOfCells())
  {
    std::cerr << "Output has wrong number of cells\n";
    return false;
  }

  const int nbOfPointsArrays = input->GetPointData()->GetNumberOfArrays();

  for (unsigned int cc = 0; cc < nbOfParts; ++cc)
  {
    vtkDataSet* outputDS = vtkDataSet::SafeDownCast(output->GetPartitionAsDataObject(cc, 0));
    auto partIdArray = outputDS->GetFieldData()->GetArray(arrayName.c_str());
    if (partIdArray->GetNumberOfValues() != 1)
    {
      std::cerr << "Partition should have a single value FieldData\n";
    }
    double partId = partIdArray->GetTuple1(0);
    vtkNumberToString converter;
    auto blockname = arrayName + "_" + converter.Convert(partId);
    auto name = output->GetMetaData(cc)->Get(vtkCompositeDataSet::NAME());
    if (name == nullptr || blockname != name)
    {
      cerr << "Mismatched block names" << endl;
      return false;
    }

    if (outputDS->GetPointData()->GetNumberOfArrays() != nbOfPointsArrays)
    {
      std::cerr << "Output has wrong number of arrays.\n";
    }
  }

  return true;
}

bool TestDataSet(vtkDataSet* dataset)
{
  vtkDataArray* scalars = dataset->GetCellData()->GetArray(0);
  std::string arrayName = scalars->GetName();
  const unsigned int nbOfParts = 6;

  vtkNew<vtkExplodeDataSet> split;
  split->SetInputDataObject(dataset);
  split->SetInputArrayToProcess(arrayName.c_str(), vtkDataObject::CELL);
  split->Update();

  vtkPartitionedDataSetCollection* output = split->GetOutput();
  return details::CheckOutput(dataset, output, nbOfParts, arrayName);
}
};

int TestExplodeDataSet(int argc, char* argv[])
{
  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/waveletMaterial.vti");

  vtkNew<vtkXMLImageDataReader> reader;
  reader->SetFileName(fname);
  reader->Update();
  delete[] fname;

  // add some data arrays: they should be forwarded
  vtkNew<vtkRandomAttributeGenerator> gen;
  gen->SetInputConnection(reader->GetOutputPort());
  gen->GeneratePointScalarsOn();
  gen->GenerateCellVectorsOn();
  gen->Update();
  vtkDataSet* data = vtkDataSet::SafeDownCast(gen->GetOutputDataObject(0));
  if (!details::TestDataSet(data))
  {
    std::cerr << "Split fails for image input\n";
    return EXIT_FAILURE;
  }

  vtkNew<vtkDataSetTriangleFilter> triangulate;
  triangulate->SetInputConnection(reader->GetOutputPort());
  triangulate->Update();

  if (!details::TestDataSet(triangulate->GetOutput()))
  {
    std::cerr << "Split fails for unstructured input\n";
    return EXIT_FAILURE;
  }

  vtkNew<vtkGeometryFilter> geom;
  geom->SetInputConnection(triangulate->GetOutputPort());
  geom->MergingOff();
  geom->Update();

  if (!details::TestDataSet(geom->GetOutput()))
  {
    std::cerr << "Split fails for polydata input\n";
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
