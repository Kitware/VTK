/*=========================================================================

  Program:   Visualization Toolkit
  Module:    AppendF.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkAppendFilter - appends one or more datasets together into a single unstructured grid
// .SECTION Description
// vtkAppendFilter is a filter that appends one of more datasets into a single
// unstructured grid. All geometry is extracted and appended, but point 
// attributes (i.e., scalars, vectors, normals) are extracted and appended
// only if all datasets have the point attributes available. (For example, 
// if one dataset has scalars but another does not, scalars will not be 
// appended.)

#ifndef __vtkAppendFilter_h
#define __vtkAppendFilter_h

#include "UGrid.hh"
#include "Filter.hh"
#include "DataSetC.hh"

class vtkAppendFilter : public vtkUnstructuredGrid, public vtkFilter
{
public:
  vtkAppendFilter();
  ~vtkAppendFilter();
  char *GetClassName() {return "vtkAppendFilter";};
  void PrintSelf(ostream& os, vtkIndent indent);

  void AddInput(vtkDataSet *in);
  void AddInput(vtkDataSet& in) {this->AddInput(&in);};
  void RemoveInput(vtkDataSet *in);
  void RemoveInput(vtkDataSet& in) {this->RemoveInput(&in);};
  vtkDataSetCollection *GetInput() {return &(this->InputList);};

  // filter interface
  void Update();

protected:
  // Usual data generation method
  void Execute();
  // list of data sets to append together
  vtkDataSetCollection InputList;
  //Filter interface
  int GetDataReleased();
  void SetDataReleased(int flag);
};

#endif


