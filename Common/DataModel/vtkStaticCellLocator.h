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
  void PrintSelf(ostream& os, vtkIndent indent) override;
  //@}

  //@{
  /**
   * Set the number of divisions in x-y-z directions. If the Automatic data
   * member is enabled, the Divisions are set according to the
   * NumberOfCellsPerNode and MaxNumberOfBuckets data members. The number
   * of divisions must be >= 1 in each direction.
   */
  vtkSetVector3Macro(Divisions,int);
  vtkGetVectorMacro(Divisions,int,3);
  //@}

  /**
   * Test a point to find if it is inside a cell. Returns the cellId if inside
   * or -1 if not.
   */
  vtkIdType FindCell(double pos[3], double vtkNotUsed, vtkGenericCell *cell,
                     double pcoords[3], double* weights ) override;

  /**
   * Reimplemented from vtkAbstractCellLocator to support bad compilers.
   */
  vtkIdType FindCell(double x[3]) override
    { return this->Superclass::FindCell(x); }

  /**
   * Return a list of unique cell ids inside of a given bounding box. The
   * user must provide the vtkIdList to populate. This method returns data
   * only after the locator has been built.
   */
  void FindCellsWithinBounds(double *bbox, vtkIdList *cells) override;

  /**
   * Given a finite line defined by the two points (p1,p2), return the list
   * of unique cell ids in the buckets containing the line. It is possible
   * that an empty cell list is returned. The user must provide the vtkIdList
   * cell list to populate. This method returns data only after the locator
   * has been built.
   */
  void FindCellsAlongLine(const double p1[3], const double p2[3],
                          double tolerance, vtkIdList *cells) override;

  /**
   * Return intersection point (if any) AND the cell which was intersected by
   * the finite line. The cell is returned as a cell id and as a generic cell.
   */
  int IntersectWithLine(const double a0[3], const double a1[3], double tol,
                        double& t, double x[3], double pcoords[3],
                        int &subId, vtkIdType &cellId,
                        vtkGenericCell *cell) override;

  /**
   * Reimplemented from vtkAbstractCellLocator to support bad compilers.
   */
  int IntersectWithLine(const double p1[3], const double p2[3], double tol,
                        double& t, double x[3], double pcoords[3], int &subId) override
  {
    return this->Superclass::IntersectWithLine(p1, p2, tol, t, x, pcoords, subId);
  }

  /**
   * Reimplemented from vtkAbstractCellLocator to support bad compilers.
   */
  int IntersectWithLine(const double p1[3], const double p2[3], double tol,
                        double &t, double x[3], double pcoords[3],
                        int &subId, vtkIdType &cellId) override
  {
    return this->Superclass::IntersectWithLine(p1, p2, tol, t, x, pcoords, subId, cellId);
  }

  /**
   * Reimplemented from vtkAbstractCellLocator to support bad compilers.
   */
  int IntersectWithLine(const double p1[3], const double p2[3],
                        vtkPoints *points, vtkIdList *cellIds) override
  {
    return this->Superclass::IntersectWithLine(p1, p2, points, cellIds);
  }

  //@{
  /**
   * Satisfy vtkLocator abstract interface.
   */
  void GenerateRepresentation(int level, vtkPolyData *pd) override;
  void FreeSearchStructure() override;
  void BuildLocator() override;
  //@}

  //@{
  /**
   * Set the maximum number of buckets in the locator. By default the value
   * is set to VTK_INT_MAX. Note that there are significant performance
   * implications at work here. If the number of buckets is set very large
   * (meaning > VTK_INT_MAX) then internal sorting may be performed using
   * 64-bit integers (which is much slower than using a 32-bit int). Of
   * course, memory requirements may dramatically increase as well.  It is
   * recommended that the default value be used; but for extremely large data
   * it may be desired to create a locator with an exceptionally large number
   * of buckets. Note also that during initialization of the locator if the
   * MaxNumberOfBuckets threshold is exceeded, the Divisions are scaled down
   * in such a way as not to exceed the MaxNumberOfBuckets proportionally to
   * the size of the bounding box in the x-y-z directions.
   */
  vtkSetClampMacro(MaxNumberOfBuckets,vtkIdType,1000,VTK_ID_MAX);
  vtkGetMacro(MaxNumberOfBuckets,vtkIdType);
  //@}

  /**
   * Inform the user as to whether large ids are being used. This flag only
   * has meaning after the locator has been built. Large ids are used when the
   * number of binned points, or the number of bins, is >= the maximum number
   * of buckets (specified by the user). Note that LargeIds are only available
   * on 64-bit architectures.
   */
  bool GetLargeIds() {return this->LargeIds;}

protected:
  vtkStaticCellLocator();
  ~vtkStaticCellLocator() override;

  double Bounds[6]; // Bounding box of the whole dataset
  int Divisions[3]; // Number of sub-divisions in x-y-z directions
  double H[3]; // Width of each bin in x-y-z directions

  vtkIdType MaxNumberOfBuckets; // Maximum number of buckets in locator
  bool LargeIds; //indicate whether integer ids are small or large

  // Support PIMPLd implementation
  vtkCellBinner *Binner; // Does the binning
  vtkCellProcessor *Processor; // Invokes methods (templated subclasses)

  // Support query operations
  unsigned char *CellHasBeenVisited;
  unsigned char QueryNumber;

private:
  vtkStaticCellLocator(const vtkStaticCellLocator&) = delete;
  void operator=(const vtkStaticCellLocator&) = delete;
};

#endif
