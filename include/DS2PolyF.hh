/*=========================================================================

  Program:   Visualization Library
  Module:    DS2PolyF.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
//
// DataSetToPolyFilter are filters that take DataSets in and generate PolyData
//
#ifndef __vlDataSetToPolyFilter_h
#define __vlDataSetToPolyFilter_h

#include "DataSetF.hh"
#include "PolyData.hh"

class vlDataSetToPolyFilter : public vlDataSetFilter, public vlPolyData 
{
public:
  void Update();
  char *GetClassName() {return "vlDataSetToPolyFilter";};
  void PrintSelf(ostream& os, vlIndent indent);
};

#endif


