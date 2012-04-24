/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLineSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkLineSource.h"

#include "vtkCellArray.h"
#include "vtkFloatArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"

#include <math.h>

vtkStandardNewMacro(vtkLineSource);
vtkCxxSetObjectMacro(vtkLineSource,Points,vtkPoints);

// ----------------------------------------------------------------------
vtkLineSource::vtkLineSource(int res)
{
  this->Point1[0] = -.5;
  this->Point1[1] =  .0;
  this->Point1[2] =  .0;

  this->Point2[0] =  .5;
  this->Point2[1] =  .0;
  this->Point2[2] =  .0;

  this->Points = 0;

  this->Resolution = ( res < 1 ? 1 : res );

  this->SetNumberOfInputPorts( 0 );
}

// ----------------------------------------------------------------------
vtkLineSource::~vtkLineSource()
{
  this->SetPoints( 0 );
}

// ----------------------------------------------------------------------
int vtkLineSource::RequestInformation(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
  // get the info object
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  outInfo->Set(vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES(),
               -1);
  return 1;
}

// ----------------------------------------------------------------------
int vtkLineSource::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
  // Reject meaningless parameterizations
  vtkIdType nSegments = this->Points ? this->Points->GetNumberOfPoints() - 1 : 1;
  if ( nSegments < 1 )
    {
    vtkWarningMacro( <<"Cannot define a broken line with given input.");
    return 0;
    }

  // get the info object
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the ouptut
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  if (outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()) > 0)
    {
    return 1;
    }

  // Create and allocate lines
  vtkIdType numLines = nSegments * this->Resolution;
  vtkCellArray *newLines = vtkCellArray::New();
  newLines->Allocate( newLines->EstimateSize( numLines, 2 ) );

  // Create and allocate points
  vtkIdType numPts = numLines + 1;
  vtkPoints *newPoints = vtkPoints::New();
  newPoints->Allocate( numPts );

  // Create and allocate texture coordinates
  vtkFloatArray *newTCoords = vtkFloatArray::New();
  newTCoords->SetNumberOfComponents( 2 );
  newTCoords->Allocate( 2 * numPts );
  newTCoords->SetName( "Texture Coordinates" );

  // Allocate convenience storage
  double x[3], tc[3], v[3];

  // Generate points and texture coordinates
  if ( this->Points )
    {
    // Create storage for segment endpoints
    double point1[3];
    double point2[3];

    // Point index offset for fast insertion
    vtkIdType offset = 0;

    // Iterate over segments
    for ( vtkIdType s = 0; s < nSegments; ++ s )
      {
      // Get coordinates of endpoints
      this->Points->GetPoint( s, point1 );
      this->Points->GetPoint( s + 1, point2 );

      // Calculate segment vector
      for ( int i = 0; i < 3; ++ i )
        {
        v[i] = point2[i] - point1[i];
        }

      // Generate points along segment
      tc[1] = 0.;
      tc[2] = 0.;
      for ( vtkIdType i = 0; i < this->Resolution; ++ i, ++ offset )
        {
        tc[0] = static_cast<double>( i ) / this->Resolution;
        for ( int j = 0; j < 3; ++ j )
          {
          x[j] = point1[j] + tc[0] * v[j];
          }
        newPoints->InsertPoint( offset, x );
        newTCoords->InsertTuple( offset, tc );
        }
      } // s

    // Generate last endpoint
    newPoints->InsertPoint( numLines, point2 );
    tc[0] = 1.;
    newTCoords->InsertTuple( numLines, tc );

    } // if ( this->Points )
  else
    {
    // Calculate segment vector
    for ( int i = 0; i < 3; ++ i )
      {
      v[i] = this->Point2[i] - this->Point1[i];
      }

    // Generate points along segment
    tc[1] = 0.;
    tc[2] = 0.;
    for ( vtkIdType i = 0; i < numPts; ++ i )
      {
      tc[0] = static_cast<double>( i ) / this->Resolution;
      for ( int j = 0; j < 3; ++ j )
        {
        x[j] = this->Point1[j] + tc[0] * v[j];
        }
      newPoints->InsertPoint( i, x );
      newTCoords->InsertTuple( i, tc );
      }
    } // else

  //  Generate lines
  newLines->InsertNextCell( numPts );
  for ( vtkIdType i = 0; i < numPts; ++ i )
    {
    newLines->InsertCellPoint( i );
    }

  // Update ourselves and release memory
  output->SetPoints( newPoints );
  newPoints->Delete();

  output->GetPointData()->SetTCoords( newTCoords );
  newTCoords->Delete();

  output->SetLines( newLines );
  newLines->Delete();

  return 1;
}

// ----------------------------------------------------------------------
void vtkLineSource::SetPoint1(float point1f[3])
{
  double point1d[3];
  point1d[0] = point1f[0];
  point1d[1] = point1f[1];
  point1d[2] = point1f[2];
  SetPoint1(point1d);
}

// ----------------------------------------------------------------------
void vtkLineSource::SetPoint2(float point2f[3])
{
  double point2d[3];
  point2d[0] = point2f[0];
  point2d[1] = point2f[1];
  point2d[2] = point2f[2];
  SetPoint2(point2d);
}

// ----------------------------------------------------------------------
void vtkLineSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Resolution: " << this->Resolution << "\n";

  os << indent << "Point 1: (" << this->Point1[0] << ", "
                               << this->Point1[1] << ", "
                               << this->Point1[2] << ")\n";

  os << indent << "Point 2: (" << this->Point2[0] << ", "
                               << this->Point2[1] << ", "
                               << this->Point2[2] << ")\n";

  os << indent
     << "Points: ";
  if ( this->Points )
    {
    this->Points->PrintSelf( os, indent );
    }
  else
    {
    os << "(none)" << endl;
    }
}
