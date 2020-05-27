/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestIossNoElementBlocks.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * Test for paraview/paraview#18686
 * Ensures that exodus files without any element blocks and node blocks alone
 * can be read correctly.
 */

#include <vtkDataArraySelection.h>
#include <vtkDataSet.h>
#include <vtkIossReader.h>
#include <vtkLogger.h>
#include <vtkNew.h>
#include <vtkPartitionedDataSet.h>
#include <vtkPartitionedDataSetCollection.h>
#include <vtkPointData.h>
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

int TestIossNoElementBlocks(int argc, char* argv[])
{
  vtkNew<vtkIossReader> reader0;
  reader0->AddFileName(::GetFileName(argc, argv, "Data/Exodus/hello_world_fix-d_frf.frq").c_str());
  reader0->UpdateInformation();
  reader0->GetNodeSetSelection()->EnableAllArrays();
  reader0->Update();

  auto pdc = vtkPartitionedDataSetCollection::SafeDownCast(reader0->GetOutputDataObject(0));
  VERIFY((pdc != nullptr), "expected vtkPartitionedDataSetCollection");
  VERIFY((pdc->GetNumberOfPartitionedDataSets() == 3), "expected 3 partitioned-datasets");
  auto pd = pdc ? pdc->GetPartitionedDataSet(0) : nullptr;
  auto ds = pd ? pd->GetPartition(0) : nullptr;
  VERIFY((ds != nullptr), "expected block");
  VERIFY((ds->GetNumberOfPoints() == 1), "expected 1 points");
  VERIFY(ds->GetPointData()->GetArray("disp") != nullptr, "expected 'disp' array");
  VERIFY(ds->GetPointData()->GetArray("rot") != nullptr, "expected 'rot' array");
  return EXIT_SUCCESS;
}
