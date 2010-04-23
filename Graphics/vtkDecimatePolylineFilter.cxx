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
#include <vtkstd/list>
#include <vtkstd/vector>
#include <vtkstd/queue>

vtkStandardNewMacro(vtkDecimatePolylineFilter);

//------------------------------------------------------------------
// STL list for implementing the algorithm
struct vtkPLineVertex
{
  vtkIdType PtId;
  double Error;

  vtkPLineVertex(vtkIdType id, double error) : PtId(id), Error(error) {}
};
typedef vtkstd::list<vtkPLineVertex> PLineVertexListType;
typedef vtkstd::list<vtkPLineVertex>::iterator PLineVertexListIterator;

class CompareError { 
public: 
  int operator()( const PLineVertexListIterator &x, const PLineVertexListIterator &y ) 
    { return x->Error > y->Error; } 
};
typedef vtkstd::priority_queue<PLineVertexListIterator,vtkstd::vector<PLineVertexListIterator>,CompareError> PQueueType;

//---------------------------------------------------------------------
// Create object with specified reduction of 90%.
vtkDecimatePolylineFilter::vtkDecimatePolylineFilter()
{
  this->TargetReduction = 0.90;
}

//---------------------------------------------------------------------
vtkDecimatePolylineFilter::~vtkDecimatePolylineFilter()
{
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
  vtkIdType npts, *pts, i, cellId, newId;
  double x[3], x1[3], x2[3], error, len, dist;
  PLineVertexListType VertList;
  PLineVertexListIterator vertIterator;
  PQueueType PQueue;
  for (cellId=0, inputLines->InitTraversal(); inputLines->GetNextCell(npts,pts); cellId++)
    {
    if ( npts < 3 )
      {
      newId = newLines->InsertNextCell(npts,pts);
      outCD->CopyData(inCD,cellId,newId);
      for (i=0; i < npts; i++)
        {
        newId = newPts->InsertNextPoint(inputPoints->GetPoint(pts[i]));
        outPD->CopyData(inPD,pts[i],newId);
        }
      continue; //skip the rest
      }
    
    // Start off by loading data into data structures
    VertList.clear();
    for (i=0; i < npts; i++)
      {
      if ( i == 0 || i == (npts-1) )
        {
        error = VTK_LARGE_FLOAT;
        }
      else
        {
        inputPoints->GetPoint(pts[i-1],x1);
        inputPoints->GetPoint(pts[i],x);
        inputPoints->GetPoint(pts[i+1],x2);
        if ( (len = sqrt(vtkMath::Distance2BetweenPoints(x1,x2))) <= 0.0 )
          {
          error = 0.0;
          }
        else
          {
          dist = sqrt(vtkLine::DistanceToLine(x,x1,x2));
          error = dist/len;
          }
        }
      vertIterator = VertList.insert(VertList.end(),vtkPLineVertex(pts[i],error));
      PQueue.push(vertIterator);
      }//for all points in polyline

    // Now process structures, deleting points until the decimation target is met.
    vtkIdType currentNumPts = npts;
    while ( 1.0 - (static_cast<double>(currentNumPts)/static_cast<double>(npts)) < this->TargetReduction &&
            currentNumPts > 2 )
      {
      vertIterator = PQueue.top();
      PQueue.pop();
      currentNumPts--;
      VertList.erase(vertIterator);
      }

    // What's left over is now spit out as a new polyline
    newId = newLines->InsertNextCell(currentNumPts);
    outCD->CopyData(inCD,cellId,newId);
    for ( vertIterator = VertList.begin(); vertIterator != VertList.end(); ++ vertIterator )
      {
      newId = newPts->InsertNextPoint(inputPoints->GetPoint(vertIterator->PtId));
      newLines->InsertCellPoint(newId);
      outPD->CopyData(inPD,vertIterator->PtId,newId);
      }

    // Clean up in preparation for the next line
    while ( !PQueue.empty() )  //empty the queue in preparation for the next line
      { 
      PQueue.pop(); 
      }
    }//loop over all polylines

  // Create output and clean up
  output->SetPoints( newPts );
  output->SetLines( newLines );

  newLines->Delete();
  newPts->Delete();

  return 1;
}

//---------------------------------------------------------------------
void vtkDecimatePolylineFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Target Reduction: " << this->TargetReduction << "\n";
}
