/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStaticCellLocator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkStaticCellLocator
 * @brief   perform fast cell location operations
 *
 * vtkStaticCellLocator is a type of vtkAbstractCellLocator that accelerates
 * certain operations when performing spatial operations on cells. These
 * operations include finding a point that contains a cell, and intersecting
 * cells with a line.
 *
 * vtkStaticCellLocator is an accelerated version of vtkCellLocator. It is
 * threaded (via vtkSMPTools), and supports one-time static construction
 * (i.e., incremental cell insertion is not supported).
 *
 * @warning
 * This class is templated. It may run slower than serial execution if the code
 * is not optimized during compilation. Build in Release or ReleaseWithDebugInfo.
 *
 * @warning
 * This class *always* caches cell bounds.
 *
 * @sa
 * vtkLocator vakAbstractCellLocator vtkCellLocator vtkCellTreeLocator
 * vtkModifiedBSPTree
 */


#ifndef vtkStaticCellLocator_h
#define vtkStaticCellLocator_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkAbstractCellLocator.h"


// Forward declarations for PIMPL
struct vtkCellBinner;
struct vtkCellProcessor;

class VTKCOMMONDATAMODEL_EXPORT vtkStaticCellLocator : public vtkAbstractCellLocator
{
friend struct vtkCellBinner;
friend struct vtkCellProcessor;
public:
  //@{
  /**
   * Standard methods to instantiate, print and obtain type-related information.
   */
  static vtkStaticCellLocator *New();
  vtkTypeMacro(vtkStaticCellLocator,vtkAbstractCellLocator);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;
  //@}

  //@{
  /**
   * Set the number of divisions in x-y-z directions. If the
   * vtkLocator::Automatic data member is enabled, the Divisions are set
   * according to the vtkAbstractCellLocator::NumberOfCellsPerNode data
   * member.
   */
  vtkSetVector3Macro(Divisions,int);
  vtkGetVectorMacro(Divisions,int,3);
  //@}

  /**
   * Test a point to find if it is inside a cell. Returns the cellId if inside
   * or -1 if not.
   */
  vtkIdType FindCell(double pos[3], double vtkNotUsed, vtkGenericCell *cell,
                     double pcoords[3], double* weights ) VTK_OVERRIDE;

  /**
   * Reimplemented from vtkAbstractCellLocator to support bad compilers.
   */
  vtkIdType FindCell(double x[3]) VTK_OVERRIDE
    { return this->Superclass::FindCell(x); }

  /**
   * Return a list of unique cell ids inside of a given bounding box. The
   * user must provide the vtkIdList to populate. This method returns data
   * only after the locator has been built.
   */
  void FindCellsWithinBounds(double *bbox, vtkIdList *cells) VTK_OVERRIDE;

  /**
   * Return intersection point (if any) AND the cell which was intersected by
   * the finite line. The cell is returned as a cell id and as a generic cell.
   */
  int IntersectWithLine(double a0[3], double a1[3], double tol,
                        double& t, double x[3], double pcoords[3],
                        int &subId, vtkIdType &cellId,
                        vtkGenericCell *cell) VTK_OVERRIDE;

  /**
   * Reimplemented from vtkAbstractCellLocator to support bad compilers.
   */
  int IntersectWithLine(double p1[3], double p2[3], double tol,
                        double& t, double x[3], double pcoords[3], int &subId) VTK_OVERRIDE
  {
    return this->Superclass::IntersectWithLine(p1, p2, tol, t, x, pcoords, subId);
  }

  /**
   * Reimplemented from vtkAbstractCellLocator to support bad compilers.
   */
  int IntersectWithLine(double p1[3], double p2[3], double tol,
                        double &t, double x[3], double pcoords[3],
                        int &subId, vtkIdType &cellId) VTK_OVERRIDE
  {
    return this->Superclass::IntersectWithLine(p1, p2, tol, t, x, pcoords, subId, cellId);
  }

  /**
   * Reimplemented from vtkAbstractCellLocator to support bad compilers.
   */
  int IntersectWithLine(const double p1[3], const double p2[3],
                        vtkPoints *points, vtkIdList *cellIds) VTK_OVERRIDE
  {
    return this->Superclass::IntersectWithLine(p1, p2, points, cellIds);
  }

  //@{
  /**
   * Satisfy vtkLocator abstract interface.
   */
  void GenerateRepresentation(int level, vtkPolyData *pd) VTK_OVERRIDE;
  void FreeSearchStructure() VTK_OVERRIDE;
  void BuildLocator() VTK_OVERRIDE;
  //@}

protected:
  vtkStaticCellLocator();
  ~vtkStaticCellLocator() VTK_OVERRIDE;

  double Bounds[6]; // Bounding box of the whole dataset
  int Divisions[3]; // Number of sub-divisions in x-y-z directions
  double H[3]; // Width of each bin in x-y-z directions

  // Support PIMPLd implementation
  vtkCellBinner *Binner; // Does the binning
  vtkCellProcessor *Processor; // Invokes methods (templated subclasses)

  // Support query operations
  unsigned char *CellHasBeenVisited;
  unsigned char QueryNumber;

private:
  vtkStaticCellLocator(const vtkStaticCellLocator&) VTK_DELETE_FUNCTION;
  void operator=(const vtkStaticCellLocator&) VTK_DELETE_FUNCTION;
};

#endif
