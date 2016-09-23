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
/**
 * @class   vtkGeoJSONFeature
 * @brief   Represents GeoJSON feature geometry & properties
 *
 * This class is used by the vtkGeoJSONReader when parsing GeoJSON input.
 * It is not intended to be instantiated by applications directly.
*/

#ifndef vtkGeoJSONFeature_h
#define vtkGeoJSONFeature_h

// VTK Includes
#include "vtkIOGeoJSONModule.h" // For export macro
#include "vtkDataObject.h"
#include "vtk_jsoncpp.h" // For json parser

class vtkPolyData;

// Currently implemented geoJSON compatible Geometries
#define GeoJSON_POINT                   "Point"
#define GeoJSON_MULTI_POINT             "MultiPoint"
#define GeoJSON_LINE_STRING             "LineString"
#define GeoJSON_MULTI_LINE_STRING       "MultiLineString"
#define GeoJSON_POLYGON                 "Polygon"
#define GeoJSON_MULTI_POLYGON           "MultiPolygon"
#define GeoJSON_GEOMETRY_COLLECTION     "GeometryCollection"

class VTKIOGEOJSON_EXPORT vtkGeoJSONFeature : public vtkDataObject
{
public:
  static vtkGeoJSONFeature *New();
  virtual void PrintSelf(ostream &os, vtkIndent indent);
  vtkTypeMacro(vtkGeoJSONFeature,vtkDataObject);

  //@{
  /**
   * Set/get option to generate the border outlining each polygon,
   * so that resulting cells are vtkPolyLine instead of vtkPolygon.
   * The default is off
   */
  vtkSetMacro(OutlinePolygons, bool);
  vtkGetMacro(OutlinePolygons, bool);
  vtkBooleanMacro(OutlinePolygons, bool);
  //@}

  /**
   * Extract the geometry corresponding to the geoJSON feature stored at root
   * Assign any feature properties passed as cell data
   */
  void ExtractGeoJSONFeature(const Json::Value& root, vtkPolyData *outputData);

protected:
  vtkGeoJSONFeature();
  ~vtkGeoJSONFeature();

  /**
   * Json::Value featureRoot corresponds to the root of the geoJSON feature
   * from which the geometry and properties are to be extracted
   */
  Json::Value featureRoot;

  /**
   * Id of current GeoJSON feature being parsed
   */
  char *FeatureId;

  /**
   * Set/get option to generate the border outlining each polygon,
   * so that the output cells are polyine data.
   */
  bool OutlinePolygons;

  /**
   * Extract geoJSON geometry into vtkPolyData *
   */
  void ExtractGeoJSONFeatureGeometry(const Json::Value& root,
                                     vtkPolyData *outputData);

  //@{
  /**
   * In extractXXXX() Extract geoJSON geometries XXXX into outputData
   */
  vtkPolyData *ExtractPoint(const Json::Value& coordinates,
                            vtkPolyData *outputData);
  vtkPolyData *ExtractLineString(const Json::Value& coordinates,
                                 vtkPolyData *outputData);
  vtkPolyData *ExtractPolygon(const Json::Value& coordinates,
                              vtkPolyData *outputData);
  //@}

  //@{
  /**
   * extractMultiXXXX extracts an array of geometries XXXX into the outputData
   */
  vtkPolyData *ExtractMultiPoint(const Json::Value& coordinates,
                                 vtkPolyData *outputData);
  vtkPolyData *ExtractMultiLineString(const Json::Value& coordinates,
                                      vtkPolyData *outputData);
  vtkPolyData *ExtractMultiPolygon(const Json::Value& coordinates,
                                   vtkPolyData *outputData);
  //@}

  //@{
  /**
   * Check if the root contains corresponding appropriate geometry in the
   * Jsoncpp root
   */
  bool IsPoint(const Json::Value& root);
  bool IsMultiPoint(const Json::Value& root);
  bool IsLineString(const Json::Value& root);  //To Do.
  bool IsMultiLineString(const Json::Value& root); //To Do.
  bool IsPolygon(const Json::Value& root); //To Do.
  bool IsMultiPolygon(const Json::Value& root);  //To Do.
  //@}

  /**
   * Point[] from its JSON equivalent
   */
  bool CreatePoint(const Json::Value& coordinates, double point[3]);

  void InsertFeatureProperties(vtkPolyData *outputData);

private:
  vtkGeoJSONFeature(const vtkGeoJSONFeature&) VTK_DELETE_FUNCTION;
  void operator=(const vtkGeoJSONFeature&) VTK_DELETE_FUNCTION;
};

#endif // vtkGeoJSONFeature_h
