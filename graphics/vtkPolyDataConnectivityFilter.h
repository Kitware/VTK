/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolyDataConnectivityFilter.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

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

#include "vtkPolyDataToPolyDataFilter.h"

#define VTK_EXTRACT_POINT_SEEDED_REGIONS 1
#define VTK_EXTRACT_CELL_SEEDED_REGIONS 2
#define VTK_EXTRACT_SPECIFIED_REGIONS 3
#define VTK_EXTRACT_LARGEST_REGION 4
#define VTK_EXTRACT_ALL_REGIONS 5
#define VTK_EXTRACT_CLOSEST_POINT_REGION 6

class VTK_EXPORT vtkPolyDataConnectivityFilter : public vtkPolyDataToPolyDataFilter
{
public:
  vtkTypeMacro(vtkPolyDataConnectivityFilter,vtkPolyDataToPolyDataFilter);
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
  vtkSetVectorMacro(ScalarRange,float,2);
  vtkGetVectorMacro(ScalarRange,float,2);

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
  vtkSetVector3Macro(ClosestPoint,float);
  vtkGetVectorMacro(ClosestPoint,float,3);

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
  vtkPolyDataConnectivityFilter(const vtkPolyDataConnectivityFilter&) {};
  void operator=(const vtkPolyDataConnectivityFilter&) {};

  // Usual data generation method
  void Execute();

  int ColorRegions; //boolean turns on/off scalar gen for separate regions
  int ExtractionMode; //how to extract regions
  vtkIdList *Seeds; //id's of points or cells used to seed regions
  vtkIdList *SpecifiedRegionIds; //regions specified for extraction
  vtkIntArray *RegionSizes; //size (in cells) of each region extracted

  float ClosestPoint[3];

  int ScalarConnectivity;
  float ScalarRange[2];

  void TraverseAndMark();

private:
  // used to support algorithm execution
  vtkScalars *CellScalars;
  vtkIdList *NeighborCellPointIds;
  int *Visited;
  int *PointMap;
  vtkScalars *NewScalars;
  int RegionNumber;
  int PointNumber;    
  int NumCellsInRegion;
  vtkScalars *InScalars;
  vtkPolyData *Mesh;
  vtkIdList *Wave;
  vtkIdList *Wave2;
  vtkIdList *PointIds;
  vtkIdList *CellIds;
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


