/*=========================================================================

  Program:   Visualization Library
  Module:    SGGeomF.hh
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
// Create geometry for structured data
//
#ifndef __vlStructuredGeometryFilter_h
#define __vlStructuredGeometryFilter_h

#include "SD2PolyF.hh"

class vlStructuredGeometryFilter : public vlStructuredDataSetToPolyFilter
{
public:
  vlStructuredGeometryFilter();
  ~vlStructuredGeometryFilter() {};
  char *GetClassName() {return "vlStructuredGeometryFilter";};
  void PrintSelf(ostream& os, vlIndent indent);

  void SetExtent(int iMin, int iMax, int jMin, int jMax, int kMin, int kMax);
  void SetExtent(int *extent);
  int *GetExtent() { return this->Extent;};

protected:
  void Execute();
  int Extent[6];
};

#endif


