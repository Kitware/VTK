// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// Tests issue reported in paraview/paraview#17840

#include <vtkCompositeDataIterator.h>
#include <vtkDataSet.h>
#include <vtkMultiBlockDataSet.h>
#include <vtkMultiBlockPLOT3DReader.h>
#include <vtkNew.h>
#include <vtkTestUtilities.h>

int TestBigEndianPlot3D(int argc, char* argv[])
{
  char* filename = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/bigendian.xyz");
  vtkNew<vtkMultiBlockPLOT3DReader> reader;
  reader->SetFileName(filename);
  delete[] filename;

  reader->AutoDetectFormatOn();
  reader->Update();

  vtkIdType numPts = 0;
  if (vtkMultiBlockDataSet* mb = vtkMultiBlockDataSet::SafeDownCast(reader->GetOutputDataObject(0)))
  {
    auto iter = mb->NewIterator();
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
      if (auto ds = vtkDataSet::SafeDownCast(iter->GetCurrentDataObject()))
      {
        numPts += ds->GetNumberOfPoints();
      }
    }
    iter->Delete();
  }
  return numPts == 24 ? EXIT_SUCCESS : EXIT_FAILURE;
}
