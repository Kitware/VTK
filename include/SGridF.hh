/*=========================================================================

  Program:   Visualization Library
  Module:    SGridF.hh
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
// StructuredDataSetFilter takes general structured datasets as input
//
#ifndef __vlStructuredDataSetFilter_h
#define __vlStructuredDataSetFilter_h

#include "Filter.hh"
#include "StrData.hh"

class vlStructuredDataSetFilter : public vlFilter 
{
public:
  vlStructuredDataSetFilter();
  ~vlStructuredDataSetFilter();
  char *GetClassName() {return "vlStructuredDataSetFilter";};
  void PrintSelf(ostream& os, vlIndent indent);

  void Update();
  vlSetObjectMacro(Input,vlStructuredDataSet);
  vlGetObjectMacro(Input,vlStructuredDataSet);

protected:
  vlStructuredDataSet *Input;

};

#endif


