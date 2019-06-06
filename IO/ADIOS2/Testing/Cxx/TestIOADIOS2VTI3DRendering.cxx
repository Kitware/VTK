/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestADIOS2ReaderRendering.cxx

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
 * TestADIOS2ReaderRendering.cxx : simple rendering test
 *
 *  Created on: Jun 19, 2019
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include <numeric>

#include "vtkADIOS2ReaderMultiBlock.h"
#include "vtkCamera.h"
#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkDataObject.h"
#include "vtkDataSetMapper.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkLookupTable.h"
#include "vtkMPI.h"
#include "vtkMPICommunicator.h"
#include "vtkMPIController.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiPieceDataSet.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSmartPointer.h"

#include <adios2.h>

namespace
{
MPI_Comm MPIGetComm()
{
  MPI_Comm comm = nullptr;
  vtkMultiProcessController* controller = vtkMultiProcessController::GetGlobalController();
  vtkMPICommunicator* vtkComm = vtkMPICommunicator::SafeDownCast(controller->GetCommunicator());
  if (vtkComm)
  {
    if (vtkComm->GetMPIComm())
    {
      comm = *(vtkComm->GetMPIComm()->GetHandle());
    }
  }

  if (comm == nullptr || comm == MPI_COMM_NULL)
  {
    throw std::runtime_error("ERROR: ADIOS2 requires MPI communicator for parallel reads\n");
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

int MPIGetSize()
{
  MPI_Comm comm = MPIGetComm();
  int size;
  MPI_Comm_size(comm, &size);
  return size;
}

std::size_t TotalElements(const std::vector<std::size_t>& dimensions) noexcept
{
  return std::accumulate(dimensions.begin(), dimensions.end(), 1, std::multiplies<std::size_t>());
}

void WriteBPFile3DVars(const std::string& fileName, const adios2::Dims& shape,
  const adios2::Dims& start, const adios2::Dims& count, const int rank)
{
  const size_t totalElements = TotalElements(count);

  const std::string extent = "0 " + std::to_string(shape[0] + 1) + " " + "0 " +
    std::to_string(shape[1] + 1) + " " + "0 " + std::to_string(shape[2] + 1);

  const std::string imageSchema = R"( <?xml version="1.0"?>
      <VTKFile type="ImageData" version="0.1" byte_order="LittleEndian">
        <ImageData WholeExtent=")" + extent + R"(" Origin="0 0 0" Spacing="1 1 1">
          <Piece Extent=")" + extent + R"(">
            <CellData>
              <DataArray Name="T" />
              <DataArray Name="TIME">
                time
              </DataArray>
            </CellData>
          </Piece>
        </ImageData>
      </VTKFile>)";

  // using adios2 C++ high-level API
  std::vector<double> T(totalElements);
  std::iota(T.begin(), T.end(), static_cast<double>(rank * totalElements));

  adios2::fstream fw(fileName, adios2::fstream::out, MPIGetComm());
  fw.write_attribute("vtk.xml", imageSchema);
  fw.write("time", 0);
  fw.write("T", T.data(), shape, start, count);
  fw.close();
}

} // end empty namespace

int TestIOADIOS2VTI3DRendering(int argc, char* argv[])
{
  vtkNew<vtkMPIController> mpiController;
  mpiController->Initialize(&argc, &argv, 0);
  vtkMultiProcessController::SetGlobalController(mpiController);

  const int rank = MPIGetRank();
  const int size = MPIGetSize();

  const std::string fileName = "heat3D_render.bp";
  const adios2::Dims count{ 4, 4, 8 };
  const adios2::Dims start{ static_cast<size_t>(rank) * count[0], 0, 0 };
  const adios2::Dims shape{ static_cast<size_t>(size) * count[0], count[1], count[2] };

  WriteBPFile3DVars(fileName, shape, start, count, rank);

  vtkNew<vtkADIOS2ReaderMultiBlock> adios2Reader;
  adios2Reader->SetFileName(fileName.c_str());
  adios2Reader->UpdateInformation();
  adios2Reader->Update();

  vtkMultiBlockDataSet* multiBlock = adios2Reader->GetOutput();
  vtkMultiPieceDataSet* mp = vtkMultiPieceDataSet::SafeDownCast(multiBlock->GetBlock(0));
  vtkImageData* imageData = vtkImageData::SafeDownCast(mp->GetPiece(0));

  double* data =
    reinterpret_cast<double*>(imageData->GetCellData()->GetArray("T")->GetVoidPointer(0));

  for (size_t i = 0; i < 128; ++i)
  {
    if (data[i] != static_cast<double>(i))
    {
      throw std::invalid_argument("ERROR: invalid source data for rendering\n");
    }
  }

  // set color table
  vtkSmartPointer<vtkLookupTable> lookupTable = vtkSmartPointer<vtkLookupTable>::New();
  lookupTable->SetNumberOfTableValues(10);
  lookupTable->SetRange(0.0, 1.0);
  lookupTable->Build();

  // render imageData
  vtkSmartPointer<vtkDataSetMapper> mapper = vtkSmartPointer<vtkDataSetMapper>::New();
  mapper->SetInputData(imageData);
  mapper->SetLookupTable(lookupTable);
  mapper->SelectColorArray("T");
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
