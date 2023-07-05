// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPolyDataEdgeConnectivityFilter
 * @brief   segment polygonal mesh based on shared edge connectivity
 *
 * vtkPolyDataEdgeConnectivityFilter is a filter to segment cells that
 * share common edges (i.e., are edge connected), given certain conditions on
 * edge connectivity. These conditions are 1) the shared edge is not a
 * barrier edge, and 2) the edge neighbor satisfies conditions on scalar
 * values.  Specification of both #1 and #2 are optional; in which case all
 * polygons in a region that share edges are connected. Barrier edges are
 * either defined by providing an optional second polydata input (which
 * contains a list of lines defining the barrier edges), or a condition on
 * edge length. The conditions on edge length specify whether edges within a
 * range of edge lengths are considered barrier edges.  All connected
 * polygonal cells satisfying these conditions form a region. Typically the
 * filter segments multiple regions; however the user can specify which
 * region(s) are to be extracted and output.
 *
 * The filter works in one of seven ways: 1) extract the largest (in terms of
 * total surface area) edge-connected region in the dataset; 2) extract
 * specified regions; 3) extract all regions containing user-specified
 * point ids; 4) extract all regions containing user-specified cell ids; 5)
 * extract the region closest to a user-specified point; 6) extract all
 * edge-connected regions (used to color regions, i.e., create segmentation
 * labeling); or 7) extract "large" regions, that is all regions considered
 * large in terms of their surface area relative to the total input polydata
 * surface area.
 *
 * Barrier edges add a unique twist to the filter. By using them, it is
 * possible to segment out portions of a mesh with very small, very large, or
 * in between polygon-sized features.
 *
 * Due to the nature of edge connectivity, the filter only operates on
 * polygons. Vertices, lines, and triangle strips are ignored (and not passed
 * through to the output). Point and cell attribute data are copied to the
 * output; however, an additional, optional array named "RegionId" may be
 * added to the output cell attribute data by enabling ColorRegions.
 *
 * @warning
 * If more than one output region is produced, regions are sorted based on
 * their surface area. Thus region# 0 is the largest, followed by the next
 * largest and so on.
 *
 * @warning
 * To be clear: if scalar connectivity is enabled, this filter segments data
 * based on *cell* attribute data based on edge-connected meshes. The similar
 * vtkPolyDataConnectivityFilter segments based on point attribute data and
 * point-connected meshes.
 *
 * @warning
 * A second, optional vtkPolyData (the Source) may be specified which
 * contains edges (i.e., vtkPolyData::Lines) that specify barries to edge
 * connectivity. That is, two polygons who share an edge are not connected if
 * the shared edge exists in the Source vtkPolyData. This feature can be used
 * with other filters such as vtkDelaunay2D (and its constraint edges) to
 * create segmented regions.
 *
 * @warning
 * Note that mesh regions attached at just a point are not considered
 * connected. Thus such point-connected meshes will be segmented into
 * different regions. This differs from vtkPolyDataConnectivityFilter which
 * segments produces point-connected regions.
 *
 * @sa
 * vtkPolyDataConnectivityFilter vtkConnectivityFilter vtkDelaunay2D
 */

#ifndef vtkPolyDataEdgeConnectivityFilter_h
#define vtkPolyDataEdgeConnectivityFilter_h

#include "vtkFiltersCoreModule.h" // For export macro
#include "vtkIdTypeArray.h"       //
#include "vtkPolyDataAlgorithm.h"

#define VTK_EXTRACT_POINT_SEEDED_REGIONS 1
#define VTK_EXTRACT_CELL_SEEDED_REGIONS 2
#define VTK_EXTRACT_SPECIFIED_REGIONS 3
#define VTK_EXTRACT_LARGEST_REGION 4
#define VTK_EXTRACT_ALL_REGIONS 5
#define VTK_EXTRACT_CLOSEST_POINT_REGION 6
#define VTK_EXTRACT_LARGE_REGIONS 7

VTK_ABI_NAMESPACE_BEGIN
class vtkDataArray;
class vtkCharArray;
class vtkIdList;
class vtkIdTypeArray;
class vtkEdgeTable;

class VTKFILTERSCORE_EXPORT vtkPolyDataEdgeConnectivityFilter : public vtkPolyDataAlgorithm
{
public:
  ///@{
  /**
   * Standard methods to instantiate, get type information, and print the object.
   */
  static vtkPolyDataEdgeConnectivityFilter* New();
  vtkTypeMacro(vtkPolyDataEdgeConnectivityFilter, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  ///@{
  /**
   * Control the extraction of connected surfaces.
   */
  vtkSetClampMacro(
    ExtractionMode, int, VTK_EXTRACT_POINT_SEEDED_REGIONS, VTK_EXTRACT_LARGE_REGIONS);
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
  void SetExtractionModeToLargeRegions() { this->SetExtractionMode(VTK_EXTRACT_LARGE_REGIONS); }
  void SetExtractionModeToAllRegions() { this->SetExtractionMode(VTK_EXTRACT_ALL_REGIONS); }
  const char* GetExtractionModeAsString();
  ///@}

  ///@{
  /**
   * Control connectivity traversal based on barrier edges. If enabled, then
   * either the length of edges, or a explicit specification of barrier
   * edges, is used to control what are considered connected edge neighbors.
   */
  vtkSetMacro(BarrierEdges, vtkTypeBool);
  vtkGetMacro(BarrierEdges, vtkTypeBool);
  vtkBooleanMacro(BarrierEdges, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Specify the source vtkPolyData object used to specify barrier edges
   * (this is an optional connection.) If specified, the connected traversal
   * cannot traverse across the edges indicated as they are defined as
   * barrier edges. Also note that the data member BarrierEdges must be
   * enabled.
   */
  void SetSourceData(vtkPolyData*);
  void SetSourceConnection(vtkAlgorithmOutput* algOutput);
  vtkPolyData* GetSource();
  ///@}

  ///@{
  /**
   * Edges E of length edgeLen (BarrierEdgeLength[0]<=edgeLen<=BarrierEdgeLength[1])
   * define barrier edges. If edgeLen falls within this range, then polygon
   * cells on either side of the edge E are not neighbors, since the edge is
   * a barrier edge. Note that a range of [VTK_DOUBLE_MAX,VTK_DOUBLE_MAX]
   * (which is the default range) implies that all edges are not barrier
   * edges (based on edge length).
   */
  vtkSetVector2Macro(BarrierEdgeLength, double);
  vtkGetVector2Macro(BarrierEdgeLength, double);
  ///@}

  ///@{
  /**
   * Turn on/off connectivity based on scalar value. If on, cells are
   * connected only if they share a non-barrier edge AND and cell's scalar
   * value falls within the scalar range specified.
   */
  vtkSetMacro(ScalarConnectivity, vtkTypeBool);
  vtkGetMacro(ScalarConnectivity, vtkTypeBool);
  vtkBooleanMacro(ScalarConnectivity, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Set the scalar range to extract cells based on scalar connectivity.
   */
  vtkSetVector2Macro(ScalarRange, double);
  vtkGetVector2Macro(ScalarRange, double);
  ///@}

  ///@{
  /**
   * Obtain the array containing the region sizes of the extracted
   * regions.
   */
  vtkGetObjectMacro(RegionSizes, vtkIdTypeArray);
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

  /**
   * Get number of specified regions.
   */
  int GetNumberOfSpecifiedRegions();

  ///@{
  /**
   * Use to specify x-y-z point coordinates when extracting the region
   * closest to a specified point.
   */
  vtkSetVector3Macro(ClosestPoint, double);
  vtkGetVectorMacro(ClosestPoint, double, 3);
  ///@}

  // Control the region growing process.
  enum RegionGrowingType
  {
    RegionGrowingOff = 0,
    LargeRegions = 1,
    SmallRegions = 2
  };

  ///@{
  /**
   * Specify a strategy for region growing. Regions growing is a
   * postprocessing step which assimilates small regions into larger regions;
   * i.e., region growing is an additional step as part of a segmentation
   * workflow. By default, region growing is off. If growing large regions
   * is enabled, then smaller regions are assimilated into larger regions. If
   * growing small regions is enabled, then small regions are combined to
   * form larger regions. Note that the definition of a large region is a
   * region that exceeds the large region threshold.
   */
  vtkSetClampMacro(RegionGrowing, int, RegionGrowingOff, SmallRegions);
  vtkGetMacro(RegionGrowing, int);
  void SetRegionGrowingOff() { this->SetRegionGrowing(RegionGrowingOff); }
  void GrowLargeRegionsOff() { this->SetRegionGrowing(RegionGrowingOff); }
  void GrowSmallRegionsOff() { this->SetRegionGrowing(RegionGrowingOff); }
  void SetRegionGrowingToLargeRegions() { this->SetRegionGrowing(LargeRegions); }
  void GrowLargeRegionsOn() { this->SetRegionGrowing(LargeRegions); }
  void SetRegionGrowingToSmallRegions() { this->SetRegionGrowing(SmallRegions); }
  void GrowSmallRegionsOn() { this->SetRegionGrowing(SmallRegions); }
  ///@}

  ///@{
  /**
   * Define what a large region is by specifying the fraction of total input
   * mesh area a region must be in order to be considered large. So for
   * example, if the LargeRegionThreshold is 0.10, then if the summed surface
   * area of all the cells composing a region is greater than or equal to
   * 10%, the region is considered large. By default, the LargeRegionThreshold
   * is 0.05.
   */
  vtkSetClampMacro(LargeRegionThreshold, double, 0.0, 1.0);
  vtkGetMacro(LargeRegionThreshold, double);
  ///@}

  /**
   * Obtain the number of connected regions found. This returns valid
   * information only after the filter has successfully executed.
   */
  int GetNumberOfExtractedRegions() { return this->NumberOfExtractedRegions; }

  /**
   * Obtain the total area of all regions combined.
   */
  double GetTotalArea() { return this->TotalArea; }

  ///@{
  /**
   * Turn on/off the coloring of edge-connected regions. If enabled, then
   * a array named "RegionId" is added to the output cell data. The array
   * contains, for each cell, the id with which the cell is associated.
   */
  vtkSetMacro(ColorRegions, vtkTypeBool);
  vtkGetMacro(ColorRegions, vtkTypeBool);
  vtkBooleanMacro(ColorRegions, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Turn on/off the creation of a cell array that, for each cell, contains
   * the area of the region to which the cell is associated. If enabled, then
   * an array named "CellRegionArea" is added to the output cell data.
   */
  vtkSetMacro(CellRegionAreas, vtkTypeBool);
  vtkGetMacro(CellRegionAreas, vtkTypeBool);
  vtkBooleanMacro(CellRegionAreas, vtkTypeBool);
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
  vtkPolyDataEdgeConnectivityFilter();
  ~vtkPolyDataEdgeConnectivityFilter() override;

  // Usual data generation method
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  // Optional second input
  int FillInputPortInformation(int, vtkInformation*) override;

  // Filter data members
  vtkTypeBool ColorRegions;    // boolean turns on/off scalar generation for separate regions
  vtkTypeBool CellRegionAreas; // for each cell, the area of the region the cell is associated with

  int ExtractionMode;          // how to extract regions
  vtkTypeBool BarrierEdges;    // enable barrier edges
  double BarrierEdgeLength[2]; // edges of length within this range are barrier edges
  vtkTypeBool ScalarConnectivity;
  double ScalarRange[2];
  std::vector<vtkIdType> Seeds;                // id's of points or cells used to seed regions
  std::vector<vtkIdType> SpecifiedRegionIds;   // regions specified for extraction
  vtkSmartPointer<vtkIdTypeArray> RegionSizes; // size (in cells) of each region extracted
  double ClosestPoint[3];
  int OutputPointsPrecision;

  // Methods for iterative traversal and marking cells
  void TraverseAndMark();
  void GetConnectedNeighbors(
    vtkIdType cellId, vtkIdType npts, const vtkIdType* pts, vtkIdList* neis);
  int IsScalarConnected(vtkIdType cellId, vtkIdType neiId);
  bool IsBarrierEdge(vtkIdType p0, vtkIdType p1);

  // Methods implementing iterative region growing
  int RegionGrowing;
  double LargeRegionThreshold;
  int CurrentGrowPass; // region growing is a multiple-pass process
  double ComputeRegionAreas();
  void ExchangeRegions(vtkIdType currentRegionId, vtkIdType neiId, vtkIdType neiRegId);
  void GrowLargeRegions();
  void GrowSmallRegions();
  int AssimilateCell(vtkIdType cellId, vtkIdType npts, const vtkIdType* pts);
  void SortRegionsByArea();
  vtkIdType FindNumberOfExtractedRegions();

  double TotalArea;                       // the total area of the input mesh
  std::vector<double> CellAreas;          // the area of each polygonal cell
  std::vector<double> RegionAreas;        // the total area of each region
  std::vector<char> RegionClassification; // indicate whether the region is large or small

  // used to support algorithm execution
  std::vector<vtkIdType> RegionIds;
  std::vector<vtkIdType> PointMap;
  vtkIdType NumberOfRegions;
  vtkIdType NumberOfExtractedRegions;
  vtkIdType NumberOfPoints;
  vtkIdType NumCellsInRegion;
  vtkSmartPointer<vtkDataArray> InScalars;
  vtkSmartPointer<vtkPolyData> Mesh;
  vtkSmartPointer<vtkEdgeTable> Barriers;
  std::vector<vtkIdType> Wave;
  std::vector<vtkIdType> Wave2;
  vtkSmartPointer<vtkIdList> PointIds;
  vtkSmartPointer<vtkIdList> CellIds;
  vtkSmartPointer<vtkIdList> CellNeighbors;
  vtkSmartPointer<vtkIdList> CellEdgeNeighbors;
  double BRange2[2]; // BarrierEdgeLength[0,1]**2 of edge lengths defining barriers

private:
  vtkPolyDataEdgeConnectivityFilter(const vtkPolyDataEdgeConnectivityFilter&) = delete;
  void operator=(const vtkPolyDataEdgeConnectivityFilter&) = delete;
};

/**
 * Return the method of extraction as a string.
 */
inline const char* vtkPolyDataEdgeConnectivityFilter::GetExtractionModeAsString()
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
  else if (this->ExtractionMode == VTK_EXTRACT_LARGE_REGIONS)
  {
    return "ExtractLargeRegions";
  }
  else
  {
    return "ExtractLargestRegion";
  }
}

VTK_ABI_NAMESPACE_END
#endif
