/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkConnectedPointsFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkConnectedPointsFilter
 * @brief   extract / segment points based on geometric connectivity
 *
 * vtkConnectedPointsFilter is a filter that extracts and/or segments points
 * from a point cloud based on geometric distance measures (e.g., proximity,
 * normal alignments, etc.) and optional measures such as scalar range. The
 * default operation is to segment the points into "connected" regions where
 * the connection is determined by an appropriate distance measure. Each
 * region is given a region id. Optionally, the filter can output the largest
 * connected region of points; a particular region (via id specification);
 * those regions that are seeded using a list of input point ids; or the
 * region of points closest to a specified position.
 *
 * The key parameter of this filter is the radius defining a sphere around
 * each point which defines a local neighborhood: any other points in the
 * local neighborhood are assumed connected to the point. Note that the
 * radius is defined in absolute terms.
 *
 * Other parameters are used to further qualify what it means to be a
 * neigboring point. For example, scalar range and/or point normals can be
 * used to further constrain the neighborhood. Also the extraction mode
 * defines how the filter operates. By default, all regions are extracted but
 * it is possible to extract particular regions; the region closest to a seed
 * point; seeded regions; or the largest region found while processing. By
 * default, all regions are extracted.
 *
 * On output, all points are labeled with a region number. However note that
 * the number of input and output points may not be the same: if not
 * extracting all regions then the output size may be less than the input
 * size.
 *
 * @sa
 * vtkPolyDataConnectivityFilter vtkConnectivityFilter
*/

#ifndef vtkConnectedPointsFilter_h
#define vtkConnectedPointsFilter_h

#include "vtkFiltersPointsModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

// Make these consistent with the other connectivity filters
#define VTK_EXTRACT_POINT_SEEDED_REGIONS 1
#define VTK_EXTRACT_SPECIFIED_REGIONS 3
#define VTK_EXTRACT_LARGEST_REGION 4
#define VTK_EXTRACT_ALL_REGIONS 5
#define VTK_EXTRACT_CLOSEST_POINT_REGION 6

class vtkAbstractPointLocator;
class vtkDataArray;
class vtkFloatArray;
class vtkIdList;
class vtkIdTypeArray;
class vtkIntArray;

class VTKFILTERSPOINTS_EXPORT vtkConnectedPointsFilter : public vtkPolyDataAlgorithm
{
public:
  vtkTypeMacro(vtkConnectedPointsFilter,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Construct with default extraction mode to extract the largest region.
   */
  static vtkConnectedPointsFilter *New();

  //@{
  /**
   * Set / get the radius variable specifying a local sphere used to define
   * local point neighborhood.
   */
  vtkSetClampMacro(Radius,double,0.0,VTK_DOUBLE_MAX);
  vtkGetMacro(Radius,double);
  //@}

  //@{
  /**
   * Control the extraction of connected regions.
   */
  vtkSetClampMacro(ExtractionMode,int,
            VTK_EXTRACT_POINT_SEEDED_REGIONS,VTK_EXTRACT_CLOSEST_POINT_REGION);
  vtkGetMacro(ExtractionMode,int);
  void SetExtractionModeToPointSeededRegions()
    {this->SetExtractionMode(VTK_EXTRACT_POINT_SEEDED_REGIONS);};
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

  //@{
  /**
   * Use to specify x-y-z point coordinates when extracting the region
   * closest to a specified point.
   */
  vtkSetVector3Macro(ClosestPoint,double);
  vtkGetVectorMacro(ClosestPoint,double,3);
  //@}

  /**
   * Initialize list of point ids ids used to seed regions.
   */
  void InitializeSeedList();

  /**
   * Add a non-negative point seed id. Note: ids are 0-offset.
   */
  void AddSeed(vtkIdType id);

  /**
   * Delete a point seed id. Note: ids are 0-offset.
   */
  void DeleteSeed(vtkIdType id);

  /**
   * Initialize list of region ids to extract.
   */
  void InitializeSpecifiedRegionList();

  /**
   * Add a non-negative region id to extract. Note: ids are 0-offset.
   */
  void AddSpecifiedRegion(vtkIdType id);

  /**
   * Delete a region id to extract. Note: ids are 0-offset.
   */
  void DeleteSpecifiedRegion(vtkIdType id);

  //@{
  /**
   * Turn on/off connectivity based on point normal consistency. If on, and
   * point normals are defined, points are connected only if they satisfy
   * other criterion (e.g., geometric proximity, scalar connectivity, etc.)
   * AND the angle between normals is no greater than NormalAngle;
   */
  vtkSetMacro(AlignedNormals,int);
  vtkGetMacro(AlignedNormals,int);
  vtkBooleanMacro(AlignedNormals,int);
  //@}

  //@{
  /**
   * Specify a threshold for normal angles. If AlignedNormalsOn is set, then
   * points are connected if the angle between their normals is within this
   * angle threshold (expressed in degress).
   */
  vtkSetClampMacro(NormalAngle,double,0.0001, 90.0);
  vtkGetMacro(NormalAngle,double);
  //@}

  //@{
  /**
   * Turn on/off connectivity based on scalar value. If on, points are
   * connected only if they satisfy the various geometric criterion AND one
   * of the points scalar values falls in the scalar range specified.
   */
  vtkSetMacro(ScalarConnectivity,int);
  vtkGetMacro(ScalarConnectivity,int);
  vtkBooleanMacro(ScalarConnectivity,int);
  //@}

  //@{
  /**
   * Set the scalar range to use to extract points based on scalar connectivity.
   */
  vtkSetVector2Macro(ScalarRange,double);
  vtkGetVector2Macro(ScalarRange,double);
  //@}

  /**
   * Obtain the number of connected regions. The return value is valid only
   * after the filter has executed.
   */
  int GetNumberOfExtractedRegions();

  //@{
  /**
   * Specify a point locator. By default a vtkStaticPointLocator is
   * used. The locator performs efficient searches to locate points
   * around a sample point.
   */
  void SetLocator(vtkAbstractPointLocator *locator);
  vtkGetObjectMacro(Locator,vtkAbstractPointLocator);
  //@}

protected:
  vtkConnectedPointsFilter();
  ~vtkConnectedPointsFilter() override;

  // Usual data generation method
  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *) override;
  int FillInputPortInformation(int port, vtkInformation *info) override;

  // The radius defines the proximal neighborhood of points
  double Radius;

  // indicate how to extract regions
  int ExtractionMode;

  // id's of points used to seed regions
  vtkIdList *Seeds;

  //regions specified for extraction
  vtkIdList *SpecifiedRegionIds;

  // Seed with a closest point
  double ClosestPoint[3];

  // Segment based on nearly aligned normals
  int AlignedNormals;
  double NormalAngle;
  double NormalThreshold;

  // Support segmentation based on scalar connectivity
  int ScalarConnectivity;
  double ScalarRange[2];

  // accelerate searching
  vtkAbstractPointLocator *Locator;

  // Wave propagation used to segment points
  void TraverseAndMark (vtkPoints *inPts, vtkDataArray *inScalars,
                        float *normals, vtkIdType *labels);

private:
  // used to support algorithm execution
  vtkIdType CurrentRegionNumber;
  vtkIdTypeArray *RegionLabels;
  vtkIdType NumPointsInRegion;
  vtkIdTypeArray *RegionSizes;
  vtkIdList *NeighborPointIds; //avoid repetitive new/delete
  vtkIdList *Wave;
  vtkIdList *Wave2;

private:
  vtkConnectedPointsFilter(const vtkConnectedPointsFilter&) = delete;
  void operator=(const vtkConnectedPointsFilter&) = delete;
};

//@{
/**
 * Return the method of extraction as a string.
 */
inline const char *vtkConnectedPointsFilter::GetExtractionModeAsString(void)
{
  if ( this->ExtractionMode == VTK_EXTRACT_POINT_SEEDED_REGIONS )
  {
    return "ExtractPointSeededRegions";
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
