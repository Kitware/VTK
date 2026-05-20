// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkModifiedBSPTree.h"

#include "vtkAppendPolyData.h"
#include "vtkBox.h"
#include "vtkCubeSource.h"
#include "vtkGenericCell.h"
#include "vtkIdListCollection.h"
#include "vtkObjectFactory.h"
#include "vtkPolyData.h"
#include "vtkSMPTools.h"

#include <algorithm>
#include <array>
#include <stack>
#include <vector>

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkModifiedBSPTree);

//------------------------------------------------------------------------------
enum
{
  POS_X,
  NEG_X,
  POS_Y,
  NEG_Y,
  POS_Z,
  NEG_Z
};

//////////////////////////////////////////////////////////////////////////////
// Main management and support for tree
//////////////////////////////////////////////////////////////////////////////
vtkModifiedBSPTree::vtkModifiedBSPTree()
  : Leaves(std::make_shared<std::vector<vtkIdType>>())
{
  this->NumberOfCellsPerNode = 32;
  this->mRoot = nullptr;
  this->UseExistingSearchStructure = 0;
  this->npn = this->nln = this->tot_depth = 0;
}

//------------------------------------------------------------------------------
vtkModifiedBSPTree::~vtkModifiedBSPTree()
{
  this->FreeSearchStructure();
  this->FreeCellBounds();
}

//------------------------------------------------------------------------------
void vtkModifiedBSPTree::FreeSearchStructure()
{
  this->mRoot.reset();
  this->Leaves = std::make_shared<std::vector<vtkIdType>>();
  this->Level = 0;
  this->npn = this->nln = this->tot_depth = 0;
}

//////////////////////////////////////////////////////////////////////////////
// Here's the stuff for spatial subdivision
//////////////////////////////////////////////////////////////////////////////
class cell_extents
{
public:
  double min, max;
  vtkIdType cell_ID;
};

typedef cell_extents* cell_extents_List;

//------------------------------------------------------------------------------
class Sorted_cell_extents_Lists
{
public:
  cell_extents_List Mins[3];
  cell_extents_List Maxs[3];
  //
  Sorted_cell_extents_Lists(vtkIdType nCells)
  {
    for (int i = 0; i < 3; i++)
    {
      Mins[i] = new cell_extents[nCells]; // max num <= nCells/2 ?
      Maxs[i] = new cell_extents[nCells];
    }
  }
  ~Sorted_cell_extents_Lists()
  {
    for (int i = 0; i < 3; i++)
    {
      delete[] (this->Mins[i]);
      delete[] (this->Maxs[i]);
    }
  }
};

//------------------------------------------------------------------------------
void vtkModifiedBSPTree::BuildLocator()
{
  // if a search structure already exists
  if (this->mRoot.get())
  {
    // don't rebuild if UseExistingSearchStructure is ON
    if (this->UseExistingSearchStructure)
    {
      return;
    }
    // don't rebuild if build time is newer than modified and dataset modified time
    if (this->BuildTime > this->MTime && this->BuildTime > this->DataSet->GetMTime())
    {
      return;
    }
  }
  this->BuildLocatorInternal();
}

//------------------------------------------------------------------------------
void vtkModifiedBSPTree::ForceBuildLocator()
{
  this->BuildLocatorInternal();
}

//------------------------------------------------------------------------------
void vtkModifiedBSPTree::BuildLocatorInternal()
{
  vtkIdType numCells;
  if (!this->DataSet || (numCells = this->DataSet->GetNumberOfCells()) < 1)
  {
    vtkDebugMacro(<< "No Cells to divide");
    return;
  }
  vtkDebugMacro(<< "Creating BSPTree for " << numCells << " cells");

  //  Make sure the appropriate data is available
  this->FreeSearchStructure();

  // create the root node
  this->mRoot = std::make_shared<BSPNode>();
  this->mRoot->mAxis = 0;
  this->mRoot->depth = 0;

  this->ComputeCellBounds();

  // sort the cells into 6 lists using structure for subdividing tests
  auto lists = std::make_shared<Sorted_cell_extents_Lists>(numCells);
  vtkSMPTools::For(0, numCells,
    [&](vtkIdType begin, vtkIdType end)
    {
      double cellBounds[6], *cellBoundsPtr;
      cellBoundsPtr = cellBounds;
      for (uint8_t i = 0; i < 3; ++i)
      {
        for (vtkIdType j = begin; j < end; ++j)
        {
          this->GetCellBounds(j, cellBoundsPtr);
          lists->Mins[i][j].min = cellBoundsPtr[i * 2];
          lists->Mins[i][j].max = cellBoundsPtr[i * 2 + 1];
          lists->Mins[i][j].cell_ID = j;
          lists->Maxs[i][j].min = cellBoundsPtr[i * 2];
          lists->Maxs[i][j].max = cellBoundsPtr[i * 2 + 1];
          lists->Maxs[i][j].cell_ID = j;
        }
      }
    });
  for (uint8_t i = 0; i < 3; i++)
  {
    // Sort
    vtkSMPTools::Sort(lists->Mins[i], lists->Mins[i] + numCells,
      [&](const cell_extents& tA, const cell_extents& tB) { return tA.min < tB.min; });
    vtkSMPTools::Sort(lists->Maxs[i], lists->Maxs[i] + numCells,
      [&](const cell_extents& tA, const cell_extents& tB) { return tA.max > tB.max; });
  }
  // call the recursive subdivision routine
  vtkDebugMacro(<< "Beginning Subdivision");

  // Pre-allocate the global leaves array. Each cell ends up in exactly one leaf
  // across all 6 lists, so total size is 6 * numCells.
  this->Leaves->clear();
  this->Leaves->reserve(6 * numCells);
  // Scratch buffer for partition decisions during Subdivide.
  std::vector<uint8_t> cellPart(numCells, 0xFF);
  this->Subdivide(this->mRoot.get(), lists.get(), this->DataSet, numCells, 0, this->MaxLevel,
    this->NumberOfCellsPerNode, this->Level, cellPart);

  // Child nodes are responsible for freeing the temporary sorted lists
  this->BuildTime.Modified();
  vtkDebugMacro(<< "BSP Tree Statistics \n"
                << "Num Parent/Leaf Nodes " << npn << "/" << nln << "\n"
                << "Average Depth " << double(tot_depth) / nln << " Original : " << numCells);
}

//------------------------------------------------------------------------------
// The main BSP subdivision routine. Splits along the longest bbox axis,
// uses a binary search for the split plane, and caches per-cell partition
// decisions to avoid recomputing them on the non-split axes.
//------------------------------------------------------------------------------
// The main BSP subdivision routine. Splits along the longest bbox axis,
// uses a binary search for the split plane, and caches per-cell partition
// decisions to avoid recomputing them on the non-split axes.
void vtkModifiedBSPTree::Subdivide(BSPNode* node, Sorted_cell_extents_Lists* lists,
  vtkDataSet* dataset, vtkIdType nCells, int depth, int maxlevel, vtkIdType maxCells, int& MaxDepth,
  std::vector<uint8_t>& cellPart)
{
  // We've got lists sorted on the axes, so we can easily get BBox
  // NOTE: this->mRoot->Bounds is set here
  node->setMin(lists->Mins[0][0].min, lists->Mins[1][0].min, lists->Mins[2][0].min);
  node->setMax(lists->Maxs[0][0].max, lists->Maxs[1][0].max, lists->Maxs[2][0].max);
  // Set axis to longest bounding box dimension for deterministic, geometry-aware splitting
  const double dx = node->Bounds[1] - node->Bounds[0];
  const double dy = node->Bounds[3] - node->Bounds[2];
  const double dz = node->Bounds[5] - node->Bounds[4];
  if (dx >= dy && dx >= dz)
  {
    node->mAxis = 0;
  }
  else if (dy >= dz)
  {
    node->mAxis = 1;
  }
  else
  {
    node->mAxis = 2;
  }
  // Update depth info
  if (node->depth > MaxDepth)
  {
    MaxDepth = depth;
  }
  //
  // Make sure child nodes are clear to start with
  node->mChild[2] = node->mChild[1] = node->mChild[0] = nullptr;
  //
  // Do we want to subdivide this node ?
  //
  double pDiv = 0.0;
  if ((nCells > maxCells) && (depth < maxlevel))
  {
    // Binary-search the split plane along the chosen (longest) axis.
    // Mins[axis][j].min is nondecreasing in j; Maxs[axis][j].max is nonincreasing in j.
    // So (Mins[j].min - Maxs[j].max) is nondecreasing — find smallest j where it's positive.
    // If no such j exists below the 3/4-cells threshold, we abort subdivision for this node.
    const int Daxis = node->mAxis;
    const vtkIdType TargetCount = (3 * nCells) / 4;
    const vtkIdType searchHi = std::min(nCells, TargetCount);
    vtkIdType lo = 0, hi = searchHi;
    while (lo < hi)
    {
      vtkIdType m = lo + (hi - lo) / 2;
      if (lists->Mins[Daxis][m].min > lists->Maxs[Daxis][m].max)
      {
        hi = m;
      }
      else
      {
        lo = m + 1;
      }
    }
    bool found = false;
    if (lo < searchHi && lists->Mins[Daxis][lo].min > lists->Maxs[Daxis][lo].max)
    {
      pDiv = lists->Mins[Daxis][lo].min - VTK_TOL;
      found = true;
    }
    // construct the 3 children
    if (found)
    {
      for (int i = 0; i < 3; i++)
      {
        node->mChild[i] = new BSPNode();
        node->mChild[i]->depth = node->depth + 1;
        node->mChild[i]->mAxis = 0;
      }
      Sorted_cell_extents_Lists* left = new Sorted_cell_extents_Lists(nCells);
      Sorted_cell_extents_Lists* mid = new Sorted_cell_extents_Lists(nCells);
      Sorted_cell_extents_Lists* right = new Sorted_cell_extents_Lists(nCells);
      // we ought to keep track of how many we are adding to each list
      vtkIdType Cmin_l[3] = { 0, 0, 0 }, Cmin_m[3] = { 0, 0, 0 }, Cmin_r[3] = { 0, 0, 0 };
      vtkIdType Cmax_l[3] = { 0, 0, 0 }, Cmax_m[3] = { 0, 0, 0 }, Cmax_r[3] = { 0, 0, 0 };
      // Partition the split-axis lists and record per-cell decision in cellPart.
      // cellPart[cell_ID] gets 0 (left), 1 (mid), or 2 (right). The two
      // non-split axes then dispatch on this cached decision instead of
      // re-fetching cell bounds and comparing against pDiv.
      for (vtkIdType i = 0; i < nCells; i++)
      {
        // process the MIN-List
        cell_extents ext = lists->Mins[Daxis][i];
        uint8_t part;
        // max is on left of middle node
        if (ext.max < pDiv)
        {
          left->Mins[Daxis][Cmin_l[Daxis]++] = ext;
          part = 0;
        }
        // min is on right of middle node
        else if (ext.min > pDiv)
        {
          right->Mins[Daxis][Cmin_r[Daxis]++] = ext;
          part = 2;
        }
        // neither - must be one of ours
        else
        {
          mid->Mins[Daxis][Cmin_m[Daxis]++] = ext;
          part = 1;
        }
        cellPart[ext.cell_ID] = part;
        //
        // process the MAX-List
        // Re-derive instead of reading cellPart, because Maxs[Daxis][i] may
        // refer to a different cell than Mins[Daxis][i] at this iteration —
        // the sorted orders are independent. cellPart for this Maxs cell will
        // be written when we visit its Mins entry (in some other iteration).
        ext = lists->Maxs[Daxis][i];
        // max is on left of middle node
        if (ext.max < pDiv)
        {
          left->Maxs[Daxis][Cmax_l[Daxis]++] = ext;
        }
        // min is on right of middle node
        else if (ext.min > pDiv)
        {
          right->Maxs[Daxis][Cmax_r[Daxis]++] = ext;
        }
        // neither - must be one of ours
        else
        {
          mid->Maxs[Daxis][Cmax_m[Daxis]++] = ext;
        }
      }
      // construct the sorted list of extents for the 2 remaining axes
      // using the cached partition decision from cellPart.
      for (int Oaxis = (node->mAxis + 1) % 3; Oaxis != node->mAxis; Oaxis = (Oaxis + 1) % 3)
      {
        for (vtkIdType i = 0; i < nCells; i++)
        {
          // process the MIN-List
          cell_extents ext = lists->Mins[Oaxis][i];
          switch (cellPart[ext.cell_ID])
          {
            case 0:
              left->Mins[Oaxis][Cmin_l[Oaxis]++] = ext;
              break;
            case 2:
              right->Mins[Oaxis][Cmin_r[Oaxis]++] = ext;
              break;
            default:
              mid->Mins[Oaxis][Cmin_m[Oaxis]++] = ext;
              break;
          }
          //
          // process the MAX-List
          ext = lists->Maxs[Oaxis][i];
          switch (cellPart[ext.cell_ID])
          {
            case 0:
              left->Maxs[Oaxis][Cmax_l[Oaxis]++] = ext;
              break;
            case 2:
              right->Maxs[Oaxis][Cmax_r[Oaxis]++] = ext;
              break;
            default:
              mid->Maxs[Oaxis][Cmax_m[Oaxis]++] = ext;
              break;
          }
        }
      }
      //
      // Better check we didn't make a diddly
      // this is overkill but for now I want a FULL DEBUG!
      if ((Cmin_l[0] + Cmin_r[0] + Cmin_m[0]) != nCells)
      {
        vtkWarningMacro("Error count in min lists");
      }
      if ((Cmin_l[1] + Cmin_r[1] + Cmin_m[1]) != nCells)
      {
        vtkWarningMacro("Error count in min lists");
      }
      if ((Cmin_l[2] + Cmin_r[2] + Cmin_m[2]) != nCells)
      {
        vtkWarningMacro("Error count in min lists");
      }
      if ((Cmax_l[0] + Cmax_r[0] + Cmax_m[0]) != nCells)
      {
        vtkWarningMacro("Error count in max lists");
      }
      if ((Cmax_l[1] + Cmax_r[1] + Cmax_m[1]) != nCells)
      {
        vtkWarningMacro("Error count in max lists");
      }
      if ((Cmax_l[2] + Cmax_r[2] + Cmax_m[2]) != nCells)
      {
        vtkWarningMacro("Error count in max lists");
      }
      //
      // Bug : Can sometimes get unbalanced leaves
      //
      if (!Cmin_l[0] || !Cmin_r[0])
      {
        // vtkDebugMacro(<<"Child 0 or 2 empty : Aborting subdivision for node " << Cmin_l[0] << " "
        // << Cmin_m[0] << " " << Cmin_r[0]); clean up all the memory we allocated. Yikes.
        for (int i = 0; i < 3; i++)
        {
          delete node->mChild[i];
          node->mChild[i] = nullptr;
        }
        delete left;
        delete mid;
        delete right;
      }
      else
      {
        // Store split plane for FindCell pruning.
        node->pDiv = pDiv;
        //
        // Now we can delete the lists that the parent passed on to us
        //
        //
        // And of course, we really ought to subdivide again - Hoorah!
        // NB: it is possible for a node to be empty now, so check and delete if necessary
        if (Cmin_l[0])
        {
          Subdivide(node->mChild[0], left, dataset, Cmin_l[0], depth + 1, maxlevel, maxCells,
            MaxDepth, cellPart);
        }
        else
        {
          vtkWarningMacro(<< "Child 0 Empty ! - this shouldn't happen");
        }
        delete left;

        if (Cmin_m[0])
        {
          Subdivide(node->mChild[1], mid, dataset, Cmin_m[0], depth + 1, maxlevel, maxCells,
            MaxDepth, cellPart);
        }
        else
        {
          delete node->mChild[1];
          node->mChild[1] = nullptr;
        }
        delete mid;

        if (Cmin_r[0])
        {
          Subdivide(node->mChild[2], right, dataset, Cmin_r[0], depth + 1, maxlevel, maxCells,
            MaxDepth, cellPart);
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
  // Append this leaf's 6 sorted cell ID lists to the global Leaves array.
  // Layout: [axis0 mins | axis0 maxs | axis1 mins | axis1 maxs | axis2 mins | axis2 maxs].
  node->LeavesStart = static_cast<vtkIdType>(this->Leaves->size());
  for (int i = 0; i < 3; i++)
  {
    for (vtkIdType j = 0; j < nCells; j++)
    {
      this->Leaves->push_back(lists->Mins[i][j].cell_ID);
    }
    for (vtkIdType j = 0; j < nCells; j++)
    {
      this->Leaves->push_back(lists->Maxs[i][j].cell_ID);
    }
  }
  // Thank buggery that's all over.
}

//////////////////////////////////////////////////////////////////////////////
// Generate representation for viewing structure
//////////////////////////////////////////////////////////////////////////////
// OK so this is a quick a dirty one for testing, but I can't be arsed
// working out which faces are visible
class box
{
public:
  double bounds[6];
  box(double* b)
  {
    for (int i = 0; i < 6; i++)
    {
      bounds[i] = b[i];
    }
  }
};

//------------------------------------------------------------------------------
typedef std::vector<box> boxlist;
typedef std::stack<BSPNode*, std::vector<BSPNode*>> nodestack;

//------------------------------------------------------------------------------
void vtkModifiedBSPTree::GenerateRepresentation(int level, vtkPolyData* pd)
{
  this->BuildLocator();
  if (this->mRoot == nullptr)
  {
    return;
  }
  nodestack ns;
  boxlist bl;
  BSPNode* node;
  ns.push(this->mRoot.get());
  // lets walk the tree and get all the level n node boxes
  while (!ns.empty())
  {
    node = ns.top();
    ns.pop();
    if (node->depth == level)
    {
      bl.emplace_back(node->Bounds);
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
      else if (level == -1)
      {
        bl.emplace_back(node->Bounds);
      }
    }
  }

  // Ok, now create cube(oid)s and stuff'em into a polydata thingy
  vtkAppendPolyData* polys = vtkAppendPolyData::New();
  size_t s = bl.size();
  for (size_t i = 0; i < s; i++)
  {
    vtkCubeSource* cube = vtkCubeSource::New();
    cube->SetBounds(bl[i].bounds);
    cube->Update();
    polys->AddInputConnection(cube->GetOutputPort());
    cube->Delete();
  }
  polys->Update();
  pd->SetPoints(polys->GetOutput()->GetPoints());
  pd->SetPolys(polys->GetOutput()->GetPolys());
  polys->Delete();
}

//------------------------------------------------------------------------------
void vtkModifiedBSPTree::GenerateRepresentationLeafs(vtkPolyData* pd)
{
  GenerateRepresentation(-1, pd);
}

//////////////////////////////////////////////////////////////////////////////
// Ray/BSPtree Intersection stuff
//////////////////////////////////////////////////////////////////////////////

// Ray->Box edge t-distance tests
static double getMinDistPOS_X(const double origin[3], const double dir[3], const double B[6])
{
  return ((B[0] - origin[0]) / dir[0]);
}
static double getMinDistNEG_X(const double origin[3], const double dir[3], const double B[6])
{
  return ((B[1] - origin[0]) / dir[0]);
}
static double getMinDistPOS_Y(const double origin[3], const double dir[3], const double B[6])
{
  return ((B[2] - origin[1]) / dir[1]);
}
static double getMinDistNEG_Y(const double origin[3], const double dir[3], const double B[6])
{
  return ((B[3] - origin[1]) / dir[1]);
}
static double getMinDistPOS_Z(const double origin[3], const double dir[3], const double B[6])
{
  return ((B[4] - origin[2]) / dir[2]);
}
static double getMinDistNEG_Z(const double origin[3], const double dir[3], const double B[6])
{
  return ((B[5] - origin[2]) / dir[2]);
}

//------------------------------------------------------------------------------
int BSPNode::getDominantAxis(const double dir[3])
{
  double tX = (dir[0] > 0) ? dir[0] : -dir[0];
  double tY = (dir[1] > 0) ? dir[1] : -dir[1];
  double tZ = (dir[2] > 0) ? dir[2] : -dir[2];
  if (tX > tY && tX > tZ)
  {
    return ((dir[0] > 0) ? POS_X : NEG_X);
  }
  else if (tY > tZ)
  {
    return ((dir[1] > 0) ? POS_Y : NEG_Y);
  }
  else
  {
    return ((dir[2] > 0) ? POS_Z : NEG_Z);
  }
}

//------------------------------------------------------------------------------
int vtkModifiedBSPTree::IntersectWithLine(const double p1[3], const double p2[3], double tol,
  double& t, double x[3], double pcoords[3], int& subId, vtkIdType& cellId, vtkGenericCell* cell)
{
  this->BuildLocator();
  if (this->mRoot == nullptr)
  {
    return 0;
  }
  BSPNode *node, *Near, *Mid, *Far;
  double tmin, tmax, tDist, tHitCell, tBest = VTK_DOUBLE_MAX, xBest[3], pCoordsBest[3];
  double rayDir[3], x0[3], x1[3], hitCellBoundsPosition[3], cellBounds[6], *cellBoundsPtr;
  cellBoundsPtr = cellBounds;
  int plane1, plane2, subIdBest = -1;
  vtkMath::Subtract(p2, p1, rayDir);
  vtkIdType cId, cellIdBest = -1;
  double* bounds = this->mRoot->Bounds;
  cellId = -1;

  // Does ray pass through root BBox
  if (vtkBox::IntersectWithLine(bounds, p1, p2, tmin, tmax, x0, x1, plane1, plane2) == 0)
  {
    return false;
  }
  std::vector<bool> cellHasBeenVisited(this->DataSet->GetNumberOfCells(), false);
  // Ok, setup a stack and various params
  nodestack ns;
  // setup our axis optimized ray box edge stuff
  int axis = BSPNode::getDominantAxis(rayDir);
  double (*_getMinDist)(const double origin[3], const double dir[3], const double B[6]);
  switch (axis)
  {
    case POS_X:
      _getMinDist = getMinDistPOS_X;
      break;
    case NEG_X:
      _getMinDist = getMinDistNEG_X;
      break;
    case POS_Y:
      _getMinDist = getMinDistPOS_Y;
      break;
    case NEG_Y:
      _getMinDist = getMinDistNEG_Y;
      break;
    case POS_Z:
      _getMinDist = getMinDistPOS_Z;
      break;
    default:
      _getMinDist = getMinDistNEG_Z;
      break;
  }
  // OK, lets walk the tree and find intersections
  ns.push(this->mRoot.get());
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
    while (node->mChild[0]) // this must be a parent node
    {
      // Which child node is closest to ray origin - given direction
      node->Classify(p1, rayDir, tDist, Near, Mid, Far);
      // if the distance to the far edge of the near box is > tmax, no need to test far box
      // (we still need to test Mid because it may overlap slightly)
      if ((tDist > tmax) || (tDist <= 0)) // <=0 for ray on edge
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
    // Ok, so we're a leaf node, first check the BBox against the ray
    // then test the candidates in our sorted ray direction order
    const vtkIdType* leafCells = this->Leaves->data() + node->LeavesStart + axis * node->num_cells;
    for (int i = 0; i < node->num_cells; i++)
    {
      cId = leafCells[i];
      if (!cellHasBeenVisited[cId])
      {
        cellHasBeenVisited[cId] = true;
        this->GetCellBounds(cId, cellBoundsPtr);
        if (_getMinDist(p1, rayDir, cellBoundsPtr) > tBest)
        {
          break;
        }
        // check whether we intersect the cell bounds
        int hitCellBounds =
          vtkBox::IntersectBox(cellBoundsPtr, p1, rayDir, hitCellBoundsPosition, tHitCell, tol);
        if (hitCellBounds)
        {
          this->DataSet->GetCell(cId, cell);
          if (cell->IntersectWithLine(p1, p2, tol, t, x, pcoords, subId))
          {
            if (t < tBest)
            {
              tBest = t;
              xBest[0] = x[0];
              xBest[1] = x[1];
              xBest[2] = x[2];
              pCoordsBest[0] = pcoords[0];
              pCoordsBest[1] = pcoords[1];
              pCoordsBest[2] = pcoords[2];
              subIdBest = subId;
              cellIdBest = cId;
            }
          }
        }
      }
    }
  }
  // If a cell has been intersected, recover the information and return.
  if (cellIdBest >= 0)
  {
    this->DataSet->GetCell(cellIdBest, cell);
    t = tBest;
    x[0] = xBest[0];
    x[1] = xBest[1];
    x[2] = xBest[2];
    pcoords[0] = pCoordsBest[0];
    pcoords[1] = pCoordsBest[1];
    pcoords[2] = pCoordsBest[2];
    subId = subIdBest;
    cellId = cellIdBest;
    return 1;
  }
  return 0;
}

//------------------------------------------------------------------------------
struct IntersectionInfo
{
  vtkIdType CellId;
  std::array<double, 3> IntersectionPoint;
  double T;

  IntersectionInfo(vtkIdType cellId, double x[3], double t)
    : CellId(cellId)
    , IntersectionPoint({ x[0], x[1], x[2] })
    , T(t)
  {
  }
};

//------------------------------------------------------------------------------
int vtkModifiedBSPTree::IntersectWithLine(const double p1[3], const double p2[3], double tol,
  vtkPoints* points, vtkIdList* cellIds, vtkGenericCell* cell)
{
  this->BuildLocator();
  if (this->mRoot == nullptr)
  {
    return 0;
  }
  // Initialize the list of points/cells
  if (points)
  {
    points->Reset();
  }
  if (cellIds)
  {
    cellIds->Reset();
  }
  BSPNode *node, *Near, *Mid, *Far;
  double tmin, tmax, tDist, tHitCell;
  double rayDir[3], x0[3], x1[3], hitCellBoundsPosition[3], cellBounds[6], *cellBoundsPtr;
  cellBoundsPtr = cellBounds;
  vtkMath::Subtract(p2, p1, rayDir);
  double* bounds = this->mRoot->Bounds;
  vtkIdType cId;
  int plane0, plane1, subId;
  double t, x[3], pcoords[3];

  // Does ray pass through root BBox
  if (vtkBox::IntersectWithLine(bounds, p1, p2, tmin, tmax, x0, x1, plane0, plane1) == 0)
  {
    return 0; // No intersections possible, line is outside the locator
  }

  // Initialize intersection query array if necessary. This is done
  // locally to ensure thread safety.
  std::vector<bool> cellHasBeenVisited(this->DataSet->GetNumberOfCells(), false);

  // Ok, setup a stack and various params
  nodestack ns;
  // setup our axis optimized ray box edge stuff
  int axis = BSPNode::getDominantAxis(rayDir);
  // we will sort intersections by t, so keep track using these lists
  std::vector<IntersectionInfo> cellIntersections;
  // OK, lets walk the tree and find intersections
  ns.push(this->mRoot.get());
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
      node->Classify(p1, rayDir, tDist, Near, Mid, Far);
      // if the distance to the far edge of the near box is > tmax, no need to test far box
      // (we still need to test Mid because it may overlap slightly)
      if ((tDist > tmax) || (tDist <= 0))
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
    // Ok, so we're a leaf node, first check the BBox against the ray
    // then test the candidates in our sorted ray direction order
    const vtkIdType* leafCells = this->Leaves->data() + node->LeavesStart + axis * node->num_cells;
    for (int i = 0; i < node->num_cells; i++)
    {
      cId = leafCells[i];
      if (!cellHasBeenVisited[cId])
      {
        cellHasBeenVisited[cId] = true;
        this->GetCellBounds(cId, cellBoundsPtr);
        // check whether we intersect the cell bounds
        int hitCellBounds =
          vtkBox::IntersectBox(cellBoundsPtr, p1, rayDir, hitCellBoundsPosition, tHitCell, tol);

        if (hitCellBounds)
        {
          // Note because of cellHasBeenVisited[], we know this cId is unique
          if (cell)
          {
            this->DataSet->GetCell(cId, cell);
            if (cell->IntersectWithLine(p1, p2, tol, t, x, pcoords, subId))
            {
              cellIntersections.emplace_back(cId, x, t);
            }
          }
          else
          {
            cellIntersections.emplace_back(cId, hitCellBoundsPosition, tHitCell);
          }
        }
      }
    }
  }
  // if we had intersections, sort them by increasing t
  if (!cellIntersections.empty())
  {
    vtkIdType numIntersections = static_cast<vtkIdType>(cellIntersections.size());
    std::sort(cellIntersections.begin(), cellIntersections.end(),
      [&](const IntersectionInfo& a, const IntersectionInfo& b) { return a.T < b.T; });
    if (points)
    {
      points->SetNumberOfPoints(numIntersections);
      for (vtkIdType i = 0; i < numIntersections; ++i)
      {
        points->SetPoint(i, cellIntersections[i].IntersectionPoint.data());
      }
    }
    if (cellIds)
    {
      cellIds->SetNumberOfIds(numIntersections);
      for (vtkIdType i = 0; i < numIntersections; ++i)
      {
        cellIds->SetId(i, cellIntersections[i].CellId);
      }
    }
    return 1;
  }
  return 0;
}

//------------------------------------------------------------------------------
vtkIdType vtkModifiedBSPTree::FindCell(
  double x[3], double tol2, vtkGenericCell* cell, int& subId, double pcoords[3], double* weights)
{
  this->BuildLocator();
  if (this->mRoot == nullptr)
  {
    return -1;
  }
  // check if x outside of bounds
  if (!vtkAbstractCellLocator::IsInBounds(this->mRoot->Bounds, x))
  {
    return -1;
  }
  const double tol = std::sqrt(tol2);
  vtkIdType cellId;
  nodestack ns;
  BSPNode* node;
  ns.push(this->mRoot.get());
  double closestPoint[3], dist2 = 0;
  //
  while (!ns.empty())
  {
    node = ns.top();
    ns.pop();
    if (node->mChild[0])
    { // this must be a parent node
      const int ax = node->mAxis;
      const double pd = node->pDiv;
      const double xa = x[ax];
      // Left child: only visit if x is at or left of the split plane (within tol).
      if (xa <= pd + tol && node->mChild[0]->Inside(x))
      {
        ns.push(node->mChild[0]);
      }
      // Middle child: holds straddling cells; only its own bbox is meaningful.
      if (node->mChild[1] && node->mChild[1]->Inside(x))
      {
        ns.push(node->mChild[1]);
      }
      // Right child: only visit if x is at or right of the split plane.
      if (xa >= pd - tol && node->mChild[2] && node->mChild[2]->Inside(x))
      {
        ns.push(node->mChild[2]);
      }
    }
    else
    { // a leaf, so test the cells
      // Pick the longest axis of this leaf's bbox and use the corresponding
      // min-sorted cell list so we can break out as soon as the remaining
      // cells all start past x along that axis.
      const double* lb = node->Bounds;
      int la = 0;
      double lext = lb[1] - lb[0];
      if (lb[3] - lb[2] > lext)
      {
        la = 1;
        lext = lb[3] - lb[2];
      }
      if (lb[5] - lb[4] > lext)
      {
        la = 2;
      }
      // the sorted cell IDs sorted by cell min on axis la (ascending).
      const double xla = x[la];
      const vtkIdType* leafCells =
        this->Leaves->data() + node->LeavesStart + (2 * la) * node->num_cells;
      if (this->CacheCellBounds)
      {
        for (int i = 0; i < node->num_cells; i++)
        {
          cellId = leafCells[i];
          const double* cellBounds = &this->CellBounds[6 * cellId];
          if (cellBounds[2 * la] > xla + tol)
          {
            break;
          }
          // This cell's max is below xla - tol on this axis; can't contain x.
          if (cellBounds[2 * la + 1] < xla - tol)
          {
            continue;
          }
          // Full AABB test on the other two axes (this also re-checks la, cheap).
          if (vtkAbstractCellLocator::IsInBounds(cellBounds, x, tol))
          {
            this->DataSet->GetCell(cellId, cell);
            const int stat =
              cell->EvaluatePosition(x, closestPoint, subId, pcoords, dist2, weights);
            if (stat != -1 && dist2 <= tol2)
            {
              return cellId;
            }
          }
        }
      }
      else
      {
        double bounds[3];
        for (int i = 0; i < node->num_cells; i++)
        {
          cellId = leafCells[i];
          this->DataSet->GetCellBounds(cellId, bounds);
          if (bounds[2 * la] > xla + tol)
          {
            break;
          }
          // This cell's max is below xla - tol on this axis; can't contain x.
          if (bounds[2 * la + 1] < xla - tol)
          {
            continue;
          }
          // Full AABB test on the other two axes (this also re-checks la, cheap).
          if (vtkAbstractCellLocator::IsInBounds(bounds, x, tol))
          {
            this->DataSet->GetCell(cellId, cell);
            const int stat =
              cell->EvaluatePosition(x, closestPoint, subId, pcoords, dist2, weights);
            if (stat != -1 && dist2 <= tol2)
            {
              return cellId;
            }
          }
        }
      }
    }
  }
  return -1;
}

//------------------------------------------------------------------------------
vtkIdListCollection* vtkModifiedBSPTree::GetLeafNodeCellInformation()
{
  if (this->mRoot == nullptr)
  {
    return nullptr;
  }
  //
  vtkIdListCollection* LeafCellsList = vtkIdListCollection::New();
  nodestack ns;
  BSPNode* node = nullptr;
  ns.push(this->mRoot.get());
  //
  while (!ns.empty())
  {
    node = ns.top();
    ns.pop();
    if (node->mChild[0])
    { // this must be a parent node
      ns.push(node->mChild[0]);
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
      const vtkIdType* leafCells = this->Leaves->data() + node->LeavesStart;
      for (int i = 0; i < node->num_cells; i++)
      {
        newList->SetId(i, leafCells[i]);
      }
    }
  }
  return LeafCellsList;
}

//------------------------------------------------------------------------------
void vtkModifiedBSPTree::ShallowCopy(vtkAbstractCellLocator* locator)
{
  vtkModifiedBSPTree* cellLocator = vtkModifiedBSPTree::SafeDownCast(locator);
  if (!cellLocator)
  {
    vtkErrorMacro("Cannot cast " << locator->GetClassName() << " to vtkModifiedBSPTree.");
    return;
  }
  // we only copy what's actually used by vtkModifiedBSPTree

  // vtkLocator parameters
  this->SetUseExistingSearchStructure(cellLocator->GetUseExistingSearchStructure());
  this->SetMaxLevel(cellLocator->GetMaxLevel());
  this->Level = cellLocator->Level;

  // vtkAbstractCellLocator parameters
  this->SetNumberOfCellsPerNode(cellLocator->GetNumberOfCellsPerNode());
  this->CacheCellBounds = cellLocator->CacheCellBounds;
  this->CellBoundsSharedPtr = cellLocator->CellBoundsSharedPtr; // This is important
  this->CellBounds = this->CellBoundsSharedPtr.get() ? this->CellBoundsSharedPtr->data() : nullptr;

  // vtkCellTreeLocator parameters
  this->mRoot = cellLocator->mRoot;   // This is important
  this->Leaves = cellLocator->Leaves; // This is important
  this->npn = cellLocator->npn;
  this->nln = cellLocator->nln;
  this->tot_depth = cellLocator->tot_depth;
  this->BuildTime.Modified();
}

//------------------------------------------------------------------------------
void vtkModifiedBSPTree::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "npn: " << this->npn << "\n";
  os << indent << "nln: " << this->nln << "\n";
  os << indent << "tot_depth: " << this->tot_depth << "\n";
}

//////////////////////////////////////////////////////////////////////////////
// BSPNode routines
//////////////////////////////////////////////////////////////////////////////
void BSPNode::Classify(const double origin[3], const double dir[3], double& rDist, BSPNode*& Near,
  BSPNode*& Mid, BSPNode*& Far) const
{
  double tOriginToDivPlane = mChild[0]->Bounds[mAxis * 2 + 1] - origin[mAxis];
  double tDivDirection = dir[mAxis];
  if (tOriginToDivPlane > 0)
  {
    Near = mChild[0];
    Mid = mChild[1];
    Far = mChild[2];
  }
  else if (tOriginToDivPlane < 0)
  {
    Far = mChild[0];
    Mid = mChild[1];
    Near = mChild[2];
  }
  // Ray was exactly on edge of box, check direction
  else
  {
    if (tDivDirection < 0)
    {
      Near = mChild[0];
      Mid = mChild[1];
      Far = mChild[2];
    }
    else
    {
      Far = mChild[0];
      Mid = mChild[1];
      Near = mChild[2];
    }
  }
  rDist = (tDivDirection) ? tOriginToDivPlane / tDivDirection : VTK_FLOAT_MAX;
}

//------------------------------------------------------------------------------
bool BSPNode::Inside(double point[3]) const
{
  return this->Bounds[0] <= point[0] && point[0] <= this->Bounds[1] &&
    this->Bounds[2] <= point[1] && point[1] <= this->Bounds[3] && this->Bounds[4] <= point[2] &&
    point[2] <= this->Bounds[5];
}
VTK_ABI_NAMESPACE_END
