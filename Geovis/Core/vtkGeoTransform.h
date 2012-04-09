/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGeoTransform.h

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
// .NAME vtkGeoTransform - A transformation between two geographic coordinate systems
// .SECTION Description
// This class takes two geographic projections and transforms point
// coordinates between them.

#ifndef __vtkGeoTransform_h
#define __vtkGeoTransform_h

#include "vtkAbstractTransform.h"

class vtkGeoProjection;

class VTK_GEOVIS_EXPORT vtkGeoTransform : public vtkAbstractTransform
{
public:
  static vtkGeoTransform* New();
  virtual void PrintSelf( ostream& os, vtkIndent indent );
  vtkTypeMacro(vtkGeoTransform,vtkAbstractTransform);

  // Description:
  // The source geographic projection.
  void SetSourceProjection(vtkGeoProjection* source);
  vtkGetObjectMacro(SourceProjection,vtkGeoProjection);

  // Description:
  // The target geographic projection.
  void SetDestinationProjection(vtkGeoProjection* dest);
  vtkGetObjectMacro(DestinationProjection,vtkGeoProjection);

  // Description:
  // Transform many points at once.
  virtual void TransformPoints( vtkPoints* src, vtkPoints* dst );

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

protected:
  vtkGeoTransform();
  virtual ~vtkGeoTransform();

  void InternalTransformPoints( double* ptsInOut, vtkIdType numPts, int stride );

  vtkGeoProjection* SourceProjection;
  vtkGeoProjection* DestinationProjection;

private:
  vtkGeoTransform( const vtkGeoTransform& ); // Not implemented.
  void operator = ( const vtkGeoTransform& ); // Not implemented.
};

#endif // __vtkGeoTransform_h
