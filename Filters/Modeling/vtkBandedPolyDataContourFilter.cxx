/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBandedPolyDataContourFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkBandedPolyDataContourFilter.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkEdgeTable.h"
#include "vtkExecutive.h"
#include "vtkFloatArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkTriangleStrip.h"
#include "vtkDoubleArray.h"

#include <float.h>

vtkStandardNewMacro(vtkBandedPolyDataContourFilter);

//------------------------------------------------------------------------------
// Construct object.
vtkBandedPolyDataContourFilter::vtkBandedPolyDataContourFilter()
{
  this->ContourValues = vtkContourValues::New();
  this->Clipping = 0;
  this->ScalarMode = VTK_SCALAR_MODE_INDEX;

  this->SetNumberOfOutputPorts(2);

  vtkPolyData *output2 = vtkPolyData::New();
  this->GetExecutive()->SetOutputData(1, output2);
  output2->Delete();
  this->ClipTolerance = FLT_EPSILON;
  this->InternalClipTolerance = FLT_EPSILON;
  this->GenerateContourEdges = 0;
}

//------------------------------------------------------------------------------
vtkBandedPolyDataContourFilter::~vtkBandedPolyDataContourFilter()
{
  this->ContourValues->Delete();
}

//------------------------------------------------------------------------------
int vtkBandedPolyDataContourFilter::ComputeScalarIndex(double val)
{

  for (int i=0; i < (this->NumberOfClipValues-1); i++)
    {
    if ( val >= this->ClipValues[i] && val < this->ClipValues[i+1]  )
      {
      return i;
      }
    }
  return this->NumberOfClipValues - 1;

}

//------------------------------------------------------------------------------
int vtkBandedPolyDataContourFilter::IsContourValue(double val)
{
  int i;

  // Check to see whether a vertex is an intersection point.
  for ( i=0; i < this->NumberOfClipValues; i++)
    {
    if ( val == this->ClipValues[i] )
      {
      return 1;
      }
    }
  return 0;
}

//------------------------------------------------------------------------------
// Return a flag that indicates that the ordering of vertices along the
// edge is not from v1->v2, where v1 < v2.
int vtkBandedPolyDataContourFilter::ClipEdge(int v1, int v2,
                                             vtkPoints *newPts,
                                             vtkDataArray *inScalars,
                                             vtkDoubleArray *outScalars,
                                             vtkPointData *inPD,
                                             vtkPointData *outPD)
{
  double x[3], t, sNew;
  double x1[3], x2[3];
  int ptId;
  int reverse = (v1 < v2 ? 0 : 1);

  newPts->GetPoint(v1, x1);
  newPts->GetPoint(v2, x2);

  double s1 = inScalars->GetTuple1(v1);
  double s2 = inScalars->GetTuple1(v2);

  if ( s1 <= s2 )
    {
    int idx1 = this->ComputeScalarIndex(s1);
    int idx2 = this->ComputeScalarIndex(s2);

    for (int i=1; i < (idx2-idx1+1); i++)
      {
      t = (this->ClipValues[idx1+i] - s1) / (s2 - s1);
      x[0] = x1[0] + t*(x2[0]-x1[0]);
      x[1] = x1[1] + t*(x2[1]-x1[1]);
      x[2] = x1[2] + t*(x2[2]-x1[2]);
      ptId = newPts->InsertNextPoint(x);
      outPD->InterpolateEdge(inPD,ptId,v1,v2,t);
      // We cannot use directly s1 + t*(s2-s1) as is causes rounding error
      sNew = this->ClipValues[idx1+i];
      outScalars->InsertTuple1(ptId,sNew);
      }
    return reverse;
    }
  else
    {
    int idx1 = this->ComputeScalarIndex(s1);
    int idx2 = this->ComputeScalarIndex(s2);

    for (int i=1; i < (idx1-idx2+1); i++)
      {
      t = (this->ClipValues[idx2+i] - s1) / (s2 - s1);
      x[0] = x1[0] + t*(x2[0]-x1[0]);
      x[1] = x1[1] + t*(x2[1]-x1[1]);
      x[2] = x1[2] + t*(x2[2]-x1[2]);
      ptId = newPts->InsertNextPoint(x);
      outPD->InterpolateEdge(inPD,ptId,v1,v2,t);
      // We cannot use directly s1 + t*(s2-s1) as is causes rounding error
      sNew = this->ClipValues[idx2+i];
      outScalars->InsertTuple1(ptId,sNew);
      }
    return ((reverse+1) % 2);
    }
}


//------------------------------------------------------------------------------
extern "C" {
static int vtkCompareClipValues(const void *val1, const void *val2)
{
  if ( *((double*)val1) < *((double*)val2) )
    {
    return (-1);
    }
  else if ( *((double*)val1) > *((double*)val2) )
    {
    return (1);
    }
  else
    {
    return (0);
    }
}
}

//------------------------------------------------------------------------------
inline int vtkBandedPolyDataContourFilter::InsertCell(vtkCellArray *cells,
                                                      int npts, vtkIdType *pts,
                                                      int cellId, double s,
                                                      vtkFloatArray *newS)
{
  int idx = this->ComputeScalarIndex(s+this->InternalClipTolerance);

  if ( !this->Clipping ||
       (idx >= this->ClipIndex[0] && idx < this->ClipIndex[1]) )
    {
    cells->InsertNextCell(npts,pts);

    if ( this->ScalarMode == VTK_SCALAR_MODE_INDEX )
      {
      newS->InsertTuple1(cellId++,idx);
      }
    else
      {
      newS->InsertTuple1(cellId++,this->ClipValues[idx]);
      }
    }
  return cellId;
}


//------------------------------------------------------------------------------
// Create filled contours for polydata
int vtkBandedPolyDataContourFilter::RequestData(
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

  vtkPointData *pd = input->GetPointData();
  vtkPointData *outPD = output->GetPointData();
  vtkCellData *outCD = output->GetCellData();
  vtkPoints *inPts = input->GetPoints();
  vtkDataArray *inScalars = pd->GetScalars();
  int abort=0;
  vtkPoints *newPts;
  int i, j, idx=0;
  vtkIdType npts = 0;
  vtkIdType cellId=0;
  vtkIdType *pts = 0;
  int numEdgePts, numNewPts, maxCellSize;
  vtkIdType v, vR, *intPts;
  int intsIdx, reverse;
  vtkIdType intLoc;
  vtkIdType numIntPts, intsInc;
  vtkIdType numPts, numCells, estimatedSize;

  vtkDebugMacro(<<"Executing banded contour filter");

  //  Check input
  //

  numCells = input->GetNumberOfCells();
  if ( !inPts || (numPts=inPts->GetNumberOfPoints()) < 1 ||
       !inScalars || numCells < 1 )
    {
    vtkErrorMacro(<<"No input data!");
    return 1;
    }

  // Set up supplemental data structures for processing edge/generating
  // intersections. First we sort the contour values into an ascending
  // list of clip values including the extreme min/max values.
  this->NumberOfClipValues = this->ContourValues->GetNumberOfContours() + 2;
  this->ClipValues = new double[this->NumberOfClipValues];
  double range[2];
  inScalars->GetRange(range);

  // base clip tolerance on overall input scalar range
  this->InternalClipTolerance = this->ClipTolerance*(range[1] - range[0]);

  this->ClipValues[0] =
    (range[0]<this->ContourValues->GetValue(0))?
    (range[0]):
    (this->ContourValues->GetValue(0));

  this->ClipValues[this->NumberOfClipValues - 1] =
    (range[1]>this->ContourValues->GetValue(this->NumberOfClipValues-2-1))?
    (range[1]):
    (this->ContourValues->GetValue(this->NumberOfClipValues-2-1));

  for ( i=1; i<this->NumberOfClipValues-1; i++)
    {
    this->ClipValues[i] = this->ContourValues->GetValue(i-1);
    }

  qsort(this->ClipValues, this->NumberOfClipValues, sizeof(double),
        vtkCompareClipValues);

  // toss out values which are too close together, currently within FLT_EPSILON%
  // of each other based on full scalar range, but could define temporary based
  // on percentage of scalar range...
  for ( i=0; i<(this->NumberOfClipValues-1); i++)
    {
    if ( (this->ClipValues[i] + this->InternalClipTolerance) >= this->ClipValues[i+1] )
      {
      for (j=i+1; j<(this->NumberOfClipValues-2); j++)
        {
        this->ClipValues[j] = this->ClipValues[j+1];
        }
      this->NumberOfClipValues--;
      }
    }

  this->ClipIndex[0] =
    this->ComputeScalarIndex(this->ContourValues->GetValue(0));
  this->ClipIndex[1] = this->ComputeScalarIndex(
    this->ContourValues->GetValue(this->ContourValues->GetNumberOfContours()-1));

  //
  // Estimate allocation size, stolen from vtkContourGrid...
  //
  estimatedSize=static_cast<vtkIdType>(pow(static_cast<double>(numCells),.9));
  estimatedSize *= this->NumberOfClipValues;
  estimatedSize = estimatedSize / 1024 * 1024; // multiple of 1024
  if (estimatedSize < 1024)
    {
    estimatedSize = 1024;
    }

  // The original set of points and point data are copied. Later on
  // intersection points due to clipping will be created.
  newPts = vtkPoints::New();

  // Note: since we use the output scalars in the execution of the algorithm,
  // the output point scalars MUST BE double or bad things happen due to
  // numerical precision issues.
  newPts->Allocate(estimatedSize,estimatedSize);
  outPD->CopyScalarsOff();
  outPD->InterpolateAllocate(pd,3*numPts,numPts);
  vtkDoubleArray *outScalars = vtkDoubleArray::New();
  outScalars->Allocate(3*numPts,numPts);
  outPD->SetScalars(outScalars);
  outScalars->Delete();

  for (i=0; i<numPts; i++)
    {
    newPts->InsertPoint(i,inPts->GetPoint(i));
    outPD->CopyData(pd, i, i);
    outScalars->InsertTuple1(i, inScalars->GetTuple1(i));
    }

  // These are the new cell scalars
  vtkFloatArray *newScalars = vtkFloatArray::New();
  newScalars->Allocate(numCells*5,numCells);
  newScalars->SetName("Scalars");

  // Used to keep track of intersections
  vtkEdgeTable *edgeTable = vtkEdgeTable::New();
  vtkCellArray *intList = vtkCellArray::New(); //intersection point ids

  // All vertices are filled and passed through; poly-vertices are broken
  // into single vertices. Cell data per vertex is set.
  //
  if ( input->GetVerts()->GetNumberOfCells() > 0 )
    {
    vtkCellArray *verts = input->GetVerts();
    vtkCellArray *newVerts = vtkCellArray::New();
    newVerts->Allocate(verts->GetSize());
    for ( verts->InitTraversal(); verts->GetNextCell(npts,pts) && !abort;
          abort=this->GetAbortExecute() )
      {
      for (i=0; i<npts; i++)
        {
        newVerts->InsertNextCell(1,pts+i);
        idx = this->ComputeScalarIndex(inScalars->GetTuple1(pts[i]));
        newScalars->InsertTuple1(cellId++,idx);
        }
      }
    output->SetVerts(newVerts);
    newVerts->Delete();
    }
  this->UpdateProgress(0.05);

  // Lines are chopped into line segments.
  //
  if ( input->GetLines()->GetNumberOfCells() > 0 )
    {
    vtkCellArray *lines = input->GetLines();

    maxCellSize = lines->GetMaxCellSize();
    maxCellSize *= (1 + this->NumberOfClipValues);

    vtkIdType *fullLine = new vtkIdType [maxCellSize];
    vtkCellArray *newLines = vtkCellArray::New();
    newLines->Allocate(lines->GetSize());
    edgeTable->InitEdgeInsertion(numPts,1); //store attributes on edge

    //start by generating intersection points
    for ( lines->InitTraversal(); lines->GetNextCell(npts,pts) && !abort;
          abort=this->GetAbortExecute() )
      {
      for (i=0; i<(npts-1); i++)
        {
        numNewPts = newPts->GetNumberOfPoints();
        reverse = this->ClipEdge(pts[i],pts[i+1],newPts,inScalars,outScalars,
                                 pd,outPD);
        numEdgePts = newPts->GetNumberOfPoints() - numNewPts;
        if ( numEdgePts > 0 ) //there is an intersection
          {
          if ( !reverse )
            {
            for (j=0; j<numEdgePts; j++) fullLine[j] = numNewPts + j;
            }
          else
            {
            for (j=0; j<numEdgePts; j++)
              fullLine[j] = numNewPts + numEdgePts - j - 1;
            }
          intList->InsertNextCell(numEdgePts,fullLine);
          edgeTable->InsertEdge(pts[i],pts[i+1], //associate ints with edge
                                intList->GetInsertLocation(numEdgePts));
          }
        else //no intersection points along the edge
          {
          edgeTable->InsertEdge(pts[i],pts[i+1],-1); //-1 means no points
          }
        }//for all line segments in this line
      }

    //now create line segments
    for ( lines->InitTraversal(); lines->GetNextCell(npts,pts) && !abort;
          abort=this->GetAbortExecute() )
      {
      for (i=0; i<(npts-1); i++)
        {
        v = pts[i];
        vR = pts[i+1];

        newLines->InsertNextCell(2);

        newScalars->InsertTuple1(cellId++,
                    this->ComputeScalarIndex(outScalars->GetTuple1(v)));
        newLines->InsertCellPoint(v);

        if ( (intLoc=edgeTable->IsEdge(v,vR)) != -1 )
          {
          intList->GetCell(intLoc,numIntPts,intPts);
          if ( v < vR ) {intsIdx = 0; intsInc=1;} //order of the edge
          else {intsIdx=numIntPts-1; intsInc=(-1);}

          for ( ; intsIdx >= 0 && intsIdx < numIntPts; intsIdx += intsInc )
            {
            newLines->InsertCellPoint(intPts[intsIdx]);
            newLines->InsertNextCell(2);

            newScalars->InsertTuple1(cellId++, this->ComputeScalarIndex(
              outScalars->GetTuple1(intPts[intsIdx])));
            newLines->InsertCellPoint(intPts[intsIdx]);
            }
          }
        newLines->InsertCellPoint(vR);
        }
      }

    delete [] fullLine;

    output->SetLines(newLines);
    newLines->Delete();
    }
  this->UpdateProgress(0.1);

  // Polygons are assumed convex and chopped into filled, convex polygons.
  // Triangle strips are treated similarly.
  //
  int numPolys = input->GetPolys()->GetNumberOfCells();
  int numStrips = input->GetStrips()->GetNumberOfCells();
  if ( numPolys > 0 || numStrips > 0 )
    {
    // Set up processing. We are going to store an ordered list of
    // intersections along each edge (ordered from smallest point id
    // to largest). These will later be connected into convex polygons
    // which represent a filled region in the cell.
    //
    edgeTable->InitEdgeInsertion(numPts,1); //store attributes on edge
    intList->Reset();

    vtkCellArray *polys = input->GetPolys();
    vtkCellArray *tmpPolys = NULL;

    // If contour edges requested, set things up.
    vtkCellArray *contourEdges=0;
    if ( this->GenerateContourEdges )
      {
      contourEdges = vtkCellArray::New();
      contourEdges->Allocate(numCells);
      this->GetContourEdgesOutput()->SetLines(contourEdges);
      contourEdges->Delete();
      this->GetContourEdgesOutput()->SetPoints(newPts);
      }

    // Set up structures for processing polygons
    maxCellSize = polys->GetMaxCellSize();
    if( maxCellSize == 0 )
      {
      maxCellSize = input->GetStrips()->GetMaxCellSize();
      }
    maxCellSize *= (1 + this->NumberOfClipValues);

    vtkIdType *newPolygon = new vtkIdType [maxCellSize];
    double *s = new double [maxCellSize]; //scalars at vertices
    int *isContourValue = new int [maxCellSize];
    int *isOriginalVertex = new int [maxCellSize];
    vtkIdType *fullPoly = new vtkIdType [maxCellSize];

    // Lump strips and polygons together.
    // Decompose strips into triangles.
    if ( numStrips > 0 )
      {
      vtkCellArray *strips = input->GetStrips();
      tmpPolys = vtkCellArray::New();
      if ( numPolys > 0 )
        {
        tmpPolys->DeepCopy(polys);
        }
      else
        {
        tmpPolys->Allocate(polys->EstimateSize(numStrips,5));
        }
      for ( strips->InitTraversal(); strips->GetNextCell(npts,pts); )
        {
        vtkTriangleStrip::DecomposeStrip(npts, pts, tmpPolys);
        }
      polys = tmpPolys;
      }

    // Process polygons to produce edge intersections.------------------------
    //
    numPolys = polys->GetNumberOfCells();
    vtkIdType updateCount = numPolys/20 + 1;
    vtkIdType count=0;
    for ( polys->InitTraversal(); polys->GetNextCell(npts,pts) && !abort;
      abort=this->GetAbortExecute() )
      {
      if  ( ! (++count % updateCount) )
        {
        this->UpdateProgress(0.1 + 0.45*(static_cast<double>(count)/numPolys));
        }

      for (i=0; i<npts; i++)
        {
        v = pts[i];
        vR = pts[(i+1) % npts];
        if ( edgeTable->IsEdge(v,vR) == -1 )
          {
          numNewPts = newPts->GetNumberOfPoints();
          reverse = this->ClipEdge(v,vR,newPts,inScalars,outScalars,pd,outPD);
          numEdgePts = newPts->GetNumberOfPoints() - numNewPts;
          if ( numEdgePts > 0 )
            {
            if ( !reverse )
              {
              for (j=0; j<numEdgePts; j++) fullPoly[j] = numNewPts + j;
              }
            else
              {
              for (j=0; j<numEdgePts; j++)
                fullPoly[j] = numNewPts + numEdgePts - j - 1;
              }
            intList->InsertNextCell(numEdgePts,fullPoly);
            edgeTable->InsertEdge(v,vR, //associate ints with edge
                                  intList->GetInsertLocation(numEdgePts));
            }
          else //no intersection points along the edge
            {
            edgeTable->InsertEdge(v,vR,-1); //-1 means no points
            }
          }//if edge not processed yet
        }
      }//for all polygons

    // Process polygons to produce output triangles------------------------
    //
    vtkCellArray *newPolys = vtkCellArray::New();
    newPolys->Allocate(polys->GetSize());
    int intersectionPoint;
    int mL, mR, m2L, m2R;
    int numPointsToAdd, numLeftPointsToAdd, numRightPointsToAdd;
    int numPolyPoints, numFullPts;
    count = 0;
    for ( polys->InitTraversal(); polys->GetNextCell(npts,pts) && !abort;
          abort=this->GetAbortExecute() )
      {
      if  ( ! (++count % updateCount) )
        {
        this->UpdateProgress(0.55 +
                             0.45*(static_cast<double>(count)/numPolys));
        }

      //Create a new polygon that includes all the points including the
      //intersection vertices. This hugely simplifies the logic of the
      //code.
      for ( intersectionPoint=0, numFullPts=0, i=0; i<npts; i++)
        {
        v = pts[i];
        vR = pts[(i+1)%npts];

        s[numFullPts] = outScalars->GetTuple1(v);
        isContourValue[numFullPts] = this->IsContourValue(s[numFullPts]);
        isOriginalVertex[numFullPts] = 1;
        fullPoly[numFullPts++] = v;

        //see whether intersection points need to be added.
        if ( (intLoc=edgeTable->IsEdge(v,vR)) != -1 )
          {
          intersectionPoint = 1;
          intList->GetCell(intLoc,numIntPts,intPts);
          if ( v < vR ) {intsIdx = 0; intsInc=1;} //order of the edge
          else {intsIdx=numIntPts-1; intsInc=(-1);}
          for ( ; intsIdx >= 0 && intsIdx < numIntPts; intsIdx += intsInc )
            {
            s[numFullPts] = outScalars->GetTuple1(intPts[intsIdx]);
            isContourValue[numFullPts] = 1;
            isOriginalVertex[numFullPts] = 0;
            fullPoly[numFullPts++] = intPts[intsIdx];
            }
          }
        } //for all points and edges

      //Very important: have to find the right starting vertex. The vertex
      //needs to be one where the contour values increase in both directions.
      //Really should check whether the vertex is convex.
      double minValue=VTK_DOUBLE_MAX;
      for ( i=0; i<numFullPts; i++)
        {
        if ( isOriginalVertex[i] )
          {
          if ( s[i] < minValue && s[i] <= s[(i+numFullPts-1)%numFullPts] &&
               s[i] <= s[(i+1)%numFullPts] )
            {
            idx = i;
            minValue = s[i];
            }
          }
        }

      //Trivial output - completely in a contour band or a triangle
      if ( ! intersectionPoint || numFullPts == 3 )
        {
        cellId = this->InsertCell(newPolys,npts,pts,cellId,s[idx],newScalars);
        continue;
        }

      //Produce contour edges if requested
      if ( this->GenerateContourEdges )
        {
        for (i=0; i < numFullPts; i++)
          {
          if ( isContourValue[i] && isContourValue[(i+1)%numFullPts] &&
               s[i] == s[(i+1)%numFullPts] )
            {
            contourEdges->InsertNextCell(2);
            contourEdges->InsertCellPoint(fullPoly[i]);
            contourEdges->InsertCellPoint(fullPoly[(i+1)%numFullPts]);
            }
          }
        }

      //Find the first intersection points in the polygons starting
      //from this vertex and build a polygon.
      numPointsToAdd = 1;
      for ( mR=idx, intersectionPoint=0; !intersectionPoint; )
        {
        numPointsToAdd++;
        mR = (mR + 1) % numFullPts;
        if ( isContourValue[mR] && s[mR] != s[idx] ) intersectionPoint = 1;
        }
      for ( mL=idx, intersectionPoint=0; !intersectionPoint; )
        {
        numPointsToAdd++;
        mL = (mL + numFullPts - 1) % numFullPts;
        if ( isContourValue[mL] && s[mL] != s[idx] ) intersectionPoint = 1;
        }
      for ( numPolyPoints=0, i=0; i<numPointsToAdd; i++)
        {
        newPolygon[numPolyPoints++] = fullPoly[(mL+i)%numFullPts];
        }
      if(numPolyPoints >= 3)
        {
        cellId = this->InsertCell(newPolys,numPolyPoints,newPolygon,
                                  cellId,s[idx],newScalars);
        }
      if ( this->GenerateContourEdges )
        {
        contourEdges->InsertNextCell(2);
        contourEdges->InsertCellPoint(fullPoly[mR]);
        contourEdges->InsertCellPoint(fullPoly[mL]);
        }

      //We've got an edge (mL,mR) that marks the edge of the region not yet
      //clipped. We move this edge forward from intersection point to
      //intersection point.
      m2R = mR;
      m2L = mL;
      while ( m2R != m2L )
        {
        numPointsToAdd = (mL > mR ? mL-mR+1 : numFullPts-(mR-mL)+1);
        if ( numPointsToAdd == 3 )
          {//just a triangle left
          for (i=0; i<numPointsToAdd; i++)
            {
            newPolygon[i] = fullPoly[(mR+i)%numFullPts];
            }
          cellId = this->InsertCell(newPolys,numPointsToAdd,newPolygon,
                                    cellId,s[mR],newScalars);
          if ( this->GenerateContourEdges )
            {
            contourEdges->InsertNextCell(2);
            contourEdges->InsertCellPoint(fullPoly[mR]);
            contourEdges->InsertCellPoint(fullPoly[mL]);
            }
          break;
          }
        else //find the next intersection points
          {
          numLeftPointsToAdd = 0;
          numRightPointsToAdd = 0;
          for ( intersectionPoint=0;
                !intersectionPoint && ((m2R+1)%numFullPts) != m2L; )
            {
            numRightPointsToAdd++;
            m2R = (m2R + 1) % numFullPts;
            if ( isContourValue[m2R] ) intersectionPoint = 1;
            }
          for ( intersectionPoint=0;
                !intersectionPoint && ((m2L+numFullPts-1)%numFullPts) != m2R; )
            {
            numLeftPointsToAdd++;
            m2L = (m2L + numFullPts - 1) % numFullPts;
            if ( isContourValue[m2L] ) intersectionPoint = 1;
            }

          //specify the polygon vertices. From m2L to mL, then mR to m2R.
          for ( numPolyPoints=0, i=0; i<numLeftPointsToAdd; i++)
            {
            newPolygon[numPolyPoints++] = fullPoly[(m2L+i)%numFullPts];
            }
          newPolygon[numPolyPoints++] = fullPoly[mL];
          newPolygon[numPolyPoints++] = fullPoly[mR];
          for ( i=1; i<=numRightPointsToAdd; i++)
            {
            newPolygon[numPolyPoints++] = fullPoly[(mR+i)%numFullPts];
            }

          //add the polygon
          if(numPolyPoints < 3)
            {
            break;
            }
          cellId = this->InsertCell(newPolys,numPolyPoints,newPolygon,
                                    cellId,s[mR],newScalars);
          if ( this->GenerateContourEdges )
            {
            contourEdges->InsertNextCell(2);
            contourEdges->InsertCellPoint(fullPoly[mR]);
            contourEdges->InsertCellPoint(fullPoly[mL]);
            }
          mL = m2L;
          mR = m2R;
          }//add a polygon
        }//while still removing polygons
      }//for all polygons

    delete [] s;
    delete [] newPolygon;
    delete [] isContourValue;
    delete [] isOriginalVertex;
    delete [] fullPoly;

    output->SetPolys(newPolys);
    newPolys->Delete();
    if ( tmpPolys ) {tmpPolys->Delete(); }
    }//for all polygons (and strips) in input

  vtkDebugMacro(<<"Created " << cellId << " total cells\n");
  vtkDebugMacro(<<"Created " << output->GetVerts()->GetNumberOfCells()
                << " verts\n");
  vtkDebugMacro(<<"Created " << output->GetLines()->GetNumberOfCells()
                << " lines\n");
  vtkDebugMacro(<<"Created " << output->GetPolys()->GetNumberOfCells()
                << " polys\n");
  vtkDebugMacro(<<"Created " << output->GetStrips()->GetNumberOfCells()
                << " strips\n");

  //  Update ourselves and release temporary memory
  //
  delete [] this->ClipValues;
  intList->Delete();
  edgeTable->Delete();

  output->SetPoints(newPts);
  newPts->Delete();

  int arrayIdx = outCD->AddArray(newScalars);
  outCD->SetActiveAttribute(arrayIdx, vtkDataSetAttributes::SCALARS);

  newScalars->Delete();

  output->Squeeze();

  return 1;
}

//------------------------------------------------------------------------------
vtkPolyData *vtkBandedPolyDataContourFilter::GetContourEdgesOutput()
{
  if (this->GetNumberOfOutputPorts() < 2)
    {
    return NULL;
    }

  return vtkPolyData::SafeDownCast(
    this->GetExecutive()->GetOutputData(1));
}

//------------------------------------------------------------------------------
unsigned long int vtkBandedPolyDataContourFilter::GetMTime()
{
  unsigned long mTime=this->Superclass::GetMTime();
  unsigned long time;

  time = this->ContourValues->GetMTime();
  mTime = ( time > mTime ? time : mTime );

  return mTime;
}

//------------------------------------------------------------------------------
void vtkBandedPolyDataContourFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Generate Contour Edges: "
     << (this->GenerateContourEdges ? "On\n" : "Off\n");

  this->ContourValues->PrintSelf(os,indent.GetNextIndent());
  os << indent << "Clipping: " << (this->Clipping ? "On\n" : "Off\n");

  os << indent << "Scalar Mode: ";
  if ( this->ScalarMode == VTK_SCALAR_MODE_INDEX )
    {
    os << "INDEX\n";
    }
  else
    {
    os << "VALUE\n";
    }

  os << indent << "Clip Tolerance: " << this->ClipTolerance << "\n";
}
