/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHyperTreeGridGeometry.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkHyperTreeGridGeometry
 * @brief   Hyper tree grid outer surface
 *
 * @sa
 * vtkHyperTreeGrid vtkHyperTreeGridAlgorithm
 *
 * @par Thanks:
 * This class was written by Philippe Pebay, Joachim Pouderoux, and Charles Law, Kitware 2013
 * This class was modified by Guenole Harel and Jacques-Bernard Lekien, 2014
 * This class was rewritten by Philippe Pebay, 2016
 * This class was modified by Jacques-Bernard Lekien and Guenole Harel, 2018
 * This work was supported by Commissariat a l'Energie Atomique
 * CEA, DAM, DIF, F-91297 Arpajon, France.
 */

#ifndef vtkHyperTreeGridGeometry_h
#define vtkHyperTreeGridGeometry_h

#include "vtkFiltersHyperTreeModule.h" // For export macro
#include "vtkHyperTreeGridAlgorithm.h"

class vtkBitArray;
class vtkCellArray;
class vtkHyperTreeGrid;
class vtkPoints;
class vtkIncrementalPointLocator;
class vtkDoubleArray;
class vtkHyperTreeGridNonOrientedGeometryCursor;
class vtkHyperTreeGridNonOrientedVonNeumannSuperCursor;
class vtkIdList;
class vtkIdTypeArray;
class vtkUnsignedCharArray;

class VTKFILTERSHYPERTREE_EXPORT vtkHyperTreeGridGeometry : public vtkHyperTreeGridAlgorithm
{
public:
  static vtkHyperTreeGridGeometry* New();
  vtkTypeMacro(vtkHyperTreeGridGeometry, vtkHyperTreeGridAlgorithm);
  void PrintSelf(ostream&, vtkIndent) override;

  //@{
  /**
   * Turn on/off merging of coincident points. Note that is merging is
   * on, points with different point attributes (e.g., normals) are merged,
   * which may cause rendering artifacts.
   */
  vtkSetMacro(Merging, bool);
  vtkGetMacro(Merging, bool);
  //@}

protected:
  vtkHyperTreeGridGeometry();
  ~vtkHyperTreeGridGeometry() override;

  /**
   * For this algorithm the output is a vtkPolyData instance
   */
  int FillOutputPortInformation(int, vtkInformation*) override;

  /**
   * Main routine to generate external boundary
   */
  int ProcessTrees(vtkHyperTreeGrid*, vtkDataObject*) override;

  /**
   * Recursively descend into tree down to leaves
   */
  void RecursivelyProcessTreeNot3D(vtkHyperTreeGridNonOrientedGeometryCursor*);
  void RecursivelyProcessTree3D(vtkHyperTreeGridNonOrientedVonNeumannSuperCursor*, unsigned char);

  /**
   * Process 1D leaves and issue corresponding edges (lines)
   */
  void ProcessLeaf1D(vtkHyperTreeGridNonOrientedGeometryCursor*);

  /**
   * Process 2D leaves and issue corresponding faces (quads)
   */
  void ProcessLeaf2D(vtkHyperTreeGridNonOrientedGeometryCursor*);

  /**
   * Process 3D leaves and issue corresponding cells (voxels)
   */
  void ProcessLeaf3D(vtkHyperTreeGridNonOrientedVonNeumannSuperCursor*);

  /**
   * Helper method to generate a face based on its normal and offset from cursor origin
   */
  void AddFace(vtkIdType useId, const double* origin, const double* size, unsigned int offset,
    unsigned int orientation, unsigned char hideEdge);

  void AddFace2(vtkIdType inId, vtkIdType useId, const double* origin, const double* size,
    unsigned int offset, unsigned int orientation, bool create = true);

  /**
   * material Mask
   */
  vtkBitArray* Mask;

  /**
   * Pure Material Mask
   */
  vtkBitArray* PureMask;

  /**
   * Dimension of input grid
   */
  unsigned int Dimension;

  /**
   * Orientation of input grid when dimension < 3
   */
  unsigned int Orientation;

  /**
   * Branch Factor
   */
  int BranchFactor;

  /**
   * Storage for points of output unstructured mesh
   */
  vtkPoints* Points;

  /**
   * Storage for cells of output unstructured mesh
   */
  vtkCellArray* Cells;

  /**
   *JB Un locator est utilise afin de produire un maillage avec moins
   *JB de points. Le gain en 3D est de l'ordre d'un facteur 4 !
   */
  bool Merging;
  vtkIncrementalPointLocator* Locator;

  // JB A RECUPERER DANS LE .H VTK9
  bool HasInterface;
  vtkDoubleArray* Normals;
  vtkDoubleArray* Intercepts;

  vtkIdList* FaceIDs;
  vtkPoints* FacePoints;

  vtkIdType EdgesA[12];
  vtkIdType EdgesB[12];

  vtkIdTypeArray* FacesA;
  vtkIdTypeArray* FacesB;

  vtkDoubleArray* FaceScalarsA;
  vtkDoubleArray* FaceScalarsB;

  /**
   * Array used to hide edges
   * left by masked cells.
   */
  vtkUnsignedCharArray* EdgeFlags;

private:
  vtkHyperTreeGridGeometry(const vtkHyperTreeGridGeometry&) = delete;
  void operator=(const vtkHyperTreeGridGeometry&) = delete;
};

#endif /* vtkHyperTreeGridGeometry_h */
