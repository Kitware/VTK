// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkFLUENTCFFReader.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkTestUtilities.h"
#include "vtkXMLMultiBlockDataReader.h"

#include <cstdlib>
#include <iostream>
#include <string>

//------------------------------------------------------------------------------
int CompareFLUENTCFFFiles(const std::string& h5Path, const std::string& xmlPath, bool renameFields)
{
  vtkNew<vtkFLUENTCFFReader> reader;
  reader->SetRenameArrays(renameFields);
  reader->SetFileName(h5Path);
  reader->Update();

  vtkMultiBlockDataSet* readData = vtkMultiBlockDataSet::SafeDownCast(reader->GetOutput());

  vtkNew<vtkXMLMultiBlockDataReader> xmlReader;
  xmlReader->SetFileName(xmlPath.c_str());
  xmlReader->Update();
  vtkMultiBlockDataSet* readDataXML = vtkMultiBlockDataSet::SafeDownCast(xmlReader->GetOutput());

  if (!vtkTestUtilities::CompareDataObjects(readData, readDataXML))
  {
    std::cerr << h5Path << " file isn't equal to " << xmlPath << " file." << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

//------------------------------------------------------------------------------
int TestFLUENTCFFReader(int argc, char* argv[])
{
  const std::string dataRoot = vtkTestUtilities::GetDataRoot(argc, argv);

  const std::string roomH5Path = dataRoot + "/Data/room.cas.h5";
  const std::string roomXmlPath = dataRoot + "/Data/FLUENTCFF/room.vtm";
  if (CompareFLUENTCFFFiles(roomH5Path, roomXmlPath, false) == EXIT_FAILURE)
  {
    return EXIT_FAILURE;
  }

  const std::string mesh3DH5Path = dataRoot + "/Data/mesh_3ddp.cas.h5";
  const std::string mesh3DXmlPath = dataRoot + "/Data/FLUENTCFF/mesh_3ddp.vtm";
  if (CompareFLUENTCFFFiles(mesh3DH5Path, mesh3DXmlPath, true) == EXIT_FAILURE)
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
