/*=========================================================================

  Program:   Visualization Library
  Module:    AppendP.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlAppendPolyData - appends one or more polygonal datasets together
// .SECTION Description
// vlAppendPolyData is a filter that appends one of more polygonal datasets
// into a single polygonal dataset. All geometry is extracted and appended, 
// but point attributes (i.e., scalars, vectors, normals) are extracted 
// and appended only if all datasets have the point attributes available.
// (For example, if one dataset has scalars but another does not, scalars 
// will not be appended.)

#ifndef __vlAppendPolyData_h
#define __vlAppendPolyData_h

#include "PolyData.hh"
#include "Filter.hh"
#include "PolyDatC.hh"

class vlAppendPolyData : public vlPolyData, public vlFilter
{
public:
  vlAppendPolyData();
  ~vlAppendPolyData();
  char *GetClassName() {return "vlAppendPolyData";};
  void PrintSelf(ostream& os, vlIndent indent);

  void AddInput(vlPolyData *);
  void RemoveInput(vlPolyData *);
  vlPolyDataCollection *GetInput() {return &(this->Input);};

  // filter interface
  void Update();

protected:
  // Usual data generation method
  void Execute();

  // list of data sets to append together
  vlPolyDataCollection Input;
};

#endif


