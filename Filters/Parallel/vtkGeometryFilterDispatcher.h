// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkGeometryFilterDispatcher
 * @brief   Geometry filter that does outlines for volumes.
 *
 * This filter defaults to using the outline filter unless the input
 * is a structured volume.
 */

#ifndef vtkGeometryFilterDispatcher_h
#define vtkGeometryFilterDispatcher_h

#include "vtkDataObjectAlgorithm.h"
#include "vtkFiltersParallelModule.h" // needed for export macro
#include "vtkSmartPointer.h"          // needed for vtkSmartPointer

#include "vtkNew.h" // for vtkNew

VTK_ABI_NAMESPACE_BEGIN

class vtkCartesianGrid;
class vtkCellGrid;
class vtkDataSet;
class vtkDataObjectMeshCache;
class vtkDataObjectTree;
class vtkExplicitStructuredGrid;
class vtkFeatureEdges;
class vtkGenericDataSet;
class vtkGenericGeometryFilter;
class vtkGeometryFilter;
class vtkHyperTreeGrid;
class vtkImageData;
class vtkInformationIntegerVectorKey;
class vtkInformationVector;
class vtkMultiProcessController;
class vtkOutlineSource;
class vtkPolyData;
class vtkPolyDataNormals;
class vtkRecoverGeometryWireframe;
class vtkRectilinearGrid;
class vtkStructuredGrid;
class vtkUnstructuredGridBase;
class vtkUnstructuredGridGeometryFilter;

class VTKFILTERSPARALLEL_EXPORT vtkGeometryFilterDispatcher : public vtkDataObjectAlgorithm
{
public:
  static vtkGeometryFilterDispatcher* New();
  vtkTypeMacro(vtkGeometryFilterDispatcher, vtkDataObjectAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * This flag is set during the execute method.  It indicates
   * that the input was 3d and an outline representation was used.
   */
  vtkGetMacro(OutlineFlag, bool);
  ///@}

  ///@{
  /**
   * Set/get whether to produce outline (vs. surface).
   */
  vtkSetMacro(UseOutline, bool);
  vtkGetMacro(UseOutline, bool);
  vtkBooleanMacro(UseOutline, bool);
  ///@}

  ///@{
  /**
   * Set/get whether to produce feature edges (vs. surface).
   * If both this and UseOutline are true, then an outline will be produced.
   */
  void SetGenerateFeatureEdges(bool);
  vtkGetMacro(GenerateFeatureEdges, bool);
  ///@}

  ///@{
  /**
   * Determines the number of distinct values in vtkBlockColors
   */
  vtkSetMacro(BlockColorsDistinctValues, int);
  vtkGetMacro(BlockColorsDistinctValues, int);
  ///@}

  ///@{
  /**
   * Whether to generate cell normals.
   *
   * The default value is false.
   */
  void SetGenerateCellNormals(bool);
  vtkGetMacro(GenerateCellNormals, bool);
  vtkBooleanMacro(GenerateCellNormals, bool);
  ///@}

  ///@{
  /**
   * Whether to generate point normals.
   *
   * The default value is false.
   */
  void SetGeneratePointNormals(bool);
  vtkGetMacro(GeneratePointNormals, bool);
  vtkBooleanMacro(GeneratePointNormals, bool);
  ///@}

  ///@{
  /**
   * Specify the angle that defines a sharp edge. If the difference in
   * angle across neighboring polygons is greater than this value, the
   * shared edge is considered "sharp".
   *
   * The default value is 30 degrees.
   */
  void SetFeatureAngle(double);
  vtkGetMacro(FeatureAngle, double);
  ///@}

  ///@{
  /**
   * Turn on/off the splitting of sharp edges.
   *
   * The default value is true.
   */
  void SetSplitting(bool);
  vtkGetMacro(Splitting, bool);
  vtkBooleanMacro(Splitting, bool);
  ///@}

  ///@{
  /**
   * Whether to triangulate mesh for rendering. This parameter avoid
   * rendering issues of non-convex polygons.
   * This option has no effect when using OpenGL2 rendering backend. OpenGL2
   * rendering always triangulates polygonal meshes.
   */
  vtkSetMacro(Triangulate, bool);
  vtkGetMacro(Triangulate, bool);
  vtkBooleanMacro(Triangulate, bool);
  ///@}

  ///@{
  /**
   * Nonlinear faces are approximated with flat polygons.  This parameter
   * controls how many times to subdivide nonlinear surface cells.  Higher
   * subdivisions generate closer approximations but take more memory and
   * rendering time.  Subdivision is recursive, so the number of output polygons
   * can grow exponentially with this parameter.
   */
  virtual void SetNonlinearSubdivisionLevel(int);
  vtkGetMacro(NonlinearSubdivisionLevel, int);
  ///@}

  ///@{
  /**
   * When two volumetric cells of different order are connected by their corners (for instance, a
   * quadratic hexahedron next to a linear hexahedron ), the internal face is rendered and is not
   * considered as a ghost cell. To remove these faces, switch MatchBoundariesIgnoringCellOrder to 1
   * (default is 0).
   */
  virtual void SetMatchBoundariesIgnoringCellOrder(int);
  vtkGetMacro(MatchBoundariesIgnoringCellOrder, int);
  ///@}

  ///@{
  /**
   * Set and get the controller.
   */
  virtual void SetController(vtkMultiProcessController*);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);
  ///@}

  ///@{
  /**
   * If on, the output polygonal dataset will have a celldata array that
   * holds the cell index of the original 3D cell that produced each output
   * cell. This is useful for picking but it takes memory. The default is on.
   */
  void SetPassThroughCellIds(bool);
  vtkGetMacro(PassThroughCellIds, bool);
  vtkBooleanMacro(PassThroughCellIds, bool);
  ///@}

  ///@{
  /**
   * If on, the output polygonal dataset will have a pointdata array that
   * holds the point index of the original vertex that produced each output
   * vertex. This is useful for picking but it takes memory. The default is on.
   */
  void SetPassThroughPointIds(bool);
  vtkGetMacro(PassThroughPointIds, bool);
  vtkBooleanMacro(PassThroughPointIds, bool);
  ///@}

  ///@{
  /**
   * If on, point arrays named vtkProcessId is added.
   */
  vtkSetMacro(GenerateProcessIds, bool);
  vtkGetMacro(GenerateProcessIds, bool);
  vtkBooleanMacro(GenerateProcessIds, bool);
  ///@}

  ///@{
  /**
   * This property affects the way AMR outlines and faces are generated.
   * When set to true (default), internal data-set faces/outlines for datasets within
   * the AMR grids are hidden. Set it to false to see boxes for all the datasets
   * in the AMR, internal or otherwise.
   */
  vtkSetMacro(HideInternalAMRFaces, bool);
  vtkGetMacro(HideInternalAMRFaces, bool);
  vtkBooleanMacro(HideInternalAMRFaces, bool);
  ///@}

  ///@{
  /**
   * For overlapping AMR, this property controls affects the way AMR
   * outlines are generated. When set to true (default), it uses the
   * overlapping AMR meta-data to identify the blocks present in the AMR.
   * Which implies that even if the input did not fill in the uniform grids for
   * all datasets in the AMR, this filter can generate outlines using the
   * metadata alone. When set to false, the filter will only generate outlines
   * for datasets that are actually present. Note, this only affects overlapping
   * AMR.
   */
  vtkSetMacro(UseNonOverlappingAMRMetaDataForOutlines, bool);
  vtkGetMacro(UseNonOverlappingAMRMetaDataForOutlines, bool);
  vtkBooleanMacro(UseNonOverlappingAMRMetaDataForOutlines, bool);
  ///@}

protected:
  vtkGeometryFilterDispatcher();
  ~vtkGeometryFilterDispatcher() override;

  ///@{
  /**
   * Overridden to create vtkMultiBlockDataSet when input is a
   * composite-dataset and vtkPolyData when input is a vtkDataSet.
   */
  int RequestDataObject(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  virtual int RequestAMRData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector);
  virtual int RequestDataObjectTree(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector);
  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;
  ///@}

  /**
   * Produce geometry for a block in the dataset.
   * This does not handle producing outlines. Call only when this->UseOutline ==
   * 0; \c extractface mask it is used to determine external faces.
   */
  void ExecuteAMRBlock(vtkCartesianGrid* input, vtkPolyData* output, const bool extractface[6]);

  /**
   * Used instead of ExecuteAMRBlock() when this->UseOutline is true.
   */
  void ExecuteAMRBlockOutline(
    const double bounds[6], vtkPolyData* output, const bool extractface[6]);

  void ExecuteBlock(vtkDataObject* input, vtkPolyData* output, bool doCommunicate, int updatePiece,
    int updateNumPieces, int updateGhosts, const int* wholeExtent);

  void DataSetExecute(vtkDataSet* input, vtkPolyData* output, bool doCommunicate);
  void GenericDataSetExecute(vtkGenericDataSet* input, vtkPolyData* output, bool doCommunicate);

  void ImageDataExecute(
    vtkImageData* input, vtkPolyData* output, bool doCommunicate, int updatePiece, const int* ext);

  void StructuredGridExecute(vtkStructuredGrid* input, vtkPolyData* output, int updatePiece,
    int updateNumPieces, int updateGhosts, const int* wholeExtent);

  void RectilinearGridExecute(vtkRectilinearGrid* input, vtkPolyData* output, int updatePiece,
    int updateNumPieces, int updateGhosts, const int* wholeExtent);

  void UnstructuredGridExecute(
    vtkUnstructuredGridBase* input, vtkPolyData* output, bool doCommunicate);

  void PolyDataExecute(vtkPolyData* input, vtkPolyData* output, bool doCommunicate);

  void HyperTreeGridExecute(vtkHyperTreeGrid* input, vtkPolyData* output, bool doCommunicate);

  void ExplicitStructuredGridExecute(
    vtkExplicitStructuredGrid* input, vtkPolyData* out, bool doCommunicate, const int* wholeExtent);

  void CellGridExecute(vtkCellGrid* input, vtkPolyData* output, bool doCommunicate);

  ///@{
  /**
   * Cleans up the output polydata. If doCommunicate is true the method is free
   * to communicate with other processes as needed.
   */
  void CleanupOutputData(vtkPolyData* output);
  ///@}

  bool OutlineFlag = false;
  bool UseOutline = true;
  int BlockColorsDistinctValues = 7;
  bool GenerateCellNormals = false;
  bool GeneratePointNormals = false;
  bool Splitting = true;
  double FeatureAngle = 30.0;
  bool Triangulate = false;
  int NonlinearSubdivisionLevel = 1;
  int MatchBoundariesIgnoringCellOrder = 0;

  vtkMultiProcessController* Controller = nullptr;
  vtkSmartPointer<vtkOutlineSource> OutlineSource;
  vtkSmartPointer<vtkGeometryFilter> GeometryFilter;
  vtkSmartPointer<vtkGenericGeometryFilter> GenericGeometryFilter;
  vtkSmartPointer<vtkUnstructuredGridGeometryFilter> UnstructuredGridGeometryFilter;
  vtkSmartPointer<vtkRecoverGeometryWireframe> RecoverWireframeFilter;
  vtkSmartPointer<vtkFeatureEdges> FeatureEdgesFilter;
  vtkSmartPointer<vtkPolyDataNormals> PolyDataNormals;

  /**
   * Call CheckAttributes on the \c input which ensures that all attribute
   * arrays have valid lengths.
   */
  int CheckAttributes(vtkDataObject* input);

  // Callback for recording progress of internal filters.
  void HandleGeometryFilterProgress(vtkObject* caller, unsigned long, void*);

  int FillInputPortInformation(int, vtkInformation*) override;

  void ReportReferences(vtkGarbageCollector*) override;

  bool GenerateProcessIds = false;
  bool PassThroughCellIds = true;
  bool PassThroughPointIds = true;
  bool HideInternalAMRFaces = true;
  bool UseNonOverlappingAMRMetaDataForOutlines = true;
  bool GenerateFeatureEdges = false;

private:
  vtkGeometryFilterDispatcher(const vtkGeometryFilterDispatcher&) = delete;
  void operator=(const vtkGeometryFilterDispatcher&) = delete;

  /**
   * Add vtkBlockColors and vtkCompositeIndex arrays to output vtkDataObjectTrees.
   * The realInput is needed to get the correct flat index when the input has been
   * converted to a vtkPartitionedDataSetCollection.
   */
  void AddDataObjectTreeArrays(vtkDataObjectTree* realInput, vtkDataObjectTree* output);

  /**
   * Adds a point and cell data array called "vtkCompositeIndex" to a vtkPolyData.
   */
  void AddCompositeIndex(vtkPolyData* pd, unsigned int index);
  ///@{
  /**
   * Adds a field array called "vtkBlockColors". The array is
   * added to each block only if the dataset is a composite
   * dataset. The array has one value set to
   * (blockIndex % BlockColorsDistinctValues)
   */
  void AddBlockColors(vtkDataObject* pd, unsigned int index);
  void AddHierarchicalIndex(vtkPolyData* pd, unsigned int level, unsigned int index);
  class BoundsReductionOperation;
  ///@}

  /**
   * Generate feature edges for the input hyper tree grid.
   * We need this dedicated function because generating feature edges
   * for an HTG is different than for other data objects: the
   * vtkHyperTreeGridFeatureEdges filter operates on the input HTG
   * directly, and not on the output polydata of the internal geometry
   * filter.
   */
  void GenerateFeatureEdgesHTG(vtkHyperTreeGrid* input, vtkPolyData* output);

  /**
   * Execute normals computation for the output polydata.
   */
  void ExecuteNormalsComputation(vtkPolyData* output);

  /**
   * Generate the "vtkProcessId" point and cell data arrays on the output
   * vtkPolydata.
   */
  void GenerateProcessIdsArrays(vtkPolyData* output);

  /**
   * Use cache to fill output from input if possible.
   * Return true on success.
   */
  bool UseCacheIfPossible(vtkDataObject* input, vtkDataObject* output);

  /**
   * Update cache content with given data object.
   */
  void UpdateCache(vtkDataObject* output);

  /**
   * Get the input as a vtkDataObjectTree.
   *
   * In fact, only vtkMultiBlockDataSet and vtkPartionedDataSetCollection are supported.
   * Other vtkDataObjectTree subclasses are converted to vtkPartitionedDataSetCollection.
   *
   * Temporary OriginalIds are added (for caching purpose)
   */
  vtkSmartPointer<vtkDataObjectTree> GetDataObjectTreeInput(vtkInformationVector** inputVector);

  vtkNew<vtkDataObjectMeshCache> MeshCache;
};

VTK_ABI_NAMESPACE_END

#endif
