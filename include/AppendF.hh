/*=========================================================================

  Program:   Visualization Library
  Module:    AppendF.hh
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
// Appends one or more datasets together into a single UnstructuredGrid.
//
#ifndef __vlAppendFilter_h
#define __vlAppendFilter_h

#include "UGrid.hh"
#include "Filter.hh"
#include "DataSetC.hh"

class vlAppendFilter : public vlUnstructuredGrid, public vlFilter
{
public:
  vlAppendFilter();
  ~vlAppendFilter();
  char *GetClassName() {return "vlAppendFilter";};
  void PrintSelf(ostream& os, vlIndent indent);

  void AddInput(vlDataSet *);
  void RemoveInput(vlDataSet *);
  vlDataSetCollection *GetInput() {return &(this->Input);};

  // filter interface
  void Update();

protected:
  // Usual data generation method
  void Execute();
  // list of data sets to append together
  vlDataSetCollection Input;
};

#endif


