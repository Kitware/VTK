// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkAbstractInterpolatedVelocityField
 * @brief   An abstract class for
 *  obtaining the interpolated velocity values at a point
 *
 *
 *  vtkAbstractInterpolatedVelocityField acts as a continuous velocity field
 *  by performing cell interpolation on the underlying vtkDataSet (or in the
 *  case of vtkCompositeInterpolatedVelocityField,
 *  vtkCompositeDataSets). This is an abstract sub-class of vtkFunctionSet,
 *  NumberOfIndependentVariables = 4 (x,y,z,t) and NumberOfFunctions = 3
 *  (u,v,w). With a brute-force scheme, every time an evaluation is
 *  performed, the target cell containing point (x,y,z) needs to be found by
 *  calling FindCell(); however vtkAbstractInterpolatedVelocityField uses
 *  locators to accelerate this operation via an instance of
 *  vtkFindCellStrategy. Even with the use of locators, the cost of the find
 *  cell operation can be large, hence this class performs local caching to
 *  reduce the number of invocations of FindCell(). As a result, this class
 *  is not thread safe as it contains local state (such as the cache
 *  information). Writing a threaded operations requires separate instances of
 *  vtkAbstractInterpolatedVelocityField for each thread.
 *
 *  For vtkCompositeInterpolatedVelocityField with CLOSEST_POINT strategy,
 *  level #0 begins with intra-cell caching.
 *  Specifically if the previous cell is valid and the next point is still in
 *  it ( i.e., vtkCell::EvaluatePosition() returns 1, coupled with newly created
 *  parametric coordinates & weights ), the function values can be interpolated
 *  and only vtkCell::EvaluatePosition() is invoked. If this fails, then level #1
 *  follows by inter-cell search for the target cell that contains the next point.
 *  By an inter-cell search, the previous cell provides an important clue or serves
 *  as an immediate neighbor to aid in locating the target cell via vtkPointSet::
 *  FindCell(). If this still fails, a global cell location / search is invoked via
 *  vtkFindCellStrategy. Finally, if this operation fails, the streamline is
 *  considered terminated.
 *
 *  Note the particular find cell strategy employed can affect the behavior
 *  of this class. If the strategy involved using a point locator (e.g.,
 *  vtkStaticPointLocator or vtkPointLocator via vtkClosestPointStrategy or
 *  vtkClosestNPointsStrategy) the performance of the class improves to the
 *  detriment of robustness. Using a cell locator (e.g., vtkStaticCellLocator
 *  or vtkCellLocator via vtkCellLocatorStrategy) improves robustness at some
 *  cost to performance. Originally, these different behaviors (i.e., using
 *  different locators) was codified into different subclasses of
 *  vtkAbstractInterpolatedVelocityField.
 *
 *  Note that topologically structured classes such as vtkImageData and
 *  vtkRectilinearGrid are able to provide fast robust cell location. Hence
 *  the specified find cell strategy is only applicable to subclasses of
 *  vtkPointSet (such as vtkUnstructuredGrid).
 *
 *
 * @warning
 *  vtkAbstractInterpolatedVelocityField is not thread safe. A new instance
 *  should be created by each thread.
 *
 * @sa
 *  vtkCompositeInterpolatedVelocityField vtkAMRInterpolatedVelocityField
 *  vtkGenericInterpolatedVelocityField vtkTemporalInterpolatedVelocityField
 *  vtkFunctionSet vtkStreamTracer vtkFindCellStrategy
 */

#ifndef vtkAbstractInterpolatedVelocityField_h
#define vtkAbstractInterpolatedVelocityField_h

#include "vtkFiltersFlowPathsModule.h" // For export macro
#include "vtkFunctionSet.h"
#include "vtkNew.h"          // for vtkNew
#include "vtkSmartPointer.h" // for vtkSmartPointer

#include <vector> // for weights

VTK_ABI_NAMESPACE_BEGIN
class vtkCellLocatorStrategy;
class vtkClosestPointStrategy;
class vtkClosestNPointsStrategy;
class vtkCompositeDataSet;
class vtkDataObject;
class vtkDataSet;
class vtkDataArray;
class vtkIdList;
class vtkPointData;
class vtkGenericCell;
class vtkFindCellStrategy;

class VTKFILTERSFLOWPATHS_EXPORT vtkAbstractInterpolatedVelocityField : public vtkFunctionSet
{
public:
  ///@{
  /**
   * Standard methods for obtaining type information and printing the object state.
   */
  vtkTypeMacro(vtkAbstractInterpolatedVelocityField, vtkFunctionSet);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  // Keep track of how the interpolated velocity field is
  // initialized. Currently, all datasets that compose the velocity field are
  // initialized (meaning that supporting structures like locators are
  // built).
  enum VelocityFieldInitializationState
  {
    NOT_INITIALIZED = 0,
    INITIALIZE_ALL_DATASETS = 1,
    SELF_INITIALIZE = 2
  };

  ///@{
  /**
   * The Initialize() method is used to build and cache supporting structures
   * (such as locators) which are used when operating on the interpolated
   * velocity field. This method is needed mainly to deal with thread safety
   * issues; i.e., these supporting structures must be built at the right
   * time to avoid race conditions. Currently this method is used by
   * vtkStreamTracer (and related classes) which process composite datasets
   * (in the future other dataset types may be supported). Also, a
   * initialization strategy can be specified which controls how the
   * initialization process functions (this is a API placeholder for the
   * future). Note that some subclasses may override the initialize
   * method (via SelfInitialize()) because they have special methods of
   * setting up the interpolated velocity field.
   */
  virtual void Initialize(vtkCompositeDataSet* compDS, int initStrategy = INITIALIZE_ALL_DATASETS);
  vtkGetMacro(InitializationState, int);
  ///@}

  ///@{
  /**
   * Set/Get the caching flag. If this flag is turned ON, there are two levels
   * of caching for when the strategy is CLOSEST_POINT and one level of caching
   * when the strategy is CELL_LOCATOR. Otherwise a global cell location is always
   * invoked for evaluating the function values at any point.
   */
  vtkSetMacro(Caching, bool);
  vtkGetMacro(Caching, bool);
  ///@}

  ///@{
  /**
   * Get the caching statistics. CacheHit refers to the number of level #0 cache
   * hits while CacheMiss is the number of level #0 cache misses.
   */
  vtkGetMacro(CacheHit, int);
  vtkGetMacro(CacheMiss, int);
  ///@}

  vtkGetObjectMacro(LastDataSet, vtkDataSet);

  ///@{
  /**
   * Get/Set the id of the cell cached from last evaluation.
   */
  vtkGetMacro(LastCellId, vtkIdType);
  virtual void SetLastCellId(vtkIdType c) { this->LastCellId = c; }
  ///@}

  /**
   * Set the id of the most recently visited cell of a dataset.
   */
  virtual void SetLastCellId(vtkIdType c, int dataindex) = 0;

  ///@{
  /**
   * Get/Set the name of a specified vector array. By default it is nullptr, with
   * the active vector array for use.
   */
  vtkGetStringMacro(VectorsSelection);
  vtkGetMacro(VectorsType, int);
  ///@}

  /**
   * the association type (see vtkDataObject::FieldAssociations)
   * and the name of the velocity data field
   */
  void SelectVectors(int fieldAssociation, const char* fieldName);

  ///@{
  /**
   * Set/Get the flag indicating vector post-normalization (following vector
   * interpolation). Vector post-normalization is required to avoid the
   * 'curve-overshooting' problem (caused by high velocity magnitude) that
   * occurs when Cell-Length is used as the step size unit (particularly the
   * Minimum step size unit). Furthermore, it is required by RK45 to achieve,
   * as expected, high numerical accuracy (or high smoothness of flow lines)
   * through adaptive step sizing. Note this operation is performed (when
   * NormalizeVector TRUE) right after vector interpolation such that the
   * differing amount of contribution of each node (of a cell) to the
   * resulting direction of the interpolated vector, due to the possibly
   * significantly-differing velocity magnitude values at the nodes (which is
   * the case with large cells), can be reflected as is. Also note that this
   * flag needs to be turned to FALSE after vtkInitialValueProblemSolver::
   * ComputeNextStep() as subsequent operations, e.g., vorticity computation,
   * may need non-normalized vectors.
   */
  vtkSetMacro(NormalizeVector, bool);
  vtkGetMacro(NormalizeVector, bool);
  ///@}

  ///@{
  /**
   * If set to true, the first three point of the cell will be used to compute a normal to the cell,
   * this normal will then be removed from the vorticity so the resulting vector in tangent to the
   * cell.
   *
   * This means that the input dataset should only contains 2D planar cells.
   */
  vtkSetMacro(ForceSurfaceTangentVector, bool);
  vtkGetMacro(ForceSurfaceTangentVector, bool);
  ///@}

  ///@{
  /**
   * If set to true, cell within tolerance factor will always be found, except for edges.
   * Please note 2D planar cells are expected.
   */
  vtkSetMacro(SurfaceDataset, bool);
  vtkGetMacro(SurfaceDataset, bool);
  ///@}

  /**
   * Copy essential parameters between instances of this class. This
   * generally is used to copy from instance prototype to another, or to copy
   * interpolators between thread instances.  Sub-classes can contribute to
   * the parameter copying process via chaining.
   */
  virtual void CopyParameters(vtkAbstractInterpolatedVelocityField* from);

  using Superclass::FunctionValues;
  /**
   * Evaluate the velocity field f at point (x, y, z).
   */
  int FunctionValues(double* x, double* f) override = 0;

  /**
   * Set the last cell id to -1 to incur a global cell search for the next point.
   */
  void ClearLastCellId() { this->LastCellId = -1; }

  ///@{
  /**
   * Get the interpolation weights cached from last evaluation. Return 1 if the
   * cached cell is valid and 0 otherwise.
   */
  int GetLastWeights(double* w);
  int GetLastLocalCoordinates(double pcoords[3]);
  ///@}

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
  vtkAbstractInterpolatedVelocityField();
  ~vtkAbstractInterpolatedVelocityField() override;

  static const double TOLERANCE_SCALE;
  static const double SURFACE_TOLERANCE_SCALE;

  int CacheHit;
  int CacheMiss;
  bool Caching;
  bool NormalizeVector;
  bool ForceSurfaceTangentVector;
  bool SurfaceDataset;
  int VectorsType;
  char* VectorsSelection;
  std::vector<double> Weights;
  double LastPCoords[3];
  int LastSubId;
  double LastClosestPoint[3];
  vtkIdType LastCellId;
  vtkDataSet* LastDataSet;
  vtkNew<vtkGenericCell> LastCell;
  vtkNew<vtkGenericCell> CurrentCell;
  vtkNew<vtkIdList> PointIds;

  /**
   * Make sure the velocity field is initialized: record the
   * initialization strategy.
   */
  int InitializationState;

  // This is used to keep track of the find cell strategy and vector array
  // associated with each dataset forming the velocity field. Note that the
  // find cells strategy can be null, this means the find cell is invoked
  // using the dataset's FindCell() method.
  struct vtkDataSetInformation
  {
    vtkDataSet* DataSet;
    vtkFindCellStrategy* Strategy;
    vtkDataArray* Vectors;

    vtkDataSetInformation(vtkDataSet* dataSet, vtkFindCellStrategy* strategy, vtkDataArray* vectors)
      : DataSet(dataSet)
      , Strategy(strategy)
      , Vectors(vectors)
    {
    }
  };
  ///@{
  /**
   * Define a FindCell() strategy, keep track of the strategies (and other
   * cached information) associated with each dataset.
   */
  vtkFindCellStrategy* FindCellStrategy;
  std::vector<vtkDataSetInformation> DataSetsInfo;
  std::vector<vtkDataSetInformation>::iterator GetDataSetInfo(vtkDataSet* dataset);
  ///@}

  ///@{
  /**
   * Set the name of a specific vector to be interpolated.
   */
  vtkSetStringMacro(VectorsSelection);
  ///@}

  /**
   * Evaluate the velocity field f at point (x, y, z) in a specified dataset
   * by invoking vtkDataSet::FindCell() to locate the next cell if the given
   * point is outside the current cell. To address vtkPointSet, vtkPointLocator
   * is involved via vtkPointSet::FindCell() using CLOSEST_POINT strategy
   * for cell location. In vtkCompositeInterpolatedVelocityField with a CELL_LOCATOR strategy,
   * this function is invoked just to handle vtkImageData and vtkRectilinearGrid that are not
   * assigned with any vtkAbstractCellLocator-type cell locator.
   * If activated, returned vector will be tangential to the first
   * three point of the cell
   */
  virtual int FunctionValues(vtkDataSet* ds, double* x, double* f);

  /**
   * Try to find the cell closest to provided x point in provided dataset,
   * By first testing inclusion in it's cached cell and neighbor
   * Then testing globally. Then, only if surface is activated finding the
   * closest cell using FindClosestPointWithinRadius
   */
  virtual bool FindAndUpdateCell(vtkDataSet* ds, vtkFindCellStrategy* strategy, double* x);

  friend class vtkTemporalInterpolatedVelocityField;
  ///@{
  /**
   * If all weights have been computed (parametric coords etc all valid), a
   * scalar/vector can be quickly interpolated using the known weights and
   * the cached generic cell. This function is primarily reserved for use by
   * vtkTemporalInterpolatedVelocityField
   */
  void FastCompute(vtkDataArray* vectors, double f[3]);
  void FastCompute(vtkAbstractInterpolatedVelocityField* inIVF, vtkDataArray* vectors, double f[3]);
  bool InterpolatePoint(vtkPointData* outPD, vtkIdType outIndex);
  bool InterpolatePoint(
    vtkAbstractInterpolatedVelocityField* inIVF, vtkPointData* outPD, vtkIdType outIndex);
  vtkGenericCell* GetLastCell()
  {
    return (this->LastCellId != -1) ? this->CurrentCell.Get() : nullptr;
  }
  ///@}

  ///@{
  /**
   * These methods pertain to initializing the vector field by subclasses (which
   * may have special initialization needs). The first allows a subclass to
   * perform additional initialization. The second enabled the subclass to add
   * a dataset, find cell strtegy, and associated vectors to FunctionHashMap.
   */
  virtual int SelfInitialize() { return 0; }
  void AddToDataSetsInfo(vtkDataSet*, vtkFindCellStrategy*, vtkDataArray* vectors);
  size_t GetDataSetsInfoSize();
  ///@}

private:
  vtkAbstractInterpolatedVelocityField(const vtkAbstractInterpolatedVelocityField&) = delete;
  void operator=(const vtkAbstractInterpolatedVelocityField&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
