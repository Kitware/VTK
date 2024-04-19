// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) 2021, Triad National Security, LLC.
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-LANL-USGov

#include "vtkMPIController.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiPieceDataSet.h"
#include "vtkMultiProcessController.h"
#include "vtkPIOReader.h"
#include "vtkTestUtilities.h"
#include "vtkTesting.h"
#include "vtkUnstructuredGrid.h"

//----------------------------------------------------------------------------
int TestPIOReaderHDF5(int argc, char* argv[])
{
  vtkNew<vtkMPIController> contr;

  contr->Initialize(&argc, &argv);
  vtkMultiProcessController::SetGlobalController(contr);

  int retVal = EXIT_SUCCESS;
  int myrank = contr->GetLocalProcessId();
  int numRanks = contr->GetNumberOfProcesses();
  if (numRanks != 2)
  {
    if (myrank == 0)
    {
      std::cerr << "Error : Number of processes needs to be 2. It is currently " << numRanks << "."
                << std::endl;
    }
    retVal = EXIT_FAILURE;
  }

  // open hdf5 pio file
  vtkNew<vtkTesting> testHelper;
  testHelper->AddArguments(argc, argv);
  if (!testHelper->IsFlagSpecified("-D"))
  {
    if (myrank == 0)
    {
      std::cerr << "Error : -D /path/to/data was not specified." << std::endl;
    }
    retVal = EXIT_FAILURE;
  }
  std::string dataRoot = testHelper->GetDataRoot();
  std::string inputFileName = dataRoot + "/Data/PIO/simple_h5.pio";

  vtkNew<vtkPIOReader> reader;
  reader->SetFileName(inputFileName.c_str());
  reader->UpdateInformation();

  // TODO: confirm default arrays are enabled
  // default_arrays = ["tev", "pres", "rade", "cell_energy", "kemax",
  //        "vel", "eng", "rho"]
  // selection = pioreader.GetCellDataArraySelection()
  // for name in default_arrays:
  //    if not selection.ArrayExists(name) or selection.ArrayIsEnabled(name):
  //        # all's well
  //        pass
  //    else:
  //        raise RuntimeError("'%s' should have been enabled by default." % name)

  reader->SetCurrentTimeStep(1);
  reader->Update();

  vtkMultiBlockDataSet* multiblock = reader->GetOutput();
  vtkMultiPieceDataSet* multipiece = vtkMultiPieceDataSet::SafeDownCast(multiblock->GetBlock(0));
  vtkUnstructuredGrid* ug = vtkUnstructuredGrid::SafeDownCast(multipiece->GetPiece(myrank));

  int numBlocks = multiblock->GetNumberOfBlocks();
  int numCells = ug->GetNumberOfCells();
  int numPoints = ug->GetNumberOfPoints();

  if (myrank == 0)
  {
    if (numBlocks != 1)
    {
      std::cerr << "Error : Number of blocks is not correct. Expected 1, got " << numBlocks << "."
                << std::endl;
      retVal = EXIT_FAILURE;
    }
    if (numCells != 8)
    {
      std::cerr << "Error : Number of cells is not correct. Expected 8, got " << numCells << "."
                << std::endl;
      retVal = EXIT_FAILURE;
    }
    if (numPoints != 27)
    {
      std::cerr << "Error : Number of points is not correct. Expected 27, got " << numPoints << "."
                << std::endl;
      retVal = EXIT_FAILURE;
    }
  }
  else if (myrank == 1)
  {
    if (numBlocks != 1)
    {
      std::cerr << "Error : Number of blocks is not correct. Expected 1, got " << numBlocks << "."
                << std::endl;
      retVal = EXIT_FAILURE;
    }
    if (numCells != 8)
    {
      std::cerr << "Error : Number of cells is not correct. Expected 8, got " << numCells << "."
                << std::endl;
      retVal = EXIT_FAILURE;
    }
    if (numPoints != 27)
    {
      std::cerr << "Error : Number of points is not correct. Expected 27, got " << numPoints << "."
                << std::endl;
      retVal = EXIT_FAILURE;
    }
  }

  vtkMultiProcessController::SetGlobalController(nullptr);
  contr->Finalize();
  return retVal;
}
