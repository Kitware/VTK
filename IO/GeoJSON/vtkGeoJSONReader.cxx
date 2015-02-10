/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGeoJSONReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkGeoJSONReader.h"
#include "vtkGeoJSONFeature.h"

// VTK Includes
#include "vtkAbstractArray.h"
#include "vtkBitArray.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkDoubleArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtk_jsoncpp.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPolyData.h"
#include "vtkStringArray.h"
#include "vtkTriangleFilter.h"

// C++ includes
#include <fstream>
#include <iostream>

vtkStandardNewMacro(vtkGeoJSONReader);

//----------------------------------------------------------------------------
class vtkGeoJSONReader::GeoJSONReaderInternal
{
public:
  // List of property names to read. Property value is used the default
  std::vector<vtkGeoJSONProperty> PropertySpecs;

  // Parse the Json Value corresponding to the root of the geoJSON data from the file
  void ParseRoot(const Json::Value& root, vtkPolyData *output, bool outlinePolygons);

  // Verify if file exists and can be read by the parser
  // If exists, parse into Jsoncpp data structure
  int CanParseFile(const char *filename, Json::Value &root);

  // Verify if string can be read by the parser
  // If exists, parse into Jsoncpp data structure
  int CanParseString(char *input, Json::Value &root);

  // Extract property values from json node
  void ParseFeatureProperties(const Json::Value& propertiesNode,
    std::vector<vtkGeoJSONProperty>& properties);
};

//----------------------------------------------------------------------------
void vtkGeoJSONReader::
GeoJSONReaderInternal::ParseRoot(const Json::Value& root, vtkPolyData *output,
                                 bool outlinePolygons)
{
  // Initialize geometry containers
  vtkNew<vtkPoints> points;
  points->SetDataTypeToDouble();
  output->SetPoints(points.GetPointer());
  vtkNew<vtkCellArray> verts;
  output->SetVerts(verts.GetPointer());
  vtkNew<vtkCellArray> lines;
  output->SetLines(lines.GetPointer());
  vtkNew<vtkCellArray> polys;
  output->SetPolys(polys.GetPointer());

  // Initialize feature-id array
  vtkStringArray *featureIdArray = vtkStringArray::New();
  featureIdArray->SetName("feature-id");
  output->GetCellData()->AddArray(featureIdArray);
  featureIdArray->Delete();

  // Initialize properties arrays
  vtkAbstractArray *array;
  std::vector<vtkGeoJSONProperty>::iterator iter =
    this->PropertySpecs.begin();
  for (; iter != this->PropertySpecs.end(); iter++)
    {
    array = NULL;
    switch (iter->Value.GetType())
      {
      case VTK_BIT:
        array = vtkBitArray::New();
        break;

      case VTK_INT:
        array = vtkIntArray::New();
        break;

      case VTK_DOUBLE:
        array = vtkDoubleArray::New();
        break;

      case VTK_STRING:
        array = vtkStringArray::New();
        break;

      default:
        vtkGenericWarningMacro("unexpected data type " << iter->Value.GetType());
        break;
      }

    // Skip if array not created for some reason
    if (!array)
      {
      continue;
      }

    array->SetName(iter->Name.c_str());
    output->GetCellData()->AddArray(array);
    array->Delete();
    }

  // Check type
  Json::Value rootType = root["type"];
  if (rootType.isNull())
    {
    vtkGenericWarningMacro(<<"ParseRoot: Missing type node");
    return;
    }

  // Parse features
  Json::Value rootFeatures;
  std::string strRootType = rootType.asString();
  std::vector<vtkGeoJSONProperty> properties;
  if ("FeatureCollection" == strRootType)
    {
    rootFeatures = root["features"];
    if (rootFeatures.isNull())
      {
      vtkGenericWarningMacro(<<"ParseRoot: Missing \"features\" node")
      return;
      }

    if (!rootFeatures.isArray())
      {
      vtkGenericWarningMacro(<< "ParseRoot: features node is not an array");
      return;
      }

    vtkGeoJSONProperty property;
    for (int i = 0; i < rootFeatures.size(); i++)
      {
      // Append extracted geometry to existing outputData
      Json::Value featureNode = rootFeatures[i];
      Json::Value propertiesNode = featureNode["properties"];
      this->ParseFeatureProperties(propertiesNode, properties);
      vtkNew<vtkGeoJSONFeature> feature;
      feature->SetOutlinePolygons(outlinePolygons);
      feature->SetFeatureProperties(properties);
      feature->ExtractGeoJSONFeature(featureNode, output);
      }
    }
  else if ("Feature" == strRootType)
    {
    // Process single feature
    this->ParseFeatureProperties(root, properties);
    vtkNew<vtkGeoJSONFeature> feature;
    feature->SetOutlinePolygons(outlinePolygons);
    feature->SetFeatureProperties(properties);
    feature->ExtractGeoJSONFeature(root, output);
    }
  else
    {
    vtkGenericWarningMacro(<< "ParseRoot: do not support root type \""
                           << strRootType << "\"");
    }
}

//----------------------------------------------------------------------------
int vtkGeoJSONReader::
GeoJSONReaderInternal::CanParseFile(const char *filename, Json::Value &root)
{
  if (!filename)
    {
    vtkGenericWarningMacro(<< "Input filename not specified");
    return VTK_ERROR;
    }

  ifstream file;
  file.open( filename );

  if ( ! file.is_open() )
    {
    vtkGenericWarningMacro(<< "Unable to Open File " << filename);
    return VTK_ERROR;
    }

  Json::Reader reader;

  //parse the entire geoJSON data into the Json::Value root
  bool parsedSuccess = reader.parse(file, root, false);

  if ( ! parsedSuccess )
    {
    // Report failures and their locations in the document
    vtkGenericWarningMacro(<<"Failed to parse JSON" << endl
                           << reader.getFormatedErrorMessages());
    return VTK_ERROR;
    }

  return VTK_OK;
}

//----------------------------------------------------------------------------
int vtkGeoJSONReader::
GeoJSONReaderInternal::CanParseString(char *input, Json::Value &root)
{
  if (!input)
    {
    vtkGenericWarningMacro(<< "Input string is empty");
    return VTK_ERROR;
    }

  Json::Reader reader;

  //parse the entire geoJSON data into the Json::Value root
  bool parsedSuccess = reader.parse(input, root, false);

  if ( ! parsedSuccess )
    {
    // Report failures and their locations in the document
    vtkGenericWarningMacro(<<"Failed to parse JSON" << endl
                           << reader.getFormatedErrorMessages());
    return VTK_ERROR;
    }

  return VTK_OK;
}

//----------------------------------------------------------------------------
void vtkGeoJSONReader::
GeoJSONReaderInternal::ParseFeatureProperties(const Json::Value& propertiesNode,
     std::vector<vtkGeoJSONProperty>& featureProperties)
{
  featureProperties.clear();

  vtkGeoJSONProperty spec;
  vtkGeoJSONProperty property;
  std::vector<vtkGeoJSONProperty>::iterator iter =
    this->PropertySpecs.begin();
  for (; iter != this->PropertySpecs.end(); iter++)
    {
    spec = *iter;
    property.Name = spec.Name;

    Json::Value propertyNode = propertiesNode[spec.Name];
    if (propertyNode.isNull())
      {
      property.Value = spec.Value;
      featureProperties.push_back(property);
      continue;
      }

    // (else)
    switch (spec.Value.GetType())
      {
      case VTK_BIT:
        property.Value = vtkVariant(propertyNode.asBool());
        break;

      case VTK_DOUBLE:
        property.Value = vtkVariant(propertyNode.asDouble());
        break;

      case VTK_INT:
        property.Value = vtkVariant(propertyNode.asInt());
        break;

      case VTK_STRING:
        property.Value = vtkVariant(propertyNode.asString());
        break;
      }

    featureProperties.push_back(property);
    }
}


//----------------------------------------------------------------------------
vtkGeoJSONReader::vtkGeoJSONReader()
{
  this->FileName = NULL;
  this->StringInput = NULL;
  this->StringInputMode = false;
  this->TriangulatePolygons = false;
  this->OutlinePolygons = false;
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
  this->Internal = new GeoJSONReaderInternal;
}

//----------------------------------------------------------------------------
vtkGeoJSONReader::~vtkGeoJSONReader()
{
  delete[] FileName;
  delete[] StringInput;
  delete Internal;
}

//----------------------------------------------------------------------------
void vtkGeoJSONReader::
AddFeatureProperty(char *name, vtkVariant& typeAndDefaultValue)
{
  vtkGeoJSONProperty property;

  // Traverse internal list checking if name already used
  std::vector<vtkGeoJSONProperty>::iterator iter =
    this->Internal->PropertySpecs.begin();
  for (; iter != this->Internal->PropertySpecs.end(); iter++)
    {
    if (iter->Name == name)
      {
      vtkGenericWarningMacro(<< "Overwriting property spec for name " << name);
      property.Name = name;
      property.Value = typeAndDefaultValue;
      *iter = property;
      break;
      }
    }

  // If not found, add to list
  if (iter == this->Internal->PropertySpecs.end())
    {
    property.Name = name;
    property.Value = typeAndDefaultValue;
    this->Internal->PropertySpecs.push_back(property);
    vtkDebugMacro(<< "Added feature property " << property.Name);
    }
}

//----------------------------------------------------------------------------
int vtkGeoJSONReader::RequestData(vtkInformation* vtkNotUsed(request),
                                   vtkInformationVector** vtkNotUsed(request),
                                   vtkInformationVector* outputVector)
{
  // Get the info object
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // Get the ouptut
  vtkPolyData* output = vtkPolyData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // Parse either string input of file, depeding on mode
  Json::Value root;
  int parseResult = 0;
  if (this->StringInputMode)
    {
    parseResult = this->Internal->CanParseString(this->StringInput, root);
    }
  else
    {
    parseResult = this->Internal->CanParseFile(this->FileName, root);
    }

  if (parseResult != VTK_OK)
    {
    return VTK_ERROR;
    }

  // If parsed successfully into Json, then convert it
  // into appropriate vtkPolyData
  if (root.isObject())
    {
    this->Internal->ParseRoot(root, output, this->OutlinePolygons);

    // Convert Concave Polygons to convex polygons using triangulation
    if (output->GetNumberOfPolys() && this->TriangulatePolygons)
      {
      vtkNew<vtkTriangleFilter> filter;
      filter->SetInputData(output);
      filter->Update();

      output->ShallowCopy(filter->GetOutput());
      }
    }
  return VTK_OK;
}

//----------------------------------------------------------------------------
void vtkGeoJSONReader::PrintSelf(ostream &os, vtkIndent indent)
{
  Superclass::PrintSelf(os, indent);
  os << "vtkGeoJSONReader" << std::endl;
  os << "Filename: " << this->FileName << std::endl;
}
