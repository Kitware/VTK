/*=========================================================================

  Program:   Visualization Toolkit
  Module:    UnitTestIOADIOS2.cxx

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
 * UnitTestIOADIOS2.cxx : unit tests covering nearly 100%
 *
 *  Created on: Jun 13, 2019
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "vtkADIOS2VTXReader.h"

#include "vtkMPI.h"
#include "vtkMPICommunicator.h"
#include "vtkMPIController.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkTesting.h"

#include <string>

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

void WriteBPFileNoSchema(const std::string& fileName)
{
  const std::string extent = "0 10 0 10 0 10";

  adios2::fstream fw(fileName, adios2::fstream::out, MPIGetComm());
  for (size_t t = 0; t < 2; ++t)
  {
    fw.write("dummy", t);
    fw.end_step();
  }
  fw.close();
}

void WriteBPFileMissingVTKFileNode(const std::string& fileName)
{
  const std::string extent = "0 10 0 10 0 10";

  const std::string imageSchema = R"(<?xml version="1.0"?>
    <VTKFileWrong type="ImageData" version="0.1" byte_order="LittleEndian">
      <ImageData XX=")" + extent +
                                R"(" Origin="0 0 0" Spacing="1 1 1">
        <Piece Extent=")" + extent +
                                R"(">
          <CellData Scalars="U">
              <DataArray Name="T" />
          </CellData>
        </Piece>
      </ImageData>
    </VTKFileWrong>)";

  adios2::fstream fw(fileName, adios2::fstream::out, MPIGetComm());
  fw.write_attribute("vtk.xml", imageSchema);
  fw.close();
}

void WriteBPFileUnsupportedExtent(const std::string& fileName)
{
  const std::string extent = "0 10 0 10 0 10";

    const std::string imageSchema = R"(<?xml version="1.0"?>
        <VTKFile type="ImageData" version="0.1" byte_order="LittleEndian">
          <ImageData XX=")" + extent +
                                    R"(" Origin="0 0 0" Spacing="1 1 1">
            <Piece Extent=")" + extent +
                                    R"(">
              <CellData Scalars="U">
                  <DataArray Name="T" />
              </CellData>
            </Piece>
          </ImageData>
        </VTKFile>)";

    adios2::fstream fw(fileName, adios2::fstream::out, MPIGetComm());
    fw.write_attribute("vtk.xml", imageSchema);
    fw.close();
}

void WriteBPFileUnsupportedVTKType(const std::string& fileName)
{
  const std::string extent = "0 10 0 10 0 10";

    const std::string imageSchema = R"(<?xml version="1.0"?>
        <VTKFile type="XXX" version="0.1" byte_order="LittleEndian">
          <ImageData WholeExtent=")" +
                                    extent +
                                    R"(" Origin="0 0 0" Spacing="1 1 1">
            <Piece Extent=")" + extent +
                                    R"(">
              <CellData Scalars="U">
                  <DataArray Name="Tlong_double" />
              </CellData>
            </Piece>
          </ImageData>
        </VTKFile>)";

    adios2::fstream fw(fileName, adios2::fstream::out, MPIGetComm());
    fw.write_attribute("vtk.xml", imageSchema);
    fw.close();
}

void WriteBPFileNoVTKFileNode(const std::string& fileName)
{
  const std::string extent = "0 10 0 10 0 10";

    const std::string imageSchema = R"(<?xml version="1.0"?>
        <XXX type="ImageData" version="0.1" byte_order="LittleEndian">
          <ImageData WholeExtent=")" +
                                    extent +
                                    R"(" Origin="0 0 0" Spacing="1 1 1">
            <Piece Extent=")" + extent +
                                    R"(">
              <CellData Scalars="U">
                  <DataArray Name="Tlong_double" />
              </CellData>
            </Piece>
          </ImageData>
        </VTKFile>)";

    adios2::fstream fw(fileName, adios2::fstream::out, MPIGetComm());
    fw.write_attribute("vtk.xml", imageSchema);
    fw.close();
}

void WriteBPFileNoTime(const std::string& fileName)
{
  const std::string extent = "0 10 0 10 0 10";

    const std::string imageSchema = R"(<?xml version="1.0"?>
        <VTKFile type="ImageData" version="0.1" byte_order="LittleEndian">
          <ImageData WholeExtent=")" +
                                    extent +
                                    R"(" Origin="0 0 0" Spacing="1 1 1">
            <Piece Extent=")" + extent +
                                    R"(">
              <CellData Scalars="U">
                <DataArray Name="T" />
                <DataArray Name="TIME">
                  time
                </DataArray>
              </CellData>
            </Piece>
          </ImageData>
        </VTKFile>)";

    adios2::fstream fw(fileName, adios2::fstream::out, MPIGetComm());
    fw.write_attribute("vtk.xml", imageSchema);
    fw.close();
}

void WriteBPFileTwoNodes(const std::string& fileName)
{
  const std::string extent = "0 10 0 10 0 10";

    const std::string imageSchema = R"(<?xml version="1.0"?>
        <VTKFile type="ImageData" version="0.1" byte_order="LittleEndian">
          <ImageData WholeExtent=")" +
                                    extent +
                                    R"(" Origin="0 0 0" Spacing="1 1 1">
            <Piece Extent=")" + extent +
                                    R"(">
              <CellData Scalars="U">
                  <DataArray Name="T" />
                <DataArray Name="TIME">
                  time
                </DataArray>
              </CellData>
            </Piece>
          </ImageData>
        </VTKFile>

     <VTKFile type="ImageData" version="0.1" byte_order="LittleEndian" />)";

    adios2::fstream fw(fileName, adios2::fstream::out, MPIGetComm());
    fw.write_attribute("vtk.xml", imageSchema);
    fw.close();
}

void WriteBPFileWrongWholeExtent(const std::string& fileName)
{
  const std::string extent = "0 10 0 10 0"; // only 5

    const std::string imageSchema = R"(<?xml version="1.0"?>
        <VTKFile type="ImageData" version="0.1" byte_order="LittleEndian">
          <ImageData WholeExtent=")" +
                                    extent +
                                    R"(" Origin="0 0 0" Spacing="1 1 1">
            <Piece Extent=")" + extent +
                                    R"(">
              <CellData Scalars="U">
                  <DataArray Name="T" />
              </CellData>
            </Piece>
          </ImageData>
        </VTKFile>)";

    adios2::fstream fw(fileName, adios2::fstream::out, MPIGetComm());
    fw.write_attribute("vtk.xml", imageSchema);
    fw.close();
}

void WriteBPFileWrongOrigin(const std::string& fileName)
{
  const std::string extent = "0 10 0 10 0 10";

    const std::string imageSchema = R"(<?xml version="1.0"?>
        <VTKFile type="ImageData" version="0.1" byte_order="LittleEndian">
          <ImageData WholeExtent=")" +
                                    extent + R"(" Origin="0 0" Spacing="1 1 1">
            <Piece Extent=")" + extent +
                                    R"(">
              <CellData Scalars="U">
                  <DataArray Name="T" />
              </CellData>
            </Piece>
          </ImageData>
        </VTKFile>)";

    adios2::fstream fw(fileName, adios2::fstream::out, MPIGetComm());
    fw.write_attribute("vtk.xml", imageSchema);
    fw.close();
}

void WriteBPFileMandatoryNode(const std::string& fileName)
{
  const std::string extent = "0 10 0 10 0 10";

    const std::string imageSchema = R"( <?xml version="1.0"?>
  <VTKFile type="ImageData" version="0.1" byte_order="LittleEndian">
    <XXXImageData WholeExtent=")" + extent +
                                    R"(" Origin="0 0 0" Spacing="1 1 1">
        <Piece Extent=")" + extent + R"(">
          <CellData Scalars="T">
            <DataArray Name="T" />
          </CellData>
        </Piece>
      </XXXImageData>
  </VTKFile>)";

    adios2::fstream fw(fileName, adios2::fstream::out, MPIGetComm());
    fw.write_attribute("vtk.xml", imageSchema);
    for (size_t t = 0; t < 2; ++t)
    {
      fw.write("T", t);
      fw.end_step();
    }
    fw.close();
}

void WriteBPFileTwoImageNodes(const std::string& fileName)
{
  const std::string extent = "0 10 0 10 0 10";

    const std::string imageSchema = R"( <?xml version="1.0"?>
  <VTKFile type="ImageData" version="0.1" byte_order="LittleEndian">
    <ImageData WholeExtent=")" + extent +
                                    R"(" Origin="0 0 0" Spacing="1 1 1">
        <Piece Extent=")" + extent + R"(">
          <CellData Scalars="T">
            <DataArray Name="T" />
          </CellData>
        </Piece>
     </ImageData>
    <ImageData />
  </VTKFile>)";

    adios2::fstream fw(fileName, adios2::fstream::out, MPIGetComm());
    fw.write_attribute("vtk.xml", imageSchema);
    for (size_t t = 0; t < 2; ++t)
    {
      fw.write("T", t);
      fw.end_step();
    }
    fw.close();
}

void WriteBPFileWrongNumberOfComponents(const std::string& fileName)
{
  const std::string extent = "0 10 0 10 0 10";

    const std::string imageSchema = R"(<?xml version="1.0"?>
        <XXX type="ImageData" version="0.1" byte_order="LittleEndian">
          <ImageData WholeExtent=")" +
                                    extent +
                                    R"(" Origin="0 0 0" Spacing="1 1 1">
            <Piece Extent=")" + extent +
                                    R"(">
              <CellData Scalars="U">
                  <DataArray Name="Tlong_double" NumberOfComponents="3">
                    x y
                  </DataArray>
              </CellData>
            </Piece>
          </ImageData>
        </VTKFile>)";

    adios2::fstream fw(fileName, adios2::fstream::out, MPIGetComm());
    fw.write_attribute("vtk.xml", imageSchema);
    fw.close();
}

void WriteBPFileWrongTime(const std::string& fileName)
{
  const std::string extent = "0 10 0 10 0 10";

    const std::string imageSchema = R"(<?xml version="1.0"?>
        <VTKFile type="ImageData" version="0.1" byte_order="LittleEndian">
          <ImageData WholeExtent=")" +
                                    extent +
                                    R"(" Origin="0 0 0" Spacing="1 1 1">
            <Piece Extent=")" + extent +
                                    R"(">
              <CellData Scalars="U">
                  <DataArray Name="T" />
                <DataArray Name="TIME">
                </DataArray>
              </CellData>
            </Piece>
          </ImageData>
        </VTKFile>)";

    adios2::fstream fw(fileName, adios2::fstream::out, MPIGetComm());
    fw.write_attribute("vtk.xml", imageSchema);
    fw.close();
}

void WriteBPFileWrongNodePC1(const std::string& fileName)
{
  const std::string extent = "0 10 0 10 0 10";

    const std::string imageSchema = R"(<?xml version="1.0"?>
        <VTKFile type="ImageData" version="0.1" byte_order="LittleEndian">
          <ImageData WholeExtent=")" +
                                    extent +
                                    R"(" Origin="0 0 0" Spacing="1 1 1">
            <Piece Extent=")" + extent +
                                    R"(">
              <CellData Scalars="U">
                <DataArray Name="T" />
                <DataArray Name="WrongPC" NumberOfComponents="3">
                </DataArray>
              </CellData>
            </Piece>
          </ImageData>
        </VTKFile>)";

    adios2::fstream fw(fileName, adios2::fstream::out, MPIGetComm());
    fw.write_attribute("vtk.xml", imageSchema);
    fw.close();
}

void WriteBPFileWrongNodePC2(const std::string& fileName)
{
  const std::string extent = "0 10 0 10 0 10";

    const std::string imageSchema = R"(<?xml version="1.0"?>
        <VTKFile type="ImageData" version="0.1" byte_order="LittleEndian">
          <ImageData WholeExtent=")" +
                                    extent +
                                    R"(" Origin="0 0 0" Spacing="1 1 1">
            <Piece Extent=")" + extent +
                                    R"(">
              <CellData Scalars="U">
                <DataArray Name="T" />
                <DataArray Name="WrongPC" NumberOfComponents="3">
                  <DataArray Name="X" />
                </DataArray>
              </CellData>
            </Piece>
          </ImageData>
        </VTKFile>)";

    adios2::fstream fw(fileName, adios2::fstream::out, MPIGetComm());
    fw.write_attribute("vtk.xml", imageSchema);
    fw.close();
}

void WriteBPFileNoPieceVTI(const std::string& fileName)
{
  const std::string extent = "0 10 0 10 0 10";

    const std::string imageSchema = R"(<?xml version="1.0"?>
        <VTKFile type="ImageData" version="0.1" byte_order="LittleEndian">
          <ImageData WholeExtent=")" +
                                    extent +
                                    R"(" Origin="0 0 0" Spacing="1 1 1">
          </ImageData>
        </VTKFile>)";

    adios2::fstream fw(fileName, adios2::fstream::out, MPIGetComm());
    fw.write_attribute("vtk.xml", imageSchema);
    fw.close();
}

void WriteBPFileNoPieceVTU(const std::string& fileName)
{
    const std::string unstructureGridSchema = R"(
        <VTKFile type="UnstructuredGrid">
          <UnstructuredGrid>
          </UnstructuredGrid>
        </VTKFile>)";

    adios2::fstream fs(fileName, adios2::fstream::out, MPI_COMM_SELF);
    fs.write_attribute("vtk.xml", unstructureGridSchema);
    fs.close();
}

void WriteBPFileUnsupportedShape(const std::string& fileName)
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
              </PointData>
            </Piece>
          </UnstructuredGrid>
        </VTKFile>)";

    adios2::fstream fs(fileName, adios2::fstream::out, MPI_COMM_SELF);

    std::vector<uint32_t> dummyConnectivity(18, 1);
    std::vector<double> dummyVertices(9, 1.05);
    std::vector<double> dummySol(3, -1);

    fs.write("types", 11);
    fs.write("connectivity", dummyConnectivity.data(), { 2, 9 }, { 0, 0 }, { 2, 9 });
    fs.write("vertices", dummyVertices.data(), { 3, 3 }, { 0, 0 }, { 3, 3 });
    fs.write_attribute("vtk.xml", unstructureGridSchema);
    fs.write("sol", dummySol.data(), { 3 }, { 0 }, { 3 });

    fs.close();
}

void WriteBPFileUnsupportedType(const std::string& fileName)
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
              </PointData>
            </Piece>
          </UnstructuredGrid>
        </VTKFile>)";

    std::vector<uint32_t> dummyConnectivity = {8, 0, 1, 2, 3, 4, 5, 6, 7};
    // clang-format off
  std::vector<double> dummyVertices = {
    0., 0., 0.,
    0., 0., 1.,
    0., 1., 0.,
    0., 1., 1.,
    1., 0., 0.,
    1., 0., 1.,
    1., 1., 0.,
    1., 1., 1.
  };
    // clang-format on
    std::vector<double> dummySol(8, -1);

    adios2::fstream fs(fileName, adios2::fstream::out, MPI_COMM_SELF);
    fs.write<double>("types", 11.);
    fs.write("connectivity", dummyConnectivity.data(), {}, {}, { 1, 9 });
    fs.write("vertices", dummyVertices.data(), {}, {}, { 8, 3 });
    fs.write_attribute("vtk.xml", unstructureGridSchema);
    fs.write("sol", dummySol.data(), {}, {}, { 8 });
    fs.close();
}

} // end empty namespace

int UnitTestIOADIOS2VTX(int argc, char* argv[])
{
  auto lf_GetFileName = [](const size_t id) -> std::string {
    vtkNew<vtkTesting> testing;
    const std::string rootDirectory(testing->GetTempDirectory());
    return rootDirectory + "/dummy_" + std::to_string(id) + ".bp";
  };

  auto lf_Test = [&](const std::string& fileName, const size_t id, const bool print = false) {
    bool isCaught = false;
    try
    {
      vtkNew<vtkADIOS2VTXReader> reader;
      reader->SetFileName(fileName.c_str());
      reader->Update();
    }
    catch (std::exception& e)
    {
      isCaught = true;
      if (print)
      {
        std::cout << e.what() << "\n";
      }
    }
    if (!isCaught)
    {
      throw std::logic_error(
        "ERROR: ADIOS2 VTK Reader unit test " + std::to_string(id) + " failed\n");
    }
  };

  vtkNew<vtkMPIController> mpiController;
  mpiController->Initialize(&argc, &argv, 0);
  vtkMultiProcessController::SetGlobalController(mpiController);

  size_t testID = 0;
  std::string fileName;

#define ADIOS2VTK_UNIT_TEST(function)                                                              \
  ++testID;                                                                                        \
  fileName = lf_GetFileName(testID);                                                               \
  function(fileName);                                                                              \
  lf_Test(fileName, testID);

  ADIOS2VTK_UNIT_TEST(WriteBPFileNoSchema)
  ADIOS2VTK_UNIT_TEST(WriteBPFileMissingVTKFileNode)
  ADIOS2VTK_UNIT_TEST(WriteBPFileUnsupportedExtent)
  ADIOS2VTK_UNIT_TEST(WriteBPFileUnsupportedVTKType)
  ADIOS2VTK_UNIT_TEST(WriteBPFileNoVTKFileNode)
  ADIOS2VTK_UNIT_TEST(WriteBPFileNoTime)
  ADIOS2VTK_UNIT_TEST(WriteBPFileTwoNodes)
  ADIOS2VTK_UNIT_TEST(WriteBPFileWrongWholeExtent)
  ADIOS2VTK_UNIT_TEST(WriteBPFileWrongOrigin)
  ADIOS2VTK_UNIT_TEST(WriteBPFileMandatoryNode)
  ADIOS2VTK_UNIT_TEST(WriteBPFileTwoImageNodes)
  ADIOS2VTK_UNIT_TEST(WriteBPFileWrongNumberOfComponents)
  ADIOS2VTK_UNIT_TEST(WriteBPFileWrongTime)
  ADIOS2VTK_UNIT_TEST(WriteBPFileWrongNodePC1)
  ADIOS2VTK_UNIT_TEST(WriteBPFileWrongNodePC2)
  ADIOS2VTK_UNIT_TEST(WriteBPFileNoPieceVTI)
  ADIOS2VTK_UNIT_TEST(WriteBPFileNoPieceVTU)
  ADIOS2VTK_UNIT_TEST(WriteBPFileUnsupportedShape)
  ADIOS2VTK_UNIT_TEST(WriteBPFileUnsupportedType)

  ++testID;
  bool failed = true;
  try
  {
    vtkNew<vtkADIOS2VTXReader> reader;
    reader->SetFileName("NONE.bp");
    reader->Update();
  }
  catch (std::exception& e)
  {
    failed = false;
  }
  if (failed)
  {
    throw std::logic_error(
      "ERROR: ADIOS2 VTK Reader unit test " + std::to_string(testID) + " failed\n");
  }

  mpiController->Finalize();
  return 0;
}
