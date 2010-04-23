/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolyDataConnectivityFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPolyDataConnectivityFilter - extract polygonal data based on geometric connectivity
// .SECTION Description
// vtkPolyDataConnectivityFilter is a filter that extracts cells that
// share common points and/or satisfy a scalar threshold
// criterion. (Such a group of cells is called a region.) The filter
// works in one of six ways: 1) extract the largest connected region
// in the dataset; 2) extract specified region numbers; 3) extract all
// regions sharing specified point ids; 4) extract all regions sharing
// specified cell ids; 5) extract the region closest to the specified
// point; or 6) extract all regions (used to color regions).
//
// This filter is specialized for polygonal data. This means it runs a bit 
// faster and is easier to construct visualization networks that process
// polygonal data.
//
// The behavior of vtkPolyDataConnectivityFilter can be modified by turning
// on the boolean ivar ScalarConnectivity. If this flag is on, the
// connectivity algorithm is modified so that cells are considered connected
// only if 1) they are geometrically connected (share a point) and 2) the
// scalar values of one of the cell's points falls in the scalar range
// specified. This use of ScalarConnectivity is particularly useful for
// selecting cells for later processing.

// .SECTION See Also
// vtkConnectivityFilter

#ifndef __vtkPolyDataConnectivityFilter_h
#define __vtkPolyDataConnectivityFilter_h

#include "vtkPolyDataAlgorithm.h"

#define VTK_EXTRACT_POINT_SEEDED_REGIONS 1
#define VTK_EXTRACT_CELL_SEEDED_REGIONS 2
#define VTK_EXTRACT_SPECIFIED_REGIONS 3
#define VTK_EXTRACT_LARGEST_REGION 4
#define VTK_EXTRACT_ALL_REGIONS 5
#define VTK_EXTRACT_CLOSEST_POINT_REGION 6

class vtkDataArray;
class vtkIdList;
class vtkIdTypeArray;

class VTK_GRAPHICS_EXPORT vtkPolyDataConnectivityFilter : public vtkPolyDataAlgorithm
{
public:
  vtkTypeMacro(vtkPolyDataConnectivityFilter,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct with default extraction mode to extract largest regions.
  static vtkPolyDataConnectivityFilter *New();

  // Description:
  // Turn on/off connectivity based on scalar value. If on, cells are connected
  // only if they share points AND one of the cells scalar values falls in the
  // scalar range specified.
  vtkSetMacro(ScalarConnectivity,int);
  vtkGetMacro(ScalarConnectivity,int);
  vtkBooleanMacro(ScalarConnectivity,int);

  // Description:
  // Set the scalar range to use to extract cells based on scalar connectivity.
  vtkSetVector2Macro(ScalarRange,double);
  vtkGetVector2Macro(ScalarRange,double);

  // Description:
  // Control the extraction of connected surfaces.
  vtkSetClampMacro(ExtractionMode,int,
                   VTK_EXTRACT_POINT_SEEDED_REGIONS,
                   VTK_EXTRACT_CLOSEST_POINT_REGION);
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

  // Description:
  // Initialize list of point ids/cell ids used to seed regions.
  void InitializeSeedList();

  // Description:
  // Add a seed id (point or cell id). Note: ids are 0-offset.
  void AddSeed(int id);

  // Description:
  // Delete a seed id (point or cell id). Note: ids are 0-offset.
  void DeleteSeed(int id);

  // Description:
  // Initialize list of region ids to extract.
  void InitializeSpecifiedRegionList();

  // Description:
  // Add a region id to extract. Note: ids are 0-offset.
  void AddSpecifiedRegion(int id);

  // Description:
  // Delete a region id to extract. Note: ids are 0-offset.
  void DeleteSpecifiedRegion(int id);

  // Description:
  // Use to specify x-y-z point coordinates when extracting the region 
  // closest to a specified point.
  vtkSetVector3Macro(ClosestPoint,double);
  vtkGetVectorMacro(ClosestPoint,double,3);

  // Description:
  // Obtain the number of connected regions.
  int GetNumberOfExtractedRegions();

  // Description:
  // Turn on/off the coloring of connected regions.
  vtkSetMacro(ColorRegions,int);
  vtkGetMacro(ColorRegions,int);
  vtkBooleanMacro(ColorRegions,int);

protected:
  vtkPolyDataConnectivityFilter();
  ~vtkPolyDataConnectivityFilter();

  // Usual data generation method
  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

  int ColorRegions; //boolean turns on/off scalar gen for separate regions
  int ExtractionMode; //how to extract regions
  vtkIdList *Seeds; //id's of points or cells used to seed regions
  vtkIdList *SpecifiedRegionIds; //regions specified for extraction
  vtkIdTypeArray *RegionSizes; //size (in cells) of each region extracted

  double ClosestPoint[3];

  int ScalarConnectivity;
  double ScalarRange[2];

  void TraverseAndMark();

private:
  // used to support algorithm execution
  vtkDataArray *CellScalars;
  vtkIdList *NeighborCellPointIds;
  vtkIdType *Visited;
  vtkIdType *PointMap;
  vtkDataArray *NewScalars;
  vtkIdType RegionNumber;
  vtkIdType PointNumber;    
  vtkIdType NumCellsInRegion;
  vtkDataArray *InScalars;
  vtkPolyData *Mesh;
  vtkIdList *Wave;
  vtkIdList *Wave2;
  vtkIdList *PointIds;
  vtkIdList *CellIds;
private:
  vtkPolyDataConnectivityFilter(const vtkPolyDataConnectivityFilter&);  // Not implemented.
  void operator=(const vtkPolyDataConnectivityFilter&);  // Not implemented.
};

// Description:
// Return the method of extraction as a string.
inline const char *vtkPolyDataConnectivityFilter::GetExtractionModeAsString(void)
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


#endif
