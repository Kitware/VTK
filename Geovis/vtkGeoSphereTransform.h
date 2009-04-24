/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGeoSphereTransform.h

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
// .NAME vtkGeoSphereTransform - A transformation between lat-long-alt and rect coords
// .SECTION Description

#ifndef __vtkGeoSphereTransform_h
#define __vtkGeoSphereTransform_h

#include "vtkAbstractTransform.h"

class vtkGeoProjection;

class VTK_GEOVIS_EXPORT vtkGeoSphereTransform : public vtkAbstractTransform
{
public:
  static vtkGeoSphereTransform* New();
  virtual void PrintSelf( ostream& os, vtkIndent indent );
  vtkTypeRevisionMacro(vtkGeoSphereTransform,vtkAbstractTransform);

  // Description:
  // Invert the transformation.
  virtual void Inverse();

  // Description:
  // This will calculate the transformation without calling Update.
  // Meant for use only within other VTK classes.
  virtual void InternalTransformPoint( const float in[3], float out[3] );
  virtual void InternalTransformPoint( const double in[3], double out[3] );

  // Description:
  // This will transform a point and, at the same time, calculate a
  // 3x3 Jacobian matrix that provides the partial derivatives of the
  // transformation at that point.  This method does not call Update.
  // Meant for use only within other VTK classes.
  virtual void InternalTransformDerivative( const float in[3], float out[3], float derivative[3][3] );
  virtual void InternalTransformDerivative( const double in[3], double out[3], double derivative[3][3] );

  // Description:
  // Make another transform of the same type.
  virtual vtkAbstractTransform* MakeTransform();

  // Description:
  // If on, this transform converts (long,lat,alt) triples to (x,y,z) as an offset
  // from the center of the earth. Alt, x, y, and z are all be in meters.
  // If off, the tranform works in the reverse direction.
  vtkSetMacro(ToRectangular, bool);
  vtkGetMacro(ToRectangular, bool);
  vtkBooleanMacro(ToRectangular, bool);

protected:
  vtkGeoSphereTransform();
  virtual ~vtkGeoSphereTransform();

  bool ToRectangular;

private:
  vtkGeoSphereTransform( const vtkGeoSphereTransform& ); // Not implemented.
  void operator = ( const vtkGeoSphereTransform& ); // Not implemented.
};

#endif // __vtkGeoSphereTransform_h
