/*=========================================================================

  Program:   Visualization Library
  Module:    ConnectF.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlConnectivityFilter - extract geometry based on geometric connectivity
// .SECTION Description
// vlConnectivityFilter is a filter that extracts cells that share common 
// points. The filter works in one of four ways: 1) extract the largest
// connected region in the dataset, 2) extract specified region numbers,
// 3) extract all regions sharing specified point ids, and 4) extract
// all regions sharing specified cell ids.

#ifndef __vlConnectivityFilter_h
#define __vlConnectivityFilter_h

#include "DS2UGrid.hh"

#define EXTRACT_POINT_SEEDED_REGIONS 1
#define EXTRACT_CELL_SEEDED_REGIONS 2
#define EXTRACT_SPECIFIED_REGIONS 3
#define EXTRACT_LARGEST_REGION 4

class vlConnectivityFilter : public vlDataSetToUnstructuredGridFilter
{
public:
  vlConnectivityFilter();
  ~vlConnectivityFilter() {};
  char *GetClassName() {return "vlConnectivityFilter";};
  void PrintSelf(ostream& os, vlIndent indent);

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
  // Extraction algorithm works recursively. In some systems the stack depth
  // is limited. This methods specifies the maximum recursion depth.
  vlSetClampMacro(MaxRecursionDepth,int,10,LARGE_INTEGER);
  vlGetMacro(MaxRecursionDepth,int);

  // Description:
  // Turn on/off the coloring of connected regions.
  vlSetMacro(ColorRegions,int);
  vlGetMacro(ColorRegions,int);
  vlBooleanMacro(ColorRegions,int);

protected:
  // Usual data generation method
  void Execute();

  int ColorRegions; //boolean turns on/off scalar generation for separate regions
  int ExtractionMode; //how to extract regions
  vlIdList Seeds; //id's of points or cells used to seed regions
  int MaxRecursionDepth; //prevent excessive recursion
  vlIdList SpecifiedRegionIds; //regions specified for extraction
  vlIntArray RegionSizes; //size (in cells) of each region extracted

  void TraverseAndMark(int cellId);
};

#endif


