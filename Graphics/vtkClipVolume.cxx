/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkClipVolume.cxx
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
#include <math.h>
#include "vtkClipVolume.h"
#include "vtkMergePoints.h"
#include "vtkVoxel.h"
#include "vtkOrderedTriangulator.h"
#include "vtkObjectFactory.h"
#include "vtkFloatArray.h"

//------------------------------------------------------------------------
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

  this->Triangulator = vtkOrderedTriangulator::New();
  this->Triangulator->PreSortedOn();
  
  // optional clipped output
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
  vtkImageData *input = this->GetInput();
  if (input == NULL) {return;}
  vtkUnstructuredGrid *output = this->GetOutput();
  vtkUnstructuredGrid *clippedOutput = this->GetClippedOutput();
  vtkUnstructuredGrid *outputPtr;
  vtkIdType cellId, newCellId, i;
  int j, k, flip, iflip, jflip, kflip;
  vtkPoints *cellPts;
  vtkDataArray *clipScalars;
  vtkFloatArray *cellScalars; 
  vtkCell *cell;
  vtkPoints *newPoints;
  vtkIdList *cellIds;
  float value, s, *x, origin[3], spacing[3];
  vtkIdType estimatedSize, numCells=input->GetNumberOfCells();
  vtkIdType numPts=input->GetNumberOfPoints();
  vtkPointData *inPD=input->GetPointData(), *outPD=output->GetPointData();
  vtkCellData *inCD=input->GetCellData(), *outCD=output->GetCellData();
  vtkCellData *clippedCD=clippedOutput->GetCellData();
  int dims[3], dimension, numICells, numJCells, numKCells, sliceSize;
  int above, below;
  vtkIdList *tetraIds;
  vtkPoints *tetraPts; 
  int ii, jj, id, ntetra;
  vtkIdType pts[4];
  
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
    vtkFloatArray *tmpScalars = vtkFloatArray::New();
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
      tmpScalars->InsertTuple(i,&s);
      }
    clipScalars = tmpScalars;
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
    this->GetClippedOutput()->Allocate(estimatedSize); //storage for cells
    }

  // perform clipping on voxels - compute approriate numbers
  value = this->Value;
  numICells = dims[0] - 1;
  numJCells = dims[1] - 1;
  numKCells = dims[2] - 1;
  sliceSize = numICells * numJCells;
  
  tetraIds = vtkIdList::New(); tetraIds->Allocate(20);
  cellScalars = vtkFloatArray::New(); cellScalars->Allocate(8);
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
          s = clipScalars->GetComponent(cellIds->GetId(ii),0);
          cellScalars->SetTuple(ii, &s);
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
void vtkClipVolume::ClipVoxel(float value, vtkDataArray *cellScalars, 
                              int flip, float vtkNotUsed(origin)[3],
                              float spacing[3], 
                              vtkIdList *cellIds, vtkPoints *cellPts,
                              vtkPointData *inPD, vtkPointData *outPD,
                              vtkCellData *vtkNotUsed(inCD),
			      vtkIdType vtkNotUsed(cellId), 
                              vtkCellData *vtkNotUsed(outCD),
			      vtkCellData *vtkNotUsed(clippedCD))
{
  float x[3], *xPtr, s1, s2, t, voxelOrigin[3];
  float bounds[6], p1[3], p2[3];
  int i, edgeNum, numPts;
  vtkIdType id, ptId;
  vtkUnstructuredGrid *output=this->GetOutput();
  vtkUnstructuredGrid *clippedOutput=this->GetClippedOutput();
  static int edges[12][2] = { {0,1}, {2,3}, {4,5}, {6,7},
                              {0,2}, {1,3}, {4,6}, {5,7},
                              {0,4}, {1,5}, {2,6}, {3,7}};
  static int order[2][8] = { {0,3,5,6,1,2,4,7},
                             {1,2,4,7,0,3,5,6}};//injection order based on flip

  // compute bounds for voxel and initialize
  cellPts->GetPoint(0,voxelOrigin);
  for (i=0; i<3; i++)
    {
    bounds[2*i] = voxelOrigin[i];
    bounds[2*i+1] = voxelOrigin[i] + spacing[i];
    }
  
  // Initialize Delaunay insertion process with voxel triangulation.
  // No more than 20 points (8 corners + 12 edges) may be inserted.
  this->Triangulator->InitTriangulation(bounds,20);

  // Inject ordered voxel corner points into triangulation. Recall
  // that the PreSortedOn() flag was set in the triangulator.
  int type;
  vtkIdType internalId[8]; //used to merge points if nearby edge intersection
  for (numPts=0; numPts<8; numPts++)
    {
    ptId = order[flip][numPts];
      
    // Currently all points are injected because of the possibility 
    // of intersection point merging.
    s1 = cellScalars->GetComponent(ptId,0);
    if ( (s1 >= value && !this->InsideOut) || (s1 < value && this->InsideOut) )
      {
      type = 0; //inside
      }
    else
      {
      type = 4; //no insert, but its type might change later
      }

    xPtr = cellPts->GetPoint(ptId);
    if ( this->Locator->InsertUniquePoint(xPtr, id) )
      {
      outPD->CopyData(inPD,cellIds->GetId(ptId), id);
      }
    internalId[ptId] = this->Triangulator->InsertPoint(id, xPtr, type);
    }//for eight voxel corner points
  
  // For each edge intersection point, insert into triangulation. Edge
  // intersections come from clipping value. Have to be careful of 
  // intersections near exisiting points (causes bad Delaunay behavior).
  for (edgeNum=0; edgeNum < 12; edgeNum++)
    {
    s1 = cellScalars->GetComponent(edges[edgeNum][0],0);
    s2 = cellScalars->GetComponent(edges[edgeNum][1],0);
    if ( (s1 < value && s2 >= value) || (s1 >= value && s2 < value) )
      {
      t = (value - s1) / (s2 - s1);

      // Check to see whether near the intersection is near a voxel corner. 
      // If so,have to merge requiring a change of type to type=boundary.
      if ( t < this->MergeTolerance )
        {
        this->Triangulator->UpdatePointType(internalId[edges[edgeNum][0]], 2);
        continue;
        }
      else if (t > (1.0 - this->MergeTolerance) )
        {
        this->Triangulator->UpdatePointType(internalId[edges[edgeNum][1]], 2);
        continue;
        }

      // generate edge intersection point
      cellPts->GetPoint(edges[edgeNum][0],p1);
      cellPts->GetPoint(edges[edgeNum][1],p2);
      for (i=0; i<3; i++)
        {
        x[i] = p1[i] + t * (p2[i] - p1[i]);
        }
      
      // Incorporate point into output and interpolate edge data as necessary
      if ( this->Locator->InsertUniquePoint(x, ptId) )
        {
        outPD->InterpolateEdge(inPD, ptId, cellIds->GetId(edges[edgeNum][0]),
                               cellIds->GetId(edges[edgeNum][1]), t);
        }

      //Insert into Delaunay triangulation
      this->Triangulator->InsertPoint(ptId,x,2);

      }//if edge intersects value
    }//for all edges
  
  // triangulate the points
  this->Triangulator->Triangulate();

  // Add the triangulation to the mesh
  this->Triangulator->AddTetras(0,output);
  if ( this->GenerateClippedOutput )
    {
    this->Triangulator->AddTetras(1,clippedOutput);
    }
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

  os << indent << "Generate Clip Scalars: " 
     << (this->GenerateClipScalars ? "On\n" : "Off\n");

  os << indent << "Generate Clipped Output: " 
     << (this->GenerateClippedOutput ? "On\n" : "Off\n");
}
