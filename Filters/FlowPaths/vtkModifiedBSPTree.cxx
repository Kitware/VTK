/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkModifiedBSPTree.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkObjectFactory.h"
#include "vtkModifiedBSPTree.h"
#include "vtkPolyData.h"
#include "vtkGenericCell.h"
#include "vtkIdListCollection.h"

#include <stack>
#include <vector>
#include <algorithm>
#include <functional>

#include <vtkAppendPolyData.h>
#include <vtkCubeSource.h>
//
//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkModifiedBSPTree);
//----------------------------------------------------------------------------
//
//
enum { POS_X, NEG_X, POS_Y, NEG_Y, POS_Z, NEG_Z };
//
const double Epsilon_=1E-8;

//////////////////////////////////////////////////////////////////////////////
// Main management and support for tree
//////////////////////////////////////////////////////////////////////////////
vtkModifiedBSPTree::vtkModifiedBSPTree(void)
{
  this->NumberOfCellsPerNode       = 32;
  this->mRoot                      = NULL;
  this->UseExistingSearchStructure = 0;
  this->LazyEvaluation             = 1;
  //
  this->npn = this->nln = this->tot_depth = 0;
}
//---------------------------------------------------------------------------
vtkModifiedBSPTree::~vtkModifiedBSPTree(void)
{
  this->FreeSearchStructure();
  this->FreeCellBounds();
}
//---------------------------------------------------------------------------
void vtkModifiedBSPTree::FreeSearchStructure(void)
{
  if (this->mRoot)
    {
    delete this->mRoot;
    }
  this->mRoot                     = NULL;
  this->Level        = 0;
  this->npn = this->nln = this->tot_depth = 0;
}
//---------------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////////
// Here's the stuff for spatial subdivision
//////////////////////////////////////////////////////////////////////////////
class cell_extents
{
public:
  double    min, max;
  vtkIdType cell_ID;
};

typedef cell_extents *cell_extents_List;

static int global_list_count = 0;

class Sorted_cell_extents_Lists
{
public:
  cell_extents_List Mins[3];
  cell_extents_List Maxs[3];
  //
  Sorted_cell_extents_Lists(vtkIdType nCells)
  {
    for (int i=0; i<3; i++)
      {
      Mins[i] = new cell_extents[nCells]; // max num <= nCells/2 ?
      Maxs[i] = new cell_extents[nCells];
      }
    global_list_count += 1;
  };
  ~Sorted_cell_extents_Lists(void)
  {
    for (int i=0; i<3; i++)
      {
      delete [](Mins[i]);
      delete [](Maxs[i]);
      }
    global_list_count -= 1;
  }
};

extern "C" int __compareMin(const void *pA, const void *B )
{
  cell_extents *tA  = (cell_extents *) pA;
  cell_extents *tB  = (cell_extents *) B;
  if ( tA->min == tB->min )
    {
    return 0;
    }
  else
    {
    return tA->min < tB->min ? -1 : 1 ;
    }
}

extern "C" int __compareMax(const void *pA, const void *B )
{
  cell_extents *tA  = (cell_extents *) pA;
  cell_extents *tB  = (cell_extents *) B;
  if ( tA->max == tB->max )
    {
    return 0;
    }
  else
    {
    return tA->max > tB->max ? -1 : 1;
    }
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
void vtkModifiedBSPTree::BuildLocator()
{
  if (this->LazyEvaluation)
    {
    return;
    }
  this->ForceBuildLocator();
}
//---------------------------------------------------------------------------
void vtkModifiedBSPTree::BuildLocatorIfNeeded()
{
  if (this->LazyEvaluation)
    {
    if (!this->mRoot || (this->mRoot && (this->MTime>this->BuildTime)))
      {
      this->Modified();
      vtkDebugMacro(<< "Forcing BuildLocator");
      this->ForceBuildLocator();
      }
    }
}
//---------------------------------------------------------------------------
void vtkModifiedBSPTree::ForceBuildLocator()
{
  //
  // don't rebuild if build time is newer than modified and dataset modified time
  if ( (this->mRoot) &&
       (this->BuildTime>this->MTime) &&
       (this->BuildTime>DataSet->GetMTime()))
    {
    return;
    }
  // don't rebuild if UseExistingSearchStructure is ON and a tree structure already exists
  if ( (this->mRoot) && this->UseExistingSearchStructure)
    {
    this->BuildTime.Modified();
    vtkDebugMacro(<< "BuildLocator exited - UseExistingSearchStructure");
    return;
    }
  this->BuildLocatorInternal();
}
//---------------------------------------------------------------------------
void vtkModifiedBSPTree::BuildLocatorInternal()
{
  //
  vtkIdType numCells;
  if ( !this->DataSet ||
       (numCells = this->DataSet->GetNumberOfCells()) < 1 )
    {
    vtkDebugMacro( << "No Cells to divide");
    numCells = 0;
    }
  vtkDebugMacro( << "Creating BSPTree for " << numCells << " cells");

  //
  //  Make sure the appropriate data is available
  //
  this->FreeSearchStructure();
  this->FreeCellBounds();

  // create the root node
  this->mRoot = new BSPNode();
  this->mRoot->mAxis = rand() % 3;
  this->mRoot->depth = 0;
  //
  if (numCells==0)
    {
    return;
    }
  //
  this->StoreCellBounds();
  //
  // sort the cells into 6 lists using structure for subdividing tests
  Sorted_cell_extents_Lists *lists = new Sorted_cell_extents_Lists(numCells);
  for (int i=0; i<3; i++)
    { // loop over each axis
    for (vtkIdType j=0; j<numCells; j++)
      { // loop over each cell
      lists->Mins[i][j].min   = CellBounds[j][i*2];   // i=0 xmin, i=1 ymin, i=2 zmin
      lists->Mins[i][j].max   = CellBounds[j][i*2+1]; // i=0 xmax, i=1 ymax, i=2 zmax
      lists->Mins[i][j].cell_ID = j;
      //
      lists->Maxs[i][j].min   = CellBounds[j][i*2];
      lists->Maxs[i][j].max   = CellBounds[j][i*2+1];
      lists->Maxs[i][j].cell_ID = j;
      }
    // Sort
    qsort( lists->Mins[i], numCells, sizeof(cell_extents), __compareMin) ;
    qsort( lists->Maxs[i], numCells, sizeof(cell_extents), __compareMax) ;
    }
  //
  // call the recursive subdivision routine
  //
  vtkDebugMacro( << "Beginning Subdivision" );
  //
  if (numCells>0)
    {
    Subdivide(this->mRoot, lists, this->DataSet, numCells, 0,
              this->MaxLevel, this->NumberOfCellsPerNode, this->Level);
    }
  delete lists;
  // Child nodes are responsible for freeing the temporary sorted lists
  //
  this->BuildTime.Modified();
  //
  double av_depth = (double)tot_depth/nln; (void)av_depth;
  vtkDebugMacro( << "BSP Tree Statistics \n"
                 << "Num Parent/Leaf Nodes " << npn
                 << "/" << nln << "\n"
                 << "Average Depth " << av_depth
                 << " Original : " << numCells);
}

//
// The main BSP subdivision routine : The code which does the division is only
// a small part of this, the rest is just bookkeeping - it looks worse than it is.
//
void vtkModifiedBSPTree::Subdivide(BSPNode *node,
                                   Sorted_cell_extents_Lists *lists,
                                   vtkDataSet *dataset,
                                   vtkIdType nCells,
                                   int depth,
                                   int maxlevel,
                                   vtkIdType maxCells,
                                   int &MaxDepth)
{
  //
  // We've got lists sorted on the axes, so we can easily get BBox
  node->setMin( lists->Mins[0][0].min,
                lists->Mins[1][0].min,
                lists->Mins[2][0].min );
  node->setMax( lists->Maxs[0][0].max,
                lists->Maxs[1][0].max,
                lists->Maxs[2][0].max );
  // Update depth info
  if (node->depth>MaxDepth)
    {
    MaxDepth = depth;
    }
  //
  // Make sure child nodes are clear to start with
  node->mChild[2] = node->mChild[1] = node->mChild[0] = NULL;
  //
  // Do we want to subdivide this node ?
  //
  double pDiv = 0.0;
  if ((nCells  > maxCells) && (depth < maxlevel))
    {
    // test for optimal subdivision
    bool      found = false, abort = false;
    int       Daxis;
    vtkIdType TargetCount = (3*nCells)/4;
    //
    for (vtkIdType k,j=0; j<nCells && !found && !abort; j++)
      {
      // for each axis..
      // test to see which x,y,z axis we should divide along
      for (Daxis=node->mAxis, k=0; k<3; Daxis=(Daxis+1)%3, k++)
        {
        // eg for X axis, move left to right, and right to left
        // when left overlaps right stop - at the same time, scan down and up
        // in and out, and whichever crosses first - bingo !
        if (lists->Mins[Daxis][j].min > lists->Maxs[Daxis][j].max)
          {
          pDiv = lists->Mins[Daxis][j].min - Epsilon_;
          node->mAxis = Daxis;
          found = true;
          break;
          }
        else
          {
          // if we have searched more than 3/4 of the cells and still
          // not found a good plane, then abort division for this node
          if (j>=TargetCount)
            {
            // vtkDebugMacro("Aborted node division : excessive overlap : Cell count = " << nCells);
            abort = true;
            break;
            }
          }
        }
      }
    // construct the 3 children
    if (found)
      {
      for (int i=0; i<3; i++)
        {
        node->mChild[i]    = new BSPNode();
        node->mChild[i]->depth = node->depth+1;
        node->mChild[i]->mAxis = rand() % 3;
        }
      Daxis = node->mAxis;
      Sorted_cell_extents_Lists *left  = new Sorted_cell_extents_Lists(nCells);
      Sorted_cell_extents_Lists *mid   = new Sorted_cell_extents_Lists(nCells);
      Sorted_cell_extents_Lists *right = new Sorted_cell_extents_Lists(nCells);
      // we ought to keep track of how many we are adding to each list
      vtkIdType Cmin_l[3] = {0, 0, 0},
        Cmin_m[3] = {0, 0, 0},
          Cmin_r[3] = {0, 0, 0};
          vtkIdType Cmax_l[3] = {0, 0, 0},
            Cmax_m[3] = {0, 0, 0},
              Cmax_r[3] = {0, 0, 0};
              // Partition the cells into the correct child lists
              // here we use the lists for the axis we're dividing along
              for (vtkIdType i=0; i<nCells; i++)
                {
                // process the MIN-List
                cell_extents ext = lists->Mins[Daxis][i];
                // max is on left of middle node
                if    (ext.max < pDiv)
                  {
                  left ->Mins[Daxis][Cmin_l[Daxis]++] = ext;
                  }
                // min is on right of middle node
                else if (ext.min > pDiv)
                  {
                  right->Mins[Daxis][Cmin_r[Daxis]++] = ext;
                  }
                // neither - must be one of ours
                else
                  {
                  mid  ->Mins[Daxis][Cmin_m[Daxis]++] = ext;
                  }
                //
                // process the MAX-List
                ext = lists->Maxs[Daxis][i];
                // max is on left of middle node
                if    (ext.max < pDiv)
                  {
                  left ->Maxs[Daxis][Cmax_l[Daxis]++] = ext;
                  }
                // min is on right of middle node
                else if (ext.min > pDiv)
                  {
                  right->Maxs[Daxis][Cmax_r[Daxis]++] = ext;
                  }
                // neither - must be one of ours
                else
                  {
                  mid  ->Maxs[Daxis][Cmax_m[Daxis]++] = ext;
                  }
                }
              // construct the sorted list of extents for the 2 remaining axes
              // do everything in order so our sorted lists aren't munged
              for (Daxis=(node->mAxis+1)%3; Daxis!=node->mAxis; Daxis=(Daxis+1)%3)
                {
                for (vtkIdType i=0; i<nCells; i++)
                  {
                  // process the MIN-List
                  cell_extents ext = lists->Mins[Daxis][i];
                  if (this->CellBounds[ext.cell_ID][2*node->mAxis+1] < pDiv)
                    {
                    left ->Mins[Daxis][Cmin_l[Daxis]++] = ext;
                    }
                  else if (this->CellBounds[ext.cell_ID][2*node->mAxis] > pDiv)
                    {
                    right->Mins[Daxis][Cmin_r[Daxis]++] = ext;
                    }
                  else
                    {
                    mid  ->Mins[Daxis][Cmin_m[Daxis]++] = ext;
                    }
                  //
                  // process the MAX-List
                  ext = lists->Maxs[Daxis][i];
                  if (this->CellBounds[ext.cell_ID][2*node->mAxis+1] < pDiv)
                    {
                    left ->Maxs[Daxis][Cmax_l[Daxis]++] = ext;
                    }
                  else if (this->CellBounds[ext.cell_ID][2*node->mAxis] > pDiv)
                    {
                    right->Maxs[Daxis][Cmax_r[Daxis]++] = ext;
                    }
                  else
                    {
                    mid  ->Maxs[Daxis][Cmax_m[Daxis]++] = ext;
                    }
                  }
                }
              //
              // Better check we didn't make a diddly
              // this is overkill but for now I want a FULL DEBUG!
              if ( (Cmin_l[0] + Cmin_r[0] + Cmin_m[0])!=nCells )
                {
                vtkWarningMacro("Error count in min lists");
                }
              if ( (Cmin_l[1] + Cmin_r[1] + Cmin_m[1])!=nCells )
                {
                vtkWarningMacro("Error count in min lists");
                }
              if ( (Cmin_l[2] + Cmin_r[2] + Cmin_m[2])!=nCells )
                {
                vtkWarningMacro("Error count in min lists");
                }
              if ( (Cmax_l[0] + Cmax_r[0] + Cmax_m[0])!=nCells )
                {
                vtkWarningMacro("Error count in max lists");
                }
              if ( (Cmax_l[1] + Cmax_r[1] + Cmax_m[1])!=nCells )
                {
                vtkWarningMacro("Error count in max lists");
                }
              if ( (Cmax_l[2] + Cmax_r[2] + Cmax_m[2])!=nCells )
                {
                vtkWarningMacro("Error count in max lists");
                }
              //
              // Bug : Can sometimes get unbalanced leaves
              //
              if (!Cmin_l[0] || !Cmin_r[0])
                {
                // vtkDebugMacro(<<"Child 0 or 2 empty : Aborting subdivision for node " << Cmin_l[0] << " " << Cmin_m[0] << " " << Cmin_r[0]);
                // clean up all the memory we allocated. Yikes.
                for (int i=0; i<3; i++)
                  {
                  delete node->mChild[i];
                  node->mChild[i] = NULL;
                  }
                delete left;
                delete mid;
                delete right;
                }
              else
                {
                //
                // Now we can delete the lists that the parent passed on to us
                //
                //
                // And of course, we really ought to subdivide again - Hoorah!
                // NB: it is possible for a node to be empty now, so check and delete if necessary
                if (Cmin_l[0])
                  {
                  Subdivide(node->mChild[0], left, dataset, Cmin_l[0], depth+1, maxlevel, maxCells, MaxDepth);
                  }
                else
                  {
                  vtkWarningMacro(<< "Child 0 Empty ! - this shouldn't happen");
                  }
                delete left;

                if (Cmin_m[0])
                  {
                  Subdivide(node->mChild[1], mid,  dataset, Cmin_m[0], depth+1, maxlevel, maxCells, MaxDepth);
                  }
                else
                  {
                  delete node->mChild[1]; node->mChild[1] = NULL;
                  }
                delete mid;

                if (Cmin_r[0])
                  {
                  Subdivide(node->mChild[2], right,dataset, Cmin_r[0], depth+1, maxlevel, maxCells, MaxDepth);
                  }
                else
                  {
                  vtkWarningMacro(<< "Child 2 Empty ! - this shouldn't happen");
                  }
                delete right;
                //
                npn += 1; // Parent node
                //
                // we've done all we were asked to do
                //
                return;
                }
      }
    }
  // if we got here, either no further subdivision is necessary,
  // or we couldn't find a split plane...(or we aborted)
  //
  // Copy the cell IDs into the actual node structure for proper use
  node->num_cells = nCells;
  nln += 1; // Leaf node
  tot_depth += node->depth;
  for (int i=0; i<6; i++)
    {
    node->sorted_cell_lists[i] = new vtkIdType[nCells];
    }
  //
  for (int i=0; i<3; i++)
    {
    for (vtkIdType j=0; j<nCells; j++)
      {
      node->sorted_cell_lists[i*2][j]   = lists->Mins[i][j].cell_ID;
      node->sorted_cell_lists[i*2+1][j] = lists->Maxs[i][j].cell_ID;
      }
    }
  // Thank buggery that's all over.
}

//////////////////////////////////////////////////////////////////////////////
// Generate representation for viewing structure
//////////////////////////////////////////////////////////////////////////////
// OK so this is a quick a dirty one for testing, but I can't be arsed
// working out which faces are visible

class _box
{
public:
  double bounds[6];
  _box(double *b)
  {
    for (int i=0; i<6; i++)
      {
      bounds[i] = b[i];
      }
  };
};

typedef std::vector<_box> boxlist;
typedef std::stack<BSPNode*, std::vector<BSPNode*> > nodestack;

void vtkModifiedBSPTree::GenerateRepresentation(int level, vtkPolyData *pd)
{
  nodestack ns;
  boxlist   bl;
  BSPNode   *node;
  this->BuildLocatorIfNeeded();
  ns.push(this->mRoot);
  // lets walk the tree and get all the level n node boxes
  while (!ns.empty())
    {
    node = ns.top();
    ns.pop();
    if (node->depth==level)
      {
      bl.push_back(_box(node->bounds));
      }
    else
      {
      if (node->mChild[0])
        {
        ns.push(node->mChild[0]);
        if (node->mChild[1])
          {
          ns.push(node->mChild[1]);
          }
        ns.push(node->mChild[2]);
        }
      else if (level==-1)
        {
        bl.push_back(_box(node->bounds));
        }
      }
    }

  // Ok, now create cube(oid)s and stuff'em into a polydata thingy
  vtkAppendPolyData *polys = vtkAppendPolyData::New();
  size_t s = bl.size();
  for (size_t i=0; i<s; i++)
    {
    vtkCubeSource *cube = vtkCubeSource::New();
    cube->SetBounds( bl[i].bounds );
    cube->Update();
    polys->AddInputConnection(cube->GetOutputPort());
    cube->Delete();
    }
  polys->Update();
  pd->SetPoints(polys->GetOutput()->GetPoints());
  pd->SetPolys(polys->GetOutput()->GetPolys());
  polys->Delete();
}

void vtkModifiedBSPTree::GenerateRepresentationLeafs(vtkPolyData *pd)
{
  GenerateRepresentation(-1,pd);
}

//////////////////////////////////////////////////////////////////////////////
// Ray/BSPtree Intersection stuff
//////////////////////////////////////////////////////////////////////////////

// Ray->Box edge t-distance tests
double _getMinDistPOS_X(const double origin[3], const double dir[3], const double B[6])
{
  return ((B[0] - origin[0]) / dir[0]);
}
double _getMinDistNEG_X(const double origin[3], const double dir[3], const double B[6])
{
  return ((B[1] - origin[0]) / dir[0]);
}
double _getMinDistPOS_Y(const double origin[3], const double dir[3], const double B[6])
{
  return ((B[2] - origin[1]) / dir[1]);
}
double _getMinDistNEG_Y(const double origin[3], const double dir[3], const double B[6])
{
  return ((B[3] - origin[1]) / dir[1]);
}
double _getMinDistPOS_Z(const double origin[3], const double dir[3], const double B[6])
{
  return ((B[4] - origin[2]) / dir[2]);
}
double _getMinDistNEG_Z(const double origin[3], const double dir[3], const double B[6])
{
  return ((B[5] - origin[2]) / dir[2]);
}

int BSPNode::getDominantAxis(const double dir[3])
{
  double tX = (dir[0]>0) ? dir[0] : -dir[0];
  double tY = (dir[1]>0) ? dir[1] : -dir[1];
  double tZ = (dir[2]>0) ? dir[2] : -dir[2];
  if (tX > tY && tX > tZ)
    {
    return ((dir[0] > 0) ? POS_X : NEG_X);
    }
  else if ( tY > tZ )
    {
    return ((dir[1] > 0) ? POS_Y : NEG_Y);
    }
  else
    {
    return ((dir[2] > 0) ? POS_Z : NEG_Z);
    }
}
//---------------------------------------------------------------------------
int vtkModifiedBSPTree::IntersectWithLine(double p1[3],
                                          double p2[3],
                                          double tol,
                                          double &t,
                                          double x[3],
                                          double pcoords[3],
                                          int &subId,
                                          vtkIdType &cellId,
                                          vtkGenericCell *cell)
{
  int hit = this->IntersectWithLine(p1, p2, tol, t, x, pcoords, subId, cellId);
  if (hit)
    {
    this->DataSet->GetCell(cellId, cell);
    }
  return hit;
}
//---------------------------------------------------------------------------
int vtkModifiedBSPTree::IntersectWithLine(double p1[3], double p2[3], double tol,
                                          double &t, double x[3], double pcoords[3], int &subId, vtkIdType &cellId)
{
  //
  BSPNode  *node, *Near, *Mid, *Far;
  double    ctmin, ctmax, tmin, tmax, _tmin, _tmax, tDist;
  double    ray_vec[3] = { p2[0]-p1[0], p2[1]-p1[1], p2[2]-p1[2] };
  //
  this->BuildLocatorIfNeeded();
  //
  // Does ray pass through root BBox
  tmin = 0; tmax = 1;
  if (!this->mRoot->RayMinMaxT(p1, ray_vec, tmin, tmax))
    {
    return false;
    }
  // Ok, setup a stack and various params
  nodestack  ns;
  double    closest_intersection = VTK_LARGE_FLOAT;
  bool     HIT = false;
  // setup our axis optimized ray box edge stuff
  int axis = BSPNode::getDominantAxis(ray_vec);
  double (*_getMinDist)(const double origin[3], const double dir[3], const double B[6]);
  switch (axis)
    {
    case POS_X: _getMinDist = _getMinDistPOS_X; break;
    case NEG_X: _getMinDist = _getMinDistNEG_X; break;
    case POS_Y: _getMinDist = _getMinDistPOS_Y; break;
    case NEG_Y: _getMinDist = _getMinDistNEG_Y; break;
    case POS_Z: _getMinDist = _getMinDistPOS_Z; break;
    default:    _getMinDist = _getMinDistNEG_Z; break;
    }
  //
  // OK, lets walk the tree and find intersections
  //
  ns.push(this->mRoot);
  while (!ns.empty())
    {
    node = ns.top();
    ns.pop();
    // We do as few tests on the way down as possible, because our BBoxes
    // can be quite tight and we want to reject as many boxes as possible without
    // testing them at all - mainly because we quickly get to a leaf node and
    // test candidates, once we've found a hit, we note the intersection t val,
    // as soon as we pull a BBox of the stack that has a closest point further
    // than the t val, we know we can stop.
    //
    while (node->mChild[0])  // this must be a parent node
      {
      // Which child node is closest to ray origin - given direction
      node->Classify(p1, ray_vec, tDist, Near, Mid, Far);
      // if the distance to the far edge of the near box is > tmax, no need to test far box
      // (we still need to test Mid because it may overlap slightly)
      if ((tDist > tmax) || (tDist <= 0) )  // <=0 for ray on edge
        {
        if (Mid)
          {
          ns.push(Mid);
          }
        node = Near;
        }
      // if the distance to the far edge of the near box is < tmin, no need to test near box
      else if (tDist < tmin)
        {
        if (Mid)
          {
          ns.push(Far);
          node = Mid;
          }
        else
          {
          node = Far;
          }
        }
      // All the child nodes may be candidates, keep near, push far then mid
      else
        {
        ns.push(Far);
        if (Mid)
          {
          ns.push(Mid);
          }
        node = Near;
        }
      }
    double t_hit, ipt[3];
    // Ok, so we're a leaf node, first check the BBox against the ray
    // then test the candidates in our sorted ray direction order
    _tmin = tmin; _tmax = tmax;
//    if (node->RayMinMaxT(p1, ray_vec, _tmin, _tmax)) {
    // Was the closest point on the box was > intersection point
//      if (_tmax>closest_intersection) break;
    //
    for (int i=0; i<node->num_cells; i++)
      {
      vtkIdType cell_ID = node->sorted_cell_lists[axis][i];
      //
      if (_getMinDist(p1, ray_vec, CellBounds[cell_ID]) > closest_intersection)
        {
        break;
        }
      //
      ctmin = _tmin; ctmax = _tmax;
      if (BSPNode::RayMinMaxT(CellBounds[cell_ID], p1, ray_vec, ctmin, ctmax))
        {
        if (this->IntersectCellInternal(cell_ID, p1, p2, tol, t_hit, ipt, pcoords, subId))
          {
          if (t_hit<closest_intersection)
            {
            HIT = true;
            closest_intersection = t_hit;
            cellId = cell_ID;
            x[0] = ipt[0];
            x[1] = ipt[1];
            x[2] = ipt[2];
            }
          }
        }
      }
//    }
    }
  if (HIT)
    {
    t = closest_intersection;
    }
  //
  return HIT;
}
//---------------------------------------------------------------------------
typedef std::pair<double, int> Intersection;
//
struct Isort : public std::binary_function<Intersection, Intersection, bool> {
  bool operator()(const Intersection &x, const Intersection &y) {
    return x.first < y.first;
  }
};
//---------------------------------------------------------------------------
int vtkModifiedBSPTree::IntersectWithLine(
  const double p1[3], const double p2[3], const double tol,
  vtkPoints *points, vtkIdList *cellIds)
{
  //
  BSPNode  *node, *Near, *Mid, *Far;
  double    ctmin, ctmax, tmin, tmax, _tmin, _tmax, tDist, pcoords[3];
  double    ray_vec[3] = { p2[0]-p1[0], p2[1]-p1[1], p2[2]-p1[2] };
  int       subId;
  //
  this->BuildLocatorIfNeeded();
  //
  // Does ray pass through root BBox
  tmin = 0; tmax = 1;
  if (!this->mRoot->RayMinMaxT(p1, ray_vec, tmin, tmax))
    {
    return false;
    }
  // Ok, setup a stack and various params
  nodestack  ns;
  double    closest_intersection = VTK_LARGE_FLOAT;
  bool     HIT = false;
  // setup our axis optimized ray box edge stuff
  int axis = BSPNode::getDominantAxis(ray_vec);
  double (*_getMinDist)(const double origin[3], const double dir[3], const double B[6]);
  switch (axis) {
  case POS_X: _getMinDist = _getMinDistPOS_X; break;
  case NEG_X: _getMinDist = _getMinDistNEG_X; break;
  case POS_Y: _getMinDist = _getMinDistPOS_Y; break;
  case NEG_Y: _getMinDist = _getMinDistNEG_Y; break;
  case POS_Z: _getMinDist = _getMinDistPOS_Z; break;
  default:    _getMinDist = _getMinDistNEG_Z; break;
  }

  //
  // we will sort intersections by t, so keep track using these lists
  //
  std::vector<Intersection> t_list;
  vtkSmartPointer<vtkPoints> tempPoints;
  vtkSmartPointer<vtkIdList>    tempIds;
  if (points)
    {
    tempPoints = vtkSmartPointer<vtkPoints>::New();
    }
  if (cellIds)
    {
    tempIds = vtkSmartPointer<vtkIdList>::New();
    }
  int icount = 0;
  //
  // OK, lets walk the tree and find intersections
  //
  ns.push(this->mRoot);
  while (!ns.empty())
    {
    node = ns.top();
    ns.pop();
    // We do as few tests on the way down as possible, because our BBoxes
    // can be quite tight and we want to reject as many boxes as possible without
    // testing them at all - mainly because we quickly get to a leaf node and
    // test candidates, once we've found a hit, we note the intersection t val,
    // as soon as we pull a BBox of the stack that has a closest point further
    // than the t val, we know we can stop.
    //
    while (node->mChild[0])
      { // this must be a parent node
      // Which child node is closest to ray origin - given direction
      node->Classify(p1, ray_vec, tDist, Near, Mid, Far);
      // if the distance to the far edge of the near box is > tmax, no need to test far box
      // (we still need to test Mid because it may overlap slightly)
      if ((tDist > tmax) || (tDist <= 0) )
        { // <=0 for ray on edge
        if (Mid)
          {
          ns.push(Mid);
          }
        node = Near;
        }
      // if the distance to the far edge of the near box is < tmin, no need to test near box
      else if (tDist < tmin)
        {
        if (Mid)
          {
          ns.push(Far);
          node = Mid;
          }
        else
          {
          node = Far;
          }
        }
      // All the child nodes may be candidates, keep near, push far then mid
      else
        {
        ns.push(Far);
        if (Mid)
          {
          ns.push(Mid);
          }
        node = Near;
        }
      }
    double t_hit, ipt[3];
    // Ok, so we're a leaf node, first check the BBox against the ray
    // then test the candidates in our sorted ray direction order
    _tmin = tmin; _tmax = tmax;
//    if (node->RayMinMaxT(p1, ray_vec, _tmin, _tmax)) {
    // Was the closest point on the box > intersection point
//      if (_tmax>closest_intersection) break;
    //
    for (int i=0; i<node->num_cells; i++)
      {
      vtkIdType cell_ID = node->sorted_cell_lists[axis][i];
      //
      if (_getMinDist(p1, ray_vec, CellBounds[cell_ID]) > closest_intersection)
        {
        break;
        }
      //
      ctmin = _tmin; ctmax = _tmax;
      if (BSPNode::RayMinMaxT(CellBounds[cell_ID], p1, ray_vec, ctmin, ctmax))
        {
        if (this->IntersectCellInternal(cell_ID, p1, p2, tol, t_hit, ipt, pcoords, subId))
          {
          if (points)
            {
            tempPoints->InsertNextPoint(ipt);
            }
          if (cellIds)
            {
            tempIds->InsertNextId(cell_ID);
            }
          t_list.push_back(Intersection(t_hit, icount++));
          HIT = true;
          }
        }
      }
//    }
    }
  if (HIT)
    {
    std::sort(t_list.begin(), t_list.end(), Isort());
    int N = static_cast<int>(t_list.size());
    if (points)
      {
      points->SetNumberOfPoints(N);
      }
    if (cellIds)
      {
      cellIds->SetNumberOfIds(N);
      }
    for (int n=0; n<N; n++)
      {
      Intersection &i = t_list[n];
      if (points)
        {
        points->SetPoint(n, tempPoints->GetPoint(i.second));
        }
      if (cellIds)
        {
        cellIds->SetId(n, tempIds->GetId(i.second));
        }
      }
    }
  //
  return HIT;
}
//---------------------------------------------------------------------------
int vtkModifiedBSPTree::IntersectCellInternal(
  vtkIdType cell_ID,
  const double p1[3],
  const double p2[3],
  const double tol,
  double &t,
  double ipt[3],
  double pcoords[3],
  int &subId)
{
  this->DataSet->GetCell(cell_ID, this->GenericCell);
  return this->GenericCell->IntersectWithLine(const_cast<double*>(p1), const_cast<double*>(p2), tol, t, ipt, pcoords, subId);
}
//////////////////////////////////////////////////////////////////////////////
// FindCell stuff
//////////////////////////////////////////////////////////////////////////////
//---------------------------------------------------------------------------
bool vtkModifiedBSPTree_Inside(double bounds[6], double point[3]);
//---------------------------------------------------------------------------
vtkIdType vtkModifiedBSPTree::FindCell(
  double x[3], double , vtkGenericCell *cell,
  double pcoords[3], double *weights)
{
  //
  this->BuildLocatorIfNeeded();
  //
  nodestack ns;
  BSPNode   *node;
  ns.push(this->mRoot);
  double closestPoint[3], dist2;
  int subId;
  //
  while (!ns.empty())
    {
    node = ns.top();
    ns.pop();
    if (node->mChild[0])
      { // this must be a parent node
      if (node->mChild[0]->Inside(x)) ns.push(node->mChild[0]);
      if (node->mChild[1] && node->mChild[1]->Inside(x)) ns.push(node->mChild[1]);
      if (node->mChild[2]->Inside(x)) ns.push(node->mChild[2]);
      }
    else
      { // a leaf, so test the cells
      for (int i=0; i<node->num_cells; i++)
        {
        int cell_ID = node->sorted_cell_lists[0][i];
        //
        if (vtkModifiedBSPTree_Inside(CellBounds[cell_ID], x))
          {
          this->DataSet->GetCell(cell_ID, cell);
          if (cell->EvaluatePosition(x, closestPoint, subId, pcoords, dist2, weights)==1)
            {
            return cell_ID;
            }
//          if (dist2<tol2) return cell_ID;
          }
        }
      }
    }
  return -1;
}
//---------------------------------------------------------------------------
bool vtkModifiedBSPTree::InsideCellBounds(double x[3], vtkIdType cell_ID)
{
  //
  this->BuildLocatorIfNeeded();
  //
  return vtkModifiedBSPTree_Inside(this->CellBounds[cell_ID], x);
}
//----------------------------------------------------------------------------
vtkIdListCollection *vtkModifiedBSPTree::GetLeafNodeCellInformation()
{
  if (!this->mRoot)
    {
    return NULL;
    }
  this->BuildLocatorIfNeeded();
  //
  vtkIdListCollection *LeafCellsList = vtkIdListCollection::New();
  nodestack ns;
  BSPNode   *node = NULL;
  ns.push(this->mRoot);
  //
  while (!ns.empty())
    {
    node = ns.top();
    ns.pop();
    if (node->mChild[0])
      { // this must be a parent node
      if (node->mChild[0])
        {
        ns.push(node->mChild[0]);
        }
      if (node->mChild[1])
        {
        ns.push(node->mChild[1]);
        }
      if (node->mChild[2])
        {
        ns.push(node->mChild[2]);
        }
      }
    else
      { // a leaf
      vtkSmartPointer<vtkIdList> newList = vtkSmartPointer<vtkIdList>::New();
      LeafCellsList->AddItem(newList);
      newList->SetNumberOfIds(node->num_cells);
      for (int i=0; i<node->num_cells; i++)
        {
        newList->SetId(i, node->sorted_cell_lists[0][i]);
        }
      }
    }
  return LeafCellsList;
}
//----------------------------------------------------------------------------
void vtkModifiedBSPTree::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//////////////////////////////////////////////////////////////////////////////
// BSPNode routines
//////////////////////////////////////////////////////////////////////////////
void BSPNode::Classify(const double origin[3],
                       const double dir[3],
                       double &rDist,
                       BSPNode *&Near,
                       BSPNode *&Mid,
                       BSPNode *&Far) const
{
  double tOriginToDivPlane = mChild[0]->bounds[mAxis*2+1] - origin[mAxis];
  double tDivDirection   = dir[mAxis];
  if ( tOriginToDivPlane > 0 )
    {
    Near = mChild[0];
    Mid  = mChild[1];
    Far  = mChild[2];
    }
  else if (tOriginToDivPlane < 0)
    {
    Far  = mChild[0];
    Mid  = mChild[1];
    Near = mChild[2];
    }
  // Ray was exactly on edge of box, check direction
  else
    {
    if ( tDivDirection < 0)
      {
      Near = mChild[0];
      Mid  = mChild[1];
      Far  = mChild[2];
      }
    else
      {
      Far  = mChild[0];
      Mid  = mChild[1];
      Near = mChild[2];
      }
    }
  rDist = (tDivDirection) ? tOriginToDivPlane / tDivDirection : VTK_LARGE_FLOAT;
}
//---------------------------------------------------------------------------
// Update the two t values for the ray against the box, return false if misses
bool BSPNode::RayMinMaxT(const double origin[3],
                         const double dir[3],
                         double &rTmin,
                         double &rTmax) const
{
  double tT;
  // X-Axis
  if (dir[0] < -Epsilon_)
    {                // ray travelling in -x direction
    tT = (bounds[0] - origin[0]) / dir[0];
    if (tT < rTmin)
      {
      return (false);        // ray already left of box. Can't hit
      }
    if (tT <= rTmax)
      {
      rTmax = tT;           // update new tmax
      }
    tT = (bounds[1] - origin[0]) / dir[0]; // distance to right edge
    if (tT >= rTmin)
      {                     // can't see this ever happening
      if (tT > rTmax)
        {
        return false;        // clip start of ray to right edge
        }
      rTmin = tT;
      }
    }
  else if (dir[0] > Epsilon_)
    {
    tT = (bounds[1] - origin[0]) / dir[0];
    if (tT < rTmin)
      {
      return (false);
      }
    if (tT <= rTmax)
      {
      rTmax = tT;
      }
    tT = (bounds[0] - origin[0]) / dir[0];
    if (tT >= rTmin)
      {
      if (tT > rTmax)
        {
        return (false);
        }
      rTmin = tT;
      }
    }
  else if (origin[0] < bounds[0] || origin[0] > bounds[1])
    {
    return (false);
    }
  // Y-Axis
  if (dir[1] < -Epsilon_)
    {
    tT = (bounds[2] - origin[1]) / dir[1];
    if (tT < rTmin)
      {
      return (false);
      }
    if (tT <= rTmax)
      {
      rTmax = tT;
      }
    tT = (bounds[3] - origin[1]) / dir[1];
    if (tT >= rTmin)
      {
      if (tT > rTmax)
        {
        return (false);
        }
      rTmin = tT;
      }
    }
  else if (dir[1] > Epsilon_)
    {
    tT = (bounds[3] - origin[1]) / dir[1];
    if (tT < rTmin)
      {
      return (false);
      }
    if (tT <= rTmax)
      {
      rTmax = tT;
      }
    tT = (bounds[2] - origin[1]) / dir[1];
    if (tT >= rTmin)
      {
      if (tT > rTmax)
        {
        return (false);
        }
      rTmin = tT;
      }
    }
  else if (origin[1] < bounds[2] || origin[1] > bounds[3])
    {
    return (false);
    }
  // Z-Axis
  if (dir[2] < -Epsilon_)
    {
    tT = (bounds[4] - origin[2]) / dir[2];
    if (tT < rTmin)
      {
      return (false);
      }
    if (tT <= rTmax)
      {
      rTmax = tT;
      }
    tT = (bounds[5] - origin[2]) / dir[2];
    if (tT >= rTmin)
      {
      if (tT > rTmax)
        {
        return (false);
        }
      rTmin = tT;
      }
    }
  else if (dir[2] > Epsilon_)
    {
    tT = (bounds[5] - origin[2]) / dir[2];
    if (tT < rTmin)
      {
      return (false);
      }
    if (tT <= rTmax)
      {
      rTmax = tT;
      }
    tT = (bounds[4] - origin[2]) / dir[2];
    if (tT >= rTmin)
      {
      if (tT > rTmax)
        {
        return (false);
        }
      rTmin = tT;
      }
    }
  else if (origin[2] < bounds[4] || origin[2] > bounds[5])
    {
    return (false);
    }
  return (true);
}
//---------------------------------------------------------------------------
// Update the two t values for the ray against the box, return false if misses
bool BSPNode::RayMinMaxT(const double bounds[6],
                         const double origin[3],
                         const double dir[3],
                         double &rTmin,
                         double &rTmax)
{
  double tT;
  // X-Axis
  if (dir[0] < -Epsilon_)
    {                // ray travelling in -x direction
    tT = (bounds[0] - origin[0]) / dir[0]; // Ipoint less than minT - ray outside box!
    if (tT < rTmin)
      {
      return (false);
      }
    if (tT <= rTmax)
      {
      rTmax = tT;           // update new tmax
      }
    tT = (bounds[1] - origin[0]) / dir[0]; // distance to right edge
    if (tT >= rTmin)
      {                     // can't see this ever happening
      if (tT > rTmax)
        {
        return false;        // clip start of ray to right edge
        }
      rTmin = tT;
      }
    }
  else if (dir[0] > Epsilon_)
    {
    tT = (bounds[1] - origin[0]) / dir[0];
    if (tT < rTmin)
      {
      return (false);
      }
    if (tT <= rTmax)
      {
      rTmax = tT;
      }
    tT = (bounds[0] - origin[0]) / dir[0];
    if (tT >= rTmin)
      {
      if (tT > rTmax)
        {
        return (false);
        }
      rTmin = tT;
      }
    }
  else if (origin[0] < bounds[0] || origin[0] > bounds[1])
    {
    return (false);
    }
  // Y-Axis
  if (dir[1] < -Epsilon_)
    {
    tT = (bounds[2] - origin[1]) / dir[1];
    if (tT < rTmin)
      {
      return (false);
      }
    if (tT <= rTmax) rTmax = tT;
    tT = (bounds[3] - origin[1]) / dir[1];
    if (tT >= rTmin)
      {
      if (tT > rTmax)
        {
        return (false);
        }
      rTmin = tT;
      }
    }
  else if (dir[1] > Epsilon_)
    {
    tT = (bounds[3] - origin[1]) / dir[1];
    if (tT < rTmin)
      {
      return (false);
      }
    if (tT <= rTmax)
      {
      rTmax = tT;
      }
    tT = (bounds[2] - origin[1]) / dir[1];
    if (tT >= rTmin)
      {
      if (tT > rTmax)
        {
        return (false);
        }
      rTmin = tT;
      }
    }
  else if (origin[1] < bounds[2] || origin[1] > bounds[3])
    {
    return (false);
    }
  // Z-Axis
  if (dir[2] < -Epsilon_)
    {
    tT = (bounds[4] - origin[2]) / dir[2];
    if (tT < rTmin)
      {
      return (false);
      }
    if (tT <= rTmax)
      {
      rTmax = tT;
      }
    tT = (bounds[5] - origin[2]) / dir[2];
    if (tT >= rTmin)
      {
      if (tT > rTmax)
        {
        return (false);
        }
      rTmin = tT;
      }
    }
  else if (dir[2] > Epsilon_)
    {
    tT = (bounds[5] - origin[2]) / dir[2];
    if (tT < rTmin)
      {
      return (false);
      }
    if (tT <= rTmax)
      {
      rTmax = tT;
      }
    tT = (bounds[4] - origin[2]) / dir[2];
    if (tT >= rTmin)
      {
      if (tT > rTmax)
        {
        return (false);
        }
      rTmin = tT;
      }
    }
  else if (origin[2] < bounds[4] || origin[2] > bounds[5])
    {
    return (false);
    }
  return (true);
}
//---------------------------------------------------------------------------
bool BSPNode::Inside(double point[3]) const
{
  if (point[0]<this->bounds[0] || point[0]>this->bounds[1] ||
      point[1]<this->bounds[2] || point[1]>this->bounds[3] ||
      point[2]<this->bounds[4] || point[2]>this->bounds[5])
    {
    return 0;
    }
  return 1;
}
//---------------------------------------------------------------------------
bool vtkModifiedBSPTree_Inside(double bounds[6], double point[3])
{
  if (point[0]<bounds[0] || point[0]>bounds[1] ||
      point[1]<bounds[2] || point[1]>bounds[3] ||
      point[2]<bounds[4] || point[2]>bounds[5])
    {
    return 0;
    }
  return 1;
}
