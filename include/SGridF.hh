/*=========================================================================

  Program:   Visualization Library
  Module:    SGridF.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlStructuredDataFilter - filter that takes vlStructuredData as input
// .SECTION Description
// vlStructuredDataFilter is a filter that takes a single 
// vlStructuredData data object as input.

#ifndef __vlStructuredDataFilter_h
#define __vlStructuredDataFilter_h

#include "Filter.hh"
#include "StrData.hh"

class vlStructuredDataFilter : public vlFilter 
{
public:
  vlStructuredDataFilter();
  ~vlStructuredDataFilter();
  char *GetClassName() {return "vlStructuredDataFilter";};
  void PrintSelf(ostream& os, vlIndent indent);

  void Update();

  // Description:
  // Specify the input object.
  vlSetObjectMacro(Input,vlStructuredData);
  vlGetObjectMacro(Input,vlStructuredData);

protected:
  vlStructuredData *Input;

};

#endif


