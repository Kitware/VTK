/*=========================================================================

  Program:   Visualization Library
  Module:    DataSetF.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
//
// DataSetFilter takes general DataSet as input
//
#ifndef __vlDataSetFilter_h
#define __vlDataSetFilter_h

#include "Filter.hh"
#include "DataSet.hh"

class vlDataSetFilter : public vlFilter 
{
public:
  vlDataSetFilter();
  ~vlDataSetFilter();
  char *GetClassName() {return "vlDataSetFilter";};
  void PrintSelf(ostream& os, vlIndent indent);

  void Update();
  vlSetObjectMacro(Input,vlDataSet);
  vlGetObjectMacro(Input,vlDataSet);

protected:
  vlDataSet *Input;

};

#endif


