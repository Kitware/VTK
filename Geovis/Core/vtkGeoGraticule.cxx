/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGeoGraticule.cxx

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
#include "vtkGeoGraticule.h"

#include "vtkDoubleArray.h"
#include "vtkObjectFactory.h"
#include "vtkPolyData.h"
#include "vtkCellData.h"
#include "vtkCellType.h"
#include "vtkCellArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkPoints.h"
#include "vtkPointData.h"

#include <vector>

vtkStandardNewMacro(vtkGeoGraticule);

double vtkGeoGraticule::LatitudeLevelTics[12] =
{
  90.,         // level  0:  90 degrees
  30.,         // level  1:  30 degrees
  10.,         // level  2:  10 degrees
  5.,          // level  3:   5 degrees
  1.,          // level  4:   1 degree
  1. / 4.,     // level  5:  15 minutes (0.25    degrees)
  1. / 12.,    // level  6:   5 minutes (0.08333 degrees)
  1. / 60.,    // level  7:   1 minute  (0.01667 degrees)
  1. / 240.,   // level  8:  15 seconds (0.25    minutes)
  1. / 720.,   // level  9:   5 seconds (0.08333 minutes)
  1. / 3600.,  // level 10:   1 second  (0.01667 minutes)
  1. / 7200.   // level 11: 1/2 second  (0.00833 minutes)
};

double vtkGeoGraticule::LongitudeLevelTics[12] =
{
  90.,         // level  0:  90 degrees
  30.,         // level  1:  30 degrees
  10.,         // level  2:  10 degrees
  5.,          // level  3:   5 degrees
  1.,          // level  4:   1 degree
  1. / 4.,     // level  5:  15 minutes (0.25    degrees)
  1. / 12.,    // level  6:   5 minutes (0.08333 degrees)
  1. / 60.,    // level  7:   1 minute  (0.01667 degrees)
  1. / 240.,   // level  8:  15 seconds (0.25    minutes)
  1. / 720.,   // level  9:   5 seconds (0.08333 minutes)
  1. / 3600.,  // level 10:   1 second  (0.01667 minutes)
  1. / 7200.   // level 11: 1/2 second  (0.00833 minutes)
};

vtkGeoGraticule::vtkGeoGraticule()
{
  this->SetNumberOfInputPorts( 0 );
  this->LatitudeLevel = 2;
  this->LongitudeLevel = 1;
  this->LatitudeBounds[0]  = -90.;
  this->LatitudeBounds[1]  =  90.;
  this->LongitudeBounds[0] =   0.;
  this->LongitudeBounds[1] = 180.;
  this->GeometryType = vtkGeoGraticule::POLYLINES;
}

vtkGeoGraticule::~vtkGeoGraticule()
{
}

void vtkGeoGraticule::PrintSelf( ostream& os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );
  os << indent << "GeometryType: " << this->GeometryType << "\n";
  os << indent << "LatitudeLevel: " << this->LatitudeLevel << "\n";
  os << indent << "LongitudeLevel: " << this->LongitudeLevel << "\n";
  os << indent << "LatitudeBounds:  [ " <<  this->LatitudeBounds[0] << ", " <<  this->LatitudeBounds[1] << " [\n";
  os << indent << "LongitudeBounds: [ " << this->LongitudeBounds[0] << ", " << this->LongitudeBounds[1] << " [\n";
}

inline double vtkGraticuleLowerBound( double endpt, double incr )
{
  return incr * floor( endpt / incr );
}

inline double vtkGraticuleUpperBound( double endpt, double incr )
{
  return incr * ceil( endpt / incr );
}

int vtkGeoGraticule::RequestData( vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector )
{
  if  (
    ( this->LatitudeBounds[0]  ==  this->LatitudeBounds[1] ) ||
    ( this->LongitudeBounds[0] == this->LongitudeBounds[1] ) )
    { // no work to do.
    return 1;
    }

  vtkInformation* outInfo = outputVector->GetInformationObject( 0 );
  if ( ! outInfo )
    {
    return 0;
    }

  vtkPolyData* output = vtkPolyData::SafeDownCast( outInfo->Get( vtkDataObject::DATA_OBJECT() ) );
  if ( ! output )
    {
    return 0;
    }

  vtkPoints* pts = vtkPoints::New();
  output->SetPoints( pts );
  pts->Delete();
  output->Allocate( 1000 );

  // Clamp the bounds
  double latbds[2];
  double lngbds[2];

  if ( this->LatitudeBounds[0] > this->LatitudeBounds[1] )
    {
    latbds[0] = this->LatitudeBounds[1];
    latbds[1] = this->LatitudeBounds[0];
    }
  else
    {
    latbds[0] = this->LatitudeBounds[0];
    latbds[1] = this->LatitudeBounds[1];
    }

  if ( this->LongitudeBounds[0] > this->LongitudeBounds[1] )
    {
    lngbds[0] = this->LongitudeBounds[1];
    lngbds[1] = this->LongitudeBounds[0];
    }
  else
    {
    lngbds[0] = this->LongitudeBounds[0];
    lngbds[1] = this->LongitudeBounds[1];
    }

  // Now, if the bounds don't line up on a tic, jiggle them so that they are
  // on the closest mark at the current Level that is off-screen (i.e. covering
  // more than the requested area).
  double latTicIncrement = vtkGeoGraticule::LatitudeLevelTics[this->LatitudeLevel];
  double lngTicIncrement = vtkGeoGraticule::LongitudeLevelTics[this->LongitudeLevel];

  latbds[0] = vtkGraticuleLowerBound( latbds[0], latTicIncrement );
  latbds[1] = vtkGraticuleUpperBound( latbds[1], latTicIncrement );

  lngbds[0] = vtkGraticuleLowerBound( lngbds[0], lngTicIncrement );
  lngbds[1] = vtkGraticuleUpperBound( lngbds[1], lngTicIncrement );

  // Clamp to a meaningful range
  if ( latbds[0] < -90. )
    latbds[0] = -90.;
  if ( latbds[1] >  90. )
    latbds[1] =  90.;
  // NB: Allow "out-of-bounds" values for longitude.


  this->GenerateGraticule( output, latbds, lngbds );

  return 1;
}

void vtkGeoGraticule::GenerateGraticule( vtkPolyData* output, double latbds[2], double lngbds[2] )
{
  vtkPoints* pts = output->GetPoints();
  vtkCellArray* edges = vtkCellArray::New();
  vtkIntArray* width = vtkIntArray::New();
  width->SetName( "LineLevel" );
  width->SetNumberOfComponents( 1 );

  vtkDoubleArray* latLong = vtkDoubleArray::New();
  latLong->SetNumberOfComponents( 2 );
  latLong->SetName( "LatLong" );

  // Do the parallels first and record the start of each so we can do the meridians afterwards
  std::vector<vtkIdType> offsets; // where each row of points starts.
  double latTicIncrement = vtkGeoGraticule::LatitudeLevelTics[this->LatitudeLevel];
  double lngTicIncrement = vtkGeoGraticule::LongitudeLevelTics[this->LongitudeLevel];
  double pt[3] = { 0., 0., 0. };

  double lat;
  double lng;
  vtkIdType m = 0;
  vtkIdType n = -1;
  vtkIdType p = 0;
  for ( lat = latbds[0]; lat < latbds[1] + latTicIncrement; lat += latTicIncrement, ++ p )
    {
    offsets.push_back( n + 1 );
    if ( this->GeometryType & vtkGeoGraticule::POLYLINES )
      {
      edges->InsertNextCell( 1 );
      }
    pt[1] = lat;
    m = 0;
    for ( lng = lngbds[0]; lng < lngbds[1] + lngTicIncrement; lng += lngTicIncrement, ++m )
      {
      pt[0] = lng;
      n = pts->InsertNextPoint( pt );
      latLong->InsertNextTuple2( lat, lng );
      if ( this->GeometryType & vtkGeoGraticule::POLYLINES )
        {
        edges->InsertCellPoint( n );
        }
      }
    if ( this->GeometryType & vtkGeoGraticule::POLYLINES )
      {
      edges->UpdateCellCount( m );
      width->InsertNextValue( this->ComputeLineLevel( p, this->LatitudeLevel, vtkGeoGraticule::LatitudeLevelTics ) );
      }
    }
  vtkIdType gridSize[2] = { m, p };

  // Now do the meridians.
  if ( this->GeometryType & vtkGeoGraticule::POLYLINES && p == static_cast<int>( offsets.size() ) )
    {
    int lineLevel;
    int polarLatitudeLevel = this->LatitudeLevel - 2 >= 0 ? this->LatitudeLevel - 2 : 0;
    int k;
    p = 0;
    for ( lng = lngbds[0]; lng <= lngbds[1]; lng += lngTicIncrement, ++ p )
      {
      n = 0;
      k = 0;
      lineLevel = this->ComputeLineLevel( p, this->LongitudeLevel, vtkGeoGraticule::LongitudeLevelTics );
      edges->InsertNextCell( 1 );
      for ( lat = latbds[0]; lat <= latbds[1]; lat += latTicIncrement, ++ n )
        {
        // When near the poles, include fewer meridians.
        if ( fabs(lat) <= 60. || lineLevel <= polarLatitudeLevel )
          {
          edges->InsertCellPoint( offsets[n] );
          ++ k;
          }
        ++ offsets[n];
        }
      edges->UpdateCellCount( k );
      width->InsertNextValue( lineLevel );
      }
    }
  output->SetLines( edges );
  edges->FastDelete();

  // Now create the quads to texture
  if ( this->GeometryType & vtkGeoGraticule::QUADRILATERALS )
    {
    vtkCellArray* quads = vtkCellArray::New();
    m = 0;
    vtkIdType quadConn[4];
    for ( p = 0; p < gridSize[1] - 1; ++ p )
      {
      for ( m = 0; m < gridSize[0] - 1; ++ m )
        {
        quadConn[0] =   p       * gridSize[0] + m;
        quadConn[1] =   p       * gridSize[0] + m + 1;
        quadConn[2] = ( p + 1 ) * gridSize[0] + m + 1;
        quadConn[3] = ( p + 1 ) * gridSize[0] + m;
        quads->InsertNextCell( 4, quadConn );
        width->InsertNextValue( -1 );
        }
      }
    output->SetPolys( quads );
    quads->FastDelete();
    }

  output->GetCellData()->AddArray( width );
  output->GetCellData()->SetActiveScalars( "LineLevel" );
  width->FastDelete();
  output->GetPointData()->AddArray( latLong );
  //output->GetPointData()->SetActiveTCoords( "LatLong" );
  latLong->FastDelete();
}

int vtkGeoGraticule::ComputeLineLevel( int ticId, int baseLevel, const double* levelIncrements )
{
  for ( int curLevel = 0; curLevel < baseLevel; ++ curLevel )
    {
    if ( ticId % static_cast<int>( levelIncrements[curLevel] / levelIncrements[baseLevel] ) == 0 )
      {
      return curLevel;
      }
    }
  return baseLevel;
}

