/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBandedPolyDataContourFilter.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkBandedPolyDataContourFilter.h"
#include "vtkEdgeTable.h"
#include "vtkFloatArray.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkBandedPolyDataContourFilter, "1.6");
vtkStandardNewMacro(vtkBandedPolyDataContourFilter);

// Construct object.
vtkBandedPolyDataContourFilter::vtkBandedPolyDataContourFilter()
{
  this->ContourValues = vtkContourValues::New();
}

vtkBandedPolyDataContourFilter::~vtkBandedPolyDataContourFilter()
{
  this->ContourValues->Delete();
}

int vtkBandedPolyDataContourFilter::ComputeLowerScalarIndex(float val)
{
  for (int i=0; i < (this->NumberOfClipValues-1); i++)
    {
    if ( val >= this->ClipValues[i] && val < this->ClipValues[i+1]  )
      {
      return i;
      }
    }
  return 0;
}

int vtkBandedPolyDataContourFilter::ComputeUpperScalarIndex(float val)
{
  for (int i=0; i < (this->NumberOfClipValues-1); i++)
    {
    if ( val > this->ClipValues[i] && val <= this->ClipValues[i+1]  )
      {
      return i;
      }
    }
  return 0;
}

// v1 assumed < v2
int vtkBandedPolyDataContourFilter::ClipEdge(int v1, int v2,
                                             vtkPoints *newPts, 
                                             vtkDataArray *scalars,
                                             vtkPointData *inPD,
                                             vtkPointData *outPD)
{
  float x[3];
  int ptId;

  float s1 = scalars->GetTuple(v1)[0];
  float s2 = scalars->GetTuple(v2)[0];
  
  int idx1 = this->ComputeLowerScalarIndex(s1);
  int idx2 = this->ComputeUpperScalarIndex(s2);

  float *x1 = newPts->GetPoint(v1);
  float *x2 = newPts->GetPoint(v2);

  this->PtIds[0] = v1;
  this->PtIds[idx2-idx1+1] = v2;
  this->T[0] = 0.0;
  this->T[idx2-idx1+1] = 1.0;
  this->CellScalars[0] = idx1;
  
  for (int i=1; i < (idx2-idx1+1); i++)
    {
    this->T[i] = (this->ClipValues[idx1+i] - s1) / (s2 - s1);
    x[0] = x1[0] + this->T[i]*(x2[0]-x1[0]);
    x[1] = x1[1] + this->T[i]*(x2[1]-x1[1]);
    x[2] = x1[2] + this->T[i]*(x2[2]-x1[2]);
    ptId = this->PtIds[i] = newPts->InsertNextPoint(x);
    outPD->InterpolateEdge(inPD,ptId,v1,v2,this->T[i]);
    this->CellScalars[i] = idx1 + i;
    }

  return (idx2-idx1+1);
}


extern "C" {
int vtkCompareContourValues(const void *val1, const void *val2)
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

// Create filled contours for polydata
void vtkBandedPolyDataContourFilter::Execute()
{
  vtkPolyData *input = this->GetInput();
  vtkPointData *pd = input->GetPointData();
  vtkCellData *cd = input->GetCellData();
  vtkPolyData *output = this->GetOutput();
  vtkPointData *outPD = input->GetPointData();
  vtkCellData *outCD = output->GetCellData();
  vtkPoints *inPts = input->GetPoints();
  vtkDataArray *inScalars = pd->GetScalars();
  int numPts, numCells;
  int abort=0;
  vtkPoints *newPts;
  int i, j, idx, npts, cellId=0, ptId=0;
  vtkIdType *pts;

  vtkDebugMacro(<<"Executing banded contour filter");

  //  Check input
  //
  numPts = inPts->GetNumberOfPoints();
  numCells = input->GetNumberOfCells();
  if ( numCells < 1 || numPts < 1 )
    {
    vtkErrorMacro(<<"No input data!");
    return;
    }

  // Set up supplemental data structures for processing edge/generating
  // intersections. First we sort the contour values into an ascending
  // list of clip values including the extreme min/max values.
  this->NumberOfClipValues = this->ContourValues->GetNumberOfContours() + 2;
  this->ClipValues = new float[this->NumberOfClipValues];
  float range[2], tol;
  inScalars->GetRange(range); tol = (range[1]-range[0])/100.0;
  this->ClipValues[0] = range[0];
  this->ClipValues[1] = range[1];
  for ( i=2; i<this->NumberOfClipValues; i++)
    {
    this->ClipValues[i] = this->ContourValues->GetValue(i-2);
    }
  qsort((void *)this->ClipValues, this->NumberOfClipValues, sizeof(float), 
        vtkCompareContourValues);
  for ( i=0; i<(this->NumberOfClipValues-1); i++)
    {
    if ( (this->ClipValues[i]+tol) >= this->ClipValues[i+1] )
      {
      for (j=i+1; j<(this->NumberOfClipValues-2); j++)
        {
        this->ClipValues[i+1] = this->ClipValues[i+2];
        }
      this->NumberOfClipValues--;
      }
    }

  //used for edge clipping
  this->PtIds = new int[this->NumberOfClipValues];
  this->T = new float[this->NumberOfClipValues];
  this->CellScalars = new int[this->NumberOfClipValues];

  // The original set of points and point data are copied. Later on 
  // intersection points due to clipping will be created.
  newPts = vtkPoints::New();
  newPts->Allocate(3*numPts);
  outPD->CopyAllocate(pd,3*numPts,numPts);
  for (i=0; i<numPts; i++)
    {
    newPts->InsertPoint(i,inPts->GetPoint(i));
    outPD->CopyData(pd, i, i);
    }

  // These are the new cell scalars
  vtkFloatArray *newScalars = vtkFloatArray::New();
  newScalars->Allocate(numCells*5,numCells);

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
        idx = this->ComputeLowerScalarIndex(inScalars->GetTuple(pts[i])[0]);
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
    int numSegments;
    vtkCellArray *lines = input->GetLines();
    vtkCellArray *newLines = vtkCellArray::New();
    newLines->Allocate(lines->GetSize());
    for ( lines->InitTraversal(); lines->GetNextCell(npts,pts) && !abort; 
          this->GetAbortExecute() )
      {
      for (i=0; i<(npts-1); i++)
        {
        if ( pts[i] < pts[i+1] )
          {
          numSegments = this->ClipEdge(pts[i],pts[i+1],newPts,inScalars,
                                       pd,outPD);
          }
        else
          {
          numSegments = this->ClipEdge(pts[i+1],pts[i],newPts,inScalars,
                                       pd,outPD);
          }
        for (j=0; j<numSegments; j++)
          {
          newLines->InsertNextCell(2,this->PtIds+j);
          newScalars->InsertTuple1(cellId++,this->CellScalars[j]);
          }
        }
      }
    output->SetLines(newLines);
    newLines->Delete();
    }
  
  // Polygons are assumed convex and chopped into filled, convex polygons.
  // Triangle strips are treated similarly.
  //
  if ( input->GetPolys()->GetNumberOfCells() > 0 ||
       input->GetStrips()->GetNumberOfCells() > 0 )
    {
    // Set up processing. We are going to store an ordered list of
    // intersections along each edge (ordered from smallest point id
    // to largest). These will later be connected into convex polygons
    // which represent a filled region in the cell.
    //
    vtkEdgeTable *edgeTable;
    edgeTable = vtkEdgeTable::New();
    edgeTable->InitEdgeInsertion(numPts,1); //store attributes on edge

    vtkCellArray *polys = input->GetPolys();
    vtkCellArray *strips = input->GetStrips();
    vtkCellArray *intList = vtkCellArray::New();

    int numEdgePts, numNewPts, numSegments;
    vtkIdType v, vL, vR;

    //process polygons
    for ( polys->InitTraversal(); polys->GetNextCell(npts,pts) && !abort; )
      {
      for (i=0; i<npts; i++)
        {
        v = pts[i];
        vR = pts[(i+1) % npts];
        if ( edgeTable->IsEdge(v,vR) == -1 )
          {
          numNewPts = newPts->GetNumberOfPoints();
          if ( v < vR )
            {
            numSegments = this->ClipEdge(v,vR,newPts,inScalars,pd,outPD);
            }
          else
            {
            numSegments = this->ClipEdge(vR,v,newPts,inScalars,pd,outPD);
            }
          numEdgePts = newPts->GetNumberOfPoints() - numNewPts;
          if ( numEdgePts > 0 )
            {
            intList->InsertNextCell(numEdgePts);
            edgeTable->InsertEdge(v,vR,intList->GetInsertLocation(0));
            for (j=0; j<numEdgePts; j++)
              {
              intList->InsertCellPoint(numNewPts+j);
              }
            }
          else //no intersection points along the edge
            {
            edgeTable->InsertEdge(v,vR,-1); //-1 means no points
            }
          }//if edge not processed yet
        }
      }//for all polygons
    
    //process polygons again, this time building convex polygons
    vtkCellArray *newPolys = vtkCellArray::New();
    newPolys->Allocate(polys->GetSize());
    vtkIdType *intPtsL, *intPtsR;
    vtkIdType mvL, mvR, mv2L, mv2R;
    vtkIdType bandPts[4];
    int cIdxL, cIdxR, idxL, idxR;
    int intLocL, intLocR, numIntPtsL, numIntPtsR, incL, incR;
      
    for ( polys->InitTraversal(); polys->GetNextCell(npts,pts) && !abort; 
          this->GetAbortExecute() )
      {
      cIdxL = npts-1;
      cIdxR = 1;

      vL = pts[cIdxL--]; //vertex to the left
      v = pts[0]; //this vert
      vR = pts[cIdxR++]; //vertex to the right

      intLocL = edgeTable->IsEdge(vL,v);
      intLocR = edgeTable->IsEdge(v,vR);

      //put the initial triangle into the output
      if ( intLocL == -1 ) 
        {
        mvL = vL;
        }
      else
        {
        intList->GetCell(intLocL,numIntPtsL,intPtsL);
        if ( v < vL ) {idxL = 0; incL=1;}
        else {idxL = numIntPtsL-1; incL=(-1);}
        mvL = intPtsL[idxL];
        }

      if ( intLocR == -1 ) 
        {
        mvR = vR;
        }
      else
        {
        intList->GetCell(intLocR,numIntPtsR,intPtsR);
        if ( v < vR ) {idxR = 0; incR=1;}
        else {idxR = numIntPtsR-1; incR=(-1);}
        mvR = intPtsR[idxR];
        }

      bandPts[0] = mvL;
      bandPts[1] = v;
      bandPts[2] = mvR;
      newPolys->InsertNextCell(3,bandPts);
      newScalars->InsertTuple1(cellId++, 1);

      //now move the segment around the polygon
      mv2L = mvL;
      mv2R = mvR;
      while ( cIdxL > 0 && cIdxR <= npts )
        {
        //advance the left vertex
        mvL = mv2L;
        idxL += incL;
        if ( idxL < numIntPtsL && idxL >= 0 )
          {
          mv2L = intPtsL[idxL];
          }
        else
          {
          mv2L = vL;
          if ( (intLocL = edgeTable->IsEdge(vL,pts[cIdxL])) != -1 )
            {
            intList->GetCell(intLocL,numIntPtsL,intPtsL);
            if ( vL < pts[cIdxL] ) {idxL = (-1); incL=1;}
            else {idxL = numIntPtsL; incL=(-1);}
            vL = pts[cIdxL--];
            }
          else
            {
            numIntPtsL = 0;
            }
          }
        
        //advance the right vertex
        mvR = mv2R;
        idxR += incR;
        if ( idxR < numIntPtsR && idxR >= 0 )
          {
          mv2R = intPtsR[idxR];
          }
        else
          {
          mv2R = vR;
          if ( (intLocL = edgeTable->IsEdge(vL,pts[cIdxL])) != -1 )
            {
            intLocR = edgeTable->IsEdge(vR,pts[cIdxR]);           
            intList->GetCell(intLocR,numIntPtsR,intPtsR);
            if ( vR < pts[cIdxR] ) {idxR = (-1); incR=1;}
            else {idxR = numIntPtsR; incR=(-1);}
            vR = pts[cIdxR++];
            }
          else
            {
            numIntPtsR = 0;
            }
          }

        //create the output polygon
        bandPts[0] = mv2L;
        bandPts[1] = mvL;
        bandPts[2] = mvR;
        bandPts[3] = mv2R;
        if ( bandPts[0] == bandPts[3] )
          {
          newPolys->InsertNextCell(3,bandPts); //terminating triangle
          }
        else
          {
          newPolys->InsertNextCell(4,bandPts);
          }
        newScalars->InsertTuple1(cellId++, 1);
        }

      }//for all polygons

    edgeTable->Delete();
    output->SetPolys(newPolys);
    newPolys->Delete();
    }
  
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
  delete [] this->PtIds;
  delete [] this->T;
  delete [] this->CellScalars;

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

  this->ContourValues->PrintSelf(os,indent);
}


