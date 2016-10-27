/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAdaptiveSubdivisionFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkAdaptiveSubdivisionFilter.h"

#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkTriangle.h"
#include "vtkPolyData.h"
#include "vtkCellArray.h"
#include "vtkPointData.h"
#include "vtkCellData.h"
#include "vtkMergePoints.h"

vtkStandardNewMacro(vtkAdaptiveSubdivisionFilter);
vtkCxxSetObjectMacro(vtkAdaptiveSubdivisionFilter,Locator,vtkIncrementalPointLocator);

//----------------------------------------------------------------------------
// Construct object
vtkAdaptiveSubdivisionFilter::vtkAdaptiveSubdivisionFilter()
{
  this->MaximumEdgeLength = 1.0;
  this->MaximumTriangleArea = 1.0;
  this->MaximumNumberOfTriangles = VTK_ID_MAX;
  this->MaximumNumberOfPasses = VTK_ID_MAX;
  this->Locator = NULL;
  this->OutputPointsPrecision = DEFAULT_PRECISION;
}

//----------------------------------------------------------------------------
// Construct object with number of subdivisions set to 1.
vtkAdaptiveSubdivisionFilter::~vtkAdaptiveSubdivisionFilter()
{
  this->SetLocator(NULL);
}

//-----------------------------------------------------------------------------
void vtkAdaptiveSubdivisionFilter::CreateDefaultLocator()
{
  if ( this->Locator == NULL )
  {
    this->Locator = vtkMergePoints::New();
    this->Locator->Register(this);
    this->Locator->Delete();
  }
}

//-----------------------------------------------------------------------------
// Overload standard modified time function.
vtkMTimeType vtkAdaptiveSubdivisionFilter::GetMTime()
{
  vtkMTimeType mTime=this->Superclass::GetMTime();
  vtkMTimeType time;

  if (this->Locator)
  {
    time = this->Locator->GetMTime();
    mTime = ( time > mTime ? time : mTime );
  }

  return mTime;
}

// Helper functions and structures
namespace {

  // There are eight possible subdivision cases (each of the three edges may
  // or may not be subdivided). Case 0 just outputs the original triangle;
  // the other cases output between 2 and four triangles. Note that when
  // three triangles are generated, then the diagonal of the quadrilateral
  // produced can go one of two ways. The tetCases is set up so that the two
  // triangles forming the quad are the last two triangles and can be
  // adjusted as necessary.
  int CASE_MASK[3] = {1,2,4};
  vtkIdType tessCases[16][13] = {
    {1, 0,1,2, 0,0,0, 0,0,0, 0,0,0}, //case 0
    {2, 0,3,2, 3,1,2, 0,0,0, 0,0,0}, //case 1
    {2, 0,1,4, 4,2,0, 0,0,0, 0,0,0}, //case 2
    {3, 3,1,4, 3,4,2, 2,0,3, 0,0,0}, //case 3
    {2, 0,1,5, 5,1,2, 0,0,0, 0,0,0}, //case 4
    {3, 0,3,5, 5,3,1, 1,2,5, 0,0,0}, //case 5
    {3, 5,4,2, 0,1,4, 4,5,0, 0,0,0}, //case 6
    {4, 0,3,5, 3,1,4, 5,3,4, 5,4,2}, //case 7
    {1, 0,1,2, 0,0,0, 0,0,0, 0,0,0}, //case 0a
    {2, 0,3,2, 3,1,2, 0,0,0, 0,0,0}, //case 1a
    {2, 0,1,4, 4,2,0, 0,0,0, 0,0,0}, //case 2a
    {3, 3,1,4, 0,3,4, 4,2,0, 0,0,0}, //case 3a
    {2, 0,1,5, 5,1,2, 0,0,0, 0,0,0}, //case 4a
    {3, 0,3,5, 3,1,2, 2,5,3, 0,0,0}, //case 5a
    {3, 4,2,5, 5,0,1, 1,4,5, 0,0,0}, //case 6a
    {4, 0,3,5, 3,1,4, 5,3,4, 5,4,2}, //case 7a
  };

  // This method assumes that the diagonal of the quadrilateral formed by
  // triangles 2 & 3 may be "swapped" to produce a better triangulation. It
  // assumes a lot about the ordering of the connectivity array (subTess).
  vtkIdType *SelectTessellation(unsigned char subCase, vtkIdType *ptIds,
                                vtkPoints *newPts)
  {
    // If no choice in triangulation return the table entry
    if ( tessCases[subCase][0] != 3 )
    {
      return tessCases[subCase];
    }

    // Else select best triangulation based on diagonal length
    vtkIdType *subTess = tessCases[subCase];
    double x0[3], x1[3], x2[3], x3[3];
    newPts->GetPoint(ptIds[subTess[4]], x0);
    newPts->GetPoint(ptIds[subTess[6]], x1);
    newPts->GetPoint(ptIds[subTess[5]], x2);
    newPts->GetPoint(ptIds[subTess[8]], x3);

    if ( vtkMath::Distance2BetweenPoints(x0,x1) <=
         vtkMath::Distance2BetweenPoints(x2,x3) )
    {
      return tessCases[subCase];
    }
    else
    {
      return tessCases[subCase + 8]; //alternate triangulation
    }
  }

}//anonymous namespace


//----------------------------------------------------------------------------
// This uses a very simple, serial implementation that makes repeated passes
// over the triangles using a swap buffer approach.
int vtkAdaptiveSubdivisionFilter::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output and check its validity
  vtkPolyData *input = vtkPolyData::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkIdType numPts = input->GetNumberOfPoints();
  vtkCellArray *inTris = input->GetPolys();
  vtkIdType numTris = inTris->GetNumberOfCells();
  if (numPts < 1 || numTris < 1)
  {
    vtkDebugMacro(<<"No data to subdivide!");
    return 1;
  }
  vtkPointData *inPointData = input->GetPointData();
  vtkCellData *inCellData = input->GetCellData();

  // This is a quick check that all cells are a triangle. It is not foolproof
  // however.... it may be necessary to tighten this up at some point.
  vtkIdType connLen = inTris->GetNumberOfConnectivityEntries();
  if ( (connLen / 4) != numTris )
  {
    vtkDebugMacro(<<"Filter operates only on triangles!");
    return 1;
  }

  // Need a locator
  if ( ! this->Locator )
  {
    this->CreateDefaultLocator();
  }

  // The first thing is to take the existing points and push them into the
  // incremental point locator. We know that we are going to use the original
  // points. Note that points are only created and are not swapped as each
  // pass is invoked.
  vtkPoints *inPts = input->GetPoints();
  vtkPoints *newPts = vtkPoints::New();
  vtkPointData *swapPointData, *newPointData = vtkPointData::New();
  newPointData->CopyAllocate(inPointData);

  // set precision for the points in the output
  if ( this->OutputPointsPrecision == vtkAlgorithm::DEFAULT_PRECISION )
  {
    newPts->SetDataType(inPts->GetDataType());
  }
  else if (this->OutputPointsPrecision == vtkAlgorithm::SINGLE_PRECISION)
  {
    newPts->SetDataType(VTK_FLOAT);
  }
  else if(this->OutputPointsPrecision == vtkAlgorithm::DOUBLE_PRECISION)
  {
    newPts->SetDataType(VTK_DOUBLE);
  }
  this->Locator->InitPointInsertion (newPts,
                                     input->GetBounds(),
                                     input->GetNumberOfPoints());
  // Load in the already existing points. Also load in the point data
  // associated with the existing points.
  for (vtkIdType ptId=0; ptId < numPts; ++ptId)
  {
    this->Locator->InsertNextPoint(inPts->GetPoint(ptId));
    newPointData->CopyData(inPointData, ptId, ptId);
  }

  // This is a multipass algorithm. From a list of triangles, check each
  // against the edge length and area criteria. If necessary, break the
  // triangle (using a case table) into smaller triangles by inserting one or
  // more points on edges (the edge is broken at its midpoint). The new
  // triangles are placed into a new list which serves as the starting point
  // for the next pass. An important note: triangles are split independently
  // without neighbor "links" (i.e.,cell links) and new points are merged
  // into the locator. Since the algorithm treats edges on triangles in an
  // identical way, the end result is that triangle neighbors remain
  // compatible (due to conincident point merging).
  vtkIdType *currTris = inTris->GetPointer();
  vtkCellArray *swapTris, *newTris = vtkCellArray::New();
  newTris->Allocate(inTris->EstimateSize(2*numTris,3), numTris);
  vtkCellData *swapCellData, *newCellData = vtkCellData::New();
  newCellData->CopyAllocate(inCellData);

  int i;
  double area, eLengths[3];
  double maxLen2=this->MaximumEdgeLength*this->MaximumEdgeLength;
  double maxArea=this->MaximumTriangleArea;
  double x[6][3]; //three vertices plus potential mid-edge points
  vtkIdType *tri, triId, newId;
  vtkIdType passNum;
  vtkIdType totalTriangles=0;
  bool changesMade;

  for ( passNum=0, changesMade=true;
        passNum < this->MaximumNumberOfPasses && totalTriangles < this->MaximumNumberOfTriangles && changesMade;
        ++passNum )
  {
    changesMade = false;
    for (triId=0; triId < numTris; ++triId)
    {
      tri = currTris + 4*triId + 1; //get point ids defining triangle
      newPts->GetPoint(tri[0],x[0]);
      newPts->GetPoint(tri[1],x[1]);
      newPts->GetPoint(tri[2],x[2]);
      eLengths[0] = vtkMath::Distance2BetweenPoints(x[0],x[1]);
      eLengths[1] = vtkMath::Distance2BetweenPoints(x[1],x[2]);
      eLengths[2] = vtkMath::Distance2BetweenPoints(x[2],x[0]);
      area = vtkTriangle::TriangleArea(x[0],x[1],x[2]);

      // Various subdivision cases are possible
      unsigned char subCase=0;
      if ( area > maxArea )
      {
        subCase = 7;
      }
      else
      {
        for (i=0; i<3; ++i)
        {
          if ( eLengths[i] > maxLen2 )
          {
            subCase |= CASE_MASK[i];
          }
        }
      }//determine edges to divide

      // If not just outputting original triangle then changes are made
      if (subCase > 0 )
      {
        changesMade = true;
      }

      // Now create new points and triangles dividing edges as appropriate.
      double xNew[3];
      vtkIdType ptIds[6];
      ptIds[0] = tri[0]; ptIds[1] = tri[1]; ptIds[2] = tri[2];
      for (i=0; i<3; ++i)
      {
        if ( subCase & CASE_MASK[i] ) //ith edge needs subdivision
        {
          xNew[0] = 0.5*(x[i][0] + x[(i+1)%3][0]);
          xNew[1] = 0.5*(x[i][1] + x[(i+1)%3][1]);
          xNew[2] = 0.5*(x[i][2] + x[(i+1)%3][2]);
          if ( (ptIds[3+i] = this->Locator->IsInsertedPoint(xNew)) < 0 )
          {
            ptIds[3+i] = this->Locator->InsertNextPoint(xNew);
            newPointData->InterpolateEdge(inPointData, ptIds[3+i], tri[i], tri[(i+1)%3], 0.5);
          }
        }
      }

      // The tessellation may vary based on geometric concerns (selecting best
      // diagonal during triangulation of quadrilateral)
      vtkIdType newTIds[3], *subTess;
      subTess = SelectTessellation(subCase,ptIds,newPts);
      vtkIdType numTessTris = *subTess++;

      for (i=0; i<numTessTris; ++i, subTess+=3)
      {
        newTIds[0] = ptIds[subTess[0]];
        newTIds[1] = ptIds[subTess[1]];
        newTIds[2] = ptIds[subTess[2]];
        newId = newTris->InsertNextCell(3,newTIds);
        newCellData->CopyData(inCellData, triId, newId);
        if ( ++totalTriangles >= this->MaximumNumberOfTriangles )
        {
          break;
        }
      }
    }//for all triangles in this pass

    // Prepare for the next pass, which means swapping input and output.
    // Remember that the initial pass uses the filter input; subsequent passes
    // cannot modify the input to a new cell array must be created to support
    // the swapping.
    if ( passNum == 0 )
    {
      inTris = vtkCellArray::New();
      inCellData = vtkCellData::New();
      inCellData->CopyAllocate(newCellData);

      inPointData = vtkPointData::New();
      inPointData->CopyAllocate(newPointData);
    }

    // Prepare for new triangles
    swapTris = newTris;
    newTris = inTris;
    inTris = swapTris;
    currTris = inTris->GetPointer();

    numTris = inTris->GetNumberOfCells();
    newTris->Reset();
    newTris->Allocate(inTris->EstimateSize(2*newTris->GetNumberOfCells(),3),
                     newTris->GetNumberOfCells());

    // Prepare for new cell data
    swapCellData = newCellData;
    newCellData = inCellData;
    inCellData = swapCellData;

    // Prepare for new point data
    numPts = newPts->GetNumberOfPoints();
    swapPointData = newPointData;
    newPointData = inPointData;
    inPointData = swapPointData;
    for (vtkIdType ptId=0; ptId < numPts; ++ptId)
    {
      newPointData->CopyData(inPointData, ptId, ptId);
    }
  }//for another pass

  // Configure output and clean up
  output->SetPoints(newPts);
  newPts->Delete();
  output->GetPointData()->ShallowCopy(inPointData);
  newPointData->Delete();

  output->SetPolys(inTris);
  inTris->Delete();
  newTris->Delete();
  output->GetCellData()->ShallowCopy(inCellData);
  inCellData->Delete();
  inPointData->Delete();
  newCellData->Delete();

  return 1;
}

//----------------------------------------------------------------------------
void vtkAdaptiveSubdivisionFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Maximum Edge Length: " << this->MaximumEdgeLength << "\n";
  os << indent << "Maximum Triangle Area: " << this->MaximumTriangleArea << "\n";
  os << indent << "Maximum Number Of Triangles: " << this->MaximumNumberOfTriangles << "\n";
  os << indent << "Maximum Number Of Passes: " << this->MaximumNumberOfPasses << "\n";

  if ( this->Locator )
  {
    os << indent << "Locator: " << this->Locator << "\n";
  }
  else
  {
    os << indent << "Locator: (none)\n";
  }

  os << indent << "Precision of the output points: "
     << this->OutputPointsPrecision << "\n";
}
