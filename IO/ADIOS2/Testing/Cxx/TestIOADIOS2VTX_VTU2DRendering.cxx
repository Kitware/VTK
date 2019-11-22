/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestIOADIOS2VTX_VTU2DRendering.cxx

-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*
 * TestIOADIOS2VTX_VTU2DRendering.cxx : simple rendering test for unstructured
 *                                      grid data from 2D to 3D
 *
 *  Created on: Jun 19, 2019
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "vtkADIOS2VTXReader.h"

#include <numeric> //std::iota

#include "vtkCamera.h"
#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkDataObject.h"
#include "vtkDataSetMapper.h"
#include "vtkInformation.h"
#include "vtkLookupTable.h"
#include "vtkMPI.h"
#include "vtkMPICommunicator.h"
#include "vtkMPIController.h"
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

void WriteBP(const std::string& fileName)
{

  // clang-format off
  const std::vector<std::uint64_t> connectivity = {
          4, 0, 1, 2, 3,
          4, 2, 3, 4, 5};

  const std::vector<double> vertices = { 0, 0,
                                         1, 0,
                                         0, 1,
                                         1, 1,
                                         0, 2,
                                         1, 2};
  // clang-format on

  std::vector<double> sol(6);
  std::iota(sol.begin(), sol.end(), 1.);

  adios2::fstream fs(fileName, adios2::fstream::out, MPI_COMM_SELF);
  fs.write("types", 8);
  fs.write("connectivity", connectivity.data(), {}, {}, { 2, 5 });
  fs.write("vertices", vertices.data(), {}, {}, { 6, 2 });
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

int TestIOADIOS2VTX_VTU2DRendering(int argc, char* argv[])
{
  vtkNew<vtkMPIController> mpiController;
  mpiController->Initialize(&argc, &argv, 0);
  vtkMultiProcessController::SetGlobalController(mpiController);
  const int rank = MPIGetRank();

  vtkNew<vtkTesting> testing;
  const std::string rootDirectory(testing->GetTempDirectory());
  const std::string fileName = rootDirectory + "/testVTU2D.bp";
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

  mpiController->Finalize();

  return EXIT_SUCCESS;
}
