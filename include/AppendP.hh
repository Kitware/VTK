/*=========================================================================

  Program:   Visualization Toolkit
  Module:    AppendP.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkAppendPolyData - appends one or more polygonal datasets together
// .SECTION Description
// vtkAppendPolyData is a filter that appends one of more polygonal datasets
// into a single polygonal dataset. All geometry is extracted and appended, 
// but point attributes (i.e., scalars, vectors, normals) are extracted 
// and appended only if all datasets have the point attributes available.
// (For example, if one dataset has scalars but another does not, scalars 
// will not be appended.)

#ifndef __vtkAppendPolyData_h
#define __vtkAppendPolyData_h

#include "PolyData.hh"
#include "Filter.hh"
#include "PolyDatC.hh"

class vtkAppendPolyData : public vtkPolyData, public vtkFilter
{
public:
  vtkAppendPolyData();
  ~vtkAppendPolyData();
  char *GetClassName() {return "vtkAppendPolyData";};
  void PrintSelf(ostream& os, vtkIndent indent);

  void AddInput(vtkPolyData *);
  void AddInput(vtkPolyData& in) {this->AddInput(&in);};
  void RemoveInput(vtkPolyData *);
  void RemoveInput(vtkPolyData& in) {this->RemoveInput(&in);};
  vtkPolyDataCollection *GetInput() {return &(this->InputList);};

  // filter interface
  void Update();

protected:
  // Usual data generation method
  void Execute();

  // list of data sets to append together
  vtkPolyDataCollection InputList;

  //Filter interface
  int GetDataReleased();
  void SetDataReleased(int flag);
};

#endif


