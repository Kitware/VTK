/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkModifiedBSPTree.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*=========================================================================
  This code is derived from an earlier work and is distributed
  with permission from, and thanks to

  ------------------------------------------
  Copyright (C) 1997-2000 John Biddiscombe
  Rutherford Appleton Laboratory,
  Chilton, Oxon, England
  ------------------------------------------
  Copyright (C) 2000-2004 John Biddiscombe
  Skipping Mouse Software Ltd,
  Blewbury, England
  ------------------------------------------
  Copyright (C) 2004-2009 John Biddiscombe
  CSCS - Swiss National Supercomputing Centre
  Galleria 2 - Via Cantonale
  CH-6928 Manno, Switzerland
  ------------------------------------
=========================================================================*/
// .NAME vtkModifiedBSPTree - Generate axis aligned BBox tree for raycasting and other Locator based searches
//
// .SECTION Description
// vtkModifiedBSPTree creates an evenly balanced BSP tree using a top down
// implementation. Axis aligned split planes are found which evenly divide
// cells into two buckets. Generally a split plane will intersect some cells
// and these are usually stored in both child nodes of the current parent.
// (Or split into separate cells which we cannot consider in this case).
// Storing cells in multiple buckets creates problems associated with multiple
// tests against rays and increases the required storage as complex meshes
// will have many cells straddling a split plane (and further splits may
// cause multiple copies of these).
//
// During a discussion with Arno Formella in 1998 he suggested using
// a third child node to store objects which straddle split planes. I've not
// seen this published (Yes! - see below), but thought it worth trying. This
// implementation of the BSP tree creates a third child node for storing cells
// lying across split planes, the third cell may overlap the other two, but the
// two 'proper' nodes otherwise conform to usual BSP rules.
//
// The advantage of this implementation is cells only ever lie in one node
// and mailbox testing is avoided. All BBoxes are axis aligned and a ray cast
// uses an efficient search strategy based on near/far nodes and rejects
// all BBoxes using simple tests.
//
// For fast raytracing, 6 copies of cell lists are stored in each leaf node
// each list is in axis sorted order +/- x,y,z and cells are always tested
// in the direction of the ray dominant axis. Once an intersection is found
// any cell or BBox with a closest point further than the I-point can be
// instantly rejected and raytracing stops as soon as no nodes can be closer
// than the current best intersection point.
//
// The addition of the 'middle' node upsets the optimal balance of the tree,
// but is a minor overhead during the raytrace. Each child node is contracted
// such that it tightly fits all cells inside it, enabling further ray/box
// rejections.
//
// This class is intented for persons requiring many ray tests and is optimized
// for this purpose. As no cell ever lies in more than one leaf node, and parent
// nodes do not maintain cell lists, the memory overhead of the sorted cell
// lists is 6*num_cells*4 for 6 lists of ints, each num_cells in length.
// The memory requirement of the nodes themselves is usually of minor
// significance.
//
// Subdividision is controlled by MaxCellsPerNode - any node with more than
// this number will be subdivided providing a good split plane can be found and
// the max depth is not exceeded.
//
// The average cells per leaf will usually be around half the MaxCellsPerNode,
// though the middle node is usually sparsely populated and lowers the average
// slightly. The middle node will not be created when not needed.
// Subdividing down to very small cells per node is not generally suggested
// as then the 6 stored cell lists are effectively redundant.
//
// Values of MaxcellsPerNode of around 16->128 depending on dataset size will
// usually give good results.
//
// Cells are only sorted into 6 lists once - before tree creation, each node
// segments the lists and passes them down to the new child nodes whilst
// maintaining sorted order. This makes for an efficient subdivision strategy.
//
// NB. The following reference has been sent to me
//   @Article{formella-1995-ray,
//     author =     "Arno Formella and Christian Gill",
//     title =      "{Ray Tracing: A Quantitative Analysis and a New
//                   Practical Algorithm}",
//     journal =    "{The Visual Computer}",
//     year =       "{1995}",
//     month =       dec,
//     pages =      "{465--476}",
//     volume =     "{11}",
//     number =     "{9}",
//     publisher =  "{Springer}",
//     keywords =   "{ray tracing, space subdivision, plane traversal,
//                    octree, clustering, benchmark scenes}",
//     annote =     "{We present a new method to accelerate the process of
//                    finding nearest ray--object intersections in ray
//                    tracing. The algorithm consumes an amount of memory
//                    more or less linear in the number of objects. The basic
//                    ideas can be characterized with a modified BSP--tree
//                    and plane traversal. Plane traversal is a fast linear
//                    time algorithm to find the closest intersection point
//                    in a list of bounding volumes hit by a ray. We use
//                    plane traversal at every node of the high outdegree
//                    BSP--tree. Our implementation is competitive to fast
//                    ray tracing programs. We present a benchmark suite
//                    which allows for an extensive comparison of ray tracing
//                    algorithms.}",
//   }
//
// .SECTION Thanks
//  John Biddiscombe for developing and contributing this class
//
// .SECTION ToDo
// -------------
// Implement intersection heap for testing rays against transparent objects
//
// .SECTION Style
// --------------
// This class is currently maintained by J. Biddiscombe who has specially
// requested that the code style not be modified to the kitware standard.
// Please respect the contribution of this class by keeping the style
// as close as possible to the author's original.
//

#ifndef _vtkModifiedBSPTree_h
#define _vtkModifiedBSPTree_h

#include "vtkAbstractCellLocator.h"
#include "vtkSmartPointer.h"     // required because it is nice

//BTX
class Sorted_cell_extents_Lists;
class BSPNode;
class vtkGenericCell;
class vtkIdList;
class vtkIdListCollection;
//ETX

class VTK_FILTERING_EXPORT vtkModifiedBSPTree : public vtkAbstractCellLocator {
  public:
  // Description:
  // Standard Type-Macro
  vtkTypeMacro(vtkModifiedBSPTree,vtkAbstractCellLocator);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct with maximum 32 cells per node. (average 16->31)
  static vtkModifiedBSPTree *New();

//BTX
  using vtkAbstractCellLocator::IntersectWithLine;
  using vtkAbstractCellLocator::FindClosestPoint;
  using vtkAbstractCellLocator::FindClosestPointWithinRadius;
//ETX

  // Description:
  // Free tree memory
  void FreeSearchStructure();

  // Description:
  // Build Tree
  void BuildLocator();

//BTX
  // Description:
  // Generate BBox representation of Nth level
  virtual void GenerateRepresentation(int level, vtkPolyData *pd);

  // Description:
  // Generate BBox representation of all leaf nodes
  virtual void GenerateRepresentationLeafs(vtkPolyData *pd);

  // Description:
  // Return intersection point (if any) of finite line with cells contained
  // in cell locator.
  virtual int IntersectWithLine(
    double p1[3], double p2[3], double tol, double& t, double x[3],
    double pcoords[3], int &subId)
    { return this->Superclass::IntersectWithLine(p1, p2, tol, t, x, pcoords, subId); }

  // Description:
  // Return intersection point (if any) AND the cell which was intersected by
  // the finite line. Uses fast tree-search BBox rejection tests.
  virtual int IntersectWithLine(
    double p1[3], double p2[3], double tol, double &t, double x[3],
    double pcoords[3], int &subId, vtkIdType &cellId);

  // Description:
  // Return intersection point (if any) AND the cell which was intersected by
  // the finite line. The cell is returned as a cell id and as a generic cell.
  virtual int IntersectWithLine(
    double p1[3], double p2[3], double tol, double &t, double x[3], 
    double pcoords[3], int &subId, vtkIdType &cellId, vtkGenericCell *cell);

  // Description:
  // Take the passed line segment and intersect it with the data set.
  // This method assumes that the data set is a vtkPolyData that describes
  // a closed surface, and the intersection points that are returned in
  // 'points' alternate between entrance points and exit points.
  // The return value of the function is 0 if no intersections were found,
  // -1 if point 'a0' lies inside the closed surface, or +1 if point 'a0'
  // lies outside the closed surface.
  // Either 'points' or 'cellIds' can be set to NULL if you don't want
  // to receive that information. This method is currently only implemented
  // in vtkOBBTree
  virtual int IntersectWithLine(
    const double p1[3], const double p2[3],
    vtkPoints *points, vtkIdList *cellIds)
    { return this->Superclass::IntersectWithLine(p1, p2, points, cellIds); }  
  
  // Description:
  // Take the passed line segment and intersect it with the data set.
  // The return value of the function is 0 if no intersections were found.
  // For each intersection found, the vtkPoints and CellIds objects
  // have the relevant information added in order of intersection increasing
  // from ray start to end. If either vtkPoints or CellIds are NULL 
  // pointers, then no information is generated for that list.
  virtual int IntersectWithLine(
    const double p1[3], const double p2[3], const double tol,
    vtkPoints *points, vtkIdList *cellIds);

  // Description:
  // Returns the Id of the cell containing the point, 
  // returns -1 if no cell found. This interface uses a tolerance of zero
  virtual vtkIdType FindCell(double x[3])
    { return this->Superclass::FindCell(x); }

  // Description:
  // Test a point to find if it is inside a cell. Returns the cellId if inside
  // or -1 if not.
  virtual vtkIdType FindCell(double x[3], double tol2, vtkGenericCell *GenCell, 
    double pcoords[3], double *weights);

  bool InsideCellBounds(double x[3], vtkIdType cell_ID);

  // Description:
  // After subdivision has completed, one may wish to query the tree to find
  // which cells are in which leaf nodes. This function returns a list
  // which holds a cell Id list for each leaf node.
  vtkIdListCollection *GetLeafNodeCellInformation();

//ETX
  protected:
   vtkModifiedBSPTree();
  ~vtkModifiedBSPTree();
  //
  BSPNode  *mRoot;               // bounding box root node
  int       npn;
  int       nln;
  int       tot_depth;
//BTX
  //
  // The main subdivision routine
  void Subdivide(BSPNode *node, Sorted_cell_extents_Lists *lists, vtkDataSet *dataSet,
    vtkIdType nCells, int depth, int maxlevel, vtkIdType maxCells, int &MaxDepth);

  // We provide a function which does the cell/ray test so that
  // it can be overriden by subclasses to perform special treatment
  // (Example : Particles stored in tree, have no dimension, so we must
  // override the cell test to return a value based on some particle size
  virtual int IntersectCellInternal(vtkIdType cell_ID, const double p1[3], const double p2[3], 
    const double tol, double &t, double ipt[3], double pcoords[3], int &subId);

//ETX
  void BuildLocatorIfNeeded();
  void ForceBuildLocator();
  void BuildLocatorInternal();
private:
  vtkModifiedBSPTree(const vtkModifiedBSPTree&);  // Not implemented.
  void operator=(const vtkModifiedBSPTree&);      // Not implemented.
};

//BTX

///////////////////////////////////////////////////////////////////////////////
// BSP Node
// A BSP Node is a BBox - axis aligned etc etc
///////////////////////////////////////////////////////////////////////////////
#ifndef DOXYGEN_SHOULD_SKIP_THIS

class BSPNode {
  public:
    // Constructor
    BSPNode(void) {
      mChild[0] = mChild[1] = mChild[2] = NULL;
      for (int i=0; i<6; i++) sorted_cell_lists[i] = NULL;
      for (int i=0; i<3; i++) { bounds[i*2] = VTK_LARGE_FLOAT; bounds[i*2+1] = -VTK_LARGE_FLOAT; }
    }
    // Destructor
    ~BSPNode(void) {
      for (int i=0; i<3; i++) if (mChild[i]) delete mChild[i];
      for (int i=0; i<6; i++) if (sorted_cell_lists[i]) delete []sorted_cell_lists[i];
    }
    // Set min box limits
    void setMin(double minx, double miny, double minz) {
      bounds[0] = minx; bounds[2] = miny; bounds[4] = minz;
    }
    // Set max box limits
    void setMax(double maxx, double maxy, double maxz) {
      bounds[1] = maxx; bounds[3] = maxy; bounds[5] = maxz;
    }
    //
    bool Inside(double point[3]) const;
    // BBox
    double       bounds[6];
  protected:
    // The child nodes of this one (if present - NULL otherwise)
    BSPNode   *mChild[3];
    // The axis we subdivide this voxel along
    int        mAxis;
    // Just for reference
    int        depth;
    // the number of cells in this node
    int        num_cells;
    // 6 lists, sorted after the 6 dominant axes
    vtkIdType *sorted_cell_lists[6];
    // Order nodes as near/mid far relative to ray
    void Classify(const double origin[3], const double dir[3], 
      double &rDist, BSPNode *&Near, BSPNode *&Mid, BSPNode *&Far) const;
    // Test ray against node BBox : clip t values to extremes
    bool RayMinMaxT(const double origin[3], const double dir[3], 
      double &rTmin, double &rTmax) const;
    //
    friend class vtkModifiedBSPTree;
    friend class vtkParticleBoxTree;
  public:
  static bool VTK_FILTERING_EXPORT RayMinMaxT(
    const double bounds[6], const double origin[3], const double dir[3], double &rTmin, double &rTmax);
  static int  VTK_FILTERING_EXPORT getDominantAxis(const double dir[3]);
};

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

//ETX

#endif


