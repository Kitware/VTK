/*=========================================================================

  Program:   Visualization Library
  Module:    DS2UGrid.hh
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
// DataSetToUnstructuredGridFilter are filters that take DataSets in 
// and generate UnstructuredGrid data as output.
//
#ifndef __vlDataSetToUnstructuredGridFilter_h
#define __vlDataSetToUnstructuredGridFilter_h

#include "DataSetF.hh"
#include "UGrid.hh"

class vlDataSetToUnstructuredGridFilter : public vlUnstructuredGrid, public vlDataSetFilter
{
public:
  void Update();
  char *GetClassName() {return "vlDataSetToUnstructuredGridFilter";};
  void PrintSelf(ostream& os, vlIndent indent);
};

#endif


