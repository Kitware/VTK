// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov

/*
 * TestIOADIOS2VTX_VTU3DRendering.cxx : simple rendering test for unstructured
 *                                      grid data
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
  const std::vector<std::uint64_t> connectivity = { 8, 0, 12, 32, 15, 20, 33, 43, 36, 8, 1, 24, 38, 13,
    21, 39, 44, 34, 8, 12, 1, 13, 32, 33, 21, 34, 43, 8, 32, 13, 4, 14, 43, 34, 22, 35, 8, 15, 32,
    14, 3, 36, 43, 35, 23, 8, 20, 33, 43, 36, 6, 16, 37, 19, 8, 33, 21, 34, 43, 16, 7, 17, 37, 8,
    43, 34, 22, 35, 37, 17, 10, 18, 8, 36, 43, 35, 23, 19, 37, 18, 9, 8, 24, 2, 25, 38, 39, 30, 40,
    44, 8, 38, 25, 5, 26, 44, 40, 31, 41, 8, 13, 38, 26, 4, 34, 44, 41, 22, 8, 21, 39, 44, 34, 7,
    27, 42, 17, 8, 39, 30, 40, 44, 27, 8, 28, 42, 8, 44, 40, 31, 41, 42, 28, 11, 29, 8, 34, 44, 41,
    22, 17, 42, 29, 10 };

  const std::vector<double> vertices = { 3.98975, -0.000438888, -0.0455599, 4.91756, -0.0080733,
    -0.149567, 5.86422, -0.00533255, -0.38101, 3.98975, 1.00044, -0.0455599, 4.91756, 1.00807,
    -0.149567, 5.86422, 1.00533, -0.38101, 4.01025, 0.000438888, 0.95444, 5.08244, 0.0080733,
    0.850433, 6.13578, 0.00533255, 0.61899, 4.01025, 0.999561, 0.95444, 5.08244, 0.991927, 0.850433,
    6.13578, 0.994667, 0.61899, 4.45173, -0.00961903, -0.0802818, 4.91711, 0.5, -0.153657, 4.45173,
    1.00962, -0.0802818, 3.98987, 0.5, -0.0457531, 4.54827, 0.00961903, 0.919718, 5.08289, 0.5,
    0.846343, 4.54827, 0.990381, 0.919718, 4.01013, 0.5, 0.954247, 4, 1.17739e-13, 0.454655, 5,
    3.36224e-12, 0.354149, 5, 1, 0.354149, 4, 1, 0.454655, 5.38824, -0.00666013, -0.252066, 5.86382,
    0.5, -0.383679, 5.38824, 1.00666, -0.252066, 5.61176, 0.00666013, 0.747934, 6.13618, 0.5,
    0.616321, 5.61176, 0.99334, 0.747934, 6, -1.7895e-12, 0.121648, 6, 1, 0.121648, 4.4528, 0.5,
    -0.0845428, 4.5, -1.95761e-12, 0.425493, 5, 0.5, 0.350191, 4.5, 1, 0.425493, 4, 0.5, 0.454445,
    4.5472, 0.5, 0.915457, 5.38782, 0.5, -0.255387, 5.5, 6.97152e-13, 0.251323, 6, 0.5, 0.118984,
    5.5, 1, 0.251323, 5.61218, 0.5, 0.744613, 4.5, 0.5, 0.421259, 5.5, 0.5, 0.247968 };

  const std::vector<std::int32_t> material = { 1, 2, 3 , 4 , 5 , 6, 7, 8, 9, 10,
                                               10, 10, 10, 10, 10, 10 };
  // clang-format on

  std::vector<double> sol(45);
  std::iota(sol.begin(), sol.end(), 1.);

  ADIOS_OPEN(fs, fileName);
  fs.write("types", 11);
  fs.write("connectivity", connectivity.data(), {}, {}, { 16, 9 });
  fs.write("material", material.data(), {}, {}, { 16 });
  fs.write("vertices", vertices.data(), {}, {}, { 45, 3 });
  fs.write("sol", sol.data(), {}, {}, { 45 });

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
        <CellData>
          <DataArray Name="material" />
        </CellData>
      </Piece>
    </UnstructuredGrid>
  </VTKFile>)";

  fs.write_attribute("vtk.xml", vtuXML);
  fs.close();
}

} // end empty namespace

int TestIOADIOS2VTX_VTU3DRendering(int argc, char* argv[])
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
  const std::string fileName = rootDirectory + "/testVTU3D.bp";
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
