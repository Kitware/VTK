/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBSPIntersections.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

// .NAME vtkBSPIntersections - Perform calculations (mostly intersection
//   calculations) on regions of a 3D binary spatial partitioning.
//
// .SECTION Description
//    Given an axis aligned binary spatial partitioning described by a
//    vtkBSPCuts object, perform intersection queries on various
//    geometric entities with regions of the spatial partitioning.
//
// .SECTION See Also
//    vtkBSPCuts  vtkKdTree

#ifndef __vtkBSPIntersections_h
#define __vtkBSPIntersections_h

#include "vtkObject.h"

class vtkTimeStamp;
class vtkCell;
class vtkKdNode;
class vtkBSPCuts;

class VTK_FILTERING_EXPORT vtkBSPIntersections : public vtkObject
{
public:
  vtkTypeMacro(vtkBSPIntersections, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  static vtkBSPIntersections *New();

  // Description:
  //   Define the binary spatial partitioning.

  void SetCuts(vtkBSPCuts *cuts);
  vtkGetObjectMacro(Cuts, vtkBSPCuts);

  // Description:
  //   Get the bounds of the whole space (xmin, xmax, ymin, ymax, zmin, zmax)
  //   Return 0 if OK, 1 on error.

  int GetBounds(double *bounds);

  // Description:
  //   The number of regions in the binary spatial partitioning

  int GetNumberOfRegions();

  // Description:
  //   Get the spatial bounds of a particular region
  //   Return 0 if OK, 1 on error.

  int GetRegionBounds(int regionID, double bounds[6]);

  // Description:
  //    Get the bounds of the data within the k-d tree region, possibly
  //    smaller than the bounds of the region.
  //   Return 0 if OK, 1 on error.

  int GetRegionDataBounds(int regionID, double bounds[6]);

  // Description:
  //    Determine whether a region of the spatial decomposition 
  //    intersects an axis aligned box.
  int IntersectsBox(int regionId, double *x); 
  int IntersectsBox(int regionId, double xmin, double xmax, 
                    double ymin, double ymax, 
                    double zmin, double zmax); 

  // Description:
  //    Compute a list of the Ids of all regions that 
  //    intersect the specified axis aligned box.
  //    Returns: the number of ids in the list.
  int IntersectsBox(int *ids, int len,  double *x); 
  int IntersectsBox(int *ids, int len,  double x0, double x1, 
                    double y0, double y1, double z0, double z1); 

  // Description:
  //    Determine whether a region of the spatial decomposition 
  //    intersects a sphere, given the center of the sphere 
  //    and the square of it's radius.
  int IntersectsSphere2(int regionId, 
                        double x, double y, double z, double rSquared);

  // Description:
  //    Compute a list of the Ids of all regions that 
  //    intersect the specified sphere.  The sphere is given
  //    by it's center and the square of it's radius.
  //    Returns: the number of ids in the list.
  int IntersectsSphere2(int *ids, int len, 
                        double x, double y, double z, double rSquared);

  // Description:
  //    Determine whether a region of the spatial decomposition
  //    intersects the given cell.  If you already
  //    know the region that the cell centroid lies in, provide 
  //    that as the last argument to make the computation quicker.
  int IntersectsCell(int regionId, vtkCell *cell, int cellRegion=-1);

  // Description:
  //    Compute a list of the Ids of all regions that
  //    intersect the given cell.  If you alrady
  //    know the region that the cell centroid lies in, provide
  //    that as the last argument to make the computation quicker.
  //    Returns the number of regions the cell intersects.
  int IntersectsCell(int *ids, int len, vtkCell *cell, int cellRegion=-1);

  // Description:
  //   When computing the intersection of k-d tree regions with other
  //   objects, we use the spatial bounds of the region.  To use the
  //   tighter bound of the bounding box of the data within the region,
  //   set this variable ON.  (Specifying data bounds in the vtkBSPCuts
  //   object is optional.  If data bounds were not specified, this
  //   option has no meaning.)

  vtkGetMacro(ComputeIntersectionsUsingDataBounds, int);
  void SetComputeIntersectionsUsingDataBounds(int c);
  void ComputeIntersectionsUsingDataBoundsOn();
  void ComputeIntersectionsUsingDataBoundsOff();

protected:

  vtkBSPIntersections();
  ~vtkBSPIntersections();

  vtkGetMacro(RegionListBuildTime, unsigned long);

  int BuildRegionList();

  vtkKdNode **GetRegionList(){return this->RegionList;}

  double CellBoundsCache[6];   // to speed cell intersection queries

//BTX
  enum {
    XDIM = 0,  // don't change these values
    YDIM = 1,
    ZDIM = 2
  };
//ETX

private:

  static int NumberOfLeafNodes(vtkKdNode *kd);
  static void SetIDRanges(vtkKdNode *kd, int &min, int &max);

  int SelfRegister(vtkKdNode *kd);

  static void SetCellBounds(vtkCell *cell, double *bounds);

  int _IntersectsBox(vtkKdNode *node, int *ids, int len,
                     double x0, double x1,
                     double y0, double y1, 
                     double z0, double z1);

  int _IntersectsSphere2(vtkKdNode *node, int *ids, int len,
                         double x, double y, double z, double rSquared);

  int _IntersectsCell(vtkKdNode *node, int *ids, int len,
                      vtkCell *cell, int cellRegion=-1);

  vtkBSPCuts *Cuts;

  int NumberOfRegions;
  vtkKdNode **RegionList;

  vtkTimeStamp RegionListBuildTime;

  int ComputeIntersectionsUsingDataBounds;

  vtkBSPIntersections(const vtkBSPIntersections&); // Not implemented
  void operator=(const vtkBSPIntersections&); // Not implemented
};
#endif
