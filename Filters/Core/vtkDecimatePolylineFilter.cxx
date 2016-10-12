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

#include <map>

vtkStandardNewMacro(vtkDecimatePolylineFilter);

// Representation of a polyline as a doubly linked list of vertices.
class vtkDecimatePolylineFilter::Polyline
{
public:
  struct Vertex
  {
    unsigned index;
    vtkIdType id;
    Vertex* prev;
    Vertex* next;
    bool removable;
  };

  Polyline(vtkIdType* vertexOrdering, vtkIdType size)
  {
    this->Size = size;
    Vertices = new Vertex[size];
    for (vtkIdType idx=0;idx<size;idx++)
    {
      Vertices[idx].index = idx;
      Vertices[idx].id = vertexOrdering[idx];
      Vertices[idx].prev = (idx > 0 ? &Vertices[idx-1] : NULL);
      Vertices[idx].next = (idx < size-1 ? &Vertices[idx+1] : NULL);
      Vertices[idx].removable = true;
    }
    Vertices[0].removable = Vertices[size-1].removable = false;
  }

  ~Polyline()
  {
    if (Vertices)
    {
      delete [] Vertices;
      Vertices = NULL;
    }
  }

  void Remove(vtkIdType vertexIdx)
  {
    this->Size--;
    (*(this->Vertices[vertexIdx].prev)).next = this->Vertices[vertexIdx].next;
    (*(this->Vertices[vertexIdx].next)).prev = this->Vertices[vertexIdx].prev;
  }

  vtkIdType Size;
  Vertex* Vertices;
};

//---------------------------------------------------------------------
// Create object with specified reduction of 90%.
vtkDecimatePolylineFilter::vtkDecimatePolylineFilter()
{
  this->TargetReduction = 0.90;
  this->PriorityQueue = vtkSmartPointer< vtkPriorityQueue >::New();
  this->OutputPointsPrecision = vtkAlgorithm::DEFAULT_PRECISION;
}

//---------------------------------------------------------------------
vtkDecimatePolylineFilter::~vtkDecimatePolylineFilter()
{
}

//---------------------------------------------------------------------
double
vtkDecimatePolylineFilter::ComputeError( vtkPolyData* input,
                                         Polyline* polyline,
                                         vtkIdType idx )
{
  vtkPoints * inputPoints = input->GetPoints();

  double x1[3], x[3], x2[3];
  inputPoints->GetPoint( polyline->Vertices[idx].prev->id, x1 );
  inputPoints->GetPoint( polyline->Vertices[idx].id, x );
  inputPoints->GetPoint( polyline->Vertices[idx].next->id, x2 );

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

  // Set the desired precision for the points in the output.
  if(this->OutputPointsPrecision == vtkAlgorithm::DEFAULT_PRECISION)
  {
    newPts->SetDataType(inputPoints->GetDataType());
  }
  else if(this->OutputPointsPrecision == vtkAlgorithm::SINGLE_PRECISION)
  {
    newPts->SetDataType(VTK_FLOAT);
  }
  else if(this->OutputPointsPrecision == vtkAlgorithm::DOUBLE_PRECISION)
  {
    newPts->SetDataType(VTK_DOUBLE);
  }

  vtkCellArray *newLines = vtkCellArray::New();
  newLines->Allocate(numLines,2);
  vtkPointData *inPD = input->GetPointData();
  vtkPointData *outPD = output->GetPointData();
  vtkCellData *inCD = input->GetCellData();
  vtkCellData *outCD = output->GetCellData();
  outPD->CopyAllocate(inPD);
  outCD->CopyAllocate(inCD);

  vtkIdType *linePtr = inputLines->GetPointer();
  vtkIdType firstVertexIndex = 0;
  vtkIdType polylineSize = 0;

  // Decimate each polyline (represented as a single cell) in series
  for ( vtkIdType lineId=0;lineId<numLines;lineId++,
          firstVertexIndex+=polylineSize)
  {
    polylineSize = linePtr[firstVertexIndex + lineId];

    // construct a polyline as a doubly linked list
    vtkDecimatePolylineFilter::Polyline* polyline =
      new vtkDecimatePolylineFilter::Polyline(&(linePtr[firstVertexIndex +
                                                        lineId + 1]),
                                              polylineSize);

    double error;
    for (vtkIdType vertexIdx=0;vertexIdx<polyline->Size;++vertexIdx)
    {
      // only vertices that are removable have associated error values
      if (polyline->Vertices[vertexIdx].removable)
      {
        error = this->ComputeError(input,polyline,vertexIdx);
        this->PriorityQueue->Insert(error,vertexIdx);
      }
    }

    // Now process structures,
    // deleting vertices until the decimation target is met.
    vtkIdType currentNumPts = polylineSize;
    while ( 1.0 - ( static_cast<double>( currentNumPts ) / static_cast<double>(
                      polylineSize ) )
            < this->TargetReduction &&
            currentNumPts > 2 )
    {
      --currentNumPts;
      vtkIdType poppedIdx = this->PriorityQueue->Pop();
      polyline->Remove(poppedIdx);
      vtkIdType prevIdx = polyline->Vertices[poppedIdx].prev->index;
      vtkIdType nextIdx = polyline->Vertices[poppedIdx].next->index;

      // again, only vertices that are removable have associated error values
      if (polyline->Vertices[poppedIdx].prev->removable)
      {
        error = this->ComputeError(input,polyline,prevIdx);
        this->PriorityQueue->DeleteId(prevIdx);
        this->PriorityQueue->Insert(error,prevIdx);
      }

      if (polyline->Vertices[poppedIdx].next->removable)
      {
        error = this->ComputeError(input,polyline,nextIdx);
        this->PriorityQueue->DeleteId(nextIdx);
        this->PriorityQueue->Insert(error,nextIdx);
      }
    }

    // What's left over is now spit out as a new polyline
    vtkIdType newId = newLines->InsertNextCell(currentNumPts);
    outCD->CopyData(inCD,firstVertexIndex,newId);

    std::map<vtkIdType,vtkIdType> pointIdMap;
    std::map<vtkIdType,vtkIdType>::iterator it;

    Polyline::Vertex* vertex = &(polyline->Vertices[0]);
    while (vertex != NULL)
    {
      // points that are repeated within a single polyline are represented by
      // only one point instance
      it = pointIdMap.find(vertex->id);
      if (it == pointIdMap.end())
      {
        newId = newPts->InsertNextPoint( inputPoints->GetPoint( vertex->id ) );
        newLines->InsertCellPoint( newId );
        outPD->CopyData( inPD, vertex->id, newId );
      }
      else
        newLines->InsertCellPoint( it->second );

      vertex = vertex->next;
    }

    delete polyline;
  }

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
  os << indent << "Output Points Precision: " << this->OutputPointsPrecision
     << "\n";
}
