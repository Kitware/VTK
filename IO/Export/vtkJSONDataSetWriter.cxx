/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkJSONDataSetWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkJSONDataSetWriter.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkDataSet.h"
#include "vtkDataSetAttributes.h"
#include "vtkIdTypeArray.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkSmartPointer.h"
#include "vtkTypeInt32Array.h"
#include "vtkTypeInt64Array.h"
#include "vtkTypeUInt32Array.h"
#include "vtkTypeUInt64Array.h"
#include "vtksys/FStream.hxx"
#include "vtksys/MD5.h"
#include "vtksys/SystemTools.hxx"

#include "vtkArchiver.h"

#include <fstream>
#include <sstream>
#include <string>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkJSONDataSetWriter);
vtkCxxSetObjectMacro(vtkJSONDataSetWriter, Archiver, vtkArchiver);

//----------------------------------------------------------------------------
vtkJSONDataSetWriter::vtkJSONDataSetWriter()
{
  this->Archiver = vtkArchiver::New();
  this->ValidStringCount = 1;
}

//----------------------------------------------------------------------------
vtkJSONDataSetWriter::~vtkJSONDataSetWriter()
{
  this->SetArchiver(nullptr);
}

//----------------------------------------------------------------------------

#if !defined(VTK_LEGACY_REMOVE)
void vtkJSONDataSetWriter::SetFileName(const char* archiveName)
{
  this->Archiver->SetArchiveName(archiveName);
}
#endif

//----------------------------------------------------------------------------

#if !defined(VTK_LEGACY_REMOVE)
char* vtkJSONDataSetWriter::GetFileName()
{
  return this->Archiver->GetArchiveName();
}
#endif

// ----------------------------------------------------------------------------

vtkDataSet* vtkJSONDataSetWriter::GetInput()
{
  return vtkDataSet::SafeDownCast(this->Superclass::GetInput());
}

// ----------------------------------------------------------------------------

vtkDataSet* vtkJSONDataSetWriter::GetInput(int port)
{
  return vtkDataSet::SafeDownCast(this->Superclass::GetInput(port));
}

// ----------------------------------------------------------------------------

std::string vtkJSONDataSetWriter::WriteDataSetAttributes(
  vtkDataSetAttributes* fields, const char* className)
{
  int nbArrayWritten = 0;
  vtkIdType activeTCoords = -1;
  vtkIdType activeScalars = -1;
  vtkIdType activeNormals = -1;
  vtkIdType activeGlobalIds = -1;
  vtkIdType activeTensors = -1;
  vtkIdType activePedigreeIds = -1;
  vtkIdType activeVectors = -1;

  vtkIdType nbFields = fields->GetNumberOfArrays();

  if (nbFields == 0)
  {
    return "";
  }

  std::stringstream jsonSnippet;
  jsonSnippet << "  \"" << className << "\": {"
              << "\n    \"vtkClass\": \"vtkDataSetAttributes\","
              << "\n    \"arrays\": [\n";
  for (vtkIdType idx = 0; idx < nbFields; idx++)
  {
    vtkDataArray* field = fields->GetArray(idx);
    if (field == nullptr)
    {
      continue;
    }

    if (nbArrayWritten)
    {
      jsonSnippet << ",\n";
    }

    jsonSnippet << "      { \"data\": " << this->WriteArray(field, "vtkDataArray") << "}";

    // Update active field if any
    activeTCoords = field == fields->GetTCoords() ? nbArrayWritten : activeTCoords;
    activeScalars = field == fields->GetScalars() ? nbArrayWritten : activeScalars;
    activeNormals = field == fields->GetNormals() ? nbArrayWritten : activeNormals;
    activeGlobalIds = field == fields->GetGlobalIds() ? nbArrayWritten : activeGlobalIds;
    activeTensors = field == fields->GetTensors() ? nbArrayWritten : activeTensors;
    activePedigreeIds = field == fields->GetPedigreeIds() ? nbArrayWritten : activePedigreeIds;
    activeVectors = field == fields->GetVectors() ? nbArrayWritten : activeVectors;

    // Increment the number of array currently in the list
    nbArrayWritten++;
  }
  jsonSnippet << "\n    ],\n"
              << "    \"activeTCoords\": " << activeTCoords << ",\n"
              << "    \"activeScalars\": " << activeScalars << ",\n"
              << "    \"activeNormals\": " << activeNormals << ",\n"
              << "    \"activeGlobalIds\": " << activeGlobalIds << ",\n"
              << "    \"activeTensors\": " << activeTensors << ",\n"
              << "    \"activePedigreeIds\": " << activePedigreeIds << ",\n"
              << "    \"activeVectors\": " << activeVectors << "\n"
              << "  }";

  return jsonSnippet.str();
}

// ----------------------------------------------------------------------------

std::string vtkJSONDataSetWriter::WriteArray(
  vtkDataArray* array, const char* className, const char* arrayName)
{
  bool needConvert;
  std::string id = vtkJSONDataSetWriter::GetUID(array, needConvert);
  std::stringstream arrayPath;
  arrayPath << "data/" << id.c_str();
  bool success = vtkJSONDataSetWriter::WriteArrayContents(array, arrayPath.str().c_str());

  if (!success)
  {
    return "{}";
  }

  const char* INDENT = "    ";
  std::stringstream ss;
  ss << "{\n"
     << INDENT << "  \"vtkClass\": \"" << className << "\",\n"
     << INDENT << "  \"name\": \""
     << this->GetValidString(arrayName == nullptr ? array->GetName() : arrayName) << "\",\n"
     << INDENT << "  \"numberOfComponents\": " << array->GetNumberOfComponents() << ",\n"
     << INDENT << "  \"dataType\": \"" << vtkJSONDataSetWriter::GetShortType(array, needConvert)
     << "Array\",\n"
     << INDENT << "  \"ref\": {\n"
     << INDENT << "     \"encode\": \"LittleEndian\",\n"
     << INDENT << "     \"basepath\": \"data\",\n"
     << INDENT << "     \"id\": \"" << id.c_str() << "\"\n"
     << INDENT << "  },\n"
     << INDENT << "  \"size\": " << array->GetNumberOfValues() << "\n"
     << INDENT << "}";

  return ss.str();
}

//----------------------------------------------------------------------------
void vtkJSONDataSetWriter::Write(vtkDataSet* dataset)
{
  vtkImageData* imageData = vtkImageData::SafeDownCast(dataset);
  vtkPolyData* polyData = vtkPolyData::SafeDownCast(dataset);
  this->ValidDataSet = false;

  // Get input and check data
  if (dataset == nullptr)
  {
    vtkErrorMacro(<< "No data to write!");
    return;
  }

  this->GetArchiver()->OpenArchive();

  // Capture vtkDataSet definition
  std::stringstream metaJsonFile;
  metaJsonFile << "{\n";
  metaJsonFile << "  \"vtkClass\": \"" << dataset->GetClassName() << "\"";

  // ImageData
  if (imageData)
  {
    this->ValidDataSet = true;

    // Spacing
    metaJsonFile << ",\n  \"spacing\": [" << imageData->GetSpacing()[0] << ", "
                 << imageData->GetSpacing()[1] << ", " << imageData->GetSpacing()[2] << "]";

    // Origin
    metaJsonFile << ",\n  \"origin\": [" << imageData->GetOrigin()[0] << ", "
                 << imageData->GetOrigin()[1] << ", " << imageData->GetOrigin()[2] << "]";

    // Extent
    metaJsonFile << ",\n  \"extent\": [" << imageData->GetExtent()[0] << ", "
                 << imageData->GetExtent()[1] << ", " << imageData->GetExtent()[2] << ", "
                 << imageData->GetExtent()[3] << ", " << imageData->GetExtent()[4] << ", "
                 << imageData->GetExtent()[5] << "]";
  }

  // PolyData
  if (polyData && polyData->GetPoints())
  {
    this->ValidDataSet = true;

    vtkPoints* points = polyData->GetPoints();
    metaJsonFile << ",\n  \"points\": "
                 << this->WriteArray(points->GetData(), "vtkPoints", "points").c_str();

    // Verts
    vtkNew<vtkIdTypeArray> cells;
    polyData->GetVerts()->ExportLegacyFormat(cells);
    if (cells->GetNumberOfValues())
    {
      metaJsonFile << ",\n  \"verts\": "
                   << this->WriteArray(cells, "vtkCellArray", "verts").c_str();
    }

    // Lines
    polyData->GetLines()->ExportLegacyFormat(cells);
    if (cells->GetNumberOfValues())
    {
      metaJsonFile << ",\n  \"lines\": "
                   << this->WriteArray(cells, "vtkCellArray", "lines").c_str();
    }

    // Strips
    polyData->GetStrips()->ExportLegacyFormat(cells);
    if (cells->GetNumberOfValues())
    {
      metaJsonFile << ",\n  \"strips\": "
                   << this->WriteArray(cells, "vtkCellArray", "strips").c_str();
    }

    // Polys
    polyData->GetPolys()->ExportLegacyFormat(cells);
    if (cells->GetNumberOfValues())
    {
      metaJsonFile << ",\n  \"polys\": "
                   << this->WriteArray(cells, "vtkCellArray", "polys").c_str();
    }
  }

  // PointData
  std::string fieldJSON = this->WriteDataSetAttributes(dataset->GetPointData(), "pointData");
  if (!fieldJSON.empty())
  {
    metaJsonFile << ",\n" << fieldJSON.c_str();
  }

  // CellData
  fieldJSON = this->WriteDataSetAttributes(dataset->GetCellData(), "cellData");
  if (!fieldJSON.empty())
  {
    metaJsonFile << ",\n" << fieldJSON.c_str();
  }

  metaJsonFile << "}\n";

  // Write meta-data file
  std::string metaJsonFileStr = metaJsonFile.str();
  this->GetArchiver()->InsertIntoArchive(
    "index.json", metaJsonFileStr.c_str(), metaJsonFileStr.size());

  this->GetArchiver()->CloseArchive();
}

//----------------------------------------------------------------------------
void vtkJSONDataSetWriter::WriteData()
{
  vtkDataSet* dataset = this->GetInput();
  this->Write(dataset);
}

// ----------------------------------------------------------------------------
bool vtkJSONDataSetWriter::WriteArrayContents(vtkDataArray* input, const char* filePath)
{
  if (input->GetDataTypeSize() == 0)
  {
    // Skip BIT arrays
    return false;
  }

  // Check if we need to convert the (u)int64 to (u)int32
  vtkSmartPointer<vtkDataArray> arrayToWrite = input;
  vtkIdType arraySize = input->GetNumberOfTuples() * input->GetNumberOfComponents();
  switch (input->GetDataType())
  {
    case VTK_UNSIGNED_CHAR:
    case VTK_UNSIGNED_LONG:
    case VTK_UNSIGNED_LONG_LONG:
      if (input->GetDataTypeSize() > 4)
      {
        vtkNew<vtkTypeUInt64Array> srcUInt64;
        srcUInt64->ShallowCopy(input);
        vtkNew<vtkTypeUInt32Array> uint32;
        uint32->SetNumberOfValues(arraySize);
        uint32->SetName(input->GetName());
        for (vtkIdType i = 0; i < arraySize; i++)
        {
          uint32->SetValue(i, srcUInt64->GetValue(i));
        }
        arrayToWrite = uint32;
      }
      break;
    case VTK_LONG:
    case VTK_LONG_LONG:
    case VTK_ID_TYPE:
      if (input->GetDataTypeSize() > 4)
      {
        vtkNew<vtkTypeInt64Array> srcInt64;
        srcInt64->ShallowCopy(input);
        vtkNew<vtkTypeInt32Array> int32;
        int32->SetNumberOfTuples(arraySize);
        int32->SetName(input->GetName());
        for (vtkIdType i = 0; i < arraySize; i++)
        {
          int32->SetValue(i, srcInt64->GetValue(i));
        }
        arrayToWrite = int32;
      }
      break;
  }

  const char* content = (const char*)arrayToWrite->GetVoidPointer(0);
  size_t size = arrayToWrite->GetNumberOfValues() * arrayToWrite->GetDataTypeSize();

  this->GetArchiver()->InsertIntoArchive(filePath, content, size);
  return true;
}

//----------------------------------------------------------------------------
namespace
{
class vtkSingleFileArchiver : public vtkArchiver
{
public:
  static vtkSingleFileArchiver* New();
  vtkTypeMacro(vtkSingleFileArchiver, vtkArchiver);

  virtual void OpenArchive() override {}
  virtual void CloseArchive() override {}
  virtual void InsertIntoArchive(
    const std::string& filePath, const char* data, std::size_t size) override
  {
    vtksys::ofstream file;
    file.open(filePath.c_str(), ios::out | ios::binary);
    file.write(data, size);
    file.close();
  }

private:
  vtkSingleFileArchiver() = default;
  virtual ~vtkSingleFileArchiver() override = default;
};
vtkStandardNewMacro(vtkSingleFileArchiver);
}

//----------------------------------------------------------------------------
bool vtkJSONDataSetWriter::WriteArrayAsRAW(vtkDataArray* array, const char* filePath)
{
  vtkNew<vtkJSONDataSetWriter> writer;
  vtkNew<vtkSingleFileArchiver> archiver;
  writer->SetArchiver(archiver);
  return writer->WriteArrayContents(array, filePath);
}

//----------------------------------------------------------------------------
void vtkJSONDataSetWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

// ----------------------------------------------------------------------------
int vtkJSONDataSetWriter::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  return 1;
}

// ----------------------------------------------------------------------------
// Static helper functions
// ----------------------------------------------------------------------------

void vtkJSONDataSetWriter::ComputeMD5(const unsigned char* content, int size, std::string& hash)
{
  unsigned char digest[16];
  char md5Hash[33];
  md5Hash[32] = '\0';

  vtksysMD5* md5 = vtksysMD5_New();
  vtksysMD5_Initialize(md5);
  vtksysMD5_Append(md5, content, size);
  vtksysMD5_Finalize(md5, digest);
  vtksysMD5_DigestToHex(digest, md5Hash);
  vtksysMD5_Delete(md5);

  hash = md5Hash;
}

// ----------------------------------------------------------------------------

std::string vtkJSONDataSetWriter::GetShortType(vtkDataArray* input, bool& needConversion)
{
  needConversion = false;
  std::stringstream ss;
  switch (input->GetDataType())
  {
    case VTK_UNSIGNED_CHAR:
    case VTK_UNSIGNED_SHORT:
    case VTK_UNSIGNED_INT:
    case VTK_UNSIGNED_LONG:
    case VTK_UNSIGNED_LONG_LONG:
      ss << "Uint";
      if (input->GetDataTypeSize() <= 4)
      {
        ss << (input->GetDataTypeSize() * 8);
      }
      else
      {
        needConversion = true;
        ss << "32";
      }

      break;

    case VTK_CHAR:
    case VTK_SIGNED_CHAR:
    case VTK_SHORT:
    case VTK_INT:
    case VTK_LONG:
    case VTK_LONG_LONG:
    case VTK_ID_TYPE:
      ss << "Int";
      if (input->GetDataTypeSize() <= 4)
      {
        ss << (input->GetDataTypeSize() * 8);
      }
      else
      {
        needConversion = true;
        ss << "32";
      }
      break;

    case VTK_FLOAT:
    case VTK_DOUBLE:
      ss << "Float";
      ss << (input->GetDataTypeSize() * 8);
      break;

    case VTK_BIT:
    case VTK_STRING:
    case VTK_UNICODE_STRING:
    case VTK_VARIANT:
    default:
      ss << "xxx";
      break;
  }

  return ss.str();
}

// ----------------------------------------------------------------------------

std::string vtkJSONDataSetWriter::GetUID(vtkDataArray* input, bool& needConversion)
{
  const unsigned char* content = (const unsigned char*)input->GetVoidPointer(0);
  int size = input->GetNumberOfValues() * input->GetDataTypeSize();
  std::string hash;
  vtkJSONDataSetWriter::ComputeMD5(content, size, hash);

  std::stringstream ss;
  ss << vtkJSONDataSetWriter::GetShortType(input, needConversion) << "_"
     << input->GetNumberOfValues() << "-" << hash.c_str();

  return ss.str();
}

// ----------------------------------------------------------------------------

std::string vtkJSONDataSetWriter::GetValidString(const char* name)
{
  if (name != nullptr && strlen(name))
  {
    return name;
  }
  std::stringstream ss;
  ss << "invalid_" << this->ValidStringCount++;

  return ss.str();
}
