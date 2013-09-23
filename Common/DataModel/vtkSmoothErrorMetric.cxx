/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSmoothErrorMetric.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSmoothErrorMetric.h"

#include "vtkObjectFactory.h"
#include "vtkGenericAttribute.h"
#include "vtkGenericAttributeCollection.h"
#include "vtkGenericAdaptorCell.h"
#include "vtkGenericDataSet.h"
#include "vtkMath.h"
#include <cassert>

vtkStandardNewMacro(vtkSmoothErrorMetric);

//-----------------------------------------------------------------------------
vtkSmoothErrorMetric::vtkSmoothErrorMetric()
{
  this->AngleTolerance = 90.1; // in degrees
  this->CosTolerance = cos( vtkMath::RadiansFromDegrees( this->AngleTolerance ) );
}

//-----------------------------------------------------------------------------
vtkSmoothErrorMetric::~vtkSmoothErrorMetric()
{
}

//-----------------------------------------------------------------------------
double vtkSmoothErrorMetric::GetAngleTolerance()
{
  return this->AngleTolerance;
}

//-----------------------------------------------------------------------------
void vtkSmoothErrorMetric::SetAngleTolerance(double value)
{
//  assert("pre: positive_value" && value>90 && value<180);
  if(this->AngleTolerance!=value)
    {
    // Clamp the value
    if(value<=90)
      {
      vtkWarningMacro(<< "value " << value << " out of range ]90,180[, clamped to 90.1" );
      this->AngleTolerance=90.1;
      }
    else
      {
      if(value>=180)
        {
        vtkWarningMacro(<< "value " << value << " out of range ]90,180[, clamped to 179.9" );
        this->AngleTolerance=179.9;
        }
      else
        {
        this->AngleTolerance=value;
        }
      }
    this->CosTolerance = cos( vtkMath::RadiansFromDegrees( this->AngleTolerance ) );
    this->Modified();
    }
}

//-----------------------------------------------------------------------------
int vtkSmoothErrorMetric::RequiresEdgeSubdivision(double *leftPoint,
                                                  double *midPoint,
                                                  double *rightPoint,
                                                  double vtkNotUsed(alpha)
  )
{
  assert( "pre: leftPoint_exists" && leftPoint != 0 );
  assert( "pre: midPoint_exists" && midPoint != 0 );
  assert( "pre: rightPoint_exists" && rightPoint != 0 );
//  assert( "pre: clamped_alpha" && alpha>0 && alpha < 1. ); // or else true
  if( this->GenericCell->IsGeometryLinear() )
    {
    //don't need to do anything:
    return 0;
    }

  double a[3];
  double b[3];

  a[0] = leftPoint[0] - midPoint[0];
  a[1] = leftPoint[1] - midPoint[1];
  a[2] = leftPoint[2] - midPoint[2];
  b[0] = rightPoint[0] - midPoint[0];
  b[1] = rightPoint[1] - midPoint[1];
  b[2] = rightPoint[2] - midPoint[2];


  double dota = vtkMath::Dot( a, a );
  double dotb = vtkMath::Dot( b, b );
  double cosa;

  if( dota == 0 || dotb == 0 )
    {
    cosa = -1.;
    }
  else
    {
    cosa = vtkMath::Dot( a, b ) / sqrt( dota * dotb );
    }

  int result = ( cosa > this->CosTolerance );

  return result;
}

//-----------------------------------------------------------------------------
// Description:
// Return the error at the mid-point. The type of error depends on the state
// of the concrete error metric. For instance, it can return an absolute
// or relative error metric.
// See RequiresEdgeSubdivision() for a description of the arguments.
// \post positive_result: result>=0
double vtkSmoothErrorMetric::GetError(double *leftPoint,
                                      double *midPoint,
                                      double *rightPoint,
                                      double vtkNotUsed(alpha)
  )
{
  assert( "pre: leftPoint_exists" && leftPoint != 0 );
  assert( "pre: midPoint_exists" && midPoint != 0 );
  assert( "pre: rightPoint_exists" && rightPoint != 0 );
//  assert( "pre: clamped_alpha" && alpha > 0. && alpha < 1. );
  if( this->GenericCell->IsGeometryLinear() )
    {
    //don't need to do anything:
    return 0.;
    }

  double a[3];
  double b[3];

  a[0] = leftPoint[0] - midPoint[0];
  a[1] = leftPoint[1] - midPoint[1];
  a[2] = leftPoint[2] - midPoint[2];
  b[0] = rightPoint[0] - midPoint[0];
  b[1] = rightPoint[1] - midPoint[1];
  b[2] = rightPoint[2] - midPoint[2];


  double dota = vtkMath::Dot(a,a);
  double dotb = vtkMath::Dot(b,b);
  double cosa;

  if( dota == 0. || dotb==0. )
    {
    cosa = -1.;
    }
  else
    {
    cosa = vtkMath::Dot( a, b ) / sqrt( dota * dotb );
    }

  if( cosa > 1.)
    {
    cosa = 1.;
    }
  else if( cosa < -1. )
    {
    cosa = -1.;
    }
  double result = 180. - vtkMath::RadiansFromDegrees( acos( cosa ) );
  assert( "post: positive_result" && result >= 0. );
  return result;
}

//-----------------------------------------------------------------------------
void vtkSmoothErrorMetric::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "AngleTolerance: "  << this->AngleTolerance << endl;
  os << indent << "CosTolerance: "  << this->CosTolerance << endl;
}
