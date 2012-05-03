/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDecimatePolylineFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkDecimatePolylineFilter.h"

#include "vtkDoubleArray.h"
#include "vtkLine.h"
#include "vtkMath.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkLine.h"
#include "vtkPolyData.h"
#include "vtkPoints.h"
#include "vtkCellArray.h"
#include "vtkPointData.h"
#include "vtkCellData.h"
#include "vtkPriorityQueue.h"
#include <vector>
#include <queue>
#include <map>

struct vtkDecimatePolylineFilter::vtkDecimatePolylineVertexErrorSTLMap
{
  std::map< int, double > VertexErrorMap;
};


vtkStandardNewMacro(vtkDecimatePolylineFilter);

//---------------------------------------------------------------------
// Create object with specified reduction of 90%.
vtkDecimatePolylineFilter::vtkDecimatePolylineFilter()
{
  this->TargetReduction = 0.90;
  this->Closed = true;
  this->PriorityQueue = vtkSmartPointer< vtkPriorityQueue >::New();
  this->ErrorMap = new vtkDecimatePolylineVertexErrorSTLMap;
}

//---------------------------------------------------------------------
vtkDecimatePolylineFilter::~vtkDecimatePolylineFilter()
{
  delete this->ErrorMap;
}

//---------------------------------------------------------------------
double
vtkDecimatePolylineFilter::ComputeError( vtkPolyData* input,
  int prev, int id, int next )
{
  vtkPoints * inputPoints = input->GetPoints();

  double x1[3], x[3], x2[3];
  inputPoints->GetPoint( prev, x1 );
  inputPoints->GetPoint( id, x );
  inputPoints->GetPoint( next, x2 );

  if ( vtkMath::Distance2BetweenPoints( x1, x2 ) == 0.0 )
    {
    return 0.0;
    }
  else
    {
    return vtkLine::DistanceToLine( x, x1, x2 );
    }
}
//---------------------------------------------------------------------
//  Reduce the number of points in a set of polylines
//
int vtkDecimatePolylineFilter::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkPolyData *input = vtkPolyData::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkCellArray *inputLines = input->GetLines();
  vtkPoints * inputPoints = input->GetPoints();

  vtkDebugMacro("Decimating polylines");

  if (!inputLines || !inputPoints)
    {
    return 1;
    }
  vtkIdType numLines = inputLines->GetNumberOfCells();
  vtkIdType numPts = inputPoints->GetNumberOfPoints();
  if ( numLines < 1 || numPts < 1 )
    {
    return 1;
    }

  // Allocate memory and prepare for data processing
  vtkPoints *newPts = vtkPoints::New();
  vtkCellArray *newLines = vtkCellArray::New();
  newLines->Allocate(numLines,2);
  vtkPointData *inPD = input->GetPointData();
  vtkPointData *outPD = output->GetPointData();
  vtkCellData *inCD = input->GetCellData();
  vtkCellData *outCD = output->GetCellData();
  outPD->CopyAllocate(inPD);
  outCD->CopyAllocate(inCD);

  // Loop over all polylines, decimating each independently
  vtkIdType i, cellId = 0, newId;
  double error;

  for ( i = 0; i < numPts; ++i )
    {
    this->ErrorMap->VertexErrorMap[i] = 0.;
    }

  for ( i = 0; i < numPts; ++i )
    {
    error = ComputeError( input, GetPrev(i), i, GetNext(i) );
    this->ErrorMap->VertexErrorMap[i] = error;
    this->PriorityQueue->Insert( error, i );
    }//for all points in polyline

  // Now process structures,
  // deleting points until the decimation target is met.
  vtkIdType currentNumPts = this->PriorityQueue->GetNumberOfItems();
  while ( 1.0 - ( static_cast<double>( currentNumPts ) / static_cast<double>(
                    numPts ) )
          < this->TargetReduction &&
          currentNumPts > 2 )
    {
    i = this->PriorityQueue->Pop( );
    --currentNumPts;
    UpdateError( input, i );
    this->ErrorMap->VertexErrorMap.erase( i );
    }

  // What's left over is now spit out as a new polyline
  newId = newLines->InsertNextCell( currentNumPts + 1);
  outCD->CopyData( inCD, cellId, newId );

  std::map< int, double >::iterator it = this->ErrorMap->VertexErrorMap.begin();

  while( it != this->ErrorMap->VertexErrorMap.end() )
    {
    newId = newPts->InsertNextPoint( inputPoints->GetPoint( it->first ) );
    newLines->InsertCellPoint( newId );
    outPD->CopyData( inPD, it->first, newId );
    ++it;
    }
  if( this->Closed )
    {
    newId = newPts->InsertNextPoint( newPts->GetPoint( 0 ) );
    newLines->InsertCellPoint( 0 );
    outPD->CopyData( inPD, this->ErrorMap->VertexErrorMap.begin()->first, newId );
    }

  // Clean up in preparation for the next line
  this->PriorityQueue->Reset();

  // Create output and clean up
  output->SetPoints( newPts );
  output->SetLines( newLines );

  newLines->Delete();
  newPts->Delete();

  return 1;
}
//---------------------------------------------------------------------
int vtkDecimatePolylineFilter::GetPrev( int iId )
{
  std::map< int, double >::iterator it = this->ErrorMap->VertexErrorMap.find( iId );

  if( it == this->ErrorMap->VertexErrorMap.begin() )
    {
    if( this->Closed )
      {
      it = this->ErrorMap->VertexErrorMap.end();
      --it;
      return it->first;
      }
    else
      {
      return iId;
      }
    }
  else
    {
    --it;
    return it->first;
    }
}
//---------------------------------------------------------------------
int vtkDecimatePolylineFilter::GetNext( int iId )
{
  std::map< int, double >::iterator
    it = this->ErrorMap->VertexErrorMap.find( iId );
  std::map< int, double >::iterator
    end_it = this->ErrorMap->VertexErrorMap.end();
  --end_it;
  if( it == end_it )
    {
    if( this->Closed )
      {
      return this->ErrorMap->VertexErrorMap.begin()->first;
      }
    else
      {
      return iId;
      }
    }
  else
    {
    ++it;
    return it->first;
    }
}
//---------------------------------------------------------------------
void vtkDecimatePolylineFilter::UpdateError( vtkPolyData* input, int iId )
{
  int prev = GetPrev( iId );
  int prev_prev = GetPrev( prev );
  int next = GetNext( iId );
  int next_next = GetNext( next );

  double prev_error = ComputeError( input, prev_prev, prev, next );
  this->ErrorMap->VertexErrorMap[prev] = prev_error;
  this->PriorityQueue->DeleteId( prev );
  this->PriorityQueue->Insert( prev_error, prev );

  double next_error = ComputeError( input, prev, next, next_next );
  this->ErrorMap->VertexErrorMap[next] = next_error;
  this->PriorityQueue->DeleteId( next );
  this->PriorityQueue->Insert( next_error, next );
}
//---------------------------------------------------------------------

//---------------------------------------------------------------------
void vtkDecimatePolylineFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Target Reduction: " << this->TargetReduction << "\n";
}
