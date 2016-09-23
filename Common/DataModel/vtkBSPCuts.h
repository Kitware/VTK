/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBSPCuts.h

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
 * @class   vtkBSPCuts
 * @brief   This class represents an axis-aligned Binary Spatial
 *    Partitioning of a 3D space.
 *
 *
 *    This class converts between the vtkKdTree
 *    representation of a tree of vtkKdNodes (used by vtkDistributedDataFilter)
 *    and a compact array representation that might be provided by a
 *    graph partitioning library like Zoltan.  Such a representation
 *    could be used in message passing.
 *
 * @sa
 *      vtkKdTree vtkKdNode vtkDistributedDataFilter
*/

#ifndef vtkBSPCuts_h
#define vtkBSPCuts_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkDataObject.h"

class vtkKdNode;

class VTKCOMMONDATAMODEL_EXPORT vtkBSPCuts : public vtkDataObject
{
public:
  static vtkBSPCuts *New();
  vtkTypeMacro(vtkBSPCuts, vtkDataObject);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Initialize the cuts with arrays of information.  This type of
   * information would be obtained from a graph partitioning software
   * package like Zoltan.

   * bounds - the bounds (xmin, xmax, ymin, ymax, zmin, zmax) of the
   * space being partitioned
   * ncuts - the number cuts, also the size of the following arrays
   * dim   - the dimension along which the cut is made (x/y/z - 0/1/2)
   * coord - the location of the cut along the axis
   * lower - array index for the lower region bounded by the cut
   * upper - array index for the upper region bounded by the cut
   * lowerDataCoord - optional upper bound of the data in the lower region
   * upperDataCoord - optional lower bound of the data in the upper region
   * npoints - optional number of points in the spatial region
   */

  void CreateCuts(double *bounds,
                  int ncuts, int *dim, double *coord,
                  int *lower, int *upper,
                  double *lowerDataCoord, double *upperDataCoord,
                  int *npoints);

  /**
   * Initialize the cuts from a tree of vtkKdNode's
   */

  void CreateCuts(vtkKdNode *kd);

  /**
   * Return a tree of vtkKdNode's representing the cuts specified
   * in this object.  This is our copy, don't delete it.
   */

  vtkKdNode *GetKdNodeTree(){return this->Top;}

  /**
   * Get the number of cuts in the partitioning, which also the size of
   * the arrays in the array representation of the partitioning.
   */

  vtkGetMacro(NumberOfCuts, int);

  /**
   * Get the arrays representing the cuts in the partitioning.
   */

  int GetArrays(int len, int *dim, double *coord, int *lower, int *upper,
                double *lowerDataCoord, double *upperDataCoord, int *npoints);

  /**
   * Compare these cuts with those of the other tree.  Returns true if
   * the two trees are the same.
   */
  int Equals(vtkBSPCuts *other, double tolerance = 0.0);

  void PrintTree();
  void PrintArrays();

  //@{
  /**
   * Retrieve an instance of this class from an information object.
   */
  static vtkBSPCuts* GetData(vtkInformation* info);
  static vtkBSPCuts* GetData(vtkInformationVector* v, int i=0);
  //@}

  /**
   * Restore data object to initial state,
   */
  void Initialize() VTK_OVERRIDE;

  //@{
  /**
   * Shallow copy.  These copy the data, but not any of the
   * pipeline connections.
   */
  void ShallowCopy(vtkDataObject *src) VTK_OVERRIDE;
  void DeepCopy(vtkDataObject *src) VTK_OVERRIDE;
  //@}

protected:

  vtkBSPCuts();
  ~vtkBSPCuts() VTK_OVERRIDE;

  static void DeleteAllDescendants(vtkKdNode *kd);

  static int CountNodes(vtkKdNode *kd);
  static void SetMinMaxId(vtkKdNode *kd);
  static void _PrintTree(vtkKdNode *kd, int depth);

  void BuildTree(vtkKdNode *kd, int idx);
  int WriteArray(vtkKdNode *kd, int loc);

  void ResetArrays();
  void AllocateArrays(int size);

  vtkKdNode *Top;

  // required cut information

  int NumberOfCuts;// number of cuts, also length of each array
  int *Dim;        // dimension (x/y/z - 0/1/2) where cut occurs
  double *Coord;   // location of cut along axis
  int *Lower;               // location in arrays of left (lower) child info
  int *Upper;               // location in arrays of right (lower) child info

  // optional cut information

  double *LowerDataCoord;   // coordinate of uppermost data in lower half
  double *UpperDataCoord;   // coordinate of lowermost data in upper half
  int *Npoints;             // number of data values in partition

  double Bounds[6];

  vtkBSPCuts(const vtkBSPCuts&) VTK_DELETE_FUNCTION;
  void operator=(const vtkBSPCuts&) VTK_DELETE_FUNCTION;
};

#endif
