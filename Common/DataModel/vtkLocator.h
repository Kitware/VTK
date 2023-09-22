// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkLocator
 * @brief   abstract base class for objects that accelerate spatial searches
 *
 * vtkLocator is an abstract base class for spatial search objects, or
 * locators. The principle behind locators is that they divide 3-space into
 * small regions (or "buckets") that can be quickly found in response to
 * queries about point location, line intersection, or object-object
 * intersection.
 *
 * The purpose of this base class is to provide data members and methods
 * shared by all locators. The GenerateRepresentation() is one such
 * interesting method.  This method works in conjunction with
 * vtkLocatorFilter to create polygonal representations for the locator. For
 * example, if the locator is an OBB tree (i.e., vtkOBBTree.h), then the
 * representation is a set of one or more oriented bounding boxes, depending
 * upon the specified level.
 *
 * Locators typically work as follows. One or more "entities", such as points
 * or cells, are inserted into the locator structure. These entities are
 * associated with one or more buckets. Then, when performing geometric
 * operations, the operations are performed first on the buckets, and then if
 * the operation tests positive, then on the entities in the bucket. For
 * example, during collision tests, the locators are collided first to
 * identify intersecting buckets. If an intersection is found, more expensive
 * operations are then carried out on the entities in the bucket.
 *
 * To obtain good performance, locators are often organized in a tree
 * structure.  In such a structure, there are frequently multiple "levels"
 * corresponding to different nodes in the tree. So the word level (in the
 * context of the locator) can be used to specify a particular representation
 * in the tree.  For example, in an octree (which is a tree with 8 children),
 * level 0 is the bounding box, or root octant, and level 1 consists of its
 * eight children.
 *
 * @warning
 * There is a concept of static and incremental locators. Static locators are
 * constructed one time, and then support appropriate queries. Incremental
 * locators may have data inserted into them over time (e.g., adding new
 * points during the process of isocontouring).
 *
 * @sa
 * vtkPointLocator vtkCellLocator vtkOBBTree vtkMergePoints
 */

#ifndef vtkLocator_h
#define vtkLocator_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkObject.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkDataSet;
class vtkPolyData;

class VTKCOMMONDATAMODEL_EXPORT vtkLocator : public vtkObject
{
public:
  ///@{
  /**
   * Standard type and print methods.
   */
  vtkTypeMacro(vtkLocator, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  ///@{
  /**
   * Build the locator from the points/cells defining this dataset.
   */
  virtual void SetDataSet(vtkDataSet*);
  vtkGetObjectMacro(DataSet, vtkDataSet);
  ///@}

  ///@{
  /**
   * Set the maximum allowable level for the tree. If the Automatic ivar is
   * off, this will be the target depth of the locator.
   * Initial value is 8.
   */
  vtkSetClampMacro(MaxLevel, int, 0, VTK_INT_MAX);
  vtkGetMacro(MaxLevel, int);
  ///@}

  ///@{
  /**
   * Get the level of the locator (determined automatically if Automatic is
   * true). The value of this ivar may change each time the locator is built.
   * Initial value is 8.
   */
  vtkGetMacro(Level, int);
  ///@}

  ///@{
  /**
   * Boolean controls whether locator depth/resolution of locator is computed
   * automatically from average number of entities in bucket. If not set,
   * there will be an explicit method to control the construction of the
   * locator (found in the subclass).
   */
  vtkSetMacro(Automatic, vtkTypeBool);
  vtkGetMacro(Automatic, vtkTypeBool);
  vtkBooleanMacro(Automatic, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Specify absolute tolerance (in world coordinates) for performing
   * geometric operations.
   */
  vtkSetClampMacro(Tolerance, double, 0.0, VTK_DOUBLE_MAX);
  vtkGetMacro(Tolerance, double);
  ///@}

  ///@{
  /**
   * Get/Set UseExistingSearchStructure, which when enabled it allows the locator to NOT be
   * built again. This is useful when you have a dataset that either changes because
   * the FieldData (PointData/CellData) changed or the actual dataset object changed
   * but it's actually the same geometry (useful when a dataset has timesteps).
   *
   * When this flag is on you need to use ForceBuildLocator() to rebuild the locator,
   * if your dataset changes.
   *
   * Default is off.
   */
  vtkSetMacro(UseExistingSearchStructure, vtkTypeBool);
  vtkGetMacro(UseExistingSearchStructure, vtkTypeBool);
  vtkBooleanMacro(UseExistingSearchStructure, vtkTypeBool);
  ///@}

  /**
   * Cause the locator to rebuild itself if it or its input dataset has
   * changed.
   */
  virtual void Update();

  /**
   * Initialize locator. Frees memory and resets object as appropriate.
   */
  virtual void Initialize();

  /**
   * Build the locator from the input dataset. This will NOT do anything if
   * UseExistingSearchStructure is on.
   */
  virtual void BuildLocator() = 0;

  /**
   * Build the locator from the input dataset (even if UseExistingSearchStructure is on).
   *
   * This function is not pure virtual to maintain backwards compatibility.
   */
  virtual void ForceBuildLocator() {}

  /**
   * Free the memory required for the spatial data structure.
   */
  virtual void FreeSearchStructure() = 0;

  /**
   * Method to build a representation at a particular level. Note that the
   * method GetLevel() returns the maximum number of levels available for
   * the tree. You must provide a vtkPolyData object into which to place the
   * data.
   */
  virtual void GenerateRepresentation(int level, vtkPolyData* pd) = 0;

  ///@{
  /**
   * Return the time of the last data structure build.
   */
  vtkGetMacro(BuildTime, vtkMTimeType);
  ///@}

  ///@{
  /**
   * Handle the PointSet <-> Locator loop.
   */
  bool UsesGarbageCollector() const override { return true; }
  ///@}

protected:
  vtkLocator();
  ~vtkLocator() override;

  /**
   * This function is not pure virtual to maintain backwards compatibility.
   */
  virtual void BuildLocatorInternal() {}

  vtkDataSet* DataSet;
  vtkTypeBool UseExistingSearchStructure;
  vtkTypeBool Automatic; // boolean controls automatic subdivision (or uses user spec.)
  double Tolerance;      // for performing merging
  int MaxLevel;
  int Level;

  vtkTimeStamp BuildTime; // time at which locator was built

  void ReportReferences(vtkGarbageCollector*) override;

private:
  vtkLocator(const vtkLocator&) = delete;
  void operator=(const vtkLocator&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
