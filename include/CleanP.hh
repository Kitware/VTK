/*=========================================================================

  Program:   Visualization Toolkit
  Module:    CleanP.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkCleanPolyData - merge duplicate points and remove degenerate primitives
// .SECTION Description
// vtkCleanPolyData is a filter that takes polygonal data as input and 
// generates polygonal as output. vtkCleanPolyData merges duplicate 
// points (within specified tolerance) and transforms degenerate 
// topology into appropriate form (for example, triangle is converted
// into line if two points of triangle are merged).
//    If tolerance is specified precisely=0.0, then this object will use
// the vtkMergePoints object to merge points (very fast). Otherwise the 
// slower vtkLocator is used.
// .SECTION Caveats
// Merging points can alter topology including introducing non-manifold 
// forms. Tolerance should be chosen carefully to avoid these problems.

#ifndef __vtkCleanPolyData_h
#define __vtkCleanPolyData_h

#include "P2PF.hh"

class vtkCleanPolyData : public vtkPolyToPolyFilter
{
public:
  vtkCleanPolyData();
  ~vtkCleanPolyData();
  char *GetClassName() {return "vtkCleanPolyData";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify tolerance in terms of percentage of bounding box.
  vtkSetClampMacro(Tolerance,float,0.0,1.0);
  vtkGetMacro(Tolerance,float);

  void SetLocator(vtkLocator *locator);
  void SetLocator(vtkLocator& locator) {this->SetLocator(&locator);};
  vtkGetObjectMacro(Locator,vtkLocator);

  // Description:
  // Create default locator. Used to create one when none is specified.
  void CreateDefaultLocator();

protected:
  // Usual data generation method
  void Execute();

  float Tolerance;
  vtkLocator *Locator;
  int SelfCreatedLocator;
};

#endif


