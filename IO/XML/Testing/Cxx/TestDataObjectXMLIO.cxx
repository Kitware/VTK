#include <vtkBitArray.h>
#include <vtkCellData.h>
#include <vtkCubeSource.h>
#include <vtkDataObjectWriter.h>
#include <vtkDelaunay3D.h>
#include <vtkDirectedGraph.h>
#include <vtkEdgeListIterator.h>
#include <vtkFieldData.h>
#include <vtkFloatArray.h>
#include <vtkGraph.h>
#include <vtkImageData.h>
#include <vtkImageNoiseSource.h>
#include <vtkInformation.h>
#include <vtkIntArray.h>
#include <vtkMutableDirectedGraph.h>
#include <vtkNew.h>
#include <vtkPermuteOptions.h>
#include <vtkPointData.h>
#include <vtkPolyData.h>
#include <vtkRandomGraphSource.h>
#include <vtkRectilinearGrid.h>
#include <vtkSmartPointer.h>
#include <vtkStructuredGrid.h>
#include <vtkTable.h>
#include <vtkTesting.h>
#include <vtkTree.h>
#include <vtkUndirectedGraph.h>
#include <vtkUniformGrid.h>
#include <vtkUnstructuredGrid.h>
#include <vtkVariant.h>
#include <vtkXMLDataSetWriter.h>
#include <vtkXMLGenericDataObjectReader.h>

#include <sstream>
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

static vtkNew<vtkTesting> TestingData; // For temporary path

static const char* BIT_ARRAY_NAME = "BitArray";
static const char* IDTYPE_ARRAY_NAME = "IdTypeArray";

static vtkInformationDoubleKey* TestDoubleKey =
  vtkInformationDoubleKey::MakeKey("Double", "XMLTestKey");
// Test RequiredLength keys. DoubleVector must have Length() == 3
static vtkInformationDoubleVectorKey* TestDoubleVectorKey =
  vtkInformationDoubleVectorKey::MakeKey("DoubleVector", "XMLTestKey", 3);
static vtkInformationIdTypeKey* TestIdTypeKey =
  vtkInformationIdTypeKey::MakeKey("IdType", "XMLTestKey");
static vtkInformationIntegerKey* TestIntegerKey =
  vtkInformationIntegerKey::MakeKey("Integer", "XMLTestKey");
static vtkInformationIntegerVectorKey* TestIntegerVectorKey =
  vtkInformationIntegerVectorKey::MakeKey("IntegerVector", "XMLTestKey");
static vtkInformationStringKey* TestStringKey =
  vtkInformationStringKey::MakeKey("String", "XMLTestKey");
static vtkInformationStringVectorKey* TestStringVectorKey =
  vtkInformationStringVectorKey::MakeKey("StringVector", "XMLTestKey");
static vtkInformationUnsignedLongKey* TestUnsignedLongKey =
  vtkInformationUnsignedLongKey::MakeKey("UnsignedLong", "XMLTestKey");

bool stringEqual(const std::string& expect, const std::string& actual)
{
  if (expect != actual)
  {
    std::cerr << "Strings do not match! Expected: '" << expect << "', got: '" << actual << "'.\n";
    return false;
  }
  return true;
}

bool stringEqual(const std::string& expect, const char* actual)
{
  return stringEqual(expect, std::string(actual ? actual : ""));
}

template <typename T>
bool compareValues(const std::string& desc, T expect, T actual)
{
  if (expect != actual)
  {
    std::cerr << "Failed comparison for '" << desc << "'. Expected '" << expect << "', got '"
              << actual << "'.\n";
    return false;
  }
  return true;
}

// Generate a somewhat interesting bit pattern for the test bit arrays:
int bitArrayFunc(vtkIdType i)
{
  return (i + (i / 2) + (i / 3) + (i / 5) + (i / 7) + (i / 11)) % 2;
}

void fillBitArray(vtkBitArray* bits)
{
  bits->SetName(BIT_ARRAY_NAME);
  bits->SetNumberOfComponents(4);
  bits->SetNumberOfTuples(100);
  vtkIdType numValues = bits->GetNumberOfValues();
  for (vtkIdType i = 0; i < numValues; ++i)
  {
    bits->SetValue(i, bitArrayFunc(i));
  }
}

bool validateBitArray(vtkAbstractArray* abits)
{
  if (!abits)
  {
    std::cerr << "Bit array not found.\n";
    return false;
  }

  vtkBitArray* bits = vtkBitArray::SafeDownCast(abits);
  if (!bits)
  {
    std::cerr << "Bit Array is incorrect type: " << abits->GetClassName() << ".\n";
    return false;
  }

  vtkIdType numValues = bits->GetNumberOfValues();
  if (numValues != 400)
  {
    std::cerr << "Expected 400 values in bit array, got: " << numValues << "\n";
    return false;
  }

  for (vtkIdType i = 0; i < numValues; ++i)
  {
    int expected = bitArrayFunc(i);
    int actual = bits->GetValue(i);
    if (actual != expected)
    {
      std::cerr << "Bit array invalid - expected " << expected << " , got " << actual
                << " for valueIdx " << i << ".\n";
      return false;
    }
  }

  return true;
}

void fillIdTypeArray(vtkIdTypeArray* ids)
{
  ids->SetName(IDTYPE_ARRAY_NAME);
  ids->SetNumberOfComponents(1);
  ids->SetNumberOfTuples(100);
  for (vtkIdType i = 0; i < 100; ++i)
  {
    ids->SetValue(i, i);
  }
}

bool validateIdTypeArray(vtkAbstractArray* aids)
{
  if (!aids)
  {
    std::cerr << "IdType array not found.\n";
    return false;
  }

  // Ignore the case when the aids is of smaller type than vtkIdType size
  // As this is a possible case when saving data as 32bit with 64bit ids.
  if (aids->GetDataTypeSize() < VTK_SIZEOF_ID_TYPE)
  {
    return true;
  }

  vtkIdTypeArray* ids = vtkIdTypeArray::SafeDownCast(aids);
  if (!ids)
  {
    std::cerr << "idType Array is of incorrect type: " << aids->GetClassName() << ".\n";
    return false;
  }

  vtkIdType numValues = ids->GetNumberOfValues();
  if (numValues != 100)
  {
    std::cerr << "Expected 100 values in id array, got: " << numValues << "\n";
    return false;
  }

  for (vtkIdType i = 0; i < numValues; ++i)
  {
    if (ids->GetValue(i) != i)
    {
      std::cerr << "id array invalid - expected " << i << " , got " << ids->GetValue(i)
                << " for valueIdx " << i << ".\n";
      return false;
    }
  }

  return true;
}

void InitializeDataCommon(vtkDataObject* data)
{
  vtkFieldData* fd = data->GetFieldData();
  if (!fd)
  {
    fd = vtkFieldData::New();
    data->SetFieldData(fd);
    fd->FastDelete();
  }

  // Add a dummy array to test component name and information key serialization.
  vtkNew<vtkFloatArray> array;
  array->SetName("Test Array");
  fd->AddArray(array);
  array->SetNumberOfComponents(3);
  array->SetComponentName(0, "Component 0 name");
  array->SetComponentName(1, "Component 1 name");
  array->SetComponentName(2, "Component 2 name");

  // Test information keys that can be serialized
  vtkInformation* info = array->GetInformation();
  info->Set(TestDoubleKey, 1.0);
  // Setting from an array, since keys with RequiredLength cannot use Append.
  double doubleVecData[3] = { 1., 90., 260. };
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

  // Ensure that bit arrays are handled properly (#17197)
  vtkNew<vtkBitArray> bits;
  fillBitArray(bits);
  fd->AddArray(bits);

  // Ensure that idType arrays are handled properly (#17421)
  vtkNew<vtkIdTypeArray> ids;
  fillIdTypeArray(ids);
  fd->AddArray(ids);
}

bool CompareDataCommon(vtkDataObject* data)
{
  vtkFieldData* fd = data->GetFieldData();
  if (!fd)
  {
    std::cerr << "Field data object missing.\n";
    return false;
  }

  vtkDataArray* array = fd->GetArray("Test Array");
  if (!array)
  {
    std::cerr << "Missing testing array from field data.\n";
    return false;
  }

  if (array->GetNumberOfComponents() != 3)
  {
    std::cerr << "Test array expected to have 3 components, has " << array->GetNumberOfComponents()
              << std::endl;
    return false;
  }

  if (!array->GetComponentName(0) ||
    (strcmp("Component 0 name", array->GetComponentName(0)) != 0) || !array->GetComponentName(1) ||
    (strcmp("Component 1 name", array->GetComponentName(1)) != 0) || !array->GetComponentName(2) ||
    (strcmp("Component 2 name", array->GetComponentName(2)) != 0))
  {
    std::cerr << "Incorrect component names on test array.\n";
    return false;
  }

  vtkInformation* info = array->GetInformation();
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

  if (!validateBitArray(fd->GetAbstractArray(BIT_ARRAY_NAME)))
  {
    return false;
  }

  if (!validateIdTypeArray(fd->GetAbstractArray(IDTYPE_ARRAY_NAME)))
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

  if (memcmp(Input->GetDimensions(), Output->GetDimensions(), 3 * sizeof(int)))
    return false;

  const int point_count =
    Input->GetDimensions()[0] * Input->GetDimensions()[1] * Input->GetDimensions()[2];
  for (int point = 0; point != point_count; ++point)
  {
    if (memcmp(Input->GetPoint(point), Output->GetPoint(point), 3 * sizeof(double)))
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
  if (Input->GetNumberOfPoints() != Output->GetNumberOfPoints())
    return false;
  if (Input->GetNumberOfPolys() != Output->GetNumberOfPolys())
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
  if (memcmp(Input->GetDimensions(), Output->GetDimensions(), 3 * sizeof(int)))
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
  if (Input->GetNumberOfPoints() != Output->GetNumberOfPoints())
    return false;
  if (Input->GetNumberOfCells() != Output->GetNumberOfCells())
    return false;

  return true;
}

//------------------------------------------------------------------------------
// Determine the data object read as member Type for a given WriterDataObjectT.
template <typename WriterDataObjectT>
struct GetReaderDataObjectType
{
  using Type = WriterDataObjectT;
};

// Specialize for vtkUniformGrid --> vtkImageData
template <>
struct GetReaderDataObjectType<vtkUniformGrid>
{
  using Type = vtkImageData;
};

class WriterConfig : public vtkPermuteOptions<vtkXMLDataSetWriter>
{
public:
  WriterConfig()
  {
    this->AddOptionValues("ByteOrder", &vtkXMLDataObjectWriter::SetByteOrder, "BigEndian",
      vtkXMLWriter::BigEndian, "LittleEndian", vtkXMLWriter::LittleEndian);
    this->AddOptionValues("HeaderType", &vtkXMLDataObjectWriter::SetHeaderType, "32Bit",
      vtkXMLWriter::UInt32, "64Bit", vtkXMLWriter::UInt64);
    this->AddOptionValues("CompressorType", &vtkXMLDataObjectWriter::SetCompressorType, "NONE",
      vtkXMLWriter::NONE, "ZLIB", vtkXMLWriter::ZLIB, "LZ4", vtkXMLWriter::LZ4);
    this->AddOptionValues("DataMode", &vtkXMLDataObjectWriter::SetDataMode, "Ascii",
      vtkXMLWriter::Ascii, "Binary", vtkXMLWriter::Binary, "Appended", vtkXMLWriter::Appended);

    // Calling vtkXMLWriter::SetIdType throws an Error while requesting 64 bit
    // ids if this option isn't set:
    this->AddOptionValue(
      "IdType", &vtkXMLDataObjectWriter::SetIdType, "32Bit", vtkXMLWriter::Int32);
#ifdef VTK_USE_64BIT_IDS
    this->AddOptionValue(
      "IdType", &vtkXMLDataObjectWriter::SetIdType, "64Bit", vtkXMLWriter::Int64);
#endif
  }
};

//------------------------------------------------------------------------------
// Main test function for a given data type and writer configuration.
template <typename WriterDataObjectT>
bool TestDataObjectXMLSerialization(const WriterConfig& writerConfig)
{
  using ReaderDataObjectT = typename GetReaderDataObjectType<WriterDataObjectT>::Type;

  WriterDataObjectT* const output_data = WriterDataObjectT::New();
  InitializeData(output_data);

  std::ostringstream filename;
  filename << TestingData->GetTempDirectory() << "/" << output_data->GetClassName() << "-"
           << writerConfig.GetCurrentPermutationName();

  vtkXMLDataSetWriter* const writer = vtkXMLDataSetWriter::New();
  writer->SetInputData(output_data);
  writer->SetFileName(filename.str().c_str());
  writerConfig.ApplyCurrentPermutation(writer);
  writer->Write();
  writer->Delete();

  vtkXMLGenericDataObjectReader* const reader = vtkXMLGenericDataObjectReader::New();
  reader->SetFileName(filename.str().c_str());
  reader->Update();

  vtkDataObject* obj = reader->GetOutput();
  ReaderDataObjectT* input_data = ReaderDataObjectT::SafeDownCast(obj);
  if (!input_data)
  {
    reader->Delete();
    output_data->Delete();
    return false;
  }

  const bool result = CompareData(output_data, input_data);

  reader->Delete();
  output_data->Delete();

  if (!result)
  {
    std::cerr << "Comparison failed. Filename: " << filename.str() << "\n";
  }

  return result;
}

//------------------------------------------------------------------------------
// Test all permutations of the writer configuration with a given data type.
template <typename WriterDataObjectT>
bool TestWriterPermutations()
{
  bool result = true;
  WriterConfig config;

  config.InitPermutations();
  while (!config.IsDoneWithPermutations())
  {
    { // Some progress/debugging output:
      std::string testName;
      vtkNew<WriterDataObjectT> dummy;
      std::ostringstream tmp;
      tmp << dummy->GetClassName() << " [" << config.GetCurrentPermutationName() << "]";
      testName = tmp.str();
      std::cerr << "Testing: " << testName << "..." << std::endl;
    }

    if (!TestDataObjectXMLSerialization<WriterDataObjectT>(config))
    {
      std::cerr << "Failed.\n\n";
      result = false;
    }

    config.GoToNextPermutation();
  }

  return result;
}

} // end anon namespace

int TestDataObjectXMLIO(int argc, char* argv[])
{
  TestingData->AddArguments(argc, argv);

  int result = 0;

  if (!TestWriterPermutations<vtkImageData>())
  {
    result = 1;
  }
  if (!TestWriterPermutations<vtkUniformGrid>())
  {
    // note that the current output from serializing a vtkUniformGrid
    // is a vtkImageData. this is the same as writing out a
    // vtkUniformGrid using vtkXMLImageDataWriter.
    result = 1;
  }
  if (!TestWriterPermutations<vtkPolyData>())
  {
    result = 1;
  }
  if (!TestWriterPermutations<vtkRectilinearGrid>())
  {
    result = 1;
  }
  //  if(!TestWriterPermutations<vtkStructuredGrid>())
  //    {
  //    result = 1;
  //    }
  if (!TestWriterPermutations<vtkUnstructuredGrid>())
  {
    result = 1;
  }

  return result;
}
