/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkConnectivityFilter.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
// .NAME vtkConnectivityFilter - extract data based on geometric connectivity
// .SECTION Description
// vtkConnectivityFilter is a filter that extracts cells that share common 
// points. The filter works in one of four ways: 1) extract the largest
// connected region in the dataset; 2) extract specified region numbers;
// 3) extract all regions sharing specified point ids; or 4) extract
// all regions sharing specified cell ids.
//
// vtkConnectivityFilter is generalized to handle any type of input dataset.
// It generates output data of type vtkUnstructuredGrid. If you know that your
// input type is vtkPolyData, you may wish to use vtkPolyDataConnectivityFilter.
//
// The behavior of vtkConnectivityFilter can be modified by turning on the 
// boolean ivar ScalarConnectivity. If this flag is on, the connectivity
// algorithm is modified so that cells are considered connected only if 1) they 
// are geometrically connected (share a vertex) and 2) the scalar values of one
// of the cell's points falls in the scalar range specified. This use of
// ScalarConnectivity is particularly useful for volume datasets: it can be used
// as a simple "connected segmentation" algorithm. For example, by using a seed
// voxel (i.e., cell) on a known anatomical structure, connectivity will pull
// out all voxels "containing" the anatomical structure. These voxels can then
// be contoured or processed by other visualization filters.

// .SECTION See Also
// vtkPolyDataConnectivityFilter

#ifndef __vtkConnectivityFilter_h
#define __vtkConnectivityFilter_h

#include "vtkDataSetToUnstructuredGridFilter.h"

#define VTK_EXTRACT_POINT_SEEDED_REGIONS 1
#define VTK_EXTRACT_CELL_SEEDED_REGIONS 2
#define VTK_EXTRACT_SPECIFIED_REGIONS 3
#define VTK_EXTRACT_LARGEST_REGION 4

class VTK_EXPORT vtkConnectivityFilter : public vtkDataSetToUnstructuredGridFilter
{
public:
  vtkConnectivityFilter();
  ~vtkConnectivityFilter();
  static vtkConnectivityFilter *New() {return new vtkConnectivityFilter;};
  const char *GetClassName() {return "vtkConnectivityFilter";};
  void PrintSelf(ostream& os, vtkIndent indent);

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
                  VTK_EXTRACT_POINT_SEEDED_REGIONS,VTK_EXTRACT_LARGEST_REGION);
  vtkGetMacro(ExtractionMode,int);
  void SetExtractionModeToPointSeededRegions()
    {this->SetExtractionMode(VTK_EXTRACT_POINT_SEEDED_REGIONS);};
  void SetExtractionModeToCellSeededRegions()
    {this->SetExtractionMode(VTK_EXTRACT_CELL_SEEDED_REGIONS);};
  void SetExtractionModeToLargestRegion()
    {this->SetExtractionMode(VTK_EXTRACT_LARGEST_REGION);};
  void SetExtractionModeToSpecifiedRegions()
    {this->SetExtractionMode(VTK_EXTRACT_SPECIFIED_REGIONS);};
  char *GetExtractionModeAsString();

  // Use with point or cell seeded extraction methods
  void InitializeSeedList();
  void AddSeed(int id);
  void DeleteSeed(int id);

  // Use with extract specified regions 
  void InitializeSpecifiedRegionList();
  void AddSpecifiedRegion(int id);
  void DeleteSpecifiedRegion(int id);

  int GetNumberOfExtractedRegions();

  // Description:
  // The connectivity extraction algorithm works recursively. In some systems 
  // the stack depth is limited. This methods specifies the maximum recursion 
  // depth.
  vtkSetClampMacro(MaxRecursionDepth,int,10,VTK_LARGE_INTEGER);
  vtkGetMacro(MaxRecursionDepth,int);

  // Description:
  // Turn on/off the coloring of connected regions.
  vtkSetMacro(ColorRegions,int);
  vtkGetMacro(ColorRegions,int);
  vtkBooleanMacro(ColorRegions,int);

protected:
  // Usual data generation method
  void Execute();

  int ColorRegions; //boolean turns on/off scalar gen for separate regions
  int ExtractionMode; //how to extract regions
  vtkIdList Seeds; //id's of points or cells used to seed regions
  int MaxRecursionDepth; //prevent excessive recursion
  vtkIdList SpecifiedRegionIds; //regions specified for extraction
  vtkIntArray *RegionSizes; //size (in cells) of each region extracted

  int ScalarConnectivity;
  float ScalarRange[2];

  void TraverseAndMark(int cellId);

private:
  vtkFloatScalars *CellScalars;
  vtkIdList *NeighborCellPointIds;

};

// Description:
// Return the method of extraction as a string.
inline char *vtkConnectivityFilter::GetExtractionModeAsString(void)
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
  else 
    {
    return "ExtractLargestRegion";
    }
}

#endif


