/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestADIOS2ReaderUnitTests.cxx

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
 * TestADIOS2ReaderUnitTests.cxx : unit tests covering nearly 100%
 *
 *  Created on: Jun 13, 2019
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "vtkADIOS2ReaderMultiBlock.h"
#include "vtkMPI.h"
#include "vtkMPICommunicator.h"
#include "vtkMPIController.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"

#include <string>

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

void WriteBPFileUnsupportedExtent(const std::string& fileName)
{
  const std::string extent = "0 10 0 10 0 10";

  const std::string imageSchema = R"(<?xml version="1.0"?>
        <VTKFile type="ImageData" version="0.1" byte_order="LittleEndian">
          <ImageData XX=")" + extent + R"(" Origin="0 0 0" Spacing="1 1 1">
            <Piece Extent=")" + extent + R"(">
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
          <ImageData WholeExtent=")" + extent + R"(" Origin="0 0 0" Spacing="1 1 1">
            <Piece Extent=")" + extent + R"(">
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
          <ImageData WholeExtent=")" + extent + R"(" Origin="0 0 0" Spacing="1 1 1">
            <Piece Extent=")" + extent + R"(">
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
          <ImageData WholeExtent=")" + extent + R"(" Origin="0 0 0" Spacing="1 1 1">
            <Piece Extent=")" + extent + R"(">
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
          <ImageData WholeExtent=")" + extent + R"(" Origin="0 0 0" Spacing="1 1 1">
            <Piece Extent=")" + extent + R"(">
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
          <ImageData WholeExtent=")" + extent + R"(" Origin="0 0 0" Spacing="1 1 1">
            <Piece Extent=")" + extent + R"(">
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
          <ImageData WholeExtent=")" + extent + R"(" Origin="0 0" Spacing="1 1 1">
            <Piece Extent=")" + extent + R"(">
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

void WriteBPFileWrongShape(const std::string& fileName)
{
  const std::string extent = "0 10 0 10 0 10";

  const std::string imageSchema = R"(<?xml version="1.0"?>
        <VTKFile type="ImageData" version="0.1" byte_order="LittleEndian">
          <ImageData WholeExtent=")" + extent + R"(" Origin="0 0 0" Spacing="1 1 1">
            <Piece Extent=")" + extent + R"(">
              <CellData Scalars="U">
                  <DataArray Name="T" />
              </CellData>
            </Piece>
          </ImageData>
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

void WriteBPFileMandatoryNode(const std::string& fileName)
{
  const std::string extent = "0 10 0 10 0 10";

  const std::string imageSchema = R"( <?xml version="1.0"?>
  <VTKFile type="ImageData" version="0.1" byte_order="LittleEndian">
    <XXXImageData WholeExtent=")" + extent + R"(" Origin="0 0 0" Spacing="1 1 1">
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
    <ImageData WholeExtent=")" + extent + R"(" Origin="0 0 0" Spacing="1 1 1">
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
          <ImageData WholeExtent=")" + extent + R"(" Origin="0 0 0" Spacing="1 1 1">
            <Piece Extent=")" + extent + R"(">
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
          <ImageData WholeExtent=")" + extent + R"(" Origin="0 0 0" Spacing="1 1 1">
            <Piece Extent=")" + extent + R"(">
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
          <ImageData WholeExtent=")" + extent + R"(" Origin="0 0 0" Spacing="1 1 1">
            <Piece Extent=")" + extent + R"(">
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
          <ImageData WholeExtent=")" + extent + R"(" Origin="0 0 0" Spacing="1 1 1">
            <Piece Extent=")" + extent + R"(">
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

void WriteBPFileNoPiece(const std::string& fileName)
{
  const std::string extent = "0 10 0 10 0 10";

  const std::string imageSchema = R"(<?xml version="1.0"?>
        <VTKFile type="ImageData" version="0.1" byte_order="LittleEndian">
          <ImageData WholeExtent=")" + extent + R"(" Origin="0 0 0" Spacing="1 1 1">
          </ImageData>
        </VTKFile>)";

  adios2::fstream fw(fileName, adios2::fstream::out, MPIGetComm());
  fw.write_attribute("vtk.xml", imageSchema);
  fw.close();
}

void WriteBPFileWrongDataType(const std::string& fileName)
{
  const std::string extent = "0 10 0 10 0 10";

  const std::string imageSchema = R"(<?xml version="1.0"?>
        <VTKFile type="ImageData" version="0.1" byte_order="LittleEndian">
          <ImageData WholeExtent=")" + extent + R"(" Origin="0 0 0" Spacing="1 1 1">
            <Piece Extent=")" + extent + R"(">
              <WrongData Scalars="U">
                <DataArray Name="T" />
              </WrongData>
            </Piece>
          </ImageData>
        </VTKFile>)";

  adios2::fstream fw(fileName, adios2::fstream::out, MPIGetComm());
  fw.write_attribute("vtk.xml", imageSchema);
  fw.close();
}

} // end empty namespace

int UnitTestIOADIOS2(int argc, char* argv[])
{
  auto lf_IsCaught = [&](const std::string& fileName, const bool print = false) -> bool {
    bool isCaught = false;
    try
    {
      vtkNew<vtkADIOS2ReaderMultiBlock> vtkADIOS2Reader;
      vtkADIOS2Reader->SetFileName(fileName.c_str());
      vtkADIOS2Reader->Update();
    }
    catch (std::exception& e)
    {
      isCaught = true;
      if (print)
      {
        std::cout << e.what() << "\n";
      }
    }
    return isCaught;
  };

  vtkNew<vtkMPIController> mpiController;
  mpiController->Initialize(&argc, &argv, 0);
  vtkMultiProcessController::SetGlobalController(mpiController);

  // first test: no schema
  const std::string fileName1 = "dummy1.bp";
  WriteBPFileNoSchema(fileName1);
  if (!lf_IsCaught(fileName1))
  {
    throw std::logic_error("ERROR: ADIOS2 VTK Reader unit test 1 failed\n");
  }

  // second test; unsupported types
  const std::string fileName2 = "dummy2.bp";
  WriteBPFileUnsupportedExtent(fileName2);
  if (!lf_IsCaught(fileName2))
  {
    throw std::logic_error("ERROR: ADIOS2 VTK Reader unit test 2 failed\n");
  }

  // second test; unsupported types
  const std::string fileName3 = "dummy3.bp";
  WriteBPFileUnsupportedVTKType(fileName3);
  if (!lf_IsCaught(fileName3))
  {
    throw std::logic_error("ERROR: ADIOS2 VTK Reader unit test 3 failed\n");
  }

  // second test; unsupported types
  const std::string fileName4 = "dummy4.bp";
  WriteBPFileNoVTKFileNode(fileName4);
  if (!lf_IsCaught(fileName4))
  {
    throw std::logic_error("ERROR: ADIOS2 VTK Reader unit test 4 failed\n");
  }

  const std::string fileName5 = "dummy5.bp";
  WriteBPFileNoTime(fileName5);
  if (!lf_IsCaught(fileName5))
  {
    throw std::logic_error("ERROR: ADIOS2 VTK Reader unit test 5 failed\n");
  }

  const std::string fileName6 = "dummy6.bp";
  WriteBPFileTwoNodes(fileName6);
  if (!lf_IsCaught(fileName6))
  {
    throw std::logic_error("ERROR: ADIOS2 VTK Reader unit test 6 failed\n");
  }

  const std::string fileName7 = "dummy7.bp";
  WriteBPFileWrongWholeExtent(fileName7);
  if (!lf_IsCaught(fileName7))
  {
    throw std::logic_error("ERROR: ADIOS2 VTK Reader unit test 7 failed\n");
  }

  const std::string fileName8 = "dummy8.bp";
  WriteBPFileWrongOrigin(fileName8);
  if (!lf_IsCaught(fileName8))
  {
    throw std::logic_error("ERROR: ADIOS2 VTK Reader unit test 8 failed\n");
  }

  const std::string fileName9 = "dummy9.bp";
  WriteBPFileWrongShape(fileName9);
  if (!lf_IsCaught(fileName9))
  {
    throw std::logic_error("ERROR: ADIOS2 VTK Reader unit test 9 failed\n");
  }

  const std::string fileName10 = "dummy10.bp";
  WriteBPFileMandatoryNode(fileName10);
  if (!lf_IsCaught(fileName10))
  {
    throw std::logic_error("ERROR: ADIOS2 VTK Reader unit test 10 failed\n");
  }

  const std::string fileName11 = "dummy11.bp";
  WriteBPFileTwoImageNodes(fileName11);
  if (!lf_IsCaught(fileName11))
  {
    throw std::logic_error("ERROR: ADIOS2 VTK Reader unit test 11 failed\n");
  }

  const std::string fileName12 = "dummy12.bp";
  WriteBPFileWrongNumberOfComponents(fileName12);
  if (!lf_IsCaught(fileName12))
  {
    throw std::logic_error("ERROR: ADIOS2 VTK Reader unit test 12 failed\n");
  }

  const std::string fileName13 = "dummy13.bp";
  WriteBPFileWrongTime(fileName13);
  if (!lf_IsCaught(fileName13))
  {
    throw std::logic_error("ERROR: ADIOS2 VTK Reader unit test 13 failed\n");
  }

  const std::string fileName14 = "dummy14.bp";
  WriteBPFileWrongNodePC1(fileName14);
  if (!lf_IsCaught(fileName14))
  {
    throw std::logic_error("ERROR: ADIOS2 VTK Reader unit test 14 failed\n");
  }

  const std::string fileName15 = "dummy15.bp";
  WriteBPFileWrongNodePC2(fileName15);
  if (!lf_IsCaught(fileName15))
  {
    throw std::logic_error("ERROR: ADIOS2 VTK Reader unit test 15 failed\n");
  }

  const std::string fileName16 = "dummy16.bp";
  WriteBPFileNoPiece(fileName16);
  if (!lf_IsCaught(fileName16))
  {
    throw std::logic_error("ERROR: ADIOS2 VTK Reader unit test 16 failed\n");
  }

  // test 17
  bool failed = true;
  try
  {
    vtkNew<vtkADIOS2ReaderMultiBlock> vtkADIOS2Reader;
    vtkADIOS2Reader->SetFileName("NONE.bp");
    vtkADIOS2Reader->Update();
  }
  catch (std::exception& e)
  {
    failed = false;
  }
  if (failed)
  {
    throw std::logic_error("ERROR: ADIOS2 VTK Reader unit test 17 failed\n");
  }

  return 0;
}
