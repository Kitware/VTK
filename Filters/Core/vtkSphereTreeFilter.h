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
 * radius, and level number for the cell spheres and/or the different levels
 * in the tree hierarchy (assuming that the hierarchy is built). The output
 * can be glyphed using a filter like vtkGlyph3D to actually visualize the
 * sphere tree. The primary use of this class is for visualization of sphere
 * trees, and debugging the construction and use of sphere trees.
 *
 * Note that this class may create a sphere tree, and then build it,
 * for the input dataset to this filter (if no sphere tree is
 * provided). If the user specifies a sphere tree, then the specified
 * sphere tree is used. Thus the input to the filter is optional.
 *
 * @sa
 * vtkSphereTree vtkPlaneCutter
 */

#ifndef __vtkSphereTreeFilter_h
#define __vtkSphereTreeFilter_h

#include "vtkFiltersCoreModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

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
  void PrintSelf(ostream& os, vtkIndent indent);
  //@}

  /**
   * Modified GetMTime because the sphere tree may have changed.
   */
  vtkMTimeType GetMTime() VTK_OVERRIDE;

  //@{
  /**
   * Enable or disable the building and generation of the sphere tree
   * hierarchy. The hierarchy represents different levels in the tree
   * and enables rapid traversal of the tree.
   */
  vtkSetMacro(TreeHierarchy,int);
  vtkGetMacro(TreeHierarchy,int);
  vtkBooleanMacro(TreeHierarchy,int);
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
   * Specify the level of the tree to extract. A value of (-1) means all
   * levels. Note that level 0 is the root of the sphere tree. By default all
   * levels are extracted. Note that if TreeHierarchy is off, then it is only
   * possible to extract leaf spheres (i.e., spheres for each cell of the
   * associated dataset).
   */
  vtkSetClampMacro(Level,int,-1,VTK_INT_MAX);
  vtkGetMacro(Level,int);
  //@}

protected:
  vtkSphereTreeFilter();
  ~vtkSphereTreeFilter() VTK_OVERRIDE;

  vtkSphereTree *SphereTree;
  int TreeHierarchy;
  int Level;

  virtual int RequestData(vtkInformation *, vtkInformationVector **,
                          vtkInformationVector *) VTK_OVERRIDE;
  virtual int FillInputPortInformation(int port,
                                       vtkInformation *info) VTK_OVERRIDE;

private:
  vtkSphereTreeFilter(const vtkSphereTreeFilter&) VTK_DELETE_FUNCTION;
  void operator=(const vtkSphereTreeFilter&) VTK_DELETE_FUNCTION;

};

#endif
