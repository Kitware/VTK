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
// .NAME vtkCachingInterpolatedVelocityField - Interface for obtaining
// interpolated velocity values
// .SECTION Description
// vtkCachingInterpolatedVelocityField acts as a continuous velocity field
// by performing cell interpolation on the underlying vtkDataSet.
// This is a concrete sub-class of vtkFunctionSet with
// NumberOfIndependentVariables = 4 (x,y,z,t) and
// NumberOfFunctions = 3 (u,v,w). Normally, every time an evaluation
// is performed, the cell which contains the point (x,y,z) has to
// be found by calling FindCell. This is a computationally expensive
// operation. In certain cases, the cell search can be avoided or shortened
// by providing a guess for the cell id. For example, in streamline
// integration, the next evaluation is usually in the same or a neighbour
// cell. For this reason, vtkCachingInterpolatedVelocityField stores the last
// cell id. If caching is turned on, it uses this id as the starting point.

// .SECTION Caveats
// vtkCachingInterpolatedVelocityField is not thread safe. A new instance should
// be created by each thread.

// .SECTION See Also
// vtkFunctionSet vtkStreamer

// .SECTION TODO
// Need to clean up style to match vtk/Kitware standards. Please help.

#ifndef vtkTInterpolatedVelocityField_h
#define vtkTInterpolatedVelocityField_h

#include "vtkFiltersFlowPathsModule.h" // For export macro
#include "vtkFunctionSet.h"
#include "vtkSmartPointer.h" // this is allowed
//BTX
#include <vector> // we need them
//ETX

class vtkDataSet;
class vtkDataArray;
class vtkPointData;
class vtkGenericCell;
class vtkAbstractCellLocator;
//BTX
//---------------------------------------------------------------------------
class IVFDataSetInfo;
//---------------------------------------------------------------------------
class IVFCacheList : public std::vector< IVFDataSetInfo > {};
//---------------------------------------------------------------------------
//ETX
class VTKFILTERSFLOWPATHS_EXPORT vtkCachingInterpolatedVelocityField : public vtkFunctionSet
{
public:
  vtkTypeMacro(vtkCachingInterpolatedVelocityField,vtkFunctionSet);
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct a vtkCachingInterpolatedVelocityField with no initial data set.
  // LastCellId is set to -1.
  static vtkCachingInterpolatedVelocityField *New();

  // Description:
  // Evaluate the velocity field, f={u,v,w}, at {x, y, z}.
  // returns 1 if valid, 0 if test failed
  virtual int FunctionValues(double* x, double* f);
  virtual int InsideTest(double* x);

  // Description:
  // Add a dataset used by the interpolation function evaluation.
  virtual void SetDataSet(int I, vtkDataSet* dataset, bool staticdataset, vtkAbstractCellLocator *locator);

  // Description:
  // If you want to work with an arbitrary vector array, then set its name
  // here. By default this in NULL and the filter will use the active vector
  // array.
  vtkGetStringMacro(VectorsSelection);
  void SelectVectors(const char *fieldName)
    {this->SetVectorsSelection(fieldName);}

  // Description:
  // Return the cell id cached from last evaluation.
  void SetLastCellInfo(vtkIdType c, int datasetindex);

  // Description:
  // Set the last cell id to -1 so that the next search does not
  // start from the previous cell
  void ClearLastCellInfo();

  // Description:
  // Returns the interpolation weights/pcoords cached from last evaluation
  // if the cached cell is valid (returns 1). Otherwise, it does not
  // change w and returns 0.
  int GetLastWeights(double* w);
  int GetLastLocalCoordinates(double pcoords[3]);

  // Description:
  // Caching statistics.
  vtkGetMacro(CellCacheHit, int);
  vtkGetMacro(DataSetCacheHit, int);
  vtkGetMacro(CacheMiss, int);

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
//BTX
  std::vector<double>   Weights;
//ETX

  vtkSetStringMacro(VectorsSelection);

  // private versions which work on the passed dataset/cache
  // these do the real computation
  int FunctionValues(IVFDataSetInfo *cache, double *x, double *f);
  int InsideTest(IVFDataSetInfo *cache, double* x);

//BTX
  friend class vtkTemporalInterpolatedVelocityField;
  // Description:
  // If all weights have been computed (parametric coords etc all valid)
  // then we can quickly interpolate a scalar/vector using the known weights
  // and the generic cell which has been stored.
  // This function is primarily reserved for use by
  // vtkTemporalInterpolatedVelocityField
  void FastCompute(IVFDataSetInfo *cache, double f[3]);
  bool InterpolatePoint(vtkPointData *outPD, vtkIdType outIndex);
  bool InterpolatePoint(vtkCachingInterpolatedVelocityField *inCIVF,
                        vtkPointData *outPD, vtkIdType outIndex);
  vtkGenericCell *GetLastCell();
//ETX

private:
  vtkCachingInterpolatedVelocityField(const vtkCachingInterpolatedVelocityField&);  // Not implemented.
  void operator=(const vtkCachingInterpolatedVelocityField&);  // Not implemented.
};

//---------------------------------------------------------------------------

///////////////////////////////////////////////////////////////////////////////
// IVFDataSetInfo
///////////////////////////////////////////////////////////////////////////////
#ifndef DOXYGEN_SHOULD_SKIP_THIS
//
//BTX
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
//ETX
//

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#endif
