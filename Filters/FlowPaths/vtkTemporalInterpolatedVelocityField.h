/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTemporalInterpolatedVelocityField.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkTemporalInterpolatedVelocityField
 * @brief   A helper class for
 * interpolating between times during particle tracing
 *
 * vtkTemporalInterpolatedVelocityField is a general purpose
 * helper for the temporal particle tracing code (vtkTemporalStreamTracer)
 *
 * It maintains two copies of vtkCachingInterpolatedVelocityField internally
 * and uses them to obtain velocity values at time T0 and T1.
 *
 * In fact the class does quite a bit more than this because when the geometry
 * of the datasets is the same at T0 and T1, we can re-use cached cell Ids and
 * weights used in the cell interpolation routines.
 * Additionally, the same weights can be used when interpolating (point) scalar
 * values and computing vorticity etc.
 *
 * @warning
 * vtkTemporalInterpolatedVelocityField is probably not thread safe.
 * A new instance should be created by each thread.
 *
 * @warning
 * Datasets are added in lists. The list for T1 must be idential to that for T0
 * in structure/topology and dataset order, and any datasets marked as static,
 * must remain so for all T - changing a dataset from static to dynamic
 * between time steps will result in undefined behaviour (=crash probably)
 *
 *
 * @sa
 * vtkCachingInterpolatedVelocityField vtkTemporalStreamTracer
*/

#ifndef vtkTemporalInterpolatedVelocityField_h
#define vtkTemporalInterpolatedVelocityField_h

#include "vtkFiltersFlowPathsModule.h" // For export macro
#include "vtkFunctionSet.h"
#include "vtkSmartPointer.h" // because it is good

#include <vector> // Because they are good

#define ID_INSIDE_ALL  00
#define ID_OUTSIDE_ALL 01
#define ID_OUTSIDE_T0  02
#define ID_OUTSIDE_T1  03

class vtkDataSet;
class vtkDataArray;
class vtkPointData;
class vtkGenericCell;
class vtkDoubleArray;
class vtkCachingInterpolatedVelocityField;

class VTKFILTERSFLOWPATHS_EXPORT vtkTemporalInterpolatedVelocityField : public vtkFunctionSet
{
public:
  vtkTypeMacro(vtkTemporalInterpolatedVelocityField,vtkFunctionSet);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Construct a vtkTemporalInterpolatedVelocityField with no initial data set.
   * Caching is on. LastCellId is set to -1.
   */
  static vtkTemporalInterpolatedVelocityField *New();

  //@{
  /**
   * Evaluate the velocity field, f, at (x, y, z, t).
   * For now, t is ignored.
   */
  virtual int FunctionValues(double* x, double* u);
  int FunctionValuesAtT(int T, double* x, double* u);
  //@}

  /**
   * If you want to work with an arbitrary vector array, then set its name
   * here. By default this is NULL and the filter will use the active vector
   * array.
   */
  void SelectVectors(const char *fieldName)
    {this->SetVectorsSelection(fieldName);}

  /**
   * In order to use this class, two sets of data must be supplied,
   * corresponding to times T1 and T2. Data is added via
   * this function.
   */
  void SetDataSetAtTime(int I, int N, double T, vtkDataSet* dataset, bool staticdataset);

  //@{
  /**
   * Between iterations of the Particle Tracer, Id's of the Cell
   * are stored and then at the start of the next particle the
   * Ids are set to 'pre-fill' the cache.
   */
  bool GetCachedCellIds(vtkIdType id[2], int ds[2]);
  void SetCachedCellIds(vtkIdType id[2], int ds[2]);
  //@}

  /**
   * Set the last cell id to -1 so that the next search does not
   * start from the previous cell
   */
  void ClearCache();

  //@{
  /**
   * A utility function which evaluates the point at T1, T2 to see
   * if it is inside the data at both times or only one.
   */
  int TestPoint(double* x);
  int QuickTestPoint(double* x);
  //@}

  //@{
  /**
   * If an interpolation was successful, we can retrieve the last computed
   * value from here. Initial value is (0.0,0.0,0.0)
   */
  vtkGetVector3Macro(LastGoodVelocity,double);
  //@}

  //@{
  /**
   * Get the most recent weight between 0->1 from T1->T2. Initial value is 0.
   */
  vtkGetMacro(CurrentWeight,double);
  //@}

  bool InterpolatePoint(vtkPointData *outPD1,
    vtkPointData *outPD2, vtkIdType outIndex);

  bool InterpolatePoint(int T, vtkPointData *outPD1, vtkIdType outIndex);

  bool GetVorticityData(
    int T, double pcoords[3], double *weights,
    vtkGenericCell *&cell, vtkDoubleArray *cellVectors);

  void ShowCacheResults();
  bool IsStatic(int datasetIndex);

  void AdvanceOneTimeStep();

protected:
  vtkTemporalInterpolatedVelocityField();
  ~vtkTemporalInterpolatedVelocityField();

  int FunctionValues(vtkDataSet* ds, double* x, double* f);
  virtual void SetVectorsSelection(const char *v);

  double Vals1[3];
  double Vals2[3];
  double Times[2];
  double LastGoodVelocity[3];

  // The weight (0.0->1.0) of the value of T between the two avaiable
  // time values for the current computation
  double CurrentWeight;
  // One minus the CurrentWeight
  double OneMinusWeight;
  // A scaling factor used when calculating the CurrentWeight { 1.0/(T2-T1) }
  double ScaleCoeff;

  vtkSmartPointer<vtkCachingInterpolatedVelocityField> IVF[2];
  // we want to keep track of static datasets so we can optimize caching
  std::vector<bool> StaticDataSets;

private:
  // Hide this since we need multiple time steps and are using a different
  // function prototype
  virtual void AddDataSet(vtkDataSet*) {}

private:
  vtkTemporalInterpolatedVelocityField(const vtkTemporalInterpolatedVelocityField&) VTK_DELETE_FUNCTION;
  void operator=(const vtkTemporalInterpolatedVelocityField&) VTK_DELETE_FUNCTION;
};

#endif
