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
/**
 * @class   vtkGeoTransform
 * @brief   A transformation between two geographic coordinate systems
 *
 * This class takes two geographic projections and transforms point
 * coordinates between them.
*/

#ifndef vtkGeoTransform_h
#define vtkGeoTransform_h

#include "vtkGeovisCoreModule.h" // For export macro
#include "vtkAbstractTransform.h"

class vtkGeoProjection;

class VTKGEOVISCORE_EXPORT vtkGeoTransform : public vtkAbstractTransform
{
public:
  static vtkGeoTransform* New();
  void PrintSelf( ostream& os, vtkIndent indent ) VTK_OVERRIDE;
  vtkTypeMacro(vtkGeoTransform,vtkAbstractTransform);

  //@{
  /**
   * The source geographic projection.
   */
  void SetSourceProjection(vtkGeoProjection* source);
  vtkGetObjectMacro(SourceProjection,vtkGeoProjection);
  //@}

  //@{
  /**
   * The target geographic projection.
   */
  void SetDestinationProjection(vtkGeoProjection* dest);
  vtkGetObjectMacro(DestinationProjection,vtkGeoProjection);
  //@}

  /**
   * Transform many points at once.
   */
  void TransformPoints( vtkPoints* src, vtkPoints* dst ) VTK_OVERRIDE;

  /**
   * Invert the transformation.
   */
  void Inverse() VTK_OVERRIDE;

  //@{
  /**
   * This will calculate the transformation without calling Update.
   * Meant for use only within other VTK classes.
   */
  void InternalTransformPoint( const float in[3], float out[3] ) VTK_OVERRIDE;
  void InternalTransformPoint( const double in[3], double out[3] ) VTK_OVERRIDE;
  //@}

  //@{
  /**
   * This will transform a point and, at the same time, calculate a
   * 3x3 Jacobian matrix that provides the partial derivatives of the
   * transformation at that point.  This method does not call Update.
   * Meant for use only within other VTK classes.
   */
  void InternalTransformDerivative( const float in[3], float out[3], float derivative[3][3] ) VTK_OVERRIDE;
  void InternalTransformDerivative( const double in[3], double out[3], double derivative[3][3] ) VTK_OVERRIDE;
  //@}

  /**
   * Make another transform of the same type.
   */
  vtkAbstractTransform* MakeTransform() VTK_OVERRIDE;

protected:
  vtkGeoTransform();
  ~vtkGeoTransform() VTK_OVERRIDE;

  void InternalTransformPoints( double* ptsInOut, vtkIdType numPts, int stride );

  vtkGeoProjection* SourceProjection;
  vtkGeoProjection* DestinationProjection;

private:
  vtkGeoTransform( const vtkGeoTransform& ) VTK_DELETE_FUNCTION;
  void operator = ( const vtkGeoTransform& ) VTK_DELETE_FUNCTION;
};

#endif // vtkGeoTransform_h
