#include "vtkAMRGaussianPulseSource.h"
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

namespace
{
#define vtk_assert(x)                                                                              \
  do                                                                                               \
  {                                                                                                \
    if (!(x))                                                                                      \
    {                                                                                              \
      cerr << "ERROR: Condition FAILED!! : " << #x << endl;                                        \
      return false;                                                                                \
    }                                                                                              \
  } while (false)

bool Validate(vtkOverlappingAMR* input, vtkOverlappingAMR* result)
{
  vtk_assert(input->GetNumberOfLevels() == result->GetNumberOfLevels());
  vtk_assert(input->GetOrigin()[0] == result->GetOrigin()[0]);
  vtk_assert(input->GetOrigin()[1] == result->GetOrigin()[1]);
  vtk_assert(input->GetOrigin()[2] == result->GetOrigin()[2]);

  for (unsigned int level = 0; level < input->GetNumberOfLevels(); level++)
  {
    vtk_assert(input->GetNumberOfDataSets(level) == result->GetNumberOfDataSets(level));
  }

  cout << "Audit Input" << endl;
  input->Audit();
  cout << "Audit Output" << endl;
  result->Audit();
  return true;
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

  return Validate(vtkOverlappingAMR::SafeDownCast(pulse->GetOutputDataObject(0)),
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

  return Validate(vtkOverlappingAMR::SafeDownCast(pulse->GetOutputDataObject(0)),
    vtkOverlappingAMR::SafeDownCast(reader->GetOutputDataObject(0)));
}

bool TestAMRXMLIO_HierarchicalBox(const std::string& input_dir, const std::string& output_dir)
{
  std::string filename = input_dir + "/AMR/HierarchicalBoxDataset.v1.1.vthb";
  // for vtkHierarchicalBoxDataSet, vtkXMLGenericDataObjectReader creates the
  // legacy reader by default. For version 1.1, we should use the
  // vtkXMLUniformGridAMRReader explicitly. vtkHierarchicalBoxDataSet itself is
  // obsolete.
  vtkNew<vtkXMLUniformGridAMRReader> reader;
  reader->SetFileName(filename.c_str());
  reader->Update();

  vtkOverlappingAMR* output = vtkOverlappingAMR::SafeDownCast(reader->GetOutputDataObject(0));
  vtk_assert(output->GetNumberOfLevels() == 4);
  vtk_assert(output->GetNumberOfDataSets(0) == 1);
  vtk_assert(output->GetNumberOfDataSets(1) == 8);
  vtk_assert(output->GetNumberOfDataSets(2) == 40);
  vtk_assert(output->GetNumberOfDataSets(3) == 32);
  vtk_assert(output->GetGridDescription() == VTK_XYZ_GRID);
  output->Audit();

  filename = output_dir + "/TestAMRXMLIO_HierarchicalBox.vth";
  vtkNew<vtkXMLUniformGridAMRWriter> writer;
  writer->SetFileName(filename.c_str());
  writer->SetInputDataObject(output);
  writer->Write();

  vtkNew<vtkXMLUniformGridAMRReader> reader2;
  reader2->SetFileName(filename.c_str());
  reader2->Update();
  return Validate(output, vtkOverlappingAMR::SafeDownCast(reader2->GetOutputDataObject(0)));
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
  auto firstDataSet = output->GetDataSet(0, 0);
  if (firstDataSet->GetCellData()->GetArray("Centroid") ||
    firstDataSet->GetCellData()->GetArray("Gaussian-Pulse"))
  {
    cerr << "Array status failure. Some disabled array are not available." << endl;
    return false;
  }

  reader->SetCellArrayStatus("Centroid", 1);
  reader->Update();
  output = vtkOverlappingAMR::SafeDownCast(reader->GetOutputDataObject(0));
  firstDataSet = output->GetDataSet(0, 0);
  if (!firstDataSet->GetCellData()->GetArray("Centroid"))
  {
    cerr << "Array status failure. Enabled array, Centroid, is not available." << endl;
    return false;
  }
  if (firstDataSet->GetCellData()->GetArray("Gaussian-Pulse"))
  {
    cerr << "Array status failure. Disabled array, Gaussian-Pulse, is available." << endl;
    return false;
  }

  reader->SetCellArrayStatus("Centroid", 0);
  reader->SetCellArrayStatus("Gaussian-Pulse", 1);
  reader->Update();
  output = vtkOverlappingAMR::SafeDownCast(reader->GetOutputDataObject(0));
  firstDataSet = output->GetDataSet(0, 0);
  if (!firstDataSet->GetCellData()->GetArray("Gaussian-Pulse"))
  {
    cerr << "Array status failure. Enabled array, Gaussian-Pulse, is not available." << endl;
    return false;
  }
  if (firstDataSet->GetCellData()->GetArray("Centroid"))
  {
    cerr << "Array status failure. Disabled array, Centroid, is available." << endl;
    return false;
  }

  reader->SetCellArrayStatus("Centroid", 1);
  reader->SetCellArrayStatus("Gaussian-Pulse", 1);
  reader->Update();
  output = vtkOverlappingAMR::SafeDownCast(reader->GetOutputDataObject(0));
  firstDataSet = output->GetDataSet(0, 0);
  if (!firstDataSet->GetCellData()->GetArray("Centroid") ||
    !firstDataSet->GetCellData()->GetArray("Gaussian-Pulse"))
  {
    cerr << "Array status failure. Some enabled arrays are not available." << endl;
    return false;
  }
  return true;
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
    cerr << "Could not determine temporary directory." << endl;
    return VTK_FAILURE;
  }

  std::string output_dir = temp_dir;
  delete[] temp_dir;

  cout << "Test Overlapping AMR (2D)" << endl;
  if (!TestAMRXMLIO_OverlappingAMR2D(output_dir))
  {
    return VTK_FAILURE;
  }

  cout << "Test Overlapping AMR (3D)" << endl;
  if (!TestAMRXMLIO_OverlappingAMR3D(output_dir))
  {
    return VTK_FAILURE;
  }

  char* data_dir = vtkTestUtilities::GetDataRoot(argc, argv);
  if (!data_dir)
  {
    cerr << "Could not determine data directory." << endl;
    return VTK_FAILURE;
  }

  std::string input_dir = data_dir;
  input_dir += "/Data";
  delete[] data_dir;

  cout << "Test HierarchicalBox AMR (v1.1)" << endl;
  if (!TestAMRXMLIO_HierarchicalBox(input_dir, output_dir))
  {
    return VTK_FAILURE;
  }

  cout << "Test DataArraySelection" << endl;
  if (!TestAMRXMLIO_DataArraySelection(output_dir))
  {
    return VTK_FAILURE;
  }

  return VTK_SUCCESS;
}
