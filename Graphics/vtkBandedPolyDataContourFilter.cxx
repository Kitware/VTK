/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBandedPolyDataContourFilter.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkBandedPolyDataContourFilter.h"
#include "vtkEdgeTable.h"
#include "vtkFloatArray.h"
#include "vtkTriangleStrip.h"
#include "vtkObjectFactory.h"
#include <float.h>

vtkCxxRevisionMacro(vtkBandedPolyDataContourFilter, "1.20");
vtkStandardNewMacro(vtkBandedPolyDataContourFilter);

// Construct object.
vtkBandedPolyDataContourFilter::vtkBandedPolyDataContourFilter()
{
  this->ContourValues = vtkContourValues::New();
  this->Clipping = 0;
  this->ScalarMode = VTK_SCALAR_MODE_INDEX;
  this->SetNthOutput(1,vtkPolyData::New());
  this->Outputs[1]->Delete();
  this->ClipTolerance = FLT_EPSILON;
}

vtkBandedPolyDataContourFilter::~vtkBandedPolyDataContourFilter()
{
  this->ContourValues->Delete();
}

int vtkBandedPolyDataContourFilter::ComputeScalarIndex(float val)
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

int vtkBandedPolyDataContourFilter::IsContourValue(float val)
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

// Return a flag that indicates that the ordering of vertices along the
// edge is not from v1->v2, where v1 < v2.
int vtkBandedPolyDataContourFilter::ClipEdge(int v1, int v2,
                                             vtkPoints *newPts, 
                                             vtkDataArray *scalars,
                                             vtkPointData *inPD,
                                             vtkPointData *outPD)
{
  float x[3], t;
  int ptId;
  int reverse = (v1 < v2 ? 0 : 1);

  float *x1 = newPts->GetPoint(v1);
  float *x2 = newPts->GetPoint(v2);

  float s1 = scalars->GetTuple1(v1);
  float s2 = scalars->GetTuple1(v2);
  
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
      }
    return ((reverse+1) % 2);
    }
}


extern "C" {
int vtkCompareClipValues(const void *val1, const void *val2)
{
  if ( *((float*)val1) < *((float*)val2) ) 
    {
    return (-1);
    }
  else if ( *((float*)val1) > *((float*)val2) ) 
    {
    return (1);
    }
  else 
    {
    return (0);
    }
}
}

inline int vtkBandedPolyDataContourFilter::InsertCell(vtkCellArray *cells,
                                                      int npts, vtkIdType *pts,
                                                      int cellId, float s,
                                                      vtkFloatArray *newS)
{

  int idx = this->ComputeScalarIndex(s+this->ClipTolerance);

  if ( !this->Clipping || 
       idx >= this->ClipIndex[0] && idx < this->ClipIndex[1] )
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


// Create filled contours for polydata
void vtkBandedPolyDataContourFilter::Execute()
{
  vtkPolyData *input = this->GetInput();
  vtkPointData *pd = input->GetPointData();
  vtkPolyData *output = this->GetOutput();
  vtkPointData *outPD = input->GetPointData();
  vtkCellData *outCD = output->GetCellData();
  vtkPoints *inPts = input->GetPoints();
  vtkDataArray *inScalars = pd->GetScalars();
  int numPts, numCells;
  int abort=0;
  vtkPoints *newPts;
  int i, j, idx, npts, cellId=0;
  vtkIdType *pts;
  int numEdgePts, numNewPts, maxCellSize;
  vtkIdType v, vR, *intPts;
  int intLoc, intsIdx, reverse;
  int numIntPts, intsInc;

  vtkDebugMacro(<<"Executing banded contour filter");

  //  Check input
  //
  
  numCells = input->GetNumberOfCells();
  if ( !inPts || (numPts=inPts->GetNumberOfPoints()) < 1 || 
       !inScalars || numCells < 1 )
    {
    vtkErrorMacro(<<"No input data!");
    return;
    }

  // Set up supplemental data structures for processing edge/generating
  // intersections. First we sort the contour values into an ascending
  // list of clip values including the extreme min/max values.
  this->NumberOfClipValues = this->ContourValues->GetNumberOfContours() + 2;
  this->ClipValues = new float[this->NumberOfClipValues];
  float range[2];
  inScalars->GetRange(range); 

  this->ClipValues[0] = range[0];
  this->ClipValues[this->NumberOfClipValues - 1] = range[1];
  for ( i=1; i<this->NumberOfClipValues-1; i++)
    {
    this->ClipValues[i] = this->ContourValues->GetValue(i-1);
    }

  qsort((void *)this->ClipValues, this->NumberOfClipValues, sizeof(float), 
        vtkCompareClipValues);

  // toss out values which are too close together, currently within FLT_EPSILON%
  // of each other based on full scalar range, but could define temporary based
  // on percentage of scalar range...
  for ( i=0; i<(this->NumberOfClipValues-1); i++)
    {
    if ( (this->ClipValues[i] + this->ClipTolerance) >= this->ClipValues[i+1] )
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

  // The original set of points and point data are copied. Later on 
  // intersection points due to clipping will be created.
  newPts = vtkPoints::New();

  // here is a problem.  If I don't allocate a massive chunk (i.e. 30*numPts), 
  // I get a segfault when using a large number of bands
  newPts->Allocate(3*numPts);

  outPD->InterpolateAllocate(pd,3*numPts,numPts);
  vtkDataArray *outScalars = outPD->GetScalars();

  for (i=0; i<numPts; i++)
    {
    newPts->InsertPoint(i,inPts->GetPoint(i));
    outPD->CopyData(pd, i, i);
    }

  // These are the new cell scalars
  vtkFloatArray *newScalars = vtkFloatArray::New();
  newScalars->Allocate(numCells*5,numCells);

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
          this->GetAbortExecute() )
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
          this->GetAbortExecute() )
      {
      for (i=0; i<(npts-1); i++)
        {
        numNewPts = newPts->GetNumberOfPoints();
        reverse = this->ClipEdge(pts[i],pts[i+1],newPts,inScalars,pd,outPD);
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
          this->GetAbortExecute() )
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
    vtkCellArray *contourEdges;
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
    maxCellSize *= (1 + this->NumberOfClipValues);

    vtkIdType *newPolygon = new vtkIdType [maxCellSize];
    float *s = new float [maxCellSize]; //scalars at vertices
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
    for ( polys->InitTraversal(); polys->GetNextCell(npts,pts) && !abort; )
      {
      for (i=0; i<npts; i++)
        {
        v = pts[i];
        vR = pts[(i+1) % npts];
        if ( edgeTable->IsEdge(v,vR) == -1 )
          {
          numNewPts = newPts->GetNumberOfPoints();
          reverse = this->ClipEdge(v,vR,newPts,inScalars,pd,outPD);
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
    
    // Process polygons to produce output triangles.------------------------
    //
    vtkCellArray *newPolys = vtkCellArray::New();
    newPolys->Allocate(polys->GetSize());
    int intersectionPoint;
    int mL, mR, m2L, m2R;
    int numPointsToAdd, numLeftPointsToAdd, numRightPointsToAdd;
    int numPolyPoints, numFullPts;
      
    for ( polys->InitTraversal(); polys->GetNextCell(npts,pts) && !abort; 
          this->GetAbortExecute() )
      {
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
      
      //Very important: have to find the right starting vertex. The vertex
      //needs to be one where the contour values increase in both directions.
      //Really should check whether the vertex is convex.
      float minValue=VTK_LARGE_FLOAT;
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
      if ( ! intersectionPoint || numFullPts <= 3 )
        {
        cellId = this->InsertCell(newPolys,npts,pts,cellId,s[idx],newScalars);
        continue;
        }

      //Find the first intersection points in the polygons starting
      //from this vertex and build a polygon.
      numPointsToAdd = 1;
      for ( mR=idx, intersectionPoint=0; !intersectionPoint; )
        {
        numPointsToAdd++;
        mR = (mR + 1) % numFullPts;
        if ( isContourValue[mR] ) intersectionPoint = 1;
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
      cellId = this->InsertCell(newPolys,numPolyPoints,newPolygon,
                                cellId,s[idx],newScalars);
      if ( this->GenerateContourEdges )
        {
        contourEdges->InsertNextCell(2);
        contourEdges->InsertCellPoint(fullPoly[mR]);
        contourEdges->InsertCellPoint(fullPoly[mL]);
        }

      //We've got an edge (mL,mR) that marks the edge of the region not yet
      //clipped. We move this edge forward from intersection point to
      //interection point.
      m2R = mR;
      m2L = mL;
      while ( m2R != m2L )
        {
        numPointsToAdd = (mL > mR ? mL-mR+1 : numFullPts-(mR-mL)+1);
        if ( numPointsToAdd <= 3 )
          {//just a triangle left
          for (numPolyPoints=0, i=0; i<numPointsToAdd; i++)
            {
            newPolygon[i] = fullPoly[(mR+i)%numFullPts];
            }
          cellId = this->InsertCell(newPolys,numPointsToAdd,newPolygon,
                                    cellId,s[mR],newScalars);
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

  outCD->SetScalars(newScalars);
  newScalars->Delete();
  
  output->Squeeze();
}


unsigned long int vtkBandedPolyDataContourFilter::GetMTime()
{
  unsigned long mTime=this-> vtkPolyDataToPolyDataFilter::GetMTime();
  unsigned long time;

  time = this->ContourValues->GetMTime();
  mTime = ( time > mTime ? time : mTime );
    
  return mTime;
}

void vtkBandedPolyDataContourFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Generate Contour Edges: " 
     << (this->GenerateContourEdges ? "On\n" : "Off\n");

  this->ContourValues->PrintSelf(os,indent);
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
}


