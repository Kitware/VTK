// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkConnectivityFilter
 * @brief   extract data based on geometric connectivity
 *
 * vtkConnectivityFilter is a filter that extracts cells that share common
 * points and/or meet other connectivity criterion. (Cells that share
 * vertices and meet other connectivity criterion such as scalar range are
 * known as a region.)  The filter works in one of six ways: 1) extract the
 * largest connected region in the dataset; 2) extract specified region
 * numbers; 3) extract all regions sharing specified point ids; 4) extract
 * all regions sharing specified cell ids; 5) extract the region closest to
 * the specified point; or 6) extract all regions (used to color the data by
 * region).
 *
 * vtkConnectivityFilter is generalized to handle any type of input dataset.
 * If the input to this filter is a vtkPolyData, the output will be a vtkPolyData.
 * For all other input types, it generates output data of type vtkUnstructuredGrid.
 * Note that the only Get*Output() methods that will return a non-null pointer
 * are GetUnstructuredGridOutput() and GetPolyDataOutput() when the output of the
 * filter is a vtkUnstructuredGrid or vtkPolyData, respectively.
 *
 * The behavior of vtkConnectivityFilter can be modified by turning on the
 * boolean ivar ScalarConnectivity. If this flag is on, the connectivity
 * algorithm is modified so that cells are considered connected only if 1)
 * they are geometrically connected (share a point) and 2) the scalar values
 * of one of the cell's points falls in the scalar range specified. This use
 * of ScalarConnectivity is particularly useful for volume datasets: it can
 * be used as a simple "connected segmentation" algorithm. For example, by
 * using a seed voxel (i.e., cell) on a known anatomical structure,
 * connectivity will pull out all voxels "containing" the anatomical
 * structure. These voxels can then be contoured or processed by other
 * visualization filters.
 *
 * If the extraction mode is set to all regions and ColorRegions is enabled,
 * The RegionIds are assigned to each region by the order in which the region
 * was processed and has no other significance with respect to the size of
 * or number of cells.
 *
 * @sa
 * vtkPolyDataConnectivityFilter, vtkGenerateRegionIds
 */

#ifndef vtkConnectivityFilter_h
#define vtkConnectivityFilter_h

#include "vtkFiltersCoreModule.h" // For export macro
#include "vtkPointSetAlgorithm.h"

#include "vtkNew.h" // for member

#define VTK_EXTRACT_POINT_SEEDED_REGIONS 1
#define VTK_EXTRACT_CELL_SEEDED_REGIONS 2
#define VTK_EXTRACT_SPECIFIED_REGIONS 3
#define VTK_EXTRACT_LARGEST_REGION 4
#define VTK_EXTRACT_ALL_REGIONS 5
#define VTK_EXTRACT_CLOSEST_POINT_REGION 6

VTK_ABI_NAMESPACE_BEGIN
class vtkDataArray;
class vtkDataSet;
class vtkFloatArray;
class vtkIdList;
class vtkIdTypeArray;
class vtkIntArray;
class vtkPolyData;

class VTKFILTERSCORE_EXPORT vtkConnectivityFilter : public vtkPointSetAlgorithm
{
public:
  vtkTypeMacro(vtkConnectivityFilter, vtkPointSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Construct with default extraction mode to extract largest regions.
   */
  static vtkConnectivityFilter* New();

  ///@{
  /**
   * Turn on/off connectivity based on scalar value. If on, cells are connected
   * only if they share points AND one of the cells scalar values falls in the
   * scalar range specified.
   */
  vtkSetMacro(ScalarConnectivity, vtkTypeBool);
  vtkGetMacro(ScalarConnectivity, vtkTypeBool);
  vtkBooleanMacro(ScalarConnectivity, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Set the scalar range to use to extract cells based on scalar connectivity.
   */
  vtkSetVector2Macro(ScalarRange, double);
  vtkGetVector2Macro(ScalarRange, double);
  ///@}

  ///@{
  /**
   * Control the extraction of connected surfaces.
   */
  vtkSetClampMacro(
    ExtractionMode, int, VTK_EXTRACT_POINT_SEEDED_REGIONS, VTK_EXTRACT_CLOSEST_POINT_REGION);
  vtkGetMacro(ExtractionMode, int);
  void SetExtractionModeToPointSeededRegions()
  {
    this->SetExtractionMode(VTK_EXTRACT_POINT_SEEDED_REGIONS);
  }
  void SetExtractionModeToCellSeededRegions()
  {
    this->SetExtractionMode(VTK_EXTRACT_CELL_SEEDED_REGIONS);
  }
  void SetExtractionModeToLargestRegion() { this->SetExtractionMode(VTK_EXTRACT_LARGEST_REGION); }
  void SetExtractionModeToSpecifiedRegions()
  {
    this->SetExtractionMode(VTK_EXTRACT_SPECIFIED_REGIONS);
  }
  void SetExtractionModeToClosestPointRegion()
  {
    this->SetExtractionMode(VTK_EXTRACT_CLOSEST_POINT_REGION);
  }
  void SetExtractionModeToAllRegions() { this->SetExtractionMode(VTK_EXTRACT_ALL_REGIONS); }
  const char* GetExtractionModeAsString();
  ///@}

  /**
   * Initialize list of point ids/cell ids used to seed regions.
   */
  void InitializeSeedList();

  /**
   * Add a seed id (point or cell id). Note: ids are 0-offset.
   */
  void AddSeed(vtkIdType id);

  /**
   * Delete a seed id (point or cell id). Note: ids are 0-offset.
   */
  void DeleteSeed(vtkIdType id);

  /**
   * Initialize list of region ids to extract.
   */
  void InitializeSpecifiedRegionList();

  /**
   * Add a region id to extract. Note: ids are 0-offset.
   */
  void AddSpecifiedRegion(int id);

  /**
   * Delete a region id to extract. Note: ids are 0-offset.
   */
  void DeleteSpecifiedRegion(int id);

  ///@{
  /**
   * Use to specify x-y-z point coordinates when extracting the region
   * closest to a specified point.
   */
  vtkSetVector3Macro(ClosestPoint, double);
  vtkGetVectorMacro(ClosestPoint, double, 3);
  ///@}

  /**
   * Obtain the number of connected regions.
   */
  int GetNumberOfExtractedRegions();

  ///@{
  /**
   * Turn on/off the coloring of connected regions.
   */
  vtkSetMacro(ColorRegions, vtkTypeBool);
  vtkGetMacro(ColorRegions, vtkTypeBool);
  vtkBooleanMacro(ColorRegions, vtkTypeBool);
  ///@}

  /**
   * Enumeration of the various ways to assign RegionIds when
   * the ColorRegions option is on.
   */
  enum RegionIdAssignment
  {
    UNSPECIFIED,
    CELL_COUNT_DESCENDING,
    CELL_COUNT_ASCENDING
  };

  ///@{
  /**
   * Set/get mode controlling how RegionIds are assigned.
   */
  ///@}
  vtkSetMacro(RegionIdAssignmentMode, int);
  vtkGetMacro(RegionIdAssignmentMode, int);

  ///@{
  /**
   * Set/get the desired precision for the output types. See the documentation
   * for the vtkAlgorithm::DesiredOutputPrecision enum for an explanation of
   * the available precision settings.
   */
  vtkSetMacro(OutputPointsPrecision, int);
  vtkGetMacro(OutputPointsPrecision, int);
  ///@}

  ///@{
  /**
   * Set/get the activation of the compression for the output arrays.
   * When on, the output arrays is compressed to optimize memory.
   * This is used only when ColorRegions is true.
   * Default is true.
   */
  vtkSetMacro(CompressArrays, bool);
  vtkGetMacro(CompressArrays, bool);
  vtkBooleanMacro(CompressArrays, bool);
  ///@}

protected:
  vtkConnectivityFilter();
  ~vtkConnectivityFilter() override;

  ///@{
  /**
   * Usual vtkAlgorithm method implementations.
   */
  vtkTypeBool ProcessRequest(
    vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestDataObject(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  // Requires a vtkDataSet
  int FillInputPortInformation(int port, vtkInformation* info) override;
  // Outputs a vtkDataSet
  int FillOutputPortInformation(int vtkNotUsed(port), vtkInformation* info) override;
  ///@}

  /**
   * Add regions ids array to output dataset.
   * Compress arrays if CompressArrays is on.
   */
  void AddRegionsIds(vtkDataSet* output, vtkDataArray* pointArray, vtkDataArray* cellArray);

  // boolean turns on/off scalar gen for separate regions
  vtkTypeBool ColorRegions = 0;
  // how to extract regions
  int ExtractionMode = VTK_EXTRACT_LARGEST_REGION;
  int OutputPointsPrecision = vtkAlgorithm::DEFAULT_PRECISION;
  // id's of points or cells used to seed regions
  vtkIdList* Seeds = nullptr;
  // regions specified for extraction
  vtkIdList* SpecifiedRegionIds = nullptr;
  // size (in cells) of each region extracted
  vtkIdTypeArray* RegionSizes = nullptr;

  double ClosestPoint[3] = { 0, 0, 0 };

  vtkTypeBool ScalarConnectivity = 0;
  double ScalarRange[2] = { 0, 1 };

  int RegionIdAssignmentMode = UNSPECIFIED;

  /**
   * Mark current cell as visited and assign region number.  Note:
   * traversal occurs across shared vertices.
   */
  void TraverseAndMark(vtkDataSet* input);

  void OrderRegionIds(vtkIdTypeArray* pointRegionIds, vtkIdTypeArray* cellRegionIds);

  /**
   * Compress the given array, returning a vtkImplicitArray.
   * Useful for RegionId arrays, that often have a small amount of different values.
   *
   * see ColorRegions.
   * Uses vtkToImplicitArrayFilter and relevant strategy.
   */
  vtkSmartPointer<vtkDataArray> CompressWithImplicit(vtkDataArray* array);

private:
  // used to support algorithm execution
  vtkNew<vtkFloatArray> CellScalars;
  vtkNew<vtkIdList> NeighborCellPointIds;
  vtkIdType* Visited = nullptr;
  vtkIdType* PointMap = nullptr;
  vtkNew<vtkIdTypeArray> NewScalars;
  vtkNew<vtkIdTypeArray> NewCellScalars;
  vtkIdType RegionNumber = 0;
  vtkIdType PointNumber = 0;
  vtkIdType NumCellsInRegion = 0;
  vtkDataArray* InScalars = nullptr;
  vtkIdList* Wave = nullptr;
  vtkIdList* Wave2 = nullptr;
  vtkIdList* PointIds = nullptr;
  vtkIdList* CellIds = nullptr;
  bool CompressArrays = true;

  vtkConnectivityFilter(const vtkConnectivityFilter&) = delete;
  void operator=(const vtkConnectivityFilter&) = delete;
};

/**
 * Return the method of extraction as a string.
 */
inline const char* vtkConnectivityFilter::GetExtractionModeAsString()
{
  if (this->ExtractionMode == VTK_EXTRACT_POINT_SEEDED_REGIONS)
  {
    return "ExtractPointSeededRegions";
  }
  else if (this->ExtractionMode == VTK_EXTRACT_CELL_SEEDED_REGIONS)
  {
    return "ExtractCellSeededRegions";
  }
  else if (this->ExtractionMode == VTK_EXTRACT_SPECIFIED_REGIONS)
  {
    return "ExtractSpecifiedRegions";
  }
  else if (this->ExtractionMode == VTK_EXTRACT_ALL_REGIONS)
  {
    return "ExtractAllRegions";
  }
  else if (this->ExtractionMode == VTK_EXTRACT_CLOSEST_POINT_REGION)
  {
    return "ExtractClosestPointRegion";
  }
  else
  {
    return "ExtractLargestRegion";
  }
}

VTK_ABI_NAMESPACE_END
#endif
