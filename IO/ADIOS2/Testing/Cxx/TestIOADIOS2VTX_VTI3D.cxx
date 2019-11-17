/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestIOADIOS2VTX_VTI3D.cxx

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
 * TestIOADIOS2VTX_VTI3D.cxx : pipeline tests for image data reader 1D and 3D
 * vars
 *
 *  Created on: Jun 13, 2019
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "vtkADIOS2VTXReader.h"

#include <algorithm> //std::equal
#include <array>
#include <iostream>
#include <numeric> //std::iota
#include <string>
#include <vector>

#include "vtkAlgorithm.h"
#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkDemandDrivenPipeline.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMPI.h"
#include "vtkMPICommunicator.h"
#include "vtkMPIController.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiPieceDataSet.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTestUtilities.h"

#include <adios2.h>
#include <vtksys/SystemTools.hxx>

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

int MPIGetSize()
{
  MPI_Comm comm = MPIGetComm();
  int size;
  MPI_Comm_size(comm, &size);
  return size;
}

template <class T>
void ExpectEqual(const T& one, const T& two, const std::string& message)
{
  if (one != two)
  {
    std::ostringstream messageSS;
    messageSS << "ERROR: found different values, " << one << " and " << two << " , " << message
              << "\n";

    throw std::logic_error(messageSS.str());
  }
}

template <class T>
void TStep(std::vector<T>& data, const size_t step, const int rank)
{
  const T initialValue = static_cast<T>(step + rank);
  std::iota(data.begin(), data.end(), initialValue);
}

template <class T>
bool CompareData(
  const std::string& name, vtkImageData* imageData, const size_t step, const int rank)
{
  vtkDataArray* vtkInput = imageData->GetCellData()->GetArray(name.c_str());

  // expected
  std::vector<T> Texpected(vtkInput->GetDataSize());
  TStep(Texpected, step, rank);

  T* vtkInputData = reinterpret_cast<T*>(vtkInput->GetVoidPointer(0));
  return std::equal(Texpected.begin(), Texpected.end(), vtkInputData);
}
} // end empty namespace

// Pipeline
class TesterVTI3D : public vtkAlgorithm
{
public:
  static TesterVTI3D* New();
  vtkTypeMacro(TesterVTI3D, vtkAlgorithm);
  TesterVTI3D()
  {
    this->SetNumberOfInputPorts(1);
    this->SetNumberOfOutputPorts(0);
  }

  void Init(const std::string& streamName, const size_t steps)
  {
    this->StreamName = streamName;
    this->Steps = steps;
  }

  int ProcessRequest(
    vtkInformation* request, vtkInformationVector** input, vtkInformationVector* output)
  {
    if (request->Has(vtkStreamingDemandDrivenPipeline::REQUEST_UPDATE_EXTENT()))
    {
      vtkInformation* inputInfo = input[0]->GetInformationObject(0);
      inputInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(),
        static_cast<double>(this->CurrentStep));
      return 1;
    }

    if (request->Has(vtkDemandDrivenPipeline::REQUEST_DATA()))
    {
      vtkMultiBlockDataSet* inputMultiBlock =
        vtkMultiBlockDataSet::SafeDownCast(this->GetInputDataObject(0, 0));

      if (!DoCheckData(inputMultiBlock))
      {
        throw std::invalid_argument("ERROR: data check failed\n");
      }

      ++this->CurrentStep;
      return 1;
    }
    return this->Superclass::ProcessRequest(request, input, output);
  }

protected:
  size_t CurrentStep = 0;
  std::string StreamName;
  size_t Steps = 1;

private:
  int FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info) final
  {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkImageData");
    return 1;
  }

  bool DoCheckData(vtkMultiBlockDataSet* multiBlock)
  {
    if (multiBlock == nullptr)
    {
      return false;
    }
    vtkMultiPieceDataSet* multiPiece = vtkMultiPieceDataSet::SafeDownCast(multiBlock->GetBlock(0));
    const size_t step = static_cast<size_t>(this->CurrentStep);
    const int rank = MPIGetRank();
    vtkImageData* imageData = vtkImageData::SafeDownCast(multiPiece->GetPiece(rank));

    if (!CompareData<double>("Tdouble", imageData, step, rank))
    {
      return false;
    }
    if (!CompareData<float>("Tfloat", imageData, step, rank))
    {
      return false;
    }
    if (!CompareData<int64_t>("Tint64", imageData, step, rank))
    {
      return false;
    }
    if (!CompareData<uint64_t>("Tuint64", imageData, step, rank))
    {
      return false;
    }
    if (!CompareData<int32_t>("Tint32", imageData, step, rank))
    {
      return false;
    }
    if (!CompareData<uint32_t>("Tuint32", imageData, step, rank))
    {
      return false;
    }

    return true;
  }
};

vtkStandardNewMacro(TesterVTI3D);

int TestIOADIOS2VTX_VTI3D(int argc, char* argv[])
{
  auto lf_DoTest = [&](const std::string& fileName, const size_t steps) {
    vtkNew<vtkADIOS2VTXReader> adios2Reader;
    adios2Reader->SetFileName(fileName.c_str());
    // check FileName
    char* outFileName = adios2Reader->GetFileName();
    ExpectEqual(fileName, std::string(outFileName), " file names");
    // check PrintSelf
    adios2Reader->Print(std::cout);

    vtkNew<TesterVTI3D> testerVTI3D;
    testerVTI3D->Init(fileName, steps);
    testerVTI3D->SetInputConnection(adios2Reader->GetOutputPort());

    for (size_t s = 0; s < steps; ++s)
    {
      testerVTI3D->UpdateInformation();
      testerVTI3D->Update();
    }
  };

  vtkNew<vtkMPIController> mpiController;
  mpiController->Initialize(&argc, &argv, 0);
  vtkMultiProcessController::SetGlobalController(mpiController);
  const int rank = MPIGetRank();
  const int size = MPIGetSize();

  const size_t steps = 3;
  // this are cell data dimensions
  const adios2::Dims count{ 10, 10, 4 };
  const adios2::Dims start{ static_cast<size_t>(rank) * count[0], 0, 0 };
  const adios2::Dims shape{ static_cast<size_t>(size) * count[0], count[1], count[2] };

  char* filePath;
  std::string fileName;

  const std::vector<std::string> directories = { "bp3", "bp4" };
  constexpr std::array<unsigned int, 4> ids = { 1, 2, 3, 4 };

  for (const std::string& dir : directories)
  {
    // 3D tests
    for (const auto id : ids)
    {
      fileName = "Data/ADIOS2/vtx/" + dir + "/heat3D_" + std::to_string(id) + ".bp";
      filePath = vtkTestUtilities::ExpandDataFileName(argc, argv, fileName.c_str());
      lf_DoTest(filePath, steps);
    }

    // 1D tests
    fileName = "Data/ADIOS2/vtx/" + dir + "/heat3D_1.bp";
    filePath = vtkTestUtilities::ExpandDataFileName(argc, argv, fileName.c_str());
    lf_DoTest(filePath, steps);
  }

  mpiController->Finalize();
  return 0;
}
