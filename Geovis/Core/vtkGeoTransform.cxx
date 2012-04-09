/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGeoTransform.cxx

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

#include "vtkGeoTransform.h"

#include "vtkDoubleArray.h"
#include "vtkGeoProjection.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPoints.h"

#include "vtk_libproj4.h"

vtkStandardNewMacro(vtkGeoTransform);
vtkCxxSetObjectMacro(vtkGeoTransform, SourceProjection, vtkGeoProjection);
vtkCxxSetObjectMacro(vtkGeoTransform, DestinationProjection, vtkGeoProjection);

vtkGeoTransform::vtkGeoTransform()
{
  this->SourceProjection = 0;
  this->DestinationProjection = 0;
}

vtkGeoTransform::~vtkGeoTransform()
{
  if ( this->SourceProjection )
    {
    this->SourceProjection->Delete();
    }
  if ( this->DestinationProjection )
    {
    this->DestinationProjection->Delete();
    }
}

void vtkGeoTransform::PrintSelf( ostream& os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );
  os << indent << "SourceProjection: " << this->SourceProjection << "\n";
  os << indent << "DestinationProjection: " << this->DestinationProjection << "\n";
}

void vtkGeoTransform::TransformPoints( vtkPoints* srcPts, vtkPoints* dstPts )
{
  if ( ! srcPts || ! dstPts )
    {
    return;
    }

  vtkDoubleArray* srcCoords = vtkDoubleArray::SafeDownCast( srcPts->GetData() );
  vtkDoubleArray* dstCoords = vtkDoubleArray::SafeDownCast( dstPts->GetData() );
  if ( ! srcCoords || ! dstCoords )
    { // data not in a form we can use directly anyway...
    this->Superclass::TransformPoints( srcPts, dstPts );
    return;
    }
  dstCoords->DeepCopy( srcCoords );

  PROJ* src = this->SourceProjection ? this->SourceProjection->GetProjection() : 0;
  PROJ* dst = this->DestinationProjection ? this->DestinationProjection->GetProjection() : 0;
  if ( ! src && ! dst )
    {
    // we've already copied srcCoords to dstCoords and src=dst=0 implies no transform...
    return;
    }

  if ( srcCoords->GetNumberOfComponents() < 2 )
    {
    vtkErrorMacro( << "Source coordinate array " << srcCoords << " only has " << srcCoords->GetNumberOfComponents()
      << " components and at least 2 are required for geographic projections." );
    return;
    }

  this->InternalTransformPoints( dstCoords->GetPointer( 0 ), dstCoords->GetNumberOfTuples(), dstCoords->GetNumberOfComponents() );
}

void vtkGeoTransform::Inverse()
{
  vtkGeoProjection* tmp = this->SourceProjection;
  this->SourceProjection = this->DestinationProjection;
  this->DestinationProjection = tmp;
  this->Modified();
}

void vtkGeoTransform::InternalTransformPoint( const float in[3], float out[3] )
{
  double ind[3];
  double oud[3];
  int i;
  for ( i = 0; i < 3; ++ i )
    ind[i] = in[i];
  this->InternalTransformPoint( ind, oud );
  for ( i = 0; i < 3; ++ i )
    out[i] = static_cast<float>(oud[i]);
}

void vtkGeoTransform::InternalTransformPoint( const double in[3], double out[3] )
{
  for ( int i = 0; i < 3; ++ i )
    {
    out[i] = in[i];
    }
  this->InternalTransformPoints( out, 1, 3 );
}

void vtkGeoTransform::InternalTransformDerivative( const float in[3], float out[3], float derivative[3][3] )
{
  double ind[3];
  double oud[3];
  double drd[3][3];
  int i;
  for ( i = 0; i < 3; ++ i )
    ind[i] = in[i];
  this->InternalTransformDerivative( ind, oud, drd );
  for ( i = 0; i < 3; ++ i )
    {
    out[i] = static_cast<float>(oud[i]);
    for ( int j = 0; j < 3; ++ j )
      {
      derivative[i][j] = drd[i][j];
      }
    }
}

void vtkGeoTransform::InternalTransformDerivative( const double in[3], double out[3], double derivative[3][3] )
{
  // FIXME: Need to use pj_factors for both source and inverted dest projection
  (void) in;
  (void) out;
  (void) derivative;
}


vtkAbstractTransform* vtkGeoTransform::MakeTransform()
{
  vtkGeoTransform* geoTrans = vtkGeoTransform::New();
  return geoTrans;
}

void vtkGeoTransform::InternalTransformPoints( double* x, vtkIdType numPts, int stride )
{
  PROJ* src = this->SourceProjection ? this->SourceProjection->GetProjection() : 0;
  PROJ* dst = this->DestinationProjection ? this->DestinationProjection->GetProjection() : 0;
  int delta = stride - 2;
  PROJ_LP lp;
  PROJ_XY xy;
  if ( src )
    {
    // Convert from src system to lat/long using inverse of src transform
    double* coord = x;
    for ( vtkIdType i = 0; i < numPts; ++ i )
      {
      xy.x = coord[0]; xy.y = coord[1];
      lp = proj_inv( xy, src );
      coord[0] = lp.lam; coord[1] = lp.phi;
      }
    }
  else // ! src
    {
    // src coords are in degrees, convert to radians
    double* coord = x;
    for ( vtkIdType i = 0; i < numPts; ++ i )
      {
      for ( int j = 0; j < 2; ++ j, ++ coord )
        {
        *coord = vtkMath::RadiansFromDegrees( *coord );
        }
      coord += delta;
      }
    }
  if ( dst )
    {
    double* coord = x;
    for ( vtkIdType i = 0; i < numPts; ++ i )
      {
      lp.lam = coord[0]; lp.phi = coord[1];
      xy = proj_fwd( lp, dst );
      coord[0] = xy.x; coord[1] = xy.y;
      }
    }
  else // ! dst
    {
    // dst coords are in radians, convert to degrees
    double* coord = x;
    for ( vtkIdType i = 0; i < numPts; ++ i )
      {
      for ( int j = 0; j < 2; ++ j, ++ coord )
        {
        *coord = vtkMath::DegreesFromRadians( *coord );
        }
      coord += delta;
      }
    }
}

