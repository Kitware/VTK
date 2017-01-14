/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkContourLoopExtraction.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkContourLoopExtraction.h"

#include "vtkCellArray.h"
#include "vtkExecutive.h"
#include "vtkGarbageCollector.h"
#include "vtkLine.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkCellData.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkPolygon.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <cfloat>
#include <vector>

vtkStandardNewMacro(vtkContourLoopExtraction);

//----------------------------------------------------------------------------
namespace {
  // Note on the definition of parametric coordinates: Given a sequence of
  // lines segments (vi,vi+1) that form a primitive (e.g., polyline or
  // polygon), the parametric coordinate t along the primitive is
  // [i,i+1). Any point (like an intersection point on the segment) is i+t,
  // where 0 <= t < 1.


  // Infrastructure for cropping----------------------------------------------
  struct LoopPoint
  {
    double T; //parametric coordinate along linked lines
    vtkIdType Id;
    LoopPoint(double t, vtkIdType id) : T(t), Id(id)
    {
    }
  };

  // Special sort operation on primitive parametric coordinate T-------------
  bool PointSorter(LoopPoint const& lhs, LoopPoint const& rhs)
  {
    return lhs.T < rhs.T;
  }

  // Vectors are used to hold points.
  typedef std::vector<LoopPoint> LoopPointType;

  // Update the scalar range-------------------------------------------------
  void UpdateRange(vtkDataArray *scalars, vtkIdType pid, double range[2])
  {
    if ( ! scalars )
    {
      return;
    }

    int numComp = scalars->GetNumberOfComponents();
    double s;
    for (int i=0; i < numComp; ++i)
    {
      s = scalars->GetComponent(pid,i);
      range[0] = (s < range[0] ? s : range[0]);
      range[1] = (s > range[1] ? s : range[1]);
    }
  }

  // March along connected lines to the end----------------------------------
  // pts[0] is assumed to be the starting point and already inserted.
  vtkIdType TraverseLoop(double dir, vtkPolyData *polyData, vtkIdType lineId,
                         vtkIdType start, LoopPointType &sortedPoints,
                         char *visited, vtkDataArray *scalars, double range[2])
  {
    vtkIdType last=0, numInserted=0;
    double t = 0.0;
    bool terminated=false;
    unsigned short ncells;
    vtkIdType npts, *pts, *cells, nei, lastCell=lineId;
    polyData->GetCellPoints(lineId,npts,pts);

    // Recall that we are working with 2-pt lines
    while ( !terminated )
    {
      last = pts[1];
      numInserted++;
      t = dir * static_cast<double>(numInserted);
      sortedPoints.push_back(LoopPoint(t,last));
      UpdateRange(scalars,last,range);

      polyData->GetPointCells(last, ncells, cells);
      if (ncells == 1 || last == start ) //this is the last point
      {
        return last;
      }
      else if (ncells == 2) //continue along loop
      {
        nei = (cells[0] != lastCell ? cells[0] : cells[1]);
        polyData->GetCellPoints(nei,npts,pts);
        visited[nei] = 1;
        lastCell = nei;
      }
      else // non-manifold, for now just quit
      {
        terminated = true;
        break;
        // double x0[3], x1[3], x10[3];
        // // first get a vector along the current cell
        // polyData->GetPoint(pts[0],x0);
        // polyData->GetPoint(pts[1],x1);
        // vtkMath::Subtract(x1,x0,x10);
        // vtkMath::Normalize(x10);

        // // Now find the path that turns the most left
        // for (int i=0; i < ncells; ++i)
        // {
        //   if ( (nei=cells[i]) != lastCell )
        //   {
        //   }
        // }
      }
    }

    return last;
  }

  // March along connected lines to the end----------------------------------
  void OutputPolygon(LoopPointType &sortedPoints, vtkPoints *inPts,
                     vtkCellArray *outPolys, int loopClosure)
  {
    // Check to see that last point is the same as the first. Such a loop is
    // closed and can be directly output. Otherwise, check on the strategy
    // for closing the loop and close as appropriate.
    vtkIdType num = static_cast<vtkIdType>(sortedPoints.size());
    if ( sortedPoints[0].Id == sortedPoints[num-1].Id )
    {
      --num;
      sortedPoints.erase(sortedPoints.begin() + num);
    }

    else if (loopClosure == VTK_LOOP_CLOSURE_ALL)
    {
      ; //do nothing and it will close between the first and last points
    }

    // If here we assume that the loop begins and ends on a rectangular
    // boundary. Close the loop by walking the rectangular boundary.
    else //loopClosure == VTK_LOOP_CLOSURE_BOUNDARY
    {
      // First check the simple case, complete the loop along the boundary
      double x[3], y[3], delX, delY;
      inPts->GetPoint(sortedPoints[0].Id, x);
      inPts->GetPoint(sortedPoints[1].Id, y);
      delX = fabs(x[0]-y[0]);
      delY = fabs(x[1]-y[1]);
      // if no change in the x or y direction just return loop will complete
      if ( delX < FLT_EPSILON || delY < FLT_EPSILON )
      {
        ; //do nothing loop will complete
      }
      else
      {
        // Otherwise complete the loop to maintain polygon normal. This will cause
        // new points to be inserted, not sure this should be done.
        //
        // Get the bounds of the partial loop; gather info for normal computation
        // double n[3];
        // double bds[6] = {VTK_FLOAT_MAX,VTK_FLOAT_MIN, VTK_FLOAT_MAX,VTK_FLOAT_MIN,
        //                  VTK_FLOAT_MAX,VTK_FLOAT_MIN};
        // vtkIdType *pts = new vtkIdType [num];
        // for (int i=0; i < num; ++i)
        // {
        //   pts[i] = sortedPoints[i].Id;
        //   inPts->GetPoint(sortedPoints[i].Id, x);
        //   for (int j=0; j < 3; ++j)
        //   {
        //     bds[2*i] = (x[j] < bds[2*j] ? x[j] : bds[2*j]);
        //     bds[2*i+1] = (x[j] > bds[2*j+1] ? x[j] : bds[2*j+1]);
        //   }
        // }

        // // Compute the normal to the partial loop
        // vtkPolygon::ComputeNormal(inPts,num,pts,n);

        // For now this situation is skipped and no
        // loop is returned.
        return;
      }
    }

    // Return if not a valid loop
    if ( num < 3 )
    {
      return;
    }

    // Output the loop
    outPolys->InsertNextCell(num);
    for (vtkIdType i=0; i < num; ++i)
    {
      outPolys->InsertCellPoint(sortedPoints[i].Id);
    }
  }

} //anonymous namespace

//----------------------------------------------------------------------------
// Instantiate object with empty loop.
vtkContourLoopExtraction::vtkContourLoopExtraction()
{
  this->LoopClosure = VTK_LOOP_CLOSURE_BOUNDARY;
  this->ScalarThresholding = false;

  this->ScalarRange[0] = 0.0;
  this->ScalarRange[1] = 1.0;

  this->Normal[0] = 0.0;
  this->Normal[1] = 0.0;
  this->Normal[2] = 1.0;
}

//----------------------------------------------------------------------------
vtkContourLoopExtraction::~vtkContourLoopExtraction()
{
}

//----------------------------------------------------------------------------
int vtkContourLoopExtraction::RequestData(
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

  // Initialize and check data
  vtkDebugMacro(<<"Loop extraction...");

  vtkPoints *points = input->GetPoints();
  vtkIdType numPts;
  if ( !points || (numPts = input->GetNumberOfPoints()) < 1 )
  {
    vtkErrorMacro("Input contains no points");
    return 1;
  }

  vtkCellArray *lines = input->GetLines();
  vtkIdType numLines = lines->GetNumberOfCells();
  if ( numLines < 1 )
  {
    vtkErrorMacro("Input contains no lines");
    return 1;
  }

  vtkPointData *inPD = input->GetPointData();

  vtkDataArray *scalars = NULL;
  if ( this->ScalarThresholding )
  {
    scalars = inPD->GetScalars();
  }

  // Prepare output
  output->SetPoints(points);
  vtkCellArray *outPolys = vtkCellArray::New();
  output->SetPolys(outPolys);
  output->GetPointData()->PassData(inPD);

  // Create a clean polydata containing only line segments and without other
  // topological types. This simplifies the filter.
  vtkIdType npts, *pts, lineId;
  vtkCellArray *newLines = vtkCellArray::New();
  newLines->Allocate(numLines,2);
  for ( lineId=0, lines->InitTraversal(); lines->GetNextCell(npts,pts); ++lineId)
  {
    for ( int i=0; i < (npts-1); ++i)
    {
      newLines->InsertNextCell(2,pts+i);
    }
  }
  vtkPolyData *polyData = vtkPolyData::New();
  polyData->SetPoints(points);
  polyData->SetLines(newLines);
  polyData->GetPointData()->PassData(inPD);
  polyData->BuildLinks();

  // Keep track of what cells are visited
  numLines = newLines->GetNumberOfCells();
  char *visited = new char[numLines];
  std::fill_n(visited, numLines, 0);

  // Loop over all lines, visit each one. Build a loop from the seed line if
  // not visited.
  vtkIdType start, rightEnd;
  LoopPointType sortedPoints;
  double range[2];
  for ( lineId=0, lines->InitTraversal(); lines->GetNextCell(npts,pts); ++lineId)
  {
    if ( ! visited[lineId] )
    {
      visited[lineId] = 1;
      start = pts[0];
      sortedPoints.clear();
      sortedPoints.push_back(LoopPoint(0.0,start));
      range[0] = VTK_FLOAT_MAX;
      range[1] = VTK_FLOAT_MIN;
      UpdateRange(scalars,start,range);

      rightEnd = TraverseLoop(1.0,polyData,lineId,start,sortedPoints,
                              visited,scalars,range);
      if ( rightEnd == start )
      {
        // extract loop, we've traversed all the way around
        if ( !scalars ||
             (range[0] <= this->ScalarRange[1] && range[1] >= this->ScalarRange[0]) )
        {
          OutputPolygon(sortedPoints,points,outPolys,this->LoopClosure);
        }
      }
      else
      {
        //go the other direction and see where we end up
        TraverseLoop(-1.0,polyData,lineId,start,sortedPoints,
                     visited,scalars,range);
        std::sort(sortedPoints.begin(), sortedPoints.end(), &PointSorter);
        OutputPolygon(sortedPoints,points,outPolys,this->LoopClosure);
      }
    }//if not visited start a loop
  }

  vtkDebugMacro(<< "Generated " << outPolys->GetNumberOfCells()
                << " polygons\n");

  // Clean up
  newLines->Delete();
  outPolys->Delete();
  polyData->Delete();
  delete [] visited;

  return 1;
}

//----------------------------------------------------------------------------
const char *vtkContourLoopExtraction::
GetLoopClosureAsString(void)
{
  if ( this->LoopClosure == VTK_LOOP_CLOSURE_OFF )
  {
    return "LoopClosureOff";
  }
  else if ( this->LoopClosure == VTK_LOOP_CLOSURE_BOUNDARY )
  {
    return "LoopClosureBoundary";
  }
  else
  {
    return "LoopClosureAll";
  }
}

//----------------------------------------------------------------------------
void vtkContourLoopExtraction::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Loop Closure: ";
  os << this->GetLoopClosureAsString() << "\n";

  os << indent << "Scalar Thresholding: "
     << (this->ScalarThresholding ? "On\n" : "Off\n");

  double *range = this->GetScalarRange();
  os << indent << "Scalar Range: (" << range[0] << ", " << range[1] << ")\n";

  double *n = this->GetNormal();
  os << indent << "Normal: (" << n[0] << ", " << n[1] << ", " << n[2] << ")\n";

}
