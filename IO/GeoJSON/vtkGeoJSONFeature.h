/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGeoJSONFeature.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkGeoJSONFeature - Convert GeoJSON objects/Geometries into vtkPolyData
// .SECTION Description
// Outputs a vtkPolyData containing geometry from the input
// Geo JSON data (http://www.geojson.org)

#ifndef __vtkGeoJSONFeature_h
#define __vtkGeoJSONFeature_h

// VTK Includes
#include "vtkDataObject.h"
#include "vtkGeoJSONProperty.h"
#include "vtk_jsoncpp.h" // For json parser
#include <string>
#include <vector>

class vtkPolyData;

// Currently implemented geoJSON compatible Geometries
#define GeoJSON_POINT                   "Point"
#define GeoJSON_MULTI_POINT             "MultiPoint"
#define GeoJSON_LINE_STRING             "LineString"
#define GeoJSON_MULTI_LINE_STRING       "MultiLineString"
#define GeoJSON_POLYGON                 "Polygon"
#define GeoJSON_MULTI_POLYGON           "MultiPolygon"
#define GeoJSON_GEOMETRY_COLLECTION     "GeometryCollection"

class vtkGeoJSONFeature : public vtkDataObject
{
public:
  static vtkGeoJSONFeature *New();
  virtual void PrintSelf(ostream &os, vtkIndent indent);
  vtkTypeMacro(vtkGeoJSONFeature,vtkDataObject);

  // Description:
  // Extract the geometry corresponding to the geoJSON feature stored at root
  // Assign any feature properties passed as cell data
  void ExtractGeoJSONFeature(Json::Value root, vtkPolyData *outputData);

  void SetFeatureProperties(std::vector<vtkGeoJSONProperty>& properties);
  void GetFeatureProperties(std::vector<vtkGeoJSONProperty>& properties);

protected:
  vtkGeoJSONFeature();
  ~vtkGeoJSONFeature();

  // Description:
  // Json::Value featureRoot corresponds to the root of the geoJSON feature
  // from which the geometry and properties are to be extracted
  Json::Value featureRoot;

  // Description:
  // Id of current GeoJSON feature being parsed
  std::string FeatureId;

  // Description:
  // Properties of current GeoJSON feature being parsed
  std::vector<vtkGeoJSONProperty> FeatureProperties;

  // Description:
  // Extract geoJSON geometry into vtkPolyData *
  void ExtractGeoJSONFeatureGeometry(Json::Value root, vtkPolyData *outputData);

  // Description:
  // In extractXXXX() Extract geoJSON geometries XXXX into outputData
  vtkPolyData *ExtractPoint(Json::Value coordinates, vtkPolyData *outputData);
  vtkPolyData *ExtractLineString(Json::Value coordinates, vtkPolyData *outputData);
  vtkPolyData *ExtractPolygon(Json::Value coordinate, vtkPolyData *outputData);

  // Description:
  // extractMultiXXXX extracts an array of geometries XXXX into the outputData
  vtkPolyData *ExtractMultiPoint(Json::Value coordinates, vtkPolyData *outputData);
  vtkPolyData *ExtractMultiLineString(Json::Value coordinateArray, vtkPolyData *outputData);
  vtkPolyData *ExtractMultiPolygon(Json::Value coordinateArray, vtkPolyData *outputData);

  // Description:
  // Check if the root contains corresponding appropriate geometry in the
  // Jsoncpp root
  bool IsPoint(Json::Value root);
  bool IsMultiPoint(Json::Value root);
  bool IsLineString(Json::Value root);  //To Do.
  bool IsMultiLineString(Json::Value root); //To Do.
  bool IsPolygon(Json::Value root); //To Do.
  bool IsMultiPolygon(Json::Value root);  //To Do.

  // Description:
  // Point[] from its JSON equivalent
  double *CreatePoint(Json::Value coordinates);

  // Description:
  void InsertFeatureProperties(vtkPolyData *outputData);

private:
  vtkGeoJSONFeature(const vtkGeoJSONFeature&);  //Not implemented
  void operator=(const vtkGeoJSONFeature&); //Not implemented
};

#endif // __vtkGeoJSONFeature_h
