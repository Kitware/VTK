/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGeoGraticule.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
// .NAME vtkGeoGraticule - Create a polygonal lat-long grid
//
// .SECTION Description
// This filter generates polydata to illustrate the distortions introduced
// by a map projection. The level parameter specifies the number of lines
// to be drawn. Poles are treated differently than other regions; hence the
// use of a Level parameter instead of a NumberOfLines parameter.
// The latitude and longitude are specified as half-open intervals with units
// of degrees. By default the latitude bounds are [-90,90[ and the longitude
// bounds are [0,180[.

#ifndef __vtkGeoGraticule_h
#define __vtkGeoGraticule_h

#include "vtkPolyDataAlgorithm.h"

class vtkPolyData;

class VTK_GEOVIS_EXPORT vtkGeoGraticule : public vtkPolyDataAlgorithm
{
public:
  static vtkGeoGraticule* New();
  virtual void PrintSelf( ostream& os, vtkIndent indent );
  vtkTypeMacro(vtkGeoGraticule,vtkPolyDataAlgorithm);

  // Description:
  // The latitude bounds of the graticule.
  vtkSetVector2Macro(LatitudeBounds,double);
  vtkGetVector2Macro(LatitudeBounds,double);

  // Description:
  // The longitude bounds of the graticule.
  vtkSetVector2Macro(LongitudeBounds,double);
  vtkGetVector2Macro(LongitudeBounds,double);

  //BTX
  enum LevelLimits {
    LEVEL_MIN = 0,
    LEVEL_MAX = 11,
    NUMBER_OF_LEVELS = ( LEVEL_MAX - LEVEL_MIN + 1 )
  };
  //ETX

  // Description:
  // The frequency level of latitude lines.
  vtkSetClampMacro(LatitudeLevel,int,LEVEL_MIN,LEVEL_MAX);
  vtkGetMacro(LatitudeLevel,int);

  // Description:
  // The frequency level of longitude lines.
  vtkSetClampMacro(LongitudeLevel,int,LEVEL_MIN,LEVEL_MAX);
  vtkGetMacro(LongitudeLevel,int);

  // Description:
  // The latitude delta at a certain frequency level.
  static double GetLatitudeDelta(int level)
    { return LatitudeLevelTics[level]; }

  // Description:
  // The longitude delta at a certain frequency level.
  static double GetLongitudeDelta(int level)
    { return LongitudeLevelTics[level]; }

  // Description:
  // Set//get the type(s) of cells that will be
  // output by the filter. By default, polylines
  // are output. You may also request quadrilaterals. 
  // This is a bit vector of GeometryType enums.
  vtkSetMacro(GeometryType,int);
  vtkGetMacro(GeometryType,int);

  //BTX
  enum GeometryType {
    POLYLINES      = 0x1,
    QUADRILATERALS = 0x2
  };
  //ETX

protected:
  vtkGeoGraticule();
  virtual ~vtkGeoGraticule();

  int GeometryType;
  double LatitudeBounds[2];
  double LongitudeBounds[2];
  int LatitudeLevel;
  int LongitudeLevel;

  // Description:
  // The distance between tic marks at each level, in degrees.
  static double LatitudeLevelTics[NUMBER_OF_LEVELS];
  static double LongitudeLevelTics[NUMBER_OF_LEVELS];

  virtual int RequestData( vtkInformation*, vtkInformationVector**, vtkInformationVector* );

  void GenerateGraticule( vtkPolyData* output, double latbds[2], double lngbds[2] );
  int ComputeLineLevel( int ticId, int baseLevel, const double* levelIncrements );

private:
  vtkGeoGraticule( const vtkGeoGraticule& ); // Not implemented.
  void operator = ( const vtkGeoGraticule& ); // Not implemented.
};

#endif // __vtkGeoGraticule_h
