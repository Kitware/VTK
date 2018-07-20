/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHyperTreeGridPlaneCutter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkHyperTreeGridPlaneCutter
 * @brief   cut a hyper tree grid volume with
 * a plane and generate a polygonal cut surface.
 *
 *
 * vtkHyperTreeGridPlaneCutter is a filter that takes as input a hyper tree
 * grid and a single plane and generates the polygonal data intersection surface.
 * This cut is computed at the leaf cells of the hyper tree.
 * It is left as an option to decide whether the cut should be computed over
 * the original AMR mesh or over its dual; in the latter case, perfect
 * connectivity (i.e., mesh conformity in the FE sense) is achieved at the
 * cost of interpolation to the dual of the input AMR mesh, and therefore
 * of missing intersection plane pieces near the primal boundary.
 *
 * @sa
 * vtkHyperTreeGrid vtkHyperTreeGridAlgorithm
 *
 * @par Thanks:
 * This class was written by Philippe Pebay on a idea of Guénolé Harel and Jacques-Bernard Lekien, 2016
 * This class was modified by Rogeli Grima Torres, 2016
 * This work was supported by Commissariat a l'Energie Atomique (CEA/DIF)
 *
*/

#ifndef vtkHyperTreeGridPlaneCutter_h
#define vtkHyperTreeGridPlaneCutter_h

#include "vtkFiltersHyperTreeModule.h" // For export macro
#include "vtkHyperTreeGridAlgorithm.h"

class vtkHyperTreeGridCursor;
class vtkCellArray;
class vtkCutter;
class vtkIdList;
class vtkPoints;
class vtkPlane;

class VTKFILTERSHYPERTREE_EXPORT vtkHyperTreeGridPlaneCutter : public vtkHyperTreeGridAlgorithm
{
public:
  static vtkHyperTreeGridPlaneCutter* New();
  vtkTypeMacro( vtkHyperTreeGridPlaneCutter, vtkHyperTreeGridAlgorithm );
  void PrintSelf( ostream&, vtkIndent ) override;

  //@{
  /**
   * Specify the plane with its [a,b,c,d] Cartesian coefficients:
   * a*x + b*y + c*z = d
   *
   * @note This will be overridden the vtkPlane object, if specified.
   */
  vtkSetVector4Macro(Plane,double);
  vtkGetVector4Macro(Plane,double);
  //@}

  /**
   * Set the plane by specifying a vtkPlane object. This will override the
   * plane equation if set, and may be null, in which case the equation will
   * be used.
   *
   * @note This will override plane equation state when specified.
   */
  void SetPlane(vtkPlane *plane) { this->SetPlaneObj(plane); }
  void SetPlaneObj(vtkPlane*);
  vtkGetObjectMacro(PlaneObj, vtkPlane)

  //@{
  /**
   * Set/Get whether output mesh should be computed on dual grid
   */
  vtkSetMacro(Dual,int);
  vtkGetMacro(Dual,int);
  vtkBooleanMacro(Dual,int);
  //@}

  vtkMTimeType GetMTime() override;

protected:
  vtkHyperTreeGridPlaneCutter();
  ~vtkHyperTreeGridPlaneCutter() override;

  /**
   * For this algorithm the output is a vtkPolyData instance
   */
  int FillOutputPortInformation( int, vtkInformation* ) override;

  /**
   * Top-level routine to generate plane cut
   */
  int ProcessTrees( vtkHyperTreeGrid*, vtkDataObject* ) override;

  /**
   * Recursively descend into tree down to leaves, cutting primal cells
   */
  void RecursivelyProcessTreePrimal( vtkHyperTreeGridCursor*, vtkBitArray* );

  /**
   * Recursively decide whether cell is intersected by plane
   */
  bool RecursivelyPreProcessTree( vtkHyperTreeGridCursor*, vtkBitArray* );

  /**
   * Recursively descend into tree down to leaves, cutting dual cells
   */
  void RecursivelyProcessTreeDual( vtkHyperTreeGridCursor*, vtkBitArray* );

  /**
   * Check if a cursor is intersected by a plane
   */
  bool CheckIntersection( double[8][3], double[8] );

  // Check if a cursor is intersected by a plane.
  // Don't return function evaluations
  bool CheckIntersection( double[8][3] );

  /**
   * Compute the intersection between an edge and a plane
   */
  void PlaneCut( int, int, double[8][3], int&, double[][3] );

  /**
   * Reorder cut points following the perimeter of the cut.
   */
  void ReorderCutPoints( int, double[][3] );

  /**
   * Storage for the plane cutter parameters
   */
  double Plane[4];

  /**
   * Decide whether output mesh should be a computed on dual grid
   */
  int Dual;

  /**
   * Storage for pre-selected cells to be processed in dual mode
   */
  vtkBitArray* SelectedCells;

  /**
   * Storage for points of output unstructured mesh
   */
  vtkPoints* Points;

  /**
   * Storage for cells of output unstructured mesh
   */
  vtkCellArray* Cells;

  /**
   * Storage for dual vertex indices
   */
  vtkIdList* Leaves;

  /**
   * Storage for dual vertices at center of primal cells
   */
  vtkPoints* Centers;

  /**
   * Cutter to be used on dual cells
   */
  vtkCutter* Cutter;

  /**
   * Plane object used to hold plane state.
   */
  vtkPlane *PlaneObj;

private:
  vtkHyperTreeGridPlaneCutter(const vtkHyperTreeGridPlaneCutter&) = delete;
  void operator=(const vtkHyperTreeGridPlaneCutter&) = delete;
};

#endif /* vtkHyperTreeGridPlaneCutter_h */
