#ifndef VTKGEOJSONFEATURE_H
#define VTKGEOJSONFEATURE_H

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

    //Extract the geometry and properties corresponding to the geoJSON feature stored at root
    vtkPolyData *extractGeoJSONFeature(Json::Value root, vtkPolyData *outputData);

    //Return the vtkPolyData corresponding to the geoJSON feature stord in featureRoot
    vtkPolyData *GetOutput();

protected:
    //vtkPolyData containing the polydata generated from the geoJSON feature
    vtkPolyData *outputData;

    //Json::Value featureRoot corresponds to the root of the geoJSON feature from which the geometry and properties are to be extracted
    Json::Value featureRoot;

    //Extract geoJSON geometry into vtkPolyData *
    vtkPolyData *extractGeoJSONFeatureGeometry(Json::Value root, vtkPolyData *outputData);

    //Extract geoJSON Point into vtkPolyData *
    vtkPolyData *extractPoint(Json::Value coordinates, vtkPolyData *outputData);

    //Extract geoJSON MultiPoint into vtkPolyData *
    vtkPolyData *extractMultiPoint(Json::Value coordinates, vtkPolyData *outputData);

    //Extract geoJSON Line String into vtkPolyData *
    vtkPolyData *extractLineString(Json::Value coordinates, vtkPolyData *outputData);

    //Extract geoJSON Multi Line String into vtkPolyData *
    vtkPolyData *extractMultiLineString(Json::Value coordinateArray, vtkPolyData *outputData);

    //Extract geoJSON Polygon into vtkPolyData *
    vtkPolyData *extractPolygon(Json::Value coordinate, vtkPolyData *outputData);

    //Extract geoJSON Multi Polygon into vtkPolyData *
    vtkPolyData *extractMultiPolygon(Json::Value coordinateArray, vtkPolyData *outputData);

    //Point[] from its JSON equivalent
    double *createPoint(Json::Value coordinates);

    //Case insensitive string comparison
    bool isEqual(vtkStdString str1, vtkStdString str2);

    //Check if the root contains data corresponding to a point
    bool isPoint(Json::Value root);

    //Check if the root contains data corresponding to Multi Point
    bool isMultiPoint(Json::Value root);

    //To Do.
    //Check if the root contains data corresponding to a Line String
    bool isLineString(Json::Value root);

    //To Do.
    //Check if the root contains data corresponding to Multi Line String
    bool isMultiLineString(Json::Value root);

    //To Do.
    //Check if the root contains data corresponding to a Polygon
    bool isPolygon(Json::Value root);

    //To Do.
    //Check if the root contains data corresponding to Multi Polygon
    bool isMultiPolygon(Json::Value root);
};

#endif // VTKGEOJSONFEATURE_H
