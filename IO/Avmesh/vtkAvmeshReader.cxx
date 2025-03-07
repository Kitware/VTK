#include "vtkAvmeshReader.h"
#include "AvmeshInternals.h"
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkMultiBlockDataSet.h>
#include <vtksys/SystemTools.hxx>

vtkAvmeshReader::vtkAvmeshReader()
  : FileName("")
  , SurfaceOnly(false)
{
}

vtkAvmeshReader::~vtkAvmeshReader() = default;

int vtkAvmeshReader::CanReadFile(VTK_FILEPATH const char* filename)
{
  return vtksys::SystemTools::TestFileAccess(filename, vtksys::TEST_FILE_READ);
}

int vtkAvmeshReader::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  vtkMultiBlockDataSet* output =
    vtkMultiBlockDataSet::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  try
  {
    ReadAvmesh(output, FileName, SurfaceOnly);
  }
  catch (AvmeshError const& ex)
  {
    vtkErrorMacro(<< ex.what());
    return 0;
  }

  return 1;
}
