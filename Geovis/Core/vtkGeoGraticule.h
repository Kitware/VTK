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
/**
 * @class   vtkGeoGraticule
 * @brief   Create a polygonal lat-long grid
 *
 *
 * This filter generates polydata to illustrate the distortions introduced
 * by a map projection. The level parameter specifies the number of lines
 * to be drawn. Poles are treated differently than other regions; hence the
 * use of a Level parameter instead of a NumberOfLines parameter.
 * The latitude and longitude are specified as half-open intervals with units
 * of degrees. By default the latitude bounds are [-90,90[ and the longitude
 * bounds are [0,180[.
*/

#ifndef vtkGeoGraticule_h
#define vtkGeoGraticule_h

#include "vtkGeovisCoreModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

class vtkPolyData;

class VTKGEOVISCORE_EXPORT vtkGeoGraticule : public vtkPolyDataAlgorithm
{
public:
  static vtkGeoGraticule* New();
  void PrintSelf( ostream& os, vtkIndent indent ) VTK_OVERRIDE;
  vtkTypeMacro(vtkGeoGraticule,vtkPolyDataAlgorithm);

  //@{
  /**
   * The latitude bounds of the graticule.
   */
  vtkSetVector2Macro(LatitudeBounds,double);
  vtkGetVector2Macro(LatitudeBounds,double);
  //@}

  //@{
  /**
   * The longitude bounds of the graticule.
   */
  vtkSetVector2Macro(LongitudeBounds,double);
  vtkGetVector2Macro(LongitudeBounds,double);
  //@}

  enum LevelLimits {
    LEVEL_MIN = 0,
    LEVEL_MAX = 11,
    NUMBER_OF_LEVELS = ( LEVEL_MAX - LEVEL_MIN + 1 )
  };

  //@{
  /**
   * The frequency level of latitude lines.
   */
  vtkSetClampMacro(LatitudeLevel,int,LEVEL_MIN,LEVEL_MAX);
  vtkGetMacro(LatitudeLevel,int);
  //@}

  //@{
  /**
   * The frequency level of longitude lines.
   */
  vtkSetClampMacro(LongitudeLevel,int,LEVEL_MIN,LEVEL_MAX);
  vtkGetMacro(LongitudeLevel,int);
  //@}

  /**
   * The latitude delta at a certain frequency level.
   */
  static double GetLatitudeDelta(int level)
    { return LatitudeLevelTics[level]; }

  /**
   * The longitude delta at a certain frequency level.
   */
  static double GetLongitudeDelta(int level)
    { return LongitudeLevelTics[level]; }

  //@{
  /**
   * Set//get the type(s) of cells that will be
   * output by the filter. By default, polylines
   * are output. You may also request quadrilaterals.
   * This is a bit vector of GeometryType enums.
   */
  vtkSetMacro(GeometryType,int);
  vtkGetMacro(GeometryType,int);
  //@}

  enum GeometryType {
    POLYLINES      = 0x1,
    QUADRILATERALS = 0x2
  };

protected:
  vtkGeoGraticule();
  ~vtkGeoGraticule() VTK_OVERRIDE;

  int GeometryType;
  double LatitudeBounds[2];
  double LongitudeBounds[2];
  int LatitudeLevel;
  int LongitudeLevel;

  //@{
  /**
   * The distance between tic marks at each level, in degrees.
   */
  static double LatitudeLevelTics[NUMBER_OF_LEVELS];
  static double LongitudeLevelTics[NUMBER_OF_LEVELS];
  //@}

  int RequestData( vtkInformation*, vtkInformationVector**, vtkInformationVector* ) VTK_OVERRIDE;

  void GenerateGraticule( vtkPolyData* output, double latbds[2], double lngbds[2] );
  int ComputeLineLevel( int ticId, int baseLevel, const double* levelIncrements );

private:
  vtkGeoGraticule( const vtkGeoGraticule& ) VTK_DELETE_FUNCTION;
  void operator = ( const vtkGeoGraticule& ) VTK_DELETE_FUNCTION;
};

#endif // vtkGeoGraticule_h
