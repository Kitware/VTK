/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestIOADIOS2VTX_VTU3D.cxx

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
 * TestIOADIOS2VTX_VTU3D.cxx : pipeline tests for unstructured grid reader
 *  3D vars
 *
 *  Created on: Jun 13, 2019
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "vtkADIOS2VTXReader.h"

#include <algorithm> //std::equal
#include <iostream>
#include <numeric> //std::iota
#include <string>
#include <vector>

#include "vtkAlgorithm.h"
#include "vtkDataArray.h"
#include "vtkDemandDrivenPipeline.h"
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
#include "vtkPointData.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTesting.h"
#include "vtkUnstructuredGrid.h"

#include <adios2.h>
#include <vtksys/FStream.hxx>
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
bool CompareData(T* data, const size_t size, const size_t step, const int rank)
{
  // expected
  std::vector<T> Texpected(size);
  TStep(Texpected, step, rank);
  return std::equal(Texpected.begin(), Texpected.end(), data);
}

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

// clang-format on

} // end empty namespace

// Pipeline
class TesterVTU3D : public vtkAlgorithm
{
public:
  static TesterVTU3D* New();
  vtkTypeMacro(TesterVTU3D, vtkAlgorithm);
  TesterVTU3D()
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
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkUnstructuredGrid");
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
    vtkUnstructuredGrid* unstructuredGrid =
      vtkUnstructuredGrid::SafeDownCast(multiPiece->GetPiece(0));

    double* psol = reinterpret_cast<double*>(
      unstructuredGrid->GetPointData()->GetArray("sol")->GetVoidPointer(0));
    const size_t size =
      static_cast<size_t>(unstructuredGrid->GetPointData()->GetArray("sol")->GetDataSize());

    if (!CompareData<double>(psol, size, step, rank))
    {
      return false;
    }

    const double* pvertices =
      reinterpret_cast<double*>(unstructuredGrid->GetPoints()->GetVoidPointer(0));
    // TODO
    if (!std::equal(vertices.begin(), vertices.end(), pvertices))
    {
      return false;
    }

    return true;
  }
};

vtkStandardNewMacro(TesterVTU3D);

namespace
{

void WriteBPFile3DVars(const std::string& fileName, const size_t steps, const int rank,
  const bool isAttribute, const bool /*hasTime*/, const bool unsignedType,
  const std::string& engineType)
{
    const std::string unstructureGridSchema = R"(
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
                <DataArray Name="TIME">
                  steps
                </DataArray>
              </PointData>
            </Piece>
          </UnstructuredGrid>
        </VTKFile>)";

    std::vector<double> sol(45);

    adios2::fstream fs(fileName, adios2::fstream::out, MPI_COMM_SELF, engineType);

    for (size_t s = 0; s < steps; ++s)
    {
      if (s == 0 && rank == 0)
      {
        if (unsignedType)
        {
          fs.write<uint32_t>("types", 11);
        }
        else
        {
          fs.write<int32_t>("types", 11);
        }

        fs.write("connectivity", connectivity.data(), {}, {}, { 16, 9 });
        fs.write("vertices", vertices.data(), {}, {}, { 45, 3 });
        if (isAttribute)
        {
          fs.write_attribute("vtk.xml", unstructureGridSchema);
        }
      }

      if (rank == 0)
      {
        fs.write("steps", s);
      }

      TStep(sol, s, rank);
      fs.write("sol", sol.data(), {}, {}, { sol.size() });
      fs.end_step();
    }
    fs.close();

    if (!isAttribute && rank == 0)
    {
      const std::string vtkFileName = vtksys::SystemTools::FileIsDirectory(fileName)
        ? fileName + "/vtk.xml"
        : fileName + ".dir/vtk.xml";

      vtksys::ofstream fxml(vtkFileName, vtksys::ofstream::out);
      fxml << unstructureGridSchema << "\n";
      fxml.close();
    }
}

} // end empty namespace

int TestIOADIOS2VTX_VTU3D(int argc, char* argv[])
{
  auto lf_DoTest = [&](const std::string& fileName, const size_t steps) {
    vtkNew<vtkADIOS2VTXReader> adios2Reader;
    adios2Reader->SetFileName(fileName.c_str());
    // check FileName
    char* outFileName = adios2Reader->GetFileName();
    ExpectEqual(fileName, std::string(outFileName), " file names");
    // check PrintSelf
    adios2Reader->Print(std::cout);

    vtkNew<TesterVTU3D> testerVTU3D;
    testerVTU3D->Init(fileName, steps);
    testerVTU3D->SetInputConnection(adios2Reader->GetOutputPort());

    for (size_t s = 0; s < steps; ++s)
    {
      testerVTU3D->Modified();
      testerVTU3D->UpdateInformation();
      testerVTU3D->Update();
    }
  };

  vtkNew<vtkMPIController> mpiController;
  mpiController->Initialize(&argc, &argv, 0);
  vtkMultiProcessController::SetGlobalController(mpiController);
  const int rank = MPIGetRank();
  const size_t steps = 3;

  vtksys::SystemTools::MakeDirectory("bp3");
  vtksys::SystemTools::MakeDirectory("bp4");

  const std::vector<std::string> engineTypes = { "bp3", "bp4" };
  vtkNew<vtkTesting> testing;
  const std::string rootDirectory(testing->GetTempDirectory());
  std::string fileName;

  for (const std::string& engineType : engineTypes)
  {
    // schema as attribute in bp file
    fileName = rootDirectory + "/" + engineType + "/ex2_mfem_1.bp";
    WriteBPFile3DVars(fileName, steps, rank, false, true, true, engineType);
    lf_DoTest(fileName, steps);

    // schema as file in bp directory
    fileName = rootDirectory + "/" + engineType + "/ex2_mfem_2.bp";
    WriteBPFile3DVars(fileName, steps, rank, false, false, false, engineType);
    lf_DoTest(fileName, steps);
  }

  mpiController->Finalize();
  return 0;
}
