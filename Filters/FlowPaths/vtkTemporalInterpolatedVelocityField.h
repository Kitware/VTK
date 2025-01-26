// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkTemporalInterpolatedVelocityField
 * @brief   A helper class for interpolating between times during particle tracing
 *
 * vtkTemporalInterpolatedVelocityField is a general purpose
 * helper for the temporal particle tracing code (vtkParticleTracerBase)
 *
 * It maintains two copies of vtkCompositeInterpolatedVelocityField internally
 * and uses them to obtain velocity values at time T0 and T1.
 *
 * In fact the class does quite a bit more than this because when the geometry
 * of the datasets is the same at T0 and T1, we can reuse cached cell Ids and
 * weights used in the cell interpolation routines.
 * Additionally, the same weights can be used when interpolating (point) scalar
 * values and computing vorticity etc.
 *
 * @warning
 * vtkTemporalInterpolatedVelocityField is not thread safe.
 * A new instance should be created by each thread.
 *
 * @warning
 * Datasets are added in lists. The list for T1 must be identical to that for T0
 * in structure/topology and dataset order, and any datasets marked as static,
 * must remain so for all T - changing a dataset from static to dynamic
 * between time steps will result in undefined behaviour (=crash probably)
 *
 *
 * @sa
 * vtkCompositeInterpolatedVelocityField vtkParticleTracerBase
 * vtkParticleTracer vtkParticlePathFilter vtkStreaklineFilter
 */

#ifndef vtkTemporalInterpolatedVelocityField_h
#define vtkTemporalInterpolatedVelocityField_h

#include "vtkFiltersFlowPathsModule.h" // For export macro
#include "vtkFunctionSet.h"
#include "vtkSmartPointer.h" // For vtkSmartPointer

#include <vector> // For internal structures

VTK_ABI_NAMESPACE_BEGIN
class vtkAbstractCellLinks;
class vtkCompositeDataSet;
class vtkCompositeInterpolatedVelocityField;
class vtkDataArray;
class vtkDataSet;
class vtkDoubleArray;
class vtkFindCellStrategy;
class vtkGenericCell;
class vtkLocator;
class vtkPointData;

class VTKFILTERSFLOWPATHS_EXPORT vtkTemporalInterpolatedVelocityField : public vtkFunctionSet
{
public:
  vtkTypeMacro(vtkTemporalInterpolatedVelocityField, vtkFunctionSet);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Construct a vtkTemporalInterpolatedVelocityField with no initial data set.
   * Caching is on. LastCellId is set to -1.
   */
  static vtkTemporalInterpolatedVelocityField* New();

  /**
   * States that define where the cell id are
   */
  enum IDStates
  {
    INSIDE_ALL = 0,
    OUTSIDE_ALL = 1,
    OUTSIDE_T0 = 2,
    OUTSIDE_T1 = 3
  };

  /**
   * Types of Variance of Mesh over time
   */
  enum MeshOverTimeTypes
  {
    DIFFERENT = 0,
    STATIC = 1,
    LINEAR_TRANSFORMATION = 2,
    SAME_TOPOLOGY = 3
  };

  ///@{
  /*
   * Set/Get the type of variance of the mesh over time.
   *
   * DIFFERENT = 0,
   * STATIC = 1,
   * LINEAR_TRANSFORMATION = 2
   * SAME_TOPOLOGY = 3
   */
  vtkSetClampMacro(MeshOverTime, int, DIFFERENT, SAME_TOPOLOGY);
  void SetMeshOverTimeToDifferent() { this->SetMeshOverTime(DIFFERENT); }
  void SetMeshOverTimeToStatic() { this->SetMeshOverTime(STATIC); }
  void SetMeshOverTimeToLinearTransformation() { this->SetMeshOverTime(LINEAR_TRANSFORMATION); }
  void SetMeshOverTimeToSameTopology() { this->SetMeshOverTime(SAME_TOPOLOGY); }
  vtkGetMacro(MeshOverTime, int);
  ///@}

  /**
   * The Initialize() method is used to build and cache supporting structures
   * (such as locators) which are used when operating on the interpolated
   * velocity field. This method is needed mainly to deal with thread safety
   * issues; i.e., these supporting structures must be built at the right
   * time to avoid race conditions.
   */
  void Initialize(vtkCompositeDataSet* t0, vtkCompositeDataSet* t1);

  /**
   * Copy essential parameters between instances of this class. This
   * generally is used to copy from instance prototype to another, or to copy
   * interpolators between thread instances.  Sub-classes can contribute to
   * the parameter copying process via chaining.
   */
  virtual void CopyParameters(vtkTemporalInterpolatedVelocityField* from);

  using Superclass::FunctionValues;
  ///@{
  /**
   * Evaluate the velocity field, f, at (x, y, z, t).
   * For now, t is ignored.
   */
  int FunctionValues(double* x, double* u) override;
  int FunctionValuesAtT(int T, double* x, double* u);
  ///@}

  /**
   * If you want to work with an arbitrary vector array, then set its name
   * here. By default this is nullptr and the filter will use the active vector
   * array.
   */
  void SelectVectors(const char* fieldName) { this->SetVectorsSelection(fieldName); }

  ///@{
  /**
   * In order to use this class, two sets of data must be supplied,
   * corresponding to times T1 and T2. Data is added via
   * this function.
   */
  void AddDataSetAtTime(int N, double T, vtkDataSet* dataset);
  ///@}

  ///@{
  /**
   * Between iterations of the Particle Tracer, Id's of the Cell
   * are stored and then at the start of the next particle the
   * Ids are set to 'pre-fill' the cache.
   */
  bool GetCachedCellIds(vtkIdType id[2], int ds[2]);
  void SetCachedCellIds(vtkIdType id[2], int ds[2]);
  ///@}

  /**
   * Set the last cell id to -1 so that the next search does not
   * start from the previous cell
   */
  void ClearCache();

  ///@{
  /**
   * A utility function which evaluates the point at T1, T2 to see
   * if it is inside the data at both times or only one.
   */
  int TestPoint(double* x);
  int QuickTestPoint(double* x);
  ///@}

  ///@{
  /**
   * If an interpolation was successful, we can retrieve the last computed
   * value from here. Initial value is (0.0,0.0,0.0)
   */
  vtkGetVector3Macro(LastGoodVelocity, double);
  ///@}

  ///@{
  /**
   * Get the most recent weight between 0->1 from T1->T2. Initial value is 0.
   */
  vtkGetMacro(CurrentWeight, double);
  ///@}

  bool InterpolatePoint(vtkPointData* outPD1, vtkPointData* outPD2, vtkIdType outIndex);

  bool InterpolatePoint(int T, vtkPointData* outPD1, vtkIdType outIndex);

  bool GetVorticityData(
    int T, double pcoords[3], double* weights, vtkGenericCell*& cell, vtkDoubleArray* cellVectors);

  void ShowCacheResults();

  void AdvanceOneTimeStep();

  ///@{
  /**
   * Set / get the strategy used to perform the FindCell() operation. This
   * strategy is used when operating on vtkPointSet subclasses. Note if the
   * input is a composite dataset then the strategy will be used to clone
   * one strategy per leaf dataset.
   */
  virtual void SetFindCellStrategy(vtkFindCellStrategy*);
  vtkGetObjectMacro(FindCellStrategy, vtkFindCellStrategy);
  ///@}

protected:
  vtkTemporalInterpolatedVelocityField();
  ~vtkTemporalInterpolatedVelocityField() override;

  virtual void SetVectorsSelection(const char* v);

  int MeshOverTime = MeshOverTimeTypes::DIFFERENT;

  void InitializeWithLocators(vtkCompositeInterpolatedVelocityField* ivf,
    const std::vector<vtkDataSet*>& datasets, vtkFindCellStrategy* strategy,
    const std::vector<vtkSmartPointer<vtkLocator>>& locators,
    const std::vector<vtkSmartPointer<vtkAbstractCellLinks>>& links);

  void CreateLocators(const std::vector<vtkDataSet*>& datasets, vtkFindCellStrategy* strategy,
    std::vector<vtkSmartPointer<vtkLocator>>& locators);
  void CreateLinks(const std::vector<vtkDataSet*>& datasets,
    std::vector<vtkSmartPointer<vtkAbstractCellLinks>>& links);
  void CreateLinearTransformCellLocators(const std::vector<vtkSmartPointer<vtkLocator>>& locators,
    std::vector<vtkSmartPointer<vtkLocator>>& linearCellLocators);

  double Vals1[3];
  double Vals2[3];
  double Times[2];
  double LastGoodVelocity[3];

  static const double WEIGHT_TO_TOLERANCE;

  // The weight (0.0->1.0) of the value of T between the two available
  // time values for the current computation
  double CurrentWeight;
  // One minus the CurrentWeight
  double OneMinusWeight;
  // A scaling factor used when calculating the CurrentWeight { 1.0/(T2-T1) }
  double ScaleCoeff;

  vtkSmartPointer<vtkCompositeInterpolatedVelocityField> IVF[2];
  std::vector<vtkSmartPointer<vtkLocator>> Locators[2];
  std::vector<vtkSmartPointer<vtkLocator>> InitialCellLocators;
  std::vector<vtkSmartPointer<vtkAbstractCellLinks>> Links[2];
  std::vector<size_t> MaxCellSizes[2];

  vtkFindCellStrategy* FindCellStrategy;

private:
  // Hide this since we need multiple time steps and are using a different
  // function prototype
  virtual void AddDataSet(vtkDataSet*) {}

  vtkTemporalInterpolatedVelocityField(const vtkTemporalInterpolatedVelocityField&) = delete;
  void operator=(const vtkTemporalInterpolatedVelocityField&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
