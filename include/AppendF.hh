/*=========================================================================

  Program:   Visualization Library
  Module:    AppendF.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlAppendFilter - appends one or more datasets together into a single unstructured grid
// .SECTION Description
// vlAppendFilter is a filter that appends one of more datasets into a single
// unstructured grid. All geometry is extracted and appended, but point 
// attributes (i.e., scalars, vectors, normals) are extracted and appended
// only if all datasets have the point attributes available. (For example, 
// if one dataset has scalars but another does not, scalars will not be 
// appended.)

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

  void AddInput(vlDataSet *in);
  void AddInput(vlDataSet& in) {this->AddInput(&in);};
  void RemoveInput(vlDataSet *in);
  void RemoveInput(vlDataSet& in) {this->RemoveInput(&in);};
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


