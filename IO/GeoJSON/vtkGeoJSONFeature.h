#ifndef __vtkGeoJSONFeature_h
#define __vtkGeoJSONFeature_h

//VTK Includes
#include <vtkPolyData.h>
#include <vtkPoints.h>
#include <vtkCellArray.h>
#include <vtkAppendPolyData.h>
#include <vtkCleanPolyData.h>
#include <vtkLine.h>
#include <vtkPolygon.h>

//Parser Include
#include <vtk_jsoncpp.h>

//Currently implemented geoJSON compatible Geometries
#define POINT                   "Point"
#define MULTI_POINT             "MultiPoint"
#define LINE_STRING             "LineString"
#define MULTI_LINE_STRING       "MultiLineString"
#define POLYGON                 "Polygon"
#define MULTI_POLYGON           "MultiPolygon"
#define GEOMETRY_COLLECTION     "GeometryCollection"

class vtkGeoJSONFeature
{
public:
  vtkGeoJSONFeature();

  //Description:
  //Extract the geometry and properties corresponding to the geoJSON feature stored at root
  vtkPolyData *extractGeoJSONFeature(Json::Value root, vtkPolyData *outputData);

  //Description:
  //Return the vtkPolyData corresponding to the geoJSON feature stord in featureRoot
  vtkPolyData *GetOutput();

protected:
  //Description:
  //vtkPolyData containing the polydata generated from the geoJSON feature
  vtkPolyData *outputData;

  //Description:
  //Json::Value featureRoot corresponds to the root of the geoJSON feature from which the geometry and properties are to be extracted
  Json::Value featureRoot;

  //Description:
  //Extract geoJSON geometry into vtkPolyData *
  vtkPolyData *extractGeoJSONFeatureGeometry(Json::Value root, vtkPolyData *outputData);

  //Description:
  //Extract geoJSON Point into vtkPolyData *
  vtkPolyData *extractPoint(Json::Value coordinates, vtkPolyData *outputData);

  //Description:
  //Extract geoJSON MultiPoint into vtkPolyData *
  vtkPolyData *extractMultiPoint(Json::Value coordinates, vtkPolyData *outputData);

  //Description:
  //Extract geoJSON Line String into vtkPolyData *
  vtkPolyData *extractLineString(Json::Value coordinates, vtkPolyData *outputData);

  //Description:
  //Extract geoJSON Multi Line String into vtkPolyData *
  vtkPolyData *extractMultiLineString(Json::Value coordinateArray, vtkPolyData *outputData);

  //Description:
  //Extract geoJSON Polygon into vtkPolyData *
  vtkPolyData *extractPolygon(Json::Value coordinate, vtkPolyData *outputData);

  //Description:
  //Extract geoJSON Multi Polygon into vtkPolyData *
  vtkPolyData *extractMultiPolygon(Json::Value coordinateArray, vtkPolyData *outputData);

  //Description:
  //Point[] from its JSON equivalent
  double *createPoint(Json::Value coordinates);

  //Description:
  //Case insensitive string comparison
  bool isEqual(vtkStdString str1, vtkStdString str2);

  //Description:
  //Check if the root contains data corresponding to a point
  bool isPoint(Json::Value root);

  //Description:
  //Check if the root contains data corresponding to Multi Point
  bool isMultiPoint(Json::Value root);

  //Description:
  //Check if the root contains data corresponding to a Line String
  bool isLineString(Json::Value root);  //To Do.

  //Description:
  //Check if the root contains data corresponding to Multi Line String
  bool isMultiLineString(Json::Value root); //To Do.

  //Description:
  //Check if the root contains data corresponding to a Polygon
  bool isPolygon(Json::Value root); //To Do.

  //Description:
  //To Do.
  //Check if the root contains data corresponding to Multi Polygon
  bool isMultiPolygon(Json::Value root);  //To Do.

private:
  vtkGeoJSONFeature(const vtkGeoJSONFeature&);  //Not implemented
  void operator=(const vtkGeoJSONFeature&); //Not implemented
};

#endif // __vtkGeoJSONFeature_h
