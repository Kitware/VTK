/*=========================================================================

  Program:   Visualization Library
  Module:    PtSetF.hh
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
// PointSetFilter takes general structured datasets as input
//
#ifndef __vlPointSetFilter_h
#define __vlPointSetFilter_h

#include "Filter.hh"
#include "PointSet.hh"

class vlPointSetFilter : public vlFilter 
{
public:
  vlPointSetFilter();
  ~vlPointSetFilter();
  char *GetClassName() {return "vlPointSetFilter";};
  void PrintSelf(ostream& os, vlIndent indent);

  void Update();
  vlSetObjectMacro(Input,vlPointSet);
  vlGetObjectMacro(Input,vlPointSet);

protected:
  vlPointSet *Input;

};

#endif


