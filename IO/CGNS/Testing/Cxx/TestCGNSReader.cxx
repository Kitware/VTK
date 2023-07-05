// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Menno Deij - van Rijswijk, MARIN, The Netherlands
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkCGNSReader.h"
#include "vtkCell.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkStructuredGrid.h"
#include "vtkTestUtilities.h"
#include "vtkUnstructuredGrid.h"

#define vtk_assert(x)                                                                              \
  do                                                                                               \
  {                                                                                                \
    if (!(x))                                                                                      \
    {                                                                                              \
      cerr << "On line " << __LINE__ << " ERROR: Condition FAILED!! : " << #x << endl;             \
      return EXIT_FAILURE;                                                                         \
    }                                                                                              \
  } while (false)

int TestOutput(vtkMultiBlockDataSet* mb, int nCells, VTKCellType type)
{
  unsigned int nBlocks = mb->GetNumberOfBlocks();
  vtk_assert(nBlocks > 0);
  for (unsigned int i = 0; i < nBlocks; ++i)
  {
    vtkMultiBlockDataSet* mb2 = vtkMultiBlockDataSet::SafeDownCast(mb->GetBlock(i));
    for (unsigned int j = 0; j < mb2->GetNumberOfBlocks(); ++j)
    {
      vtkUnstructuredGrid* ug = vtkUnstructuredGrid::SafeDownCast(mb2->GetBlock(j));
      int nc = ug->GetNumberOfCells();
      vtk_assert(nc == nCells);
      for (vtkIdType k = 0; k < ug->GetNumberOfCells(); ++k)
      {
        vtkCell* cell = ug->GetCell(k);
        vtk_assert(cell->GetCellType() == type);
      }
    }
  }
  return 0;
}

int TestCGNSReader(int argc, char* argv[])
{
  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/Example_mixed.cgns");
  std::string mixed = fname ? fname : "";
  delete[] fname;

  cout << "Opening " << mixed << endl;
  vtkNew<vtkCGNSReader> mixedReader;
  mixedReader->SetFileName(mixed.c_str());
  mixedReader->Update();

  vtkMultiBlockDataSet* mb = mixedReader->GetOutput();

  if (0 != TestOutput(mb, 7, VTK_HEXAHEDRON))
  {
    return EXIT_FAILURE;
  }

  fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/Example_nface_n.cgns");
  std::string nfacen = fname ? fname : "";
  delete[] fname;

  cout << "Opening " << nfacen << endl;
  vtkNew<vtkCGNSReader> nfacenReader;
  nfacenReader->SetFileName(nfacen.c_str());
  nfacenReader->Update();
  mb = nfacenReader->GetOutput();

  if (0 != TestOutput(mb, 7, VTK_POLYHEDRON))
  {
    return EXIT_FAILURE;
  }

  fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/Example_ngon_pe.cgns");
  std::string ngonpe = fname ? fname : "";
  delete[] fname;

  cout << "Opening " << ngonpe.c_str() << endl;
  vtkNew<vtkCGNSReader> ngonpeReader;
  ngonpeReader->SetFileName(ngonpe.c_str());
  ngonpeReader->Update();
  mb = ngonpeReader->GetOutput();

  if (0 != TestOutput(mb, 7, VTK_POLYHEDRON))
  {
    return EXIT_FAILURE;
  }

  fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/Example_ngon_2d_base.cgns");
  std::string ngon_2d_base = fname ? fname : "";
  delete[] fname;

  cout << "Opening " << ngon_2d_base.c_str() << endl;
  vtkNew<vtkCGNSReader> ngonBaseReader;
  ngonBaseReader->SetFileName(ngon_2d_base.c_str());
  ngonBaseReader->UpdateInformation();
  ngonBaseReader->EnableAllBases();
  ngonBaseReader->EnableAllFamilies();
  ngonBaseReader->EnableAllCellArrays();
  ngonBaseReader->Update();
  mb = ngonBaseReader->GetOutput();
  if (mb->GetNumberOfBlocks() != 2)
  {
    return EXIT_FAILURE;
  }

  fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/test_cylinder.cgns");
  std::string bcfile = fname ? fname : "";
  delete[] fname;

  cout << "Opening " << bcfile << endl;
  vtkNew<vtkCGNSReader> bcReader;
  bcReader->SetFileName(bcfile.c_str());
  bcReader->Update();

  cout << __FILE__ << " tests passed." << endl;
  return EXIT_SUCCESS;
}
