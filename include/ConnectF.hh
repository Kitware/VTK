/*=========================================================================

  Program:   Visualization Library
  Module:    ConnectF.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Description:
---------------------------------------------------------------------------
This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
//
// Extracts geometry connected at common vertices.
//
#ifndef __vlConnectivityFilter_h
#define __vlConnectivityFilter_h

#include "DS2UGrid.hh"

#define EXTRACT_POINT_SEEDED_REGIONS 1
#define EXTRACT_CELL_SEEDED_REGIONS 2
#define EXTRACT_SPECIFIED_REGIONS 3
#define EXTRACT_LARGEST_REGIONS 4

class vlConnectivityFilter : public vlDataSetToUnstructuredGridFilter
{
public:
  vlConnectivityFilter();
  ~vlConnectivityFilter() {};
  char *GetClassName() {return "vlConnectivityFilter";};
  void PrintSelf(ostream& os, vlIndent indent);

  vlSetClampMacro(RecursionDepth,int,10,LARGE_INTEGER);
  vlGetMacro(RecursionDepth,int);

  vlSetMacro(ColorRegions,int);
  vlGetMacro(ColorRegions,int);
  vlBooleanMacro(ColorRegions,int);

  void InitializeSeedList();
  void AddSeed(int id);
  void DeleteSeed(int id);

  void ExtractPointSeededRegions();
  void ExtractCellSeededRegions();

  void ExtractLargestRegions(int numberOfRegions);

  void ExtractSpecifiedRegions();
  void InitializeSpecifiedRegionList();
  void AddSpecifiedRegion(int id);
  void DeleteSpecifiedRegion(int id);

  int GetNumberOfExtractedRegions();

protected:
  // Usual data generation method
  void Execute();

  int ColorRegions; //boolean turns on/off scalar generation for separate regions
  int ExtractionMode; //how to extract regions
  vlIdList Seeds; //id's of points or cells used to seed regions
  int RecursionDepth; //prevent excessive recursion
  vlIdList SpecifiedRegionIds; //regions specified for extraction
  vlIntArray RegionSizes; //size (in cells) of each region extracted
  int NumberOfRegionsToExtract;

  void TraverseAndMark(int cellId);
};

#endif


