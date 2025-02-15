// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSphereTree
 * @brief   class to build and traverse sphere trees
 *
 * vtkSphereTree is a helper class used to build and traverse sphere
 * trees. Various types of trees can be constructed for different VTK
 * dataset types, as well well as different approaches to organize
 * the tree into hierarchies.
 *
 * Typically building a complete sphere tree consists of two parts: 1)
 * creating spheres for each cell in the dataset, then 2) creating an
 * organizing hierarchy. The structure of the hierarchy varies depending on
 * the topological characteristics of the dataset.
 *
 * Once the tree is constructed, various geometric operations are available
 * for quickly selecting cells based on sphere tree operations; for example,
 * process all cells intersecting a plane (i.e., use the sphere tree to identify
 * candidate cells for plane intersection).
 *
 * This class does not necessarily create optimal sphere trees because
 * some of its requirements (fast build time, provide simple reference
 * code, a single bounding sphere per cell, etc.) precludes optimal
 * performance. It is also oriented to computing on cells versus the
 * classic problem of collision detection for polygonal models. For
 * more information you want to read Gareth Bradshaw's PhD thesis
 * "Bounding Volume Hierarchies for Level-of-Detail Collision
 * Handling" which does a nice job of laying out the challenges and
 * important algorithms relative to sphere trees and BVH (bounding
 * volume hierarchies).
 *
 * @sa
 * vtkSphereTreeFilter vtkPlaneCutter
 */

#ifndef vtkSphereTree_h
#define vtkSphereTree_h

#include "vtkCommonExecutionModelModule.h" // For export macro
#include "vtkObject.h"
#include "vtkPlane.h" // to specify the cutting plane

VTK_ABI_NAMESPACE_BEGIN
class vtkDoubleArray;
class vtkDataArray;
class vtkIdList;
class vtkDataSet;
class vtkStructuredGrid;
class vtkUnstructuredGrid;
class vtkTimeStamp;
struct vtkSphereTreeHierarchy;

#define VTK_MAX_SPHERE_TREE_RESOLUTION 10
#define VTK_MAX_SPHERE_TREE_LEVELS 20

// VTK Class proper
class VTKCOMMONEXECUTIONMODEL_EXPORT vtkSphereTree : public vtkObject
{
public:
  /**
   * Instantiate the sphere tree.
   */
  static vtkSphereTree* New();

  ///@{
  /**
   * Standard type related macros and PrintSelf() method.
   */
  vtkTypeMacro(vtkSphereTree, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  ///@{
  /**
   * Specify the dataset from which to build the sphere tree.
   */
  virtual void SetDataSet(vtkDataSet*);
  vtkGetObjectMacro(DataSet, vtkDataSet);
  ///@}

  ///@{
  /**
   * Build the sphere tree (if necessary) from the data set specified. The
   * build time is recorded so the sphere tree will only build if something has
   * changed. An alternative method is available to both set the dataset and
   * then build the sphere tree.
   */
  void Build();
  void Build(vtkDataSet* input);
  ///@}

  ///@{
  /**
   * Control whether the tree hierarchy is built. If not, then just
   * cell spheres are created (one for each cell).
   */
  vtkSetMacro(BuildHierarchy, bool);
  vtkGetMacro(BuildHierarchy, bool);
  vtkBooleanMacro(BuildHierarchy, bool);
  ///@}

  ///@{
  /**
   * Methods for cell selection based on a geometric query. Internally
   * different methods are used depending on the dataset type. The array
   * returned is set to non-zero for each cell that intersects the geometric
   * entity. SelectPoint marks all cells with a non-zero value that may
   * contain a point. SelectLine marks all cells that may intersect an
   * infinite line. SelectPlane marks all cells that may intersect with an
   * infinite plane.
   */
  const unsigned char* SelectPoint(double point[3], vtkIdType& numSelected);
  const unsigned char* SelectLine(double origin[3], double ray[3], vtkIdType& numSelected);
  const unsigned char* SelectPlane(double origin[3], double normal[3], vtkIdType& numSelected);
  ///@}

  ///@{
  /**
   * Methods for cell selection based on a geometric query. Internally
   * different methods are used depending on the dataset type. The method
   * populates an vtkIdList with cell ids that may satisfy the geometric
   * query (the user must provide a vtkIdList which the methods fill in).
   * SelectPoint lists all cells with a non-zero value that may contain a
   * point. SelectLine lists all cells that may intersect an infinite
   * line. SelectPlane lists all cells that may intersect with an infinite
   * plane.
   */
  void SelectPoint(double point[3], vtkIdList* cellIds);
  void SelectLine(double origin[3], double ray[3], vtkIdList* cellIds);
  void SelectPlane(double origin[3], double normal[3], vtkIdList* cellIds);
  ///@}

  ///@{
  /**
   * Sphere tree creation requires gathering spheres into groups. The
   * Resolution variable is a rough guide to the size of each group (the size
   * different meanings depending on the type of data (structured versus
   * unstructured). For example, in 3D structured data, blocks of resolution
   * Resolution^3 are created. By default the Resolution is three.
   */
  vtkSetClampMacro(Resolution, int, 2, VTK_MAX_SPHERE_TREE_RESOLUTION);
  vtkGetMacro(Resolution, int);
  ///@}

  ///@{
  /**
   * Specify the maximum number of levels for the tree. By default, the
   * number of levels is set to ten. If the number of levels is set to one or
   * less, then no hierarchy is built (i.e., just the spheres for each cell
   * are created). Note that the actual level of the tree may be less than
   * this value depending on the number of cells and Resolution factor.
   */
  vtkSetClampMacro(MaxLevel, int, 1, VTK_MAX_SPHERE_TREE_LEVELS);
  vtkGetMacro(MaxLevel, int);
  ///@}

  /**
   * Get the current depth of the sphere tree. This value may change each
   * time the sphere tree is built and the branching factor (i.e.,
   * resolution) changes. Note that after building the sphere tree there are
   * [0,this->NumberOfLevels) defined levels.
   */
  vtkGetMacro(NumberOfLevels, int);

  ///@{
  /**
   * Special methods to retrieve the sphere tree data. This is
   * generally used for debugging or with filters like
   * vtkSphereTreeFilter. Both methods return an array of double*
   * where four doubles represent a sphere (center + radius). In the
   * first method a sphere per cell is returned. In the second method
   * the user must also specify a level in the sphere tree (used to
   * retrieve the hierarchy of the tree). Note that null pointers can
   * be returned if the request is not consistent with the state of
   * the sphere tree.
   */
  const double* GetCellSpheres();
  const double* GetTreeSpheres(int level, vtkIdType& numSpheres);
  ///@}

  ///@{
  /**
   * Participate in garbage collection via ReportReferences.
   */
  bool UsesGarbageCollector() const override { return true; }
  ///@}

protected:
  vtkSphereTree();
  ~vtkSphereTree() override;

  // Data members
  vtkDataSet* DataSet;
  unsigned char* Selected;
  int Resolution;
  int MaxLevel;
  int NumberOfLevels;
  bool BuildHierarchy;

  // The tree and its hierarchy
  vtkDoubleArray* Tree;
  double* TreePtr;
  vtkSphereTreeHierarchy* Hierarchy;

  // Supporting data members
  double AverageRadius;   // average radius of cell sphere
  double SphereBounds[6]; // the dataset bounds computed from cell spheres
  vtkTimeStamp BuildTime; // time at which tree was built

  // Supporting methods
  void BuildTreeSpheres(vtkDataSet* input);
  void ExtractCellIds(const unsigned char* selected, vtkIdList* cellIds, vtkIdType numSelected);

  void BuildTreeHierarchy(vtkDataSet* input);
  void BuildStructuredHierarchy(vtkStructuredGrid* input, double* tree);
  void BuildUnstructuredHierarchy(vtkDataSet* input, double* tree);
  int SphereTreeType; // keep track of the type of tree hierarchy generated

  void ReportReferences(vtkGarbageCollector*) override;

private:
  vtkSphereTree(const vtkSphereTree&) = delete;
  void operator=(const vtkSphereTree&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
