/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkClipVolume.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2000 Ken Martin, Will Schroeder, Bill Lorensen 
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
ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include <math.h>
#include "vtkClipVolume.h"
#include "vtkMergePoints.h"
#include "vtkVoxel.h"
#include "vtkDelaunay3D.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkClipVolume* vtkClipVolume::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkClipVolume");
  if(ret)
    {
    return (vtkClipVolume*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkClipVolume;
}




// Construct with user-specified implicit function; InsideOut turned off; value
// set to 0.0; and generate clip scalars turned off. The merge tolerance is set
// to 0.01.
vtkClipVolume::vtkClipVolume(vtkImplicitFunction *cf)
{
  this->ClipFunction = cf;
  this->InsideOut = 0;
  this->Locator = NULL;
  this->Value = 0.0;
  this->GenerateClipScalars = 0;

  this->GenerateClippedOutput = 0;
  this->MergeTolerance = 0.01;
  
  this->Mesh = NULL;

  this->MeshLocator = vtkMergePoints::New();
  this->MeshLocator->SetDivisions(2,2,2);
  this->MeshLocator->AutomaticOff();
  
  this->Triangulator = vtkDelaunay3D::New();
  this->Triangulator->SetLocator(this->MeshLocator);
  this->vtkSource::SetNthOutput(1,vtkUnstructuredGrid::New());
  this->Outputs[1]->Delete();
}

vtkClipVolume::~vtkClipVolume()
{
  if ( this->Locator )
    {
    this->Locator->UnRegister(this);
    this->Locator = NULL;
    }
  
  if ( this->Mesh )
    {
    this->Mesh->Delete();
    }
  this->MeshLocator->UnRegister(this);
  this->Triangulator->Delete();
  this->SetClipFunction(NULL);
}

vtkUnstructuredGrid *vtkClipVolume::GetClippedOutput()
{
  if (this->NumberOfOutputs < 2)
    {
    return NULL;
    }
  
  return (vtkUnstructuredGrid *)(this->Outputs[1]);
}

// Overload standard modified time function. If Clip functions is modified,
// then this object is modified as well.
unsigned long vtkClipVolume::GetMTime()
{
  unsigned long mTime, time;

  mTime=this->vtkStructuredPointsToUnstructuredGridFilter::GetMTime();

  if ( this->Locator != NULL )
    {
    time = this->Locator->GetMTime();
    mTime = ( time > mTime ? time : mTime );
    }

  if ( this->ClipFunction != NULL )
    {
    time = this->ClipFunction->GetMTime();
    mTime = ( time > mTime ? time : mTime );
    }

  return mTime;
}

//
// Clip through volume generating tetrahedra
//
void vtkClipVolume::Execute()
{
  vtkStructuredPoints *input = this->GetInput();
  vtkUnstructuredGrid *output = this->GetOutput();
  vtkUnstructuredGrid *clippedOutput = this->GetClippedOutput();
  vtkUnstructuredGrid *outputPtr;
  int cellId, i, j, k, flip, iflip, jflip, kflip, newCellId;
  vtkPoints *cellPts;
  vtkScalars *clipScalars;
  vtkScalars *cellScalars; 
  vtkCell *cell;
  vtkPoints *newPoints;
  vtkIdList *cellIds;
  float value, s, *x, origin[3], spacing[3];
  int estimatedSize, numCells=input->GetNumberOfCells();
  int numPts=input->GetNumberOfPoints();
  vtkPointData *inPD=input->GetPointData(), *outPD=output->GetPointData();
  vtkCellData *inCD=input->GetCellData(), *outCD=output->GetCellData();
  vtkCellData *clippedCD=clippedOutput->GetCellData();
  int dims[3], dimension, numICells, numJCells, numKCells, sliceSize;
  int above, below;
  vtkIdList *tetraIds;
  vtkPoints *tetraPts; 
  int ii, jj, pts[4], id, ntetra;
    
  vtkDebugMacro(<< "Clipping volume");
  
  // Initialize self; create output objects
  //
  input->GetDimensions(dims);
  input->GetOrigin(origin);
  input->GetSpacing(spacing);
  
  for (dimension=3, i=0; i<3; i++)
    {
    if ( dims[0] <= 1 )
      {
      dimension--;
      }
    }
  if ( dimension < 3 )
    {
    vtkErrorMacro("This filter only clips 3D volume data");
    return;
    }

  if ( !this->ClipFunction && this->GenerateClipScalars )
    {
    vtkErrorMacro(<<"Cannot generate clip scalars without clip function");
    return;
    }

  //
  // Create objects to hold output of clip operation
  //
  estimatedSize = numCells;
  estimatedSize = estimatedSize / 1024 * 1024; //multiple of 1024
  if (estimatedSize < 1024)
    {
    estimatedSize = 1024;
    }

  newPoints = vtkPoints::New();
  newPoints->Allocate(estimatedSize/2,estimatedSize/2);
  output->Allocate(estimatedSize*2); //allocate storage for cells

  // locator used to merge potentially duplicate points
  if ( this->Locator == NULL )
    {
    this->CreateDefaultLocator();
    }
  this->Locator->InitPointInsertion (newPoints, input->GetBounds());

  // Determine whether we're clipping with input scalars or a clip function
  // and do necessary setup.
  if ( this->ClipFunction )
    {
    vtkScalars *tmpScalars = vtkScalars::New();
    tmpScalars->Allocate(numPts);
    inPD = vtkPointData::New();
    inPD->ShallowCopy(input->GetPointData());
    //    inPD = new vtkPointData(*(input->GetPointData()));//copies original
    if ( this->GenerateClipScalars )
      {
      inPD->SetScalars(tmpScalars);
      }
    for ( i=0; i < numPts; i++ )
      {
      s = this->ClipFunction->FunctionValue(input->GetPoint(i));
      tmpScalars->InsertScalar(i,s);
      }
    clipScalars = (vtkScalars *)tmpScalars;
    }
  else //using input scalars
    {
    clipScalars = inPD->GetScalars();
    if ( !clipScalars )
      {
      vtkErrorMacro(<<"Cannot clip without clip function or input scalars");
      return;
      }
    }
    
  if ( !this->GenerateClipScalars && !input->GetPointData()->GetScalars())
    {
    outPD->CopyScalarsOff();
    }
  else
    {
    outPD->CopyScalarsOn();
    }
  outPD->InterpolateAllocate(inPD,estimatedSize,estimatedSize/2);
  outCD->CopyAllocate(inCD,estimatedSize,estimatedSize/2);

  // If generating second output, setup clipped output
  if ( this->GenerateClippedOutput )
    {
    this->GetClippedOutput()->Initialize();
    this->GetClippedOutput()->Allocate(estimatedSize); //allocate storage for cells
    }

  // perform clipping on voxels - compute approriate numbers
  value = this->Value;
  numICells = dims[0] - 1;
  numJCells = dims[1] - 1;
  numKCells = dims[2] - 1;
  sliceSize = numICells * numJCells;
  
  tetraIds = vtkIdList::New(); tetraIds->Allocate(20);
  cellScalars = vtkScalars::New(); cellScalars->Allocate(8);
  tetraPts = vtkPoints::New(); tetraPts->Allocate(20); 
  
  // Loop over i-j-k directions so that we can control the direction of
  // face diagonals on voxels (i.e., the flip variable). The flip variable
  // also controls the ordered Delaunay triangulation used in ClipVoxel().
  for ( iflip=0, k=0; k < numKCells; k++)
    {
    if ( !(sliceSize % 2) && (k % 2) )
      {
      kflip = 1;
      }
    else
      {
      kflip =0;
      }

    for ( j=0; j < numJCells; j++)
      {
      if ( !(numICells % 2) && (j % 2) )
	{
	jflip = 1;
	}
      else
	{
	jflip =0;
	}
      
      for ( i=0; i < numICells; i++, iflip = (iflip ? 0 : 1) )
        {
        flip = (iflip + jflip + kflip) % 2;

        cellId = i + j*numICells + k*sliceSize;
        
        cell = input->GetCell(cellId);
        cellPts = cell->GetPoints();
        cellIds = cell->GetPointIds();

        // gather scalar values for the cell and keep
        for ( above=below=0, ii=0; ii < 8; ii++ )
          {
          s = clipScalars->GetScalar(cellIds->GetId(ii));
          cellScalars->SetScalar(ii, s);
          if ( s >= value )
	    {
	    above = 1;
	    }
          else if ( s < value )
	    {
	    below = 1;
	    }
          }
    
        // take into account inside/out flag
        if ( this->InsideOut )
          {
          above = !above;
          below = !below;
          }
        
        // see whether voxel is fully inside or outside
        if ( (above && !below) || 
	     (this->GenerateClippedOutput && (below && !above)) )
          {
          ((vtkVoxel *)cell)->Triangulate(flip, tetraIds, tetraPts);
          ntetra = tetraPts->GetNumberOfPoints() / 4;

          if (above && !below)
	    {
	    outputPtr = output;
	    }
          else
	    {
	    outputPtr = clippedOutput;
	    }

          for (ii=0; ii<ntetra; ii++)
            {
            id = ii*4;
            for (jj=0; jj<4; jj++)
              {
              x = tetraPts->GetPoint(id+jj);
              if ( this->Locator->InsertUniquePoint(x, pts[jj]) )
                {
                outPD->CopyData(inPD,tetraIds->GetId(id+jj),pts[jj]);
                }
              }
            newCellId = outputPtr->InsertNextCell(VTK_TETRA, 4, pts);
	    outCD->CopyData(inCD,cellId,newCellId);
            }
          }
        
        else if (above == below ) // clipped voxel, have to triangulate 
          {
          this->ClipVoxel(value, cellScalars, flip, origin, spacing, 
                          cellIds, cellPts, inPD, outPD, inCD, cellId, 
			  outCD, clippedCD);
          }
          
        }// for i
      }// for j
    }// for k

  vtkDebugMacro(<<"Created: " 
                << newPoints->GetNumberOfPoints() << " points, " 
                << output->GetNumberOfCells() << " tetra" );
 
  if ( this->GenerateClippedOutput )
    {
    vtkDebugMacro(<<"Created (clipped output): " 
                  << clippedOutput->GetNumberOfCells() << " tetra");
    }

  // Update ourselves.  Because we don't know upfront how many cells
  // we've created, take care to reclaim memory. 
  //
  if ( this->ClipFunction ) 
    {
    clipScalars->Delete();
    inPD->Delete();
    }

  if ( this->GenerateClippedOutput )
    {
    this->GetClippedOutput()->SetPoints(newPoints);
    this->GetClippedOutput()->GetPointData()->PassData(outPD);
    this->GetClippedOutput()->Squeeze();
    }

  output->SetPoints(newPoints);
  newPoints->Delete();
  tetraIds->Delete();
  tetraPts->Delete();
  cellScalars->Delete();
  
  this->Locator->Initialize();//release any extra memory
  output->Squeeze();
}


// Method to triangulate and clip voxel using ordered Delaunay
// triangulation. Voxel is initially triangulated with 8 corner points in
// special order (to control direction of face diagonals). Then edge
// intersection points are injected into triangulation. Because of convex,
// regular spacing of voxel points, we don't have to worry about constrained
// Delaunay problems.
void vtkClipVolume::ClipVoxel(float value, vtkScalars *cellScalars, 
                              int flip, float vtkNotUsed(origin)[3],
			      float spacing[3], 
                              vtkIdList *cellIds, vtkPoints *cellPts,
                              vtkPointData *inPD, vtkPointData *outPD,
                              vtkCellData *inCD, int cellId, 
			      vtkCellData *outCD, vtkCellData *clippedCD)
{
  float x[3], *xPtr, s1, s2, t, voxelOrigin[3];
  float length, center[3], p1[3], p2[3];
  int i, j, k, edgeNum, numPts, numTetras, npts, *pts, tPts[4];
  int numOutTetras, numMergedPts, ptId, newCellId;
  vtkIdList *holeTetras, *cells, *mergedPts;
  vtkPoints *points;
  vtkUnstructuredGrid *output=this->GetOutput();
  vtkUnstructuredGrid *clippedOutput=this->GetClippedOutput();
  char *tetraUse;
  static int edges[12][2] = { {0,1}, {2,3}, {4,5}, {6,7},
                              {0,2}, {1,3}, {4,6}, {5,7},
                              {0,4}, {1,5}, {2,6}, {3,7}};
  static int order[2][8] = { {0,3,5,6,1,2,4,7},
                             {1,2,4,7,0,3,5,6}};//injection order based on flip

  // quick fix until constructors are changed
  holeTetras = vtkIdList::New();
  holeTetras->Allocate(10);
  cells = vtkIdList::New();
  cells->Allocate(64);
  mergedPts = vtkIdList::New();
  mergedPts->Allocate(12);
  
  // compute bounds for voxel and initialize
  cellPts->GetPoint(0,voxelOrigin);
  for (length=0.0, i=0; i<3; i++)
    {
    center[i] = voxelOrigin[i] + 0.5*spacing[i];
    length = (spacing[i] > length ? spacing[i] : length);
    }
  
  // Initialize Delaunay insertion process with voxel triangulation.
  // No more than 20 points (8 corners + 12 edges) may be inserted.
  this->Mesh = this->Triangulator->InitPointInsertion(center, 2.5*length, 
                                                      20, points);

  // Inject ordered voxel corner points into triangulation
  for (numPts=0; numPts<8; numPts++)
    {
    ptId = order[flip][numPts];
    xPtr = cellPts->GetPoint(ptId);
    this->Triangulator->InsertPoint(this->Mesh, points, ptId, xPtr, holeTetras);
      
    // Incorporate points into output if appropriate
    s1 = cellScalars->GetScalar(ptId);
    if ( (s1 >= value && !this->InsideOut) ||
    (s1 < value && this->InsideOut) || this->GenerateClippedOutput )
      {
      if ( this->Locator->InsertUniquePoint(xPtr, tPts[0]) )
        {
        outPD->CopyData(inPD,cellIds->GetId(ptId),tPts[0]);
        }
      }
    }//for eight voxel corner points
  
  // For each edge intersection point, insert into triangulation. Edge
  // intersections come from clipping value. Have to be careful of 
  // intersections near exisiting points (causes bad Delaunay behavior).
  for (edgeNum=0; edgeNum < 12; edgeNum++)
    {
    s1 = cellScalars->GetScalar(edges[edgeNum][0]);
    s2 = cellScalars->GetScalar(edges[edgeNum][1]);
    if ( (s1 < value && s2 >= value) || (s1 >= value && s2 < value) )
      {
      t = (value - s1) / (s2 - s1);
      //check to see whether near voxel corner point - have to merge
      if ( t < this->MergeTolerance )
        {
        mergedPts->InsertNextId(edges[edgeNum][0]);
        continue;
        }
      else if ( t > (1.0 - this->MergeTolerance) )
        {
        mergedPts->InsertNextId(edges[edgeNum][1]);
        continue;
        }

      // generate edge intersection point
      cellPts->GetPoint(edges[edgeNum][0],p1);
      cellPts->GetPoint(edges[edgeNum][1],p2);
      for (i=0; i<3; i++)
        {
        x[i] = p1[i] + t * (p2[i] - p1[i]);
        }
      
      //Insert into Delaunay triangulation
      this->Triangulator->InsertPoint(this->Mesh, points, numPts++, 
                                      x, holeTetras);
      
      // Incorporate point into output and interpolate edge data as necessary
      if ( this->Locator->InsertUniquePoint(x, ptId) )
        {
        outPD->InterpolateEdge(inPD, ptId, cellIds->GetId(edges[edgeNum][0]),
                               cellIds->GetId(edges[edgeNum][1]), t);
        }

      }//if edge intersects value
    }//for all edges
  
  this->Triangulator->EndPointInsertion();

  // Begin classification of tetrahedra. First initialize in/out array.
  numTetras = this->Mesh->GetNumberOfCells();
  tetraUse = new char[numTetras];
  for (i=0; i < numTetras; i++)
    {
    tetraUse[i] = 1;
    }
  for (i=0; i<holeTetras->GetNumberOfIds(); i++ )
    {
    tetraUse[holeTetras->GetId(i)] = 0;
    }

  // Delete tetras connected to Delaunay boundary points
  for (i=20; i < 26; i++)
    {
    this->Mesh->GetPointCells(i, cells);
    numOutTetras = cells->GetNumberOfIds();
    for (j=0; j < numOutTetras; j++)
      {
      tetraUse[cells->GetId(j)] = 0; //mark as deleted
      }
    }

  // Adjust the merged points so that the following code (which determines
  // in/out of tetra) will work correctly.
  numMergedPts = mergedPts->GetNumberOfIds();
  for (i=0; i<numMergedPts; i++)
    {
    ptId = mergedPts->GetId(i);
    cellScalars->SetScalar(ptId, value);
    xPtr = cellPts->GetPoint(ptId);
    if ( this->Locator->InsertUniquePoint(xPtr, tPts[0]) )
      {
      outPD->CopyData(inPD,cellIds->GetId(ptId),tPts[0]);
      }
    }
  
  // Classify all tetra as inside or outside contour value and send to output
  for (i=0; i < numTetras; i++)
    {
    if ( tetraUse[i] ) //tetra not deleted
      {
      this->Mesh->GetCellPoints(i, npts, pts);

      for (j=0; j<4; j++) //for each tetra point
        {
        if ( pts[j] < 8 ) //one of voxel corners
          {
          //when outside of contour value break out of loop
          if ( cellScalars->GetScalar(pts[j]) < value )
	    {
	    break;
	    }
          }
        }
      
      if ( this->InsideOut )
        {
        if (j >= 4)
	  {
	  j = 0;
	  }
        else
	  {
	  j = 4;
	  }
        }
      
      if ( j >= 4 || this->GenerateClippedOutput )
        {
        for (k=0; k<4; k++)
          {
          xPtr = points->GetPoint(pts[k]);
          //point was previously inserted - will always return valid id
          tPts[k] = this->Locator->IsInsertedPoint(xPtr); 
          }

        if ( j >= 4 ) 
	  {
	  newCellId = output->InsertNextCell(VTK_TETRA, 4, tPts);
	  outCD->CopyData(inCD, cellId, newCellId);
	  }
        else 
	  {
	  newCellId = clippedOutput->InsertNextCell(VTK_TETRA, 4, tPts);
	  clippedCD->CopyData(inCD, cellId, newCellId);
	  }
        }

      }//if tetra is used
    }//for all tetras
  
  // Clean up after ourselves
  delete [] tetraUse;
  
  holeTetras->Delete();
  cells->Delete();
  mergedPts->Delete();
  
  this->Mesh->Delete();
  this->Mesh = NULL;
  this->MeshLocator->Initialize();
}


// Specify a spatial locator for merging points. By default, 
// an instance of vtkMergePoints is used.
void vtkClipVolume::SetLocator(vtkPointLocator *locator)
{
  if ( this->Locator == locator ) 
    {
    return;
    }
  if ( this->Locator )
    {
    this->Locator->UnRegister(this);
    this->Locator = NULL;
    }

  if (locator)
    {
    locator->Register(this);
    }

  this->Locator = locator;
  this->Modified();
}

void vtkClipVolume::CreateDefaultLocator()
{
  if ( this->Locator == NULL )
    {
    this->Locator = vtkMergePoints::New();
    }
}

void vtkClipVolume::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkStructuredPointsToUnstructuredGridFilter::PrintSelf(os,indent);

  if ( this->ClipFunction )
    {
    os << indent << "Clip Function: " << this->ClipFunction << "\n";
    }
  else
    {
    os << indent << "Clip Function: (none)\n";
    }
  os << indent << "InsideOut: " << (this->InsideOut ? "On\n" : "Off\n");
  os << indent << "Value: " << this->Value << "\n";
  os << indent << "Merge Tolerance: " << this->MergeTolerance << "\n";
  if ( this->Locator )
    {
    os << indent << "Locator: " << this->Locator << "\n";
    }
  else
    {
    os << indent << "Locator: (none)\n";
    }

  os << indent << "Generate Clip Scalars: " << (this->GenerateClipScalars ? "On\n" : "Off\n");

  os << indent << "Generate Clipped Output: " << (this->GenerateClippedOutput ? "On\n" : "Off\n");
}
