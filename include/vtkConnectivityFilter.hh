/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkConnectivityFilter.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1996 Ken Martin, Will Schroeder, Bill Lorensen.

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

#ifndef __vtkConnectivityFilter_h
#define __vtkConnectivityFilter_h

#include "vtkDataSetToUnstructuredGridFilter.hh"

#define VTK_EXTRACT_POINT_SEEDED_REGIONS 1
#define VTK_EXTRACT_CELL_SEEDED_REGIONS 2
#define VTK_EXTRACT_SPECIFIED_REGIONS 3
#define VTK_EXTRACT_LARGEST_REGION 4

class vtkConnectivityFilter : public vtkDataSetToUnstructuredGridFilter
{
public:
  vtkConnectivityFilter();
  char *GetClassName() {return "vtkConnectivityFilter";};
  void PrintSelf(ostream& os, vtkIndent indent);

  void ExtractPointSeededRegions();
  void ExtractCellSeededRegions();

  void ExtractLargestRegion();

  void ExtractSpecifiedRegions();
  void InitializeSpecifiedRegionList();
  void AddSpecifiedRegion(int id);
  void DeleteSpecifiedRegion(int id);

  int GetNumberOfExtractedRegions();

  void InitializeSeedList();
  void AddSeed(int id);
  void DeleteSeed(int id);

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
  vtkIntArray RegionSizes; //size (in cells) of each region extracted

  void TraverseAndMark(int cellId);
};

#endif


