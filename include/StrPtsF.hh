/*=========================================================================

  Program:   Visualization Library
  Module:    StrPtsF.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlStructuredPointsFilter - filter that takes vlStructuredPoints as input
// .SECTION Description
// vlStructuredPointsFilter is a filter that takes a single vlStructuredPoints 
// data object as input.


#ifndef __vlStructuredPointsFilter_h
#define __vlStructuredPointsFilter_h

#include "Filter.hh"
#include "StrPts.hh"

class vlStructuredPointsFilter : public vlFilter 
{
public:
  vlStructuredPointsFilter();
  ~vlStructuredPointsFilter();
  char *GetClassName() {return "vlStructuredPointsFilter";};
  void PrintSelf(ostream& os, vlIndent indent);

  void Update();

  // Description:
  // Specify the input object.
  vlSetObjectMacro(Input,vlStructuredPoints);
  vlGetObjectMacro(Input,vlStructuredPoints);

protected:
  vlStructuredPoints *Input;

};

#endif


