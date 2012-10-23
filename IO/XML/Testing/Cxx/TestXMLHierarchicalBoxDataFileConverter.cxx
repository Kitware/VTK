#include "vtkNew.h"
#include "vtkOverlappingAMR.h"
#include "vtkTestUtilities.h"
#include "vtkXMLGenericDataObjectReader.h"
#include "vtkXMLHierarchicalBoxDataFileConverter.h"

#include <vtksys/SystemTools.hxx>
#include <string>

#define VTK_SUCCESS 0
#define VTK_FAILURE 1
int TestXMLHierarchicalBoxDataFileConverter(int argc, char* argv[])
{
  char* temp_dir = vtkTestUtilities::GetArgOrEnvOrDefault(
    "-T", argc, argv, "VTK_TEMP_DIR", "Testing/Temporary");
  if (!temp_dir)
    {
    cerr << "Could not determine temporary directory." << endl;
    return VTK_FAILURE;
    }

  char* data_dir = vtkTestUtilities::GetDataRoot(argc, argv);
  if (!data_dir)
    {
    cerr << "Could not determine data directory." << endl;
    return VTK_FAILURE;
    }

  std::string input = data_dir;
  input += "/Data/AMR/HierarchicalBoxDataset.v1.0.vthb";

  std::string output = temp_dir;
  output += "/HierarchicalBoxDataset.Converted.v1.1.vthb";

  vtkNew<vtkXMLHierarchicalBoxDataFileConverter> converter;
  converter->SetInputFileName(input.c_str());
  converter->SetOutputFileName(output.c_str());

  if (!converter->Convert())
    {
    return VTK_FAILURE;
    }

  // Copy the subfiles over to the temporary directory so that we can test
  // loading the written file.
  std::string input_dir = data_dir;
  input_dir += "/Data/AMR/HierarchicalBoxDataset.v1.0";

  std::string output_dir = temp_dir;
  output_dir += "/HierarchicalBoxDataset.Converted.v1.1";

  vtksys::SystemTools::RemoveADirectory(output_dir.c_str());
  if (!vtksys::SystemTools::CopyADirectory(
      input_dir.c_str(), output_dir.c_str()))
    {
    cerr << "Failed to copy image data files over for testing." << endl;
    return VTK_FAILURE;
    }

  vtkNew<vtkXMLGenericDataObjectReader> reader;
  reader->SetFileName(output.c_str());
  reader->Update();
  vtkOverlappingAMR::SafeDownCast(reader->GetOutputDataObject(0))->Audit();
  return VTK_SUCCESS;
}
