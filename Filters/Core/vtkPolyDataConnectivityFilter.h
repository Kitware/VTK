// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPolyDataConnectivityFilter
 * @brief   extract polygonal data based on geometric connectivity
 *
 * vtkPolyDataConnectivityFilter is a filter that extracts cells that
 * share common points and/or satisfy a scalar threshold
 * criterion. (Such a group of cells is called a region.) The filter
 * works in one of six ways: 1) extract the largest (most points) connected region
 * in the dataset; 2) extract specified region numbers; 3) extract all
 * regions sharing specified point ids; 4) extract all regions sharing
 * specified cell ids; 5) extract the region closest to the specified
 * point; or 6) extract all regions (used to color regions).
 *
 * This filter is specialized for polygonal data. This means it runs a bit
 * faster and is easier to construct visualization networks that process
 * polygonal data.
 *
 * The behavior of vtkPolyDataConnectivityFilter can be modified by turning
 * on the boolean ivar ScalarConnectivity. If this flag is on, the
 * connectivity algorithm is modified so that cells are considered connected
 * only if 1) they are geometrically connected (share a point) and 2) the
 * scalar values of the cell's points falls in the scalar range specified.
 * If ScalarConnectivity and FullScalarConnectivity is ON, all the cell's
 * points must lie in the scalar range specified for the cell to qualify as
 * being connected. If FullScalarConnectivity is OFF, any one of the cell's
 * points may lie in the user specified scalar range for the cell to qualify
 * as being connected.
 *
 * This use of ScalarConnectivity is particularly useful for selecting cells
 * for later processing.
 *
 * @sa
 * vtkConnectivityFilter
 */

#ifndef vtkPolyDataConnectivityFilter_h
#define vtkPolyDataConnectivityFilter_h

#include "vtkFiltersCoreModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

#define VTK_EXTRACT_POINT_SEEDED_REGIONS 1
#define VTK_EXTRACT_CELL_SEEDED_REGIONS 2
#define VTK_EXTRACT_SPECIFIED_REGIONS 3
#define VTK_EXTRACT_LARGEST_REGION 4
#define VTK_EXTRACT_ALL_REGIONS 5
#define VTK_EXTRACT_CLOSEST_POINT_REGION 6

VTK_ABI_NAMESPACE_BEGIN
class vtkDataArray;
class vtkIdList;
class vtkIdTypeArray;

class VTKFILTERSCORE_EXPORT vtkPolyDataConnectivityFilter : public vtkPolyDataAlgorithm
{
public:
  vtkTypeMacro(vtkPolyDataConnectivityFilter, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Obtain the array containing the region sizes of the extracted
   * regions
   */
  vtkGetObjectMacro(RegionSizes, vtkIdTypeArray);
  ///@}

  /**
   * Construct with default extraction mode to extract largest regions.
   */
  static vtkPolyDataConnectivityFilter* New();

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
   * Turn on/off the use of Fully connected scalar connectivity. This is off
   * by default. The flag is used only if ScalarConnectivity is on. If
   * FullScalarConnectivity is ON, all the cell's points must lie in the
   * scalar range specified for the cell to qualify as being connected. If
   * FullScalarConnectivity is OFF, any one of the cell's points may lie in
   * the user specified scalar range for the cell to qualify as being
   * connected.
   */
  vtkSetMacro(FullScalarConnectivity, vtkTypeBool);
  vtkGetMacro(FullScalarConnectivity, vtkTypeBool);
  vtkBooleanMacro(FullScalarConnectivity, vtkTypeBool);
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
  void AddSeed(int id);

  /**
   * Delete a seed id (point or cell id). Note: ids are 0-offset.
   */
  void DeleteSeed(int id);

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

  ///@{
  /**
   * Specify whether to record input point ids that appear in the output connected
   * components. It may be useful to extract the visited point ids for use by a
   * downstream filter. Default is OFF.
   */
  vtkSetMacro(MarkVisitedPointIds, vtkTypeBool);
  vtkGetMacro(MarkVisitedPointIds, vtkTypeBool);
  vtkBooleanMacro(MarkVisitedPointIds, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Get the input point ids that appear in the output connected components. This is
   * non-empty only if MarkVisitedPointIds has been set.
   */
  vtkGetObjectMacro(VisitedPointIds, vtkIdList);
  ///@}

  ///@{
  /**
   * Set/get the desired precision for the output types. See the documentation
   * for the vtkAlgorithm::DesiredOutputPrecision enum for an explanation of
   * the available precision settings.
   */
  vtkSetMacro(OutputPointsPrecision, int);
  vtkGetMacro(OutputPointsPrecision, int);
  ///@}

protected:
  vtkPolyDataConnectivityFilter();
  ~vtkPolyDataConnectivityFilter() override;

  // Usual data generation method
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  vtkTypeBool ColorRegions;      // boolean turns on/off scalar gen for separate regions
  int ExtractionMode;            // how to extract regions
  vtkIdList* Seeds;              // id's of points or cells used to seed regions
  vtkIdList* SpecifiedRegionIds; // regions specified for extraction
  vtkIdTypeArray* RegionSizes;   // size (in cells) of each region extracted

  double ClosestPoint[3];

  vtkTypeBool ScalarConnectivity;
  vtkTypeBool FullScalarConnectivity;

  // Does this cell qualify as being scalar connected ?
  int IsScalarConnected(vtkIdType cellId);

  double ScalarRange[2];

  void TraverseAndMark();

  // used to support algorithm execution
  vtkDataArray* CellScalars;
  vtkIdList* NeighborCellPointIds;
  vtkIdType* Visited;
  vtkIdType* PointMap;
  vtkDataArray* NewScalars;
  vtkIdType RegionNumber;
  vtkIdType PointNumber;
  vtkIdType NumCellsInRegion;
  vtkDataArray* InScalars;
  vtkPolyData* Mesh;
  std::vector<vtkIdType> Wave;
  std::vector<vtkIdType> Wave2;
  vtkIdList* PointIds;
  vtkIdList* CellIds;
  vtkIdList* VisitedPointIds;

  vtkTypeBool MarkVisitedPointIds;
  int OutputPointsPrecision;

private:
  vtkPolyDataConnectivityFilter(const vtkPolyDataConnectivityFilter&) = delete;
  void operator=(const vtkPolyDataConnectivityFilter&) = delete;
};

/**
 * Return the method of extraction as a string.
 */
inline const char* vtkPolyDataConnectivityFilter::GetExtractionModeAsString()
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
