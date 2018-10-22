/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkConnectivityFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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
 * vtkPolyDataConnectivityFilter
*/

#ifndef vtkConnectivityFilter_h
#define vtkConnectivityFilter_h

#include "vtkFiltersCoreModule.h" // For export macro
#include "vtkPointSetAlgorithm.h"

#define VTK_EXTRACT_POINT_SEEDED_REGIONS 1
#define VTK_EXTRACT_CELL_SEEDED_REGIONS 2
#define VTK_EXTRACT_SPECIFIED_REGIONS 3
#define VTK_EXTRACT_LARGEST_REGION 4
#define VTK_EXTRACT_ALL_REGIONS 5
#define VTK_EXTRACT_CLOSEST_POINT_REGION 6

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
  vtkTypeMacro(vtkConnectivityFilter,vtkPointSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Construct with default extraction mode to extract largest regions.
   */
  static vtkConnectivityFilter *New();

  //@{
  /**
   * Turn on/off connectivity based on scalar value. If on, cells are connected
   * only if they share points AND one of the cells scalar values falls in the
   * scalar range specified.
   */
  vtkSetMacro(ScalarConnectivity,vtkTypeBool);
  vtkGetMacro(ScalarConnectivity,vtkTypeBool);
  vtkBooleanMacro(ScalarConnectivity,vtkTypeBool);
  //@}

  //@{
  /**
   * Set the scalar range to use to extract cells based on scalar connectivity.
   */
  vtkSetVector2Macro(ScalarRange,double);
  vtkGetVector2Macro(ScalarRange,double);
  //@}

  //@{
  /**
   * Control the extraction of connected surfaces.
   */
  vtkSetClampMacro(ExtractionMode,int,
            VTK_EXTRACT_POINT_SEEDED_REGIONS,VTK_EXTRACT_CLOSEST_POINT_REGION);
  vtkGetMacro(ExtractionMode,int);
  void SetExtractionModeToPointSeededRegions()
    {this->SetExtractionMode(VTK_EXTRACT_POINT_SEEDED_REGIONS);};
  void SetExtractionModeToCellSeededRegions()
    {this->SetExtractionMode(VTK_EXTRACT_CELL_SEEDED_REGIONS);};
  void SetExtractionModeToLargestRegion()
    {this->SetExtractionMode(VTK_EXTRACT_LARGEST_REGION);};
  void SetExtractionModeToSpecifiedRegions()
    {this->SetExtractionMode(VTK_EXTRACT_SPECIFIED_REGIONS);};
  void SetExtractionModeToClosestPointRegion()
    {this->SetExtractionMode(VTK_EXTRACT_CLOSEST_POINT_REGION);};
  void SetExtractionModeToAllRegions()
    {this->SetExtractionMode(VTK_EXTRACT_ALL_REGIONS);};
  const char *GetExtractionModeAsString();
  //@}

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

  //@{
  /**
   * Use to specify x-y-z point coordinates when extracting the region
   * closest to a specified point.
   */
  vtkSetVector3Macro(ClosestPoint,double);
  vtkGetVectorMacro(ClosestPoint,double,3);
  //@}

  /**
   * Obtain the number of connected regions.
   */
  int GetNumberOfExtractedRegions();

  //@{
  /**
   * Turn on/off the coloring of connected regions.
   */
  vtkSetMacro(ColorRegions,vtkTypeBool);
  vtkGetMacro(ColorRegions,vtkTypeBool);
  vtkBooleanMacro(ColorRegions,vtkTypeBool);
  //@}

  /**
   * Enumeration of the various ways to assign RegionIds when
   * the ColorRegions option is on.
   */
  enum RegionIdAssignment {
    UNSPECIFIED,
    CELL_COUNT_DESCENDING,
    CELL_COUNT_ASCENDING
  };

  //@{
  /**
   * Set/get mode controlling how RegionIds are assigned.
   */
  //@}
  vtkSetMacro(RegionIdAssignmentMode, int);
  vtkGetMacro(RegionIdAssignmentMode, int);

  //@{
  /**
   * Set/get the desired precision for the output types. See the documentation
   * for the vtkAlgorithm::DesiredOutputPrecision enum for an explanation of
   * the available precision settings.
   */
  vtkSetMacro(OutputPointsPrecision,int);
  vtkGetMacro(OutputPointsPrecision,int);
  //@}

  int ProcessRequest(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

protected:
  vtkConnectivityFilter();
  ~vtkConnectivityFilter() override;

  // Usual data generation method
  int RequestDataObject(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;
  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *) override;
  int FillInputPortInformation(int port, vtkInformation *info) override;
  int FillOutputPortInformation(int vtkNotUsed(port), vtkInformation* info) override;

  vtkTypeBool ColorRegions; //boolean turns on/off scalar gen for separate regions
  int ExtractionMode; //how to extract regions
  int OutputPointsPrecision;
  vtkIdList *Seeds; //id's of points or cells used to seed regions
  vtkIdList *SpecifiedRegionIds; //regions specified for extraction
  vtkIdTypeArray *RegionSizes; //size (in cells) of each region extracted

  double ClosestPoint[3];

  vtkTypeBool ScalarConnectivity;
  double ScalarRange[2];

  int RegionIdAssignmentMode;

  void TraverseAndMark(vtkDataSet *input);

  void OrderRegionIds(vtkIdTypeArray* pointRegionIds, vtkIdTypeArray* cellRegionIds);

private:
  // used to support algorithm execution
  vtkFloatArray *CellScalars;
  vtkIdList *NeighborCellPointIds;
  vtkIdType *Visited;
  vtkIdType *PointMap;
  vtkIdTypeArray *NewScalars;
  vtkIdTypeArray *NewCellScalars;
  vtkIdType RegionNumber;
  vtkIdType PointNumber;
  vtkIdType NumCellsInRegion;
  vtkDataArray *InScalars;
  vtkIdList *Wave;
  vtkIdList *Wave2;
  vtkIdList *PointIds;
  vtkIdList *CellIds;
private:
  vtkConnectivityFilter(const vtkConnectivityFilter&) = delete;
  void operator=(const vtkConnectivityFilter&) = delete;
};

//@{
/**
 * Return the method of extraction as a string.
 */
inline const char *vtkConnectivityFilter::GetExtractionModeAsString(void)
{
  if ( this->ExtractionMode == VTK_EXTRACT_POINT_SEEDED_REGIONS )
  {
    return "ExtractPointSeededRegions";
  }
  else if ( this->ExtractionMode == VTK_EXTRACT_CELL_SEEDED_REGIONS )
  {
    return "ExtractCellSeededRegions";
  }
  else if ( this->ExtractionMode == VTK_EXTRACT_SPECIFIED_REGIONS )
  {
    return "ExtractSpecifiedRegions";
  }
  else if ( this->ExtractionMode == VTK_EXTRACT_ALL_REGIONS )
  {
    return "ExtractAllRegions";
  }
  else if ( this->ExtractionMode == VTK_EXTRACT_CLOSEST_POINT_REGION )
  {
    return "ExtractClosestPointRegion";
  }
  else
  {
    return "ExtractLargestRegion";
  }
}
//@}

#endif
