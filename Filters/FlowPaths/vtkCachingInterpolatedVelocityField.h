/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCachingInterpolatedVelocityField.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkCachingInterpolatedVelocityField
 * @brief   Interface for obtaining
 * interpolated velocity values
 *
 * vtkCachingInterpolatedVelocityField acts as a continuous velocity field
 * by performing cell interpolation on the underlying vtkDataSet.
 * This is a concrete sub-class of vtkFunctionSet with
 * NumberOfIndependentVariables = 4 (x,y,z,t) and
 * NumberOfFunctions = 3 (u,v,w). Normally, every time an evaluation
 * is performed, the cell which contains the point (x,y,z) has to
 * be found by calling FindCell. This is a computationally expensive
 * operation. In certain cases, the cell search can be avoided or shortened
 * by providing a guess for the cell id. For example, in streamline
 * integration, the next evaluation is usually in the same or a neighbour
 * cell. For this reason, vtkCachingInterpolatedVelocityField stores the last
 * cell id. If caching is turned on, it uses this id as the starting point.
 *
 * @warning
 * vtkCachingInterpolatedVelocityField is not thread safe. A new instance should
 * be created by each thread.
 *
 * @sa
 * vtkFunctionSet vtkStreamTracer
 *
 * @todo
 * Need to clean up style to match vtk/Kitware standards. Please help.
*/

#ifndef vtkCachingInterpolatedVelocityField_h
#define vtkCachingInterpolatedVelocityField_h

#include "vtkFiltersFlowPathsModule.h" // For export macro
#include "vtkFunctionSet.h"
#include "vtkSmartPointer.h" // this is allowed

#include <vector> // we need them

class vtkDataSet;
class vtkDataArray;
class vtkPointData;
class vtkGenericCell;
class vtkAbstractCellLocator;

//---------------------------------------------------------------------------
class IVFDataSetInfo;
//---------------------------------------------------------------------------
class IVFCacheList : public std::vector< IVFDataSetInfo > {};
//---------------------------------------------------------------------------

class VTKFILTERSFLOWPATHS_EXPORT vtkCachingInterpolatedVelocityField : public vtkFunctionSet
{
public:
  vtkTypeMacro(vtkCachingInterpolatedVelocityField,vtkFunctionSet);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Construct a vtkCachingInterpolatedVelocityField with no initial data set.
   * LastCellId is set to -1.
   */
  static vtkCachingInterpolatedVelocityField *New();

  //@{
  /**
   * Evaluate the velocity field, f={u,v,w}, at {x, y, z}.
   * returns 1 if valid, 0 if test failed
   */
  virtual int FunctionValues(double* x, double* f);
  virtual int InsideTest(double* x);
  //@}

  /**
   * Add a dataset used by the interpolation function evaluation.
   */
  virtual void SetDataSet(int I, vtkDataSet* dataset, bool staticdataset, vtkAbstractCellLocator *locator);

  //@{
  /**
   * If you want to work with an arbitrary vector array, then set its name
   * here. By default this in NULL and the filter will use the active vector
   * array.
   */
  vtkGetStringMacro(VectorsSelection);
  void SelectVectors(const char *fieldName)
    {this->SetVectorsSelection(fieldName);}
  //@}

  /**
   * Set LastCellId to c and LastCacheIndex datasetindex, cached from last evaluation.
   * If c isn't -1 then the corresponding cell is stored in Cache->Cell.
   * These values should be valid or an assertion will be triggered.
   */
  void SetLastCellInfo(vtkIdType c, int datasetindex);

  /**
   * Set LastCellId to -1 and Cache to NULL so that the next
   * search does not  start from the previous cell.
   */
  void ClearLastCellInfo();

  //@{
  /**
   * Returns the interpolation weights/pcoords cached from last evaluation
   * if the cached cell is valid (returns 1). Otherwise, it does not
   * change w and returns 0.
   */
  int GetLastWeights(double* w);
  int GetLastLocalCoordinates(double pcoords[3]);
  //@}

  //@{
  /**
   * Caching statistics.
   */
  vtkGetMacro(CellCacheHit, int);
  vtkGetMacro(DataSetCacheHit, int);
  vtkGetMacro(CacheMiss, int);
  //@}

protected:
  vtkCachingInterpolatedVelocityField();
 ~vtkCachingInterpolatedVelocityField();

  vtkGenericCell          *TempCell;
  int                      CellCacheHit;
  int                      DataSetCacheHit;
  int                      CacheMiss;
  int                      LastCacheIndex;
  int                      LastCellId;
  IVFDataSetInfo          *Cache;
  IVFCacheList          CacheList;
  char                    *VectorsSelection;

  std::vector<double>   Weights;

  vtkSetStringMacro(VectorsSelection);

  // private versions which work on the passed dataset/cache
  // these do the real computation
  int FunctionValues(IVFDataSetInfo *cache, double *x, double *f);
  int InsideTest(IVFDataSetInfo *cache, double* x);

  friend class vtkTemporalInterpolatedVelocityField;
  //@{
  /**
   * If all weights have been computed (parametric coords etc all valid)
   * then we can quickly interpolate a scalar/vector using the known weights
   * and the generic cell which has been stored.
   * This function is primarily reserved for use by
   * vtkTemporalInterpolatedVelocityField
   */
  void FastCompute(IVFDataSetInfo *cache, double f[3]);
  bool InterpolatePoint(vtkPointData *outPD, vtkIdType outIndex);
  bool InterpolatePoint(vtkCachingInterpolatedVelocityField *inCIVF,
                        vtkPointData *outPD, vtkIdType outIndex);
  vtkGenericCell *GetLastCell();
  //@}

private:
  vtkCachingInterpolatedVelocityField(const vtkCachingInterpolatedVelocityField&) VTK_DELETE_FUNCTION;
  void operator=(const vtkCachingInterpolatedVelocityField&) VTK_DELETE_FUNCTION;
};

//---------------------------------------------------------------------------

///////////////////////////////////////////////////////////////////////////////
// IVFDataSetInfo
///////////////////////////////////////////////////////////////////////////////
#ifndef DOXYGEN_SHOULD_SKIP_THIS
//

//
class IVFDataSetInfo
{
public:
  vtkSmartPointer<vtkDataSet>             DataSet;
  vtkSmartPointer<vtkAbstractCellLocator> BSPTree;
  vtkSmartPointer<vtkGenericCell>         Cell;
  double                                  PCoords[3];
  float                                  *VelocityFloat;
  double                                 *VelocityDouble;
  double                                  Tolerance;
  bool                                    StaticDataSet;
  IVFDataSetInfo();
  IVFDataSetInfo(const IVFDataSetInfo &ivfci);
  IVFDataSetInfo &operator=(const IVFDataSetInfo &ivfci);
  void SetDataSet(vtkDataSet *data, char *velocity, bool staticdataset, vtkAbstractCellLocator *locator);
  //
  static const double TOLERANCE_SCALE;
};

//

//

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#endif
