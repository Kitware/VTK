#include <vtkCellData.h>
#include <vtkCubeSource.h>
#include <vtkDataObjectWriter.h>
#include <vtkDelaunay3D.h>
#include <vtkDirectedGraph.h>
#include <vtkEdgeListIterator.h>
#include <vtkXMLGenericDataObjectReader.h>
#include <vtkXMLDataSetWriter.h>
#include <vtkFieldData.h>
#include <vtkFloatArray.h>
#include <vtkGraph.h>
#include <vtkImageData.h>
#include <vtkImageNoiseSource.h>
#include <vtkInformation.h>
#include <vtkIntArray.h>
#include <vtkMutableDirectedGraph.h>
#include <vtkNew.h>
#include <vtkPointData.h>
#include <vtkPolyData.h>
#include <vtkRandomGraphSource.h>
#include <vtkRectilinearGrid.h>
#include <vtkSmartPointer.h>
#include <vtkStructuredGrid.h>
#include <vtkTable.h>
#include <vtkTree.h>
#include <vtkUndirectedGraph.h>
#include <vtkUniformGrid.h>
#include <vtkUnstructuredGrid.h>
#include <vtkVariant.h>

#include <string>

// Serializable keys to test:
#include "vtkInformationDoubleKey.h"
#include "vtkInformationDoubleVectorKey.h"
#include "vtkInformationIdTypeKey.h"
#include "vtkInformationIntegerKey.h"
#include "vtkInformationIntegerVectorKey.h"
#include "vtkInformationStringKey.h"
#include "vtkInformationStringVectorKey.h"
#include "vtkInformationUnsignedLongKey.h"

namespace
{

static vtkInformationDoubleKey *TestDoubleKey =
    vtkInformationDoubleKey::MakeKey("Double", "XMLTestKey");
// Test RequiredLength keys. DoubleVector must have Length() == 3
static vtkInformationDoubleVectorKey *TestDoubleVectorKey =
    vtkInformationDoubleVectorKey::MakeKey("DoubleVector", "XMLTestKey", 3);
static vtkInformationIdTypeKey *TestIdTypeKey =
    vtkInformationIdTypeKey::MakeKey("IdType", "XMLTestKey");
static vtkInformationIntegerKey *TestIntegerKey =
    vtkInformationIntegerKey::MakeKey("Integer", "XMLTestKey");
static vtkInformationIntegerVectorKey *TestIntegerVectorKey =
    vtkInformationIntegerVectorKey::MakeKey("IntegerVector", "XMLTestKey");
static vtkInformationStringKey *TestStringKey =
    vtkInformationStringKey::MakeKey("String", "XMLTestKey");
static vtkInformationStringVectorKey *TestStringVectorKey =
    vtkInformationStringVectorKey::MakeKey("StringVector", "XMLTestKey");
static vtkInformationUnsignedLongKey *TestUnsignedLongKey =
    vtkInformationUnsignedLongKey::MakeKey("UnsignedLong", "XMLTestKey");

bool stringEqual(const std::string &expect, const std::string &actual)
{
  if (expect != actual)
  {
    std::cerr << "Strings do not match! Expected: '" << expect << "', got: '"
              << actual << "'.\n";
    return false;
  }
  return true;
}

bool stringEqual(const std::string &expect, const char *actual)
{
  return stringEqual(expect, std::string(actual ? actual : ""));
}

template <typename T>
bool compareValues(const std::string &desc, T expect, T actual)
{
  if (expect != actual)
  {
    std::cerr << "Failed comparison for '" << desc << "'. Expected '" << expect
              << "', got '" << actual << "'.\n";
    return false;
  }
  return true;
}

void InitializeDataCommon(vtkDataObject *data)
{
  vtkFieldData *fd = data->GetFieldData();
  if (!fd)
  {
    fd = vtkFieldData::New();
    data->SetFieldData(fd);
    fd->FastDelete();
  }

  // Add a dummy array to test component name and information key serialization.
  vtkNew<vtkFloatArray> array;
  array->SetName("Test Array");
  fd->AddArray(array.GetPointer());
  array->SetNumberOfComponents(3);
  array->SetComponentName(0, "Component 0 name");
  array->SetComponentName(1, "Component 1 name");
  array->SetComponentName(2, "Component 2 name");

  // Test information keys that can be serialized
  vtkInformation *info = array->GetInformation();
  info->Set(TestDoubleKey, 1.0);
  // Setting from an array, since keys with RequiredLength cannot use Append.
  double doubleVecData[3] = {1., 90., 260.};
  info->Set(TestDoubleVectorKey, doubleVecData, 3);
  info->Set(TestIdTypeKey, 5);
  info->Set(TestIntegerKey, 408);
  info->Append(TestIntegerVectorKey, 1);
  info->Append(TestIntegerVectorKey, 5);
  info->Append(TestIntegerVectorKey, 45);
  info->Set(TestStringKey, "Test String!\nLine2");
  info->Append(TestStringVectorKey, "First");
  info->Append(TestStringVectorKey, "Second (with whitespace!)");
  info->Append(TestStringVectorKey, "Third (with\nnewline!)");
  info->Set(TestUnsignedLongKey, 9);
}

bool CompareDataCommon(vtkDataObject *data)
{
  vtkFieldData *fd = data->GetFieldData();
  if (!fd)
  {
    std::cerr << "Field data object missing.\n";
    return false;
  }

  vtkDataArray *array = fd->GetArray("Test Array");
  if (!array)
  {
    std::cerr << "Missing testing array from field data.\n";
    return false;
  }

  if (array->GetNumberOfComponents() != 3)
  {
    std::cerr << "Test array expected to have 3 components, has "
              << array->GetNumberOfComponents() << std::endl;
    return false;
  }

  if (!array->GetComponentName(0) ||
      (strcmp("Component 0 name", array->GetComponentName(0)) != 0) ||
      !array->GetComponentName(1) ||
      (strcmp("Component 1 name", array->GetComponentName(1)) != 0) ||
      !array->GetComponentName(2) ||
      (strcmp("Component 2 name", array->GetComponentName(2)) != 0))
  {
    std::cerr << "Incorrect component names on test array.\n";
    return false;
  }

  vtkInformation *info = array->GetInformation();
  if (!info)
  {
    std::cerr << "Missing array information.\n";
    return false;
  }

  if (!compareValues("double key", 1., info->Get(TestDoubleKey)) ||
      !compareValues("double vector key length", 3, info->Length(TestDoubleVectorKey)) ||
      !compareValues("double vector key @0", 1., info->Get(TestDoubleVectorKey, 0)) ||
      !compareValues("double vector key @1", 90., info->Get(TestDoubleVectorKey, 1)) ||
      !compareValues("double vector key @2", 260., info->Get(TestDoubleVectorKey, 2)) ||
      !compareValues<vtkIdType>("idtype key", 5, info->Get(TestIdTypeKey)) ||
      !compareValues("integer key", 408, info->Get(TestIntegerKey)) ||
      !compareValues("integer vector key length", 3, info->Length(TestIntegerVectorKey)) ||
      !compareValues("integer vector key @0", 1, info->Get(TestIntegerVectorKey, 0)) ||
      !compareValues("integer vector key @1", 5, info->Get(TestIntegerVectorKey, 1)) ||
      !compareValues("integer vector key @2", 45, info->Get(TestIntegerVectorKey, 2)) ||
      !stringEqual("Test String!\nLine2", info->Get(TestStringKey)) ||
      !compareValues("string vector key length", 3, info->Length(TestStringVectorKey)) ||
      !stringEqual("First", info->Get(TestStringVectorKey, 0)) ||
      !stringEqual("Second (with whitespace!)", info->Get(TestStringVectorKey, 1)) ||
      !stringEqual("Third (with\nnewline!)", info->Get(TestStringVectorKey, 2)) ||
      !compareValues("unsigned long key", 9ul, info->Get(TestUnsignedLongKey)))
  {
    return false;
  }

  return true;
}

void InitializeData(vtkImageData* Data)
{
  vtkImageNoiseSource* const source = vtkImageNoiseSource::New();
  source->SetWholeExtent(0, 15, 0, 15, 0, 0);
  source->Update();

  Data->ShallowCopy(source->GetOutput());
  source->Delete();

  InitializeDataCommon(Data);
}

bool CompareData(vtkImageData* Output, vtkImageData* Input)
{
  // Compare both input and output as a sanity check.
  if (!CompareDataCommon(Input) || !CompareDataCommon(Output))
  {
    return false;
  }

  if(memcmp(Input->GetDimensions(), Output->GetDimensions(), 3 * sizeof(int)))
    return false;

  const int point_count = Input->GetDimensions()[0] * Input->GetDimensions()[1] * Input->GetDimensions()[2];
  for(int point = 0; point != point_count; ++point)
  {
    if(memcmp(Input->GetPoint(point), Output->GetPoint(point), 3 * sizeof(double)))
      return false;
  }

  return true;
}

void InitializeData(vtkPolyData* Data)
{
  vtkCubeSource* const source = vtkCubeSource::New();
  source->Update();

  Data->ShallowCopy(source->GetOutput());
  source->Delete();

  InitializeDataCommon(Data);
}

bool CompareData(vtkPolyData* Output, vtkPolyData* Input)
{
  // Compare both input and output as a sanity check.
  if (!CompareDataCommon(Input) || !CompareDataCommon(Output))
  {
    return false;
  }
  if(Input->GetNumberOfPoints() != Output->GetNumberOfPoints())
    return false;
  if(Input->GetNumberOfPolys() != Output->GetNumberOfPolys())
    return false;

  return true;
}

void InitializeData(vtkRectilinearGrid* Data)
{
  Data->SetDimensions(2, 3, 4);
  InitializeDataCommon(Data);
}

bool CompareData(vtkRectilinearGrid* Output, vtkRectilinearGrid* Input)
{
  // Compare both input and output as a sanity check.
  if (!CompareDataCommon(Input) || !CompareDataCommon(Output))
  {
    return false;
  }
  if(memcmp(Input->GetDimensions(), Output->GetDimensions(), 3 * sizeof(int)))
    return false;

  return true;
}

void InitializeData(vtkUniformGrid* Data)
{
  InitializeData(static_cast<vtkImageData*>(Data));
  InitializeDataCommon(Data);
}

void InitializeData(vtkUnstructuredGrid* Data)
{
  vtkCubeSource* const source = vtkCubeSource::New();
  vtkDelaunay3D* const delaunay = vtkDelaunay3D::New();
  delaunay->AddInputConnection(source->GetOutputPort());
  delaunay->Update();

  Data->ShallowCopy(delaunay->GetOutput());

  delaunay->Delete();
  source->Delete();

  InitializeDataCommon(Data);
}

bool CompareData(vtkUnstructuredGrid* Output, vtkUnstructuredGrid* Input)
{
  // Compare both input and output as a sanity check.
  if (!CompareDataCommon(Input) || !CompareDataCommon(Output))
  {
    return false;
  }
  if(Input->GetNumberOfPoints() != Output->GetNumberOfPoints())
    return false;
  if(Input->GetNumberOfCells() != Output->GetNumberOfCells())
    return false;

  return true;
}

template<typename DataT>
bool TestDataObjectXMLSerialization()
{
  DataT* const output_data = DataT::New();
  InitializeData(output_data);

  const char* const filename = output_data->GetClassName();

  vtkXMLDataSetWriter* const writer =
    vtkXMLDataSetWriter::New();
  writer->SetInputData(output_data);
  writer->SetFileName(filename);
  writer->Write();
  writer->Delete();

  vtkXMLGenericDataObjectReader* const reader =
    vtkXMLGenericDataObjectReader::New();
  reader->SetFileName(filename);
  reader->Update();

  vtkDataObject *obj = reader->GetOutput();
  DataT* input_data = DataT::SafeDownCast(obj);
  if(!input_data)
  {
    reader->Delete();
    output_data->Delete();
    return false;
  }

  const bool result = CompareData(output_data, input_data);

  reader->Delete();
  output_data->Delete();

  return result;
}

bool TestUniformGridXMLSerialization()
{
  vtkUniformGrid* const output_data = vtkUniformGrid::New();
  InitializeData(output_data);

  const char* const filename = output_data->GetClassName();

  vtkXMLDataSetWriter* const writer =
    vtkXMLDataSetWriter::New();
  writer->SetInputData(output_data);
  writer->SetFileName(filename);
  writer->Write();
  writer->Delete();

  vtkXMLGenericDataObjectReader* const reader =
    vtkXMLGenericDataObjectReader::New();
  reader->SetFileName(filename);
  reader->Update();

  vtkDataObject *obj = reader->GetOutput();
  vtkImageData* input_data = vtkImageData::SafeDownCast(obj);
  if(!input_data)
  {
    reader->Delete();
    output_data->Delete();
    return false;
  }

  const bool result = CompareData(output_data, input_data);

  reader->Delete();
  output_data->Delete();

  return result;
}
}
int TestDataObjectXMLIO(int /*argc*/, char* /*argv*/[])
{
  int result = 0;

  if(!TestDataObjectXMLSerialization<vtkImageData>())
  {
    cerr << "Error: failure serializing vtkImageData" << endl;
    result = 1;
  }
  if(!TestUniformGridXMLSerialization())
  {
    // note that the current output from serializing a vtkUniformGrid
    // is a vtkImageData. this is the same as writing out a
    // vtkUniformGrid using vtkXMLImageDataWriter.
    cerr << "Error: failure serializing vtkUniformGrid" << endl;
    result = 1;
  }
  if(!TestDataObjectXMLSerialization<vtkPolyData>())
  {
    cerr << "Error: failure serializing vtkPolyData" << endl;
    result = 1;
  }
  if(!TestDataObjectXMLSerialization<vtkRectilinearGrid>())
  {
    cerr << "Error: failure serializing vtkRectilinearGrid" << endl;
    result = 1;
  }
//  if(!TestDataObjectXMLSerialization<vtkStructuredGrid>())
//    {
//    cerr << "Error: failure serializing vtkStructuredGrid" << endl;
//    result = 1;
//    }
  if(!TestDataObjectXMLSerialization<vtkUnstructuredGrid>())
  {
    cerr << "Error: failure serializing vtkUnstructuredGrid" << endl;
    result = 1;
  }

  return result;
}
