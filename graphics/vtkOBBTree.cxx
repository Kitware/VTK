/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOBBTree.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
#include "vtkOBBTree.h"
#include "vtkMath.h"
#include "vtkLine.h"
#include "vtkPolyData.h"

vtkOBBNode::vtkOBBNode()
{
  this->Cells = NULL;
  this->Parent = NULL;
  this->Kids = NULL;
}

vtkOBBNode::~vtkOBBNode()
{
  if (this->Kids) delete [] this->Kids;
  if (this->Cells) delete this->Cells;
}


// Description:
// Construct with automatic computation of divisions, averaging
// 25 cells per octant.
vtkOBBTree::vtkOBBTree()
{
  this->DataSet = NULL;
  this->Level = 4;
  this->MaxLevel = 12;
  this->Automatic = 1;
  this->Tolerance = 0.01;
  this->Tree = NULL;
}

void vtkOBBTree::FreeSearchStructure()
{
  if ( this->Tree )
    {
    this->DeleteTree(this->Tree);
    }
}

void vtkOBBTree::DeleteTree(vtkOBBNode *OBBptr)
{
  if ( OBBptr->Kids != NULL )
    {
    this->DeleteTree(OBBptr->Kids[0]);
    this->DeleteTree(OBBptr->Kids[1]);
    delete OBBptr->Kids[0];
    delete OBBptr->Kids[1];
    }
}

// Description:
// Compute an OBB from the list of points given. Return the corner point
// and the three axes defining the orientation of the OBB. Also return
// a sorted list of relative "sizes" of axes for comparison purposes.
void vtkOBBTree::ComputeOBB(vtkFloatPoints *pts, float corner[3], float max[3],
                            float mid[3], float min[3], float size[3])
{
  int numPts, i, pointId;
  float *x, mean[3], xp[3], *v[3], v0[3], v1[3], v2[3];
  float *a[3], a0[3], a1[3], a2[3];
  float tMin[3], tMax[3], closest[3], t;

  //
  // Compute mean
  //
  numPts = pts->GetNumberOfPoints();
  mean[0] = mean[1] = mean[2] = 0.0;
  for (pointId=0; pointId < numPts; pointId++ )
    {
    x = pts->GetPoint(pointId);
    for (i=0; i < 3; i++) mean[i] += x[i];
    }
  for (i=0; i < 3; i++) mean[i] /= numPts;

  //
  // Compute covariance matrix
  //
  a[0] = a0; a[1] = a1; a[2] = a2; 
  for (i=0; i < 3; i++) a0[i] = a1[i] = a2[i] = 0.0;

  for (pointId=0; pointId < numPts; pointId++ )
    {
    x = pts->GetPoint(pointId);
    xp[0] = x[0] - mean[0]; xp[1] = x[1] - mean[1]; xp[2] = x[2] - mean[2];
    for (i=0; i < 3; i++)
      {
      a0[i] += xp[0] * xp[i];
      a1[i] += xp[1] * xp[i];
      a2[i] += xp[2] * xp[i];
      }
    }//for all points

  for (i=0; i < 3; i++)
    {
    a0[i] /= numPts;
    a1[i] /= numPts;
    a2[i] /= numPts;
    }

  //
  // Extract axes (i.e., eigenvectors) from covariance matrix. 
  //
  v[0] = v0; v[1] = v1; v[2] = v2; 
  vtkMath::Jacobi(a,size,v);
  max[0] = v[0][0]; max[1] = v[1][0]; max[2] = v[2][0];
  mid[0] = v[0][1]; mid[1] = v[1][1]; mid[2] = v[2][1];
  min[0] = v[0][2]; min[1] = v[1][2]; min[2] = v[2][2];

  for (i=0; i < 3; i++)
    {
    a[0][i] = mean[i] + max[i];
    a[1][i] = mean[i] + mid[i];
    a[2][i] = mean[i] + min[i];
    }

  //
  // Create oriented bounding box by projecting points onto eigenvectors.
  //
  tMin[0] = tMin[1] = tMin[2] = VTK_LARGE_FLOAT;
  tMax[0] = tMax[1] = tMax[2] = -VTK_LARGE_FLOAT;

  for (pointId=0; pointId < numPts; pointId++ )
    {
    x = pts->GetPoint(pointId);
    for (i=0; i < 3; i++)
      {
      vtkLine::DistanceToLine(x, mean, a[i], t, closest);
      if ( t < tMin[i] ) tMin[i] = t;
      if ( t > tMax[i] ) tMax[i] = t;
      }
    }//for all points

  for (i=0; i < 3; i++)
    {
    corner[i] = mean[i] + tMin[0]*max[i] + tMin[1]*mid[i] + tMin[2]*min[i];

    max[i] = (tMax[0] - tMin[0]) * max[i];
    mid[i] = (tMax[1] - tMin[1]) * mid[i];
    min[i] = (tMax[2] - tMin[2]) * min[i];
    }
}

// Description:
// Return intersection point of line defined by two points (a0,a1) in dataset
// coordinate system; returning cellId (or -1 if no intersection). The 
// argument list returns the intersection parametric coordinate, t, along 
// the line; the coordinate of intersection, x[3]; the cell parametric
// coordinates, pcoords[3]; and subId of the cell. (Not yet implemented.)
int vtkOBBTree::IntersectWithLine(float a0[3], float a1[3], float& t, 
                                  float x[3], float pcoords[3],
                                  int &subId)
{
  return -1;
}

//
//  Method to form subdivision of space based on the cells provided and
//  subject to the constraints of levels and NumberOfCellsInOctant.
//  The result is directly addressable and of uniform subdivision.
//
void vtkOBBTree::BuildLocator()
{
  int numPts, numCells, i;
  vtkIdList *cellList;

  vtkDebugMacro(<<"Building OBB tree");
  if ( this->Tree != NULL && this->BuildTime > this->MTime ) return;

  numPts = this->DataSet->GetNumberOfPoints();
  numCells = this->DataSet->GetNumberOfCells();
  if ( this->DataSet == NULL || numPts < 1 || numCells < 1 )
    {
    vtkErrorMacro(<<"Can't build OBB tree - no data available!");
    return;
    }

  this->OBBCount = 0;
  this->InsertedPoints = new int[numPts];
  for (i=0; i < numPts; i++) this->InsertedPoints[i] = 0;
  this->PointsList = vtkFloatPoints::New();
  this->PointsList->Allocate(numPts);

  //
  // Begin recursively creating OBB's
  //
  cellList = vtkIdList::New();
  cellList->Allocate(numCells);
  for (i=0; i < numCells; i++)
    {
    cellList->InsertId(i,i);
    }

  this->Tree = new vtkOBBNode;
  this->DeepestLevel = 0;
  this->BuildTree(cellList,this->Tree,0);
  this->Level = this->DeepestLevel;

  vtkDebugMacro(<<"Deepest tree level: " << this->DeepestLevel
                <<", Created: " << OBBCount << " OBB nodes");

  //
  // Clean up
  //
  delete [] this->InsertedPoints;
  this->PointsList->Delete();

  this->BuildTime.Modified();
}

void vtkOBBTree::BuildTree(vtkIdList *cells, vtkOBBNode *OBBptr, int level)
{
  int i, j, numCells=cells->GetNumberOfIds();
  int cellId, ptId;
  vtkIdList cellPts;
  float size[3];

  if ( level > this->DeepestLevel ) this->DeepestLevel = level;
  this->PointsList->Reset();
  //
  // Gather all the cell's point coordinates into a single list
  //
  this->OBBCount++;
  for ( i=0; i < numCells; i++ )
    {
    cellId = cells->GetId(i);
    this->DataSet->GetCellPoints(cellId, cellPts);
    for ( j=0; j < cellPts.GetNumberOfIds(); j++ )
      {
      if ( this->InsertedPoints[(ptId = cellPts.GetId(j))] != this->OBBCount )
        {
        this->InsertedPoints[ptId] = this->OBBCount;
        this->PointsList->InsertNextPoint(this->DataSet->GetPoint(ptId));
        }
      }//for all points of this cell
    }//for all cells

  //
  // Now compute the OBB
  //
  this->ComputeOBB(this->PointsList, OBBptr->Corner, OBBptr->Axes[0], 
                   OBBptr->Axes[1], OBBptr->Axes[2], size);

  //
  // Check whether to continue recursing; if so, create two children and
  // assign cells to appropriate child.
  //
  if ( level < this->MaxLevel && numCells > this->NumberOfCellsPerBucket )
    {
    vtkIdList *LHlist = vtkIdList::New();
    LHlist->Allocate(cells->GetNumberOfIds()/2);
    vtkIdList *RHlist = vtkIdList::New();
    RHlist->Allocate(cells->GetNumberOfIds()/2);
    float n[3], p[3], *x, val, ratio, bestRatio;
    int negative, positive, splitAcceptable, splitPlane;
    int foundBestSplit, bestPlane=0, numStraddles;
    int numInLHnode, numInRHnode;

    //loop over three split planes to find acceptable one
    for (i=0; i < 3; i++) //compute split point
      {
      p[i] = OBBptr->Corner[i] + OBBptr->Axes[0][i]/2.0 + 
             OBBptr->Axes[1][i]/2.0 + OBBptr->Axes[2][i]/2.0;
      }

    bestRatio = VTK_LARGE_FLOAT;
    foundBestSplit = 0;
    numStraddles = 0;
    for (splitPlane=0,splitAcceptable=0; !splitAcceptable && splitPlane < 3; )
      {
      // compute split normal
      for (i=0 ; i < 3; i++) n[i] = OBBptr->Axes[splitPlane][i];
      vtkMath::Normalize(n);

      //traverse cells, assigning to appropriate child list as necessary
      for ( i=0; i < numCells; i++ )
        {
        cellId = cells->GetId(i);
        this->DataSet->GetCellPoints(cellId, cellPts);
        for ( negative=positive=j=0; j < cellPts.GetNumberOfIds(); j++ )
          {
          ptId = cellPts.GetId(j);
          x = this->DataSet->GetPoint(ptId);
          val = n[0]*(x[0]-p[0]) + n[1]*(x[1]-p[1]) + n[2]*(x[2]-p[2]);
          if ( val < 0.0 ) negative = 1;
          else positive = 1;
          }

        if ( negative ) LHlist->InsertNextId(cellId);
        else RHlist->InsertNextId(cellId);

        if ( negative && positive ) numStraddles++;

        }//for all cells

      //evaluate this split
      numInLHnode = LHlist->GetNumberOfIds() - numStraddles;
      numInRHnode = RHlist->GetNumberOfIds() - numStraddles;
      if (  numInLHnode < 1 || numInRHnode < 1 )
        {
        ratio = VTK_LARGE_FLOAT / 2.0;
        }
      else 
        {
        ratio = fabs((double) numInRHnode / numInLHnode ) - 1.0;
        }

      //see whether we've found acceptable split plane       
      if ( ratio < 10.0 ) //accept right off the bat
        { 
        splitAcceptable = 1;
        }
      else if ( foundBestSplit ) //force termination of this loop
        {
        if ( ratio < 100.0 ) splitAcceptable = 1;
        splitPlane = 10;
        }
      else //not a great split try another
        {
        LHlist->Reset();
        RHlist->Reset();
        if ( ratio < bestRatio ) 
          {
          bestRatio = ratio;;
          bestPlane = splitPlane;
          }
        }//don't accept split

      if ( ++splitPlane == 3 && bestRatio < VTK_LARGE_FLOAT )
        {
        splitPlane = bestPlane;
        foundBestSplit = 1;
        }

      }//for each split

    if ( splitAcceptable ) //otherwise recursion terminates
      {
      vtkOBBNode *LHnode= new vtkOBBNode;
      vtkOBBNode *RHnode= new vtkOBBNode;
      OBBptr->Kids = new vtkOBBNode *[2];
      OBBptr->Kids[0] = LHnode;
      OBBptr->Kids[1] = RHnode;
      LHnode->Parent = OBBptr;
      RHnode->Parent = OBBptr;

      delete cells; cells = NULL; //don't need to keep anymore
      this->BuildTree(LHlist, LHnode, level+1);
      this->BuildTree(RHlist, RHnode, level+1);
      }
    }//if should build tree

  if ( cells && this->RetainCellLists ) 
    {
    cells->Squeeze();
    OBBptr->Cells = cells;
    }
  else if ( cells )
    {
    delete cells;
    }
}

// Description:
// Create polygonal representation for OBB tree at specified level. If 
// level < 0, then the leaf OBB nodes will be gathered. The aspect ratio (ar)
// and line diameter (d) are used to control the building of the 
// representation. If a OBB node edge ratio's are greater than ar, then the
// dimension of the OBB is collapsed (OBB->plane->line). A "line" OBB will be
// represented either as two crossed polygons, or as a line, depending on
// the relative diameter of the OBB compared to the diameter (d).

void vtkOBBTree::GenerateRepresentation(int level, vtkPolyData *pd)
{
  vtkFloatPoints *pts;
  vtkCellArray *polys;

  if ( this->Tree == NULL )
    {
    vtkErrorMacro(<<"No tree to generate representation from");
    return;
    }

  pts = vtkFloatPoints::New();
  pts->Allocate(5000);
  polys = vtkCellArray::New();
  polys->Allocate(10000);
  this->GeneratePolygons(this->Tree,0,level,pts,polys);

  pd->SetPoints(pts);
  pts->Delete();
  pd->SetPolys(polys);
  polys->Delete();
  pd->Squeeze();
}

void vtkOBBTree::GeneratePolygons(vtkOBBNode *OBBptr, int level, int repLevel,
                                  vtkFloatPoints *pts, vtkCellArray *polys)

{
  if ( level == repLevel || (repLevel < 0 && OBBptr->Kids == NULL) )
    {
    float x[3];
    int ptIds[4], cubeIds[8];

    x[0] = OBBptr->Corner[0];
    x[1] = OBBptr->Corner[1];
    x[2] = OBBptr->Corner[2];
    cubeIds[0] = pts->InsertNextPoint(x);

    x[0] = OBBptr->Corner[0] + OBBptr->Axes[0][0];
    x[1] = OBBptr->Corner[1] + OBBptr->Axes[0][1];
    x[2] = OBBptr->Corner[2] + OBBptr->Axes[0][2];
    cubeIds[1] = pts->InsertNextPoint(x);

    x[0] = OBBptr->Corner[0] + OBBptr->Axes[1][0];
    x[1] = OBBptr->Corner[1] + OBBptr->Axes[1][1];
    x[2] = OBBptr->Corner[2] + OBBptr->Axes[1][2];
    cubeIds[2] = pts->InsertNextPoint(x);

    x[0] = OBBptr->Corner[0] + OBBptr->Axes[0][0] + OBBptr->Axes[1][0];
    x[1] = OBBptr->Corner[1] + OBBptr->Axes[0][1] + OBBptr->Axes[1][1];
    x[2] = OBBptr->Corner[2] + OBBptr->Axes[0][2] + OBBptr->Axes[1][2];
    cubeIds[3] = pts->InsertNextPoint(x);

    x[0] = OBBptr->Corner[0] + OBBptr->Axes[2][0];
    x[1] = OBBptr->Corner[1] + OBBptr->Axes[2][1];
    x[2] = OBBptr->Corner[2] + OBBptr->Axes[2][2];
    cubeIds[4] = pts->InsertNextPoint(x);

    x[0] = OBBptr->Corner[0] + OBBptr->Axes[0][0] + OBBptr->Axes[2][0];
    x[1] = OBBptr->Corner[1] + OBBptr->Axes[0][1] + OBBptr->Axes[2][1];
    x[2] = OBBptr->Corner[2] + OBBptr->Axes[0][2] + OBBptr->Axes[2][2];
    cubeIds[5] = pts->InsertNextPoint(x);

    x[0] = OBBptr->Corner[0] + OBBptr->Axes[1][0] + OBBptr->Axes[2][0];
    x[1] = OBBptr->Corner[1] + OBBptr->Axes[1][1] + OBBptr->Axes[2][1];
    x[2] = OBBptr->Corner[2] + OBBptr->Axes[1][2] + OBBptr->Axes[2][2];
    cubeIds[6] = pts->InsertNextPoint(x);

    x[0] = OBBptr->Corner[0] + OBBptr->Axes[0][0] + OBBptr->Axes[1][0] 
           + OBBptr->Axes[2][0];
    x[1] = OBBptr->Corner[1] + OBBptr->Axes[0][1] + OBBptr->Axes[1][1] 
           + OBBptr->Axes[2][1];
    x[2] = OBBptr->Corner[2] + OBBptr->Axes[0][2] + OBBptr->Axes[1][2] 
           + OBBptr->Axes[2][2];
    cubeIds[7] = pts->InsertNextPoint(x);

    ptIds[0] = cubeIds[0]; ptIds[1] = cubeIds[2]; 
    ptIds[2] = cubeIds[3]; ptIds[3] = cubeIds[1];
    polys->InsertNextCell(4,ptIds);

    ptIds[0] = cubeIds[0]; ptIds[1] = cubeIds[1]; 
    ptIds[2] = cubeIds[5]; ptIds[3] = cubeIds[4];
    polys->InsertNextCell(4,ptIds);

    ptIds[0] = cubeIds[0]; ptIds[1] = cubeIds[4]; 
    ptIds[2] = cubeIds[6]; ptIds[3] = cubeIds[2];
    polys->InsertNextCell(4,ptIds);

    ptIds[0] = cubeIds[1]; ptIds[1] = cubeIds[3]; 
    ptIds[2] = cubeIds[7]; ptIds[3] = cubeIds[5];
    polys->InsertNextCell(4,ptIds);

    ptIds[0] = cubeIds[4]; ptIds[1] = cubeIds[5]; 
    ptIds[2] = cubeIds[7]; ptIds[3] = cubeIds[6];
    polys->InsertNextCell(4,ptIds);

    ptIds[0] = cubeIds[2]; ptIds[1] = cubeIds[6]; 
    ptIds[2] = cubeIds[7]; ptIds[3] = cubeIds[3];
    polys->InsertNextCell(4,ptIds);
    }

  else if ( (level < repLevel || repLevel < 0) && OBBptr->Kids != NULL )
    {
    this->GeneratePolygons(OBBptr->Kids[0],level+1,repLevel,pts,polys);
    this->GeneratePolygons(OBBptr->Kids[1],level+1,repLevel,pts,polys);
    }
}
