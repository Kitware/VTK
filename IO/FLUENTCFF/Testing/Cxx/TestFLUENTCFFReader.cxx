// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkDataArraySelection.h"
#include "vtkFLUENTCFFReader.h"
#include "vtkInformation.h"
#include "vtkLogger.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkTestUtilities.h"
#include "vtkXMLMultiBlockDataReader.h"

#include <cstdlib>
#include <string>

namespace
{
//------------------------------------------------------------------------------
bool CompareFLUENTCFFFiles(
  const std::string& h5Path, const std::string& xmlPath, bool renameFields, bool readFaces)
{
  vtkNew<vtkFLUENTCFFReader> reader;
  reader->SetRenameArrays(renameFields);
  reader->SetReadFaces(readFaces);
  reader->SetFileName(h5Path);
  reader->Update();

  vtkMultiBlockDataSet* readData = vtkMultiBlockDataSet::SafeDownCast(reader->GetOutput());

  vtkNew<vtkXMLMultiBlockDataReader> xmlReader;
  xmlReader->SetFileName(xmlPath.c_str());
  xmlReader->Update();
  vtkMultiBlockDataSet* readDataXML = vtkMultiBlockDataSet::SafeDownCast(xmlReader->GetOutput());

  bool sameData = vtkTestUtilities::CompareDataObjects(readData, readDataXML);
  vtkLogIf(ERROR, !sameData, << h5Path << " read data isn't equal to " << xmlPath << " file.");

  return sameData;
}

//------------------------------------------------------------------------------
bool TestReadFaces(const std::string& h5Path)
{
  vtkNew<vtkFLUENTCFFReader> reader;
  reader->SetReadFaces(true);
  reader->SetFileName(h5Path);
  reader->UpdateInformation();

  vtkDataArraySelection* faceSelection = reader->GetFaceSelection();

  bool defaultSelect =
    faceSelection->GetNumberOfArrays() == faceSelection->GetNumberOfArraysEnabled();
  vtkLogIf(ERROR, !defaultSelect, << "Faces should be selected by default");

  reader->Update();
  vtkMultiBlockDataSet* readData = vtkMultiBlockDataSet::SafeDownCast(reader->GetOutput());

  // 1 block per face and 1 still there
  bool facesInBlocks = readData->GetNumberOfBlocks() ==
    static_cast<unsigned int>(faceSelection->GetNumberOfArraysEnabled() + 1);
  vtkLogIf(
    ERROR, !facesInBlocks, << "Wrong number of generated blocks: " << readData->GetNumberOfBlocks()
                           << " instead of " << faceSelection->GetNumberOfArraysEnabled() + 1);

  if (!facesInBlocks)
  {
    return false;
  }

  const std::string inletFaceName = "inlet";
  const int inletBlockNumber = 1;
  faceSelection->DisableAllArrays();
  faceSelection->EnableArray(inletFaceName.c_str());
  reader->Update();
  facesInBlocks &= readData->GetNumberOfBlocks() ==
    static_cast<unsigned int>(faceSelection->GetNumberOfArraysEnabled() + 1);
  vtkLogIf(
    ERROR, !facesInBlocks, << "Wrong number of generated blocks: " << readData->GetNumberOfBlocks()
                           << " instead of " << faceSelection->GetNumberOfArraysEnabled() + 1);

  vtkInformation* faceMetaData = readData->GetMetaData(inletBlockNumber);
  std::string faceName = faceMetaData->Get(vtkCompositeDataSet::NAME());

  bool faceNaming = faceName == inletFaceName;
  vtkLogIf(ERROR, !faceNaming, << "Wrong name for the inlet face block. Has " << faceName
                               << " instead of " << inletFaceName);

  return defaultSelect && facesInBlocks && faceNaming;
}

}

//------------------------------------------------------------------------------
int TestFLUENTCFFReader(int argc, char* argv[])
{
  const std::string dataRoot = vtkTestUtilities::GetDataRoot(argc, argv);

  const std::string roomH5Path = dataRoot + "/Data/room.cas.h5";
  const std::string roomXmlPath = dataRoot + "/Data/FLUENTCFF/room.vtm";
  bool ret = ::CompareFLUENTCFFFiles(roomH5Path, roomXmlPath, false, false);

  const std::string mesh3DH5Path = dataRoot + "/Data/mesh_3ddp.cas.h5";
  const std::string mesh3DXmlPath = dataRoot + "/Data/FLUENTCFF/mesh_3ddp.vtm";
  ret &= ::CompareFLUENTCFFFiles(mesh3DH5Path, mesh3DXmlPath, true, true);

  ret &= ::TestReadFaces(roomH5Path);

  return ret ? EXIT_SUCCESS : EXIT_FAILURE;
}
