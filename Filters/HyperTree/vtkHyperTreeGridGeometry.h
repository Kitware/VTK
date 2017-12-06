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
 * This class was written by Philippe Pebay, Joachim Pouderoux,
 * and Charles Law, Kitware 2013
 * This class was modified by Guenole Harel and Jacques-Bernard Lekien, 2014
 * This class was rewritten by Philippe Pebay, NexGen Analytics 2017
 * This work was supported by Commissariat a l'Energie Atomique (CEA/DIF)
*/

#ifndef vtkHyperTreeGridGeometry_h
#define vtkHyperTreeGridGeometry_h

#include "vtkFiltersHyperTreeModule.h" // For export macro
#include "vtkHyperTreeGridAlgorithm.h"

class vtkBitArray;
class vtkCellArray;
class vtkDoubleArray;
class vtkHyperTreeGrid;
class vtkHyperTreeGridCursor;
class vtkIdList;
class vtkIdTypeArray;
class vtkPoints;

class VTKFILTERSHYPERTREE_EXPORT vtkHyperTreeGridGeometry : public vtkHyperTreeGridAlgorithm
{
public:
  static vtkHyperTreeGridGeometry* New();
  vtkTypeMacro( vtkHyperTreeGridGeometry, vtkHyperTreeGridAlgorithm );
  void PrintSelf( ostream&, vtkIndent ) override;

protected:
  vtkHyperTreeGridGeometry();
  ~vtkHyperTreeGridGeometry() override;

  /**
   * For this algorithm the output is a vtkPolyData instance
   */
  int FillOutputPortInformation( int, vtkInformation* ) override;

  /**
   * Main routine to generate external boundary
   */
  int ProcessTrees( vtkHyperTreeGrid*, vtkDataObject* ) override;

  /**
   * Recursively descend into tree down to leaves
   */
  void RecursivelyProcessTree( vtkHyperTreeGridCursor*, vtkBitArray* );

  /**
   * Process 1D leaves and issue corresponding edges (lines)
   */
  void ProcessLeaf1D( vtkHyperTreeGridCursor* );

  /**
   * Process 2D leaves and issue corresponding faces (quads)
   */
  void ProcessLeaf2D( vtkHyperTreeGridCursor*, vtkBitArray* );

  /**
   * Process 3D leaves and issue corresponding cells (voxels)
   */
  void ProcessLeaf3D( vtkHyperTreeGridCursor*, vtkBitArray* );

  /**
   * Helper method to generate a face based on its normal and offset from cursor origin
   */
  void AddFace( vtkIdType, double*, double*, int, unsigned int, bool create = true );

  /**
   * Dimension of input grid
   */
  unsigned int Dimension;

  /**
   * Orientation of input grid when dimension < 3
   */
  unsigned int Orientation;

  /**
   * Storage for points of output unstructured mesh
   */
  vtkPoints* Points;

  /**
   * Storage for cells of output unstructured mesh
   */
  vtkCellArray* Cells;

  //@{
  /**
   * Keep track of input interface parameters
   */
  bool HasInterface;
  vtkDoubleArray* Normals;
  vtkDoubleArray* Intercepts;
  //@}

  //@{
  /**
   * Storage for interface points
   */
  vtkPoints* FacePoints;
  vtkIdList* FaceIDs;
  //@}

  //@{
  /**
   * Storage for interface edges
   */
  vtkIdType EdgesA[12];
  vtkIdType EdgesB[12];
  //@}

  //@{
  /**
   * Storage for interface faces
   */
  vtkIdTypeArray* FacesA;
  vtkIdTypeArray* FacesB;
  //@}

  //@{
  /**
   * Storage for interface scalars
   */
  vtkDoubleArray* FaceScalarsA;
  vtkDoubleArray* FaceScalarsB;
  //@}

private:
  vtkHyperTreeGridGeometry(const vtkHyperTreeGridGeometry&) = delete;
  void operator=(const vtkHyperTreeGridGeometry&) = delete;
};

#endif /* vtkHyperTreeGridGeometry_h */
