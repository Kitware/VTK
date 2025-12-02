// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkNew.h"
#include "vtkOverlappingAMR.h"
#include "vtkTestUtilities.h"
#include "vtkXMLGenericDataObjectReader.h"
#include "vtkXMLHierarchicalBoxDataFileConverter.h"

#include <string>
#include <vtksys/SystemTools.hxx>

#include <iostream>

#define VTK_SUCCESS 0
#define VTK_FAILURE 1
int TestXMLHierarchicalBoxDataFileConverter(int argc, char* argv[])
{
  char* temp_dir =
    vtkTestUtilities::GetArgOrEnvOrDefault("-T", argc, argv, "VTK_TEMP_DIR", "Testing/Temporary");
  if (!temp_dir)
  {
    std::cerr << "Could not determine temporary directory." << std::endl;
    return VTK_FAILURE;
  }
  std::string tempDirStr = temp_dir;
  delete[] temp_dir;

  char* data_dir = vtkTestUtilities::GetDataRoot(argc, argv);
  if (!data_dir)
  {
    std::cerr << "Could not determine data directory." << std::endl;
    return VTK_FAILURE;
  }
  std::string dataDirStr = data_dir;
  delete[] data_dir;

  std::string input = dataDirStr + "/Data/AMR/HierarchicalBoxDataset.v1.0.vthb";

  std::string output = tempDirStr + "/HierarchicalBoxDataset.Converted.v1.1.vthb";

  vtkNew<vtkXMLHierarchicalBoxDataFileConverter> converter;
  converter->SetInputFileName(input.c_str());
  converter->SetOutputFileName(output.c_str());

  if (!converter->Convert())
  {
    return VTK_FAILURE;
  }

  // Copy the subfiles over to the temporary directory so that we can test
  // loading the written file.
  std::string inputDir = dataDirStr + "/Data/AMR/HierarchicalBoxDataset.v1.0";

  std::string outputDir = tempDirStr + "/HierarchicalBoxDataset.Converted.v1.1";

  vtksys::SystemTools::RemoveADirectory(outputDir);
  if (!vtksys::SystemTools::CopyADirectory(inputDir, outputDir))
  {
    std::cerr << "Failed to copy image data files over for testing." << std::endl;
    return VTK_FAILURE;
  }

  vtkNew<vtkXMLGenericDataObjectReader> reader;
  reader->SetFileName(output.c_str());
  reader->Update();
  if (!vtkOverlappingAMR::SafeDownCast(reader->GetOutputDataObject(0))->CheckValidity())
  {
    std::cerr << "Failed to CheckValidity." << std::endl;
    return VTK_FAILURE;
  }
  return VTK_SUCCESS;
}
