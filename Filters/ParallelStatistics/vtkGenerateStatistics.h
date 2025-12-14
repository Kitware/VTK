// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright 2025 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
/**
 * @class   vtkGenerateStatistics
 * @brief   Statistical modeling of non-tabular data adapted from VTK geometric datasets.
 *
 * This filter computes a statistical model from an input data object.
 * One of its member variables is the statistics algorithm
 * to use when creating the model; this class itself only
 * adapts/subsamples the input data into tables for processing.
 *
 * This class provides a simplified interface to vtkStatisticsAlgorithm for
 * solely the Learn and Derive stages.
 *
 * The output of this filter is always a partitioned dataset collection
 * of statistical model objects summarizing the input data.
 * In the case that the input is itself a partitioned dataset collection,
 * it may hold a tree of statistcal models in its vtkDataAssembly or a
 * single model of the entire tree (depending on whether SingleModel is set).
 *
 * This filter should accept any type of vtkDataObject as input, but
 * is especially geared to handle:
 * + vtkDataSet subclasses, both structured and unstructured;
 * + vtkCompositeDataSet subclasses, both structural- (partitioned dataset
 *   collections) and refinement-hierarchies (AMR grids); and
 * + vtkTable (which it will subsample if \a TrainingFraction < 1, but otherwise
 *   just passes the data through).
 *
 * Data objects such as vtkCellGrid, vtkGraph, and vtkHyperTreeGrid are not
 * fully supported at this point but functions to add support have been stubbed
 * out.
 *
 * This filter does not yet provide an option to weight samples by the
 * measure (volume, area, length) of the cell or point-neighborhood.
 * If you need statistics computed on a weighted basis, you must first use
 * the vtkCellMeasure filter and a calculator filter to compute the product
 * of the measure with your field of interest, then divide the output model
 * parameters by the sum of the measures across all samples.
 *
 * If you run this filter on distributed data, you are responsible for ensuring
 * that the relevant vtkFieldData::GetGhostArray() returns an array with bit 0
 * (either vtkDataSetAttributes::DUPLICATEPOINT or vtkDataSetAttributes::DUPLICATECELL
 * depending on the association of the field) set for any sample that should be
 * omitted (presumably because it is owned by a remote process or even another
 * partition in the same vtkPartitionedDataSet instance).
 */

#ifndef vtkGenerateStatistics_h
#define vtkGenerateStatistics_h

#include "vtkFiltersParallelStatisticsModule.h" //needed for exports
#include "vtkPartitionedDataSetCollectionAlgorithm.h"

#include <memory>
#include <unordered_map>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN

class vtkCompositeDataSet;
class vtkDataObjectToTable;
class vtkFieldData;
class vtkInformationIntegerKey;
class vtkUniformGridAMR;
class vtkPartitionedDataSet;
class vtkCellGrid;
class vtkDataSet;
class vtkGraph;
class vtkTable;
class vtkMultiProcessController;
class vtkStatisticalModel;
class vtkStatisticsAccumulator;
class vtkStatisticsAlgorithm;
class vtkStatisticsAlgorithmPrivate;
class vtkUnsignedCharArray;

class VTKFILTERSPARALLELSTATISTICS_EXPORT vtkGenerateStatistics
  : public vtkPartitionedDataSetCollectionAlgorithm
{
public:
  static vtkGenerateStatistics* New();
  vtkTypeMacro(vtkGenerateStatistics, vtkPartitionedDataSetCollectionAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Set/get the type of field attribute (cell, point, row, …, field)
   *
   * Values for this ivar should be taken from vtkDataObject::AttributeTypes.
   * When processing vtkCellGrid data, this should be set to vtkDataObject::CELL.
   */
  vtkGetMacro(AttributeMode, int);
  vtkSetMacro(AttributeMode, int);
  ///@}

  /**
   * Return the number of columns available for the current value of \a AttributeMode.
   */
  int GetNumberOfAttributeArrays();

  /**
   * Get the name of the \a nn-th array ffor the current value of \a AttributeMode.
   */
  const char* GetAttributeArrayName(int nn);

  /**
   * Get the status of the specified array (i.e., whether or not it is a column of interest).
   */
  int GetAttributeArrayStatus(const char* arrName);

  ///@{
  /**
   * An alternate interface for preparing a selection of arrays to process.
   */
  void EnableAttributeArray(const char* arrName);
  void ClearAttributeArrays();
  ///@}

  ///@{
  /**
   * Set/get the amount of data to be used for training.
   *
   * When 0.0 < \a TrainingFraction < 1.0, a randomly-sampled
   * subset of the data is used for training.
   * When an assessment is requested, all data (including the training data) is assessed,
   * regardless of the value of TrainingFraction.
   * The default value is 0.1.
   *
   * The random sample of the original dataset (say, of size N) is
   * obtained by choosing N random numbers in [0,1).
   * Any sample where the random number is less than \a TrainingFraction
   * is included in the training data.
   * Samples are then randomly added or removed from the training data
   * until it is the desired size.
   */
  vtkSetClampMacro(TrainingFraction, double, 0.0, 1.0);
  vtkGetMacro(TrainingFraction, double);
  ///@}

  ///@{
  /**
   * Get/Set the multiprocess controller. If no controller is set, single process is assumed.
   */
  virtual void SetController(vtkMultiProcessController*);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);
  ///@}

  ///@{
  /**
   * Get/set the statistics filter used to create a model of the data.
   */
  virtual void SetStatisticsAlgorithm(vtkStatisticsAlgorithm*);
  vtkGetObjectMacro(StatisticsAlgorithm, vtkStatisticsAlgorithm);
  ///@}

  ///@{
  /// Set/get whether to weight cells (respectively, graph-edges) by their measure when
  /// simultaneous sampling of cell-data (respectively, edge-data) and point-data
  /// (respectively graph-vertex-data) is required.
  ///
  /// If true, computation/lookup of these measures is performed – which will slow this
  /// algorithm down. The default is false.
  vtkSetMacro(WeightByCellMeasure, vtkTypeBool);
  vtkGetMacro(WeightByCellMeasure, vtkTypeBool);
  vtkBooleanMacro(WeightByCellMeasure, vtkTypeBool);
  ///@}

  ///@{
  /// Set/get whether to aggregate all the models in a composite dataset or report
  /// a model per tree entry.
  vtkSetMacro(SingleModel, vtkTypeBool);
  vtkGetMacro(SingleModel, vtkTypeBool);
  vtkBooleanMacro(SingleModel, vtkTypeBool);
  ///@}

protected:
  vtkGenerateStatistics();
  ~vtkGenerateStatistics() override;

  int FillInputPortInformation(int port, vtkInformation* info) override;
  // int FillOutputPortInformation(int port, vtkInformation* info) override;

  int RequestData(
    vtkInformation* request, vtkInformationVector** input, vtkInformationVector* output) override;

  /// Translate requests from field names to "field_component" names for vector/tensor fields.
  ///
  /// For example, if a field named "velocity" has 3 components, this->P will
  /// contain "velocity" as a request but columnRequests should contain "velocity_0",
  /// "velocity_1", and "velocity_2".
  ///
  /// The user requests are held in this->P; this method updates this->StatisticsAlgorithm
  /// to hold the table-column requests.
  void TranslateRequests(vtkDataObject* data);

  /// Determine the type of \a dataObject and call the matching variant below.
  ///
  /// This will call some combination of RequestDataAMR, RequestDataPD, RequestDataPDC,
  /// and RequestDataNonComposite (which in turn calls RequestDataCellGrid or RequestDataPlain)
  /// depending on the input data.
  int RequestLocalDataDispatch(
    vtkDataObject* dataObject, vtkPartitionedDataSetCollection* modelTree);

  /// Determine the type of non-composite \a dataObject and call the matching variant below.
  ///
  /// This will call some RequestDataCellGrid or RequestDataPlain depending on the input data.
  /// If additional types, such as vtkAbstractElectronicData or vtkAnnotation, need to be
  /// handled, checking should be performed here.
  int RequestDataNonComposite(vtkDataObject* dataObject, vtkStatisticalModel* model);

  /// Populate \a model with the statistics of a uniform-grid AMR dataset, \a amr.
  int RequestDataAMR(vtkUniformGridAMR* amr, vtkPartitionedDataSetCollection* modelTree);
  /// Populate \a model with the statistics of a partitioned dataset collection, \ a pdc.
  int RequestDataPDC(
    vtkPartitionedDataSetCollection* pdc, vtkPartitionedDataSetCollection* modelTree);
  /// Populate \a model with the statistics of a partitioned dataset, \ a pd.
  int RequestDataPD(vtkPartitionedDataSet* pd, vtkStatisticalModel* model);
  /// Populate \a model with the statistics of a cell-grid, \a cellGrid.
  int RequestDataCellGrid(vtkCellGrid* cellGrid, vtkStatisticalModel* model);
  /// Populate \a model with the statistics of a "plain" data object, \a dataObject.
  /// This handles vtkDataSet, vtkGraph, vtkTable, and other data held only in vtkDataSetAttributes.
  int RequestDataPlain(vtkDataObject* dataObject, vtkStatisticalModel* model);

  /**
   * This method translates input-array specifications made on vtkGenerateStatistics
   * into requests on its internal vtkStatisticsAlgorithm instance.
   *
   * If the internal StatisticsAlgorithm provides a non-zero limit N on request
   * size and the number of input arrays is M, then M-choose-N requests are
   * created.
   */
  void PrepareAlgorithmRequests(const std::vector<vtkSmartPointer<vtkAbstractArray>>& columns);

  /**
   * Subclasses <b>may</b> (but need not) override this function to guarantee that
   * some minimum number of observations are included in the training data.
   * By default, it returns the maximum of:
   * observations->GetNumberOfRows() * this->TrainingFraction and
   * min( observations->GetNumberOfRows(), 100 ).
   * Thus, it will require the entire set of observations unless there are more than 100.
   * Parameter N is the number of non-ghost observations.
   */
  virtual vtkIdType GetNumberOfObservationsForTraining(vtkIdType N);

  /**
   * A variant of shallow copy that calls vtkDataObject::ShallowCopy() and then
   * for composite datasets, creates clones for each leaf node that then shallow
   * copies the fields and geometry.
   */
  void ShallowCopy(vtkDataObject* out, vtkDataObject* in);

  /**
   * Generate a subset of IDs according to the training fraction and ghost markings.
   *
   * The output \a subset holds a map from input IDs to their output location in a
   * dense array (i.e., it maps N values from [0, numberOfTuples - 1] to [0, N - 1].
   */
  void GenerateSubset(std::unordered_map<vtkIdType, vtkIdType>& subset, vtkIdType numberOfTuples,
    double trainingFraction, vtkUnsignedCharArray* ghostMarks, unsigned char ghostMask);

  using PointsOfCellsWeightMap =
    std::unordered_map<vtkIdType, std::unordered_map<vtkIdType, double>>;
  /**
   * Given a vtkDataSet, compute weights for each point of each cell in \a subset
   * (or all cells if \a subset is empty).
   *
   */
  void ComputeCellToPointWeights(PointsOfCellsWeightMap& cellToPointsToWeights, vtkDataSet* dataSet,
    const std::unordered_map<vtkIdType, vtkIdType>& subset);

  /**
   * Given a vtkGraph, compute weights for each vertex of each edge in \a subset
   * (or all edges if \a subset is empty).
   *
   */
  void ComputeEdgeToVertexWeights(
    std::unordered_map<vtkIdType, std::unordered_map<vtkIdType, double>>& edgesToVertsToWeights,
    vtkGraph* graph, const std::unordered_map<vtkIdType, vtkIdType>& subset);

  /**
   * Return the array itself or a subset as specified.
   */
  vtkSmartPointer<vtkAbstractArray> SubsetArray(vtkSmartPointer<vtkAbstractArray> fullArray,
    const std::unordered_map<vtkIdType, vtkIdType>& subset);

  /**
   * Resample a cell array to points.
   */
  vtkSmartPointer<vtkAbstractArray> CellToPointSamples(vtkSmartPointer<vtkAbstractArray> fullArray,
    vtkDataSet* data, const std::unordered_map<vtkIdType, vtkIdType>& subset,
    const PointsOfCellsWeightMap& cellsToPointsToWeights);

  vtkSmartPointer<vtkAbstractArray> EdgeToVertexSamples(vtkSmartPointer<vtkAbstractArray> fullArray,
    vtkGraph* data, const std::unordered_map<vtkIdType, vtkIdType>& subset,
    const std::unordered_map<vtkIdType, std::unordered_map<vtkIdType, double>>&
      edgesToVertsToWeights);

  vtkSmartPointer<vtkAbstractArray> FieldDataToSamples(vtkSmartPointer<vtkAbstractArray> fullArray,
    vtkDataObject* data, const std::unordered_map<vtkIdType, vtkIdType>& subset,
    vtkIdType numberOfSamples);

  /// Communicate with other ranks to merge all remote models into \a modelTree.
  ///
  /// This method will reduce all models in \a modelTree to rank 0 and then
  /// broadcast the resulting tree to all ranks (so that all ranks can assess
  /// and test data against the model).
  int MergeRemoteModels(vtkPartitionedDataSetCollection* modelTree);

  /// Merge all the models from \a other into \a target.
  ///
  /// If SingleModel is true, this method will call this->StatisticsAlgorithm::Aggregate()
  /// at most once. If false, it may aggregate many times (as many as there are models in
  /// the two trees, which must have the same structure).
  int MergeModelTrees(
    vtkPartitionedDataSetCollection* other, vtkPartitionedDataSetCollection* target);

  /// Once remote model(s) have been merged, derived information is computed on all ranks.
  int ComputeDerivedData(vtkPartitionedDataSetCollection* modelTree);

  int AttributeMode;
  double TrainingFraction;
  vtkTypeBool SingleModel;
  vtkTypeBool WeightByCellMeasure;
  std::unique_ptr<vtkStatisticsAlgorithmPrivate> P;
  vtkMultiProcessController* Controller;
  vtkStatisticsAlgorithm* StatisticsAlgorithm;

private:
  // This class is internal so it can access protected data.
  class StatisticsAccumulator;

  vtkGenerateStatistics(const vtkGenerateStatistics&) = delete;
  void operator=(const vtkGenerateStatistics&) = delete;
};

VTK_ABI_NAMESPACE_END

#endif // vtkGenerateStatistics_h
