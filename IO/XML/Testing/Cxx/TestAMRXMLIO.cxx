// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkAMRGaussianPulseSource.h"
#include "vtkAMRMetaData.h"
#include "vtkCellData.h"
#include "vtkDataSet.h"
#include "vtkDataSetAttributes.h"
#include "vtkNew.h"
#include "vtkNonOverlappingAMR.h"
#include "vtkOverlappingAMR.h"
#include "vtkStructuredData.h"
#include "vtkTestUtilities.h"
#include "vtkUniformGrid.h"
#include "vtkXMLGenericDataObjectReader.h"
#include "vtkXMLUniformGridAMRReader.h"
#include "vtkXMLUniformGridAMRWriter.h"

#include <string>

#include <iostream>

namespace
{
#define vtk_assert(x)                                                                              \
  do                                                                                               \
  {                                                                                                \
    if (!(x))                                                                                      \
    {                                                                                              \
      std::cerr << "ERROR: Condition FAILED!! : " << #x << std::endl;                              \
      return false;                                                                                \
    }                                                                                              \
  } while (false)

bool Validate(vtkAMRDataObject* input, vtkAMRDataObject* result)
{
  vtk_assert(input->GetNumberOfLevels() == result->GetNumberOfLevels());
  for (unsigned int level = 0; level < input->GetNumberOfLevels(); level++)
  {
    vtk_assert(input->GetNumberOfBlocks(level) == result->GetNumberOfBlocks(level));
  }

  vtk_assert(*input->GetAMRMetaData() == *result->GetAMRMetaData());

  return true;
}

bool ValidateOAMR(vtkOverlappingAMR* input, vtkOverlappingAMR* result)
{
  if (!::Validate(input, result))
  {
    return false;
  }
  vtk_assert(input->GetOrigin()[0] == result->GetOrigin()[0]);
  vtk_assert(input->GetOrigin()[1] == result->GetOrigin()[1]);
  vtk_assert(input->GetOrigin()[2] == result->GetOrigin()[2]);

  std::cout << "Check input validity" << std::endl;
  bool ret = input->CheckValidity();
  std::cout << "Check output validity" << std::endl;
  ret &= result->CheckValidity();
  return ret;
}

bool TestAMRXMLIO_OverlappingAMR2D(const std::string& output_dir)
{
  vtkNew<vtkAMRGaussianPulseSource> pulse;
  pulse->SetDimension(2);
  pulse->SetRootSpacing(5);

  std::string filename = output_dir + "/TestAMRXMLIO_OverlappingAMR2D.vth";

  vtkNew<vtkXMLUniformGridAMRWriter> writer;
  writer->SetInputConnection(pulse->GetOutputPort());
  writer->SetFileName(filename.c_str());
  writer->Write();

  vtkNew<vtkXMLGenericDataObjectReader> reader;
  reader->SetFileName(filename.c_str());
  reader->Update();

  return ValidateOAMR(vtkOverlappingAMR::SafeDownCast(pulse->GetOutputDataObject(0)),
    vtkOverlappingAMR::SafeDownCast(reader->GetOutputDataObject(0)));
}

bool TestAMRXMLIO_OverlappingAMR3D(const std::string& output_dir)
{
  vtkNew<vtkAMRGaussianPulseSource> pulse;
  pulse->SetDimension(3);
  pulse->SetRootSpacing(13);

  std::string filename = output_dir + "/TestAMRXMLIO_OverlappingAMR3D.vth";

  vtkNew<vtkXMLUniformGridAMRWriter> writer;
  writer->SetInputConnection(pulse->GetOutputPort());
  writer->SetFileName(filename.c_str());
  writer->Write();

  vtkNew<vtkXMLGenericDataObjectReader> reader;
  reader->SetFileName(filename.c_str());
  reader->Update();

  return ValidateOAMR(vtkOverlappingAMR::SafeDownCast(pulse->GetOutputDataObject(0)),
    vtkOverlappingAMR::SafeDownCast(reader->GetOutputDataObject(0)));
}

bool TestAMRXMLIO_HierarchicalBox(const std::string& input_dir, const std::string& output_dir)
{
  std::string filename = input_dir + "/AMR/HierarchicalBoxDataset.v1.1.vthb";

  vtkNew<vtkXMLUniformGridAMRReader> reader;
  reader->SetFileName(filename.c_str());
  reader->Update();

  vtkOverlappingAMR* output = vtkOverlappingAMR::SafeDownCast(reader->GetOutputDataObject(0));
  vtk_assert(output->GetNumberOfLevels() == 4);
  vtk_assert(output->GetNumberOfBlocks(0) == 1);
  vtk_assert(output->GetNumberOfBlocks(1) == 8);
  vtk_assert(output->GetNumberOfBlocks(2) == 40);
  vtk_assert(output->GetNumberOfBlocks(3) == 32);
  vtk_assert(output->GetGridDescription() == vtkStructuredData::VTK_STRUCTURED_XYZ_GRID);
  if (!output->CheckValidity())
  {
    return false;
  }

  filename = output_dir + "/TestAMRXMLIO_HierarchicalBox.vth";
  vtkNew<vtkXMLUniformGridAMRWriter> writer;
  writer->SetFileName(filename.c_str());
  writer->SetInputDataObject(output);
  writer->Write();

  vtkNew<vtkXMLUniformGridAMRReader> reader2;
  reader2->SetFileName(filename.c_str());
  reader2->Update();
  return ValidateOAMR(output, vtkOverlappingAMR::SafeDownCast(reader2->GetOutputDataObject(0)));
}

bool TestAMRXMLIO_DataArraySelection(const std::string& output_dir)
{
  vtkNew<vtkAMRGaussianPulseSource> pulse;
  pulse->SetDimension(3);
  pulse->SetRootSpacing(13);

  std::string filename = output_dir + "/TestAMRXMLIO_DataArraySelection.vth";

  vtkNew<vtkXMLUniformGridAMRWriter> writer;
  writer->SetInputConnection(pulse->GetOutputPort());
  writer->SetFileName(filename.c_str());
  writer->Write();

  vtkNew<vtkXMLUniformGridAMRReader> reader;
  reader->SetFileName(filename.c_str());

  reader->SetCellArrayStatus("Centroid", 0);
  reader->SetCellArrayStatus("Gaussian-Pulse", 0);
  reader->Update();
  auto output = vtkOverlappingAMR::SafeDownCast(reader->GetOutputDataObject(0));
  auto firstDataSet = output->GetDataSetAsImageData(0, 0);
  if (firstDataSet->GetCellData()->GetArray("Centroid") ||
    firstDataSet->GetCellData()->GetArray("Gaussian-Pulse"))
  {
    std::cerr << "Array status failure. Some disabled array are not available." << std::endl;
    return false;
  }

  reader->SetCellArrayStatus("Centroid", 1);
  reader->Update();
  output = vtkOverlappingAMR::SafeDownCast(reader->GetOutputDataObject(0));
  firstDataSet = output->GetDataSetAsImageData(0, 0);
  if (!firstDataSet->GetCellData()->GetArray("Centroid"))
  {
    std::cerr << "Array status failure. Enabled array, Centroid, is not available." << std::endl;
    return false;
  }
  if (firstDataSet->GetCellData()->GetArray("Gaussian-Pulse"))
  {
    std::cerr << "Array status failure. Disabled array, Gaussian-Pulse, is available." << std::endl;
    return false;
  }

  reader->SetCellArrayStatus("Centroid", 0);
  reader->SetCellArrayStatus("Gaussian-Pulse", 1);
  reader->Update();
  output = vtkOverlappingAMR::SafeDownCast(reader->GetOutputDataObject(0));
  firstDataSet = output->GetDataSetAsImageData(0, 0);
  if (!firstDataSet->GetCellData()->GetArray("Gaussian-Pulse"))
  {
    std::cerr << "Array status failure. Enabled array, Gaussian-Pulse, is not available."
              << std::endl;
    return false;
  }
  if (firstDataSet->GetCellData()->GetArray("Centroid"))
  {
    std::cerr << "Array status failure. Disabled array, Centroid, is available." << std::endl;
    return false;
  }

  reader->SetCellArrayStatus("Centroid", 1);
  reader->SetCellArrayStatus("Gaussian-Pulse", 1);
  reader->Update();
  output = vtkOverlappingAMR::SafeDownCast(reader->GetOutputDataObject(0));
  firstDataSet = output->GetDataSetAsImageData(0, 0);
  if (!firstDataSet->GetCellData()->GetArray("Centroid") ||
    !firstDataSet->GetCellData()->GetArray("Gaussian-Pulse"))
  {
    std::cerr << "Array status failure. Some enabled arrays are not available." << std::endl;
    return false;
  }
  return true;
}

bool TestAMRXMLIO_NonOverlappingAMR(
  const std::string& input_dir, const std::string& output_dir, const std::string& file)
{
  std::string inputFilename = input_dir + "/" + file;
  vtkNew<vtkXMLUniformGridAMRReader> reader;
  reader->SetFileName(inputFilename.c_str());

  std::string outputFilename = output_dir + "/" + file;
  vtkNew<vtkXMLUniformGridAMRWriter> writer;
  writer->SetInputConnection(reader->GetOutputPort());
  writer->SetFileName(outputFilename.c_str());
  writer->Write();

  vtkNew<vtkXMLUniformGridAMRReader> reader2;
  reader2->SetFileName(outputFilename.c_str());
  reader2->Update();

  return Validate(vtkNonOverlappingAMR::SafeDownCast(reader->GetOutputDataObject(0)),
    vtkNonOverlappingAMR::SafeDownCast(reader->GetOutputDataObject(0)));
}

bool TestAMRXMLIO_OverlappingAMR(
  const std::string& input_dir, const std::string& output_dir, const std::string& file)
{
  std::string inputFilename = input_dir + "/" + file;
  vtkNew<vtkXMLUniformGridAMRReader> reader;
  reader->SetFileName(inputFilename.c_str());

  std::string outputFilename = output_dir + "/" + file;
  vtkNew<vtkXMLUniformGridAMRWriter> writer;
  writer->SetInputConnection(reader->GetOutputPort());
  writer->SetFileName(outputFilename.c_str());
  writer->Write();

  vtkNew<vtkXMLUniformGridAMRReader> reader2;
  reader2->SetFileName(outputFilename.c_str());
  reader2->Update();

  return ValidateOAMR(vtkOverlappingAMR::SafeDownCast(reader->GetOutputDataObject(0)),
    vtkOverlappingAMR::SafeDownCast(reader->GetOutputDataObject(0)));
}

}

#define VTK_SUCCESS 0
#define VTK_FAILURE 1
int TestAMRXMLIO(int argc, char* argv[])
{
  char* temp_dir =
    vtkTestUtilities::GetArgOrEnvOrDefault("-T", argc, argv, "VTK_TEMP_DIR", "Testing/Temporary");
  if (!temp_dir)
  {
    std::cerr << "Could not determine temporary directory." << std::endl;
    return VTK_FAILURE;
  }

  std::string output_dir = temp_dir;
  delete[] temp_dir;

  std::cout << "Test Overlapping AMR (2D)" << std::endl;
  if (!TestAMRXMLIO_OverlappingAMR2D(output_dir))
  {
    return VTK_FAILURE;
  }

  std::cout << "Test Overlapping AMR (3D)" << std::endl;
  if (!TestAMRXMLIO_OverlappingAMR3D(output_dir))
  {
    return VTK_FAILURE;
  }

  char* data_dir = vtkTestUtilities::GetDataRoot(argc, argv);
  if (!data_dir)
  {
    std::cerr << "Could not determine data directory." << std::endl;
    return VTK_FAILURE;
  }

  std::string input_dir = data_dir;
  input_dir += "/Data";
  delete[] data_dir;

  std::cout << "Test NonOverlapping AMR (UG)" << std::endl;
  if (!TestAMRXMLIO_NonOverlappingAMR(input_dir, output_dir, "AMR/noamr_ug.vth"))
  {
    return VTK_FAILURE;
  }

  std::cout << "Test NonOverlapping AMR (RG)" << std::endl;
  if (!TestAMRXMLIO_NonOverlappingAMR(input_dir, output_dir, "AMR/noamr_rg.vth"))
  {
    return VTK_FAILURE;
  }

  std::cout << "Test Overlapping AMR (RG)" << std::endl;
  if (!TestAMRXMLIO_OverlappingAMR(input_dir, output_dir, "AMR/amr_rg.vth"))
  {
    return VTK_FAILURE;
  }

  std::cout << "Test HierarchicalBox AMR (v1.1)" << std::endl;
  if (!TestAMRXMLIO_HierarchicalBox(input_dir, output_dir))
  {
    return VTK_FAILURE;
  }

  std::cout << "Test DataArraySelection" << std::endl;
  if (!TestAMRXMLIO_DataArraySelection(output_dir))
  {
    return VTK_FAILURE;
  }

  return VTK_SUCCESS;
}
