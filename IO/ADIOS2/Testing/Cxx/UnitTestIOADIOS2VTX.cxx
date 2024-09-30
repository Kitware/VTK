// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov

/*
 * UnitTestIOADIOS2.cxx : unit tests covering nearly 100%
 *
 *  Created on: Jun 13, 2019
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "ADIOSTestUtilities.h"
#include "vtkADIOS2VTXReader.h"

#if VTK_MODULE_ENABLE_VTK_ParallelMPI
#include "vtkMPI.h"
#include "vtkMPICommunicator.h"
#include "vtkMPIController.h"
#endif
#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkDataSet.h"
#include "vtkInformation.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTestUtilities.h"
#include "vtkTesting.h"

#include <string>

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
#endif

void WriteBPFileNoSchema(const std::string& fileName)
{
  const std::string extent = "0 10 0 10 0 10";

  ADIOS_OPEN(fw, fileName);
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
      <ImageData XX=")" +
    extent +
    R"(" Origin="0 0 0" Spacing="1 1 1">
        <Piece Extent=")" +
    extent +
    R"(">
          <CellData Scalars="U">
              <DataArray Name="T" />
          </CellData>
        </Piece>
      </ImageData>
    </VTKFileWrong>)";

  ADIOS_OPEN(fw, fileName);
  fw.write_attribute("vtk.xml", imageSchema);
  fw.close();
}

void WriteBPFileUnsupportedExtent(const std::string& fileName)
{
  const std::string extent = "0 10 0 10 0 10";

  const std::string imageSchema = R"(<?xml version="1.0"?>
        <VTKFile type="ImageData" version="0.1" byte_order="LittleEndian">
          <ImageData XX=")" +
    extent +
    R"(" Origin="0 0 0" Spacing="1 1 1">
            <Piece Extent=")" +
    extent +
    R"(">
              <CellData Scalars="U">
                  <DataArray Name="T" />
              </CellData>
            </Piece>
          </ImageData>
        </VTKFile>)";

  ADIOS_OPEN(fw, fileName);
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
            <Piece Extent=")" +
    extent +
    R"(">
              <CellData Scalars="U">
                  <DataArray Name="Tlong_double" />
              </CellData>
            </Piece>
          </ImageData>
        </VTKFile>)";

  ADIOS_OPEN(fw, fileName);
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
            <Piece Extent=")" +
    extent +
    R"(">
              <CellData Scalars="U">
                  <DataArray Name="Tlong_double" />
              </CellData>
            </Piece>
          </ImageData>
        </VTKFile>)";

  ADIOS_OPEN(fw, fileName);
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
            <Piece Extent=")" +
    extent +
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

  ADIOS_OPEN(fw, fileName);
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
            <Piece Extent=")" +
    extent +
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

  ADIOS_OPEN(fw, fileName);
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
            <Piece Extent=")" +
    extent +
    R"(">
              <CellData Scalars="U">
                  <DataArray Name="T" />
              </CellData>
            </Piece>
          </ImageData>
        </VTKFile>)";

  ADIOS_OPEN(fw, fileName);
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
            <Piece Extent=")" +
    extent +
    R"(">
              <CellData Scalars="U">
                  <DataArray Name="T" />
              </CellData>
            </Piece>
          </ImageData>
        </VTKFile>)";

  ADIOS_OPEN(fw, fileName);
  fw.write_attribute("vtk.xml", imageSchema);
  fw.close();
}

void WriteBPFileMandatoryNode(const std::string& fileName)
{
  const std::string extent = "0 10 0 10 0 10";

  const std::string imageSchema = R"( <?xml version="1.0"?>
  <VTKFile type="ImageData" version="0.1" byte_order="LittleEndian">
    <XXXImageData WholeExtent=")" +
    extent +
    R"(" Origin="0 0 0" Spacing="1 1 1">
        <Piece Extent=")" +
    extent + R"(">
          <CellData Scalars="T">
            <DataArray Name="T" />
          </CellData>
        </Piece>
      </XXXImageData>
  </VTKFile>)";

  ADIOS_OPEN(fw, fileName);
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
    <ImageData WholeExtent=")" +
    extent +
    R"(" Origin="0 0 0" Spacing="1 1 1">
        <Piece Extent=")" +
    extent + R"(">
          <CellData Scalars="T">
            <DataArray Name="T" />
          </CellData>
        </Piece>
     </ImageData>
    <ImageData />
  </VTKFile>)";

  ADIOS_OPEN(fw, fileName);
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
            <Piece Extent=")" +
    extent +
    R"(">
              <CellData Scalars="U">
                  <DataArray Name="Tlong_double" NumberOfComponents="3">
                    x y
                  </DataArray>
              </CellData>
            </Piece>
          </ImageData>
        </VTKFile>)";

  ADIOS_OPEN(fw, fileName);
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
            <Piece Extent=")" +
    extent +
    R"(">
              <CellData Scalars="U">
                  <DataArray Name="T" />
                <DataArray Name="TIME">
                </DataArray>
              </CellData>
            </Piece>
          </ImageData>
        </VTKFile>)";

  ADIOS_OPEN(fw, fileName);
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
            <Piece Extent=")" +
    extent +
    R"(">
              <CellData Scalars="U">
                <DataArray Name="T" />
                <DataArray Name="WrongPC" NumberOfComponents="3">
                </DataArray>
              </CellData>
            </Piece>
          </ImageData>
        </VTKFile>)";

  ADIOS_OPEN(fw, fileName);
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
            <Piece Extent=")" +
    extent +
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

  ADIOS_OPEN(fw, fileName);
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

  ADIOS_OPEN(fw, fileName);
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

  ADIOS_OPEN(fs, fileName);
  fs.write_attribute("vtk.xml", unstructureGridSchema);
  fs.close();
}

void WriteBPFileMissingTypes(const std::string& fileName)
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

  ADIOS_OPEN(fs, fileName);

  std::vector<uint32_t> dummyConnectivity(18, 1);
  std::vector<double> dummyVertices(9, 1.05);
  std::vector<double> dummySol(3, -1);

  fs.write("type", 1);
  fs.write("connectivity", dummyConnectivity.data(), {}, {}, {});
  fs.write("vertices", dummyVertices.data(), {}, {}, {});
  fs.write("sol", dummySol.data(), {}, {}, {});
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

  ADIOS_OPEN(fs, fileName);

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

  std::vector<uint32_t> dummyConnectivity = { 8, 0, 1, 2, 3, 4, 5, 6, 7 };
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

  ADIOS_OPEN(fs, fileName);
  fs.write<double>("types", 11.);
  fs.write("connectivity", dummyConnectivity.data(), {}, {}, { 1, 9 });
  fs.write("vertices", dummyVertices.data(), {}, {}, { 8, 3 });
  fs.write_attribute("vtk.xml", unstructureGridSchema);
  fs.write("sol", dummySol.data(), {}, {}, { 8 });
  fs.close();
}

bool TestNoFile(const std::string&)
{
  vtkNew<vtkADIOS2VTXReader> reader;
  reader->SetFileName("NONE.bp");
  // This is equivalent to `reader->Update()`, but it allows us to get the
  // status of the request back.
  if (reader->GetExecutive()->Update())
  {
    std::cout << "Expected non-existing file to return pipeline error.\n";
    return false;
  }
  else
  {
    return true;
  }
}

bool TestPointDataTime(const std::string& baseDir)
{
  std::string filename = baseDir + "heat3D_4.bp";
  vtkNew<vtkADIOS2VTXReader> adios2Reader;
  adios2Reader->SetFileName(filename.c_str());

  adios2Reader->Update();

  auto checkTimestep = [&](double timestep)
  {
    adios2Reader->GetOutputInformation(0)->Set(
      vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(), timestep);
    adios2Reader->Update();
    vtkSmartPointer<vtkCompositeDataIterator> iter;
    iter.TakeReference(adios2Reader->GetOutput()->NewIterator());
    iter->GoToFirstItem();
    vtkDataSet* output = vtkDataSet::SafeDownCast(iter->GetCurrentDataObject());
    vtkDataArray* field = output->GetPointData()->GetArray("Tdouble");
    for (vtkIdType index = 0; index < field->GetNumberOfValues(); ++index)
    {
      double expected = timestep + static_cast<double>(index);
      double read = field->GetTuple1(index);
      if (expected != read)
      {
        throw std::logic_error(
          "Unexpected value read from file at time " + std::to_string(timestep));
      }
    }
  };

  try
  {
    checkTimestep(0);
    checkTimestep(1);
    checkTimestep(2);
    checkTimestep(2);
    checkTimestep(1);
    checkTimestep(0);
  }
  catch (std::logic_error& e)
  {
    std::cout << e.what() << std::endl;
    return false;
  }

  return true;
}

bool TestCellDataTime(const std::string& baseDir)
{
  std::string filename = baseDir + "cell-data-time.bp";
  vtkNew<vtkADIOS2VTXReader> adios2Reader;
  adios2Reader->SetFileName(filename.c_str());

  adios2Reader->Update();

  adios2Reader->GetOutputInformation(0)->Set(
    vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(), 0);
  adios2Reader->Update();
  vtkSmartPointer<vtkCompositeDataIterator> iter;
  iter.TakeReference(adios2Reader->GetOutput()->NewIterator());
  iter->GoToFirstItem();
  vtkDataSet* output = vtkDataSet::SafeDownCast(iter->GetCurrentDataObject());
  vtkDataArray* field = output->GetCellData()->GetArray("f");
  if ((field->GetTuple1(0) != 0) || (field->GetTuple1(1) != 0))
  {
    std::cout << "Bad value at time 0\n";
    return false;
  }

  adios2Reader->GetOutputInformation(0)->Set(
    vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(), 1);
  adios2Reader->Update();
  iter.TakeReference(adios2Reader->GetOutput()->NewIterator());
  iter->GoToFirstItem();
  output = vtkDataSet::SafeDownCast(iter->GetCurrentDataObject());
  field = output->GetCellData()->GetArray("f");
  if ((field->GetTuple1(0) != 1) || (field->GetTuple1(1) != 2))
  {
    std::cout << "Bad value at time 1\n";
    return false;
  }

  return true;
}

} // end empty namespace

int UnitTestIOADIOS2VTX(int argc, char* argv[])
{
  auto lf_GetFileName = [](const size_t id) -> std::string
  {
    vtkNew<vtkTesting> testing;
    const std::string rootDirectory(testing->GetTempDirectory());
    return rootDirectory + "/dummy_" + std::to_string(id) + ".bp";
  };

  auto lf_TestBadFile = [&](const std::string& fileName, const size_t id)
  {
    std::cout << id << " " << fileName << "\n";
    vtkNew<vtkADIOS2VTXReader> reader;
    reader->SetFileName(fileName.c_str());
    // This is equivalent to `reader->Update()`, but it allows us to get the
    // status of the request back.
    if (reader->GetExecutive()->Update())
    {
      std::cout << "ERROR: ADIOS2 VTK Reader unit test " << id << "(" << fileName << ") failed\n";
      std::cout << "Expected bad file to return pipeline error.\n";
    }
    else
    {
      // All good. Expected this pipeline error for a bad file.
    }
  };

#if VTK_MODULE_ENABLE_VTK_ParallelMPI
  vtkNew<vtkMPIController> mpiController;
  mpiController->Initialize(&argc, &argv, 0);
  vtkMultiProcessController::SetGlobalController(mpiController);
#else
  (void)argc;
  (void)argv;
#endif

  size_t testID = 0;
  std::string fileName;

#define ADIOS2VTK_UNIT_TEST(function)                                                              \
  ++testID;                                                                                        \
  fileName = lf_GetFileName(testID);                                                               \
  function(fileName);                                                                              \
  lf_TestBadFile(fileName, testID);

  try
  {
    // NOTE: For these tests we are expecting lots of reported pipeline failures
    // but no crashes.
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
    ADIOS2VTK_UNIT_TEST(WriteBPFileMissingTypes)
    ADIOS2VTK_UNIT_TEST(WriteBPFileUnsupportedShape)
    ADIOS2VTK_UNIT_TEST(WriteBPFileUnsupportedType)
  }
  catch (std::exception& e)
  {
    std::cout << "Caught error!\n";
    std::cout << e.what() << std::endl;
    return 1;
  }

  std::string baseDir = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/ADIOS2/vtx/bp4/");

  auto lf_TestCornerCase = [&](bool (*function)(const std::string&), const char* name)
  {
    ++testID;
    std::cout << testID << " " << name << "\n";
    bool success = function(baseDir);
    if (!success)
    {
      throw std::logic_error(
        "ERROR: ADIOS2 VTK Reader unit test " + std::to_string(testID) + "(" + name + ") failed\n");
    }
  };

#define ADIOS2VTK_CORNER_CASE_TEST(function) lf_TestCornerCase(function, #function);

  try
  {
    ADIOS2VTK_CORNER_CASE_TEST(TestNoFile)
    ADIOS2VTK_CORNER_CASE_TEST(TestPointDataTime)
    ADIOS2VTK_CORNER_CASE_TEST(TestCellDataTime)
  }
  catch (std::exception& e)
  {
    std::cout << "Caught error!\n";
    std::cout << e.what() << std::endl;
    return 1;
  }

#if VTK_MODULE_ENABLE_VTK_ParallelMPI
  mpiController->Finalize();
#endif
  return 0;
}
