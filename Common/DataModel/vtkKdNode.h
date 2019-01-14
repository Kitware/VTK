/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKdNode.h

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

/**
 * @class   vtkKdNode
 * @brief   This class represents a single spatial region
 *     in an 3D axis aligned binary spatial partitioning.  It is assumed
 *     the region bounds some set of points.  Regions are represented
 *     as nodes in a binary tree.
 *
 *
 *
 * @sa
 *      vtkKdTree vtkOBSPCuts
*/

#ifndef vtkKdNode_h
#define vtkKdNode_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkObject.h"

class vtkCell;
class vtkPlanesIntersection;

class VTKCOMMONDATAMODEL_EXPORT vtkKdNode : public vtkObject
{
public:
  vtkTypeMacro(vtkKdNode, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  static vtkKdNode *New();

  //@{
  /**
   * Set/Get the dimension along which this region is divided.
   * (0 - x, 1 - y, 2 - z, 3 - leaf node (default)).
   */
  vtkSetMacro(Dim, int);
  vtkGetMacro(Dim, int);
  //@}

  /**
   * Get the location of the division plane along the axis the region
   * is divided.  See also GetDim().  The result is undertermined if
   * this node is not divided (a leaf node).
   */
  virtual double GetDivisionPosition();

  //@{
  /**
   * Set/Get the number of points contained in this region.
   */
  vtkSetMacro(NumberOfPoints, int);
  vtkGetMacro(NumberOfPoints, int);
  //@}

  //@{
  /**
   * Set/Get the bounds of the spatial region represented by this node.
   * Caller allocates storage for 6-vector in GetBounds.
   */
  void SetBounds(double x1,double x2,double y1,double y2,double z1,double z2);
  void SetBounds(const double b[6])
  {
    this->SetBounds(b[0], b[1], b[2], b[3], b[4], b[5]);
  }
  void GetBounds(double *b) const;
  //@}

  //@{
  /**
   * Set/Get the bounds of the points contained in this spatial region.
   * This may be smaller than the bounds of the region itself.
   * Caller allocates storage for 6-vector in GetDataBounds.
   */
  void SetDataBounds(double x1,double x2,double y1,double y2,double z1,double z2);
  void GetDataBounds(double *b) const;
  //@}

  /**
   * Given a pointer to NumberOfPoints points, set the DataBounds of this
   * node to the bounds of these points.
   */
  void SetDataBounds(float *v);

  /**
   * Get a pointer to the 3 bound minima (xmin, ymin and zmin) or the
   * 3 bound maxima (xmax, ymax, zmax).  Don't free this pointer.
   */
  double *GetMinBounds() VTK_SIZEHINT(3) {return this->Min;}
  double *GetMaxBounds() VTK_SIZEHINT(3) {return this->Max;}

  /**
   * Set the xmin, ymin and zmin value of the bounds of this region
   */
  void SetMinBounds(const double *mb);

  /**
   * Set the xmax, ymax and zmax value of the bounds of this region
   */
  void SetMaxBounds(const double *mb);

  /**
   * Get a pointer to the 3 data bound minima (xmin, ymin and zmin) or the
   * 3 data bound maxima (xmax, ymax, zmax).  Don't free this pointer.
   */
  double *GetMinDataBounds() VTK_SIZEHINT(3) {return this->MinVal;}
  double *GetMaxDataBounds() VTK_SIZEHINT(3) {return this->MaxVal;}

  /**
   * Set the xmin, ymin and zmin value of the bounds of this
   * data within this region
   */
  void SetMinDataBounds(const double *mb);

  /**
   * Set the xmax, ymax and zmax value of the bounds of this
   * data within this region
   */
  void SetMaxDataBounds(const double *mb);

  //@{
  /**
   * Set/Get the ID associated with the region described by this node.  If
   * this is not a leaf node, this value should be -1.
   */
  vtkSetMacro(ID, int);
  vtkGetMacro(ID, int);
  //@}

  //@{
  /**
   * If this node is not a leaf node, there are leaf nodes below it whose
   * regions represent a partitioning of this region.  The IDs of these
   * leaf nodes form a contiguous set.  Set/Get the range of the IDs of
   * the leaf nodes below this node.  If this is already a leaf node, these
   * values should be the same as the ID.
   */
  vtkGetMacro(MinID, int);
  vtkGetMacro(MaxID, int);
  vtkSetMacro(MinID, int);
  vtkSetMacro(MaxID, int);
  //@}

  /**
   * Add the left and right children.
   */
  void AddChildNodes(vtkKdNode *left, vtkKdNode *right);

  /**
   * Delete the left and right children.
   */
  void DeleteChildNodes();

  //@{
  /**
   * Set/Get a pointer to the left child of this node.
   */
  vtkGetObjectMacro(Left, vtkKdNode);
  void SetLeft(vtkKdNode* left);
  //@}

  //@{
  /**
   * Set/Get a pointer to the right child of this node.
   */
  vtkGetObjectMacro(Right, vtkKdNode);
  void SetRight(vtkKdNode *right);
  //@}

  //@{
  /**
   * Set/Get a pointer to the parent of this node.
   */
  vtkGetObjectMacro(Up, vtkKdNode);
  void SetUp(vtkKdNode* up);
  //@}

  /**
   * Return 1 if this spatial region intersects the axis-aligned box given
   * by the bounds passed in.  Use the possibly smaller bounds of the points
   * within the region if useDataBounds is non-zero.
   */
  int IntersectsBox(double x1,double x2,double y1,double y2,double z1,double z2,
                    int useDataBounds);

  /**
   * Return 1 if this spatial region intersects a sphere described by
   * it's center and the square of it's radius. Use the possibly smaller
   * bounds of the points within the region if useDataBounds is non-zero.
   */
  int IntersectsSphere2(double x, double y, double z, double rSquared,
                        int useDataBounds);

  /**
   * A vtkPlanesIntersection object represents a convex 3D region bounded
   * by planes, and it is capable of computing intersections of
   * boxes with itself.  Return 1 if this spatial region intersects
   * the spatial region described by the vtkPlanesIntersection object.
   * Use the possibly smaller bounds of the points within the region
   * if useDataBounds is non-zero.
   */
  int IntersectsRegion(vtkPlanesIntersection *pi, int useDataBounds);

  /**
   * Return 1 if the cell specified intersects this region.  If you
   * already know the ID of the region containing the cell's centroid,
   * provide that as an argument.  If you already know the bounds of the
   * cell, provide that as well, in the form of xmin,xmax,ymin,ymax,zmin,
   * zmax.  Either of these may speed the calculation.
   * Use the possibly smaller bounds of the points within the region
   * if useDataBounds is non-zero.
   */
  int IntersectsCell(vtkCell *cell, int useDataBounds,
                     int cellRegion=-1, double *cellBounds=nullptr);

  /**
   * Return 1 if this spatial region entirely contains a box specified
   * by it's bounds. Use the possibly smaller
   * bounds of the points within the region if useDataBounds is non-zero.
   */
  int ContainsBox(double x1,double x2,double y1,double y2,double z1,double z2,
                  int useDataBounds);

  /**
   * Return 1 if this spatial region entirely contains the given point.
   * Use the possibly smaller bounds of the points within the region
   * if useDataBounds is non-zero.
   */
  vtkTypeBool ContainsPoint(double x, double y, double z, int useDataBounds);

  /**
   * Calculate the distance squared from any point to the boundary of this
   * region.  Use the boundary of the points within the region if useDataBounds
   * is non-zero.
   */
  double GetDistance2ToBoundary(double x, double y, double z, int useDataBounds);

  /**
   * Calculate the distance squared from any point to the boundary of this
   * region.  Use the boundary of the points within the region if useDataBounds
   * is non-zero.  Set boundaryPt to the point on the boundary.
   */
  double GetDistance2ToBoundary(double x, double y, double z, double *boundaryPt,
                                int useDataBounds);

  /**
   * Calculate the distance from the specified point (which is required to
   * be inside this spatial region) to an interior boundary.  An interior
   * boundary is one that is not also an boundary of the entire space
   * partitioned by the tree of vtkKdNode's.
   */
  double GetDistance2ToInnerBoundary(double x, double y, double z);

  //@{
  /**
   * For debugging purposes, print out this node.
   */
  void PrintNode(int depth);
  void PrintVerboseNode(int depth);
  //@}

protected:

  vtkKdNode();
  ~vtkKdNode() override;

private:

  double _GetDistance2ToBoundary(
    double x, double y, double z, double *boundaryPt,
    int innerBoundaryOnly, int useDataBounds);

  double Min[3];       // spatial bounds of node
  double Max[3];       // spatial bounds of node
  double MinVal[3];    // spatial bounds of data within node
  double MaxVal[3];    // spatial bounds of data within node
  int NumberOfPoints;

  vtkKdNode *Up;

  vtkKdNode *Left;
  vtkKdNode *Right;

  int Dim;

  int ID;        // region id

  int MinID;
  int MaxID;

  vtkKdNode(const vtkKdNode&) = delete;
  void operator=(const vtkKdNode&) = delete;
};

#endif
