/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestADIOS2ReaderVTI3D.cxx

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
 * TestADIOS2ReaderVTI3D.cxx : pipeline tests for image data reader 1D and 3D vars
 *
 *  Created on: Jun 13, 2019
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include <algorithm> //std::equal
#include <iostream>
#include <numeric> //std::iota
#include <string>
#include <vector>

#include "vtkADIOS2ReaderMultiBlock.h"
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

#include <adios2.h>
#include <vtksys/SystemTools.hxx>

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

template<class T>
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

template<class T>
void TStep(std::vector<T>& data, const size_t step, const int rank)
{
  const T initialValue = static_cast<T>(step + rank);
  std::iota(data.begin(), data.end(), initialValue);
}

template<class T>
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

namespace
{

void WriteBPFile3DVars(const std::string& fileName, const adios2::Dims& shape,
  const adios2::Dims& start, const adios2::Dims& count, const size_t steps, const int rank,
  const bool isAttribute, const bool hasTime, const bool isCellData = true)
{
  const size_t totalElements = TotalElements(count);

  const size_t Nx = isCellData ? shape[0] + 1 : shape[0];
  const size_t Ny = isCellData ? shape[1] + 1 : shape[1];
  const size_t Nz = isCellData ? shape[2] + 1 : shape[2];

  const std::string extent =
    "0 " + std::to_string(Nx) + " " + "0 " + std::to_string(Ny) + " " + "0 " + std::to_string(Nz);

  const std::string dataSetType = isCellData ? "CellData" : "PointData";

  const std::string timeStr = hasTime ? R"(
   <DataArray Name="TIME">
       time
   </DataArray> )" : "";

  const std::string imageSchema = R"(<?xml version="1.0"?>
      <VTKFile type="ImageData" version="0.1" byte_order="LittleEndian">
        <ImageData WholeExtent=")" + extent + R"(" Origin="0 0 0" Spacing="1 1 1">
          <Piece Extent=")" + extent + R"(">
            <)" + dataSetType + R"(>
                <DataArray Name="Tdouble" />
                <DataArray Name="Tfloat" />
                <DataArray Name="Tint64" />
                <DataArray Name="Tuint64" />
                <DataArray Name="Tint32" />
                <DataArray Name="Tuint32" />
                )" + timeStr + R"(
            </)" + dataSetType + R"(>
          </Piece>
        </ImageData>
      </VTKFile>)";

  // using adios2 C++ high-level API
  std::vector<double> Tdouble(totalElements);
  std::vector<float> Tfloat(totalElements);
  std::vector<int64_t> Tint64(totalElements);
  std::vector<uint64_t> Tuint64(totalElements);
  std::vector<int32_t> Tint32(totalElements);
  std::vector<uint32_t> Tuint32(totalElements);

  adios2::fstream fw(fileName, adios2::fstream::out, MPIGetComm());
  if (isAttribute)
  {
    fw.write_attribute("vtk.xml", imageSchema);
  }

  for (size_t t = 0; t < steps; ++t)
  {
    TStep(Tdouble, t, rank);
    TStep(Tfloat, t, rank);
    TStep(Tint64, t, rank);
    TStep(Tuint64, t, rank);
    TStep(Tint32, t, rank);
    TStep(Tuint32, t, rank);

    if (hasTime)
    {
      fw.write("time", t);
    }

    fw.write("Tdouble", Tdouble.data(), shape, start, count);
    fw.write("Tfloat", Tfloat.data(), shape, start, count);
    fw.write("Tint64", Tint64.data(), shape, start, count);
    fw.write("Tuint64", Tuint64.data(), shape, start, count);
    fw.write("Tint32", Tint32.data(), shape, start, count);
    fw.write("Tuint32", Tuint32.data(), shape, start, count);
    fw.end_step();
  }
  fw.close();

  if (!isAttribute)
  {
    std::ofstream fxml(fileName + ".dir/vtk.xml", ofstream::out);
    fxml << imageSchema << "\n";
    fxml.close();
  }
}

void WriteBPFile1DVars(const std::string& fileName, const adios2::Dims& shape,
  const adios2::Dims& start, const adios2::Dims& count, const size_t steps, const int rank,
  const bool isAttribute, const bool hasTime, const bool isCellData = true)
{
  const size_t totalElements = TotalElements(count);

  const size_t Nx = isCellData ? shape[0] + 1 : shape[0];
  const size_t Ny = isCellData ? shape[1] + 1 : shape[1];
  const size_t Nz = isCellData ? shape[2] + 1 : shape[2];

  const std::string extent =
    "0 " + std::to_string(Nx) + " " + "0 " + std::to_string(Ny) + " " + "0 " + std::to_string(Nz);

  const std::string dataSetType = isCellData ? "CellData" : "PointData";

  const std::string timeStr = hasTime ? R"(
   <DataArray Name="TIME">
       time
   </DataArray> )" : "";

  const std::string imageSchema = R"(<?xml version="1.0"?>
      <VTKFile type="ImageData" version="0.1" byte_order="LittleEndian">
        <ImageData WholeExtent=")" + extent + R"(" Origin="0 0 0" Spacing="1 1 1">
          <Piece Extent=")" + extent + R"(">
            <)" + dataSetType + R"(>
                <DataArray Name="Tdouble" />
                <DataArray Name="Tfloat" />
                <DataArray Name="Tint64" />
                <DataArray Name="Tuint64" />
                <DataArray Name="Tint32" />
                <DataArray Name="Tuint32" />
                )" + timeStr + R"(
            </)" + dataSetType + R"(>
          </Piece>
        </ImageData>
      </VTKFile>)";

  // using adios2 C++ high-level API
  std::vector<double> Tdouble(totalElements);
  std::vector<float> Tfloat(totalElements);
  std::vector<int64_t> Tint64(totalElements);
  std::vector<uint64_t> Tuint64(totalElements);
  std::vector<int32_t> Tint32(totalElements);
  std::vector<uint32_t> Tuint32(totalElements);

  const adios2::Dims shape1D = { TotalElements(shape) };

  const size_t linearStart = start[0] * shape[1] * shape[2] + start[1] * shape[2] + start[2];
  const adios2::Dims start1D = { linearStart };

  const adios2::Dims count1D = { TotalElements(count) };

  adios2::fstream fw(fileName, adios2::fstream::out, MPIGetComm());
  if (isAttribute)
  {
    fw.write_attribute("vtk.xml", imageSchema);
  }

  for (size_t t = 0; t < steps; ++t)
  {
    TStep(Tdouble, t, rank);
    TStep(Tfloat, t, rank);
    TStep(Tint64, t, rank);
    TStep(Tuint64, t, rank);
    TStep(Tint32, t, rank);
    TStep(Tuint32, t, rank);

    if (hasTime)
    {
      fw.write("time", t);
    }

    fw.write("Tdouble", Tdouble.data(), shape1D, start1D, count1D);
    fw.write("Tfloat", Tfloat.data(), shape1D, start1D, count1D);
    fw.write("Tint64", Tint64.data(), shape1D, start1D, count1D);
    fw.write("Tuint64", Tuint64.data(), shape1D, start1D, count1D);
    fw.write("Tint32", Tint32.data(), shape1D, start1D, count1D);
    fw.write("Tuint32", Tuint32.data(), shape1D, start1D, count1D);
    fw.end_step();
  }
  fw.close();

  if (!isAttribute)
  {
    std::ofstream fxml(fileName + ".dir/vtk.xml", ofstream::out);
    fxml << imageSchema << "\n";
    fxml.close();
  }
}

} // end empty namespace

int TestIOADIOS2VTI3D(int argc, char* argv[])
{
  auto lf_DoTest = [&](const std::string& fileName, const size_t steps) {
    vtkNew<vtkADIOS2ReaderMultiBlock> adios2Reader;
    adios2Reader->SetFileName(fileName.c_str());
    // check FileName
    char* outFileName = adios2Reader->GetFileName();
    ExpectEqual(fileName, std::string(outFileName), " file names");
    // check PrintSelf
    adios2Reader->Print(std::cout);

    vtkNew<TesterVTI3D> testerVTI3D;
    testerVTI3D->Init(fileName, steps);
    testerVTI3D->SetInputConnection(adios2Reader->GetOutputPort());

    for (auto s = 0; s < steps; ++s)
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
  const adios2::Dims count{ 10, 10, 8 };
  const adios2::Dims start{ static_cast<size_t>(rank) * count[0], 0, 0 };
  const adios2::Dims shape{ static_cast<size_t>(size) * count[0], count[1], count[2] };

  const std::string fileName = "heat3D.bp";

  // schema as file in bp dir without time
  WriteBPFile3DVars(fileName, shape, start, count, steps, rank, false, false);
  lf_DoTest(fileName, steps);
  vtksys::SystemTools::RemoveADirectory("heat3D.bp.dir");
  vtksys::SystemTools::RemoveFile("heat3D.bp");

  // schema as attribute in bp file
  WriteBPFile3DVars(fileName, shape, start, count, steps, rank, true, true);
  lf_DoTest(fileName, steps);
  vtksys::SystemTools::RemoveADirectory("heat3D.bp.dir");
  vtksys::SystemTools::RemoveFile("heat3D.bp");

  // schema as file in bp dir
  WriteBPFile3DVars(fileName, shape, start, count, steps, rank, false, true);
  lf_DoTest(fileName, steps);
  vtksys::SystemTools::RemoveADirectory("heat3D.bp.dir");
  vtksys::SystemTools::RemoveFile("heat3D.bp");

  // schema for point data
  WriteBPFile3DVars(fileName, shape, start, count, steps, rank, false, true, false);
  lf_DoTest(fileName, steps);
  vtksys::SystemTools::RemoveADirectory("heat3D.bp.dir");
  vtksys::SystemTools::RemoveFile("heat3D.bp");

  // cell data from 1D arrays
  WriteBPFile1DVars(fileName, shape, start, count, steps, rank, false, true);
  lf_DoTest(fileName, steps);
  vtksys::SystemTools::RemoveADirectory("heat3D.bp.dir");
  vtksys::SystemTools::RemoveFile("heat3D.bp");

  mpiController->Finalize();
  return 0;
}
