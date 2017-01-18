/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGeoAssignCoordinates.h

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
 * @class   vtkGeoAssignCoordinates
 * @brief   Given latitude and longitude arrays,
 * take the values in those arrays and convert them to x,y,z world coordinates.
 *
 *
 * Givem latitude and longitude arrays,
 * take the values in those arrays and convert them to x,y,z world coordinates.
 * Uses a spherical model of the earth to do the conversion.
 * The position is in meters relative to the center of the earth.
 *
 * If a transform is given, use the transform to convert latitude and longitude
 * to the world coordinate.
*/

#ifndef vtkGeoAssignCoordinates_h
#define vtkGeoAssignCoordinates_h

#include "vtkGeovisCoreModule.h" // For export macro
#include "vtkPassInputTypeAlgorithm.h"

class vtkAbstractTransform;

class VTKGEOVISCORE_EXPORT vtkGeoAssignCoordinates : public vtkPassInputTypeAlgorithm
{
public:
  static vtkGeoAssignCoordinates *New();

  vtkTypeMacro(vtkGeoAssignCoordinates, vtkPassInputTypeAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * Set the longitude coordinate array name.
   */
  vtkSetStringMacro(LongitudeArrayName);
  vtkGetStringMacro(LongitudeArrayName);
  //@}

  //@{
  /**
   * Set the latitude coordinate array name.
   */
  vtkSetStringMacro(LatitudeArrayName);
  vtkGetStringMacro(LatitudeArrayName);
  //@}

  //@{
  /**
   * The base radius to use in GLOBAL mode.
   * Default is the earth's radius.
   */
  vtkSetMacro(GlobeRadius, double);
  vtkGetMacro(GlobeRadius, double);
  //@}

  //@{
  /**
   * The transform to use to convert coordinates of the form
   * (lat, long, 0) to (x, y z). If this is NULL (the default),
   * use GlobeRadius to perform a spherical embedding.
   */
  virtual void SetTransform(vtkAbstractTransform* trans);
  vtkGetObjectMacro(Transform, vtkAbstractTransform);
  //@}

  //@{
  /**
   * If on, uses LatitudeArrayName and LongitudeArrayName to
   * move values in data arrays into the points of the data set.
   * Turn off if the latitude and longitude are already in
   * the points.
   */
  vtkSetMacro(CoordinatesInArrays, bool);
  vtkGetMacro(CoordinatesInArrays, bool);
  vtkBooleanMacro(CoordinatesInArrays, bool);
  //@}

protected:
  vtkGeoAssignCoordinates();
  ~vtkGeoAssignCoordinates() VTK_OVERRIDE;

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *) VTK_OVERRIDE;
  int FillInputPortInformation(int port, vtkInformation* info) VTK_OVERRIDE;

private:

  char* LongitudeArrayName;
  char* LatitudeArrayName;
  double GlobeRadius;
  bool CoordinatesInArrays;
  vtkAbstractTransform* Transform;

  vtkGeoAssignCoordinates(const vtkGeoAssignCoordinates&) VTK_DELETE_FUNCTION;
  void operator=(const vtkGeoAssignCoordinates&) VTK_DELETE_FUNCTION;
};

#endif

