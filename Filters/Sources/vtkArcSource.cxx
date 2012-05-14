/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkArcSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkArcSource.h"

#include "vtkCellArray.h"
#include "vtkFloatArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkMath.h"

#include <math.h>

vtkStandardNewMacro(vtkArcSource);

// --------------------------------------------------------------------------
vtkArcSource::vtkArcSource(int res)
{
  // Default first point
  this->Point1[0] =  0.0;
  this->Point1[1] =  0.5;
  this->Point1[2] =  0.0;

  // Default second point
  this->Point2[0] =  0.5;
  this->Point2[1] =  0.0;
  this->Point2[2] =  0.0;

  // Default center is origin
  this->Center[0] =  0.0;
  this->Center[1] =  0.0;
  this->Center[2] =  0.0;

  // Default normal vector is unit in Oz direction
  this->Normal[0] =  1.0;
  this->Normal[1] =  0.0;
  this->Normal[2] =  0.0;

  // Default polar vector
  this->PolarVector[0] =  0.0;
  this->PolarVector[1] =  0.0;
  this->PolarVector[2] =  1.0;

  // Default arc is a quarter-circle 
  this->Angle =  90.;

  // Default resolution
  this->Resolution = (res < 1 ? 1 : res);

  // Default resolution
  this->Negative = false;

  // By default use the original API
  this->UseNormalAndAngle = false;

  // This is a source
  this->SetNumberOfInputPorts( 0 );
}

// --------------------------------------------------------------------------
int vtkArcSource::RequestInformation(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
  // get the info object
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  outInfo->Set( vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES(), -1 );
  return 1;
}

// --------------------------------------------------------------------------
int vtkArcSource::RequestData( vtkInformation* vtkNotUsed(request),
                               vtkInformationVector** vtkNotUsed(inputVector),
                               vtkInformationVector* outputVector)
{
  int numLines = this->Resolution;
  int numPts = this->Resolution +1;
  double tc[3] = { 0.0, 0.0, 0.0 };

  // get the info object
  vtkInformation* outInfo = outputVector->GetInformationObject( 0 );

  if ( outInfo->Get( vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER() ) > 0 )
    {
    return 1;
    }

  // get the ouptut
  vtkPolyData* output
    = vtkPolyData::SafeDownCast( outInfo->Get( vtkDataObject::DATA_OBJECT() ) );

  // Calculate vector from origin to first point
  
  // Normal and angle are either specified (consistent API) or calculated (original API)
  double angle = 0.0;
  double radius = 0.5;
  double perpendicular[3];
  double v1[3];
  if ( this->UseNormalAndAngle )
    {
    // Retrieve angle, which is specified with this API
    angle = vtkMath::RadiansFromDegrees( this->Angle );

    // Retrieve polar vector, which is specified with this API
    for ( int i = 0; i < 3; ++ i )
      {
      v1[i] = this->PolarVector[i];
      }

    // Calculate perpendicular vector with normal which is specified with this API
    vtkMath::Cross( this->Normal, this->PolarVector, perpendicular );
    
    // Calculate radius
    radius = vtkMath::Normalize( v1 );
    }
  else // if ( this->UseNormalAndRadius )
    {
    // Compute the cross product of the two vectors.
    for ( int i = 0; i < 3; ++ i )
      {
      v1[i] = this->Point1[i] - this->Center[i];
      }

    double v2[3] = { this->Point2[0] - this->Center[0],
                     this->Point2[1] - this->Center[1],
                     this->Point2[2] - this->Center[2] };
    
    double normal[3];
    vtkMath::Cross( v1, v2, normal );
    vtkMath::Cross( normal, v1, perpendicular );
    double dotprod =
      vtkMath::Dot( v1, v2 ) / ( vtkMath::Norm( v1 ) * vtkMath::Norm( v2 ) );
    angle = acos( dotprod );
    if ( this->Negative )
      {
      angle -= (2.0 * vtkMath::Pi());
      }

    // Calcute radius
    radius = vtkMath::Normalize( v1 );
    } // else
  
  // Calcute angle increment
  double angleInc = angle / this->Resolution;

  // Normalize perpendicular vector
  vtkMath::Normalize( perpendicular );

  // Now create arc points and segments
  vtkPoints *newPoints = vtkPoints::New();
  newPoints->Allocate( numPts );
  vtkFloatArray *newTCoords = vtkFloatArray::New();
  newTCoords->SetNumberOfComponents( 2 );
  newTCoords->Allocate( 2 * numPts );
  newTCoords->SetName( "Texture Coordinates" );
  vtkCellArray *newLines = vtkCellArray::New();
  newLines->Allocate( newLines->EstimateSize( numLines, 2 ) );

  double theta = 0.0;
  // Iterate over angle increments
  for ( int i = 0; i <= this->Resolution; ++ i, theta += angleInc )
    {
    const double cosine = cos(theta);
    const double sine = sin(theta);
    double p[3] =
      { this->Center[0] + cosine*radius*v1[0] + sine*radius*perpendicular[0],
        this->Center[1] + cosine*radius*v1[1] + sine*radius*perpendicular[1],
        this->Center[2] + cosine*radius*v1[2] + sine*radius*perpendicular[2] };

    tc[0] = static_cast<double>( i ) / this->Resolution;
    newPoints->InsertPoint( i ,p );
    newTCoords->InsertTuple( i, tc );
    }

  newLines->InsertNextCell( numPts );
  for ( int k = 0; k < numPts; ++ k )
    {
    newLines->InsertCellPoint( k );
    }

  output->SetPoints( newPoints );
  newPoints->Delete();

  output->GetPointData()->SetTCoords( newTCoords );
  newTCoords->Delete();

  output->SetLines( newLines );
  newLines->Delete();

  return 1;
}

// --------------------------------------------------------------------------
void vtkArcSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Resolution: " << this->Resolution << "\n";

  os << indent << "Point 1: (" << this->Point1[0] << ", "
                               << this->Point1[1] << ", "
                               << this->Point1[2] << ")\n";

  os << indent << "Point 2: (" << this->Point2[0] << ", "
                               << this->Point2[1] << ", "
                               << this->Point2[2] << ")\n";

  os << indent << "Center: (" << this->Center[0] << ", "
                              << this->Center[1] << ", "
                              << this->Center[2] << ")\n";

  os << indent << "Normal: (" << this->Normal[0] << ", "
                              << this->Normal[1] << ", "
                              << this->Normal[2] << ")\n";

  os << indent << "PolarVector: (" << this->PolarVector[0] << ", "
                                   << this->PolarVector[1] << ", "
                                   << this->PolarVector[2] << ")\n";

  os << indent << "Angle: " << this->Angle << "\n";

  os << indent << "Negative: " << this->Negative << "\n";

  os << indent << "UseNormalAndAngle: " << this->UseNormalAndAngle << "\n";
}

