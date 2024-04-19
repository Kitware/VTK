// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov

/*
 * TestIOADIOS2VTX_VTU2DRendering.cxx : simple rendering test for unstructured
 *                                      grid data from 2D to 3D
 *
 *  Created on: Jun 19, 2019
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "ADIOSTestUtilities.h"
#include "vtkADIOS2VTXReader.h"

#include <numeric> //std::iota

#include "vtkCamera.h"
#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkDataObject.h"
#include "vtkDataSetMapper.h"
#include "vtkInformation.h"
#include "vtkLookupTable.h"
#if VTK_MODULE_ENABLE_VTK_ParallelMPI
#include "vtkMPI.h"
#include "vtkMPICommunicator.h"
#include "vtkMPIController.h"
#endif
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiPieceDataSet.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSmartPointer.h"
#include "vtkTesting.h"
#include "vtkUnstructuredGrid.h"

#include <adios2.h>

namespace
{
#if VTK_MODULE_ENABLE_VTK_ParallelMPI
MPI_Comm MPIGetComm()
{
  MPI_Comm comm = MPI_COMM_NULL;
  vtkMultiProcessController* controller = vtkMultiProcessController::GetGlobalController();
  vtkMPICommunicator* vtkComm = vtkMPICommunicator::SafeDownCast(controller->GetCommunicator());
  if (vtkComm)
  {
    if (vtkComm->GetMPIComm())
    {
      comm = *(vtkComm->GetMPIComm()->GetHandle());
    }
  }

  return comm;
}

int MPIGetRank()
{
  MPI_Comm comm = MPIGetComm();
  int rank;
  MPI_Comm_rank(comm, &rank);
  return rank;
}
#endif

void WriteBP(const std::string& fileName)
{

  // clang-format off
  const std::vector<std::uint64_t> connectivity = {
          2, 0, 1,
          2, 1, 2,
          2, 2, 3,
          2, 3, 4,
          2, 4, 5};

  const std::vector<double> vertices = { 0,
                                         1,
                                         2,
                                         3,
                                         4,
                                         5,};
  // clang-format on

  std::vector<double> sol(6);
  std::iota(sol.begin(), sol.end(), 1.);

  ADIOS_OPEN(fs, fileName);
  fs.write("types", 3);
  fs.write("connectivity", connectivity.data(), {}, {}, { 5, 3 });
  fs.write("vertices", vertices.data(), {}, {}, { 6, 1 });
  fs.write("sol", sol.data(), {}, {}, { 6 });

  const std::string vtuXML = R"(
  <VTKFile type="UnstructuredGrid">
    <UnstructuredGrid>
      <Piece>
        <Points>
          <DataArray Name="vertices" />
        </Points>
        <Cells>
          <DataArray Name="connectivity" />
          <DataArray Name="types" />
        </Cells>
        <PointData>
          <DataArray Name="sol" />
        </PointData>
      </Piece>
    </UnstructuredGrid>
  </VTKFile>)";

  fs.write_attribute("vtk.xml", vtuXML);
  fs.close();
}

} // end empty namespace

int TestIOADIOS2VTX_VTU1DRendering(int argc, char* argv[])
{
#if VTK_MODULE_ENABLE_VTK_ParallelMPI
  vtkNew<vtkMPIController> mpiController;
  mpiController->Initialize(&argc, &argv, 0);
  vtkMultiProcessController::SetGlobalController(mpiController);
  const int rank = MPIGetRank();
#else
  (void)argc;
  (void)argv;
  const int rank = 0;
#endif

  vtkNew<vtkTesting> testing;
  const std::string rootDirectory(testing->GetTempDirectory());
  const std::string fileName = rootDirectory + "/testVTU1D.bp";
  if (rank == 0)
  {
    WriteBP(fileName);
  }

  vtkNew<vtkADIOS2VTXReader> adios2Reader;
  adios2Reader->SetFileName(fileName.c_str());
  adios2Reader->UpdateInformation();
  adios2Reader->Update();

  vtkMultiBlockDataSet* multiBlock = adios2Reader->GetOutput();
  vtkMultiPieceDataSet* mp = vtkMultiPieceDataSet::SafeDownCast(multiBlock->GetBlock(0));
  vtkUnstructuredGrid* unstructuredGrid = vtkUnstructuredGrid::SafeDownCast(mp->GetPiece(0));

  // set color table
  vtkSmartPointer<vtkLookupTable> lookupTable = vtkSmartPointer<vtkLookupTable>::New();
  lookupTable->SetNumberOfTableValues(10);
  lookupTable->SetRange(0.0, 1.0);
  lookupTable->Build();

  // render unstructured grid
  vtkSmartPointer<vtkDataSetMapper> mapper = vtkSmartPointer<vtkDataSetMapper>::New();
  mapper->SetInputData(unstructuredGrid);
  mapper->SetLookupTable(lookupTable);
  mapper->SelectColorArray("sol");
  mapper->SetScalarModeToUseCellFieldData();

  vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
  actor->SetMapper(mapper);

  vtkSmartPointer<vtkRenderWindow> renderWindow = vtkSmartPointer<vtkRenderWindow>::New();

  vtkSmartPointer<vtkRenderer> renderer = vtkSmartPointer<vtkRenderer>::New();

  // Add both renderers to the window
  renderWindow->AddRenderer(renderer);
  renderer->AddActor(actor);
  renderer->ResetCamera();

  vtkSmartPointer<vtkRenderWindowInteractor> renderWindowInteractor =
    vtkSmartPointer<vtkRenderWindowInteractor>::New();
  renderWindowInteractor->SetRenderWindow(renderWindow);
  renderWindow->Render();

#if VTK_MODULE_ENABLE_VTK_ParallelMPI
  mpiController->Finalize();
#endif

  return EXIT_SUCCESS;
}
