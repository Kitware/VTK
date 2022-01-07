/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestIOSSExodus.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * Test for paraview/paraview#17404
 */

#include <vtkCellData.h>
#include <vtkDataSet.h>
#include <vtkIOSSReader.h>
#include <vtkLogger.h>
#include <vtkNew.h>
#include <vtkPartitionedDataSet.h>
#include <vtkPartitionedDataSetCollection.h>
#include <vtkTestUtilities.h>

#include <string>

static std::string GetFileName(int argc, char* argv[], const char* fnameC)
{
  char* fileNameC = vtkTestUtilities::ExpandDataFileName(argc, argv, fnameC);
  std::string fname(fileNameC);
  delete[] fileNameC;
  return fname;
}

#define VERIFY(x, y)                                                                               \
  if ((x) == false)                                                                                \
  {                                                                                                \
    vtkLogF(ERROR, "%s -- failed!", (y));                                                          \
    return EXIT_FAILURE;                                                                           \
  }                                                                                                \
  else                                                                                             \
  {                                                                                                \
    vtkLogF(1, "%s -- success", (y));                                                              \
  }

int TestIOSSAttributes(int argc, char* argv[])
{
  vtkNew<vtkIOSSReader> reader0;
  reader0->AddFileName(
    ::GetFileName(argc, argv, "Data/Exodus/RubiksCubeWithRotations_gold.g").c_str());
  reader0->Update();

  auto pdc = vtkPartitionedDataSetCollection::SafeDownCast(reader0->GetOutputDataObject(0));
  auto pd = pdc ? pdc->GetPartitionedDataSet(0) : nullptr;
  auto ds = pd ? pd->GetPartition(0) : nullptr;
  VERIFY((ds != nullptr), "expected block");
  VERIFY(ds->GetCellData()->GetArray("attribute") != nullptr, "expected 'attribute' array");
  VERIFY(
    ds->GetCellData()->GetArray("rotation_matrix") != nullptr, "expected 'rotation_matrix' array");

  reader0->ClearFileNames();
  reader0->AddFileName(::GetFileName(argc, argv, "Data/Exodus/block_with_attributes.g").c_str());
  reader0->Update();

  pdc = vtkPartitionedDataSetCollection::SafeDownCast(reader0->GetOutputDataObject(0));
  pd = pdc ? pdc->GetPartitionedDataSet(0) : nullptr;
  ds = pd ? pd->GetPartition(0) : nullptr;
  VERIFY((ds != nullptr), "expected block");
  VERIFY(ds->GetCellData()->GetArray("attribute") != nullptr, "expected 'attribute' array");
  VERIFY(ds->GetCellData()->GetArray("block_0_attribute_label") != nullptr,
    "expected 'block_0_attribute_label' array");
  return EXIT_SUCCESS;
}
