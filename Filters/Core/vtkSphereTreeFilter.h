/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSphereTreeFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkSphereTreeFilter
 * @brief represent a sphere tree as vtkPolyData
 *
 * vtkSphereTreeFilter is a filter that produces a vtkPolyData representation
 * of a sphere tree (vtkSphereTree). Basically it generates a point, a scalar
 * radius, and tree level number for the cell spheres and/or the different levels
 * in the tree hierarchy (assuming that the hierarchy is built). The output
 * can be glyphed using a filter like vtkGlyph3D to actually visualize the
 * sphere tree. The primary use of this class is for visualization of sphere
 * trees, and debugging the construction and use of sphere trees.
 *
 * Additional capabilities include production of candidate spheres based on
 * geometric queries. For example, queries based on a point, infinite line,
 * and infinite plane are possible.
 *
 * Note that this class may create a sphere tree, and then build it, for the
 * input dataset to this filter (if no sphere tree is provided). If the user
 * specifies a sphere tree, then the specified sphere tree is used. Thus the
 * input to the filter is optional. Consequently this filter can act like a source,
 * or as a filter in a pipeline.
 *
 * @sa
 * vtkSphereTree vtkPlaneCutter
 */

#ifndef vtkSphereTreeFilter_h
#define vtkSphereTreeFilter_h

#include "vtkFiltersCoreModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

#define VTK_SPHERE_TREE_LEVELS 0
#define VTK_SPHERE_TREE_POINT  1
#define VTK_SPHERE_TREE_LINE   2
#define VTK_SPHERE_TREE_PLANE  3


class vtkSphereTree;

class VTKFILTERSCORE_EXPORT vtkSphereTreeFilter : public vtkPolyDataAlgorithm
{
public:
  /**
   * Instantiate the sphere tree filter.
   */
  static vtkSphereTreeFilter *New();

  //@{
  /**
   * Standard type related macros and PrintSelf() method.
   */
  vtkTypeMacro(vtkSphereTreeFilter,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  //@}

  //@{
  /**
   * Specify and retrieve the sphere tree.
   */
  virtual void SetSphereTree(vtkSphereTree*);
  vtkGetObjectMacro(SphereTree,vtkSphereTree);
  //@}

  //@{
  /**
   * Specify what information this filter is to extract from the sphere
   * tree. Options include: spheres that make up one or more levels; spheres
   * that intersect a specified plane; spheres that intersect a specified line;
   * and spheres that intersect a specified point. What is extracted are sphere
   * centers, a radius, and an optional level. By default the specified levels
   * are extracted.
   */
  vtkSetMacro(ExtractionMode,int);
  vtkGetMacro(ExtractionMode,int);
  void SetExtractionModeToLevels()
    {this->SetExtractionMode(VTK_SPHERE_TREE_LEVELS);}
  void SetExtractionModeToPoint()
    {this->SetExtractionMode(VTK_SPHERE_TREE_POINT);}
  void SetExtractionModeToLine()
    {this->SetExtractionMode(VTK_SPHERE_TREE_LINE);}
  void SetExtractionModeToPlane()
    {this->SetExtractionMode(VTK_SPHERE_TREE_PLANE);}
  const char *GetExtractionModeAsString();
  //@}

  //@{
  /**
   * Enable or disable the building and generation of the sphere tree
   * hierarchy. The hierarchy represents different levels in the tree
   * and enables rapid traversal of the tree.
   */
  vtkSetMacro(TreeHierarchy, bool);
  vtkGetMacro(TreeHierarchy, bool);
  vtkBooleanMacro(TreeHierarchy, bool);
  //@}

  //@{
  /**
   * Specify the level of the tree to extract (used when ExtractionMode is
   * set to Levels). A value of (-1) means all levels. Note that level 0 is
   * the root of the sphere tree. By default all levels are extracted. Note
   * that if TreeHierarchy is off, then it is only possible to extract leaf
   * spheres (i.e., spheres for each cell of the associated dataset).
   */
  vtkSetClampMacro(Level,int,-1,VTK_SHORT_MAX);
  vtkGetMacro(Level,int);
  //@}

  //@{
  /**
   * Specify a point used to extract one or more leaf spheres. This method is
   * used when extracting spheres using a point, line, or plane.
   */
  vtkSetVector3Macro(Point,double);
  vtkGetVectorMacro(Point,double,3);
  //@}

  //@{
  /**
   * Specify a line used to extract spheres (used when ExtractionMode is set
   * to Line). The Ray plus Point define an infinite line. The ray is a
   * vector defining the direction of the line.
   */
  vtkSetVector3Macro(Ray,double);
  vtkGetVectorMacro(Ray,double,3);
  //@}

  //@{
  /**
   * Specify a plane used to extract spheres (used when ExtractionMode is set
   * to Plane). The plane Normal plus Point define an infinite plane.
   */
  vtkSetVector3Macro(Normal,double);
  vtkGetVectorMacro(Normal,double,3);
  //@}

  /**
   * Modified GetMTime because the sphere tree may have changed.
   */
  vtkMTimeType GetMTime() override;

protected:
  vtkSphereTreeFilter();
  ~vtkSphereTreeFilter() override;

  vtkSphereTree *SphereTree;
  bool TreeHierarchy;
  int ExtractionMode;
  int Level;
  double Point[3];
  double Ray[3];
  double Normal[3];

  int RequestData(vtkInformation *, vtkInformationVector **,
                  vtkInformationVector *) override;
  int FillInputPortInformation(int port,
                               vtkInformation *info) override;

private:
  vtkSphereTreeFilter(const vtkSphereTreeFilter&) = delete;
  void operator=(const vtkSphereTreeFilter&) = delete;

};

#endif
