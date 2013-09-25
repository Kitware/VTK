/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOctreePointLocatorNode.h

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

// .NAME vtkOctreePointLocatorNode - Octree node that has 8 children each of equal size
//
// .SECTION Description
// This class represents a single spatial region in a 3D axis octant
// partitioning.  It is intended to work efficiently with the
// vtkOctreePointLocator and is not meant for general use.  It is assumed
// the region bounds some set of points.  The ordering of the children is
// (-x,-y,-z),(+x,-y,-z),(-x,+y,-z),(+x,+y,-z),(-x,-y,+z),(+x,-y,+z),
// (-x,+y,+z),(+x,+y,+z).  The portion of the domain assigned to an
// octant is Min < x <= Max.
//
// .SECTION See Also
// vtkOctreePointLocator

#ifndef __vtkOctreePointLocatorNode_h
#define __vtkOctreePointLocatorNode_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkObject.h"

class vtkCell;
class vtkPlanesIntersection;

class VTKCOMMONDATAMODEL_EXPORT vtkOctreePointLocatorNode : public vtkObject
{
public:
  vtkTypeMacro(vtkOctreePointLocatorNode, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  static vtkOctreePointLocatorNode *New();

  // Description:
  // Set/Get the number of points contained in this region.
  void SetNumberOfPoints(int numberOfPoints)
  {
    this->NumberOfPoints = numberOfPoints;
  }
  vtkGetMacro(NumberOfPoints, int);

  // Description:
  //   Set/Get the bounds of the spatial region represented by this node.
  //   Caller allocates storage for 6-vector in GetBounds.
  void SetBounds(double xMin, double xMax, double yMin,
                 double yMax, double zMin, double zMax);
  void SetBounds(const double b[6])
    {
    this->SetBounds(b[0], b[1], b[2], b[3], b[4], b[5]);
    }
  void GetBounds(double *b) const;

  // Description:
  //   Set/Get the bounds of the points contained in this spatial region.
  //   This may be smaller than the bounds of the region itself.
  //   Caller allocates storage for 6-vector in GetDataBounds.
  void SetDataBounds(double xMin, double xMax, double yMin,
                     double yMax, double zMin, double zMax);
  void GetDataBounds(double *b) const;

//BTX
  // Description:
  //   Get a pointer to the 3 bound minima (xmin, ymin and zmin) or the
  //   3 bound maxima (xmax, ymax, zmax).  Don't free this pointer.
  vtkGetMacro(MinBounds, double*);
  vtkGetMacro(MaxBounds, double*);
//ETX

  // Description:
  //   Set the xmin, ymin and zmin value of the bounds of this region
  void SetMinBounds(double minBounds[3])
  {
    this->MinBounds[0] = minBounds[0];
    this->MinBounds[1] = minBounds[1];
    this->MinBounds[2] = minBounds[2];
  }

  // Description:
  //   Set the xmax, ymax and zmax value of the bounds of this region
  void SetMaxBounds(double maxBounds[3])
  {
    this->MaxBounds[0] = maxBounds[0];
    this->MaxBounds[1] = maxBounds[1];
    this->MaxBounds[2] = maxBounds[2];
  }

//BTX
  // Description:
  //   Get a pointer to the 3 data bound minima (xmin, ymin and zmin) or the
  //   3 data bound maxima (xmax, ymax, zmax).  Don't free this pointer.
  vtkGetMacro(MinDataBounds, double*);
  vtkGetMacro(MaxDataBounds, double*);
//ETX

  // Description:
  //   Set the xmin, ymin and zmin value of the bounds of this
  //   data within this region.
  void SetMinDataBounds(double minDataBounds[3])
  {
    this->MinDataBounds[0] = minDataBounds[0];
    this->MinDataBounds[1] = minDataBounds[1];
    this->MinDataBounds[2] = minDataBounds[2];
  }

  // Description:
  //   Set the xmax, ymax and zmax value of the bounds of this
  //   data within this region.
  void SetMaxDataBounds(double maxDataBounds[3])
  {
    this->MaxDataBounds[0] = maxDataBounds[0];
    this->MaxDataBounds[1] = maxDataBounds[1];
    this->MaxDataBounds[2] = maxDataBounds[2];
  }

  // Description:
  //   Get the ID associated with the region described by this node.  If
  //   this is not a leaf node, this value should be -1.
  vtkGetMacro(ID, int);

  // Description:
  //   If this node is not a leaf node, there are leaf nodes below it whose
  //   regions represent a partitioning of this region.  The IDs of these
  //   leaf nodes form a contigous set.  Get the first of the first point's
  //   ID that is contained in this node.
  vtkGetMacro(MinID, int);

  // Description:
  //   Add the 8 children.
  void CreateChildNodes();

  // Description:
  //   Delete the 8 children.
  void DeleteChildNodes();

  // Description:
  //   Get a pointer to the ith child of this node.
  vtkOctreePointLocatorNode* GetChild(int i);

  // Description:
  //   A vtkPlanesIntersection object represents a convex 3D region bounded
  //   by planes, and it is capable of computing intersections of
  //   boxes with itself.  Return 1 if this spatial region intersects
  //   the spatial region described by the vtkPlanesIntersection object.
  //   Use the possibly smaller bounds of the points within the region
  //   if useDataBounds is non-zero.
  int IntersectsRegion(vtkPlanesIntersection *pi, int useDataBounds);

  // Description:
  //   Return 1 if this spatial region entirely contains the given point.
  //   Use the possibly smaller bounds of the points within the region
  //   if useDataBounds is non-zero.
  int ContainsPoint(double x, double y, double z, int useDataBounds);

  // Description:
  //   Calculate the distance squared from any point to the boundary of this
  //   region.  Use the boundary of the points within the region if useDataBounds
  //   is non-zero.
  double GetDistance2ToBoundary(double x, double y, double z,
                                vtkOctreePointLocatorNode* top, int useDataBounds);

  // Description:
  //   Calculate the distance squared from any point to the boundary of this
  //   region.  Use the boundary of the points within the region if useDataBounds
  //   is non-zero.  Set boundaryPt to the point on the boundary.
  double GetDistance2ToBoundary(double x, double y, double z,
                                double *boundaryPt, vtkOctreePointLocatorNode* top,
                                int useDataBounds);

  // Description:
  //   Calculate the distance from the specified point (which is required to
  //   be inside this spatial region) to an interior boundary.  An interior
  //   boundary is one that is not also an boundary of the entire space
  //   partitioned by the tree of vtkOctreePointLocatorNode's.
  double GetDistance2ToInnerBoundary(double x, double y, double z,
                                     vtkOctreePointLocatorNode* top);

  // Description:
  // Return the id of the suboctant that a given point is in.
  // If CheckContainment is non-zero then it checks whether
  // the point is in the actual bounding box of the suboctant,
  // otherwise it only checks which octant the point is in
  // that is created from the axis-aligned partitioning of
  // the domain at this octant's center.
  int GetSubOctantIndex(double* point, int CheckContainment);

  // Description:
  // Recursive function to compute ID, MinVal, MaxVal, and MinID.
  // Parent is used for MinVal and MaxVal in the case that no
  // points are in the leaf node.
  void ComputeOctreeNodeInformation(vtkOctreePointLocatorNode* Parent,
                                    int& NextLeafId, int & NextMinId,
                                    float* coordinates);

protected:
  vtkOctreePointLocatorNode();
  ~vtkOctreePointLocatorNode();

private:

  double _GetDistance2ToBoundary(
    double x, double y, double z, double *boundaryPt,
    int innerBoundaryOnly, vtkOctreePointLocatorNode* top,
    int useDataBounds);

  // Description:
  // The minimum coordinate location of the node.
  double MinBounds[3];

  // Description:
  // The maximum coordinate location of the node.
  double MaxBounds[3];

  // Description:
  // The minimum coordinate location of the points contained
  // within this node.
  double MinDataBounds[3];

  // Description:
  // The maximum coordinate location of the points contained
  // within this node.
  double MaxDataBounds[3];

  // Description:
  // Get the number of points associated with this octant.
  // The octant does not have to be a leaf octant.  For example,
  // for the root octant NumberOfPoints is equal to the number
  // of points in the dataset.
  int NumberOfPoints;

  // Description:
  // A pointer to the 8 children of this node.
  vtkOctreePointLocatorNode** Children;

  // Description:
  // The ID of this octant.  If it is not a leaf octant then ID=-1.
  int ID;

  // Description:
  // The minimum Id of the ordered points in this octant (note that
  // this Id is different than the vtkIdType used for referencing
  // the point in the data set.
  int MinID;

  vtkOctreePointLocatorNode(const vtkOctreePointLocatorNode&); // Not implemented
  void operator=(const vtkOctreePointLocatorNode&); // Not implemented
};

#endif
