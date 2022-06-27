/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestReadCGNSFiles.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
//  Copyright (c) Menno Deij - van Rijswijk, MARIN, The Netherlands
//  All rights reserved.
#include "vtkCGNSReader.h"
#include "vtkCell.h"
#include "vtkCellData.h"
#include "vtkInformation.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkTestUtilities.h"
#include "vtkUnstructuredGrid.h"

#include <string>

#define vtk_assert(x)                                                                              \
  do                                                                                               \
  {                                                                                                \
    if (!(x))                                                                                      \
    {                                                                                              \
      cerr << "On line " << __LINE__ << " ERROR: Condition FAILED!! : " << #x << endl;             \
      return EXIT_FAILURE;                                                                         \
    }                                                                                              \
  } while (false)

int TestOutput(vtkMultiBlockDataSet* mb, int nCells, VTKCellType type);

int TestOutputData(vtkMultiBlockDataSet* mb, int nCells, int nArrays)
{
  unsigned int nBlocks = mb->GetNumberOfBlocks();
  vtk_assert(nBlocks > 0);
  for (unsigned int i = 0; i < nBlocks; ++i)
  {
    vtkMultiBlockDataSet* mb2 = vtkMultiBlockDataSet::SafeDownCast(mb->GetBlock(i));
    for (unsigned int j = 0; j < mb2->GetNumberOfBlocks(); ++j)
    {
      vtkUnstructuredGrid* ug = vtkUnstructuredGrid::SafeDownCast(mb2->GetBlock(j));
      vtkCellData* cd = ug->GetCellData();
      int nArr = cd->GetNumberOfArrays();
      if (nArr != nArrays)
        return 1;
      for (int k = 0; k < nArr; ++k)
      {
        vtkDataArray* arr = cd->GetArray(k);
        vtkIdType nTpl = arr->GetNumberOfTuples();
        vtk_assert(nTpl == nCells);
      }
    }
  }
  return 0;
}

int TestReadCGNSSolution(int argc, char* argv[])
{
  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/channelBump_solution.cgns");
  std::string solution = fname ? fname : "";
  delete[] fname;

  vtkNew<vtkCGNSReader> reader;
  reader->SetFileName(solution.c_str());
  reader->UpdateInformation();
  reader->EnableAllCellArrays();
  reader->EnableAllPointArrays();
  reader->Update();

  vtkMultiBlockDataSet* mb = reader->GetOutput();
  if (0 != TestOutput(mb, 19742, VTK_POLYHEDRON))
  {
    return EXIT_FAILURE;
  }

  if (0 != TestOutputData(mb, 19742, 20))
  {
    return EXIT_FAILURE;
  }

  cout << __FILE__ << " tests passed." << endl;
  return EXIT_SUCCESS;
}
