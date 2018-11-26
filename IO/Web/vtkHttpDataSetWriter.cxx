/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHttpDataSetWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkHttpDataSetWriter.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"

#include "vtkDataArrayHelper.h"
#include "vtkDataSet.h"
#include "vtkDataSetAttributes.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"

#include <fstream>
#include <sstream>
#include <string>

vtkStandardNewMacro(vtkHttpDataSetWriter);

// ----------------------------------------------------------------------------

vtkHttpDataSetWriter::vtkHttpDataSetWriter()
{
  this->FileName = nullptr;
}

// ----------------------------------------------------------------------------

vtkHttpDataSetWriter::~vtkHttpDataSetWriter()
{
  delete[] this->FileName;
}

// ----------------------------------------------------------------------------

vtkDataSet* vtkHttpDataSetWriter::GetInput()
{
  return vtkDataSet::SafeDownCast(this->Superclass::GetInput());
}

// ----------------------------------------------------------------------------

vtkDataSet* vtkHttpDataSetWriter::GetInput(int port)
{
  return vtkDataSet::SafeDownCast(this->Superclass::GetInput(port));
}

// ----------------------------------------------------------------------------

std::string vtkHttpDataSetWriter::WriteDataSetAttributes(
  vtkDataSetAttributes* fields, const char* className)
{
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
    if (idx)
    {
      jsonSnippet << ",\n";
    }
    vtkDataArray* field = fields->GetArray(idx);
    jsonSnippet << "      { \"data\": " << this->WriteArray(field, "vtkDataArray") << "}";

    // Update active field if any
    activeTCoords = field == fields->GetTCoords() ? idx : activeTCoords;
    activeScalars = field == fields->GetScalars() ? idx : activeScalars;
    activeNormals = field == fields->GetNormals() ? idx : activeNormals;
    activeGlobalIds = field == fields->GetGlobalIds() ? idx : activeGlobalIds;
    activeTensors = field == fields->GetTensors() ? idx : activeTensors;
    activePedigreeIds = field == fields->GetPedigreeIds() ? idx : activePedigreeIds;
    activeVectors = field == fields->GetVectors() ? idx : activeVectors;
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

std::string vtkHttpDataSetWriter::WriteArray(
  vtkDataArray* array, const char* className, const char* arrayName)
{
  bool needConvert;
  std::string id = vtkDataArrayHelper::GetUID(array, needConvert);
  std::stringstream arrayPath;
  arrayPath << this->FileName << "/data/" << id.c_str();
  bool success = vtkDataArrayHelper::WriteArray(array, arrayPath.str().c_str());

  if (!success)
  {
    return "{}";
  }

  const char* INDENT = "    ";
  std::stringstream ss;
  ss << "{\n"
     << INDENT << "  \"vtkClass\": \"" << className << "\",\n"
     << INDENT << "  \"name\": \"" << (arrayName == nullptr ? array->GetName() : arrayName) << "\",\n"
     << INDENT << "  \"numberOfComponents\": " << array->GetNumberOfComponents() << ",\n"
     << INDENT << "  \"dataType\": \"" << vtkDataArrayHelper::GetShortType(array, needConvert) << "Array\",\n"
     << INDENT << "  \"ref\": {\n"
     << INDENT << "     \"encode\": \"LittleEndian\",\n"
     << INDENT << "     \"basepath\": \"data\",\n"
     << INDENT << "     \"id\": \"" << id.c_str() << "\"\n"
     << INDENT << "  },\n"
     << INDENT << "  \"size\": " << array->GetNumberOfValues() << "\n"
     << INDENT << "}";

  return ss.str();
}

// ----------------------------------------------------------------------------

void vtkHttpDataSetWriter::WriteData()
{
  vtkDataSet* ds = this->GetInput();
  vtkImageData* imageData = vtkImageData::SafeDownCast(ds);
  vtkPolyData* polyData = vtkPolyData::SafeDownCast(ds);
  this->ValidDataSet = false;

  // Get input and check data
  if (ds == nullptr)
  {
    vtkErrorMacro(<< "No data to write!");
    return;
  }

  if (this->FileName == nullptr)
  {
    vtkErrorMacro(<< "Please specify FileName to write");
    return;
  }

  // Capture vtkDataSet definition
  std::stringstream metaJsonFile;
  metaJsonFile << "{\n";
  metaJsonFile << "  \"vtkClass\": \"" << ds->GetClassName() << "\"";

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
    vtkDataArray* cells = polyData->GetVerts()->GetData();
    if (cells->GetNumberOfValues())
    {
      metaJsonFile << ",\n  \"verts\": "
                   << this->WriteArray(cells, "vtkCellArray", "verts").c_str();
    }

    // Lines
    cells = polyData->GetLines()->GetData();
    if (cells->GetNumberOfValues())
    {
      metaJsonFile << ",\n  \"lines\": "
                   << this->WriteArray(cells, "vtkCellArray", "lines").c_str();
    }

    // Strips
    cells = polyData->GetStrips()->GetData();
    if (cells->GetNumberOfValues())
    {
      metaJsonFile << ",\n  \"strips\": "
                   << this->WriteArray(cells, "vtkCellArray", "strips").c_str();
    }

    // Polys
    cells = polyData->GetPolys()->GetData();
    if (cells->GetNumberOfValues())
    {
      metaJsonFile << ",\n  \"polys\": "
                   << this->WriteArray(cells, "vtkCellArray", "polys").c_str();
    }
  }

  // PointData
  std::string fieldJSON = this->WriteDataSetAttributes(ds->GetPointData(), "pointData");
  if (!fieldJSON.empty())
  {
    metaJsonFile << ",\n" << fieldJSON.c_str();
  }

  // CellData
  fieldJSON = this->WriteDataSetAttributes(ds->GetCellData(), "cellData");
  if (!fieldJSON.empty())
  {
    metaJsonFile << ",\n" << fieldJSON.c_str();
  }

  metaJsonFile << "}\n";

  // Write meta-data file
  std::stringstream scenePath;
  scenePath << this->FileName << "/index.json";

  ofstream file;
  file.open(scenePath.str().c_str(), ios::out);
  file << metaJsonFile.str().c_str();
  file.close();
}

// ----------------------------------------------------------------------------

void vtkHttpDataSetWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

// ----------------------------------------------------------------------------

int vtkHttpDataSetWriter::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  return 1;
}
