/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLocator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkLocator - abstract base class for objects that accelerate spatial searches
// .SECTION Description
// vtkLocator is an abstract base class for spatial search objects, or
// locators. The principle behind locators is that they divide 3-space into
// small pieces (or "buckets") that can be quickly found in response to
// queries like point location, line intersection, or object-object
// intersection.
//
// The purpose of this base class is to provide ivars and methods shared by
// all locators. The GenerateRepresentation() is one such interesting method.
// This method works in conjunction with vtkLocatorFilter to create polygonal
// representations for the locator. For example, if the locator is an OBB tree
// (i.e., vtkOBBTree.h), then the representation is a set of one or more
// oriented bounding boxes, depending upon the specified level.
//
// Locators typically work as follows. One or more "entities", such as
// points or cells, are inserted into the tree. These entities are associated
// with one or more buckets. Then, when performing geometric operations, the
// operations are performed first on the buckets, and then if the operation
// tests positive, then on the entities in the bucket. For example, during
// collision tests, the locators are collided first to identify intersecting
// buckets. If an intersection is found, more expensive operations are then
// carried out on the entities in the bucket.
//
// To obtain good performance, locators are often organized in a tree
// structure.  In such a structure, there are frequently multiple "levels"
// corresponding to different nodes in the tree. So the word level (in the
// context of the locator) can be used to specify a particular representation
// in the tree.  For example, in an octree (which is a tree with 8 children),
// level 0 is the bounding box, or root octant, and level 1 consists of its
// eight children.

// .SECTION See Also
// vtkPointLocator vtkCellLocator vtkOBBTree vtkMergePoints

#ifndef vtkLocator_h
#define vtkLocator_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkObject.h"

class vtkDataSet;
class vtkPolyData;

class VTKCOMMONDATAMODEL_EXPORT vtkLocator : public vtkObject
{
public:
  vtkTypeMacro(vtkLocator,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Build the locator from the points/cells defining this dataset.
  virtual void SetDataSet(vtkDataSet*);
  vtkGetObjectMacro(DataSet,vtkDataSet);

  // Description:
  // Set the maximum allowable level for the tree. If the Automatic ivar is
  // off, this will be the target depth of the locator.
  // Initial value is 8.
  vtkSetClampMacro(MaxLevel,int,0,VTK_INT_MAX);
  vtkGetMacro(MaxLevel,int);

  // Description:
  // Get the level of the locator (determined automatically if Automatic is
  // true). The value of this ivar may change each time the locator is built.
  // Initial value is 8.
  vtkGetMacro(Level,int);

  // Description:
  // Boolean controls whether locator depth/resolution of locator is computed
  // automatically from average number of entities in bucket. If not set,
  // there will be an explicit method to control the construction of the
  // locator (found in the subclass).
  vtkSetMacro(Automatic,int);
  vtkGetMacro(Automatic,int);
  vtkBooleanMacro(Automatic,int);

  // Description:
  // Specify absolute tolerance (in world coordinates) for performing
  // geometric operations.
  vtkSetClampMacro(Tolerance,double,0.0,VTK_DOUBLE_MAX);
  vtkGetMacro(Tolerance,double);

  // Description:
  // Cause the locator to rebuild itself if it or its input dataset has
  // changed.
  virtual void Update();

  // Description:
  // Initialize locator. Frees memory and resets object as appropriate.
  virtual void Initialize();

  // Description:
  // Build the locator from the input dataset.
  virtual void BuildLocator() = 0;

  // Description:
  // Free the memory required for the spatial data structure.
  virtual void FreeSearchStructure() = 0;

  // Description:
  // Method to build a representation at a particular level. Note that the
  // method GetLevel() returns the maximum number of levels available for
  // the tree. You must provide a vtkPolyData object into which to place the
  // data.
  virtual void GenerateRepresentation(int level, vtkPolyData *pd) = 0;

  // Description:
  // Return the time of the last data structure build.
  vtkGetMacro(BuildTime, unsigned long);

  // Description:
  // Handle the PointSet <-> Locator loop.
  virtual void Register(vtkObjectBase *o);
  virtual void UnRegister(vtkObjectBase *o);
protected:
  vtkLocator();
  ~vtkLocator();

  vtkDataSet *DataSet;
  int Automatic; // boolean controls automatic subdivision (or uses user spec.)
  double Tolerance; // for performing merging
  int MaxLevel;
  int Level;

  vtkTimeStamp BuildTime;  // time at which locator was built

  virtual void ReportReferences(vtkGarbageCollector*);
private:
  vtkLocator(const vtkLocator&);  // Not implemented.
  void operator=(const vtkLocator&);  // Not implemented.
};

#endif


