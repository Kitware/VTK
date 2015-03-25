/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGeoJSONFeature.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkGeoJSONFeature.h"

// VTK Includes
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkLine.h"
#include "vtkObjectFactory.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkPolygon.h"
#include "vtkStringArray.h"

#include <sstream>

vtkStandardNewMacro(vtkGeoJSONFeature);

//----------------------------------------------------------------------------
vtkGeoJSONFeature::vtkGeoJSONFeature()
{
  this->outputData = NULL;
}

//----------------------------------------------------------------------------
vtkGeoJSONFeature::~vtkGeoJSONFeature()
{
  outputData->Delete();
}

//----------------------------------------------------------------------------
vtkPolyData *vtkGeoJSONFeature::GetOutput()
{
  //Check if output data contains appropriate data and return values accordingly
  return outputData;
}

//----------------------------------------------------------------------------
double *vtkGeoJSONFeature::CreatePoint(Json::Value coordinates)
{
  //Check if Coordinates corresponds to Point
  if ( ! IsPoint( coordinates ) )
    {
    vtkErrorMacro(<< "Wrong data format for a point!");
    return NULL;
    }

  //Do isDouble before asDouble to prevent inconsistency
  //Probably check for float/int too

  //Initialise the 3D coordinates to 0
  double *point = new double[3];
  point[0] = 0;
  point[1] = 0;
  point[2] = 0;

  if ( coordinates.size() == 1 )
    {
    //Update the 3D Coordinates using the 1 Value in the array and rest of the 2 as 0
    Json::Value x = coordinates[0];
    point[0] = x.asDouble();
    }
  else if ( coordinates.size() == 2 )
    {
    //Update the 3D Coordinates using the 2 Values in the array and 3rd as 0
    Json::Value x = coordinates[0];
    Json::Value y = coordinates[1];
    point[0] = x.asDouble();
    point[1] = y.asDouble();
    }
  else if ( coordinates.size() == 3 )
    {
    //Update the 3D Coordinates using the 3 Values in the array
    Json::Value x = coordinates[0];
    Json::Value y = coordinates[1];
    Json::Value z = coordinates[2];
    point[0] = x.asDouble();
    point[1] = y.asDouble();
    point[2] = z.asDouble();
    }

  //Return the 3D point as generated above
  return point;
}

//----------------------------------------------------------------------------
vtkPolyData *vtkGeoJSONFeature::
ExtractPoint(Json::Value coordinates, vtkPolyData *outputData)
{
  //Check if Coordinates corresponds to Single Point
  if ( ! IsPoint( coordinates ) )
    {
    vtkErrorMacro (<< "Wrong data format for a point!");
    return NULL;
    }

  //Obtain point data from Json structure and add to outputData
  double *point = CreatePoint(coordinates);

  const int PID_SIZE = 1;
  vtkIdType pid[ PID_SIZE ];

  vtkPoints *points = outputData->GetPoints();
  pid[0] = points->InsertNextPoint( point );

  vtkCellArray *verts = outputData->GetVerts();
  verts->InsertNextCell( PID_SIZE, pid);

  vtkAbstractArray *array =
    outputData->GetCellData()->GetAbstractArray("feature-id");
  vtkStringArray *ids = vtkStringArray::SafeDownCast(array);
  ids->InsertNextValue(this->FeatureId);

  return outputData;
}

//----------------------------------------------------------------------------
vtkPolyData *vtkGeoJSONFeature::
ExtractMultiPoint(Json::Value coordinates, vtkPolyData *outputData)
{
  //Check if Coordinates corresponds to Multi Points
  if ( ! IsMultiPoint( coordinates ) )
    {
    vtkErrorMacro (<< "Wrong data format for a Multi Point!");
    return NULL;
    }

  if ( coordinates.isArray() )
    {
    vtkPoints *points = outputData->GetPoints();//Contain the locations of the points
    vtkCellArray *verts = outputData->GetVerts();//Contain the indices corresponding to the position of the vertices

    vtkAbstractArray *array =
      outputData->GetCellData()->GetAbstractArray("feature-id");
    vtkStringArray *ids = vtkStringArray::SafeDownCast(array);

    const int PID_SIZE = coordinates.size();
    vtkIdType pid[ PID_SIZE ];

    for (int i = 0; i < PID_SIZE; i++)
      {
      //Parse point from Json object to double array and add it to the points array
      double *point = CreatePoint(coordinates[i]);
      pid[i] = points->InsertNextPoint(point);
      }

    //Update polyData vertices to store multiple points
    verts->InsertNextCell(PID_SIZE, pid);
    ids->InsertNextValue(this->FeatureId);
    }

  return outputData;
}

//----------------------------------------------------------------------------
vtkPolyData *vtkGeoJSONFeature::
ExtractLineString(Json::Value coordinates, vtkPolyData *outputData)
{
  //Check if Coordinates corresponds to Line String
  if ( ! IsLineString( coordinates ) )
    {
    vtkErrorMacro (<< "Wrong data format for a Line String!");
    return NULL;
    }

  vtkCellArray *lines = outputData->GetLines();
  vtkPoints *points = outputData->GetPoints();
  vtkAbstractArray *array =
    outputData->GetCellData()->GetAbstractArray("feature-id");
  vtkStringArray *ids = vtkStringArray::SafeDownCast(array);

  int LINE_COUNT = coordinates.size();

  double *start = CreatePoint(coordinates[0]);

  vtkIdType lineId[2];
  lineId[0] = points->InsertNextPoint(start);

  for (int i = 1; i < LINE_COUNT; i++)
    {
    double *end = CreatePoint(coordinates[i]);

    lineId[1] = points->InsertNextPoint(end);

    vtkLine *line = vtkLine::New();

    line->GetPointIds()->SetId(0, lineId[0]);
    line->GetPointIds()->SetId(1, lineId[1]);

    lines->InsertNextCell(line);
    ids->InsertNextValue(this->FeatureId);

    start = end;
    lineId[0] = lineId[1];
    }

  return outputData;
}

//----------------------------------------------------------------------------
vtkPolyData *vtkGeoJSONFeature::
ExtractMultiLineString(Json::Value coordinateArray, vtkPolyData *outputData)
{
  //Check if Coordinate Array corresponds to Multi Line String
  if ( ! IsMultiLineString( coordinateArray ) )
    {
    vtkErrorMacro(<< "Wrong data format for a Multi Line String!");
    return NULL;
    }

  int LINE_STRING_COUNT = coordinateArray.size();

  for (int i = 0; i < LINE_STRING_COUNT; i++)
    {
    this->ExtractLineString( coordinateArray[i], outputData);
    }

  return outputData;
}

//----------------------------------------------------------------------------
vtkPolyData *vtkGeoJSONFeature::
ExtractPolygon(Json::Value coordinate, vtkPolyData *outputData)
{
  //Check if Coordinate Array corresponds to Polygon
  if ( ! IsPolygon( coordinate ) )
    {
    vtkErrorMacro (<< "Wrong data format for a Polygon!");
    return NULL;
    }

  bool POLYGON_WITH_HOLES = coordinate.size() > 1 ? true : false;

  vtkPoints *points = outputData->GetPoints();
  vtkCellArray *polys = outputData->GetPolys();
  vtkAbstractArray *array =
    outputData->GetCellData()->GetAbstractArray("feature-id");
  vtkStringArray *ids = vtkStringArray::SafeDownCast(array);

  vtkPolygon *exteriorPoly = vtkPolygon::New();

  //For exterior Polygon
  Json::Value exteriorPolygon = coordinate[0];

  int EXTERIOR_POLYGON_VERTEX_COUNT = exteriorPolygon.size() - 1;
  exteriorPoly->GetPointIds()->SetNumberOfIds(EXTERIOR_POLYGON_VERTEX_COUNT);

  for (int i = 0; i < EXTERIOR_POLYGON_VERTEX_COUNT; i++)
    {
    double *point = CreatePoint(exteriorPolygon[i]);
    vtkIdType id = points->InsertNextPoint(point);
    exteriorPoly->GetPointIds()->SetId(i, id);
    }

  polys->InsertNextCell(exteriorPoly);
  ids->InsertNextValue(this->FeatureId);

  if ( ! POLYGON_WITH_HOLES )
    return outputData;

  //Modify polydata to support polygon with holes

  return outputData;
}

//----------------------------------------------------------------------------
vtkPolyData *vtkGeoJSONFeature::
ExtractMultiPolygon(Json::Value coordinateArray, vtkPolyData *outputData)
{
  //Check if Coordinate Array corresponds to Multi Polygon
  if ( ! IsMultiPolygon( coordinateArray ) )
    {
    vtkErrorMacro (<< "Wrong data format for a Multi Polygon!");
    return NULL;
    }

  int POLYGON_COUNT = coordinateArray.size();

  for (int i = 0; i < POLYGON_COUNT; i++)
    {
    //Extract polygon into different polyData and append into a common polyData using the appendPolyData Filter
    this->ExtractPolygon( coordinateArray[i], outputData);
    }

  return outputData;
}

//----------------------------------------------------------------------------
void vtkGeoJSONFeature::ExtractGeoJSONFeature(Json::Value root, vtkPolyData *outputData)
{
  this->featureRoot = root;

  // Check that type is Feature
  Json::Value typeNode = root["type"];
  if (typeNode.isNull() || "Feature" != typeNode.asString())
    {
    vtkErrorMacro (<< "Unknown type. \"Feature\" expected");
    return;
    }

  // Check for geometry node
  Json::Value geometryNode = root["geometry"];
  if (geometryNode.isNull())
    {
    vtkErrorMacro (<< "Missing geometry node");
    return;
    }

  // Check for properties node
  Json::Value propertiesNode = root["properties"];
  if (propertiesNode.isNull())
    {
    vtkErrorMacro (<< "Missing properties node");
    return;
    }

  // Check for feature id
  this->FeatureId = "";
  Json::Value idNode = root["id"];
  // No Json::Value::toString() method, so homebrew one here
  std::stringstream oss;
  switch (idNode.type())
    {
    case Json::nullValue:
      break;

    case Json::intValue:
    case Json::uintValue:
      oss << idNode.asInt();
      this->FeatureId = oss.str();
      break;

    case Json::realValue:
      oss << idNode.asDouble();
      this->FeatureId = oss.str();
      break;

    case Json::stringValue:
      this->FeatureId = idNode.asString();
      break;

    default:
      vtkWarningMacro(<< "Unsupported \"id\" type: " << idNode.type());
      break;
    }

  this->ExtractGeoJSONFeatureGeometry(geometryNode, outputData);
}

//----------------------------------------------------------------------------
void vtkGeoJSONFeature::ExtractGeoJSONFeatureGeometry(Json::Value geometryRoot,
                                                      vtkPolyData *outputData)
{
  // Check for geometry-type node
  Json::Value geometryTypeNode = geometryRoot["type"];
  if (geometryTypeNode.isNull())
    {
    vtkErrorMacro (<< "Missing geometry-type node");
    return;
    }
  if (!geometryTypeNode.isString())
    {
    vtkErrorMacro(<< "Invalid geometry-type node");
    return;
    }

  std::string typeString = geometryTypeNode.asString();
  if (typeString == GeoJSON_GEOMETRY_COLLECTION)
    {
    //For GeometryCollection
    Json::Value geometries = geometryRoot["geometries"];
    for (int i = 0; i < geometries.size(); i++)
      {
      Json::Value child = geometries[i];
      this->ExtractGeoJSONFeatureGeometry(child, outputData);
      }
    return;
    }

  // (else)
  Json::Value coordinates = geometryRoot["coordinates"];
  if (typeString == GeoJSON_POINT)
    {
    this->ExtractPoint(coordinates, outputData);
    }
  else if (typeString == GeoJSON_MULTI_POINT)
    {
    this->ExtractMultiPoint(coordinates, outputData);
    }
  else if (typeString == GeoJSON_LINE_STRING)
    {
    this->ExtractLineString(coordinates, outputData);
    }
  else if (typeString == GeoJSON_MULTI_LINE_STRING)
    {
    this->ExtractMultiLineString(coordinates, outputData);
    }
  else if (typeString == GeoJSON_POLYGON)
    {
    this->ExtractPolygon(coordinates, outputData);
    }
  else if (typeString == GeoJSON_MULTI_POLYGON)
    {
    this->ExtractMultiPolygon(coordinates, outputData);
    }
  else
    {
    vtkErrorMacro (<< "Unknown or unsupported geometry type " << typeString);
    }
}

//----------------------------------------------------------------------------
bool vtkGeoJSONFeature::IsLineString(Json::Value root)
{
  if ( ! root.isArray() )
    {
    vtkErrorMacro (<<"Expected Arrays as input for point at " << root);
    return false;
    }

  if ( root.size() < 1 )
    {
    vtkErrorMacro (<< "Expected atleast 1 value at " << root);
    return false;
    }

  for (int i = 0; i < root.size(); i++)
    {
    Json::Value child = root[i];
    if ( ! IsPoint( child ) )
      {
      return false;
      }
    }

  return true;
}

//----------------------------------------------------------------------------
bool vtkGeoJSONFeature::IsMultiLineString(Json::Value root)
{
  if ( ! root.isArray() )
    {
    vtkErrorMacro (<< "Expected Array as input for point at " << root);
    return false;
    }

  if ( root.size() < 1 )
    {
    vtkErrorMacro (<< "Expected atleast 1 value at " << root);
    return false;
    }

  for (int i = 0; i < root.size(); i++)
    {
    Json::Value child = root[i];
    if ( ! IsLineString( child ) )
      {
      return false;
      }
    }

  return true;
}

//----------------------------------------------------------------------------
bool vtkGeoJSONFeature::IsPoint(Json::Value root)
{
  if ( ! root.isArray() )
    {
    vtkErrorMacro ("Expected Array as input for point at " << root);
    return false;
    }

  if ( ! ( root.size() > 0 && root.size() < 4 ) )
    {
    vtkErrorMacro (<< "Expected 3 or less dimension values at " << root << " for point");
    return false;
    }

  for (int i = 0; i < root.size(); i++)
    {
    Json::Value child = root[i];
    if ( ! child.isNumeric() )
      {
      vtkErrorMacro (<<"Value not Numeric as expected at " << child);
      return false;
      }
    }

  return true;
}

//----------------------------------------------------------------------------
bool vtkGeoJSONFeature::IsMultiPoint(Json::Value root)
{
  if( ! root.isArray() )
    {
    vtkErrorMacro (<< "Expected Array as input for multi point at " << root);
    return false;
    }

  if( root.size() < 1 )
    {
      vtkErrorMacro (<< "Expected atleast 1 value at " << root << " for multipoint");
    return false;
    }

  for (int i = 0; i < root.size(); i++)
    {
    Json::Value child = root[i];
    if ( ! IsPoint( child ) )
      {
      return false;
      }
    }

  return true;
}

//----------------------------------------------------------------------------
bool vtkGeoJSONFeature::IsPolygon(Json::Value root)
{
  if ( ! root.isArray() )
    {
    vtkErrorMacro (<< "Expected Array as input for polygon at " << root);
    return false;
    }

  if ( root.size() < 1 )
    {
    vtkErrorMacro (<< "Expected atleast 1 value at " << root << "for polygon");
    return false;
    }

  for (int i = 0; i < root.size(); i++)
    {
    Json::Value child = root[i];
    if( ! IsLineString( child ) )
      {
      return false;
      }
    //First and last element of child must be same to complete the polygon loop.
    //Check not added.
    }

  return true;
}

//----------------------------------------------------------------------------
bool vtkGeoJSONFeature::IsMultiPolygon(Json::Value root)
{
  if ( ! root.isArray() )
    {
    vtkErrorMacro (<< "Expected Array as input for multi polygon at " << root);
    return false;
    }

  if ( root.size() < 1 )
    {
      vtkErrorMacro (<< "Expected atleast 1 value at " << root << " for multi polygon");
    return false;
    }

  for (int i = 0; i < root.size(); i++)
    {
    Json::Value child = root[i];
    if( ! IsPolygon( child ) )
      {
      return false;
      }
    }

  return true;
}

//----------------------------------------------------------------------------
void vtkGeoJSONFeature::PrintSelf(ostream &os, vtkIndent indent)
{
    Superclass::PrintSelf(os, indent);
    os << indent << "vtkGeoJSONFeature" << std::endl;
    os << indent << "Root: " << this->featureRoot << std::endl;
}
